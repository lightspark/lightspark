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

#ifndef SCRIPTING_FLASH_DISPLAY_BITMAPDATA_H
#define SCRIPTING_FLASH_DISPLAY_BITMAPDATA_H 1

#include "scripting/flash/display/IBitmapDrawable.h"
#include "asobject.h"
#include "scripting/flash/display/BitmapContainer.h"

namespace lightspark
{

class Bitmap;
class DisplayObject;

class BitmapData: public ASObject, public BitmapContainer, public IBitmapDrawable
{
	bool disposed;
	uint32_t getPixelPriv(uint32_t x, uint32_t y);
	void setPixelPriv(uint32_t x, uint32_t y, uint32_t color, bool setAlpha);
	//Avoid cycles by not using automatic references
	//Bitmap will take care of removing itself when needed
	std::set<Bitmap*> users;
	void notifyUsers() const;
public:
	BitmapData(Class_base* c);
	BitmapData(Class_base* c, const BitmapContainer& b);
	BitmapData(Class_base* c, uint32_t width, uint32_t height);
	static void sinit(Class_base* c);
	~BitmapData();
	/* the bitmaps data in premultiplied, native-endian 32 bit
	 * ARGB format. stride is the number of bytes per row, may be
	 * larger than width. dataSize is the total allocated size of
	 * data (=stride*height) */
	void addUser(Bitmap* b);
	void removeUser(Bitmap* b);
	/*
	 * Utility method to draw a DisplayObject on the surface
	 */
	void drawDisplayObject(DisplayObject* d, const MATRIX& initialMatrix);
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
	ASFUNCTION(hitTest);
	ASFUNCTION(_getter_width);
	ASFUNCTION(_getter_height);
};

};
#endif /* SCRIPTING_FLASH_DISPLAY_BITMAPDATA_H */
