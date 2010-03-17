/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "thread_pool.h"
#include "compat.h"

using namespace lightspark;

extern TLSDATA SystemState* sys;

ThreadPool::ThreadPool(SystemState* s)
{
	m_sys=s;
	sem_init(&mutex,0,1);
	sem_init(&num_jobs,0,0);
	for(int i=0;i<NUM_THREADS;i++)
		pthread_create(&threads[i],NULL,job_worker,this);
}

void* ThreadPool::job_worker(void* t)
{
	ThreadPool* th=static_cast<ThreadPool*>(t);
	sys=th->m_sys;

	while(1)
	{
		sem_wait(&th->num_jobs);
		sem_wait(&th->mutex);
		IThreadJob* myJob=th->jobs.front();
		th->jobs.pop_front();
		sem_post(&th->mutex);

		myJob->run();
	}
	return NULL;
}

void ThreadPool::addJob(IThreadJob* j)
{
	sem_wait(&mutex);
	jobs.push_back(j);
	sem_post(&mutex);
	sem_post(&num_jobs);
}

void IThreadJob::run()
{
	executing=true;
	execute();
	executing=false;
}
