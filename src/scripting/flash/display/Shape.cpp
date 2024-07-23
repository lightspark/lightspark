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
#include "scripting/flash/display/Shape.h"
#include "parsing/tags.h"

using namespace std;
using namespace lightspark;

bool Shape::boundsRect(number_t &xmin, number_t &xmax, number_t &ymin, number_t &ymax, bool visibleOnly)
{
	if (visibleOnly && !this->isVisible())
		return false;
	if (!this->legacy || (fromTag==nullptr))
	{
		if (graphics && graphics->hasBounds())
			return graphics->boundsRect(xmin,xmax,ymin,ymax);
		return TokenContainer::boundsRect(xmin,xmax,ymin,ymax,this->tokens);
	}
	xmin=fromTag->ShapeBounds.Xmin/20.0;
	xmax=fromTag->ShapeBounds.Xmax/20.0;
	ymin=fromTag->ShapeBounds.Ymin/20.0;
	ymax=fromTag->ShapeBounds.Ymax/20.0;
	return true;
}

_NR<DisplayObject> Shape::hitTestImpl(const Vector2f& globalPoint, const Vector2f& localPoint, HIT_TYPE type, bool interactiveObjectsOnly)
{
	number_t xmin, xmax, ymin, ymax;
	// TODO: Add an overload for RECT.
	boundsRect(xmin, xmax, ymin, ymax,false);
	//TODO: Add a point intersect function to RECT, and use that instead.
	if (localPoint.x<xmin || localPoint.x>xmax || localPoint.y<ymin || localPoint.y>ymax)
		return NullRef;
	if (!this->legacy || (fromTag==nullptr))
	{
		if (graphics && graphics->hasBounds())
		{
			if (graphics->hitTest(Vector2f(localPoint.x,localPoint.y)))
			{
				this->incRef();
				return _MR(this);
			}
			return NullRef;
		}
	}
	if (TokenContainer::hitTestImpl(Vector2f(localPoint.x-xmin,localPoint.y-ymin),tokens))
	{
		this->incRef();
		return _MR(this);
	}
	return NullRef;
}

void Shape::fillGraphicsData(Vector* v, bool recursive)
{
	if (graphics && graphics->hasBounds())
		graphics->fillGraphicsData(v);
	else
		TokenContainer::fillGraphicsData(v,this->tokens->filltokens,this->tokens->stroketokens,this->tokens->boundsRect.Xmin,this->tokens->boundsRect.Ymin);
}

Shape::Shape(ASWorker* wrk, Class_base* c):DisplayObject(wrk,c),TokenContainer(this),graphics(NullRef),fromTag(nullptr)
{
	subtype=SUBTYPE_SHAPE;
}

void Shape::setupShape(DefineShapeTag* tag, float _scaling)
{
	tokens = tag->tokens;
	fromTag = tag;
	scaling=_scaling;
}

uint32_t Shape::getTagID() const 
{
	return fromTag ? fromTag->getId() : UINT32_MAX; 
}

bool Shape::destruct()
{
	graphics.reset();
	fromTag=nullptr;
	tokens=nullptr;
	return DisplayObject::destruct();
}

void Shape::finalize()
{
	graphics.reset();
	tokens=nullptr;
	DisplayObject::finalize();
}

void Shape::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	DisplayObject::prepareShutdown();
	if (graphics)
		graphics->prepareShutdown();
}

void Shape::sinit(Class_base* c)
{
	CLASS_SETUP(c, DisplayObject, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("graphics","",c->getSystemState()->getBuiltinFunction(_getGraphics,0,Class<Graphics>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
}

void Shape::requestInvalidation(InvalidateQueue* q, bool forceTextureRefresh)
{
	if (graphics && graphics->hasBounds())
	{
		requestInvalidationFilterParent(q);
		incRef();
		q->addToInvalidateQueue(_MR(this));
		
	}
	TokenContainer::requestInvalidation(q,forceTextureRefresh);
}

void Shape::refreshSurfaceState()
{
	if (graphics)
		graphics->refreshSurfaceState();
}
IDrawable *Shape::invalidate(bool smoothing)
{
	IDrawable* res = nullptr;
	if (graphics && graphics->hasBounds())
		res = graphics->invalidate(smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS : SMOOTH_MODE::SMOOTH_NONE);
	else
		res = TokenContainer::invalidate(smoothing ? SMOOTH_MODE::SMOOTH_ANTIALIAS : SMOOTH_MODE::SMOOTH_NONE,false,*this->tokens);
	return res;
}

string Shape::toDebugString() const
{
	std::string res = DisplayObject::toDebugString();
	if (graphics && graphics->hasBounds())
		res += " hasgraphics";
	return res;
}

ASFUNCTIONBODY_ATOM(Shape,_constructor)
{
	DisplayObject::_constructor(ret,wrk,obj,nullptr,0);
}

ASFUNCTIONBODY_ATOM(Shape,_getGraphics)
{
	Shape* th=asAtomHandler::as<Shape>(obj);
	if(th->graphics.isNull())
		th->graphics=_MR(Class<Graphics>::getInstanceS(wrk,th));
	th->graphics->incRef();
	ret = asAtomHandler::fromObject(th->graphics.getPtr());
}
