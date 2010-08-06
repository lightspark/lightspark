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

#include <assert.h>

#include "threading.h"
#include "exceptions.h"
#include "logger.h"

using namespace lightspark;

//NOTE: thread jobs can be run only once
IThreadJob::IThreadJob():destroyMe(false),executing(false),aborting(false)
{
	sem_init(&terminated, 0, 0);
}

IThreadJob::~IThreadJob()
{
	if(executing)
		sem_wait(&terminated);
	sem_destroy(&terminated);
}

void IThreadJob::run()
{
	try
	{
		assert(thisJob);
		execute();
	}
	catch(JobTerminationException& ex)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Job terminated");
	}

	sem_post(&terminated);
}

void IThreadJob::stop()
{
	if(executing)
	{
		aborting=true;
		this->threadAbort();
	}
}

Mutex::Mutex(const char* n):name(n),foundBusy(0)
{
	sem_init(&sem,0,1);
}

Mutex::~Mutex()
{
	if(name)
		LOG(LOG_TRACE,"Mutex " << name << " waited " << foundBusy << " times");
	sem_destroy(&sem);
}

void Mutex::lock()
{
	if(name)
	{
		//If the semaphore can be acquired immediately just return
		if(sem_trywait(&sem)==0)
			return;

		//Otherwise log the busy event and do a real wait
		foundBusy++;
	}

	sem_wait(&sem);
}

void Mutex::unlock()
{
	sem_post(&sem);
}

Semaphore::Semaphore(uint32_t init)//:blocked(0),maxBlocked(max)
{
	sem_init(&sem,0,init);
}

Semaphore::~Semaphore()
{
	//On destrucion unblocks the blocked thread
	signal();
	sem_destroy(&sem);
}

void Semaphore::wait()
{
	sem_wait(&sem);
}

bool Semaphore::try_wait()
{
	return sem_trywait(&sem)==0;
}

void Semaphore::signal()
{
	sem_post(&sem);
}
