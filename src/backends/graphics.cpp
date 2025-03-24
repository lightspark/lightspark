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
#include "backends/cachedsurface.h"
#include "backends/rendering.h"
#include "backends/config.h"
#include "compat.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/display/Bitmap.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/system/flashsystem.h"
#include "parsing/tags.h"
#include <pango/pangocairo.h>

using namespace lightspark;

void saveToPNG(uint8_t* data, uint32_t w, uint32_t h, const char* filename)
{
	cairo_surface_t* cairoSurface=cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, w, h, w*4);
	cairo_t* cr=cairo_create(cairoSurface);
	cairo_surface_destroy(cairoSurface); 
	cairo_surface_write_to_png(cairoSurface,filename);
	cairo_destroy(cr);
}

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

void TextureChunk::setChunks(uint8_t* buf)
{
	assert(chunks == nullptr);
	chunks=(uint32_t*)buf;
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

CairoRenderer::CairoRenderer(const MATRIX& _m, float _x, float _y, float _w, float _h, float _xs, float _ys,
		bool _ismask, bool _cacheAsBitmap,
		float _scaling, float _a,
		const ColorTransformBase& _colortransform,
		SMOOTH_MODE _smoothing,
		AS_BLENDMODE _blendmode)
	: IDrawable(_w, _h, _x, _y, _xs, _ys, _xs, _ys, _ismask,_cacheAsBitmap,_scaling,_a,
				_colortransform,_smoothing,_blendmode,_m)
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
			float r,g,b,a;
			r = color.rf();
			g = color.gf();
			b = color.bf();
			a = isMask ? 1.0 : color.af();
			pattern = cairo_pattern_create_rgba(r,g,b,a);
			break;
		}
		case LINEAR_GRADIENT:
		case RADIAL_GRADIENT:
		case FOCAL_RADIAL_GRADIENT:
		{
			const std::vector<GRADRECORD>& gradrecords = style.Gradient.GradientRecords;

			// We want an opaque black background... a gradient with no stops
			// in cairo will give us transparency.
			if (gradrecords.size() == 0)
			{
				pattern = cairo_pattern_create_rgb(0, 0, 0);
				return pattern;
			}
			// The dimensions of the pattern space are specified in SWF specs
			// as a 32768x32768 box centered at (0,0)
			if (style.FillStyleType == LINEAR_GRADIENT)
			{
				MATRIX tmp=style.Matrix;
				tmp.x0 -= number_t(style.ShapeBounds.Xmin)/20.0;
				tmp.y0 -= number_t(style.ShapeBounds.Ymin)/20.0;
				tmp.x0/=scaleCorrection;
				tmp.y0/=scaleCorrection;
				double x0,y0,x1,y1;
				tmp.multiply2D(-16384.0, 0,x0,y0);
				tmp.multiply2D(16384.0, 0,x1,y1);
				pattern = cairo_pattern_create_linear(x0,y0,x1,y1);
			}
			else
			{
				number_t x0 =style.FillStyleType == FOCAL_RADIAL_GRADIENT ? style.Gradient.FocalPoint*16384.0 : 0.0;
				pattern = cairo_pattern_create_radial(x0, 0, 0, x0, 0, 16384.0);
			}

			int spreadmode = style.Gradient.SpreadMode;
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
				float r,g,b,a;
				r = color.rf();
				g = color.gf();
				b = color.bf();
				a = color.af();
				cairo_pattern_add_color_stop_rgba(pattern, ratio,r,g,b,a);
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
			if (bm->cachedCairoPattern == nullptr)
			{
				cairo_surface_t* surface = nullptr;
				uint8_t* buf = nullptr;
				//Do an explicit cast, the data will not be modified
				buf = (uint8_t*)bm->getData();
				surface = cairo_image_surface_create_for_data (buf,
									CAIRO_FORMAT_ARGB32,
									bm->getWidth(),
									bm->getHeight(),
									cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, bm->getWidth()));
				bm->cachedCairoPattern = cairo_pattern_create_for_surface(surface);
				cairo_surface_destroy(surface);
			}
			pattern = cairo_pattern_reference(bm->cachedCairoPattern);
			//Make a copy to invert it
			cairo_matrix_t mat=style.Matrix;
			mat.x0 -= number_t(style.ShapeBounds.Xmin)/20.0;
			mat.y0 -= number_t(style.ShapeBounds.Ymin)/20.0;
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
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);

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
void CairoTokenRenderer::adjustFillStyle(cairo_t* cr, const FILLSTYLE* style, const MATRIX& origmat, double scaleCorrection)
{
	switch (style->FillStyleType)
	{
		case RADIAL_GRADIENT:
		case FOCAL_RADIAL_GRADIENT:
		{
			MATRIX tmp=style->Matrix;
			tmp.x0 -= number_t(style->ShapeBounds.Xmin)*scaleCorrection;
			tmp.y0 -= number_t(style->ShapeBounds.Ymin)*scaleCorrection;
			tmp.x0/=scaleCorrection;
			tmp.y0/=scaleCorrection;
			// adjust gradient translation to current scaling
			tmp.x0*=origmat.xx;
			tmp.y0*=origmat.yy;
			MATRIX m;
			MATRIX m2 = origmat;
			m2.x0/=scaleCorrection;
			m2.y0/=scaleCorrection;
			cairo_matrix_multiply(&m,&m2,&tmp);
			// TODO there seems to be a bug in cairo (https://gitlab.freedesktop.org/cairo/cairo/-/issues/238) 
			// that leads to CAIRO_STATUS_NO_MEMORY if the matrix scale is too small, so we ignore those cases for now
			if (abs(m.getScaleX()) > 1.0/32768.0
				&& abs(m.getScaleY()) > 1.0/32768.0) 
				cairo_set_matrix(cr,&m);
			break;
		}
		case NON_SMOOTHED_CLIPPED_BITMAP:
		case NON_SMOOTHED_REPEATING_BITMAP:
			cairo_set_antialias(cr,CAIRO_ANTIALIAS_NONE);
			break;
		default:
			break;
	}

}
void CairoTokenRenderer::executefill(cairo_t* cr, const FILLSTYLE* style, cairo_pattern_t* pattern, double scaleCorrection)
{
	if (!style || !pattern)
		return;
	MATRIX origmat;
	cairo_get_matrix(cr,&origmat);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	adjustFillStyle(cr, style, origmat, scaleCorrection);
	cairo_set_source(cr, pattern);
	cairo_fill(cr);
	cairo_set_matrix(cr,&origmat);
}

void CairoTokenRenderer::executestroke(cairo_t* cr, const LINESTYLE2* style, cairo_pattern_t* pattern, double scaleCorrection, bool isMask, CairoTokenRenderer* th)
{
	if (!style)
		return;
	MATRIX origmat;
	cairo_get_matrix(cr,&origmat);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	if (style->HasFillFlag)
	{
		assert(pattern);
		adjustFillStyle(cr, &style->FillType, origmat, scaleCorrection);
		cairo_set_source(cr, pattern);
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
	else if (int(style->Width * scaleCorrection) == 1)
	{
		// ensure linewidth is properly scaled if Width equals to 1
		double linewidth = th ? th->getState()->xscale : 1.0;
		cairo_device_to_user_distance(cr,&linewidth,&linewidth);
		cairo_set_line_width(cr, linewidth);
	}
	else 
	{
		double linewidth = (double)(style->Width) * scaleCorrection;
		if (th)
			linewidth *= th->getState()->xscale;
		cairo_device_to_user_distance(cr,&linewidth,&linewidth);
		cairo_set_line_width(cr, linewidth);
	}
	cairo_stroke(cr);
	cairo_set_matrix(cr,&origmat);
}
bool CairoTokenRenderer::cairoPathFromTokens(cairo_t* cr, _NR<tokenListRef> tokens, double scaleCorrection, bool skipPaint, bool isMask, number_t xstart, number_t ystart, CairoTokenRenderer* th, int* starttoken)
{
	if (skipPaint && starttoken && (!tokens || tokens->tokens.empty()))
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
	const FILLSTYLE* currentfillstyle = nullptr;
	const LINESTYLE2* currentstrokestyle = nullptr;
	cairo_pattern_t* currentfillpattern=nullptr;
	cairo_pattern_t* currentstrokepattern=nullptr;
	bool instroke = false;
	bool infill = false;
	TokenList::const_iterator itbegin = tokens->tokens.cbegin();
	TokenList::const_iterator it = itbegin + (starttoken ? *starttoken : 0);
	
	bool pathdone = false;
	while (it != tokens->tokens.cend() && !pathdone)
	{
		GeomToken p(*it,false);
		switch(p.type)
		{
			case MOVE:
			{
				GeomToken p1(*(++it),false);
				if(skipPaint && !infill)
					break;
				cairo_move_to(cr,(p1.vec.x), (p1.vec.y));
				break;
			}
			case STRAIGHT:
			{
				GeomToken p1(*(++it),false);
				if(skipPaint && !infill)
					break;
				cairo_line_to(cr, (p1.vec.x), (p1.vec.y));
				empty = false;
				break;
			}
			case CURVE_QUADRATIC:
			{
				GeomToken p1(*(++it),false);
				GeomToken p2(*(++it),false);
				if(skipPaint && !infill)
					break;
				quadraticBezier(cr,
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
				if(skipPaint && !infill)
					break;
				cairo_curve_to(cr,
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
						*starttoken=it-itbegin + 1;
						pathdone = true;
					}
					infill=true;
					break;
				}
				if (instroke)
					executestroke(cr,currentstrokestyle,currentstrokepattern,scaleCorrection,isMask,th);
				if (infill)
				{
					cairo_close_path(cr);
					executefill(cr,currentfillstyle,currentfillpattern,scaleCorrection);
				}
				infill=true;
				currentfillstyle=p1.fillStyle;
				if (currentfillpattern)
					cairo_pattern_destroy(currentfillpattern);
				currentfillpattern = FILLSTYLEToCairo(*currentfillstyle, scaleCorrection,isMask);
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
						*starttoken=it-itbegin + 1;
						pathdone = true;
					}
					break;
				}
				if (instroke)
					executestroke(cr,currentstrokestyle,currentstrokepattern,scaleCorrection,isMask,th);
				if (infill)
					executefill(cr,currentfillstyle,currentfillpattern,scaleCorrection);
				instroke = true;
				currentstrokestyle = p1.lineStyle;
				if (currentstrokepattern)
					cairo_pattern_destroy(currentstrokepattern);
				if (currentstrokestyle->HasFillFlag)
					currentstrokepattern = FILLSTYLEToCairo(currentstrokestyle->FillType, scaleCorrection,isMask);
				else
					currentstrokepattern = nullptr;
				break;
			}
			case CLEAR_FILL:
			case FILL_KEEP_SOURCE:
				infill=false;
				if(skipPaint)
				{
					if (starttoken)
					{
						cairo_close_path(cr);
						*starttoken=it-itbegin + 1;
						pathdone = true;
					}
					if(p.type==CLEAR_FILL)
						// Clear source.
						cairo_set_operator(cr, CAIRO_OPERATOR_DEST);
					break;
				}
				cairo_close_path(cr);
				executefill(cr,currentfillstyle,currentfillpattern,scaleCorrection);
				if (currentfillpattern)
					cairo_pattern_destroy(currentfillpattern);
				if(p.type==CLEAR_FILL)
				{
					// Clear source.
					cairo_set_operator(cr, CAIRO_OPERATOR_DEST);
					currentfillstyle=nullptr;
					currentfillpattern=nullptr;
				}
				break;
			case CLEAR_STROKE:
				instroke = false;
				if(skipPaint)
				{
					if (starttoken)
					{
						cairo_close_path(cr);
						*starttoken=it-itbegin + 1;
					}
					break;
				}
				executestroke(cr,currentstrokestyle,currentstrokepattern,scaleCorrection,isMask,th);
				if (currentstrokepattern)
					cairo_pattern_destroy(currentstrokepattern);
				currentstrokepattern=nullptr;
				currentstrokestyle=nullptr;
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
				executefill(cr,currentfillstyle,currentfillpattern,scaleCorrection);
				cairo_pattern_set_matrix(pattern, &origmat);
				break;
			}
			default:
				assert(false);
		}
		it++;
	}

	if(!skipPaint)
	{
		if (instroke)
			executestroke(cr,currentstrokestyle,currentstrokepattern,scaleCorrection,isMask,th);
		if (infill)
			executefill(cr,currentfillstyle,currentfillpattern,scaleCorrection);
	}
	
	if (currentfillpattern)
		cairo_pattern_destroy(currentfillpattern);
	if (currentstrokepattern)
		cairo_pattern_destroy(currentstrokepattern);
	if (starttoken && !pathdone)
		*starttoken=-1;
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
	cairo_set_antialias(cr,getState()->smoothing ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
	if (filltokens)
		cairoPathFromTokens(cr, filltokens, state->scaling, false,getState()->isMask,xstart,ystart,this);
	if (stroketokens)
		cairoPathFromTokens(cr, stroketokens, state->scaling, false,getState()->isMask,xstart,ystart,this);
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

	cairo_scale(cr, getState()->xscale, getState()->yscale);

	cairo_surface_destroy(cairoSurface); /* cr has an reference to it */
	cairoClean(cr);
	cairo_set_antialias(cr,getState()->smoothing ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);

	executeDraw(cr);

//	cairo_surface_write_to_png(cairoSurface,"/tmp/cairo.png");
	cairo_destroy(cr);
	return ret;
}

bool CairoRenderer::isCachedSurfaceUsable(const DisplayObject* o) const
{
	const TextureChunk* tex = o->cachedSurface->tex;

	// arbitrary regen threshold
	return !tex || !tex->isValid() ||
		(abs(getState()->xscale / tex->xContentScale) < 2
		&& abs(getState()->yscale / tex->yContentScale) < 2);
}

bool CairoTokenRenderer::hitTest(_NR<tokenListRef> tokens, float scaleFactor, const Vector2f& point)
{
	if (!tokens || tokens->tokens.empty())
		return false;
	cairo_surface_t* cairoSurface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);

	int starttoken=0;
	bool ret=false;
	bool empty=true;
	while (starttoken >=0)
	{
		// loop over all paths of the tokenvector separately
		cairo_t *cr=cairo_create(cairoSurface);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
		empty=cairoPathFromTokens(cr, tokens, scaleFactor, true,true,0,0,nullptr,&starttoken);
		if(!empty)
		{
			/* reset the matrix so x and y are not scaled
			 * by the current cairo transformation
			 */
			cairo_identity_matrix(cr);
			ret=cairo_in_fill(cr, point.x, point.y);
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

CairoTokenRenderer::CairoTokenRenderer(_NR<tokenListRef> _filltokens,_NR<tokenListRef> _stroketokens, const MATRIX &_m, int32_t _x, int32_t _y, int32_t _w, int32_t _h
									   , float _xs, float _ys
									   , bool _ismask, bool _cacheAsBitmap
									   , float _scaling, float _a
									   , const ColorTransformBase& _colortransform
									   , SMOOTH_MODE _smoothing, AS_BLENDMODE _blendmode
									   , number_t _xstart, number_t _ystart)
	: CairoRenderer(_m,_x,_y,_w,_h,_xs,_ys,_ismask,_cacheAsBitmap,_scaling,_a
					, _colortransform
					,_smoothing,_blendmode),filltokens(_filltokens),stroketokens(_stroketokens),xstart(_xstart),ystart(_ystart)
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
		case ALIGNMENT::AS_RIGHT:
			xpos = textData.width-textData.textWidth;
			break;
		case ALIGNMENT::AS_CENTER:
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
	cairo_set_source_rgb (cr, textData.textColor.Red/255., textData.textColor.Green/255., textData.textColor.Blue/255);
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

AsyncDrawJob::AsyncDrawJob(IDrawable* d, _R<DisplayObject> o):drawable(d),owner(o),surfaceBytes(nullptr),uploadNeeded(false),isBufferOwner(true)
{
	owner->cachedSurface->wasUpdated=false;
}

AsyncDrawJob::~AsyncDrawJob()
{
	owner->getSystemState()->AsyncDrawJobCompleted(this);
	delete drawable;
	if (surfaceBytes && isBufferOwner)
		delete[] surfaceBytes;
}

void AsyncDrawJob::execute()
{
	if(!threadAborting)
		surfaceBytes=drawable->getPixelBuffer(&isBufferOwner);
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
	CachedSurface* surface=owner->cachedSurface.getPtr();
	uint32_t width=drawable->getWidth();
	uint32_t height=drawable->getHeight();
	//Verify that the texture is large enough
	if (!surface->tex)
	{
		surface->tex=new TextureChunk();
		surface->isChunkOwner=true;
	}
	if(!surface->tex->resizeIfLargeEnough(width, height))
		*surface->tex=owner->getSystemState()->getRenderThread()->allocateTexture(width, height,false);
	if (!surface->wasUpdated) // surface may have already been changed by DisplayObject::updateCachedSurface() before it was uploaded
	{
		surface->SetState(drawable->getState());
		owner->refreshSurfaceState();
		surface->isValid=true;
		surface->isInitialized=true;
	}
	return *surface->tex;
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

void AsyncDrawJob::contentScale(number_t& x, number_t& y) const
{
	x = drawable->getXContentScale();
	y = drawable->getYContentScale();
}

void AsyncDrawJob::contentOffset(number_t& x, number_t& y) const
{
	x = drawable->getState()->xOffset;
	y = drawable->getState()->yOffset;
}

IDrawable::IDrawable(float w, float h, float x, float y, float xs, float ys, float xcs, float ycs, bool _ismask, bool _cacheAsBitmap, float _scaling, float a, const ColorTransformBase& _colortransform, SMOOTH_MODE _smoothing, AS_BLENDMODE _blendmode, const MATRIX& _m)
	:width(w),height(h), xContentScale(xcs), yContentScale(ycs)
{
	// will be deleted in cachedSurface
	state = new SurfaceState(x,y,a,xs,ys,_colortransform,_m,_ismask,_cacheAsBitmap,_blendmode,_smoothing,_scaling);
}

IDrawable::~IDrawable()
{
}

RefreshableDrawable::RefreshableDrawable(float _x, float _y, float _w, float _h, float _xs, float _ys,
										 bool _ismask, bool _cacheAsBitmap, float _scaling,
										 float _a,
										 const ColorTransformBase& _colortransform, SMOOTH_MODE _smoothing, AS_BLENDMODE _blendmode, const MATRIX& _m)
	: IDrawable(_w, _h, _x, _y, _xs, _ys, 1, 1, _ismask, _cacheAsBitmap,_scaling,_a,
				_colortransform,_smoothing,_blendmode,_m)
{
}

uint8_t* CharacterRenderer::upload(bool refresh)
{
	return this->data;
}

TextureChunk& CharacterRenderer::getTexture()
{
	if(!chunk.resizeIfLargeEnough(width, height))
		chunk=getSys()->getRenderThread()->allocateTexture(width, height,false,true);
	return chunk;
}

tiny_string TextData::getText(uint32_t line) const
{
	tiny_string text;
	if (line == UINT32_MAX)
	{
		for (auto it = textlines.begin(); it != textlines.end(); it++)
		{
			text += (*it).text;
			if (it->linebreaks)
			{
				for (uint32_t i = 0; i < it->linebreaks; i++)
					text += "\r";
			}
			else if ((this->swfversion < 7 && it->needsnewline)
				|| (this->swfversion >= 7 && this->multiline && it->needsnewline))
				text += "\r";
		}
	}
	else if (textlines.size() > line)
		text = textlines[line].text;
	return text;
}

void TextData::setText(const char* text, bool firstlineonly)
{
	textlines.clear();
	appendText(text,firstlineonly);
}
void TextData::appendText(const char *text, bool firstlineonly, const FormatText* format, uint32_t swfversion, bool condenseWhite)
{
	if (!this->multiline && (swfversion >= 7) && *text == 0x00)
		return;
	tiny_string t = text;
	FormatText newformat;
	if (format)
		newformat=*format;
	bool mergeline = false;
	if (getLineCount())
	{
		mergeline=true;
		if (format)
		{
			if (swfversion < 8)
			{
				mergeline = condenseWhite && !(
								(this->multiline && (format->paragraph || format->bullet))
								|| textlines.back().format.paramsChanged(format));
			}
			else
			{
				mergeline = (textlines.back().text.endsWithHTMLWhitespace() && t.isWhiteSpaceOnly())
							|| !(this->multiline && (textlines.back().format.paragraph || textlines.back().format.bullet));
				if ((!t.isWhiteSpaceOnly() || t.endsWith("\n")) && !(textlines.back().format.paragraph || textlines.back().format.bullet))
					mergeline &= !textlines.back().format.paramsChanged(format);
			}
		}
		if (mergeline)
		{
			t = textlines.back().text + t;
			newformat = textlines.back().format;
			textlines.pop_back();
		}
		else if (textlines.back().format.paragraph || textlines.back().format.bullet)
			textlines.back().needsnewline = true;
	}
	bool hasnewline=false;
	if (swfversion >= 8 && condenseWhite)
	{
		tiny_string s = t.compactHTMLWhiteSpace(true,this->multiline ? &hasnewline : nullptr);
		if (!t.isWhiteSpaceOnly())
			t = s;
		else if (this->multiline)
		{
			if (mergeline || newformat.bullet)
				t = s;
			else if (!hasnewline && !(newformat.paragraph || newformat.bullet))
				return;
		}
	}
	number_t w,h;
	if (embeddedFont)
		embeddedFont->getTextBounds("",fontSize,w,h);
	else
		CairoPangoRenderer::getBounds(*this,"", w, h);
	
	uint32_t index=0;
	do
	{
		textline line;
		line.format = newformat;
		line.autosizeposition=0;
		line.textwidth=UINT32_MAX;
		line.needsnewline = t.getLine(index,line.text);
		textlines.push_back(line);
	}
	while (!condenseWhite && index != tiny_string::npos && !firstlineonly);
}

void TextData::appendFormatText(const char *text, const FormatText& format, uint32_t swfversion, bool condensewhite)
{
	appendText(text, false, &format, swfversion, condensewhite);
}

void TextData::appendLineBreak(bool needsadditionalbreak, bool emptyline,FormatText format)
{
	if (!emptyline)
	{
		if (!textlines.empty())
		{
			textlines.back().linebreaks++;
			needsadditionalbreak &= (textlines.back().format.paragraph);
			format = textlines.back().format;
		}
		else
			needsadditionalbreak=true;
	}
	if (needsadditionalbreak || emptyline)
	{
		if (format.bullet && !textlines.empty() && !textlines.back().format.paragraph && !textlines.back().format.bullet)
			textlines.back().needsnewline=true;
		textline line;
		line.format = format;
		line.autosizeposition=0;
		line.textwidth=UINT32_MAX;
		line.linebreaks=1;
		textlines.push_back(line);
	}
}

void TextData::clear()
{
	textlines.clear();
}

bool TextData::isWhitespaceOnly(bool multiline) const
{
	for (auto it = textlines.begin(); it != textlines.end(); it++)
	{
		if (!it->text.isWhiteSpaceOnly())
			return false;
		if (multiline && (it->format.paragraph || it->format.bullet))
		  	return false;
	}
	return true;
}

void TextData::getTextSizes(const tiny_string& text, number_t& tw, number_t& th)
{
	if (embeddedFont)
		embeddedFont->getTextBounds(text,fontSize,tw,th);
	else
		CairoPangoRenderer::getBounds(*this,text, tw, th);
	
}

bool TextData::TextIsEqual(const std::vector<tiny_string>& lines) const
{
	if (this->textlines.size() != lines.size())
		return false;
	for (uint32_t i = 0; i < this->textlines.size(); i++)
	{
		if (this->textlines[i].text != lines[i])
			return false;
	}
	return true;
}

FontTag* TextData::checkEmbeddedFont(DisplayObject* d)
{
	ApplicationDomain* currentDomain=d->loadedFrom;
	if (!currentDomain) currentDomain = d->getSystemState()->mainClip->applicationDomain.getPtr();
	FontTag* embeddedfont = (fontID != UINT32_MAX ? currentDomain->getEmbeddedFontByID(fontID) : currentDomain->getEmbeddedFont(font));
	if (embeddedfont)
	{
		for (auto it = textlines.begin(); it != textlines.end(); it++)
		{
			if (!embeddedfont->hasGlyphs((*it).text))
			{
				embeddedfont=nullptr;
				break;
			}
		}
	}
	this->embeddedFont=embeddedfont;
	return embeddedfont;
}

void TextData::checklastline(bool needsadditionalline)
{
	if (!textlines.empty() && ((textlines.back().text.empty() && !textlines.back().linebreaks) || textlines.back().format.paragraph || textlines.back().format.bullet))
		textlines.back().needsnewline=true;
	if (needsadditionalline && swfversion < 8)
	{
		if (textlines.empty() || !textlines.back().text.empty())
		{
			textline line;
			line.autosizeposition=0;
			line.textwidth=UINT32_MAX;
			line.needsnewline=true;
			if (textlines.empty())
				line.format.paragraph=true;
			else
			{
				line.format.bullet = textlines.back().format.bullet;
				line.format.paragraph = textlines.back().format.paragraph;
				line.format.font = textlines.back().format.font;
				line.format.fontColor = textlines.back().format.fontColor;
				line.format.fontSize = textlines.back().format.fontSize;
				line.format.align = textlines.back().format.align;
			}
			textlines.push_back(line);
		}
	}
	if (!textlines.empty())
	{
		if ((textlines.back().text.empty() && !textlines.back().linebreaks) || textlines.back().format.paragraph)
			textlines.back().needsnewline=true;
		else if (!textlines.back().text.isWhiteSpaceOnly())
		{
			bool canhaveparagraph = !textlines.back().needsnewline;
			if (textlines.size() > 1 && !((textlines.end()-2)->needsnewline))
			 	canhaveparagraph =textlines.back().format.paramsChanged(&(textlines.end()-1)->format);
			if (canhaveparagraph)
				textlines.back().format.paragraph=true;
		}
	}
}

void ColorTransformBase::fillConcatenated(DisplayObject* src, bool ignoreBlendMode)
{
	ColorTransform* ct = src->colorTransform.getPtr();
	DisplayObjectContainer* p = src->getParent();
	if (ct)
	{
		*this=*ct;
	}
	while (p)
	{
		if (p->colorTransform)
		{
			ct = p->colorTransform.getPtr();
			if (!ct)
			{
				ct = p->colorTransform.getPtr();
				*this=*ct;
			}
			else
			{
				ct = p->colorTransform.getPtr();
				redMultiplier*=ct->redMultiplier;
				greenMultiplier*=ct->greenMultiplier;
				blueMultiplier*=ct->blueMultiplier;
				alphaMultiplier*=ct->alphaMultiplier;
				redOffset+=ct->redOffset;
				greenOffset+=ct->greenOffset;
				blueOffset+=ct->blueOffset;
				alphaOffset+=ct->alphaOffset;
			}
		}
		if (!ignoreBlendMode && p->isShaderBlendMode(p->getBlendMode()))
			break;
		p = p->getParent();
	}
}
void ColorTransformBase::applyTransformation(uint8_t* bm, uint32_t size) const
{
	if (isIdentity())
		return;

	for (uint32_t i = 0; i < size; i+=4)
	{
		bm[i+3] = max(0,min(255,int(((number_t(bm[i+3]) * alphaMultiplier) + alphaOffset))));
		bm[i+2] = max(0,min(255,int(((number_t(bm[i+2]) *  blueMultiplier) +  blueOffset)*(number_t(bm[i+3])/255.0))));
		bm[i+1] = max(0,min(255,int(((number_t(bm[i+1]) * greenMultiplier) + greenOffset)*(number_t(bm[i+3])/255.0))));
		bm[i  ] = max(0,min(255,int(((number_t(bm[i  ]) *   redMultiplier) +   redOffset)*(number_t(bm[i+3])/255.0))));
	}
}
uint8_t *ColorTransformBase::applyTransformation(BitmapContainer* bm)
{
	if (redMultiplier==1.0 &&
		greenMultiplier==1.0 &&
		blueMultiplier==1.0 &&
		alphaMultiplier==1.0 &&
		redOffset==0.0 &&
		greenOffset==0.0 &&
		blueOffset==0.0 &&
		alphaOffset==0.0)
		return (uint8_t*)bm->getData();

	const uint8_t* src = bm->getOriginalData();
	uint8_t* dst = bm->getDataColorTransformed();
	uint32_t size = bm->getWidth()*bm->getHeight()*4;
	for (uint32_t i = 0; i < size; i+=4)
	{
		dst[i+3] = max(0,min(255,int(((number_t(src[i+3]) * alphaMultiplier) + alphaOffset))));
		dst[i+2] = max(0,min(255,int(((number_t(src[i+2]) *  blueMultiplier) +  blueOffset)*(number_t(dst[i+3])/255.0))));
		dst[i+1] = max(0,min(255,int(((number_t(src[i+1]) * greenMultiplier) + greenOffset)*(number_t(dst[i+3])/255.0))));
		dst[i  ] = max(0,min(255,int(((number_t(src[i  ]) *   redMultiplier) +   redOffset)*(number_t(dst[i+3])/255.0))));
	}
	return (uint8_t*)bm->getDataColorTransformed();
}

InvalidateQueue::InvalidateQueue(_NR<DisplayObject> _cacheAsBitmapObject):cacheAsBitmapObject(_cacheAsBitmapObject)
{
}

InvalidateQueue::~InvalidateQueue()
{
}

_NR<DisplayObject> InvalidateQueue::getCacheAsBitmapObject() const
{
	return cacheAsBitmapObject;
}

bool FormatText::paramsChanged(const FormatText* f) const
{
	return
		bold!=f->bold ||
		italic!=f->italic ||
		underline!=f->underline ||
		!(fontColor==f->fontColor) ||
		fontSize!=f->fontSize ||
		font!=f->font ||
		kerning!=f->kerning ||
		letterspacing!=f->letterspacing ||
		url!=f->url ||
		target!=f->target;
}
