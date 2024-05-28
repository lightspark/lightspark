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

#include "swf.h"
#include <cstdlib>
#include <cassert>

#include "timer.h"
#include "compat.h"
#include "interfaces/timer.h"

using namespace lightspark;
using namespace std;

TimerThread::TimerThread(SystemState* s):m_sys(s),stopped(false),joined(false)
{
	t = SDL_CreateThread(&TimerThread::worker,"TimerThread",this);
}

void TimerThread::stop()
{
	if(!stopped)
		stopped=true;
	newEvent.signal();
}

void TimerThread::wait()
{
	if(!joined)
	{
		joined=true;
		stop();
		SDL_WaitThread(t,nullptr);
	}
}

TimerThread::~TimerThread()
{
	stop();
	list<TimingEvent*>::iterator it=pendingEvents.begin();
	for(;it!=pendingEvents.end();++it)
	{
		if ((*it)->job)
			(*it)->job->tickFence();
		delete *it;
	}
}

void TimerThread::insertNewEvent_nolock(TimingEvent* e)
{
	list<TimingEvent*>::iterator it=pendingEvents.begin();
	//If there are no events pending, or this is earlier than the first, signal newEvent
	if(pendingEvents.empty() || (*it)->wakeUpTime > e->wakeUpTime)
	{
		pendingEvents.insert(it, e);
		newEvent.signal();
		return;
	}
	++it;

	for(;it!=pendingEvents.end();++it)
	{
		if((*it)->wakeUpTime > e->wakeUpTime)
		{
			pendingEvents.insert(it, e);
			return;
		}
	}
	//Event has to be inserted after all the others
	pendingEvents.insert(pendingEvents.end(), e);
}

void TimerThread::insertNewEvent(TimingEvent* e)
{
	Locker l(mutex);
	insertNewEvent_nolock(e);
}

//Unsafe debugging routine
void TimerThread::dumpJobs()
{
	list<TimingEvent*>::iterator it=pendingEvents.begin();
	//Find if the job is in the list
	for(;it!=pendingEvents.end();++it)
		LOG(LOG_INFO, (*it)->job );
}

/*
 * Worker executing the queued events.
 *
 * It holds "mutex" all the time but
 *   1. when waiting for on newEvent or for the correct time to execute a job.
 *   2. while executing e->job->tick() (during this time inExectution == e->job)
 * The pendingEvents queue may be altered by another thread with "mutex"
 * An event may be deleted by another thread with "mutex" only if inExectution != jobToDelete
 */
int TimerThread::worker(void *d)
{
	TimerThread* th = (TimerThread*)d;
	setTLSSys(th->m_sys);

	Locker l(th->mutex);
	while(1)
	{
		/* Wait until the first event appears */
		while(th->pendingEvents.empty())
		{
			th->newEvent.wait(th->mutex);
			if(th->stopped)
				return 0;
		}

		/* Get expiration of first event */
		CondTime timing=th->pendingEvents.front()->wakeUpTime;
		/* Wait for the absolute time or a newEvent signal
		 * this unlocks the mutex and relocks it before returing
		 */
		timing.wait(th->mutex,th->newEvent);

		if(th->stopped)
			return 0;

		if(th->pendingEvents.empty())
			continue;

		TimingEvent* e=th->pendingEvents.front();

		/* check if the top event is due now. It could be have been removed/inserted
		 * while we slept */
		if(e->wakeUpTime.isInTheFuture())
			continue;

		th->pendingEvents.pop_front();

		if(e->job->stopMe)
		{
			e->job->tickFence();
			delete e;
			continue;
		}

		if(e->isTick)
		{
			/* re-enqueue*/
			e->wakeUpTime.addMilliseconds(e->tickTime);
			th->insertNewEvent_nolock(e);
		}

		/* If e->isTick == false, e is not in pendingQueue anymore and this function has the only reference to it.
		 * If e->isTick == true, we just enqueued e another time. If removeJob() is called on e->job from
		 * job->tick() or another thread, then this will remove e from pendingQueue and delete e after we release the mutex.
		 * In that case we may not access e after 'l.release()'.
		 */
		ITickJob* job = e->job;
		bool isTick = e->isTick;
		l.release();

		job->tick();

		l.acquire();

		/* Cleanup */
		if(!isTick)
		{
			e->job->tickFence();
			delete e;
		}
	}
	return 0;
}

void TimerThread::addTick(uint32_t tickTime, ITickJob* job)
{
	TimingEvent* e=new TimingEvent(job, true, tickTime, 0);
	insertNewEvent(e);
}

void TimerThread::addWait(uint32_t waitTime, ITickJob* job)
{
	TimingEvent* e=new TimingEvent(job, false, 0, waitTime);
	insertNewEvent(e);
}

/*
 * removeJob()
 *
 * Removes the given job from the pendingEvents queue
 */
void TimerThread::removeJob(ITickJob* job)
{
	Locker l(mutex);
	removeJob_noLock(job);
}
void TimerThread::removeJob_noLock(ITickJob* job)
{

	/* See if that job is currently pending */
	list<TimingEvent*>::iterator it=pendingEvents.begin();
	bool first=true;
	//Find if the job is in the list
	for(;it!=pendingEvents.end();++it)
	{
		if((*it)->job==job)
			break;
		first=false;
	}

	if(it==pendingEvents.end())
		return;

	TimingEvent* e=*it;
	pendingEvents.erase(it);
	delete e;

	/* the worker is waiting on this job, wake him up */
	if(first)
		newEvent.signal();
}

Chronometer::Chronometer()
{
	start = compat_get_thread_cputime_us();
}

uint32_t lightspark::Chronometer::checkpoint()
{
	uint64_t newstart;
	uint32_t ret;
	newstart=compat_get_thread_cputime_us();
	assert((newstart-start) < UINT32_MAX);
	ret=newstart-start;
	start=newstart;
	return ret;
}
