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
#include "scripting/toplevel/toplevel.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "parsing/amf3_generator.h"

using namespace lightspark;
using namespace std;

Boolean* lightspark::abstract_b(SystemState *sys, bool v)
{
	if(v==true)
		return sys->getTrueRef();
	else
		return sys->getFalseRef();
}

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
	case T_ARRAY:
	case T_OBJECT:
		// not constructed objects return false
		if (!o->isConstructed())
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
		return asAtom(args[0].Boolean_concrete());
	else
		return asAtom::falseAtom;
}

void Boolean::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(c->getSystemState(),_valueOf),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Boolean,_constructor)
{
	Boolean* th=static_cast<Boolean*>(obj.getObject());
	if(argslen==0)
	{
		//No need to handle default argument. The object is initialized to false anyway
		return asAtom::invalidAtom;
	}
	th->val=args[0].Boolean_concrete();
	return asAtom::invalidAtom;
}

ASFUNCTIONBODY(Boolean,_toString)
{
	if(Class<Boolean>::getClass(obj->getSystemState())->prototype->getObj() == obj) //See ECMA 15.6.4
		return abstract_s(obj->getSystemState(),"false");

	if(!obj->is<Boolean>())
		throw Class<TypeError>::getInstanceS(obj->getSystemState(),"");

	Boolean* th=static_cast<Boolean*>(obj);
	return abstract_s(obj->getSystemState(),th->toString());
}

ASFUNCTIONBODY(Boolean,_valueOf)
{
	if(Class<Boolean>::getClass(obj->getSystemState())->prototype->getObj() == obj)
		return abstract_b(obj->getSystemState(),false);

	if(!obj->is<Boolean>())
			throw Class<TypeError>::getInstanceS(obj->getSystemState(),"");

	//The ecma3 spec is unspecific, but testing showed that we should return
	//a new object
	return abstract_b(obj->getSystemState(),obj->as<Boolean>()->val);
}

void Boolean::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	if (out->getObjectEncoding() == ObjectEncoding::AMF0)
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
			double val2=r->toPrimitive()->toNumber();
			if(std::isnan(val2)) return TUNDEFINED;
			return (val<val2)?TTRUE:TFALSE;
		}
	}
}
