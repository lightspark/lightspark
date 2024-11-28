/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <regex>
#include <sys/wait.h>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>

#include <cpptrace/cpptrace.hpp>

#include <libtest++/test_runner.h>
#include <libtest++/args.h>

#include <lightspark/interfaces/backends/event_loop.h>
#include <lightspark/interfaces/timer.h>
#include <lightspark/logger.h>
#include <lightspark/swf.h>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>

#include "config.h"
#include "framework/test.h"
#include "framework/runner.h"
#include "utils/args_parser.h"
#include "utils/filesystem_overloads.h"


namespace fs = std::filesystem;
using namespace lightspark;
using namespace libtestpp;
using path_t = fs::path;

static TestFormat fromStr(const tiny_string& name)
{
	if (name.startsWith("lightspark"))
		return TestFormat::Lightspark;
	else if (name.startsWith("ruffle"))
		return TestFormat::Ruffle;
	else if (name.startsWith("shumway"))
		return TestFormat::Shumway;
	else if (name.startsWith("avmplus"))
		return TestFormat::AVMPlus;
	else
		return TestFormat::Unknown;
}

static TestFormat fromDirName(const tiny_string& name)
{
	if (name == "lightspark")
		return TestFormat::Lightspark;
	else if (name == "ruffle")
		return TestFormat::Ruffle;
	else if (name == "shumway")
		return TestFormat::Shumway;
	else if (name == "avmplus")
		return TestFormat::AVMPlus;
	else
		return TestFormat::Unknown;
}

static tiny_string toDirName(const TestFormat& testFormat)
{
	switch (testFormat)
	{
		case TestFormat::Lightspark: return "lightspark"; break;
		case TestFormat::Ruffle: return "ruffle"; break;
		case TestFormat::Shumway: return "shumway"; break;
		case TestFormat::AVMPlus: return "avmplus"; break;
		case TestFormat::Unknown: return ""; break;
	}
	return "";
}

static tiny_string toInfoFileName(const TestFormat& testFormat)
{
	switch (testFormat)
	{
		// Default to our own test format, if we encounter an unknown format.
		case TestFormat::Unknown:
		case TestFormat::Lightspark: return "test_info"; break;
		case TestFormat::Ruffle: return "test.toml"; break;
		// TODO: Implement this.
		case TestFormat::Shumway: return ""; break;
		// TODO: Implement this.
		case TestFormat::AVMPlus: return ""; break;
	}
	return "";
}

static path_t stripPrefix(const path_t& path, const path_t& prefix)
{
	return path.lexically_relative(prefix);
}

struct Options : public Arguments
{
private:
	ArgsParser argsParser;
public:
	// Display log output of Lightspark
	bool debug { false };
	// Log level of Lightspark
	LOG_LEVEL logLevel { LOG_INFO };

	Options() : argsParser("Lightspark test runner", VERSION) { createOptions(); }
	Options(int argc, char** argv) : argsParser("Lightspark test runner", VERSION)
	{
		createOptions();
		parse(argc, argv);
	}

	void parse(int argc, char** argv) { Arguments::parse(argsParser, argc, argv); }
private:
	void createOptions()
	{
		argsParser.addOption(debug, "Display log output of Lightspark, along with other debug logs", "debug", "d");
		argsParser.addOption(logLevel, "Log level of Lightspark", "log-level", "l", "0-4");
	}
};

// Convert the filter (e.g. from the CLI) into a test name.
//
// These two values may differ due to how
// libtest++ handles test kind annotations:
// A test may be named `foo`, or `[kind] foo` when a kind is present.
// This function removes the "kind" prefix from the name to match
// tests similarly to libtest++.
static tiny_string filterToTestName(const tiny_string& filter)
{
	// NOTE: Using a static regex here is a lot faster, due to it
	// compiling the regex once.
	static const std::regex regex("^\\[[^]+] ");
	return std::regex_replace((std::string)filter, regex, "");
}

bool isCandidate(const Arguments& args, const tiny_string& testName, const TestFormat& testFormat)
{
	if (args.filter.hasValue())
	{
		auto expectedTestName = filterToTestName(*args.filter);
		if (args.exact && testName != expectedTestName)
			return false;
		else if (!args.exact && testName.find(expectedTestName) == tiny_string::npos)
			return false;
	}

	for (auto skipFilter : args.skip)
	{
		auto skippedTestName = filterToTestName(skipFilter);
		if (args.exact && testName == skippedTestName)
			return false;
		else if (!args.exact && testName.find(skippedTestName) != tiny_string::npos)
			return false;
	}

	return true;
}

Trial runTest(const Options& options, const path_t& file, const tiny_string& name, const TestFormat& testFormat)
{
	using OutcomeType = Outcome::Type;

	auto root = file.parent_path();
	Test test(TestOptions(name, file, testFormat), root, "test.swf", name, testFormat);

	bool ignore = !test.shouldRun();
	return Trial::test(test.name, [=]
	{
		// TODO: Support non-trace based tests.
		if (test.type != Test::Type::Trace)
			return Outcome(OutcomeType::Ignored);

		Optional<tiny_string> failReason;
		auto runner = test.createTestRunner(options.debug, options.logLevel);
		if (runner.sys->isOnError())
			return Outcome(Failed { tiny_string(runner.sys->getErrorCause()) });

		while (true)
		{
			try
			{
				runner.tick();
				if (runner.test() == TestStatus::Finished)
					break;
			}
			catch (std::exception& e)
			{
				failReason = tiny_string(e.what(), true);
				break;
			}
		}

		return failReason.orElseIf(test.options.knownFailure, [&]
		{
			std::stringstream s;
			s << test.name << " was a known failure, but passes now. Please mark it as passing!";
			return tiny_string(s.str());
		}).transformOr(Outcome(OutcomeType::Passed), [](const tiny_string& reason)
		{
			return Outcome(Failed { reason });
		});
	}).with_ignored_flag(ignore);
}

Optional<Trial> lookUpTest(const path_t& root, const Options& options)
{
	auto name = filterToTestName(*options.filter);
	TestFormat testFormat = fromStr(name);
	auto absRoot = fs::canonical(root);
	auto infoFile = toInfoFileName(testFormat);
	auto dirName = toDirName(testFormat);
	auto realName = name.stripPrefix(dirName, 1);

	auto path = fs::canonical(absRoot/dirName/realName/infoFile);
	
	// Make sure that:
	//	1. There's no path traversal (e.g. `../../test`)
	//	2. The path is still exact (e.g. `foo/../foo/test`)
	if (stripPrefix(path, absRoot) != (path_t(dirName)/realName/infoFile) || !fs::is_regular_file(path))
		return nullOpt;
	return runTest
	(
		options,
		path,
		stripPrefix(path.parent_path(), absRoot/dirName).generic_string(),
		testFormat
	);
}

std::thread thr;
std::mutex m;
std::condition_variable readCond;
bool canRead = false;
std::atomic<bool> signalOccured = false;

size_t numSafeFrames = 0;
cpptrace::safe_object_frame safeFrames[128];

void initCppTrace()
{
	auto crashHandler = [](int sigNum, siginfo_t* info, void* context)
	{
		signalOccured = true;
		auto printStr = [](const char* str)
		{
			return write(STDERR_FILENO, str, strlen(str));
		};
	
		const char* sigName = "Unknown signal";
		switch (sigNum)
		{
			case SIGSEGV: sigName = "SIGSEGV"; break;
			case SIGABRT: sigName = "SIGABRT"; break;
		}

		printStr("\n");
		printStr(sigName);
		printStr(" occured:\n");

		cpptrace::frame_ptr frames[128];
		numSafeFrames = cpptrace::safe_generate_raw_trace(frames, 128);

		for (size_t i = 0; i < numSafeFrames; ++i)
			cpptrace::get_safe_object_frame(frames[i], &safeFrames[i]);

		{
			std::lock_guard l(m);
			canRead = true;
			readCond.notify_one();
		}
		thr.join();
		_exit(1);
	};

	cpptrace::absorb_trace_exceptions(false);
	cpptrace::register_terminate_handler();

	// Needed here to deal with dynamic loading stuff.
	cpptrace::frame_ptr buffer[10];
	(void)cpptrace::safe_generate_raw_trace(buffer, 10);
	cpptrace::safe_object_frame frame;
	cpptrace::get_safe_object_frame(buffer[0], &frame);

	struct sigaction segFaultAction {};
	struct sigaction abortAction {};
	segFaultAction.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER | SA_RESETHAND;
	segFaultAction.sa_sigaction = crashHandler;
	abortAction.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER | SA_RESETHAND;
	abortAction.sa_sigaction = crashHandler;

	sigaction(SIGSEGV, &segFaultAction, nullptr);
	sigaction(SIGABRT, &abortAction, nullptr);
}

int main(int argc, char* argv[])
{
	initCppTrace();
	Options options(argc, argv);

	// When this is true, we're looking for a specific test.
	// This is an important optimization, as we currently can't run
	// tests in parallel, due to Lightspark making heavy use of threads.
	bool filterExact = options.exact && options.filter.hasValue();
	path_t root = path_t(TESTS_DIR).append("swfs");
	std::vector<Trial> tests;

	if (filterExact)
	{
		tests = lookUpTest(root, options).transformOr(decltype(tests) {}, [&](const Trial& test)
		{
			options.filter = test.name();
			return std::vector<Trial> { test };
		});
	}
	else
	{
		TestFormat testFormat;
		for (auto it = fs::recursive_directory_iterator(root); it != fs::end(it); it++)
		{
			auto entry = *it;
			auto path = entry.path();
			if (entry.is_directory() && it.depth() == 0)
				testFormat = fromDirName(std::prev(path.end())->string());
			else if (entry.is_regular_file())
			{
				auto infoFile = toInfoFileName(testFormat);
				// Skip invalid tests.
				if (!infoFile.empty() && path.filename() != infoFile)
					continue;

				auto name = stripPrefix
				(
					path.parent_path(),
					root / toDirName(testFormat)
				).generic_string();

				if (!name.empty() && isCandidate(options, name, testFormat))
					tests.push_back(runTest(options, path, name, testFormat));
			}
		}
	}

	std::sort
	(
		tests.begin(),
		tests.end(),
		[](const Trial& a, const Trial& b)
		{
			return a.name() < b.name();
		}
	);

	thr = std::thread([]
	{
		cpptrace::object_trace trace;
		{
			std::unique_lock l(m);
			readCond.wait(l, [] { return canRead; });
		}

		if (!numSafeFrames)
			return;

		for (size_t i = 0; i < numSafeFrames; ++i)
			trace.frames.push_back(safeFrames[i].resolve());

		trace.resolve().print();
	});

	SystemState::staticInit();
	auto conclusion = libtestpp::run(options, tests);

	if (!signalOccured)
	{
		std::lock_guard l(m);
		numSafeFrames = 0;
		canRead = true;
		readCond.notify_one();
	}
	thr.join();

	SystemState::staticDeinit();
	conclusion.exit();
}
