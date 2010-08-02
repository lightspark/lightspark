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

#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include "compat.h"
#include <deque>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include "threading.h"

extern TLSDATA lightspark::IThreadJob* thisJob;

namespace lightspark
{

#define NUM_THREADS 5

class SystemState;

class ThreadPool
{
private:
	sem_t mutex;
	pthread_t threads[NUM_THREADS];
	IThreadJob* curJobs[NUM_THREADS];
	std::deque<IThreadJob*> jobs;
	sem_t num_jobs;
	static void* job_worker(void*);
	SystemState* m_sys;
	bool stopFlag;
public:
	ThreadPool(SystemState* s);
	~ThreadPool();
	void addJob(IThreadJob* j);
	void stop();
};

};

#endif
