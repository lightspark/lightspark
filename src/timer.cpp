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

TimerThread::TimerThread(SystemState* s, ITimingEventList* _eventList, ITime* _time):eventList(_eventList != nullptr ? _eventList : new TimingEventList()),time(_time != nullptr ? _time : new Time()),m_sys(s),stopped(false),joined(false)
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
	auto pendingEvents = eventList->getPendingEvents();
	list<ITimingEvent*>::iterator it=pendingEvents.begin();
	for(;it!=pendingEvents.end();++it)
	{
		if ((*it)->job)
			(*it)->job->tickFence();
		delete *it;
	}
}

void ITimingEventList::insertEventNoLock(ITimingEvent* e, Cond& newEvent)
{
	list<ITimingEvent*>::iterator it=pendingEvents.begin();
	//If there are no events pending, or this is earlier than the first, signal newEvent
	if(pendingEvents.empty() || **it > *e)
	{
		pendingEvents.insert(it, e);
		newEvent.signal();
		return;
	}
	++it;

	for(;it!=pendingEvents.end();++it)
	{
		if(**it > *e)
		{
			pendingEvents.insert(it, e);
			return;
		}
	}
	//Event has to be inserted after all the others
	pendingEvents.insert(pendingEvents.end(), e);
}

void ITimingEventList::insertEvent(ITimingEvent* e, Mutex& mutex, Cond& newEvent)
{
	Locker l(mutex);
	insertEventNoLock(e, newEvent);
}

//Unsafe debugging routine
void TimerThread::dumpJobs()
{
	auto pendingEvents = eventList->getPendingEvents();
	list<ITimingEvent*>::iterator it=pendingEvents.begin();
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
		while(th->eventList->isEmpty())
		{
			th->newEvent.wait(th->mutex);
			if(th->stopped)
				return 0;
		}

		/* Get expiration of first event */
		ITimingEvent* e=th->eventList->getFrontEvent();
		/* Wait for the absolute time or a newEvent signal
		 * this unlocks the mutex and relocks it before returing
		 */
		e->wait(th->mutex,th->newEvent);

		if(th->stopped)
			return 0;

		if(th->eventList->isEmpty())
			continue;

		/* check if the top event is due now. It could be have been removed/inserted
		 * while we slept */
		if(e->isInTheFuture())
			continue;

		th->eventList->popFrontEvent();

		if(e->job->stopMe)
		{
			e->job->tickFence();
			delete e;
			continue;
		}

		if(e->isTick)
		{
			/* re-enqueue*/
			e->addMilliseconds(e->tickTime);
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

void TimerThread::TimingEventList::addJob(uint32_t ms, ITickJob* job, bool isTick, Mutex& mutex, Cond& newEvent)
{
	uint32_t tickTime = isTick ? ms : 0;
	uint32_t waitTime = isTick ? 0 : ms;
	TimingEvent* e=new TimingEvent(job, isTick, tickTime, waitTime);
	insertEvent(e, mutex, newEvent);
}

/*
 * removeJob()
 *
 * Removes the given job from the pendingEvents queue
 */
void ITimingEventList::removeJob(ITickJob* job, Mutex& mutex, Cond& newEvent)
{
	Locker l(mutex);
	removeJobNoLock(job, newEvent);
}

void ITimingEventList::removeJobNoLock(ITickJob* job, Cond& newEvent)
{
	/* See if that job is currently pending */
	list<ITimingEvent*>::iterator it=pendingEvents.begin();
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

	job->tickFence();
	TimingEvent* e=*it;
	pendingEvents.erase(it);
	delete e;

	/* the worker is waiting on this job, wake him up */
	if(first)
		newEvent.signal();
}

Chronometer::Chronometer() : IChronometer(compat_get_thread_cputime_us())
{
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
