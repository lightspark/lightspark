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
#include "backends/cachedsurface.h"
#include "platforms/engineutils.h"
#include "logger.h"
#include "exceptions.h"
#include "backends/rendering.h"
#include "backends/config.h"
#include "compat.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/display/Bitmap.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/filters/flashfilters.h"
#include "scripting/toplevel/Array.h"
#include "parsing/tags.h"
#include "backends/lsopengl.h"
#include "3rdparty/nanovg/src/nanovg.h"
#include "3rdparty/nanovg/src/nanovg_gl.h"
#include <algorithm>

using namespace lightspark;

SurfaceState::SurfaceState(float _xoffset, float _yoffset, float _alpha, float _xscale, float _yscale, const ColorTransformBase& _colortransform, const MATRIX& _matrix, bool _ismask, bool _cacheAsBitmap, AS_BLENDMODE _blendmode, SMOOTH_MODE _smoothing, float _scaling, bool _needsfilterrefresh, bool _needslayer)
	:xOffset(_xoffset),yOffset(_yoffset),alpha(_alpha),xscale(_xscale),yscale(_yscale)
	,colortransform(_colortransform),matrix(_matrix)
	,depth(0),clipdepth(0),maxfilterborder(0)
	,blendmode(_blendmode),smoothing(_smoothing),scaling(_scaling)
	,visible(true),allowAsMask(true),isMask(_ismask),cacheAsBitmap(_cacheAsBitmap)
	,needsFilterRefresh(_needsfilterrefresh),needsLayer(_needslayer),isYUV(false),renderWithNanoVG(false),hasOpaqueBackground(false)
{
#ifndef _NDEBUG
	src=nullptr;
#endif
}

SurfaceState::~SurfaceState()
{
	reset();
}

void SurfaceState::reset()
{
#ifndef _NDEBUG
	src=nullptr;
#endif
	xOffset=0.0;
	yOffset=0.0;
	alpha=1.0;
	xscale=1.0;
	yscale=1.0;
	colortransform=ColorTransformBase();
	matrix=MATRIX();
	tokens.clear();
	childrenlist.clear();
	mask.reset();
	filters.clear();
	bounds = RectF();
	depth=0;
	clipdepth=0;
	maxfilterborder=0;
	smoothing=SMOOTH_MODE::SMOOTH_ANTIALIAS;
	blendmode= AS_BLENDMODE::BLENDMODE_NORMAL;
	scaling=TWIPS_SCALING_FACTOR;
	scrollRect=RECT();
	scalingGrid=RECT();
	visible=true;
	allowAsMask=true;
	isMask=false;
	renderWithNanoVG=false;
	cacheAsBitmap=false;
	needsFilterRefresh=true;
	needsLayer=false;
	hasOpaqueBackground=false;
}

void SurfaceState::setupChildrenList(std::vector<DisplayObject*>& dynamicDisplayList)
{
	childrenlist.clear();
	childrenlist.reserve(dynamicDisplayList.size());
	needsLayer=false;
	for (auto it=dynamicDisplayList.cbegin(); it != dynamicDisplayList.cend(); it++)
	{
		childrenlist.push_back((*it)->getCachedSurface());
		if (DisplayObject::isShaderBlendMode((*it)->getBlendMode()))
			needsLayer=true;
	}
}

FORCE_INLINE bool isRepeating(FILL_STYLE_TYPE type)
{
	return type == FILL_STYLE_TYPE::NON_SMOOTHED_REPEATING_BITMAP || type == FILL_STYLE_TYPE::REPEATING_BITMAP;
}

FORCE_INLINE bool isSmoothed(FILL_STYLE_TYPE type)
{
	return type == FILL_STYLE_TYPE::REPEATING_BITMAP || type == FILL_STYLE_TYPE::CLIPPED_BITMAP;
}

void nanoVGDeleteImage(int image)
{
	NVGcontext* nvgctxt = getSys()->getEngineData() ? getSys()->getEngineData()->nvgcontext : nullptr;
	if (nvgctxt)
		nvgDeleteImage(nvgctxt,image);
}

int setNanoVGImage(NVGcontext* nvgctxt,const FILLSTYLE* style)
{
	if (!style->bitmap)
		return -1;
	if (style->bitmap->nanoVGImageHandle == -1)
	{
		int imageFlags = NVG_IMAGE_GENERATE_MIPMAPS;
		if (!isSmoothed(style->FillStyleType))
			imageFlags |= NVG_IMAGE_NEAREST;
		if (isRepeating(style->FillStyleType))
			imageFlags |= NVG_IMAGE_REPEATX|NVG_IMAGE_REPEATY;
		style->bitmap->nanoVGImageHandle = nvgCreateImageRGBA(nvgctxt,style->bitmap->getWidth(),style->bitmap->getHeight(),imageFlags,style->bitmap->getData());
	}
	return style->bitmap->nanoVGImageHandle;
}

int toNanoVGSpreadMode(int spreadMode)
{
	switch (spreadMode)
	{
		case 0: return NVG_SPREAD_PAD; break; // PAD
		case 1: return NVG_SPREAD_REFLECT; break; // REFLECT
		case 2: return NVG_SPREAD_REPEAT; break; // REPEAT
		default: return -1; break;
	}
}

void CachedSurface::Render(SystemState* sys,RenderContext& ctxt, const MATRIX* startmatrix, RenderDisplayObjectToBitmapContainer* container)
{
	if (!state)
		return;
	if (!state->mask.isNull() && !state->mask->state)
		return;
	if (!state->maskee.isNull())
		return;
	if((!state->isMask && !state->clipdepth && !state->visible) || state->alpha==0.0 || (state->isMask && !state->clipdepth))
		return;
	MATRIX _matrix;
	if (startmatrix)
		_matrix = *startmatrix;
	else
		_matrix = state->matrix;
	_matrix.translate(-state->scrollRect.Xmin,-state->scrollRect.Ymin);
	if (container)
	{
		ctxt.transformStack().push(Transform2D(
			_matrix,
			container->ct ? *container->ct : state->colortransform,
			container->blendMode
			));
	}
	else
	{
		ctxt.transformStack().push(Transform2D(
			_matrix,
			state->colortransform,
			state->blendmode
			));
	}
	EngineData* engineData = sys->getEngineData();
	bool needscachedtexture = (!container && state->cacheAsBitmap)
							  || ctxt.transformStack().transform().blendmode == BLENDMODE_LAYER
							  || !state->filters.empty()
							  || DisplayObject::isShaderBlendMode(ctxt.transformStack().transform().blendmode)
							  || (state->needsLayer && sys->getRenderThread()->filterframebufferstack.empty());
	if (needscachedtexture && (state->needsFilterRefresh || cachedFilterTextureID != UINT32_MAX))
	{
		if (!isInitialized)
		{
			ctxt.transformStack().pop();
			return;
		}
		bool needsFilterRefresh = state->needsFilterRefresh && needscachedtexture;
		auto baseTransform = ctxt.transformStack().transform();
		Vector2f scale = sys->getRenderThread()->getScale();
		MATRIX initialMatrix;
		initialMatrix.scale(scale.x, scale.y);
		RectF bounds = boundsRectWithRenderTransform(baseTransform.matrix, initialMatrix);
		Vector2f offset(bounds.min.x-baseTransform.matrix.x0,bounds.min.y-baseTransform.matrix.y0);
		Vector2f size = bounds.size();
		
		if (needsFilterRefresh)
		{
			MATRIX m = baseTransform.matrix;
			m.x0 = -offset.x;
			m.y0 = -offset.y;
			bool maskactive = ctxt.isMaskActive();
			if (maskactive)
				ctxt.deactivateMask();
			this->renderFilters(sys,ctxt,size.x,size.y,m);
			if (maskactive)
				ctxt.activateMask();
		}
		if (cachedFilterTextureID != UINT32_MAX)
		{
			MATRIX m;
			m.x0 = std::round(baseTransform.matrix.x0+offset.x);
			m.y0 = std::round(baseTransform.matrix.y0+offset.y);
			if (state->src->getTagID()==531)
			{
				int x=0;
			}
			if (DisplayObject::isShaderBlendMode(state->blendmode))
			{
				assert (!sys->getRenderThread()->filterframebufferstack.empty());
				filterstackentry feparent = sys->getRenderThread()->filterframebufferstack.back();
				// set original texture as blend texture
				engineData->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_BLEND);
				engineData->exec_glBindTexture_GL_TEXTURE_2D(feparent.filtertextureID);
			}
			sys->getRenderThread()->setModelView(m);
			sys->getRenderThread()->setupRenderingState(state->alpha,ctxt.transformStack().transform().colorTransform,state->smoothing,state->blendmode);
			bool maskactive = ctxt.isMaskActive();
			if (maskactive)
			{
				// we have an active mask, so make sure it is applied to the texture being rendered
				engineData->exec_glEnable_GL_STENCIL_TEST();
				engineData->exec_glStencilMask(0x7f);
				engineData->exec_glStencilFunc(DEPTHSTENCIL_FUNCTION::DEPTHSTENCIL_EQUAL,0x80,0x80);
			}
			sys->getRenderThread()->renderTextureToFrameBuffer(cachedFilterTextureID,size.x,size.y,nullptr,nullptr,false,true,false);
			if (maskactive)
				engineData->exec_glDisable_GL_STENCIL_TEST();
			ctxt.transformStack().pop();
			return;
		}
	}
	
	SurfaceState* maskstate = state->mask.isNull() ? nullptr : state->mask->getState();
	if (maskstate && maskstate->clipdepth)
	{
		ctxt.transformStack().push(Transform2D(maskstate->matrix, ColorTransformBase(),AS_BLENDMODE::BLENDMODE_NORMAL));
		ctxt.pushMask();
		state->mask->renderImpl(sys,ctxt);
		ctxt.transformStack().pop();
		ctxt.activateMask();
	}
	renderImpl(sys,ctxt);
	if (maskstate && maskstate->clipdepth)
	{
		ctxt.deactivateMask();
		ctxt.popMask();
	}
	ctxt.transformStack().pop();
}
void CachedSurface::renderImpl(SystemState* sys,RenderContext& ctxt)
{
	if (state->scrollRect.Xmin || state->scrollRect.Xmax || state->scrollRect.Ymin || state->scrollRect.Ymax)
	{
		MATRIX m = ctxt.transformStack().transform().matrix;
		sys->getEngineData()->exec_glScissor(m.getTranslateX()+state->scrollRect.Xmin*m.getScaleX()
											 ,sys->getRenderThread()->windowHeight-m.getTranslateY()-state->scrollRect.Ymax*m.getScaleY()
											 ,(state->scrollRect.Xmax-state->scrollRect.Xmin)*m.getScaleX()
											 ,(state->scrollRect.Ymax-state->scrollRect.Ymin)*m.getScaleY());
	}
	// first look if we have tokens or bitmaps to render
	if (state->renderWithNanoVG)
	{
		NVGcontext* nvgctxt = sys->getEngineData()->nvgcontext;
		if (nvgctxt)
		{
			if (state->alpha == 0)
				return;
			ColorTransformBase ct = ctxt.transformStack().transform().colorTransform;
			nvgResetTransform(nvgctxt);
			nvgBeginFrame(nvgctxt, sys->getRenderThread()->currentframebufferWidth, sys->getRenderThread()->currentframebufferHeight, 1.0);
			if (!ctxt.isMaskActive() && !ctxt.isDrawingMask())
				nvgDeactivateClipping(nvgctxt);
			switch (ctxt.transformStack().transform().blendmode)
			{
				case BLENDMODE_NORMAL:
				case BLENDMODE_LAYER:
					nvgGlobalCompositeBlendFunc(nvgctxt,NVG_ONE,NVG_ONE_MINUS_SRC_ALPHA);
					break;
				case BLENDMODE_MULTIPLY:
					nvgGlobalCompositeBlendFunc(nvgctxt,NVG_DST_COLOR,NVG_ONE_MINUS_SRC_ALPHA);
					break;
				case BLENDMODE_ADD:
					nvgGlobalCompositeBlendFunc(nvgctxt,NVG_ONE,NVG_ONE);
					break;
				case BLENDMODE_SCREEN:
					nvgGlobalCompositeBlendFunc(nvgctxt,NVG_ONE,NVG_ONE_MINUS_SRC_COLOR);
					break;
				case BLENDMODE_ERASE:
					nvgGlobalCompositeBlendFunc(nvgctxt,NVG_ZERO,NVG_ONE_MINUS_SRC_ALPHA);
					break;
				case BLENDMODE_INVERT:
				 	nvgGlobalCompositeBlendFunc(nvgctxt,NVG_ONE_MINUS_DST_COLOR,NVG_ZERO);
				 	break;
				default:
					LOG(LOG_NOT_IMPLEMENTED,"renderTextured of nanoVG blend mode "<<(int)ctxt.transformStack().transform().blendmode);
					break;
			}
			if (sys->getRenderThread()->filterframebufferstack.empty())
			{
				if (!sys->getRenderThread()->getFlipVertical())
				{
					nvgTranslate(nvgctxt,0,sys->getRenderThread()->currentframebufferHeight);
					nvgScale(nvgctxt,1.0,-1.0);
				}
				Vector2f offset = sys->getRenderThread()->getOffset();
				nvgTranslate(nvgctxt,offset.x,offset.y);
			}
			MATRIX m = ctxt.transformStack().transform().matrix;
			nvgTransform(nvgctxt,m.xx,m.yx,m.xy,m.yy,m.x0,m.y0);
			nvgTranslate(nvgctxt,state->xOffset,state->yOffset);
			nvgScale(nvgctxt,state->scaling,state->scaling);
			NVGcolor startcolor = nvgRGBA(0,0,0,0);
			nvgBeginPath(nvgctxt);
			if (ctxt.isDrawingMask())
				nvgBeginClip(nvgctxt);
			nvgFillColor(nvgctxt,startcolor);
			nvgStrokeColor(nvgctxt,startcolor);
			bool instroke = false;
			bool infill = false;
			bool renderneeded=false;
			number_t strokescalex=1.0;
			number_t strokescaley=1.0;
			int tokentype = 1;
			while (tokentype)
			{
				std::vector<uint64_t>::const_iterator it;
				std::vector<uint64_t>::const_iterator itbegin;
				std::vector<uint64_t>::const_iterator itend;
				switch(tokentype)
				{
					case 1:
						itbegin = state->tokens.filltokens.begin();
						itend = state->tokens.filltokens.end();
						it = state->tokens.filltokens.begin();
						tokentype++;
						break;
					case 2:
						it = state->tokens.stroketokens.begin();
						itbegin = state->tokens.stroketokens.begin();
						itend = state->tokens.stroketokens.end();
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
							nvgMoveTo(nvgctxt, (p1.vec.x)*strokescalex, (p1.vec.y)*strokescaley);
							break;
						}
						case STRAIGHT:
						{
							renderneeded=true;
							GeomToken p1(*(++it),false);
							nvgLineTo(nvgctxt, (p1.vec.x)*strokescalex, (p1.vec.y)*strokescaley);
							break;
						}
						case CURVE_QUADRATIC:
						{
							renderneeded=true;
							GeomToken p1(*(++it),false);
							GeomToken p2(*(++it),false);
							nvgQuadTo(nvgctxt, (p1.vec.x)*strokescalex, (p1.vec.y)*strokescaley, (p2.vec.x)*strokescalex, (p2.vec.y)*strokescaley);
							break;
						}
						case CURVE_CUBIC:
						{
							renderneeded=true;
							GeomToken p1(*(++it),false);
							GeomToken p2(*(++it),false);
							GeomToken p3(*(++it),false);
							nvgBezierTo(nvgctxt, (p1.vec.x)*strokescalex, (p1.vec.y)*strokescaley, (p2.vec.x)*strokescalex, (p2.vec.y)*strokescaley, (p3.vec.x)*strokescalex, (p3.vec.y)*strokescaley);
							break;
						}
						case SET_FILL:
						{
							GeomToken p1(*(++it),false);
							if (renderneeded)
							{
								if (ctxt.isDrawingMask())
									nvgEndClip(nvgctxt);
								if (instroke)
									nvgStroke(nvgctxt);
								if (infill)
									nvgFill(nvgctxt);
								renderneeded=false;
								nvgClosePath(nvgctxt);
								nvgBeginPath(nvgctxt);
								if (ctxt.isDrawingMask())
									nvgBeginClip(nvgctxt);
							}
							if (strokescalex != 1.0)
							{
								nvgScale(nvgctxt,strokescalex,1.0);
								strokescalex = 1.0;
							}
							if (strokescaley != 1.0)
							{
								nvgScale(nvgctxt,2.0,strokescaley);
								strokescaley = 1.0;
							}
							infill=true;
							const FILLSTYLE* style = p1.fillStyle;
							switch (style->FillStyleType)
							{
								case SOLID_FILL:
								{
									RGBA color = style->Color;
									float r,g,b,a;
									ct.applyTransformation(color,r,g,b,a);
									NVGcolor c = nvgRGBA(r*255.0,g*255.0,b*255.0,a*255.0);
									nvgFillColor(nvgctxt,c);
									break;
								}
								case REPEATING_BITMAP:
								case CLIPPED_BITMAP:
								case NON_SMOOTHED_REPEATING_BITMAP:
								case NON_SMOOTHED_CLIPPED_BITMAP:
								{
									int img =setNanoVGImage(nvgctxt,style);
									if (img != -1)
									{
										MATRIX m = style->Matrix;
										NVGpaint pattern = nvgImagePattern(nvgctxt,
																		   0,
																		   0,
																		   style->bitmap->getWidth(),
																		   style->bitmap->getHeight(),
																		   0,
																		   img,
																		   1.0);
										pattern.xform[0] = m.xx;
										pattern.xform[1] = m.yx;
										pattern.xform[2] = m.xy;
										pattern.xform[3] = m.yy;
										pattern.xform[4] = m.x0/state->scaling - style->ShapeBounds.Xmin;
										pattern.xform[5] = m.y0/state->scaling - style->ShapeBounds.Ymin;
										float r,g,b,a;
										RGBA color(255,255,255,255);
										ct.applyTransformation(color,r,g,b,a);
										NVGcolor c = nvgRGBA(r*255.0,g*255.0,b*255.0,a*255.0);
										pattern.innerColor = pattern.outerColor = c;
										nvgFillPaint(nvgctxt, pattern);
									}
									break;
								}
								case LINEAR_GRADIENT:
								case RADIAL_GRADIENT:
								case FOCAL_RADIAL_GRADIENT:
								{
									bool isFocal = style->FillStyleType == FOCAL_RADIAL_GRADIENT;
									bool isLinear = style->FillStyleType == LINEAR_GRADIENT;
									MATRIX m = style->Matrix;
									const std::vector<GRADRECORD>& gradRecords = isFocal ? style->FocalGradient.GradientRecords : style->Gradient.GradientRecords;
									std::vector<NVGgradientStop> stops(gradRecords.size());
									std::transform(gradRecords.begin(), gradRecords.end(), stops.begin(), [&](const GRADRECORD& record)
									{
										float r, g, b, a;
										ct.applyTransformation(record.Color, r, g, b, a);
										return NVGgradientStop { nvgRGBAf(r, g, b, a), float(record.Ratio) / 255.0f };
									});
									if (stops.back().stop != 1.0) // ensure we end gradient with ratio 1.0
										stops.push_back(NVGgradientStop {stops.back().color,1.0});

									int spreadMode = toNanoVGSpreadMode(isFocal ? style->FocalGradient.SpreadMode : style->Gradient.SpreadMode);

									NVGpaint pattern;
									if (isLinear)
									{
										MATRIX tmp = m;
										tmp.x0 = m.x0/state->scaling - style->ShapeBounds.Xmin;
										tmp.y0 = m.y0/state->scaling - style->ShapeBounds.Ymin;
										Vector2f start = tmp.multiply2D(Vector2f(-16384.0, 0));
										Vector2f end = tmp.multiply2D(Vector2f(16384.0, 0));
										pattern = nvgLinearGradientStops(nvgctxt, start.x, start.y, end.x, end.y, stops.data(), stops.size(), spreadMode);
									}
									else
									{
										number_t x0 = isFocal ? style->FocalGradient.FocalPoint*16384.0 : 0.0;
										pattern = nvgRadialGradientStops(nvgctxt, x0, 0, 0, 16384.0, stops.data(), stops.size(), spreadMode);
										float xform[6] =
										{
											(float)m.xx,
											(float)m.yx,
											(float)m.xy,
											(float)m.yy,
											(float)m.x0/state->scaling - style->ShapeBounds.Xmin,
											(float)m.y0/state->scaling - style->ShapeBounds.Ymin,
										};
										nvgTransformMultiply(pattern.xform, xform);
									}
									nvgFillPaint(nvgctxt, pattern);
									break;
								}
								default:
									LOG(LOG_NOT_IMPLEMENTED,"nanovg fillstyle:"<<hex<<(int)style->FillStyleType);
									break;
							}
							break;
						}
						case SET_STROKE:
						{
							GeomToken p1(*(++it),false);
							if (renderneeded)
							{
								if (ctxt.isDrawingMask())
									nvgEndClip(nvgctxt);
								if (instroke)
									nvgStroke(nvgctxt);
								if (infill)
									nvgFill(nvgctxt);
								renderneeded=false;
								nvgClosePath(nvgctxt);
								nvgBeginPath(nvgctxt);
								if (ctxt.isDrawingMask())
									nvgBeginClip(nvgctxt);
							}
							instroke = true;
							const LINESTYLE2* style = p1.lineStyle;
							if (style->HasFillFlag)
							{
								switch (style->FillType.FillStyleType)
								{
									case SOLID_FILL:
									{
										RGBA color = style->FillType.Color;
										float r,g,b,a;
										ct.applyTransformation(color,r,g,b,a);
										NVGcolor c = nvgRGBA(r*255.0,g*255.0,b*255.0,a*255.0);
										nvgStrokeColor(nvgctxt,c);
										break;
									}
									case REPEATING_BITMAP:
									case CLIPPED_BITMAP:
									case NON_SMOOTHED_REPEATING_BITMAP:
									case NON_SMOOTHED_CLIPPED_BITMAP:
									{
										int img =setNanoVGImage(nvgctxt,&style->FillType);
										if (img != -1)
										{
											MATRIX m = style->FillType.Matrix;
											NVGpaint pattern = nvgImagePattern(nvgctxt,
																			   0,
																			   0,
																			   style->FillType.bitmap->getWidth(),
																			   style->FillType.bitmap->getHeight(),
																			   0,
																			   img,
																			   1.0);
											pattern.xform[0] = m.xx;
											pattern.xform[1] = m.yx;
											pattern.xform[2] = m.xy;
											pattern.xform[3] = m.yy;
											pattern.xform[4] = m.x0/state->scaling - style->FillType.ShapeBounds.Xmin;
											pattern.xform[5] = m.y0/state->scaling - style->FillType.ShapeBounds.Ymin;
											float r,g,b,a;
											RGBA color(255,255,255,255);
											ct.applyTransformation(color,r,g,b,a);
											NVGcolor c = nvgRGBA(r*255.0,g*255.0,b*255.0,a*255.0);
											pattern.innerColor = pattern.outerColor = c;
											nvgStrokePaint(nvgctxt, pattern);
										}
										break;
									}
									case LINEAR_GRADIENT:
									case RADIAL_GRADIENT:
									case FOCAL_RADIAL_GRADIENT:
									{
										auto fill = style->FillType;
										bool isFocal = fill.FillStyleType == FOCAL_RADIAL_GRADIENT;
										bool isLinear = fill.FillStyleType == LINEAR_GRADIENT;
										MATRIX m = fill.Matrix;
										const std::vector<GRADRECORD>& gradRecords = isFocal ? fill.FocalGradient.GradientRecords : fill.Gradient.GradientRecords;
										std::vector<NVGgradientStop> stops(gradRecords.size());
										std::transform(gradRecords.begin(), gradRecords.end(), stops.begin(), [&](const GRADRECORD& record)
										{
											float r, g, b, a;
											ct.applyTransformation(record.Color, r, g, b, a);
											return NVGgradientStop { nvgRGBAf(r, g, b, a), float(record.Ratio) / 255.0f };
										});

										int spreadMode = toNanoVGSpreadMode(isFocal ? fill.FocalGradient.SpreadMode : fill.Gradient.SpreadMode);

										NVGpaint pattern;
										if (isLinear)
										{
											MATRIX tmp = m;
											tmp.x0 = m.x0/state->scaling - fill.ShapeBounds.Xmin;
											tmp.y0 = m.y0/state->scaling - fill.ShapeBounds.Ymin;
											Vector2f start = tmp.multiply2D(Vector2f(-16384.0, 0));
											Vector2f end = tmp.multiply2D(Vector2f(16384.0, 0));
											pattern = nvgLinearGradientStops(nvgctxt, start.x, start.y, end.x, end.y, stops.data(), stops.size(), spreadMode);
										}
										else
										{
											number_t x0 = isFocal ? fill.FocalGradient.FocalPoint*16384.0 : 0.0;
											pattern = nvgRadialGradientStops(nvgctxt, x0, 0, 0, 16384.0, stops.data(), stops.size(), spreadMode);
											float xform[6] =
											{
												(float)m.xx,
												(float)m.yx,
												(float)m.xy,
												(float)m.yy,
												(float)m.x0/state->scaling - fill.ShapeBounds.Xmin,
												(float)m.y0/state->scaling - fill.ShapeBounds.Ymin,
											};
											nvgTransformMultiply(pattern.xform, xform);
										}
										nvgStrokePaint(nvgctxt, pattern);
										break;
									}
									default:
										LOG(LOG_NOT_IMPLEMENTED,"nanovg linestyle fillType:"<<hex<<(int)style->FillType.FillStyleType);
										break;
								}
							}
							else
							{
								RGBA color = style->Color;
								float r,g,b,a;
								ct.applyTransformation(color,r,g,b,a);
								NVGcolor c = nvgRGBA(r*255.0,g*255.0,b*255.0,a*255.0);
								nvgStrokeColor(nvgctxt,c);
							}
							// TODO: EndCapStyle
							if (style->StartCapStyle == 0)
								nvgLineCap(nvgctxt,NVG_ROUND);
							else if (style->StartCapStyle == 1)
								nvgLineCap(nvgctxt,NVG_BUTT);
							else if (style->StartCapStyle == 2)
								nvgLineCap(nvgctxt,NVG_SQUARE);
							if (style->JointStyle == 0)
								nvgLineJoin(nvgctxt, NVG_ROUND);
							else if (style->JointStyle == 1)
								nvgLineJoin(nvgctxt, NVG_BEVEL);
							else if (style->JointStyle == 2) {
								nvgLineJoin(nvgctxt, NVG_MITER);
								nvgMiterLimit(nvgctxt,style->MiterLimitFactor);
							}
							nvgStrokeWidth(nvgctxt,style->Width==0 ? 1.0/state->scaling :(float)style->Width);
							if (style->NoHScaleFlag)
							{
								strokescalex=m.getScaleX();
								nvgScale(nvgctxt,1.0/strokescalex,1.0);
							}
							if (style->NoVScaleFlag)
							{
								strokescaley=m.getScaleY();
								nvgScale(nvgctxt,1.0,1.0/strokescaley);
							}
							break;
						}
						case CLEAR_FILL:
						case FILL_KEEP_SOURCE:
						{
							if (renderneeded)
							{
								if (ctxt.isDrawingMask())
									nvgEndClip(nvgctxt);
								if (instroke)
									nvgStroke(nvgctxt);
								if (infill)
									nvgFill(nvgctxt);
								renderneeded=false;
								nvgClosePath(nvgctxt);
								nvgBeginPath(nvgctxt);
								if (ctxt.isDrawingMask())
									nvgBeginClip(nvgctxt);
							}
							if (strokescalex != 1.0)
							{
								nvgScale(nvgctxt,strokescalex,1.0);
								strokescalex = 1.0;
							}
							if (strokescaley != 1.0)
							{
								nvgScale(nvgctxt,1.0,strokescaley);
								strokescaley = 1.0;
							}
							infill=false;
							if(p.type==CLEAR_FILL)
								nvgFillColor(nvgctxt,startcolor);
							break;
						}
						case CLEAR_STROKE:
							if (renderneeded)
							{
								if (ctxt.isDrawingMask())
									nvgEndClip(nvgctxt);
								if (instroke)
									nvgStroke(nvgctxt);
								if (infill)
									nvgFill(nvgctxt);
								renderneeded=false;
								nvgClosePath(nvgctxt);
								nvgBeginPath(nvgctxt);
								if (ctxt.isDrawingMask())
									nvgBeginClip(nvgctxt);
							}
							if (strokescalex != 1.0)
							{
								nvgScale(nvgctxt,strokescalex,1.0);
								strokescalex = 1.0;
							}
							if (strokescaley != 1.0)
							{
								nvgScale(nvgctxt,2.0,strokescaley);
								strokescaley = 1.0;
							}
							instroke = false;
							nvgStrokeColor(nvgctxt,startcolor);
							break;
						default:
							assert(false);
					}
					it++;
				}
			}
			if (ctxt.isDrawingMask())
				nvgEndClip(nvgctxt);
			if (renderneeded)
			{
				if (instroke)
					nvgStroke(nvgctxt);
				if (infill)
					nvgFill(nvgctxt);
			}
			nvgClosePath(nvgctxt);
			if (!ctxt.isDrawingMask())
				nvgEndFrame(nvgctxt);
			sys->getEngineData()->exec_glStencilFunc_GL_ALWAYS();
			sys->getEngineData()->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_STANDARD);
			sys->getEngineData()->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_ALPHA);
			sys->getEngineData()->exec_glUseProgram(((RenderThread&)ctxt).gpu_program);
			((GLRenderContext&)ctxt).lsglLoadIdentity();
			((GLRenderContext&)ctxt).setMatrixUniform(GLRenderContext::LSGL_MODELVIEW);
		}
	}
	else
		defaultRender(ctxt);
	
	int clipDepth = 0;
	vector<pair<int, CachedSurface*>> clipDepthStack;
	//Now draw also the display list
	auto it= state->childrenlist.begin();
	for(;it!=state->childrenlist.end();++it)
	{
		CachedSurface* child = (*it).getPtr();
		SurfaceState* childstate = child->getState();
		if (!childstate)
			continue;
		int depth = childstate->depth;
		// Pop off masks (if any).
		while (!clipDepthStack.empty() && clipDepth > 0 && depth > clipDepth)
		{
			clipDepth = clipDepthStack.back().first;
			clipDepthStack.pop_back();
			
			ctxt.deactivateMask();
			ctxt.popMask();
		}
		
		if (childstate->clipdepth > 0 && childstate->allowAsMask)
		{
			// Push, and render this mask.
			clipDepthStack.push_back(make_pair(clipDepth, child));
			clipDepth = childstate->clipdepth;
			
			ctxt.pushMask();
			child->Render(sys,ctxt);
			ctxt.activateMask();
		}
		else if ((childstate->visible && !childstate->clipdepth && !childstate->isMask) || ctxt.isDrawingMask())
			child->Render(sys,ctxt);
	}
	
	// Pop remaining masks (if any).
	for_each(clipDepthStack.rbegin(), clipDepthStack.rend(), [&](pair<int, CachedSurface*>& it)
	{
		ctxt.deactivateMask();
		ctxt.popMask();
	});
	sys->getEngineData()->exec_glDisable_GL_SCISSOR_TEST();
}
void CachedSurface::renderFilters(SystemState* sys,RenderContext& ctxt, uint32_t w, uint32_t h, const MATRIX& m)
{
	// rendering of filters currently works as follows:
	// - generate fbo
	// - generate texture with computed width/height of this DisplayObject as window size and set it as color attachment for fbo
	// - render DisplayObject to texture
	// - set texture as "g_tex_filter1" in fragment shader
	// - generate two more textures with computed width/height of this DisplayObject as window size
	// - for every filter
	//   - for every step (blur, dropshadow...)
	//     - set uniforms for step
	//     - set one of the two textures as color attachment for fbo (use first generated texture in first step)
	//     - render to texture 
	//     - swap textures
	//   - render resulting texture to "g_tex_filter2"
	// - remember resulting texture in cachedSurface.cachedFilterTextureID
	
	if (w == 0 || h == 0)
		return;

	ctxt.createTransformStack();
	ctxt.transformStack().push(Transform2D(m,ColorTransformBase(),AS_BLENDMODE::BLENDMODE_NORMAL));

	EngineData* engineData = sys->getEngineData();
	if (cachedFilterTextureID != UINT32_MAX) // remove previously used texture
		sys->getRenderThread()->addDeletedTexture(cachedFilterTextureID);
	cachedFilterTextureID = UINT32_MAX;
	
	// render filter source to texture
	uint32_t filterTextureIDoriginal;
	engineData->exec_glGenTextures(1, &filterTextureIDoriginal);
	uint32_t filterframebuffer = engineData->exec_glGenFramebuffer();
	engineData->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_STANDARD);
	engineData->exec_glBindTexture_GL_TEXTURE_2D(filterTextureIDoriginal);
	engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(filterframebuffer);
	uint32_t filterrenderbuffer = engineData->exec_glGenRenderbuffer();
	engineData->exec_glBindRenderbuffer_GL_RENDERBUFFER(filterrenderbuffer);
	engineData->exec_glRenderbufferStorage_GL_RENDERBUFFER_GL_STENCIL_INDEX8(w, h);
	engineData->exec_glFramebufferRenderbuffer_GL_FRAMEBUFFER_GL_STENCIL_ATTACHMENT(filterrenderbuffer);
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_NEAREST();
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_NEAREST();
	engineData->exec_glFramebufferTexture2D_GL_FRAMEBUFFER(filterTextureIDoriginal);
	engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(0, w, h, 0, nullptr,true);
	uint32_t parentframebufferWidth = sys->getRenderThread()->currentframebufferWidth;
	uint32_t parentframebufferHeight = sys->getRenderThread()->currentframebufferHeight;
	
	sys->getRenderThread()->setViewPort(w,h,true);
	if (state->hasOpaqueBackground)
		engineData->exec_glClearColor(float(state->opaqueBackground.Red)/255.0,float(state->opaqueBackground.Green)/255.0,float(state->opaqueBackground.Blue)/255.0,1.0);
	else
		engineData->exec_glClearColor(0,0,0,0);

	engineData->exec_glClear(CLEARMASK(CLEARMASK::COLOR|CLEARMASK::DEPTH|CLEARMASK::STENCIL));
	filterstackentry fe;
	fe.filterframebuffer=filterframebuffer;
	fe.filterrenderbuffer=filterrenderbuffer;
	fe.filtertextureID=filterTextureIDoriginal;
	
	Vector2f scale = sys->getRenderThread()->getScale();
	fe.filterborderx=(-state->bounds.min.x+state->maxfilterborder)*scale.x;
	fe.filterbordery=(-state->bounds.min.y+state->maxfilterborder)*scale.y;
	sys->getRenderThread()->filterframebufferstack.push_back(fe);
	renderImpl(sys,ctxt);
	// bind rendered filter source to g_tex_filter1
	engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(filterframebuffer);
	engineData->exec_glBindRenderbuffer_GL_RENDERBUFFER(filterrenderbuffer);
	engineData->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_FILTER);
	engineData->exec_glBindTexture_GL_TEXTURE_2D(filterTextureIDoriginal);
	
	// create filter output texture, and bind it to g_tex_filter2
	engineData->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_FILTER_DST);
	uint32_t filterDstTexture;
	engineData->exec_glGenTextures(1, &filterDstTexture);
	engineData->exec_glBindTexture_GL_TEXTURE_2D(filterDstTexture);
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_NEAREST();
	engineData->exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_NEAREST();
	engineData->exec_glFramebufferTexture2D_GL_FRAMEBUFFER(filterDstTexture);
	engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(0, w, h, 0, nullptr,true);
	
	// apply all filter steps
	engineData->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_STANDARD);
	uint32_t filterTextureID1;
	uint32_t filterTextureID2;
	engineData->exec_glGenTextures(1, &filterTextureID1);
	engineData->exec_glBindTexture_GL_TEXTURE_2D(filterTextureID1);
	engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(0, w, h, 0, nullptr,true);
	engineData->exec_glGenTextures(1, &filterTextureID2);
	engineData->exec_glBindTexture_GL_TEXTURE_2D(filterTextureID2);
	engineData->exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(0, w, h, 0, nullptr,true);
	sys->getRenderThread()->setViewPort(w,h,true);
	uint32_t texture1 = filterTextureIDoriginal;
	uint32_t texture2 = filterTextureID2;
	bool firstfilter = true;
	for (auto it = state->filters.begin(); it != state->filters.end(); it++)
	{
		if ((*it).filterdata[0] == 0) // end of filter
		{
			engineData->exec_glFramebufferTexture2D_GL_FRAMEBUFFER(filterDstTexture);
			engineData->exec_glClearColor(0,0,0,0);
			engineData->exec_glClear(CLEARMASK(CLEARMASK::COLOR|CLEARMASK::DEPTH|CLEARMASK::STENCIL));
			sys->getRenderThread()->renderTextureToFrameBuffer(texture1,w,h,nullptr,nullptr,false, false);
			firstfilter=false;
		}
		else
		{
			engineData->exec_glFramebufferTexture2D_GL_FRAMEBUFFER(texture2);
			engineData->exec_glClearColor(0,0,0,0);
			engineData->exec_glClear(CLEARMASK(CLEARMASK::COLOR|CLEARMASK::DEPTH|CLEARMASK::STENCIL));
			sys->getRenderThread()->renderTextureToFrameBuffer(texture1,w,h,((*it).filterdata),((*it).gradientcolors),firstfilter,false);
			if (texture1 == filterTextureIDoriginal)
				texture1 = filterTextureID1;
			std::swap(texture1,texture2);
		}
	}
	sys->getRenderThread()->filterframebufferstack.pop_back();
	if (sys->getRenderThread()->filterframebufferstack.empty())
	{
		sys->getRenderThread()->resetCurrentFrameBuffer();
		if (!sys->getRenderThread()->getFlipVertical())
			sys->getRenderThread()->setViewPort(parentframebufferWidth,parentframebufferHeight,false);
		else
			sys->getRenderThread()->resetViewPort();
		engineData->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_STANDARD);
	}
	else
	{
		filterstackentry feparent = sys->getRenderThread()->filterframebufferstack.back();
		engineData->exec_glBindFramebuffer_GL_FRAMEBUFFER(feparent.filterframebuffer);
		engineData->exec_glBindRenderbuffer_GL_RENDERBUFFER(feparent.filterrenderbuffer);
		sys->getRenderThread()->setViewPort(parentframebufferWidth,parentframebufferHeight,true);
	}
	engineData->exec_glDeleteFramebuffers(1,&filterframebuffer);
	engineData->exec_glDeleteRenderbuffers(1,&filterrenderbuffer);
	cachedFilterTextureID=texture1;
	engineData->exec_glDeleteTextures(1,&texture2);
	engineData->exec_glDeleteTextures(1,&filterDstTexture);
	if (state->filters.empty())
		engineData->exec_glDeleteTextures(1,&filterTextureID1);
	else
		engineData->exec_glDeleteTextures(1,&filterTextureIDoriginal);
	ctxt.transformStack().pop();
	ctxt.removeTransformStack();
	state->needsFilterRefresh=false;
}
void CachedSurface::defaultRender(RenderContext& ctxt)
{
	const Transform2D& t = ctxt.transformStack().transform();
	if(!isValid || !isInitialized || !tex || !tex->isValid())
		return;
	if (tex->width == 0 || tex->height == 0)
		return ;
	
	ctxt.lsglLoadIdentity();
	ColorTransformBase ct = t.colorTransform;
	MATRIX m = t.matrix;
	ctxt.renderTextured(*tex, state->alpha, state->isYUV ? RenderContext::YUV_MODE : RenderContext::RGB_MODE,
						ct, false,0.0,RGB(),state->smoothing,m,state->scalingGrid,t.blendmode);
}
RectF CachedSurface::boundsRectWithRenderTransform(const MATRIX& matrix, const MATRIX& initialMatrix)
{
	RectF bounds = state->bounds;
	bounds *= matrix;
	for (auto child : state->childrenlist)
	{
		if (!child->state)
			continue;
		MATRIX m = matrix.multiplyMatrix(child->state->matrix);
		bounds = bounds._union(child->boundsRectWithRenderTransform(m, initialMatrix));
	}
	if (!state->filters.empty())
	{
		number_t filterborder = state->maxfilterborder;
		bounds.min.x -= filterborder*initialMatrix.getScaleX();
		bounds.max.x += filterborder*initialMatrix.getScaleX();
		bounds.min.y -= filterborder*initialMatrix.getScaleY();
		bounds.max.y += filterborder*initialMatrix.getScaleY();
	}
	return bounds;
}

CachedSurface::~CachedSurface()
{
	if (isChunkOwner)
	{
		if (tex)
			delete tex;
		if (state)
			delete state;
	}
	if (cachedFilterTextureID != UINT32_MAX)
 {
		SystemState* sys = getSys();
		if (sys && sys->getRenderThread())
			sys->getRenderThread()->addDeletedTexture(cachedFilterTextureID);
	}
}
