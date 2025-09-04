/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2025  Ludger Krämer <dbluelle@onlinehome.de>

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
#include "compat.h"
#include "scripting/abc_interpreter_helper.h"
#include "scripting/abc_optimized_getscopeobject.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/system/ApplicationDomain.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/class.h"
#include "parsing/streams.h"

using namespace std;
using namespace lightspark;

namespace lightspark
{
bool preload_findprop(preloadstate& state,memorystream& code,uint32_t t,bool rewritecode,Class_base** lastlocalresulttype,std::vector<typestackentry>& typestack)
{
#ifdef ENABLE_OPTIMIZATION
	uint32_t scopepos=UINT32_MAX;
	asAtom o=asAtomHandler::invalidAtom;
	bool found = false;
	bool done=false;
	ASObject* resulttype = nullptr;
	multiname* name=state.mi->context->getMultiname(t,nullptr);
	if (!name && !rewritecode)
		return false;
	if (!name
		&& state.mi->context->constant_pool.multinames[t].runtimeargs==1
		&& state.operandlist.size() > 0
		&& state.operandlist.back().type != OPERANDTYPES::OP_LOCAL
		&& state.operandlist.back().type != OPERANDTYPES::OP_CACHED_SLOT)
	{
		asAtom o = *state.mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
		name = state.mi->context->getMultinameImpl(o,nullptr,t);
		if (name)
		{
			state.operandlist.back().removeArg(state);
			state.operandlist.pop_back();
		}
	}
	bool isborrowed = false;
	variable* v = nullptr;
	Class_base* cls = state.function->inClass;
	asAtom otmp = asAtomHandler::invalidAtom;
	if (name)
	{
		if (state.function->inClass && (state.scopelist.empty() || !state.scopelist.back().considerDynamic)) // class method
		{
			// property may be a slot variable, so check the class instance first
			v = getTempVariableFromClass(cls,otmp,name,state.worker);
			if (v)
				isborrowed=true;
			else
			{
				// property not a slot variable, so check the class the function belongs to
				Class_base* c = cls;
				do
				{
					v = c->findVariableByMultiname(*name,c,nullptr,&isborrowed,false,state.worker);
					if (v)
						break;
					if (!c->isSealed)
						break;
					c = c->super.getPtr();
				}
				while (c);
				if (v)
					cls=c;
			}
			if (v)
			{
				found =true;
				if ((!state.function->isStatic || state.function == state.function->inClass->getConstructor()) && (isborrowed || v->kind == INSTANCE_TRAIT))
				{
					if (!rewritecode)
					{
						ASATOM_DECREF(otmp);
						return true;
					}
					state.preloadedcode.push_back((uint32_t)0xd0); // convert to getlocal_0
					state.refreshOldNewPosition(code);
					state.operandlist.push_back(operands(OP_LOCAL,state.function->inClass, 0,1,state.preloadedcode.size()-1));
					typestack.push_back(typestackentry(state.function->inClass,false));
					ASATOM_DECREF(otmp);
					return true;
				}
				if (state.function->isStatic && !isborrowed && (v->kind & DECLARED_TRAIT))
				{
					// if property is a static variable of the class this function belongs to we have to check the scopes first
					found = false;
				}
			}
		}
		if (!found && (!state.function->inClass || state.function->inClass->isSealed))
		{
			uint32_t spos = state.scopelist.size();
			auto it=state.scopelist.rbegin();
			while(it!=state.scopelist.rend())
			{
				spos--;
				if (it->considerDynamic)
				{
					found = true;
					break;
				}
				if (!asAtomHandler::isObject(it->object))
				{
					break;
				}
				ASObject* obj = asAtomHandler::getObjectNoCheck(it->object);
				if (obj->hasPropertyByMultiname(*name, false, true,state.worker))
				{
					found = true;
					done=true;
					if (!rewritecode)
					{
						ASATOM_DECREF(otmp);
						return true;
					}
					if (state.function->isStatic && obj==state.function->inClass)
					{
						// property is a static variable of the class this function belongs to
						o=asAtomHandler::fromObjectNoPrimitive(state.function->inClass);
						addCachedConstant(state,state.mi, o,code);
						typestack.push_back(typestackentry(state.function->inClass,true));
						ASATOM_DECREF(otmp);
						return true;
					}
					else
						scopepos=spos;
					break;
				}
				++it;
			}
		}
		if(!found && !state.function->func_scope.isNull()) // check scope stack
		{
			uint32_t spos = state.function->func_scope->scope.size();
			auto it=state.function->func_scope->scope.rbegin();
			while(it!=state.function->func_scope->scope.rend())
			{
				spos--;
				if (it->considerDynamic)
				{
					found = true;
					break;
				}
				if (asAtomHandler::is<Class_base>(it->object) && !asAtomHandler::as<Class_base>(it->object)->isSealed)
				{
					break;
				}
				ASObject* obj = asAtomHandler::toObject(it->object,state.worker);
				if (obj->hasPropertyByMultiname(*name, false, true,state.worker))
				{
					found = true;
					done=true;
					if (!rewritecode)
					{
						ASATOM_DECREF(otmp);
						return true;
					}
					if (obj->is<Global>())
					{
						o=asAtomHandler::fromObjectNoPrimitive(obj);
						addCachedConstant(state,state.mi, o,code);
						typestack.push_back(typestackentry(obj,false));
						ASATOM_DECREF(otmp);
						return true;
					}
					state.preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_GETFUNCSCOPEOBJECT);
					state.refreshOldNewPosition(code);
					if (checkForLocalResult(state,code,0,obj->is<Class_base>() ? nullptr : obj->getClass()))
					{
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func=abc_getfuncscopeobject_localresult;
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=spos;
						resulttype = obj;
					}
					else
					{
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=spos;
						clearOperands(state,true,lastlocalresulttype);
					}
					ASATOM_DECREF(otmp);
					typestack.push_back(typestackentry(resulttype,false));
					return true;
				}
				++it;
			}
		}
		if (!found	&& (state.scopelist.empty() || !state.scopelist.back().considerDynamic)) // class method
		{
			ASObject* target=nullptr;
			if (name->cachedType)
				target = name->cachedType->getGlobalScope();
			if (!target || (target->is<Global>() && target->as<Global>()->isAVM1()))
				state.mi->context->applicationDomain->findTargetByMultiname(*name, target,state.worker);
			if (target)
			{
				found = true;
				if (target->is<Global>() && (state.function->isStatic || state.function->isFromNewFunction()))
				{
					variable* vglobal = target->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,state.worker);
					found = (vglobal->kind & TRAIT_KIND::DECLARED_TRAIT)!=0;
				}
				if (found)
				{
					if (!rewritecode)
					{
						ASATOM_DECREF(otmp);
						return true;
					}
					o=asAtomHandler::fromObjectNoPrimitive(target);
					addCachedConstant(state,state.mi, o,code);
					typestack.push_back(typestackentry(target,false));
					ASATOM_DECREF(otmp);
					return true;
				}
			}
		}
	}
	if (scopepos!= UINT32_MAX)
	{
		if (!rewritecode)
		{
			ASATOM_DECREF(otmp);
			return true;
		}
		state.preloadedcode.push_back((uint32_t)0x65); //getscopeobject
		state.refreshOldNewPosition(code);
		if (checkForLocalResult(state,code,0,nullptr))
		{
			state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func=abc_getscopeobject_localresult;
			state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=scopepos;
		}
		else
		{
			state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint=scopepos;
			clearOperands(state,true,lastlocalresulttype);
		}
		done=true;
		typestack.push_back(typestackentry(nullptr,false));
		ASATOM_DECREF(otmp);
		return true;
	}
	else if(!done)
	{
		if (v && !state.function->isFromNewFunction()
			&& (state.function != cls->getConstructor() || !isborrowed)
			&& (!state.function->isStatic || !isborrowed ))
		{
			if (!rewritecode)
			{
				ASATOM_DECREF(otmp);
				return true;
			}
			asAtom value = asAtomHandler::fromObjectNoPrimitive(cls);
			addCachedConstant(state,state.mi, value,code);
			typestack.push_back(typestackentry(cls,isborrowed));
			ASATOM_DECREF(otmp);
			return true;
		}
	}
	ASATOM_DECREF(otmp);
#endif
	return false;
}

}
