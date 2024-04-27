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

#ifndef BACKENDS_DECODER_H
#define BACKENDS_DECODER_H 1

#include "compat.h"
#include "threading.h"
#include "backends/graphics.h"
#ifdef ENABLE_LIBAVCODEC
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#ifdef HAVE_LIBSWRESAMPLE
#include <libswresample/swresample.h>
#elif defined HAVE_LIBAVRESAMPLE
#include <libavresample/avresample.h>
#endif
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#ifndef AVCODEC_MAX_AUDIO_FRAME_SIZE
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#endif
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(54,51,100)
#define CodecID AVCodecID
#define CODEC_ID_NONE AV_CODEC_ID_NONE
#define CODEC_ID_H264 AV_CODEC_ID_H264
#define CODEC_ID_FLV1 AV_CODEC_ID_FLV1
#define CODEC_ID_VP6F AV_CODEC_ID_VP6F
#define CODEC_ID_VP6A AV_CODEC_ID_VP6A
#define CODEC_ID_AAC AV_CODEC_ID_AAC
#define CODEC_ID_MP3 AV_CODEC_ID_MP3
#define CODEC_ID_PCM_S16BE AV_CODEC_ID_PCM_S16BE
#define CODEC_ID_PCM_S16LE AV_CODEC_ID_PCM_S16LE
#define CODEC_ID_PCM_F32BE AV_CODEC_ID_PCM_F32BE
#define CODEC_ID_PCM_F32LE AV_CODEC_ID_PCM_F32LE
#define CODEC_ID_ADPCM_SWF AV_CODEC_ID_ADPCM_SWF
#define CODEC_ID_GIF AV_CODEC_ID_GIF

#endif
#define MAX_AUDIO_FRAME_SIZE AVCODEC_MAX_AUDIO_FRAME_SIZE
}
#else
// Correct size? 192000?
// TODO: a real plugins system
#define MAX_AUDIO_FRAME_SIZE 20
#define AV_INPUT_BUFFER_PADDING_SIZE 0
#endif

namespace lightspark
{

class AudioFormat
{
public:
	AudioFormat(LS_AUDIO_CODEC co, int sr, int ch):codec(co),sampleRate(sr),channels(ch) {}
	LS_AUDIO_CODEC codec;
	int sampleRate;
	int channels;
};
class NetStream;
class EngineData;

class Decoder
{
protected:
	Semaphore flushed;
	enum STATUS { PREINIT=0, INIT, VALID, FLUSHED};
	STATUS status;
	bool flushing;
public:
	Decoder():flushed(0),status(PREINIT),flushing(false){}
	virtual ~Decoder(){}
	bool isValid() const
	{
		return status>=VALID;
	}
	virtual void setFlushing()=0;
	void waitFlushed()
	{
		if (status != VALID)
			return;
		flushed.wait();
	}
};
class DefineVideoStreamTag;
class VideoDecoder: public Decoder, public ITextureUploadable
{
protected:
	uint8_t* decodedframebuffer;
public:
	VideoDecoder();
	virtual ~VideoDecoder();
	virtual void switchCodec(LS_VIDEO_CODEC codecId, uint8_t* initdata, uint32_t datalen, double frameRateHint)=0;
	virtual bool decodeData(uint8_t* data, uint32_t datalen, uint32_t time)=0;
	virtual bool discardFrame()=0;
	virtual uint32_t skipUntil(uint32_t time)=0;
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
	uint32_t framesdecoded;
	uint32_t framesdropped;
	/*
		Useful to avoid destruction of the object while a pending upload is waiting
	*/
	void waitForFencing();
	//ITextureUploadable interface
	void sizeNeeded(uint32_t& w, uint32_t& h) const;
	TextureChunk& getTexture();
	void uploadFence();
	void markForDestruction();
	bool isUploading() { return fenceCount; }
	void setVideoFrameToDecode(uint32_t frame) { currentframe=frame; }
	void clearFrameBuffer();
protected:
	TextureChunk videoTexture;
	uint32_t frameWidth;
	uint32_t frameHeight;
	uint32_t lastframe;
	uint32_t currentframe;
	/*
		Derived classes must spinwaits on this to become false before deleting
	*/
	ATOMIC_INT32(fenceCount);
	bool setSize(uint32_t w, uint32_t h);
	bool resizeIfNeeded(TextureChunk& tex);
	LS_VIDEO_CODEC videoCodec;
private:
	bool resizeGLBuffers;
	bool markedForDeletion;
};

class NullVideoDecoder: public VideoDecoder
{
public:
	NullVideoDecoder() {status=VALID;}
	~NullVideoDecoder() { while(fenceCount); }
	void switchCodec(LS_VIDEO_CODEC codecId, uint8_t* initdata, uint32_t datalen, double frameRateHint) override {}
	bool decodeData(uint8_t* data, uint32_t datalen, uint32_t time) override {return false;}
	bool discardFrame() override {return false;}
	uint32_t skipUntil(uint32_t time) override { return 0;}
	void skipAll() override {}
	void setFlushing() override
	{
		flushing=true;
	}
	//ITextureUploadable interface
	uint8_t* upload(bool refresh) override
	{
		return nullptr;
	}
};
#ifdef ENABLE_LIBAVCODEC
#define FFMPEGVIDEODECODERBUFFERSIZE 80
class FFMpegVideoDecoder: public VideoDecoder
{
private:
	class YUVBuffer
	{
	YUVBuffer(const YUVBuffer&); /* no impl */
	YUVBuffer& operator=(const YUVBuffer&); /* no impl */
	public:
		uint8_t* ch[4];
		uint32_t time;
		YUVBuffer():time(0){ch[0]=nullptr;ch[1]=nullptr;ch[2]=nullptr;ch[3]=nullptr;}
		~YUVBuffer()
		{
			setDecodedData(nullptr);
		}
		void setDecodedData(uint8_t* data)
		{
			if(ch[0])
				aligned_free(ch[0]);
			if(ch[1])
				aligned_free(ch[1]);
			if(ch[2])
				aligned_free(ch[2]);
			if(ch[3])
				aligned_free(ch[3]);
			ch[0]=data;
			ch[1]=nullptr;
			ch[2]=nullptr;
			ch[3]=nullptr;
		}
		void init()
		{
			ch[0]=nullptr;
			ch[1]=nullptr;
			ch[2]=nullptr;
			ch[3]=nullptr;
		}
		void cleanup()
		{
			setDecodedData(nullptr);
		}
	};
	class YUVBufferGenerator
	{
	private:
		uint32_t bufferSize;
		bool hasAlpha;
		bool hasChannels;
	public:
		YUVBufferGenerator(uint32_t b, bool _hasalpha, bool _haschannels):bufferSize(b),hasAlpha(_hasalpha),hasChannels(_haschannels){}
		void init(YUVBuffer& buf) const;
	};
	bool ownedContext;
	uint32_t curBuffer;
	AVCodecContext* codecContext;
	BlockingCircularQueue<YUVBuffer> streamingbuffers;
	BlockingCircularQueue<YUVBuffer> embeddedbuffers;
	AVFrame* frameIn;
	void copyFrameToBuffers(const AVFrame* frameIn, uint32_t time);
	void setSize(uint32_t w, uint32_t h);
	bool fillDataAndCheckValidity();
	uint32_t curBufferOffset;
	DefineVideoStreamTag* embeddedvideotag;
public:
	FFMpegVideoDecoder(LS_VIDEO_CODEC codec, uint8_t* initdata, uint32_t datalen, double frameRateHint,DefineVideoStreamTag* tag=nullptr);
	/*
	   Specialized constructor used by FFMpegStreamDecoder
	*/
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
	FFMpegVideoDecoder(AVCodecParameters* codecPar, double frameRateHint);
#else
	FFMpegVideoDecoder(AVCodecContext* codecContext, double frameRateHint);
#endif
	~FFMpegVideoDecoder();
	/*
	   Specialized decoding used by FFMpegStreamDecoder
	*/
	bool decodePacket(AVPacket* pkt, uint32_t time);
	void switchCodec(LS_VIDEO_CODEC codecId, uint8_t* initdata, uint32_t datalen, double frameRateHint) override;
	bool decodeData(uint8_t* data, uint32_t datalen, uint32_t time) override;
	bool discardFrame() override;
	uint32_t skipUntil(uint32_t time) override;
	void skipAll() override;
	void setFlushing() override
	{
		flushing=true;
		if (embeddedvideotag)
		{
			if(embeddedbuffers.isEmpty())
			{
				status=FLUSHED;
				flushed.signal();
			}
		}
		else
		{
			if(streamingbuffers.isEmpty())
			{
				status=FLUSHED;
				flushed.signal();
			}
		}
	}
	//ITextureUploadable interface
	uint8_t* upload(bool refresh) override;
};
#endif

class AudioDecoder: public Decoder
{
protected:
	class FrameSamplesS16
	{
	public:
		int16_t samples[MAX_AUDIO_FRAME_SIZE/2];
		__attribute__ ((aligned (16)))
		int16_t* current;
		uint32_t len;
		uint32_t time;
		FrameSamplesS16():current(samples),len(0),time(0){}
		void init()
		{
			len=0;
			time=0;
		}
		void cleanup()
		{
		}
		
	};
	class FrameSamplesF32
	{
	public:
		float samples[MAX_AUDIO_FRAME_SIZE/2];
		__attribute__ ((aligned (32)))
		float* current;
		uint32_t len;
		uint32_t time;
		FrameSamplesF32():current(samples),len(0),time(0){}
		void init()
		{
			len=0;
			time=0;
		}
		void cleanup()
		{
		}
		
	};
#ifdef HAVE_LIBSWRESAMPLE
	SwrContext* resamplecontext;
#elif defined HAVE_LIBAVRESAMPLE
	AVAudioResampleContext* resamplecontext;
#endif
public:
	uint32_t sampleRate;
	EngineData* engine;
protected:
	BlockingCircularQueue<FrameSamplesS16> samplesBufferS16;
	BlockingCircularQueue<FrameSamplesF32> samplesBufferF32;
	virtual void samplesconsumed(uint32_t samples) {}
	bool discardFrameS16();
	bool discardFrameF32();
public:
	/**
	  	The AudioDecoder contains audio buffers that must be aligned to 16 bytes, so we redefine the allocator
	*/
	AudioDecoder(uint32_t size, EngineData* _engine);
	virtual ~AudioDecoder();
	virtual void switchCodec(LS_AUDIO_CODEC codecId, uint8_t* initdata, uint32_t datalen)=0;
	virtual uint32_t decodeData(uint8_t* data, int32_t datalen, uint32_t time)=0;
	bool hasDecodedFrames() const
	{
		return !samplesBufferS16.isEmpty() || !samplesBufferF32.isEmpty();
	}
	uint32_t getFrontTime() const;
	uint32_t getBytesPerMSec() const
	{
		return sampleRate*channelCount*2/1000;
	}
	uint32_t copyFrameS16(int16_t* dest, uint32_t len) DLL_PUBLIC;
	uint32_t copyFrameF32(float* dest, uint32_t len) DLL_PUBLIC;
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
	void setFlushing() override
	{
		flushing=true;
		if(samplesBufferS16.isEmpty() && samplesBufferF32.isEmpty())
		{
			status=FLUSHED;
			flushed.signal();
		}
	}
	uint32_t channelCount;
	//Saves the timestamp of the first decoded frame
	uint32_t initialTime;
	// if true, decoder is only used for extracting parts meaning audio data will always be provided as 32bit floats for 2 channels
	bool forExtraction;
};

class NullAudioDecoder: public AudioDecoder
{
public:
	NullAudioDecoder():AudioDecoder(0,nullptr)
	{
		status=VALID;
		sampleRate=44100;
		channelCount=2;
	}
	void switchCodec(LS_AUDIO_CODEC codecId, uint8_t* initdata, uint32_t datalen) override {}
	uint32_t decodeData(uint8_t* data, int32_t datalen, uint32_t time) override {return 0;}
};

class SoundChannel;
// this is the AudioDecoder for streaming Sounds by SampleDataEvent
class SampleDataAudioDecoder: public AudioDecoder
{
private:
	ATOMIC_INT32(bufferedsamples);
	SoundChannel* soundchannel;
protected:
	void samplesconsumed(uint32_t samples) override;
public:
	SampleDataAudioDecoder(SoundChannel* _soundchannel,uint32_t buffertime,EngineData* engine):AudioDecoder(buffertime,engine),soundchannel(_soundchannel)
	{
		sampleRate=44100;
		channelCount=2;
		bufferedsamples=0;
	}
	void switchCodec(LS_AUDIO_CODEC codecId, uint8_t* initdata, uint32_t datalen) override {}
	// the data is always expected to be floats
	uint32_t decodeData(uint8_t* data, int32_t datalen, uint32_t time) override;
	inline int32_t getBufferedSamples() const { return  bufferedsamples; }
};


#ifdef ENABLE_LIBAVCODEC
class FFMpegAudioDecoder: public AudioDecoder
{
private:
	bool ownedContext;
	AVCodecContext* codecContext;
	std::vector<uint8_t> overflowBuffer;
	bool fillDataAndCheckValidity();
	CodecID LSToFFMpegCodec(LS_AUDIO_CODEC lscodec);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	AVFrame* frameIn;
	void resampleFrame(uint8_t** output, int& outputsize);
#endif
public:
	FFMpegAudioDecoder(EngineData* eng,LS_AUDIO_CODEC codec, uint8_t* initdata, uint32_t datalen, uint32_t buffertime);
	FFMpegAudioDecoder(EngineData* eng,LS_AUDIO_CODEC codec, int sampleRate, int channels, uint32_t buffertime,bool);
	/*
	   Specialized constructor used by FFMpegStreamDecoder
	*/
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
	FFMpegAudioDecoder(EngineData* eng,AVCodecParameters* codecPar, uint32_t buffertime);
#else
	FFMpegAudioDecoder(EngineData* eng,AVCodecContext* codecContext, uint32_t buffertime);
#endif
	~FFMpegAudioDecoder();
	/*
	   Specialized decoding used by FFMpegStreamDecoder
	*/
	void decodePacket(AVPacket* pkt, uint32_t time);
	void switchCodec(LS_AUDIO_CODEC audioCodec, uint8_t* initdata, uint32_t datalen) override;
	uint32_t decodeData(uint8_t* data, int32_t datalen, uint32_t time) override;
};
#endif

class StreamDecoder
{
public:
	StreamDecoder():audioDecoder(nullptr),videoDecoder(nullptr),valid(false),hasvideo(false){}
	virtual ~StreamDecoder();
	virtual bool decodeNextFrame() = 0;
	virtual void jumpToPosition(number_t position) = 0;
	bool isValid() const { return valid; }
	AudioDecoder* audioDecoder;
	VideoDecoder* videoDecoder;
	bool hasVideo() const { return hasvideo; }
protected:
	bool valid;
	bool hasvideo;
};

#ifdef ENABLE_LIBAVCODEC
class FFMpegStreamDecoder: public StreamDecoder
{
private:
	NetStream* netstream;
	bool audioFound;
	bool videoFound;
	std::istream& stream;
	AVFormatContext* formatCtx;
	int32_t audioIndex;
	int32_t videoIndex;
	//We use our own copy of these to have access of the ffmpeg specific methods
	FFMpegAudioDecoder* customAudioDecoder;
	FFMpegVideoDecoder* customVideoDecoder;
	//Helpers for custom I/O of libavformat
	uint8_t* avioBuffer;
	static int avioReadPacket(void* t, uint8_t* buf, int buf_size);
	static int64_t avioSeek(void *opaque, int64_t offset, int whence);
	//NOTE: this will become AVIOContext in FFMpeg 0.7
#if LIBAVUTIL_VERSION_MAJOR < 51
	ByteIOContext* avioContext;
#else
	AVIOContext* avioContext;
#endif
	int availablestreamlength;
	int fullstreamlength;
public:
	FFMpegStreamDecoder(NetStream* ns,EngineData* eng,std::istream& s, uint32_t buffertime, AudioFormat* format = nullptr, int streamsize = -1, bool forExtraction=false);
	~FFMpegStreamDecoder();
	void jumpToPosition(number_t position) override;
	bool decodeNextFrame() override;
	int getAudioSampleRate();
};
#endif

}
#endif /* BACKENDS_DECODER */
