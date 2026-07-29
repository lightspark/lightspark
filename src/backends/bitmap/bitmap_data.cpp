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

uint32_t LehmerRandom(uint32_t& seed)
{
	seed = (uint64_t(seed) * 16807U) % 2147483647;
	return seed;
}

ASFUNCTIONBODY_ATOM(BitmapData,noise)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

	int32_t randomSeed;
	uint32_t low;
	uint32_t high;
	uint32_t channelOptions;
	bool grayScale;
	ARG_CHECK(ARG_UNPACK(randomSeed)(low, 0) (high, 255) (channelOptions, 7) (grayScale, false));

	uint32_t randomval = randomSeed <= 0 ? -randomSeed+1 : randomSeed;
	if (high < low)
		high=low;
	th->getBitmapContainer()->flushRenderCalls(th->getSystemState()->getRenderThread());

	uint32_t range = (uint8_t)high-(uint8_t)low;
	for (int32_t y=0; y<th->getHeight(); y++)
	{
		for (int32_t x=0; x<th->getWidth(); x++)
		{
			uint32_t pixel = 0;

			if (grayScale)
			{
				uint8_t v = (LehmerRandom(randomval) % (range +1) + low) & 0xff;
				pixel |= v<<16 | v<<8 | v;
				if((channelOptions & 0x8) == 0x8) // A
					pixel |= (LehmerRandom(randomval) % (range +1) + low)<<24;
				else
					pixel |= 0xff<<24;
			}
			else
			{
				if((channelOptions & 0x1) == 0x1) // R
					pixel |= ((LehmerRandom(randomval) % (range +1) + low) & 0xff)<<16;
				if((channelOptions & 0x2) == 0x2) // G
					pixel |= ((LehmerRandom(randomval) % (range +1) + low) & 0xff)<<8;
				if((channelOptions & 0x4) == 0x4) // B
					pixel |= ((LehmerRandom(randomval) % (range +1) + low) & 0xff);
				if((channelOptions & 0x8) == 0x8) // A
					pixel |= ((LehmerRandom(randomval) % (range +1) + low) & 0xff)<<24;
				else
					pixel |= 0xff<<24;
			}
			th->pixels->setPixel(x, y,pixel,true,false);
		}
	}
}
ASFUNCTIONBODY_ATOM(BitmapData,perlinNoise)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

	number_t baseX;
	number_t baseY;
	unsigned int numOctaves;
	int randomSeed;
	bool stitch;
	bool fractalNoise;
	unsigned int channelOptions;
	bool grayScale;
	_NR<Array> offsets;
	ARG_CHECK(ARG_UNPACK(baseX)(baseY)(numOctaves)(randomSeed)(stitch) (fractalNoise) (channelOptions, 7) (grayScale, false) (offsets, NullRef));

	if (stitch)
		LOG(LOG_NOT_IMPLEMENTED,"perlinNoise: parameter stitch is ignored");
	if (fractalNoise)
		LOG(LOG_NOT_IMPLEMENTED,"perlinNoise: parameter fractalNoise is ignored");
	if (!offsets.isNull())
		LOG(LOG_NOT_IMPLEMENTED,"perlinNoise: parameter offsets is ignored");
	th->getBitmapContainer()->flushRenderCalls(th->getSystemState()->getRenderThread());

	const siv::PerlinNoise perlin(randomSeed);
	for (int32_t x=0; x<th->getWidth(); x++)
	{
		for (int32_t y=0; y<th->getHeight(); y++)
		{
			uint32_t pixel = 0x000000ff;
			number_t v1 = perlin.octaveNoise0_1(x / baseX, y / baseY, numOctaves);
			if (grayScale)
			{
				uint8_t v = v1 >= 1.0 ? 255 : v1 <= 0.0 ? 0 : static_cast<std::uint8_t>(v1 * 255.0 + 0.5);
				pixel |= v<<24 | v<<16 | v<<8;
			}
			else
			{
				if((channelOptions & 0x1) == 0x1) // R
				{
					uint32_t v = v1 >= 1.0 ? 255 : v1 <= 0.0 ? 0 : static_cast<std::uint32_t>(v1 * UINT32_MAX + 0.5);
					pixel |= v&0xff000000;
				}
				if((channelOptions & 0x2) == 0x2) // G
				{
					uint32_t v = v1 >= 1.0 ? 255 : v1 <= 0.0 ? 0 : static_cast<std::uint32_t>(v1 * UINT32_MAX + 0.5);
					pixel |= v&0x00ff0000;
				}
				if((channelOptions & 0x4) == 0x4) // B
				{
					uint32_t v = v1 >= 1.0 ? 255 : v1 <= 0.0 ? 0 : static_cast<std::uint32_t>(v1 * UINT32_MAX + 0.5);
					pixel |= v&0x0000ff00;
				}
				if((channelOptions & 0x8) == 0x8) // A
				{
					uint32_t v = v1 >= 1.0 ? 255 : v1 <= 0.0 ? 0 : static_cast<std::uint32_t>(v1 * UINT32_MAX + 0.5);
					pixel |= v&0x000000ff;
				}
			}
			th->pixels->setPixel(x, y,pixel,true,true);
			//LOG(LOG_INFO,"perlinnoise pixel:"<<x<<" "<<y<<" "<<hex<<th->pixels->getPixel(x,y)<<" "<<grayScale);
		}
	}
}
ASFUNCTIONBODY_ATOM(BitmapData,threshold)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	tiny_string operation;
	uint32_t threshold;
	uint32_t color;
	uint32_t mask;
	bool copySource;
	ARG_CHECK(ARG_UNPACK(sourceBitmapData)(sourceRect)(destPoint)(operation)(threshold) (color,0) (mask, 0xFFFFFFFF) (copySource, false));

	LOG(LOG_NOT_IMPLEMENTED,"BitmapData.threshold not implemented");
}
ASFUNCTIONBODY_ATOM(BitmapData,merge)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;
	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	uint32_t redMultiplier;
	uint32_t greenMultiplier;
	uint32_t blueMultiplier;
	uint32_t alphaMultiplier;
	ARG_CHECK(ARG_UNPACK(sourceBitmapData)(sourceRect) (destPoint) (redMultiplier) (greenMultiplier) (blueMultiplier) (alphaMultiplier));

	LOG(LOG_NOT_IMPLEMENTED,"BitmapData.merge not implemented");
}
ASFUNCTIONBODY_ATOM(BitmapData,paletteMap)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	_NR<Array> redArray;
	_NR<Array> greenArray;
	_NR<Array> blueArray;
	_NR<Array> alphaArray;
	ARG_CHECK(ARG_UNPACK(sourceBitmapData)(sourceRect) (destPoint) (redArray, NullRef) (greenArray, NullRef) (blueArray, NullRef) (alphaArray, NullRef));

	LOG(LOG_NOT_IMPLEMENTED,"BitmapData.paletteMap not implemented");
}
ASFUNCTIONBODY_ATOM(BitmapData,pixelDissolve)
{
	BitmapData* th = asAtomHandler::as<BitmapData>(obj);
	if(th->checkDisposed(ret))
		return;

	ret = asAtomHandler::fromInt(-1);
	_NR<BitmapData> sourceBitmapData;
	_NR<Rectangle> sourceRect;
	_NR<Point> destPoint;
	int32_t randomSeed;
	int32_t numPixels;
	uint32_t fillColor;
	if (wrk->needsActionScript3())
	{
		ARG_CHECK(ARG_UNPACK(sourceBitmapData)(sourceRect) (destPoint) (randomSeed, 0) (numPixels, 0) (fillColor, 0xff000000));
		if (numPixels < 0)
		{
			createError<RangeError>(wrk,kParamRangeNonNegativeError,"numPixels",asAtomHandler::toString(args[4],wrk));
			return;
		}
	}
	else
	{
		// contrary to specs default fillcolor seems to be fully opaque black for AS3
		ARG_CHECK(ARG_UNPACK_NO_ERROR(sourceBitmapData)(sourceRect) (destPoint) (randomSeed, 0) (numPixels, 0) (fillColor, 0));
		if (wrk->AVM1callStack.back()->exceptionthrown)
		{
			wrk->AVM1callStack.back()->exceptionthrown->decRef();
			wrk->AVM1callStack.back()->exceptionthrown=nullptr;
			return;
		}
	}
	if (sourceBitmapData.isNull())
	{
		if (wrk->needsActionScript3())
			createError<TypeError>(wrk,kNullPointerError,"sourceBitmapData");
		return;
	}
	if(sourceBitmapData->pixels.isNull())
	{
		if (wrk->needsActionScript3())
			createError<ArgumentError>(wrk,kInvalidBitmapData);
		return;
	}
	if (sourceRect.isNull())
	{
		if (wrk->needsActionScript3())
			createError<TypeError>(wrk,kNullPointerError,"sourceRect");
		else
			ret = asAtomHandler::fromInt(-4);
		return;
	}
	if (destPoint.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError,"destPoint");
		return;
	}

	uint32_t seed = (uint32_t)randomSeed;
	RECT rc = sourceRect->getRect();
	if (rc.Xmin<0) rc.Xmin=0;
	if (rc.Ymin<0) rc.Ymin=0;
	if (rc.Xmin > th->pixels->getWidth()
		|| rc.Ymin > th->pixels->getHeight()
		|| rc.Xmax<=rc.Xmin
		|| rc.Ymax<=rc.Ymin)
	{
		ret = asAtomHandler::fromInt(seed);
		return;
	}
	uint32_t w =(rc.Xmax-rc.Xmin);
	uint32_t h =(rc.Ymax-rc.Ymin);
	uint32_t destx=0;
	uint32_t desty=0;
	th->getBitmapContainer()->flushRenderCalls(th->getSystemState()->getRenderThread());
	sourceBitmapData->getBitmapContainer()->flushRenderCalls(th->getSystemState()->getRenderThread());
	if (destPoint->getX()>=0)
		destx = (uint32_t)destPoint->getX();
	else
	{
		destx =0;
		if ((int32_t)w + destPoint->getX() > 0)
			w = uint32_t((int32_t)w + destPoint->getX());
		else
		{
			ret = asAtomHandler::fromInt(seed);
			return;
		}
	}
	if (destPoint->getY()>=0)
		desty = (uint32_t)destPoint->getY();
	else
	{
		desty =0;
		if ((int32_t)h + destPoint->getY() > 0)
			h = uint32_t((int32_t)h + destPoint->getY());
		else
		{
			ret = asAtomHandler::fromInt(seed);
			return;
		}
	}
	if (sourceBitmapData.getPtr() == th) // it seems that first pixel is always set if source and target are the same
		th->pixels->setPixel(destx,desty,fillColor,true);
	if (w > th->pixels->getWidth()-destx-rc.Xmin )
		w = th->pixels->getWidth()-destx-rc.Xmin;
	if (h > th->pixels->getHeight()-desty-rc.Ymin)
		h = th->pixels->getHeight()-desty-rc.Ymin;
	th->pixels->flushRenderCalls(wrk->getSystemState()->getRenderThread());
	for (int32_t i = 0; i < numPixels; i++)
	{
		int x = (((number_t)LehmerRandom(seed))/(number_t)UINT32_MAX)*w+destx;
		int y = (((number_t)LehmerRandom(seed))/(number_t)UINT32_MAX)*h+desty;
		if (sourceBitmapData.getPtr() == th)
		{
			int x1=x;
			int y1=y;
			while (th->pixels->getPixel(x,y)==fillColor)
			{
				// simple search for next unfilled position
				x++;
				if (x >= int(w+destx))
				{
					x = destx;
					y++;
					if (y >= int(h+desty))
					{
						y = desty;
					}
				}
				if (y == y1 && x == x1)
				{
					// all is filled
					ret = asAtomHandler::fromInt(seed);
					return;
				}
			}
			th->pixels->setPixel(x,y,fillColor,true);
		}
		else
		{
			th->pixels->setPixel(x,y,sourceBitmapData->pixels->getPixel(rc.Xmin+x,rc.Ymin+y),true);
		}
	}
	ret = asAtomHandler::fromInt(seed);
}
