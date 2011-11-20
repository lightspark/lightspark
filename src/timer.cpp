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

#include "swf.h"
#include <stdlib.h>
#include <assert.h>

#include "timer.h"
#include "compat.h"

using namespace lightspark;
using namespace std;

TimerThread::TimerThread(SystemState* s):m_sys(s),stopped(false),joined(false)
{
	t = Thread::create(sigc::bind<0>(&TimerThread::worker,this), true);
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
	if(pendingEvents.empty() || (*it)->timing > e->timing)
	{
		pendingEvents.insert(it, e);
		newEvent.signal();
		return;
	}
	++it;

	for(;it!=pendingEvents.end();++it)
	{
		if((*it)->timing > e->timing)
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
	mutex.lock();
	insertNewEvent_nolock(e);
	mutex.unlock();
}

//Unsafe debugging routine
void TimerThread::dumpJobs()
{
	list<TimingEvent*>::iterator it=pendingEvents.begin();
	//Find if the job is in the list
	for(;it!=pendingEvents.end();++it)
		LOG(LOG_INFO, (*it)->job );
}

void TimerThread::worker(TimerThread* th)
{
	setTLSSys(th->m_sys);

	th->mutex.lock();
	while(1)
	{
		/* Wait until the first event appears */
		while(th->pendingEvents.empty())
		{
			th->newEvent.wait(th->mutex);
			if(th->stopped)
				break;
		}

		/* Get expiration of first event */
		Glib::TimeVal timing=th->pendingEvents.front()->timing;
		/* Wait for the absolute time or a newEvent signal
		 * this unlocks the mutex and relocks it before returing
		 */
		th->newEvent.timed_wait(th->mutex,timing);

		if(th->stopped)
			break;

		if(th->pendingEvents.empty())
			continue;

		TimingEvent* e=th->pendingEvents.front();

		/* check if the top even is due now. It could be have been removed/inserted
		 * while we slept */
		Glib::TimeVal now;
		now.assign_current_time();
		if((now-timing).negative()) /* timing > now */
			continue;

		th->pendingEvents.pop_front();

		if(e->job->stopMe)
		{
			delete e;
			continue;
		}

		bool destroyEvent=true;
		if(e->isTick)
		{
			e->timing.add_milliseconds(e->tickTime);
			th->insertNewEvent_nolock(e);
			destroyEvent=false;
		}

		th->mutex.unlock();
		e->job->tick();
		th->mutex.lock();

		/* Cleanup */
		if(destroyEvent)
			delete e;
	}
	th->mutex.unlock();
}

void TimerThread::addTick(uint32_t tickTime, ITickJob* job)
{
	TimingEvent* e=new TimingEvent;
	e->isTick=true;
	e->job=job;
	e->tickTime=tickTime;
	e->timing.assign_current_time();
	e->timing.add_milliseconds(tickTime);
	insertNewEvent(e);
}

void TimerThread::addWait(uint32_t waitTime, ITickJob* job)
{
	TimingEvent* e=new TimingEvent;
	e->isTick=false;
	e->job=job;
	e->tickTime=0;
	e->timing.assign_current_time();
	e->timing.add_milliseconds(waitTime);
	insertNewEvent(e);
}

bool TimerThread::removeJob(ITickJob* job)
{
	Mutex::Lock l(mutex);

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
		return false;

	TimingEvent* e=*it;
	pendingEvents.erase(it);
	delete e;

	/* the worker is waiting on this job, wake him up */
	if(first)
		newEvent.signal();

	return true;
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
