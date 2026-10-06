/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013 Alessandro Pignotti (a.pignotti@sssup.it)
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

#include "3rdparty/perlinnoise/PerlinNoise.hpp"
#include "backends/bitmap/bitmap_container.h"
#include "backends/bitmap/bitmap_data.h"
#include "backends/cachedsurface.h"
#include "backends/rendering.h"
#include "display_object/Bitmap.h"
#include "display_object/DisplayObject.h"

using namespace lightspark;

class BitmapRef : public RefCountable, public Bitmap
{
public:
	using Bitmap::Bitmap;
};

BitmapData::BitmapData(SystemState* _sys, const Vector2u& size) :
sys(_sys),
pixels(_MR(new BitmapContainer(_sys->unaccountedMemory))),
locked(0),
needsUpload(true),
transparent(true)
{
	if (size == Vector2u())
		return;

	auto pixelArray = new uint32_t[size.x * size.y];
	memset
	(
		pixelArray,
		0,
		size.x * size.y * sizeof(uint32_t)
	);

	pixels->fromRGB
	(
		reinterpret_cast<uint8_t*>(pixelArray),
		size
		BitmapContainer::ARGB32
	);
}

BitmapData::BitmapData(SystemState* _sys, _R<BitmapContainer> _pixels) :
sys(_sys)
pixels(_pixels),
locked(0),
needsUpload(true),
transparent(true)
{
}

BitmapData::BitmapData(const BitmapData& other) :
sys(other.sys),
pixels(other.pixels),
locked(other.locked),
needsUpload(other.needsUpload),
transparent(other.transparent)
{
}

Bitmap* BitmapData::getRenderCallBitmap()
{
	if (temporaryBitmap.isNull())
	{
		temporaryBitmap = _MR(new BitmapRef(sys, sys->rootSWF));
		temporaryBitmap->setupRenderCallBitmap(this);
	}

	if (!temporaryBitmap->setRenderCall())
	{
		temporaryBitmap->incRef();
		return temporaryBitmap.getPtr();
	}

	// temporaryBitmap is already used in a rendercall, we have to create another one
	auto ret = new Bitmap(sys, sys->rootSWF);
	ret->setupRenderCallBitmap(this);
	return ret;
}

void BitmapData::addUser(Bitmap& bitmap, bool startUpload)
{
	users.insert(bitmap);

	if (!startUpload)
		return;
	needsUpload = true;
	bitmap.updatedData();
}

void BitmapData::removeUser(Bitmap& bitmap)
{
	users.erase(bitmap);
}

// needs to be called in renderThread
void BitmapData::checkForUpload()
{
	if (pixels.isNull() || !needsUpload)
		return;

	pixels->checkTextureForUpload(sys);
	needsUpload = false;
}

void BitmapData::notifyUsers()
{
	if (locked > 0 || users.empty())
		return;
	needsUpload = true;
	for (auto& user : users)
		user.get().updatedData();
}

void BitmapData::dispose()
{
	if (isDisposed())
		return;
	pixels->flushRenderCalls(sys->getRenderThread());
	pixels.reset();
	notifyUsers();
}

void BitmapData::drawDisplayObject
(
	DisplayObject* obj,
	const MATRIX& initialMatrix,
	bool smoothing,
	const AS_BLENDMODE& blendMode,
	Optional<const ColorTransform&> ct,
	Optional<const Rect<Twips>&> clipRect,
	bool needsCopy,
	Optional<const RGBA&> fillColor,
	uint8_t qualityFactor
)
{
	RenderDisplayObjectToBitmapContainer r;
	r.initialMatrix = initialMatrix;
	r.blendMode = blendMode;
	if (ct.hasValue())
		r.ct = *ct;
	r.smoothing = smoothing;
	r.qualityFactor = qualityFactor;
	r.backgroundColor = fillColor;
	r.clipRect = clipRect;

	if (obj != nullptr)
	{
		obj->invalidateForRenderToBitmap
		(
			pixels->getRenderData(),
			smoothing
		);
		r.cachedsurface = obj->getCachedSurface();
	}

	r.needsCopy = needsCopy;
	pixels->addRenderCall(r);
}

void BitmapData::fillRect(const Rect<int32_t>& rect, const RGBA& color)
{
	if (isDisposed())
		return;

	auto c = color;
	//premultiply alpha
	if (transparent && c.Alpha != 0xff)
	{
		c.Red = (c.Red * c.Alpha + 0x7f) / 0xff;
		c.Green = (c.Green * c.Alpha + 0x7f) / 0xff;
		c.Blue = (c.Blue * c.Alpha + 0x7f) / 0xff;
	}

	c.Alpha = transparent ? c.Alpha : 0xff;

	drawDisplayObject
	(
		nullptr,
		MATRIX(),
		true,
		BLENDMODE_NORMAL,
		{},
		makeOptionalRef(rect).filter(rect != getRect()),
		false,
		makeOptionalRef(c)
	);
	notifyUsers();
}

void BitmapData::copyPixels
(
	_R<BitmapData> source,
	const Rect<int32_t>& srcRect,
	const Vector2& destPoint,
	_R<BitmapData> alphaBitmap,
	const Vector2f& alphaPoint,
	bool mergeAlpha
)
{
	if (isDisposed())
		return;

	auto d = source->getRenderCallBitmap();
	d->setScrollRect(srcRect);
	MATRIX m;
	m.translate(destPoint);

	drawDisplayObject
	(
		d,
		m,
		true,
		mergeAlpha ? BLENDMODE_NORMAL : BLENDMODE_INTERN_REPLACE,
		{},
		{},
		source->pixels == pixels
	);

	pixels->addRenderCallBitmap(sys->getRenderThread(), d);
	notifyUsers();
}

void BitmapData::copyPixelsToBuffer
(
	const Rect<int32_t>& rect,
	std::vector<uint32_t>& data
)
{
	if (isDisposed())
		return;

	pixels->flushRenderCalls(sys->getRenderThread());
	auto _rect = pixels->clipRect(rect);
	for (ssize_t y = _rect.min.y; y < _rect.max.y; ++y)
	{
		for (ssize_t x = _rect.min.x; x < _rect.max.x; ++x)
			data.push_back(pixels->getPixel(x, y, false));
	}
}

bool BitmapData::hitTestBitmap
(
	const Vector2& point,
	uint8_t threshold,
	_R<BitmapData> other,
	const Vector2& otherPoint,
	uint8_t otherThreshold
)
{
	if (isDisposed() || other->isDisposed())
		return false;

	auto size = getSize();
	for (size_t y = 0; y < size.y; ++y)
	{
		for (size_t x = 0; x < size.x; ++x)
		{
			Vector2 _point(x, y);
			if (pixels->getPixel(point + _point) < threshold)
				continue;
			auto secondPoint = otherPoint + _point;
			if (other->pixels->getPixel(secondPoint) >= otherThreshold)
				return true;
		}
	}
	return false;
}

bool BitmapData::hitTestRect
(
	const Vector2& point,
	uint8_t threshold,
	const Vector2& size
)
{
	if (isDisposed())
		return false;

	auto rect = makeRect(point, size).clamp(getSize());
	for (ssize_t y = rect.min.y; y < rect.max.y; ++y)
	{
		for (ssize_t x = rect.min.x; x < rect.max.x; ++x)
		{
			if (pixels->getPixel(x, y).Alpha >= threshold)
				return true;
		}
	}
	return false;
}

bool BitmapData::hitTestPoint(const Vector2& point, uint8_t threshold)
{
	if (isDisposed())
		return false;
	return
	(
		getRect().intersects(point) &&
		pixels->getPixel(point).Alpha >= threshold
	);
}

void BitmapData::scroll(const Vector2& pos)
{
	if (isDisposed())
		return;

	auto copySize = std::max<Vector2>
	(
		getSize() - pos.abs(),
		Vector2()
	);

	if (pos == Vector2() || copySize == Vector2u())
		return;

	auto d = getRenderCallBitmap();
	auto rect = makeRect(-pos, copySize);
	d->setScrollRect(makeRect(-pos, copySize));

	drawDisplayObject
	(
		d,
		MATRIX(),
		false,
		BLENDMODE_NORMAL,
		{},
		{},
		true
	);
	pixels->addRenderCallBitmap(sys->getRenderThread(), d);
	notifyUsers();
}

_NR<BitmapData> BitmapData::clone()
{
	if (isDisposed())
		return NullRef;
	auto _clone = _MR(new BitmapData(sys, getSize()));
	_clone->transparent = transparent;
	if (pixels.isNull())
		return _clone;

	pixels->flushRenderCalls(sys->getRenderThread());
	pixels->clone(_clone->pixels);
	return _clone;
}

void BitmapData::copyChannel
(
	_R<BitmapData> source,
	const Vector2& destPoint,
	const Rect<int32_t>& srcRect,
	size_t srcChannel,
	size_t destChannel
)
{
	if (isDisposed())
		return;

	auto srcShift = BitmapDataChannel::channelShift(srcChannel);
	auto destShift = BitmapDataChannel::channelShift(destChannel);

	auto _srcRect
	Rect<int32_t> _srcRect;
	Vector2 _destPoint;
	std::tie(_srcRect, _destPoint) = th->pixels->clipRect
	(
		source->pixels,
		srcRect,
		destPoint
	);

	auto regionSize = _srcRect.size();

	if (regionSize < Vector2())
		return;
	pixels->flushRenderCalls(sys->getRenderThread());
	source->pixels->flushRenderCalls(sys->getRenderThread());

	bool isPremultiplied = srcChannel == BitmapDataChannel::ALPHA;
	uint32_t chMask = ~(0xFF << destShift);
	for (size_t y = 0; y < regionSize.y; ++y)
	{
		for (size_t x = 0; x < regionSize.x; ++x)
		{
			Vector2 point(x, y);
			auto srcPos = _srcRect.min + point;
			auto srcPx = source->pixels->getPixel
			(
				srcPos,
				false
			).toUInt();

			uint8_t channel = srcPx >> srcShift;
			auto destPos = _destPoint + point;
			auto oldPx = pixels->getPixel
			(
				destPos,
				isPremultiplied
			).toUInt();

			auto dstChannel = uint32_t(channel) << destShift;

			auto newPx = ((oldPx & chMask) | dstChannel);
			pixels->setPixel
			(
				destPos,
				newPx,
				true,
				isPremultiplied
			);
		}
	}

	notifyUsers();
}

void BitmapData::unlock()
{
	if (!locked)
		return;
	if (!--locked)
		notifyUsers();
}

void BitmapData::histogram
(
	Optional<const Rect<int32_t>&> rect
	std::array<std::array<uint32_t, 256>, 4>& arr
)
{
	if (isDisposed())
		return;

	// red, green, blue, alpha
	static constexpr size_t channelOrder[] =
	{
		2,
		1,
		0,
		3
	};

	auto _rect = rect.transformOr(getRect(), [&](const auto& rect)
	{
		return pixels->clipRect(rect);
	});

	pixels->flushRenderCalls(sys->getRenderThread());

	for (ssize_t y = _rect.min.y; y < _rect.max.y; ++y)
	{
		for (ssize_t x = _rect.min.x; x < _rect.max.x; ++x)
		{
			auto px = pixels->getPixel(x, y);
			for (size_t i = 0; i < 4; ++i)
				++arr[i][px[channelOrder[i]]];
		}
	}
}

Rect<uint32_t> BitmapData::getColorBoundsRect
(
	bool findColor,
	uint32_t mask,
	const RGBA& color
)
{
	pixels->flushRenderCalls(sys->getRenderThread());

	auto rect = getRect()
	auto size = getSize();
	auto _color = color.toUInt();
	for (size_t y = 0; y < size.y; ++y)
	{
		for (size_t x = 0; x < size.x; ++x)
		{
			auto px = pixels->getPixel(x, y).toUInt();
			#if 1
			if (((px & mask) != _color) ^ findColor)
			#else
			bool updateBounds =
			(
				(findColor && (px & mask) == _color) ||
				(!findColor && (px & mask) != _color)
			);
			if (!updateBounds)
			#endif
				continue;

			rect = rect._union(makeRect(x, y, x, y));
		}
	}

	if (rect.max > Vector2())
		return rect;
	return Rect<uint32_t> {};
}

void BitmapData::getPixels
(
	const Rect<int32_t>& rect,
	std::vector<uint32_t>& data
)
{
	if (isDisposed())
		return;

	auto pxVec = pixels->getPixelVector(rect);
	for (auto px : pxVec)
		data.emplace_back(px);
}

void BitmapData::setPixels
(
	const Rect<int32_t>& rect,
	Span<uint32_t> data
)
{
	if (isDisposed())
		return;

	auto _rect = pixels->clipRect(rect);
	pixels->flushRenderCalls(sys->getRenderThread());

	auto it = data.begin();
	for (ssize_t y = _rect.min.y; y < _rect.max.y; ++y)
	{
		for (ssize_t x = _rect.min.x; x < _rect.max.x; ++x)
			pixels->setPixel(x, y, *it++, transparent, false);
	}

	notifyUsers();
}

void BitmapData::colorTransform
(
	const Rect<int32_t>& rect,
	const CXFORMWITHALPHA& cxform
)
{
	if (isDisposed())
		return;

	bool isInvalid =
	(
		cxform.RedMultTerm == 0x100 &&
		cxform.GreenMultTerm == 0x100 &&
		cxform.BlueMultTerm == 0x100 &&
		// NOTE, FLASH-PLAYER-BUG: Applying a color transform with only
		// an alpha multiplier greater than 1 is treated as a no-op.
		cxform.AlphaMultTerm >= 0x100 &&
		!cxform.RedAddTerm &&
		!cxform.GreenAddTerm &&
		!cxform.BlueAddTerm &&
		!cxform.AlphaAddTerm
	);
	if (isInvalid)
		return;

	auto _rect = pixels->clipRect(rect);
	pixels->flushRenderCalls(sys->getRenderThread());

	for (ssize_t y = _rect.min.y; y < _rect.max.y; ++y)
	{
		for (ssize_t x = _rect.min.x; x < _rect.max.x; ++x)
		{
			Vector2 point(x, y);
			pixels->setPixel
			(
				point,
				cxform * pixels->getPixel(point),
				transparent,
				false
			);
		}
	}
	notifyUsers();
}

_NR<BitmapData> BitmapData::compare(_R<BitmapData> source)
{
	if (getSize() != source->getSize())
		return NullRef;

	auto rect = getRect();
	auto ret = _MR(new BitmapData(sys, getSize()));
	bool different = false;

	for (ssize_t y = _rect.min.y; y < _rect.max.y; ++y)
	{
		for (ssize_t x = _rect.min.x; x < _rect.max.x; ++x)
		{
			Vector2 pos(x, y);
			auto px = pixels->getPixel(pos, false);
			auto srcPx = source->pixels->getPixel(pos, false);
			if (px == srcPx)
			{
				ret->pixels->setPixel(pos, RGBA(), true);
				continue;
			}

			different = true;
			ret->pixels->setPixel
			(
				pos,
				px.toRGB() == srcPx.toRGB() ?
				RGBA(0xffffff, px.Alpha - srcPx.Alpha) :
				px.toRGB() - srcPx.toRGB(),
				true,
				false
			);
		}
	}

	return different ? ret : NullRef;
}

void BitmapData::applyFilter
(
	_R<BitmapData> source,
	const Vector2& destPoint,
	const Rect<int32_t>& srcRect,
	const Filter& filter
)
{
	if (isDisposed())
		return;

	auto d = source->getRenderCallBitmap();
	d->setFilter(filter);
	d->setScrollRect(srcRect);

	MATRIX m;
	m.translate(destPoint);

	drawDisplayObject
	(
		d,
		m,
		true,
		BLENDMODE_NORMAL,
		{},
		{},
		source->pixels == pixels
	);

	pixels->flushRenderCalls(sys->getRenderThread(), d);
	notifyUsers();
}

static uint32_t LehmerRandom(uint32_t& seed)
{
	seed = (uint64_t(seed) * 16807U) % 2147483647;
	return seed;
}

void BitmapData::noise
(
	uint32_t randomSeed,
	uint8_t low,
	uint8_t high,
	const BitmapChannelOptions& opts,
	bool grayScale
)
{
	if (isDisposed())
		return;

	if (high < low)
		high = low;
	pixels->flushRenderCalls(sys->getRenderThread());

	auto range = high - low;
	auto size = getSize();
	auto getRand = [&] -> uint8_t
	{
		return LehmerRandom(randomSeed) % (range + 1) + low;
	};

	for (size_t y = 0; y < size.y; ++y)
	{
		for (size_t x = 0; x < size.x; ++x)
		{

			if (grayScale)
			{
				auto v = getRand();
				pixels->setPixel(x, y, RGBA
				(
					v,
					v,
					v,
					opts & 8 ? getRand() : 0xff
				), true, false);
				continue;
			}

			pixels->setPixel(x, y, RGBA
			(
				opts & 1 ? getRand() : 0,
				opts & 2 ? getRand() : 0,
				opts & 4 ? getRand() : 0,
				opts & 8 ? getRand() : 0xff
			), true, false);
		}
	}
}

void BitmapData::perlinNoise
(
	const Vector2f& base,
	size_t numOctaves,
	uint32_t randomSeed,
	bool stitch,
	bool fractalNoise,
	const BitmapChannelOptions& opts,
	bool grayScale,
	Span<Vector2f> offsets
)
{
	if (isDisposed())
		return;

	if (stitch)
		LOG(LOG_NOT_IMPLEMENTED, "perlinNoise: parameter stitch is ignored");
	if (fractalNoise)
		LOG(LOG_NOT_IMPLEMENTED, "perlinNoise: parameter fractalNoise is ignored");
	if (!offsets.empty())
		LOG(LOG_NOT_IMPLEMENTED, "perlinNoise: parameter offsets is ignored");
	pixels->flushRenderCalls(sys->getRenderThread());

	const siv::PerlinNoise perlin(randomSeed);
	for (size_t y = 0; y < size.y; ++y)
	{
		for (size_t x = 0; x < size.x; ++x)
		{
			Vector2u pos(x, y);
			Vector2f _pos = pos / base;
			auto v1 = perlin.octaveNoise0_1(_pos.x, _pos.y, numOctaves);
			if (grayScale)
			{
				uint8_t v = dclamp(v1, 0, 1) * 255 + 0.5;
				pixels->setPixel
				(
					pos,
					RGB(v, v, v),
					true,
					true
				);
				continue;
			}

			uint32_t v = v1 >= 1 ? 255 : v1 <= 0 ? 0 :
			(
				v1 *
				UINT32_MAX +
				0.5
			);


			pixels->setPixel(pos, RGBA
			(
				opts & 1 ? v >> 24 : 0,
				opts & 2 ? v >> 16 : 0,
				opts & 4 ? v >> 8 : 0,
				opts & 8 ? v & 0xff : 0xff
			), true, true);
			//LOG(LOG_INFO,"perlinnoise pixel:"<<x<<" "<<y<<" "<<hex<<th->pixels->getPixel(x,y)<<" "<<grayScale);
		}
	}
}

size_t BitmapData::getThreshold
(
	_R<BitmapData> source,
	const Rect<int32_t>& srcRect,
	const Vector2& destPoint,
	const ThresholdOp& op,
	size_t threshold,
	const RGBA& color,
	uint32_t mask,
	bool copySource
)
{
	if (isDisposed())
		return 0;

	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"BitmapData::getThreshold isn't implemented yet"
	);
	return 0;
}

void BitmapData::merge
(
	_R<BitmapData> source,
	const Vector2& destPoint,
	const Rect<int32_t>& srcRect,
	const RGBA& mult
)
{
	if (isDisposed())
		return;

	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"BitmapData::merge isn't implemented yet"
	);
}

void BitmapData::paletteMap
(
	_R<BitmapData> source,
	const Rect<int32_t>& srcRect,
	const Vector2& destPoint,
	const std::array<uint32_t, 256>& redArray,
	const std::array<uint32_t, 256>& greenArray,
	const std::array<uint32_t, 256>& blueArray,
	const std::array<uint32_t, 256>& alphaArray
)
{
	if (isDisposed())
		return;

	LOG
	(
		LOG_NOT_IMPLEMENTED,
		"BitmapData::paletteMap isn't implemented yet"
	);
}

uint32_t BitmapData::pixelDissolve
(
	_R<BitmapData> source,
	const Rect<int32_t>& srcRect,
	const Vector2& destPoint,
	uint32_t randomSeed,
	size_t numPixels,
	const RGBA& fillColor
)
{
	if (isDisposed())
		return randomSeed;

	auto _srcRect = Rect<int32_t>
	{
		srcRect.min.max(Vector2()),
		srcRect.max
	};

	auto size = getSize();
	if (_srcRect.min > size || _srcRect.max <= _srcRect.min)
		return randomSeed;

	auto _size = _srcRect.size();
	pixels->flushRenderCalls(sys->getRenderThread());
	source->pixels->flushRenderCalls(sys->getRenderThread());

	if (destPoint < Vector2() && _size + destPoint <= Vector2())
		return randomSeed;

	Vector2u _destPoint = destPoint.max(Vector2());
	_size += destPoint.max(Vector2());

	if (source.getPtr == this) // it seems that first pixel is always set if source and target are the same
		pixels->setPixel(_destPoint, fillColor, true);

	_size = _size.min(size - _destPoint - _srcRect.min);
	pixels->flushRenderCalls(sts->getRenderThread());
	for (size_t i = 0; i < numPixels; ++i)
	{
		Vector2u pos = (Vector2f
		(
			LehmerRandom(randomSeed),
			LehmerRandom(randomSeed)
		) / UINT32_MAX) * _size + _destPoint;
		if (source.getPtr() != this)
		{
			pixels->setPixel
			(
				pos,
				source->pixels->getPixel(_srcRect + pos),
				true
			);
			continue;
		}

		auto initPos = pos;
		while (pixels->getPixel(pos) == fillColor)
		{
			// simple search for next unfilled position
			pos = (pos + 1).min(_size + _destPoint);
			if (pos == initPos)
			{
				// all is filled
				return randomSeed;
			}
		}
		pixels->setPixel(pos, fillColor, true);
	}

	return randomSeed;
}
