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
void preload_getlex(preloadstate& state, std::vector<typestackentry>& typestack, memorystream& code, int opcode, Class_base** lastlocalresulttype, std::list<scope_entry>& scopelist,uint32_t simple_getter_opcode_pos)
{
	Class_base* resulttype = nullptr;
	int32_t p = code.tellg();
	state.checkClearOperands(p,lastlocalresulttype);
	uint32_t t =code.readu30();
	multiname* name=state.mi->context->getMultiname(t,nullptr);
	if (!name || !name->isStatic)
	{
		createError<VerifyError>(state.worker,kIllegalOpMultinameError,"getlex","multiname not static");
		return;
	}
#ifdef ENABLE_OPTIMIZATION
	// check for entry on exception scope
	for (auto it = scopelist.rbegin(); it != scopelist.rend(); it++)
	{
		if (it->considerDynamic)
			break;
		ASObject* obj = asAtomHandler::getObject(it->object);
		if (obj && obj->is<Catchscope_object>())
		{
			variable* v = obj->findVariableByMultiname(*name,nullptr,nullptr,nullptr,true,state.worker);
			if (v && v->slotid==1) // slotid 1 always contains the thrown exception object in the Catchscope_object
			{
				resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
				// set exception object via getlocal
				state.preloadedcode.push_back((uint32_t)0x62);//getlocal
				uint32_t value = state.mi->body->getReturnValuePos()+1; // first localresult is reserved for exception catch
				state.preloadedcode.back().pcode.arg3_uint=value;
				state.operandlist.push_back(operands(OP_LOCAL,resulttype,value,1,state.preloadedcode.size()-1));
				typestack.push_back(typestackentry(resulttype,false));
				return;
			}
		}
	}
	if (state.function->inClass && (scopelist.begin()==scopelist.end() || !scopelist.back().considerDynamic)) // class method
	{
		if (state.function->isStatic && state.function != state.function->inClass->getConstructor())
		{
			variable* v = state.function->inClass->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,state.worker);
			if (v)
			{
				if (v->kind == TRAIT_KIND::CONSTANT_TRAIT)
				{
					asAtom a = v->getVar(state.worker);
					addCachedConstant(state,state.mi, a,code);
					if (v->isResolved && dynamic_cast<const Class_base*>(v->type))
						resulttype = (Class_base*)v->type;
					typestack.push_back(typestackentry(resulttype,false));
					return;
				}
				else if (v->kind==DECLARED_TRAIT)
				{
					// property is static variable from class
					resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
					state.refreshOldNewPosition(code);
					asAtom clAtom = asAtomHandler::fromObjectNoPrimitive(state.function->inClass);
					addCachedConstant(state,state.mi,clAtom,code);
					if (v->slotid)
					{
						// convert to getslot on class
						setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,false,ABC_OP_OPTIMZED_GETSLOT_SETSLOT);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v->slotid-1;
					}
					else
					{
						// convert to getprop on class
						setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
					}
					typestack.push_back(typestackentry(resulttype,false));
					return;
				}
			}
		}
		else
		{
			bool isborrowed = false;
			variable* v = nullptr;
			Class_base* cls = state.function->inClass;
			asAtom otmp = asAtomHandler::invalidAtom;
			if (!cls->hasoverriddenmethod(name))
			{
				// property may be a slot variable, so check the class instance first
				v = getTempVariableFromClass(cls->as<Class_base>(),otmp,name,state.worker);
				if (v)
					isborrowed=true;
			}
			if (!v)
			{
				do
				{
					v = cls->findVariableByMultiname(*name,cls,nullptr,&isborrowed,false,state.worker);
					if (!v)
						cls = cls->super.getPtr();
				}
				while (!v && cls && cls->isSealed);
			}
			if (v)
			{
				if ((isborrowed || v->kind == INSTANCE_TRAIT) && asAtomHandler::isValid(v->getter))
				{
					// property is getter from class
					resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
					state.preloadedcode.push_back((uint32_t)0xd0);
					state.refreshOldNewPosition(code);
					state.operandlist.push_back(operands(OP_LOCAL,state.function->inClass, 0,1,state.preloadedcode.size()-1));
					if (state.function->inClass->isInterfaceMethod(*name) ||
						(state.function->inClass->is<Class_inherit>() && state.function->inClass->as<Class_inherit>()->hasoverriddenmethod(name)))
					{
						// convert to getprop on local[0]
						setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
					}
					else
					{
						// convert to callprop on local[0] (this) with 0 args
						setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS,0x46,code,true, false,resulttype,p,true,false,false,false,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = asAtomHandler::getObject(v->getter);
					}
					typestack.push_back(typestackentry(resulttype,false));
					ASATOM_DECREF(otmp);
					return;
				}
				else if ((isborrowed || v->kind == INSTANCE_TRAIT) && v->isValidVar())
				{
					// property is variable from class
					resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
					state.preloadedcode.push_back((uint32_t)0xd0);
					state.refreshOldNewPosition(code);
					state.operandlist.push_back(operands(OP_LOCAL,state.function->inClass, 0,1,state.preloadedcode.size()-1));
					if (state.function->inClass->is<Class_inherit>()
						&& !state.function->inClass->as<Class_inherit>()->hasoverriddenmethod(name)
						&& v->slotid)
					{
						asAtom otmp = asAtomHandler::invalidAtom;
						variable* v1 = getTempVariableFromClass(cls->as<Class_base>(),otmp,name,state.worker);
						if (!asAtomHandler::isPrimitive(otmp) && v1 && v1->slotid)
						{
							// convert to getslot on local[0]
							setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,false,ABC_OP_OPTIMZED_GETSLOT_SETSLOT);
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v1->slotid-1;
						}
						else
						{
							// convert to getprop on local[0]
							setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
						}
						ASATOM_DECREF(otmp);
					}
					else
					{
						// convert to getprop on local[0]
						setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
					}
					typestack.push_back(typestackentry(resulttype,false));
					ASATOM_DECREF(otmp);
					return;
				}
				else if (!isborrowed && v->kind==DECLARED_TRAIT)
				{
					// property is static variable from class
					resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
					state.refreshOldNewPosition(code);
					asAtom clAtom = asAtomHandler::fromObjectNoPrimitive(cls);
					addCachedConstant(state,state.mi,clAtom,code);
					if (v->slotid)
					{
						// convert to getslot on class
						setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,false,ABC_OP_OPTIMZED_GETSLOT_SETSLOT);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v->slotid-1;
					}
					else
					{
						// convert to getprop on class
						setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
					}
					typestack.push_back(typestackentry(resulttype,false));
					ASATOM_DECREF(otmp);
					return;
				}
				else if (v->kind == TRAIT_KIND::CONSTANT_TRAIT && !v->isNullVar()) // class may not be constructed yet, so the result is null and we do not cache
				{
					asAtom a=v->getVar(state.worker);
					addCachedConstant(state,state.mi, a,code);
					if (v->isResolved && dynamic_cast<const Class_base*>(v->type))
						resulttype = (Class_base*)v->type;
					typestack.push_back(typestackentry(resulttype,false));
					ASATOM_DECREF(otmp)
					return;
				}
			}
			ASATOM_DECREF(otmp);
		}
		if ((simple_getter_opcode_pos != UINT32_MAX) // function is simple getter
			&& state.function->inClass->isFinal // TODO also enable optimization for classes where it is guarranteed that the method is not overridden in derived classes
			&& state.function->inClass->getInterfaces().empty()) // class doesn't implement any interfaces
		{
			variable* v = state.function->inClass->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,state.worker);
			if (v && v->kind == TRAIT_KIND::INSTANCE_TRAIT)
				state.function->setSimpleGetterSetter(name);
		}
		if (getLexFindClass(state,name,true,typestack,code))
			return;
	}
	else if (!state.function->isStatic && (scopelist.begin()==scopelist.end() || !scopelist.back().considerDynamic))
	{
		if (state.function->isFromNewFunction())
		{
			bool found=false;
			bool considerDynamic=false;
			if(!state.function->func_scope.isNull()) // check scope stack
			{
				int32_t num=0;
				auto it=state.function->func_scope->scope.rbegin();
				while(it!=state.function->func_scope->scope.rend())
				{
					ASObject* o = asAtomHandler::getObject(it->object);
					if(it->considerDynamic)
					{
						considerDynamic=true;
						break;
					}
					if (asAtomHandler::is<Class_inherit>(it->object))
						asAtomHandler::as<Class_inherit>(it->object)->checkScriptInit();
					variable* v = o->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,state.worker);
					if (v && v->slotid && (v->slotid != UINT32_MAX))
					{
						state.preloadedcode.push_back(ABC_OP_OPTIMZED_GETLEX_FROMSLOT);
						state.preloadedcode.back().pcode.arg1_uint=v->slotid;
						state.preloadedcode.back().pcode.arg2_int=num;
						state.refreshOldNewPosition(code);
						checkForLocalResult(state,code,1,nullptr);
						typestack.push_back(typestackentry(nullptr,false));
						found=true;
						break;
					}
					++it;
					++num;
				}
			}
			if (found)
				return;
			if (!considerDynamic)
			{
				asAtom o=asAtomHandler::invalidAtom;
				if(asAtomHandler::isInvalid(o))
				{
					GET_VARIABLE_OPTION opt= (GET_VARIABLE_OPTION)(FROM_GETLEX | DONT_CALL_GETTER | NO_INCREF);
					state.mi->context->applicationDomain->getVariableByMultiname(o,*name,opt,state.worker);
				}
				if(asAtomHandler::isInvalid(o))
				{
					ASObject* cls = (ASObject*)dynamic_cast<const Class_base*>(name->cachedType);
					if (cls)
						o = asAtomHandler::fromObjectNoPrimitive(cls);
				}
				// fast check for builtin classes if no custom class with same name is defined
				if(asAtomHandler::isInvalid(o) && state.mi->context->applicationDomain->customClasses.find(name->name_s_id) == state.mi->context->applicationDomain->customClasses.end())
				{
					ASObject* cls = state.mi->context->applicationDomain->getSystemState()->systemDomain->getVariableByMultinameOpportunistic(*name,state.worker);
					if (cls)
						o = asAtomHandler::fromObject(cls);
					if (cls && !cls->is<Class_base>() && cls->getConstant())
					{
						// global builtin method/constant
						addCachedConstant(state,state.mi, o,code);
						typestack.push_back(typestackentry(cls->getClass(),false));
						return;
					}
				}
				if(asAtomHandler::isInvalid(o))
				{
					Type* t = Type::getTypeFromMultiname(name,state.mi->context,true);
					Class_base* cls = dynamic_cast<Class_base*>(t);
					if (cls)
						o = asAtomHandler::fromObject(cls);
				}
				if (asAtomHandler::is<Template_base>(o))
				{
					addCachedConstant(state,state.mi, o,code);
					typestack.push_back(typestackentry(nullptr,false));
					return;
				}
				if (asAtomHandler::is<Class_base>(o))
				{
					resulttype = asAtomHandler::as<Class_base>(o);
					addCachedConstant(state,state.mi, o,code);
					typestack.push_back(typestackentry(resulttype,true));
					return;
				}
			}
		}
		else if (getLexFindClass(state,name,true,typestack,code))
			return;
	}
	else if (state.function->isStatic && !state.function->isFromNewFunction() && (scopelist.begin()==scopelist.end() || !scopelist.back().considerDynamic))
	{
		// check for variables declared outside of a class (in a global object)
		bool done=false;
		if(!state.function->func_scope.isNull()) // check scope stack
		{
			auto it=state.function->func_scope->scope.rbegin();
			while(it!=state.function->func_scope->scope.rend())
			{
				if(it->considerDynamic)
					break;
				ASObject* scope = asAtomHandler::toObject(it->object,state.worker);
				variable* v = scope->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,state.worker);
				if (v)
				{
					if (v->kind == TRAIT_KIND::CONSTANT_TRAIT)
					{
						asAtom a = v->getVar(state.worker);
						addCachedConstant(state,state.mi, a,code);
						if (v->isResolved && dynamic_cast<const Class_base*>(v->type))
							resulttype = (Class_base*)v->type;
						typestack.push_back(typestackentry(resulttype,false));
						done=true;
						break;
					}
					else if (v->kind==DECLARED_TRAIT)
					{
						if (v->isClassVar())
						{
							asAtom a = v->getVar(state.worker);
							addCachedConstant(state,state.mi, a,code);
							if (v->isResolved && dynamic_cast<const Class_base*>(v->type))
								resulttype = (Class_base*)v->type;
							typestack.push_back(typestackentry(resulttype,false));
							done=true;
							break;
						}
						// property is static variable from global
						resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
						state.refreshOldNewPosition(code);
						asAtom clAtom = asAtomHandler::fromObjectNoPrimitive(scope);
						addCachedConstant(state,state.mi,clAtom,code);
						if (v->slotid)
						{
							// convert to getslot on global
							setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,false,ABC_OP_OPTIMZED_GETSLOT_SETSLOT);
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v->slotid-1;
						}
						else
						{
							// convert to getprop on class
							setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,0x66,code,true, false,resulttype,p,true,false,false,false);
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
						}
						typestack.push_back(typestackentry(resulttype,false));
						done=true;
						break;
					}
				}
				++it;
			}
		}
		if (done || getLexFindClass(state,name,false,typestack,code))
			return;
	}
#endif
	state.preloadedcode.push_back(ABC_OP_OPTIMZED_GETLEX);
	state.refreshOldNewPosition(code);
	state.preloadedcode[state.preloadedcode.size()-1].pcode.cachedmultiname2=name;
	if (!checkForLocalResult(state,code,0,resulttype))
	{
		// no local result possible, use standard operation
		state.preloadedcode[state.preloadedcode.size()-1].opcode=0x60;//getlex
		clearOperands(state,true,lastlocalresulttype);
	}
	typestack.push_back(typestackentry(resulttype,true));
}

}
