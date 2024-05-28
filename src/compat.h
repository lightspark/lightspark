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

#ifndef COMPAT_H
#define COMPAT_H 1

#include <cstddef>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <cmath>

#if _MSC_VER
	#define FORCE_INLINE __forceinline
	#define USUALLY_TRUE(x) (x)
	#define USUALLY_FALSE(x) (x)
#elif __GNUC__
	#define FORCE_INLINE inline __attribute__((always_inline))
	#define USUALLY_TRUE(x) __builtin_expect(!!(x), true)
	#define USUALLY_FALSE(x) __builtin_expect(!!(x), false)
#else
	#define FORCE_INLINE inline
	#define USUALLY_TRUE(x) (x)
	#define USUALLY_FALSE(x) (x)
#endif

#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

#ifdef _WIN32
	#include <sys/types.h> //for ssize_t
	#include <io.h> //for close(), unlink()
#endif


#include <glib.h>
#include <cstdlib>


#if defined(__GNUC__) && defined(_WIN32)
/* There is a bug in mingw preventing those from being declared */
extern "C" {
_CRTIMP int __cdecl __MINGW_NOTHROW	_stricmp (const char*, const char*);
_CRTIMP int __cdecl __MINGW_NOTHROW	_strnicmp (const char*, const char*, size_t);
_CRTIMP int __cdecl __MINGW_NOTHROW _close (int);
_CRTIMP void * __cdecl __MINGW_NOTHROW _aligned_malloc (size_t, size_t);
_CRTIMP void __cdecl __MINGW_NOTHROW _aligned_free (void*);
_CRTIMP char* __cdecl __MINGW_NOTHROW   _strdup (const char*) __MINGW_ATTRIB_MALLOC;
}
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define strdup _strdup
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
	inline void aligned_malloc(void **memptr, std::size_t alignment, std::size_t size)
	{
		*memptr = _aligned_malloc(size, alignment);
		if(!*memptr)
			throw std::bad_alloc();
	}
	inline void aligned_free(void *mem)
	{
		_aligned_free(mem);
	}
#else
	void aligned_malloc(void **memptr, std::size_t alignment, std::size_t size);
	void aligned_free(void *mem);
#endif

#ifndef _WIN32
#	define CALLBACK
#endif

//Support both atomic header ( gcc >= 4.6 ), and earlier ( stdatomic.h )
#ifdef HAVE_ATOMIC
#	include <atomic>
#else
#	include <cstdatomic>
#endif

#define ATOMIC_INT32(x) std::atomic<int32_t> x
#define ATOMIC_INCREMENT(x) (x.fetch_add(1)+1)
#define ATOMIC_DECREMENT(x) (x.fetch_sub(1)-1)
#define ATOMIC_ADD(x, v) (x.fetch_add(v)+v)
#define ATOMIC_SUB(x, v) (x.fetch_sub(v)-v)

//Boolean type with acquire release barrier semantics
#define ACQUIRE_RELEASE_FLAG(x) std::atomic_bool x
#define ACQUIRE_RELEASE_VARIABLE(t, x) std::atomic<t> x
#define ACQUIRE_READ(x) x.load(std::memory_order_acquire)
#define RELEASE_WRITE(x, v) x.store(v, std::memory_order_release)


/* DLL_LOCAL / DLL_PUBLIC */
/* When building on win32, DLL_PUBLIC is set to __declspec(dllexport)
 * during build of the audio plugins.
 * The browser plugin uses its own definitions from npapi.
 * And the liblightspark.dll is linked directly (without need for dllexport)
 */
#ifndef DLL_PUBLIC
#ifdef _WIN32
#	define DLL_PUBLIC __declspec(dllexport)
#	define DLL_LOCAL
#elif __GNUC__ >= 4
#	define DLL_PUBLIC __attribute__ ((visibility("default")))
#	define DLL_LOCAL  __attribute__ ((visibility("hidden")))
#else
#	define DLL_PUBLIC
#	define DLL_LOCAL
#endif
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

/* gcd */
template<class T>
inline T gcdTmpl(T a, T b)
{
	return !b ? a : gcdTmpl(b, a % b);
}
#define igcd gcdTmpl<int>
#define dgcd gcdTmpl<double>

/* timing */

uint64_t compat_perfcount();
uint64_t compat_perffreq();
uint64_t compat_msectiming();
uint64_t compat_usectiming();
uint64_t compat_nsectiming();
void compat_msleep(unsigned int time);
void compat_usleep(uint64_t us);
void compat_nsleep(uint64_t ns);
uint64_t compat_get_thread_cputime_us();

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

/* spawning */
#ifdef _WIN32
/* returns the path of the current executable */
const char* getExectuablePath();
typedef void* HANDLE;
HANDLE compat_spawn(char** args, int* stdinfd);
#endif

int kill_child(GPid p);

#endif /* COMPAT_H */
