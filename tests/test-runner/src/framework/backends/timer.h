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

#ifndef FRAMEWORK_BACKENDS_TIMER_H
#define FRAMEWORK_BACKENDS_TIMER_H 1

#include <cstdint>
#include <lightspark/timer.h>

using namespace lightspark;
namespace lightspark
{
	class TimeSpec;
};

struct TestRunner;

class TestRunnerTime : public Time
{
private:
	TestRunner* runner;
public:
	TestRunnerTime() : runner(nullptr) {}
	TestRunnerTime(TestRunner* _runner) : runner(_runner) {}

	uint64_t getCurrentTime_ms() const override;
	uint64_t getCurrentTime_us() const override;
	uint64_t getCurrentTime_ns() const override;
	TimeSpec now() const override;
};

#endif /* FRAMEWORK_BACKENDS_TIMER_H */
