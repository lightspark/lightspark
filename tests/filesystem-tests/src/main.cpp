/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)

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
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>

#include <cpptrace/cpptrace.hpp>

#include <libtest++/args.h>
#include <libtest++/test_runner.h>

#include "config.h"

using namespace lightspark;
using namespace libtestpp;

std::vector<Trial> addTests(const Arguments& args);

#ifdef SIGNAL_BACKTRACE
std::thread thr;
std::mutex m;
std::condition_variable readCond;
bool canRead = false;
std::atomic<bool> signalOccured = false;

size_t numSafeFrames = 0;
cpptrace::safe_object_frame safeFrames[128];
#endif

void initCppTrace()
{
	#ifdef SIGNAL_BACKTRACE
	auto crashHandler = [](int sigNum)
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
	#endif

	cpptrace::absorb_trace_exceptions(false);
	cpptrace::register_terminate_handler();

	// Needed here to deal with dynamic loading stuff.
	cpptrace::frame_ptr buffer[10];
	(void)cpptrace::safe_generate_raw_trace(buffer, 10);
	cpptrace::safe_object_frame frame;
	cpptrace::get_safe_object_frame(buffer[0], &frame);

	#ifdef SIGNAL_BACKTRACE
	#ifndef _WIN32
	struct sigaction segFaultAction {};
	struct sigaction abortAction {};
	segFaultAction.sa_flags = SA_ONSTACK | SA_NODEFER | SA_RESETHAND;
	segFaultAction.sa_handler = crashHandler;
	abortAction.sa_flags = SA_ONSTACK | SA_NODEFER | SA_RESETHAND;
	abortAction.sa_handler = crashHandler;

	sigaction(SIGSEGV, &segFaultAction, nullptr);
	sigaction(SIGABRT, &abortAction, nullptr);
	#else
	// Windows doesn't support `sigaction()`, so fallback to using
	// `signal()`.
	signal(SIGSEGV, crashHandler);
	signal(SIGABRT, crashHandler);
	#endif
	#endif
}

int main(int argc, char* argv[])
{
	initCppTrace();
	Arguments args(argc, argv);

	std::vector<Trial> tests = addTests(args);

	#ifdef SIGNAL_BACKTRACE
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
	#endif

	auto conclusion = libtestpp::run(args, tests);

	#ifdef SIGNAL_BACKTRACE
	if (!signalOccured)
	{
		std::lock_guard l(m);
		numSafeFrames = 0;
		canRead = true;
		readCond.notify_one();
	}
	thr.join();
	#endif

	conclusion.exit();
}
