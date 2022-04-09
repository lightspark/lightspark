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

#ifndef THREAD_POOL_H
#define THREAD_POOL_H 1

#include "compat.h"
#include <deque>
#include <cstdlib>
#include "threading.h"

namespace lightspark
{

#define NUM_THREADS 20

class SystemState;

class ThreadPool
{
private:
	struct ThreadPoolData
	{
		ThreadPool* pool;
		int index;
	};
	Mutex mutex;
	SDL_Thread* threads[NUM_THREADS];
	ThreadPoolData data[NUM_THREADS];
	IThreadJob* volatile curJobs[NUM_THREADS];
	std::deque<IThreadJob*> jobs;
	Semaphore num_jobs;
	static int job_worker(void* d);
	SystemState* m_sys;
	volatile bool stopFlag;
	uint32_t runcount;
	void runAdditionalThread(IThreadJob* j);
	static int additional_job_worker(void* d);
public:
	ThreadPool(SystemState* s);
	~ThreadPool();
	void addJob(IThreadJob* j);
	void forceStop();
};

}

#endif /* THREAD_POOL_H */
