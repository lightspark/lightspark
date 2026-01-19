/**************************************************************************
    Lightspark, a free flash player implementation

	Copyright (C) 2025  Ludger Krämer <dbluelle@onlinehome.de>

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

#include "avm1rectangle.h"
#include "avm1point.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;


AVM1Rectangle::AVM1Rectangle(ASWorker* wrk, Class_base* c):Rectangle(wrk,c)
	,atomX(asAtomHandler::invalidAtom)
	,atomY(asAtomHandler::invalidAtom)
	,atomWidth(asAtomHandler::invalidAtom)
	,atomHeight(asAtomHandler::invalidAtom)
{
	subtype=SUBTYPE_AVM1RECTANGLE;
}

void AVM1Rectangle::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);

	c->prototype->setDeclaredMethodByQName("left","",c->getSystemState()->getBuiltinFunction(AVM1_getLeft,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("left","",c->getSystemState()->getBuiltinFunction(AVM1_setLeft),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(AVM1_getLeft,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(AVM1_setX),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("right","",c->getSystemState()->getBuiltinFunction(AVM1_getRight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("right","",c->getSystemState()->getBuiltinFunction(AVM1_setRight),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(AVM1_getWidth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("width","",c->getSystemState()->getBuiltinFunction(AVM1_setWidth),SETTER_METHOD,false,false);

	c->prototype->setDeclaredMethodByQName("top","",c->getSystemState()->getBuiltinFunction(AVM1_getTop,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("top","",c->getSystemState()->getBuiltinFunction(AVM1_setTop),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("y","",c->getSystemState()->getBuiltinFunction(AVM1_getTop,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("y","",c->getSystemState()->getBuiltinFunction(AVM1_setY),SETTER_METHOD,false,false);

	c->prototype->setDeclaredMethodByQName("bottom","",c->getSystemState()->getBuiltinFunction(AVM1_getBottom,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("bottom","",c->getSystemState()->getBuiltinFunction(AVM1_setBottom),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(AVM1_getHeight,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("height","",c->getSystemState()->getBuiltinFunction(AVM1_setHeight),SETTER_METHOD,false,false);

	c->prototype->setDeclaredMethodByQName("bottomRight","",c->getSystemState()->getBuiltinFunction(AVM1_getBottomRight,0,Class<Point>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("bottomRight","",c->getSystemState()->getBuiltinFunction(AVM1_setBottomRight),SETTER_METHOD,false,false);

	c->prototype->setDeclaredMethodByQName("size","",c->getSystemState()->getBuiltinFunction(AVM1_getSize,0,Class<Point>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("size","",c->getSystemState()->getBuiltinFunction(AVM1_setSize),SETTER_METHOD,false,false);

	c->prototype->setDeclaredMethodByQName("topLeft","",c->getSystemState()->getBuiltinFunction(AVM1_getTopLeft,0,Class<Point>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("topLeft","",c->getSystemState()->getBuiltinFunction(AVM1_setTopLeft),SETTER_METHOD,false,false);

	c->prototype->setVariableByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<AVM1Rectangle>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("contains","",c->getSystemState()->getBuiltinFunction(AVM1_contains,2,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("containsPoint","",c->getSystemState()->getBuiltinFunction(AVM1_containsPoint,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("containsRectangle","",c->getSystemState()->getBuiltinFunction(AVM1_containsRectangle,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("equals","",c->getSystemState()->getBuiltinFunction(equals,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("inflate","",c->getSystemState()->getBuiltinFunction(AVM1_inflate),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("inflatePoint","",c->getSystemState()->getBuiltinFunction(AVM1_inflatePoint),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("intersection","",c->getSystemState()->getBuiltinFunction(AVM1_intersection,1,Class<AVM1Rectangle>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("intersects","",c->getSystemState()->getBuiltinFunction(AVM1_intersects,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("isEmpty","",c->getSystemState()->getBuiltinFunction(AVM1_isEmpty,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("offset","",c->getSystemState()->getBuiltinFunction(AVM1_offset),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("offsetPoint","",c->getSystemState()->getBuiltinFunction(AVM1_offsetPoint),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("setEmpty","",c->getSystemState()->getBuiltinFunction(AVM1_setEmpty),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("union","",c->getSystemState()->getBuiltinFunction(AVM1_union,1,Class<AVM1Rectangle>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(AVM1_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);

}

void AVM1Rectangle::finalize()
{
	ASATOM_REMOVESTOREDMEMBER(atomX);
	atomX=asAtomHandler::invalidAtom;
	ASATOM_REMOVESTOREDMEMBER(atomY);
	atomY=asAtomHandler::invalidAtom;
	ASATOM_REMOVESTOREDMEMBER(atomWidth);
	atomWidth=asAtomHandler::invalidAtom;
	ASATOM_REMOVESTOREDMEMBER(atomHeight);
	atomHeight=asAtomHandler::invalidAtom;
	Rectangle::finalize();
}
bool AVM1Rectangle::destruct()
{
	ASATOM_REMOVESTOREDMEMBER(atomX);
	atomX=asAtomHandler::invalidAtom;
	ASATOM_REMOVESTOREDMEMBER(atomY);
	atomY=asAtomHandler::invalidAtom;
	ASATOM_REMOVESTOREDMEMBER(atomWidth);
	atomWidth=asAtomHandler::invalidAtom;
	ASATOM_REMOVESTOREDMEMBER(atomHeight);
	atomHeight=asAtomHandler::invalidAtom;
	return Rectangle::destruct();
}

void AVM1Rectangle::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASATOM_PREPARESHUTDOWN(atomX);
	ASATOM_REMOVESTOREDMEMBER(atomX);
	atomX=asAtomHandler::invalidAtom;
	ASATOM_PREPARESHUTDOWN(atomY);
	ASATOM_REMOVESTOREDMEMBER(atomY);
	atomY=asAtomHandler::invalidAtom;
	ASATOM_PREPARESHUTDOWN(atomWidth);
	ASATOM_REMOVESTOREDMEMBER(atomWidth);
	atomWidth=asAtomHandler::invalidAtom;
	ASATOM_PREPARESHUTDOWN(atomHeight);
	ASATOM_REMOVESTOREDMEMBER(atomHeight);
	atomHeight=asAtomHandler::invalidAtom;
	Rectangle::prepareShutdown();
}

bool AVM1Rectangle::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = Rectangle::countCylicMemberReferences(gcstate);
	ASObject* o;
	o = asAtomHandler::getObject(atomX);
	if (o)
		ret = o->countAllCylicMemberReferences(gcstate) || ret;
	o = asAtomHandler::getObject(atomY);
	if (o)
		ret = o->countAllCylicMemberReferences(gcstate) || ret;
	o = asAtomHandler::getObject(atomWidth);
	if (o)
		ret = o->countAllCylicMemberReferences(gcstate) || ret;
	o = asAtomHandler::getObject(atomHeight);
	if (o)
		ret = o->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void AVM1Rectangle::setX(number_t val, ASWorker* wrk)
{
	x = val;
	asAtom oldX=atomX;
	asAtomHandler::setNumber(atomX,x);
	ASATOM_REMOVESTOREDMEMBER(oldX);
}
void AVM1Rectangle::setY(number_t val, ASWorker* wrk)
{
	y = val;
	asAtom oldY=atomY;
	asAtomHandler::setNumber(atomY,y);
	ASATOM_REMOVESTOREDMEMBER(oldY);
}

void AVM1Rectangle::setWidth(number_t val, ASWorker* wrk)
{
	width = val;
	asAtom oldWidth=atomWidth;
	asAtomHandler::setNumber(atomWidth,width);
	ASATOM_REMOVESTOREDMEMBER(oldWidth);
}
void AVM1Rectangle::setHeight(number_t val, ASWorker* wrk)
{
	height = val;
	asAtom oldHeight=atomHeight;
	asAtomHandler::setNumber(atomHeight,height);
	ASATOM_REMOVESTOREDMEMBER(oldHeight);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,_constructor)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	if (argslen==0)
	{
		th->atomX = asAtomHandler::fromInt(0);
		th->atomY = asAtomHandler::fromInt(0);
		th->atomWidth = asAtomHandler::fromInt(0);
		th->atomHeight = asAtomHandler::fromInt(0);
		th->x=0;
		th->y=0;
		th->width=0;
		th->height=0;
		return;
	}

	if(argslen>=1)
	{
		th->atomX = args[0];
		ASATOM_ADDSTOREDMEMBER(th->atomX);
		th->x=asAtomHandler::toNumber(args[0]);
	}
	else
	{
		th->x=Number::NaN;
		th->atomX=asAtomHandler::undefinedAtom;
	}

	if(argslen>=2)
	{
		th->atomY = args[1];
		ASATOM_ADDSTOREDMEMBER(th->atomY);
		th->y=asAtomHandler::toNumber(args[1]);
	}
	else
	{
		th->y=Number::NaN;
		th->atomY=asAtomHandler::undefinedAtom;
	}

	if(argslen>=3)
	{
		th->atomWidth = args[2];
		ASATOM_ADDSTOREDMEMBER(th->atomWidth);
		th->width=asAtomHandler::toNumber(args[2]);
	}
	else
	{
		th->width=Number::NaN;
		th->atomWidth=asAtomHandler::undefinedAtom;
	}

	if(argslen>=4)
	{
		th->atomHeight= args[3];
		ASATOM_ADDSTOREDMEMBER(th->atomHeight);
		th->height=asAtomHandler::toNumber(args[3]);
	}
	else
	{
		th->height=Number::NaN;
		th->atomHeight=asAtomHandler::undefinedAtom;
	}
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_getLeft)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	ret = th->atomX;
	ASATOM_INCREF(ret);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setLeft)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	ASATOM_ADDSTOREDMEMBER(args[0]);
	ASATOM_REMOVESTOREDMEMBER(th->atomX);
	th->atomX=args[0];
	if (th->x != val)
	{
		th->setWidth(th->width+th->x-val,wrk);
		th->x=val;
		th->notifyUsers();
	}
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setX)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	ASATOM_ADDSTOREDMEMBER(args[0]);
	ASATOM_REMOVESTOREDMEMBER(th->atomX);
	th->atomX=args[0];
	if (th->x != val)
	{
		th->x=val;
		th->notifyUsers();
	}
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_getTop)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	ret = th->atomY;
	ASATOM_INCREF(ret);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setTop)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	ASATOM_ADDSTOREDMEMBER(args[0]);
	ASATOM_REMOVESTOREDMEMBER(th->atomY);
	th->atomY=args[0];
	if (th->y != val)
	{
		th->setHeight(th->height+th->y-val,wrk);
		th->y=val;
		th->notifyUsers();
	}
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setY)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	ASATOM_ADDSTOREDMEMBER(args[0]);
	ASATOM_REMOVESTOREDMEMBER(th->atomY);
	th->atomY=args[0];
	if (th->y != val)
	{
		th->y=val;
		th->notifyUsers();
	}
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_getHeight)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	ret = th->atomHeight;
	ASATOM_INCREF(ret);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setHeight)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	ASATOM_ADDSTOREDMEMBER(args[0]);
	ASATOM_REMOVESTOREDMEMBER(th->atomHeight);
	th->atomHeight=args[0];
	if (th->height != val)
	{
		th->height=val;
		th->notifyUsers();
	}
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_getWidth)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	ret = th->atomWidth;
	ASATOM_INCREF(ret);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setWidth)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	ASATOM_ADDSTOREDMEMBER(args[0]);
	ASATOM_REMOVESTOREDMEMBER(th->atomWidth);
	th->atomWidth=args[0];
	if (th->width != val)
	{
		th->width=val;
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_getTopLeft)
{
	assert_and_throw(argslen==0);
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	AVM1Point* res = Class<AVM1Point>::getInstanceSNoArgs(wrk);
	res->setX(th->atomX,wrk,true);
	res->setY(th->atomY,wrk,true);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setTopLeft)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	asAtom arg = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(arg));
	ASATOM_REMOVESTOREDMEMBER(th->atomX);
	ASATOM_REMOVESTOREDMEMBER(th->atomY);
	if (asAtomHandler::is<Point>(arg))
	{
		Point* point = asAtomHandler::as<Point>(arg);
		if (th->x != point->getX() || th->y != point->getY())
		{
			th->setWidth(th->width+th->x-point->getX(),wrk);
			th->setHeight(th->height+th->y-point->getY(),wrk);
			th->atomX=asAtomHandler::fromNumber(point->getX());
			th->x = point->getX();
			th->atomY=asAtomHandler::fromNumber(point->getY());
			th->y = point->getY();
			th->notifyUsers();
		}
	}
	else if (asAtomHandler::isObject(arg))
	{
		ASObject* o = asAtomHandler::getObjectNoCheck(arg);
		number_t newx=th->x;
		number_t newy=th->y;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
		if (o->hasPropertyByMultiname(m,true,false,wrk))
		{
			o->getVariableByMultiname(th->atomX,m,GET_VARIABLE_OPTION::NONE,wrk);
			ASObject* a =asAtomHandler::getObject(th->atomX);
			if (a)
				a->addStoredMember();
			newx = asAtomHandler::AVM1toNumber(th->atomX,wrk->AVM1getSwfVersion());
		}
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
		if (o->hasPropertyByMultiname(m,true,false,wrk))
		{
			o->getVariableByMultiname(th->atomY,m,GET_VARIABLE_OPTION::NONE,wrk);
			ASObject* a =asAtomHandler::getObject(th->atomY);
			if (a)
				a->addStoredMember();
			newy = asAtomHandler::AVM1toNumber(th->atomY,wrk->AVM1getSwfVersion());
		}
		if (newx != th->x || newy != th->y)
		{
			th->setWidth(th->width+th->x-newx,wrk);
			th->setHeight(th->height+th->y-newy,wrk);
			th->x = newx;
			th->y = newy;
			th->notifyUsers();
		}
	}
	else
	{
		th->atomX=asAtomHandler::undefinedAtom;
		th->atomY=asAtomHandler::undefinedAtom;
		th->setWidth(Number::NaN,wrk);
		th->setHeight(Number::NaN,wrk);
		th->notifyUsers();
	}
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_getBottomRight)
{
	assert_and_throw(argslen==0);
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	AVM1Point* res = Class<AVM1Point>::getInstanceSNoArgs(wrk);
	asAtom right= asAtomHandler::fromNumber(th->x + th->width);
	res->setX(right,wrk,false);
	asAtom bottom= asAtomHandler::fromNumber(th->y + th->height);
	res->setY(bottom,wrk,false);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setBottomRight)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	Vector2f point;
	ARG_CHECK(ARG_UNPACK(point));
	if (th->width != point.x - th->x || th->y != point.y - th->y)
	{
		th->setWidth(point.x-th->x,wrk);
		th->setHeight(point.y - th->y,wrk);
		th->notifyUsers();
	}
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_getSize)
{
	assert_and_throw(argslen==0);
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	AVM1Point* res = Class<AVM1Point>::getInstanceS(wrk);
	res->setX(th->atomWidth,wrk,true);
	res->setY(th->atomHeight,wrk,true);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setSize)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	asAtom arg=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(arg));
	if (asAtomHandler::is<Point>(arg))
	{
		Point* point = asAtomHandler::as<Point>(arg);
		if (th->width != point->getX() || th->height != point->getY())
		{
			th->setWidth(point->getX(),wrk);
			th->setHeight(point->getY(),wrk);
			th->notifyUsers();
		}
	}
	else if (asAtomHandler::isObject(arg))
	{
		ASObject* o = asAtomHandler::getObjectNoCheck(arg);
		number_t newx=th->x;
		number_t newy=th->y;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
		if (o->hasPropertyByMultiname(m,true,false,wrk))
		{
			o->getVariableByMultiname(th->atomWidth,m,GET_VARIABLE_OPTION::NONE,wrk);
			ASObject* a =asAtomHandler::getObject(th->atomWidth);
			if (a)
				a->addStoredMember();
			newx = asAtomHandler::AVM1toNumber(th->atomWidth,wrk->AVM1getSwfVersion());
		}
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
		if (o->hasPropertyByMultiname(m,true,false,wrk))
		{
			o->getVariableByMultiname(th->atomHeight,m,GET_VARIABLE_OPTION::NONE,wrk);
			ASObject* a =asAtomHandler::getObject(th->atomHeight);
			if (a)
				a->addStoredMember();
			newy = asAtomHandler::AVM1toNumber(th->atomHeight,wrk->AVM1getSwfVersion());
		}
		if (newx != th->width || newy != th->height)
		{
			th->setWidth(newx,wrk);
			th->setHeight(newy,wrk);
			th->notifyUsers();
		}
	}
	else
	{
		th->atomWidth=asAtomHandler::undefinedAtom;
		th->atomHeight=asAtomHandler::undefinedAtom;
		th->width=Number::NaN;
		th->height=Number::NaN;
		th->notifyUsers();
	}
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_getBottom)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	asAtomHandler::setNumber(ret, th->y + th->height);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setBottom)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->height != val-th->y)
	{
		th->setHeight(val-th->y,wrk);
		th->notifyUsers();
	}
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_getRight)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	asAtomHandler::setNumber(ret, th->x + th->width);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setRight)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	if (th->width != val-th->x)
	{
		th->setWidth(val-th->x,wrk);
		th->notifyUsers();
	}
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_setEmpty)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	th->atomX = asAtomHandler::fromInt(0);
	th->x=0;
	th->atomY = asAtomHandler::fromInt(0);
	th->y=0;
	th->atomWidth = asAtomHandler::fromInt(0);
	th->width=0;
	th->atomHeight = asAtomHandler::fromInt(0);
	th->height=0;
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_contains)
{
	if (argslen < 2)
	{
		ret = asAtomHandler::undefinedAtom;
		return;
	}
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t x = asAtomHandler::toNumber(args[0]);
	number_t y = asAtomHandler::toNumber(args[1]);
	if (std::isnan(x) || std::isnan(y))
	{
		ret = asAtomHandler::undefinedAtom;
		return;
	}


	asAtomHandler::setBool(ret,th->x <= x && x < th->x + th->width
									&& th->y <= y && y < th->y + th->height );
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_containsPoint)
{
	if (argslen < 1)
	{
		ret = asAtomHandler::undefinedAtom;
		return;
	}
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t x = Number::NaN;
	number_t y = Number::NaN;
	if (asAtomHandler::is<AVM1Point>(args[0]))
	{
		AVM1Point* br = asAtomHandler::as<AVM1Point>(args[0]);
		x = asAtomHandler::AVM1toNumber(br->getAtomX(),wrk->AVM1getSwfVersion());
		y = asAtomHandler::AVM1toNumber(br->getAtomY(),wrk->AVM1getSwfVersion());
	}
	else if (asAtomHandler::isObject(args[0]))
	{
		ASObject* o = asAtomHandler::getObjectNoCheck(args[0]);
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
		if (o->hasPropertyByMultiname(m,true,false,wrk))
		{
			asAtom a=asAtomHandler::invalidAtom;
			o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
			x = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
			ASATOM_DECREF(a);
		}
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
		if (o->hasPropertyByMultiname(m,true,false,wrk))
		{
			asAtom a=asAtomHandler::invalidAtom;
			o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
			y = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
			ASATOM_DECREF(a);
		}
	}

	if (std::isnan(x) || std::isnan(y))
	{
		ret = asAtomHandler::undefinedAtom;
		return;
	}


	asAtomHandler::setBool(ret,th->x <= x && x < th->x + th->width
									&& th->y <= y && y < th->y + th->height );
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_containsRectangle)
{
	if (argslen < 1 || !asAtomHandler::isObject(args[0]))
	{
		ret = asAtomHandler::undefinedAtom;
		return;
	}
	number_t cx=Number::NaN;
	number_t cy=Number::NaN;
	number_t cw=Number::NaN;
	number_t ch=Number::NaN;
	if (asAtomHandler::is<Rectangle>(args[0]))
	{
		Rectangle* cr = asAtomHandler::as<Rectangle>(args[0]);
		cx = cr->x;
		cy = cr->y;
		cw = cr->width;
		ch = cr->height;
	}
	else
	{
		ASObject* o = asAtomHandler::getObjectNoCheck(args[0]);
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
		if (o->hasPropertyByMultiname(m,true,false,wrk))
		{
			asAtom a=asAtomHandler::invalidAtom;
			o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
			cx = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
			ASATOM_DECREF(a);
		}
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
		if (o->hasPropertyByMultiname(m,true,false,wrk))
		{
			asAtom a=asAtomHandler::invalidAtom;
			o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
			cy = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
			ASATOM_DECREF(a);
		}
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("width");
		if (o->hasPropertyByMultiname(m,true,false,wrk))
		{
			asAtom a=asAtomHandler::invalidAtom;
			o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
			cw = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
			ASATOM_DECREF(a);
		}
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("height");
		if (o->hasPropertyByMultiname(m,true,false,wrk))
		{
			asAtom a=asAtomHandler::invalidAtom;
			o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
			ch = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
			ASATOM_DECREF(a);
		}

	}
	Rectangle* th=asAtomHandler::as<Rectangle>(obj);
	if (std::isnan(th->x) || std::isnan(th->y) || std::isnan(th->width) || std::isnan(th->height) ||
		std::isnan(cx) || std::isnan(cy) || std::isnan(cw) || std::isnan(ch))
	{
		ret = asAtomHandler::undefinedAtom;
		return;
	}

	asAtomHandler::setBool(ret,th->x <= cx && cx + cw <= th->x + th->width
									&& th->y <= cy && cy + ch <= th->y + th->height );
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_inflate)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t dx = Number::NaN;
	number_t dy = Number::NaN;
	if (argslen >= 1)
		dx = asAtomHandler::AVM1toNumber(args[0],wrk->AVM1getSwfVersion());
	if (argslen >= 2)
		dy = asAtomHandler::AVM1toNumber(args[1],wrk->AVM1getSwfVersion());

	th->setX(th->x - dx,wrk);
	th->setY(th->y - dy,wrk);
	th->setWidth(th->width + 2 * dx,wrk);
	th->setHeight(th->height + 2 * dy,wrk);
	th->notifyUsers();
}
ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_inflatePoint)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t dx = Number::NaN;
	number_t dy = Number::NaN;
	if (argslen >= 1)
	{
		if (asAtomHandler::is<Point>(args[0]))
		{
			Point* po = asAtomHandler::as<Point>(args[0]);
			dx = po->getX();
			dy = po->getY();
		}
		else if (asAtomHandler::isObject(args[0]))
		{
			ASObject* o = asAtomHandler::getObjectNoCheck(args[0]);
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				dx = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				dy = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
			}
		}
	}

	th->setX(th->x - dx,wrk);
	th->setY(th->y - dy,wrk);
	th->setWidth(th->width + 2 * dx,wrk);
	th->setHeight(th->height + 2 * dy,wrk);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_intersection)
{
	number_t cx=Number::NaN;
	number_t cy=Number::NaN;
	number_t cw=Number::NaN;
	number_t ch=Number::NaN;
	bool ok=false;
	if (argslen > 0 && asAtomHandler::isObject(args[0]))
	{
		if (asAtomHandler::is<Rectangle>(args[0]))
		{
			Rectangle* cr = asAtomHandler::as<Rectangle>(args[0]);
			cx = cr->x;
			cy = cr->y;
			cw = cr->width;
			ch = cr->height;
			ok=true;
		}
		else
		{
			int varcount=0;
			ASObject* o = asAtomHandler::getObjectNoCheck(args[0]);
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				cx = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
				varcount++;
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				cy = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
				varcount++;
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("width");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				cw = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
				varcount++;
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("height");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				ch = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
				varcount++;
			}
			ok = varcount==4;
		}
	}
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	AVM1Rectangle* res = Class<AVM1Rectangle>::getInstanceS(wrk);

	number_t thtop = th->y;
	number_t thleft = th->x;
	number_t thright = th->x + th->width;
	number_t thbottom = th->y + th->height;

	number_t titop = cy;
	number_t tileft = cx;
	number_t tiright = cx + cw;
	number_t tibottom = cy + ch;

	if (argslen == 0 || !ok ||
		thtop > tibottom || thright < tileft ||
		thbottom < titop || thleft > tiright )
	{
		// rectangles don't intersect
		res->setX(0,wrk);
		res->setY(0,wrk);
		res->setWidth(0,wrk);
		res->setHeight(0,wrk);
		ret = asAtomHandler::fromObject(res);
		return;
	}

	number_t leftmost_x=cx;
	number_t leftmost_y=cy;
	number_t leftmost_w=cw;
	number_t leftmost_h=ch;
	number_t rightmost_x=th->x;
	number_t rightmost_y=th->y;
	number_t rightmost_w=th->width;
	number_t rightmost_h=th->height;

	// find left most
	if ( thleft < tileft )
	{
		std::swap(leftmost_x,rightmost_x);
		std::swap(leftmost_y,rightmost_y);
		std::swap(leftmost_w,rightmost_w);
		std::swap(leftmost_h,rightmost_h);
	}

	number_t topmost_x=cx;
	number_t topmost_y=cy;
	number_t topmost_w=cw;
	number_t topmost_h=ch;
	number_t bottommost_x=th->x;
	number_t bottommost_y=th->y;
	number_t bottommost_w=th->width;
	number_t bottommost_h=th->height;

	// find top most
	if ( thtop < titop )
	{
		std::swap(topmost_x,bottommost_x);
		std::swap(topmost_y,bottommost_y);
		std::swap(topmost_w,bottommost_w);
		std::swap(topmost_h,bottommost_h);
	}

	res->setX(rightmost_x,wrk);
	res->setY(bottommost_y,wrk);
	res->setWidth(min(leftmost_x + leftmost_w, rightmost_x + rightmost_w) - rightmost_x,wrk);
	res->setHeight(min(topmost_y + topmost_h, bottommost_y + bottommost_h) - bottommost_y,wrk);

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_intersects)
{
	if (argslen < 1)
	{
		ret = asAtomHandler::falseAtom;
		return;
	}
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	number_t cx=Number::NaN;
	number_t cy=Number::NaN;
	number_t cw=Number::NaN;
	number_t ch=Number::NaN;
	if (argslen > 0 && asAtomHandler::isObject(args[0]))
	{
		if (asAtomHandler::is<Rectangle>(args[0]))
		{
			Rectangle* cr = asAtomHandler::as<Rectangle>(args[0]);
			cx = cr->x;
			cy = cr->y;
			cw = cr->width;
			ch = cr->height;
		}
		else
		{
			int varcount=0;
			ASObject* o = asAtomHandler::getObjectNoCheck(args[0]);
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				cx = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
				varcount++;
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				cy = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
				varcount++;
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("width");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				cw = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
				varcount++;
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("height");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				ch = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
				varcount++;
			}
		}
	}
	if (std::isnan(cw) || std::isnan(ch) || cw==0 || ch==0  ||
		std::isnan(th->width) || std::isnan(th->height) || th->width==0 || th->height==0 )
	{
		ret = asAtomHandler::falseAtom;
		return;
	}

	number_t thtop = th->y;
	number_t thleft = th->x;
	number_t thright = th->x + th->width;
	number_t thbottom = th->y + th->height;

	number_t titop = cy;
	number_t tileft = cx;
	number_t tiright = cx + cw;
	number_t tibottom = cy + ch;

	asAtomHandler::setBool(ret,!(thtop > tibottom || thright < tileft ||
								  thbottom < titop || thleft > tiright) );
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_offset)
{
	number_t dx=Number::NaN;
	number_t dy=Number::NaN;
	if (argslen >= 1)
		dx = asAtomHandler::AVM1toNumber(args[0],wrk->AVM1getSwfVersion());
	if (argslen >= 2)
		dy = asAtomHandler::AVM1toNumber(args[1],wrk->AVM1getSwfVersion());
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);

	th->setX(th->x + dx,wrk);
	th->setY(th->y + dy,wrk);

	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_offsetPoint)
{
	number_t dx=Number::NaN;
	number_t dy=Number::NaN;

	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);

	if (argslen > 0 && asAtomHandler::isObject(args[0]))
	{
		if (asAtomHandler::is<Point>(args[0]))
		{
			Point* cp = asAtomHandler::as<Point>(args[0]);
			dx = cp->getX();
			dy = cp->getY();
		}
		else
		{
			ASObject* o = asAtomHandler::getObjectNoCheck(args[0]);
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				dx = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				dy = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
			}
		}
	}

	th->setX(th->x + dx,wrk);
	th->setY(th->y + dy,wrk);
	th->notifyUsers();
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_union)
{
	number_t cx=Number::NaN;
	number_t cy=Number::NaN;
	number_t cw=Number::NaN;
	number_t ch=Number::NaN;
	bool ok=false;
	if (argslen > 0 && asAtomHandler::isObject(args[0]))
	{
		if (asAtomHandler::is<Rectangle>(args[0]))
		{
			Rectangle* cr = asAtomHandler::as<Rectangle>(args[0]);
			cx = cr->x;
			cy = cr->y;
			cw = cr->width;
			ch = cr->height;
			ok=true;
		}
		else
		{
			ASObject* o = asAtomHandler::getObjectNoCheck(args[0]);
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("x");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				cx = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("y");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				cy = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("width");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				cw = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
			}
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("height");
			if (o->hasPropertyByMultiname(m,true,false,wrk))
			{
				asAtom a=asAtomHandler::invalidAtom;
				o->getVariableByMultiname(a,m,GET_VARIABLE_OPTION::NONE,wrk);
				ch = asAtomHandler::AVM1toNumber(a,wrk->AVM1getSwfVersion());
				ASATOM_DECREF(a);
			}
			ok=true;
		}
	}
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	AVM1Rectangle* res = Class<AVM1Rectangle>::getInstanceS(wrk);


	if ( th->width <= 0 || th->height <= 0  || !ok)
	{
		res->setX(Number::NaN,wrk);
		res->setY(Number::NaN,wrk);
		res->setWidth(Number::NaN,wrk);
		res->setHeight(Number::NaN,wrk);
		ret = asAtomHandler::fromObject(res);
		return;
	}

	number_t rx=min(th->x, cx);
	number_t ry=min(th->y, cy);
	number_t rw=th->width;
	number_t rh=th->height;
	if (std::isnan(th->width) || std::isnan(cw))
		rw=Number::NaN;
	else
		rw = max(th->x + th->width, cx + cw)-rx;
	if (std::isnan(th->height) || std::isnan(ch))
		rh=Number::NaN;
	else
		rh = max(th->y + th->height, cy + ch)-ry;


	res->setX(rx,wrk);
	res->setY(ry,wrk);
	res->setWidth(rw,wrk);
	res->setHeight(rh,wrk);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_isEmpty)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	asAtomHandler::setBool(ret,th->width <= 0 || th->height <= 0 || std::isnan(th->width) || std::isnan(th->height));
}

ASFUNCTIONBODY_ATOM(AVM1Rectangle,AVM1_toString)
{
	AVM1Rectangle* th=asAtomHandler::as<AVM1Rectangle>(obj);
	tiny_string s = "(x=";
	s+= asAtomHandler::AVM1toString(th->atomX,wrk,true);
	s+= ", y=";
	s+= asAtomHandler::AVM1toString(th->atomY,wrk,true);
	s+= ", w=";
	s+= asAtomHandler::AVM1toString(th->atomWidth,wrk,true);
	s+= ", h=";
	s+= asAtomHandler::AVM1toString(th->atomHeight,wrk,true);
	s+= ")";
	ret = asAtomHandler::fromObject(abstract_s(wrk,s));
}
