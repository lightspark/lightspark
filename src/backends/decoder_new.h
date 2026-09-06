/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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

#ifndef BACKENDS_DECODER_H
#define BACKENDS_DECODER_H 1

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

#include "compat.h"
#include "threading.h"
#include "backends/graphics.h"

namespace lightspark
{

class DefineVideoStreamTag;
class EngineData;
class NetStream;
class SoundChannel;

struct AudioFormat
{
	LS_AUDIO_CODEC codec;
	size_t sampleRate;
	uint8_t channels;

	AudioFormat
	(
		const LS_AUDIO_CODEC& _codec,
		size_t _sampleRate,
		uint8_t _channels
	) :
	codec(_codec),
	sampleRate(_sampleRate),
	channels(_channels) {}
};

class Decoder
{
protected:
	enum STATUS
	{
		PREINIT = 0,
		INIT,
		VALID,
		FLUSHED
	};

	Semaphore flushed;
	STATUS status;
	bool flushing;
public:
	Decoder() : flushed(0), status(PREINIT), flushing(false) {}
	virtual ~Decoder() {}

	bool isValid() const { return status >= VALID; }
	virtual void setFlushing() = 0;
	void waitFlushed()
	{
		if (status != VALID)
			return;
		flushed.wait();
	}

	bool isFlushed() const { return status == FLUSHED; }
};

class VideoDecoder : public Decoder, public ITextureUploadable
{
private:
	bool resizeGLBuffers;
	bool markedForDeletion;
protected:
	uint8_t* decodedFrameBuffer;

	TextureChunk videoTexture;
	Vector2u frameSize;
	size_t lastFrame;
	size_t currentFrame;
	/*
		Derived classes must spinwaits on this to become false before deleting
	*/
	ACQUIRE_RELEASE_VARIABLE(size_t, fenceCount);
	LS_VIDEO_CODEC videoCodec;

	bool setSize(const Vector2u& size);
	bool resizeIfNeeded(TextureChunk& tex);
public:
	number_t frameRate;
	size_t framesDecoded;
	size_t framesDropped;

	VideoDecoder();
	virtual ~VideoDecoder();
	virtual void switchCodec
	(
		const LS_VIDEO_CODEC& codecId,
		Span<const uint8_t> initData,
		number_t frameRateHint
	) = 0;

	virtual bool decodeData
	(
		Span<const uint8_t> data,
		Optional<const TimeSpec&> time
	) = 0;

	virtual bool discardFrame() = 0;
	virtual size_t skipUntil(const TimeSpec& time) = 0;
	virtual void skipAll() = 0;
	uint32_t getWidth() const { return frameSize.x; }
	uint32_t getHeight() const { return frameSize.y; }
	const Vector2u& getSize() const { return frameSize; }
	/*
		Useful to avoid destruction of the object while a pending upload is waiting
	*/
	void waitForFencing();
	//ITextureUploadable interface
	void sizeNeeded(uint32_t& w, uint32_t& h) const override;
	TextureChunk& getTexture() override;
	void uploadFence() override;
	void markForDestruction();
	bool isUploading() const { return fenceCount; }
	void setVideoFrameToDecode(size_t frame) { currentFrame = frame; }
	void clearFrameBuffer();
};

class NullVideoDecoder : public VideoDecoder
{
public:
	NullVideoDecoder() : status(VALID) {}
	~NullVideoDecoder() { while (fenceCount); }
	void switchCodec
	(
		const LS_VIDEO_CODEC& codecId,
		Span<const uint8_t> initData,
		number_t frameRateHint
	) override {}

	bool decodeData
	(
		Span<const uint8_t> data,
		Optional<const TimeSpec&> time
	) override {}

	bool discardFrame() override { return false; }
	size_t skipUntil(const TimeSpec& time) override { return 0; }
	void skipAll() override {}
	void setFlushing() override { flushing = true; }
	//ITextureUploadable interface
	uint8_t* upload(bool refresh) override { return nullptr; }
};

#ifdef ENABLE_LIBAVCODEC
class FFMpegVideoDecoder : public VideoDecoder
{
private:
	constexpr static size_t _bufferSize = 80;

	struct YUVBuffer
	{
	public:
		uint8_t* ch[4];
		TimeSpec time;

		YUVBuffer() : ch({ nullptr, nullptr, nullptr, nullptr }) {}
		YUVBuffer(const YUVBuffer&) = delete;
		~YUVBuffer() { setDecodedData(nullptr); }
		YUVBuffer& operator=(const YUVBuffer&) = delete;

		void setDecodedData(uint8_t* data)
		{
			if (ch[0] != nullptr)
				aligned_free(ch[0]);
			if (ch[1] != nullptr)
				aligned_free(ch[1]);
			if (ch[2] != nullptr)
				aligned_free(ch[2]);
			if (ch[3] != nullptr)
				aligned_free(ch[3]);
			ch[0] = data;
			ch[1] = nullptr;
			ch[2] = nullptr;
			ch[3] = nullptr;
		}

		void init()
		{
			ch[0] = nullptr;
			ch[1] = nullptr;
			ch[2] = nullptr;
			ch[3] = nullptr;
		}

		void cleanup() { setDecodedData(nullptr); }
	};

	class YUVBufferGenerator
	{
	private:
		size_t bufferSize;
		bool hasAlpha;
		bool hasChannels;
	public:
		YUVBufferGenerator
		(
			size_t size,
			bool _hasAlpha,
			bool _hasChannels
		) :
		bufferSize(size),
		hasAlpha(_hasAlpha),
		hasChannels(_hasChannels) {}

		void init(YUVBuffer& buf) const;
	};

	bool ownedContext;
	AVCodecContext* codecContext;
	size_t curBuffer;
	size_t curBufferOffset;
	DefineVideoStreamTag* embeddedVideoTag;
	BlockingCircularQueue<YUVBuffer> streamingBuffers;
	BlockingCircularQueue<YUVBuffer> embeddedBuffers;
	AVFrame* frameIn;

	const BlockingCircularQueue<YUVBuffer>& getBuffers() const
	{
		bool isEmbedded = embeddedVideoTag != nullptr;
		return isEmbedded ? embeddedBuffers : streamingBuffers;
	}

	BlockingCircularQueue<YUVBuffer>& getBuffers()
	{
		bool isEmbedded = embeddedVideoTag != nullptr;
		return isEmbedded ? embeddedBuffers : streamingBuffers;
	}

	void copyFrameToBuffers(const AVFrame* frameIn, const TimeSpec& time);
	void setSize(const Vector2u& size);
	bool fillDataAndCheckValidity();
public:
	FFMpegVideoDecoder
	(
		const LS_VIDEO_CODEC& codec,
		Span<const uint8_t> initData,
		number_t frameRateHint,
		DefineVideoStreamTag* tag = nullptr
	);

	/*
	   Specialized constructor used by FFMpegStreamDecoder
	*/
	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
	FFMpegVideoDecoder(AVCodecParameters* codecPar, number_t frameRateHint);
	#else
	FFMpegVideoDecoder(AVCodecContext* codecContext, number_t frameRateHint);
	#endif
	~FFMpegVideoDecoder();
	/*
	   Specialized decoding used by FFMpegStreamDecoder
	*/
	bool decodePacket(AVPacket* pkt, const TimeSpec& time);
	virtual void switchCodec
	(
		const LS_VIDEO_CODEC& codecId,
		Span<const uint8_t> initData,
		number_t frameRateHint
	) = 0;

	virtual bool decodeData
	(
		Span<const uint8_t> data,
		Optional<const TimeSpec&> time
	) = 0;

	bool discardFrame() override;
	size_t skipUntil(const TimeSpec& time) override;
	void skipAll() override;
	void setFlushing() override
	{
		flushing = true;
		if (getBuffers.isEmpty())
		{
			status = FLUSHED;
			flushed.signal();
		}
	}

	//ITextureUploadable interface
	uint8_t* upload(bool refresh) override;
};
#endif

class AudioDecoder : public Decoder
{
public:
	using F32SamplePair = std::pair<float, float>;
	using S16SamplePair = std::pair<int16_t, int16_t>;
private:
	void skipUntilF32(const TimeSpec& time);
	void skipUntilS16(const TimeSpec& time);
protected:
	template<typename T>
	struct FrameSamples
	{
		T samples[MAX_AUDIO_FRAME_SIZE / 2];
		__attribute__((aligned(8 * sizeof(T)))) T* current;
		size_t size;
		TimeSpec time;

		FrameSamples() : current(samples), size(0) {}
		void cleanup() {}
		void init()
		{
			size = 0;
			time = TimeSpec();
		}
	};

	#ifdef HAVE_LIBSWRESAMPLE
	SwrContext* resampleContext;
	#elif defined HAVE_LIBAVRESAMPLE
	AVAudioResampleContext* resampleContext;
	#endif
	BlockingCircularQueue<FrameSamples<int16_t>> samplesBufferS16;
	BlockingCircularQueue<FrameSamples<float>> samplesBufferF32;

	virtual void samplesConsumed(size_t samples) {}
	bool discardFrameS16();
	bool discardFrameF32();
	void signalFlushed()
	{
		if (!flushed || hasDecodedFrames())
			return;
		status = FLUSHED;
		flushed.signal();
	}
public:
	EngineData* engineData;
	size_t sampleRate;

	/**
	  	The AudioDecoder contains audio buffers that must be aligned to 16 bytes, so we redefine the allocator
	*/
	AudioDecoder(size_t size, EngineData* _engineData);
	virtual ~AudioDecoder();
	virtual void switchCodec
	(
		const LS_VIDEO_CODEC& codecId,
		Span<const uint8_t> initData
	) = 0;

	virtual size_t decodeData
	(
		Span<const uint8_t> data,
		Optional<const TimeSpec&> time
	) = 0;

	F32SamplePair getNextSampleF32() = 0;
	S16SamplePair getNextSampleS16() = 0;
	size_t getSamples(Span<F32SamplePair> span) = 0;
	size_t getSamples(Span<S16SamplePair> span) = 0;

	bool hasDecodedFrames() const
	{
		return
		(
			!samplesBufferS16.isEmpty() ||
			!samplesBufferF32.isEmpty()
		);
	}

	uint32_t getFrontTime() const;
	size_t getBytesPerMSec() const
	{
		return sampleRate * channelCount * 2 / 1000;
	}

	size_t copyFrameS16(Span<int16_t> data) DLL_PUBLIC;
	size_t copyFrameF32(Span<float> data) DLL_PUBLIC;
	/**
	  	Skip samples until the given time

		@param time the desired time in `TimeSpec` form
	*/
	void skipUntil(const TimeSpec& time);
	/**
	  	Skip all the samples
	*/
	void skipAll() DLL_PUBLIC;
	bool discardFrame();
	void setFlushing() override
	{
		flushing = true;
		signalFlushed();
	}

	uint8_t channelCount;
	//Saves the timestamp of the first decoded frame
	TimeSpec initialTime;
	// if true, decoder is only used for extracting parts meaning audio data will always be provided as 32bit floats for 2 channels
	bool forExtraction;
};

class NullAudioDecoder: public AudioDecoder
{
public:
	NullAudioDecoder() :
	AudioDecoder(0, nullptr),
	status(VALID),
	sampleRate(44100),
	channelCount(2) {}

	void switchCodec
	(
		const LS_VIDEO_CODEC& codecId,
		Span<const uint8_t> initData
	) override {}

	size_t decodeData
	(
		Span<const uint8_t> data,
		Optional<const TimeSpec&> time
	) override { return 0; }
};

// this is the AudioDecoder for streaming Sounds by SampleDataEvent
class SampleDataAudioDecoder : public AudioDecoder
{
private:
	ACQUIRE_RELEASE_VARIABLE(size_t, bufferedSamples);
	SoundChannel* soundChannel;
protected:
	void samplesConsumed(size_t samples) override;
public:
	SampleDataAudioDecoder
	(
		SoundChannel* _soundChannel,
		size_t bufferTime,
		EngineData* engineData
	) :
	AudioDecoder(bufferTime, engineData),
	sampleRate(44100),
	channelCount(2),
	bufferedSamples(0),
	soundChannel(_soundChannel) {}

	void switchCodec
	(
		const LS_VIDEO_CODEC& codecId,
		Span<const uint8_t> initData
	) override {}

	// the data is always expected to be floats
	size_t decodeData
	(
		Span<const uint8_t> data,
		Optional<const TimeSpec&> time
	) override;

	F32SamplePair getNextSampleF32() override;
	S16SamplePair getNextSampleS16() override;
	size_t getSamples(Span<F32SamplePair> span) override;
	size_t getSamples(Span<S16SamplePair> span) override;

	size_t getBufferedSamples() const
	{
		return ACQUIRE_READ(bufferedSamples);
	}
};


#ifdef ENABLE_LIBAVCODEC
class FFMpegAudioDecoder : public AudioDecoder
{
private:
	bool ownedContext;
	AVCodecContext* codecContext;
	std::vector<uint8_t> overflowBuffer;
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	AVFrame* frameIn;
	size_t resampleFrame(uint8_t** output);
	#endif
	bool fillDataAndCheckValidity();
	CodecID toFFMpegCodec(const LS_AUDIO_CODEC& codec);
public:
	FFMpegAudioDecoder
	(
		EngineData* _engineData,
		const LS_AUDIO_CODEC& codec,
		Span<const uint8_t> initData,
		size_t buffertime
	);

	FFMpegAudioDecoder
	(
		EngineData* _engineData,
		const LS_AUDIO_CODEC& codec,
		size_t sampleRate,
		uint8_t channels
		size_t buffertime,
		bool
	);

	/*
	   Specialized constructor used by FFMpegStreamDecoder
	*/
	FFMpegAudioDecoder(EngineData* eng,AVCodecParameters* codecPar, uint32_t buffertime);
	(
		EngineData* _engineData,
		#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		AVCodecParameters* codecPar,
		#else
		AVCodecContext* codecContext,
		#endif
		size_t buffertime
	);

	~FFMpegAudioDecoder();
	/*
	   Specialized decoding used by FFMpegStreamDecoder
	*/
	int decodePacket(AVPacket* pkt, const TimeSpec& time);
	void switchCodec
	(
		const LS_VIDEO_CODEC& codecId,
		Span<const uint8_t> initData
	) override;

	size_t decodeData
	(
		Span<const uint8_t> data,
		Optional<const TimeSpec&> time
	) override;

	F32SamplePair getNextSampleF32() override;
	S16SamplePair getNextSampleS16() override;
	size_t getSamples(Span<F32SamplePair> span) override;
	size_t getSamples(Span<S16SamplePair> span) override;
};
#endif

class StreamDecoder
{
protected:
	bool valid { false };
	bool _hasVideo { false };
	bool atEnd { false };
public:
	AudioDecoder* audioDecoder { nullptr };
	VideoDecoder* videoDecoder { nullptr };

	StreamDecoder() = default;
	virtual ~StreamDecoder();
	virtual bool decodeNextFrame() = 0;
	virtual void jumpToPosition(const TimeSpec& pos) = 0;
	virtual void jumpToFrame(size_t frame) = 0;
	bool isValid() const { return valid; }
	bool hasVideo() const { return _hasVideo; }
	bool isAtEnd() const  { return atEnd; }
};

#ifdef ENABLE_LIBAVCODEC
class FFMpegStreamDecoder: public StreamDecoder
{
private:
	NetStream* netStream;
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
	Span<uint8_t> avioBuffer;
	uint8_t* avioBuffer;
	static int avioReadPacket(void* data, Span<uint8_t> buf);
	static size_t avioSeek(void *data, ssize_t offset, size_t type);
	//NOTE: this will become AVIOContext in FFMpeg 0.7
	#if LIBAVUTIL_VERSION_MAJOR < 51
	ByteIOContext* avioContext;
	#else
	AVIOContext* avioContext;
	#endif
	size_t availableStreamSize;
	size_t fullStreamSize;
public:
	FFMpegStreamDecoder
	(
		NetStream* _netStream,
		EngineData* engineData,
		std::istream& _stream,
		size_t bufferTime,
		AudioFormat* format = nullptr,
		size_t streamSize = -1,
		bool forExtraction = false
	);

	~FFMpegStreamDecoder();
	bool decodeNextFrame() override;
	void jumpToPosition(const TimeSpec& pos) override;
	void jumpToFrame(size_t frame) override;
	size_t getAudioSampleRate();
};
#endif

}
#endif /* BACKENDS_DECODER_H */
