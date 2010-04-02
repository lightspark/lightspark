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

#ifndef _DECODER_H
#define _DECODER_H

#include <GL/gl.h>
#include <semaphore.h>
extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace lightspark
{

class Decoder
{
protected:
	uint32_t frameWidth;
	uint32_t frameHeight;
public:
	Decoder():frameWidth(0),frameHeight(0){}
	virtual ~Decoder(){}
	virtual bool decodeData(uint8_t* data, uint32_t datalen)=0;
	virtual void discardFrame()=0;
	virtual bool copyFrameToBindedTexture()=0;
	uint32_t getWidth()
	{
		return frameWidth;
	}
	uint32_t getHeight()
	{
		return frameHeight;
	}
};

class FFMpegDecoder: public Decoder
{
public:
	GLuint videoBuffers[2];
	unsigned int curBuffer;
	AVCodecContext* codecContext;
	uint8_t* buffers[10][3];
	//Counting semaphores for buffers
	sem_t freeBuffers;
	sem_t usedBuffers;
	bool empty;
	uint32_t bufferHead;
	uint32_t bufferTail;
	bool resizeGLBuffers;
	bool initialized;
	AVFrame* frameIn;
	void setSize(uint32_t w, uint32_t h);
	void copyFrameToBuffers(const AVFrame* frameIn, uint32_t width, uint32_t height);
public:
	FFMpegDecoder(uint8_t* initdata, uint32_t datalen);
	~FFMpegDecoder();
	bool decodeData(uint8_t* data, uint32_t datalen);
	void discardFrame();
	bool copyFrameToBindedTexture();
};

};
#endif
