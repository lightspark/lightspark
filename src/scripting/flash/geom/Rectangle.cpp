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

#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"

using namespace lightspark;
using namespace std;

void Rectangle::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	
	c->setDeclaredMethodByQName("left","",c->getSystemState()->getBuiltinFunction(_getLeft,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("left","",c->getSystemState()->getBuiltinFunction(_setLeft),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(_getLeft,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(_setLeft),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("right","",c->getSystemState()->getBuiltinFunction(_getRight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("right","",c->getSystemState()->getBuiltinFunction(_setRight),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_getWidth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(_setWidth),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("top","",c->getSystemState()->getBuiltinFunction(_getTop,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("top","",c->getSystemState()->getBuiltinFunction(_setTop),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",c->getSystemState()->getBuiltinFunction(_getTop,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",c->getSystemState()->getBuiltinFunction(_setTop),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("bottom","",c->getSystemState()->getBuiltinFunction(_getBottom,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bottom","",c->getSystemState()->getBuiltinFunction(_setBottom),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_getHeight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(_setHeight),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("bottomRight","",c->getSystemState()->getBuiltinFunction(_getBottomRight,0,Class<Point>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bottomRight","",c->getSystemState()->getBuiltinFunction(_setBottomRight),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("size","",c->getSystemState()->getBuiltinFunction(_getSize,0,Class<Point>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("size","",c->getSystemState()->getBuiltinFunction(_setSize),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("topLeft","",c->getSystemState()->getBuiltinFunction(_getTopLeft,0,Class<Point>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("topLeft","",c->getSystemState()->getBuiltinFunction(_setTopLeft),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains","",c->getSystemState()->getBuiltinFunction(contains,2,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("containsPoint","",c->getSystemState()->getBuiltinFunction(containsPoint,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("containsRect","",c->getSystemState()->getBuiltinFunction(containsRect,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",c->getSystemState()->getBuiltinFunction(equals,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inflate","",c->getSystemState()->getBuiltinFunction(inflate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inflatePoint","",c->getSystemState()->getBuiltinFunction(inflatePoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("intersection","",c->getSystemState()->getBuiltinFunction(intersection,1,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("intersects","",c->getSystemState()->getBuiltinFunction(intersects,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isEmpty","",c->getSystemState()->getBuiltinFunction(isEmpty,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offset","",c->getSystemState()->getBuiltinFunction(offset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offsetPoint","",c->getSystemState()->getBuiltinFunction(offsetPoint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setEmpty","",c->getSystemState()->getBuiltinFunction(setEmpty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("union","",c->getSystemState()->getBuiltinFunction(_union,1,Class<Rectangle>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTo","",c->getSystemState()->getBuiltinFunction(setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",c->getSystemState()->getBuiltinFunction(copyFrom),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

const RECT Rectangle::getRect() const
{
	return RECT(x,x+width,y,y+height);
}

bool Rectangle::destruct()
{
	x=0;
	y=0;
	width=0;
	height=0;
	return destructIntern();
}
void Rectangle::addUser(DisplayObject* u)
{
	users.insert(u);
}

void Rectangle::removeUser(DisplayObject* u)
{
	users.erase(u);
}
void Rectangle::notifyUsers()
{
	for(auto it=users.begin();it!=users.end();it++)
		(*it)->updatedRect();
}

ASFUNCTIONBODY_ATOM(Rectangle,_constructor)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	if(argslen>=1)
		th->x=asAtomHandler::toNumber(args[0]);
	if(argslen>=2)
		th->y=asAtomHandler::toNumber(args[1]);
	if(argslen>=3)
		th->width=asAtomHandler::toNumber(args[2]);
	if(argslen>=4)
		th->height=asAtomHandler::toNumber(args[3]);
}

ASFUNCTIONBODY_ATOM(Rectangle,_getLeft)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->x);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setLeft)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->x != val)
	{
		th->x=val;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getRight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->x + th->width);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setRight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->width != val-th->x)
	{
		th->width=val-th->x;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getWidth)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->width);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setWidth)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->width != val)
	{
		th->width=val;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getTop)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->y);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setTop)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->y != val)
	{
		th->y=val;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getBottom)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->y + th->height);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setBottom)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->height != val-th->y)
	{
		th->height=val-th->y;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getBottomRight)
{
	assert_and_throw(argslen==0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* res = Class<Point>::getInstanceS(wrk,th->x + th->width, th->y + th->height);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setBottomRight)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	if (th->width != br->getX() - th->x || th->height != br->getY() - th->y)
	{
		th->width = br->getX() - th->x;
		th->height = br->getY() - th->y;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getTopLeft)
{
	assert_and_throw(argslen==0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* res = Class<Point>::getInstanceS(wrk,th->x, th->y);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setTopLeft)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	if (th->x != br->getX()|| th->y != br->getY())
	{
		th->x = br->getX();
		th->y = br->getY();
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getSize)
{
	assert_and_throw(argslen==0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* res = Class<Point>::getInstanceS(wrk,th->width, th->height);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setSize)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	if (th->width != br->getX()|| th->height != br->getY())
	{
		th->width = br->getX();
		th->height = br->getY();
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,_getHeight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	asAtomHandler::setNumber(ret,wrk,th->height);
}

ASFUNCTIONBODY_ATOM(Rectangle,_setHeight)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->height != val)
	{
		th->height=val;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(Rectangle,clone)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* res=Class<Rectangle>::getInstanceS(wrk);
	res->x=th->x;
	res->y=th->y;
	res->width=th->width;
	res->height=th->height;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,contains)
{
	assert_and_throw(argslen == 2);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t x = asAtomHandler::toNumber(args[0]);
	number_t y = asAtomHandler::toNumber(args[1]);

	asAtomHandler::setBool(ret,th->x <= x && x <= th->x + th->width
						&& th->y <= y && y <= th->y + th->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,containsPoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* br = asAtomHandler::as<Point>(args[0]);
	number_t x = br->getX();
	number_t y = br->getY();

	asAtomHandler::setBool(ret,th->x <= x && x <= th->x + th->width
						&& th->y <= y && y <= th->y + th->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,containsRect)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* cr = asAtomHandler::as<Rectangle>(args[0]);

	asAtomHandler::setBool(ret,th->x <= cr->x && cr->x + cr->width <= th->x + th->width
						&& th->y <= cr->y && cr->y + cr->height <= th->y + th->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,equals)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* co = asAtomHandler::as<Rectangle>(args[0]);

	asAtomHandler::setBool(ret,th->x == co->x && th->width == co->width
						&& th->y == co->y && th->height == co->height );
}

ASFUNCTIONBODY_ATOM(Rectangle,inflate)
{
	assert_and_throw(argslen == 2);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t dx = asAtomHandler::toNumber(args[0]);
	number_t dy = asAtomHandler::toNumber(args[1]);

	th->x -= dx;
	th->width += 2 * dx;
	th->y -= dy;
	th->height += 2 * dy;
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(Rectangle,inflatePoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* po = asAtomHandler::as<Point>(args[0]);
	number_t dx = po->getX();
	number_t dy = po->getY();

	th->x -= dx;
	th->width += 2 * dx;
	th->y -= dy;
	th->height += 2 * dy;
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(Rectangle,intersection)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* ti = asAtomHandler::as<Rectangle>(args[0]);
	Rectangle* res = Class<Rectangle>::getInstanceS(wrk);

	number_t thtop = th->y;
	number_t thleft = th->x;
	number_t thright = th->x + th->width;
	number_t thbottom = th->y + th->height;

	number_t titop = ti->y;
	number_t tileft = ti->x;
	number_t tiright = ti->x + ti->width;
	number_t tibottom = ti->y + ti->height;

	if ( thtop > tibottom || thright < tileft ||
						thbottom < titop || thleft > tiright )
	{
		// rectangles don't intersect
		res->x = 0;
		res->y = 0;
		res->width = 0;
		res->height = 0;
		ret = asAtomHandler::fromObject(res);
		return;
	}

	Rectangle* leftmost = ti;
	Rectangle* rightmost = th;

	// find left most
	if ( thleft < tileft )
	{
		leftmost = th;
		rightmost = ti;
	}

	Rectangle* topmost = ti;
	Rectangle* bottommost = th;

	// find top most
	if ( thtop < titop )
	{
		topmost = th;
		bottommost = ti;
	}

	res->x = rightmost->x;
	res->width = min(leftmost->x + leftmost->width, rightmost->x + rightmost->width) - rightmost->x;
	res->y = bottommost->y;
	res->height = min(topmost->y + topmost->height, bottommost->y + bottommost->height) - bottommost->y;

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,intersects)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* ti = asAtomHandler::as<Rectangle>(args[0]);

	number_t thtop = th->y;
	number_t thleft = th->x;
	number_t thright = th->x + th->width;
	number_t thbottom = th->y + th->height;

	number_t titop = ti->y;
	number_t tileft = ti->x;
	number_t tiright = ti->x + ti->width;
	number_t tibottom = ti->y + ti->height;

	asAtomHandler::setBool(ret,!(thtop > tibottom || thright < tileft ||
						thbottom < titop || thleft > tiright) );
}

ASFUNCTIONBODY_ATOM(Rectangle,isEmpty)
{
	assert_and_throw(argslen == 0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	asAtomHandler::setBool(ret,th->width <= 0 || th->height <= 0 );
}

ASFUNCTIONBODY_ATOM(Rectangle,offset)
{
	assert_and_throw(argslen == 2);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	th->x += asAtomHandler::toNumber(args[0]);
	th->y += asAtomHandler::toNumber(args[1]);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(Rectangle,offsetPoint)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Point* po = asAtomHandler::as<Point>(args[0]);

	th->x += po->getX();
	th->y += po->getY();
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(Rectangle,setEmpty)
{
	assert_and_throw(argslen == 0);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);

	th->x = 0;
	th->y = 0;
	th->width = 0;
	th->height = 0;
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(Rectangle,_union)
{
	assert_and_throw(argslen == 1);
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	Rectangle* ti = asAtomHandler::as<Rectangle>(args[0]);
	Rectangle* res = Class<Rectangle>::getInstanceS(wrk);

	res->x = th->x;
	res->y = th->y;
	res->width = th->width;
	res->height = th->height;

	if ( ti->width == 0 || ti->height == 0 )
	{
		ret = asAtomHandler::fromObject(res);
		return;
	}

	res->x = min(th->x, ti->x);
	res->y = min(th->y, ti->y);
	res->width = max(th->x + th->width, ti->x + ti->width);
	res->height = max(th->y + th->height, ti->y + ti->height);

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Rectangle,_toString)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	tiny_string s = "(x=";
	s+= Number::toString(th->x);
	s+= ", y=";
	s+= Number::toString(th->y);
	s+= ", w=";
	s+= Number::toString(th->width);
	s+= ", h=";
	s+= Number::toString(th->height);
	s+= ")";
	ret = asAtomHandler::fromObject(abstract_s(wrk,s));
}

ASFUNCTIONBODY_ATOM(Rectangle,setTo)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	number_t x,y,wi,h;
	ARG_CHECK(ARG_UNPACK(x)(y)(wi)(h));
	th->x = x;
	th->y = y;
	th->width = wi;
	th->height = h;
	th->notifyUsers();
}
ASFUNCTIONBODY_ATOM(Rectangle,copyFrom)
{
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	_NR<Rectangle> sourcerect;
	ARG_CHECK(ARG_UNPACK(sourcerect));
	if (!sourcerect.isNull())
	{
		th->x = sourcerect->x;
		th->y = sourcerect->y;
		th->width = sourcerect->width;
		th->height = sourcerect->height;
		th->notifyUsers();
	}
}

