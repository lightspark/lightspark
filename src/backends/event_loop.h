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

#include "interfaces/backends/event_loop.h"
#include "interfaces/timer.h"
#include "utils/optional.h"
#include "utils/timespec.h"
#include "events.h"
#include "threading.h"
#include <list>
#include <utility>
#include <SDL.h>

namespace lightspark
{

// SDL event.
class SDLEvent : public IEvent
{
friend class SDLEventLoop;
protected:
	SDL_Event event;
public:
	SDLEvent() = default;
	SDLEvent(const SDL_Event& ev) : event(ev) {}
	// Converts a platform/application specific event into an LSEvent.
	LSEventStorage toLSEvent(SystemState* sys) const override;
	// Converts an LSEvent into a platform/application specific event.
	IEvent& fromLSEvent(const LSEvent& event) override;
	// Returns the underlying platform/application specific event.
	void* getEvent() const override { return (void*)&event; }
};

// SDL event loop.
class DLL_PUBLIC SDLEventLoop : public IEventLoop
{
private:
	Optional<TimeSpec> deadline;
	TimeSpec startTime;
public:
	SDLEventLoop(ITime* time) : IEventLoop(time) {}
	// Wait indefinitely for an event.
	// First bool returns true if we got an event, or false if an
	// error occured.
	// Second bool returns true if we've been notified of a
	// non-platform event (if supported), and false otherwise.
	std::pair<bool, bool> waitEvent(IEvent& event, SystemState* sys) override;
	// Returns true if the platform supports handling timers in the
	// event loop.
	bool timersInEventLoop() const override { return true; }
};

};
#endif /* BACKENDS_EVENT_LOOP_H */
