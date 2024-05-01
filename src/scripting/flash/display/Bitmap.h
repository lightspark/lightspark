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

#ifndef SCRIPTING_FLASH_DISPLAY_BITMAP_H
#define SCRIPTING_FLASH_DISPLAY_BITMAP_H 1

#include "scripting/flash/display/DisplayObject.h"

namespace lightspark
{
class BitmapData;

class IntSize
{
	public:
	uint32_t width;
	uint32_t height;
	IntSize(uint32_t w, uint32_t h):width(w),height(h){}
};

class Bitmap: public DisplayObject
{
friend class CairoTokenRenderer;
private:
	void onBitmapData(_NR<BitmapData>);
	void onSmoothingChanged(bool);
	void onPixelSnappingChanged(tiny_string snapping);
public:
	ASPROPERTY_GETTER_SETTER(_NR<BitmapData>,bitmapData);
	ASPROPERTY_GETTER_SETTER(bool, smoothing);
	ASPROPERTY_GETTER_SETTER(tiny_string,pixelSnapping);
	/* Call this after updating any member of 'data' */
	void updatedData();
	Bitmap(ASWorker* wrk, Class_base* c, _NR<LoaderInfo> li=NullRef, std::istream *s = NULL, FILE_TYPE type=FT_UNKNOWN);
	Bitmap(ASWorker* wrk, Class_base* c, _R<BitmapData> data, bool startupload=true);
	~Bitmap();
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	void setOnStage(bool staged, bool force, bool inskipping=false) override;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	void refreshSurfaceState() override;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
	virtual IntSize getBitmapSize() const;
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	IDrawable* invalidate(bool smoothing) override;
	void invalidateForRenderToBitmap(RenderDisplayObjectToBitmapContainer* container) override;
};

}
#endif /* SCRIPTING_FLASH_DISPLAY_BITMAP_H */
