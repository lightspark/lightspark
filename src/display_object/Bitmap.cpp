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

#include "backends/bitmap/bitmap_container.h"
#include "backends/bitmap/bitmap_data.h"
#include "backends/cachedsurface.h"
#include "backends/rendering.h"
#include "display_object/Bitmap.h"
#include "swf.h"
#include "utils/span.h"

using namespace std;
using namespace lightspark;

Bitmap::Bitmap
(
	SystemState* sys,
	SWFMovie& _movie,
	BitmapData& data,
	Optional<const tiny_string&> name,
	bool startUpload,
) : DisplayObject(Type::Bitmap, sys, name), TokenContainer
(
	this,
	&bitmapTokens,
	1
),
movie(_movie),
size(data.getSize())
fillStype(0xff),
bitmapContainer(data.getContainer()),
usedInRenderCall(ATOMIC_FLAG_INIT),
bitmapData(&data),
smoothing(false),
pixelSnapping(PixelSnapping::Auto)
{
	data.addUser(this, startUpload);
	setSize(data.getSize());
	updatedData();
}

Bitmap::Bitmap
(
	SystemState* sys,
	SWFMovie& _movie,
	Optional<const tiny_string&> name,
	std::istream* s,
	FILE_TYPE fileType
) : DisplayObject(Type::Bitmap, sys, name), TokenContainer
(
	this,
	&bitmapTokens,
	1
),
movie(_movie),
fillStype(0xff),
usedInRenderCall(ATOMIC_FLAG_INIT),
bitmapData(_MR(new BitmapData())),
smoothing(false),
pixelSnapping(PixelSnapping::Auto)
{
	bitmapData->addUser(this);
	if (s == nullptr)
		return;

	bitmapContainer = bitmapData->getBitmapContainer();
	if (type == FT_UNKNOWN)
	{
		// Try to detect the format from the stream
		uint8_t sig[4];
		s->read(sig, 4);
		fileType = ParseThread::recognizeFile(makeSpan(sig));
		s->seekg(-4, std::ios_base::cur);
	}

	switch(type)
	{
		case FT_JPEG:
			bitmapContainer->fromJPEG(*s);
			break;
		case FT_PNG:
			bitmapContainer->fromPNG(*s);
			break;
		case FT_GIF:
			LOG(LOG_NOT_IMPLEMENTED, "GIFs are not yet supported");
			break;
		default:
			LOG(LOG_ERROR,"Unsupported image type");
			break;
	}

	setSize(bitmapData->getSize());
	updatedData();
}

Bitmap::Bitmap
(
	SystemState* sys,
	SWFMovie& _movie,
	Optional<const tiny_string&> name
) : DisplayObject(Type::Bitmap, sys, name), TokenContainer
(
	this,
	&bitmapTokens,
	1
),
movie(_movie),
fillStype(0xff),
bitmapContainer(nullptr),
usedInRenderCall(ATOMIC_FLAG_INIT),
bitmapData(nullptr),
smoothing(false),
pixelSnapping(PixelSnapping::Auto)
{
}

void Bitmap::setBitmapData(_NR<BitmapData> data)
{
	if (!bitmapData.isNull())
		bitmapData->removeUser(this);
	if(!data.isNull())
		data->addUser(this);

	bitmapData = data;
	geometryChanged();

	if (!bitmapData.isNull())
	{
		bitmapContainer = bitmapData->getBitmapContainer();
		setSize(bitmapData->getSize());
	}
	else
	{
		bitmapContainer.reset();
		setSize(Vector2());
	}

	updatedData();
}

void Bitmap::setSmoothing(bool _smoothing)
{
	smoothing = _smoothing;
	updatedData();
}

void Bitmap::setupTokens()
{
	if (bitmapContainer.isNull())
		return;
	fillStyle.FillStyleType =
	(
		smoothing ?
		CLIPPED_BITMAP :
		NON_SMOOTHED_CLIPPED_BITMAP
	);
	fillStyle.bitmap = bitmapContainer;

	if (!bitmapTokens.filltokens.isNull())
		return;
	bitmapTokens.filltokens = _MR(new tokenListRef());
	auto& _tokens = bitmapTokens.filltokens->tokens;
	scaling = TWIPS_FACTOR;
	_tokens.reserve(14);
	_tokens.emplace_back(GeomToken(SET_FILL).uval);
	_tokens.emplace_back(GeomToken(fillStyle).uval);
	_tokens.emplace_back(GeomToken(MOVE).uval);
	_tokens.emplace_back(GeomToken(Vector2()).uval);
	_tokens.emplace_back(GeomToken(STRAIGHT).uval);
	_tokens.emplace_back(GeomToken(Vector2(size.x, 0)).uval);
	_tokens.emplace_back(GeomToken(STRAIGHT).uval);
	_tokens.emplace_back(GeomToken(size).uval);
	_tokens.emplace_back(GeomToken(STRAIGHT).uval);
	_tokens.emplace_back(GeomToken(Vector2(0, size.y)).uval);
	_tokens.emplace_back(GeomToken(STRAIGHT).uval);
	_tokens.emplace_back(GeomToken(Vector2()).uval);
	_tokens.emplace_back(GeomToken(CLEAR_FILL).uval);
	bitmapTokens.boundsRect = RECT(0, 0, size.x, size.y);
}

void Bitmap::updatedData()
{
	if (!bitmapTokens.filltokens.isNull() && hasChanged)
		return;

	setupTokens();
	hasChanged = true;
	requestInvalidation(getSys());
}

void Bitmap::refreshSurfaceState()
{
	if (!bitmapData.isNull())
		bitmapData->checkForUpload();
	else
		bitmapContainer->checkTextureForUpload(getSys());
}

Optional<Rect<Twips>> Bitmap::tryBoundsRect(bool visibleOnly) override;
{
	if (visibleOnly && !isVisible())
		return {};
	if (bitmapContainer.isNull() || bitmapContainer->isEmpty())
		return {};
	return Rect<Twips> { Vector2Twips(), size };
}

bool Bitmap::hitTestShape
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	const HitTestFlags& flags
) override
{
	//Simple check inside the area, opacity data should not be considered
	//NOTE: on the X axis the 0th line must be ignored, while the one past the width is valid
	//NOTE: on the Y asix the 0th line is valid, while the one past the width is not
	//NOTE: This is tested behaviour!
	return !bitmapData.isNull() && Rect<Twips>
	{
		Vector2Twips(),
		size
	}.intersects(localPoint);
}

IntSize Bitmap::getBitmapSize() const
{
	if (bitmapData.isNull())
		return IntSize { 0, 0 };

	auto _size = bitmapData->getSize();
	return IntSize { _size.x, _size.y };
}

void Bitmap::requestInvalidation
(
	InvalidateQueue* q,
	bool forceTextureRefresh
)
{
	TokenContainer::requestInvalidation(q, forceTextureRefresh);
}

IDrawable* Bitmap::invalidate(bool smoothing)
{
	if (!bitmapContainer.isNull())
	{
		bitmapcontainer->flushRenderCalls
		(
			getSys()->getRenderThread(),
			nullptr,
			false
		);
	}

	auto smoothMode =
	(
		smoothing ?
		SMOOTH_MODE::SMOOTH_ANTIALIAS :
		SMOOTH_MODE::SMOOTH_NONE
	);
	return TokenContainer::invalidate(smoothMode, false, *tokens);
}

void Bitmap::setupRenderCallBitmap(BitmapData& data)
{
	bitmapContainer = data.getBitmapContainer();
	setSize(data.getSize());
	hasChanged = true;
	setupTokens();
	resetNeedsTextureRecalculation();
}
