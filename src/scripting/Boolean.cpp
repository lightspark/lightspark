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

using namespace lightspark;
using namespace std;

SET_NAMESPACE("");
REGISTER_CLASS_NAME(Boolean);

Boolean* lightspark::abstract_b(bool i)
{
	return Class<Boolean>::getInstanceS(i);
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
		return o->as<ASString>()->data.size() > 0;
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->prototype->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(_valueOf),NORMAL_METHOD,false);
}

ASFUNCTIONBODY(Boolean,_constructor)
{
	Boolean* th=static_cast<Boolean*>(obj);
	//TODO: if argslen == 0, this should set
	//th->val = false;
	//But if one calls Class<Boolean>::getInstanceS(true), it does
	//  Boolean* b = new Boolean(true);
	//  b::_constructor(b,NULL,0);
	//so we would here override the value set in the Boolean::Boolean()
	if(argslen == 1)
	{
		th->val=Boolean_concrete(args[0]);
	}
	return NULL;
}

ASFUNCTIONBODY(Boolean,_toString)
{
	if(!obj->is<Boolean>())
		throw Class<TypeError>::getInstanceS("");

	Boolean* th=static_cast<Boolean*>(obj);
	return Class<ASString>::getInstanceS(th->toString(false));
}

ASFUNCTIONBODY(Boolean,_valueOf)
{
	if(!obj->is<Boolean>())
			throw Class<TypeError>::getInstanceS("");

	//The ecma3 spec is unspecific, but testing showed that we should return
	//a new object
	return abstract_b(obj->as<Boolean>()->val);
}

void Boolean::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	throw UnsupportedException("Boolean:serialize not implemented");
}

tiny_string Boolean::toString(bool debugMsg)
{
	return (val)?"true":"false";
}

bool Boolean::isEqual(ASObject* r)
{
	if(r->getObjectType()==T_BOOLEAN)
	{
		const Boolean* b=static_cast<const Boolean*>(r);
		return b->val==val;
	}
	else if(r->getObjectType()==T_INTEGER ||
		r->getObjectType()==T_UINTEGER ||
		r->getObjectType()==T_NUMBER)
	{
		return val==r->toNumber();
	}
	else
	{
		return ASObject::isEqual(r);
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
		r->getObjectType()==T_NUMBER)
	{
		double d=r->toNumber();
		if(std::isnan(d)) return TUNDEFINED;
		return (val<d)?TTRUE:TFALSE;
	}
	else
		return ASObject::isLess(r);
}
