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
#include "platforms/engineutils.h"
#include "scripting/flash/display/TokenContainer.h"
#include "scripting/flash/display/GraphicsSolidFill.h"
#include "scripting/flash/display/GraphicsEndFill.h"
#include "scripting/flash/display/GraphicsPath.h"
#include "scripting/toplevel/Vector.h"
#include "swf.h"
#include "scripting/flash/display/BitmapData.h"
#include "parsing/tags.h"
#include "backends/rendering.h"
#include "scripting/flash/geom/flashgeom.h"
#include "backends/cachedsurface.h"


using namespace lightspark;
using namespace std;

extern int setNanoVGImage(NVGcontext* nvgctxt,const FILLSTYLE* style);

TokenContainer::TokenContainer(DisplayObject* _o) : owner(_o)
  ,scaling(0.05),renderWithNanoVG(false)
{
}

TokenContainer::TokenContainer(DisplayObject* _o, const tokensVector& _tokens, float _scaling) : owner(_o)
	,scaling(_scaling),renderWithNanoVG(false)

{
	tokens.filltokens.assign(_tokens.filltokens.begin(),_tokens.filltokens.end());
	tokens.stroketokens.assign(_tokens.stroketokens.begin(),_tokens.stroketokens.end());
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
	if(tokens.empty())
		return;
	owner->requestInvalidationFilterParent(q);
	
	owner->incRef();
	if (forceTextureRefresh)
		owner->setNeedsTextureRecalculation();
	q->addToInvalidateQueue(_MR(owner));
}

bool TokenContainer::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
{
	if (this->tokensEmpty())
		return false;
	xmin = this->tokens.boundsRect.Xmin*scaling;
	xmax = this->tokens.boundsRect.Xmax*scaling;
	ymin = this->tokens.boundsRect.Ymin*scaling;
	ymax = this->tokens.boundsRect.Ymax*scaling;
	return true;
}

IDrawable* TokenContainer::invalidate(SMOOTH_MODE smoothing, bool fromgraphics)
{
	number_t x,y;
	number_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(!owner->boundsRectWithoutChildren(bxmin,bxmax,bymin,bymax,false))
	{
		//No contents, nothing to do
		return nullptr;
	}
	MATRIX matrix = owner->getMatrix();
	bool isMask=false;
	MATRIX m;
	m.scale(matrix.getScaleX(),matrix.getScaleY());
	owner->computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,m);

	if (isnan(width) || isnan(height))
	{
		// on stage with invalid contatenatedMatrix. Create a trash initial texture
		width = 1;
		height = 1;
	}

	if(width==0 || height==0)
		return nullptr;
	ColorTransformBase ct;
	if (owner->colorTransform)
		ct = *owner->colorTransform.getPtr();
	
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
	if (owner->getSystemState()->getEngineData()->nvgcontext
		&& !tokens.empty()
		&& !r
		)
	{
		renderWithNanoVG=true;
		if (fromgraphics)
		{
			x=0;
			y=0;
		}
		else
		{
			x=bxmin;
			y=bymin;
		}
		owner->resetNeedsTextureRecalculation();
		IDrawable* ret = new RefreshableDrawable(x, y, ceil(width), ceil(height)
									   , matrix.getScaleX(), matrix.getScaleY()
									   , isMask, owner->cacheAsBitmap
									   , this->scaling, owner->getConcatenatedAlpha()
									   , ct, smoothing,owner->getBlendMode(),matrix);
		ret->getState()->tokens = this->tokens;
		ret->getState()->renderWithNanoVG = renderWithNanoVG;
		owner->resetNeedsTextureRecalculation();
		return ret;
	}
	else if (renderWithNanoVG)
	{
		// this was previously rendered with nanoVG but some condition has changed, so we need to recalculate the owners texture
		owner->setNeedsTextureRecalculation();
		renderWithNanoVG=false;
	}
	IDrawable* ret = new CairoTokenRenderer(tokens,matrix
				, x, y, ceil(width), ceil(height)
				, matrix.getScaleX(), matrix.getScaleY()
				, isMask, owner->cacheAsBitmap
				, scaling,owner->getConcatenatedAlpha()
				, ct, smoothing ? SMOOTH_ANTIALIAS : SMOOTH_NONE,owner->getBlendMode(), regpointx, regpointy);
	ret->getState()->renderWithNanoVG = renderWithNanoVG;
	return ret;
}

bool TokenContainer::hitTestImpl(const Vector2f& point) const
{
	//Masks have been already checked along the way

	owner->startDrawJob(); // ensure that tokens are not changed during hitTest
	if(CairoTokenRenderer::hitTest(tokens, scaling, point))
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

std::vector<uint64_t>::const_iterator beginGraphicsFill(std::vector<uint64_t>::const_iterator it, ASWorker* wrk,Vector* v, bool& infill)
{
	it++;
	infill=true;
	const FILLSTYLE* style=GeomToken(*it,false).fillStyle;
	const FILL_STYLE_TYPE& fstype=style->FillStyleType;
	if (fstype == SOLID_FILL)
	{
		GraphicsSolidFill* f = Class<GraphicsSolidFill>::getInstanceSNoArgs(wrk);
		f->color =style->Color.Red<<16|style->Color.Green<<8|style->Color.Blue;
		f->alpha = style->Color.af();
		asAtom a=asAtomHandler::fromObjectNoPrimitive(f);
		v->append(a);
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for FillStyle Token type:"<<fstype);
	return it;
}
void endGraphicsFill(GraphicsPath** currentpath, bool& infill, Vector* v, ASWorker* wrk)
{
	if (*currentpath)
	{
		asAtom a=asAtomHandler::fromObjectNoPrimitive(*currentpath);
		v->append(a);
		*currentpath=nullptr;
	}
	if (infill)
	{
		GraphicsEndFill* f = Class<GraphicsEndFill>::getInstanceSNoArgs(wrk);
		asAtom a=asAtomHandler::fromObjectNoPrimitive(f);
		v->append(a);
		infill=false;
	}
}
std::vector<uint64_t>::const_iterator addDrawCommand(GRAPHICSPATH_COMMANDTYPE cmd, int argcount, std::vector<uint64_t>::const_iterator it, GraphicsPath** currentpath, ASWorker* wrk, const MATRIX& matrix, number_t scale)
{
	if (*currentpath==nullptr)
	{
		*currentpath = Class<GraphicsPath>::getInstanceSNoArgs(wrk);
		(*currentpath)->ensureValid();
	}
	asAtom c = asAtomHandler::fromInt(cmd);
	(*currentpath)->commands->append(c);
	for (int i = 0; i < argcount; i++)
	{
		GeomToken p(*(++it),false);
		Vector2f v(p.vec.x*scale,p.vec.y*scale);
		v = matrix.multiply2D(v);
		asAtom d1 = asAtomHandler::fromNumber(wrk,v.x,false);
		(*currentpath)->data->append(d1);
		asAtom d2 = asAtomHandler::fromNumber(wrk,v.y,false);
		(*currentpath)->data->append(d2);
	}
	return it;
}
void TokenContainer::fillGraphicsData(Vector* v)
{
	ASWorker* wrk = owner->getInstanceWorker();
	MATRIX m;
	// TODO it seems that contrary to specs the coordinates are _not_ in relation to stage
	m.translate(owner->tx+tokens.boundsRect.Xmin*this->scaling,owner->ty+tokens.boundsRect.Ymin*this->scaling);	

	GraphicsPath* currentpath=nullptr;
	bool infill=false;
	for (auto it = tokens.filltokens.cbegin(); it != tokens.filltokens.cend(); it++)
	{
		GeomToken t(*it,false); 
		switch (t.type)
		{
			case SET_STROKE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it++;
				break;
			case STRAIGHT:
				it = addDrawCommand(GRAPHICSPATH_COMMANDTYPE::LINE_TO,1,it,&currentpath,wrk,m,this->scaling);
				break;
			case MOVE:
				it = addDrawCommand(GRAPHICSPATH_COMMANDTYPE::MOVE_TO,1,it,&currentpath,wrk,m,this->scaling);
				break;
			case CURVE_QUADRATIC:
				it = addDrawCommand(GRAPHICSPATH_COMMANDTYPE::CURVE_TO,2,it,&currentpath,wrk,m,this->scaling);
				break;
			case CLEAR_FILL:
				endGraphicsFill(&currentpath,infill,v,wrk);
				break;
			case CLEAR_STROKE:
			case FILL_KEEP_SOURCE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				break;
			case CURVE_CUBIC:
				it = addDrawCommand(GRAPHICSPATH_COMMANDTYPE::CUBIC_CURVE_TO,3,it,&currentpath,wrk,m,this->scaling);
				break;
			case FILL_TRANSFORM_TEXTURE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it+=6;
				break;
			case SET_FILL:
				endGraphicsFill(&currentpath,infill,v,wrk);
				it = beginGraphicsFill(it,wrk,v,infill);
				break;
		}
	}
	endGraphicsFill(&currentpath,infill,v,wrk);
	for (auto it = tokens.stroketokens.cbegin(); it != tokens.stroketokens.cend(); it++)
	{
		GeomToken t(*it,false); 
		switch (t.type)
		{
			case SET_STROKE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it++;
				break;
			case STRAIGHT:
				it = addDrawCommand(GRAPHICSPATH_COMMANDTYPE::LINE_TO,1,it,&currentpath,wrk,m,this->scaling);
				break;
			case MOVE:
				it = addDrawCommand(GRAPHICSPATH_COMMANDTYPE::MOVE_TO,1,it,&currentpath,wrk,m,this->scaling);
				break;
			case CURVE_QUADRATIC:
				it = addDrawCommand(GRAPHICSPATH_COMMANDTYPE::CURVE_TO,2,it,&currentpath,wrk,m,this->scaling);
				break;
			case CLEAR_FILL:
				endGraphicsFill(&currentpath,infill,v,wrk);
				break;
			case CLEAR_STROKE:
			case FILL_KEEP_SOURCE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				break;
			case CURVE_CUBIC:
				it = addDrawCommand(GRAPHICSPATH_COMMANDTYPE::CUBIC_CURVE_TO,3,it,&currentpath,wrk,m,this->scaling);
				break;
			case FILL_TRANSFORM_TEXTURE:
				LOG(LOG_NOT_IMPLEMENTED,"fillGraphicsData for Token type:"<<t.type<<" "<<this->owner->toDebugString());
				it+=6;
				break;
			case SET_FILL:
			{
				endGraphicsFill(&currentpath,infill,v,wrk);
				it = beginGraphicsFill(it,wrk,v,infill);
				break;
			}
		}
	}
	endGraphicsFill(&currentpath,infill,v,wrk);
}
