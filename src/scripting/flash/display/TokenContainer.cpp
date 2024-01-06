/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <cairo.h>
#include "scripting/flash/display/TokenContainer.h"
#include "scripting/flash/display/GraphicsSolidFill.h"
#include "scripting/toplevel/Vector.h"
#include "swf.h"
#include "scripting/flash/display/BitmapData.h"
#include "parsing/tags.h"
#include "backends/rendering.h"
#include "scripting/flash/geom/flashgeom.h"
#include "backends/lsopengl.h"
#include "3rdparty/nanovg/src/nanovg.h"
#include "3rdparty/nanovg/src/nanovg_gl.h"


using namespace lightspark;
using namespace std;

FORCE_INLINE bool isRepeating(FILL_STYLE_TYPE type)
{
	return type == FILL_STYLE_TYPE::NON_SMOOTHED_REPEATING_BITMAP || type == FILL_STYLE_TYPE::REPEATING_BITMAP;
}

FORCE_INLINE bool isSmoothed(FILL_STYLE_TYPE type)
{
	return type == FILL_STYLE_TYPE::REPEATING_BITMAP || type == FILL_STYLE_TYPE::CLIPPED_BITMAP;
}

FORCE_INLINE bool isSupportedGLBlendMode(AS_BLENDMODE mode)
{
	switch (mode)
	{
		case BLENDMODE_NORMAL:
		case BLENDMODE_LAYER:
		case BLENDMODE_MULTIPLY:
		case BLENDMODE_ADD:
		case BLENDMODE_SCREEN:
		case BLENDMODE_OVERLAY:
		case BLENDMODE_HARDLIGHT:
			return true;
			break;
		default:
			return false;
			break;
	}
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


TokenContainer::TokenContainer(DisplayObject* _o) : owner(_o)
  ,scaling(0.05),renderWithNanoVG(false)
{
}

TokenContainer::TokenContainer(DisplayObject* _o, const tokensVector& _tokens, float _scaling) : owner(_o)
	,scaling(_scaling),renderWithNanoVG(false)

{
	tokens.filltokens.assign(_tokens.filltokens.begin(),_tokens.filltokens.end());
	tokens.stroketokens.assign(_tokens.stroketokens.begin(),_tokens.stroketokens.end());
	tokens.canRenderToGL = _tokens.canRenderToGL;
}

bool TokenContainer::renderImpl(RenderContext& ctxt)
{
	SystemState* sys = owner->getSystemState();
	if (ctxt.contextType== RenderContext::GL && renderWithNanoVG)
	{
		NVGcontext* nvgctxt = sys->getEngineData()->nvgcontext;
		if (nvgctxt)
		{
			if (owner->getConcatenatedAlpha() == 0)
				return false;
			switch (owner->getBlendMode())
			{
				case BLENDMODE_NORMAL:
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
				default:
					LOG(LOG_NOT_IMPLEMENTED,"renderTextured of nanoVG blend mode "<<(int)owner->getBlendMode());
					break;
			}
			nvgResetTransform(nvgctxt);
			nvgBeginFrame(nvgctxt, sys->getRenderThread()->currentframebufferWidth, sys->getRenderThread()->currentframebufferHeight, 1.0);
			// xOffsetTransformed/yOffsetTransformed contain the offsets from the border of the window
			nvgTranslate(nvgctxt,owner->cachedSurface.xOffsetTransformed,owner->cachedSurface.yOffsetTransformed);
			MATRIX m = owner->cachedSurface.matrix;
			nvgTransform(nvgctxt,m.xx,m.yx,m.xy,m.yy,m.x0,m.y0);
			nvgTranslate(nvgctxt,owner->cachedSurface.xOffset,owner->cachedSurface.yOffset);
			nvgScale(nvgctxt,scaling,scaling);
			NVGcolor startcolor = nvgRGBA(0,0,0,0);
			nvgBeginPath(nvgctxt);
			nvgFillColor(nvgctxt,startcolor);
			nvgStrokeColor(nvgctxt,startcolor);
			bool instroke = false;
			bool infill = false;
			bool renderneeded=false;
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
						it = tokens.filltokens.begin();
						tokentype++;
						break;
					case 2:
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
							nvgMoveTo(nvgctxt, (p1.vec.x), (p1.vec.y));
							break;
						}
						case STRAIGHT:
						{
							renderneeded=true;
							GeomToken p1(*(++it),false);
							nvgLineTo(nvgctxt, (p1.vec.x), (p1.vec.y));
							break;
						}
						case CURVE_QUADRATIC:
						{
							renderneeded=true;
							GeomToken p1(*(++it),false);
							GeomToken p2(*(++it),false);
							nvgQuadTo(nvgctxt, (p1.vec.x), (p1.vec.y), (p2.vec.x), (p2.vec.y));
							break;
						}
						case CURVE_CUBIC:
						{
							renderneeded=true;
							GeomToken p1(*(++it),false);
							GeomToken p2(*(++it),false);
							GeomToken p3(*(++it),false);
							nvgBezierTo(nvgctxt, (p1.vec.x), (p1.vec.y), (p2.vec.x), (p2.vec.y), (p3.vec.x), (p3.vec.y));
							break;
						}
						case SET_FILL:
						{
							GeomToken p1(*(++it),false);
							if (renderneeded)
							{
								if (instroke)
									nvgStroke(nvgctxt);
								if (infill)
									nvgFill(nvgctxt);
								renderneeded=false;
								nvgClosePath(nvgctxt);
								nvgBeginPath(nvgctxt);
							}
							infill=true;
							const FILLSTYLE* style = p1.fillStyle;
							switch (style->FillStyleType)
							{
								case SOLID_FILL:
								{
									RGBA color = style->Color;
									float r,g,b,a;
									currentcolortransform.applyTransformation(color,r,g,b,a);
									NVGcolor c = nvgRGBA(r*255.0,g*255.0,b*255.0,a*owner->getConcatenatedAlpha()*255.0);
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
										pattern.xform[4] = m.x0/scaling - style->ShapeBounds.Xmin;
										pattern.xform[5] = m.y0/scaling - style->ShapeBounds.Ymin;
										float r,g,b,a;
										RGBA color(255,255,255,255);
										currentcolortransform.applyTransformation(color,r,g,b,a);
										NVGcolor c = nvgRGBA(r*255.0,g*255.0,b*255.0,a*255.0);
										pattern.innerColor = pattern.outerColor = c;
										nvgFillPaint(nvgctxt, pattern);
									}
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
								if (instroke)
									nvgStroke(nvgctxt);
								if (infill)
									nvgFill(nvgctxt);
								renderneeded=false;
								nvgClosePath(nvgctxt);
								nvgBeginPath(nvgctxt);
							}
							instroke = true;
							const LINESTYLE2* style = p1.lineStyle;
							if (style->HasFillFlag)
							{
								LOG(LOG_NOT_IMPLEMENTED,"nanovg linestyle with fill flag");
							}
							else
							{
								RGBA color = style->Color;
								float r,g,b,a;
								currentcolortransform.applyTransformation(color,r,g,b,a);
								NVGcolor c = nvgRGBA(r*255.0,g*255.0,b*255.0,a*owner->getConcatenatedAlpha()*255.0);
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
							nvgStrokeWidth(nvgctxt,style->Width==0 ? 1.0f :(float)style->Width);
							break;
						}
						case CLEAR_FILL:
						case FILL_KEEP_SOURCE:
						{
							if (renderneeded)
							{
								if (instroke)
									nvgStroke(nvgctxt);
								if (infill)
									nvgFill(nvgctxt);
								renderneeded=false;
								nvgClosePath(nvgctxt);
								nvgBeginPath(nvgctxt);
							}
							infill=false;
							if(p.type==CLEAR_FILL)
								nvgFillColor(nvgctxt,startcolor);
							break;
						}
						case CLEAR_STROKE:
							if (renderneeded)
							{
								if (instroke)
									nvgStroke(nvgctxt);
								if (infill)
									nvgFill(nvgctxt);
								renderneeded=false;
								nvgClosePath(nvgctxt);
								nvgBeginPath(nvgctxt);
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
			if (renderneeded)
			{
				if (instroke)
					nvgStroke(nvgctxt);
				if (infill)
					nvgFill(nvgctxt);
			}
			nvgClosePath(nvgctxt);
			nvgEndFrame(nvgctxt);
			sys->getEngineData()->exec_glStencilFunc_GL_ALWAYS();
			sys->getEngineData()->exec_glActiveTexture_GL_TEXTURE0(SAMPLEPOSITION::SAMPLEPOS_STANDARD);
			sys->getEngineData()->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_ALPHA);
			sys->getEngineData()->exec_glUseProgram(((RenderThread&)ctxt).gpu_program);
			((GLRenderContext&)ctxt).lsglLoadIdentity();
			((GLRenderContext&)ctxt).setMatrixUniform(GLRenderContext::LSGL_MODELVIEW);
			return false;
		}
	}
	return owner->defaultRender(ctxt);
}

/*! \brief Generate a vector of shapes from a SHAPERECORD list
* * \param cur SHAPERECORD list head
* * \param shapes a vector to be populated with the shapes */

void TokenContainer::FromShaperecordListToShapeVector(const std::vector<SHAPERECORD>& shapeRecords,
								  tokensVector& tokens,
								  const std::list<FILLSTYLE>& fillStyles,
								  const MATRIX& matrix,
								  const std::list<LINESTYLE2>& lineStyles,
								  const RECT& shapebounds)
{
	tokens.boundsRect = shapebounds;
	Vector2f cursor;
	unsigned int color0=0;
	unsigned int color1=0;
	unsigned int linestyle=0;

	ShapesBuilder shapesBuilder;

	cursor.x= -shapebounds.Xmin;
	cursor.y= -shapebounds.Ymin;
	Vector2f p1(matrix.multiply2D(cursor));
	for(unsigned int i=0;i<shapeRecords.size();i++)
	{
		const SHAPERECORD* cur=&shapeRecords[i];
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				cursor.x += cur->DeltaX;
				cursor.y += cur->DeltaY;
				Vector2f p2(matrix.multiply2D(cursor));
				shapesBuilder.extendOutline(p1, p2,linestyle);
				p1.x=p2.x;
				p1.y=p2.y;
			}
			else
			{
				cursor.x += cur->ControlDeltaX;
				cursor.y += cur->ControlDeltaY;
				Vector2f p2(matrix.multiply2D(cursor));
				cursor.x += cur->AnchorDeltaX;
				cursor.y += cur->AnchorDeltaY;
				Vector2f p3(matrix.multiply2D(cursor));

				shapesBuilder.extendOutlineCurve(p1, p2, p3,linestyle);
				p1.x=p3.x;
				p1.y=p3.y;
			}
		}
		else
		{
			shapesBuilder.endSubpathForStyles(color0, color1, linestyle,false);
			if(cur->StateMoveTo)
			{
				cursor.x= cur->MoveDeltaX-shapebounds.Xmin;
				cursor.y= cur->MoveDeltaY-shapebounds.Ymin;
				Vector2f tmp(matrix.multiply2D(cursor));
				p1.x = tmp.x;
				p1.y = tmp.y;
			}
			if(cur->StateLineStyle)
				linestyle = cur->LineStyle;
			if(cur->StateFillStyle1)
				color1=cur->FillStyle1;
			if(cur->StateFillStyle0)
				color0=cur->FillStyle0;
		}
	}

	shapesBuilder.outputTokens(fillStyles,lineStyles, tokens);
}

void TokenContainer::FromDefineMorphShapeTagToShapeVector(DefineMorphShapeTag *tag, tokensVector &tokens, uint16_t ratio)
{
	Vector2 cursor;
	unsigned int color0=0;
	unsigned int color1=0;
	unsigned int linestyle=0;

	const MATRIX matrix;
	ShapesBuilder shapesBuilder;
	float curratiofactor = float(ratio)/65535.0;

	int boundsx = tag->StartBounds.Xmin + (float(tag->EndBounds.Xmin - tag->StartBounds.Xmin)*curratiofactor);
	int boundsy = tag->StartBounds.Ymin + (float(tag->EndBounds.Ymin - tag->StartBounds.Ymin)*curratiofactor);
	RECT boundsrc;
	boundsrc.Xmin=boundsx;
	boundsrc.Ymin=boundsy;
	cursor.x= -boundsx;
	cursor.y= -boundsy;
	auto itstart = tag->StartEdges.ShapeRecords.begin();
	auto itend = tag->EndEdges.ShapeRecords.begin();
	Vector2 p1(matrix.multiply2D(cursor));
	while (itstart != tag->StartEdges.ShapeRecords.end() && itend != tag->EndEdges.ShapeRecords.end())
	{
		const SHAPERECORD* curstart=&(*itstart);
		const SHAPERECORD* curend=&(*itend);
		if(curstart->TypeFlag)
		{
			int controlX;
			int controlY;
			int anchorX;
			int anchorY;
			if(curstart->StraightFlag)
			{
				if (curend->StraightFlag)
				{
					controlX = curstart->DeltaX/2.0+float(curend->DeltaX/2.0-curstart->DeltaX/2.0)*curratiofactor;
					controlY = curstart->DeltaY/2.0+float(curend->DeltaY/2.0-curstart->DeltaY/2.0)*curratiofactor;
					anchorX = curstart->DeltaX/2.0+float(curend->DeltaX/2.0-curstart->DeltaX/2.0)*curratiofactor;
					anchorY = curstart->DeltaY/2.0+float(curend->DeltaY/2.0-curstart->DeltaY/2.0)*curratiofactor;
				}
				else
				{
					controlX = curstart->DeltaX/2.0+float(curend->ControlDeltaX-curstart->DeltaX/2.0)*curratiofactor;
					controlY = curstart->DeltaY/2.0+float(curend->ControlDeltaY-curstart->DeltaY/2.0)*curratiofactor;
					anchorX = curstart->DeltaX/2.0+float(curend->AnchorDeltaX-curstart->DeltaX/2.0)*curratiofactor;
					anchorY = curstart->DeltaY/2.0+float(curend->AnchorDeltaY-curstart->DeltaY/2.0)*curratiofactor;
				}
			}
			else
			{
				if (curend->StraightFlag)
				{
					controlX = curstart->ControlDeltaX+float(curend->DeltaX/2.0-curstart->ControlDeltaX)*curratiofactor;
					controlY = curstart->ControlDeltaY+float(curend->DeltaY/2.0-curstart->ControlDeltaY)*curratiofactor;
					anchorX = curstart->AnchorDeltaX+float(curend->DeltaX/2.0-curstart->AnchorDeltaX)*curratiofactor;
					anchorY = curstart->AnchorDeltaY+float(curend->DeltaY/2.0-curstart->AnchorDeltaY)*curratiofactor;
				}
				else
				{
					controlX = curstart->ControlDeltaX+float(curend->ControlDeltaX-curstart->ControlDeltaX)*curratiofactor;
					controlY = curstart->ControlDeltaY+float(curend->ControlDeltaY-curstart->ControlDeltaY)*curratiofactor;
					anchorX = curstart->AnchorDeltaX+float(curend->AnchorDeltaX-curstart->AnchorDeltaX)*curratiofactor;
					anchorY = curstart->AnchorDeltaY+float(curend->AnchorDeltaY-curstart->AnchorDeltaY)*curratiofactor;
				}
			}
			cursor.x += controlX;
			cursor.y += controlY;
			Vector2 p2(matrix.multiply2D(cursor));
			cursor.x += anchorX;
			cursor.y += anchorY;
			Vector2 p3(matrix.multiply2D(cursor));
			
			shapesBuilder.extendOutlineCurve(p1, p2, p3,linestyle);
			p1.x=p3.x;
			p1.y=p3.y;
			itstart++;
			itend++;
		}
		else
		{
			shapesBuilder.endSubpathForStyles(color0, color1, linestyle,true);
			if(curstart->StateMoveTo)
			{
				cursor.x=(curstart->MoveDeltaX-boundsx)+float(curend->MoveDeltaX-curstart->MoveDeltaX)*curratiofactor;
				cursor.y=(curstart->MoveDeltaY-boundsy)+float(curend->MoveDeltaY-curstart->MoveDeltaY)*curratiofactor;
				Vector2 tmp(matrix.multiply2D(cursor));
				p1.x = tmp.x;
				p1.y = tmp.y;
				itstart++;
				itend++;
			}
			else
				itstart++;
			if(curstart->StateLineStyle)
				linestyle = curstart->LineStyle;
			if(curstart->StateFillStyle1)
				color1=curstart->FillStyle1;
			if(curstart->StateFillStyle0)
				color0=curstart->FillStyle0;
		}
	}
	tokens.clear();
	shapesBuilder.outputMorphTokens(tag->MorphFillStyles.FillStyles,tag->MorphLineStyles.LineStyles2, tokens,ratio,boundsrc);
}

void TokenContainer::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if (owner->cachedAsBitmapOf && !q->isSoftwareQueue)
		return;
	if (!owner->computeCacheAsBitmap())
	{
		if(tokens.empty() || owner->skipRender())
			return;
	}
	owner->requestInvalidationFilterParent();
	
	if (owner->requestInvalidationForCacheAsBitmap(q))
		return;
	owner->incRef();
	if (forceTextureRefresh)
		owner->setNeedsTextureRecalculation();
	q->addToInvalidateQueue(_MR(owner));
}

IDrawable* TokenContainer::invalidate(DisplayObject* target, const MATRIX& initialMatrix, SMOOTH_MODE smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap, bool fromgraphics)
{
	if (owner->computeCacheAsBitmap() && (!q || !q->getCacheAsBitmapObject() || q->getCacheAsBitmapObject().getPtr()!=owner))
	{
		return owner->getCachedBitmapDrawable(target, initialMatrix, cachedBitmap, smoothing != SMOOTH_MODE::SMOOTH_NONE);
	}
	number_t x,y,rx,ry;
	number_t width,height;
	number_t rwidth,rheight;
	number_t bxmin,bxmax,bymin,bymax;
	if(!owner->boundsRectWithoutChildren(bxmin,bxmax,bymin,bymax,false))
	{
		//No contents, nothing to do
		return nullptr;
	}
	//Compute the matrix and the masks that are relevant
	MATRIX totalMatrix;
	MATRIX filterMatrix;
	
	std::vector<IDrawable::MaskData> masks;

	bool isMask=false;
	bool infilter=false;
	number_t alpha=1.0;
	_NR<DisplayObject> mask;
	if (target)
	{
		infilter=owner->computeMasksAndMatrix(target,masks,totalMatrix,false,isMask,mask,alpha,filterMatrix,initialMatrix);
		MATRIX initialNoRotation(initialMatrix.getScaleX(), initialMatrix.getScaleY());
		totalMatrix=initialNoRotation.multiplyMatrix(totalMatrix);
		totalMatrix.xx = abs(totalMatrix.xx);
		totalMatrix.yy = abs(totalMatrix.yy);
		totalMatrix.x0 = 0;
		totalMatrix.y0 = 0;
	}
	owner->computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,totalMatrix,infilter);

	if (isnan(width) || isnan(height))
	{
		// on stage with invalid contatenatedMatrix. Create a trash initial texture
		width = 1;
		height = 1;
	}

	MATRIX totalMatrix2;
	MATRIX filterMatrix2;
	std::vector<IDrawable::MaskData> masks2;
	if (target)
	{
		infilter = owner->computeMasksAndMatrix(target,masks2,totalMatrix2,true,isMask,mask,alpha,filterMatrix2,initialMatrix);
		totalMatrix2=initialMatrix.multiplyMatrix(totalMatrix2);
	}
	owner->computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,rx,ry,rwidth,rheight,totalMatrix2,infilter);
	if(width==0 || height==0)
		return nullptr;
	ColorTransformBase ct;
	ct.fillConcatenated(owner);
	
	Rectangle* r = owner->scalingGrid.getPtr();
	if (!r && owner->getParent())
		r = owner->getParent()->scalingGrid.getPtr();
	
	number_t regpointx = 0.0;
	number_t regpointy = 0.0;
	if (fromgraphics)
	{
		// the tokens are generated from graphics, so we have to translate them to the registration point of the sprite/shape
		regpointx=bxmin;
		regpointy=bymin;
	}
	owner->cachedSurface.isValid=true;
	if (!q || !q->isSoftwareQueue)
	{
		if (owner->getSystemState()->getEngineData()->nvgcontext
			&& !tokens.empty() 
			&& tokens.canRenderToGL
			&& mask.isNull()
			&& !isMask
			&& !owner->ClipDepth
			&& !owner->computeCacheAsBitmap()
			&& isSupportedGLBlendMode(owner->getBlendMode())
			&& !r
			&& !owner->getSystemState()->stage->renderToTextureCount
			&& !owner->hasFilters()
			)
		{
			currentcolortransform = ct;
			renderWithNanoVG=true;
			if (!infilter)
			{
				// in the NanoVG case xOffsetTransformed/yOffsetTransformed are used for the offsets from the border of the main window
				int offsetX;
				int offsetY;
				float scaleX;
				float scaleY;
				owner->getSystemState()->stageCoordinateMapping(owner->getSystemState()->getRenderThread()->windowWidth, owner->getSystemState()->getRenderThread()->windowHeight,
																offsetX, offsetY, scaleX, scaleY);
				owner->cachedSurface.xOffsetTransformed=offsetX;
				owner->cachedSurface.yOffsetTransformed=offsetY;
			}
			else
			{
				owner->cachedSurface.xOffsetTransformed=0;
				owner->cachedSurface.yOffsetTransformed=0;
			}
			if (fromgraphics)
			{
				owner->cachedSurface.xOffset=0;
				owner->cachedSurface.yOffset=0;
			}
			else
			{
				owner->cachedSurface.xOffset=bxmin;
				owner->cachedSurface.yOffset=bymin;
			}
			owner->cachedSurface.matrix=totalMatrix2;
			owner->cachedSurface.filtermatrix=filterMatrix2;
			owner->resetNeedsTextureRecalculation();
			return nullptr;
		}
		else if (renderWithNanoVG)
		{
			// this was previously rendered with nanoVG but some condition has changed, so we need to recalculate the owners texture
			owner->setNeedsTextureRecalculation();
			renderWithNanoVG=false;
		}
	}
	return new CairoTokenRenderer(tokens,totalMatrix2
				, x, y, ceil(width), ceil(height)
				, rx, ry, ceil(rwidth), ceil(rheight), 0
				, totalMatrix.getScaleX(), totalMatrix.getScaleY()
				, isMask, mask
				, scaling,(!q || !q->isSoftwareQueue ? owner->getConcatenatedAlpha() : alpha), masks
				, ct, smoothing, regpointx, regpointy,q && q->isSoftwareQueue,filterMatrix2);
}

bool TokenContainer::hitTestImpl(const Vector2f& point) const
{
	//Masks have been already checked along the way

	owner->startDrawJob(false); // ensure that tokens are not changed during hitTest
	if(CairoTokenRenderer::hitTest(tokens, scaling, point))
	{
		owner->endDrawJob(false);
		return true;
	}
	owner->endDrawJob(false);
	return false;
}

bool TokenContainer::boundsRectFromTokens(const tokensVector& tokens,float scaling, number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax)
{

	#define VECTOR_BOUNDS(v) \
		xmin=dmin(v.x-strokeWidth,xmin); \
		xmax=dmax(v.x+strokeWidth,xmax); \
		ymin=dmin(v.y-strokeWidth,ymin); \
		ymax=dmax(v.y+strokeWidth,ymax);

	if(tokens.size()==0)
		return false;

	xmin = numeric_limits<double>::infinity();
	ymin = numeric_limits<double>::infinity();
	xmax = -numeric_limits<double>::infinity();
	ymax = -numeric_limits<double>::infinity();

	bool hasContent = false;
	double strokeWidth = 0;

	auto it = tokens.filltokens.begin();
	while(it != tokens.filltokens.end())
	{
		GeomToken p(*it,false);
		switch(p.type)
		{
			case CURVE_CUBIC:
			{
				GeomToken p1(*(++it),false);
				VECTOR_BOUNDS(p1.vec);
			}
			// falls through
			case CURVE_QUADRATIC:
			{
				GeomToken p1(*(++it),false);
				VECTOR_BOUNDS(p1.vec);
			}
			// falls through
			case STRAIGHT:
			{
				hasContent = true;
			}
			// falls through
			case MOVE:
			{
				GeomToken p1(*(++it),false);
				VECTOR_BOUNDS(p1.vec);
				break;
			}
			case CLEAR_FILL:
			case CLEAR_STROKE:
			case FILL_KEEP_SOURCE:
				break;
			case SET_STROKE:
			{
				GeomToken p1(*(++it),false);
				strokeWidth = (double)(p1.lineStyle->Width);
				break;
			}
			case SET_FILL:
				it++;
				break;
			case FILL_TRANSFORM_TEXTURE:
				it+=6;
				break;
		}
		it++;
	}
	auto it2 = tokens.stroketokens.begin();
	while(it2 != tokens.stroketokens.end())
	{
		GeomToken p(*it2,false);
		switch(p.type)
		{
			case CURVE_CUBIC:
			{
				GeomToken p1(*(++it2),false);
				VECTOR_BOUNDS(p1.vec);
			}
			// falls through
			case CURVE_QUADRATIC:
			{
				GeomToken p1(*(++it2),false);
				VECTOR_BOUNDS(p1.vec);
			}
			// falls through
			case STRAIGHT:
			{
				hasContent = true;
			}
			// falls through
			case MOVE:
			{
				GeomToken p1(*(++it2),false);
				VECTOR_BOUNDS(p1.vec);
				break;
			}
			case CLEAR_FILL:
			case CLEAR_STROKE:
			case FILL_KEEP_SOURCE:
				break;
			case SET_STROKE:
			{
				GeomToken p1(*(++it2),false);
				strokeWidth = (double)(p1.lineStyle->Width);
				break;
			}
			case SET_FILL:
				it2++;
				break;
			case FILL_TRANSFORM_TEXTURE:
				it2+=6;
				break;
		}
		it2++;
	}
	if(hasContent)
	{
		xmin = round(xmin*scaling);
		xmax = round(xmax*scaling);
		ymin = round(ymin*scaling);
		ymax = round(ymax*scaling);
	}
	return hasContent;

#undef VECTOR_BOUNDS
}

/* Find the size of the active texture (bitmap set by the latest SET_FILL). */
void TokenContainer::getTextureSize(std::vector<uint64_t>& tokens, int *width, int *height)
{
	*width=0;
	*height=0;

	uint32_t lastindex=UINT32_MAX;
	for(uint32_t i=0;i<tokens.size();i++)
	{
		switch (GeomToken(tokens[i],false).type)
		{
			case SET_STROKE:
			case STRAIGHT:
			case MOVE:
				i++;
				break;
			case CURVE_QUADRATIC:
				i+=2;
				break;
			case CLEAR_FILL:
			case CLEAR_STROKE:
			case FILL_KEEP_SOURCE:
				break;
			case CURVE_CUBIC:
				i+=3;
				break;
			case FILL_TRANSFORM_TEXTURE:
				i+=6;
				break;
			case SET_FILL:
			{
				i++;
				const FILLSTYLE* style=GeomToken(tokens[i],false).fillStyle;
				const FILL_STYLE_TYPE& fstype=style->FillStyleType;
				if(fstype==REPEATING_BITMAP ||
					fstype==NON_SMOOTHED_REPEATING_BITMAP ||
					fstype==CLIPPED_BITMAP ||
					fstype==NON_SMOOTHED_CLIPPED_BITMAP)
				{
					lastindex=i;
				}
				break;
			}
		}
	}
	if (lastindex != UINT32_MAX)
	{
		const FILLSTYLE* style=GeomToken(tokens[lastindex],false).fillStyle;
		if (style->bitmap.isNull())
			return;
		*width=style->bitmap->getWidth();
		*height=style->bitmap->getHeight();
	}
}

/* Return the width of the latest SET_STROKE */
uint16_t TokenContainer::getCurrentLineWidth() const
{
	return tokens.currentLineWidth;
}

void TokenContainer::fillGraphicsData(Vector* v)
{
	for (auto it = tokens.filltokens.begin(); it != tokens.filltokens.end(); it++)
	{
		GeomToken t(*it,false); 
		switch (t.type)
		{
			case SET_STROKE:
			case STRAIGHT:
			case MOVE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it++;
				break;
			case CURVE_QUADRATIC:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it+=2;
				break;
			case CLEAR_FILL:
			case CLEAR_STROKE:
			case FILL_KEEP_SOURCE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				break;
			case CURVE_CUBIC:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it+=3;
				break;
			case FILL_TRANSFORM_TEXTURE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it+=6;
				break;
			case SET_FILL:
			{
				it++;
				const FILLSTYLE* style=GeomToken(*it,false).fillStyle;
				const FILL_STYLE_TYPE& fstype=style->FillStyleType;
				if (fstype == SOLID_FILL)
				{
					GraphicsSolidFill* f = Class<GraphicsSolidFill>::getInstanceSNoArgs(this->owner->getInstanceWorker());
					f->color =style->Color.Red<<16|style->Color.Green<<8|style->Color.Blue;
					f->alpha = style->Color.af();
					asAtom a=asAtomHandler::fromObjectNoPrimitive(f);
					v->append(a);
				}
				else
					LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for FillStyle Token type:"<<fstype);
				break;
			}
		}
	}
	for (auto it = tokens.stroketokens.begin(); it != tokens.stroketokens.end(); it++)
	{
		GeomToken t(*it,false); 
		switch (t.type)
		{
			case SET_STROKE:
			case STRAIGHT:
			case MOVE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it++;
				break;
			case CURVE_QUADRATIC:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it+=2;
				break;
			case CLEAR_FILL:
			case CLEAR_STROKE:
			case FILL_KEEP_SOURCE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				break;
			case CURVE_CUBIC:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it+=3;
				break;
			case FILL_TRANSFORM_TEXTURE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it+=6;
				break;
			case SET_FILL:
			{
				it++;
				const FILLSTYLE* style=GeomToken(*it,false).fillStyle;
				const FILL_STYLE_TYPE& fstype=style->FillStyleType;
				if (fstype == SOLID_FILL)
				{
					GraphicsSolidFill* f = Class<GraphicsSolidFill>::getInstanceSNoArgs(this->owner->getInstanceWorker());
					f->color =style->Color.Red<<16|style->Color.Green<<8|style->Color.Blue;
					f->alpha = style->Color.af();
					asAtom a=asAtomHandler::fromObjectNoPrimitive(f);
					v->append(a);
				}
				else
					LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for FillStyle Token type:"<<fstype);
				break;
			}
		}
		
	}
}
