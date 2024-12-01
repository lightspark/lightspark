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

#ifndef BACKENDS_EVENT_LOOP_H
#define BACKENDS_EVENT_LOOP_H 1

#include "forwards/events.h"
#include "interfaces/backends/event_loop.h"
#include "interfaces/timer.h"
#include "utils/optional.h"
#include "utils/timespec.h"
#include "threading.h"
#include <list>
#include <utility>

namespace lightspark
{

// Generic event loop.
class DLL_PUBLIC EventLoop : public IEventLoop
{
private:
	std::list<LSEventStorage> events;
	mutable Mutex eventMutex;

	// Platform specific implementation of `waitEvent()`, used when the
	// generic event queue is empty.
	virtual Optional<LSEventStorage> waitEventImpl(SystemState* sys) = 0;

	// Notifies the platform event loop that an event was pushed.
	virtual void notify() = 0;
protected:
	Optional<TimeSpec> deadline;
	TimeSpec startTime;

	Optional<LSEventStorage> popEvent();
	Optional<LSEventStorage> peekEvent() const;
public:
	EventLoop(ITime* time) : IEventLoop(time) {}
	void pushEventNoLock(const LSEvent& event);
	void pushEvent(const LSEvent& event);
	// Wait indefinitely for an event.
	// Optionally returns an event, if one was received.
	Optional<LSEventStorage> waitEvent(SystemState* sys) override;
};

};
#endif /* BACKENDS_EVENT_LOOP_H */
