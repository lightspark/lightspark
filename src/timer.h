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
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include "thread_pool.h"

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
	virtual ~ITickJob(){};
};

uint64_t timespecToUsecs(timespec t);
uint64_t timespecToMsecs(timespec t);
timespec msecsToTimespec(uint64_t time);

typedef void* (*thread_worker)(void*);
class TimerThread
{
private:
	class TimingEvent
	{
	public:
		bool isTick;
		ITickJob* job;
		//Timing are in milliseconds
		uint64_t timing;
		uint32_t tickTime;
	};
	sem_t mutex;
	sem_t newEvent;
	pthread_t t;
	std::list<TimingEvent*> pendingEvents;
	SystemState* m_sys;
	ITickJob* volatile currentJob;
	bool stopped;
	bool joined;
	static void* timer_worker(TimerThread*);
	void insertNewEvent(TimingEvent* e);
	void insertNewEvent_nolock(TimingEvent* e);
	void dumpJobs();
public:
	TimerThread(SystemState* s);
	void stop();
	void wait();
	~TimerThread();
	void addTick(uint32_t tickTime, ITickJob* job);
	void addWait(uint32_t waitTime, ITickJob* job);
	//Returns if the job has been found or not
	//If the canceled job is currently executing this waits for it to complete
	bool removeJob(ITickJob* job);
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
