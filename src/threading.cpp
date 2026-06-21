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

#include <cassert>

#include "threading.h"
#include "exceptions.h"
#include "logger.h"
#include "compat.h"

using namespace lightspark;

void lightspark::tls_set(SDL_TLSID key, void* value)
{
	SDL_TLSSet(key, value,0);
}

void* lightspark::tls_get(SDL_TLSID key)
{
	return SDL_TLSGet(key);
}

Semaphore::Semaphore(uint32_t init)
{
	sem = SDL_CreateSemaphore(init);
}

Semaphore::~Semaphore()
{
	SDL_DestroySemaphore(sem);
}

void Semaphore::wait()
{
	SDL_SemWait(sem);
}

bool Semaphore::try_wait()
{
	return SDL_SemTryWait(sem)==0;
}

void Semaphore::signal()
{
	SDL_SemPost(sem);
}

CondTime::CondTime(long milliseconds)
{
	timepoint=compat_msectiming();
}

bool CondTime::operator<(const CondTime& c) const
{
	return timepoint<c.timepoint;
}

bool CondTime::operator>(const CondTime& c) const
{
	return timepoint>c.timepoint;
}

bool CondTime::isInTheFuture() const
{
	uint64_t now=compat_msectiming();
	return timepoint>now;
}

void CondTime::addMilliseconds(long ms)
{
	timepoint+=ms;
	// don't allow that next timepoint will be in the past
	uint64_t now=compat_msectiming();
	if (timepoint < now)
		timepoint= now + ms;
}

bool CondTime::wait(Mutex& mutex, Cond& cond)
{
	uint64_t now=compat_msectiming();
	return cond.wait_until(mutex, (timepoint > now ? timepoint-now : 0));
}
