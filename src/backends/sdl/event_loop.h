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

#ifndef BACKENDS_SDL_EVENT_LOOP_H
#define BACKENDS_SDL_EVENT_LOOP_H 1

#include "backends/event_loop.h"

union SDL_Event;

namespace lightspark
{

class DLL_PUBLIC SDLEventLoop : public EventLoop
{
private:
	// Wait indefinitely for an event.
	// Optionally returns an event, if one was received.
	Optional<LSEventStorage> waitEventImpl(SystemState* sys) override;

	// Notifies the platform event loop that an event was pushed.
	void notify() override;
	bool notified(const SDL_Event& event) const;
public:
	SDLEventLoop(ITime* time) : EventLoop(time) {}
	// Returns true if the platform supports handling timers in the
	// event loop.
	bool timersInEventLoop() const override { return true; }

	static LSEventStorage toLSEvent(SystemState* sys, const SDL_Event& event);
};

}
#endif /* BACKENDS_SDL_EVENT_LOOP_H */
