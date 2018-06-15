/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "scripting/flash/display/DisplayObject.h"

class AudioStream;

namespace lightspark
{

class AudioDecoder;
class NetStream;
class StreamCache;

class Sound: public EventDispatcher, public ILoadable
{
private:
	URLInfo url;
	std::vector<uint8_t> postData;
	Downloader* downloader;
	_R<StreamCache> soundData;
	// If container is true, audio format is parsed from
	// soundData. If container is false, soundData is raw samples
	// and format is defined by format member.
	bool container;
	AudioFormat format;
	ASPROPERTY_GETTER(uint32_t,bytesLoaded);
	ASPROPERTY_GETTER(uint32_t,bytesTotal);
	ASPROPERTY_GETTER(number_t,length);
	//ILoadable interface
	void setBytesTotal(uint32_t b);
	void setBytesLoaded(uint32_t b);
public:
	Sound(Class_base* c);
	Sound(Class_base* c, _R<StreamCache> soundData, AudioFormat format);
	~Sound();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(load);
	ASFUNCTION_ATOM(play);
	ASFUNCTION_ATOM(close);
};

class SoundTransform: public ASObject
{
public:
	SoundTransform(Class_base* c);
	ASPROPERTY_GETTER_SETTER(number_t,volume);
	ASPROPERTY_GETTER_SETTER(number_t,pan);
	ASPROPERTY_GETTER_SETTER(number_t,leftToLeft);
	ASPROPERTY_GETTER_SETTER(number_t,leftToRight);
	ASPROPERTY_GETTER_SETTER(number_t,rightToLeft);
	ASPROPERTY_GETTER_SETTER(number_t,rightToRight);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class SoundChannel : public EventDispatcher, public IThreadJob
{
private:
	_NR<StreamCache> stream;
	Mutex mutex;
	ACQUIRE_RELEASE_FLAG(stopped);
	AudioDecoder* audioDecoder;
	AudioStream* audioStream;
	AudioFormat format;
	number_t oldVolume;
	ASPROPERTY_GETTER_SETTER(_NR<SoundTransform>,soundTransform);
	void validateSoundTransform(_NR<SoundTransform>);
	void playStream();
public:
	SoundChannel(Class_base* c, _NR<StreamCache> stream=NullRef, AudioFormat format=AudioFormat(CODEC_NONE,0,0));
	~SoundChannel();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	void finalize();
	ASPROPERTY_GETTER(number_t,leftPeak);
	ASPROPERTY_GETTER(number_t,position);
	ASPROPERTY_GETTER(number_t,rightPeak);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(stop);

	//IThreadJob interface
	void execute();
	void jobFence();
	void threadAbort();
};

class Video: public DisplayObject
{
private:
	mutable Mutex mutex;
	uint32_t width, height;
	mutable uint32_t videoWidth, videoHeight;
	bool initialized;
	_NR<NetStream> netStream;
	ASPROPERTY_GETTER_SETTER(int32_t, deblocking);
	ASPROPERTY_GETTER_SETTER(bool, smoothing);
public:
	Video(Class_base* c, uint32_t w=320, uint32_t h=240);
	void finalize();
	~Video();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getVideoWidth);
	ASFUNCTION_ATOM(_getVideoHeight);
	ASFUNCTION_ATOM(_getWidth);
	ASFUNCTION_ATOM(_setWidth);
	ASFUNCTION_ATOM(_getHeight);
	ASFUNCTION_ATOM(_setHeight);
	ASFUNCTION_ATOM(attachNetStream);
	ASFUNCTION_ATOM(clear);
	void renderImpl(RenderContext& ctxt) const;
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const;
	_NR<DisplayObject> hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type);
};

class SoundMixer : public ASObject
{
public:
	SoundMixer(Class_base* c):ASObject(c),bufferTime(0){}
	static void sinit(Class_base*);
	ASPROPERTY_GETTER_SETTER(number_t,bufferTime);
	ASPROPERTY_GETTER_SETTER(_NR<SoundTransform>,soundTransform);
	ASFUNCTION_ATOM(stopAll);
	ASFUNCTION_ATOM(computeSpectrum);
};
class SoundLoaderContext : public ASObject
{
private:
	ASPROPERTY_GETTER_SETTER(number_t,bufferTime);
	ASPROPERTY_GETTER_SETTER(bool,checkPolicyFile);
public:
	SoundLoaderContext(Class_base* c):ASObject(c){}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class StageVideo : public EventDispatcher
{
private:
	mutable Mutex mutex;
	mutable uint32_t videoWidth, videoHeight;
	_NR<NetStream> netStream;
public:
	StageVideo(Class_base* c):EventDispatcher(c),videoWidth(0),videoHeight(0){}
	void finalize();
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getVideoWidth);
	ASFUNCTION_ATOM(_getVideoHeight);
	ASFUNCTION_ATOM(attachNetStream);
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

class Microphone : public ASObject
{
public:
	Microphone(Class_base* c):ASObject(c),isSupported(false){}
	static void sinit(Class_base*);
	ASPROPERTY_GETTER(bool ,isSupported);
	ASFUNCTION_ATOM(getMicrophone);
};
class Camera : public EventDispatcher
{
public:
	Camera(Class_base* c):EventDispatcher(c),isSupported(false){}
	static void sinit(Class_base*);
	ASPROPERTY_GETTER(bool ,isSupported);
};
class VideoStreamSettings : public ASObject
{
public:
	VideoStreamSettings(Class_base* c):ASObject(c){}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(setKeyFrameInterval);
	ASFUNCTION_ATOM(setMode);
};
class H264VideoStreamSettings : public VideoStreamSettings
{
public:
	H264VideoStreamSettings(Class_base* c):VideoStreamSettings(c){}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};
	
}

#endif /* SCRIPTING_FLASH_MEDIA_FLASHMEDIA_H */
