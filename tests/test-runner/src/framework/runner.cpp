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

#include <exception>
#include <iterator>

#include <dtl/dtl.hpp>

#include <lightspark/backends/security.h>
#include <lightspark/logger.h>
#include <lightspark/scripting/abc.h>
#include <lightspark/scripting/flash/display/RootMovieClip.h>
#include <lightspark/swf.h>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/visitor.h>

#include "framework/backends/engineutils.h"
#include "framework/backends/netutils.h"
#include "framework/runner.h"
#include "framework/test.h"

void assertTextMatches(const tiny_string& actual, const tiny_string& expected);

static SystemState::FLASH_MODE fromFlashMode(const FlashMode& mode)
{
	using FLASH_MODE = SystemState::FLASH_MODE;

	switch (mode)
	{
		case FlashMode::Flash: return FLASH_MODE::FLASH; break;
		case FlashMode::AIR: return FLASH_MODE::AIR; break;
		case FlashMode::AvmPlus: return FLASH_MODE::AVMPLUS; break;
	}
	return FLASH_MODE::FLASH;
}

static bool runEventLoop(TestRunnerEventLoop& eventLoop, SystemState* sys, bool sendInputEvents = true)
{
	TestRunnerEvent ev;
	bool gotEvent, notified;
	while (std::tie(gotEvent, notified) = eventLoop.waitEvent(ev, sys), gotEvent)
	{
		if (EngineData::mainloop_handleevent(notified ? EngineData::popEvent() : ev.toLSEvent(sys), sys))
			return true;
	}
	return false;
}

TestRunner::TestRunner(const Test& test, const InputInjector& _injector, bool debug, const LOG_LEVEL& logLevel) :
rootPath(test.rootPath),
outputPath(test.outputPath),
options(test.options),
sys(nullptr),
pt(nullptr),
injector(_injector),
eventLoop(TestRunnerEventLoop(this)),
remainingTicks(-1),
currentTick(0),
swfFile(test.swfPath)
{
	Log::setLogLevel(debug ? logLevel : LOG_LEVEL(-1));

	swfFile.seekg(0, std::ios::end);
	auto fileSize = swfFile.tellg();
	swfFile.seekg(0, std::ios::beg);

	EngineData::enablerendering = false;	
	sys = new SystemState
	(
		fileSize,
		fromFlashMode(options.playerOptions.flashMode),
		&eventLoop,
		nullptr,
		true
	);
	pt = new ParseThread(swfFile, sys->mainClip);

	setTLSSys(getSys());
	setTLSWorker(sys->worker);
	sys->setParamsAndEngine(new TestRunnerEngineData(rootPath, this), true);
	sys->mainClip->setOrigin(tiny_string("file://") + test.swfPath.generic_string());
	sys->securityManager->setSandboxType(SecurityManager::LOCAL_TRUSTED);
	sys->ignoreUnhandledExceptions = true;
	sys->useFastInterpreter = true;
	sys->downloadManager = new TestRunnerDownloadManager(rootPath);

	pt->execute();

	auto delta = 1.0 / options.tickRate.valueOr(sys->mainClip->getFrameRate());
	frameTime = TimeSpec::fromFloat(delta);
	remainingTicks = options.numFrames.valueOr(-1);
}

TestRunner::~TestRunner()
{
	if (!sys->isShuttingDown())
		sys->setShutdownFlag();
	sys->destroy();
	delete pt;
	delete sys;
	EngineData::clearEvents();
}

void TestRunner::tick()
{
	sys->runTick(frameTime);
	if (sys->hasError())
		throw TestRunnerException(sys->getErrorCause());

	if (remainingTicks > 0)
		remainingTicks--;
	currentTick++;
}

TestStatus TestRunner::test()
{
	bool done = runEventLoop(eventLoop, sys);
	done |= injector.hasEvents() && injector.endOfInput();

	if (!remainingTicks || done)
	{
		// End of test, check if everything went well.
		std::string trace = log.traceOutput();

		// Remove any null bytes from the trace output.
		for (auto it = trace.begin(); it != trace.end(); ++it)
		{
			if (*it == '\0')
				trace.erase(it);
		}
		compareOutput(trace);

		return TestStatus::Finished;
	}
	return TestStatus::Continue;
}

void TestRunner::compareOutput(const tiny_string& actualOutput)
{
	std::ifstream stream(outputPath);
	tiny_string expectedOutput = std::string
	(
		std::istreambuf_iterator<char> { stream },
		{}
	);

	(void)options.approximations.andThen([&](const Approximations& approx)
	{
		auto actualLineCount = actualOutput.split('\n').size();
		auto expectedLineCount = expectedOutput.split('\n').size();

		if (actualLineCount != expectedLineCount)
		{
			std::stringstream s;
			s << "Number of output lines didn't match (expected " <<
			expectedLineCount << " from Flash Player, but got " <<
			actualLineCount << " from Lightspark).";
			throw TestRunnerException(s.str());
		}
		
		auto actualLines = actualOutput.split('\n');
		auto expectedLines = expectedOutput.split('\n');

		auto it = actualLines.begin();
		auto it2 = expectedLines.begin();
		while (it != actualLines.end())
		{
			std::string actual = *it++;
			std::string expected = *it2++;
			
			try
			{
				auto actualValue = std::stod(actual);
				auto expectedValue = std::stod(expected);
				// NaNs should be allowed to pass, in an approx test.
				if (std::isnan(actualValue) && std::isnan(expectedValue))
					continue;
				approx.compare(actualValue, expectedValue);
				continue;
			}
			catch (TestRunnerException& e)
			{
				std::rethrow_exception(std::current_exception());
			}
			catch (std::exception& e)
			{
			}

			bool found = false;

			// Check all the user-provided regexes for a match.
			for (auto pattern : approx.getNumberPatterns())
			{
				std::smatch actualMatches;
				std::smatch expectedMatches;
				if
				(
					!std::regex_match(actual, actualMatches, pattern) ||
					!std::regex_match(expected, expectedMatches, pattern)
				)
					continue;

				found = true;
				if (actualMatches.size() != expectedMatches.size())
				{

					std::stringstream s;
					s << "Differing number of regex matches (expected " <<
					expectedMatches.size() << ", got" <<
					actualMatches.size() << ").";
					throw TestRunnerException(s.str());
				}

				// Each capture group (except group 0) represents a
				// float value.
				for (size_t i = 1; i < actualMatches.size(); ++i)
				{
					approx.compare
					(
						std::stod(actualMatches[i]),
						std::stod(expectedMatches[i])
					);
				}

				assertTextMatches
				(
					std::regex_replace(actual, pattern, ""),
					std::regex_replace(expected, pattern, "")
				);
				break;
			}

			if (!found)
				assertTextMatches(actual, expected);
		}

		return Optional(approx);
	}).orElse([&]
	{
		assertTextMatches(actualOutput, expectedOutput);
		return nullOpt;
	});
}

void assertTextMatches(const tiny_string& actual, const tiny_string& expected)
{
	if (actual != expected)
	{
		auto _actualLines = actual.split('\n');
		auto _expectedLines = expected.split('\n');
		std::vector<tiny_string> actualLines(_actualLines.begin(), _actualLines.end());
		std::vector<tiny_string> expectedLines(_expectedLines.begin(), _expectedLines.end());

		dtl::Diff<tiny_string> diff(expectedLines, actualLines);
		diff.compose();
		diff.composeUnifiedHunks();

		std::stringstream s;
		s << "Assertion failed: `(flashExpected == lightsparkActual)`" << std::endl << std::endl;
		diff.printUnifiedFormat(s);
		s << std::endl;
		throw TestRunnerException(s.str());
	}
}
