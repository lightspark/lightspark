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
	width=w;
	height=h;
	if(w==0 || h==0)
	{
		chunks=nullptr;
		return;
	}
	const uint32_t blocksW=(width+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
	const uint32_t blocksH=(height+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
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
	uint32_t blocksW=(width+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
	uint32_t blocksH=(height+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
	texId=r.texId;
	if(r.chunks)
	{
		chunks=new uint32_t[blocksW*blocksH];
		memcpy(chunks, r.chunks, blocksW*blocksH*4);
	}
	else
		chunks=nullptr;
	xContentScale = r.xContentScale;
	yContentScale = r.yContentScale;
	xOffset = r.xOffset;
	yOffset = r.yOffset;
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
	const uint32_t blocksW=(width+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
	const uint32_t blocksH=(height+CHUNKSIZE_REAL-1)/CHUNKSIZE_REAL;
	if(w<=blocksW*CHUNKSIZE_REAL && h<=blocksH*CHUNKSIZE_REAL)
	{
		width=w;
		height=h;
		return true;
	}
	return false;
}

CairoRenderer::CairoRenderer(const MATRIX& _m, int32_t _x, int32_t _y, int32_t _w, int32_t _h, int32_t _rx, int32_t _ry, int32_t _rw, int32_t _rh, float _r, float _xs, float _ys, bool _im, _NR<DisplayObject> _mask,
		float _s, float _a, const std::vector<MaskData>& _ms,
		float _redMultiplier, float _greenMultiplier, float _blueMultiplier, float _alphaMultiplier,
		float _redOffset, float _greenOffset, float _blueOffset, float _alphaOffset,
		SMOOTH_MODE _smoothing)
	: IDrawable(_w, _h, _x, _y, _rw, _rh, _rx, _ry, _r, _xs, _ys, _xs, _ys, _im, _mask,_a, _ms,
				_redMultiplier,_greenMultiplier,_blueMultiplier,_alphaMultiplier,
				_redOffset,_greenOffset,_blueOffset,_alphaOffset,_smoothing,_m)
	, scaleFactor(_s)
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

cairo_pattern_t* CairoTokenRenderer::FILLSTYLEToCairo(const FILLSTYLE& style, double scaleCorrection, bool isMask)
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
		case FOCAL_RADIAL_GRADIENT:
		{
			const std::vector<GRADRECORD>& gradrecords = style.FillStyleType==FOCAL_RADIAL_GRADIENT ? style.FocalGradient.GradientRecords : style.Gradient.GradientRecords;

			// We want an opaque black background... a gradient with no stops
			// in cairo will give us transparency.
			if (gradrecords.size() == 0)
			{
				pattern = cairo_pattern_create_rgb(0, 0, 0);
				return pattern;
			}

			MATRIX tmp=style.Matrix;
			tmp.x0 -= number_t(style.ShapeBounds.Xmin)/20.0;
			tmp.y0 -= number_t(style.ShapeBounds.Ymin)/20.0;
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
				tmp.multiply2D(style.FillStyleType == FOCAL_RADIAL_GRADIENT ? style.FocalGradient.FocalPoint*16384.0 : 0, 0,x0,y0);
				tmp.multiply2D(16384.0, 0,x1,y1);
				double radius=x1-x0;
				pattern = cairo_pattern_create_radial(x0, y0, 0, x0, y0, radius);
			}

			int spreadmode = style.FillStyleType == FOCAL_RADIAL_GRADIENT ? style.FocalGradient.SpreadMode : style.Gradient.SpreadMode;
			if (spreadmode == 0) // PAD
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_PAD);
			else if (spreadmode == 1) // REFLECT
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REFLECT);
			else if (spreadmode == 2) // REPEAT
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

			for(uint32_t i=0;i<gradrecords.size();i++)
			{
				double ratio = number_t(gradrecords[i].Ratio) / 255.0;
				const RGBA& color=gradrecords[i].Color;
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

bool CairoTokenRenderer::cairoPathFromTokens(cairo_t* cr, const tokensVector& tokens, double scaleCorrection, bool skipPaint, bool isMask, number_t xstart, number_t ystart, int* starttoken)
{
	if (skipPaint && starttoken && tokens.size()==0)
	{
		*starttoken=-1;
		return true;
	}
	cairo_scale(cr, scaleCorrection, scaleCorrection);

	bool empty=true;

	if (xstart || ystart)
	{
		cairo_matrix_t mat;
		cairo_get_matrix(cr,&mat);
		cairo_matrix_translate(&mat,-xstart/scaleCorrection,-ystart/scaleCorrection);
		cairo_set_matrix(cr, &mat);
	}
	// Make sure not to draw anything until a fill is set.
	cairo_set_operator(cr, CAIRO_OPERATOR_DEST);

#define PATH(operation, args...) \
	operation(cr, ## args);

	bool instroke = false;
	int tokentype = 1;
	while (tokentype)
	{
		std::vector<uint64_t>::const_iterator it;
		std::vector<uint64_t>::const_iterator itbegin;
		std::vector<uint64_t>::const_iterator itend;
		switch(tokentype)
		{
			case 1:
				itbegin = tokens.filltokens.begin();
				itend = tokens.filltokens.end();
				if (skipPaint && starttoken)
				{
					if (*starttoken >= int(tokens.filltokens.size()))
					{
						instroke=true;
						tokentype=2;
						itbegin = tokens.stroketokens.begin();
						itend = tokens.stroketokens.end();
						it = tokens.stroketokens.begin()+(*starttoken - tokens.filltokens.size());
					}
					else
						it = tokens.filltokens.begin()+ (*starttoken);
					*starttoken = -1;
				}
				else
					it = tokens.filltokens.begin();
				tokentype++;
				break;
			case 2:
				if(skipPaint && !tokens.filltokens.empty())
				{
					tokentype = 0;
					break;
				}
				it = tokens.stroketokens.begin();
				itbegin = tokens.stroketokens.begin();
				itend = tokens.stroketokens.end();
				tokentype++;
				break;
			default:
				tokentype = 0;
				break;
		}
		if (tokentype == 0)
			break;
		while (it != itend && tokentype)
		{
			GeomToken p(*it,false);
			switch(p.type)
			{
				case MOVE:
				{
					GeomToken p1(*(++it),false);
					PATH(cairo_move_to, (p1.vec.x), (p1.vec.y));
					break;
				}
				case STRAIGHT:
				{
					GeomToken p1(*(++it),false);
					PATH(cairo_line_to, (p1.vec.x), (p1.vec.y));
					empty = false;
					break;
				}
				case CURVE_QUADRATIC:
				{
					GeomToken p1(*(++it),false);
					GeomToken p2(*(++it),false);
					PATH(quadraticBezier,
					   (p1.vec.x), (p1.vec.y),
					   (p2.vec.x), (p2.vec.y));
					empty = false;
					break;
				}
				case CURVE_CUBIC:
				{
					GeomToken p1(*(++it),false);
					GeomToken p2(*(++it),false);
					GeomToken p3(*(++it),false);
					PATH(cairo_curve_to,
					   (p1.vec.x), (p1.vec.y),
					   (p2.vec.x), (p2.vec.y),
					   (p3.vec.x), (p3.vec.y));
					empty = false;
					break;
				}
				case SET_FILL:
				{
					GeomToken p1(*(++it),false);
					if(skipPaint)
					{
						if (starttoken && !empty)
						{
							cairo_close_path(cr);
							*starttoken=it-itbegin + 1 +(tokentype > 2 ? tokens.filltokens.size() : 0);
							tokentype=0;
						}
						break;
					}
					if (instroke)
						cairo_stroke(cr);
					else if (!tokens.filltokens.empty())
						cairo_fill(cr);
					instroke=false;
					cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
					const FILLSTYLE* style = p1.fillStyle;
					if (style->FillStyleType == NON_SMOOTHED_CLIPPED_BITMAP ||
						style->FillStyleType == NON_SMOOTHED_REPEATING_BITMAP)
						cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
					cairo_pattern_t* pattern = FILLSTYLEToCairo(*style, scaleCorrection,isMask);
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
					GeomToken p1(*(++it),false);
					if(skipPaint)
					{
						if (starttoken && !empty)
						{
							cairo_close_path(cr);
							*starttoken=it-itbegin + 1 +(tokentype > 2 ? tokens.filltokens.size() : 0);
							tokentype=0;
						}
						break;
					}
					if (instroke)
						cairo_stroke(cr);
					else if (!tokens.filltokens.empty())
						cairo_fill(cr);
					instroke = true;
					const LINESTYLE2* style = p1.lineStyle;
					cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
					if (style->HasFillFlag)
					{
						if (style->FillType.FillStyleType == NON_SMOOTHED_CLIPPED_BITMAP ||
							style->FillType.FillStyleType == NON_SMOOTHED_REPEATING_BITMAP)
							cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
						cairo_pattern_t* pattern = FILLSTYLEToCairo(style->FillType, scaleCorrection,isMask);
						if (pattern)
						{
							cairo_set_source(cr, pattern);
							cairo_pattern_destroy(pattern);
						}
					} else {
						const RGBA& color = style->Color;
						float r,g,b,a;
						r = color.rf();
						g = color.gf();
						b = color.bf();
						a = isMask ? 1.0 : color.af();
						cairo_set_source_rgba(cr, r, g, b, a);
					}
					// TODO: EndCapStyle
					if (style->StartCapStyle == 0)
						cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
					else if (style->StartCapStyle == 1)
						cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
					else if (style->StartCapStyle == 2)
						cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

					if (style->JointStyle == 0)
						cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
					else if (style->JointStyle == 1)
						cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
					else if (style->JointStyle == 2) {
						cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
						cairo_set_miter_limit(cr, style->MiterLimitFactor);
					}
					if (style->Width == 0)
					{
						//Width == 0 should be a hairline, but
						//cairo does not support hairlines.
						//using cairo_device_to_user_distance seems to work instead
						double linewidth = 1.0;
						cairo_device_to_user_distance(cr,&linewidth,&linewidth);
						cairo_set_line_width(cr, linewidth);
					}
					else 
					{
						double linewidth = (double)(style->Width) * scaleCorrection;
						cairo_device_to_user_distance(cr,&linewidth,&linewidth);
						cairo_set_line_width(cr, linewidth);
					}
					break;
				}
				case CLEAR_FILL:
				case FILL_KEEP_SOURCE:
					if(skipPaint)
					{
						if (starttoken)
						{
							cairo_close_path(cr);
							if (tokens.filltokens.empty())
								*starttoken=-1;
							else
								*starttoken=it-itbegin + 1 +(tokentype > 2 ? tokens.filltokens.size() : 0);
							tokentype=0;
						}
						if(p.type==CLEAR_FILL)
							// Clear source.
							cairo_set_operator(cr, CAIRO_OPERATOR_DEST);
						break;
					}
					cairo_fill(cr);
					if(p.type==CLEAR_FILL)
						// Clear source.
						cairo_set_operator(cr, CAIRO_OPERATOR_DEST);
					break;
				case CLEAR_STROKE:
					instroke = false;
					if(skipPaint)
					{
						if (starttoken)
						{
							cairo_close_path(cr);
							*starttoken=it-itbegin + 1 +(tokentype > 2 ? tokens.filltokens.size() : 0);
							tokentype=0;
						}
						break;
					}
					cairo_stroke(cr);
					// Clear source.
					cairo_set_operator(cr, CAIRO_OPERATOR_DEST);
					break;
				case FILL_TRANSFORM_TEXTURE:
				{
					GeomToken p1(*(++it),false);
					GeomToken p2(*(++it),false);
					GeomToken p3(*(++it),false);
					GeomToken p4(*(++it),false);
					GeomToken p5(*(++it),false);
					GeomToken p6(*(++it),false);
					if(skipPaint)
						break;
					cairo_matrix_t origmat;
					cairo_pattern_t* pattern;
					pattern=cairo_get_source(cr);
					cairo_pattern_get_matrix(pattern, &origmat);
					MATRIX m(p1.value,p2.value,p3.value,p4.value,p5.value,p6.value);
					cairo_pattern_set_matrix(pattern, &m);
					cairo_fill(cr);
					cairo_pattern_set_matrix(pattern, &origmat);
					break;
				}
				default:
					assert(false);
			}
			it++;
		}
	}
	#undef PATH

	if(!skipPaint)
	{
		if (instroke)
			cairo_stroke(cr);
		else if (!tokens.filltokens.empty())
			cairo_fill(cr);
	}
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
	cairo_set_antialias(cr,smoothing ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
	cairoPathFromTokens(cr, tokens, scaleFactor, false,isMask,xstart,ystart);
}

uint8_t* CairoRenderer::getPixelBuffer(bool *isBufferOwner, uint32_t* bufsize)
{
	if (isBufferOwner)
		*isBufferOwner=true;
	if (bufsize)
		*bufsize=width*height*4;
	if(width<=0 || height<=0 || !Config::getConfig()->isRenderingEnabled())
		return nullptr;

	uint8_t* ret=nullptr;
	cairo_surface_t* cairoSurface=allocateSurface(ret);
	cairo_t* cr=cairo_create(cairoSurface);

	cairo_scale(cr, xscale, yscale);

	cairo_surface_destroy(cairoSurface); /* cr has an reference to it */
	cairoClean(cr);
	cairo_set_antialias(cr,smoothing ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);

	//Apply all the masks to clip the drawn part
	for(uint32_t i=0;i<masks.size();i++)
	{
		if(masks[i].maskMode != HARD_MASK)
			continue;
		masks[i].m->applyCairoMask(cr,xOffset,yOffset);
	}

	executeDraw(cr);

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
		uint8_t* maskData = masks[i].m->getPixelBuffer();
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
//	cairo_surface_write_to_png(cairoSurface,"/tmp/cairo.png");
	cairo_destroy(cr);
	return ret;
}

bool CairoRenderer::isCachedSurfaceUsable(const DisplayObject* o) const
{
	const TextureChunk* tex = o->cachedSurface.tex;

	// arbitrary regen threshold
	return !tex || !tex->isValid() ||
		(abs(xscale / tex->xContentScale) < 2
		&& abs(yscale / tex->yContentScale) < 2);
}

bool CairoTokenRenderer::hitTest(const tokensVector& tokens, float scaleFactor, number_t x, number_t y)
{
	cairo_surface_t* cairoSurface=cairo_image_surface_create_for_data(nullptr, CAIRO_FORMAT_ARGB32, 0, 0, 0);

	int starttoken=0;
	bool ret=false;
	bool empty=true;
	while (starttoken >=0)
	{
		// loop over all paths of the tokenvector separately
		cairo_t *cr=cairo_create(cairoSurface);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
		empty=cairoPathFromTokens(cr, tokens, scaleFactor, true,true,0,0,&starttoken);
		if(!empty)
		{
			/* reset the matrix so x and y are not scaled
			 * by the current cairo transformation
			 */
			cairo_identity_matrix(cr);
			ret=cairo_in_fill(cr, x, y);
			if (ret)
			{
				cairo_destroy(cr);
				break;
			}
		}
		cairo_destroy(cr);
	}
	cairo_surface_destroy(cairoSurface);
	return ret;
}

void CairoTokenRenderer::applyCairoMask(cairo_t* cr,int32_t xOffset,int32_t yOffset) const
{
	cairo_matrix_t mat;
	cairo_get_matrix(cr,&mat);
	cairo_translate(cr,xOffset,yOffset);
	cairo_set_matrix(cr,&mat);
	
	cairoPathFromTokens(cr, tokens, scaleFactor, true,true,xstart,ystart);
	cairo_clip(cr);
}

CairoTokenRenderer::CairoTokenRenderer(const tokensVector &_g, const MATRIX &_m, int32_t _x, int32_t _y, int32_t _w, int32_t _h
									   , int32_t _rx, int32_t _ry, int32_t _rw, int32_t _rh, float _r
									   , float _xs, float _ys, bool _im, _NR<DisplayObject> _mask, float _s, float _a
									   , const std::vector<IDrawable::MaskData> &_ms
									   , float _redMultiplier,float _greenMultiplier,float _blueMultiplier,float _alphaMultiplier
									   , float _redOffset,float _greenOffset,float _blueOffset,float _alphaOffset
									   , SMOOTH_MODE _smoothing, number_t _xstart, number_t _ystart)
	: CairoRenderer(_m,_x,_y,_w,_h,_rx,_ry,_rw,_rh,_r,_xs,_ys,_im,_mask,_s,_a,_ms
					, _redMultiplier,_greenMultiplier,_blueMultiplier,_alphaMultiplier
					, _redOffset,_greenOffset,_blueOffset,_alphaOffset
					,_smoothing),tokens(_g),xstart(_xstart),ystart(_ystart)
{
}

void CairoRenderer::convertBitmapWithAlphaToCairo(std::vector<uint8_t, reporter_allocator<uint8_t>>& data, uint8_t* inData, uint32_t width,
												  uint32_t height, size_t* dataSize, size_t* stride, bool frompng)
{
	*stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
	*dataSize = *stride * height;
	data.resize(*dataSize);
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

void CairoPangoRenderer::pangoLayoutFromData(PangoLayout* layout, const TextData& tData, const tiny_string& text)
{
	PangoFontDescription* desc;

	if (tData.isPassword)
	{
		tiny_string pwtxt;
		for (uint32_t i = 0; i < text.numChars(); i++)
			pwtxt+="*";
		pango_layout_set_text(layout, pwtxt.raw_buf(), -1);
	}
	else
		pango_layout_set_text(layout, text.raw_buf(), -1);


	/* setup font description */
	desc = pango_font_description_new();
	pango_font_description_set_family(desc, tData.font.raw_buf());
	pango_font_description_set_absolute_size(desc, PANGO_SCALE*tData.fontSize);
	pango_font_description_set_style(desc,tData.isItalic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
	pango_font_description_set_weight(desc,tData.isBold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
}

void CairoPangoRenderer::executeDraw(cairo_t* cr)
{
	PangoLayout* layout;

	layout = pango_cairo_create_layout(cr);

	int xpos=0;
	switch(textData.autoSize)
	{
		case TextData::ALIGNMENT::AS_RIGHT:
			xpos = textData.width-textData.textWidth;
			break;
		case TextData::ALIGNMENT::AS_CENTER:
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
	int32_t linepos=0;
	for (auto it = textData.textlines.begin(); it != textData.textlines.end(); it++)
	{
		
		cairo_translate(cr, it->autosizeposition, linepos);
		tiny_string text = it->text;
		pangoLayoutFromData(layout, textData,text);
		if (textData.isPassword)
		{
			tiny_string pwtxt;
			for (uint32_t i = 0; i < text.numChars(); i++)
				pwtxt+="*";
			pango_layout_set_text(layout, pwtxt.raw_buf(), -1);
		}
		else
			pango_layout_set_text(layout, text.raw_buf(), -1);
		pango_cairo_show_layout(cr, layout);
		cairo_translate(cr, -it->autosizeposition, -linepos);
		linepos += textData.fontSize+textData.leading;
	}
	cairo_translate(cr, -translateX, -translateY);
	cairo_translate(cr, -xpos, 0);

	if(textData.border)
	{
		cairo_set_source_rgb(cr, textData.borderColor.Red/255., textData.borderColor.Green/255., textData.borderColor.Blue/255.);
		cairo_set_line_width(cr, 1);
		cairo_rectangle(cr, 0, 0, this->width, this->height);
		cairo_stroke(cr);
	}
	if(textData.caretblinkstate)
	{
		uint32_t tw=TEXTFIELD_PADDING;
		uint32_t thstart=TEXTFIELD_PADDING;
		uint32_t thend=PANGO_PIXELS(PANGO_SCALE*textData.fontSize)+TEXTFIELD_PADDING;
		if (!textData.textlines.empty())
		{
			number_t tw1,th1;
			tiny_string currenttext = textData.getText(0);
			if (caretIndex < currenttext.numChars())
			{
				currenttext = currenttext.substr(0,caretIndex);
			}
			getBounds(textData,currenttext,tw1,th1);
			tw += tw1;
		}
		tw+=xpos;
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_set_line_width(cr, 2);
		cairo_move_to(cr,tw, thstart);
		cairo_line_to(cr,tw, thend);
		cairo_stroke(cr);
	}

	g_object_unref(layout);
}

bool CairoPangoRenderer::getBounds(const TextData& tData, const tiny_string& text, number_t& tw, number_t& th)
{
	cairo_surface_t* cairoSurface=cairo_image_surface_create_for_data(nullptr, CAIRO_FORMAT_ARGB32, 0, 0, 0);
	cairo_t *cr=cairo_create(cairoSurface);

	PangoLayout* layout;

	layout = pango_cairo_create_layout(cr);
	pangoLayoutFromData(layout, tData,text);

	PangoRectangle ink_rect, logical_rect;
	pango_layout_get_pixel_extents(layout,&ink_rect,&logical_rect);//TODO: check the rounding during pango conversion

	g_object_unref(layout);
	cairo_destroy(cr);
	cairo_surface_destroy(cairoSurface);

	//This should be safe check precision
	tw = ink_rect.width + ink_rect.x;
	th = ink_rect.height + ink_rect.y;
	return (th!=0) && (tw!=0);
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
	tiny_string text = _textData.getText();
	pangoLayoutFromData(layout, _textData,text);

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
				  text.bytePosToIndex(line->start_index),
				  text.substr_bytes(line->start_index, line->length).numChars(),
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

void CairoPangoRenderer::applyCairoMask(cairo_t* cr, int32_t xOffset, int32_t yOffset) const
{
	assert(false);
}

AsyncDrawJob::AsyncDrawJob(IDrawable* d, _R<DisplayObject> o):drawable(d),owner(o),surfaceBytes(nullptr),uploadNeeded(false),isBufferOwner(true)
{
}

AsyncDrawJob::~AsyncDrawJob()
{
	owner->cachedSurface.wasUpdated=false;
	owner->getSystemState()->AsyncDrawJobCompleted(this);
	delete drawable;
	if (surfaceBytes && isBufferOwner)
		delete[] surfaceBytes;
}

void AsyncDrawJob::execute()
{
	owner->startDrawJob();
	if(!threadAborting)
		surfaceBytes=drawable->getPixelBuffer(&isBufferOwner);
	if(!threadAborting && surfaceBytes)
		uploadNeeded=true;
	owner->endDrawJob();
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

uint8_t* AsyncDrawJob::upload(bool refresh)
{
	assert(surfaceBytes);
	return surfaceBytes;
}

void AsyncDrawJob::sizeNeeded(uint32_t& w, uint32_t& h) const
{
	w=drawable->getWidth();
	h=drawable->getHeight();
}

TextureChunk& AsyncDrawJob::getTexture()
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
	if (!surface.wasUpdated) // surface may have already been changed by DisplayObject::updateCachedSurface() before it was uploaded
	{
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
		surface.mask = drawable->getMask();
		surface.smoothing = drawable->getSmoothing();
		surface.redMultiplier=drawable->getRedMultiplier();
		surface.greenMultiplier=drawable->getGreenMultiplier();
		surface.blueMultiplier=drawable->getBlueMultiplier();
		surface.alphaMultiplier=drawable->getAlphaMultiplier();
		surface.redOffset=drawable->getRedOffset();
		surface.greenOffset=drawable->getGreenOffset();
		surface.blueOffset=drawable->getBlueOffset();
		surface.alphaOffset=drawable->getAlphaOffset();
		surface.matrix=drawable->getMatrix();
		surface.isValid=true;
		surface.isInitialized=true;
	}
	return *surface.tex;
}

void AsyncDrawJob::uploadFence()
{
	ITextureUploadable::uploadFence();
	// ensure that the owner is moved to freelist in vm thread
	if (getVm(owner->getSystemState()))
	{
		owner->incRef();
		getVm(owner->getSystemState())->addDeletableObject(owner.getPtr());
	}
	delete this;
}

void AsyncDrawJob::contentScale(float& x, float& y) const
{
	x = drawable->getXContentScale();
	y = drawable->getYContentScale();
}

void AsyncDrawJob::contentOffset(float& x, float& y) const
{
	x = drawable->getXOffset();
	y = drawable->getYOffset();
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

BitmapRenderer::BitmapRenderer(_NR<BitmapContainer> _data, int32_t _x, int32_t _y, int32_t _w, int32_t _h, int32_t _rx, int32_t _ry, int32_t _rw, int32_t _rh, float _r, float _xs, float _ys, bool _im, _NR<DisplayObject> _mask,
		float _a, const std::vector<MaskData>& _ms,
		float _redMultiplier, float _greenMultiplier, float _blueMultiplier, float _alphaMultiplier,
		float _redOffset, float _greenOffset, float _blueOffset, float _alphaOffset, SMOOTH_MODE _smoothing, const MATRIX& _m)
	: IDrawable(_w, _h, _x, _y, _rw, _rh, _rx, _ry, _r, _xs, _ys, 1, 1, _im, _mask,_a, _ms,
				_redMultiplier,_greenMultiplier,_blueMultiplier,_alphaMultiplier,
				_redOffset,_greenOffset,_blueOffset,_alphaOffset,_smoothing,_m)
	, data(_data)
{
}

uint8_t *BitmapRenderer::getPixelBuffer(bool *isBufferOwner, uint32_t* bufsize)
{
	if (isBufferOwner)
		*isBufferOwner=false;
	if (bufsize)
		*bufsize=data->getWidth()*data->getHeight()*4;
	return data->getData();
}


uint8_t* CharacterRenderer::upload(bool refresh)
{
	return this->data;
}

TextureChunk& CharacterRenderer::getTexture()
{
	if(!chunk.resizeIfLargeEnough(width, height))
		chunk=getSys()->getRenderThread()->allocateTexture(width, height,false);
	return chunk;
}

tiny_string TextData::getText(uint32_t line) const
{
	tiny_string text;
	if (line == UINT32_MAX)
	{
		for (auto it = textlines.begin(); it != textlines.end(); it++)
		{
			if (!text.empty())
				text += "\n";
			text += (*it).text;
		}
	}
	else if (textlines.size() > line)
		text = textlines[line].text;
	return text;
}

void TextData::setText(const char* text)
{
	textlines.clear();
	if (*text == 0x00)
		return;
	tiny_string t = text;
	uint32_t index = tiny_string::npos;
	uint32_t index1 = tiny_string::npos;
	uint32_t index2 = tiny_string::npos;
	do
	{
		index1 = t.find("\n");
		index2 = t.find("\r");
		index = min(index1,index2);
		textline line;
		line.autosizeposition=0;
		line.textwidth=UINT32_MAX;
		if (index != tiny_string::npos)
		{
			line.text = t.substr_bytes(0,index).raw_buf();
			if (index < t.numChars()-1 && (t.charAt(index+1)=='\r' || t.charAt(index+1)=='\n'))
				t=t.substr_bytes(index+2,UINT32_MAX);
			else
				t=t.substr_bytes(index+1,UINT32_MAX);
		}
		else
		{
			if (t.numBytes() > 0 &&  t.charAt(t.numBytes()-1)=='\r')
				line.text = t.substr_bytes(0,t.numBytes()-1).raw_buf();
			else
				line.text = t.raw_buf();
		}
		textlines.push_back(line);
	}
	while (index != tiny_string::npos);
}
