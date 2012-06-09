/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

using namespace lightspark;
using namespace std;

TimerThread::TimerThread(SystemState* s):m_sys(s),stopped(false),joined(false),inExecution(NULL)
{
#ifdef HAVE_NEW_GLIBMM_THREAD_API
	t = Thread::create(sigc::mem_fun(this,&TimerThread::worker));
#else
	t = Thread::create(sigc::mem_fun(this,&TimerThread::worker),true);
#endif
}

void TimerThread::stop()
{
	if(!stopped)
	{
		stopped=true;
		newEvent.signal();
	}
}

void TimerThread::wait()
{
	if(!joined)
	{
		joined=true;
		stop();
		t->join();
	}
}

TimerThread::~TimerThread()
{
	stop();
	list<TimingEvent*>::iterator it=pendingEvents.begin();
	for(;it!=pendingEvents.end();++it)
		delete *it;
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
	Mutex::Lock l(mutex);
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

void TimerThread::worker()
{
	setTLSSys(m_sys);

	Mutex::Lock l(mutex);
	while(1)
	{
		/* Wait until the first event appears */
		while(pendingEvents.empty())
		{
			newEvent.wait(mutex);
			if(stopped)
				return;
		}

		/* Get expiration of first event */
		CondTime timing=pendingEvents.front()->wakeUpTime;
		/* Wait for the absolute time or a newEvent signal
		 * this unlocks the mutex and relocks it before returing
		 */
		timing.wait(mutex,newEvent);

		if(stopped)
			return;

		if(pendingEvents.empty())
			continue;

		TimingEvent* e=pendingEvents.front();

		/* check if the top event is due now. It could be have been removed/inserted
		 * while we slept */
		if(e->wakeUpTime.isInTheFuture())
			continue;

		pendingEvents.pop_front();

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
			insertNewEvent_nolock(e);
		}

		/* let removeJob() know what we are currently doing */
		inExecution = e->job;
		l.release();
		e->job->tick();
		inExecution = NULL;
		l.acquire();

		/* Cleanup */
		if(!e->isTick)
		{
			e->job->tickFence();
			delete e;
		}
	}
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

void TimerThread::removeJob(ITickJob* job)
{
	Mutex::Lock l(mutex);
	/* Busy-wait until job is not executing anymore */
	while(inExecution == job)
		Thread::yield();

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
