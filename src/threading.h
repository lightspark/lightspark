/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef THREADING_H
#define THREADING_H 1

#include "compat.h"
#include <cstdlib>
#include <cassert>
#include <vector>
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_thread.h>

namespace lightspark
{
class Mutex
{
friend class Cond;
friend class Locker;
	SDL_mutex* m;
public:
	Mutex()
	{
		m = SDL_CreateMutex();
	}
	~Mutex()
	{
		SDL_DestroyMutex(m);
	}
	inline void lock()
	{
		SDL_LockMutex(m);
	}
	inline void unlock()
	{
		SDL_UnlockMutex(m);
	}
	inline bool trylock()
	{
		return SDL_TryLockMutex(m)==0;
	}
};

class Locker
{
	SDL_mutex* m;
public:
	Locker(Mutex& mutex):m(mutex.m)
	{
		SDL_LockMutex(m);
	}
	~Locker()
	{
		SDL_UnlockMutex(m);
	}
	inline void acquire()
	{
		SDL_LockMutex(m);
	}
	inline void release()
	{
		SDL_UnlockMutex(m);
	}
};
class Cond
{
	SDL_cond* c;
public:
	Cond()
	{
		c= SDL_CreateCond();
	}
	~Cond()
	{
		SDL_DestroyCond(c);
	}
	inline void broadcast()
	{
		SDL_CondBroadcast(c);
	}
	inline void wait(Mutex& mutex)
	{
		SDL_CondWait(c,mutex.m);
	}
	inline void signal()
	{
		SDL_CondSignal(c);
	}
	inline bool wait_until(Mutex& mutex,uint32_t ms)
	{
		return SDL_CondWaitTimeout(c,mutex.m,ms)==0;
	}
};

#define DEFINE_AND_INITIALIZE_TLS(name) static SDL_TLSID name = SDL_TLSCreate()
void tls_set(SDL_TLSID key, void* value);
void* tls_get(SDL_TLSID key);

class DLL_PUBLIC Semaphore
{
private:
	SDL_sem * sem;
public:
	Semaphore(uint32_t init);
	~Semaphore();
	void signal();
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

template<class T>
class BlockingCircularQueue
{
private:
	T* queue;
	//Counting semaphores for the queue
	Semaphore freeBuffers;
	Semaphore usedBuffers;
	uint32_t bufferHead;
	uint32_t bufferTail;
	uint32_t size;
	ACQUIRE_RELEASE_FLAG(empty);
public:
	BlockingCircularQueue(uint32_t _size):freeBuffers(_size),usedBuffers(0),bufferHead(0),bufferTail(0),size(_size),empty(true)
	{
		aligned_malloc((void**)&queue,16,size*sizeof(T));
		for(uint32_t i=0;i<size;i++)
			queue[i].init();
	}
	~BlockingCircularQueue()
	{
		for(uint32_t i=0;i<size;i++)
			queue[i].cleanup();
		aligned_free(queue);
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

// This class represents the end time when waiting on a conditional
// variable.
class CondTime {
private:
	gint64 timepoint;
public:
	CondTime(long milliseconds);
	bool operator<(const CondTime& c) const;
	bool operator>(const CondTime& c) const;
	bool isInTheFuture() const;
	void addMilliseconds(long ms);
	bool wait(Mutex &mutex, Cond& cond);
};
}

#endif /* THREADING_H */
