/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "scripting/flash/system/flashsystem.h"

using namespace lightspark;

ThreadPool::ThreadPool(SystemState* s):num_jobs(0),stopFlag(false),runcount(0)
{
	m_sys=s;
	for(uint32_t i=0;i<NUM_THREADS;i++)
	{
		curJobs[i]=nullptr;
		data[i].index=i;
		data[i].pool = this;
		threads[i] = SDL_CreateThread(job_worker,"ThreadPool",&data[i]);
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
			SDL_WaitThread(threads[i],nullptr);
		}
	}
}

ThreadPool::~ThreadPool()
{
	forceStop();
}

int ThreadPool::job_worker(void *d)
{
	ThreadPoolData* data = (ThreadPoolData*)d;
	setTLSSys(data->pool->m_sys);

	ThreadProfile* profile=data->pool->m_sys->allocateProfiler(RGB(200,200,0));
	char buf[16];
	snprintf(buf,16,"Thread %u",data->index);
	profile->setTag(buf);

	Chronometer chronometer;
	while(1)
	{
		data->pool->num_jobs.wait();
		if(data->pool->stopFlag)
			return 0;
		Locker l(data->pool->mutex);
		IThreadJob* myJob=data->pool->jobs.front();
		data->pool->jobs.pop_front();
		data->pool->curJobs[data->index]=myJob;
		data->pool->runcount++;
		l.release();

		setTLSWorker(myJob->fromWorker);
		chronometer.checkpoint();
		try
		{
			// it's possible that a job was added and will be executed while forcestop() has been called
			if(data->pool->stopFlag)
				return 0;
			myJob->execute();
		}
		catch(JobTerminationException& ex)
		{
			LOG(LOG_NOT_IMPLEMENTED,"Job terminated");
		}
		catch(LightsparkException& e)
		{
			LOG(LOG_ERROR,"Exception in ThreadPool " << e.what());
			data->pool->m_sys->setError(e.cause);
		}
		catch(std::exception& e)
		{
			LOG(LOG_ERROR,"std Exception in ThreadPool:"<<myJob<<" "<<e.what());
			data->pool->m_sys->setError(e.what());
		}
		
		profile->accountTime(chronometer.checkpoint());

		l.acquire();
		data->pool->curJobs[data->index]=nullptr;
		data->pool->runcount--;
		l.release();

		//jobFencing is allowed to happen outside the mutex
		myJob->jobFence();
	}
	return 0;
}

void ThreadPool::addJob(IThreadJob* j)
{
	Locker l(mutex);
	j->setWorker(getWorker());
	if(stopFlag)
	{
		j->jobFence();
		return;
	}
	assert(j);
	if (runcount == NUM_THREADS)
	{
		// no slot available, we create an additional thread
		runAdditionalThread(j);
	}
	else
	{
		jobs.push_back(j);
		num_jobs.signal();
	}
}
void ThreadPool::runAdditionalThread(IThreadJob* j)
{
	SDL_Thread* t = SDL_CreateThread(additional_job_worker,"additionalThread",j);
	SDL_DetachThread(t);
}

int ThreadPool::additional_job_worker(void* d)
{
	IThreadJob* myJob=(IThreadJob*)d;

	setTLSSys(myJob->fromWorker->getSystemState());
	setTLSWorker(myJob->fromWorker);
	try
	{
		myJob->execute();
	}
	catch(JobTerminationException& ex)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Job terminated");
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Exception in AdditionalThread " << e.what());
		myJob->fromWorker->getSystemState()->setError(e.cause);
	}
	catch(std::exception& e)
	{
		LOG(LOG_ERROR,"std Exception in AdditionalThread:"<<myJob<<" "<<e.what());
		myJob->fromWorker->getSystemState()->setError(e.what());
	}
	myJob->jobFence();
	return 0;
}
