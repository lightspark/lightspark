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

#ifndef WIN32
#include <unistd.h>
#include <time.h>
#endif

#include "compat.h"

#ifdef WIN32
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN __declspec(noreturn)
#endif

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
