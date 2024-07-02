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

#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "parsing/amf3_generator.h"

using namespace lightspark;
using namespace std;

/* implements ecma3's ToBoolean() operation, see section 9.2, but returns the value instead of an Boolean object */
bool lightspark::Boolean_concrete(const ASObject* o)
{
	switch(o->getObjectType())
	{
	case T_UNDEFINED:
	case T_NULL:
		return false;
	case T_BOOLEAN:
		return o->as<Boolean>()->val;
	case T_NUMBER:
		return (o->as<Number>()->isfloat ? o->as<Number>()->dval != 0.0 && !std::isnan(o->as<Number>()->dval) : o->as<Number>()->ival != 0);
	case T_INTEGER:
		return o->as<Integer>()->val != 0;
	case T_UINTEGER:
		return o->as<UInteger>()->val != 0;
	case T_STRING:
		if (!o->isConstructed())
			return false;
		return !o->as<ASString>()->isEmpty();
	case T_FUNCTION:
		// not constructed functions return false
		if (!o->isConstructed())
			return false;
		return true;
	case T_ARRAY:
	case T_OBJECT:
		// not initialized objects return false
		if (!o->isInitialized())
			return false;
		return true;
	default:
		//everything else is an Object regarding to the spec
		return true;
	}
}

/* this implements the global Boolean() function */
ASFUNCTIONBODY_ATOM(Boolean,generator)
{
	if(argslen==1)
		ret = asAtomHandler::fromBool(asAtomHandler::Boolean_concrete(args[0]));
	else
		ret = asAtomHandler::falseAtom;
}

void Boolean::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("toString",AS3,c->getSystemState()->getBuiltinFunction(_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",c->getSystemState()->getBuiltinFunction(_valueOf),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Boolean,_constructor)
{
	Boolean* th=asAtomHandler::as<Boolean>(obj);
	if(argslen==0)
	{
		//No need to handle default argument. The object is initialized to false anyway
		return;
	}
	th->val=asAtomHandler::Boolean_concrete(args[0]);
}

ASFUNCTIONBODY_ATOM(Boolean,_toString)
{
	if(Class<Boolean>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj)) //See ECMA 15.6.4
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"false");
		return;
	}

	if(!asAtomHandler::is<Boolean>(obj))
	{
		createError<TypeError>(wrk,0,"");
		return;
	}

	ret = asAtomHandler::fromString(wrk->getSystemState(),asAtomHandler::toString(obj,wrk));
}

ASFUNCTIONBODY_ATOM(Boolean,_valueOf)
{
	if(Class<Boolean>::getClass(wrk->getSystemState())->prototype->getObj() == asAtomHandler::getObject(obj))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	if(!asAtomHandler::is<Boolean>(obj))
	{
		createError<TypeError>(wrk,0,"");
		return;
	}

	//The ecma3 spec is unspecific, but testing showed that we should return
	//a new object
	asAtomHandler::setBool(ret,asAtomHandler::Boolean_concrete(obj));
}

void Boolean::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		out->writeByte(amf0_boolean_marker);
		out->writeByte(val ? 1:0);
	}
	else
		out->writeByte(val ? true_marker : false_marker);
}

bool Boolean::isEqual(ASObject* r)
{
	switch(r->getObjectType())
	{
		case T_BOOLEAN:
		{
			const Boolean* b=static_cast<const Boolean*>(r);
			return b->val==val;
		}
		case T_STRING:
			if (!r->isConstructed())
				return false;
			return val==r->toNumber();
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
			return val==r->toNumber();
		case T_NULL:
		case T_UNDEFINED:
			return false;
		default:
			return r->isEqual(this);
	}
}

TRISTATE Boolean::isLess(ASObject* r)
{
	switch (r->getObjectType())
	{
		case T_BOOLEAN:
		{
			const Boolean* b=static_cast<const Boolean*>(r);
			return (val<b->val)?TTRUE:TFALSE;
		}
		case T_INTEGER:
		{
			int32_t d=r->toInt();
			return (val<d)?TTRUE:TFALSE;
		}
		case T_UINTEGER:
		{
			uint32_t d=r->toUInt();
			return (val<d)?TTRUE:TFALSE;
		}
		case T_NUMBER:
		case T_STRING:
		{
			double d=r->toNumber();
			if(std::isnan(d)) return TUNDEFINED;
			return (val<d)?TTRUE:TFALSE;
		}
		case T_NULL:
			return (val)?TFALSE:TTRUE;
		case T_UNDEFINED:
			return TUNDEFINED;
		default:
		{
			asAtom val2p=asAtomHandler::invalidAtom;
			bool isrefcounted;
			r->toPrimitive(val2p,isrefcounted);
			double val2=asAtomHandler::toNumber(val2p);
			if (isrefcounted)
				ASATOM_DECREF(val2p);
			if(std::isnan(val2)) return TUNDEFINED;
			return (val<val2)?TTRUE:TFALSE;
		}
	}
}

TRISTATE Boolean::isLessAtom(asAtom& r)
{
	switch (asAtomHandler::getObjectType(r))
	{
		case T_BOOLEAN:
		{
			return (val<asAtomHandler::toInt(r))?TTRUE:TFALSE;
		}
		case T_INTEGER:
		{
			int32_t d=asAtomHandler::toInt(r);
			return (val<d)?TTRUE:TFALSE;
		}
		case T_UINTEGER:
		{
			uint32_t d=asAtomHandler::toUInt(r);
			return (val<d)?TTRUE:TFALSE;
		}
		case T_NUMBER:
		case T_STRING:
		{
			double d=asAtomHandler::toNumber(r);
			if(std::isnan(d)) return TUNDEFINED;
			return (val<d)?TTRUE:TFALSE;
		}
		case T_NULL:
			return (val)?TFALSE:TTRUE;
		case T_UNDEFINED:
			return TUNDEFINED;
		default:
		{
			asAtom val2p=asAtomHandler::invalidAtom;
			bool isrefcounted;
			asAtomHandler::getObject(r)->toPrimitive(val2p,isrefcounted);
			double val2=asAtomHandler::toNumber(val2p);
			if (isrefcounted)
				ASATOM_DECREF(val2p);
			if(std::isnan(val2)) return TUNDEFINED;
			return (val<val2)?TTRUE:TFALSE;
		}
	}
}
