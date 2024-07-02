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

#include "scripting/flash/geom/Point.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"

using namespace lightspark;
using namespace std;

void Point::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(_getX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",c->getSystemState()->getBuiltinFunction(_getY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(_getlength,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(_setX),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("y","",c->getSystemState()->getBuiltinFunction(_setY),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("interpolate","",c->getSystemState()->getBuiltinFunction(interpolate,3,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("distance","",c->getSystemState()->getBuiltinFunction(distance,2,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("add","",c->getSystemState()->getBuiltinFunction(add,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("subtract","",c->getSystemState()->getBuiltinFunction(subtract,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",c->getSystemState()->getBuiltinFunction(equals,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("normalize","",c->getSystemState()->getBuiltinFunction(normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("offset","",c->getSystemState()->getBuiltinFunction(offset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("polar","",c->getSystemState()->getBuiltinFunction(polar,2,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("setTo","",c->getSystemState()->getBuiltinFunction(setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",c->getSystemState()->getBuiltinFunction(copyFrom),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Point,_toString)
{
	Point* th=asAtomHandler::as<Point>(obj);
	char buf[512];
	snprintf(buf,512,"(a=%f, b=%f)",th->x,th->y);
	ret = asAtomHandler::fromObject(abstract_s(wrk,buf));
}

number_t Point::lenImpl(number_t x, number_t y)
{
	return sqrt(x*x + y*y);
}

bool Point::destruct()
{
	x=0;
	y=0;
	return ASObject::destruct();
}

number_t Point::len() const
{
	return lenImpl(x, y);
}

ASFUNCTIONBODY_ATOM(Point,_constructor)
{
	Point* th=asAtomHandler::as<Point>(obj);
	if(argslen>=1)
		th->x=asAtomHandler::toNumber(args[0]);
	if(argslen>=2)
		th->y=asAtomHandler::toNumber(args[1]);
}

ASFUNCTIONBODY_ATOM(Point,_getX)
{
	Point* th=asAtomHandler::as<Point>(obj);
	asAtomHandler::setNumber(ret,wrk,th->x);
}

ASFUNCTIONBODY_ATOM(Point,_getY)
{
	Point* th=asAtomHandler::as<Point>(obj);
	asAtomHandler::setNumber(ret,wrk,th->y);
}

ASFUNCTIONBODY_ATOM(Point,_setX)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	th->x = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Point,_setY)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	th->y = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Point,_getlength)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==0);
	asAtomHandler::setNumber(ret,wrk,th->len());
}

ASFUNCTIONBODY_ATOM(Point,interpolate)
{
	assert_and_throw(argslen==3);
	Point* pt1=asAtomHandler::as<Point>(args[0]);
	Point* pt2=asAtomHandler::as<Point>(args[1]);
	number_t f=asAtomHandler::toNumber(args[2]);
	Point* res=Class<Point>::getInstanceS(wrk);
	res->x = pt2->x - (pt2->x - pt1->x) * f;
	res->y = pt2->y - (pt2->y - pt1->y) * f;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,distance)
{
	assert_and_throw(argslen==2);
	Point* pt1=asAtomHandler::as<Point>(args[0]);
	Point* pt2=asAtomHandler::as<Point>(args[1]);
	number_t res=lenImpl(pt2->x - pt1->x, pt2->y - pt1->y);
	asAtomHandler::setNumber(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(Point,add)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	Point* v=asAtomHandler::as<Point>(args[0]);
	Point* res=Class<Point>::getInstanceS(wrk);
	res->x = th->x + v->x;
	res->y = th->y + v->y;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,subtract)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	Point* v=asAtomHandler::as<Point>(args[0]);
	Point* res=Class<Point>::getInstanceS(wrk);
	res->x = th->x - v->x;
	res->y = th->y - v->y;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,clone)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==0);
	Point* res=Class<Point>::getInstanceS(wrk);
	res->x = th->x;
	res->y = th->y;
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Point,equals)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==1);
	Point* toCompare=asAtomHandler::as<Point>(args[0]);
	asAtomHandler::setBool(ret,(th->x == toCompare->x) & (th->y == toCompare->y));
}

ASFUNCTIONBODY_ATOM(Point,normalize)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen<2);
	number_t thickness = argslen > 0 ? asAtomHandler::toNumber(args[0]) : 1.0;
	number_t len = th->len();
	th->x = len == 0 ? 0 : th->x * thickness / len;
	th->y = len == 0 ? 0 : th->y * thickness / len;
}

ASFUNCTIONBODY_ATOM(Point,offset)
{
	Point* th=asAtomHandler::as<Point>(obj);
	assert_and_throw(argslen==2);
	number_t dx = asAtomHandler::toNumber(args[0]);
	number_t dy = asAtomHandler::toNumber(args[1]);
	th->x += dx;
	th->y += dy;
}

ASFUNCTIONBODY_ATOM(Point,polar)
{
	assert_and_throw(argslen==2);
	number_t len = asAtomHandler::toNumber(args[0]);
	number_t angle = asAtomHandler::toNumber(args[1]);
	Point* res=Class<Point>::getInstanceS(wrk);
	res->x = len * cos(angle);
	res->y = len * sin(angle);
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Point,setTo)
{
	Point* th=asAtomHandler::as<Point>(obj);
	number_t x;
	number_t y;
	ARG_CHECK(ARG_UNPACK(x)(y));
	th->x = x;
	th->y = y;
}
ASFUNCTIONBODY_ATOM(Point,copyFrom)
{
	Point* th=asAtomHandler::as<Point>(obj);
	_NR<Point> sourcepoint;
	ARG_CHECK(ARG_UNPACK(sourcepoint));
	if (!sourcepoint.isNull())
	{
		th->x = sourcepoint->x;
		th->y = sourcepoint->y;
	}
}
