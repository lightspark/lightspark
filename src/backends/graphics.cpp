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

#include <assert.h>

#include "swf.h"
#include "graphics.h"
#include "logger.h"
#include "exceptions.h"
#include "backends/rendering.h"
#include "glmatrices.h"
#include "compat.h"
#include "scripting/flash/text/flashtext.h"

#include <iostream>

using namespace lightspark;

void lightspark::cleanGLErrors()
{
#ifdef EXPENSIVE_DEBUG
	int errorCount = 0;
	GLenum err;
	while(1)
	{
		err=glGetError();
		if(err!=GL_NO_ERROR)
		{
			errorCount++;
			LOG(LOG_ERROR,_("GL error ")<< err);
		}
		else
			break;
	}

	if(errorCount)
	{
		LOG(LOG_ERROR,_("Ignoring ") << errorCount << _(" openGL errors"));
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
		init(w, h, f);
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
				throw RunTimeException("GL_INVALID_VALUE in TextureBuffer::resize");
			}
		}
		width=w;
		height=h;
#ifdef EXPENSIVE_DEBUG
		cleanGLErrors();
#endif
	}
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
	lsglPushMatrix();

	//TODO: implement smart stack flush
	//Save all the current stack, compute using SSE the final matrix and push that one
	//Maybe mulps, shuffle and parallel add
	//On unapply the stack will be reset as before
}

MatrixApplier::MatrixApplier(const MATRIX& m)
{
	//First of all try to preserve current matrix
	lsglPushMatrix();

	float matrix[16];
	m.get4DMatrix(matrix);
	lsglMultMatrixf(matrix);
	rt->setMatrixUniform(LSGL_MODELVIEW);
}

void MatrixApplier::concat(const MATRIX& m)
{
	float matrix[16];
	m.get4DMatrix(matrix);
	lsglMultMatrixf(matrix);
	rt->setMatrixUniform(LSGL_MODELVIEW);
}

void MatrixApplier::unapply()
{
	lsglPopMatrix();
	rt->setMatrixUniform(LSGL_MODELVIEW);
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
	const uint32_t blocksW=(w+CHUNKSIZE-1)/CHUNKSIZE;
	const uint32_t blocksH=(h+CHUNKSIZE-1)/CHUNKSIZE;
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
	uint32_t blocksW=(width+CHUNKSIZE-1)/CHUNKSIZE;
	uint32_t blocksH=(height+CHUNKSIZE-1)/CHUNKSIZE;
	texId=r.texId;
	if(r.chunks)
	{
		chunks=new uint32_t[blocksW*blocksH];
		memcpy(chunks, r.chunks, blocksW*blocksH*4);
	}
	else
		chunks=NULL;
	return *this;
}

TextureChunk::~TextureChunk()
{
	delete[] chunks;
}

void TextureChunk::makeEmpty()
{
	width=0;
	height=0;
	texId=0;
	delete[] chunks;
	chunks=NULL;
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
	const uint32_t blocksW=(width+CHUNKSIZE-1)/CHUNKSIZE;
	const uint32_t blocksH=(height+CHUNKSIZE-1)/CHUNKSIZE;
	if(w<=blocksW*CHUNKSIZE && h<=blocksH*CHUNKSIZE)
	{
		width=w;
		height=h;
		return true;
	}
	return false;
}

CairoRenderer::CairoRenderer(ASObject* _o, CachedSurface& _t, const MATRIX& _m,
		int32_t _x, int32_t _y, int32_t _w, int32_t _h, float _s, float _a)
	: owner(_o),surface(_t),matrix(_m),xOffset(_x),yOffset(_y),alpha(_a),width(_w),height(_h),
	surfaceBytes(NULL),scaleFactor(_s),uploadNeeded(true)
{
	owner->incRef();
}

CairoRenderer::~CairoRenderer()
{
	delete[] surfaceBytes;
	owner->decRef();
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
	/* This is called in the render thread,
	 * so we need no locking for surface */
	//Verify that the texture is large enough
	if(!surface.tex.resizeIfLargeEnough(width, height))
		surface.tex=sys->getRenderThread()->allocateTexture(width, height,false);
	surface.xOffset=xOffset;
	surface.yOffset=yOffset;
	surface.alpha=alpha;
	return surface.tex;
}

void CairoRenderer::uploadFence()
{
	delete this;
}

cairo_matrix_t CairoRenderer::MATRIXToCairo(const MATRIX& matrix)
{
	cairo_matrix_t ret;

	cairo_matrix_init(&ret,
	                  matrix.ScaleX, matrix.RotateSkew0,
	                  matrix.RotateSkew1, matrix.ScaleY,
	                  matrix.TranslateX, matrix.TranslateY);

	return ret;
}

void CairoRenderer::threadAbort()
{
	//Nothing special to be done
}

void CairoRenderer::jobFence()
{
	//If the data must be uploaded (there were no errors) the Job add itself to the upload queue.
	//Otherwise it destroys itself
	if(uploadNeeded)
		sys->getRenderThread()->addUploadJob(this);
	else
		delete this;
}

void CairoTokenRenderer::quadraticBezier(cairo_t* cr, double control_x, double control_y, double end_x, double end_y)
{
	double start_x, start_y;
	cairo_get_current_point(cr, &start_x, &start_y);
	double control_1x = control_x*(2.0/3.0) + start_x*(1.0/3.0);
	double control_1y = control_y*(2.0/3.0) + start_y*(1.0/3.0);
	double control_2x = control_x*(2.0/3.0) + end_x*(1.0/3.0);
	double control_2y = control_y*(2.0/3.0) + end_y*(1.0/3.0);
	cairo_curve_to(cr,
	               control_1x, control_1y,
	               control_2x, control_2y,
	               end_x, end_y);
}

cairo_pattern_t* CairoTokenRenderer::FILLSTYLEToCairo(const FILLSTYLE& style, double scaleCorrection)
{
	cairo_pattern_t* pattern = NULL;

	switch(style.FillStyleType)
	{
		case SOLID_FILL:
		{
			const RGBA& color = style.Color;
			pattern = cairo_pattern_create_rgba(color.rf(), color.gf(),
			                                    color.bf(), color.af());
			break;
		}
		case LINEAR_GRADIENT:
		case RADIAL_GRADIENT:
		{
			const GRADIENT& grad = style.Gradient;

			// We want an opaque black background... a gradient with no stops
			// in cairo will give us transparency.
			if (grad.GradientRecords.size() == 0)
			{
				pattern = cairo_pattern_create_rgb(0, 0, 0);
				return pattern;
			}

			MATRIX tmp=style.Matrix;
			tmp.TranslateX/=scaleCorrection;
			tmp.TranslateY/=scaleCorrection;
			// The dimensions of the pattern space are specified in SWF specs
			// as a 32768x32768 box centered at (0,0)
			if (style.FillStyleType == LINEAR_GRADIENT)
			{
				double x0,y0,x1,y1;
				tmp.multiply2D(-16384.0, 0,x0,y0);
				tmp.multiply2D(16384.0, 0,x1,y1);
				pattern = cairo_pattern_create_linear(x0,y0,x1,y1);
			}
			else
			{
				double x0,y0; //Center of the circles
				double x1,y1; //Point on the circle edge at 0°
				tmp.multiply2D(0, 0,x0,y0);
				tmp.multiply2D(16384.0, 0,x1,y1);
				double radius=sqrt(x1*x1+y1*y1);
				pattern = cairo_pattern_create_radial(x0, y0, 0, x0, y0, radius);
			}

			if (grad.SpreadMode == 0) // PAD
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
			else if (grad.SpreadMode == 1) // REFLECT
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT);
			else if (grad.SpreadMode == 2) // REPEAT
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

			for(uint32_t i=0;i<grad.GradientRecords.size();i++)
			{
				double ratio = grad.GradientRecords[i].Ratio / 255.0;
				const RGBA& color=style.Gradient.GradientRecords[i].Color;

				cairo_pattern_add_color_stop_rgba(pattern, ratio,
				    color.rf(), color.gf(), color.bf(), color.af());
			}
			break;
		}

		case NON_SMOOTHED_REPEATING_BITMAP:
		case NON_SMOOTHED_CLIPPED_BITMAP:
		case REPEATING_BITMAP:
		case CLIPPED_BITMAP:
		{
			if(style.bitmap==NULL)
				throw RunTimeException("Invalid bitmap");

			IntSize size = style.bitmap->getBitmapSize();

			cairo_surface_t* surface = cairo_image_surface_create_for_data (style.bitmap->data,
										CAIRO_FORMAT_ARGB32, size.width, size.height,
										cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, size.width));

			pattern = cairo_pattern_create_for_surface(surface);
			cairo_surface_destroy(surface);

			cairo_matrix_t mat = MATRIXToCairo(style.Matrix);
			cairo_status_t st = cairo_matrix_invert(&mat);
			assert(st == CAIRO_STATUS_SUCCESS);
			mat.x0 /= scaleCorrection;
			mat.y0 /= scaleCorrection;

			cairo_pattern_set_matrix (pattern, &mat);
			assert(cairo_pattern_status(pattern) == CAIRO_STATUS_SUCCESS);

			if(style.FillStyleType == NON_SMOOTHED_REPEATING_BITMAP ||
				style.FillStyleType == REPEATING_BITMAP)
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
			else
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);

			if(style.FillStyleType == NON_SMOOTHED_REPEATING_BITMAP ||
			   style.FillStyleType == NON_SMOOTHED_CLIPPED_BITMAP)
				cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
			else
				cairo_pattern_set_filter(pattern, CAIRO_FILTER_BILINEAR);
			break;
		}
		default:
			LOG(LOG_NOT_IMPLEMENTED, "Unsupported fill style " << (int)style.FillStyleType);
			return NULL;
	}

	return pattern;
}

bool CairoTokenRenderer::cairoPathFromTokens(cairo_t* cr, const std::vector<GeomToken>& tokens, double scaleCorrection, bool skipPaint)
{
	cairo_scale(cr, scaleCorrection, scaleCorrection);

	bool empty=true;

	cairo_t *stroke_cr = cairo_create(cairo_get_group_target(cr));
	cairo_push_group(stroke_cr);

	// Make sure not to draw anything until a fill is set.
	cairo_set_operator(stroke_cr, CAIRO_OPERATOR_DEST);
	cairo_set_operator(cr, CAIRO_OPERATOR_DEST);

	#define PATH(operation, args...) \
		operation(cr, ## args); \
		operation(stroke_cr, ## args);

	for(uint32_t i=0;i<tokens.size();i++)
	{
		switch(tokens[i].type)
		{
			case MOVE:
				PATH(cairo_move_to, tokens[i].p1.x, tokens[i].p1.y);
				break;
			case STRAIGHT:
				PATH(cairo_line_to, tokens[i].p1.x, tokens[i].p1.y);
				empty = false;
				break;
			case CURVE_QUADRATIC:
				PATH(quadraticBezier,
				   tokens[i].p1.x, tokens[i].p1.y,
				   tokens[i].p2.x, tokens[i].p2.y);
				empty = false;
				break;
			case CURVE_CUBIC:
				PATH(cairo_curve_to,
				   tokens[i].p1.x, tokens[i].p1.y,
				   tokens[i].p2.x, tokens[i].p2.y,
				   tokens[i].p3.x, tokens[i].p3.y);
				empty = false;
				break;
			case SET_FILL:
			{
				if(skipPaint)
					break;

				cairo_fill(cr);

				cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

				const FILLSTYLE& style = tokens[i].fillStyle;
				cairo_pattern_t* pattern = FILLSTYLEToCairo(style, scaleCorrection);
				if(pattern)
				{
					cairo_set_source(cr, pattern);
					// Destroy the first reference.
					cairo_pattern_destroy(pattern);
				}

				break;
			}
			case SET_STROKE:
			{
				if(skipPaint)
					break;

				cairo_stroke(stroke_cr);

				const LINESTYLE2& style = tokens[i].lineStyle;
				const RGBA& color = style.Color;

				cairo_set_operator(stroke_cr, CAIRO_OPERATOR_OVER);
				cairo_set_source_rgba(stroke_cr, color.rf(), color.gf(), color.bf(), color.af());

				// TODO: EndCapStyle
				if (style.StartCapStyle == 0)
					cairo_set_line_cap(stroke_cr, CAIRO_LINE_CAP_ROUND);
				else if (style.StartCapStyle == 1)
					cairo_set_line_cap(stroke_cr, CAIRO_LINE_CAP_BUTT);
				else if (style.StartCapStyle == 2)
					cairo_set_line_cap(stroke_cr, CAIRO_LINE_CAP_SQUARE);

				if (style.JointStyle == 0)
					cairo_set_line_join(stroke_cr, CAIRO_LINE_JOIN_ROUND);
				else if (style.JointStyle == 1)
					cairo_set_line_join(stroke_cr, CAIRO_LINE_JOIN_BEVEL);
				else if (style.JointStyle == 2) {
					cairo_set_line_join(stroke_cr, CAIRO_LINE_JOIN_MITER);
					cairo_set_miter_limit(stroke_cr, style.MiterLimitFactor);
				}

				cairo_set_line_width(stroke_cr, (double)(style.Width / 20.0));
				break;
			}

			case CLEAR_FILL:
				if(skipPaint)
					break;

				cairo_fill(cr);

				// Clear source.
				cairo_set_operator(cr, CAIRO_OPERATOR_DEST);
				break;
			case CLEAR_STROKE:
				if(skipPaint)
					break;

				cairo_stroke(stroke_cr);

				// Clear source.
				cairo_set_operator(stroke_cr, CAIRO_OPERATOR_DEST);
				break;
			default:
				assert(false);
		}
	}

	#undef PATH

	if(!skipPaint)
	{
		cairo_fill(cr);
		cairo_stroke(stroke_cr);
	}

	cairo_pattern_t *stroke_pattern = cairo_pop_group(stroke_cr);
	cairo_set_source(cr, stroke_pattern);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	if(!skipPaint)
		cairo_paint(cr);

	cairo_pattern_destroy(stroke_pattern);
	cairo_destroy(stroke_cr);

	return empty;
}

void CairoRenderer::cairoClean(cairo_t* cr)
{
	cairo_set_source_rgba(cr, 0, 0, 0, 0);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
}

cairo_surface_t* CairoRenderer::allocateSurface()
{
	int32_t cairoWidthStride=cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	assert(cairoWidthStride==width*4);
	assert(surfaceBytes==NULL);
	surfaceBytes=new uint8_t[cairoWidthStride*height];
	return cairo_image_surface_create_for_data(surfaceBytes, CAIRO_FORMAT_ARGB32, width, height, cairoWidthStride);
}

void CairoTokenRenderer::executeDraw(cairo_t* cr)
{
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

	cairoPathFromTokens(cr, tokens, scaleFactor, false);
}

void CairoRenderer::execute()
{
	if(width==0 || height==0)
	{
		uploadNeeded = false;
		return;
	}

	int32_t windowWidth=sys->getRenderThread()->windowWidth;
	int32_t windowHeight=sys->getRenderThread()->windowHeight;
	//Discard stuff that it's outside the visible part
	if(xOffset >= windowWidth || yOffset >= windowHeight
		|| xOffset + width <= 0 || yOffset + height <= 0)
	{
		uploadNeeded = false;
		return;
	}

	//TODO:clip on the right and bottom also
	if(xOffset<0)
		width+=xOffset;
	if(yOffset<0)
		height+=yOffset;

	//Clip the size to the screen borders
	if((xOffset>=0) && (width+xOffset) > windowWidth)
		width=windowWidth-xOffset;
	if((yOffset>0) && (height+yOffset) > windowHeight)
		height=windowHeight-yOffset;
	cairo_surface_t* cairoSurface=allocateSurface();

	cairo_t* cr=cairo_create(cairoSurface);
	cairo_surface_destroy(cairoSurface); /* cr has an reference to it */
	cairoClean(cr);

	//Make sure the rendering starts at 0,0 in surface coordinates
	//This also guarantees that all the shape fills in width/height pixels
	//We don't translate for negative offsets as we don't want to see what's in negative coords
	if(xOffset >= 0)
		matrix.TranslateX-=xOffset;
	if(yOffset >= 0)
		matrix.TranslateY-=yOffset;
	const cairo_matrix_t& mat=MATRIXToCairo(matrix);
	cairo_transform(cr, &mat);

	executeDraw(cr);

	cairo_destroy(cr);
}

bool CairoTokenRenderer::hitTest(const std::vector<GeomToken>& tokens, float scaleFactor, number_t x, number_t y)
{
	cairo_surface_t* cairoSurface=cairo_image_surface_create_for_data(NULL, CAIRO_FORMAT_ARGB32, 0, 0, 0);

	cairo_t *cr=cairo_create(cairoSurface);
	bool empty=cairoPathFromTokens(cr, tokens, scaleFactor, true);
	bool ret=false;
	if(!empty)
	{
		/* reset the matrix so x and y are not scaled
		 * by the current cairo transformation
		 */
		cairo_identity_matrix(cr);
		ret=cairo_in_fill(cr, x, y);
	}
	cairo_destroy(cr);
	cairo_surface_destroy(cairoSurface);
	return ret;
}

bool CairoTokenRenderer::isOpaque(const std::vector<GeomToken>& tokens, float scaleFactor, number_t x, number_t y)
{
	//We render the alpha value of a single pixel, hopefully this is not too slow
	int32_t cairoWidthStride=cairo_format_stride_for_width(CAIRO_FORMAT_A8, 1);
	uint8_t* pixelBytes=new uint8_t[cairoWidthStride];
	cairo_surface_t* cairoSurface=cairo_image_surface_create_for_data(pixelBytes, CAIRO_FORMAT_A8, 1, 1, cairoWidthStride);

	cairo_t* cr=cairo_create(cairoSurface);
	cairoClean(cr);

	//Make sure the rendering starts at 0,0 in surface coordinates
	cairo_translate(cr, -x, -y);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

	cairoPathFromTokens(cr, tokens, scaleFactor, false);

	cairo_destroy(cr);
	cairo_surface_destroy(cairoSurface);
	//CHECK: the condition is alpha > 0x00 or alpha==0xff
	return pixelBytes[0]!=0x00;
}

uint8_t* CairoRenderer::convertBitmapWithAlphaToCairo(uint8_t* inData, uint32_t width, uint32_t height)
{
	uint32_t stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	uint8_t* outData = new uint8_t[stride * height];
	uint32_t* inData32 = (uint32_t*)inData;

	for(uint32_t i = 0; i < height; i++)
	{
		for(uint32_t j = 0; j < width; j++)
		{
			uint32_t* outDataPos = (uint32_t*)(outData+i*stride) + j;
			*outDataPos = BigEndianToHost32( *(inData32+(i*width+j)) );
		}
	}
	return outData;
}

uint8_t* CairoRenderer::convertBitmapToCairo(uint8_t* inData, uint32_t width, uint32_t height)
{
	uint32_t stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	uint8_t* outData = new uint8_t[stride * height];
	for(uint32_t i = 0; i < height; i++)
	{
		for(uint32_t j = 0; j < width; j++)
		{
			uint32_t* outDataPos = (uint32_t*)(outData+i*stride) + j;
			uint32_t pdata = 0xFF;
			/* the alpha channel is set to zero above */
			uint8_t* rgbData = ((uint8_t*)&pdata)+1;
			/* copy the RGB bytes to rgbData */
			memcpy(rgbData, inData+(i*width+j)*3, 3);
			/* cairo needs this in host endianess */
			*outDataPos = BigEndianToHost32(pdata);
		}
	}
	return outData;
}

Mutex CairoPangoRenderer::pangoMutex("pangoMutex");

void CairoPangoRenderer::pangoLayoutFromData(PangoLayout* layout, const TextData& tData)
{
	PangoFontDescription* desc;

	pango_layout_set_text(layout, tData.text.raw_buf(), -1);

	/* setup alignment */
	PangoAlignment alignment;

	switch(tData.autoSize)
	{
		case TextData::AUTO_SIZE::AS_NONE://TODO:check
		case TextData::AUTO_SIZE::AS_LEFT:
		{
			alignment = PANGO_ALIGN_LEFT;
			break;
		}
		case TextData::AUTO_SIZE::AS_RIGHT:
		{
			alignment = PANGO_ALIGN_RIGHT;
			break;
		}
		case TextData::AUTO_SIZE::AS_CENTER:
		{
			alignment = PANGO_ALIGN_CENTER;
			break;
		}
	}
	pango_layout_set_alignment(layout,alignment);

	//In case wordWrap is true, we already have the right width
	if(tData.wordWrap == true)
	{
		pango_layout_set_width(layout,PANGO_SCALE*tData.width);
		pango_layout_set_wrap(layout,PANGO_WRAP_WORD);//I think this is what Adobe does
	}
	//In case autoSize is NONE, we also have the height
	if(tData.autoSize == TextData::AUTO_SIZE::AS_NONE)
	{
		pango_layout_set_width(layout,PANGO_SCALE*tData.width);
		pango_layout_set_height(layout,PANGO_SCALE*tData.height);//TODO:Not sure what Pango does if the text is too long to fit
	}

	/* setup font description */
	desc = pango_font_description_new();
	pango_font_description_set_family(desc, tData.font.raw_buf());
	pango_font_description_set_size(desc, PANGO_SCALE*tData.fontSize);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
}

void CairoPangoRenderer::executeDraw(cairo_t* cr)
{
	/* TODO: pango is not fully thread-safe,
	 * but we may be able to use finer grained locking.
	 */
	Locker l(pangoMutex);
	PangoLayout* layout;

	layout = pango_cairo_create_layout(cr);
	pangoLayoutFromData(layout, textData);

	if(textData.background)
	{
		cairo_set_source_rgb (cr, textData.backgroundColor.Red, textData.backgroundColor.Green, textData.backgroundColor.Blue);
		cairo_paint(cr);
	}
	cairo_set_source_rgb (cr, textData.textColor.Red, textData.textColor.Green, textData.textColor.Blue);

	/* draw the text */
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}

bool CairoPangoRenderer::getBounds(const TextData& _textData, uint32_t& w, uint32_t& h, uint32_t& tw, uint32_t& th)
{
	//TODO:check locking
	Locker l(pangoMutex);
	cairo_surface_t* cairoSurface=cairo_image_surface_create_for_data(NULL, CAIRO_FORMAT_ARGB32, 0, 0, 0);
	cairo_t *cr=cairo_create(cairoSurface);

	PangoLayout* layout;

	layout = pango_cairo_create_layout(cr);
	pangoLayoutFromData(layout, _textData);

	PangoRectangle ink_rect, logical_rect;
	pango_layout_get_pixel_extents(layout,&ink_rect,&logical_rect);//TODO: check the rounding during pango conversion

	g_object_unref(layout);
	cairo_destroy(cr);
	cairo_surface_destroy(cairoSurface);

	//This should be safe check precision
	tw = ink_rect.width;
	th = ink_rect.height;
	if(_textData.autoSize != TextData::AUTO_SIZE::AS_NONE)
	{
		h = logical_rect.height;
		if(!_textData.wordWrap)
			w = logical_rect.width;
	}

	return (h!=0) && (w!=0);
}
