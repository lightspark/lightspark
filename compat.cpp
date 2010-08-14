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

#ifndef WIN32
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#endif

#include "compat.h"
#include <string>
#include <stdlib.h>

using namespace std;

void compat_msleep(unsigned int time)
{
#ifdef WIN32
	Sleep(time);
#else
	usleep(time*1000);
#endif
}

#ifdef WIN32
int round(double f)
{
    return ( f < 0.0 ) ? (int) ( f - 0.5 ) : (int) ( f + 0.5 );
}

long lrint(double f)
{
	return (floor(f+(f>0) ? 0.5 : -0.5));
}

#endif

uint64_t compat_msectiming()
{
#ifdef WIN32
	return GetTickCount64();
#else
	timespec t;
	clock_gettime(CLOCK_MONOTONIC,&t);
	return (t.tv_sec*1000 + t.tv_nsec/1000000);
#endif
}


#ifndef WIN32
#include "timer.h"

uint64_t compat_get_current_time_ms()
{
	timespec tp;
	//Get current clock to schedule next wakeup
	clock_gettime(CLOCK_REALTIME,&tp);
	return lightspark::timespecToMsecs(tp);
}

uint64_t compat_get_thread_cputime_us()
{
	timespec tp;
#ifndef _POSIX_THREAD_CPUTIME
	#error no thread clock available
#endif
	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&tp);
	return lightspark::timespecToUsecs(tp);
}

int aligned_malloc(void **memptr, size_t alignment, size_t size)
{
	return posix_memalign(memptr, alignment, size);
}
void aligned_free(void *mem)
{
	return free(mem);
}

int kill_child(pid_t childPid)
{
	kill(childPid, SIGTERM);
	waitpid(childPid, NULL, 0);
	return 0;
}


#else
#define EPOCH_BIAS  116444736000000000i64 // Number of 100 nanosecond units from 1/1/1601 to 1/1/1970
static uint64_t get_corrected_wintime()
{
	// Gets time in 100 nanosecond units
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER ret;
	ret.HighPart = ft.dwHighDateTime;
	ret.LowPart = ft.dwLowDateTime;

	// Compensates epoch difference
	return ret.QuadPart - EPOCH_BIAS;
}
uint64_t compat_get_current_time_ms()
{
	// Note: we could also use _ftime here
	uint64_t ret = get_corrected_wintime();
	return ret / 10000i64;
}
uint64_t compat_get_current_time_us()
{
	uint64_t ret = get_corrected_wintime();
	return ret / 10i64;
}

uint64_t compat_get_thread_cputime_us()
{
	// WINTODO: On Vista/7 we might want to use the processor cycles API
	FILETIME CreationTime, ExitTime, KernelTime, UserTime;
	GetThreadTimes(GetCurrentThread(), &CreationTime, &ExitTime, &KernelTime, &UserTime);

	uint64_t ret;
	ULARGE_INTEGER u;
	u.HighPart = KernelTime.dwHighDateTime;
	u.LowPart = KernelTime.dwLowDateTime;
	ret = u.QuadPart / 10i64;
	u.HighPart = UserTime.dwHighDateTime;
	u.LowPart = UserTime.dwLowDateTime;
	ret += u.QuadPart / 10i64;
	return ret;
}

int kill_child(pid_t pid)
{
	DebugBreak();
	return -1;
}

#endif
