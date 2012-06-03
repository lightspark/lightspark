/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "Boolean.h"
#include "toplevel.h"
#include "class.h"
#include "argconv.h"
#include "parsing/amf3_generator.h"

using namespace lightspark;
using namespace std;

SET_NAMESPACE("");
REGISTER_CLASS_NAME(Boolean);

Boolean* lightspark::abstract_b(bool v)
{
	Boolean* b = Class<Boolean>::getInstanceS();
	b->val = v;
	return b;
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
		return (o->as<Number>()->val != 0) && !std::isnan(o->as<Number>()->val);
	case T_INTEGER:
		return o->as<Integer>()->val != 0;
	case T_UINTEGER:
		return o->as<UInteger>()->val != 0;
	case T_STRING:
		return !o->as<ASString>()->data.empty();
	default:
		//everything else is an Object regarding to the spec
		return true;
	}
}

/* this implements the global Boolean() function */
ASFUNCTIONBODY(Boolean,generator)
{
	if(argslen==1)
		return abstract_b(Boolean_concrete(args[0]));
	else
		return abstract_b(false);
}

void Boolean::sinit(Class_base* c)
{
	c->isFinal=true;
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf",AS3,Class<IFunction>::getFunction(_valueOf),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY(Boolean,_constructor)
{
	Boolean* th=static_cast<Boolean*>(obj);
	_NR<ASObject> o;
	ARG_UNPACK (o,_MNR(getSys()->getUndefinedRef()));
	th->val=Boolean_concrete(o.getPtr());
	return NULL;
}

ASFUNCTIONBODY(Boolean,_toString)
{
	if(Class<Boolean>::getClass()->prototype == obj) //See ECMA 15.6.4
		return Class<ASString>::getInstanceS("false");

	if(!obj->is<Boolean>())
		throw Class<TypeError>::getInstanceS("");

	Boolean* th=static_cast<Boolean*>(obj);
	return Class<ASString>::getInstanceS(th->toString());
}

ASFUNCTIONBODY(Boolean,_valueOf)
{
	if(Class<Boolean>::getClass()->prototype == obj)
		return abstract_b(false);

	if(!obj->is<Boolean>())
			throw Class<TypeError>::getInstanceS("");

	//The ecma3 spec is unspecific, but testing showed that we should return
	//a new object
	return abstract_b(obj->as<Boolean>()->val);
}

void Boolean::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	if(val)
		out->writeByte(true_marker);
	else
		out->writeByte(false_marker);
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
	if(r->getObjectType()==T_BOOLEAN)
	{
		const Boolean* b=static_cast<const Boolean*>(r);
		return (val<b->val)?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_INTEGER ||
		r->getObjectType()==T_UINTEGER ||
		r->getObjectType()==T_NUMBER ||
		r->getObjectType()==T_STRING)
	{
		double d=r->toNumber();
		if(std::isnan(d)) return TUNDEFINED;
		return (val<d)?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NULL)
	{
		return (val<0)?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else
	{
		double val2=r->toPrimitive()->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
}
