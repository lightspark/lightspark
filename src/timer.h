/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef TIMER_H
#define TIMER_H 1

#include "forwards/timer.h"
#include "interfaces/timer.h"
#include "compat.h"
#include <list>
#include <ctime>
#include "threading.h"

namespace lightspark
{

class SystemState;

class TimeSpec
{
private:
	uint64_t sec;
	uint64_t nsec;
public:

	static constexpr uint64_t usPerSec = 1000000;
	static constexpr uint64_t nsPerSec = 1000000000;
	static constexpr uint64_t nsPerMs = 1000000;
	static constexpr uint64_t nsPerUs = 1000;

	constexpr TimeSpec(uint64_t secs = 0, uint64_t nsecs = 0) : sec(secs), nsec(nsecs) {}

	static constexpr TimeSpec fromFloat(float secs) { return fromNs(secs * nsPerSec); }
	static constexpr TimeSpec fromFloatUs(float secs) { return fromUs(secs * usPerSec); }
	static constexpr TimeSpec fromSec(uint64_t sec) { return TimeSpec(sec, 0); }
	static constexpr TimeSpec fromMs(uint64_t ms) { return fromNs(ms * nsPerMs); }
	static constexpr TimeSpec fromUs(uint64_t us) { return fromNs(us * nsPerUs); }
	static constexpr TimeSpec fromNs(uint64_t ns) { return TimeSpec(ns / nsPerSec, ns % nsPerSec); }

	constexpr bool operator==(const TimeSpec &other) const { return sec == other.sec && nsec == other.nsec; }
	constexpr bool operator!=(const TimeSpec &other) const { return sec != other.sec || nsec != other.nsec; }
	constexpr bool operator>(const TimeSpec &other) const { return sec > other.sec || (sec == other.sec && nsec > other.nsec); }
	constexpr bool operator<(const TimeSpec &other) const { return sec < other.sec || (sec == other.sec && nsec < other.nsec); }
	constexpr bool operator>=(const TimeSpec &other) const { return sec > other.sec || (sec == other.sec && nsec >= other.nsec); }
	constexpr bool operator<=(const TimeSpec &other) const { return sec < other.sec || (sec == other.sec && nsec <= other.nsec); }

	TimeSpec operator-(const TimeSpec &other) const { return TimeSpec(sec - other.sec, nsec - other.nsec).normalize(); }
	TimeSpec operator+(const TimeSpec &other) const { return TimeSpec(sec + other.sec, nsec + other.nsec).normalize(); }

	TimeSpec &operator+=(const TimeSpec &other) { return *this = *this + other; }
	TimeSpec &operator-=(const TimeSpec &other) { return *this = *this - other; }

	TimeSpec &normalize()
	{
		auto normal_secs = ((int64_t)nsec >= 0 ? nsec : nsec - (nsPerSec-1)) / nsPerSec;
		sec += normal_secs;
		nsec -= normal_secs * nsPerSec;
		return *this;
	}

	operator struct timespec() const
	{
		struct timespec ret;
		ret.tv_sec = sec;
		ret.tv_nsec = nsec;
		return ret;
	}

	constexpr uint64_t toMs() const { return toNs() / nsPerMs; }
	constexpr uint64_t toUs() const { return toNs() / nsPerUs; }
	constexpr uint64_t toNs() const { return (sec * nsPerSec) + nsec; }

	constexpr uint64_t getSecs() const { return sec; }
	constexpr uint64_t getNsecs() const { return nsec; }
};

class Time : public ITime
{
	uint64_t getCurrentTime_ms() const override { return compat_msectiming(); }
	uint64_t getCurrentTime_us() const override { return compat_usectiming(); }
	uint64_t getCurrentTime_ns() const override { return compat_nsectiming(); }
	void sleep_ms(uint32_t ms) override { compat_msleep(ms); }
	void sleep_us(uint32_t us) override { compat_usleep(us); }
	void sleep_ns(uint64_t ns) override { compat_nsleep(ns); }
};

class TimerThread
{
private:
	class TimingEvent
	{
	public:
		TimingEvent(ITickJob* _job, bool _isTick, uint32_t _tickTime, uint32_t _waitTime) 
			: job(_job),wakeUpTime(_isTick ? _tickTime : _waitTime),tickTime(_tickTime),isTick(_isTick) {}
		ITickJob* job;
		CondTime wakeUpTime;
		uint32_t tickTime;
		bool isTick;
	};
	Mutex mutex;
	Cond newEvent;
	SDL_Thread* t;
	std::list<TimingEvent*> pendingEvents;
	SystemState* m_sys;
	volatile bool stopped;
	bool joined;
	static int worker(void* d);
	void insertNewEvent(TimingEvent* e);
	void insertNewEvent_nolock(TimingEvent* e);
	void dumpJobs();
public:
	TimerThread(SystemState* s);
	/* Stopps the timer thread from executing any more jobs. This may return
	 * before the current job has finished its execution.
	 */
	void stop();
	/*
	 * Like stop, but waits until the current job has finished, too.
	 */
	void wait();
	~TimerThread();
	void addTick(uint32_t tickTime, ITickJob* job);
	void addWait(uint32_t waitTime, ITickJob* job);
	/* Remove the job from the list of pending tasks. If it is currently executing,
	 * wait until it is done.
	 */
	void removeJob(ITickJob* job);
	void removeJob_noLock(ITickJob* job);
};

class Chronometer
{
private:
	uint64_t start;
public:
	Chronometer();
	uint32_t checkpoint();
};

};
#endif /* TIMER_H */
