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

#ifndef _THREADING_H
#define _THREADING_H

#include <semaphore.h>
#include <stdlib.h>
#include "compat.h"

namespace lightspark
{

class Mutex
{
friend class Locker;
private:
	sem_t sem;
	const char* name;
	uint32_t foundBusy;
	void lock();
	void unlock();
public:
	Mutex(const char* name);
	~Mutex();
};

class IThreadJob
{
friend class ThreadPool;
friend class Condition;
private:
	sem_t terminated; 
protected:
	bool executing;
	bool aborting;
	virtual void execute()=0;
	virtual void threadAbort()=0;
public:
	IThreadJob();
	virtual ~IThreadJob();
	void run();
	void stop();
};

class Condition
{
private:
	sem_t sem;
	//TODO: use atomic incs and decs
	//uint32_t blocked;
	//uint32_t maxBlocked;
public:
	Condition(uint32_t init);
	~Condition();
	void signal();
	//void signal_all();
	void wait();
	bool try_wait();
};

class Locker
{
private:
	Mutex& _m;
	const char* message;
public:
	Locker(Mutex& m):_m(m)
	{
		_m.lock();
	}
	~Locker()
	{
		_m.unlock();
	}
};

};

extern TLSDATA lightspark::IThreadJob* thisJob;
#endif
