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

#ifndef COMPAT_H
#define COMPAT_H

// Set BOOST_FILEYSTEM_VERSION to 2 since boost-1.46 defaults to 3
#define BOOST_FILESYSTEM_VERSION 2

#include <stddef.h>
#include <assert.h>
#include <cstdint>
#include <iostream>
// TODO: This should be reworked to use CMake feature detection where possible

/* gettext support */
#include <locale.h>
#include <libintl.h>
#define _(STRING) gettext(STRING)


#include <glib.h>
#include <cstdlib>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
//#include <winsock2.h>
#include <windows.h>
//#include <intrin.h>
#include <io.h>
#undef DOUBLE_CLICK
#undef RGB
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#ifdef __GNUC__
/* There is a bug in mingw preventing those from being declared */
extern "C" {
_CRTIMP int __cdecl __MINGW_NOTHROW	_stricmp (const char*, const char*);
_CRTIMP int __cdecl __MINGW_NOTHROW	_strnicmp (const char*, const char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW _close (int);
_CRTIMP void * __cdecl __MINGW_NOTHROW _aligned_malloc (size_t, size_t);
_CRTIMP void __cdecl __MINGW_NOTHROW _aligned_free (void*);
}
#endif
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#ifdef _MSC_VER
#undef exception_info // Let's hope MS functions always use _exception_info
/* those are C++11 but not available in Visual Studio 2010 */
namespace std
{
	inline double copysign(double x, double y) { return _copysign(x, y); }
	inline bool isnan(double d) { return (bool)_isnan(d); }
	inline int signbit(double arg) { return (int)copysign(1,arg); }
	inline bool isfinite(double d) { return (bool)_finite(d); }
	inline bool isinf(double d) { return !isfinite(d) && !isnan(d); }
}

// Emulate these functions
int round(double f);
long lrint(double f);
#endif

#ifdef __GNUC__
#	ifndef __STDC_LIMIT_MACROS
#		define __STDC_LIMIT_MACROS
#	endif

#	ifndef __STDC_CONSTANT_MACROS
#		define __STDC_CONSTANT_MACROS
#	endif
#endif

/* aligned_malloc */
#ifdef _WIN32
#	include <malloc.h>
	inline int aligned_malloc(void **memptr, std::size_t alignment, std::size_t size)
	{
		*memptr = _aligned_malloc(size, alignment);
		return (*memptr != NULL) ? 0: -1;
	}
	inline void aligned_free(void *mem)
	{
		_aligned_free(mem);
	}
#else
	int aligned_malloc(void **memptr, std::size_t alignment, std::size_t size);
	void aligned_free(void *mem);
#endif

#ifdef _MSC_VER
// WINTODO: Hopefully, the MSVC instrinsics are similar enough
//          to what the standard mandates
#	define ATOMIC_INT32(x) __declspec(align(4)) volatile long x
#	define ATOMIC_INCREMENT(x) InterlockedIncrement(&x)
#	define ATOMIC_DECREMENT(x) InterlockedDecrement(&x)
#	define ACQUIRE_RELEASE_FLAG(x) ATOMIC_INT32(x)
#	define ACQUIRE_READ(x) InterlockedCompareExchange(const_cast<long*>(&x),1,1)
#	define RELEASE_WRITE(x, v) InterlockedExchange(&x,v)
#else //GCC
#ifndef _WIN32
#	define CALLBACK
#endif

//Support both atomic header ( gcc >= 4.6 ), and earlier ( stdatomic.h )
#	ifdef HAVE_ATOMIC
#		include <atomic>
#	else
#		include <cstdatomic>
#	endif

#	define ATOMIC_INT32(x) std::atomic<int32_t> x
#	define ATOMIC_INCREMENT(x) x.fetch_add(1)
#	define ATOMIC_DECREMENT(x) (x.fetch_sub(1)-1)

//Boolean type con acquire release barrier semantics
#	define ACQUIRE_RELEASE_FLAG(x) std::atomic_bool x
#	define ACQUIRE_READ(x) x.load(std::memory_order_acquire)
#	define RELEASE_WRITE(x, v) x.store(v, std::memory_order_release)
#endif


/* DLL_LOCAL / DLL_PUBLIC */
#if defined _WIN32
/* There is no dllexport here, because we use direct linking with mingw */
#	define DLL_PUBLIC
#	define DLL_PUBLIC
#	define DLL_LOCAL
#else
#	if __GNUC__ >= 4
#		define DLL_PUBLIC __attribute__ ((visibility("default")))
#		define DLL_LOCAL  __attribute__ ((visibility("hidden")))
#	else
#		define DLL_PUBLIC
#		define DLL_LOCAL
#	endif
#endif

/* min/max */
template<class T>
inline T minTmpl(T a, T b)
{
	return (a<b)?a:b;
}
template<class T>
inline T maxTmpl(T a, T b)
{
	return (a>b)?a:b;
}
#define imin minTmpl<int>
#define imax maxTmpl<int>
#define dmin minTmpl<double>
#define dmax maxTmpl<double>

/* timing */

uint64_t compat_msectiming();
void compat_msleep(unsigned int time);
uint64_t compat_get_thread_cputime_us();

int kill_child(GPid p);

/* byte order */
#if G_BYTE_ORDER == G_BIG_ENDIAN

inline uint32_t LittleEndianToSignedHost24(uint32_t x)
{
	uint32_t ret=GINT32_FROM_LE(x);
	assert(ret<0x1000000);
	//Sign extend
	if(ret&0x800000)
		ret|=0xff000000;
	return ret;
}

inline uint32_t LittleEndianToUnsignedHost24(uint32_t x)
{
	assert(x<0x1000000);
	uint32_t ret=GINT32_FROM_LE(x);
	return ret;
}

inline uint32_t BigEndianToSignedHost24(uint32_t x)
{
	//Sign extend
	x>>=8;
	assert(x<0x1000000);
	if(x&0x800000)
		x|=0xff000000;
	return x;
}

inline uint32_t BigEndianToUnsignedHost24(uint32_t x)
{
	x>>=8;
	assert(x<0x1000000);
	return x;
}


#else //__BYTE_ORDER == __LITTLE_ENDIAN
inline uint32_t LittleEndianToSignedHost24(uint32_t x)
{
	assert(x<0x1000000);
	if(x&0x800000)
		x|=0xff000000;
	return x;
}

inline uint32_t LittleEndianToUnsignedHost24(uint32_t x)
{
	assert(x<0x1000000);
	return x;
}

inline uint32_t BigEndianToSignedHost24(uint32_t x)
{
	assert(x<0x1000000);
	//Discard the lowest byte, as it was the highest
	uint32_t ret=GINT32_FROM_BE(x)>>8;
	//Sign extend
	if(ret&0x800000)
		ret|=0xff000000;
	return ret;
}

inline uint32_t BigEndianToUnsignedHost24(uint32_t x)
{
	assert(x<0x1000000);
	//Discard the lowest byte, as it was the highest
	uint32_t ret=GINT32_FROM_BE(x)>>8;
	return ret;
}
#endif // __BYTE_ORDER == __BIG_ENDIAN

#endif
