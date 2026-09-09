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

#include <cassert>

#include "backends/decoder.h"
#include "backends/rendering.h"
#include "compat.h"

#include "parsing/tags.h"
#include "platforms/fastpaths.h"
#include "platforms/engineutils.h"
#include "scripting/class.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/media/flashmedia.h"
#include "swf.h"

#ifdef ENABLE_LIBAVCODEC
#if LIBAVUTIL_VERSION_MAJOR < 51
#define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
#define AVMEDIA_TYPE_AUDIO CODEC_TYPE_AUDIO
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 45, 101)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_unref avcodec_get_frame_defaults
#endif
#endif

using namespace lightspark;

bool VideoDecoder::setSize(const Vector2u& size)
{
	if (size == frameSize)
		return false;

	frameSize = size;
	LOG
	(
		LOG_INFO,
		"VIDEO DEC: "
		"Video frame size " << frameSize.x << 'x' << frameSize.y
	);

	resizeGLBuffers = true;

	if (decodedFrameBuffer != nullptr)
		aligned_free(decodedFrameBuffer);

	decodedFrameBuffer = aligned_malloc
	(
		frameSize.x * frameSize.y * 4,
		16
	);

	if (decodedFrameBuffer != nullptr)
		return true;

	LOG
	(
		LOG_ERROR,
		"`aligned_malloc` couldn't allocate enough memory."
	);
	return true;
}

bool VideoDecoder::resizeIfNeeded(TextureChunk& tex)
{
	if (!resizeGLBuffers)
		return false;
	//Chunks are at least aligned to 128, we need 16
	assert_and_throw(Vector2u(tex.width, tex.height) == frameSize);
	resizeGLBuffers = false;
	return true;
}

void VideoDecoder::sizeNeeded(uint32_t& w, uint32_t& h) const
{
	//Return the actual width aligned to 16, the SSE2 packer is advantaged by this
	//and it comes for free as the texture tiles are aligned to 128
	w = (frameSize.x + 15) & 0xfffffff0;
	h = frameSize.y;
}

TextureChunk& VideoDecoder::getTexture()
{
	if (videoTexture.isValid())
		return videoTexture;

	videoTexture = getSys()->getRenderThread->allocateTexture
	(
		frameSize.x,
		frameSize.y,
		true
	);
	return videoTexture;
}

void VideoDecoder::uploadFence()
{
	ITextureUploadable::uploadFence();
	assert(fenceCount--);
	if (markedForDeletion && !fenceCount)
		delete this;
}

void VideoDecoder::markForDestruction()
{
	markedForDeletion = true;
}

void VideoDecoder::clearFrameBuffer()
{
	if (decodedFrameBuffer == nullptr)
		return;
	memset(decodedFrameBuffer, 0, frameSize.x * frameSize.y * 4);
}

VideoDecoder::VideoDecoder() :
resizeGLBuffers(false),
markedForDeletion(false),
decodedFrameBuffer(nullptr),
lastFrame(-1),
currentFrame(-1),
frameRate(0),
framesDecoded(0),
framesDropped(0),
fenceCount(0),
{
}

VideoDecoder::~VideoDecoder()
{
	auto rt = getSys()->getRenderThread();
	if (videoTexture.isValid() && rt != nullptr && rt->isStarted())
		rt->releaseTexture(getTexture());

	if (decodedFrameBuffer != nullptr)
		aligned_free(decodedFrameBuffer);
}

void VideoDecoder::waitForFencing()
{
	fenceCount++;
}

#ifdef ENABLE_LIBAVCODEC
bool FFMpegVideoDecoder::fillDataAndCheckValidity()
{
	if (!frameRate && codecContext->time_base.num)
	{
		frameRate =
		(
			codecContext->time_base.den /
			codecContext->time_base.num
		);
		if(videoCodec == H264) //H264 has half ticks (usually?)
			frameRate /= 2;
	}
	else if (!frameRate)
		return false;

	Vector2u size(codecContext->width, codecContext->height);
	if (size == Vector2u())
		return false;

	setSize(size);
	return true;
}

FFMpegVideoDecoder::FFMpegVideoDecoder
(
	const LS_VIDEO_CODEC& codec,
	Span<const uint8_t> initData,
	number_t frameRateHint,
	DefineVideoStreamTag* tag
) :
ownedContext(true),
curBuffer(0),
codecContext(nullptr),
streamingBuffers(_bufferSize),
embeddedBuffers(2),
curBufferOffset(0),
embeddedVideoTag(tag)
{
	//The tag is the header, initialize decoding
	switchCodec(codecId, initData, frameRateHint);
	frameIn = av_frame_alloc();
	if (tag == nullptr)
		return;
	// immediately decode 1 frame to obtain size:
	// 'define stream' tag information not always correct + cannot resize during an 'upload' call
	auto frameTag = tag->getFrame(0);
	Span<const uint8_t> span
	{
		frameTag->getData(),
		frameTag->getNumBytes()
	};

	decodeData(span, {});
}

void FFMpegVideoDecoder::switchCodec
(
	const LS_VIDEO_CODEC& codecId,
	Span<const uint8_t> initData,
	number_t frameRateHint
)
{
	if (codecContext != nullptr)
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 63, 100)
		avcodec_free_context(&codecContext);
	#else
	{
		avcodec_close(codecContext);
		if(ownedContext)
			av_free(codecContext);
	}
	#endif

	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 8, 0)
	codecContext = avcodec_alloc_context3(nullptr);
	#else
	codecContext = avcodec_alloc_context();
	#endif

	CodecID avCodecId = CODEC_ID_NONE;
	videoCodec = codecId;

	switch (codecId)
	{
		case H264: avCodecId = CODEC_ID_H264; break;
		case H263: avCodecId = CODEC_ID_FLV1; break;
		case VP6: avCodecId = CODEC_ID_VP6F; break;
		case VP6A: avCodecId = CODEC_ID_VP6A; break;
		case GIF: avCodecId = CODEC_ID_GIF; break;
	}

	//TODO: serialize access to avcodec_open
	auto codec = avcodec_find_decoder(avCodecId);
	assert(codec != nullptr);

	// Ignore the `frameRateHint` for H264, since the frame rate is
	// obtained from the video data.
	if (codecId != H264)
	{
		//Exploit the frame rate information
		assert(frameRateHint);
		frameRate = frameRateHint;
	}

	if (!initData.empty())
	{
		codecContext->extradata = initData.getData();
		codecContext->extradata_size = initData.getSize();
	}

	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 8, 0)
	if (avcodec_open2(codecContext, codec, nullptr) < 0)
	#else
	if (avcodec_open(codecContext, codec) < 0)
	#endif
		throw RunTimeException("Cannot open decoder");

	status = fillDataAndCheckValidity() ? VALID : INIT;
}

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
FFMpegVideoDecoder::FFMpegVideoDecoder
(
	AVCodecParameters* codecPar,
	number_t frameRateHint
) :
status(INIT),
ownedContext(true),
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 8, 0)
codecContext(avcodec_alloc_context3(nullptr)),
#else
codecContext(avcodec_alloc_context()),
#endif
#else
FFMpegVideoDecoder::FFMpegVideoDecoder
(
	AVCodecContext* _c,
	number_t frameRateHint
) :
status(INIT),
ownedContext(false),
codecContext(_c),
#endif
curBuffer(0),
curBufferOffset(0),
embeddedVideoTag(nullptr),
streamingBuffers(_bufferSize),
embeddedBuffers(2),
frameIn(av_frame_alloc())
{
	//The tag is the header, initialize decoding
	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
	switch (codecPar->codec_id)
	#else
	switch (codecContext->codec_id)
	#endif
	{
		case CODEC_ID_H264: videoCodec = H264; break;
		case CODEC_ID_FLV1: videoCodec = H263; break;
		case CODEC_ID_VP6F: videoCodec = VP6; break;
		case CODEC_ID_GIF: videoCodec = GIF; break;
		default: return;
	}

	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
	avcodec_parameters_to_context(codecContext, codecPar);
	auto codec = avcodec_find_decoder(codecPar->codec_id);
	#else
	auto codec = avcodec_find_decoder(codecContext->codec_id);
	#endif

	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 8, 0)
	if (avcodec_open2(codecContext, codec, nullptr) < 0)
	#else
	if (avcodec_open(codecContext, codec) < 0)
	#endif
		return;

	frameRate = frameRateHint;
	if (fillDataAndCheckValidity())
		status = VALID;
}

FFMpegVideoDecoder::~FFMpegVideoDecoder()
{
	while (fenceCount);
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 63, 100)
	avcodec_free_context(&codecContext);
	#else
	avcodec_close(codecContext);
	if (ownedContext)
		av_free(codecContext);
	#endif

	#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 0, 0)
	av_frame_free(&frameIn);
	#else
	av_free(frameIn);
	#endif
}

//setSize is called from the routine that inserts new frames
void FFMpegVideoDecoder::setSize(const Vector2u& size)
{
	if (!VideoDecoder::setSize(size))
		return;

	//Discard all the frames
	while (discardFrame());

	//As the size changed, reset the buffer
	getBuffers().regen(YUVBufferGenerator
	(
		frameSize.x * frameSize.y/* * 4*/,
		codecContext->pix_fmt == AV_PIX_FMT_YUVA420P,
		codecContext->pix_fmt != AV_PIX_FMT_BGRA
	));
}

size_t FFMpegVideoDecoder::skipUntil(const TimeSpec& time)
{
	auto& buffers = getBuffers();
	size_t i = 0;
	for (; !buffers.isEmpty() && buffers.front().time < time; ++i)
		discardFrame();
	return i;
}
void FFMpegVideoDecoder::skipAll()
{
	while (!streamingBuffers.isEmpty())
		discardFrame();
	while (!embeddedBuffers.isEmpty())
		discardFrame();
}

bool FFMpegVideoDecoder::discardFrame()
{
	// We don't want to block if there aren't any frames.
	auto& buffers = getBuffers();
	bool ret = buffers.nonBlockingPopFront();
	if (flushing && buffers.isEmpty()) //End of our work
	{
		status = FLUSHED;
		flushed.signal();
	}

	framesDropped++;
	return ret;
}

bool FFMpegVideoDecoder::decodeData
(
	Span<const uint8_t> data,
	Optional<const TimeSpec&> time
)
{
	if (data.empty())
		return false;

	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	AVPacket* pkt = av_packet_alloc();
	if (!pkt)
		return 0;
	pkt->data=data;
	pkt->size=datalen;
	auto ret = avcodec_send_packet(codecContext, pkt);
	while (!ret)
	{
		ret = avcodec_receive_frame(codecContext,frameIn);
		if (ret && ret != AVERROR(EAGAIN))
		{
			LOG(LOG_INFO,"not decoded:"<<ret);
			#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 12, 100)
			av_packet_unref(pkt);
			#else
			av_free_packet(pkt);
			#endif
			return false;
		}
		else if (ret)
			break;

		if (status == INIT && fillDataAndCheckValidity())
			status=VALID;

		assert
		(
			frameIn->pts == int64_t(AV_NOPTS_VALUE) ||
			!frameIn->pts
		);
		if (time.hasValue())
			copyFrameToBuffers(frameIn, *time);
	}
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 12, 100)
	av_packet_unref(pkt);
	#else
	av_free_packet(pkt);
	#endif
	av_packet_free(&pkt);
	#else
	int frameOk = 0;
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 23, 0)
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = data.getData();
	pkt.size = data.getSize();
	auto ret = avcodec_decode_video2(codecContext, frameIn, &frameOk, &pkt);
	#else
	auto ret = avcodec_decode_video(codecContext, frameIn, &frameOk, data, datalen);
	#endif
	if (ret < 0)
	{
		LOG(LOG_INFO,"not decoded:"<< ret << ' ' << frameOk);
		return false;
	}

	if(!frameOk)
		return true;

	//assert(codecContext->pix_fmt==PIX_FMT_YUV420P);
	if (status == INIT && fillDataAndCheckValidity())
		status=VALID;

	assert(frameIn->pts == int64_t(AV_NOPTS_VALUE) || !frameIn->pts);
	if (time.hasValue())
		copyFrameToBuffers(frameIn, *time);
	#endif
	return true;
}

bool FFMpegVideoDecoder::decodePacket(AVPacket* pkt, const TimeSpec& time)
{
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 106, 102)
	auto ret = avcodec_send_packet(codecContext, pkt);
	auto getDict = [&](AVDictionaryEntry* prev = nullptr)
	{
		auto meta = frameIn->meta;
		if (meta == nullptr)
			return nullptr;
		return av_dict_get(meta, "", prev, AV_DICT_IGNORE_SUFFIX);
	};

	while (!ret)
	{
		ret = avcodec_receive_frame(codecContext, frameIn);
		if (ret && ret != AVERROR(EAGAIN))
		{
			LOG(LOG_INFO, "not decoded:" << ret);
			return false;
		}
		else if (ret)
			break;

		if (status == INIT && fillDataAndCheckValidity())
			status = VALID;

		for (auto it = getDict(); it != nullptr; it = getDict(it))
		{
			LOG
			(
				LOG_NOT_IMPLEMENTED,
				"sending metadata from stream:" <<
				entry->key << ' ' << entry->value
			);
		}
		copyFrameToBuffers(frameIn, time);
	}
	#else
	int frameOk = 0;

	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 23, 0)
	auto ret = avcodec_decode_video2
	(
		codecContext,
		frameIn,
		&frameOk,
		pkt
	);
	#else
	auto ret = avcodec_decode_video
	(
		codecContext,
		frameIn,
		&frameOk,
		pkt->data,
		pkt->size
	);
	#endif
	if (ret < 0)
	{
		LOG(LOG_INFO, "not decoded:" << ret << ' ' << frameOk);
		return false;
	}

	assert_and_throw(ret == pkt->size);
	if (!frameOk)
		return true;

	//assert(codecContext->pix_fmt==PIX_FMT_YUV420P);

	if(status==INIT && fillDataAndCheckValidity())
		status=VALID;

	assert(frameIn->pts == int64_t(AV_NOPTS_VALUE) || !frameIn->pts);
	copyFrameToBuffers(frameIn, time);
	#endif
	return true;
}

void FFMpegVideoDecoder::copyFrameToBuffers
(
	const AVFrame* frameIn,
	const TimeSpec& time
)
{
	auto curTail = getBuffers.acquireLast();
	//Only one thread may access the tail
	size_t offset[3] = {0, 0, 0};
	auto copyChannel = [&]
	(
		size_t y,
		size_t chIdx,
		size_t i,
		size_t size
	)
	{
		memcpy
		(
			curTail.ch[chIdx] + offset[i],
			frameIn->data[chIdx] + (y * frameIn->linesize[i]),
			size
		);
	};

	if (codecContext->pix_fmt != AV_PIX_FMT_BGRA)
	{
		for (size_t y = 0; y < frameSize.y; ++y)
		{
			copyChannel(y, 0, 0, frameSize.x);
			if (codecContext->pix_fmt == AV_PIX_FMT_YUVA420P)
				copyChannel(y, 3, 0, frameSize.x);
			offset[0] += frameSize.x;
		}

		for (size_t y = 0; y < frameSize.y / 2; ++y)
		{
			copyChannel(y, 1, 1, frameSize.x / 2);
			copyChannel(y, 2, 2, frameSize.x / 2);
			offset[1] += frameSize.x/2;
			offset[2] += frameSize.x/2;
		}
		goto end;
	}

	// ffmpeg seems to decode GIFs in AV_PIX_FMT_BGRA format and puts all data in first channel
	auto chSpan = Span<uint8_t>
	(
		curTail.ch[0],
		frameSize.x * frameSize.y * 4
	).as<uint32_t>();

	auto dataSpan = Span<uint8_t>
	(
		frameIn->data[0],
		frameIn->linesize[0] * frameSize.y * 4
	).as<RGBA>();

	for (size_t y = 0; y < frameSize.y; ++y)
	{
		for (size_t x = 0; x < frameSize.x; ++x)
		{
			// convert BGRA to RGBA
			chSpan[frameSize.x * y + x] = dataSpan
			[
				frameIn->linesize[0] *
				y +
				x
			].toRGBA();
		}
	}
end:
	curTail.time = time;
	getBuffers().commitLast();
}

uint8_t* FFMpegVideoDecoder::upload(bool refresh)
{
	assert_and_throw(decodedFrameBuffer != nullptr);
	if (!refresh)
		return decodedFrameBuffer;

	if (embeddedVideoTag == nullptr && streamingBuffers.isEmpty())
		return decodedFrameBuffer;

	// on embedded video we decode the frames during upload
	if (currentFrame < lastFrame)
	{
		currentFrame = 0;
		lastFrame = -1;
	}

	skipAll();
	for (uint32_t i = lastFrame + 1; i <= currentframe; i++)
	{
		auto frameTag = embeddedVideoTag->getFrame(i);
		if (frameTag == nullptr)
			return decodedFrameBuffer;
		Span<const uint8_t> span
		(
			frameTag->getData(),
			frameTag->getNumBytes()
		);

		decodeData(span, makeOptional(TimeSpec()).filter
		(
			i == currentFrame
		).asRef());
		lastFrame = i;
	}

	if (embeddedBuffers.isEmpty())
		return decodedFrameBuffer;

	//At least a frame is available
	auto cur = getBuffers().front();
	if (codecContext->pix_fmt == AV_PIX_FMT_BGRA)
	{
		memcpy
		(
			decodedFrameBuffer,
			cur->ch[0],
			frameSize.x * frameSize.y * 4
		);
		return decodedFrameBuffer;
	}

	fastYUV420ChannelsToYUV0Buffer
	(
		cur->ch[0],
		cur->ch[1],
		cur->ch[2],
		decodedFrameBuffer,
		frameSize.x,
		frameSize.y
	);

	if (codecContext->pix_fmt != AV_PIX_FMT_YUVA420P)
		return decodedFrameBuffer;

	auto texWidth = (frameSize.x + 15) & 0xfffffff0;
	for (size_t y = 0; y < frameSize.y; ++y)
	{
		for (size_t x = 0; x < frameSize.x; ++x)
		{
			size_t px = y * texWidth + x;
			decodedFrameBuffer[px * 4 + 3] = cur->ch[3]
			[
				y *
				frameSize.x +
				x
			];
		}
	}
	return decodedFrameBuffer;
}

void FFMpegVideoDecoder::YUVBufferGenerator::init(YUVBuffer& buf) const
{
	if (buf.ch[0] == nullptr)
		goto allocChannels;

	aligned_free(buf.ch[0]);
	if (hasChannels)
	{
		aligned_free(buf.ch[1]);
		aligned_free(buf.ch[2]);
		if (buf.ch[3] != nullptr)
			aligned_free(buf.ch[3]);
	}

allocChannels:
	if (!hasChannels)
	{
		buf.ch[0] = aligned_malloc(16, bufferSize * 4);
		return;
	}

	buf.ch[0] = aligned_malloc(16, bufferSize);
	buf.ch[1] = aligned_malloc(16, bufferSize / 4);
	buf.ch[2] = aligned_malloc(16, bufferSize / 4);
	if (hasAlpha)
		buf.ch[3] = aligned_malloc(16, bufferSize);
}
#endif //ENABLE_LIBAVCODEC

bool AudioDecoder::discardFrame()
{
	return
	(
		engineData->audio_useFloatSampleFormat() ?
		discardFrameF32() :
		discardFrameS16()
	);
}

bool AudioDecoder::discardFrameS16()
{
	//We don't want to block if no frame is available
	bool ret = samplesBufferS16.nonBlockingPopFront();
	if (!ret)
	{
		LOG
		(
			LOG_ERROR,
			"discardFrame blocking " <<
			flushing << ' ' << samplesBufferS16.isEmpty()
		);
	}

	if (flushing && samplesBufferS16.isEmpty()) //End of our work
	{
		status = FLUSHED;
		flushed.signal();
	}
	return ret;
}

bool AudioDecoder::discardFrameF32()
{
	//We don't want to block if no frame is available
	bool ret = samplesBufferF32.nonBlockingPopFront();
	if (!ret)
	{
		LOG
		(
			LOG_ERROR,
			"discardFrame blocking " <<
			flushing << ' ' << samplesBufferF32.isEmpty()
		);
	}

	if (flushing && samplesBufferF32.isEmpty()) //End of our work
	{
		status = FLUSHED;
		flushed.signal();
	}
	return ret;
}

size_t AudioDecoder::copyFrameS16(Span<int16_t> data)
{
	assert(!data.empty());

	if (samplesBufferS16.isEmpty())
	{
		signalFlushed();
		return 0;
	}

	auto frameSize = std::min<size_t>
	(
		samplesBufferS16.front().len,
		data.getSize()
	);

	memcpy(data.getData(), samplesBufferS16.front().current, frameSize);
	samplesBufferS16.front().len -= frameSize;
	assert(!(samplesBufferS16.front().len & 0x80000000));
	if(!samplesBufferS16.front().len)
	{
		samplesBufferS16.nonBlockingPopFront();
		signalFlushed();
	}
	else
	{
		samplesBufferS16.front().current += frameSize / 2;
		samplesBufferS16.front().time += TimeSpec::fromMs
		(
			frameSize /
			getBytesPerMSec()
		);
	}

	samplesConsumed(frameSize / 2);
	return frameSize;
}

size_t AudioDecoder::copyFrameF32(Span<float> data)
{
	assert(!data.empty());

	if(samplesBufferF32.isEmpty())
	{
		signalFlushed();
		return 0;
	}

	auto frameSize = std::min<size_t>
	(
		samplesBufferF32.front().len,
		data.getSize()
	);

	memcpy(data.getData(), samplesBufferF32.front().current, frameSize);
	samplesBufferF32.front().len -= frameSize;
	assert(!(samplesBufferF32.front().len & 0x80000000));
	if (!samplesBufferF32.front().len)
	{
		samplesBufferF32.nonBlockingPopFront();
		signalFlushed();
	}
	else
	{
		samplesBufferF32.front().current+=frameSize/4;
		samplesBufferF32.front().time += TimeSpec::fromMs
		(
			frameSize /
			getBytesPerMSec()
		);
	}

	samplesConsumed(frameSize / 4);
	return frameSize;
}

AudioDecoder::AudioDecoder(size_t size, EngineData* _engineData) :
#if defined HAVE_LIBAVRESAMPLE || defined HAVE_LIBSWRESAMPLE
resampleContext(nullptr),
#endif
sampleRate(0),
engineData(_engineData),
samplesBufferS16(_engineData->audio_useFloatSampleFormat() ? 0 : size),
samplesBufferF32(_engineData->audio_useFloatSampleFormat() ? size : 0),
channelCount(0),
initialTime(-1, 0),
forExtraction(false)
{
}

AudioDecoder::~AudioDecoder()
{
	#ifdef HAVE_LIBSWRESAMPLE
	if (resampleContext != nullptr)
		swr_free(&resamplecontext);
	resampleContext = nullptr;
	#elif defined(HAVE_LIBAVRESAMPLE)
	if (resampleContext != nullptr)
		avresample_free(&resampleContext);
	resampleContext = nullptr;
	#endif
}

const TimeSpec& AudioDecoder::getFrontTime() const
{
	assert(hasDecodedFrames());
	return
	(
		engine->audio_useFloatSampleFormat() ?
		samplesBufferF32.front().time :
		samplesBufferS16.front().time
	);
}

void AudioDecoder::skipUntilF32(const TimeSpec& time)
{
	if (time == TimeSpec()) // Nothing to skip
		return;
	for (; samplesBufferF32.isEmpty();)
	{
		auto& cur = samplesBufferF32.front();
		assert(time == cur.time);
		//Check how many bytes are needed to fill the gap
		auto samples =
		(
			time.absDiff(cur.time) *
			sampleRate
		).getSecs();

		auto bytesToDiscard = (samples * channelCount * 4) & ~1;
		if (cur.len <= bytesToDiscard) //The whole frame is droppable
		{
			discardFrameF32();
			continue;
		}

		assert(!(bytesToDiscard % 2));
		cur.len -= bytesToDiscard;
		assert(!(cur.len & 0x80000000));
		cur.current += bytesToDiscard / 2;
		cur.time = time;
		break;
	}
}

void AudioDecoder::skipUntilS16(const TimeSpec& time)
{
	if (time == TimeSpec()) // Nothing to skip
		return;
	for (; samplesBufferS16.isEmpty();)
	{
		auto& cur = samplesBufferS16.front();
		assert(time == cur.time);
		//Check how many bytes are needed to fill the gap
		auto samples =
		(
			time.absDiff(cur.time) *
			sampleRate
		).getSecs();

		auto bytesToDiscard = (samples * channelCount * 2) & ~1;
		if (cur.len <= bytesToDiscard) //The whole frame is droppable
		{
			discardFrameS16();
			continue;
		}

		assert(!(bytesToDiscard % 2));
		cur.len -= bytesToDiscard;
		assert(!(cur.len & 0x80000000));
		cur.current += bytesToDiscard / 2;
		cur.time = time;
		break;
	}
}

void AudioDecoder::skipUntil(const TimeSpec& time)
{
	assert(isValid());
	if (engine->audio_useFloatSampleFormat())
		skipUntilF32(time);
	else
		skipUntilS16(time);
}

void AudioDecoder::skipAll()
{
	while (!samplesBufferS16.isEmpty())
		discardFrameS16();
	while (!samplesBufferF32.isEmpty())
		discardFrameF32();
}

#ifdef ENABLE_LIBAVCODEC
FFMpegAudioDecoder::FFMpegAudioDecoder
(
	EngineData* _engineData,
	const LS_AUDIO_CODEC& codec,
	Span<const uint8_t> initData,
	size_t bufferTime
) :
AudioDecoder(bufferTime + 1, _engineData),
ownedContext(true),
codecContext(nullptr)
{
	switchCodec(audioCodec, initData);
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 106, 102)
	frameIn = av_frame_alloc();
	#endif
}

void FFMpegAudioDecoder::switchCodec
(
	const LS_VIDEO_CODEC& codecId,
	Span<const uint8_t> initData
)
{
	if (codecContext != nullptr)
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 63, 100)
		avcodec_free_context(&codecContext);
	#else
		avcodec_close(codecContext);
	#endif

	#ifdef HAVE_LIBSWRESAMPLE
	if (resampleContext != nullptr)
		swr_free(&resamplecontext);
	resampleContext = nullptr;
	#elif defined(HAVE_LIBAVRESAMPLE)
	if (resampleContext != nullptr)
		avresample_free(&resampleContext);
	resampleContext = nullptr;
	#endif
	auto codec = avcodec_find_decoder(LSToFFMpegCodec(audioCodec));
	assert(codec != nullptr);

	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 8, 0)
	codecContext = avcodec_alloc_context3(nullptr);
	#else
	codecContext = avcodec_alloc_context();
	#endif

	if (!initData.empty())
	{
		codecContext->extradata = initData.getData();
		codecContext->extradata_size = initData.getSize();
	}

	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 8, 0)
	if (avcodec_open2(codecContext, codec, nullptr) < 0)
	#else
	if (avcodec_open(codecContext, codec) < 0)
	#endif
		throw RunTimeException("Cannot open decoder");

	status = fillDataAndCheckValidity() ? VALID : INIT;
}

FFMpegAudioDecoder::FFMpegAudioDecoder(EngineData* eng, LS_AUDIO_CODEC lscodec, int sampleRate, int channels, uint32_t buffertime, bool):AudioDecoder(buffertime+1,eng),ownedContext(true)
(
	EngineData* _engineData,
	const LS_AUDIO_CODEC& codec,
	size_t sampleRate,
	uint8_t channels
	size_t bufferTime,
	bool
) : AudioDecoder(bufferTime + 1, _engineData), ownedContext(true)
{
	status = INIT;

	auto codecId = toFFMpegCodec(codec);
	auto _codec = avcodec_find_decoder(codecId);
	assert(_codec != nullptr);
	codecContext = avcodec_alloc_context3(codec);
	codecContext->codec_id = codecId;
	codecContext->sample_rate = sampleRate;
	#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
	codecContext->ch_layout.nb_channels = channels;
	#else
	codecContext->channels = channels;
	#endif
	switch (channels)
	{
		#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
		case 1:
			codecContext->ch_layout = AV_CHANNEL_LAYOUT_MONO;
			break;
		case 2:
			codecContext->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
			break;
		#else
		case 1:
			codecContext->channel_layout = AV_CH_LAYOUT_MONO;
			break;
		case 2:
			codecContext->channel_layout = AV_CH_LAYOUT_STEREO;
			break;
		default:
		#endif
		default:
			LOG
			(
				LOG_NOT_IMPLEMENTED,
				"FFMpegAudioDecoder: "
				"channel layout for " << channels << " channels"
			);
			break;
	}

	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 8, 0)
	if (avcodec_open2(codecContext, codec, nullptr) < 0)
	#else
	if (avcodec_open(codecContext, codec) < 0)
	#endif
		return;

	if (fillDataAndCheckValidity())
		status = VALID;
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 106, 102)
	frameIn = av_frame_alloc();
	#endif
}

FFMpegAudioDecoder::FFMpegAudioDecoder
(
	EngineData* _engineData,
	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
	AVCodecParameters* codecPar,
	#else
	AVCodecContext* _codecContext,
	#endif
	size_t bufferTime
) :
AudioDecoder(bufferTime + 1, _engineData),
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(57, 40, 101)
ownedContext(true),
codecContext(_codecContext)
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53, 8, 0)
codecContext(avcodec_alloc_context3(nullptr))
#else
codecContext(avcodec_alloc_context())
#endif
#else
ownedContext(false),
codecContext(_codecContext)
#endif
{
	status = INIT;
	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
	auto codec = avcodec_find_decoder(codecPar->codec_id);
	avcodec_parameters_to_context(codecContext ,codecPar);
	#else
	auto codec = avcodec_find_decoder(codecContext->codec_id);
	#endif
	assert(codec != nullptr);

	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	if (avcodec_open2(codecContext, codec, nullptr) < 0)
	#else
	if (avcodec_open(codecContext, codec) < 0)
	#endif
		return;

	if (fillDataAndCheckValidity())
		status = VALID;

	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 106, 102)
	frameIn = av_frame_alloc();
	#endif
}

FFMpegAudioDecoder::~FFMpegAudioDecoder()
{
	if (ownedContext)
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 63, 100)
		avcodec_free_context(&codecContext);
	#else
	{
		avcodec_close(codecContext);
		av_free(codecContext);
	}
	#endif
	#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 0, 0)
	av_frame_free(&frameIn);
	#elif LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 106, 102)
	av_free(frameIn);
	#endif
}

CodecID FFMpegAudioDecoder::toFFMpegCodec(const LS_AUDIO_CODEC& codec)
{
	switch (codec)
	{
		case AAC: return CODEC_ID_AAC;
		case MP3: return CODEC_ID_MP3;
		case ADPCM: return CODEC_ID_ADPCM_SWF;
		case NELLYMOSER: return AV_CODEC_ID_NELLYMOSER;
		case LINEAR_PCM_LE: return CODEC_ID_PCM_S16LE;
		#if __BYTE_ORDER == __BIG_ENDIAN
		case LINEAR_PCM_PLATFORM_ENDIAN:
			return CODEC_ID_PCM_S16BE;
		case LINEAR_PCM_FLOAT_PLATFORM_ENDIAN:
			return CODEC_ID_PCM_F32BE;
		#else
		case LINEAR_PCM_PLATFORM_ENDIAN:
			return CODEC_ID_PCM_S16LE;
		case LINEAR_PCM_FLOAT_PLATFORM_ENDIAN:
			return CODEC_ID_PCM_F32LE;
		#endif
		default: return CODEC_ID_NONE;
	}
}

bool FFMpegAudioDecoder::fillDataAndCheckValidity()
{
	if (!codecContext->sample_rate)
		return false;
	LOG
	(
		LOG_TRACE,
		"AUDIO DEC: "
		"Audio sample rate " << codecContext->sample_rate
	);
	sampleRate = codecContext->sample_rate;

	#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
	const auto& channels = codecContext->ch_layout.nb_channels;
	#else
	const auto& channels = codecContext->channels;
	#endif
	if (!channels)
		return false;

	LOG(LOG_TRACE, "AUDIO DEC: Audio channels " << channels);
	channelCount = channels;

	if (initialTime.getSecs() != -1 || !hasDecodedFrames())
		return false;

	initialTime = getFrontTime();
	LOG
	(
		LOG_TRACE,
		"AUDIO DEC: Initial timestamp " <<
		initialTime.getSecs() << "s " <<
		initialTime.getNsecs() << "ns"
	);
	return true;
}

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 106, 102)
template<typename T>
void FFMPegAudioDecoder::decodeDataImpl
(
	T& samples,
	Span<const uint8_t> data,
	const TimeSpec& time
)
{
	while (!data.empty())
	{
		auto& tail = samples.acquireLast();
		auto size = std::min(data.getSize(), sizeof(T::samples));
		assert(!(size % 2));

		memcpy(tail.samples, data.getData(), size);
		data = data.subSpan(size);
		tail.size = size;
		assert(!(tail.size & 0x80000000));
		tail.current = tail.samples;
		tail.time = time;
		samples.commitLast();
	}
}

size_t FFMpegAudioDecoder::decodeData
(
	Span<const uint8_t> data,
	const TimeSpec& time
)
{
	auto pkt = av_packet_alloc();
	if (pkt == nullptr)
		return 0;

	// If some data was left unprocessed on previous call,
	// concatenate.
	std::vector<uint8_t> combinedBuffer;
	if (overflowBuffer.empty())
	{
		pkt->data = data.getData();
		pkt->size = data.getSize();
	}
	else
	{
		combinedBuffer = overflowBuffer;
		if (!data.empty())
		{
			combinedBuffer.insert
			(
				combinedBuffer.end(),
				data.begin(),
				data.end()
			);
		}

		pkt->data = &combinedBuffer[0];
		pkt->size = combinedBuffer.size();
		overflowBuffer.clear();
	}

	av_frame_unref(frameIn);
	auto ret = avcodec_send_packet(codecContext, pkt);
	size_t maxSize = 0;

	while (!ret)
	{
		ret = avcodec_receive_frame(codecContext, frameIn);
		if (ret && ret != AVERROR(EAGAIN))
		{
			LOG(LOG_ERROR, "not decoded audio:" << ret);
			continue;
		}
		else if (ret)
			break;
		auto output = resampleFrame();
		#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(56, 0, 100)
		maxSize = av_frame_get_pkt_size(frameIn);
		#elif LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
		maxSize = pkt->size;
		#else
		maxSize = frameIn->pkt_size;
		#endif
		if (engineData->audio_useFloatSampleFormat())
			decodeDataImpl(samplesBufferF32, output, time);
		else
			decodeDataImpl(samplesBufferS16, output, time);
		if (!output.empty())
			av_freep(&output.getData());
		if (status == INIT && fillDataAndCheckValidity())
			status = VALID;
	}

	if (maxSize)
	{
		auto tmp = Span<const uint8_t>
		(
			pkt->data,
			pkt->size
		).subSpan(maxSize);
		if (!tmp.empty())
			overflowBuffer.assign(tmp.begin(), tmp.end());
	}

	av_packet_unref(pkt);
	av_packet_free(&pkt);
	return maxSize;
}

AudioDecoder::F32SamplePair FFMpegAudioDecoder::getNextSampleF32()
{
}

AudioDecoder::S16SamplePair FFMpegAudioDecoder::getNextSampleS16()
{
}

size_t FFMpegAudioDecoder::getSamples(Span<F32SamplePair> span)
{
}

size_t FFMpegAudioDecoder::getSamples(Span<S16SamplePair> span)
{
}

int FFMpegAudioDecoder::decodePacket(AVPacket* pkt, const TimeSpec& time)
{
	av_frame_unref(frameIn);
	auto ret = avcodec_send_packet(codecContext, pkt);
	while (!ret)
	{
		ret = avcodec_receive_frame(codecContext,frameIn);
		if (ret && ret != AVERROR(EAGAIN))
		{
			LOG(LOG_ERROR, "not decoded audio:" << ret);
			return ret;
		}
		else if (ret)
			return ret;

		auto output = resampleFrame();
		if (engineData->audio_useFloatSampleFormat())
			decodeDataImpl(samplesBufferF32, output, time);
		else
			decodeDataImpl(samplesBufferS16, output, time);

		if (output)
			av_freep(&output);
		if(status==INIT && fillDataAndCheckValidity())
			status=VALID;
	}

	return ret;
}

Span<const uint8_t> FFMpegAudioDecoder::resampleFrame()
{
	auto _sampleRate = engineData->audio_getSampleRate();
	#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
	auto chLayout = AV_CHANNEL_LAYOUT_STEREO;
	#else
	auto chLayout = AV_CH_LAYOUT_STEREO;
	#endif
	#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(56, 0, 100)
	auto framesamplerate = av_frame_get_sample_rate(frameIn);
	#else
	auto frameSampleRate = frameIn->sample_rate;
	#endif
	auto outSampleFmt =
	(
		forExtraction ||
		engine->audio_useFloatSampleFormat()
	) ? AV_SAMPLE_FMT_FLT : AV_SAMPLE_FMT_S16;

	auto outSampleFmtSize =
	(
		forExtraction ||
		engine->audio_useFloatSampleFormat()
	) ? sizeof(float) : sizeof(int16_t);

	#ifdef HAVE_LIBSWRESAMPLE
	if (resampleContext == nullptr)
	{
		resampleContext = swr_alloc();
		#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57,24,100)
		av_opt_set_chlayout(resamplecontext, "in_chlayout",  &frameIn->ch_layout, 0);
		av_opt_set_chlayout(resamplecontext, "out_chlayout", &chLayout,  0);
		#else
		av_opt_set_int(resamplecontext, "in_channel_layout",  frameIn->channel_layout, 0);
		av_opt_set_int(resamplecontext, "out_channel_layout", chLayout,  0);
		#endif
		av_opt_set_int(resamplecontext, "in_sample_rate", frameSampleRate, 0);
		av_opt_set_int(resamplecontext, "out_sample_rate", sampleRate, 0);
		av_opt_set_int(resamplecontext, "in_sample_fmt", frameIn->format, 0);
		av_opt_set_int(resamplecontext, "out_sample_fmt", outputsampleformat, 0);
		swr_init(resamplecontext);
	}

	const uint8_t* output = nullptr;
	auto outSamples = swr_get_out_samples(resampleContext, frameIn->nb_samples);
	auto _ret = av_samples_alloc(output, nullptr, 2, outSamples, outSampleFmt, 0);

	if (_ret < 0)
	{
		LOG(LOG_ERROR, "resampling failed, error code:" << _ret);
		return { nullptr, 0 };
	}

	auto outSize = swr_convert
	(
		resampleContext,
		output,
		outSamples,
		(const uint8_t**)frameIn->extended_data,
		frameIn->nb_samples
	) *
	(
		outSampleFmtSize *
		#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
		channel_layout.nb_channels
		#else
		av_get_channel_layout_nb_channels(channel_layout)
		#endif
	);

	if (outSize <= 0)
	{
		LOG(LOG_ERROR, "resampling failed");
		return { nullptr, 0 };
	}

	return { output, outSize };
	#elif defined HAVE_LIBAVRESAMPLE
	if (resampleContext == nullptr)
	{
		resamplContext = avresample_alloc_context();
		av_opt_set_int(resampleContext, "in_channel_layout", frameIn->channel_layout, 0);
		av_opt_set_int(resampleContext, "out_channel_layout", chLayout, 0);
		av_opt_set_int(resampleContext, "in_sample_rate", frameSampleRate, 0);
		av_opt_set_int(resampleContext, "out_sample_rate", _sampleRate, 0);
		av_opt_set_int(resampleContext, "in_sample_fmt", frameIn->format, 0);
		av_opt_set_int(resampleContext, "out_sample_fmt", outSampleFmt, 0);
		avresample_open(resampleContext);
	}

	auto outSamples =
	(
		avresample_available(resampleContext) + av_rescale_rnd
		(
			avresample_get_delay(resampleContext) +
			frameIn->linesize[0],
			_sampleRate,
			_sampleRate,
			AV_ROUND_UP
		)
	);

	int outLineSize = 0;
	auto _ret = av_samples_alloc
	(
		output,
		&outLineSize,
		frameIn->nb_samples,
		outSamples,
		outSampleFmt,
		0
	);

	if (_ret < 0)
	{
		LOG(LOG_ERROR, "resampling failed, error code:" << _ret);
		return { nullptr, 0 };
	}

	size_t outSize = avresample_convert
	(
		resampleContext,
		output,
		outLineSize,
		outSamples,
		frameIn->extended_data,
		frameIn->linesize[0],
		frameIn->nb_samples
	) * outSampleFmtSize * av_get_channel_layout_nb_channels(chLayout);
	return { output, outSize };
	#else
	LOG
	(
		LOG_ERROR,
		"unexpected sample format and can't resample, recompile with "
		"libswresample"
	);
	return { nullptr, 0 };
	#endif
}
#else
template<typename T>
size_t FFMpegAudioDecoder::decodeDataImpl
(
	T& samples,
	Span<const uint8_t> data,
	const TimeSpec& time
)
{
	auto& curTail = samples.acquireLast();
	int maxSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	AVPacket pkt;
	av_init_packet(&pkt);

	// If some data was left unprocessed on previous call,
	// concatenate.
	std::vector<uint8_t> combinedBuffer;
	if (overflowBuffer.empty())
	{
		pkt.data = data.getData();
		pkt.size = data.getSize();
	}
	else
	{
		combinedBuffer = overflowBuffer;
		if (!data.empty())
		{
			combinedBuffer.insert
			(
				combinedBuffer.end(),
				data.begin(),
				data.end()
			);
		}

		pkt.data = combinedBuffer.data();
		pkt.size = combinedBuffer.size();
		overflowBuffer.clear();
	}

	auto ret = avcodec_decode_audio3
	(
		codecContext,
		curTail.samples,
		&maxSize,
		&pkt
	);

	if (ret > 0)
	{
		auto tmp = Span<const uint8_t>
		(
			pkt.data,
			pkt.size
		).subSpan(ret);
		if (!tmp.empty())
			overflowBuffer.assign(tmp.begin(), tmp.end());
	}

	curTail.size = maxSize;
	assert(!(curTail.size & 0x80000000) && !(maxSize % 2));
	curTail.current = curTail.samples;
	curTail.time = time;
	samples.commitLast();
	return ret;
}

size_t FFMpegAudioDecoder::decodeData
(
	Span<const uint8_t> data,
	const TimeSpec& time
)
{
	auto ret =
	(
		engineData->audio_useFloatSampleFormat() ?
		decodeDataImpl(samplesBufferF32, data, time) :
		decodeDataImpl(samplesBufferS16, data, time)
	);

	if (status == INIT && fillDataAndCheckValidity())
		status = VALID;
	return maxLen;
}

template<template T>
size_t FFMpegAudioDecoder::decodePacketImpl
(
	T& samples,
	AVPacket* pkt,
	const TimeSpec& time
)
{
	auto& curTail = samples.acquireLast();
	int maxSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
	#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52, 23, 0)
	auto ret = avcodec_decode_audio3
	(
		codecContext,
		curTail.samples,
		&maxSize,
		pkt
	);
	#else
	auto ret = avcodec_decode_audio2
	(
		codecContext,
		curTail.samples,
		&maxSize,
		pkt->data,
		pkt->size
	);
	#endif

	if (ret < 0)
	{
		//A decoding error occurred, create an empty sample buffer
		LOG(LOG_ERROR, "Malformed audio packet");
		curTail.size = 0;
		curTail.current = curTail.samples;
		curTail.time = time;
		samples.commitLast();
		return maxSize;
	}

	assert_and_throw(ret == pkt->size);
	if (status == INIT && fillDataAndCheckValidity())
		status = VALID;

	curTail.size = maxSize;
	assert(!(curTail.size & 0x80000000) && !(maxLen % 2));
	curTail.current = curTail.samples;
	curTail.time = time;
	samples.commitLast();
	return ret;
}

size_t FFMpegAudioDecoder::decodePacket(AVPacket* pkt, const TimeSpec& time)
{
	return
	(
		engineData->audio_useFloatSampleFormat() ?
		decodePacketImpl(samplesBufferF32, pkt, time) :
		decodePacketImpl(samplesBufferS16, pkt, time)
	);
}
#endif
#endif //ENABLE_LIBAVCODEC

StreamDecoder::~StreamDecoder()
{
	delete audioDecoder;
	delete videoDecoder;
}

#ifdef ENABLE_LIBAVCODEC
FFMpegStreamDecoder::FFMpegStreamDecoder
(
	NetStream* _netStream,
	EngineData* engineData,
	std::istream& _stream,
	size_t bufferTime,
	Optional<const AudioFormat&> format,
	size_t streamSize,
	bool forExtraction
) :
valid(false),
netStream(_netStream),
stream(_stream),
formatCtx(nullptr),
audioIndex(-1),
videoIndex(-1),
avioContext(nullptr),
availableStreamSize(streamSize),
fullStreamSize(streamSize)
{
	auto clampStreamSize = [&](size_t _default)
	{
		return
		(
			streamSize != -1 ?
			std::min(_default, streamSize) :
			_default
		);
	};

	auto _size = clampStreamSize(4096);
	avioBuffer = { av_malloc(_size), _size };
	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 105, 0)
	avioContext = avio_alloc_context
	(
		avioBuffer.getData(),
		avioBuffer.getSize(),
		0,
		this,
		avioReadPacket,
		nullptr,
		streamSize != -1 ? avioSeek : nullptr
	);
	#else
	avioContext = av_alloc_put_byte
	(
		avioBuffer.getData(),
		avioBuffer.getSize(),
		0,
		this,
		avioReadPacket,
		nullptr,
		nullptr
	);
	#endif
	if (avioContext == nullptr)
		return;

	#if LIBAVFORMAT_VERSION_MAJOR > 52 || (LIBAVFORMAT_VERSION_MAJOR == 52 && LIBAVFORMAT_VERSION_MINOR > 64)
 	avioContext->seekable = 0;
	#else
	avioContext->is_streamed = 1;
	#endif

	auto fmt = format.transformOr(nullptr, [&](const auto& fmt)
	{
		const char* str = nullptr;
		switch (fmt.codec)
		{
			case LS_AUDIO_CODEC::MP3:
				return av_find_input_format("mp3");
			case LS_AUDIO_CODEC::AAC:
				return av_find_input_format("aac");
			#if __BYTE_ORDER == __BIG_ENDIAN
			case LS_AUDIO_CODEC::LINEAR_PCM_PLATFORM_ENDIAN:
				return av_find_input_format("s16be");
			case LS_AUDIO_CODEC::LINEAR_PCM_FLOAT_PLATFORM_ENDIAN:
				return av_find_input_format("f32be");
			#else
			case LS_AUDIO_CODEC::LINEAR_PCM_PLATFORM_ENDIAN:
				return av_find_input_format("s16le");
			case LS_AUDIO_CODEC::LINEAR_PCM_FLOAT_PLATFORM_ENDIAN:
				return av_find_input_format("f32le");
			#endif
			case LS_AUDIO_CODEC::LINEAR_PCM_LE:
				return av_find_input_format("s16le");
			case LS_AUDIO_CODEC::NELLYMOSER:
			case LS_AUDIO_CODEC::ADPCM:
				format.reset();
				return av_find_input_format("flv");
			case LS_AUDIO_CODEC::CODEC_NONE: return nullptr;
			default:
				LOG
				(
					LOG_NOT_IMPLEMENTED,
					"unsupported audio codec:" << fmt.codec
				);
				return nullptr;
		}
	});

	if (fmt == nullptr)
	{
		//Probe the stream format.
		//NOTE: in FFMpeg 0.7 there is av_probe_input_buffer
		AVProbeData probeData;
		probeData.filename = "lightspark_stream";
		#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		probeData.mime_type = nullptr;
		#endif
		probeData.buf = new uint8_t[8192 + AVPROBE_PADDING_SIZE];
		memset(probeData.buf, 0, 8192 + AVPROBE_PADDING_SIZE);
		auto readCount = clampStreamSize(8192);
		auto read = stream.read
		(
			static_cast<char*>(probeData.buf),
			readCount
		).gcount();

		if (read != readcount)
		{
			LOG
			(
				LOG_ERROR,
				"Not sufficient data is available from the stream:" << read
			);
		}

		probeData.buf_size = read;
		stream.seekg(0);
		fmt = av_probe_input_format(&probeData, 1);
		delete[] probeData.buf;
	}

	if (fmt == nullptr)
		return;

	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 105, 0)
	formatCtx = avformat_alloc_context();
	formatCtx->pb = avioContext;
	auto ret = avformat_open_input
	(
		&formatCtx,
		"lightspark_stream",
		fmt,
		nullptr
	);
	#else
	auto ret = av_open_input_stream
	(
		&formatCtx,
		avioContext,
		"lightspark_stream",
		fmt,
		nullptr
	);
	#endif

	if (ret < 0)
	{
		char buf[1000];
		av_strerror(ret, buf, 1000);
		LOG(LOG_ERROR, "could not open ffmpeg stream:" << buf);
		return;
	}

	if (!format.hasValue())
	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53, 6, 0)
		ret = avformat_find_stream_info(formatCtx, nullptr);
	#else
		ret = av_find_stream_info(formatCtx);
	#endif
	if (ret < 0)
		return;

	LOG_CALL("FFMpeg found " << formatCtx->nb_streams << " streams");
	for (size_t i = 0; i < formatCtx->nb_streams; ++i)
	{
		auto stream = formatCtx->streams[i];
		#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		auto codecType = stream->codecpar->codec_type;
		#else
		auto codecType = stream->codec->codec_type;
		#endif
		if (codecType == AVMEDIA_TYPE_VIDEO && videoIndex < 0)
			videoIndex = i;
		else if (codecType == AVMEDIA_TYPE_AUDIO && audioIndex < 0)
			audioIndex = i;

	}

	if (videoIndex >= 0)
	{
		//Pass the frame rate from the container, the once from the codec is often wrong
		auto stream = formatCtx->streams[videoIndex];
		if (stream->nb_frames > 1 && stream->codecpar->codec_id == CODEC_ID_GIF)
		{
			LOG
			(
				LOG_NOT_IMPLEMENTED,
				"GIF with multiple frames is not properly handled yet"
			);
		}

		videoDecoder = new FFMpegVideoDecoder
		(
			#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
			stream->codecpar,
			#else
			stream->codec,
			#endif
			#if LIBAVUTIL_VERSION_MAJOR < 54
			av_q2d(stream->r_frame_rate)
			#else
			av_q2d(stream->avg_frame_rate)
			#endif
		);
	}

	auto fmtCodec = format.transformOr(CODEC_NONE, [](const auto& fmt)
	{
		return fmt.codec;
	});

	audioDecoder = audioIndex < 0 ? nullptr :
	(
		format.hasValue() &&
		format->codec != CODEC_NONE
	) ? new FFMpegAudioDecoder
	(
		engineData,
		format->codec,
		format->sampleRate,
		format->channels,
		bufferTime,
		true
	) : new FFMpegAudioDecoder
	(
		engineData,
		#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		formatCtx->streams[audioIndex]->codecpar,
		(
			videoDecoder != nullptr ?
			FFMpegVideoDecoder::_bufferSize :
			1
		) *
		#else
		formatCtx->streams[audioIndex]->codec,
		#endif
		bufferTime
	);

	if (audioDecoder != nullptr)
		audioDecoder->forExtraction = forExtraction;

	if (netStream == nullptr || !formatCtx->duration)
	{
		valid = true;
		return;
	}

	auto sys = netStream->getSystemState();
	auto wrk = netStream->getInstanceWorker();
	std::list<asAtom> dataObjList;
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = sys->getUniqueStringId("duration");
	m.isAttribute = false;

	auto dataObj = new_asobject(wrk);
	auto v = asAtomHandler::fromInt(formatCtx->duration / AV_TIME_BASE);

	dataObj->setVariableByMultiname
	(
		m,
		v,
		CONST_NOT_ALLOWED,
		nullptr,
		wrk
	);
	dataObjList.push_back(asAtomHandler::fromObjectNoPrimitive(dataObj));
	netstream->sendClientNotification("onMetaData", dataObjList);

	valid = true;
}

FFMpegStreamDecoder::~FFMpegStreamDecoder()
{
	//Delete the decoders before deleting the input stream to avoid a crash in ffmpeg code
	delete audioDecoder;
	delete videoDecoder;
	audioDecoder = nullptr;
	videoDecoder = nullptr;

	if (formatCtx != nullptr)
	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53, 25, 0)
		avformat_close_input(&formatCtx);
	#elif LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 105, 0)
		av_close_input_file(formatCtx);
	#else
		av_close_input_stream(formatCtx);
	#endif

	if (avioContext != nullptr)
	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 80, 100)
		avio_context_free(&avioContext);
	#else
		av_free(avioContext);
	#endif

	#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 96, 0)
	avformat_free_context(formatCtx);
	#endif
}

void FFMpegStreamDecoder::jumpToPosition(const TimeSpec& pos)
{
	int64_t _pos = pos.toSFloat() * AV_TIME_BASE;
	av_seek_frame(formatCtx, -1, _pos, 0);
	atEnd = false;
}

void FFMpegStreamDecoder::jumpToFrame(size_t frame, bool isVideo)
{
	auto idx = isVideo ? videoIndex : audioIndex;
	assert_and_throw(idx >= 0);
	av_seek_frame(formatCtx, idx, frame, 0);
	atEnd = false;
}

bool FFMpegStreamDecoder::decodeNextFrame()
{
	struct Packet : AVPacket
	{
		bool _initialized { false };

		Packet(AVFormatContext* fmtCtx)
		{
			_initialized = av_read_frame(fmtCtx, this) >= 0;
		}

		~Packet()
		{
			if (!_initialized)
				return;
			#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 12, 100)
			av_packet_unref(this);
			#else
			av_free_packet(this);
			#endif
		}
	} pkt(formatCtx);

	if (!pkt._initialized)
		return false;

	auto timeBase = formatCtx->streams[pkt.stream_index]->time_base;
	//Should use dts
	auto time = TimeSpec::fromFloat(pkt.dts *
	(
		timeBase.den ?
		timeBase.num / number_t(timeBase.den) :
		number_t(timeBase.num)
	));

	if (pkt.stream_index == audioIndex && audioDecoder != nullptr)
	{
		auto _audioDecoder = static_cast
		<
			FFMpegAudioDecoder*
		>(audioDecoder);

		auto _ret = _audioDecoder->decodePacket(&pkt, time);
		// check if the last packet is decoded (only if we know the full size of the audio data)
		atEnd |= fullStreamSize != -1 &&
		(
			_ret == AVERROR_EOF ||
			_ret == AVERROR(EAGAIN)
		);
		return true;
	}
	else if (audioDecoder == nullptr || videoDecoder == nullptr)
		return true;

	auto _videoDecoder = static_cast<FFMpegVideoDecoder*>(videoDecoder);
	if (!_videoDecoder->decodePacket(&pkt, time))
	{
		_videoDecoder->framesDecoded++;
		_hasVideo = true;
	}
	return true;
}

size_t FFMpegStreamDecoder::getAudioSampleRate() const
{
	return audioDecoder != nullptr ? audioDecoder->sampleRate : 0;
}

int FFMpegStreamDecoder::avioReadPacket(void* data, uint8_t* buf, int size)
{
	auto th = static_cast<FFMpegStreamDecoder*>(data);
	// check for available bytes to avoid exception on eof
	if (!th->availableStreamSize)
		return AVERROR_EOF;

	auto ret = th->stream.read
	(
		static_cast<char*>(buf),
		th->availableStreamSize == -1 ? size : std::min<size_t>
		(
			size,
			th->availableStreamSize
		)
	).gcount();
	if (th->availableStreamSize != -1)
		th->availableStreamSize -= ret;
	return ret;
}

int64_t FFMpegStreamDecoder::avioSeek(void* data, int64_t offset, int type)
{
	auto th = static_cast<FFMpegStreamDecoder*>(data);
	switch (type)
	{
		case SEEK_SET:
			th->stream.seekg(offset, std::ios_base::beg);
			th->availableStreamSize = th->fullStreamSize - offset;
			return th->stream.tellg();
		case SEEK_CUR:
			th->availableStreamSize = th->stream.tellg() + offset;
			th->stream.seekg(offset, std::ios_base::cur);
			return th->stream.tellg();
		case SEEK_END:
			th->stream.seekg(offset, std::ios_base::end);
			th->availableStreamSize = -offset;
			return th->stream.tellg();
		case AVSEEK_SIZE: return th->fullStreamSize;
	}
	return -1;
}
#endif //ENABLE_LIBAVCODEC

void SampleDataAudioDecoder::samplesConsumed(size_t samples)
{
	bufferedSamples -= samples;
	soundchannel->semSampleData.signal();
}

size_t SampleDataAudioDecoder::decodeData
(
	Span<const uint8_t> data,
	const TimeSpec& time
)
{
	status =
	(
		status == PREINIT ? INIT :
		status == INIT ? VALID :
		status
	);

	if (engineData->audio_useFloatSampleFormat())
	{
		// no need for conversion as Sample data is always 32 bit float
		auto& curTail = samplesBufferF32.acquireLast();
		memcpy(curTail.samples, data.getData(), data.getSize());
		curTail.size = data.getSize();
		curTail.current = curTail.samples;
		curTail.time = time;
		samplesBufferF32.commitLast();
		bufferedSamples += data.getSizeAs<float>();
		return data.getSize();
	}

	auto& curTail = samplesBufferS16.acquireLast();
	auto sampleCount = std::min<size_t>
	(
		data.getSizeAs<float>(),
		MAX_AUDIO_FRAME_SIZE / 2
	);

	#ifdef HAVE_LIBSWRESAMPLE
	auto _sampleRate = engineData->audio_getSampleRate();
	#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
	auto channelLayout = AV_CHANNEL_LAYOUT_STEREO;
	#else
	auto channelLayout = AV_CH_LAYOUT_STEREO;
	#endif

	auto frameSampleRate = 44100;
	auto outSampleFmt =  AV_SAMPLE_FMT_S16;
	auto outSampleFmtSize = sizeof(int16_t);
	if (resampleContext == nullptr)
	{
		resampleContext = swr_alloc();
		#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57,24,100)
		av_opt_set_chlayout(resampleContext, "in_chlayout", &channelLayout, 0);
		av_opt_set_chlayout(resampleContext, "out_chlayout", &channelLayout, 0);
		#else
		av_opt_set_int(resampleContext, "in_channel_layout", channelLayout, 0);
		av_opt_set_int(resampleContext, "out_channel_layout", channelLayout, 0);
		#endif
		av_opt_set_int(resampleContext, "in_sample_rate",     framesampleRate, 0);
		av_opt_set_int(resampleContext, "out_sample_rate",    _sampleRate, 0);
		av_opt_set_int(resampleContext, "in_sample_fmt",      AV_SAMPLE_FMT_FLT, 0);
		av_opt_set_int(resampleContext, "out_sample_fmt",     outSampleFmt, 0);
		swr_init(resampleContext);
	}

	uint8_t* output;
	auto outSamples = swr_get_out_samples
	(
		resamplContext,
		sampleCount
	);

	auto _ret = av_samples_alloc
	(
		&output,
		nullptr,
		2,
		outSamples,
		outSampleFmt,
		0
	);

	if (_ret < 0)
	{
		LOG
		(
			LOG_ERROR,
			"sampledata resampling failed, error code:" << _ret
		);
		memset(curTail.samples, 0, curTail.size);
		goto end;
	}

	auto _data = &data.getData();
	auto maxSize = swr_convert
	(
		resampleContext,
		&output,
		outSamples,
		_data,
		sampleCount
	) * outSampleFmtSize * 2;

	if (maxSize > 0)
		memcpy(curTail.samples, output, maxSize);
	else
	{
		LOG(LOG_ERROR, "sampledata resampling failed");
		memset(curTail.samples, 0, curTail.size);
	}
	av_freep(&output);
end:
	#elif defined HAVE_LIBAVRESAMPLE
	if (resampleContext == nullptr)
	{
		resampleContext = avresample_alloc_context();
		av_opt_set_int(resampleContext, "in_channel_layout",  channel_layout, 0);
		av_opt_set_int(resampleContext, "out_channel_layout", channel_layout,  0);
		av_opt_set_int(resampleContext, "in_sample_rate",     framesamplerate,     0);
		av_opt_set_int(resampleContext, "out_sample_rate",    sample_rate,     0);
		av_opt_set_int(resampleContext, "in_sample_fmt",      AV_SAMPLE_FMT_FLT,   0);
		av_opt_set_int(resampleContext, "out_sample_fmt",     outputsampleformat,    0);
		avresample_open(resampleContext);
	}

	uint8_t *output;
	int outLineSize;
	auto outSamples =
	(
		avresample_available(resampleContext) +
		av_rescale_rnd
		(
			avresample_get_delay(resampleContext) +
			data.getSize(),
			_sampleRate,
			_sampleRate,
			AV_ROUND_UP
		)
	);

	auto _ret = av_samples_alloc
	(
		&output,
		&outLineSize,
		sampleCount,
		outSamples,
		outSampleFmt,
		0
	);

	if (_ret < 0)
	{
		LOG
		(
			LOG_ERROR,
			"sampledata resampling failed, error code:" << _ret
		);
		memset(curTail.samples, 0, curTail.size);
		goto end;
	}
	auto maxSize = avresample_convert
	(
		resampleContext,
		&output, outLineSize,
		outSamples,
		data.getData(),
		data.getSize(),
		sampleCount
	) *
	(
		outSampleFmtSize *
		av_get_channel_layout_nb_channels(channelLayout)
	);
	memcpy(curTail.samples, output, maxLen);
	av_freep(&output);
end:
	#else
	auto dataF32 = data.as<float>();
	for (size_t i = 0; i < sampleCount; ++i)
	{
		curTail.samples[i] =
		(
			dataF32[i] > 1 ? INT16_MAX :
			dataF32[i] < -1 ? INT16_MIN :
			int32_t(dataF32[i] * 32768 + 32768.5) - 32768
		);
		if (dataF32[i] > 1 || dataF32[i] < -1)
		{
			LOG
			(
				LOG_ERROR,
				"decodedata:" << dataF32[i] << ' ' <<
				dataF32[i] * 32768.0f << << curTail.samples[i]
			);
		}
	}
	#endif
	curTail.size = sampleCount * 2;
	curTail.current = curTail.samples;
	curTail.time = time;
	samplesBufferS16.commitLast();
	bufferedSamples += sampleCount;
	return sampleCount * 2;
}

AudioDecoder::F32SamplePair SampleDataAudioDecoder::getNextSampleF32()
{
}

AudioDecoder::S16SamplePair SampleDataAudioDecoder::getNextSampleS16()
{
}

size_t SampleDataAudioDecoder::getSamples(Span<F32SamplePair> span)
{
}

size_t SampleDataAudioDecoder::getSamples(Span<S16SamplePair> span)
{
}
