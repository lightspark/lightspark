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

#include "avm1point.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

AVM1Point::AVM1Point(ASWorker* wrk, Class_base* c):Point(wrk,c,Number::NaN,Number::NaN)
	,atomX(asAtomHandler::undefinedAtom)
	,atomY(asAtomHandler::undefinedAtom)
{
	subtype=SUBTYPE_AVM1POINT;
}

void AVM1Point::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);

	c->prototype->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(AVM1_getX,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("y","",c->getSystemState()->getBuiltinFunction(AVM1_getY,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(_getlength,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("x","",c->getSystemState()->getBuiltinFunction(AVM1_setX),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("y","",c->getSystemState()->getBuiltinFunction(AVM1_setY),SETTER_METHOD,false,false);

	c->prototype->setVariableByQName("interpolate","",c->getSystemState()->getBuiltinFunction(AVM1_interpolate,3,Class<AVM1Point>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("distance","",c->getSystemState()->getBuiltinFunction(AVM1_distance,2,Class<AVM1Point>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("add","",c->getSystemState()->getBuiltinFunction(add,1,Class<Point>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("subtract","",c->getSystemState()->getBuiltinFunction(subtract,1,Class<Point>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<Point>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("equals","",c->getSystemState()->getBuiltinFunction(equals,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("normalize","",c->getSystemState()->getBuiltinFunction(AVM1_normalize),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("offset","",c->getSystemState()->getBuiltinFunction(AVM1_offset),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("polar","",c->getSystemState()->getBuiltinFunction(AVM1_polar,2,Class<Point>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(AVM1_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

void AVM1Point::finalize()
{
	ASATOM_REMOVESTOREDMEMBER(atomX);
	atomX=asAtomHandler::undefinedAtom;
	ASATOM_REMOVESTOREDMEMBER(atomY);
	atomY=asAtomHandler::undefinedAtom;
	Point::finalize();
}
bool AVM1Point::destruct()
{
	ASATOM_REMOVESTOREDMEMBER(atomX);
	atomX=asAtomHandler::undefinedAtom;
	ASATOM_REMOVESTOREDMEMBER(atomY);
	atomY=asAtomHandler::undefinedAtom;
	return Point::destruct();
}

void AVM1Point::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASATOM_PREPARESHUTDOWN(atomX);
	ASATOM_REMOVESTOREDMEMBER(atomX);
	atomX=asAtomHandler::invalidAtom;
	ASATOM_PREPARESHUTDOWN(atomY);
	ASATOM_REMOVESTOREDMEMBER(atomY);
	atomY=asAtomHandler::invalidAtom;
	Point::prepareShutdown();
}

bool AVM1Point::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = Point::countCylicMemberReferences(gcstate);
	ASObject* o;
	o = asAtomHandler::getObject(atomX);
	if (o)
		ret = o->countAllCylicMemberReferences(gcstate) || ret;
	o = asAtomHandler::getObject(atomY);
	if (o)
		ret = o->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

ASFUNCTIONBODY_ATOM(AVM1Point,_constructor)
{
	AVM1Point* th=asAtomHandler::as<AVM1Point>(obj);
	if (argslen==0)
	{
		th->atomX = asAtomHandler::fromInt(0);
		th->atomY = asAtomHandler::fromInt(0);
		th->x=0;
		th->y=0;
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
}
ASFUNCTIONBODY_ATOM(AVM1Point,AVM1_getX)
{
	AVM1Point* th=asAtomHandler::as<AVM1Point>(obj);
	ret = th->atomX;
	ASATOM_INCREF(ret);
}
ASFUNCTIONBODY_ATOM(AVM1Point,AVM1_setX)
{
	AVM1Point* th=asAtomHandler::as<AVM1Point>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	ASATOM_ADDSTOREDMEMBER(args[0]);
	ASATOM_REMOVESTOREDMEMBER(th->atomX);
	th->atomX=args[0];
	th->x=val;
}
ASFUNCTIONBODY_ATOM(AVM1Point,AVM1_getY)
{
	AVM1Point* th=asAtomHandler::as<AVM1Point>(obj);
	ret = th->atomY;
	ASATOM_INCREF(ret);
}
ASFUNCTIONBODY_ATOM(AVM1Point,AVM1_setY)
{
	AVM1Point* th=asAtomHandler::as<AVM1Point>(obj);
	number_t val;
	ARG_CHECK(ARG_UNPACK(val));
	ASATOM_ADDSTOREDMEMBER(args[0]);
	ASATOM_REMOVESTOREDMEMBER(th->atomY);
	th->atomY=args[0];
	th->y=val;
}
ASFUNCTIONBODY_ATOM(AVM1Point,AVM1_interpolate)
{
	Vector2f pt1;
	Vector2f pt2;
	number_t f;
	AVM1Point* res=Class<AVM1Point>::getInstanceS(wrk);
	if (argslen < 3)
	{
		res->setX(wrk->getSystemState()->nanAtom,wrk,false);
		res->setY(wrk->getSystemState()->nanAtom,wrk,false);
		ret = asAtomHandler::fromObject(res);
		return;
	}

	ARG_CHECK(ARG_UNPACK_NO_ERROR(pt1)(pt2)(f, Number::NaN));

	Vector2f interpPoint = pt2 - (pt2 - pt1) * f;
	res->setX(asAtomHandler::fromNumber(wrk,interpPoint.x,false),wrk,false);
	res->setY(asAtomHandler::fromNumber(wrk,interpPoint.y,false),wrk,false);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(AVM1Point,AVM1_distance)
{
	if (argslen < 2)
	{
		ret = wrk->getSystemState()->nanAtom;
		return;
	}
	if (!asAtomHandler::is<Point>(args[0])
		|| !asAtomHandler::is<Point>(args[1]))
	{
		ret = asAtomHandler::undefinedAtom;
		return;
	}
	Vector2f pt1;
	Vector2f pt2;

	ARG_CHECK(ARG_UNPACK_NO_ERROR(pt1)(pt2));

	Vector2f diff = pt2 - pt1;
	number_t res=lenImpl(diff.x, diff.y);
	wrk->setBuiltinCallResultLocalNumber(ret, res);
}

ASFUNCTIONBODY_ATOM(AVM1Point,AVM1_normalize)
{
	AVM1Point* th=asAtomHandler::as<AVM1Point>(obj);
	number_t thickness = argslen==0 ? Number::NaN : asAtomHandler::AVM1toNumber(args[0],wrk->AVM1getSwfVersion());
	number_t len = th->len();
	if (std::isnan(len) || !asAtomHandler::isNumeric(th->atomX) || !asAtomHandler::isNumeric(th->atomY))
		return;
	th->setX(asAtomHandler::fromNumber(wrk,len == 0 ? 0 : th->x * thickness / len,false),wrk,false);
	th->setY(asAtomHandler::fromNumber(wrk,len == 0 ? 0 : th->y * thickness / len,false),wrk,false);
}

ASFUNCTIONBODY_ATOM(AVM1Point,AVM1_offset)
{
	AVM1Point* th=asAtomHandler::as<AVM1Point>(obj);
	if (argslen == 0)
		return;
	number_t dx = asAtomHandler::toNumber(args[0]);
	number_t dy = argslen > 1 ? asAtomHandler::toNumber(args[1]) : Number::NaN;
	th->setX(asAtomHandler::fromNumber(wrk,th->x + dx,false),wrk,false);
	th->setY(asAtomHandler::fromNumber(wrk,th->y + dy,false),wrk,false);
}

ASFUNCTIONBODY_ATOM(AVM1Point,AVM1_polar)
{
	if (argslen == 0)
		return;
	number_t len = asAtomHandler::toNumber(args[0]);
	number_t angle = argslen > 1 ? asAtomHandler::toNumber(args[1]) : Number::NaN;
	AVM1Point* res=Class<AVM1Point>::getInstanceS(wrk);
	res->setX(asAtomHandler::fromNumber(wrk,len * cos(angle),false),wrk,false);
	res->setY(asAtomHandler::fromNumber(wrk,len * sin(angle),false),wrk,false);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(AVM1Point,AVM1_toString)
{
	AVM1Point* th=asAtomHandler::as<AVM1Point>(obj);
	char buf[512];
	snprintf(buf,512,"(x=%s, y=%s)",asAtomHandler::AVM1toString(th->atomX,wrk,true).raw_buf(),asAtomHandler::AVM1toString(th->atomY,wrk,true).raw_buf());
	ret = asAtomHandler::fromObject(abstract_s(wrk,buf));
}

void AVM1Point::setX(asAtom val, ASWorker* wrk, bool incref)
{
	ASATOM_REMOVESTOREDMEMBER(atomX);
	atomX = val;
	if (incref)
	{
		ASATOM_ADDSTOREDMEMBER(atomX);
	}
	else if (asAtomHandler::isObject(val))
		asAtomHandler::getObjectNoCheck(val)->addStoredMember();
	x=asAtomHandler::AVM1toNumber(val,wrk->AVM1getSwfVersion());
}
void AVM1Point::setY(asAtom val, ASWorker* wrk, bool incref)
{
	ASATOM_REMOVESTOREDMEMBER(atomY);
	atomY = val;
	if (incref)
	{
		ASATOM_ADDSTOREDMEMBER(atomY);
	}
	else if (asAtomHandler::isObject(val))
		asAtomHandler::getObjectNoCheck(val)->addStoredMember();
	y=asAtomHandler::AVM1toNumber(val,wrk->AVM1getSwfVersion());
}
