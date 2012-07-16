/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_DISPLAY_BITMAPCONTAINER_H
#define SCRIPTING_FLASH_DISPLAY_BITMAPCONTAINER_H 1

#include "compat.h"
#include "memory_support.h"
#include <vector>

namespace lightspark
{

class BitmapContainer
{
protected:
	size_t stride;
	size_t dataSize;
	int32_t width;
	int32_t height;
public:
	BitmapContainer(MemoryAccount* m);
	std::vector<uint8_t, reporter_allocator<uint8_t>> data;
	uint8_t* getData() { return &data[0]; }
	const uint8_t* getData() const { return &data[0]; }
	bool fromRGB(uint8_t* rgb, uint32_t width, uint32_t height, bool hasAlpha);
	bool fromJPEG(uint8_t* data, int len);
	bool fromJPEG(std::istream& s);
	bool fromPNG(std::istream& s);
	int getWidth() const { return width; }
	int getHeight() const { return height; }
	void reset();
};

};
#endif /* SCRIPTING_FLASH_DISPLAY_BITMAPCONTAINER_H */
