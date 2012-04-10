/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _TIMER_H
#define _TIMER_H

#include "compat.h"
#include <list>
#include <time.h>
#include "threading.h"

namespace lightspark
{

class SystemState;
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
	virtual ~ITickJob(){};
	//Last method to be called when no more ticks will be sent
	virtual void tickFence() = 0;
};

class TimerThread
{
private:
	class TimingEvent
	{
	public:
		TimingEvent(ITickJob* _job, bool _isTick, uint32_t _tickTime, uint32_t _waitTime) 
			: isTick(_isTick),job(_job),wakeUpTime(_isTick ? _tickTime : _waitTime), tickTime(_tickTime) {};
		bool isTick;
		ITickJob* job;
		CondTime wakeUpTime;
		uint32_t tickTime;
	};
	Mutex mutex;
	Cond newEvent;
	Thread* t;
	std::list<TimingEvent*> pendingEvents;
	SystemState* m_sys;
	volatile bool stopped;
	bool joined;
	volatile ITickJob* inExecution;
	void worker();
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
#endif
