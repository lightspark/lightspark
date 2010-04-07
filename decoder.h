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
#include "threading.h"
extern "C"
{
#include <libavcodec/avcodec.h>
}

#ifdef USE_VAAPI
namespace gnash
{
	class VaapiSurface;
	class VaapiSurfaceProxy;
	class VaapiSurfaceGLX;
	namespace media
	{
		namespace ffmpeg
		{
			class VaapiContextFfmpeg;
		};
	};
};
using gnash::VaapiSurface;
using gnash::VaapiSurfaceProxy;
using gnash::VaapiSurfaceGLX;
using gnash::media::ffmpeg::VaapiContextFfmpeg;
#endif

namespace lightspark
{

class Decoder
{
private:
	bool resizeGLBuffers;
protected:
	uint32_t frameWidth;
	uint32_t frameHeight;
	bool setSize(uint32_t w, uint32_t h);
public:
	Decoder():resizeGLBuffers(false),frameWidth(0),frameHeight(0){}
	virtual ~Decoder(){}
	virtual bool decodeData(uint8_t* data, uint32_t datalen)=0;
	virtual bool discardFrame()=0;
	//NOTE: the base implementation returns true if resizing of buffers should be done
	//This should be called in every derived implementation
	virtual bool copyFrameToTexture(GLuint tex)=0;
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
private:
	GLuint videoBuffers[2];
	unsigned int curBuffer;
	AVCodecContext* codecContext;
	uint8_t* buffers[10][3];
	//Counting semaphores for buffers
	Condition freeBuffers;
	Condition usedBuffers;
	Mutex mutex;
	bool empty;
	uint32_t bufferHead;
	uint32_t bufferTail;
	bool initialized;
	AVFrame* frameIn;
	void copyFrameToBuffers(const AVFrame* frameIn, uint32_t width, uint32_t height);
	void setSize(uint32_t w, uint32_t h);
public:
	FFMpegDecoder(uint8_t* initdata, uint32_t datalen);
	~FFMpegDecoder();
	bool decodeData(uint8_t* data, uint32_t datalen);
	bool discardFrame();
	bool copyFrameToTexture(GLuint tex);
};

#ifdef USE_VAAPI
class VaapiDecoder: public Decoder
{
private:
	AVCodecContext* codecContext;
	VaapiSurfaceProxy* surfaces[10];
	//Counting semaphores for surfaces
	Condition freeBuffers;
	Condition usedBuffers;
	Mutex mutex;
	bool empty;
	uint32_t bufferHead;
	uint32_t bufferTail;
	AVFrame* frameIn;
	GLuint validTexture;
	VaapiSurfaceGLX* glxSurface;
	void copyFrameToSurfaces(const AVFrame* frameIn);
	void setSize(uint32_t w, uint32_t h);
	/* --- VA-API glue --- */
	static inline VaapiContextFfmpeg *vaapi_get_context(AVCodecContext *avctx)
	{
		return static_cast<VaapiContextFfmpeg *>(avctx->hwaccel_context);
	}
	static inline void vaapi_set_context(AVCodecContext *avctx, VaapiContextFfmpeg *vactx)
	{
		avctx->hwaccel_context = vactx;
	}
	/// (Re)set AVCodecContext to sane values 
	static void reset_context(AVCodecContext *avctx, VaapiContextFfmpeg *vactx = NULL);
	/// AVCodecContext.get_format() implementation
	static enum PixelFormat vaapi_get_format(AVCodecContext *avctx, const enum PixelFormat *fmt);
	/// AVCodecContext.get_buffer() implementation
	static int vaapi_get_buffer(AVCodecContext *avctx, AVFrame *pic);
	/// AVCodecContext.reget_buffer() implementation
	static int vaapi_reget_buffer(AVCodecContext *avctx, AVFrame *pic);
	/// AVCodecContext.release_buffer() implementation
	static void vaapi_release_buffer(AVCodecContext *avctx, AVFrame *pic);
	static bool vaapi_init_context(AVCodecContext *avctx, enum CodecID codecId);
public:
	VaapiDecoder(uint8_t* initdata, uint32_t datalen);
	~VaapiDecoder();
	bool decodeData(uint8_t* data, uint32_t datalen);
	bool discardFrame();
	bool copyFrameToTexture(GLuint tex);
};
#endif

};
#endif
