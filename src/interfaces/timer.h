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

#ifndef INTERFACES_TIMER_H
#define INTERFACES_TIMER_H 1

#include "forwards/threading.h"
#include <cstdint>

namespace lightspark
{

//Jobs that run on tick are supposed to be very short
//For longer jobs use ThreadPool
class ITickJob
{
friend class TimerThread;
protected:
	/*
	   Helper flag to remove a job

	   If the flag is true no more ticks will happen
	*/
	bool stopMe;
public:
	virtual void tick()=0;
	ITickJob():stopMe(false){}
	virtual ~ITickJob(){}
	// This is called after tick() for single-shot jobs (i.e. enqueued with isTick==false)
	virtual void tickFence() = 0;
};

class ITimingEvent
{
public:
	ITimingEvent(ITickJob* _job, bool _isTick, uint32_t _tickTime) 
		: job(_job),tickTime(_tickTime),isTick(_isTick) {}

	virtual ~ITimingEvent() {}
	virtual bool operator<(const ITimingEvent& other) const = 0;
	virtual bool operator>(const ITimingEvent& other) const = 0;
	virtual void addMilliseconds(int32_t ms) = 0;
	virtual bool isInTheFuture() const = 0;
	virtual bool wait(Mutex& mutex, Cond& cond) = 0;

	ITickJob* job;
	uint32_t tickTime;
	bool isTick;
};

class IChronometer
{
protected:
	uint64_t start;
public:
	IChronometer(uint64_t _start) : start(_start) {}
	virtual ~IChronometer() {}
	virtual uint32_t checkpoint() = 0;
};

};
#endif /* INTERFACEs_TIMER_H */
