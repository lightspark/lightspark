/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifdef HAVE_NEW_GLIBMM_THREAD_API
#include <glibmm/threads.h>
#else
#include <glibmm/thread.h>
#endif

namespace lightspark
{

/* Be aware that on win32, both Mutex and RecMutex are recursive! */
#ifdef HAVE_NEW_GLIBMM_THREAD_API
using Glib::Threads::Mutex;
using Glib::Threads::RecMutex;
using Glib::Threads::Cond;
using Glib::Threads::Thread;
typedef Mutex StaticMutex; // GLib::Threads::Mutex can be static
typedef RecMutex StaticRecMutex; // GLib::Threads::RecMutex can be static
#else
using Glib::Mutex;
using Glib::RecMutex;
using Glib::StaticMutex;
using Glib::StaticRecMutex;
using Glib::Cond;
using Glib::Thread;
#endif

typedef Mutex::Lock Locker;
typedef Mutex Spinlock;
typedef Mutex::Lock SpinlockLocker;

class DLL_PUBLIC Semaphore
{
private:
	Mutex mutex;
	Cond cond;
	uint32_t value;
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

class IThreadJob
{
friend class ThreadPool;
public:
	/*
	 * Set to true by the ThreadPool just before threadAbort()
	 * is called. For some implementations, it may be enough
	 * to poll threadAborted and not implement threadAbort().
	 */
	volatile bool threadAborting;
	/*
	 * Called in a dedicated thread to do the actual
	 * work. You may throw a JobTerminationException
	 * to quit the job. (Or better: just return)
	 */
	virtual void execute()=0;
	/*
	 * Called asynchronously to abort a job
	 * who is currently in execute().
	 * 'aborting' is set to true before calling
	 * this function.
	 */
	virtual void threadAbort() {};
	/*
	 * Called after the job has finished execute()'ing or
	 * if the ThreadPool aborts and the job did not have
	 * a chance to run yet.
	 * You should use jobFence to signal blocking semaphores
	 * and a like. There is no access to this object after
	 * jobFence() is called, so you may safely call
	 * 'delete this'.
	 */
	virtual void jobFence()=0;
	IThreadJob() : threadAborting(false) {}
	virtual ~IThreadJob() {}
};

template<class T, uint32_t size>
class BlockingCircularQueue
{
private:
	T queue[size];
	//Counting semaphores for the queue
	Semaphore freeBuffers;
	Semaphore usedBuffers;
	uint32_t bufferHead;
	uint32_t bufferTail;
	bool empty;
public:
	BlockingCircularQueue():freeBuffers(size),usedBuffers(0),bufferHead(0),bufferTail(0),empty(true)
	{
	}
	template<class GENERATOR>
	BlockingCircularQueue(const GENERATOR& g):freeBuffers(size),usedBuffers(0),bufferHead(0),bufferTail(0),empty(true)
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

// This class represents the end time when waiting on a conditional
// variable. It encapsulates the differences between new and old
// glibmm API.
class CondTime {
private:
#ifdef HAVE_NEW_GLIBMM_THREAD_API
	gint64 timepoint;
#else
        Glib::TimeVal timepoint;
#endif
public:
	CondTime(long milliseconds);
	bool operator<(CondTime& c) const;
	bool operator>(CondTime& c) const;
	bool isInTheFuture() const;
	void addMilliseconds(long ms);
	bool wait(Mutex& mutex, Cond& cond);
};
};

#endif /* THREADING_H */
