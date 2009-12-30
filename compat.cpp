/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

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
#endif

#include "compat.h"

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
