/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "TokenContainer.h"
#include "swf.h"
#include "backends/input.h"

using namespace lightspark;
using namespace std;

TokenContainer::TokenContainer(DisplayObject* _o) : owner(_o), scaling(1.0f)
{
}

TokenContainer::TokenContainer(DisplayObject* _o, const tokensVector& _tokens, float _scaling) :
	owner(_o), scaling(_scaling),
	tokens(_tokens.begin(),_tokens.end())
{
}

void TokenContainer::renderImpl(RenderContext& ctxt, bool maskEnabled, number_t t1, number_t t2, number_t t3, number_t t4) const
{
	owner->defaultRender(ctxt, maskEnabled);
}

/*! \brief Generate a vector of shapes from a SHAPERECORD list
* * \param cur SHAPERECORD list head
* * \param shapes a vector to be populated with the shapes */

void TokenContainer::FromShaperecordListToShapeVector(const std::vector<SHAPERECORD>& shapeRecords,
								  tokensVector& tokens,
								  const std::list<FILLSTYLE>& fillStyles,
								  const Vector2& offset, int scaling)
{
	int startX=offset.x;
	int startY=offset.y;
	unsigned int color0=0;
	unsigned int color1=0;

	ShapesBuilder shapesBuilder;

	for(unsigned int i=0;i<shapeRecords.size();i++)
	{
		const SHAPERECORD* cur=&shapeRecords[i];
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				Vector2 p1(startX,startY);
				startX+=cur->DeltaX * scaling;
				startY+=cur->DeltaY * scaling;
				Vector2 p2(startX,startY);

				if(color0)
					shapesBuilder.extendFilledOutlineForColor(color0,p1,p2);
				if(color1)
					shapesBuilder.extendFilledOutlineForColor(color1,p1,p2);
			}
			else
			{
				Vector2 p1(startX,startY);
				startX+=cur->ControlDeltaX * scaling;
				startY+=cur->ControlDeltaY * scaling;
				Vector2 p2(startX,startY);
				startX+=cur->AnchorDeltaX * scaling;
				startY+=cur->AnchorDeltaY * scaling;
				Vector2 p3(startX,startY);

				if(color0)
					shapesBuilder.extendFilledOutlineForColorCurve(color0,p1,p2,p3);
				if(color1)
					shapesBuilder.extendFilledOutlineForColorCurve(color1,p1,p2,p3);
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				startX=cur->MoveDeltaX * scaling + offset.x;
				startY=cur->MoveDeltaY * scaling + offset.y;
			}
/*			if(cur->StateLineStyle)
			{
				cur_path->state.validStroke=true;
				cur_path->state.stroke=cur->LineStyle;
			}*/
			if(cur->StateFillStyle1)
			{
				color1=cur->FillStyle1;
			}
			if(cur->StateFillStyle0)
			{
				color0=cur->FillStyle0;
			}
		}
	}

	shapesBuilder.outputTokens(fillStyles, tokens);
}

void TokenContainer::requestInvalidation(InvalidateQueue* q)
{
	if(tokens.empty())
		return;
	owner->incRef();
	q->addToInvalidateQueue(_MR(owner));
}

void TokenContainer::invalidate()
{
	int32_t x,y;
	uint32_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax)==false)
	{
		//No contents, nothing to do
		return;
	}

	owner->computeDeviceBoundsForRect(bxmin,bxmax,bymin,bymax,x,y,width,height);
	if(width==0 || height==0)
		return;
	CairoRenderer* r=new CairoTokenRenderer(owner, owner->cachedSurface, tokens,
				owner->getConcatenatedMatrix(), x, y, width, height, scaling,
				owner->getConcatenatedAlpha());
	getSys()->addJob(r);
}

bool TokenContainer::isOpaqueImpl(number_t x, number_t y) const
{
	return CairoTokenRenderer::isOpaque(tokens, scaling, x, y);
}

_NR<InteractiveObject> TokenContainer::hitTestImpl(_NR<InteractiveObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type) const
{
	//TODO: test against the CachedSurface
	if(CairoTokenRenderer::hitTest(tokens, scaling, x, y))
	{
		if(getSys()->getInputThread()->isMaskPresent())
		{
			number_t globalX, globalY;
			owner->getConcatenatedMatrix().multiply2D(x,y,globalX,globalY);
			if(!getSys()->getInputThread()->isMasked(globalX, globalY))//You must be under the mask to be hit
				return NullRef;
		}
		return last;
	}
	return NullRef;
}

bool TokenContainer::boundsRect(number_t& xmin, number_t& xmax, number_t& ymin, number_t& ymax) const
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

	for(unsigned int i=0;i<tokens.size();i++)
	{
		switch(tokens[i].type)
		{
			case CURVE_CUBIC:
			{
				VECTOR_BOUNDS(tokens[i].p3);
				// fall through
			}
			case CURVE_QUADRATIC:
			{
				VECTOR_BOUNDS(tokens[i].p2);
				// fall through
			}
			case STRAIGHT:
			{
				hasContent = true;
				// fall through
			}
			case MOVE:
			{
				VECTOR_BOUNDS(tokens[i].p1);
				break;
			}
			case CLEAR_FILL:
			case CLEAR_STROKE:
			case SET_FILL:
			case FILL_KEEP_SOURCE:
			case FILL_TRANSFORM_TEXTURE:
				break;
			case SET_STROKE:
				strokeWidth = (double)(tokens[i].lineStyle.Width / 20.0);
				break;
		}
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
void TokenContainer::getTextureSize(int *width, int *height) const
{
	*width=0;
	*height=0;

	unsigned int len=tokens.size();
	for(unsigned int i=0;i<len;i++)
	{
		const FILLSTYLE& style=tokens[len-i-1].fillStyle;
		const FILL_STYLE_TYPE& fstype=style.FillStyleType;
		if(tokens[len-i-1].type==SET_FILL && 
		   (fstype==REPEATING_BITMAP ||
		    fstype==NON_SMOOTHED_REPEATING_BITMAP ||
		    fstype==CLIPPED_BITMAP ||
		    fstype==NON_SMOOTHED_CLIPPED_BITMAP) &&
		   style.bitmap)
		{
			*width=style.bitmap->getWidth();
			*height=style.bitmap->getHeight();
			return;
		}
	}
}
