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

#ifndef DISPLAY_OBJECT_VIDEO_H
#define DISPLAY_OBJECT_VIDEO_H 1

#include "display_object/DisplayObject.h"
#include "swftypes.h"
#include "threading.h"
#include "utils/optional.h"

namespace lightspark
{

class DefineVideoStreamTag;
class NetStream;
class SWFMovie;
class VideoDecoder;

class Video : public DisplayObject
{
private:
	SWFMovie& movie;
	mutable Mutex mutex;
	Vector2u size;
	mutable Vector2u videoSize;
	_NR<NetStream> netStream;
	DefineVideoStreamTag* tag;
	VideoDecoder* embeddedVideoDecoder;
	uint32_t lastUploadedFrame;
	bool isRendering;

	int32_t deblocking;
	bool smoothing;

	void resetDecoder();
	void tryInitDecoder();
public:
	Video
	(
		SystemState* sys,
		SWFMovie& _movie,
		DefineVideoStreamTag& _tag,
		Optional<const tiny_string&> name = {},
		const Vector2u& _size = Vector2u(320, 240)
	) : Video(sys, _movie, name, _size, &_tag) {}

	Video
	(
		SystemState* sys,
		SWFMovie& _movie,
		Optional<const tiny_string&> name = {},
		const Vector2u& _size = Vector2u(320, 240),
		DefineVideoStreamTag* _tag = nullptr
	);

	void advanceFrame(bool implicit) override;
	void refreshSurfaceState() override;
	void requestInvalidation
	(
		InvalidateQueue* q,
		bool forceTextureRefresh = false
	) override;

	IDrawable* invalidate(bool smoothing) override;
	void checkRatio(uint16_t ratio, bool inskipping) override;
	void afterTimelineDeletion(bool inskipping) override;
	uint32_t getTagID() const override;

	int32_t getDeblocking() const { return deblocking; }
	void setDeblocking(int32_t val) { deblocking = val; }
	bool getSmoothing() const { return smoothing; }
	void setSmoothing(bool _smoothing) { smoothing = _smoothing; }
	const Vector2u& getVideoSize() const;
	const Vector2u& getSize() const;
	void setSize(const Vector2u& _size);
	const Vector2u& getSizeNoLock() const { return size; }
	void setSizeNoLock(const Vector2u& _size) { size = _size; }

	void attachNetStream(_NR<NetStream> stream);
	void clear();

	Optional<Rect<Twips>> tryBoundsRect(bool visibleOnly) override;
	bool hitTestShape
	(
		const Vector2Twips& globalPoint,
		const Vector2Twips& localPoint,
		const HitTestFlags& flags
	) override;
};

}
#endif /* DISPLAY_OBJECT_VIDEO_H */
