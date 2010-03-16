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

#ifndef _FAST_PATHS_H
#define _FAST_PATHS_H

#include <inttypes.h>

extern "C"
{
//Packing of YUV channels in a single buffer (YUVA)
void fastYUV420ChannelsToBuffer(uint8_t* y, uint8_t* u, uint8_t* v, uint8_t* out, uint32_t size);
}

#endif
