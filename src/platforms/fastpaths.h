/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef PLATFORMS_FASTPATHS_H
#define PLATFORMS_FASTPATHS_H 1

#include "compat.h"
#include <cinttypes>

namespace lightspark
{

/**
	Packing of YUV channels in a single buffer (YUVA). Full aligned version

	@param y Planar Y buffer
	@param u Planar U buffer
	@param u Planar V buffer
	@param out Destination YUV0 buffer
	@param width Frame width in pixels
	@param height Frame width in pixels
	@param All the buffers must be 16 bytes aligned.
*/
void fastYUV420ChannelsToYUV0Buffer(uint8_t* y, uint8_t* u, uint8_t* v, uint8_t* out, uint32_t width, uint32_t height);

};
#endif /* PLATFORMS_FASTPATHS_H */
