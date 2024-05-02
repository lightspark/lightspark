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

#include "compat.h"
#include <cassert>

#include "backends/decoder.h"
#include "platforms/fastpaths.h"
#include "platforms/engineutils.h"
#include "swf.h"
#include "backends/rendering.h"
#include "scripting/class.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/media/flashmedia.h"
#include "parsing/tags.h"

#if LIBAVUTIL_VERSION_MAJOR < 51
#define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
#define AVMEDIA_TYPE_AUDIO CODEC_TYPE_AUDIO
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,45,101)
#define av_frame_alloc avcodec_alloc_frame
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,45,101)
#define av_frame_unref avcodec_get_frame_defaults
#endif

using namespace lightspark;
using namespace std;

bool VideoDecoder::setSize(uint32_t w, uint32_t h)
{
	if(w!=frameWidth || h!=frameHeight)
	{
		frameWidth=w;
		frameHeight=h;
		LOG(LOG_INFO,"VIDEO DEC: Video frame size " << frameWidth << 'x' << frameHeight);
		resizeGLBuffers=true;
#ifdef _WIN32
		if (decodedframebuffer)
			_aligned_free(decodedframebuffer);
		decodedframebuffer = (uint8_t*)_aligned_malloc(frameWidth*frameHeight*4, 16);
		if (!decodedframebuffer) {
			LOG(LOG_ERROR, "posix_memalign could not allocate memory");
		}
#else
		if (decodedframebuffer)
			free(decodedframebuffer);
		if(posix_memalign((void **)&decodedframebuffer, 16, frameWidth*frameHeight*4)) {
			LOG(LOG_ERROR, "posix_memalign could not allocate memory");
		}
#endif
		return true;
	}
	else
		return false;
}

bool VideoDecoder::resizeIfNeeded(TextureChunk& tex)
{
	if(!resizeGLBuffers)
		return false;

	//Chunks are at least aligned to 128, we need 16
	assert_and_throw(tex.width==frameWidth && tex.height==frameHeight);
	resizeGLBuffers=false;
	return true;
}

void VideoDecoder::sizeNeeded(uint32_t& w, uint32_t& h) const
{
	//Return the actual width aligned to 16, the SSE2 packer is advantaged by this
	//and it comes for free as the texture tiles are aligned to 128
	w=(frameWidth+15)&0xfffffff0;
	h=frameHeight;
}

TextureChunk& VideoDecoder::getTexture()
{
	if (!videoTexture.isValid())
		videoTexture=getSys()->getRenderThread()->allocateTexture(frameWidth, frameHeight, true);
	
	return videoTexture;
}

void VideoDecoder::uploadFence()
{
	ITextureUploadable::uploadFence();
	assert(fenceCount);
	fenceCount--;
	if (markedForDeletion && fenceCount==0)
		delete this;
}

void VideoDecoder::markForDestruction()
{
	markedForDeletion=true;
}

void VideoDecoder::clearFrameBuffer()
{
	if (decodedframebuffer)
		memset(decodedframebuffer,0,frameWidth*frameHeight*4);
}
VideoDecoder::VideoDecoder():decodedframebuffer(nullptr),frameRate(0),framesdecoded(0),framesdropped(0),frameWidth(0),frameHeight(0),lastframe(UINT32_MAX),currentframe(UINT32_MAX),fenceCount(0),resizeGLBuffers(false),markedForDeletion(false)
{
}

VideoDecoder::~VideoDecoder()
{
	if(videoTexture.isValid())
	{
		RenderThread *rt=getSys()->getRenderThread();
		if(rt && rt->isStarted())
			rt->releaseTexture(getTexture());
	}
#ifdef _WIN32
	if (decodedframebuffer)
		_aligned_free(decodedframebuffer);
#else
	if (decodedframebuffer)
		free(decodedframebuffer);
#endif
}

void VideoDecoder::waitForFencing()
{
	fenceCount++;
}

#ifdef ENABLE_LIBAVCODEC
bool FFMpegVideoDecoder::fillDataAndCheckValidity()
{
	if(frameRate==0 && codecContext->time_base.num!=0)
	{
		frameRate=codecContext->time_base.den;
		frameRate/=codecContext->time_base.num;
		if(videoCodec==H264) //H264 has half ticks (usually?)
			frameRate/=2;
	}
	else if(frameRate==0)
		return false;

	if(codecContext->width!=0 && codecContext->height!=0)
		setSize(codecContext->width, codecContext->height);
	else
		return false;

	return true;
}

FFMpegVideoDecoder::FFMpegVideoDecoder(LS_VIDEO_CODEC codecId, uint8_t* initdata, uint32_t datalen, double frameRateHint, DefineVideoStreamTag *tag):
	ownedContext(true),curBuffer(0),codecContext(nullptr),streamingbuffers(FFMPEGVIDEODECODERBUFFERSIZE),embeddedbuffers(2),curBufferOffset(0),embeddedvideotag(tag)
{
	//The tag is the header, initialize decoding
	switchCodec(codecId, initdata, datalen, frameRateHint);
	frameIn=av_frame_alloc();
	// immediately decode 1 frame to obtain size:
	// 'define stream' tag information not always correct + cannot resize during an 'upload' call
	if (tag)
	{
		VideoFrameTag* t = tag->getFrame(0);
		decodeData(t->getData(), t->getNumBytes(), UINT32_MAX);
	}
}

void FFMpegVideoDecoder::switchCodec(LS_VIDEO_CODEC codecId, uint8_t *initdata, uint32_t datalen, double frameRateHint)
{
	if (codecContext)
	{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,63,100)
		avcodec_free_context(&codecContext);
#else
		avcodec_close(codecContext);
		if(ownedContext)
			av_free(codecContext);
#endif
	}
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	codecContext=avcodec_alloc_context3(nullptr);
#else
	codecContext=avcodec_alloc_context();
#endif
	const AVCodec* codec=nullptr;
	videoCodec=codecId;
	if(codecId==H264)
	{
		//TODO: serialize access to avcodec_open
		codec=avcodec_find_decoder(CODEC_ID_H264);
		assert(codec);
		//Ignore the frameRateHint as the rate is gathered from the video data
	}
	else if(codecId==H263)
	{
		//TODO: serialize access to avcodec_open
		codec=avcodec_find_decoder(CODEC_ID_FLV1);
		assert(codec);

		//Exploit the frame rate information
		assert(frameRateHint!=0.0);
		frameRate=frameRateHint;
	}
	else if(codecId==VP6)
	{
		//TODO: serialize access to avcodec_open
		codec=avcodec_find_decoder(CODEC_ID_VP6F);
		assert(codec);

		//Exploit the frame rate information
		assert(frameRateHint!=0.0);
		frameRate=frameRateHint;
	}
	else if(codecId==VP6A)
	{
		//TODO: serialize access to avcodec_open
		codec=avcodec_find_decoder(CODEC_ID_VP6A);
		assert(codec);

		//Exploit the frame rate information
		assert(frameRateHint!=0.0);
		frameRate=frameRateHint;
	}
	else if(codecId==GIF)
	{
		//TODO: serialize access to avcodec_open
		codec=avcodec_find_decoder(CODEC_ID_GIF);
		assert(codec);

		//Exploit the frame rate information
		assert(frameRateHint!=0.0);
		frameRate=frameRateHint;
	}
	if (initdata)
	{
		codecContext->extradata=initdata;
		codecContext->extradata_size=datalen;
	}
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	if(avcodec_open2(codecContext, codec, nullptr)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif
		throw RunTimeException("Cannot open decoder");

	if(fillDataAndCheckValidity())
		status=VALID;
	else
		status=INIT;
}
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
FFMpegVideoDecoder::FFMpegVideoDecoder(AVCodecParameters* codecPar, double frameRateHint):
	ownedContext(true),curBuffer(0),codecContext(nullptr),streamingbuffers(FFMPEGVIDEODECODERBUFFERSIZE),embeddedbuffers(2),curBufferOffset(0),embeddedvideotag(nullptr)
{
	status=INIT;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	codecContext=avcodec_alloc_context3(nullptr);
#else
	codecContext=avcodec_alloc_context();
#endif
	frameIn=av_frame_alloc();
	//The tag is the header, initialize decoding
	switch(codecPar->codec_id)
	{
		case CODEC_ID_H264:
			videoCodec=H264;
			break;
		case CODEC_ID_FLV1:
			videoCodec=H263;
			break;
		case CODEC_ID_VP6F:
			videoCodec=VP6;
			break;
		case CODEC_ID_GIF:
			videoCodec=GIF;
			break;
		default:
			return;
	}
	avcodec_parameters_to_context(codecContext,codecPar);
	const AVCodec* codec=avcodec_find_decoder(codecPar->codec_id);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	if(avcodec_open2(codecContext, codec, nullptr)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif
		return;

	frameRate=frameRateHint;

	if(fillDataAndCheckValidity())
		status=VALID;

}
#else
FFMpegVideoDecoder::FFMpegVideoDecoder(AVCodecContext* _c, double frameRateHint):
	ownedContext(false),curBuffer(0),codecContext(_c),curBufferOffset(0),embeddedvideotag(nullptr)
{
	frameIn=av_frame_alloc();
	status=INIT;
	//The tag is the header, initialize decoding
	switch(codecContext->codec_id)
	{
		case CODEC_ID_H264:
			videoCodec=H264;
			break;
		case CODEC_ID_FLV1:
			videoCodec=H263;
			break;
		case CODEC_ID_VP6F:
			videoCodec=VP6;
			break;
		default:
			return;
	}
	const AVCodec* codec=avcodec_find_decoder(codecContext->codec_id);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	if(avcodec_open2(codecContext, codec, nullptr)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif
		return;

	frameRate=frameRateHint;

	if(fillDataAndCheckValidity())
		status=VALID;

}
#endif

FFMpegVideoDecoder::~FFMpegVideoDecoder()
{
	while(fenceCount);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55,63,100)
	avcodec_free_context(&codecContext);
#else
	avcodec_close(codecContext);
	if(ownedContext)
		av_free(codecContext);
#endif
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 0, 0)
	av_frame_free(&frameIn);
#else
	av_free(frameIn);
#endif
}

//setSize is called from the routine that inserts new frames
void FFMpegVideoDecoder::setSize(uint32_t w, uint32_t h)
{
	if(VideoDecoder::setSize(w,h))
	{
		//Discard all the frames
		while(discardFrame());
	
		//As the size changed, reset the buffer
		uint32_t bufferSize=frameWidth*frameHeight/**4*/;
		if (embeddedvideotag)
			embeddedbuffers.regen(YUVBufferGenerator(bufferSize,this->codecContext->pix_fmt==AV_PIX_FMT_YUVA420P, this->codecContext->pix_fmt!=AV_PIX_FMT_BGRA));
		else
			streamingbuffers.regen(YUVBufferGenerator(bufferSize,this->codecContext->pix_fmt==AV_PIX_FMT_YUVA420P, this->codecContext->pix_fmt!=AV_PIX_FMT_BGRA));
	}
}

uint32_t FFMpegVideoDecoder::skipUntil(uint32_t time)
{
	uint32_t ret=0;
	if (embeddedvideotag)
	{
		while(1)
		{
			if(embeddedbuffers.isEmpty())
				break;
			if(embeddedbuffers.front().time>=time)
				break;
			discardFrame();
			ret++;
		}
	}
	else
	{
		while(1)
		{
			if(streamingbuffers.isEmpty())
				break;
			if(streamingbuffers.front().time>=time)
				break;
			discardFrame();
			ret++;
		}
	}
	return ret;
}
void FFMpegVideoDecoder::skipAll()
{
	while(!streamingbuffers.isEmpty())
		discardFrame();
	while(!embeddedbuffers.isEmpty())
		discardFrame();
}

bool FFMpegVideoDecoder::discardFrame()
{
	//We don't want ot block if no frame is available
	if (embeddedvideotag)
	{
		bool ret=embeddedbuffers.nonBlockingPopFront();
		if(flushing && embeddedbuffers.isEmpty()) //End of our work
		{
			status=FLUSHED;
			flushed.signal();
		}
		framesdropped++;
	
		return ret;
	}
	else
	{
		bool ret=streamingbuffers.nonBlockingPopFront();
		if(flushing && streamingbuffers.isEmpty()) //End of our work
		{
			status=FLUSHED;
			flushed.signal();
		}
		framesdropped++;
	
		return ret;
	}
}

bool FFMpegVideoDecoder::decodeData(uint8_t* data, uint32_t datalen, uint32_t time)
{
	if(datalen==0)
		return false;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	AVPacket* pkt = av_packet_alloc();
	if (!pkt)
		return 0;
	pkt->data=data;
	pkt->size=datalen;
	int ret = avcodec_send_packet(codecContext, pkt);
	while (ret == 0)
	{
		ret = avcodec_receive_frame(codecContext,frameIn);
		if (ret != 0)
		{
			if (ret != AVERROR(EAGAIN))
			{
				LOG(LOG_INFO,"not decoded:"<<ret);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,12,100)
				av_packet_unref(pkt);
#else
				av_free_packet(pkt);
#endif
				return false;
			}
		}
		else
		{
			if(status==INIT && fillDataAndCheckValidity())
				status=VALID;
	
			assert(frameIn->pts==(int64_t)AV_NOPTS_VALUE || frameIn->pts==0);
			if (time != UINT32_MAX)
				copyFrameToBuffers(frameIn, time);
		}
	}
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,12,100)
	av_packet_unref(pkt);
#else
	av_free_packet(pkt);
#endif
	av_packet_free(&pkt);
#else
	int frameOk=0;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,23,0)
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data=data;
	pkt.size=datalen;
	int ret=avcodec_decode_video2(codecContext, frameIn, &frameOk, &pkt);
#else
	int ret=avcodec_decode_video(codecContext, frameIn, &frameOk, data, datalen);
#endif
	if (ret < 0)
	{
		LOG(LOG_INFO,"not decoded:"<<ret<<" "<< frameOk);
		return false;
	}
	if(frameOk)
	{
		//assert(codecContext->pix_fmt==PIX_FMT_YUV420P);

		if(status==INIT && fillDataAndCheckValidity())
			status=VALID;

		assert(frameIn->pts==(int64_t)AV_NOPTS_VALUE || frameIn->pts==0);

		if (time != UINT32_MAX)
			copyFrameToBuffers(frameIn, time);
	}
#endif
	return true;
}

bool FFMpegVideoDecoder::decodePacket(AVPacket* pkt, uint32_t time)
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	int ret = avcodec_send_packet(codecContext, pkt);
	while (ret == 0)
	{
		ret = avcodec_receive_frame(codecContext,frameIn);
		if (ret != 0)
		{
			if (ret != AVERROR(EAGAIN))
			{
				LOG(LOG_INFO,"not decoded:"<<ret);
				return false;
			}
		}
		else
		{
			if(status==INIT && fillDataAndCheckValidity())
				status=VALID;
	
			AVDictionary* meta = frameIn->metadata;
			if (meta)
			{
				AVDictionaryEntry* entry = nullptr;
				while (true)
				{
					entry = av_dict_get(meta, "",entry,AV_DICT_IGNORE_SUFFIX);
					if (!entry)
						break;
					LOG(LOG_NOT_IMPLEMENTED,"sending metadata from stream:"<<entry->key<<" "<<entry->value);
				}
			}
			copyFrameToBuffers(frameIn, time);
		}
	}
#else
	int frameOk=0;

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,23,0)
	int ret=avcodec_decode_video2(codecContext, frameIn, &frameOk, pkt);
#else
	int ret=avcodec_decode_video(codecContext, frameIn, &frameOk, pkt->data, pkt->size);
#endif
	if (ret < 0)
	{
		LOG(LOG_INFO,"not decoded:"<<ret<<" "<< frameOk);
		return false;
	}

	assert_and_throw(ret==(int)pkt->size);
	if(frameOk)
	{
		//assert(codecContext->pix_fmt==PIX_FMT_YUV420P);

		if(status==INIT && fillDataAndCheckValidity())
			status=VALID;

		assert(frameIn->pts==(int64_t)AV_NOPTS_VALUE || frameIn->pts==0);

		copyFrameToBuffers(frameIn, time);
	}
#endif
	return true;
}

void FFMpegVideoDecoder::copyFrameToBuffers(const AVFrame* frameIn, uint32_t time)
{
	YUVBuffer* curTail=nullptr;
	curTail=embeddedvideotag ?  &embeddedbuffers.acquireLast() : &streamingbuffers.acquireLast();
	//Only one thread may access the tail
	int offset[3]={0,0,0};
	// ffmpeg seems to decode GIFs in AV_PIX_FMT_BGRA format and puts all data in first channel
	if (codecContext->pix_fmt==AV_PIX_FMT_BGRA)
	{
		uint32_t fw = frameWidth*4;
		for(uint32_t y=0;y<frameHeight;y++)
		{
			for(uint32_t x=0;x<frameWidth;x++)
			{
				// convert BGRA to RGBA
				curTail->ch[0][fw*y+x*4+2] = frameIn->data[0][y*frameIn->linesize[0]+x*4  ];
				curTail->ch[0][fw*y+x*4+1] = frameIn->data[0][y*frameIn->linesize[0]+x*4+1];
				curTail->ch[0][fw*y+x*4  ] = frameIn->data[0][y*frameIn->linesize[0]+x*4+2];
				curTail->ch[0][fw*y+x*4+3] = frameIn->data[0][y*frameIn->linesize[0]+x*4+3];
			}
		}
	}
	else
	{
		for(uint32_t y=0;y<frameHeight;y++)
		{
			memcpy(curTail->ch[0]+offset[0],frameIn->data[0]+(y*frameIn->linesize[0]),frameWidth);
			if (codecContext->pix_fmt==AV_PIX_FMT_YUVA420P)
				memcpy(curTail->ch[3]+offset[0],frameIn->data[3]+(y*frameIn->linesize[0]),frameWidth);
			offset[0]+=frameWidth;
		}
		if (codecContext->pix_fmt!=AV_PIX_FMT_BGRA)
		{
			for(uint32_t y=0;y<frameHeight/2;y++)
			{
				memcpy(curTail->ch[1]+offset[1],frameIn->data[1]+(y*frameIn->linesize[1]),frameWidth/2);
				memcpy(curTail->ch[2]+offset[2],frameIn->data[2]+(y*frameIn->linesize[2]),frameWidth/2);
				offset[1]+=frameWidth/2;
				offset[2]+=frameWidth/2;
			}
		}
	}
	curTail->time=time;
	if (embeddedvideotag)
		embeddedbuffers.commitLast();
	else
		streamingbuffers.commitLast();
}

uint8_t* FFMpegVideoDecoder::upload(bool refresh)
{
	assert_and_throw(decodedframebuffer);
	if (!refresh)
		return decodedframebuffer;
	if (embeddedvideotag) // on embedded video we decode the frames during upload
	{
		if (currentframe < lastframe)
		{
			currentframe = 0;
			lastframe = UINT32_MAX;
		}

		skipAll();
		for (uint32_t i = lastframe+1; i <= currentframe; i++)
		{
			VideoFrameTag* t = embeddedvideotag->getFrame(i);
			if (!t)
				return decodedframebuffer;
			decodeData(t->getData(), t->getNumBytes(), (i == currentframe) ? 0 : UINT32_MAX);
			lastframe = i;
		}
		if(embeddedbuffers.isEmpty())
			return decodedframebuffer;
	}
	else
	{
		if(streamingbuffers.isEmpty())
			return decodedframebuffer;
	}
	//At least a frame is available
	YUVBuffer* cur=embeddedvideotag ? &embeddedbuffers.front() : &streamingbuffers.front();
	if (codecContext->pix_fmt==AV_PIX_FMT_BGRA)
	{
		memcpy(decodedframebuffer,cur->ch[0],frameWidth*frameHeight*4);
	}
	else
	{
		fastYUV420ChannelsToYUV0Buffer(cur->ch[0],cur->ch[1],cur->ch[2],decodedframebuffer,frameWidth,frameHeight);
		if (codecContext->pix_fmt==AV_PIX_FMT_YUVA420P)
		{
			uint32_t texw= (frameWidth+15)&0xfffffff0;
			for(uint32_t i=0;i<frameHeight;i++)
			{
				for(uint32_t j=0;j<frameWidth;j++)
				{
					uint32_t pixelCoordFull=i*texw+j;
					decodedframebuffer[pixelCoordFull*4+3]=cur->ch[3][i*frameWidth+j];
				}
			}
		}
	}
	return decodedframebuffer;
}

void FFMpegVideoDecoder::YUVBufferGenerator::init(YUVBuffer& buf) const
{
	if(buf.ch[0])
	{
		aligned_free(buf.ch[0]);
		if (hasChannels)
		{
			aligned_free(buf.ch[1]);
			aligned_free(buf.ch[2]);
			if (buf.ch[3])
				aligned_free(buf.ch[3]);
		}
	}
	if (hasChannels)
	{
		aligned_malloc((void**)&buf.ch[0], 16, bufferSize);
		aligned_malloc((void**)&buf.ch[1], 16, bufferSize/4);
		aligned_malloc((void**)&buf.ch[2], 16, bufferSize/4);
		if (hasAlpha)
			aligned_malloc((void**)&buf.ch[3], 16, bufferSize);
	}
	else
	{
		aligned_malloc((void**)&buf.ch[0], 16, bufferSize*4);
	}
}
#endif //ENABLE_LIBAVCODEC

bool AudioDecoder::discardFrame()
{
	return engine->audio_useFloatSampleFormat() ? discardFrameF32() : discardFrameS16();
}
bool AudioDecoder::discardFrameS16()
{
	//We don't want to block if no frame is available
	bool ret= samplesBufferS16.nonBlockingPopFront();
	if (!ret)
		LOG(LOG_ERROR,"discardFrame blocking "<<flushing<<" "<<samplesBufferS16.isEmpty());
	if(flushing && samplesBufferS16.isEmpty()) //End of our work
	{
		status=FLUSHED;
		flushed.signal();
	}
	return ret;
}
bool AudioDecoder::discardFrameF32()
{
	//We don't want to block if no frame is available
	bool ret= samplesBufferF32.nonBlockingPopFront();
	if (!ret)
		LOG(LOG_ERROR,"discardFrame blocking "<<flushing<<" "<<samplesBufferF32.isEmpty());
	if(flushing && samplesBufferF32.isEmpty()) //End of our work
	{
		status=FLUSHED;
		flushed.signal();
	}
	return ret;
}

uint32_t AudioDecoder::copyFrameS16(int16_t* dest, uint32_t len)
{
	assert(dest);
	if(samplesBufferS16.isEmpty())
	{
		if(flushing) //End of our work
		{
			status=FLUSHED;
			flushed.signal();
		}
		return 0;
	}
	uint32_t frameSize=min(samplesBufferS16.front().len,len);
	memcpy(dest,samplesBufferS16.front().current,frameSize);
	samplesBufferS16.front().len-=frameSize;
	assert(!(samplesBufferS16.front().len&0x80000000));
	if(samplesBufferS16.front().len==0)
	{
		samplesBufferS16.nonBlockingPopFront();
		if(flushing && samplesBufferS16.isEmpty()) //End of our work
		{
			status=FLUSHED;
			flushed.signal();
		}
	}
	else
	{
		samplesBufferS16.front().current+=frameSize/2;
		samplesBufferS16.front().time+=frameSize/getBytesPerMSec();
	}
	samplesconsumed(frameSize/2);
	return frameSize;
}

uint32_t AudioDecoder::copyFrameF32(float* dest, uint32_t len)
{
	assert(dest);
	if(samplesBufferF32.isEmpty())
	{
		if(flushing) //End of our work
		{
			status=FLUSHED;
			flushed.signal();
		}
		return 0;
	}
	uint32_t frameSize=min(samplesBufferF32.front().len,len);
	memcpy(dest,samplesBufferF32.front().current,frameSize);
	samplesBufferF32.front().len-=frameSize;
	assert(!(samplesBufferF32.front().len&0x80000000));
	if(samplesBufferF32.front().len==0)
	{
		samplesBufferF32.nonBlockingPopFront();
		if(flushing && samplesBufferF32.isEmpty()) //End of our work
		{
			status=FLUSHED;
			flushed.signal();
		}
	}
	else
	{
		samplesBufferF32.front().current+=frameSize/4;
		samplesBufferF32.front().time+=frameSize/getBytesPerMSec();
	}
	samplesconsumed(frameSize/4);
	return frameSize;
}

AudioDecoder::AudioDecoder(uint32_t size, EngineData* _engine):
#if defined HAVE_LIBAVRESAMPLE || defined HAVE_LIBSWRESAMPLE
	resamplecontext(nullptr),
#endif
	sampleRate(0),engine(_engine),samplesBufferS16(_engine->audio_useFloatSampleFormat() ? 0 : size),samplesBufferF32(_engine->audio_useFloatSampleFormat() ? size : 0),channelCount(0),initialTime(-1),forExtraction(false)
{
	
}

AudioDecoder::~AudioDecoder()
{
#ifdef HAVE_LIBSWRESAMPLE
	if (resamplecontext)
		swr_free(&resamplecontext);
	resamplecontext=nullptr;
#elif defined HAVE_LIBAVRESAMPLE
	if (resamplecontext)
		avresample_free(&resamplecontext);
	resamplecontext=nullptr;
#endif
}

uint32_t AudioDecoder::getFrontTime() const
{
	assert(!samplesBufferS16.isEmpty() || !samplesBufferF32.isEmpty());
	return engine->audio_useFloatSampleFormat() ? samplesBufferF32.front().time : samplesBufferS16.front().time;
}

void AudioDecoder::skipUntil(uint32_t time, uint32_t usecs)
{
	assert(isValid());
	if (engine->audio_useFloatSampleFormat())
	{
		//	while(1) //Should loop, but currently only usec adjustements are requested
		{
			if(samplesBufferF32.isEmpty())
				return;
			FrameSamplesF32& cur=samplesBufferF32.front();
			assert(time==cur.time);
			if(usecs==0) //Nothing to skip
				return;
			//Check how many bytes are needed to fill the gap
			uint32_t bytesToDiscard=(time-cur.time)*getBytesPerMSec()+usecs*getBytesPerMSec()/1000;
			bytesToDiscard&=0xfffffffe;
			
			if(cur.len<=bytesToDiscard) //The whole frame is droppable
				discardFrameF32();
			else
			{
				assert((bytesToDiscard%2)==0);
				cur.len-=bytesToDiscard;
				assert(!(cur.len&0x80000000));
				cur.current+=(bytesToDiscard/2);
				cur.time=time;
				return;
			}
		}
	}
	else
	{
		//	while(1) //Should loop, but currently only usec adjustements are requested
		{
			if(samplesBufferS16.isEmpty())
				return;
			FrameSamplesS16& cur=samplesBufferS16.front();
			assert(time==cur.time);
			if(usecs==0) //Nothing to skip
				return;
			//Check how many bytes are needed to fill the gap
			uint32_t bytesToDiscard=(time-cur.time)*getBytesPerMSec()+usecs*getBytesPerMSec()/1000;
			bytesToDiscard&=0xfffffffe;
	
			if(cur.len<=bytesToDiscard) //The whole frame is droppable
				discardFrameS16();
			else
			{
				assert((bytesToDiscard%2)==0);
				cur.len-=bytesToDiscard;
				assert(!(cur.len&0x80000000));
				cur.current+=(bytesToDiscard/2);
				cur.time=time;
				return;
			}
		}
	}
}

void AudioDecoder::skipAll()
{
	while(!samplesBufferS16.isEmpty())
		discardFrameS16();
	while(!samplesBufferF32.isEmpty())
		discardFrameF32();
}

#ifdef ENABLE_LIBAVCODEC
FFMpegAudioDecoder::FFMpegAudioDecoder(EngineData* eng, LS_AUDIO_CODEC audioCodec, uint8_t* initdata, uint32_t datalen, uint32_t buffertime):AudioDecoder(buffertime+1,eng),ownedContext(true)
{
	switchCodec(audioCodec,initdata,datalen);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	frameIn=av_frame_alloc();
#endif
}
void FFMpegAudioDecoder::switchCodec(LS_AUDIO_CODEC audioCodec, uint8_t* initdata, uint32_t datalen)
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 63, 100)
	if (codecContext)
		avcodec_free_context(&codecContext);
#else
	if (codecContext)
		avcodec_close(codecContext);
#endif
#ifdef HAVE_LIBSWRESAMPLE
	if (resamplecontext)
		swr_free(&resamplecontext);
	resamplecontext=nullptr;
#elif defined HAVE_LIBAVRESAMPLE
	if (resamplecontext)
		avresample_free(&resamplecontext);
	resamplecontext=nullptr;
#endif
	const AVCodec* codec=avcodec_find_decoder(LSToFFMpegCodec(audioCodec));
	assert(codec);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	codecContext=avcodec_alloc_context3(nullptr);
#else
	codecContext=avcodec_alloc_context();
#endif

	if(initdata)
	{
		codecContext->extradata=initdata;
		codecContext->extradata_size=datalen;
	}

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	if(avcodec_open2(codecContext, codec, nullptr)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif
		throw RunTimeException("Cannot open decoder");

	if(fillDataAndCheckValidity())
		status=VALID;
	else
		status=INIT;
}

FFMpegAudioDecoder::FFMpegAudioDecoder(EngineData* eng, LS_AUDIO_CODEC lscodec, int sampleRate, int channels, uint32_t buffertime, bool):AudioDecoder(buffertime+1,eng),ownedContext(true)
{
	status=INIT;

	CodecID codecId = LSToFFMpegCodec(lscodec);
	const AVCodec* codec=avcodec_find_decoder(codecId);
	assert(codec);
	codecContext=avcodec_alloc_context3(codec);
	codecContext->codec_id = codecId;
	codecContext->sample_rate = sampleRate;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57,24,100)
	codecContext->ch_layout.nb_channels = channels;
	switch (channels)
	{
		case 1:
			codecContext->ch_layout = AV_CHANNEL_LAYOUT_MONO;
			break;
		case 2:
			codecContext->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"FFMpegAudioDecoder: channel layout for "<<channels<<" channels");
			break;
	}
#else
	codecContext->channels = channels;
	switch (channels)
	{
		case 1:
			codecContext->channel_layout = AV_CH_LAYOUT_MONO;
			break;
		case 2:
			codecContext->channel_layout = AV_CH_LAYOUT_STEREO;
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"FFMpegAudioDecoder: channel layout for "<<channels<<" channels");
			break;
	}
#endif
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	if(avcodec_open2(codecContext, codec, nullptr)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif
		return;

	if(fillDataAndCheckValidity())
		status=VALID;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	frameIn=av_frame_alloc();
#endif
}

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
FFMpegAudioDecoder::FFMpegAudioDecoder(EngineData* eng,AVCodecParameters* codecPar, uint32_t buffertime):AudioDecoder(buffertime+1,eng),ownedContext(true),codecContext(nullptr)
{
	status=INIT;
	const AVCodec* codec=avcodec_find_decoder(codecPar->codec_id);
	assert(codec);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	codecContext=avcodec_alloc_context3(nullptr);
#else
	codecContext=avcodec_alloc_context();
#endif
	avcodec_parameters_to_context(codecContext,codecPar);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	if(avcodec_open2(codecContext, codec, nullptr)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif
		return;

	if(fillDataAndCheckValidity())
		status=VALID;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	frameIn=av_frame_alloc();
#endif
}
#else
FFMpegAudioDecoder::FFMpegAudioDecoder(EngineData* eng,AVCodecContext* _c, uint32_t buffertime):AudioDecoder(buffertime+1,eng),ownedContext(false),codecContext(_c)
{
	status=INIT;
	const AVCodec* codec=avcodec_find_decoder(codecContext->codec_id);
	assert(codec);

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,8,0)
	if(avcodec_open2(codecContext, codec, nullptr)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif
		return;

	if(fillDataAndCheckValidity())
		status=VALID;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	frameIn=av_frame_alloc();
#endif
}
#endif

FFMpegAudioDecoder::~FFMpegAudioDecoder()
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 63, 100)
	if(ownedContext)
		avcodec_free_context(&codecContext);
#else
	if(ownedContext)
	{
		avcodec_close(codecContext);
		av_free(codecContext);
	}
#endif
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 0, 0)
	av_frame_free(&frameIn);
#elif LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	av_free(frameIn);
#endif
}

CodecID FFMpegAudioDecoder::LSToFFMpegCodec(LS_AUDIO_CODEC LSCodec)
{
	switch(LSCodec)
	{
		case AAC:
			return CODEC_ID_AAC;
		case MP3:
			return CODEC_ID_MP3;
		case ADPCM:
			return CODEC_ID_ADPCM_SWF;
		case LINEAR_PCM_PLATFORM_ENDIAN:
#if __BYTE_ORDER == __BIG_ENDIAN
			return CODEC_ID_PCM_S16BE;
#else
			return CODEC_ID_PCM_S16LE;
#endif
		case LINEAR_PCM_LE:
			return CODEC_ID_PCM_S16LE;
		case LINEAR_PCM_FLOAT_PLATFORM_ENDIAN:
#if __BYTE_ORDER == __BIG_ENDIAN
			return CODEC_ID_PCM_F32BE;
#else
			return CODEC_ID_PCM_F32LE;
#endif
		default:
			return CODEC_ID_NONE;
	}
}

bool FFMpegAudioDecoder::fillDataAndCheckValidity()
{
	if(codecContext->sample_rate!=0)
	{
		LOG(LOG_INFO,"AUDIO DEC: Audio sample rate " << codecContext->sample_rate);
		sampleRate=codecContext->sample_rate;
	}
	else
		return false;

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57,24,100)
	if(codecContext->ch_layout.nb_channels!=0)
	{
		LOG(LOG_INFO, "AUDIO DEC: Audio channels " << codecContext->ch_layout.nb_channels);
		channelCount=codecContext->ch_layout.nb_channels;
	}
#else
	if(codecContext->channels!=0)
	{
		LOG(LOG_INFO, "AUDIO DEC: Audio channels " << codecContext->channels);
		channelCount=codecContext->channels;
	}
#endif
	else
		return false;

	if(initialTime==(uint32_t)-1 && (!samplesBufferS16.isEmpty() || !samplesBufferF32.isEmpty()))
	{
		initialTime=getFrontTime();
		LOG(LOG_INFO,"AUDIO DEC: Initial timestamp " << initialTime);
	}
	else
		return false;

	return true;
}

uint32_t FFMpegAudioDecoder::decodeData(uint8_t* data, int32_t datalen, uint32_t time)
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	AVPacket* pkt = av_packet_alloc();
	if (!pkt)
		return 0;

	// If some data was left unprocessed on previous call,
	// concatenate.
	std::vector<uint8_t> combinedBuffer;
	if (overflowBuffer.empty())
	{
		pkt->data=data;
		pkt->size=datalen;
	}
	else
	{
		combinedBuffer.assign(overflowBuffer.begin(), overflowBuffer.end());
		if (datalen > 0)
			combinedBuffer.insert(combinedBuffer.end(), data, data+datalen);
		pkt->data = &combinedBuffer[0];
		pkt->size = combinedBuffer.size();
		overflowBuffer.clear();
	}
	av_frame_unref(frameIn);
	int ret = avcodec_send_packet(codecContext, pkt);
	int maxLen = 0;
	while (ret == 0)
	{
		
		ret = avcodec_receive_frame(codecContext,frameIn);
		
		if(ret != 0)
		{
			if (ret != AVERROR(EAGAIN))
				LOG(LOG_ERROR,"not decoded audio:"<<ret );
		}
		else
		{
			uint8_t* output=nullptr;
			int len;
			resampleFrame(&output,len);
#if ( LIBAVUTIL_VERSION_INT < AV_VERSION_INT(56,0,100) )
			maxLen = av_frame_get_pkt_size (frameIn);
#elif ( LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58,29,100) )
			maxLen = pkt->size;
#else
			maxLen = frameIn->pkt_size;
#endif
			if (engine->audio_useFloatSampleFormat())
			{
				int curpos = 0;
				while (len)
				{
					FrameSamplesF32& curTail=samplesBufferF32.acquireLast();
					int curbufsize= min(len,(int)sizeof(FrameSamplesF32::samples));
					memcpy(curTail.samples,output+curpos,curbufsize);
					curpos += curbufsize;
					curTail.len=curbufsize;
					assert(!(curTail.len&0x80000000));
					assert(curbufsize%2==0);
					curTail.current=curTail.samples;
					curTail.time=time;
					samplesBufferF32.commitLast();
					len -= curbufsize;
				}
			}
			else
			{
				int curpos = 0;
				while (len)
				{
					FrameSamplesS16& curTail=samplesBufferS16.acquireLast();
					int curbufsize= min(len,(int)sizeof(FrameSamplesS16::samples));
					memcpy(curTail.samples,output+curpos,curbufsize);
					curpos += curbufsize;
					curTail.len=curbufsize;
					assert(!(curTail.len&0x80000000));
					assert(curbufsize%2==0);
					curTail.current=curTail.samples;
					curTail.time=time;
					samplesBufferS16.commitLast();
					len -= curbufsize;
				}
			}
			if (output)
				av_freep(&output);
			if(status==INIT && fillDataAndCheckValidity())
				status=VALID;
		}
	}
	if (maxLen > 0)
	{
		int tmpsize = pkt->size - maxLen;
		uint8_t* tmpdata = pkt->data;
		tmpdata += maxLen;

		if (tmpsize > 0)
		{
			overflowBuffer.assign(tmpdata , tmpdata+tmpsize);
		}
	}
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,12,100)
	av_packet_unref(pkt);
#else
	av_free_packet(pkt);
#endif
	av_packet_free(&pkt);
	return maxLen;
#else
	uint8_t* output=nullptr;
	int len;
	resampleFrame(&output,len);
	if (engine->audio_useFloatSampleFormat())
	{
		FrameSamplesF32& curTail=samplesBufferF32.acquireLast();
		int maxLen=AVCODEC_MAX_AUDIO_FRAME_SIZE;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,23,0)
		AVPacket pkt;
		av_init_packet(&pkt);
		
		// If some data was left unprocessed on previous call,
		// concatenate.
		std::vector<uint8_t> combinedBuffer;
		if (overflowBuffer.empty())
		{
			pkt.data=data;
			pkt.size=datalen;
		}
		else
		{
			combinedBuffer.assign(overflowBuffer.begin(), overflowBuffer.end());
			if (datalen > 0)
				combinedBuffer.insert(combinedBuffer.end(), data, data+datalen);
			pkt.data = &combinedBuffer[0];
			pkt.size = combinedBuffer.size();
			overflowBuffer.clear();
		}
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
		av_frame_unref(frameIn);
		int frameOk=0;
		int32_t ret=avcodec_decode_audio4(codecContext, frameIn, &frameOk, &pkt);
		if(frameOk==0)
		{
			LOG(LOG_ERROR,"not decoded audio:"<<ret);
		}
		else
		{
			uint8_t* output=nullptr;
			int len;
			resampleFrame(&output,len);
			maxLen=len;
			int curpos = 0;
			while (len)
			{
				int curbufsize= min(len,(int)sizeof(FrameSamplesF32::samples));
				memcpy(curTail.samples,output+curpos,curbufsize);
				curpos += curbufsize;
				curTail.len=curbufsize;
				assert(!(curTail.len&0x80000000));
				assert(curbufsize%2==0);
				curTail.current=curTail.samples;
				curTail.time=time;
				samplesBufferF32.commitLast();
				len -= curbufsize;
			}
			if (output)
				av_freep(&output[0]);
		}
#else
		int32_t ret=avcodec_decode_audio3(codecContext, curTail.samples, &maxLen, &pkt);
#endif
		
		if (ret > 0)
		{
			pkt.data += ret;
			pkt.size -= ret;
			
			if (pkt.size > 0)
			{
				overflowBuffer.assign(pkt.data, pkt.data+pkt.size);
			}
		}
		
#else
		int32_t ret=avcodec_decode_audio2(codecContext, curTail.samples, &maxLen, data, datalen);
#endif
		
		curTail.len=maxLen;
		assert(!(curTail.len&0x80000000));
		assert(maxLen%2==0);
		curTail.current=curTail.samples;
		curTail.time=time;
		samplesBufferF32.commitLast();
	}
	else
	{
		FrameSamplesS16& curTail=samplesBufferS16.acquireLast();
		int maxLen=AVCODEC_MAX_AUDIO_FRAME_SIZE;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,23,0)
		AVPacket pkt;
		av_init_packet(&pkt);
		
		// If some data was left unprocessed on previous call,
		// concatenate.
		std::vector<uint8_t> combinedBuffer;
		if (overflowBuffer.empty())
		{
			pkt.data=data;
			pkt.size=datalen;
		}
		else
		{
			combinedBuffer.assign(overflowBuffer.begin(), overflowBuffer.end());
			if (datalen > 0)
				combinedBuffer.insert(combinedBuffer.end(), data, data+datalen);
			pkt.data = &combinedBuffer[0];
			pkt.size = combinedBuffer.size();
			overflowBuffer.clear();
		}
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
		av_frame_unref(frameIn);
		int frameOk=0;
		int32_t ret=avcodec_decode_audio4(codecContext, frameIn, &frameOk, &pkt);
		if(frameOk==0)
		{
			LOG(LOG_ERROR,"not decoded audio:"<<ret);
		}
		else
		{
			uint8_t* output=nullptr;
			int len;
			resampleFrame(&output,len);
			maxLen=len;
			int curpos = 0;
			while (len)
			{
				int curbufsize= min(len,(int)sizeof(FrameSamplesS16::samples));
				memcpy(curTail.samples,output+curpos,curbufsize);
				curpos += curbufsize;
				curTail.len=curbufsize;
				assert(!(curTail.len&0x80000000));
				assert(curbufsize%2==0);
				curTail.current=curTail.samples;
				curTail.time=time;
				samplesBufferS16.commitLast();
				len -= curbufsize;
			}
			if (output)
				av_freep(&output[0]);
		}
#else
		int32_t ret=avcodec_decode_audio3(codecContext, curTail.samples, &maxLen, &pkt);
#endif
		
		if (ret > 0)
		{
			pkt.data += ret;
			pkt.size -= ret;
			
			if (pkt.size > 0)
			{
				overflowBuffer.assign(pkt.data, pkt.data+pkt.size);
			}
		}
		
#else
		int32_t ret=avcodec_decode_audio2(codecContext, curTail.samples, &maxLen, data, datalen);
#endif
		
		curTail.len=maxLen;
		assert(!(curTail.len&0x80000000));
		assert(maxLen%2==0);
		curTail.current=curTail.samples;
		curTail.time=time;
		samplesBufferS16.commitLast();
	}
	if(status==INIT && fillDataAndCheckValidity())
		status=VALID;

	return maxLen;
#endif
}

void FFMpegAudioDecoder::decodePacket(AVPacket* pkt, uint32_t time)
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	av_frame_unref(frameIn);
	int ret = avcodec_send_packet(codecContext, pkt);
	while (ret == 0)
	{
		ret = avcodec_receive_frame(codecContext,frameIn);
		if(ret != 0)
		{
			if (ret != AVERROR(EAGAIN))
				LOG(LOG_ERROR,"not decoded audio:"<<ret);
		}
		else
		{
			uint8_t* output=nullptr;
			int len;
			resampleFrame(&output,len);
			if (engine->audio_useFloatSampleFormat())
			{
				int curpos = 0;
				while (len)
				{
					FrameSamplesF32& curTail=samplesBufferF32.acquireLast();
					int curbufsize= min(len,(int)sizeof(FrameSamplesF32::samples));
					memcpy(curTail.samples,output+curpos,curbufsize);
					curpos += curbufsize;
					curTail.len=curbufsize;
					assert(!(curTail.len&0x80000000));
					assert(curbufsize%2==0);
					curTail.current=curTail.samples;
					curTail.time=time;
					samplesBufferF32.commitLast();
					len -= curbufsize;
				}
			}
			else
			{
				int curpos = 0;
				while (len)
				{
					FrameSamplesS16& curTail=samplesBufferS16.acquireLast();
					int curbufsize= min(len,(int)sizeof(FrameSamplesS16::samples));
					memcpy(curTail.samples,output+curpos,curbufsize);
					curpos += curbufsize;
					curTail.len=curbufsize;
					assert(!(curTail.len&0x80000000));
					assert(curbufsize%2==0);
					curTail.current=curTail.samples;
					curTail.time=time;
					samplesBufferS16.commitLast();
					len -= curbufsize;
				}
			}
			if (output)
				av_freep(&output);
			if(status==INIT && fillDataAndCheckValidity())
				status=VALID;
		}
	}
#else
	if (engine->audio_useFloatSampleFormat())
	{
		FrameSamplesF32& curTail=samplesBufferF32.acquireLast();
		int maxLen=AVCODEC_MAX_AUDIO_FRAME_SIZE;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
		av_frame_unref(frameIn);
		int frameOk=0;
		int ret=avcodec_decode_audio4(codecContext, frameIn, &frameOk, pkt);
		if(frameOk==0)
		{
			LOG(LOG_ERROR,"not decoded audio:"<<ret);
		}
		else
		{
			uint8_t* output=nullptr;
			int len;
			resampleFrame(&output,len);
			maxLen=len;
			int curpos = 0;
			while (len)
			{
				int curbufsize= min(len,(int)sizeof(FrameSamplesF32::samples));
				memcpy(curTail.samples,output+curpos,curbufsize);
				curpos += curbufsize;
				curTail.len=curbufsize;
				assert(!(curTail.len&0x80000000));
				assert(curbufsize%2==0);
				curTail.current=curTail.samples;
				curTail.time=time;
				samplesBufferF32.commitLast();
				len -= curbufsize;
			}
			if (output)
				av_freep(&output[0]);
		}
#elif LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,23,0)
		int ret=avcodec_decode_audio3(codecContext, curTail.samples, &maxLen, pkt);
#else
		int ret=avcodec_decode_audio2(codecContext, curTail.samples, &maxLen, pkt->data, pkt->size);
#endif
		
		if(ret==-1)
		{
			//A decoding error occurred, create an empty sample buffer
			LOG(LOG_ERROR,"Malformed audio packet");
			curTail.len=0;
			curTail.current=curTail.samples;
			curTail.time=time;
			samplesBufferF32.commitLast();
			return maxLen;
		}
		
		assert_and_throw(ret==pkt->size);
		
		if(status==INIT && fillDataAndCheckValidity())
			status=VALID;
		
		curTail.len=maxLen;
		assert(!(curTail.len&0x80000000));
		assert(maxLen%2==0);
		curTail.current=curTail.samples;
		curTail.time=time;
		samplesBufferF32.commitLast();
	}
	else
	{
		FrameSamplesS16& curTail=samplesBufferS16.acquireLast();
		int maxLen=AVCODEC_MAX_AUDIO_FRAME_SIZE;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
		av_frame_unref(frameIn);
		int frameOk=0;
		int ret=avcodec_decode_audio4(codecContext, frameIn, &frameOk, pkt);
		if(frameOk==0)
		{
			LOG(LOG_ERROR,"not decoded audio:"<<ret);
		}
		else
		{
			uint8_t* output=nullptr;
			int len;
			resampleFrame(&output,len);
			maxLen=len;
			int curpos = 0;
			while (len)
			{
				int curbufsize= min(len,(int)sizeof(FrameSamplesS16::samples));
				memcpy(curTail.samples,output+curpos,curbufsize);
				curpos += curbufsize;
				curTail.len=curbufsize;
				assert(!(curTail.len&0x80000000));
				assert(curbufsize%2==0);
				curTail.current=curTail.samples;
				curTail.time=time;
				samplesBufferS16.commitLast();
				len -= curbufsize;
			}
			if (output)
				av_freep(&output[0]);
		}
#elif LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,23,0)
		int ret=avcodec_decode_audio3(codecContext, curTail.samples, &maxLen, pkt);
#else
		int ret=avcodec_decode_audio2(codecContext, curTail.samples, &maxLen, pkt->data, pkt->size);
#endif
		
		if(ret==-1)
		{
			//A decoding error occurred, create an empty sample buffer
			LOG(LOG_ERROR,"Malformed audio packet");
			curTail.len=0;
			curTail.current=curTail.samples;
			curTail.time=time;
			samplesBufferS16.commitLast();
			return maxLen;
		}
		
		assert_and_throw(ret==pkt->size);
		
		if(status==INIT && fillDataAndCheckValidity())
			status=VALID;
		
		curTail.len=maxLen;
		assert(!(curTail.len&0x80000000));
		assert(maxLen%2==0);
		curTail.current=curTail.samples;
		curTail.time=time;
		samplesBufferS16.commitLast();
	}
#endif
}
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
void FFMpegAudioDecoder::resampleFrame(uint8_t **output, int& outputsize)
{
	int sample_rate = engine->audio_getSampleRate();
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57,24,100)
	AVChannelLayout channel_layout = AV_CHANNEL_LAYOUT_STEREO;
#else
	unsigned int channel_layout = AV_CH_LAYOUT_STEREO;
#endif
#if ( LIBAVUTIL_VERSION_INT < AV_VERSION_INT(56,0,100) )
 	int framesamplerate = av_frame_get_sample_rate(frameIn);
#else
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,106,102)
	int framesamplerate = frameIn->sample_rate;
#else
	int framesamplerate = this->codecContext->sample_rate;
#endif
#endif
	AVSampleFormat outputsampleformat = forExtraction || engine->audio_useFloatSampleFormat() ? AV_SAMPLE_FMT_FLT : AV_SAMPLE_FMT_S16;
	int outputsampleformatsize = forExtraction || engine->audio_useFloatSampleFormat() ? sizeof(float) : sizeof(int16_t);
#ifdef HAVE_LIBSWRESAMPLE
	if (!resamplecontext)
	{
		resamplecontext = swr_alloc();
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57,24,100)
		av_opt_set_chlayout(resamplecontext, "in_chlayout",  &frameIn->ch_layout, 0);
		av_opt_set_chlayout(resamplecontext, "out_chlayout", &channel_layout,  0);
#else
		av_opt_set_int(resamplecontext, "in_channel_layout",  frameIn->channel_layout, 0);
		av_opt_set_int(resamplecontext, "out_channel_layout", channel_layout,  0);
#endif
		av_opt_set_int(resamplecontext, "in_sample_rate",     framesamplerate,     0);
		av_opt_set_int(resamplecontext, "out_sample_rate",    sample_rate,     0);
		av_opt_set_int(resamplecontext, "in_sample_fmt",      frameIn->format,   0);
		av_opt_set_int(resamplecontext, "out_sample_fmt",     outputsampleformat,    0);
		swr_init(resamplecontext);
	}

	int out_samples = swr_get_out_samples(resamplecontext,frameIn->nb_samples);
	int res = av_samples_alloc(output, nullptr, 2, out_samples,outputsampleformat, 0);

	if (res >= 0)
	{
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57,24,100)
		outputsize = swr_convert(resamplecontext, output, out_samples, (const uint8_t**)frameIn->extended_data, frameIn->nb_samples)*outputsampleformatsize*channel_layout.nb_channels;
#else
		outputsize = swr_convert(resamplecontext, output, out_samples, (const uint8_t**)frameIn->extended_data, frameIn->nb_samples)*outputsampleformatsize*av_get_channel_layout_nb_channels(channel_layout);
#endif
		if (outputsize <= 0)
		{
			LOG(LOG_ERROR, "resampling failed");
			outputsize=0;
		}
	}
	else
	{
		LOG(LOG_ERROR, "resampling failed, error code:"<<res);
		outputsize=0;
	}
#elif defined HAVE_LIBAVRESAMPLE
	if (!resamplecontext)
	{
		resamplecontext = avresample_alloc_context();
		av_opt_set_int(resamplecontext, "in_channel_layout",  frameIn->channel_layout, 0);
		av_opt_set_int(resamplecontext, "out_channel_layout", channel_layout,  0);
		av_opt_set_int(resamplecontext, "in_sample_rate",     framesamplerate,     0);
		av_opt_set_int(resamplecontext, "out_sample_rate",    sample_rate,     0);
		av_opt_set_int(resamplecontext, "in_sample_fmt",      frameIn->format,   0);
		av_opt_set_int(resamplecontext, "out_sample_fmt",     outputsampleformat,    0);
		avresample_open(resamplecontext);
	}

	int out_linesize;
	int out_samples = avresample_available(resamplecontext) + av_rescale_rnd(avresample_get_delay(resamplecontext) + frameIn->linesize[0], sample_rate, sample_rate, AV_ROUND_UP);
	int res = av_samples_alloc(output, &out_linesize, frameIn->nb_samples, out_samples, outputsampleformat, 0);
	if (res >= 0)
	{
		outputsize = avresample_convert(resamplecontext, output, out_linesize, out_samples, frameIn->extended_data, frameIn->linesize[0], frameIn->nb_samples)*outputsampleformatsize*av_get_channel_layout_nb_channels(channel_layout);
	}
	else
	{
		LOG(LOG_ERROR, "resampling failed, error code:"<<res);
		outputsize=0;
	}
#else
	LOG(LOG_ERROR, "unexpected sample format and can't resample, recompile with libswresample");
	outputsize=0;
#endif
}

#endif

#endif //ENABLE_LIBAVCODEC

StreamDecoder::~StreamDecoder()
{
	delete audioDecoder;
	delete videoDecoder;
}

#ifdef ENABLE_LIBAVCODEC
FFMpegStreamDecoder::FFMpegStreamDecoder(NetStream *ns, EngineData *eng, std::istream& s, uint32_t buffertime, AudioFormat* format, int streamsize, bool forExtraction)
 : netstream(ns),audioFound(false),videoFound(false),stream(s),formatCtx(nullptr),audioIndex(-1),
   videoIndex(-1),customAudioDecoder(nullptr),customVideoDecoder(nullptr),avioContext(nullptr),availablestreamlength(streamsize),fullstreamlength(streamsize)
{
	int aviobufsize = streamsize == -1 ? 4096 : min(4096, streamsize);
	valid=false;
	avioBuffer = (uint8_t*)av_malloc(aviobufsize);
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 105, 0)
	avioContext=avio_alloc_context(avioBuffer,aviobufsize,0,this,avioReadPacket,nullptr,streamsize < 0 ? nullptr : avioSeek);
#else
	avioContext=av_alloc_put_byte(avioBuffer,aviobufsize,0,this,avioReadPacket,nullptr,nullptr);
#endif
	if(avioContext==nullptr)
		return;

#if LIBAVFORMAT_VERSION_MAJOR > 52 || (LIBAVFORMAT_VERSION_MAJOR == 52  && LIBAVFORMAT_VERSION_MINOR > 64)
 	avioContext->seekable = 0;
#else
	avioContext->is_streamed=1;
#endif

#if LIBAVFORMAT_VERSION_MAJOR > 58
	const AVInputFormat* fmt = nullptr;
#else
	AVInputFormat* fmt = nullptr;
#endif
	if (format)
	{
		switch (format->codec)
		{
			case LS_AUDIO_CODEC::MP3:
				fmt = av_find_input_format("mp3");
				break;
			case LS_AUDIO_CODEC::AAC:
				fmt = av_find_input_format("aac");
				break;
			case LS_AUDIO_CODEC::LINEAR_PCM_PLATFORM_ENDIAN:
#if __BYTE_ORDER == __BIG_ENDIAN
				fmt = av_find_input_format("s16be");
#else
				fmt = av_find_input_format("s16le");
#endif
				break;
			case LS_AUDIO_CODEC::LINEAR_PCM_LE:
				fmt = av_find_input_format("s16le");
				break;
			case LS_AUDIO_CODEC::LINEAR_PCM_FLOAT_PLATFORM_ENDIAN:
#if __BYTE_ORDER == __BIG_ENDIAN
				fmt = av_find_input_format("f32be");
#else
				fmt = av_find_input_format("f32le");
#endif
				break;
			case LS_AUDIO_CODEC::ADPCM:
				fmt = av_find_input_format("flv");
				format=nullptr;
				break;
			case LS_AUDIO_CODEC::CODEC_NONE:
				break;
		}
	}
	if (fmt == nullptr)
	{
		//Probe the stream format.
		//NOTE: in FFMpeg 0.7 there is av_probe_input_buffer
		AVProbeData probeData;
		probeData.filename="lightspark_stream";
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		probeData.mime_type=nullptr;
#endif
		probeData.buf=new uint8_t[8192+AVPROBE_PADDING_SIZE];
		memset(probeData.buf,0,8192+AVPROBE_PADDING_SIZE);
		int readcount=streamsize == -1 ? 8192 : min(8192,streamsize);
		stream.read((char*)probeData.buf,readcount);
		int read=stream.gcount();
		if(read!=readcount)
			LOG(LOG_ERROR,"Not sufficient data is available from the stream:"<<read);
		probeData.buf_size=read;

		stream.seekg(0);
		fmt=av_probe_input_format(&probeData,1);
		delete[] probeData.buf;
	}
	if (fmt == nullptr)
		return;

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 105, 0)
	formatCtx=avformat_alloc_context();
	formatCtx->pb = avioContext;
	int ret=avformat_open_input(&formatCtx, "lightspark_stream", fmt, nullptr);
#else
	int ret=av_open_input_stream(&formatCtx, avioContext, "lightspark_stream", fmt, nullptr);
#endif
	if(ret<0)
	{
		char buf[1000];
		av_strerror(ret, buf, 1000);
		LOG(LOG_ERROR,"could not open ffmpeg stream:"<<buf);
		return;
	}
	if (!format)
	{
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53, 6, 0)
		ret=avformat_find_stream_info(formatCtx,nullptr);
#else
		ret=av_find_stream_info(formatCtx);
#endif
	}
	if(ret<0)
		return;

	LOG_CALL("FFMpeg found " << formatCtx->nb_streams << " streams");
	for(uint32_t i=0;i<formatCtx->nb_streams;i++)
	{
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		if(formatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO && videoFound==false)
#else
		if(formatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO && videoFound==false)
#endif
		{
			videoFound=true;
			videoIndex=(int32_t)i;
		}
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		else if(formatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO && formatCtx->streams[i]->codecpar->codec_id!=CODEC_ID_NONE && audioFound==false)
#else
		else if(formatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO && formatCtx->streams[i]->codec->codec_id!=CODEC_ID_NONE && audioFound==false)
#endif
		{
			audioFound=true;
			audioIndex=(int32_t)i;
		}

	}
	if(videoFound)
	{
		//Pass the frame rate from the container, the once from the codec is often wrong
		AVStream *stream = formatCtx->streams[videoIndex];
		if (stream->nb_frames > 1 && stream->codecpar->codec_id==CODEC_ID_GIF)
			LOG(LOG_NOT_IMPLEMENTED,"GIF with multiple frames is not properly handled yet");
#if LIBAVUTIL_VERSION_MAJOR < 54
		AVRational rateRational = stream->r_frame_rate;
#else
		AVRational rateRational = stream->avg_frame_rate;
#endif
		double frameRate=av_q2d(rateRational);
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		customVideoDecoder=new FFMpegVideoDecoder(formatCtx->streams[videoIndex]->codecpar,frameRate);
#else
		customVideoDecoder=new FFMpegVideoDecoder(formatCtx->streams[videoIndex]->codec,frameRate);
#endif
		videoDecoder=customVideoDecoder;
	}

	if(audioFound)
	{
		if (format && (format->codec != LS_AUDIO_CODEC::CODEC_NONE))
			customAudioDecoder=new FFMpegAudioDecoder(eng,format->codec,format->sampleRate,format->channels,buffertime,true);
		else
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
			customAudioDecoder=new FFMpegAudioDecoder(eng,formatCtx->streams[audioIndex]->codecpar,customVideoDecoder ? FFMPEGVIDEODECODERBUFFERSIZE*buffertime : buffertime);
#else
			customAudioDecoder=new FFMpegAudioDecoder(eng,formatCtx->streams[audioIndex]->codec,buffertime);
#endif
		audioDecoder=customAudioDecoder;
		audioDecoder->forExtraction=forExtraction;
	}
	if (netstream && formatCtx->duration)
	{
		std::list<asAtom> dataobjectlist;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=netstream->getSystemState()->getUniqueStringId("duration");
		m.isAttribute = false;
		ASObject* dataobj = Class<ASObject>::getInstanceS(netstream->getInstanceWorker());
		asAtom v = asAtomHandler::fromInt(formatCtx->duration/AV_TIME_BASE);
		dataobj->setVariableByMultiname(m,v,ASObject::CONST_NOT_ALLOWED,nullptr,netstream->getInstanceWorker());
		dataobjectlist.push_back(asAtomHandler::fromObjectNoPrimitive(dataobj));
		netstream->sendClientNotification("onMetaData",dataobjectlist);
	}
	valid=true;
}

FFMpegStreamDecoder::~FFMpegStreamDecoder()
{
	//Delete the decoders before deleting the input stream to avoid a crash in ffmpeg code
	delete audioDecoder;
	delete videoDecoder;
	audioDecoder=nullptr;
	videoDecoder=nullptr;
	customAudioDecoder=nullptr;
	customVideoDecoder=nullptr;
	if(formatCtx)
	{
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 105, 0)
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53, 25, 0)
		avformat_close_input(&formatCtx);
#else
		av_close_input_file(formatCtx);
#endif
#else
		av_close_input_stream(formatCtx);
#endif
	}
	if(avioContext)
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 80, 100)
		avio_context_free(&avioContext);
#else
		av_free(avioContext);
#endif
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(52, 96, 0)
	avformat_free_context(formatCtx);
#endif
}

void FFMpegStreamDecoder::jumpToPosition(number_t position)
{
	int64_t pos = (position* AV_TIME_BASE) / 1000;
	av_seek_frame(formatCtx,-1,pos,0);
}

bool FFMpegStreamDecoder::decodeNextFrame()
{
	AVPacket pkt;
	int ret=av_read_frame(formatCtx, &pkt);
	if(ret<0)
		return false;
	auto time_base=formatCtx->streams[pkt.stream_index]->time_base;
	//Should use dts
	uint32_t mtime=pkt.dts*1000*(time_base.den ? (number_t)time_base.num/(number_t)time_base.den : (number_t)time_base.num);
	if (pkt.stream_index==(int)audioIndex)
	{
		if (customAudioDecoder)
			customAudioDecoder->decodePacket(&pkt, mtime);
	}
	else 
	{
		if (customVideoDecoder)
		{
			if (customVideoDecoder->decodePacket(&pkt, mtime))
			{
				customVideoDecoder->framesdecoded++;
				hasvideo=true;
			}
		}
	}
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,12,100)
	av_packet_unref(&pkt);
#else
	av_free_packet(&pkt);
#endif
	return true;
}

int FFMpegStreamDecoder::getAudioSampleRate()
{
	if (customAudioDecoder)
		return customAudioDecoder->sampleRate;
	return 0;
}

int FFMpegStreamDecoder::avioReadPacket(void* t, uint8_t* buf, int buf_size)
{
	FFMpegStreamDecoder* th=static_cast<FFMpegStreamDecoder*>(t);
	// check for available bytes to avoid exception on eof
	if (th->availablestreamlength == 0)
		return AVERROR_EOF;
	th->stream.read((char*)buf,th->availablestreamlength == -1 ? buf_size : min(buf_size,th->availablestreamlength));
	int ret=th->stream.gcount();
	if (th->availablestreamlength != -1)
		th->availablestreamlength -= ret;
	return ret;
}

int64_t FFMpegStreamDecoder::avioSeek(void* opaque, int64_t offset, int whence)
{
	FFMpegStreamDecoder* th=static_cast<FFMpegStreamDecoder*>(opaque);
	switch (whence)
	{
		case SEEK_SET:
			th->stream.seekg(offset,ios_base::beg);
			th->availablestreamlength = th->fullstreamlength-offset;
			return th->stream.tellg();
		case SEEK_CUR:
			th->availablestreamlength = th->stream.tellg()+offset;
			th->stream.seekg(offset,ios_base::cur);
			return th->stream.tellg();
		case SEEK_END:
			th->stream.seekg(offset,ios_base::end);
			th->availablestreamlength = -offset;
			return th->stream.tellg();
		case AVSEEK_SIZE:
			return th->fullstreamlength;
	}
	return -1;
}
#endif //ENABLE_LIBAVCODEC

void SampleDataAudioDecoder::samplesconsumed(uint32_t samples)
{
	bufferedsamples -= samples;
	soundchannel->semSampleData.signal();
}

uint32_t SampleDataAudioDecoder::decodeData(uint8_t* data, int32_t datalen, uint32_t time)
{
	if (status == PREINIT)
		status = INIT;
	else if (status == INIT)
		status = VALID;
	if (engine->audio_useFloatSampleFormat())
	{
		// no need for conversion as Sample data is always 32 bit float
		FrameSamplesF32& curTail=samplesBufferF32.acquireLast();
		memcpy(curTail.samples, data, datalen);
		curTail.len=datalen;
		curTail.current=curTail.samples;
		curTail.time=time;
		samplesBufferF32.commitLast();
		bufferedsamples += datalen/4;
		return datalen;
	}
	FrameSamplesS16& curTail=samplesBufferS16.acquireLast();
	uint32_t samplecount = min(datalen/4,MAX_AUDIO_FRAME_SIZE/2);
#ifdef HAVE_LIBSWRESAMPLE
	int sample_rate = engine->audio_getSampleRate();
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57,24,100)
	AVChannelLayout channel_layout = AV_CHANNEL_LAYOUT_STEREO;
#else
	unsigned int channel_layout = AV_CH_LAYOUT_STEREO;
#endif
	int framesamplerate = 44100;
	AVSampleFormat outputsampleformat =  AV_SAMPLE_FMT_S16;
	int outputsampleformatsize = sizeof(int16_t);
	if (!resamplecontext)
	{
		resamplecontext = swr_alloc();
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57,24,100)
		av_opt_set_chlayout(resamplecontext, "in_chlayout",  &channel_layout, 0);
		av_opt_set_chlayout(resamplecontext, "out_chlayout", &channel_layout,  0);
#else
		av_opt_set_int(resamplecontext, "in_channel_layout",  channel_layout, 0);
		av_opt_set_int(resamplecontext, "out_channel_layout", channel_layout,  0);
#endif
		av_opt_set_int(resamplecontext, "in_sample_rate",     framesamplerate,     0);
		av_opt_set_int(resamplecontext, "out_sample_rate",    sample_rate,     0);
		av_opt_set_int(resamplecontext, "in_sample_fmt",      AV_SAMPLE_FMT_FLT,   0);
		av_opt_set_int(resamplecontext, "out_sample_fmt",     outputsampleformat,    0);
		swr_init(resamplecontext);
	}

	uint8_t *output;
	int out_samples = swr_get_out_samples(resamplecontext,samplecount);
	int res = av_samples_alloc(&output, nullptr, 2, out_samples,outputsampleformat, 0);

	if (res >= 0)
	{
		int maxLen = swr_convert(resamplecontext, &output, out_samples, (const uint8_t**)&data, samplecount)*outputsampleformatsize*2;
		if (maxLen > 0)
		{
			memcpy(curTail.samples, output, maxLen);
		}
		else
		{
			LOG(LOG_ERROR, "sampledata resampling failed");
			memset(curTail.samples, 0, curTail.len);
		}
		av_freep(&output);
	}
	else
	{
		LOG(LOG_ERROR, "sampledata resampling failed, error code:"<<res);
		memset(curTail.samples, 0, curTail.len);
	}
#elif defined HAVE_LIBAVRESAMPLE
	if (!resamplecontext)
	{
		resamplecontext = avresample_alloc_context();
		av_opt_set_int(resamplecontext, "in_channel_layout",  channel_layout, 0);
		av_opt_set_int(resamplecontext, "out_channel_layout", channel_layout,  0);
		av_opt_set_int(resamplecontext, "in_sample_rate",     framesamplerate,     0);
		av_opt_set_int(resamplecontext, "out_sample_rate",    sample_rate,     0);
		av_opt_set_int(resamplecontext, "in_sample_fmt",      AV_SAMPLE_FMT_FLT,   0);
		av_opt_set_int(resamplecontext, "out_sample_fmt",     outputsampleformat,    0);
		avresample_open(resamplecontext);
	}

	uint8_t *output;
	int out_linesize;
	int out_samples = avresample_available(resamplecontext) + av_rescale_rnd(avresample_get_delay(resamplecontext) + datalen, sample_rate, sample_rate, AV_ROUND_UP);
	int res = av_samples_alloc(&output, &out_linesize, samplecount, out_samples, outputsampleformat, 0);
	if (res >= 0)
	{
		int maxLen = avresample_convert(resamplecontext, &output, out_linesize, out_samples, data, datalen, samplecount)*outputsampleformatsize*av_get_channel_layout_nb_channels(channel_layout);
		memcpy(curTail.samples, output, maxLen);
		av_freep(&output);
	}
	else
	{
		LOG(LOG_ERROR, "sampledata resampling failed, error code:"<<res);
		memset(curTail.samples, 0, curTail.len);
	}
#else
	for (uint32_t i = 0; i < samplecount; i++)
	{
		union
		{
			uint32_t u;
			float f;
		} res;
		memcpy(&res.u,data+i*4,4);
		int16_t v;
		if (res.f > 1.0 )
			curTail.samples[i] = INT16_MAX;
		else if (res.f < -1.0)
			curTail.samples[i] = INT16_MIN;
		else
			curTail.samples[i] = (int16_t(int((res.f * 32768.0f)+ 32768.5)-32768));
		if (res.f > 1.0 || res.f < -1.0)
		{
			LOG(LOG_ERROR,"decodedata:"<<res.f<<" "<<(res.f * 32768.0f)<<curTail.samples[i]);
		}
	}
#endif
	curTail.len=samplecount*2;
	curTail.current=curTail.samples;
	curTail.time=time;
	samplesBufferS16.commitLast();
	bufferedsamples += samplecount;
	return samplecount*2;
}
