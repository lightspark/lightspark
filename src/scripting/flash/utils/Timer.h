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

#include "compat.h"
#include "swftypes.h"
#include "scripting/flash/events/flashevents.h"
#include "thread_pool.h"
#include "timer.h"


namespace lightspark
{


class Timer: public EventDispatcher, public ITickJob
{
private:
	void tick();
	void tickFence();
	//tickJobInstance keeps a reference to self while this
	//instance is being used by the timer thread.
	_NR<Timer> tickJobInstance;
protected:
	bool running;
	uint32_t delay;
	uint32_t repeatCount;
	uint32_t currentCount;
public:
	Timer(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c),running(false),delay(0),repeatCount(0),currentCount(0){}
	static void sinit(Class_base* c);
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
