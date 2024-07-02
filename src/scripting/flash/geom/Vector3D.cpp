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

#include "scripting/abc.h"
#include "scripting/flash/geom/Vector3D.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"

using namespace lightspark;
using namespace std;

void Vector3D::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;

	// constants
	Vector3D* tx = new (c->memoryAccount) Vector3D(c->getInstanceWorker(),c);
	tx->x = 1;
	c->setVariableByQName("X_AXIS","", tx, DECLARED_TRAIT);
	tx->setRefConstant();

	Vector3D* ty = new (c->memoryAccount) Vector3D(c->getInstanceWorker(),c);
	ty->y = 1;
	c->setVariableByQName("Y_AXIS","", ty, DECLARED_TRAIT);
	ty->setRefConstant();

	Vector3D* tz = new (c->memoryAccount) Vector3D(c->getInstanceWorker(),c);
	tz->z = 1;
	c->setVariableByQName("Z_AXIS","", tz, DECLARED_TRAIT);
	tz->setRefConstant();

	// properties
	REGISTER_GETTER_SETTER_RESULTTYPE(c, x, Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, y, Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, z, Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, w, Number);
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(_get_length,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("lengthSquared","",c->getSystemState()->getBuiltinFunction(_get_lengthSquared,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	
	// methods 
	c->setDeclaredMethodByQName("add","",c->getSystemState()->getBuiltinFunction(add,1,Class<Vector3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("angleBetween","",c->getSystemState()->getBuiltinFunction(angleBetween,2,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<Vector3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("crossProduct","",c->getSystemState()->getBuiltinFunction(crossProduct,1,Class<Vector3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("decrementBy","",c->getSystemState()->getBuiltinFunction(decrementBy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("distance","",c->getSystemState()->getBuiltinFunction(distance,2,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("dotProduct","",c->getSystemState()->getBuiltinFunction(dotProduct,1,Class<Vector3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",c->getSystemState()->getBuiltinFunction(equals,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("incrementBy","",c->getSystemState()->getBuiltinFunction(incrementBy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nearEquals","",c->getSystemState()->getBuiltinFunction(nearEquals,1,Class<Vector3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("negate","",c->getSystemState()->getBuiltinFunction(negate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("normalize","",c->getSystemState()->getBuiltinFunction(normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("project","",c->getSystemState()->getBuiltinFunction(project),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scaleBy","",c->getSystemState()->getBuiltinFunction(scaleBy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("subtract","",c->getSystemState()->getBuiltinFunction(subtract,1,Class<Vector3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTo","",c->getSystemState()->getBuiltinFunction(setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",c->getSystemState()->getBuiltinFunction(copyFrom),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Vector3D,_constructor)
{
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	ARG_CHECK(ARG_UNPACK(th->x,0)(th->y,0)(th->z,0)(th->w,0));
}

bool Vector3D::destruct()
{
	w = 0;
	x = 0;
	y = 0;
	z = 0;
	return destructIntern();
}

ASFUNCTIONBODY_GETTER_SETTER(Vector3D,x)
ASFUNCTIONBODY_GETTER_SETTER(Vector3D,y)
ASFUNCTIONBODY_GETTER_SETTER(Vector3D,z)
ASFUNCTIONBODY_GETTER_SETTER(Vector3D,w)

ASFUNCTIONBODY_ATOM(Vector3D,_toString)
{
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	tiny_string s("Vector3D(");
	s+=Number::toString(th->x);
	s+=", ";
	s+=Number::toString(th->y);
	s+=", ";
	s+=Number::toString(th->z);
	s+=")";
	ret = asAtomHandler::fromObject(abstract_s(wrk,s));
}


ASFUNCTIONBODY_ATOM(Vector3D,_get_length)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	asAtomHandler::setNumber(ret,wrk,sqrt(th->x * th->x + th->y * th->y + th->z * th->z));
}

ASFUNCTIONBODY_ATOM(Vector3D,_get_lengthSquared)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	asAtomHandler::setNumber(ret,wrk,th->x * th->x + th->y * th->y + th->z * th->z);
}

ASFUNCTIONBODY_ATOM(Vector3D,clone)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* res=Class<Vector3D>::getInstanceS(wrk);

	res->w = th->w;
	res->x = th->x;
	res->y = th->y;
	res->z = th->z;

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Vector3D,add)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);
	Vector3D* res=Class<Vector3D>::getInstanceS(wrk);

	res->x = th->x + vc->x;
	res->y = th->y + vc->y;
	res->z = th->z + vc->z;

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Vector3D,angleBetween)
{
	assert_and_throw(argslen==2);

	Vector3D* vc1=asAtomHandler::as<Vector3D>(args[0]);
	Vector3D* vc2=asAtomHandler::as<Vector3D>(args[1]);

	number_t angle = vc1->x * vc2->x + vc1->y * vc2->y + vc1->z * vc2->z;
	angle /= sqrt(vc1->x * vc1->x + vc1->y * vc1->y + vc1->z * vc1->z);
	angle /= sqrt(vc2->x * vc2->x + vc2->y * vc2->y + vc2->z * vc2->z);
	angle = acos(angle);

	asAtomHandler::setNumber(ret,wrk,angle);
}

ASFUNCTIONBODY_ATOM(Vector3D,crossProduct)
{
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	_NR<Vector3D> vc;
	ARG_CHECK(ARG_UNPACK(vc));
	Vector3D* res=Class<Vector3D>::getInstanceS(wrk);

	res->x = th->y * vc->z - th->z * vc->y;
	res->y = th->z * vc->x - th->x * vc->z;
	res->z = th->x * vc->y - th->y * vc->x;
	res->w = 1.0; // don't know why but adobe seems to always set w to 1;

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Vector3D,decrementBy)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);

	th->x -= vc->x;
	th->y -= vc->y;
	th->z -= vc->z;
}

ASFUNCTIONBODY_ATOM(Vector3D,distance)
{
	assert_and_throw(argslen==2);

	Vector3D* vc1=asAtomHandler::as<Vector3D>(args[0]);
	Vector3D* vc2=asAtomHandler::as<Vector3D>(args[1]);

	number_t dx, dy, dz, dist;
	dx = vc1->x - vc2->x;
	dy = vc1->y - vc2->y;
	dz = vc1->z - vc2->z;
	dist = sqrt(dx * dx + dy * dy + dz * dz);

	asAtomHandler::setNumber(ret,wrk,dist);
}

ASFUNCTIONBODY_ATOM(Vector3D,dotProduct)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);

	asAtomHandler::setNumber(ret,wrk,th->x * vc->x + th->y * vc->y + th->z * vc->z);
}

ASFUNCTIONBODY_ATOM(Vector3D,equals)
{
	assert_and_throw(argslen==1 || argslen==2);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	_NR<Vector3D> vc;
	bool allfour;
	ARG_CHECK(ARG_UNPACK(vc)(allfour,false));
	asAtomHandler::setBool(ret,th->x == vc->x &&  th->y == vc->y && th->z == vc->z && (allfour ? th->w == vc->w : true));
}

ASFUNCTIONBODY_ATOM(Vector3D,incrementBy)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	Vector3D* vc=asAtomHandler::as<Vector3D>(args[0]);

	th->x += vc->x;
	th->y += vc->y;
	th->z += vc->z;
}

ASFUNCTIONBODY_ATOM(Vector3D,nearEquals)
{
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	_NR<Vector3D> vc;
	number_t tolerance;
	bool allfour;
	ARG_CHECK(ARG_UNPACK(vc)(tolerance)(allfour,false));

	bool dx, dy, dz, dw;
	dx = abs(th->x - vc->x) < tolerance;
	dy = abs(th->y - vc->y) < tolerance;
	dz = abs(th->z - vc->z) < tolerance;
	dw = allfour ? abs(vc->w) < tolerance : true; // there seems to be a bug in the adobe player, so that in the allfour case th->w is ignored
	asAtomHandler::setBool(ret,dx && dy && dz && dw);
}

ASFUNCTIONBODY_ATOM(Vector3D,negate)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);

	th->x = -th->x;
	th->y = -th->y;
	th->z = -th->z;
}

ASFUNCTIONBODY_ATOM(Vector3D,normalize)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);

	number_t len = sqrt(th->x * th->x + th->y * th->y + th->z * th->z);

	if (std::isnan(len))
	{
		th->x = Number::NaN;
		th->y = Number::NaN;
		th->z = Number::NaN;
	}
	else if (len)
	{
		th->x /= len;
		th->y /= len;
		th->z /= len;
	}
	else
	{
		th->x = 0;
		th->y = 0;
		th->z = 0;
	}
	ret = asAtomHandler::fromNumber(wrk,len,false);
}

ASFUNCTIONBODY_ATOM(Vector3D,project)
{
	assert_and_throw(argslen==0);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);

	th->x /= th->w;
	th->y /= th->w;
	th->z /= th->w;
}

ASFUNCTIONBODY_ATOM(Vector3D,scaleBy)
{
	assert_and_throw(argslen==1);

	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	number_t scale = asAtomHandler::toNumber(args[0]);

	th->x *= scale;
	th->y *= scale;
	th->z *= scale;
}

ASFUNCTIONBODY_ATOM(Vector3D,subtract)
{
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	_NR<Vector3D> vc;
	ARG_CHECK(ARG_UNPACK(vc));
	Vector3D* res=Class<Vector3D>::getInstanceS(wrk);

	res->x = th->x - vc->x;
	res->y = th->y - vc->y;
	res->z = th->z - vc->z;

	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Vector3D,setTo)
{
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	number_t xa,ya,za;
	ARG_CHECK(ARG_UNPACK(xa)(ya)(za));
	th->x = xa;
	th->y = ya;
	th->z = za;
}
ASFUNCTIONBODY_ATOM(Vector3D,copyFrom)
{
	Vector3D* th=asAtomHandler::as<Vector3D>(obj);
	_NR<Vector3D> sourcevector;
	ARG_CHECK(ARG_UNPACK(sourcevector));
	if (!sourcevector.isNull())
	{
		// spec says "Copies all of vector data from the source Vector3D object into the calling Vector3D object."
		// but apparently that doesn't include w.
		//th->w = sourcevector->w; 
		th->x = sourcevector->x;
		th->y = sourcevector->y;
		th->z = sourcevector->z;
	}
}
