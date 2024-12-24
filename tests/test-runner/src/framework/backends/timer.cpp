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

#include <lightspark/swf.h>
#include <lightspark/utils/timespec.h>

#include "framework/backends/timer.h"
#include "framework/runner.h"

using namespace lightspark;

uint64_t TestRunnerTime::getCurrentTime_ms() const
{
	return now().toMs();
}

uint64_t TestRunnerTime::getCurrentTime_us() const
{
	return now().toUs();
}

uint64_t TestRunnerTime::getCurrentTime_ns() const
{
	return now().toNs();
}

TimeSpec TestRunnerTime::now() const
{
	return runner->sys != nullptr ? runner->sys->getFakeCurrentTime() : TimeSpec();
}
