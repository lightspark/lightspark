/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <cassert>

#include "thread_pool.h"
#include "exceptions.h"
#include "compat.h"
#include "logger.h"
#include "swf.h"

using namespace lightspark;

ThreadPool::ThreadPool(SystemState* s):num_jobs(0),stopFlag(false)
{
	m_sys=s;
	for(uint32_t i=0;i<NUM_THREADS;i++)
	{
		curJobs[i]=NULL;
#ifdef HAVE_NEW_GLIBMM_THREAD_API
		threads[i] = Thread::create(sigc::bind(&job_worker,this,i));
#else
		threads[i] = Thread::create(sigc::bind(&job_worker,this,i),true);
#endif
		
	}
}

void ThreadPool::forceStop()
{
	if(!stopFlag)
	{
		stopFlag=true;
		//Signal an event for all the threads
		for(int i=0;i<NUM_THREADS;i++)
			num_jobs.signal();

		{
			Locker l(mutex);
			//Now abort any job that is still executing
			for(int i=0;i<NUM_THREADS;i++)
			{
				if(curJobs[i])
				{
					curJobs[i]->threadAborting = true;
					curJobs[i]->threadAbort();
				}
			}
			//Fence all the non executed jobs
			std::deque<IThreadJob*>::iterator it=jobs.begin();
			for(;it!=jobs.end();++it)
				(*it)->jobFence();
			jobs.clear();
		}

		for(int i=0;i<NUM_THREADS;i++)
		{
			threads[i]->join();
		}
	}
}

ThreadPool::~ThreadPool()
{
	forceStop();
}

void ThreadPool::job_worker(ThreadPool* th, uint32_t index)
{
	setTLSSys(th->m_sys);

	ThreadProfile* profile=getSys()->allocateProfiler(RGB(200,200,0));
	char buf[16];
	snprintf(buf,16,"Thread %u",index);
	profile->setTag(buf);

	Chronometer chronometer;
	while(1)
	{
		th->num_jobs.wait();
		if(th->stopFlag)
			return;
		Locker l(th->mutex);
		IThreadJob* myJob=th->jobs.front();
		th->jobs.pop_front();
		th->curJobs[index]=myJob;
		l.release();

		chronometer.checkpoint();
		try
		{
			// it's possible that a job was added and will be executed while forcestop() has been called
			if(th->stopFlag)
				return;
			myJob->execute();
		}
	        catch(JobTerminationException& ex)
		{
			LOG(LOG_NOT_IMPLEMENTED,"Job terminated");
		}
		catch(LightsparkException& e)
		{
			LOG(LOG_ERROR,_("Exception in ThreadPool ") << e.what());
			getSys()->setError(e.cause);
		}
		profile->accountTime(chronometer.checkpoint());

		l.acquire();
		th->curJobs[index]=NULL;
		l.release();

		//jobFencing is allowed to happen outside the mutex
		myJob->jobFence();
	}
}

void ThreadPool::addJob(IThreadJob* j)
{
	Locker l(mutex);
	if(stopFlag)
	{
		j->jobFence();
		return;
	}
	assert(j);
	jobs.push_back(j);
	num_jobs.signal();
}

