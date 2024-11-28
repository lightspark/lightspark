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

#ifndef FRAMEWORK_RUNNER_H
#define FRAMEWORK_RUNNER_H 1

#include <cstdlib>
#include <fstream>
#include <lightspark/logger.h>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/timespec.h>

#include "framework/backends/event_loop.h"
#include "framework/backends/logger.h"
#include "framework/options.h"
#include "input/injector.h"
#include "utils/filesystem_overloads.h"

using namespace lightspark;

namespace lightspark
{
	class SystemState;
	class ParseThread;
};

struct Test;

enum TestStatus
{
	Continue,
	Finished,
};

struct TestRunner
{
	path_t rootPath;
	path_t outputPath;
	TestOptions options;
	SystemState* sys;
	ParseThread* pt;
	InputInjector injector;
	TimeSpec frameTime;
	TestRunnerEventLoop eventLoop;
	TestLog log;
	ssize_t remainingTicks;
	size_t currentTick;
	tiny_string clipboard;
	std::ifstream swfFile;

	TestRunner(const Test& test, const InputInjector& _injector, bool debug, const LOG_LEVEL& logLevel);
	~TestRunner();

	constexpr SystemState* getSys() const { return sys; }
	constexpr const TestOptions& getOptions() const { return options; }
	constexpr bool nextTickMayBeLast() const { return remainingTicks == 1; }

	void tick();
	TestStatus test();
	void compareOutput(const tiny_string& actualOutput);
};

#endif /* FRAMEWORK_RUNNER_H */
