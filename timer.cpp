/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "timer.h"
#include "compat.h"

using namespace lightspark;
using namespace std;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;

uint64_t lightspark::timespecToMsecs(timespec t)
{
	uint64_t ret=0;
	ret+=(t.tv_sec*1000LL);
	ret+=(t.tv_nsec/1000000LL);
	return ret;
}

uint64_t lightspark::timespecToUsecs(timespec t)
{
	uint64_t ret=0;
	ret+=(t.tv_sec*1000000LL);
	ret+=(t.tv_nsec/1000LL);
	return ret;
}

timespec lightspark::msecsToTimespec(uint64_t time)
{
	timespec ret;
	ret.tv_sec=time/1000LL;
	ret.tv_nsec=(time%1000LL)*1000000LL;
	return ret;
}

TimerThread::TimerThread(SystemState* s):m_sys(s),currentJob(NULL),stopped(false)
{
	sem_init(&mutex,0,1);
	sem_init(&newEvent,0,0);

	pthread_create(&t,NULL,(thread_worker)timer_worker,this);
}

void TimerThread::stop()
{
	if(!stopped)
	{
		stopped=true;
		sem_post(&newEvent);
	}
}

void TimerThread::wait()
{
	stop();
	pthread_join(t,NULL);
}

TimerThread::~TimerThread()
{
	stop();
	list<TimingEvent*>::iterator it=pendingEvents.begin();
	for(;it!=pendingEvents.end();it++)
		delete *it;
	sem_destroy(&mutex);
	sem_destroy(&newEvent);
}

void TimerThread::insertNewEvent_nolock(TimingEvent* e)
{
	list<TimingEvent*>::iterator it=pendingEvents.begin();
	//If there are no events pending, or this is earlier than the first, signal newEvent
	if(pendingEvents.empty() || (*it)->timing > e->timing)
	{
		pendingEvents.insert(it, e);
		sem_post(&newEvent);
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
	sem_wait(&mutex);
	insertNewEvent_nolock(e);
	sem_post(&mutex);
}

//Unsafe debugging routine
void TimerThread::dumpJobs()
{
	list<TimingEvent*>::iterator it=pendingEvents.begin();
	//Find if the job is in the list
	for(;it!=pendingEvents.end();it++)
		cout << (*it)->job << endl;
}

void* TimerThread::timer_worker(TimerThread* th)
{
	sys=th->m_sys;
	while(1)
	{
		//Wait until a time expires
		sem_wait(&th->mutex);
		//Check if there is any event
		while(th->pendingEvents.empty())
		{
			sem_post(&th->mutex);
			sem_wait(&th->newEvent);
			if(th->stopped)
				pthread_exit(0);
			sem_wait(&th->mutex);
		}

		//Get expiration of first event
		uint64_t timing=th->pendingEvents.front()->timing;
		//Wait for the absolute time, or a newEvent signal
		timespec tmpt=msecsToTimespec(timing);
		sem_post(&th->mutex);
		int ret=sem_timedwait(&th->newEvent, &tmpt);
		if(th->stopped)
			pthread_exit(0);

		if(ret==0)
			continue;

		//The first event has expired
		int err=errno;
		if(err!=ETIMEDOUT)
		{
			LOG(LOG_ERROR,"Unexpected failure of sem_timedwait.. Trying to go on. errno=" << err);
			continue;
		}

		//Note: it may happen that between the sem_timewait and this code another event gets inserted in the front. In this 
		//case it's not an error to execute it now, as it's expiration time is the first anyway and before the one expired
		sem_wait(&th->mutex);
		TimingEvent* e=th->pendingEvents.front();
		th->pendingEvents.pop_front();

		th->currentJob=e->job;

		bool destroyEvent=true;
		if(e->isTick) //Let's schedule the event again
		{
			e->timing+=e->tickTime;
			th->insertNewEvent_nolock(e); //newEvent may be signaled, and will be waited by sem_timedwait
			destroyEvent=false;
		}

		sem_post(&th->mutex);

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
	sem_wait(&mutex);

	list<TimingEvent*>::iterator it=pendingEvents.begin();
	bool first=true;
	//Find if the job is in the list
	for(;it!=pendingEvents.end();it++)
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
	{
		sem_post(&mutex);
		return false;
	}

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

	sem_post(&mutex);
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
