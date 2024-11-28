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

#include <lightspark/utils/optional.h>

#include "input/events.h"
#include "input/injector.h"

void InputInjector::runFrame(InjectorCallback callback)
{
	size_t i;
	bool done = false;
	for (i = eventIndex; i < events.size() && !done; ++i)
	{
		const LSEvent& event = events[i];
		if (event.has<TestRunnerNextFrameEvent>())
		{
			done = true;
			continue;
		}
		callback(event);
	}
	eventIndex = i;
}

Optional<LSEventStorage> InputInjector::popEvent()
{
	if (!hasEvents() || endOfInput())
		return nullOpt;
	return events[eventIndex++];
}

Optional<LSEventStorage> InputInjector::getCurrentEvent() const
{
	if (!hasEvents() || endOfInput())
		return nullOpt;
	return events[eventIndex];
}
