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
#include "graphics.h"

using namespace lightspark;

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

bool Decoder::copyFrameToTexture(TextureBuffer& tex)
{
	if(!resizeGLBuffers)
		return false;

	//Initialize texture to video size
	tex.resize(frameWidth, frameHeight);
	resizeGLBuffers=false;
	return true;
}

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

bool FFMpegDecoder::copyFrameToTexture(TextureBuffer& tex)
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
		tex.setBGRAData(0, frameWidth, frameHeight);
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
