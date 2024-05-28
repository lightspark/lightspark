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
#include <list>

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

class ITimingEventList
{
friend class TimerThread;
private:
	std::list<ITimingEvent*> pendingEvents;
protected:
	void insertEvent(ITimingEvent* e, Mutex& mutex, Cond& newEvent);
	void insertEventNoLock(ITimingEvent* e, Cond& newEvent);
public:
	const std::list<ITimingEvent*>& getPendingEvents() const { return pendingEvents; }
	ITimingEvent* getFrontEvent() const { return pendingEvents.front(); }
	void popFrontEvent() { return pendingEvents.pop_front(); }
	bool isEmpty() const { return pendingEvents.empty(); }

	void removeJob(ITickJob* job, Mutex& mutex, Cond& newEvent);
	void removeJobNoLock(ITickJob* job, Cond& newEvent);

	virtual ~ITimingEventList() {}
	virtual void addJob(uint32_t ms, ITickJob* job, bool isTick, Mutex& mutex, Cond& newEvent) = 0;
};

class ITime
{
public:
	virtual ~ITime() {}
	/* Gets the current system time in milliseconds. */
	virtual uint64_t getCurrentTime_ms() const = 0;
	/* Gets the current system time in microseconds. */
	virtual uint64_t getCurrentTime_us() const = 0;
	/* Gets the current system time in nanoseconds. */
	virtual uint64_t getCurrentTime_ns() const = 0;
	/* Sleep for `ms` milliseconds. */
	virtual void sleep_ms(uint32_t ms) = 0;
	/* Sleep for `us` microseconds. */
	virtual void sleep_us(uint32_t us) = 0;
	/* Sleep for `ns` nanoseconds. */
	virtual void sleep_ns(uint64_t ns) = 0;
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
#endif /* INTERFACES_TIMER_H */
