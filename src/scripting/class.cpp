/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "scripting/toplevel/ASString.h"
#include "scripting/class.h"
#include "parsing/tags.h"

using namespace lightspark;

ASObject* lightspark::new_asobject()
{
	return Class<ASObject>::getInstanceS();
}

Prototype* lightspark::new_objectPrototype()
{
	//Create a Prototype object, the class should be ASObject
	Class_base* c=Class<ASObject>::getClass();
	return new (c->memoryAccount) ObjectPrototype(c);
}

Prototype* lightspark::new_functionPrototype(Class_base* functionClass, _NR<Prototype> p)
{
	//Create a Prototype object, the class should be ASObject
	return new (functionClass->memoryAccount) FunctionPrototype(functionClass, p);
}

Function_object* lightspark::new_functionObject(_NR<ASObject> p)
{
	Class_base* c=Class<ASObject>::getClass();
	return new (c->memoryAccount) Function_object(c, p);
}

Class_inherit::Class_inherit(const QName& name, MemoryAccount* m):Class_base(name, m),tag(NULL),bindedToRoot(false)
{
	this->incRef(); //create on reference for the classes map
#ifndef NDEBUG
	bool ret=
#endif
	getSys()->customClasses.insert(this).second;
	assert(ret);
}

void Class_inherit::finalize()
{
	Class_base::finalize();
	class_scope.clear();
}

ASObject* Class_inherit::getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass)
{
	//We override the classdef
	if(realClass==NULL)
		realClass=this;

	ASObject* ret=NULL;
	assert_and_throw(!bindedToRoot);
	if(tag)
	{
		ret=tag->instance(realClass);
		assert_and_throw(ret);
	}
	else
	{
		assert_and_throw(super);
		//Our super should not construct, we are going to do it ourselves
		ret=super->getInstance(false,NULL,0,realClass);
	}
	if(construct)
		handleConstruction(ret,args,argslen,true);
	return ret;
}

void Class_inherit::buildInstanceTraits(ASObject* o) const
{
	assert_and_throw(class_index!=-1);
	//The class is declared in the script and has an index
	LOG(LOG_CALLS,_("Building instance traits"));

	context->buildInstanceTraits(o,class_index);
}

template<>
Global* Class<Global>::getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass)
{
	throw Class<TypeError>::getInstanceS("Error #1007: Cannot construct global object");
}

void lightspark::lookupAndLink(Class_base* c, const tiny_string& name, const tiny_string& interfaceNs)
{
	variable* var=NULL;
	Class_base* cur=c;
	//Find the origin
	while(cur)
	{
		var=cur->borrowedVariables.findObjVar(getSys()->getUniqueStringId(name),nsNameAndKind("",NAMESPACE),NO_CREATE_TRAIT,DECLARED_TRAIT);
		if(var)
			break;
		cur=cur->super.getPtr();
	}
	assert_and_throw(var);
	if(var->var)
	{
		assert_and_throw(var->var->getObjectType()==T_FUNCTION);
		IFunction* f=var->var->as<IFunction>();
		f->incRef();
		c->setDeclaredMethodByQName(name,interfaceNs,f,NORMAL_METHOD,true);
	}
	if(var->getter)
	{
		assert_and_throw(var->getter->getObjectType()==T_FUNCTION);
		IFunction *f=var->getter->as<IFunction>();
		f->incRef();
		c->setDeclaredMethodByQName(name,interfaceNs,f,GETTER_METHOD,true);
	}
	if(var->setter)
	{
		assert_and_throw(var->setter->getObjectType()==T_FUNCTION);
		IFunction *f=var->setter->as<IFunction>();
		f->incRef();
		c->setDeclaredMethodByQName(name,interfaceNs,f,SETTER_METHOD,true);
	}
}

ASObject* Class<ASObject>::getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass)
{
	if (construct && args && argslen == 1 && this == Class<ASObject>::getClass())
	{
		// Construction according to ECMA 15.2.2.1
		switch(args[0]->getObjectType())
		{
		case T_BOOLEAN:
			return abstract_b(Boolean_concrete(args[0]));
		case T_NUMBER:
			return abstract_d(args[0]->toNumber());
		case T_INTEGER:
			return abstract_i(args[0]->toInt());
		case T_UINTEGER:
			return abstract_ui(args[0]->toUInt());
		case T_STRING:
			return Class<ASString>::getInstanceS(args[0]->toString());
		case T_FUNCTION:
		case T_OBJECT:
			args[0]->incRef();
			return args[0];
		default:
			break;
		}
	}
	if(realClass==NULL)
		realClass=this;
	ASObject* ret=new (realClass->memoryAccount) ASObject(realClass);
	if(construct)
		handleConstruction(ret,args,argslen,true);
	return ret;
}
