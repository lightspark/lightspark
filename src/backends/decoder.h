/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _DECODER_H
#define _DECODER_H

#include "compat.h"
#include "threading.h"
#include "graphics.h"
#ifdef ENABLE_LIBAVCODEC
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#define MAX_AUDIO_FRAME_SIZE AVCODEC_MAX_AUDIO_FRAME_SIZE
}
#else
// Correct size? 192000?
// TODO: a real plugins system
#define MAX_AUDIO_FRAME_SIZE 20
#endif

namespace lightspark
{

enum LS_VIDEO_CODEC { H264=0, H263, VP6 };
enum LS_AUDIO_CODEC { LINEAR_PCM_PLATFORM_ENDIAN=0, ADPCM=1, MP3=2, LINEAR_PCM_LE=3, AAC=10 };

class Decoder
{
protected:
	enum STATUS { PREINIT=0, INIT, VALID, FLUSHED};
	STATUS status;
	bool flushing;
	Semaphore flushed;
public:
	Decoder():status(PREINIT),flushing(false),flushed(0){}
	virtual ~Decoder(){}
	bool isValid() const DLL_PUBLIC
	{
		return status>=VALID;
	}
	virtual void setFlushing()=0;
	void waitFlushed()
	{
		flushed.wait();
	}
};

class VideoDecoder: public Decoder, public ITextureUploadable
{
private:
	bool resizeGLBuffers;
protected:
	/*
		Derived classes must spinwaits on this to become false before deleting
	*/
	ATOMIC_INT32(fenceCount);
	uint32_t frameWidth;
	uint32_t frameHeight;
	bool setSize(uint32_t w, uint32_t h);
	bool resizeIfNeeded(TextureChunk& tex);
	LS_VIDEO_CODEC videoCodec;
	TextureChunk videoTexture;
public:
	VideoDecoder():resizeGLBuffers(false),fenceCount(0),frameWidth(0),frameHeight(0),frameRate(0){}
	virtual ~VideoDecoder(){};
	virtual bool decodeData(uint8_t* data, uint32_t datalen, uint32_t time)=0;
	virtual bool discardFrame()=0;
	virtual void skipUntil(uint32_t time)=0;
	virtual void skipAll()=0;
	uint32_t getWidth()
	{
		return frameWidth;
	}
	uint32_t getHeight()
	{
		return frameHeight;
	}
	double frameRate;
	/*
		Useful to avoid destruction of the object while a pending upload is waiting
	*/
	void waitForFencing();
	//ITextureUploadable interface
	void sizeNeeded(uint32_t& w, uint32_t& h) const;
	const TextureChunk& getTexture();
	void uploadFence();
};

class NullVideoDecoder: public VideoDecoder
{
public:
	NullVideoDecoder() {status=VALID;}
	~NullVideoDecoder() { while(fenceCount); }
	bool decodeData(uint8_t* data, uint32_t datalen, uint32_t time){return false;}
	bool discardFrame(){return false;}
	void skipUntil(uint32_t time){}
	void skipAll(){}
	void setFlushing()
	{
		flushing=true;
	}
	//ITextureUploadable interface
	void upload(uint8_t* data, uint32_t w, uint32_t h) const
	{
	}
};

#ifdef ENABLE_LIBAVCODEC
class FFMpegVideoDecoder: public VideoDecoder
{
private:
	class YUVBuffer
	{
	public:
		uint8_t* ch[3];
		uint32_t time;
		YUVBuffer():time(0){ch[0]=NULL;ch[1]=NULL;ch[2]=NULL;}
		~YUVBuffer()
		{
			if(ch[0])
			{
				free(ch[0]);
				free(ch[1]);
				free(ch[2]);
			}
		}
	};
	class YUVBufferGenerator
	{
	private:
		uint32_t bufferSize;
	public:
		YUVBufferGenerator(uint32_t b):bufferSize(b){}
		void init(YUVBuffer& buf) const;
	};
	uint32_t curBuffer;
	uint32_t curBufferOffset;
	AVCodecContext* codecContext;
	bool ownedContext;
	BlockingCircularQueue<YUVBuffer,80> buffers;
	Mutex mutex;
	AVFrame* frameIn;
	void copyFrameToBuffers(const AVFrame* frameIn, uint32_t time);
	void setSize(uint32_t w, uint32_t h);
	bool fillDataAndCheckValidity();
public:
	FFMpegVideoDecoder(LS_VIDEO_CODEC codec, uint8_t* initdata, uint32_t datalen, double frameRateHint);
	/*
	   Specialized constructor used by FFMpegStreamDecoder
	*/
	FFMpegVideoDecoder(AVCodecContext* codecContext, double frameRateHint);
	~FFMpegVideoDecoder();
	/*
	   Specialized decoding used by FFMpegStreamDecoder
	*/
	bool decodePacket(AVPacket* pkt, uint32_t time);
	bool decodeData(uint8_t* data, uint32_t datalen, uint32_t time);
	bool discardFrame();
	void skipUntil(uint32_t time);
	void skipAll();
	void setFlushing()
	{
		flushing=true;
		if(buffers.isEmpty())
		{
			status=FLUSHED;
			flushed.signal();
		}
	}
	//ITextureUploadable interface
	void upload(uint8_t* data, uint32_t w, uint32_t h) const;
};
#endif

class AudioDecoder: public Decoder
{
protected:
	class FrameSamples
	{
	public:
#ifdef _MSC_VER
		// WINTODO: a macro doesn't seem to work
		__declspec(align(16))
#endif
			int16_t samples[MAX_AUDIO_FRAME_SIZE/2];
#ifndef _MSC_VER
		__attribute__ ((aligned (16)))
#endif
		int16_t* current;
		uint32_t len;
		uint32_t time;
		FrameSamples():current(samples),len(0),time(0){}
	};
	class FrameSamplesGenerator
	{
	public:
		void init(FrameSamples& f) const {f.len=0;}
	};
	BlockingCircularQueue<FrameSamples,150> samplesBuffer;
public:
	/**
	  	The AudioDecoder contains audio buffers that must be aligned to 16 bytes, so we redefine the allocator
	*/
	void* operator new(size_t);
	void operator delete(void*);
	AudioDecoder():sampleRate(0),channelCount(0),initialTime(-1){}
	virtual ~AudioDecoder(){};
	virtual uint32_t decodeData(uint8_t* data, uint32_t datalen, uint32_t time)=0;
	bool hasDecodedFrames() const DLL_PUBLIC
	{
		return !samplesBuffer.isEmpty();
	}
	uint32_t getFrontTime() const;
	uint32_t getBytesPerMSec() const
	{
		return sampleRate*channelCount*2/1000;
	}
	uint32_t copyFrame(int16_t* dest, uint32_t len) DLL_PUBLIC;
	/**
	  	Skip samples until the given time

		@param time the desired time in millisecond
		@param a fractional time in microseconds
	*/
	void skipUntil(uint32_t time, uint32_t usecs);
	/**
	  	Skip all the samples
	*/
	void skipAll() DLL_PUBLIC;
	bool discardFrame();
	void setFlushing()
	{
		flushing=true;
		if(samplesBuffer.isEmpty())
		{
			status=FLUSHED;
			flushed.signal();
		}
	}
	uint32_t sampleRate;
	uint32_t channelCount;
	//Saves the timestamp of the first decoded frame
	uint32_t initialTime;
};

class NullAudioDecoder: public AudioDecoder
{
public:
	NullAudioDecoder()
	{
		status=VALID;
		sampleRate=44100;
		channelCount=2;
	}
	uint32_t decodeData(uint8_t* data, uint32_t datalen, uint32_t time){return 0;}
};

#ifdef ENABLE_LIBAVCODEC
class FFMpegAudioDecoder: public AudioDecoder
{
private:
	AVCodecContext* codecContext;
	bool ownedContext;
	bool fillDataAndCheckValidity();
public:
	FFMpegAudioDecoder(LS_AUDIO_CODEC codec, uint8_t* initdata, uint32_t datalen);
	/*
	   Specialized constructor used by FFMpegStreamDecoder
	*/
	FFMpegAudioDecoder(AVCodecContext* codecContext);
	~FFMpegAudioDecoder();
	/*
	   Specialized decoding used by FFMpegStreamDecoder
	*/
	uint32_t decodePacket(AVPacket* pkt, uint32_t time);
	uint32_t decodeData(uint8_t* data, uint32_t datalen, uint32_t time);
};
#endif

class StreamDecoder
{
protected:
	bool valid;
public:
	StreamDecoder():valid(false),audioDecoder(NULL),videoDecoder(NULL){}
	virtual ~StreamDecoder();
	virtual bool decodeNextFrame() = 0;
	virtual bool getMetadataInteger(const char* name, uint32_t& ret) const=0;
	virtual bool getMetadataDouble(const char* name, double& ret) const=0;
	bool isValid() const { return valid; }
	AudioDecoder* audioDecoder;
	VideoDecoder* videoDecoder;
};

#ifdef ENABLE_LIBAVCODEC
class FFMpegStreamDecoder: public StreamDecoder
{
private:
	std::istream& stream;
	AVFormatContext* formatCtx;
	bool audioFound;
	bool videoFound;
	int32_t audioIndex;
	int32_t videoIndex;
	//We use our own copy of these to have access of the ffmpeg specific methods
	FFMpegAudioDecoder* customAudioDecoder;
	FFMpegVideoDecoder* customVideoDecoder;
	//Helpers for custom I/O of libavformat
	uint8_t avioBuffer[4096];
	static int avioReadPacket(void* t, uint8_t* buf, int buf_size);
	//NOTE: this will become AVIOContext in FFMpeg 0.7
#if LIBAVUTIL_VERSION_MAJOR < 51
	ByteIOContext* avioContext;
#else
	AVIOContext* avioContext;
#endif
public:
	FFMpegStreamDecoder(std::istream& s);
	~FFMpegStreamDecoder();
	bool decodeNextFrame();
	bool getMetadataInteger(const char* name, uint32_t& ret) const;
	bool getMetadataDouble(const char* name, double& ret) const;
};
#endif

};
#endif
