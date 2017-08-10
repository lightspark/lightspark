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
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/class.h"
#include "parsing/tags.h"

using namespace lightspark;

ASObject* lightspark::new_asobject(SystemState* sys)
{
	return Class<ASObject>::getInstanceS(sys);
}

Prototype* lightspark::new_objectPrototype(SystemState* sys)
{
	//Create a Prototype object, the class should be ASObject
	Class_base* c=Class<ASObject>::getClass(sys);
	return new (c->memoryAccount) ObjectPrototype(c);
}

Prototype* lightspark::new_functionPrototype(Class_base* functionClass, _NR<Prototype> p)
{
	//Create a Prototype object, the class should be ASObject
	return new (functionClass->memoryAccount) FunctionPrototype(functionClass, p);
}

Function_object* lightspark::new_functionObject(_NR<ASObject> p)
{
	Class_base* c=Class<ASObject>::getClass(p->getSystemState());
	return new (c->memoryAccount) Function_object(c, p);
}

ObjectConstructor* lightspark::new_objectConstructor(Class_base* cls,uint32_t length)
{
	return new (cls->memoryAccount) ObjectConstructor(cls, length);
}

Activation_object* lightspark::new_activationObject(SystemState* sys)
{
	Class_base* c=Class<ASObject>::getClass(sys);
	return new (c->memoryAccount) Activation_object(c);
}


Class_inherit::Class_inherit(const QName& name, MemoryAccount* m, const traits_info *_classtrait):Class_base(name, m),tag(NULL),bindedToRoot(false),classtrait(_classtrait)
{
	this->incRef(); //create on reference for the classes map
#ifndef NDEBUG
	bool ret=
#endif
	this->getSystemState()->customClasses.insert(this).second;
	assert(ret);
	isReusable = true;
	subtype = SUBTYPE_INHERIT;
}

asAtom Class_inherit::getInstance(bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass)
{
	//We override the classdef
	if(realClass==NULL)
		realClass=this;

	asAtom ret;
	if(tag)
	{
		ret=asAtom::fromObject(tag->instance(realClass));
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
void Class_inherit::recursiveBuild(ASObject* target) const
{
	if(super && super->is<Class_inherit>())
		super->as<Class_inherit>()->recursiveBuild(target);

	buildInstanceTraits(target);
}


void Class_inherit::buildInstanceTraits(ASObject* o) const
{
	if (class_index == -1 && o->getClass()->is<Class_inherit>() && o->getClass()->as<Class_inherit>()->isBinded())
		return;

	assert_and_throw(class_index!=-1);
	//The class is declared in the script and has an index
	LOG(LOG_CALLS,_("Building instance traits"));

	context->buildInstanceTraits(o,class_index);
}
void Class_inherit::setupDeclaredTraits(ASObject *target) const
{
	if (!target->traitsInitialized)
	{
	#ifndef NDEBUG
		assert_and_throw(!target->initialized);
	#endif
		//HACK: suppress implementation handling of variables just now
		bool bak=target->implEnable;
		target->implEnable=false;
		recursiveBuild(target);
		
		//And restore it
		target->implEnable=bak;

	#ifndef NDEBUG
		target->initialized=true;
	#endif
		target->traitsInitialized = true;
	}
}

void Class_inherit::describeClassMetadata(pugi::xml_node &root) const
{
	if(classtrait && classtrait->kind&traits_info::Metadata)
	{
		describeMetadata(root, *classtrait);
	}
}


template<>
asAtom Class<Global>::getInstance(bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass)
{
	throwError<TypeError>(kConstructOfNonFunctionError);
	return asAtom::invalidAtom;
}

void lightspark::lookupAndLink(Class_base* c, const tiny_string& name, const tiny_string& interfaceNs)
{
	variable* var=NULL;
	Class_base* cur=c;
	//Find the origin
	while(cur)
	{
		var=cur->borrowedVariables.findObjVar(c->getSystemState()->getUniqueStringId(name),nsNameAndKind(c->getSystemState(),"",NAMESPACE),NO_CREATE_TRAIT,DECLARED_TRAIT);
		if(var)
			break;
		cur=cur->super.getPtr();
	}
	assert_and_throw(var);
	if(var->var.type != T_INVALID)
	{
		assert_and_throw(var->var.type==T_FUNCTION);
		ASATOM_INCREF(var->var);
		c->setDeclaredMethodAtomByQName(name,interfaceNs,var->var,NORMAL_METHOD,true);
	}
	if(var->getter.type != T_INVALID)
	{
		assert_and_throw(var->getter.type==T_FUNCTION);
		ASATOM_INCREF(var->getter);
		c->setDeclaredMethodAtomByQName(name,interfaceNs,var->getter,GETTER_METHOD,true);
	}
	if(var->setter.type != T_INVALID)
	{
		assert_and_throw(var->setter.type==T_FUNCTION);
		ASATOM_INCREF(var->setter);
		c->setDeclaredMethodAtomByQName(name,interfaceNs,var->setter,SETTER_METHOD,true);
	}
}

asAtom Class<ASObject>::getInstance(bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass)
{
	if (construct && args && argslen == 1 && this == Class<ASObject>::getClass(this->getSystemState()))
	{
		// Construction according to ECMA 15.2.2.1
		switch(args[0].type)
		{
			case T_BOOLEAN:
			case T_NUMBER:
			case T_INTEGER:
			case T_UINTEGER:
			case T_STRING:
			case T_FUNCTION:
			case T_OBJECT:
				return args[0];
			default:
				break;
		}
	}
	if(realClass==NULL)
		realClass=this;
	asAtom ret=asAtom::fromObject(new (realClass->memoryAccount) ASObject(realClass));
	if(construct)
		handleConstruction(ret,args,argslen,true);
	return ret;
}
Class<ASObject>* Class<ASObject>::getClass(SystemState* sys)
{
	uint32_t classId=ClassName<ASObject>::id;
	Class<ASObject>* ret=NULL;
	SystemState* s = sys == NULL ? getSys() : sys;
	Class_base** retAddr=&s->builtinClasses[classId];
	if(*retAddr==NULL)
	{
		//Create the class
		QName name(s->getUniqueStringId(ClassName<ASObject>::name),s->getUniqueStringId(ClassName<ASObject>::ns));
		ret=new (s->unaccountedMemory) Class<ASObject>(name, s->unaccountedMemory);
		ret->setSystemState(s);
		ret->incRef();
		*retAddr=ret;
		ret->prototype = _MNR(new_objectPrototype(sys));
		ASObject::sinit(ret);
		ret->initStandardProps();
	}
	else
		ret=static_cast<Class<ASObject>*>(*retAddr);
	
	return ret;
}
