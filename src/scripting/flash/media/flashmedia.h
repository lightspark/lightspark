/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_MEDIA_FLASHMEDIA_H
#define SCRIPTING_FLASH_MEDIA_FLASHMEDIA_H 1

#include "compat.h"
#include "asobject.h"
#include "timer.h"
#include "backends/graphics.h"
#include "backends/netutils.h"

class AudioStream;

namespace lightspark
{

class AudioDecoder;
class NetStream;

class Sound: public EventDispatcher, public IThreadJob, public ILoadable
{
friend class SoundChannel;
private:
	URLInfo url;
	std::vector<uint8_t> postData;
	Downloader* downloader;
	Mutex mutex;
	ACQUIRE_RELEASE_FLAG(stopped);
	AudioDecoder* audioDecoder;
	AudioStream* audioStream;
	ASPROPERTY_GETTER(uint32_t,bytesLoaded);
	ASPROPERTY_GETTER(uint32_t,bytesTotal);
	ASPROPERTY_GETTER(number_t,length);
	//ILoadable interface
	void setBytesTotal(uint32_t b);
	void setBytesLoaded(uint32_t b);
public:
	Sound(Class_base* c);
	~Sound();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(load);
	ASFUNCTION(play);
	ASFUNCTION(close);

	//IThreadJob interface
	void execute();
	void jobFence();
	void threadAbort();
};

class SoundTransform: public ASObject
{
public:
	SoundTransform(Class_base* c):ASObject(c){}
	ASPROPERTY_GETTER_SETTER(number_t,volume);
	ASPROPERTY_GETTER_SETTER(number_t,pan);
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
};

class SoundChannel : public EventDispatcher
{
public:
	SoundChannel(Class_base* c) : EventDispatcher(c) {}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(addEventListener);
};

class Video: public DisplayObject
{
private:
	mutable Mutex mutex;
	uint32_t width, height;
	mutable uint32_t videoWidth, videoHeight;
	bool initialized;
	_NR<NetStream> netStream;
public:
	Video(Class_base* c, uint32_t w=320, uint32_t h=240);
	void finalize();
	~Video();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getVideoWidth);
	ASFUNCTION(_getVideoHeight);
	ASFUNCTION(_getWidth);
	ASFUNCTION(_setWidth);
	ASFUNCTION(_getHeight);
	ASFUNCTION(_setHeight);
	ASFUNCTION(attachNetStream);
	void renderImpl(RenderContext& ctxt) const;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	_NR<InteractiveObject> hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
};

class SoundLoaderContext : public ASObject
{
private:
	ASPROPERTY_GETTER_SETTER(number_t,bufferTime);
	ASPROPERTY_GETTER_SETTER(bool,checkPolicyFile);
public:
	SoundLoaderContext(Class_base* c):ASObject(c){}
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
};

class StageVideo : public EventDispatcher
{
public:
	StageVideo(Class_base* c):EventDispatcher(c){}
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
};

class StageVideoAvailability : public ASObject
{
public:
	StageVideoAvailability(Class_base* c):ASObject(c){}
	static void sinit(Class_base*);
};

class VideoStatus : public ASObject
{
public:
	VideoStatus(Class_base* c):ASObject(c){}
	static void sinit(Class_base*);
};

};

#endif /* SCRIPTING_FLASH_MEDIA_FLASHMEDIA_H */
