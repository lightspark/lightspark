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

ASObject* lightspark::new_asobject(ASWorker* wrk)
{
	return Class<ASObject>::getInstanceS(wrk);
}

Prototype* lightspark::new_objectPrototype(ASWorker* wrk)
{
	//Create a Prototype object, the class should be ASObject
	Class_base* c=Class<ASObject>::getClass(wrk->getSystemState());
	Prototype* p = new (c->memoryAccount) ObjectPrototype(wrk,c);
	p->getObj()->setRefConstant();
	return p;
}

Prototype* lightspark::new_functionPrototype(ASWorker* wrk,Class_base* functionClass, _NR<Prototype> p)
{
	//Create a Prototype object, the class should be ASObject
	return new (functionClass->memoryAccount) FunctionPrototype(wrk,functionClass, p);
}

Function_object* lightspark::new_functionObject(_NR<ASObject> p)
{
	Class_base* c=Class<ASObject>::getClass(p->getSystemState());
	return new (c->memoryAccount) Function_object(p->getInstanceWorker(), c, p);
}

ObjectConstructor* lightspark::new_objectConstructor(Class_base* cls,uint32_t length)
{
	return new (cls->memoryAccount) ObjectConstructor(cls->getInstanceWorker(), cls, length);
}

Activation_object* lightspark::new_activationObject(ASWorker* wrk)
{
	Class_base* c=Class<ASObject>::getClass(wrk->getSystemState());
	return new (c->memoryAccount) Activation_object(wrk,c);
}


Class_inherit::Class_inherit(const QName& name, MemoryAccount* m, const traits_info *_classtrait, Global *_global):Class_base(name, UINT32_MAX,m),tag(nullptr),bindedToRoot(false),bindingchecked(false),inScriptInit(false),classtrait(_classtrait)
{
	this->global=_global;
	this->incRef(); //create on reference for the classes map
	isReusable = true;
	subtype = SUBTYPE_INHERIT;
}

void Class_inherit::finalize()
{
	instancefactory.reset();
	tag=nullptr;
	auto it = class_scope.begin();
	while (it != class_scope.end())
	{
		ASATOM_DECREF((*it).object);
		it++;
	}
	class_scope.clear();
	Class_base::finalize();
	
}

void Class_inherit::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	Class_base::prepareShutdown();
	if (instancefactory)
		instancefactory->prepareShutdown();
	auto it = class_scope.begin();
	while (it != class_scope.end())
	{
		ASObject* o = asAtomHandler::getObject((*it).object);
		if (o)
			o->prepareShutdown();
		it++;
	}
}

void Class_inherit::getInstance(ASWorker* worker, asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass)
{
	checkScriptInit();
	//We override the classdef
	if(realClass==nullptr)
		realClass=this;
	if (this->needsBindingCheck()) // it seems possible that an instance of a class is constructed before the binding of the class is available, so we have to check for a binding here
	{
		worker->rootClip->bindClass(this->class_name,this);
		if (worker->rootClip->hasFinishedLoading())
			this->bindingchecked=true;
	}
	if(tag)
	{
		ret=asAtomHandler::fromObject(tag->instance(realClass));
		if (instancefactory.isNull() && this->bindingchecked)
		{
			instancefactory = _MR(tag->instance(realClass));
			instancefactory->setRefConstant();
		}
	}
	else
	{
		assert_and_throw(super);
		//Our super should not construct, we are going to do it ourselves
		super->getInstance(worker,ret,false,nullptr,0,realClass);
		if (instancefactory.isNull() && this->bindingchecked)
		{
			asAtom instance=asAtomHandler::invalidAtom;
			super->getInstance(worker,instance,false,nullptr,0,realClass);
			instancefactory = _MR(asAtomHandler::getObject(instance));
			instancefactory->setRefConstant();
		}
	}
	if(construct)
		handleConstruction(ret,args,argslen,true);
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
	LOG_CALL("Building instance traits");

	context->buildInstanceTraits(o,class_index);
}
void Class_inherit::setupDeclaredTraits(ASObject *target, bool checkclone)
{
	if (!target->traitsInitialized)
	{
#ifndef NDEBUG
		assert(!target->initialized);
#endif
		bool cloneable = checkclone && !instancefactory.isNull();
		if (cloneable)
		{
			if (!instancefactory->isInitialized())
			{
				// fill instancefactory
				
				//HACK: suppress implementation handling of variables just now
				bool bak=instancefactory->implEnable;
				instancefactory->implEnable=false;
				recursiveBuild(instancefactory.getPtr());
				
				//And restore it
				instancefactory->implEnable=bak;

				instancefactory->traitsInitialized = true;
				instancefactory->setRefConstant();
				cloneable = instancefactory->cloneInstance(target);
			}
			else
				cloneable = instancefactory->cloneInstance(target);
		}
		if (!cloneable)
		{
			//HACK: suppress implementation handling of variables just now
			bool bak=target->implEnable;
			target->implEnable=false;
			recursiveBuild(target);
			
			//And restore it
			target->implEnable=bak;
		}
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

bool Class_inherit::hasoverriddenmethod(multiname *name) const
{
	// TODO we currently only check for names, not for namespaces as there are some issues regarding protected namespaces
	// so we may get some false positives here...
	if (class_index == -1)
		return true;
	const instance_info& c = this->context->instances[this->class_index];
	if (c.overriddenmethods && c.overriddenmethods->find(name->name_s_id) != c.overriddenmethods->end())
		return true;
	return Class_base::hasoverriddenmethod(name);
}

GET_VARIABLE_RESULT Class_inherit::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt,ASWorker* wrk)
{
	checkScriptInit();
	return Class_base::getVariableByMultiname(ret,name,opt,wrk);
}

variable* Class_inherit::findVariableByMultiname(const multiname& name, Class_base* cls, uint32_t* nsRealID, bool* isborrowed, bool considerdynamic, ASWorker* wrk)
{
	checkScriptInit();
	return Class_base::findVariableByMultiname(name, cls, nsRealID, isborrowed, considerdynamic, wrk);
}
string Class_inherit::toDebugString() const
{
	std::string res = Class_base::toDebugString();
	res += "tag=";
	char buf[100];
	sprintf(buf,"%u",tag ? tag->getId():UINT32_MAX);
	res += buf;
	return res;
}
template<>
void Class<Global>::getInstance(ASWorker* worker,asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass)
{
	createError<TypeError>(worker,kConstructOfNonFunctionError);
}

void lightspark::lookupAndLink(Class_base* c, const tiny_string& name, const tiny_string& interfaceNs)
{
	variable* var=nullptr;
	Class_base* cur=c;
	//Find the origin
	while(cur)
	{
		var=cur->borrowedVariables.findObjVar(c->getSystemState()->getUniqueStringId(name),nsNameAndKind(),NO_CREATE_TRAIT,DECLARED_TRAIT);
		if(var)
			break;
		cur=cur->super.getPtr();
	}
	assert_and_throw(var);
	if(asAtomHandler::isValid(var->var))
	{
		assert_and_throw(asAtomHandler::isFunction(var->var));
		ASATOM_INCREF(var->var);
		c->setDeclaredMethodAtomByQName(name,interfaceNs,var->var,NORMAL_METHOD,true);
	}
	if(asAtomHandler::isValid(var->getter))
	{
		assert_and_throw(asAtomHandler::isFunction(var->getter));
		ASATOM_INCREF(var->getter);
		c->setDeclaredMethodAtomByQName(name,interfaceNs,var->getter,GETTER_METHOD,true);
	}
	if(asAtomHandler::isValid(var->setter))
	{
		assert_and_throw(asAtomHandler::isFunction(var->setter));
		ASATOM_INCREF(var->setter);
		c->setDeclaredMethodAtomByQName(name,interfaceNs,var->setter,SETTER_METHOD,true);
	}
}

void lightspark::lookupAndLink(Class_base* c, uint32_t nameID, uint32_t interfaceNsID)
{
	variable* var=nullptr;
	Class_base* cur=c;
	//Find the origin
	while(cur)
	{
		var=cur->borrowedVariables.findObjVar(nameID,nsNameAndKind(),NO_CREATE_TRAIT,DECLARED_TRAIT);
		if(var)
			break;
		cur=cur->super.getPtr();
	}
	assert_and_throw(var);
	if(asAtomHandler::isValid(var->var))
	{
		assert_and_throw(asAtomHandler::isFunction(var->var));
		ASATOM_INCREF(var->var);
		c->setDeclaredMethodAtomByQName(nameID,nsNameAndKind(c->getSystemState(),interfaceNsID, NAMESPACE),var->var,NORMAL_METHOD,true);
	}
	if(asAtomHandler::isValid(var->getter))
	{
		assert_and_throw(asAtomHandler::isFunction(var->getter));
		ASATOM_INCREF(var->getter);
		c->setDeclaredMethodAtomByQName(nameID,nsNameAndKind(c->getSystemState(),interfaceNsID, NAMESPACE),var->getter,GETTER_METHOD,true);
	}
	if(asAtomHandler::isValid(var->setter))
	{
		assert_and_throw(asAtomHandler::isFunction(var->setter));
		ASATOM_INCREF(var->setter);
		c->setDeclaredMethodAtomByQName(nameID,nsNameAndKind(c->getSystemState(),interfaceNsID, NAMESPACE),var->setter,SETTER_METHOD,true);
	}
}

void Class<ASObject>::getInstance(ASWorker* worker, asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass)
{
	if (construct && args && argslen == 1 && this == Class<ASObject>::getClass(this->getSystemState()))
	{
		// Construction according to ECMA 15.2.2.1
		switch(asAtomHandler::getObjectType(args[0]))
		{
			case T_BOOLEAN:
			case T_NUMBER:
			case T_INTEGER:
			case T_UINTEGER:
			case T_STRING:
			case T_FUNCTION:
			case T_OBJECT:
			case T_CLASS:
			case T_ARRAY:
				ret = args[0];
				return;
			default:
				break;
		}
	}
	if(realClass==nullptr)
		realClass=this;
	ret=asAtomHandler::fromObjectNoPrimitive(new (realClass->memoryAccount) ASObject(worker,realClass));
	if(construct)
		handleConstruction(ret,args,argslen,true);
}
Class<ASObject>* Class<ASObject>::getClass(SystemState* sys)
{
	uint32_t classId=ClassName<ASObject>::id;
	Class<ASObject>* ret=nullptr;
	SystemState* s = sys == nullptr ? getSys() : sys;
	Class_base** retAddr=&s->builtinClasses[classId];
	if(*retAddr==nullptr)
	{
		//Create the class
		QName name(s->getUniqueStringId(ClassName<ASObject>::name),s->getUniqueStringId(ClassName<ASObject>::ns));
		MemoryAccount* m = s->allocateMemoryAccount(ClassName<ASObject>::name);
		ret=new (m) Class<ASObject>(name, ClassName<ASObject>::id, m);
		ret->setWorker(s->worker);
		ret->setSystemState(s);
		ret->incRef();
		*retAddr=ret;
		ret->prototype = _MNR(new_objectPrototype(sys->worker));
		ASObject::sinit(ret);
		ret->initStandardProps();
	}
	else
		ret=static_cast<Class<ASObject>*>(*retAddr);
	
	return ret;
}
