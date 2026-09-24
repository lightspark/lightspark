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

#ifndef BACKENDS_BITMAP_BITMAP_CONTAINER_H
#define BACKENDS_BITMAP_BITMAP_CONTAINER_H 1

#include <list>
#include <queue>
#include <utility>
#include <vector>

#include "backends/colortransformbase.h"
#include "backends/geometry.h"
#include "backends/graphics.h"
#include "compat.h"
#include "memory_support.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "threading.h"
#include "utils/span.h"

namespace lightspark
{

class Bitmap;
class BitmapFilter;
class CachedSurface;
class RenderThread;
class SystemState;

template<typename T>
using RefWrapper = std::reference_wrapper<T>;

struct RenderDisplayObjectToBitmapContainer
{
	MATRIX initialMatrix;
	_NR<CachedSurface> cachedSurface;
	ColorTransformBase ct;
	RECT clipRect;
	RGBA backgroundColor;
	AS_BLENDMODE blendMode;
	uint8_t qualityFactor;
	bool smoothing : 1;
	bool hasClipRect : 1;
	bool needsfill : 1;
	bool needscopy : 1;
};

struct BitmapContainerRenderData
{
	Mutex mutexRenderCallBitmaps;
	std::list<RefWrapper<ITextureUploadable>> uploads;
	std::list<RefreshableSurface> surfacesToRefresh;
	std::queue<RenderDisplayObjectToBitmapContainer> renderCalls;
	std::list<RefWrapper<Bitmap>> renderCallBitmaps;
	bool readPixels : 1;
	bool needsWait : 1;
};

class BitmapContainer : public RefCountable
{
public:
	enum BITMAP_FORMAT { RGB15, RGB24, RGB32, ARGB32 };
protected:
	size_t stride;
	Vector2u size;
	/* the pixel data in premultiplied, native-endian 32 bit
	 * ARGB format. stride is the number of bytes per row, may be
	 * larger than width. */
	std::vector<uint8_t, reporter_allocator<uint8_t>> data;
	// buffer to contain the pixels transformed by latest ColorTransformation
	std::vector<uint8_t> colorTransformedData;
	// color transformation values currently applied to data_colortransformed
	ColorTransform currentColorTransform;
	Span<uint32_t> getDataNoBoundsChecking(const Vector2f& pos);
	Span<uint8_t> getCurrentData();
	void checkModifiedTexture();
	bool hasModifiedData;
	bool hasModifiedTexture;
	bool needsClear;
	bool fromTag;
	ATOMIC_INT32(currentRenderData);
	BitmapContainerRenderData renderData[2];
public:
	BitmapContainerRenderData& getRenderData()
	{
		return renderData[currentRenderData];
	}

	BitmapContainerRenderData& swapRenderData()
	{
		currentRenderData ^= 1;
		return renderData[currentRenderData ^ 1];
	}

	Semaphore renderEvent;
	TextureChunk bitmapTexture;
	int nanoVGImageHandle;
	RGBA nanoVGImageBackgroundColor;
	#ifdef ENABLE_CAIRO
	cairo_pattern_t* cachedCairoPattern;
	#endif

	BitmapContainer(MemoryAccount* m, bool _fromTag = false);
	~BitmapContainer();
	size_t getDataSize() const { return data.size(); }
	Span<uint8_t> getData() { return getCurrentData(); }
	Span<uint8_t> getOriginalData() { return makeSpan(data); }
	Span<uint8_t> getColorTransformedData()
	{
		colorTransformedData.reserve(data.size());
		return makeSpan(colorTransformedData);
	}

	uint8_t* applyColorTransform(const ColorTransform& ct);
	uint8_t* applyColorTransform
	(
		number_t redMulti,
		number_t greenMulti,
		number_t blueMulti,
		number_t alphaMulti,
		number_t redOff,
		number_t greenOff,
		number_t blueOff,
		number_t alphaOff
	);

	// this creates a new byte array that has to be deleted by the caller
	uint8_t* getRectangleData(const Rect<int32_t>& sourceRect);
	bool fromRGB
	(
		uint8_t* rgb,
		const Vector2u& _size,
		const BITMAP_FORMAT& format,
		bool fromPNG = false
	);

	bool fromJPEG(Span<uint8_t> _data, Span<uint8_t> tablesData = {});
	bool fromJPEG(std::istream& s);
	bool fromPNG(std::istream& s);
	bool fromPNG(Span<uint8_t> _data);
	bool fromGIF(SystemState* sys, Span<uint8_t> _data);
	bool fromPalette
	(
		uint8_t* inData,
		const Vector2u& _size,
		size_t inStride,
		uint8_t* palette,
		size_t numColors,
		size_t paletteBPP
	);

	void fromRawData(uint8_t* _data, const Vector2u& _size);
	// Clip sourceRect coordinates to this BitmapContainer. The
	// output coordinates can be used to access pixels in data
	// without out-of-bounds errors.
	Rect<int32_t> clipRect(const Rect<int32_t>& srcRect) const;
	// Clip a rectangle to fit both source and destination
	// bitmaps.
	std::pair<Rect<int32_t>, Vector2> clipRect
	(
		_R<BitmapContainer> source,
		const Rect<int32_t>& srcRect,
		const Vector2& destPoint
	);

	void setAlpha(int32_t x, int32_t y, uint8_t alpha)
	{
		setAlpha(Vector2(x, y), alpha);
	}

	void setAlpha(const Vector2& pos, uint8_t alpha);
	void setPixel
	(
		int32_t x,
		int32_t y,
		uint32_t color,
		bool setAlpha,
		bool isPremultiplied = true
	)
	{
		setPixel
		(
			Vector2(x, y),
			RGBA::fromUInt(color),
			setAlpha,
			isPremultiplied
		);
	}

	void setPixel
	(
		int32_t x,
		int32_t y,
		const RGBA& color,
		bool setAlpha,
		bool isPremultiplied = true
	)
	{
		setPixel
		(
			Vector2(x, y),
			RGBA::fromUInt(color),
			setAlpha,
			isPremultiplied
		);
	}

	void setPixel
	(
		const Vector2& pos,
		uint32_t color,
		bool setAlpha,
		bool isPremultiplied = true
	)
	{
		setPixel
		(
			pos,
			RGBA::fromUInt(color),
			setAlpha,
			isPremultiplied
		);
	}

	void setPixel
	(
		const Vector2& pos,
		const RGBA& color,
		bool setAlpha,
		bool isPremultiplied = true
	);

	RGBA getPixel
	(
		int32_t x,
		int32_t y,
		bool isPremultiplied = true
	)
	{
		return getPixel(Vector2(x, y), isPremultiplied);
	}

	RGBA getPixel
	(
		const Vector2& pos,
		bool isPremultiplied = true
	);

	std::vector<RGBA> getPixelVector
	(
		const Rect<int32_t>& rect,
		bool isPremultiplied = true
	);

	void copyRectangle
	(
		_R<BitmapContainer> source,
		const Rect<int32_t>& sourceRect,
		const Vector2& destPoint,
		bool mergeAlpha
	);

	void applyFilter
	(
		_R<BitmapContainer> source,
		const Rect<int32_t>& sourceRect,
		const Vector2f& destPoint,
		BitmapFilter& filter
	);

	void fillRectangle
	(
		const Rect<int32_t>& rect,
		const RGBA& color,
		bool useAlpha
	);

	bool scroll(const Vector2& pos);
	void floodFill(const Vector2& pos, const RGBA& color);
	const Vector2u& getSize() const { return size; }
	bool isEmpty() const { return data.empty(); }
	bool isFromTag() const { return fromTag; }

	bool checkTextureForUpload(SystemState* sys);
	void clone(BitmapContainer& c);
	void setModifiedData(bool modified);
	void setModifiedTexture(bool modified);
	bool getModifiedData() const { return hasModifiedData; }
	bool getModifiedTexture() const { return hasModifiedTexture; }
	void setNeedsClear(bool clear) { needsClear = clear; }
	bool getNeedsClear() const { return needsClear; }
	void addRenderCall(RenderDisplayObjectToBitmapContainer& call);
	void flushRenderCalls
	(
		RenderThread* renderThread,
		Bitmap* tempBitmap = nullptr,
		bool wait = true
	);

	void addRenderCallBitmap(RenderThread* renderThread, Bitmap& tempBitmap);
};

}
#endif /* BACKENDS_BITMAP_BITMAP_CONTAINER_H */
