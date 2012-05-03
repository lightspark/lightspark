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

#ifndef _BITMAP_DATA_H
#define _BITMAP_DATA_H

#include "IBitmapDrawable.h"
#include "asobject.h"

namespace lightspark
{

class BitmapData: public ASObject, public IBitmapDrawable
{
protected:
	size_t stride;
	size_t dataSize;
	bool disposed;
	uint32_t getPixelPriv(uint32_t x, uint32_t y);
	void setPixelPriv(uint32_t x, uint32_t y, uint32_t color, bool setAlpha);
	void copyFrom(BitmapData *source);
public:
	BitmapData(Class_base* c);
	static void sinit(Class_base* c);
	~BitmapData();
	/* the bitmaps data in premultiplied, native-endian 32 bit
	 * ARGB format. stride is the number of bytes per row, may be
	 * larger than width. dataSize is the total allocated size of
	 * data (=stride*height) */
	std::vector<uint8_t, reporter_allocator<uint8_t>> data;
	uint8_t* getData() { return &data[0]; }
	ASPROPERTY_GETTER(int32_t, width);
	ASPROPERTY_GETTER(int32_t, height);
	ASPROPERTY_GETTER(bool, transparent);
	ASFUNCTION(_constructor);
	ASFUNCTION(dispose);
	ASFUNCTION(draw);
	ASFUNCTION(getPixel);
	ASFUNCTION(getPixel32);
	ASFUNCTION(setPixel);
	ASFUNCTION(setPixel32);
	ASFUNCTION(getRect);
	ASFUNCTION(copyPixels);
	ASFUNCTION(fillRect);
	ASFUNCTION(generateFilterRect);
	bool fromRGB(uint8_t* rgb, uint32_t width, uint32_t height, bool hasAlpha);
	bool fromJPEG(uint8_t* data, int len);
	bool fromJPEG(std::istream& s);
	bool fromPNG(std::istream& s);
	int getWidth() const { return width; }
	int getHeight() const { return height; }
};

};
#endif
