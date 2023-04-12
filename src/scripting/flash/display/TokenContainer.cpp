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


TokenContainer::TokenContainer(DisplayObject* _o) : owner(_o)
  ,redMultiplier(1.0),greenMultiplier(1.0),blueMultiplier(1.0),alphaMultiplier(1.0)
  ,redOffset(0.0),greenOffset(0.0),blueOffset(0.0),alphaOffset(0.0)
  ,scaling(0.05),renderWithNanoVG(false)
{
}

TokenContainer::TokenContainer(DisplayObject* _o, const tokensVector& _tokens, float _scaling) : owner(_o)
	,redMultiplier(1.0),greenMultiplier(1.0),blueMultiplier(1.0),alphaMultiplier(1.0)
	,redOffset(0.0),greenOffset(0.0),blueOffset(0.0),alphaOffset(0.0)
	,scaling(_scaling),renderWithNanoVG(false)

{
	tokens.filltokens.assign(_tokens.filltokens.begin(),_tokens.filltokens.end());
	tokens.stroketokens.assign(_tokens.stroketokens.begin(),_tokens.stroketokens.end());
	tokens.canRenderToGL = _tokens.canRenderToGL;
}

bool TokenContainer::renderImpl(RenderContext& ctxt)
{
	if (ctxt.contextType== RenderContext::GL && renderWithNanoVG)
	{
		NVGcontext* nvgctxt = owner->getSystemState()->getEngineData()->nvgcontext;
		if (nvgctxt)
		{
			if (owner->getConcatenatedAlpha() == 0)
				return false;
			nvgResetTransform(nvgctxt);
			nvgBeginFrame(nvgctxt, owner->getSystemState()->getRenderThread()->windowWidth, owner->getSystemState()->getRenderThread()->windowHeight, 1.0);
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
								else
									nvgFill(nvgctxt);
								renderneeded=false;
								nvgClosePath(nvgctxt);
								nvgBeginPath(nvgctxt);
							}
							instroke=false;
							const FILLSTYLE* style = p1.fillStyle;
							switch (style->FillStyleType)
							{
								case SOLID_FILL:
								{
									RGBA color = style->Color;
									float r,g,b,a;
									a = max(0.0f,min(255.0f,float((color.Alpha * alphaMultiplier * 256.0f)/256.0f + alphaOffset)))/256.0f;
									r = max(0.0f,min(255.0f,float((color.Red   *   redMultiplier * 256.0f)/256.0f +   redOffset)))/256.0f;
									g = max(0.0f,min(255.0f,float((color.Green * greenMultiplier * 256.0f)/256.0f + greenOffset)))/256.0f;
									b = max(0.0f,min(255.0f,float((color.Blue  *  blueMultiplier * 256.0f)/256.0f +  blueOffset)))/256.0f;
									NVGcolor c = nvgRGBA(r*255.0,g*255.0,b*255.0,a*owner->getConcatenatedAlpha()*255.0);
									nvgFillColor(nvgctxt,c);
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
								else
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
								a = max(0.0f,min(255.0f,float((color.Alpha * alphaMultiplier * 256.0f)/256.0f + alphaOffset)))/256.0f;
								r = max(0.0f,min(255.0f,float((color.Red   *   redMultiplier * 256.0f)/256.0f +   redOffset)))/256.0f;
								g = max(0.0f,min(255.0f,float((color.Green * greenMultiplier * 256.0f)/256.0f + greenOffset)))/256.0f;
								b = max(0.0f,min(255.0f,float((color.Blue  *  blueMultiplier * 256.0f)/256.0f +  blueOffset)))/256.0f;
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
							if (renderneeded)
							{
								if (instroke)
									nvgStroke(nvgctxt);
								else
									nvgFill(nvgctxt);
								renderneeded=false;
								nvgClosePath(nvgctxt);
								nvgBeginPath(nvgctxt);
							}
							if(p.type==CLEAR_FILL)
								nvgFillColor(nvgctxt,startcolor);
							break;
						case CLEAR_STROKE:
							if (renderneeded)
							{
								if (instroke)
									nvgStroke(nvgctxt);
								else
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
				else
					nvgFill(nvgctxt);
			}
			nvgClosePath(nvgctxt);
			nvgEndFrame(nvgctxt);
			owner->getSystemState()->getEngineData()->exec_glStencilFunc_GL_ALWAYS();
			owner->getSystemState()->getEngineData()->exec_glActiveTexture_GL_TEXTURE0(0);
			owner->getSystemState()->getEngineData()->exec_glBlendFunc(BLEND_ONE,BLEND_ONE_MINUS_SRC_ALPHA);
			owner->getSystemState()->getEngineData()->exec_glUseProgram(((RenderThread&)ctxt).gpu_program);
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
	Vector2 cursor;
	unsigned int color0=0;
	unsigned int color1=0;
	unsigned int linestyle=0;

	ShapesBuilder shapesBuilder;

	cursor.x= -shapebounds.Xmin;
	cursor.y= -shapebounds.Ymin;
	Vector2 p1(matrix.multiply2D(cursor));
	for(unsigned int i=0;i<shapeRecords.size();i++)
	{
		const SHAPERECORD* cur=&shapeRecords[i];
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				cursor.x += cur->DeltaX;
				cursor.y += cur->DeltaY;
				Vector2 p2(matrix.multiply2D(cursor));
				shapesBuilder.extendOutline(p1, p2);
				p1.x=p2.x;
				p1.y=p2.y;
			}
			else
			{
				cursor.x += cur->ControlDeltaX;
				cursor.y += cur->ControlDeltaY;
				Vector2 p2(matrix.multiply2D(cursor));
				cursor.x += cur->AnchorDeltaX;
				cursor.y += cur->AnchorDeltaY;
				Vector2 p3(matrix.multiply2D(cursor));

				shapesBuilder.extendOutlineCurve(p1, p2, p3);
				p1.x=p3.x;
				p1.y=p3.y;
			}
		}
		else
		{
			shapesBuilder.endSubpathForStyles(color0, color1, linestyle);
			if(cur->StateMoveTo)
			{
				cursor.x= cur->MoveDeltaX-shapebounds.Xmin;
				cursor.y= cur->MoveDeltaY-shapebounds.Ymin;
				Vector2 tmp(matrix.multiply2D(cursor));
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
	auto itend = tag->StartEdges.ShapeRecords.size() > tag->EndEdges.ShapeRecords.size() ? tag->StartEdges.ShapeRecords.begin() : tag->EndEdges.ShapeRecords.begin();
	Vector2 p1(matrix.multiply2D(cursor));
	while (itstart != tag->StartEdges.ShapeRecords.end())
	{
		const SHAPERECORD* curstart=&(*itstart);
		const SHAPERECORD* curend=&(*itend);
		if(curstart->TypeFlag)
		{
			if(curstart->StraightFlag)
			{
				cursor.x += curstart->DeltaX+(curend->DeltaX-curstart->DeltaX)*curratiofactor;
				cursor.y += curstart->DeltaY+(curend->DeltaY-curstart->DeltaY)*curratiofactor;
				Vector2 p2(matrix.multiply2D(cursor));

				shapesBuilder.extendOutline(p1, p2);
				p1.x=p2.x;
				p1.y=p2.y;
			}
			else
			{
				cursor.x += curstart->ControlDeltaX+(curend->ControlDeltaX-curstart->ControlDeltaX)*curratiofactor;
				cursor.y += curstart->ControlDeltaY+(curend->ControlDeltaY-curstart->ControlDeltaY)*curratiofactor;
				Vector2 p2(matrix.multiply2D(cursor));
				cursor.x += curstart->AnchorDeltaX+(curend->AnchorDeltaX-curstart->AnchorDeltaX)*curratiofactor;
				cursor.y += curstart->AnchorDeltaY+(curend->AnchorDeltaY-curstart->AnchorDeltaY)*curratiofactor;
				Vector2 p3(matrix.multiply2D(cursor));

				shapesBuilder.extendOutlineCurve(p1, p2, p3);
				p1.x=p3.x;
				p1.y=p3.y;
			}
		}
		else
		{
			shapesBuilder.endSubpathForStyles(color0, color1, linestyle);
			if(curstart->StateMoveTo)
			{
				cursor.x=(curstart->MoveDeltaX-boundsx)+(curend->MoveDeltaX-curstart->MoveDeltaX)*curratiofactor;
				cursor.y=(curstart->MoveDeltaY-boundsy)+(curend->MoveDeltaY-curstart->MoveDeltaY)*curratiofactor;
				Vector2 tmp(matrix.multiply2D(cursor));
				p1.x = tmp.x;
				p1.y = tmp.y;
			}
			if(curstart->StateLineStyle)
				linestyle = curstart->LineStyle;
			if(curstart->StateFillStyle1)
				color1=curstart->StateFillStyle1;
			if(curstart->StateFillStyle0)
				color0=curstart->StateFillStyle0;
		}
		itstart++;
		itend++;
	}
	tokens.clear();
	shapesBuilder.outputMorphTokens(tag->MorphFillStyles.FillStyles,tag->MorphLineStyles.LineStyles2, tokens,ratio,boundsrc);
}

void TokenContainer::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if((tokens.empty() && !owner->computeCacheAsBitmap()) || owner->skipRender())
		return;
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
		return owner->getCachedBitmapDrawable(target, initialMatrix, cachedBitmap);
	}
	number_t x,y,rx,ry;
	number_t width,height;
	number_t rwidth,rheight;
	number_t bxmin,bxmax,bymin,bymax;
	if(!owner->boundsRectWithoutChildren(bxmin,bxmax,bymin,bymax))
	{
		//No contents, nothing to do
		return nullptr;
	}
	//Compute the matrix and the masks that are relevant
	MATRIX totalMatrix;
	std::vector<IDrawable::MaskData> masks;

	bool isMask=false;
	_NR<DisplayObject> mask;
	if (target)
	{
		owner->computeMasksAndMatrix(target,masks,totalMatrix,false,isMask,mask);
		MATRIX initialNoRotation(initialMatrix.getScaleX(), initialMatrix.getScaleY());
		totalMatrix=initialNoRotation.multiplyMatrix(totalMatrix);
		totalMatrix.xx = abs(totalMatrix.xx);
		totalMatrix.yy = abs(totalMatrix.yy);
		totalMatrix.x0 = 0;
		totalMatrix.y0 = 0;
	}
	owner->computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,totalMatrix);

	if (isnan(width) || isnan(height))
	{
		// on stage with invalid contatenatedMatrix. Create a trash initial texture
		width = 1;
		height = 1;
	}

	float redMultiplier=1.0;
	float greenMultiplier=1.0;
	float blueMultiplier=1.0;
	float alphaMultiplier=1.0;
	float redOffset=0.0;
	float greenOffset=0.0;
	float blueOffset=0.0;
	float alphaOffset=0.0;
	MATRIX totalMatrix2;
	std::vector<IDrawable::MaskData> masks2;
	if (target)
	{
		owner->computeMasksAndMatrix(target,masks2,totalMatrix2,true,isMask,mask);
		totalMatrix2=initialMatrix.multiplyMatrix(totalMatrix2);
	}
	owner->computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,rx,ry,rwidth,rheight,totalMatrix2);
	ColorTransform* ct = owner->colorTransform.getPtr();
	DisplayObjectContainer* p = owner->getParent();
	if(width==0 || height==0)
		return nullptr;
	if (ct)
	{
		redMultiplier=ct->redMultiplier;
		greenMultiplier=ct->greenMultiplier;
		blueMultiplier=ct->blueMultiplier;
		alphaMultiplier=ct->alphaMultiplier;
		redOffset=ct->redOffset;
		greenOffset=ct->greenOffset;
		blueOffset=ct->blueOffset;
		alphaOffset=ct->alphaOffset;
	}
	while (p)
	{
		if (p->colorTransform)
		{
			if (!ct)
			{
				ct = p->colorTransform.getPtr();
				redMultiplier=ct->redMultiplier;
				greenMultiplier=ct->greenMultiplier;
				blueMultiplier=ct->blueMultiplier;
				alphaMultiplier=ct->alphaMultiplier;
				redOffset=ct->redOffset;
				greenOffset=ct->greenOffset;
				blueOffset=ct->blueOffset;
				alphaOffset=ct->alphaOffset;
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
		p = p->getParent();
	}
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
	if (owner->getSystemState()->getEngineData()->nvgcontext && (!q || !q->isSoftwareQueue) 
			&& !tokens.empty() 
			&& tokens.canRenderToGL 
			&& mask.isNull()
			&& !isMask
			&& !owner->ClipDepth
			&& !owner->computeCacheAsBitmap() 
			&& owner->getBlendMode()==BLENDMODE_NORMAL
			&& !r)
	{
		this->redMultiplier=redMultiplier;
		this->greenMultiplier=greenMultiplier;
		this->blueMultiplier=blueMultiplier;
		this->alphaMultiplier=alphaMultiplier;
		this->redOffset=redOffset;
		this->greenOffset=greenOffset;
		this->blueOffset=blueOffset;
		this->alphaOffset=alphaOffset;
		renderWithNanoVG=true;
		int offsetX;
		int offsetY;
		float scaleX;
		float scaleY;
		owner->getSystemState()->stageCoordinateMapping(owner->getSystemState()->getRenderThread()->windowWidth, owner->getSystemState()->getRenderThread()->windowHeight,
					   offsetX, offsetY, scaleX, scaleY);
		// in the NanoVG case xOffsetTransformed/yOffsetTransformed are used for the offsets from the border of the main window
		owner->cachedSurface.xOffsetTransformed=offsetX;
		owner->cachedSurface.yOffsetTransformed=offsetY;
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
		owner->resetNeedsTextureRecalculation();
		return nullptr;
	}
	renderWithNanoVG=false;
	return new CairoTokenRenderer(tokens,totalMatrix2
				, x, y, ceil(width), ceil(height)
				, rx, ry, ceil(rwidth), ceil(rheight), 0
				, totalMatrix.getScaleX(), totalMatrix.getScaleY()
				, isMask, mask
				, scaling,owner->getConcatenatedAlpha(), masks
				, redMultiplier,greenMultiplier,blueMultiplier,alphaMultiplier
				, redOffset,greenOffset,blueOffset,alphaOffset
				, smoothing, regpointx, regpointy);
}

bool TokenContainer::hitTestImpl(number_t x, number_t y) const
{
	//Masks have been already checked along the way

	owner->startDrawJob(); // ensure that tokens are not changed during hitTest
	if(CairoTokenRenderer::hitTest(tokens, scaling, x, y))
	{
		owner->endDrawJob();
		return true;
	}
	owner->endDrawJob();
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
		/* scale the bounding box coordinates and round them to a bigger integer box */
		#define roundDown(x) \
			copysign(floor(abs(x)), x)
		#define roundUp(x) \
			copysign(ceil(abs(x)), x)
		xmin = roundDown(xmin*scaling);
		xmax = roundUp(xmax*scaling);
		ymin = roundDown(ymin*scaling);
		ymax = roundUp(ymax*scaling);
		#undef roundDown
		#undef roundUp
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
	uint32_t lastindex=UINT32_MAX;
	for(uint32_t i=0;i<tokens.stroketokens.size();i++)
	{
		switch (GeomToken(tokens.stroketokens[i],false).type)
		{
			case SET_FILL:
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
			case SET_STROKE:
			{
				i++;
				lastindex=i;
				break;
			}
		}
	}
	if (lastindex != UINT32_MAX)
		return GeomToken(tokens.stroketokens[lastindex],false).lineStyle->Width;
	
	return 0;
}
