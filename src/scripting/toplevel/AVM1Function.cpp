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


#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/flash/display/DisplayObject.h"
#include "scripting/flash/system/ApplicationDomain.h"
#include "scripting/avm1/scope.h"
#include "scripting/class.h"

using namespace std;
using namespace lightspark;

AVM1Function::AVM1Function(
	ASWorker* wrk
	, Class_base* c
	, DisplayObject* cl
	, AVM1context* ctx
	, std::vector<uint32_t>& p
	, std::vector<uint8_t>& a
	, AVM1Scope* _scope
	, std::vector<uint8_t> _registernumbers
	, bool _preloadParent
	, bool _preloadRoot
	, bool _suppressSuper
	, bool _preloadSuper
	, bool _suppressArguments
	, bool _preloadArguments
	, bool _suppressThis
	, bool _preloadThis
	, bool _preloadGlobal)
	:IFunction(wrk,c,SUBTYPE_AVM1FUNCTION)
	,clip(cl)
	,actionlist(a)
	,paramnames(p)
	,paramregisternumbers(_registernumbers)
	,preloadParent(_preloadParent)
	,preloadRoot(_preloadRoot)
	,suppressSuper(_suppressSuper)
	,preloadSuper(_preloadSuper)
	,suppressArguments(_suppressArguments)
	,preloadArguments(_preloadArguments)
	,suppressThis(_suppressThis)
	,preloadThis(_preloadThis)
	,preloadGlobal(_preloadGlobal)
{
	if (ctx)
		context.avm1strings.assign(ctx->avm1strings.begin(),ctx->avm1strings.end());
	clip->addOwnedObject(this);
	scope = _scope;
	if (scope)
	{
		scope->incRef();
		scope->addStoredMember();
	}
	context.keepLocals=true;
	superobj = asAtomHandler::invalidAtom;
}

AVM1Function::~AVM1Function()
{
}

asAtom AVM1Function::computeSuper()
{
	asAtom newsuper = superobj;
	if (asAtomHandler::isValid(newsuper))
		return newsuper;
	asAtom obj = asAtomHandler::fromObject(this);
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.isAttribute = false;
	m.name_s_id = BUILTIN_STRINGS::PROTOTYPE;
	newsuper = asAtomHandler::invalidAtom;
	asAtomHandler::getObject(obj)->getVariableByMultiname(newsuper,m,GET_VARIABLE_OPTION(GET_VARIABLE_OPTION::NO_INCREF|GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE),clip->getInstanceWorker());
	ASObject* pr =asAtomHandler::getObject(newsuper);
	if (pr)
	{
		m.name_s_id = BUILTIN_STRINGS::STRING_CONSTRUCTOR;
		newsuper = asAtomHandler::invalidAtom;
		pr->getVariableByMultiname(newsuper,m,GET_VARIABLE_OPTION(GET_VARIABLE_OPTION::NO_INCREF|GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE|GET_VARIABLE_OPTION::DONT_CALL_GETTER),clip->getInstanceWorker());
		if (asAtomHandler::isInvalid(newsuper) && pr->AVM1getConstructorFunction())
			newsuper = asAtomHandler::fromObject(pr->AVM1getConstructorFunction());
	}
	setSuper(newsuper);
	return newsuper;
}
asAtom* AVM1Function::computeThis(asAtom* obj)
{
	// if (suppressThis)
	// 	return nullptr;
	return obj;
}
asAtom AVM1Function::constructObject(asAtom *args, uint32_t num_args, AVM1Function* caller)
{
	asAtom ret = asAtomHandler::invalidAtom;
	ASObject* baseclass = asAtomHandler::getObject(computeSuper());
	if (baseclass && baseclass->is<Class_base>())
	{
		if (baseclass->as<Class_base>() == Class_object::getRef(getSystemState()).getPtr())
			ret = asAtomHandler::fromObject(new_asobject(getInstanceWorker()));
		else
			baseclass->as<Class_base>()->getInstance(getInstanceWorker(),ret,true,nullptr,0);
	}
	else if (baseclass && baseclass->is<IFunction>())
	{
		ASObject* constr = asAtomHandler::getObject(baseclass->as<IFunction>()->prototype);
		constr->getClass()->getInstance(getInstanceWorker(),ret,true,nullptr,0);
	}
	else if (asAtomHandler::is<Class_base>(this->prototype))
	{
		asAtomHandler::as<Class_base>(this->prototype)->getInstance(getInstanceWorker(),ret,true,nullptr,0);
	}
	else
	{
		ASObject* constr = asAtomHandler::getObject(this->prototype);
		constr->getClass()->getInstance(getInstanceWorker(),ret,true,nullptr,0);
	}
	ASObject* o = asAtomHandler::getObjectNoCheck(ret);
	o->AVM1setConstructorFunction(this);
	asAtom pr = this->getprop_prototypeAtom();
	o->setprop_prototype(pr,BUILTIN_STRINGS::STRING_PROTO);
	asAtom newsuper = computeSuper();
	if (getInstanceWorker()->AVM1getSwfVersion() < 7)
	{
		this->incRef();
		o->setVariableByQName("constructor","",this,DYNAMIC_TRAIT,false);
	}
	ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,scope,false,nullptr,&ret, args, num_args, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,caller,this,&newsuper,true);
	this->decRef();
	for (size_t i = 0; i < num_args; i++)
		ASATOM_DECREF(args[i]);
	return ret;
}

void AVM1Function::setupPrototype(asAtom value)
{
	setprop_prototype(value);
	ASObject* pr = asAtomHandler::getObject(value);
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.isAttribute = false;
	m.name_s_id = BUILTIN_STRINGS::STRING_CONSTRUCTOR;
	asAtom constr = asAtomHandler::invalidAtom;
	pr->getVariableByMultiname(constr,m,GET_VARIABLE_OPTION(GET_VARIABLE_OPTION::DONT_CALL_GETTER|GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE),clip->getInstanceWorker());
	if (asAtomHandler::isObject(constr))
	{
		asAtom proto = asAtomHandler::getObject(constr)->getprop_prototypeAtom();
		setprop_prototype(proto,BUILTIN_STRINGS::STRING_PROTO);
		ASATOM_DECREF(constr);
	}
	ASATOM_REMOVESTOREDMEMBER(superobj);
	superobj = asAtomHandler::invalidAtom;
}

void AVM1Function::setupDefineFunctionPrototype(bool hasname)
{
	ASObject* pr = new_asobject(getInstanceWorker());
	prototype = asAtomHandler::fromObject(pr);
	pr->addStoredMember();
	asAtom proto=asAtomHandler::fromObject(Class<ASObject>::getRef(getSystemState())->prototype->getObj());
	ASATOM_INCREF(proto);
	pr->setprop_prototype(proto,BUILTIN_STRINGS::STRING_PROTO);
	if (hasname)
	{
		incRef();
		pr->setVariableByQName("constructor","",this,DYNAMIC_TRAIT,false);
	}
	else
		pr->setVariableByQName("constructor","",Class<ASObject>::getRef(getSystemState()).getPtr(),DYNAMIC_TRAIT,false);
	setprop_prototype(prototype);
}

void AVM1Function::checkInternalException()
{
	if (getInstanceWorker()->AVM1callStack.back()->exceptionthrown)
		getInstanceWorker()->AVM1_cur_recursion_internal=AVM1_RECURSION_LIMIT_INTERN+1; // indicate stackoverflow on internal call
}

uint32_t AVM1Function::getSWFVersion() const
{
	return clip->loadedFrom->version;
}

void AVM1Function::finalize()
{
	clip=nullptr;
	if (scope)
	{
		scope->removeStoredMember();
		scope->decRef();
		scope=nullptr;
	}
	ASATOM_REMOVESTOREDMEMBER(superobj);
	superobj = asAtomHandler::invalidAtom;
	for (auto it = implementedinterfaces.begin(); it != implementedinterfaces.end(); it++)
	{
		ASATOM_REMOVESTOREDMEMBER(*it);
	}
	implementedinterfaces.clear();
	context.setScope(nullptr);
	context.setGlobalScope(nullptr);
	IFunction::finalize();
}

bool AVM1Function::destruct()
{
	clip=nullptr;

	if (scope)
	{
		scope->removeStoredMember();
		scope->decRef();
		scope=nullptr;
	}
	ASATOM_REMOVESTOREDMEMBER(superobj);
	superobj=asAtomHandler::invalidAtom;
	for (auto it = implementedinterfaces.begin(); it != implementedinterfaces.end(); it++)
	{
		ASATOM_REMOVESTOREDMEMBER(*it);
	}
	implementedinterfaces.clear();
	context.setScope(nullptr);
	context.setGlobalScope(nullptr);
	return IFunction::destruct();
}

void AVM1Function::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	IFunction::prepareShutdown();
	if (context.scope)
		context.scope->prepareShutdown();
	if (context.globalScope)
		context.globalScope->prepareShutdown();
	if (scope)
		scope->prepareShutdown();
	clip=nullptr;
	ASObject* su = asAtomHandler::getObject(superobj);
	if (su)
		su->prepareShutdown();
	ASATOM_REMOVESTOREDMEMBER(superobj);
	superobj=asAtomHandler::invalidAtom;
	for (auto it = implementedinterfaces.begin(); it != implementedinterfaces.end(); it++)
	{
		ASObject* iface = asAtomHandler::getObject(*it);
		if (iface)
			iface->prepareShutdown();
		ASATOM_REMOVESTOREDMEMBER(*it);
	}
	implementedinterfaces.clear();
}

bool AVM1Function::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = IFunction::countCylicMemberReferences(gcstate);
	if (scope)
		ret |= scope->countAllCyclicMemberReferences(gcstate);
	ASObject* su = asAtomHandler::getObject(superobj);
	if (su)
		ret = su->countAllCylicMemberReferences(gcstate) || ret;
	if (context.scope)
		ret |= context.scope->countAllCyclicMemberReferences(gcstate);
	return ret;
}
bool AVM1Function::implementsInterface(asAtom &iface)
{
	for (auto it = implementedinterfaces.begin(); it != implementedinterfaces.end(); it++)
	{
		if ((*it).uintval == iface.uintval)
			return true;
		if (asAtomHandler::is<AVM1Function>(*it) &&
			asAtomHandler::as<AVM1Function>(*it)->implementsInterface(iface))
			return true;
	}
	return false;
}

void AVM1Function::gcCounterReset()
{
	if (scope)
		scope->gcCounterReset();
	if (context.scope)
		context.scope->gcCounterReset();
	if (context.globalScope)
		context.globalScope->gcCounterReset();
	ASObject::gcCounterReset();
}
std::string AVM1Function::toDebugString() const
{
	string ret = IFunction::toDebugString();
	char buf[300];
	sprintf(buf," clip:%p pr:%s",clip,asAtomHandler::toDebugString(this->prototype).c_str());
	ret += buf;
	return ret;
}