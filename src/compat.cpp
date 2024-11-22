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
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <winnt.h> // for Nt{Set,Query}TimerResolution()
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
#endif

using namespace std;
using namespace lightspark;

uint64_t compat_perfcount()
{
#ifdef _WIN32
	LARGE_INTEGER counter;
	const BOOL rc = QueryPerformanceCounter(&counter);
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
	const BOOL rc = QueryPerformanceFrequency(&frequency);
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

int kill_child(GPid childPid)
{
#ifdef _WIN32
	TerminateProcess(childPid, 0);
#else
	kill(childPid, SIGTERM);
	g_spawn_close_pid(childPid);
#endif
	return 0;
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

			pNtQueryTimerResolution NtQueryTimerResolution = (pNtQueryTimerResolution)GetProcAddress(ntdll, "NtQueryTimerResolution");
			pNtSetTimerResolution   NtSetTimerResolution   = (pNtSetTimerResolution)  GetProcAddress(ntdll, "NtSetTimerResolution");
			if (NtQueryTimerResolution != NULL &&
			    NtSetTimerResolution   != NULL)
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
DEFDLLMAIN(pango);
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
	//RUNDLLMAIN(pango); //taken care of by patches from mxe
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

/*
Routine Description:

    This routine appends the given argument to a command line such
    that CommandLineToArgvW will return the argument string unchanged.
    Arguments in a command line should be separated by spaces; this
    function does not add these spaces.

Arguments:

    Argument - Supplies the argument to encode.

    CommandLine - Supplies the command line to which we append the encoded argument string.

    Force - Supplies an indication of whether we should quote
            the argument even if it does not contain any characters that would
            ordinarily require quoting.

    Taken from http://blogs.msdn.com/b/twistylittlepassagesallalike/archive/2011/04/23/everyone-quotes-arguments-the-wrong-way.aspx
*/
void ArgvQuote(const std::string& Argument, std::string& CommandLine, bool Force = false)
{
    //
    // Unless we're told otherwise, don't quote unless we actually
    // need to do so --- hopefully avoid problems if programs won't
    // parse quotes properly
    //

    if (Force == false &&
        Argument.empty () == false &&
        Argument.find_first_of (" \t\n\v\"") == Argument.npos)
    {
        CommandLine.append (Argument);
    }
    else {
        CommandLine.push_back ('"');

        for (auto It = Argument.begin () ; ; ++It) {
            unsigned NumberBackslashes = 0;

            while (It != Argument.end () && *It == '\\') {
                ++It;
                ++NumberBackslashes;
            }

            if (It == Argument.end ()) {

                //
                // Escape all backslashes, but let the terminating
                // double quotation mark we add below be interpreted
                // as a metacharacter.
                //

                CommandLine.append (NumberBackslashes * 2, '\\');
                break;
            }
            else if (*It == '"') {

                //
                // Escape all backslashes and the following
                // double quotation mark.
                //

                CommandLine.append (NumberBackslashes * 2 + 1, '\\');
                CommandLine.push_back (*It);
            }
            else {

                //
                // Backslashes aren't special here.
                //

                CommandLine.append (NumberBackslashes, '\\');
                CommandLine.push_back (*It);
            }
        }

        CommandLine.push_back ('"');
    }
}

#include <fcntl.h>
/*
 * Spawns a process from the given args,
 * returns its process handle and writes
 * the file descriptor of its stdin to
 * stdinfd.
 * Returns NULL on error.
 */
HANDLE compat_spawn(char** arg, int* stdinfd)
{
	/* Properly quote args into a command line */
	string cmdline;
	int i=0;
	while(true)
	{
		ArgvQuote(arg[i],cmdline);
		if(arg[++i] == NULL)
			break;
		else
			cmdline += " ";
	}

	/* This is taken from http://msdn.microsoft.com/en-us/library/windows/desktop/ms682499%28v=vs.85%29.aspx */
	SECURITY_ATTRIBUTES saAttr;

	HANDLE hChildStdin_Rd = NULL;
	HANDLE hChildStdin_Wr = NULL;

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if(!CreatePipe(&hChildStdin_Rd, &hChildStdin_Wr, &saAttr, 0))
	{
		LOG(LOG_ERROR,"CreatePipe");
		return NULL;
	}

	// Ensure the write handle to the pipe for STDIN is not inherited.
	if(!SetHandleInformation(hChildStdin_Wr, HANDLE_FLAG_INHERIT, 0))
	{
		LOG(LOG_ERROR,"SetHandleInformation");
		return NULL;
	}

	// Create the child process.
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;

	// Set up members of the PROCESS_INFORMATION structure.
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

	// Set up members of the STARTUPINFO structure.
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdInput = hChildStdin_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process.
	if(!CreateProcess(NULL,
			(LPSTR)cmdline.c_str(),     // command line
			NULL,          // process security attributes
			NULL,          // primary thread security attributes
			TRUE,          // handles are inherited
			0,             // creation flags
			NULL,          // use parent's environment
			NULL,          // use parent's current directory
			&siStartInfo,  // STARTUPINFO pointer
			&piProcInfo))  // receives PROCESS_INFORMATION
	{
		LOG(LOG_ERROR,"CreateProcess");
		return NULL;
	}

	CloseHandle(piProcInfo.hThread);

	*stdinfd = _open_osfhandle((intptr_t)hChildStdin_Wr, _O_RDONLY);
	if(*stdinfd == -1)
	{
		LOG(LOG_ERROR,"_open_osfhandle");
		return NULL;
	}
#if 0
	HANDLE hInputFile = NULL;
	/* Write file to pipe */
	hInputFile = CreateFile(
			stdinfile,
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_READONLY,
			NULL);

	if(hInputFile == INVALID_HANDLE_VALUE )
	{
		LOG(LOG_ERROR,"CreateFile");
		return NULL;
	}

	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE];
	BOOL bSuccess;

	while(true)
	{
	  bSuccess = ReadFile(hInputFile, chBuf, BUFSIZE, &dwRead, NULL);
	  if(!bSuccess || dwRead == 0 ) break;

	  bSuccess = WriteFile(hChildStdin_Wr, chBuf, dwRead, &dwWritten, NULL);
	  if(!bSuccess) break;
	}

	if(!CloseHandle(hChildStdin_Wr))
		LOG(LOG_ERROR,"StdInWr CloseHandle");
#endif

	return piProcInfo.hProcess;
}

#endif
