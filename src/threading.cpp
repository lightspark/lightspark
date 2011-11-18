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

#include <assert.h>

#include "threading.h"
#include "exceptions.h"
#include "logger.h"
#include "compat.h"

using namespace lightspark;

//NOTE: thread jobs can be run only once
IThreadJob::IThreadJob():jobTerminated(0),destroyMe(false),executing(false),aborting(false)
{
}

IThreadJob::~IThreadJob()
{
	if(executing)
		waitForJobTermination();
}

void IThreadJob::waitForJobTermination()
{
	jobTerminated.wait();
}

void IThreadJob::run()
{
	try
	{
		execute();
	}
	catch(JobTerminationException& ex)
	{
		LOG(LOG_NOT_IMPLEMENTED,_("Job terminated"));
	}

	jobTerminated.signal();
}

void IThreadJob::stop()
{
	if(executing)
	{
		aborting=true;
		this->threadAbort();
	}
}

Semaphore::Semaphore(uint32_t init):value(init)
{
}

Semaphore::~Semaphore()
{
	//On destrucion unblocks all blocked threads
	value = 4096;
	cond.broadcast();
}

void Semaphore::wait()
{
	Mutex::Lock lock(mutex);
	while(value == 0)
		cond.wait(mutex);
	value--;
}

bool Semaphore::try_wait()
{
	Mutex::Lock lock(mutex);
	if(value == 0)
		return false;
	else
	{
		value--;
		return true;
	}
}

void Semaphore::signal()
{
	Mutex::Lock lock(mutex);
	value++;
	cond.signal();
}
