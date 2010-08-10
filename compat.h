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

#ifndef COMPAT_H
#define COMPAT_H

//Define cross platform helpers
// TODO: This should be reworked to use CMake feature detection where possible

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <intrin.h>
#undef min
#undef max
#undef RGB
#undef exception_info // Let's hope MS functions always use _exception_info
#define snprintf _snprintf
#define isnan _isnan

// No real functionality for now
typedef int pid_t;

// WINTODO: Hopefully, the MSVC instrinsics are similar enough
//          to what the standard mandates
#ifdef _MSC_VER
#define ATOMIC_INT32(x) __declspec(align(4)) long x
#define ATOMIC_INCREMENT(x) InterlockedIncrement(&x)
#define ATOMIC_DECREMENT(x) InterlockedDecrement(&x)

#define TLSDATA __declspec( thread )

// Current Windows is always little-endian
#define be64toh(x) _byteswap_uint64(x)

#include <malloc.h>
inline int aligned_malloc(void **memptr, size_t alignment, size_t size)
{
	*memptr = _aligned_malloc(size, alignment);
	return (*memptr != NULL) ? 0: -1;
}
inline void aligned_free(void *mem)
{
	_aligned_free(mem);
}

// Emulate these functions
int round(double f);
long lrint(double f);


// WINTODO: Should be set by CMake?
#define PATH_MAX 260
#define DATADIR "."
#define GNASH_PATH "NONEXISTENT_PATH_GNASH_SUPPORT_DISABLED"

#else
#error At the moment, only Visual C++ is supported on Windows
#endif


#else //GCC
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#define TLSDATA __thread
#define CALLBACK

#include <stdatomic.h>
#define ATOMIC_INT32(x) std::atomic<int32_t> x
#define ATOMIC_INCREMENT(x) x.fetch_add(1)
#define ATOMIC_DECREMENT(x) x.fetch_sub(1)

int aligned_malloc(void **memptr, size_t alignment, size_t size);
void aligned_free(void *mem);
#endif

//Ensure compatibility on various targets
#if defined(__FreeBSD__)
#include <sys/endian.h>
#elif defined(__APPLE__)
#define _BSD_SOURCE
#include <architecture/byte_order.h>
#elif !defined(WIN32)
#include <endian.h>
#endif

#if defined _WIN32 || defined __CYGWIN__
// No DLLs, for now
#   define DLL_PUBLIC
#	define DLL_LOCAL
#else
	#if __GNUC__ >= 4
		#define DLL_PUBLIC __attribute__ ((visibility("default")))
		#define DLL_LOCAL  __attribute__ ((visibility("hidden")))
	#else
		#error GCC version less than 4
	#endif
#endif

inline int imin(int a, int b)
{
	return (a<b)?a:b;
}

inline int imax(int a, int b)
{
	return (a>b)?a:b;
}

inline double dmin(double a,double b)
{
	return (a<b)?a:b;
}

inline double dmax(double a,double b)
{
	return (a>b)?a:b;
}

#include <cstdint>
#include <sys/types.h>
std::uint64_t compat_msectiming();
void compat_msleep(unsigned int time);
std::uint64_t compat_get_current_time_ms();
std::uint64_t compat_get_current_time_us();
std::uint64_t compat_get_thread_cputime_us();

int kill_child(pid_t p);

#endif
