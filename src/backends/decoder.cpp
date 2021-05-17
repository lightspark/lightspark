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

#include "backends/audio.h"
#include "backends/decoder.h"
#include "platforms/fastpaths.h"
#include "swf.h"
#include "backends/rendering.h"
#include "SDL2/SDL_mixer.h"
#include "scripting/class.h"
#include "scripting/flash/net/flashnet.h"
#include "parsing/tags.h"

#if LIBAVUTIL_VERSION_MAJOR < 51
#define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
#define AVMEDIA_TYPE_AUDIO CODEC_TYPE_AUDIO
#endif

#ifndef HAVE_AV_FRAME_ALLOC
#define av_frame_alloc avcodec_alloc_frame
#endif

#ifndef HAVE_AV_FRAME_UNREF
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
		LOG(LOG_INFO,_("VIDEO DEC: Video frame size ") << frameWidth << 'x' << frameHeight);
		resizeGLBuffers=true;
		videoTexture=getSys()->getRenderThread()->allocateTexture(frameWidth, frameHeight, true);
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

const TextureChunk& VideoDecoder::getTexture()
{
	return videoTexture;
}

void VideoDecoder::uploadFence()
{
	assert(fenceCount);
	ATOMIC_DECREMENT(fenceCount);
	if (markedForDeletion && fenceCount==0)
		delete this;
}

void VideoDecoder::markForDestruction()
{
	markedForDeletion=true;
}
VideoDecoder::VideoDecoder():frameRate(0),framesdecoded(0),framesdropped(0),frameWidth(0),frameHeight(0),lastframe(UINT32_MAX),currentframe(UINT32_MAX),fenceCount(0),resizeGLBuffers(false),markedForDeletion(false)
{
}

VideoDecoder::~VideoDecoder()
{
	if (videoTexture.isValid())
		getSys()->getRenderThread()->releaseTexture(getTexture());
}

void VideoDecoder::waitForFencing()
{
	ATOMIC_INCREMENT(fenceCount);
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
	ownedContext(true),curBuffer(0),codecContext(nullptr),curBufferOffset(0),currentcachedframe(UINT32_MAX),embeddedvideotag(tag)
{
	//The tag is the header, initialize decoding
	switchCodec(codecId, initdata, datalen, frameRateHint);

	frameIn=av_frame_alloc();
}

void FFMpegVideoDecoder::switchCodec(LS_VIDEO_CODEC codecId, uint8_t *initdata, uint32_t datalen, double frameRateHint)
{
	if (codecContext)
	{
		avcodec_close(codecContext);
		if(ownedContext)
			av_free(codecContext);
	}
#ifdef HAVE_AVCODEC_ALLOC_CONTEXT3
	codecContext=avcodec_alloc_context3(NULL);
#else
	codecContext=avcodec_alloc_context();
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3
	AVCodec* codec=NULL;
	videoCodec=codecId;
	if(codecId==H264)
	{
		//TODO: serialize access to avcodec_open
		const enum CodecID FFMPEGcodecId=CODEC_ID_H264;
		codec=avcodec_find_decoder(FFMPEGcodecId);
		assert(codec);
		//Ignore the frameRateHint as the rate is gathered from the video data
	}
	else if(codecId==H263)
	{
		//TODO: serialize access to avcodec_open
		const enum CodecID FFMPEGcodecId=CODEC_ID_FLV1;
		codec=avcodec_find_decoder(FFMPEGcodecId);
		assert(codec);

		//Exploit the frame rate information
		assert(frameRateHint!=0.0);
		frameRate=frameRateHint;
	}
	else if(codecId==VP6)
	{
		//TODO: serialize access to avcodec_open
		const enum CodecID FFMPEGcodecId=CODEC_ID_VP6F;
		codec=avcodec_find_decoder(FFMPEGcodecId);
		assert(codec);

		//Exploit the frame rate information
		assert(frameRateHint!=0.0);
		frameRate=frameRateHint;
	}
	else if(codecId==VP6A)
	{
		//TODO: serialize access to avcodec_open
		const enum CodecID FFMPEGcodecId=CODEC_ID_VP6A;
		codec=avcodec_find_decoder(FFMPEGcodecId);
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
#ifdef HAVE_AVCODEC_OPEN2
	if(avcodec_open2(codecContext, codec, NULL)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3
		throw RunTimeException("Cannot open decoder");

	if(fillDataAndCheckValidity())
		status=VALID;
	else
		status=INIT;
}
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
FFMpegVideoDecoder::FFMpegVideoDecoder(AVCodecParameters* codecPar, double frameRateHint):
	ownedContext(true),curBuffer(0),codecContext(NULL),curBufferOffset(0),currentcachedframe(UINT32_MAX),embeddedvideotag(nullptr)
{
	status=INIT;
#ifdef HAVE_AVCODEC_ALLOC_CONTEXT3
	codecContext=avcodec_alloc_context3(NULL);
#else
	codecContext=avcodec_alloc_context();
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3
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
		default:
			return;
	}
	avcodec_parameters_to_context(codecContext,codecPar);
	AVCodec* codec=avcodec_find_decoder(codecPar->codec_id);
#ifdef HAVE_AVCODEC_OPEN2
	if(avcodec_open2(codecContext, codec, NULL)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3
		return;

	frameRate=frameRateHint;

	if(fillDataAndCheckValidity())
		status=VALID;

}
#else
FFMpegVideoDecoder::FFMpegVideoDecoder(AVCodecContext* _c, double frameRateHint):
	ownedContext(false),curBuffer(0),codecContext(_c),curBufferOffset(0),currentcachedframe(UINT32_MAX),embeddedvideotag(nullptr)
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
	AVCodec* codec=avcodec_find_decoder(codecContext->codec_id);
#ifdef HAVE_AVCODEC_OPEN2
	if(avcodec_open2(codecContext, codec, NULL)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3
		return;

	frameRate=frameRateHint;

	if(fillDataAndCheckValidity())
		status=VALID;

}
#endif

FFMpegVideoDecoder::~FFMpegVideoDecoder()
{
	while(fenceCount);
	avcodec_close(codecContext);
	if(ownedContext)
		av_free(codecContext);
	av_free(frameIn);
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
			embeddedbuffers.regen(YUVBufferGenerator(bufferSize,this->codecContext->pix_fmt==AV_PIX_FMT_YUVA420P));
		else
			streamingbuffers.regen(YUVBufferGenerator(bufferSize,this->codecContext->pix_fmt==AV_PIX_FMT_YUVA420P));
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
	Locker locker(mutex);
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
	Locker locker(mutex);
	if(datalen==0)
		return false;
#if defined HAVE_AVCODEC_SEND_PACKET && defined HAVE_AVCODEC_RECEIVE_FRAME
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data=data;
	pkt.size=datalen;
	int ret = avcodec_send_packet(codecContext, &pkt);
	while (ret == 0)
	{
		ret = avcodec_receive_frame(codecContext,frameIn);
		if (ret != 0)
		{
			if (ret != AVERROR(EAGAIN))
			{
				LOG(LOG_INFO,"not decoded:"<<ret);
#ifdef HAVE_AV_PACKET_UNREF
				av_packet_unref(&pkt);
#else
				av_free_packet(&pkt);
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
#ifdef HAVE_AV_PACKET_UNREF
	av_packet_unref(&pkt);
#else
	av_free_packet(&pkt);
#endif
#else
	int frameOk=0;
#if HAVE_AVCODEC_DECODE_VIDEO2
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
#if defined HAVE_AVCODEC_SEND_PACKET && defined HAVE_AVCODEC_RECEIVE_FRAME
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

#if HAVE_AVCODEC_DECODE_VIDEO2
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
	for(uint32_t y=0;y<frameHeight;y++)
	{
		memcpy(curTail->ch[0]+offset[0],frameIn->data[0]+(y*frameIn->linesize[0]),frameWidth);
		if (codecContext->pix_fmt==AV_PIX_FMT_YUVA420P)
			memcpy(curTail->ch[3]+offset[0],frameIn->data[3]+(y*frameIn->linesize[0]),frameWidth);
		offset[0]+=frameWidth;
	}
	for(uint32_t y=0;y<frameHeight/2;y++)
	{
		memcpy(curTail->ch[1]+offset[1],frameIn->data[1]+(y*frameIn->linesize[1]),frameWidth/2);
		memcpy(curTail->ch[2]+offset[2],frameIn->data[2]+(y*frameIn->linesize[2]),frameWidth/2);
		offset[1]+=frameWidth/2;
		offset[2]+=frameWidth/2;
	}
	curTail->time=time;
	if (embeddedvideotag)
		embeddedbuffers.commitLast();
	else
		streamingbuffers.commitLast();
}

void FFMpegVideoDecoder::upload(uint8_t* data, uint32_t w, uint32_t h)
{
	Locker l(mutex);
	//Verify that the size are right
	assert_and_throw(w==((frameWidth+15)&0xfffffff0) && h==frameHeight);
	if (embeddedvideotag) // on embedded video we decode the frames during upload
	{
		if (currentframe != UINT32_MAX)
		{
			skipAll();
			for (uint32_t i = lastframe+1; i <currentframe; i++)
			{
				VideoFrameTag* t = embeddedvideotag->getFrame(i);
				if (t)
					decodeData(t->getData(),t->getNumBytes(),UINT32_MAX);
			}
			decodeData(embeddedvideotag->getFrame(currentframe)->getData(),embeddedvideotag->getFrame(currentframe)->getNumBytes(), 0);
			lastframe=currentframe;
		}
		else
			currentframe=0;
		if(embeddedbuffers.isEmpty())
			return;

	}
	else
	{
		if(streamingbuffers.isEmpty())
			return;
	}
	
	//At least a frame is available
	YUVBuffer* cur=embeddedvideotag ? &embeddedbuffers.front() : &streamingbuffers.front();
	fastYUV420ChannelsToYUV0Buffer(cur->ch[0],cur->ch[1],cur->ch[2],data,frameWidth,frameHeight);
	if (codecContext->pix_fmt==AV_PIX_FMT_YUVA420P)
	{
		uint32_t texw= (frameWidth+15)&0xfffffff0;
		for(uint32_t i=0;i<frameHeight;i++)
		{
			for(uint32_t j=0;j<frameWidth;j++)
			{
				uint32_t pixelCoordFull=i*texw+j;
				data[pixelCoordFull*4+3]=cur->ch[3][i*frameWidth+j];
			}
		}
	}
}

void FFMpegVideoDecoder::YUVBufferGenerator::init(YUVBuffer& buf) const
{
	if(buf.ch[0])
	{
		aligned_free(buf.ch[0]);
		aligned_free(buf.ch[1]);
		aligned_free(buf.ch[2]);
		if (buf.ch[3])
			aligned_free(buf.ch[3]);
	}
	aligned_malloc((void**)&buf.ch[0], 16, bufferSize);
	aligned_malloc((void**)&buf.ch[1], 16, bufferSize/4);
	aligned_malloc((void**)&buf.ch[2], 16, bufferSize/4);
	if (hasAlpha)
		aligned_malloc((void**)&buf.ch[3], 16, bufferSize);
}
#endif //ENABLE_LIBAVCODEC

void* AudioDecoder::operator new(size_t s)
{
	void* retAddr;
	aligned_malloc(&retAddr, 16, s);
	return retAddr;
}
void AudioDecoder::operator delete(void* addr)
{
	aligned_free(addr);
}

bool AudioDecoder::discardFrame()
{
	//We don't want ot block if no frame is available
	bool ret=samplesBuffer.nonBlockingPopFront();
	if(flushing && samplesBuffer.isEmpty()) //End of our work
	{
		status=FLUSHED;
		flushed.signal();
	}
	return ret;
}

uint32_t AudioDecoder::copyFrame(int16_t* dest, uint32_t len)
{
	assert(dest);
	if(samplesBuffer.isEmpty())
		return 0;
	uint32_t frameSize=min(samplesBuffer.front().len,len);
	memcpy(dest,samplesBuffer.front().current,frameSize);
	samplesBuffer.front().len-=frameSize;
	assert(!(samplesBuffer.front().len&0x80000000));
	if(samplesBuffer.front().len==0)
	{
		samplesBuffer.nonBlockingPopFront();
		if(flushing && samplesBuffer.isEmpty()) //End of our work
		{
			status=FLUSHED;
			flushed.signal();
		}
	}
	else
	{
		samplesBuffer.front().current+=frameSize/2;
		samplesBuffer.front().time+=frameSize/getBytesPerMSec();
	}
	return frameSize;
}

uint32_t AudioDecoder::getFrontTime() const
{
	assert(!samplesBuffer.isEmpty());
	return samplesBuffer.front().time;
}

void AudioDecoder::skipUntil(uint32_t time, uint32_t usecs)
{
	assert(isValid());
//	while(1) //Should loop, but currently only usec adjustements are requested
	{
		if(samplesBuffer.isEmpty())
			return;
		FrameSamples& cur=samplesBuffer.front();
		assert(time==cur.time);
		if(usecs==0) //Nothing to skip
			return;
		//Check how many bytes are needed to fill the gap
		uint32_t bytesToDiscard=(time-cur.time)*getBytesPerMSec()+usecs*getBytesPerMSec()/1000;
		bytesToDiscard&=0xfffffffe;

		if(cur.len<=bytesToDiscard) //The whole frame is droppable
			discardFrame();
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

void AudioDecoder::skipAll()
{
	while(!samplesBuffer.isEmpty())
		discardFrame();
}

#ifdef ENABLE_LIBAVCODEC
FFMpegAudioDecoder::FFMpegAudioDecoder(EngineData* eng,LS_AUDIO_CODEC audioCodec, uint8_t* initdata, uint32_t datalen):engine(eng),ownedContext(true)
#if defined HAVE_LIBAVRESAMPLE || defined HAVE_LIBSWRESAMPLE
	,resamplecontext(NULL)
#endif
{
	switchCodec(audioCodec,initdata,datalen);
#if HAVE_AVCODEC_DECODE_AUDIO4
	frameIn=av_frame_alloc();
#endif
}
void FFMpegAudioDecoder::switchCodec(LS_AUDIO_CODEC audioCodec, uint8_t* initdata, uint32_t datalen)
{
	if (codecContext)
		avcodec_close(codecContext);
#ifdef HAVE_LIBSWRESAMPLE
	if (resamplecontext)
		swr_free(&resamplecontext);
	resamplecontext=nullptr;
#elif defined HAVE_LIBAVRESAMPLE
	if (resamplecontext)
		avresample_free(&resamplecontext);
#endif
	AVCodec* codec=avcodec_find_decoder(LSToFFMpegCodec(audioCodec));
	assert(codec);

#ifdef HAVE_AVCODEC_ALLOC_CONTEXT3
	codecContext=avcodec_alloc_context3(NULL);
#else
	codecContext=avcodec_alloc_context();
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3

	if(initdata)
	{
		codecContext->extradata=initdata;
		codecContext->extradata_size=datalen;
	}

#ifdef HAVE_AVCODEC_OPEN2
	if(avcodec_open2(codecContext, codec, NULL)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3
		throw RunTimeException("Cannot open decoder");

	if(fillDataAndCheckValidity())
		status=VALID;
	else
		status=INIT;
}

FFMpegAudioDecoder::FFMpegAudioDecoder(EngineData* eng,LS_AUDIO_CODEC lscodec, int sampleRate, int channels, bool):engine(eng),ownedContext(true)
#if defined HAVE_LIBAVRESAMPLE || defined HAVE_LIBSWRESAMPLE
	,resamplecontext(NULL)
#endif
{
	status=INIT;

	CodecID codecId = LSToFFMpegCodec(lscodec);
	AVCodec* codec=avcodec_find_decoder(codecId);
	assert(codec);
	codecContext=avcodec_alloc_context3(codec);
	codecContext->codec_id = codecId;
	codecContext->sample_rate = sampleRate;
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
#ifdef HAVE_AVCODEC_OPEN2
	if(avcodec_open2(codecContext, codec, NULL)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3
		return;

	if(fillDataAndCheckValidity())
		status=VALID;
#ifdef HAVE_AVCODEC_DECODE_AUDIO4
	frameIn=av_frame_alloc();
#endif
}

#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
FFMpegAudioDecoder::FFMpegAudioDecoder(EngineData* eng,AVCodecParameters* codecPar):engine(eng),ownedContext(true),codecContext(NULL)
#if defined HAVE_LIBAVRESAMPLE || defined HAVE_LIBSWRESAMPLE
	,resamplecontext(NULL)
#endif
{
	status=INIT;
	AVCodec* codec=avcodec_find_decoder(codecPar->codec_id);
	assert(codec);
#ifdef HAVE_AVCODEC_ALLOC_CONTEXT3
	codecContext=avcodec_alloc_context3(NULL);
#else
	codecContext=avcodec_alloc_context();
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3
	avcodec_parameters_to_context(codecContext,codecPar);

#ifdef HAVE_AVCODEC_OPEN2
	if(avcodec_open2(codecContext, codec, NULL)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3
		return;

	if(fillDataAndCheckValidity())
		status=VALID;
#ifdef HAVE_AVCODEC_DECODE_AUDIO4
	frameIn=av_frame_alloc();
#endif
}
#else
FFMpegAudioDecoder::FFMpegAudioDecoder(EngineData* eng,AVCodecContext* _c):engine(eng),ownedContext(false),codecContext(_c)
#if defined HAVE_LIBAVRESAMPLE || defined HAVE_LIBSWRESAMPLE
	,resamplecontext(NULL)
#endif
{
	status=INIT;
	AVCodec* codec=avcodec_find_decoder(codecContext->codec_id);
	assert(codec);

#ifdef HAVE_AVCODEC_OPEN2
	if(avcodec_open2(codecContext, codec, NULL)<0)
#else
	if(avcodec_open(codecContext, codec)<0)
#endif //HAVE_AVCODEC_ALLOC_CONTEXT3
		return;

	if(fillDataAndCheckValidity())
		status=VALID;
#if HAVE_AVCODEC_DECODE_AUDIO4
	frameIn=av_frame_alloc();
#endif
}
#endif

FFMpegAudioDecoder::~FFMpegAudioDecoder()
{
	avcodec_close(codecContext);
	if(ownedContext)
		av_free(codecContext);
#if HAVE_AVCODEC_DECODE_AUDIO4
	av_free(frameIn);
#endif
#ifdef HAVE_LIBSWRESAMPLE
	if (resamplecontext)
		swr_free(&resamplecontext);
#elif defined HAVE_LIBAVRESAMPLE
	if (resamplecontext)
		avresample_free(&resamplecontext);
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
		case LINEAR_PCM_LE:
			return CODEC_ID_PCM_S16LE;
		case LINEAR_PCM_FLOAT_BE:
			return CODEC_ID_PCM_F32BE;
		default:
			return CODEC_ID_NONE;
	}
}

bool FFMpegAudioDecoder::fillDataAndCheckValidity()
{
	if(codecContext->sample_rate!=0)
	{
		LOG(LOG_INFO,_("AUDIO DEC: Audio sample rate ") << codecContext->sample_rate);
		sampleRate=codecContext->sample_rate;
	}
	else
		return false;

	if(codecContext->channels!=0)
	{
		LOG(LOG_INFO, _("AUDIO DEC: Audio channels ") << codecContext->channels);
		channelCount=codecContext->channels;
	}
	else
		return false;

	if(initialTime==(uint32_t)-1 && !samplesBuffer.isEmpty())
	{
		initialTime=getFrontTime();
		LOG(LOG_INFO,_("AUDIO DEC: Initial timestamp ") << initialTime);
	}
	else
		return false;

	return true;
}

uint32_t FFMpegAudioDecoder::decodeData(uint8_t* data, int32_t datalen, uint32_t time)
{
#if defined HAVE_AVCODEC_SEND_PACKET && defined HAVE_AVCODEC_RECEIVE_FRAME
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
	av_frame_unref(frameIn);
	int ret = avcodec_send_packet(codecContext, &pkt);
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
			FrameSamples& curTail=samplesBuffer.acquireLast();
			int len = resampleFrameToS16(curTail);
#if ( LIBAVUTIL_VERSION_INT < AV_VERSION_INT(56,0,100) )
			maxLen = pkt.size - av_frame_get_pkt_size (frameIn);
#else
			maxLen = pkt.size - frameIn->pkt_size;
#endif
			curTail.len=len;
			assert(!(curTail.len&0x80000000));
			assert(len%2==0);
			curTail.current=curTail.samples;
			curTail.time=time;
			samplesBuffer.commitLast();
			if(status==INIT && fillDataAndCheckValidity())
				status=VALID;
		}
	}
	if (maxLen > 0)
	{
		int tmpsize = pkt.size - maxLen;
		uint8_t* tmpdata = pkt.data;
		tmpdata += maxLen;

		if (tmpsize > 0)
		{
			overflowBuffer.assign(tmpdata , tmpdata+tmpsize);
		}
	}
#ifdef HAVE_AV_PACKET_UNREF
	av_packet_unref(&pkt);
#else
	av_free_packet(&pkt);
#endif
	return maxLen;
#else
	FrameSamples& curTail=samplesBuffer.acquireLast();
	int maxLen=AVCODEC_MAX_AUDIO_FRAME_SIZE;
#if defined HAVE_AVCODEC_DECODE_AUDIO3 || defined HAVE_AVCODEC_DECODE_AUDIO4
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
#ifdef HAVE_AVCODEC_DECODE_AUDIO4
	av_frame_unref(frameIn);
	int frameOk=0;
	int32_t ret=avcodec_decode_audio4(codecContext, frameIn, &frameOk, &pkt);
	if(frameOk==0)
	{
		LOG(LOG_ERROR,"not decoded audio:"<<ret);
	}
	else
	{
		maxLen = resampleFrameToS16(curTail);
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
	samplesBuffer.commitLast();

	if(status==INIT && fillDataAndCheckValidity())
		status=VALID;

	return maxLen;
#endif
}

uint32_t FFMpegAudioDecoder::decodePacket(AVPacket* pkt, uint32_t time)
{
#if defined HAVE_AVCODEC_SEND_PACKET && defined HAVE_AVCODEC_RECEIVE_FRAME
	av_frame_unref(frameIn);
	int ret = avcodec_send_packet(codecContext, pkt);
	int maxLen = 0;
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
			FrameSamples& curTail=samplesBuffer.acquireLast();
			int len = resampleFrameToS16(curTail);
#if ( LIBAVUTIL_VERSION_INT < AV_VERSION_INT(56,0,100) )
			maxLen = pkt->size - av_frame_get_pkt_size (frameIn);
#else
			maxLen = pkt->size - frameIn->pkt_size;
#endif
			curTail.len=len;
			assert(!(curTail.len&0x80000000));
			assert(len%2==0);
			curTail.current=curTail.samples;
			curTail.time=time;
			samplesBuffer.commitLast();
			if(status==INIT && fillDataAndCheckValidity())
				status=VALID;
		}
	}
	return maxLen;
#else
	FrameSamples& curTail=samplesBuffer.acquireLast();
	int maxLen=AVCODEC_MAX_AUDIO_FRAME_SIZE;
#if HAVE_AVCODEC_DECODE_AUDIO4
	av_frame_unref(frameIn);
	int frameOk=0;
	int ret=avcodec_decode_audio4(codecContext, frameIn, &frameOk, pkt);
	if(frameOk==0)
	{
		LOG(LOG_ERROR,"not decoded audio:"<<ret);
	}
	else
	{
		maxLen = resampleFrameToS16(curTail);
	}
#elif HAVE_AVCODEC_DECODE_AUDIO3
	int ret=avcodec_decode_audio3(codecContext, curTail.samples, &maxLen, pkt);
#else
	int ret=avcodec_decode_audio2(codecContext, curTail.samples, &maxLen, pkt->data, pkt->size);
#endif

	if(ret==-1)
	{
		//A decoding error occurred, create an empty sample buffer
		LOG(LOG_ERROR,_("Malformed audio packet"));
		curTail.len=0;
		curTail.current=curTail.samples;
		curTail.time=time;
		samplesBuffer.commitLast();
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
	samplesBuffer.commitLast();
	return maxLen;
#endif
}
#if defined HAVE_AVCODEC_DECODE_AUDIO4 || (defined HAVE_AVCODEC_SEND_PACKET && defined HAVE_AVCODEC_RECEIVE_FRAME)
int FFMpegAudioDecoder::resampleFrameToS16(FrameSamples& curTail)
{
	int sample_rate = engine->audio_getSampleRate();
	unsigned int channel_layout = AV_CH_LAYOUT_STEREO;
#ifdef HAVE_AV_FRAME_GET_SAMPLE_RATE
#if ( LIBAVUTIL_VERSION_INT < AV_VERSION_INT(56,0,100) )
 	int framesamplerate = av_frame_get_sample_rate(frameIn);
#else
	int framesamplerate = this->codecContext->sample_rate;
#endif
#else
	int framesamplerate = frameIn->sample_rate;
#endif
	if(frameIn->format == AV_SAMPLE_FMT_S16 && sample_rate == framesamplerate && channel_layout == frameIn->channel_layout)
	{
		//This is suboptimal but equivalent to what libavcodec
		//does for the compatibility version of avcodec_decode_audio3
		memcpy(curTail.samples, frameIn->extended_data[0], frameIn->linesize[0]);
		return frameIn->linesize[0];
	}
	int maxLen;
#ifdef HAVE_LIBSWRESAMPLE
	if (!resamplecontext)
	{
		resamplecontext = swr_alloc();
		av_opt_set_int(resamplecontext, "in_channel_layout",  frameIn->channel_layout, 0);
		av_opt_set_int(resamplecontext, "out_channel_layout", channel_layout,  0);
		av_opt_set_int(resamplecontext, "in_sample_rate",     framesamplerate,     0);
		av_opt_set_int(resamplecontext, "out_sample_rate",    sample_rate,     0);
		av_opt_set_int(resamplecontext, "in_sample_fmt",      frameIn->format,   0);
		av_opt_set_int(resamplecontext, "out_sample_fmt",     AV_SAMPLE_FMT_S16,    0);
		swr_init(resamplecontext);
	}

	uint8_t *output;
	int out_samples = swr_get_out_samples(resamplecontext,frameIn->nb_samples);
	int res = av_samples_alloc(&output, NULL, 2, out_samples,AV_SAMPLE_FMT_S16, 0);

	if (res >= 0)
	{
		maxLen = swr_convert(resamplecontext, &output, out_samples, (const uint8_t**)frameIn->extended_data, frameIn->nb_samples)*2*av_get_channel_layout_nb_channels(channel_layout);// 2 bytes in AV_SAMPLE_FMT_S16
		if (maxLen > 0)
			memcpy(curTail.samples, output, maxLen);
		else
		{
			LOG(LOG_ERROR, "resampling failed");
			memset(curTail.samples, 0, frameIn->linesize[0]);
			maxLen = frameIn->linesize[0];
		}
		av_freep(&output);
	}
	else
	{
		LOG(LOG_ERROR, "resampling failed, error code:"<<res);
		memset(curTail.samples, 0, frameIn->linesize[0]);
		maxLen = frameIn->linesize[0];
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
		av_opt_set_int(resamplecontext, "out_sample_fmt",     AV_SAMPLE_FMT_S16,    0);
		avresample_open(resamplecontext);
	}

	uint8_t *output;
	int out_linesize;
	int out_samples = avresample_available(resamplecontext) + av_rescale_rnd(avresample_get_delay(resamplecontext) + frameIn->linesize[0], sample_rate, sample_rate, AV_ROUND_UP);
	int res = av_samples_alloc(&output, &out_linesize, frameIn->nb_samples, out_samples, AV_SAMPLE_FMT_S16, 0);
	if (res >= 0)
	{
		maxLen = avresample_convert(resamplecontext, &output, out_linesize, out_samples, frameIn->extended_data, frameIn->linesize[0], frameIn->nb_samples)*2*av_get_channel_layout_nb_channels(channel_layout); // 2 bytes in AV_SAMPLE_FMT_S16
		memcpy(curTail.samples, output, maxLen);
		av_freep(&output);
	}
	else
	{
		LOG(LOG_ERROR, "resampling failed, error code:"<<res);
		memset(curTail.samples, 0, frameIn->linesize[0]);
		maxLen = frameIn->linesize[0];
	}
#else
	LOG(LOG_ERROR, "unexpected sample format and can't resample, recompile with libswresample");
	memset(curTail.samples, 0, frameIn->linesize[0]);
	maxLen = frameIn->linesize[0];
#endif
	return maxLen;
}
#endif

#endif //ENABLE_LIBAVCODEC

StreamDecoder::~StreamDecoder()
{
	delete audioDecoder;
	delete videoDecoder;
}

#ifdef ENABLE_LIBAVCODEC
FFMpegStreamDecoder::FFMpegStreamDecoder(NetStream *ns, EngineData *eng, std::istream& s, AudioFormat* format, int streamsize)
 : netstream(ns),audioFound(false),videoFound(false),stream(s),formatCtx(nullptr),audioIndex(-1),
   videoIndex(-1),customAudioDecoder(nullptr),customVideoDecoder(nullptr),avioContext(nullptr),availablestreamlength(streamsize)
{
	int aviobufsize = streamsize == -1 ? 4096 : min(4096, streamsize);
	valid=false;
	avioBuffer = (uint8_t*)av_malloc(aviobufsize);
#ifdef HAVE_AVIO_ALLOC_CONTEXT
	avioContext=avio_alloc_context(avioBuffer,aviobufsize,0,this,avioReadPacket,nullptr,nullptr);
#else
	avioContext=av_alloc_put_byte(avioBuffer,aviobufsize,0,this,avioReadPacket,nullptr,nullptr);
#endif
	if(avioContext==NULL)
		return;

#if LIBAVFORMAT_VERSION_MAJOR > 52 || (LIBAVFORMAT_VERSION_MAJOR == 52  && LIBAVFORMAT_VERSION_MINOR > 64)
 	avioContext->seekable = 0;
#else
	avioContext->is_streamed=1;
#endif

	AVInputFormat* fmt = NULL;
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
			case LS_AUDIO_CODEC::LINEAR_PCM_LE:
				fmt = av_find_input_format("s16le");
				break;
			case LS_AUDIO_CODEC::LINEAR_PCM_FLOAT_BE:
				fmt = av_find_input_format("f32be");
				break;
			case LS_AUDIO_CODEC::ADPCM:
				LOG(LOG_NOT_IMPLEMENTED,"audio codec unknown for type "<<(int)format->codec<<", using ffmpeg autodetection");
				break;
			case LS_AUDIO_CODEC::CODEC_NONE:
				break;
		}
	}
	if (fmt == NULL)
	{
		//Probe the stream format.
		//NOTE: in FFMpeg 0.7 there is av_probe_input_buffer
		AVProbeData probeData;
		probeData.filename="lightspark_stream";
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
		probeData.mime_type=NULL;
#endif
		probeData.buf=new uint8_t[8192+AVPROBE_PADDING_SIZE];
		memset(probeData.buf,0,8192+AVPROBE_PADDING_SIZE);
		stream.read((char*)probeData.buf,8192);
		int read=stream.gcount();
		if(read!=8192)
			LOG(LOG_ERROR,"Not sufficient data is available from the stream:"<<read);
		probeData.buf_size=read;

		stream.seekg(0);
		fmt=av_probe_input_format(&probeData,1);
		delete[] probeData.buf;
	}
	if(fmt==NULL)
		return;

#ifdef HAVE_AVIO_ALLOC_CONTEXT
	formatCtx=avformat_alloc_context();
	formatCtx->pb = avioContext;
	int ret=avformat_open_input(&formatCtx, "lightspark_stream", fmt, NULL);
#else
	int ret=av_open_input_stream(&formatCtx, avioContext, "lightspark_stream", fmt, NULL);
#endif
	if(ret<0)
		return;
	if (!format)
	{
#ifdef HAVE_AVFORMAT_FIND_STREAM_INFO
		ret=avformat_find_stream_info(formatCtx,NULL);
#else
		ret=av_find_stream_info(formatCtx);
#endif
	}
	if(ret<0)
		return;

	LOG_CALL(_("FFMpeg found ") << formatCtx->nb_streams << _(" streams"));
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
			customAudioDecoder=new FFMpegAudioDecoder(eng,format->codec,format->sampleRate,format->channels,true);
		else
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(57, 40, 101)
			customAudioDecoder=new FFMpegAudioDecoder(eng,formatCtx->streams[audioIndex]->codecpar);
#else
			customAudioDecoder=new FFMpegAudioDecoder(eng,formatCtx->streams[audioIndex]->codec);
#endif
		audioDecoder=customAudioDecoder;
	}
	if (netstream && formatCtx->duration)
	{
		std::list<asAtom> dataobjectlist;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=netstream->getSystemState()->getUniqueStringId("duration");
		m.isAttribute = false;
		ASObject* dataobj = Class<ASObject>::getInstanceS(netstream->getSystemState());
		asAtom v = asAtomHandler::fromInt(formatCtx->duration/AV_TIME_BASE);
		dataobj->setVariableByMultiname(m,v,ASObject::CONST_NOT_ALLOWED);
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
	audioDecoder=NULL;
	videoDecoder=NULL;
	if(formatCtx)
	{
#ifdef HAVE_AVIO_ALLOC_CONTEXT
#ifdef HAVE_AVFORMAT_CLOSE_INPUT
		avformat_close_input(&formatCtx);
#else
		av_close_input_file(formatCtx);
#endif
#else
		av_close_input_stream(formatCtx);
#endif
	}
	if(avioContext)
		av_free(avioContext);
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
	uint32_t mtime=pkt.dts*1000*time_base.num/time_base.den;

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
#ifdef HAVE_AV_PACKET_UNREF
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
		return 0;
	th->stream.read((char*)buf,th->availablestreamlength == -1 ? buf_size : min(buf_size,th->availablestreamlength));
	int ret=th->stream.gcount();
	if (th->availablestreamlength != -1)
		th->availablestreamlength -= ret;
	return ret;
}
#endif //ENABLE_LIBAVCODEC
