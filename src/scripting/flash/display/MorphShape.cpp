/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/class.h"
#include "scripting/flash/display/MorphShape.h"
#include "parsing/tags.h"


using namespace std;
using namespace lightspark;


MorphShape::MorphShape(ASWorker* wrk, Class_base* c):DisplayObject(wrk,c),TokenContainer(this),morphshapetag(nullptr),currentratio(0)
{
	subtype=SUBTYPE_MORPHSHAPE;
	scaling = 1.0f/20.0f;
}

MorphShape::MorphShape(ASWorker* wrk,Class_base *c, DefineMorphShapeTag* _morphshapetag):DisplayObject(wrk,c),TokenContainer(this),morphshapetag(_morphshapetag),currentratio(0)
{
	subtype=SUBTYPE_MORPHSHAPE;
	scaling = 1.0f/20.0f;
	if (this->morphshapetag)
		this->morphshapetag->getTokensForRatio(tokens,0);
}

void MorphShape::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, DisplayObject, CLASS_SEALED | CLASS_FINAL);
}

IDrawable* MorphShape::invalidate(bool smoothing)
{
	return TokenContainer::invalidate(smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS : SMOOTH_MODE::SMOOTH_NONE,false);
}

bool MorphShape::boundsRect(number_t &xmin, number_t &xmax, number_t &ymin, number_t &ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
	if (!this->legacy || (morphshapetag==nullptr))
		return TokenContainer::boundsRect(xmin,xmax,ymin,ymax);
	float curratiofactor = float(currentratio)/65535.0;
	xmin=(morphshapetag->StartBounds.Xmin + (float(morphshapetag->EndBounds.Xmin - morphshapetag->StartBounds.Xmin)*curratiofactor))/20.0;
	xmax=(morphshapetag->StartBounds.Xmax + (float(morphshapetag->EndBounds.Xmax - morphshapetag->StartBounds.Xmax)*curratiofactor))/20.0;
	ymin=(morphshapetag->StartBounds.Ymin + (float(morphshapetag->EndBounds.Ymin - morphshapetag->StartBounds.Ymin)*curratiofactor))/20.0;
	ymax=(morphshapetag->StartBounds.Ymax + (float(morphshapetag->EndBounds.Ymax - morphshapetag->StartBounds.Ymax)*curratiofactor))/20.0;
	return true;
}

_NR<DisplayObject> MorphShape::hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type, bool interactiveObjectsOnly)
{
	number_t xmin, xmax, ymin, ymax;
	// TODO: Add an overload for RECT.
	boundsRect(xmin, xmax, ymin, ymax,false);
	//TODO: Add a point intersect function to RECT, and use that instead.
	if (localPoint.x<xmin || localPoint.x>xmax || localPoint.y<ymin || localPoint.y>ymax)
		return NullRef;
	if (TokenContainer::hitTestImpl(Vector2f(localPoint.x-xmin,localPoint.y-ymin)))
	{
		this->incRef();
		return _MR(this);
	}
	return NullRef;
}

void MorphShape::checkRatio(uint32_t ratio, bool inskipping)
{
	if (inskipping)
		return;
	currentratio = ratio;
	if (this->morphshapetag)
	{
		this->morphshapetag->getTokensForRatio(tokens,ratio);
		geometryChanged();
	}
	this->hasChanged = true;
	this->setNeedsTextureRecalculation(true);
	if (isOnStage())
		requestInvalidation(getSystemState());
}

uint32_t MorphShape::getTagID() const
{
	return morphshapetag ? morphshapetag->getId():UINT32_MAX;
}
