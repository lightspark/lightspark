/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <assert.h>

#include "swf.h"
#include "graphics.h"
#include "logger.h"
#include "exceptions.h"
#include "backends/rendering.h"

#include <iostream>

using namespace lightspark;
extern TLSDATA RenderThread* rt;

void lightspark::cleanGLErrors()
{
#ifdef EXPENSIVE_DEBUG
	int glErrorCount = 0;
	GLenum err;
	while(1)
	{
		err=glGetError();
		if(err!=GL_NO_ERROR)
		{
			glErrorCount++;
			LOG(LOG_ERROR,"GL error "<< err);
		}
		else
			break;
	}

	if(glErrorCount)
	{
		LOG(LOG_ERROR,"Ignoring " << glErrorCount << " openGL errors");
	}
#else
	while(glGetError()!=GL_NO_ERROR);
#endif
}

void TextureBuffer::setAllocSize(uint32_t w, uint32_t h)
{
	if(rt->hasNPOTTextures)
	{
		allocWidth=w;
		allocHeight=h;
		//Now adjust for the requested alignment
		if((allocWidth%horizontalAlignment))
		{
			allocWidth+=horizontalAlignment;
			allocWidth-=(allocWidth%horizontalAlignment);
		}
		if((allocHeight%verticalAlignment))
		{
			allocHeight+=verticalAlignment;
			allocHeight-=(allocHeight%verticalAlignment);
		}
	}
	else
	{
		allocWidth=nearestPOT(w);
		allocHeight=nearestPOT(h);
		//Assert that the requested alignment is satisfied
		assert((allocWidth%horizontalAlignment)==0);
		assert((allocHeight%verticalAlignment)==0);
	}
}

uint32_t TextureBuffer::nearestPOT(uint32_t a) const
{
	if(a==0)
		return 0;
	uint32_t ret=1;
	while(ret<a)
		ret<<=1;
	return ret;
}

TextureBuffer::TextureBuffer(bool initNow, uint32_t w, uint32_t h, GLenum f):texId(0),filtering(f),allocWidth(0),allocHeight(0),
	width(w),height(h),horizontalAlignment(1),verticalAlignment(1),inited(false)
{
	if(initNow)
		init();
}

void TextureBuffer::init()
{
	assert(!inited);
	inited=true;
	cleanGLErrors();
	
	setAllocSize(width,height);
	assert(texId==0);
	glGenTextures(1,&texId);
	assert(texId!=0);
	assert(glGetError()!=GL_INVALID_OPERATION);
	
	assert(filtering==GL_NEAREST || filtering==GL_LINEAR);
	
	//If the previous call has not failed these should not fail (in specs, we trust)
	glBindTexture(GL_TEXTURE_2D,texId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filtering);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filtering);
	//Wrapping should not be very useful, we use textures carefully
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	
	//Allocate the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, allocWidth, allocHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	GLenum err=glGetError();
	assert(err!=GL_INVALID_OPERATION);
	if(err==GL_INVALID_VALUE)
	{
		LOG(LOG_ERROR,"GL_INVALID_VALUE after glTexImage2D, width=" << allocWidth << " height=" << allocHeight);
		throw RunTimeException("GL_INVALID_VALUE in TextureBuffer::init");
	}
	
	glBindTexture(GL_TEXTURE_2D,0);
	
#ifdef EXPENSIVE_DEBUG
	cleanGLErrors();
#endif
}

void TextureBuffer::init(uint32_t w, uint32_t h, GLenum f)
{
	assert(!inited);
	inited=true;
	cleanGLErrors();

	setAllocSize(w,h);
	width=w;
	height=h;
	filtering=f;
	
	assert(texId==0);
	glGenTextures(1,&texId);
	assert(texId!=0);
	assert(glGetError()!=GL_INVALID_OPERATION);
	
	assert(filtering==GL_NEAREST || filtering==GL_LINEAR);
	
	//If the previous call has not failed these should not fail (in specs, we trust)
	glBindTexture(GL_TEXTURE_2D,texId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filtering);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filtering);
	//Wrapping should not be very useful, we use textures carefully
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	
	//Allocate the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, allocWidth, allocHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	GLenum err=glGetError();
	assert(err!=GL_INVALID_OPERATION);
	if(err==GL_INVALID_VALUE)
	{
		LOG(LOG_ERROR,"GL_INVALID_VALUE after glTexImage2D, width=" << allocWidth << " height=" << allocHeight);
		throw RunTimeException("GL_INVALID_VALUE in TextureBuffer::init");
	}
	
	glBindTexture(GL_TEXTURE_2D,0);
	
#ifdef EXPENSIVE_DEBUG
	cleanGLErrors();
#endif
}

void TextureBuffer::resize(uint32_t w, uint32_t h)
{
	if(width!=w || height!=h)
	{
		if(w>allocWidth || h>allocHeight) //Destination texture should be reallocated
		{
			glBindTexture(GL_TEXTURE_2D,texId);
			LOG(LOG_CALLS,"Reallocating texture to size " << w << 'x' << h);
			setAllocSize(w,h);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, allocWidth, allocHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
			GLenum err=glGetError();
			assert(err!=GL_INVALID_OPERATION);
			if(err==GL_INVALID_VALUE)
			{
				LOG(LOG_ERROR,"GL_INVALID_VALUE after glTexImage2D, width=" << allocWidth << " height=" << allocHeight);
				throw RunTimeException("GL_INVALID_VALUE in TextureBuffer::setBGRAData");
			}
		}
		width=w;
		height=h;
#ifdef EXPENSIVE_DEBUG
		cleanGLErrors();
#endif
	}
}

void TextureBuffer::setBGRAData(uint8_t* bgraData, uint32_t w, uint32_t h)
{
	cleanGLErrors();

	//First of all resize the texture (if needed)
	resize(w, h);

	glBindTexture(GL_TEXTURE_2D,texId);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, bgraData);
	assert(glGetError()==GL_NO_ERROR);
	
#ifdef EXPENSIVE_DEBUG
	cleanGLErrors();
#endif
}

void TextureBuffer::setRequestedAlignment(uint32_t w, uint32_t h)
{
	assert(w && h);
	horizontalAlignment=w;
	verticalAlignment=h;
}

void TextureBuffer::setTexScale(GLuint uniformLocation)
{
	cleanGLErrors();

	float v1=width;
	float v2=height;
	v1/=allocWidth;
	v2/=allocHeight;
	glUniform2f(uniformLocation,v1,v2);
}

void TextureBuffer::bind()
{
	cleanGLErrors();

	glBindTexture(GL_TEXTURE_2D,texId);
	assert(glGetError()==GL_NO_ERROR);
}

void TextureBuffer::unbind()
{
	cleanGLErrors();

	glBindTexture(GL_TEXTURE_2D,0);
	assert(glGetError()==GL_NO_ERROR);
}

void TextureBuffer::shutdown()
{
	if(inited)
	{
		glDeleteTextures(1,&texId);
		inited=false;
	}
}

TextureBuffer::~TextureBuffer()
{
	shutdown();
}

MatrixApplier::MatrixApplier()
{
	//First of all try to preserve current matrix
	glPushMatrix();
	if(glGetError()==GL_STACK_OVERFLOW)
		throw RunTimeException("GL matrix stack exceeded");

	//TODO: implement smart stack flush
	//Save all the current stack, compute using SSE the final matrix and push that one
	//Maybe mulps, shuffle and parallel add
	//On unapply the stack will be reset as before
}

MatrixApplier::MatrixApplier(const MATRIX& m)
{
	//First of all try to preserve current matrix
	glPushMatrix();
	if(glGetError()==GL_STACK_OVERFLOW)
	{
		::abort();
	}

	float matrix[16];
	m.get4DMatrix(matrix);
	glMultMatrixf(matrix);
}

void MatrixApplier::concat(const MATRIX& m)
{
	float matrix[16];
	m.get4DMatrix(matrix);
	glMultMatrixf(matrix);
}

void MatrixApplier::unapply()
{
	glPopMatrix();
}
