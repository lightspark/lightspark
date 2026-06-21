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


#include "compat.h"
#include <string>
#include <cstdlib>
#include "logger.h"
#include <unistd.h>
#include "utils/timespec.h"

#ifdef _WIN32
#	include <winsdkver.h>
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	define WIN32_LEAN_AND_MEAN
#	define WINVER _WIN32_WINNT_VISTA
#	define _WIN32_WINNT _WIN32_WINNT_VISTA
#	include <windows.h>
#	include <winnt.h> // for Nt{Set,Query}TimerResolution()
#	include <synchapi.h> // for `CreateWaitableTimerEx()`
#	include <versionhelpers.h> // for `IsWindowsVistaOrGreater()`
#	undef DOUBLE_CLICK
#	undef RGB
#	undef VOID
#	ifndef PATH_MAX
#		define PATH_MAX 260
#	endif
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>   // for nanosleep
#else
#include <unistd.h> // for usleep
#endif
#ifndef _WIN32
#include <errno.h> // for errno
#include <signal.h> // for kill
#endif

using namespace std;
using namespace lightspark;

uint64_t compat_perfcount()
{
#ifdef _WIN32
	LARGE_INTEGER counter;
	(void)QueryPerformanceCounter(&counter);
	return (uint64_t)counter.QuadPart;
#else
	timespec t;
	clock_gettime(CLOCK_MONOTONIC,&t);
	return (t.tv_sec*1000000000 + t.tv_nsec);
#endif
}

uint64_t compat_perffreq()
{
#ifdef _WIN32
	LARGE_INTEGER frequency;
	(void)QueryPerformanceFrequency(&frequency);
	return (uint64_t)frequency.QuadPart;
#else
	return 1000000000;
#endif
}

uint64_t compat_msectiming()
{
	return compat_now().toMs();
}

uint64_t compat_usectiming()
{
	return compat_now().toUs();
}

uint64_t compat_nsectiming()
{
	return compat_now().toNs();
}

TimeSpec compat_now()
{
#ifdef _WIN32
	// Based on the Windows version of Rust's `Instant::now()`.
	uint64_t counter = compat_perfcount();
	uint64_t frequency = compat_perffreq();
	const uint64_t q = counter / frequency;
	const uint64_t r = counter % frequency;
	return TimeSpec(q, r * TimeSpec::nsPerSec / frequency);
#else
	timespec t;
	clock_gettime(CLOCK_MONOTONIC,&t);
	return TimeSpec(t.tv_sec, t.tv_nsec);
#endif
}

#ifdef WIN32
// Based on code from https://github.com/TurtleMan64/usleep-windows
static void initTimerRes()
{
	static int init = 0;

	if (init == 0)
	{
		init = 1;

		// Increase the accuracy of the timer once.
		const HINSTANCE ntdll = LoadLibrary("ntdll.dll");
		if (ntdll != NULL)
		{
			typedef long(NTAPI* pNtQueryTimerResolution)(unsigned long* MinimumResolution, unsigned long* MaximumResolution, unsigned long* CurrentResolution);
			typedef long(NTAPI* pNtSetTimerResolution)(unsigned long RequestedResolution, char SetResolution, unsigned long* ActualResolution);

			pNtQueryTimerResolution NtQueryTimerResolution = (pNtQueryTimerResolution)(void*)GetProcAddress(ntdll, "NtQueryTimerResolution");
			pNtSetTimerResolution NtSetTimerResolution = (pNtSetTimerResolution)(void*)GetProcAddress(ntdll, "NtSetTimerResolution");
			if (NtQueryTimerResolution != nullptr && NtSetTimerResolution != nullptr)
			{
				// Query for the highest accuracy timer resolution.
				unsigned long minimum, maximum, current;
				NtQueryTimerResolution(&minimum, &maximum, &current);

				// Set the timer resolution to the highest.
				NtSetTimerResolution(maximum, (char)1, &current);
			}

			// We can decrement the internal reference count by one
			// and NTDLL.DLL still remains loaded in the process.
			FreeLibrary(ntdll);
		}
	}
}
#endif

void compat_msleep(unsigned int time)
{
#ifdef WIN32
	Sleep(time);
#elif _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	struct timespec rem;
	ts.tv_sec = time / 1000;
	ts.tv_nsec = (time % 1000) * 1000000;
	int rc;
	do
	{
		rc = nanosleep(&ts, &rem);
		ts = rem;
	} while (rc && errno == EINTR);
#else
	usleep(time * 1000);
#endif
}

void compat_usleep(uint64_t us)
{
#ifdef WIN32
	LARGE_INTEGER period;
	initTimerRes();
	period.QuadPart = -us*10;
	HANDLE timer;
	if (IsWindowsVistaOrGreater())
		timer = CreateWaitableTimerEx(NULL, NULL, 2, TIMER_ALL_ACCESS);
	else
		timer = CreateWaitableTimer(NULL, false, NULL);
	SetWaitableTimer(timer, &period, 0, NULL, NULL, false);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
#elif _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	struct timespec rem;
	ts.tv_sec = us / 1000000;
	ts.tv_nsec = (us % 1000000) * 1000;
	int rc;
	do
	{
		rc = nanosleep(&ts, &rem);
		ts = rem;
	} while (rc && errno == EINTR);
#else
	usleep(us);
#endif
}

void compat_nsleep(uint64_t ns)
{
#ifdef WIN32
	LARGE_INTEGER period;
	initTimerRes();
	period.QuadPart = -ns/100;
	HANDLE timer;
	if (IsWindowsVistaOrGreater())
		timer = CreateWaitableTimerEx(NULL, NULL, 2, TIMER_ALL_ACCESS);
	else
		timer = CreateWaitableTimer(NULL, false, NULL);
	SetWaitableTimer(timer, &period, 0, NULL, NULL, false);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
#elif _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	struct timespec rem;
	ts.tv_sec = ns / 1000000000;
	ts.tv_nsec = (ns % 1000000000);
	int rc;
	do
	{
		rc = nanosleep(&ts, &rem);
		ts = rem;
	} while (rc && errno == EINTR);
#else
	usleep(ns/1000);
#endif
}

#ifndef _WIN32
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

void aligned_malloc(void **memptr, size_t alignment, size_t size)
{
	if(posix_memalign(memptr, alignment, size))
		throw std::bad_alloc();
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
DLL_PUBLIC HINSTANCE g_hinstance = NULL;
#define DEFDLLMAIN(x) extern "C" BOOL WINAPI x##_DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
DEFDLLMAIN(gio);
DEFDLLMAIN(glib);
DEFDLLMAIN(atk);
DEFDLLMAIN(gdk);
DEFDLLMAIN(gtk);
DEFDLLMAIN(cairo);

#define RUNDLLMAIN(x) x##_DllMain(hinstDLL, fdwReason, lpvReserved)
extern "C"
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	//RUNDLLMAIN(gio); //taken care of by patches from mxe
	//RUNDLLMAIN(glib); //taken care of by patches from mxe
	//RUNDLLMAIN(cairo); //taken care of by patches from mxe
	//RUNDLLMAIN(atk); //taken care of by patches from mxe
	//RUNDLLMAIN(gdk); //taken care of by patches from mxe
	//RUNDLLMAIN(gtk); //taken care of by patches from mxe
	return TRUE;
}

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
