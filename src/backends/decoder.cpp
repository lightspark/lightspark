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

#include "compat.h"
#include <assert.h>

#include "decoder.h"
#include "platforms/fastpaths.h"
#include "swf.h"
#include "backends/rendering.h"

#if LIBAVUTIL_VERSION_MAJOR < 51
#define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
#define AVMEDIA_TYPE_AUDIO CODEC_TYPE_AUDIO
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

FFMpegVideoDecoder::FFMpegVideoDecoder(LS_VIDEO_CODEC codecId, uint8_t* initdata, uint32_t datalen, double frameRateHint):
	curBuffer(0),curBufferOffset(0),codecContext(NULL),ownedContext(true)
{
	//The tag is the header, initialize decoding
	codecContext=avcodec_alloc_context();
	AVCodec* codec=NULL;
	videoCodec=codecId;
	if(codecId==H264)
	{
		//TODO: serialize access to avcodec_open
		const enum CodecID FFMPEGcodecId=CODEC_ID_H264;
		codec=avcodec_find_decoder(FFMPEGcodecId);
		assert(codec);

		//If this tag is the header, fill the extradata for the codec
		codecContext->extradata=initdata;
		codecContext->extradata_size=datalen;

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

	if(avcodec_open(codecContext, codec)<0)
		throw RunTimeException("Cannot open decoder");

	if(fillDataAndCheckValidity())
		status=VALID;
	else
		status=INIT;

	frameIn=avcodec_alloc_frame();
}

FFMpegVideoDecoder::FFMpegVideoDecoder(AVCodecContext* _c, double frameRateHint):
	curBuffer(0),curBufferOffset(0),codecContext(_c),ownedContext(false)
{
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
	if(avcodec_open(codecContext, codec)<0)
		return;

	frameRate=frameRateHint;

	if(fillDataAndCheckValidity())
		status=VALID;

	frameIn=avcodec_alloc_frame();
}

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

		//As the size chaged, reset the buffer
		uint32_t bufferSize=frameWidth*frameHeight/**4*/;
		buffers.regen(YUVBufferGenerator(bufferSize));
	}
}

void FFMpegVideoDecoder::skipUntil(uint32_t time)
{
	while(1)
	{
		if(buffers.isEmpty())
			break;
		if(buffers.front().time>=time)
			break;
		discardFrame();
	}
}
void FFMpegVideoDecoder::skipAll()
{
	while(!buffers.isEmpty())
		discardFrame();
}

bool FFMpegVideoDecoder::discardFrame()
{
	Locker locker(mutex);
	//We don't want ot block if no frame is available
	bool ret=buffers.nonBlockingPopFront();
	if(flushing && buffers.isEmpty()) //End of our work
	{
		status=FLUSHED;
		flushed.signal();
	}
	return ret;
}

bool FFMpegVideoDecoder::decodeData(uint8_t* data, uint32_t datalen, uint32_t time)
{
	if(datalen==0)
		return false;
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
	assert_and_throw(ret==(int)datalen);
	if(frameOk)
	{
		assert(codecContext->pix_fmt==PIX_FMT_YUV420P);

		if(status==INIT && fillDataAndCheckValidity())
			status=VALID;

		assert(frameIn->pts==(int64_t)AV_NOPTS_VALUE || frameIn->pts==0);

		copyFrameToBuffers(frameIn, time);
	}
	return true;
}

bool FFMpegVideoDecoder::decodePacket(AVPacket* pkt, uint32_t time)
{
	int frameOk=0;

#if HAVE_AVCODEC_DECODE_VIDEO2
	int ret=avcodec_decode_video2(codecContext, frameIn, &frameOk, pkt);
#else
	int ret=avcodec_decode_video(codecContext, frameIn, &frameOk, pkt->data, pkt->size);
#endif

	assert_and_throw(ret==(int)pkt->size);
	if(frameOk)
	{
		assert(codecContext->pix_fmt==PIX_FMT_YUV420P);

		if(status==INIT && fillDataAndCheckValidity())
			status=VALID;

		assert(frameIn->pts==(int64_t)AV_NOPTS_VALUE || frameIn->pts==0);

		copyFrameToBuffers(frameIn, time);
	}
	return true;
}

void FFMpegVideoDecoder::copyFrameToBuffers(const AVFrame* frameIn, uint32_t time)
{
	YUVBuffer& curTail=buffers.acquireLast();
	//Only one thread may access the tail
	int offset[3]={0,0,0};
	for(uint32_t y=0;y<frameHeight;y++)
	{
		memcpy(curTail.ch[0]+offset[0],frameIn->data[0]+(y*frameIn->linesize[0]),frameWidth);
		offset[0]+=frameWidth;
	}
	for(uint32_t y=0;y<frameHeight/2;y++)
	{
		memcpy(curTail.ch[1]+offset[1],frameIn->data[1]+(y*frameIn->linesize[1]),frameWidth/2);
		memcpy(curTail.ch[2]+offset[2],frameIn->data[2]+(y*frameIn->linesize[2]),frameWidth/2);
		offset[1]+=frameWidth/2;
		offset[2]+=frameWidth/2;
	}
	curTail.time=time;

	buffers.commitLast();
}

void FFMpegVideoDecoder::upload(uint8_t* data, uint32_t w, uint32_t h) const
{
	if(buffers.isEmpty())
		return;
	//Verify that the size are right
	assert_and_throw(w==((frameWidth+15)&0xfffffff0) && h==frameHeight);
	//At least a frame is available
	const YUVBuffer& cur=buffers.front();
	fastYUV420ChannelsToYUV0Buffer(cur.ch[0],cur.ch[1],cur.ch[2],data,frameWidth,frameHeight);
}

void FFMpegVideoDecoder::YUVBufferGenerator::init(YUVBuffer& buf) const
{
	if(buf.ch[0])
	{
		free(buf.ch[0]);
		free(buf.ch[1]);
		free(buf.ch[2]);
	}
	int ret=aligned_malloc((void**)&buf.ch[0], 16, bufferSize);
	assert(ret==0);
	ret=aligned_malloc((void**)&buf.ch[1], 16, bufferSize/4);
	assert(ret==0);
	ret=aligned_malloc((void**)&buf.ch[2], 16, bufferSize/4);
	assert(ret==0);
}
#endif //ENABLE_LIBAVCODEC

void* AudioDecoder::operator new(size_t s)
{
	void* retAddr=NULL;

	// fix warnings
#ifndef NDEBUG
	int ret =
#endif
	aligned_malloc(&retAddr, 16, s);
	assert(ret==0);

	assert(retAddr);
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
FFMpegAudioDecoder::FFMpegAudioDecoder(LS_AUDIO_CODEC audioCodec, uint8_t* initdata, uint32_t datalen):ownedContext(true)
{
	CodecID codecId;
	switch(audioCodec)
	{
		case AAC:
			codecId=CODEC_ID_AAC;
			break;
		case MP3:
			codecId=CODEC_ID_MP3;
			break;
		default:
			::abort();
	}
	AVCodec* codec=avcodec_find_decoder(codecId);
	assert(codec);

	codecContext=avcodec_alloc_context();

	if(initdata)
	{
		codecContext->extradata=initdata;
		codecContext->extradata_size=datalen;
	}

	if(avcodec_open(codecContext, codec)<0)
		throw RunTimeException("Cannot open decoder");

	if(fillDataAndCheckValidity())
		status=VALID;
	else
		status=INIT;
}

FFMpegAudioDecoder::FFMpegAudioDecoder(AVCodecContext* _c):codecContext(_c)
{
	status=INIT;
	AVCodec* codec=avcodec_find_decoder(codecContext->codec_id);
	assert(codec);

	if(avcodec_open(codecContext, codec)<0)
		return;

	if(fillDataAndCheckValidity())
		status=VALID;
}

FFMpegAudioDecoder::~FFMpegAudioDecoder()
{
	avcodec_close(codecContext);
	if(ownedContext)
		av_free(codecContext);
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

uint32_t FFMpegAudioDecoder::decodeData(uint8_t* data, uint32_t datalen, uint32_t time)
{
	FrameSamples& curTail=samplesBuffer.acquireLast();
	int maxLen=AVCODEC_MAX_AUDIO_FRAME_SIZE;
#if HAVE_AVCODEC_DECODE_AUDIO3
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data=data;
	pkt.size=datalen;
	uint32_t ret=avcodec_decode_audio3(codecContext, curTail.samples, &maxLen, &pkt);
#else
	uint32_t ret=avcodec_decode_audio2(codecContext, curTail.samples, &maxLen, data, datalen);
#endif
	assert_and_throw(ret==datalen);

	if(status==INIT && fillDataAndCheckValidity())
		status=VALID;

	curTail.len=maxLen;
	assert(!(curTail.len&0x80000000));
	assert(maxLen%2==0);
	curTail.current=curTail.samples;
	curTail.time=time;
	samplesBuffer.commitLast();
	return maxLen;
}

uint32_t FFMpegAudioDecoder::decodePacket(AVPacket* pkt, uint32_t time)
{
	FrameSamples& curTail=samplesBuffer.acquireLast();
	int maxLen=AVCODEC_MAX_AUDIO_FRAME_SIZE;

#if HAVE_AVCODEC_DECODE_AUDIO3
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
}
#endif //ENABLE_LIBAVCODEC

StreamDecoder::~StreamDecoder()
{
	delete audioDecoder;
	delete videoDecoder;
}

FFMpegStreamDecoder::FFMpegStreamDecoder(std::istream& s):stream(s),formatCtx(NULL),audioFound(false),videoFound(false),avioContext(NULL)
{
	valid=false;
	//NOTE: this will become avio_alloc_context in FFMpeg 0.7
	avioContext=av_alloc_put_byte(avioBuffer,4096,0,this,avioReadPacket,NULL,NULL);
	if(avioContext==NULL)
		return;

#if LIBAVFORMAT_VERSION_MAJOR > 52 || (LIBAVFORMAT_VERSION_MAJOR == 52  && LIBAVFORMAT_VERSION_MINOR > 64)
 	avioContext->seekable = 0;
#else
	avioContext->is_streamed=1;
#endif
	
	//Probe the stream format.
	//NOTE: in FFMpeg 0.7 there is av_probe_input_buffer
	AVProbeData probeData;
	probeData.filename="lightspark_stream";
	probeData.buf=new uint8_t[8192+AVPROBE_PADDING_SIZE];
	memset(probeData.buf,0,8192+AVPROBE_PADDING_SIZE);
	stream.read((char*)probeData.buf,8192);
	int read=stream.gcount();
	if(read!=8192)
		LOG(LOG_ERROR,_("Not sufficient data is available from the stream"));
	probeData.buf_size=read;

	stream.seekg(0);
	AVInputFormat* fmt;
	fmt=av_probe_input_format(&probeData,1);
	delete[] probeData.buf;
	if(fmt==NULL)
		return;

	int ret=av_open_input_stream(&formatCtx, avioContext, "lightspark_stream", fmt, NULL);
	if(ret<0)
		return;
	
	ret=av_find_stream_info(formatCtx);
	if(ret<0)
		return;

	LOG(LOG_CALLS,_("FFMpeg found ") << formatCtx->nb_streams << _(" streams"));
	for(uint32_t i=0;i<formatCtx->nb_streams;i++)
	{
		if(formatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO && videoFound==false)
		{
			videoFound=true;
			videoIndex=(int32_t)i;
		}
		else if(formatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO && audioFound==false)
		{
			audioFound=true;
			audioIndex=(int32_t)i;
		}

	}
	if(videoFound)
	{
		//Pass the frame rate from the container, the once from the codec is often wrong
		double frameRate=av_q2d(formatCtx->streams[videoIndex]->r_frame_rate);
		customVideoDecoder=new FFMpegVideoDecoder(formatCtx->streams[videoIndex]->codec,frameRate);
		videoDecoder=customVideoDecoder;
	}

	if(audioFound)
	{
		customAudioDecoder=new FFMpegAudioDecoder(formatCtx->streams[audioIndex]->codec);
		audioDecoder=customAudioDecoder;
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
		av_close_input_stream(formatCtx);
	if(avioContext)
		av_free(avioContext);
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

	if(pkt.stream_index==(int)audioIndex)
		customAudioDecoder->decodePacket(&pkt, mtime);
	else
		customVideoDecoder->decodePacket(&pkt, mtime);
	av_free_packet(&pkt);
	return true;
}

bool FFMpegStreamDecoder::getMetadataInteger(const char* name, uint32_t& ret) const
{
	return false;
}

bool FFMpegStreamDecoder::getMetadataDouble(const char* name, double& ret) const
{
	if( string(name) == "duration" )
	{
		ret = double(formatCtx->duration) / double(AV_TIME_BASE);
		return true;
	}
	return false;
}

int FFMpegStreamDecoder::avioReadPacket(void* t, uint8_t* buf, int buf_size)
{
	FFMpegStreamDecoder* th=static_cast<FFMpegStreamDecoder*>(t);
	th->stream.read((char*)buf,buf_size);
	int ret=th->stream.gcount();
	return ret;
}
