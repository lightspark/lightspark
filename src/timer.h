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
#include "compat.h"
#include <list>
#include <ctime>
#include "threading.h"

namespace lightspark
{

class SystemState;

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
