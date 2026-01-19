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
#include "scripting/abc_optimized_getproperty.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/class.h"
#include "parsing/streams.h"

using namespace std;
using namespace lightspark;

namespace lightspark
{
void preload_getproperty(preloadstate& state, std::vector<typestackentry>& typestack, memorystream& code, int opcode, Class_base** lastlocalresulttype)
{
	int32_t p = code.tellg();
	state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
	uint32_t t = code.readu30();
	assert_and_throw(t < state.mi->context->constant_pool.multiname_count);
	bool addname = true;
#ifdef ENABLE_OPTIMIZATION
	if (state.jumptargets.find(p) == state.jumptargets.end())
	{
		int runtimeargs = state.mi->context->constant_pool.multinames[t].runtimeargs;
		multiname* name = nullptr;
		if (runtimeargs==0)
			name = state.mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
		else if (runtimeargs==1
				 && state.operandlist.size() > 1
				 && state.operandlist.back().type != OPERANDTYPES::OP_LOCAL
				 && state.operandlist.back().type != OPERANDTYPES::OP_CACHED_SLOT
				 && (!state.operandlist[state.operandlist.size()-2].objtype
					 || dynamic_cast<TemplatedClass<Vector>*>(state.operandlist[state.operandlist.size()-2].objtype) == nullptr))
		{
			asAtom o = *state.mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
			name = state.mi->context->getMultinameImpl(o,nullptr,t,true,false);
			if (name)
			{
				state.operandlist.back().removeArg(state);
				state.operandlist.pop_back();
				removetypestack(typestack,1);
				runtimeargs=0;
			}
		}
		switch (runtimeargs)
		{
			case 0:
			{
				Class_base* resulttype = nullptr;
				if (state.operandlist.size() > 0 && state.operandlist.back().type != OP_LOCAL && state.operandlist.back().type != OP_CACHED_SLOT)
				{
					asAtom* a = state.mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
					ASObject* obj = asAtomHandler::getObject(*a);
					if (obj)
					{
						bool isborrowed=false;
						variable* v = obj->findVariableByMultiname(*name,obj->getClass(),nullptr,&isborrowed,false,state.worker);
						if (v && v->kind == CONSTANT_TRAIT && asAtomHandler::isInvalid(v->getter) )
						{
							state.operandlist.back().removeArg(state);
							state.operandlist.pop_back();
							asAtom a = v->getVar();
							addCachedConstant(state,state.mi, a,code);
							addname = false;
							removetypestack(typestack,runtimeargs+1);
							typestack.push_back(typestackentry(v->getClassVar(state.function->getSystemState()),isborrowed|| v->isClassVar()));
							break;
						}
						if (v && asAtomHandler::isValid(v->getter))
						{
							Class_base* resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
							if (!setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS,opcode,code,true, false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT))
								*lastlocalresulttype = resulttype;
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = asAtomHandler::getObject(v->getter);
							addname = false;
							removetypestack(typestack,runtimeargs+1);
							typestack.push_back(typestackentry(resulttype,false));
							break;
						}
						if (v && obj->is<Class_base>() && v->kind==DECLARED_TRAIT && !isborrowed)
						{
							if (v->isFunctionVar()) // in function declarations the resulttype of the function is stored in v->type
								resulttype = Class<IFunction>::getRef(state.function->getSystemState()).getPtr();
							else
								resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
						}
						if (v && v->slotid && (!typestack.back().obj || !typestack.back().classvar) && (!obj->is<Class_base>() || v->kind!=INSTANCE_TRAIT))
						{
							Class_base* resulttype = (Class_base*)(v->isResolved ? dynamic_cast<const Class_base*>(v->type):nullptr);
							if (obj->is<Global>())
							{
								// ensure init script is run
								asAtom ret = asAtomHandler::invalidAtom;
								obj->getVariableByMultiname(ret,*name,GET_VARIABLE_OPTION(DONT_CALL_GETTER|FROM_GETLEX),state.worker);
								ASATOM_DECREF(ret);
							}
							if (setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_GETSLOT_SETSLOT))
							{
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v->slotid-1;
								addname = false;
								removetypestack(typestack,runtimeargs+1);
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
							else
								*lastlocalresulttype = resulttype;
						}
					}
				}
				if (state.operandlist.size() > 0 && state.operandlist.back().objtype)
				{
					if (state.operandlist.back().type != OP_LOCAL && state.operandlist.back().type != OP_CACHED_SLOT && state.operandlist.back().type != OP_CACHED_CONSTANT)
					{
						// pushed builtin constants, property can be called directly and stored as a constant
						variable* v = state.operandlist.back().objtype->getBorrowedVariableByMultiname(*name);
						if (v && asAtomHandler::is<IFunction>(v->getter))
						{
							resulttype = v->isResolved && dynamic_cast<const Class_base*>(v->type) ? (Class_base*)v->type : nullptr;
							if (!state.operandlist.back().objtype->is<Class_inherit>() && resulttype==nullptr)
								LOG(LOG_NOT_IMPLEMENTED,"missing result type for builtin method for constant:"<<*name<<" "<<state.operandlist.back().objtype->toDebugString()<<" in function "<<getSys()->getStringFromUniqueId(state.function->functionname));
							if (!setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS,opcode,code,true, false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT))
								*lastlocalresulttype = resulttype;
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = asAtomHandler::getObject(v->getter);
							addname = false;
							removetypestack(typestack,runtimeargs+1);
							typestack.push_back(typestackentry(resulttype,false));
							break;
						}
					}
					else
					{
						bool isClassOperator=false;
						if (state.operandlist.back().type == OP_CACHED_CONSTANT)
						{
							asAtom* a = state.mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
							isClassOperator = asAtomHandler::is<Class_base>(*a);
						}
						if (!isClassOperator && canCallFunctionDirect(state.operandlist.back(),name))
						{
							variable* v = state.operandlist.back().objtype->getBorrowedVariableByMultiname(*name);
							if (v && asAtomHandler::is<IFunction>(v->getter))
							{
								resulttype = v->isResolved && dynamic_cast<const Class_base*>(v->type) ? (Class_base*)v->type : nullptr;
								if (!state.operandlist.back().objtype->is<Class_inherit>() && resulttype==nullptr)
									LOG(LOG_NOT_IMPLEMENTED,"missing result type for builtin method4:"<<*name<<" "<<state.operandlist.back().objtype->toDebugString()<<" in function "<<getSys()->getStringFromUniqueId(state.function->functionname));
								if (!setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS,opcode,code,true, false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT))
									*lastlocalresulttype = resulttype;
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = asAtomHandler::getObject(v->getter);
								addname = false;
								removetypestack(typestack,runtimeargs+1);
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
						}
						if (!isClassOperator && canCallFunctionDirect(state.operandlist.back(),name,true))
						{
							variable* v = state.operandlist.back().objtype->getBorrowedVariableByMultiname(*name);
							if (v)
							{
								resulttype = v->isResolved && dynamic_cast<const Class_base*>(v->type) ? (Class_base*)v->type : nullptr;
							}
						}
						if (state.operandlist.back().objtype && !state.operandlist.back().objtype->isInterface && state.operandlist.back().objtype->isInitialized()
							&& (!typestack.back().obj || !typestack.back().classvar || isClassOperator))
						{
							// check if we can replace getProperty by getSlot
							variable* v = nullptr;
							asAtom otmp = asAtomHandler::invalidAtom;
							if (isClassOperator)
							{
								asAtom* a = state.mi->context->getConstantAtom(state.operandlist.back().type,state.operandlist.back().index);
								v = asAtomHandler::getObject(*a)->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,state.worker);
								if (v && v->kind == TRAIT_KIND::INSTANCE_TRAIT)
									v=nullptr;
							}
							else
								v = getTempVariableFromClass(state.operandlist.back().objtype,otmp,name,state.worker);
							if (v && v->kind == CONSTANT_TRAIT
								&& asAtomHandler::isInvalid(v->getter)
								&& v->isValidVar()
								&& !v->isNullOrUndefinedVar())
							{
								state.operandlist.back().removeArg(state);
								state.operandlist.pop_back();
								asAtom a = v->getVar();
								addCachedConstant(state,state.mi, a,code);
								addname = false;
								removetypestack(typestack,runtimeargs+1);
								typestack.push_back(typestackentry(v->getClassVar(state.function->getSystemState()),v->isClassVar()));
								ASATOM_DECREF(otmp);
								break;
							}
							if (v && v->slotid)
							{
								resulttype = v->isResolved && dynamic_cast<const Class_base*>(v->type) ? (Class_base*)v->type : nullptr;
								// TODO the "unchanged" local may be changing it's slots during a call of one of it's methods,
								// so we can't use the cached-slot optimization for now
								// if (state.operandlist.back().type == OP_LOCAL)
								// {
								// 	if (state.unchangedlocals.find(state.operandlist.back().index) != state.unchangedlocals.end())
								// 	{
								// 		if (resulttype
								// 			&& resulttype != Class_object::getRef(state.function->getSystemState()).getPtr())
								// 		{
								// 			uint32_t index = state.operandlist.back().index;
								// 			state.operandlist.back().removeArg(state);
								// 			state.operandlist.pop_back();
								// 			addname = false;
								// 			addCachedSlot(state,index,v->slotid,code,resulttype);
								// 			removetypestack(typestack,runtimeargs+1);
								// 			typestack.push_back(typestackentry(resulttype,false));
								// 			ASATOM_DECREF(otmp);
								// 			break;
								// 		}
								// 	}
								// }

								if (setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETSLOT,opcode,code,true,false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_GETSLOT_SETSLOT))
								{
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg2_uint =v->slotid-1;
									if (state.operandlist.empty()) // indicates that checkforlocalresult returned false
										*lastlocalresulttype = resulttype;
									addname = false;
									ASATOM_DECREF(otmp);
									removetypestack(typestack,runtimeargs+1);
									typestack.push_back(typestackentry(resulttype,false));
									break;
								}
								else
								{
									if (checkForLocalResult(state,code,0,resulttype))
									{
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func = abc_getPropertyStaticName_localresult;
										addname = false;
									}
									else
										*lastlocalresulttype = resulttype;
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
									ASATOM_DECREF(otmp);
									removetypestack(typestack,runtimeargs+1);
									typestack.push_back(typestackentry(resulttype,false));
									break;
								}
							}
							else
								ASATOM_DECREF(otmp);
						}
					}
				}
				bool hasoperands = setupInstructionOneArgument(state,ABC_OP_OPTIMZED_GETPROPERTY_STATICNAME,opcode,code,true, false,resulttype,p,true);
				addname = !hasoperands;
				if (!hasoperands && checkForLocalResult(state,code,0,nullptr))
				{
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func=abc_getPropertyStaticName_localresult;
					addname = false;
				}
				state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
				removetypestack(typestack,runtimeargs+1);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			case 1:
			{
				Class_base* resulttype = nullptr;
				if (state.operandlist.size() > 1
					&& state.operandlist[state.operandlist.size()-2].objtype
					&& (state.operandlist.back().objtype == Class<Integer>::getRef(state.function->getSystemState()).getPtr() || state.operandlist.back().objtype == Class<UInteger>::getRef(state.function->getSystemState()).getPtr())
					&& dynamic_cast<TemplatedClass<Vector>*>(state.operandlist[state.operandlist.size()-2].objtype))
				{
					TemplatedClass<Vector>* cls=state.operandlist[state.operandlist.size()-2].objtype->as<TemplatedClass<Vector>>();
					resulttype = cls->getTypes().size() > 0 ? dynamic_cast<Class_base*>(cls->getTypes()[0]) : nullptr;
				}
				if (state.operandlist.size() > 0
					&& (state.operandlist.back().objtype == Class<Integer>::getRef(state.function->getSystemState()).getPtr()
						|| state.operandlist.back().objtype == Class<UInteger>::getRef(state.function->getSystemState()).getPtr()))
				{
					addname = !setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_GETPROPERTY_INTEGER,opcode,code,false,false,true,p,resulttype);
				}
				else if (state.operandlist.size() == 0
						   && typestack.size() > 0
						   && typestack.back().obj == Class<Integer>::getRef(state.function->getSystemState()).getPtr())
				{
					state.preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_GETPROPERTY_INTEGER_SIMPLE);
					clearOperands(state,true,lastlocalresulttype);
					addname=false;
				}
				else
				{
					if (setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_GETPROPERTY,opcode,code,false,false,true,p,resulttype))
					{
						state.preloadedcode.push_back(0);
						state.preloadedcode.back().pcode.arg3_uint=t;
						addname = false;
					}
				}
				removetypestack(typestack,runtimeargs+1);
				typestack.push_back(typestackentry(resulttype,false));
				break;
			}
			default:
				state.preloadedcode.push_back((uint32_t)opcode);
				clearOperands(state,true,lastlocalresulttype);
				removetypestack(typestack,runtimeargs+1);
				typestack.push_back(typestackentry(nullptr,false));
				break;
		}
	}
	else
#endif
	{
		state.preloadedcode.push_back((uint32_t)opcode);
		clearOperands(state,true,lastlocalresulttype);
		removetypestack(typestack,state.mi->context->constant_pool.multinames[t].runtimeargs+1);
		typestack.push_back(typestackentry(nullptr,false));
	}
	if (addname)
	{
		state.preloadedcode.push_back(0);
		state.preloadedcode.back().pcode.local3.pos=t;
	}
}

}
