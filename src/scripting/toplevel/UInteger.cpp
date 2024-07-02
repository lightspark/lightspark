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

#include <cmath>
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Number.h"
#include "scripting/flash/utils/ByteArray.h"

using namespace std;
using namespace lightspark;

tiny_string UInteger::toString() const
{
	return UInteger::toString(val);
}

/* static helper function */
tiny_string UInteger::toString(uint32_t val)
{
	char buf[20];
	snprintf(buf,sizeof(buf),"%u",val);
	return tiny_string(buf,true);
}

TRISTATE UInteger::isLess(ASObject* o)
{
	if(o->getObjectType()==T_UINTEGER)
	{
		uint32_t val1=val;
		uint32_t val2=o->toUInt();
		return (val1<val2)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_INTEGER ||
	   o->getObjectType()==T_BOOLEAN)
	{
		uint32_t val1=val;
		int32_t val2=o->toInt();
		if(val2<0)
			return TFALSE;
		else
			return (val1<(uint32_t)val2)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		number_t val2=o->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (number_t(val) < val2)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NULL)
	{
		// UInteger is never less than int(null) == 0
		return TFALSE;
	}
	else if(o->getObjectType()==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else if(o->getObjectType()==T_STRING)
	{
		double val2=o->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
	else
	{
		asAtom val2p=asAtomHandler::invalidAtom;
		bool isrefcounted;
		o->toPrimitive(val2p,isrefcounted,NUMBER_HINT);
		double val2=asAtomHandler::toNumber(val2p);
		if (isrefcounted)
			ASATOM_DECREF(val2p);
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
}
TRISTATE UInteger::isLessAtom(asAtom& r)
{
	if(asAtomHandler::getObjectType(r)==T_UINTEGER)
	{
		uint32_t val1=val;
		uint32_t val2=asAtomHandler::toUInt(r);
		return (val1<val2)?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_INTEGER ||
	   asAtomHandler::getObjectType(r)==T_BOOLEAN)
	{
		uint32_t val1=val;
		int32_t val2=asAtomHandler::toInt(r);
		if(val2<0)
			return TFALSE;
		else
			return (val1<(uint32_t)val2)?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_NUMBER)
	{
		number_t val2=asAtomHandler::toNumber(r);
		if(std::isnan(val2)) return TUNDEFINED;
		return (number_t(val) < val2)?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_NULL)
	{
		// UInteger is never less than int(null) == 0
		return TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else if(asAtomHandler::getObjectType(r)==T_STRING)
	{
		double val2=asAtomHandler::toNumber(r);
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
	else
	{
		asAtom val2p=asAtomHandler::invalidAtom;
		bool isrefcounted;
		asAtomHandler::getObject(r)->toPrimitive(val2p,isrefcounted,NUMBER_HINT);
		double val2=asAtomHandler::toNumber(val2p);
		if (isrefcounted)
			ASATOM_DECREF(val2p);
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
}

ASFUNCTIONBODY_ATOM(UInteger,_constructor)
{
	UInteger* th=asAtomHandler::as<UInteger>(obj);
	if(argslen==0)
	{
		//The uint is already initialized to 0
		return;
	}
	th->val=asAtomHandler::toUInt(args[0]);
}

ASFUNCTIONBODY_ATOM(UInteger,generator)
{
	if (argslen == 0)
		asAtomHandler::setUInt(ret,wrk,(uint32_t)0);
	else
		asAtomHandler::setUInt(ret,wrk,asAtomHandler::toUInt(args[0]));
}

ASFUNCTIONBODY_ATOM(UInteger,_valueOf)
{
	if(Class<UInteger>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		asAtomHandler::setUInt(ret,wrk,(uint32_t)0);
		return;
	}

	if(!asAtomHandler::isUInteger(obj))
	{
		createError<TypeError>(wrk,0,"");
		return;
	}

	asAtomHandler::setUInt(ret,wrk,asAtomHandler::toUInt(obj));
}

void UInteger::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->isReusable = true;
	c->setVariableAtomByQName("MAX_VALUE",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)0xFFFFFFFF),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MIN_VALUE",nsNameAndKind(),asAtomHandler::fromUInt((uint32_t)0),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,c->getSystemState()->getBuiltinFunction(_toString,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toFixed",AS3,c->getSystemState()->getBuiltinFunction(_toFixed,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toExponential",AS3,c->getSystemState()->getBuiltinFunction(_toExponential,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toPrecision",AS3,c->getSystemState()->getBuiltinFunction(_toPrecision,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,c->getSystemState()->getBuiltinFunction(_valueOf,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toExponential","",c->getSystemState()->getBuiltinFunction(_toExponential,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toFixed","",c->getSystemState()->getBuiltinFunction(_toFixed,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toPrecision","",c->getSystemState()->getBuiltinFunction(_toPrecision,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",c->getSystemState()->getBuiltinFunction(_valueOf,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(UInteger,_toString)
{
	if(Class<UInteger>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"0");
		return;
	}

	uint32_t radix;
	ARG_CHECK(ARG_UNPACK (radix,10));

	if (radix == 10)
	{
		char buf[20];
		snprintf(buf,20,"%u",asAtomHandler::toUInt(obj));
		ret = asAtomHandler::fromObject(abstract_s(wrk,buf));
	}
	else
	{
		tiny_string s=Number::toStringRadix(asAtomHandler::toNumber(obj), radix);
		ret = asAtomHandler::fromObject(abstract_s(wrk,s));
	}
}

bool UInteger::isEqual(ASObject* o)
{
	switch(o->getObjectType())
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_STRING:
		case T_BOOLEAN:
			return val==o->toUInt();
		case T_NULL:
		case T_UNDEFINED:
			return false;
		default:
			return o->isEqual(this);
	}
}

string UInteger::toDebugString() const
{
	tiny_string ret = toString()+"ui";
#ifndef _NDEBUG
	char buf[300];
	sprintf(buf,"(%p/%d/%d%s) ",this,this->getRefCount(),this->storedmembercount,this->isConstructed()?"":" not constructed");
	ret += buf;
#endif
	return ret;
}

ASFUNCTIONBODY_ATOM(UInteger,_toExponential)
{
	double v = asAtomHandler::toNumber(obj);
	int32_t fractionDigits;
	ARG_CHECK(ARG_UNPACK(fractionDigits, 0));
	if (argslen == 0 || asAtomHandler::is<Undefined>(args[0]))
	{
		if (v == 0)
			fractionDigits = 1;
		else
			fractionDigits = imin(imax((int32_t)ceil(::log10(v)), 1), 20);
	}
	ret = asAtomHandler::fromObject(abstract_s(wrk,Number::toExponentialString(v, fractionDigits)));
}

ASFUNCTIONBODY_ATOM(UInteger,_toFixed)
{
	int fractiondigits;
	ARG_CHECK(ARG_UNPACK (fractiondigits, 0));
	ret = asAtomHandler::fromObject(abstract_s(wrk,Number::toFixedString(asAtomHandler::toNumber(obj), fractiondigits)));
}

ASFUNCTIONBODY_ATOM(UInteger,_toPrecision)
{
	if (argslen == 0 || asAtomHandler::is<Undefined>(args[0]))
	{
		ret = asAtomHandler::fromObject(abstract_s(wrk,asAtomHandler::toString(obj,wrk)));
		return;
	}
	int precision;
	ARG_CHECK(ARG_UNPACK (precision));
	ret = asAtomHandler::fromObject(abstract_s(wrk,Number::toPrecisionString(asAtomHandler::toNumber(obj), precision)));
}

void UInteger::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	serializeValue(out,val);
}

void UInteger::serializeValue(ByteArray* out, uint32_t val)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		out->writeByte(amf0_number_marker);
		out->serializeDouble(val);
		return;
	}
	if(val>=0x40000000)
	{
		// write as double
		out->writeByte(double_marker);
		out->serializeDouble(val);
	}
	else
	{
		out->writeByte(integer_marker);
		out->writeU29((uint32_t)val);
	}
}
