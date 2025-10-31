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

using namespace std;
using namespace lightspark;

AVM1Function::AVM1Function(ASWorker* wrk, Class_base* c, DisplayObject* cl, AVM1context* ctx, std::vector<uint32_t>& p, std::vector<uint8_t>& a, AVM1Scope* _scope, std::vector<uint8_t> _registernumbers, bool _preloadParent, bool _preloadRoot, bool _suppressSuper, bool _preloadSuper, bool _suppressArguments, bool _preloadArguments, bool _suppressThis, bool _preloadThis, bool _preloadGlobal)
	:IFunction(wrk,c,SUBTYPE_AVM1FUNCTION),clip(cl),avm1Class(nullptr),actionlist(a),paramnames(p), paramregisternumbers(_registernumbers),
	  preloadParent(_preloadParent),preloadRoot(_preloadRoot),suppressSuper(_suppressSuper),preloadSuper(_preloadSuper),suppressArguments(_suppressArguments),preloadArguments(_preloadArguments),suppressThis(_suppressThis), preloadThis(_preloadThis), preloadGlobal(_preloadGlobal)
{
	if (ctx)
		context.avm1strings.assign(ctx->avm1strings.begin(),ctx->avm1strings.end());
	clip->incRef();
	clip->addStoredMember();
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
	if (clip)
		clip->removeStoredMember();
}

asAtom AVM1Function::computeSuper(asAtom obj)
{
	asAtom newsuper = superobj;
	if (asAtomHandler::isValid(newsuper))
		return newsuper;
	ASObject* pr =getprop_prototype();
	if (!pr)
		pr= asAtomHandler::getObject(this->prototype);
	if (pr)
	{
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id = BUILTIN_STRINGS::STRING_CONSTRUCTOR;
		pr->getVariableByMultiname(newsuper,m,GET_VARIABLE_OPTION::NO_INCREF,clip->getInstanceWorker());
		if (asAtomHandler::getObject(newsuper) == this)
			newsuper = asAtomHandler::fromObject(asAtomHandler::getClass(obj,clip->getSystemState()));
		if (asAtomHandler::isInvalid(newsuper))
			LOG(LOG_ERROR,"AVM1 no super found:"<<pr->toDebugString());
	}
	setSuper(newsuper);
	return newsuper;
}

void AVM1Function::checkInternalException()
{
	if (getInstanceWorker()->AVM1callStack.back()->exceptionthrown)
		getInstanceWorker()->AVM1_cur_recursion_internal=AVM1_RECURSION_LIMIT_INTERN+1; // indicate stackoverflow on internal call
}

uint32_t AVM1Function::getSWFVersion()
{
	return clip->loadedFrom->version;
}

void AVM1Function::finalize()
{
	if (clip)
		clip->removeStoredMember();
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
	if (clip)
		clip->removeStoredMember();
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
	{
		context.scope->prepareShutdown();
		context.scope->decRef();
		context.scope=nullptr;
	}
	if (context.globalScope)
	{
		context.globalScope->prepareShutdown();
		context.globalScope->decRef();
		context.globalScope=nullptr;
	}
	if (scope)
	{
		scope->prepareShutdown();
		scope->decRef();
		scope=nullptr;
	}
	if (clip)
	{
		clip->prepareShutdown();
		clip->removeStoredMember();
	}
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
	if (clip)
		ret = clip->countAllCylicMemberReferences(gcstate) || ret;
	if (scope)
		ret |= scope->countAllCyclicMemberReferences(gcstate);
	ASObject* su = asAtomHandler::getObject(superobj);
	if (su)
		ret = su->countAllCylicMemberReferences(gcstate) || ret;
	if (context.scope)
		ret |= context.scope->countAllCyclicMemberReferences(gcstate);
	if (context.globalScope)
		ret |= context.globalScope->countAllCyclicMemberReferences(gcstate);
	return ret;
}
bool AVM1Function::implementsInterface(asAtom &iface)
{
	for (auto it = implementedinterfaces.begin(); it != implementedinterfaces.end(); it++)
	{
		if ((*it).uintval == iface.uintval)
			return true;
	}
	ASObject* pr = asAtomHandler::getObject(this->prototype);
	if (pr)
		pr = pr->getprop_prototype();
	if (pr)
	{
		asAtom o = asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=BUILTIN_STRINGS::STRING_CONSTRUCTOR;
		pr->getVariableByMultiname(o,m,GET_VARIABLE_OPTION(SKIP_IMPL|NO_INCREF),getInstanceWorker());
		if (asAtomHandler::is<AVM1Function>(o))
			return asAtomHandler::as<AVM1Function>(o)->implementsInterface(iface);
	}
	return false;
}
