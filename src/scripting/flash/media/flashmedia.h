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

#include "forwards/backends/netutils.h"
#include "interfaces/backends/netutils.h"
#include "compat.h"
#include "asobject.h"
#include "timer.h"
#include "backends/graphics.h"
#include "backends/decoder.h"
#include "backends/urlutils.h"
#include "threading.h"
#include "scripting/flash/display/DisplayObject.h"

namespace lightspark
{

class AudioStream;
class AudioDecoder;
class NetStream;
class StreamCache;
class SoundChannel;
class DefineVideoStreamTag;
class DefineSoundTag;

class Sound: public EventDispatcher, public ILoadable
{
protected:
	URLInfo url;
	std::vector<uint8_t> postData;
	Downloader* downloader;
	_NR<StreamCache> soundData;
	SoundChannel* soundChannel;
	StreamDecoder* rawDataStreamDecoder;
	int32_t rawDataStartPosition;
	streambuf* rawDataStreamBuf;
	istream* rawDataStream;
	number_t buffertime;
	// If container is true, audio format is parsed from
	// soundData. If container is false, soundData is raw samples
	// and format is defined by format member.
	bool container;
	ACQUIRE_RELEASE_FLAG(sampledataprocessed);
	AudioFormat format;
	ASPROPERTY_GETTER(uint32_t,bytesLoaded);
	ASPROPERTY_GETTER(uint32_t,bytesTotal);
	ASPROPERTY_GETTER(number_t,length);
	//ILoadable interface
	void setBytesTotal(uint32_t b) override;
	void setBytesLoaded(uint32_t b) override;
	_NR<ProgressEvent> progressEvent;
	void setSoundChannel(SoundChannel* channel);
public:
	Sound(ASWorker* wrk,Class_base* c);
	Sound(ASWorker* wrk, Class_base* c, _R<StreamCache> soundData, AudioFormat format, number_t duration_in_ms);
	~Sound();
	static void sinit(Class_base*);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(load);
	ASFUNCTION_ATOM(play);
	ASFUNCTION_ATOM(close);
	ASFUNCTION_ATOM(extract);
	ASFUNCTION_ATOM(loadCompressedDataFromByteArray);
	void afterExecution(_R<Event> e) override;
	void requestSampleDataEvent(size_t position);
	number_t getBufferTime() const { return buffertime; }
	inline bool getSampleDataProcessed() const { return container && ACQUIRE_READ(sampledataprocessed); }
};

class SoundTransform: public ASObject
{
public:
	SoundTransform(ASWorker* wrk,Class_base* c);
	ASPROPERTY_GETTER_SETTER(number_t,volume);
	ASPROPERTY_GETTER_SETTER(number_t,pan);
	ASPROPERTY_GETTER_SETTER(number_t,leftToLeft);
	ASPROPERTY_GETTER_SETTER(number_t,leftToRight);
	ASPROPERTY_GETTER_SETTER(number_t,rightToLeft);
	ASPROPERTY_GETTER_SETTER(number_t,rightToRight);
	static void sinit(Class_base*);
	bool destruct() override;
	ASFUNCTION_ATOM(_constructor);
};

class SoundChannel : public EventDispatcher, public IThreadJob
{
private:
	uint32_t buffertimeseconds;
	_NR<StreamCache> stream;
	Sound* sampleproducer;
	Mutex mutex;
	ACQUIRE_RELEASE_FLAG(starting);
	ACQUIRE_RELEASE_FLAG(stopped);
	ACQUIRE_RELEASE_FLAG(terminated);
	ACQUIRE_RELEASE_FLAG(stopping);
	ACQUIRE_RELEASE_FLAG(finished);
	AudioDecoder* audioDecoder;
	AudioStream* audioStream;
	AudioFormat format;
	const SOUNDINFO* soundinfo;
	number_t oldVolume;
	void validateSoundTransform(_NR<SoundTransform>);
	void playStream();
	void playStreamFromSamples();
	number_t startTime;
	int32_t loopstogo;
	uint32_t streamposition;
	bool streamdatafinished;
	bool restartafterabort;
	bool forstreaming;
	void checkEnvelope();
public:
	SoundChannel(ASWorker* wrk,Class_base* c, uint32_t _buffertimeseconds=1, _NR<StreamCache> stream=NullRef, AudioFormat format=AudioFormat(CODEC_NONE,0,0), const SOUNDINFO* _soundinfo=nullptr
			, Sound* _sampleproducer = nullptr, bool _forstreaming=false);
	~SoundChannel();
	DefineSoundTag* fromSoundTag;
	void appendStreamBlock(unsigned char* buf, int len);
	void appendSampleData(ByteArray* data);
	void play(number_t starttime=0);
	void resume();
	void markFinished(); // indicates that all sound data is available
	void setSampleProducer(Sound* _sampleproducer) { sampleproducer = _sampleproducer; }
	void setStartTime(number_t starttime) { startTime = starttime; }
	void setLoops(int32_t loops) {loopstogo=loops;}
	static void sinit(Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	bool isPlaying() { return !ACQUIRE_READ(stopped); }
	bool isStarting() { return ACQUIRE_READ(starting); }
	ASPROPERTY_GETTER_SETTER(_NR<SoundTransform>,soundTransform);
	ASPROPERTY_GETTER(number_t,leftPeak);
	ASPROPERTY_GETTER(number_t,rightPeak);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(stop);
	ASFUNCTION_ATOM(getPosition);

	//IThreadJob interface
	void execute() override;
	void jobFence() override;
	void threadAbort() override;
	Semaphore semSampleData;
};

class Video: public DisplayObject
{
private:
	mutable Mutex mutex;
	uint32_t width, height;
	mutable uint32_t videoWidth, videoHeight;
	_NR<NetStream> netStream;
	DefineVideoStreamTag* videotag;
	VideoDecoder* embeddedVideoDecoder;
	uint32_t lastuploadedframe;
	bool isRendering;
	void resetDecoder();
public:
	Video(ASWorker* wk,Class_base* c, uint32_t w=320, uint32_t h=240, DefineVideoStreamTag* v=nullptr);
	bool destruct() override;
	void finalize() override;
	void advanceFrame(bool implicit) override;
	void refreshSurfaceState() override;
	void requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh=false) override;
	IDrawable* invalidate(bool smoothing) override;
	void checkRatio(uint32_t ratio, bool inskipping) override;
	void afterLegacyInsert() override;
	void afterLegacyDelete(bool inskipping) override;
	void setOnStage(bool staged, bool force, bool inskipping=false) override;
	uint32_t getTagID() const override;
	~Video();
	static void sinit(Class_base*);
	ASPROPERTY_GETTER_SETTER(int32_t, deblocking);
	ASPROPERTY_GETTER_SETTER(bool, smoothing);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getVideoWidth);
	ASFUNCTION_ATOM(_getVideoHeight);
	ASFUNCTION_ATOM(_getWidth);
	ASFUNCTION_ATOM(_setWidth);
	ASFUNCTION_ATOM(_getHeight);
	ASFUNCTION_ATOM(_setHeight);
	ASFUNCTION_ATOM(attachNetStream);
	ASFUNCTION_ATOM(clear);
	bool boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax, bool visibleOnly) override;
	_NR<DisplayObject> hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, DisplayObject::HIT_TYPE type,bool interactiveObjectsOnly) override;
};

class SoundMixer : public ASObject
{
public:
	SoundMixer(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base*);
	ASFUNCTION_GETTER_SETTER(bufferTime);
	ASFUNCTION_GETTER_SETTER(soundTransform);
	ASFUNCTION_ATOM(stopAll);
	ASFUNCTION_ATOM(computeSpectrum);
};
class SoundLoaderContext : public ASObject
{
public:
	SoundLoaderContext(ASWorker* wrk,Class_base* c):ASObject(wrk,c),bufferTime(1000),checkPolicyFile(false){}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(number_t,bufferTime);
	ASPROPERTY_GETTER_SETTER(bool,checkPolicyFile);
};

class StageVideo : public EventDispatcher
{
private:
	mutable Mutex mutex;
	mutable uint32_t videoWidth, videoHeight;
	_NR<NetStream> netStream;
public:
	StageVideo(ASWorker* wrk,Class_base* c);
	void finalize() override;
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getVideoWidth);
	ASFUNCTION_ATOM(_getVideoHeight);
	ASFUNCTION_ATOM(attachNetStream);
};

class StageVideoAvailability : public ASObject
{
public:
	StageVideoAvailability(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base*);
};

class VideoStatus : public ASObject
{
public:
	VideoStatus(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base*);
};

class Microphone : public ASObject
{
public:
	Microphone(ASWorker* wrk,Class_base* c):ASObject(wrk,c),isSupported(false){}
	static void sinit(Class_base*);
	ASPROPERTY_GETTER(bool ,isSupported);
	ASFUNCTION_ATOM(getMicrophone);
};
class Camera : public EventDispatcher
{
public:
	Camera(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c),isSupported(false){}
	static void sinit(Class_base*);
	ASPROPERTY_GETTER(bool ,isSupported);
};
class VideoStreamSettings : public ASObject
{
public:
	VideoStreamSettings(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(setKeyFrameInterval);
	ASFUNCTION_ATOM(setMode);
};
class H264VideoStreamSettings : public VideoStreamSettings
{
public:
	H264VideoStreamSettings(ASWorker* wrk,Class_base* c):VideoStreamSettings(wrk,c){}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(setProfileLevel);
	ASPROPERTY_GETTER_SETTER(tiny_string, codec);
	ASPROPERTY_GETTER(tiny_string, level);
	ASPROPERTY_GETTER(tiny_string, profile);
};

}

#endif /* SCRIPTING_FLASH_MEDIA_FLASHMEDIA_H */
