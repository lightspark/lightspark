/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2024, 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef BACKENDS_BITMAP_BITMAP_DATA_H
#define BACKENDS_BITMAP_BITMAP_DATA_H 1

#include <set>
#include <utility>

#include "backends/geometry.h"
#include "interfaces/backends/bitmap/IBitmapDrawable.h"
#include "smartrefs.h"
#include "utils/optional.h"
#include "utils/span.h"

namespace lightspark
{

class Bitmap;
class BitmapContainer;
class DisplayObject;

class BitmapData : public IBitmapDrawable
{
private:
	_NR<BitmapContainer> pixels;
	_NR<Bitmap> temporaryBitmap;
	size_t locked;
	bool needsupload;
	bool transparent;
	//Avoid cycles by not using automatic references
	//Bitmap will take care of removing itself when needed
	std::set<std::reference_wrapper<Bitmap>> users;
	void notifyUsers();
public:
	BitmapData();
	BitmapData(_R<BitmapContainer> _pixels);
	BitmapData(const BitmapData& other);
	BitmapData(const Vector2u& size);
	_NR<BitmapContainer> getBitmapContainer() const { return pixels; }
	Bitmap* getRenderCallBitmap();
	uint32_t getWidth() const { return getSize().x; }
	uint32_t getHeight() const { return getSize().y; }
	const Vector2u& getSize() const
	{
		return !pixels.isNull() ? pixels->getSize() : Vector2u();
	}

	void addUser(Bitmap* b, bool startUpload = true);
	void removeUser(Bitmap* b);
	void checkForUpload();
	/*
	 * Utility method to draw a DisplayObject on the surface
	 */
	void drawDisplayObject
	(
		DisplayObject& obj,
		const MATRIX& initialMatrix,
		bool smoothing,
		const AS_BLENDMODE& blendMode,
		ColorTransform* ct,
		const Rect<Twips>& clipRect,
		bool needsCopy,
		Optional<const RGBA&> fillColor = {},
		uint8_t qualityFactor = 1
	);

	bool isDisposed() const { return pixels.isNull(); }

	void dispose();
	void copyPixels
	(
		_R<BitmapData> source,
		const Rect<int32_t>& srcRect,
		const Vector2& destPoint,
		_R<BitmapData> alphaBitmap,
		const Vector2f& alphaPoint,
		bool mergeAlpha
	);

	void copyPixelsToByteArray();
	void fillRect(const Rect<int32_t>& rect, const RGBA& color);
	bool hitTestBitmap
	(
		const Vector2& point,
		uint8_t threshold,
		_R<BitmapData> other,
		const Vector2& otherPoint,
		uint8_t otherThreshold
	);

	bool hitTestRect
	(
		const Vector2& point,
		uint8_t threshold,
		const Vector2& size
	);

	bool hitTestPoint(const Vector2& point, uint8_t threshold);
	bool isTransparent() const { return transparent; }
	void scroll(const Vector2& pos);
	void copyChannel
	(
		_R<BitmapData> source,
		const Vector2& destPoint,
		const Rect<int32_t>& srcRect,
		size_t srcChannel,
		size_t destChannel
	);

	void lock() { ++locked; }
	void unlock();
	Rect<uint32_t> getColorBoundsRect
	(
		bool findColor,
		uint32_t mask,
		const RGBA& color
	);

	void setPixels
	(
		const Rect<int32_t>& rect,
		Span<uint32_t> data
	);

	void colorTransform
	(
		const Rect<int32_t>& rect,
		const CXFORMWITHALPHA& cxform
	);

	void compare(_R<BitmapData> source);
	void applyFilter
	(
		_R<BitmapData> source,
		const Vector2& destPoint,
		const Rect<int32_t>& srcRect,
		const Filter& filter
	);

	void noise
	(
		uint32_t randomSeed,
		uint8_t low,
		uint8_t high,
		const BitmapChannelOptions& channelOpts,
		bool grayScale
	);

	void perlinNoise
	(
		const Vector2f& base,
		size_t numOctaves,
		uint32_t randomSeed,
		bool stitch,
		bool fractalNoise,
		const BitmapChannelOptions& channelOpts,
		bool grayScale,
		Span<Vector2f> offsets
	);

	size_t getThreshold
	(
		_R<BitmapData> source,
		const Rect<int32_t>& srcRect,
		const Vector2& destPoint,
		const ThresholdOp& op,
		size_t threshold,
		const RGBA& color,
		uint32_t mask,
		bool copySource
	);

	void merge
	(
		_R<BitmapData> source,
		const Vector2& destPoint,
		const Rect<int32_t>& srcRect,
		const RGBA& mult
	);

	void paletteMap
	(
		_R<BitmapData> source,
		const Rect<int32_t>& srcRect,
		const Vector2& destPoint,
		const std::array<uint32_t, 256>& redArray,
		const std::array<uint32_t, 256>& greenArray,
		const std::array<uint32_t, 256>& blueArray,
		const std::array<uint32_t, 256>& alphaArray
	);

	void pixelDissolve
	(
		_R<BitmapData> source,
		const Rect<int32_t>& srcRect,
		const Vector2& destPoint,
		uint32_t randomSeed,
		size_t numPixels,
		const RGBA& fillColor
	);
};

}
#endif /* BACKENDS_BITMAP_BITMAP_DATA_H */
