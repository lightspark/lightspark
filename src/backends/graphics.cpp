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

#include <cassert>

#include "swf.h"
#include "abc.h"
#include "backends/graphics.h"
#include "logger.h"
#include "exceptions.h"
#include "backends/rendering.h"
#include "backends/config.h"
#include "compat.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/display/BitmapData.h"
#include <pango/pangocairo.h>

using namespace lightspark;

TextureChunk::TextureChunk(uint32_t w, uint32_t h)
{
	width=w+(w/CHUNKSIZE_REAL)*2;
	height=h+(h/CHUNKSIZE_REAL)*2;
	if(w==0 || h==0)
	{
		chunks=nullptr;
		return;
	}
	const uint32_t blocksW=(width+CHUNKSIZE-1)/CHUNKSIZE;
	const uint32_t blocksH=(height+CHUNKSIZE-1)/CHUNKSIZE;
	chunks=new uint32_t[blocksW*blocksH];
}

TextureChunk::TextureChunk(const TextureChunk& r):chunks(nullptr),texId(0),width(r.width),height(r.height)
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
		chunks=nullptr;
	return *this;
}

TextureChunk::~TextureChunk()
{
	if (chunks)
		delete[] chunks;
	chunks=nullptr;
}

void TextureChunk::makeEmpty()
{
	width=0;
	height=0;
	texId=0;
	if (chunks)
		delete[] chunks;
	chunks=nullptr;
}

bool TextureChunk::resizeIfLargeEnough(uint32_t w, uint32_t h)
{
	if(w==0 || h==0)
	{
		//The texture collapsed, release the resources
		getSys()->getRenderThread()->releaseTexture(*this);
		delete[] chunks;
		chunks=nullptr;
		width=w;
		height=h;
		return true;
	}
	const uint32_t blocksW=(width+CHUNKSIZE-1)/CHUNKSIZE;
	const uint32_t blocksH=(height+CHUNKSIZE-1)/CHUNKSIZE;
	w += (w/CHUNKSIZE_REAL+1)*2;
	h += (h/CHUNKSIZE_REAL+1)*2;
	if(w<=blocksW*CHUNKSIZE && h<=blocksH*CHUNKSIZE)
	{
		width=w;
		height=h;
		return true;
	}
	return false;
}

CairoRenderer::CairoRenderer(const MATRIX& _m, int32_t _x, int32_t _y, int32_t _w, int32_t _h, int32_t _rx, int32_t _ry, int32_t _rw, int32_t _rh, float _r, float _xs, float _ys, bool _im, bool _hm,
		float _s, float _a, const std::vector<MaskData>& _ms,
		float _redMultiplier,float _greenMultiplier,float _blueMultiplier,float _alphaMultiplier,
		float _redOffset,float _greenOffset,float _blueOffset,float _alphaOffset,
		bool _smoothing, number_t _xstart, number_t _ystart)
	: IDrawable(_w, _h, _x, _y, _rw, _rh, _rx, _ry, _r, _xs, _ys, _im, _hm,_a, _ms,
				_redMultiplier,_greenMultiplier,_blueMultiplier,_alphaMultiplier,
				_redOffset,_greenOffset,_blueOffset,_alphaOffset)
	, scaleFactor(_s),smoothing(_smoothing), matrix(_m),xstart(_xstart),ystart(_ystart)
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

cairo_pattern_t* CairoTokenRenderer::FILLSTYLEToCairo(const FILLSTYLE& style, double scaleCorrection, float scalex, float scaley, bool isMask)
{
	cairo_pattern_t* pattern = nullptr;

	switch(style.FillStyleType)
	{
		case SOLID_FILL:
		{
			const RGBA& color = style.Color;
			pattern = cairo_pattern_create_rgba(color.rf(), color.gf(),color.bf(), isMask ? 1.0 : color.af());
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
			tmp.x0 -= style.ShapeBounds.Xmin/20;
			tmp.y0 -= style.ShapeBounds.Ymin/20;
			cairo_matrix_scale(&tmp,scalex,scaley);
			tmp.x0/=scaleCorrection;
			tmp.y0/=scaleCorrection;
			tmp.x0*=scalex;
			tmp.y0*=scaley;
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
				double radius=x1-x0;
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
				const RGBA& color=grad.GradientRecords[i].Color;
				cairo_pattern_add_color_stop_rgba(pattern, ratio,color.rf(), color.gf(),color.bf(), color.af());
			}
			break;
		}

		case NON_SMOOTHED_REPEATING_BITMAP:
		case NON_SMOOTHED_CLIPPED_BITMAP:
		case REPEATING_BITMAP:
		case CLIPPED_BITMAP:
		{
			_NR<BitmapContainer> bm(style.bitmap);
			if(bm.isNull())
				return nullptr;
			if (!style.Matrix.isInvertible())
				return nullptr;

			cairo_surface_t* surface = nullptr;
			//Do an explicit cast, the data will not be modified
			surface = cairo_image_surface_create_for_data ((uint8_t*)bm->getData(),
									CAIRO_FORMAT_ARGB32,
									bm->getWidth(),
									bm->getHeight(),
									cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bm->getWidth()));

			pattern = cairo_pattern_create_for_surface(surface);
			cairo_surface_destroy(surface);

			//Make a copy to invert it
			cairo_matrix_t mat=style.Matrix;
			mat.x0 -= style.ShapeBounds.Xmin/20;
			mat.y0 -= style.ShapeBounds.Ymin/20;
			cairo_matrix_scale(&mat,scalex,scaley);
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
			return nullptr;
	}

	return pattern;
}

bool CairoTokenRenderer::cairoPathFromTokens(cairo_t* cr, const tokensVector& tokens, double scaleCorrection, bool skipPaint,float scalex,float scaley,number_t xstart, number_t ystart,bool isMask)
{
	cairo_scale(cr, scaleCorrection, scaleCorrection);

	bool empty=true;

	cairo_t *stroke_cr = cairo_create(cairo_get_group_target(cr));
	cairo_matrix_t mat;
	cairo_get_matrix(cr,&mat);
	cairo_set_matrix(stroke_cr, &mat);
	cairo_push_group(stroke_cr);

	// Make sure not to draw anything until a fill is set.
	cairo_set_operator(stroke_cr, CAIRO_OPERATOR_DEST);
	cairo_set_operator(cr, CAIRO_OPERATOR_DEST);

	#define PATH(operation, args...) \
		operation(instroke?stroke_cr:cr, ## args);

	bool instroke = false;
	int tokentype = 1;
	while (tokentype)
	{
		std::vector<_NR<GeomToken>, reporter_allocator<_NR<GeomToken>>>::const_iterator it;
		std::vector<_NR<GeomToken>, reporter_allocator<_NR<GeomToken>>>::const_iterator itend;
		switch(tokentype)
		{
			case 1:
				it = tokens.filltokens.begin();
				itend = tokens.filltokens.end();
				tokentype++;
				break;
			case 2:
				it = tokens.stroketokens.begin();
				itend = tokens.stroketokens.end();
				tokentype++;
				break;
			default:
				tokentype = 0;
				break;
		}
		if (tokentype == 0)
			break;
		for (;it != itend; it++)
		{
			switch((*it)->type)
			{
				case MOVE:
					PATH(cairo_move_to, (*it)->p1.x*scalex-xstart*scalex, (*it)->p1.y*scaley-ystart*scaley);
					break;
				case STRAIGHT:
					PATH(cairo_line_to, (*it)->p1.x*scalex-xstart*scalex, (*it)->p1.y*scaley-ystart*scaley);
					empty = false;
					break;
				case CURVE_QUADRATIC:
					PATH(quadraticBezier,
					   (*it)->p1.x*scalex-xstart*scalex, (*it)->p1.y*scaley-ystart*scaley,
					   (*it)->p2.x*scalex-xstart*scalex, (*it)->p2.y*scaley-ystart*scaley);
					empty = false;
					break;
				case CURVE_CUBIC:
					PATH(cairo_curve_to,
					   (*it)->p1.x*scalex-xstart*scalex, (*it)->p1.y*scaley-ystart*scaley,
					   (*it)->p2.x*scalex-xstart*scalex, (*it)->p2.y*scaley-ystart*scaley,
					   (*it)->p3.x*scalex-xstart*scalex, (*it)->p3.y*scaley-ystart*scaley);
					empty = false;
					break;
				case SET_FILL:
				{
					if(skipPaint)
						break;
	
					cairo_fill(cr);
	
					cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	
					const FILLSTYLE& style = (*it)->fillStyle;
					if (style.FillStyleType == NON_SMOOTHED_CLIPPED_BITMAP ||
						style.FillStyleType == NON_SMOOTHED_REPEATING_BITMAP)
						cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
					cairo_pattern_t* pattern = FILLSTYLEToCairo(style, scaleCorrection,scalex,scaley,isMask);
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
					instroke = true;
					if(skipPaint)
						break;
	
					cairo_stroke(stroke_cr);
	
					const LINESTYLE2& style = (*it)->lineStyle;
	
					cairo_set_operator(stroke_cr, CAIRO_OPERATOR_OVER);
					if (style.HasFillFlag)
					{
						if (style.FillType.FillStyleType == NON_SMOOTHED_CLIPPED_BITMAP ||
							style.FillType.FillStyleType == NON_SMOOTHED_REPEATING_BITMAP)
							cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
						cairo_pattern_t* pattern = FILLSTYLEToCairo(style.FillType, scaleCorrection,scalex,scaley,isMask);
						if (pattern)
						{
							cairo_set_source(stroke_cr, pattern);
							cairo_pattern_destroy(pattern);
						}
					} else {
						const RGBA& color = style.Color;
						float r,g,b,a;
						r = color.rf();
						g = color.gf();
						b = color.bf();
						a = isMask ? 1.0 : color.af();
						cairo_set_source_rgba(stroke_cr, r, g, b, a);
					}
	
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
	
					//Width == 0 should be a hairline, but
					//cairo does not support hairlines.
					//Line width 1 is not a perfect
					//substitute, because it is affected
					//by transformations.
					if (style.Width == 0)
						cairo_set_line_width(stroke_cr, 1);
					else if (style.Width < 20) // would result in line with < 1 what seems to lead to no rendering at all
						cairo_set_line_width(stroke_cr, 5);
					else 
					{
						double linewidth = (double)(style.Width / 20.0 / scaleCorrection);
						//cairo_device_to_user_distance(stroke_cr, &linewidth, &linewidth);
						cairo_set_line_width(stroke_cr, linewidth);
					}
					break;
				}
	
				case CLEAR_FILL:
				case FILL_KEEP_SOURCE:
					if(skipPaint)
						break;
	
					cairo_fill(cr);
	
					if((*it)->type==CLEAR_FILL)
						// Clear source.
						cairo_set_operator(cr, CAIRO_OPERATOR_DEST);
					break;
				case CLEAR_STROKE:
					instroke = false;
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
	
					cairo_pattern_set_matrix(pattern, &(*it)->textureTransform);
	
					cairo_fill(cr);
	
					cairo_pattern_set_matrix(pattern, &origmat);
					break;
				}
				default:
					assert(false);
			}
		}
	}

	#undef PATH

	if(!skipPaint)
	{
		cairo_fill(cr);
		if (instroke)
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

void CairoTokenRenderer::executeDraw(cairo_t* cr, float scalex, float scaley)
{
	cairo_set_antialias(cr,smoothing ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

	cairoPathFromTokens(cr, tokens, scaleFactor, false,scalex, scaley,xstart,ystart,isMask);
}

uint8_t* CairoRenderer::getPixelBuffer(float scalex, float scaley, bool *isBufferOwner)
{
	if (isBufferOwner)
		*isBufferOwner=true;
	if(width==0 || height==0 || !Config::getConfig()->isRenderingEnabled())
		return nullptr;

	uint8_t* ret=nullptr;

	cairo_surface_t* cairoSurface=allocateSurface(ret);

	cairo_t* cr=cairo_create(cairoSurface);
	cairo_surface_destroy(cairoSurface); /* cr has an reference to it */
	cairoClean(cr);
	cairo_set_antialias(cr,smoothing ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);

	//Apply all the masks to clip the drawn part
	for(uint32_t i=0;i<masks.size();i++)
	{
		if(masks[i].maskMode != HARD_MASK)
			continue;
		masks[i].m->applyCairoMask(cr,xOffset,yOffset,scalex,scaley);
	}

	executeDraw(cr,scalex, scaley);

	cairo_surface_t* maskSurface = nullptr;
	uint8_t* maskRawData = nullptr;
	cairo_t* maskCr = nullptr;
	int32_t maskXOffset = 1;
	int32_t maskYOffset = 1;
	//Also apply the soft masks
	for(uint32_t i=0;i<masks.size();i++)
	{
		if(masks[i].maskMode != SOFT_MASK)
			continue;
		//TODO: this may be optimized if needed
		uint8_t* maskData = masks[i].m->getPixelBuffer(scalex,scaley);
		if(maskData==nullptr)
			continue;

		cairo_surface_t* tmp = cairo_image_surface_create_for_data(maskData,CAIRO_FORMAT_ARGB32,
				masks[i].m->getWidth(),masks[i].m->getHeight(),masks[i].m->getWidth()*4);
		if(maskSurface==nullptr)
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

bool CairoTokenRenderer::hitTest(const tokensVector& tokens, float scaleFactor, number_t x, number_t y)
{
	cairo_surface_t* cairoSurface=cairo_image_surface_create_for_data(nullptr, CAIRO_FORMAT_ARGB32, 0, 0, 0);
	cairo_t *cr=cairo_create(cairoSurface);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);

	bool empty=cairoPathFromTokens(cr, tokens, scaleFactor, true,1.0,1.0,0,0,true);
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

void CairoTokenRenderer::applyCairoMask(cairo_t* cr,int32_t xOffset,int32_t yOffset, float scalex, float scaley) const
{
	cairo_matrix_t mat;
	cairo_get_matrix(cr,&mat);
	cairo_translate(cr,xOffset,yOffset);
	cairo_set_matrix(cr,&mat);
	
	cairoPathFromTokens(cr, tokens, scaleFactor, true,scalex,scaley,xstart,ystart,true);
	cairo_clip(cr);
}

CairoTokenRenderer::CairoTokenRenderer(const tokensVector &_g, const MATRIX &_m, int32_t _x, int32_t _y, int32_t _w, int32_t _h
									   , int32_t _rx, int32_t _ry, int32_t _rw, int32_t _rh, float _r
									   , float _xs, float _ys, bool _im, bool _hm, float _s, float _a
									   , const std::vector<IDrawable::MaskData> &_ms
									   , float _redMultiplier,float _greenMultiplier,float _blueMultiplier,float _alphaMultiplier
									   , float _redOffset,float _greenOffset,float _blueOffset,float _alphaOffset
									   , bool _smoothing, number_t _xmin, number_t _ymin)
	: CairoRenderer(_m,_x,_y,_w,_h,_rx,_ry,_rw,_rh,_r,_xs,_ys,_im,_hm,_s,_a,_ms
					, _redMultiplier,_greenMultiplier,_blueMultiplier,_alphaMultiplier
					, _redOffset,_greenOffset,_blueOffset,_alphaOffset
					,_smoothing,_xmin,_ymin),tokens(_g)
{
}

void CairoRenderer::convertBitmapWithAlphaToCairo(std::vector<uint8_t, reporter_allocator<uint8_t>>& data, uint8_t* inData, uint32_t width,
												  uint32_t height, size_t* dataSize, size_t* stride, bool frompng)
{
	*stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	*dataSize = *stride * height;
	data.resize(*dataSize, 0);
	uint8_t* outData=&data[0];

	for(uint32_t i = 0; i < height; i++)
	{
		for(uint32_t j = 0; j < width; j++)
		{
			uint32_t* outDataPos = (uint32_t*)(outData+i*(*stride)) + j;
			// PNGs are always decoded in RGBA
			*outDataPos = frompng ? inData[i*(*stride)+j*4+3]<<24 | inData[i*(*stride)+j*4  ]<<16 | inData[i*(*stride)+j*4+1]<<8 | inData[i*(*stride)+j*4+2]
								  : inData[i*(*stride)+j*4  ]<<24 | inData[i*(*stride)+j*4+1]<<16 | inData[i*(*stride)+j*4+2]<<8 | inData[i*(*stride)+j*4+3];
		}
	}
}

inline void CairoRenderer::copyRGB15To24(uint32_t& dest, uint8_t* src)
{
	// highest bit is ignored
	uint8_t r = (src[0] & 0x7C) >> 2;
	uint8_t g = ((src[0] & 0x03) << 3) + (src[1] & 0xE0 >> 5);
	uint8_t b = src[1] & 0x1F;

	r = r*255/31;
	g = g*255/31;
	b = b*255/31;

	dest |= uint32_t(r)<<16;
	dest |= uint32_t(g)<<8;
	dest |= uint32_t(b);
}

inline void CairoRenderer::copyRGB24To24(uint32_t& dest, uint8_t* src)
{
	dest |= uint32_t(src[0])<<16;
	dest |= uint32_t(src[1])<<8;
	dest |= uint32_t(src[2]);
}

void CairoRenderer::convertBitmapToCairo(std::vector<uint8_t, reporter_allocator<uint8_t>>& data, uint8_t* inData, uint32_t width,
					 uint32_t height, size_t* dataSize, size_t* stride, uint32_t bpp)
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
			// set the alpha channel to opaque
			uint32_t pdata = uint32_t(0xFF)<<24;
			// copy the RGB bytes to pdata
			switch (bpp)
			{
				case 2:
					copyRGB15To24(pdata, inData+(i*width+j)*2);
					break;
				case 3:
					copyRGB24To24(pdata, inData+(i*width+j)*3);
					break;
				case 4:
					copyRGB24To24(pdata, inData+(i*width+j)*4+1);
					break;
			}
			*outDataPos = pdata;
		}
	}
}

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

	/* setup font description */
	desc = pango_font_description_new();
	pango_font_description_set_family(desc, tData.font.raw_buf());
	pango_font_description_set_size(desc, PANGO_SCALE*tData.fontSize);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
}

void CairoPangoRenderer::executeDraw(cairo_t* cr, float /*scalex*/, float /*scaley*/)
{
	PangoLayout* layout;

	layout = pango_cairo_create_layout(cr);
	pangoLayoutFromData(layout, textData);

	int xpos=0;
	switch(textData.autoSize)
	{
		case TextData::AUTO_SIZE::AS_RIGHT:
			xpos = textData.width-textData.textWidth;
			break;
		case TextData::AUTO_SIZE::AS_CENTER:
			xpos = (textData.width-textData.textWidth)/2;
			break;
		default:
			break;
	}
	
	if(textData.background)
	{
		cairo_set_source_rgb (cr, textData.backgroundColor.Red/255., textData.backgroundColor.Green/255., textData.backgroundColor.Blue/255.);
		cairo_paint(cr);
	}

	/* text scroll position */
	int32_t translateX = textData.scrollH;
	int32_t translateY = 0;
	if (textData.scrollV > 1)
	{
		translateY = -PANGO_PIXELS(lineExtents(layout, textData.scrollV-1).y);
	}

	/* draw the text */
	cairo_translate(cr, xpos, 0);
	cairo_set_source_rgb (cr, textData.textColor.Red/255., textData.textColor.Green/255., textData.textColor.Blue/255.);
	cairo_translate(cr, translateX, translateY);
	pango_cairo_show_layout(cr, layout);
	cairo_translate(cr, -translateX, -translateY);
	cairo_translate(cr, -xpos, 0);

	if(textData.border)
	{
		cairo_set_source_rgb(cr, textData.borderColor.Red/255., textData.borderColor.Green/255., textData.borderColor.Blue/255.);
		cairo_set_line_width(cr, 1);
		cairo_rectangle(cr, 0, 0, textData.width, textData.height);
		cairo_stroke_preserve(cr);
	}
	if(textData.caretblinkstate)
	{
		uint32_t w,h,tw,th;
		tw=0;
		tiny_string currenttext = textData.text;
		if (caretIndex < currenttext.numChars())
		{
			textData.text = textData.text.substr(0,caretIndex);
		}
		getBounds(textData,w,h,tw,th);
		textData.text = currenttext;
		tw+=xpos;
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_set_line_width(cr, 2);
		cairo_move_to(cr,tw,2);
		cairo_line_to(cr,tw, textData.height-2);
		cairo_stroke_preserve(cr);
	}

	g_object_unref(layout);
}

bool CairoPangoRenderer::getBounds(const TextData& _textData, uint32_t& w, uint32_t& h, uint32_t& tw, uint32_t& th)
{
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

PangoRectangle CairoPangoRenderer::lineExtents(PangoLayout *layout, int lineNumber)
{
	PangoRectangle rect;
	memset(&rect, 0, sizeof(PangoRectangle));
	int i = 0;
	PangoLayoutIter* lineIter = pango_layout_get_iter(layout);
	do
	{
		if (i == lineNumber)
		{
			pango_layout_iter_get_line_extents(lineIter, NULL, &rect);
			break;
		}

		i++;
	} while (pango_layout_iter_next_line(lineIter));
	pango_layout_iter_free(lineIter);

	return rect;
}

std::vector<LineData> CairoPangoRenderer::getLineData(const TextData& _textData)
{
	cairo_surface_t* cairoSurface=cairo_image_surface_create_for_data(NULL, CAIRO_FORMAT_ARGB32, 0, 0, 0);
	cairo_t *cr=cairo_create(cairoSurface);

	PangoLayout* layout;
	layout = pango_cairo_create_layout(cr);
	pangoLayoutFromData(layout, _textData);

	int XOffset = _textData.scrollH;
	int YOffset = PANGO_PIXELS(lineExtents(layout, _textData.scrollV-1).y);
	std::vector<LineData> data;
	data.reserve(pango_layout_get_line_count(layout));
	PangoLayoutIter* lineIter = pango_layout_get_iter(layout);
	do
	{
		PangoRectangle rect;
		pango_layout_iter_get_line_extents(lineIter, NULL, &rect);
		PangoLayoutLine* line = pango_layout_iter_get_line(lineIter);
		data.emplace_back(PANGO_PIXELS(rect.x) - XOffset,
				  PANGO_PIXELS(rect.y) - YOffset,
				  PANGO_PIXELS(rect.width),
				  PANGO_PIXELS(rect.height),
				  _textData.text.bytePosToIndex(line->start_index),
				  _textData.text.substr_bytes(line->start_index, line->length).numChars(),
				  PANGO_PIXELS(PANGO_ASCENT(rect)),
				  PANGO_PIXELS(PANGO_DESCENT(rect)),
				  PANGO_PIXELS(PANGO_LBEARING(rect)),
				  0); // FIXME
	} while (pango_layout_iter_next_line(lineIter));
	pango_layout_iter_free(lineIter);

	g_object_unref(layout);
	cairo_destroy(cr);
	cairo_surface_destroy(cairoSurface);

	return data;
}

void CairoPangoRenderer::applyCairoMask(cairo_t* cr, int32_t xOffset, int32_t yOffset, float scalex, float scaley) const
{
	assert(false);
}

AsyncDrawJob::AsyncDrawJob(IDrawable* d, _R<DisplayObject> o):drawable(d),owner(o),surfaceBytes(nullptr),uploadNeeded(false)
{
}

AsyncDrawJob::~AsyncDrawJob()
{
	owner->getSystemState()->AsyncDrawJobCompleted(this);
	delete drawable;
	if (surfaceBytes)
		delete[] surfaceBytes;
}

void AsyncDrawJob::execute()
{
	SystemState* sys = owner->getSystemState();
	float scalex;
	float scaley;
	int offx,offy;
	sys->stageCoordinateMapping(sys->getRenderThread()->windowWidth,sys->getRenderThread()->windowHeight,offx,offy, scalex,scaley);

	if(!threadAborting)
		surfaceBytes=drawable->getPixelBuffer(scalex,scaley);
	if(!threadAborting && surfaceBytes)
		uploadNeeded=true;
}

void AsyncDrawJob::threadAbort()
{
	uploadNeeded=false;
}

void AsyncDrawJob::jobFence()
{
	//If the data must be uploaded (there were no errors) the Job add itself to the upload queue.
	//Otherwise it destroys itself
	if(uploadNeeded)
	{
		uploadNeeded=false;
		owner->getSystemState()->getRenderThread()->addUploadJob(this);
	}
	else
		delete this;
}

void AsyncDrawJob::upload(uint8_t* data, uint32_t w, uint32_t h)
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
	if (!surface.tex)
	{
		surface.tex=new TextureChunk();
		surface.isChunkOwner=true;
	}
	if(!surface.tex->resizeIfLargeEnough(width, height))
		*surface.tex=owner->getSystemState()->getRenderThread()->allocateTexture(width, height,false);
	surface.xOffset=drawable->getXOffset();
	surface.yOffset=drawable->getYOffset();
	surface.xOffsetTransformed=drawable->getXOffsetTransformed();
	surface.yOffsetTransformed=drawable->getYOffsetTransformed();
	surface.widthTransformed=drawable->getWidthTransformed();
	surface.heightTransformed=drawable->getHeightTransformed();
	surface.alpha=drawable->getAlpha();
	surface.rotation=drawable->getRotation();
	surface.xscale = drawable->getXScale();
	surface.yscale = drawable->getYScale();
	surface.isMask = drawable->getIsMask();
	surface.hasMask = drawable->getHasMask();
	surface.redMultiplier=drawable->getRedMultiplier();
	surface.greenMultiplier=drawable->getGreenMultiplier();
	surface.blueMultiplier=drawable->getBlueMultiplier();
	surface.alphaMultiplier=drawable->getAlphaMultiplier();
	surface.redOffset=drawable->getRedOffset();
	surface.greenOffset=drawable->getGreenOffset();
	surface.blueOffset=drawable->getBlueOffset();
	surface.alphaOffset=drawable->getAlphaOffset();
	return *surface.tex;
}

void AsyncDrawJob::uploadFence()
{
	// ensure that the owner is moved to freelist in vm thread
	if (getVm(owner->getSystemState()))
	{
		owner->incRef();
		getVm(owner->getSystemState())->addDeletableObject(owner.getPtr());
	}
	delete this;
}

void SoftwareInvalidateQueue::addToInvalidateQueue(_R<DisplayObject> d)
{
	queue.emplace_back(d);
}

IDrawable::~IDrawable()
{
	auto it = masks.begin();
	while (it != masks.end())
	{
		delete it->m;
		it->m=nullptr;
		it++;
	}
}

BitmapRenderer::BitmapRenderer(_NR<BitmapContainer> _data, int32_t _x, int32_t _y, int32_t _w, int32_t _h, int32_t _rx, int32_t _ry, int32_t _rw, int32_t _rh, float _r, float _xs, float _ys, bool _im, bool _hm,
		float _a, const std::vector<MaskData>& _ms,
		float _redMultiplier, float _greenMultiplier, float _blueMultiplier, float _alphaMultiplier,
		float _redOffset, float _greenOffset, float _blueOffset, float _alphaOffset)
	: IDrawable(_w, _h, _x, _y, _rw, _rh, _rx, _ry, _r, _xs, _ys, _im, _hm,_a, _ms,
				_redMultiplier,_greenMultiplier,_blueMultiplier,_alphaMultiplier,
				_redOffset,_greenOffset,_blueOffset,_alphaOffset)
	, data(_data)
{
}

uint8_t *BitmapRenderer::getPixelBuffer(float scalex, float scaley, bool *isBufferOwner)
{
	if (isBufferOwner)
		*isBufferOwner=false;
	return data->getData();
}

