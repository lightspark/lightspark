/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef DISPLAY_OBJECT_BITMAP_H
#define DISPLAY_OBJECT_BITMAP_H 1

#include <atomic>

#include "backends/geometry.h"
#include "display_object/DisplayObject.h"
#include "graphics/TokenContainer.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "utils/optional.h"

// Based on Ruffle's `display_object::Bitmap`.

namespace lightspark
{

class BitmapContainer;
class BitmapData;
class IDrawable;
class SWFMovie;

enum class PixelSnapping
{
	Always,
	Auto,
	Never,
};

struct IntSize
{
	uint32_t width;
	uint32_t height;
};

class Bitmap : public DisplayObject, public TokenContainer
{
friend class CairoTokenRenderer;
private:
	SWFMovie& movie;
	Vector2 size;
	FILLSTYLE fillStyle;
	tokensVector bitmapTokens;
	_NR<BitmapContainer> bitmapContainer;
	std::atomic_flag usedInRenderCall;

	_NR<BitmapData> bitmapData;
	bool smoothing;
	PixelSnapping pixelSnapping;

	void setupTokens();
public:
	/* Call this after updating any member of 'data' */
	void updatedData();

	Bitmap
	(
		SystemState* sys,
		SWFMovie& _movie,
		_R<BitmapData> data,
		Optional<const tiny_string&> name = {},
		bool startUpload = true,
	);

	Bitmap
	(
		SystemState* sys,
		SWFMovie& _movie,
		Optional<const tiny_string&> name = {},
		std::istream* s = nullptr,
		FILE_TYPE fileType = FT_UNKNOWN
	);

	Bitmap
	(
		SystemState* sys,
		SWFMovie& _movie,
		Optional<const tiny_string&> name = {}
	);

	void refreshSurfaceState() override;
	Optional<Rect<Twips>> tryBoundsRect(bool visibleOnly) override;
	bool hitTestShape
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		const HitTestFlags& flags
	) override;

	virtual IntSize getBitmapSize() const;
	void requestInvalidation
	(
		InvalidateQueue* q,
		bool forceTextureRefresh = false
	) override;

	IDrawable* invalidate(bool smoothing) override;

	const Vector2& getSize() const { return size; }
	void setSize(const Vector2& _size) { size = _size; }
	void setSize(int32_t width, int32_t height)
	{
		size = Vector2(width, height);
	}

	_NR<BitmapContainer> getContainer() const { return bitmapContainer; }
	_NR<BitmapData> getBitmapData() const { return bitmapData; }
	void setBitmapData(_NR<BitmapData> data);
	bool getSmoothing() const { return smoothing; }
	void setSmoothing(bool _smoothing);
	const PixelSnapping getPixelSnapping() const { return pixelSnapping; }
	void setPixelSnapping(const PixelSnapping& snapping)
	{
		pixelSnapping = snapping;
	}

	void setupRenderCallBitmap(BitmapData& data);
	bool setRenderCall() { return usedInRenderCall.test_and_set(); }
	void resetRenderCall()
	{
		usedInRenderCall.clear();
		scrollRect.reset();
	}
};

}
#endif /* DISPLAY_OBJECT_BITMAP_H */
