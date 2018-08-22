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

using namespace lightspark;
using namespace std;

TokenContainer::TokenContainer(DisplayObject* _o) : owner(_o),tokens(reporter_allocator<GeomToken>(_o->getSystemState()->tokenMemory)), scaling(1.0f)
{
}

TokenContainer::TokenContainer(DisplayObject* _o, const tokensVector& _tokens, float _scaling) :
	owner(_o), tokens(_tokens.begin(),_tokens.end(),reporter_allocator<GeomToken>(_o->getSystemState()->tokenMemory)), scaling(_scaling)

{
}

void TokenContainer::renderImpl(RenderContext& ctxt) const
{
	owner->defaultRender(ctxt);
}

/*! \brief Generate a vector of shapes from a SHAPERECORD list
* * \param cur SHAPERECORD list head
* * \param shapes a vector to be populated with the shapes */

void TokenContainer::FromShaperecordListToShapeVector(const std::vector<SHAPERECORD>& shapeRecords,
								  tokensVector& tokens,
								  const std::list<FILLSTYLE>& fillStyles,
								  const MATRIX& matrix,
								  const std::list<LINESTYLE2>& lineStyles)
{
	Vector2 cursor;
	unsigned int color0=0;
	unsigned int color1=0;
	unsigned int linestyle=0;
			
	ShapesBuilder shapesBuilder;

	for(unsigned int i=0;i<shapeRecords.size();i++)
	{
		const SHAPERECORD* cur=&shapeRecords[i];
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				Vector2 p1(matrix.multiply2D(cursor));
				cursor.x += cur->DeltaX;
				cursor.y += cur->DeltaY;
				Vector2 p2(matrix.multiply2D(cursor));

				if(color0)
					shapesBuilder.extendFilledOutlineForColor(color0,p1,p2);
				if(color1)
					shapesBuilder.extendFilledOutlineForColor(color1,p1,p2);
				if(linestyle)
					shapesBuilder.extendStrokeOutline(linestyle,p1,p2);
			}
			else
			{
				Vector2 p1(matrix.multiply2D(cursor));
				cursor.x += cur->ControlDeltaX;
				cursor.y += cur->ControlDeltaY;
				Vector2 p2(matrix.multiply2D(cursor));
				cursor.x += cur->AnchorDeltaX;
				cursor.y += cur->AnchorDeltaY;
				Vector2 p3(matrix.multiply2D(cursor));

				if(color0)
					shapesBuilder.extendFilledOutlineForColorCurve(color0,p1,p2,p3);
				if(color1)
					shapesBuilder.extendFilledOutlineForColorCurve(color1,p1,p2,p3);
				if(linestyle)
					shapesBuilder.extendStrokeOutlineCurve(linestyle,p1,p2,p3);
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				cursor.x=cur->MoveDeltaX;
				cursor.y=cur->MoveDeltaY;
			}
			if(cur->StateLineStyle)
			{
				linestyle = cur->LineStyle;
			}
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

	shapesBuilder.outputTokens(fillStyles,lineStyles, tokens);
}

void TokenContainer::FromDefineMorphShapeTagToShapeVector(SystemState* sys,DefineMorphShapeTag *tag, tokensVector &tokens, uint16_t ratio)
{
	LOG(LOG_NOT_IMPLEMENTED,"MorphShape currently ignores most morph settings and just displays the start/end shape. ID:"<<tag->getId()<<" ratio:"<<ratio);
	Vector2 cursor;
	unsigned int color0=0;
	unsigned int color1=0;
	unsigned int linestyle=0;

	const MATRIX matrix;
	ShapesBuilder shapesBuilder;

	// TODO compute SHAPERECORD entries based on ratio
	auto it = ratio == 65535 ? tag->EndEdges.ShapeRecords.begin() : tag->StartEdges.ShapeRecords.begin();
	auto last = ratio == 65535 ? tag->EndEdges.ShapeRecords.end() : tag->StartEdges.ShapeRecords.end();
	while (it != last)
	{
		const SHAPERECORD* cur=&(*it);
		it++;
		if(cur->TypeFlag)
		{
			if(cur->StraightFlag)
			{
				Vector2 p1(matrix.multiply2D(cursor));
				cursor.x += cur->DeltaX;
				cursor.y += cur->DeltaY;
				Vector2 p2(matrix.multiply2D(cursor));

				if(color0)
					shapesBuilder.extendFilledOutlineForColor(color0,p1,p2);
				if(color1)
					shapesBuilder.extendFilledOutlineForColor(color1,p1,p2);
				if(linestyle)
					shapesBuilder.extendStrokeOutline(linestyle,p1,p2);
			}
			else
			{
				Vector2 p1(matrix.multiply2D(cursor));
				cursor.x += cur->ControlDeltaX;
				cursor.y += cur->ControlDeltaY;
				Vector2 p2(matrix.multiply2D(cursor));
				cursor.x += cur->AnchorDeltaX;
				cursor.y += cur->AnchorDeltaY;
				Vector2 p3(matrix.multiply2D(cursor));

				if(color0)
					shapesBuilder.extendFilledOutlineForColorCurve(color0,p1,p2,p3);
				if(color1)
					shapesBuilder.extendFilledOutlineForColorCurve(color1,p1,p2,p3);
				if(linestyle)
					shapesBuilder.extendStrokeOutlineCurve(linestyle,p1,p2,p3);
			}
		}
		else
		{
			if(cur->StateMoveTo)
			{
				cursor.x=cur->MoveDeltaX;
				cursor.y=cur->MoveDeltaY;
			}
			if(cur->StateLineStyle)
			{
				linestyle = cur->LineStyle;
			}
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
	tokens.clear();
	shapesBuilder.outputMorphTokens(tag->MorphFillStyles.FillStyles,tag->MorphLineStyles.LineStyles2, tokens,ratio);
}

void TokenContainer::requestInvalidation(InvalidateQueue* q)
{
	if(tokens.empty())
		return;
	owner->incRef();
	q->addToInvalidateQueue(_MR(owner));
}

IDrawable* TokenContainer::invalidate(DisplayObject* target, const MATRIX& initialMatrix,bool smoothing)
{
	int32_t x,y;
	uint32_t width,height;
	number_t bxmin,bxmax,bymin,bymax;
	if(boundsRect(bxmin,bxmax,bymin,bymax)==false)
	{
		//No contents, nothing to do
		return NULL;
	}

	//Compute the matrix and the masks that are relevant
	MATRIX totalMatrix;
	std::vector<IDrawable::MaskData> masks;
	owner->computeMasksAndMatrix(target,masks,totalMatrix);
	totalMatrix=initialMatrix.multiplyMatrix(totalMatrix);
	owner->computeBoundsForTransformedRect(bxmin,bxmax,bymin,bymax,x,y,width,height,totalMatrix);
	if(width==0 || height==0)
		return NULL;
	return new CairoTokenRenderer(tokens,
				totalMatrix, x, y, width, height, scaling,
				owner->getConcatenatedAlpha(), masks,smoothing);
}

_NR<DisplayObject> TokenContainer::hitTestImpl(_NR<DisplayObject> last, number_t x, number_t y, DisplayObject::HIT_TYPE type) const
{
	//Masks have been already checked along the way

	if(CairoTokenRenderer::hitTest(tokens, scaling, x, y))
		return last;
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
			}
			// falls through
			case CURVE_QUADRATIC:
			{
				VECTOR_BOUNDS(tokens[i].p2);
			}
			// falls through
			case STRAIGHT:
			{
				hasContent = true;
			}
			// falls through
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
void TokenContainer::getTextureSize(tokensVector& tokens, int *width, int *height)
{
	*width=0;
	*height=0;

	for(int i=tokens.size()-1;i>=0;i--)
	{
		const FILLSTYLE& style=tokens[i].fillStyle;
		const FILL_STYLE_TYPE& fstype=style.FillStyleType;
		if(tokens[i].type==SET_FILL && 
		   (fstype==REPEATING_BITMAP ||
		    fstype==NON_SMOOTHED_REPEATING_BITMAP ||
		    fstype==CLIPPED_BITMAP ||
		    fstype==NON_SMOOTHED_CLIPPED_BITMAP))
		{
			if (style.bitmap.isNull())
				return;

			*width=style.bitmap->getWidth();
			*height=style.bitmap->getHeight();
			return;
		}
	}
}

/* Return the width of the latest SET_STROKE */
uint16_t TokenContainer::getCurrentLineWidth() const
{
	for(int i=tokens.size()-1;i>=0;i--)
	{
		if(tokens[i].type==SET_STROKE)
		{
			return tokens[i].lineStyle.Width;
		}
	}

	return 0;
}
