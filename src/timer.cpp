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

TimerThread::TimerThread(SystemState* s):m_sys(s),currentJob(NULL),stopped(false),joined(false)
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
		cout << (*it)->job << endl;
}

void TimerThread::worker(TimerThread* th)
{
	setTLSSys(th->m_sys);
	while(1)
	{
		//Wait until a time expires
		th->mutex.lock();
		//Check if there is any event
		while(th->pendingEvents.empty())
		{
			th->newEvent.wait(th->mutex);
			if(th->stopped)
			{
				th->mutex.unlock();
				return;
			}
		}

		//Get expiration of first event
		uint64_t timing=th->pendingEvents.front()->timing;
		Glib::TimeVal tv;
		tv.add_milliseconds(timing);
		//Wait for the absolute time, or a newEvent signal
		bool newEvent = th->newEvent.timed_wait(th->mutex,tv);

		if(th->stopped)
		{
			th->mutex.unlock();
			return;
		}

		//We got a new event, maybe with a smaller timing
		if(newEvent)
			continue;

		TimingEvent* e=th->pendingEvents.front();
		th->pendingEvents.pop_front();

		if(e->job && e->job->stopMe)
		{
			//Abort the tick by invalidating the job
			e->job=NULL;
			e->isTick=false;
		}

		th->currentJob=e->job;

		bool destroyEvent=true;
		if(e->isTick) //Let's schedule the event again
		{
			e->timing+=e->tickTime;
			th->insertNewEvent_nolock(e); //newEvent may be signaled, and will be waited by sem_timedwait
			destroyEvent=false;
		}

		th->mutex.unlock();

		//Now execute the job
		//NOTE: jobs may be invalid if they has been cancelled in the meantime
		assert(e->job || !e->isTick);
		if(e->job)
			e->job->tick();

		th->currentJob=NULL;

		//Cleanup
		if(destroyEvent)
			delete e;
	}
}

void TimerThread::addTick(uint32_t tickTime, ITickJob* job)
{
	TimingEvent* e=new TimingEvent;
	e->isTick=true;
	e->job=job;
	e->tickTime=tickTime;
	e->timing=compat_get_current_time_ms()+tickTime;
	insertNewEvent(e);
}

void TimerThread::addWait(uint32_t waitTime, ITickJob* job)
{
	TimingEvent* e=new TimingEvent;
	e->isTick=false;
	e->job=job;
	e->tickTime=0;
	e->timing=compat_get_current_time_ms()+waitTime;
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

	//Spin wait until the job ends (per design should be very short)
	//As we hold the mutex and currentJob is not NULL we are surely executing a job
	while(currentJob==job);
	//The job ended

	if(it==pendingEvents.end())
		return false;

	//If we are waiting of this event it's not safe to remove it
	//so we flag it has invalid and the worker thread will remove it
	//this is equivalent, as the event is not going to be fired
	TimingEvent* e=*it;
	if(first)
	{
		e->job=NULL;
		e->isTick=false;
	}
	else
	{
		pendingEvents.erase(it);
		delete e;
	}

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
