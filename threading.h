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

#ifndef _THREADING_H
#define _THREADING_H

#include "compat.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>

namespace lightspark
{

typedef void* (*thread_worker)(void*);

class Mutex
{
friend class Locker;
private:
	sem_t sem;
	const char* name;
	uint32_t foundBusy;
public:
	Mutex(const char* name);
	~Mutex();
	void lock();
	void unlock();
};

class IThreadJob
{
friend class ThreadPool;
friend class Condition;
private:
	sem_t jobTerminated;
	bool jobHasTerminated;
protected:
	bool destroyMe;
	bool executing;
	bool aborting;
	virtual void execute()=0;
	virtual void threadAbort()=0;
	virtual void jobFence()=0;
public:
	IThreadJob();
	virtual ~IThreadJob();
	void waitForJobTermination();
	void run();
	void stop() DLL_PUBLIC;
};

class Semaphore
{
private:
	sem_t sem;
	//TODO: use atomic incs and decs
	//uint32_t blocked;
	//uint32_t maxBlocked;
public:
	Semaphore(uint32_t init);
	~Semaphore();
	void signal();
	//void signal_all();
	void wait();
	bool try_wait();
};

class SemaphoreLighter
{
private:
	Semaphore& _s;
	bool lighted;
public:
	SemaphoreLighter(Semaphore& s):_s(s),lighted(false){}
	~SemaphoreLighter()
	{
		if(!lighted)
			_s.signal();
	}
	void light()
	{
		assert(!lighted);
		_s.signal();
		lighted=true;
	}
};

class Locker
{
private:
	Mutex& _m;
	bool acquired;
public:
	Locker(Mutex& m):_m(m),acquired(true)
	{
		_m.lock();
	}
	~Locker()
	{
		if(acquired)
			_m.unlock();
	}
	void lock()
	{
		assert(acquired==false);
		acquired=true;
		_m.lock();
	}
	void unlock()
	{
		assert(acquired==true);
		acquired=false;
		_m.unlock();
	}
};

template<class T, uint32_t size>
class BlockingCircularQueue
{
private:
	T queue[size];
	//Counting semaphores for the queue
	Semaphore freeBuffers;
	Semaphore usedBuffers;
	bool empty;
	uint32_t bufferHead;
	uint32_t bufferTail;
public:
	BlockingCircularQueue():freeBuffers(size),usedBuffers(0),empty(true),bufferHead(0),bufferTail(0)
	{
	}
	template<class GENERATOR>
	BlockingCircularQueue(const GENERATOR& g):freeBuffers(size),usedBuffers(0),empty(true),bufferHead(0),bufferTail(0)
	{
		for(uint32_t i=0;i<size;i++)
			g.init(queue[i]);
	}
	bool isEmpty() const { return empty; }
	T& front()
	{
		assert(!this->empty);
		return queue[bufferHead];
	}
	const T& front() const
	{
		assert(!this->empty);
		return queue[bufferHead];
	}
	bool nonBlockingPopFront()
	{
		//We don't want to block if empty
		if(!usedBuffers.try_wait())
			return false;
		//A frame is available
		bufferHead=(bufferHead+1)%size;
		if(bufferHead==bufferTail)
			empty=true;
		freeBuffers.signal();
		return true;
	}
	T& acquireLast()
	{
		freeBuffers.wait();
		uint32_t ret=bufferTail;
		bufferTail=(bufferTail+1)%size;
		return queue[ret];
	}
	void commitLast()
	{
		empty=false;
		usedBuffers.signal();
	}
	template<class GENERATOR>
	void regen(const GENERATOR& g)
	{
		for(uint32_t i=0;i<size;i++)
			g.init(queue[i]);
	}
	uint32_t len() const
	{
		uint32_t tmp=(bufferTail+size-bufferHead)%size;
		if(tmp==0 && !empty)
			tmp=size;
		return tmp;
	}

};

class Spinlock
{
private:
	pthread_spinlock_t l;
public:
	Spinlock()
	{
		pthread_spin_init(&l,0);
	}
	~Spinlock()
	{
		pthread_spin_destroy(&l);
	}
	void lock()
	{
		pthread_spin_lock(&l);
	}
	bool trylock()
	{
		return pthread_spin_trylock(&l)==0;
	}
	void unlock()
	{
		pthread_spin_unlock(&l);
	}
};

class SpinlockLocker
{
private:
	Spinlock& lock;
public:
	SpinlockLocker(Spinlock& _l):lock(_l)
	{
		lock.lock();
	}
	~SpinlockLocker()
	{
		lock.unlock();
	}
};

class Shepherd;

class Sheep
{
friend class Shepherd;
private:
	ACQUIRE_RELEASE_FLAG(disable);
	ACQUIRE_RELEASE_FLAG(executing);
	Shepherd* owner;
protected:
	bool lockOwner()
	{
		RELEASE_WRITE(executing,true);
		if(ACQUIRE_READ(disable))
		{
			RELEASE_WRITE(executing,false);
			return false;
		}
		return true;
	}
	void unlockOwner()
	{
		RELEASE_WRITE(executing,false);
	}
public:
	Sheep(Shepherd* _o):disable(false),executing(false),owner(_o){}
	~Sheep();
};

class Shepherd
{
private:
	std::vector<Sheep*> sheeps;
	Spinlock sheepsLock;
public:
	~Shepherd()
	{
		sheepsLock.lock();
		for(uint32_t i=0;i<sheeps.size();i++)
		{
			if(sheeps[i])
			{
				RELEASE_WRITE(sheeps[i]->disable,true);
				while(ACQUIRE_READ(sheeps[i]->executing));
				sheeps[i]->owner=NULL;
			}
		}
		sheepsLock.unlock();
	}
	void addSheep(Sheep* s)
	{
		sheepsLock.lock();
		for(uint32_t i=0;i<sheeps.size();i++)
		{
			if(sheeps[i]==NULL)
			{
				sheeps[i]=s;
				break;
			}
		}
		sheepsLock.unlock();
	}
	bool removeSheep(Sheep* s)
	{
		bool ret=sheepsLock.trylock();
		if(!ret)
			return false;
		for(uint32_t i=0;i<sheeps.size();i++)
		{
			if(sheeps[i]==s)
			{
				sheeps[i]=NULL;
				break;
			}
		}
		sheepsLock.unlock();
		return true;
	}
};

};

extern TLSDATA lightspark::IThreadJob* thisJob;
#endif
