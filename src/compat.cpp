/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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


#include "compat.h"
#include <string>
#include <stdlib.h>

using namespace std;

#ifdef _MSC_VER
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
#ifdef _WIN32
	return GetTickCount(); //TODO: use GetTickCount64
#else
	timespec t;
	clock_gettime(CLOCK_MONOTONIC,&t);
	return (t.tv_sec*1000 + t.tv_nsec/1000000);
#endif
}

int kill_child(GPid childPid)
{
#ifdef _WIN32
	TerminateProcess(childPid, 0);
#else
	kill(childPid, SIGTERM);
#endif
	g_spawn_close_pid(childPid);
	return 0;
}

#ifndef WIN32
#include "timer.h"
uint64_t timespecToUsecs(timespec t)
{
	uint64_t ret=0;
	ret+=(t.tv_sec*1000000LL);
	ret+=(t.tv_nsec/1000LL);
	return ret;
}

uint64_t compat_get_thread_cputime_us()
{
	timespec tp;
#ifndef _POSIX_THREAD_CPUTIME
	#error no thread clock available
#endif
	clock_gettime(CLOCK_THREAD_CPUTIME_ID,&tp);
	return timespecToUsecs(tp);
}

int aligned_malloc(void **memptr, size_t alignment, size_t size)
{
	return posix_memalign(memptr, alignment, size);
}
void aligned_free(void *mem)
{
	return free(mem);
}

#else
uint64_t compat_get_thread_cputime_us()
{
	// WINTODO: On Vista/7 we might want to use the processor cycles API
	FILETIME CreationTime, ExitTime, KernelTime, UserTime;
	GetThreadTimes(GetCurrentThread(), &CreationTime, &ExitTime, &KernelTime, &UserTime);

	uint64_t ret;
	ULARGE_INTEGER u;
	u.HighPart = KernelTime.dwHighDateTime;
	u.LowPart = KernelTime.dwLowDateTime;
	ret = u.QuadPart / 10;
	u.HighPart = UserTime.dwHighDateTime;
	u.LowPart = UserTime.dwLowDateTime;
	ret += u.QuadPart / 10;
	return ret;
}
#endif

#ifdef _WIN32
/* If we are run from standalone, g_hinstance stays NULL.
 * In the plugin, DLLMain sets it to the dll's instance.
 */
HINSTANCE g_hinstance = NULL;
const char* getExectuablePath()
{
	static char path[MAX_PATH] = {0};
	if(!path[0])
	{
		size_t len = GetModuleFileNameA(g_hinstance, path, MAX_PATH);
		if(!len)
			return "";
		char* delim = strrchr(path,'\\');
		if(delim)
			*delim = '\0';
	}
	return path;
}
#endif
