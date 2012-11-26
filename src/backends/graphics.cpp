/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <cassert>

#include "swf.h"
#include "backends/graphics.h"
#include "logger.h"
#include "exceptions.h"
#include "backends/rendering.h"
#include "backends/config.h"
#include "compat.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/display/BitmapData.h"

using namespace lightspark;

void TextureBuffer::setAllocSize(uint32_t w, uint32_t h)
{
	if(getRenderThread()->hasNPOTTextures)
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

	setAllocSize(w,h);
	width=w;
	height=h;
	filtering=f;
	
	assert(texId==0);
	glGenTextures(1,&texId);
	assert(texId!=0);
	
	assert(filtering==GL_NEAREST || filtering==GL_LINEAR);
	
	//If the previous call has not failed these should not fail (in specs, we trust)
	glBindTexture(GL_TEXTURE_2D,texId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filtering);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filtering);
	//Wrapping should not be very useful, we use textures carefully
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	
	//Allocate the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, allocWidth, allocHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	
	glBindTexture(GL_TEXTURE_2D,0);
	
	if(GLRenderContext::handleGLErrors())
	{
		LOG(LOG_ERROR,_("OpenGL error in TextureBuffer::init"));
		throw RunTimeException("OpenGL error in TextureBuffer::init");
	}
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
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, allocWidth, allocHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
			if(GLRenderContext::handleGLErrors())
			{
				LOG(LOG_ERROR,_("OpenGL error in TextureBuffer::resize"));
				throw RunTimeException("OpenGL error in TextureBuffer::resize");
			}
		}
		width=w;
		height=h;
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
	float v1=width;
	float v2=height;
	v1/=allocWidth;
	v2/=allocHeight;
	glUniform2f(uniformLocation,v1,v2);
}

void TextureBuffer::bind()
{
	glBindTexture(GL_TEXTURE_2D,texId);
}

void TextureBuffer::unbind()
{
	glBindTexture(GL_TEXTURE_2D,0);
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

TextureChunk::TextureChunk(const TextureChunk& r):chunks(NULL),texId(0),width(r.width),height(r.height)
{
	*this = r;
	return;
}

TextureChunk& TextureChunk::operator=(const TextureChunk& r)
{
	if(chunks)
	{
		//We were already initialized, so first clean up
		getSys()->getRenderThread()->releaseTexture(*this);
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
		getSys()->getRenderThread()->releaseTexture(*this);
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

CairoRenderer::CairoRenderer(const MATRIX& _m, int32_t _x, int32_t _y, int32_t _w, int32_t _h,
		float _s, float _a, const std::vector<MaskData>& _ms)
	: IDrawable(_w, _h, _x, _y, _a, _ms), scaleFactor(_s), matrix(_m)
{
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
			tmp.x0/=scaleCorrection;
			tmp.y0/=scaleCorrection;
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
			//bitmap is always present, it may be empty though
			//Do an explicit cast, the data will not be modified
			cairo_surface_t* surface = cairo_image_surface_create_for_data ((uint8_t*)style.bitmap.getData(),
								CAIRO_FORMAT_ARGB32, style.bitmap.getWidth(), style.bitmap.getHeight(),
								cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, style.bitmap.getWidth()));

			pattern = cairo_pattern_create_for_surface(surface);
			cairo_surface_destroy(surface);

			//Make a copy to invert it
			cairo_matrix_t mat=style.Matrix;
			cairo_status_t st = cairo_matrix_invert(&mat);
			assert(st == CAIRO_STATUS_SUCCESS);
			(void)st; // silence warning about unused variable
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
			case FILL_KEEP_SOURCE:
				if(skipPaint)
					break;

				cairo_fill(cr);

				if(tokens[i].type==CLEAR_FILL)
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
			case FILL_TRANSFORM_TEXTURE:
			{
				if(skipPaint)
					break;

				cairo_matrix_t origmat;
				cairo_pattern_t* pattern;
				pattern=cairo_get_source(cr);
				cairo_pattern_get_matrix(pattern, &origmat);

				cairo_pattern_set_matrix(pattern, &tokens[i].textureTransform);

				cairo_fill(cr);

				cairo_pattern_set_matrix(pattern, &origmat);
				break;
			}
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

cairo_surface_t* CairoRenderer::allocateSurface(uint8_t*& buf)
{
	int32_t cairoWidthStride=cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	assert(cairoWidthStride==width*4);
	buf=new uint8_t[cairoWidthStride*height];
	return cairo_image_surface_create_for_data(buf, CAIRO_FORMAT_ARGB32, width, height, cairoWidthStride);
}

void CairoTokenRenderer::executeDraw(cairo_t* cr)
{
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

	cairoPathFromTokens(cr, tokens, scaleFactor, false);
}

#ifdef HAVE_NEW_GLIBMM_THREAD_API
StaticRecMutex CairoRenderer::cairoMutex;
#else
StaticRecMutex CairoRenderer::cairoMutex = GLIBMM_STATIC_REC_MUTEX_INIT;
#endif

uint8_t* CairoRenderer::getPixelBuffer()
{
	RecMutex::Lock l(cairoMutex);
	if(width==0 || height==0 || !Config::getConfig()->isRenderingEnabled())
		return NULL;

	int32_t windowWidth=getSys()->getRenderThread()->windowWidth;
	int32_t windowHeight=getSys()->getRenderThread()->windowHeight;
	//Discard stuff that it's outside the visible part
	if(xOffset >= windowWidth || yOffset >= windowHeight
		|| xOffset + width <= 0 || yOffset + height <= 0)
	{
		width=0;
		height=0;
		return NULL;
	}

	if(xOffset<0)
	{
		width+=xOffset;
		xOffset=0;
	}
	if(yOffset<0)
	{
		height+=yOffset;
		yOffset=0;
	}

	//Clip the size to the screen borders
	if((xOffset>0) && (width+xOffset) > windowWidth)
		width=windowWidth-xOffset;
	if((yOffset>0) && (height+yOffset) > windowHeight)
		height=windowHeight-yOffset;
	uint8_t* ret=NULL;
	cairo_surface_t* cairoSurface=allocateSurface(ret);

	cairo_t* cr=cairo_create(cairoSurface);
	cairo_surface_destroy(cairoSurface); /* cr has an reference to it */
	cairoClean(cr);

	//Make sure the rendering starts at 0,0 in surface coordinates
	//This also guarantees that all the shape fills in width/height pixels
	//We don't translate for negative offsets as we don't want to see what's in negative coords
	if(xOffset >= 0)
		matrix.x0-=xOffset;
	if(yOffset >= 0)
		matrix.y0-=yOffset;

	//Apply all the masks to clip the drawn part
	for(uint32_t i=0;i<masks.size();i++)
	{
		if(masks[i].maskMode != HARD_MASK)
			continue;
		masks[i].m->applyCairoMask(cr,xOffset,yOffset);
	}

	cairo_set_matrix(cr, &matrix);
	executeDraw(cr);

	cairo_surface_t* maskSurface = NULL;
	uint8_t* maskRawData = NULL;
	cairo_t* maskCr = NULL;
	int32_t maskXOffset = 0;
	int32_t maskYOffset = 0;
	//Also apply the soft masks
	for(uint32_t i=0;i<masks.size();i++)
	{
		if(masks[i].maskMode != SOFT_MASK)
			continue;
		//TODO: this may be optimized if needed
		uint8_t* maskData = masks[i].m->getPixelBuffer();
		if(maskData==NULL)
			continue;

		cairo_surface_t* tmp = cairo_image_surface_create_for_data(maskData,CAIRO_FORMAT_ARGB32,
				masks[i].m->getWidth(),masks[i].m->getHeight(),masks[i].m->getWidth()*4);
		if(maskSurface==NULL)
		{
			maskSurface = tmp;
			maskRawData = maskData;
			maskCr = cairo_create(maskSurface);
			maskXOffset = masks[i].m->getXOffset();
			maskYOffset = masks[i].m->getYOffset();
		}
		else
		{
			//We only care about alpha here, DEST_IN multiplies the two alphas
			cairo_set_operator(maskCr, CAIRO_OPERATOR_DEST_IN);
			//TODO: consider offsets
			cairo_set_source_surface(maskCr, tmp, masks[i].m->getXOffset()-maskXOffset, masks[i].m->getYOffset()-maskYOffset);
			//cairo_set_source_surface(maskCr, tmp, 0, 0);
			cairo_paint(maskCr);
			cairo_surface_destroy(tmp);
			delete[] maskData;
		}
	}
	if(maskCr)
	{
		cairo_destroy(maskCr);
		//Do a last paint with DEST_IN to apply mask
		cairo_set_operator(cr, CAIRO_OPERATOR_DEST_IN);
		cairo_set_source_surface(cr, maskSurface, maskXOffset-getXOffset(), maskYOffset-getYOffset());
		cairo_paint(cr);
		cairo_surface_destroy(maskSurface);
		delete[] maskRawData;
	}

	cairo_destroy(cr);
	return ret;
}

bool CairoTokenRenderer::hitTest(const std::vector<GeomToken>& tokens, float scaleFactor, number_t x, number_t y)
{
	cairo_surface_t* cairoSurface=cairo_image_surface_create_for_data(NULL, CAIRO_FORMAT_ARGB32, 0, 0, 0);
	cairo_t *cr=cairo_create(cairoSurface);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

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

void CairoTokenRenderer::applyCairoMask(cairo_t* cr,int32_t xOffset,int32_t yOffset) const
{
	cairo_matrix_t tmp=matrix;
	tmp.x0-=xOffset;
	tmp.y0-=yOffset;
	cairo_set_matrix(cr, &tmp);
	cairoPathFromTokens(cr, tokens, scaleFactor, true);
	cairo_clip(cr);
}

void CairoRenderer::convertBitmapWithAlphaToCairo(std::vector<uint8_t, reporter_allocator<uint8_t>>& data, uint8_t* inData, uint32_t width,
		uint32_t height, size_t* dataSize, size_t* stride)
{
	*stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	*dataSize = *stride * height;
	data.resize(*dataSize, 0);
	uint8_t* outData=&data[0];
	uint32_t* inData32 = (uint32_t*)inData;

	for(uint32_t i = 0; i < height; i++)
	{
		for(uint32_t j = 0; j < width; j++)
		{
			uint32_t* outDataPos = (uint32_t*)(outData+i*(*stride)) + j;
			*outDataPos = GINT32_FROM_BE( *(inData32+(i*width+j)) );
		}
	}
}

inline void CairoRenderer::copyRGB15To24(uint8_t* dest, uint8_t* src)
{
	// highest bit is ignored
	uint8_t r = (src[0] & 0x7C) >> 2;
	uint8_t g = ((src[0] & 0x03) << 3) + (src[1] & 0xE0 >> 5);
	uint8_t b = src[1] & 0x1F;

	r = r*255/31;
	g = g*255/31;
	b = b*255/31;

	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
}

inline void CairoRenderer::copyRGB24To24(uint8_t* dest, uint8_t* src)
{
	memcpy(dest, src, 3);
}

void CairoRenderer::convertBitmapToCairo(std::vector<uint8_t, reporter_allocator<uint8_t>>& data, uint8_t* inData, uint32_t width,
					 uint32_t height, size_t* dataSize, size_t* stride, bool rgb15)
{
	*stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	*dataSize = *stride * height;
	data.resize(*dataSize, 0);
	uint8_t* outData = &data[0];
	for(uint32_t i = 0; i < height; i++)
	{
		for(uint32_t j = 0; j < width; j++)
		{
			uint32_t* outDataPos = (uint32_t*)(outData+i*(*stride)) + j;
			uint32_t pdata = 0xFF;
			/* the alpha channel is set to opaque above */
			uint8_t* rgbData = ((uint8_t*)&pdata)+1;
			/* copy the RGB bytes to rgbData */
			if(rgb15)
				copyRGB15To24(rgbData, inData+(i*width+j)*2);
			else
				copyRGB24To24(rgbData, inData+(i*width+j)*3);
			/* cairo needs this in host endianess */
			*outDataPos = GINT32_FROM_BE(pdata);
		}
	}
}

#ifdef HAVE_NEW_GLIBMM_THREAD_API
StaticMutex CairoPangoRenderer::pangoMutex;
#else
StaticMutex CairoPangoRenderer::pangoMutex = GLIBMM_STATIC_MUTEX_INIT;
#endif

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
		default:
			assert(false);
			return; // silence warning about uninitialised alignment
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

void CairoPangoRenderer::applyCairoMask(cairo_t* cr, int32_t xOffset, int32_t yOffset) const
{
	assert(false);
}

AsyncDrawJob::AsyncDrawJob(IDrawable* d, _R<DisplayObject> o):drawable(d),owner(o),surfaceBytes(NULL),uploadNeeded(false)
{
}

AsyncDrawJob::~AsyncDrawJob()
{
	delete drawable;
	delete[] surfaceBytes;
}

void AsyncDrawJob::execute()
{
	surfaceBytes=drawable->getPixelBuffer();
	if(surfaceBytes)
		uploadNeeded=true;
}

void AsyncDrawJob::threadAbort()
{
	//Nothing special to be done
}

void AsyncDrawJob::jobFence()
{
	//If the data must be uploaded (there were no errors) the Job add itself to the upload queue.
	//Otherwise it destroys itself
	if(uploadNeeded)
		getSys()->getRenderThread()->addUploadJob(this);
	else
		delete this;
}

void AsyncDrawJob::upload(uint8_t* data, uint32_t w, uint32_t h) const
{
	assert(surfaceBytes);
	memcpy(data, surfaceBytes, w*h*4);
}

void AsyncDrawJob::sizeNeeded(uint32_t& w, uint32_t& h) const
{
	w=drawable->getWidth();
	h=drawable->getHeight();
}

const TextureChunk& AsyncDrawJob::getTexture()
{
	/* This is called in the render thread,
	 * so we need no locking for surface */
	CachedSurface& surface=owner->cachedSurface;
	uint32_t width=drawable->getWidth();
	uint32_t height=drawable->getHeight();
	//Verify that the texture is large enough
	if(!surface.tex.resizeIfLargeEnough(width, height))
		surface.tex=getSys()->getRenderThread()->allocateTexture(width, height,false);
	surface.xOffset=drawable->getXOffset();
	surface.yOffset=drawable->getYOffset();
	surface.alpha=drawable->getAlpha();
	return surface.tex;
}

void AsyncDrawJob::uploadFence()
{
	delete this;
}

void SoftwareInvalidateQueue::addToInvalidateQueue(_R<DisplayObject> d)
{
	queue.emplace_back(d);
}
