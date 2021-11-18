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

using namespace lightspark;
using namespace std;

TokenContainer::TokenContainer(DisplayObject* _o) : owner(_o), scaling(1.0f)
{
}

TokenContainer::TokenContainer(DisplayObject* _o, const tokensVector& _tokens, float _scaling) :
	owner(_o), scaling(_scaling)

{
	tokens.filltokens.assign(_tokens.filltokens.begin(),_tokens.filltokens.end());
	tokens.stroketokens.assign(_tokens.stroketokens.begin(),_tokens.stroketokens.end());
}

bool TokenContainer::renderImpl(RenderContext& ctxt) const
{
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
	Vector2 cursor;
	unsigned int color0=0;
	unsigned int color1=0;
	unsigned int linestyle=0;
	vector< vector<ShapePathSegment> >* outlinesForColor0=nullptr;
	vector<ShapePathSegment>* lastoutlinesForColor0=nullptr;
	vector< vector<ShapePathSegment> >* outlinesForColor1=nullptr;
	vector<ShapePathSegment>* lastoutlinesForColor1=nullptr;
	vector< vector<ShapePathSegment> >* outlinesForStroke=nullptr;
	vector<ShapePathSegment>* lastoutlinesForStroke=nullptr;

	ShapesBuilder shapesBuilder;

	cursor.x= -shapebounds.Xmin;
	cursor.y= -shapebounds.Ymin;
	Vector2 p1(matrix.multiply2D(cursor));
	for(unsigned int i=0;i<shapeRecords.size();i++)
	{
		const SHAPERECORD* cur=&shapeRecords[i];
		if(cur->TypeFlag)
		{
			if (outlinesForColor0 == outlinesForColor1)
			{
				lastoutlinesForColor0=nullptr;
				lastoutlinesForColor1=nullptr;
			}
			if(cur->StraightFlag)
			{
				cursor.x += cur->DeltaX;
				cursor.y += cur->DeltaY;
				Vector2 p2(matrix.multiply2D(cursor));

				if(color0)
					lastoutlinesForColor0=shapesBuilder.extendOutline(outlinesForColor0,p1,p2,lastoutlinesForColor0);
				if(color1)
					lastoutlinesForColor1=shapesBuilder.extendOutline(outlinesForColor1,p1,p2,lastoutlinesForColor1);
				if(linestyle)
					lastoutlinesForStroke=shapesBuilder.extendOutline(outlinesForStroke,p1,p2,lastoutlinesForStroke);
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

				if(color0)
					lastoutlinesForColor0=shapesBuilder.extendOutlineCurve(outlinesForColor0,p1,p2,p3,lastoutlinesForColor0);
				if(color1)
					lastoutlinesForColor1=shapesBuilder.extendOutlineCurve(outlinesForColor1,p1,p2,p3,lastoutlinesForColor1);
				if(linestyle)
					lastoutlinesForStroke=shapesBuilder.extendOutlineCurve(outlinesForStroke,p1,p2,p3,lastoutlinesForStroke);
				p1.x=p3.x;
				p1.y=p3.y;
			}
		}
		else
		{
			lastoutlinesForColor0=nullptr;
			lastoutlinesForColor1=nullptr;
			lastoutlinesForStroke=nullptr;
			if(cur->StateMoveTo)
			{
				cursor.x= cur->MoveDeltaX-shapebounds.Xmin;
				cursor.y= cur->MoveDeltaY-shapebounds.Ymin;
				Vector2 tmp(matrix.multiply2D(cursor));
				p1.x = tmp.x;
				p1.y = tmp.y;
			}
			if(cur->StateLineStyle)
			{
				linestyle = cur->LineStyle;
				if (linestyle)
					outlinesForStroke=&shapesBuilder.strokeShapesMap[linestyle];
			}
			if(cur->StateFillStyle1)
			{
				color1=cur->FillStyle1;
				if (color1)
					outlinesForColor1=&shapesBuilder.filledShapesMap[color1];
			}
			if(cur->StateFillStyle0)
			{
				color0=cur->FillStyle0;
				if (color0)
					outlinesForColor0=&shapesBuilder.filledShapesMap[color0];
			}
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
	vector< vector<ShapePathSegment> >* outlinesForColor0=nullptr;
	vector<ShapePathSegment>* lastoutlinesForColor0=nullptr;
	vector< vector<ShapePathSegment> >* outlinesForColor1=nullptr;
	vector<ShapePathSegment>* lastoutlinesForColor1=nullptr;
	vector< vector<ShapePathSegment> >* outlinesForStroke=nullptr;
	vector<ShapePathSegment>* lastoutlinesForStroke=nullptr;

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
			if (outlinesForColor0 == outlinesForColor1)
			{
				lastoutlinesForColor0=nullptr;
				lastoutlinesForColor1=nullptr;
			}
			if(curstart->StraightFlag)
			{
				cursor.x += curstart->DeltaX+(curend->DeltaX-curstart->DeltaX)*curratiofactor;
				cursor.y += curstart->DeltaY+(curend->DeltaY-curstart->DeltaY)*curratiofactor;
				Vector2 p2(matrix.multiply2D(cursor));

				if(color0)
					lastoutlinesForColor0=shapesBuilder.extendOutline(outlinesForColor0,p1,p2,lastoutlinesForColor0);
				if(color1)
					lastoutlinesForColor1=shapesBuilder.extendOutline(outlinesForColor1,p1,p2,lastoutlinesForColor1);
				if(linestyle)
					lastoutlinesForStroke=shapesBuilder.extendOutline(outlinesForStroke,p1,p2,lastoutlinesForStroke);
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

				if(color0)
					lastoutlinesForColor0=shapesBuilder.extendOutlineCurve(outlinesForColor0,p1,p2,p3,lastoutlinesForColor0);
				if(color1)
					lastoutlinesForColor1=shapesBuilder.extendOutlineCurve(outlinesForColor1,p1,p2,p3,lastoutlinesForColor1);
				if(linestyle)
					lastoutlinesForStroke=shapesBuilder.extendOutlineCurve(outlinesForStroke,p1,p2,p3,lastoutlinesForStroke);
				p1.x=p3.x;
				p1.y=p3.y;
			}
		}
		else
		{
			lastoutlinesForColor0=nullptr;
			lastoutlinesForColor1=nullptr;
			lastoutlinesForStroke=nullptr;
			if(curstart->StateMoveTo)
			{
				cursor.x=(curstart->MoveDeltaX-boundsx)+(curend->MoveDeltaX-curstart->MoveDeltaX)*curratiofactor;
				cursor.y=(curstart->MoveDeltaY-boundsy)+(curend->MoveDeltaY-curstart->MoveDeltaY)*curratiofactor;
				Vector2 tmp(matrix.multiply2D(cursor));
				p1.x = tmp.x;
				p1.y = tmp.y;
			}
			if(curstart->StateLineStyle)
			{
				linestyle = curstart->LineStyle;
				if (linestyle)
					outlinesForStroke=&shapesBuilder.strokeShapesMap[linestyle];
			}
			if(curstart->StateFillStyle1)
			{
				color1=curstart->StateFillStyle1;
				if (color1)
					outlinesForColor1=&shapesBuilder.filledShapesMap[color1];
			}
			if(curstart->StateFillStyle0)
			{
				color0=curstart->StateFillStyle0;
				if (color0)
					outlinesForColor0=&shapesBuilder.filledShapesMap[color0];
			}
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

IDrawable* TokenContainer::invalidate(DisplayObject* target, const MATRIX& initialMatrix, bool smoothing, InvalidateQueue* q, _NR<DisplayObject>* cachedBitmap, bool fromgraphics)
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

	float scalex;
	float scaley;
	int offx,offy;
	owner->getSystemState()->stageCoordinateMapping(owner->getSystemState()->getRenderThread()->windowWidth,owner->getSystemState()->getRenderThread()->windowHeight,offx,offy, scalex,scaley);

	bool isMask=false;
	_NR<DisplayObject> mask;
	if (target)
	{
		owner->computeMasksAndMatrix(target,masks,totalMatrix,false,isMask,mask);
		totalMatrix=initialMatrix.multiplyMatrix(totalMatrix);
	}
	owner->computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,totalMatrix);

	width = bxmax-bxmin;
	height = bymax-bymin;
	float rotation = owner->getConcatenatedMatrix().getRotation();
	float xscale = owner->getConcatenatedMatrix().getScaleX();
	float yscale = owner->getConcatenatedMatrix().getScaleY();
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
		if (q && q->isSoftwareQueue)
			totalMatrix2.translate(bxmin,bymin);
		owner->computeMasksAndMatrix(target,masks2,totalMatrix2,true,isMask,mask);
		totalMatrix2=initialMatrix.multiplyMatrix(totalMatrix2);
	}
	owner->computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,rx,ry,rwidth,rheight,totalMatrix2);
	ColorTransform* ct = owner->colorTransform.getPtr();
	DisplayObjectContainer* p = owner->getParent();
	while (!ct && p)
	{
		ct = p->colorTransform.getPtr();
		p = p->getParent();
	}
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
	number_t regpointx = 0.0;
	number_t regpointy = 0.0;
	if (fromgraphics)
	{
		// the tokens are generated from graphics, so we have to translate them to the registration point of the sprite/shape
		regpointx=bxmin;
		regpointy=bymin;
	}
	owner->cachedSurface.isValid=true;
	return new CairoTokenRenderer(tokens,totalMatrix2
				, x*scalex, y*scaley, ceil(width*scalex), ceil(height*scaley)
				, rx*scalex, ry*scaley, ceil(rwidth*scalex), ceil(rheight*scaley), rotation
				, xscale, yscale
				, isMask, mask
				, scaling,owner->getConcatenatedAlpha(), masks
				, redMultiplier,greenMultiplier,blueMultiplier,alphaMultiplier
				, redOffset,greenOffset,blueOffset,alphaOffset
				, smoothing, regpointx, regpointy);
}

_NR<DisplayObject> TokenContainer::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type) const
{
	//Masks have been already checked along the way

	owner->startDrawJob(); // ensure that tokens are not changed during hitTest
	if(CairoTokenRenderer::hitTest(tokens, scaling, x, y))
	{
		owner->endDrawJob();
		return last;
	}
	owner->endDrawJob();
	return NullRef;
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
				strokeWidth = (double)(p1.lineStyle->Width / 20.0);
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
				strokeWidth = (double)(p1.lineStyle->Width / 20.0);
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
