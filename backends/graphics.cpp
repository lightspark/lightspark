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
#include "compat.h"

#include <iostream>

using namespace lightspark;

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
			LOG(LOG_ERROR,_("GL error ")<< err);
		}
		else
			break;
	}

	if(glErrorCount)
	{
		LOG(LOG_ERROR,_("Ignoring ") << glErrorCount << _(" openGL errors"));
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
		LOG(LOG_ERROR,_("GL_INVALID_VALUE after glTexImage2D, width=") << allocWidth << _(" height=") << allocHeight);
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
		LOG(LOG_ERROR,_("GL_INVALID_VALUE after glTexImage2D, width=") << allocWidth << _(" height=") << allocHeight);
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
			LOG(LOG_CALLS,_("Reallocating texture to size ") << w << 'x' << h);
			setAllocSize(w,h);
			while(glGetError()!=GL_NO_ERROR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, allocWidth, allocHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
			GLenum err=glGetError();
			assert(err!=GL_INVALID_OPERATION);
			if(err==GL_INVALID_VALUE)
			{
				LOG(LOG_ERROR,_("GL_INVALID_VALUE after glTexImage2D, width=") << allocWidth << _(" height=") << allocHeight);
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

TextureChunk::TextureChunk(uint32_t w, uint32_t h)
{
	width=w;
	height=h;
	if(w==0 || h==0)
	{
		chunks=NULL;
		return;
	}
	const uint32_t blocksW=(w+127)/128;
	const uint32_t blocksH=(h+127)/128;
	chunks=new uint32_t[blocksW*blocksH];
}

TextureChunk::TextureChunk(const TextureChunk& r):texId(0),chunks(NULL),width(r.width),height(r.height)
{
	assert(chunks==NULL);
	assert(width==0 && height==0);
	assert(texId==0);
	return;
}

TextureChunk& TextureChunk::operator=(const TextureChunk& r)
{
	if(chunks)
	{
		//We were already initialized, so first clean up
		sys->getRenderThread()->releaseTexture(*this);
		delete[] chunks;
	}
	width=r.width;
	height=r.height;
	uint32_t blocksW=(width+127)/128;
	uint32_t blocksH=(height+127)/128;
	texId=r.texId;
	chunks=new uint32_t[blocksW*blocksH];
	memcpy(chunks, r.chunks, blocksW*blocksH*4);
	return *this;
}

TextureChunk::~TextureChunk()
{
	delete[] chunks;
}

bool TextureChunk::resizeIfLargeEnough(uint32_t w, uint32_t h)
{
	if(w==0 || h==0)
	{
		//The texture collapsed, release the resources
		sys->getRenderThread()->releaseTexture(*this);
		delete[] chunks;
		chunks=NULL;
		width=w;
		height=h;
		return true;
	}
	const uint32_t blocksW=(width+127)/128;
	const uint32_t blocksH=(height+127)/128;
	if(w<=blocksW*128 && h<=blocksH*128)
	{
		width=w;
		height=h;
		return true;
	}
	return false;
}

void CairoRenderer::sizeNeeded(uint32_t& w, uint32_t& h) const
{
	w=width;
	h=height;
}

void CairoRenderer::upload(uint8_t* data, uint32_t w, uint32_t h) const
{
	if(surfaceBytes)
		memcpy(data,surfaceBytes,w*h*4);
}

const TextureChunk& CairoRenderer::getTexture()
{
	//Verify that the texture is large enough
	if(!surface.tex.resizeIfLargeEnough(width, height))
		surface.tex=sys->getRenderThread()->allocateTexture(width, height,false);
	surface.xOffset=xOffset;
	surface.yOffset=yOffset;
	return surface.tex;
}

void CairoRenderer::uploadFence()
{
	if(surfaceBytes)
		Sheep::unlockOwner();
	delete this;
}

cairo_matrix_t CairoRenderer::MATRIXToCairo(const MATRIX& matrix)
{
	cairo_matrix_t ret;
	ret.xx=matrix.ScaleX;
	ret.xy=matrix.RotateSkew1;
	ret.yx=matrix.RotateSkew0;
	ret.yy=matrix.ScaleY;
	ret.x0=matrix.TranslateX;
	ret.y0=matrix.TranslateY;
	return ret;
}

void CairoRenderer::threadAbort()
{
	//Nothing special to be done
}

void CairoRenderer::jobFence()
{
	sys->getRenderThread()->addUploadJob(this);
}

bool CairoRenderer::cairoPathFromTokens(cairo_t* cr, const std::vector<GeomToken>& tokens, double scaleCorrection, bool skipFill)
{
	cairo_scale(cr, scaleCorrection, scaleCorrection);
	bool empty=true;
	for(uint32_t i=0;i<tokens.size();i++)
	{
		switch(tokens[i].type)
		{
			case STRAIGHT:
				cairo_line_to(cr, tokens[i].p1.x, tokens[i].p1.y);
				empty=false;
				break;
			case MOVE:
				cairo_move_to(cr, tokens[i].p1.x, tokens[i].p1.y);
				break;	
			case SET_FILL:
			{
				if(skipFill)
					break;
				if(!empty)
				{
					cairo_fill(cr);
					empty=true;
				}
				//NOTE: Destruction of the pattern happens internally by refcounting
				const FILLSTYLE& style=tokens[i].style;
				if(style.FillStyleType==SOLID_FILL)
				{
					const RGBA& color=style.Color;
					cairo_set_source_rgba (cr, color.rf(), color.gf(), color.bf(), color.af());
				}
				else if(style.FillStyleType==LINEAR_GRADIENT)
				{
					cairo_pattern_t* pattern=cairo_pattern_create_linear(-16384,0,16384,0);
					const cairo_matrix_t& pattern_mat=MATRIXToCairo(style.GradientMatrix);
					cairo_pattern_set_matrix(pattern, &pattern_mat);
					
					for(uint32_t i=0;i<style.Gradient.GradientRecords.size();i++)
					{
						double ratio=style.Gradient.GradientRecords[i].Ratio;
						ratio/=255.0f;
						const RGBA& color=style.Gradient.GradientRecords[i].Color;
						cairo_pattern_add_color_stop_rgba(pattern, ratio, color.rf(), color.gf(), color.bf(), color.af());
					} 
					cairo_set_source(cr, pattern);
				}
				else
				{
					LOG(LOG_NOT_IMPLEMENTED, "Unsupported fill style");
				}
				break;
			}
			default:
				::abort();
		}
	}
	return empty;
}

void CairoRenderer::cairoClean(cairo_t* cr) const
{
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
}

cairo_surface_t* CairoRenderer::allocateSurface()
{
	uint32_t cairoWidthStride=cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	assert(cairoWidthStride==width*4);
	assert(surfaceBytes==NULL);
	surfaceBytes=new uint8_t[cairoWidthStride*height];
	return cairo_image_surface_create_for_data(surfaceBytes, CAIRO_FORMAT_ARGB32, width, height, cairoWidthStride);
}

void CairoRenderer::execute()
{
	//Will be unlocked after uploadFence
	if(!Sheep::lockOwner())
		return;
	if(width==0 || height==0)
	{
		//Nothing to do, move on
		return;
	}
	cairo_surface_t* cairoSurface=allocateSurface();
	cairo_t* cr=cairo_create(cairoSurface);

	cairoClean(cr);

	matrix.TranslateX-=xOffset;
	matrix.TranslateY-=yOffset;
	const cairo_matrix_t& mat=MATRIXToCairo(matrix);
	cairo_transform(cr, &mat);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

	bool empty=cairoPathFromTokens(cr, tokens, scaleFactor, false);
	if(!empty)
		cairo_fill(cr);

	cairo_destroy(cr);
	cairo_surface_destroy(cairoSurface);
}

bool CairoRenderer::hitTest(const std::vector<GeomToken>& tokens, float scaleFactor, number_t x, number_t y)
{
	cairo_surface_t* cairoSurface=cairo_image_surface_create_for_data(NULL, CAIRO_FORMAT_ARGB32, 0, 0, 0);
	cairo_t* cr=cairo_create(cairoSurface);
	
	bool empty=cairoPathFromTokens(cr, tokens, scaleFactor, true);
	bool ret=false;
	if(!empty)
		ret=cairo_in_fill(cr, x, y);
	cairo_destroy(cr);
	cairo_surface_destroy(cairoSurface);
	return ret;
}
