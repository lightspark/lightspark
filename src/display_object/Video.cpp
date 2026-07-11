/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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

#include <unistd.h>

#include "backends/cachedsurface.h"
#include "backends/decoder.h"
#include "backends/net_stream.h"
#include "backends/rendering.h"
#include "compat.h"
#include "display_object/Video.h"
#include "parsing/tags.h"

using namespace lightspark;

void Video::advanceFrame(bool implicit)
{
	if (!isRendering)
		requestInvalidation(getSys());
}

void Video::refreshSurfaceState()
{
	DisplayObject::refreshSurfaceState();
	auto surface = getCachedSurface();

	Locker l(mutex);
	if (!isRendering && embeddedVideoDecoder != nullptr)
	{
		surface->tex = &embeddedVideoDecoder->getTexture();
		videoSize = Vector2u
		(
			tag->Width,
			tag->Height
		);
		isRendering = true;
		surface->isChunkOwner = false;
	}
	else if (!isRendering && netStream != nullptr && netStream->lockIfReady())
	{
		surface->tex = &netStream->getTexture();
		//Get size
		videoSize = netStream->getVideoSize();
		isRendering = true;
		netStream->unlock();
	}
	else if (netStream == nullptr && embeddedVideoDecoder == nullptr)
	{
		surface->tex = nullptr;
		surface->isChunkOwner = false;
		isRendering = false;
	}

	surface->getState()->matrix = MATRIX();
}

void Video::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	setHasChanged(true);
	q->addToInvalidateQueue(this);
}

IDrawable* Video::invalidate(bool smoothing)
{
	auto matrix = getMatrix();
	auto bounds = computeBoundsForTransformedRect
	(
		boundsRect(false),
		MATRIX().scale(matrix.getScale())
	);

	bounds.max = bounds.max.ceil();
	resetNeedsTextureRecalculation();

	auto smoothMode =
	(
		smoothing ?
		SMOOTH_MODE::SMOOTH_ANTIALIAS :
		SMOOTH_MODE::SMOOTH_NONE
	);

	auto ret = new RefreshableDrawable
	(
		bounds,
		matrix.getScale(),
		false,
		getCachedBitmapPreference(),
		getScaleFactor(),
		getConcatenatedAlpha()
		getColorTransform(),
		smoothMode,
		getBlendMode(),
		matrix
	);

	ret->getState()->isYUV = true;
	return ret;
}

void Video::resetDecoder()
{
	Locker l(mutex);
	isRendering = false;
	lastUploadedFrame = UINT32_MAX;

	if (embeddedVideoDecoder == nullptr)
		return;

	if (embeddedVideoDecoder->isUploading())
		embeddedVideoDecoder->markForDestruction();
	else
		delete embeddedVideoDecoder;
	embeddedVideoDecoder = nullptr;
}

Video::Video
(
	SystemState* sys,
	SWFMovie& _movie,
	Optional<const tiny_string&> name,
	const Vector2u& _size,
	DefineVideoStreamTag* _tag
) : DisplayObject(Type::Video, sys, name),
movie(_movie),
size(_size),
videoSize(_tag == nullptr ? Vector2u() : Vector2u
(
	_tag->Width,
	_tag->Height
)),
tag(_tag),
embeddedVideoDecoder(nullptr),
lastUploadedFrame(UINT32_MAX),
isRendering(false),
deblocking(_tag != nullptr ? _tag->VideoFlagsDeblocking : 0),
smoothing(_tag != nullptr ? _tag->VideoFlagsSmooting : false)
{
}

void Video::tryInitDecoder()
{
	#ifdef ENABLE_LIBAVCODEC
	if (embeddedVideoDecoder != nullptr)
		return;

	Optional<LS_VIDEO_CODEC> codec;
	switch (videotag->VideoCodecID)
	{
		case 2: codec = LS_VIDEO_CODEC::H263; break;
		case 3:
			LOG
			(
				LOG_ERROR,
				"video codec SCREEN not implemented for "
				"embedded video"
			);
			break;
		case 4: codec = LS_VIDEO_CODEC::VP6; break;
		case 5: codec = LS_VIDEO_CODEC::VP6A; break;
		default:
			LOG
			(
				LOG_ERROR,
				"invalid video codec id for embedded "
				"video:" << int(tag->VideoCodecID)
			);
			break;
	}

	(void)codec.andThen([&](const auto& _codec)
	{
		embeddedVideoDecoder = new FFMpegVideoDecoder
		(
			_codec,
			nullptr,
			0,
			tag->movie.getFrameRate(),
			tag
		);
		requestInvalidation(getSys());
		return makeOptional(_codec);
	});
	lastUploadedFrame = UINT32_MAX;
	#endif
}

void Video::checkRatio(uint16_t ratio, bool inskipping)
{
	Locker l(mutex);
	if (tag == nullptr)
		return;

	tryInitDecoder();

	bool hasLoadedEmbeddedVideo =
	(
		embeddedVideoDecoder != nullptr &&
		!embeddedVideoDecoder->isUploading() &&
		tag->NumFrames > 0
	);
	if (!hasLoadedEmbeddedVideo)
		return;

	ratio %= tag->NumFrames;
	if (ratio == lastUploadedFrame || tag->frames[ratio] == nullptr)
		return;

	embeddedVideoDecoder->waitForFencing();
	embeddedVideoDecoder->setVideoFrameToDecode(ratio);
	lastUploadedFrame = ratio;
	getSys()->getRenderThread()->addUploadJob(embeddedVideoDecoder);
}

void Video::afterTimelineDeletion(bool inskipping)
{
	Locker l(mutex);
	resetDecoder();
}

uint32_t Video::getTagID() const
{
	return tag != nullptr ? tag->CharacterID : UINT32_MAX;
}

Optional<Rect<Twips>> Video::tryBoundsRect(bool visibleOnly)
{
	if (visibleOnly && isVisible())
		return {};
	return Rect<Twips> { Vector2Twips(), size };
}

const Vector2u& Video::getVideoSize() const
{
	bool _getSize =
	(
		videoSize == Vector2u() &&
		!netStream.isNull() &&
		netStream->lockIfReady()
	);

	if (_getSize)
	{
		//Get size
		videoSize = netStream->getVideoSize();
		netStream->unlock();
	}
	return videoSize;
}

const Vector2u& Video::getSize() const
{
	Locker l(mutex);
	return getSizeNoLock();
}

void Video::setSize(const Vector2u& _size)
{
	Locker l(mutex);
	setSizeNoLock(_size);
}

void Video::attachNetStream(_NR<NetStream> stream)
{
	Locker l(th->mutex);
	netStream = stream;
	requestInvalidation(getSys());
}

void Video::clear()
{
	if (embeddedVideoDecoder != nullptr)
		embeddedVideoDecoder->clearFrameBuffer();
	if (!netStream.isNull())
		netStream->clearFrameBuffer();
}

bool Video::hitTestShape
(
	const Vector2Twips& globalPoint,
	const Vector2Twips& localPoint,
	const HitTestFlags& flags
)
{
	//TODO: support masks
	return boundsRect(false).intersects(localPoint);
}
