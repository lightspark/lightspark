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
class BitmapRef;
class BitmapContainer;
class DisplayObject;
class SystemState;

class BitmapData : public IBitmapDrawable
{
private:
	SystemState* sys;
	_NR<BitmapContainer> pixels;
	_NR<BitmapRef> temporaryBitmap;
	size_t locked;
	bool needsUpload;
	bool transparent;
	//Avoid cycles by not using automatic references
	//Bitmap will take care of removing itself when needed
	std::set<std::reference_wrapper<Bitmap>> users;
	void notifyUsers();
public:
	BitmapData(SystemState* _sys, const Vector2u& size = Vectr2u());
	BitmapData(SystemState* _sys, _R<BitmapContainer> _pixels);
	BitmapData(const BitmapData& other);
	_NR<BitmapContainer> getBitmapContainer() const { return pixels; }
	Bitmap* getRenderCallBitmap();
	uint32_t getWidth() const { return getSize().x; }
	uint32_t getHeight() const { return getSize().y; }
	const Vector2u& getSize() const
	{
		return !pixels.isNull() ? pixels->getSize() : Vector2u();
	}

	Rect<uint32_t> getRect() const
	{
		return Rect<uint32_t> { Vector2u(), getSize() };
	}

	void addUser(Bitmap& bitmap, bool startUpload = true);
	void removeUser(Bitmap& bitmap);
	void checkForUpload();
	/*
	 * Utility method to draw a DisplayObject on the surface
	 */
	void drawDisplayObject
	(
		DisplayObject* obj,
		const MATRIX& initialMatrix,
		bool smoothing,
		const AS_BLENDMODE& blendMode,
		Optional<const ColorTransform&> ct,
		Optional<const Rect<Twips>&> clipRect,
		bool needsCopy,
		Optional<const RGBA&> fillColor = {},
		uint8_t qualityFactor = 1
	);

	_NR<BitmapData> clone();

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

	void copyPixelsToBuffer
	(
		const Rect<int32_t>& rect,
		std::vector<uint32_t>& data
	);

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
	void histogram
	(
		Optional<const Rect<int32_t>&> rect
		std::array<std::array<uint32_t, 256>, 4>& arr
	);

	Rect<uint32_t> getColorBoundsRect
	(
		bool findColor,
		uint32_t mask,
		const RGBA& color
	);

	void getPixels
	(
		const Rect<int32_t>& rect,
		std::vector<uint32_t>& data
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

	_NR<BitmapData> compare(_R<BitmapData> source);
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
