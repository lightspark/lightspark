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


using namespace std;
using namespace lightspark;

AVM1Function::AVM1Function(ASWorker* wrk, Class_base* c, DisplayObject* cl, Activation_object* act, AVM1context* ctx, std::vector<uint32_t>& p, std::vector<uint8_t>& a, std::vector<uint8_t> _registernumbers, bool _preloadParent, bool _preloadRoot, bool _suppressSuper, bool _preloadSuper, bool _suppressArguments, bool _preloadArguments, bool _suppressThis, bool _preloadThis, bool _preloadGlobal)
	:IFunction(wrk,c,SUBTYPE_AVM1FUNCTION),clip(cl),activationobject(act),actionlist(a),paramnames(p), paramregisternumbers(_registernumbers),
	  preloadParent(_preloadParent),preloadRoot(_preloadRoot),suppressSuper(_suppressSuper),preloadSuper(_preloadSuper),suppressArguments(_suppressArguments),preloadArguments(_preloadArguments),suppressThis(_suppressThis), preloadThis(_preloadThis), preloadGlobal(_preloadGlobal)
{
	if (ctx)
		context.avm1strings.assign(ctx->avm1strings.begin(),ctx->avm1strings.end());
	clip->incRef();
	clip->addStoredMember();
	if (act)
		act->addStoredMember();
	context.keepLocals=true;
	superobj = asAtomHandler::invalidAtom;
}

AVM1Function::~AVM1Function()
{
	if (activationobject)
		activationobject->removeStoredMember();
	if (clip)
		clip->removeStoredMember();
	for (auto it = scopevariables.begin(); it != scopevariables.end(); it++)
	{
		ASATOM_REMOVESTOREDMEMBER(it->second);
	}
	scopevariables.clear();
}

asAtom AVM1Function::computeSuper()
{
	asAtom newsuper = superobj;
	if (asAtomHandler::isValid(newsuper))
		return newsuper;
	ASObject* pr =getprop_prototype();
	if (!pr)
		pr= this->prototype.getPtr();
	if (pr)
	{
		pr->incRef();
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id = BUILTIN_STRINGS::STRING_CONSTRUCTOR;
		pr->getVariableByMultiname(newsuper,m,GET_VARIABLE_OPTION::NONE,clip->getInstanceWorker());
		if (asAtomHandler::isInvalid(newsuper))
			LOG(LOG_ERROR,"no super found:"<<pr->toDebugString());
	}
	return newsuper;
}

void AVM1Function::finalize()
{
	if (activationobject)
		activationobject->removeStoredMember();
	activationobject=nullptr;
	if (clip)
		clip->removeStoredMember();
	clip=nullptr;
	for (auto it = scopevariables.begin(); it != scopevariables.end(); it++)
	{
		ASATOM_REMOVESTOREDMEMBER(it->second);
	}
	scopevariables.clear();
	ASATOM_REMOVESTOREDMEMBER(superobj);
	IFunction::finalize();
}

bool AVM1Function::destruct()
{
	if (activationobject)
		activationobject->removeStoredMember();
	activationobject=nullptr;
	if (clip)
		clip->removeStoredMember();
	clip=nullptr;
	for (auto it = scopevariables.begin(); it != scopevariables.end(); it++)
	{
		ASATOM_REMOVESTOREDMEMBER(it->second);
	}
	scopevariables.clear();
	ASATOM_REMOVESTOREDMEMBER(superobj);
	superobj=asAtomHandler::invalidAtom;
	return IFunction::destruct();
}

void AVM1Function::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	IFunction::prepareShutdown();
	if (activationobject)
		activationobject->prepareShutdown();
	if (clip)
		clip->prepareShutdown();
	for (auto it = scopevariables.begin(); it != scopevariables.end(); it++)
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			o->prepareShutdown();
	}
	ASObject* su = asAtomHandler::getObject(superobj);
	if (su)
		su->prepareShutdown();
}

bool AVM1Function::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = IFunction::countCylicMemberReferences(gcstate);
	if (activationobject)
		ret = activationobject->countAllCylicMemberReferences(gcstate) || ret;
	if (clip)
		ret = clip->countAllCylicMemberReferences(gcstate) || ret;
	for (auto it = scopevariables.begin(); it != scopevariables.end(); it++)
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			ret = o->countAllCylicMemberReferences(gcstate) || ret;
	}
	ASObject* su = asAtomHandler::getObject(superobj);
	if (su)
		ret = su->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void AVM1Function::filllocals(std::map<uint32_t, asAtom>& locals)
{
	for (auto it = scopevariables.begin(); it != scopevariables.end(); it++)
	{
		ASATOM_INCREF(it->second);
		locals[it->first]=it->second;
	}
}

void AVM1Function::setscopevariables(std::map<uint32_t, asAtom>& locals)
{
	for (auto it = locals.begin(); it != locals.end(); it++)
	{
		ASATOM_REMOVESTOREDMEMBER(scopevariables [it->first]);
		ASATOM_ADDSTOREDMEMBER(it->second);
		scopevariables [it->first]=it->second;
	}
}
