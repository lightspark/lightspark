/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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

#ifndef TIMER_H
#define TIMER_H 1

#include "forwards/timer.h"
#include "interfaces/timer.h"
#include "utils/timespec.h"
#include "compat.h"
#include <list>
#include <set>
#include <ctime>
#include "threading.h"

namespace lightspark
{

class SystemState;

class Time : public ITime
{
	uint64_t getCurrentTime_ms() const override { return compat_msectiming(); }
	uint64_t getCurrentTime_us() const override { return compat_usectiming(); }
	uint64_t getCurrentTime_ns() const override { return compat_nsectiming(); }
	TimeSpec now() const override { return TimeSpec::fromNs(compat_nsectiming()); }
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

struct LSTimer
{
	enum Type
	{
		Wait,
		Tick,
		Frame,
	};
	Type type;
	TimeSpec startTime;
	// Fake/Ideal starting time, useful for deterministic timing.
	TimeSpec fakeStartTime;
	TimeSpec timeout;
	ITickJob* job;

	TimeSpec deadline() const { return startTime + timeout; }
	TimeSpec fakeDeadline() const { return fakeStartTime + timeout; }
	bool isWait() const { return type == Type::Wait; }
	bool isTick() const { return type == Type::Tick; }
	bool isFrame() const { return type == Type::Frame; }

	bool operator==(const LSTimer& other) const { return deadline() == other.deadline(); }
	bool operator!=(const LSTimer& other) const { return deadline() != other.deadline(); }
	bool operator>=(const LSTimer& other) const { return deadline() >= other.deadline(); }
	bool operator<=(const LSTimer& other) const { return deadline() <= other.deadline(); }
	bool operator>(const LSTimer& other) const { return deadline() > other.deadline(); }
	bool operator<(const LSTimer& other) const { return deadline() < other.deadline(); }
};

class LSTimers
{
	using TimerType = LSTimer::Type;
private:
	Mutex timerMutex;
	std::multiset<LSTimer> timers;
	TimeSpec currentTime;
	// Fake/Ideal current time, useful for deterministic timing.
	TimeSpec fakeCurrentTime;
	TimeSpec curFrameTime;
	TimeSpec nextFrameTime;
	SystemState* sys;

	// Maximum timer ticks per call to `updateTimers()`.
	static constexpr int maxTicks = 10;
	// Maximum frames per call to `updateTimers()`.
	static constexpr int maxFrames = 5;

	void pushTimer(const LSTimer& timer);
	void pushTimerNoLock(const LSTimer& timer);
	LSTimer popTimer();
	LSTimer popTimerNoLock();
	const LSTimer& peekTimer();
	const LSTimer& peekTimerNoLock() const;

	void addJob(const TimeSpec& time, const TimerType& type, ITickJob* job);
	TimeSpec getFrameRate() const;
public:
	LSTimers(SystemState* _sys) : sys(_sys) {}

	void addTick(const TimeSpec& tickTime, ITickJob* job) { addJob(tickTime, TimerType::Tick, job); }
	void addFrameTick(ITickJob* job) { addJob(getFrameRate(), TimerType::Frame, job); }
	void addWait(const TimeSpec& waitTime, ITickJob* job) { addJob(waitTime, TimerType::Wait, job); }
	void removeJob(ITickJob* job);
	void removeJobNoLock(ITickJob* job);
	TimeSpec updateTimers(const TimeSpec& delta);
	TimeSpec getFakeCurrentTime() const { return fakeCurrentTime; }
	void setFakeCurrentTime(const TimeSpec& time) { fakeCurrentTime = time; }
	const LSTimer& getCurrentTimer() { return peekTimer(); }
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
