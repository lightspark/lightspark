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

#ifndef COMPAT_H
#define COMPAT_H

//Define cross platform helpers
#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#define TLSDATA __declspec( thread )
#define snprintf _snprintf
int round ( double f_val );
#define isnan(x) _isnan(x)
#else //GCC
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <inttypes.h>
#define TLSDATA __thread
#endif

//Ensure compatibility on various targets
#if defined(__FreeBSD__)
#include <sys/endian.h>
#elif defined(__APPLE__)
#define _BSD_SOURCE
#include <architecture/byte_order.h>
#elif defined(WIN32)
#ifdef _MSC_VER
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
/*
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed int ssize_t;
*/
#endif

#else
#include <endian.h>
#endif

void compat_msleep(unsigned int time);

uint64_t compat_msectiming();

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

#endif
