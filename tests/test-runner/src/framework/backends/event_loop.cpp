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

#include <lightspark/events.h>
#include <lightspark/swf.h>

#include "framework/backends/event_loop.h"
#include "framework/backends/timer.h"
#include "framework/runner.h"

#include "input/injector.h"

Optional<LSEventStorage> TestRunnerEventLoop::waitEventImpl(SystemState* sys)
{
	if (notified && peekEvent().hasValue())
		return popEvent();
	return runner->injector.popEvent().filter([&](const LSEvent& event)
	{
		return !event.has<TestRunnerNextFrameEvent>();
	});
}
