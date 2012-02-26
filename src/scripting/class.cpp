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

#include "scripting/abc.h"
#include "class.h"
#include "parsing/tags.h"

using namespace lightspark;

ASObject* lightspark::new_asobject()
{
	return Class<ASObject>::getInstanceS();
}

void Class_inherit::finalize()
{
	Class_base::finalize();
	class_scope.clear();
}

ASObject* Class_inherit::getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
{
	ASObject* ret=NULL;
	assert_and_throw(!bindedToRoot);
	if(tag)
	{
		ret=tag->instance();
		assert_and_throw(ret);
	}
	else
	{
		assert_and_throw(super);
		//Our super should not construct, we are going to do it ourselves
		ret=super->getInstance(false,NULL,0);
	}
	//We override the classdef
	ret->setClass(this);
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
Global* Class<Global>::getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
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
		var=cur->Variables.findObjVar(name,nsNameAndKind("",NAMESPACE),NO_CREATE_TRAIT,BORROWED_TRAIT);
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
