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

#ifndef INTERFACES_BACKENDS_EVENT_LOOP_H
#define INTERFACES_BACKENDS_EVENT_LOOP_H 1

#include "forwards/backends/event_loop.h"
#include "forwards/swf.h"
#include "forwards/threading.h"
#include "forwards/timer.h"
#include "interfaces/timer.h"
#include "events.h"
#include <cstdint>
#include <list>

namespace lightspark
{

// Platform/Application specific event.
class IEvent
{
public:
	virtual ~IEvent() {}
	// Converts a platform/application specific event into an LSEvent.
	virtual LSEvent toLSEvent() const = 0;
	// Converts an LSEvent into a platform/application specific event.
	virtual IEvent& fromLSEvent(const LSEvent& event) = 0;
	// Returns the underlying platform/application specific event.
	virtual void* getEvent() const = 0;
};

// Platform/Application specific event loop.
class IEventLoop
{
protected:
	ITime* time;
public:
	IEventLoop(ITime* _time) : time(_time) {}
	virtual ~IEventLoop() {}
	// Wait indefinitely for an event.
	// Returns true if we got an event, or false if an error occured.
	virtual bool waitEvent(IEvent& event, SystemState* sys) = 0;
	// Adds a repating tick job to the timer list.
	virtual void addTick(uint32_t tickTime, ITickJob* job) = 0;
	// Adds a single-shot tick job to the timer list.
	virtual void addWait(uint32_t waitTime, ITickJob* job) = 0;
	// Removes a tick job from the timer list, without locking.
	virtual void removeJobNoLock(ITickJob* job) = 0;
	// Removes a tick job from the timer list.
	virtual void removeJob(ITickJob* job) = 0;
	// Returns true if the platform supports handling timers in the
	// event loop.
	virtual bool timersInEventLoop() const = 0;

	ITime* getTime() const { return time; }
};
};
#endif /* INTERFACES_BACKENDS_EVENT_LOOP_H */
