/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_UTILS_TIMER_H
#define SCRIPTING_FLASH_UTILS_TIMER_H 1

#include "forwards/timer.h"
#include "interfaces/timer.h"
#include "compat.h"
#include "swftypes.h"
#include "scripting/flash/events/flashevents.h"


namespace lightspark
{


class Timer: public EventDispatcher, public ITickJob
{
private:
	void tick();
	void tickFence();
protected:
	bool running;
	uint32_t delay;
	uint32_t repeatCount;
	uint32_t currentCount;
public:
	Timer(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	void afterHandleEvent(Event* ev) override;
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getCurrentCount);
	ASFUNCTION_ATOM(_getRepeatCount);
	ASFUNCTION_ATOM(_setRepeatCount);
	ASFUNCTION_ATOM(_getRunning);
	ASFUNCTION_ATOM(_getDelay);
	ASFUNCTION_ATOM(_setDelay);
	ASFUNCTION_ATOM(start);
	ASFUNCTION_ATOM(reset);
	ASFUNCTION_ATOM(stop);
};


}

#endif /* SCRIPTING_FLASH_UTILS_TIMER_H */
