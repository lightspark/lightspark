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

#include <cassert>

#include "threading.h"
#include "exceptions.h"
#include "logger.h"
#include "compat.h"

using namespace lightspark;

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

#ifdef HAVE_NEW_GLIBMM_THREAD_API

CondTime::CondTime(long milliseconds)
{
	timepoint=g_get_monotonic_time()+milliseconds*G_TIME_SPAN_MILLISECOND;
}

bool CondTime::operator<(CondTime& c) const
{
	return timepoint<c.timepoint;
}

bool CondTime::operator>(CondTime& c) const
{
	return timepoint>c.timepoint;
}

bool CondTime::isInTheFuture() const
{
	gint64 now=g_get_monotonic_time();
	return timepoint>now;
}

void CondTime::addMilliseconds(long ms)
{
	timepoint+=(gint64)ms*G_TIME_SPAN_MILLISECOND;
}

bool CondTime::wait(Mutex& mutex, Cond& cond)
{
	return cond.wait_until(mutex,timepoint);
}

#else // HAVE_NEW_GLIBMM_THREAD_API

CondTime::CondTime(long milliseconds)
{
	timepoint.assign_current_time();
	timepoint.add_milliseconds(milliseconds);
}

bool CondTime::operator<(CondTime& c) const
{
	return timepoint<c.timepoint;
}

bool CondTime::operator>(CondTime& c) const
{
	return timepoint>c.timepoint;
}

bool CondTime::isInTheFuture() const
{
	Glib::TimeVal now;
	now.assign_current_time();
	return (now-timepoint).negative(); // timepoint > now
}

void CondTime::addMilliseconds(long ms)
{
	timepoint.add_milliseconds(ms);
}

bool CondTime::wait(Mutex& mutex, Cond& cond)
{
	return cond.timed_wait(mutex,timepoint);
}

#endif // HAVE_NEW_GLIBMM_THREAD_API
