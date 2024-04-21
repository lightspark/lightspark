/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

class BitmapData: public ASObject, public IBitmapDrawable
{
private:
	_NR<BitmapContainer> pixels;
	int locked;
	bool needsupload;
	//Avoid cycles by not using automatic references
	//Bitmap will take care of removing itself when needed
	std::set<Bitmap*> users;
	void notifyUsers();
public:
	BitmapData(ASWorker* wrk,Class_base* c);
	BitmapData(ASWorker* wrk,Class_base* c, _R<BitmapContainer> b);
	BitmapData(ASWorker* wrk, Class_base* c, const BitmapData& other);
	BitmapData(ASWorker* wrk,Class_base* c, uint32_t width, uint32_t height);
	static void sinit(Class_base* c);
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	_NR<BitmapContainer> getBitmapContainer() const { return pixels; }
	int getWidth() const { return pixels->getWidth(); }
	int getHeight() const { return pixels->getHeight(); }
	void addUser(Bitmap* b, bool startupload=true);
	void removeUser(Bitmap* b);
	void checkForUpload();
	/*
	 * Utility method to draw a DisplayObject on the surface
	 */
	void drawDisplayObject(DisplayObject* d, const MATRIX& initialMatrix, bool smoothing, AS_BLENDMODE blendMode, ColorTransformBase* ct);
	ASPROPERTY_GETTER(bool, transparent);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(dispose);
	ASFUNCTION_ATOM(draw);
	ASFUNCTION_ATOM(drawWithQuality);
	ASFUNCTION_ATOM(getPixel);
	ASFUNCTION_ATOM(getPixel32);
	ASFUNCTION_ATOM(setPixel);
	ASFUNCTION_ATOM(setPixel32);
	ASFUNCTION_ATOM(getRect);
	ASFUNCTION_ATOM(copyPixels);
	ASFUNCTION_ATOM(fillRect);
	ASFUNCTION_ATOM(generateFilterRect);
	ASFUNCTION_ATOM(hitTest);
	ASFUNCTION_ATOM(_getWidth);
	ASFUNCTION_ATOM(_getHeight);
	ASFUNCTION_ATOM(scroll);
	ASFUNCTION_ATOM(clone);
	ASFUNCTION_ATOM(copyChannel);
	ASFUNCTION_ATOM(lock);
	ASFUNCTION_ATOM(unlock);
	ASFUNCTION_ATOM(floodFill);
	ASFUNCTION_ATOM(histogram);
	ASFUNCTION_ATOM(getColorBoundsRect);
	ASFUNCTION_ATOM(getPixels);
	ASFUNCTION_ATOM(getVector);
	ASFUNCTION_ATOM(setPixels);
	ASFUNCTION_ATOM(setVector);
	ASFUNCTION_ATOM(colorTransform);
	ASFUNCTION_ATOM(compare);
	ASFUNCTION_ATOM(applyFilter);
	ASFUNCTION_ATOM(noise);
	ASFUNCTION_ATOM(perlinNoise);
	ASFUNCTION_ATOM(threshold);
	ASFUNCTION_ATOM(merge);
	ASFUNCTION_ATOM(paletteMap);
};

}
#endif /* SCRIPTING_FLASH_DISPLAY_BITMAPDATA_H */
