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

#include <cstdlib>
#include <cassert>

#include "backends/event_loop.h"
#include "backends/input.h"
#include "interfaces/backends/event_loop.h"
#include "platforms/engineutils.h"
#include "compat.h"
#include "events.h"
#include "swf.h"
#include "timer.h"

using namespace lightspark;
using namespace std;

Optional<LSEventStorage> EventLoop::popEvent()
{
	Locker l(eventMutex);
	if (!events.empty())
	{
		LSEventStorage ev = events.front();
		events.pop_front();
		return ev;
	}
	return {};
}

Optional<LSEventStorage> EventLoop::peekEvent() const
{
	Locker l(eventMutex);
	if (!events.empty())
		return events.front();
	return {};
}

void EventLoop::pushEventNoLock(const LSEvent& event)
{
	events.push_back(event);
	notify();
}

void EventLoop::pushEvent(const LSEvent& event)
{
	Locker l(eventMutex);
	pushEventNoLock(event);
}

Optional<LSEventStorage> EventLoop::waitEvent(SystemState* sys)
{
	return popEvent().orElse([&]
	{
		return waitEventImpl(sys);
	});
}
