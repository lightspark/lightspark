/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "compat.h"
#include <assert.h>
#include <GL/glew.h>

#include "decoder.h"
#include "fastpaths.h"
#include "swf.h"

using namespace lightspark;
#ifdef USE_VAAPI
#include "vaapi_utils.h"
#include "VaapiSurfaceGLX.h"
#include "VideoDecoderFfmpegVaapi.h"
using namespace gnash;
using namespace gnash::media::ffmpeg;
#endif

extern TLSDATA SystemState* sys;

bool Decoder::setSize(uint32_t w, uint32_t h)
{
	if(w!=frameWidth || h!=frameHeight)
	{
		frameWidth=w;
		frameHeight=h;
		LOG(LOG_NO_INFO,"Video frame size " << frameWidth << 'x' << frameHeight);
		resizeGLBuffers=true;
		return true;
	}
	else
		return false;
}

bool Decoder::copyFrameToTexture(GLuint tex)
{
	if(!resizeGLBuffers)
		return false;

	//Initialize texture to video size
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, frameWidth, frameHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL); 
	resizeGLBuffers=false;
	return true;
}

#ifdef USE_VAAPI
void VaapiDecoder::reset_context(AVCodecContext *avctx, VaapiContextFfmpeg *vactx)
{
	VaapiContextFfmpeg * const old_vactx = vaapi_get_context(avctx);
	if (old_vactx)
		delete old_vactx;
	vaapi_set_context(avctx, vactx);

	avctx->thread_count = 1;
	avctx->draw_horiz_band = NULL;
	if (vactx)
		avctx->slice_flags = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
	else
		avctx->slice_flags = 0;
}

enum PixelFormat VaapiDecoder::vaapi_get_format(AVCodecContext *avctx, const enum PixelFormat *fmt)
{
	VaapiContextFfmpeg * const vactx = vaapi_get_context(avctx);

	if (vactx) {
		for (int i = 0; fmt[i] != PIX_FMT_NONE; i++) {
			if (fmt[i] != PIX_FMT_VAAPI_VLD)
				continue;
			if (vactx->initDecoder(avctx->width, avctx->height))
				return fmt[i];
		}
	}

	reset_context(avctx);
	return avcodec_default_get_format(avctx, fmt);
}

int VaapiDecoder::vaapi_get_buffer(AVCodecContext *avctx, AVFrame *pic)
{
	VaapiContextFfmpeg * const vactx = vaapi_get_context(avctx);
	if (!vactx)
		return avcodec_default_get_buffer(avctx, pic);

	if (!vactx->initDecoder(avctx->width, avctx->height))
		return -1;

	VaapiSurfaceFfmpeg * const surface = vactx->getSurface();
	if (!surface)
		return -1;
	vaapi_set_surface(pic, surface);

	static unsigned int pic_num = 0;
	pic->type = FF_BUFFER_TYPE_USER;
	pic->age  = ++pic_num - surface->getPicNum();
	surface->setPicNum(pic_num);
	return 0;
}

int VaapiDecoder::vaapi_reget_buffer(AVCodecContext *avctx, AVFrame *pic)
{
	VaapiContextFfmpeg * const vactx = vaapi_get_context(avctx);

	if (!vactx)
		return avcodec_default_reget_buffer(avctx, pic);

	return vaapi_get_buffer(avctx, pic);
}

void VaapiDecoder::vaapi_release_buffer(AVCodecContext *avctx, AVFrame *pic)
{
	VaapiContextFfmpeg * const vactx = vaapi_get_context(avctx);
	if (!vactx) {
		avcodec_default_release_buffer(avctx, pic);
		return;
	}

	pic->data[0] = NULL;
	pic->data[1] = NULL;
	pic->data[2] = NULL;
	pic->data[3] = NULL;
}

bool VaapiDecoder::vaapi_init_context(AVCodecContext *avctx, enum CodecID codecId)
{
	VaapiContextFfmpeg *vactx = VaapiContextFfmpeg::create(codecId);
	if (!vactx)
		return false;

	reset_context(avctx, vactx);
	avctx->get_format     = vaapi_get_format;
	avctx->get_buffer     = vaapi_get_buffer;
	avctx->reget_buffer   = vaapi_reget_buffer;
	avctx->release_buffer = vaapi_release_buffer;
	return true;
}

void VaapiDecoder::copyFrameToSurfaces(const AVFrame* frameIn)
{
	assert(sys->useVaapi);
	freeBuffers.wait();

	//Only one thread may access the tail
	assert(surfaces[bufferTail]=NULL);
	VaapiSurfaceFfmpeg * const surface = vaapi_get_surface(frameIn);
	assert(surface);
	surfaces[bufferTail]=surface;
	bufferTail=(bufferTail+1)%10;
	empty=false;

	usedBuffers.signal();
}

bool VaapiDecoder::discardFrame()
{
	Locker locker(mutex);
	assert(sys->useVaapi);
	//We don't want ot block if no frame is available
	if(!usedBuffers.try_wait())
		return false;

	VaapiSurfaceProxy const * proxy = surfaces[bufferHead];
	surfaces[bufferHead]=NULL;
	assert(proxy);
	//A frame is available
	bufferHead=(bufferHead+1)%10;
	delete proxy;

	if(bufferHead==bufferTail)
		empty=true;
	freeBuffers.signal();
	return true;
}

bool VaapiDecoder::copyFrameToTexture(GLuint tex)
{
	Decoder::copyFrameToTexture(tex);

	assert(sys->useVaapi);
	if(tex!=validTexture || glxSurface==NULL) //The destination texture has changed
	{
		assert(validTexture==0);
		delete glxSurface;
		glxSurface=new VaapiSurfaceGLX(GL_TEXTURE_2D, tex);
		validTexture=tex;
	}
	
	if(!empty)
	{
		VaapiSurfaceProxy const * proxy = surfaces[bufferHead];
		assert(proxy);
		VaapiSurface* const surface = proxy->get().get();
		glxSurface->update(surface);
		return true;
	}

	return false;
}

void VaapiDecoder::setSize(uint32_t w, uint32_t h)
{
	if(Decoder::setSize(w,h))
	{
		//Discard all the frames
		while(discardFrame());
	}
}

bool VaapiDecoder::decodeData(uint8_t* data, uint32_t datalen)
{
	assert(sys->useVaapi);
	int frameOk=0;
	avcodec_decode_video(codecContext, frameIn, &frameOk, data, datalen);
	if(frameOk==0)
		abort();
	assert(codecContext->pix_fmt==PIX_FMT_VAAPI_VLD);

	const uint32_t height=codecContext->height;
	const uint32_t width=codecContext->width;
	assert(frameIn->pts==AV_NOPTS_VALUE || frameIn->pts==0);

	setSize(width,height);
	copyFrameToSurfaces(frameIn);
	return true;
}

VaapiDecoder::VaapiDecoder(uint8_t* initdata, uint32_t datalen):codecContext(NULL),freeBuffers(10),usedBuffers(0),mutex("Decoder"),
		empty(true),bufferHead(0),bufferTail(0),validTexture(0),glxSurface(NULL)
{
	assert(sys->useVaapi);
	for(int i=0;i<10;i++)
		surfaces[i]=NULL;

	//The tag is the header, initialize decoding
	//TODO: serialize access to avcodec_open
	const enum CodecID codecId=CODEC_ID_H264;
	AVCodec* codec=avcodec_find_decoder(codecId);
	assert(codec);

	codecContext=avcodec_alloc_context();
	//If this tag is the header, fill the extradata for the codec
	codecContext->extradata=initdata;
	codecContext->extradata_size=datalen;
	if(vaapi_init_context(codecContext, codecId)==0)
		abort();

	if(avcodec_open(codecContext, codec)<0)
		abort();

	frameIn=avcodec_alloc_frame();
}

VaapiDecoder::~VaapiDecoder()
{
	for(int i=0;i<10;i++)
		delete surfaces[i];
	assert(codecContext);
	av_free(codecContext);
	av_free(frameIn);
}
#endif

FFMpegDecoder::FFMpegDecoder(uint8_t* initdata, uint32_t datalen):curBuffer(0),codecContext(NULL),freeBuffers(10),usedBuffers(0),
		mutex("Decoder"),empty(true),bufferHead(0),bufferTail(0),initialized(false)
{
	for(int i=0;i<10;i++)
	{
		buffers[i][0]=NULL;
		buffers[i][1]=NULL;
		buffers[i][2]=NULL;
	}

	//The tag is the header, initialize decoding
	//TODO: serialize access to avcodec_open
	const enum CodecID codecId=CODEC_ID_H264;
	AVCodec* codec=avcodec_find_decoder(codecId);
	assert(codec);

	codecContext=avcodec_alloc_context();
	//If this tag is the header, fill the extradata for the codec
	codecContext->extradata=initdata;
	codecContext->extradata_size=datalen;

	if(avcodec_open(codecContext, codec)<0)
		abort();

	frameIn=avcodec_alloc_frame();
}

FFMpegDecoder::~FFMpegDecoder()
{
	for(int i=0;i<10;i++)
	{
		if(buffers[i][0])
		{
			free(buffers[i][0]);
			free(buffers[i][1]);
			free(buffers[i][2]);
		}
	}
	assert(codecContext);
	av_free(codecContext);
	av_free(frameIn);
}

//setSize is called from the routine that inserts new frames
void FFMpegDecoder::setSize(uint32_t w, uint32_t h)
{
	if(Decoder::setSize(w,h))
	{
		//Discard all the frames
		while(discardFrame());

		//As the size chaged, reset the buffer
		for(int i=0;i<10;i++)
		{
			if(buffers[i][0])
			{
				free(buffers[i][0]);
				free(buffers[i][1]);
				free(buffers[i][2]);
				buffers[i][0]=NULL;
				buffers[i][1]=NULL;
				buffers[i][2]=NULL;
			}
		}
	}
}

bool FFMpegDecoder::discardFrame()
{
	Locker locker(mutex);
	//We don't want ot block if no frame is available
	if(!usedBuffers.try_wait())
		return false;
	//A frame is available
	bufferHead=(bufferHead+1)%10;
	if(bufferHead==bufferTail)
		empty=true;
	freeBuffers.signal();
	return true;
}

bool FFMpegDecoder::decodeData(uint8_t* data, uint32_t datalen)
{
	int frameOk=0;
	avcodec_decode_video(codecContext, frameIn, &frameOk, data, datalen);
	if(frameOk==0)
		abort();
	assert(codecContext->pix_fmt==PIX_FMT_YUV420P);

	const uint32_t height=codecContext->height;
	const uint32_t width=codecContext->width;
	assert(frameIn->pts==AV_NOPTS_VALUE || frameIn->pts==0);

	setSize(width,height);
	copyFrameToBuffers(frameIn,width,height);
	return true;
}

void FFMpegDecoder::copyFrameToBuffers(const AVFrame* frameIn, uint32_t width, uint32_t height)
{
	freeBuffers.wait();
	uint32_t bufferSize=width*height*4;
	//Only one thread may access the tail
	if(buffers[bufferTail][0]==NULL)
	{
		posix_memalign((void**)&buffers[bufferTail][0], 16, bufferSize);
		posix_memalign((void**)&buffers[bufferTail][1], 16, bufferSize/4);
		posix_memalign((void**)&buffers[bufferTail][2], 16, bufferSize/4);
	}
	int offset[3]={0,0,0};
	for(uint32_t y=0;y<height;y++)
	{
		memcpy(buffers[bufferTail][0]+offset[0],frameIn->data[0]+(y*frameIn->linesize[0]),width);
		offset[0]+=width;
	}
	for(uint32_t y=0;y<height/2;y++)
	{
		memcpy(buffers[bufferTail][1]+offset[1],frameIn->data[1]+(y*frameIn->linesize[1]),width/2);
		memcpy(buffers[bufferTail][2]+offset[2],frameIn->data[2]+(y*frameIn->linesize[2]),width/2);
		offset[1]+=width/2;
		offset[2]+=width/2;
	}

	bufferTail=(bufferTail+1)%10;
	empty=false;
	usedBuffers.signal();
}

bool FFMpegDecoder::copyFrameToTexture(GLuint tex)
{
	if(!initialized)
	{
		glGenBuffers(2,videoBuffers);
		initialized=true;
	}

	bool ret=false;
	if(Decoder::copyFrameToTexture(tex))
	{
		//Initialize both PBOs to video size
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, videoBuffers[0]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, frameWidth*frameHeight*4, 0, GL_STREAM_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, videoBuffers[1]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, frameWidth*frameHeight*4, 0, GL_STREAM_DRAW);
	}
	else
	{
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, videoBuffers[curBuffer]);
		//Copy content of the pbo to the texture, 0 is the offset in the pbo
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frameWidth, frameHeight, GL_BGRA, GL_UNSIGNED_BYTE, 0); 
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		ret=true;
	}

	if(!empty)
	{
		//Increment and wrap current buffer index
		unsigned int nextBuffer = (curBuffer + 1)%2;

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, videoBuffers[nextBuffer]);
		uint8_t* buf=(uint8_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER,GL_WRITE_ONLY);

		//At least a frame is available
		int offset=0;
		for(uint32_t y=0;y<frameHeight;y++)
		{
			fastYUV420ChannelsToBuffer(buffers[bufferHead][0]+(y*frameWidth),
					buffers[bufferHead][1]+((y>>1)*(frameWidth>>1)),
					buffers[bufferHead][2]+((y>>1)*(frameWidth>>1)),
					buf+offset,frameWidth);
			offset+=frameWidth*4;
		}

		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		curBuffer=nextBuffer;
	}
	return ret;
}
