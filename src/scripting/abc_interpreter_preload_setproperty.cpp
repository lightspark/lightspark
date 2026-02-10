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
#include "scripting/abc_optimized_setproperty.h"
#include "scripting/abc_optimized_setslot.h"
#include "scripting/toplevel/Global.h"
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
void preload_setproperty(preloadstate& state, std::vector<typestackentry>& typestack, memorystream& code, int opcode, Class_base** lastlocalresulttype,uint32_t simple_setter_opcode_pos)
{
	int32_t p = code.tellg();
	state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
	uint32_t t = code.readu30();
	assert_and_throw(t < state.mi->context->constant_pool.multiname_count);
#ifdef ENABLE_OPTIMIZATION
	if (state.jumptargets.find(p) == state.jumptargets.end())
	{
		switch (state.mi->context->constant_pool.multinames[t].runtimeargs)
		{
			case 0:
			{
				multiname* name =  state.mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
				if (state.operandlist.size() > 1)
				{
					auto it = state.operandlist.rbegin();
					Class_base* contenttype = it->objtype;
					it++;
					if (canCallFunctionDirect((*it),name) && !typestack[typestack.size()-2].classvar)
					{
						variable* v = it->objtype->getBorrowedVariableByMultiname(*name);
						if (v)
						{
							if (asAtomHandler::is<SyntheticFunction>(v->setter))
							{
								setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG_VOID,opcode,code);
								if (contenttype)
								{
									SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(v->setter);
									if (!f->getMethodInfo()->returnType)
										f->checkParamTypes();
									if (f->getMethodInfo()->paramTypes.size() && f->canSkipCoercion(0,contenttype))
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_uint = ABC_OP_COERCED;
								}
								state.preloadedcode.push_back(0);
								state.preloadedcode.back().pcode.cacheobj3 = asAtomHandler::getObject(v->setter);
								removetypestack(typestack,state.mi->context->constant_pool.multinames[t].runtimeargs+2);
								break;
							}
							if (asAtomHandler::is<Function>(v->setter))
							{
								setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG_VOID,opcode,code);
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj3 = asAtomHandler::getObject(v->setter);
								removetypestack(typestack,state.mi->context->constant_pool.multinames[t].runtimeargs+2);
								break;
							}
						}
					}
					if ((it->type == OP_LOCAL || it->type == OP_CACHED_CONSTANT || it->type == OP_CACHED_SLOT)
						&& it->objtype && !it->objtype->isInterface && it->objtype->isInitialized())
					{
						// check if we can replace setProperty by setSlot
						asAtom otmp = asAtomHandler::invalidAtom;
						variable* v = nullptr;
						if (typestack[typestack.size()-2].obj && typestack[typestack.size()-2].obj->is<Global>())
							v = typestack[typestack.size()-2].obj->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,state.worker);
						else
							v = getTempVariableFromClass(it->objtype,otmp,name,state.worker);
						if (!v && it->objtype->is<Class_inherit>())
						{
							bool isBorrowed=false;
							v = it->objtype->findVariableByMultiname(*name,nullptr,nullptr,&isBorrowed,false,state.worker);
							if (v && (v->kind != DECLARED_TRAIT || isBorrowed))
								v=nullptr;
						}
						if (!asAtomHandler::isPrimitive(otmp) && v && v->slotid)
						{
							// we can skip coercing when setting the slot value if
							// - contenttype is the same as the variable type or
							// - variable type is any or void or
							// - contenttype is subclass of variable type or
							// - contenttype is numeric and variable type is Number
							// - contenttype is Number and variable type is Integer and previous opcode was arithmetic with localresult
							int operator_start = v->isResolved && ((contenttype && contenttype == v->type) || !dynamic_cast<const Class_base*>(v->type)) ? ABC_OP_OPTIMZED_SETSLOT_NOCOERCE : ABC_OP_OPTIMZED_SETSLOT;
							if (contenttype && v->isResolved && dynamic_cast<const Class_base*>(v->type))
							{
								Class_base* vtype = (Class_base*)v->type;
								if (contenttype->isSubClass(vtype) || v->type==Type::anyType || v->type==Type::voidType
									|| (( contenttype == Class<Number>::getRef(state.function->getSystemState()).getPtr() ||
										 contenttype == Class<Integer>::getRef(state.function->getSystemState()).getPtr() ||
										 contenttype == Class<UInteger>::getRef(state.function->getSystemState()).getPtr()) &&
										( vtype == Class<Number>::getRef(state.function->getSystemState()).getPtr()))
									)
								{
									operator_start = ABC_OP_OPTIMZED_SETSLOT_NOCOERCE;
								}
								else if (checkmatchingLastObjtype(state,contenttype,vtype))
								{
									operator_start = ABC_OP_OPTIMZED_SETSLOT_NOCOERCE;
								}
							}
							bool getslotisvalue = state.preloadedcode.size() && state.preloadedcode.at(state.preloadedcode.size()-1).operator_start==ABC_OP_OPTIMZED_GETSLOT;
							setupInstructionTwoArgumentsNoResult(state,operator_start,opcode,code);
							if (!state.lastoperandsSwapped //TODO implement this optimization for setproperty following swap
								&& getslotisvalue
								&& state.preloadedcode.size() > 1
								&& v->slotid < ABC_OP_BITMASK_USED
								&& state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func == abc_setslotNoCoerce_local_local
								&& state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local_pos1 <0xffff // only optimize if local_pos1 fits in uint16_t
								&& state.preloadedcode.at(state.preloadedcode.size()-2).operator_setslot != UINT32_MAX)
							{
								// this optimized setslot can be combined with previous opcode

								// move local_pos1 to local_pos3
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos = state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local_pos1;
								state.preloadedcode.at(state.preloadedcode.size()-1).cachedslot3 = state.preloadedcode.at(state.preloadedcode.size()-1).cachedslot1;
								// move local1 of previous opcode to local1 of current opcode
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj1 = state.preloadedcode.at(state.preloadedcode.size()-2).pcode.cacheobj1;
								state.preloadedcode.at(state.preloadedcode.size()-1).cachedslot1 = state.preloadedcode.at(state.preloadedcode.size()-2).cachedslot1;
								// move local2 of previous opcode to local2 of current opcode
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = state.preloadedcode.at(state.preloadedcode.size()-2).pcode.cacheobj2;
								state.preloadedcode.at(state.preloadedcode.size()-1).cachedslot2 = state.preloadedcode.at(state.preloadedcode.size()-2).cachedslot2;
								// move flags of previous opcode to flags of current opcode
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags = state.preloadedcode.at(state.preloadedcode.size()-2).pcode.local3.flags;
								// set current opcode to optimized setslot opcode
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func = ABCVm::abcfunctions[state.preloadedcode.at(state.preloadedcode.size()-2).operator_setslot];
								// remove previous opcode
								state.preloadedcode.erase(state.preloadedcode.begin()+(state.preloadedcode.size()-2));
								// set slotid into local3.flags of current opcode
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags |=v->slotid-1;

								state.refreshOldNewPosition(code);
								if (!state.operandlist.empty())
									state.operandlist.pop_back();
							}
							else
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_uint =v->slotid-1;
							ASATOM_DECREF(otmp);
							removetypestack(typestack,state.mi->context->constant_pool.multinames[t].runtimeargs+2);
							break;
						}
						else
							ASATOM_DECREF(otmp);
					}
				}
				if (setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME,opcode,code))
					state.preloadedcode.push_back(0);
				else
				{
					state.preloadedcode.at(state.preloadedcode.size()-1).pcode.func = abc_setPropertyStaticName;
					clearOperands(state,false,lastlocalresulttype);
				}
				state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 =name;
				state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos = opcode; // use local3.pos as indicator for setproperty/initproperty
				if ((simple_setter_opcode_pos != UINT32_MAX) // function is simple setter
					&& state.function->inClass->isFinal // TODO also enable optimization for classes where it is guarranteed that the method is not overridden in derived classes
					&& state.function->inClass->getInterfaces().empty()) // class doesn't implement any interfaces
				{
					variable* v = state.function->inClass->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,state.worker);
					if (v && v->kind == TRAIT_KIND::INSTANCE_TRAIT)
						state.function->setSimpleGetterSetter(name);
				}
				removetypestack(typestack,state.mi->context->constant_pool.multinames[t].runtimeargs+2);
				break;
			}
			case 1:
				if (state.operandlist.size() > 2 && (state.operandlist[state.operandlist.size()-2].objtype == Class<Integer>::getRef(state.function->getSystemState()).getPtr()))
				{
					uint32_t startopcode = ABC_OP_OPTIMZED_SETPROPERTY_INTEGER;
					if (state.operandlist[state.operandlist.size()-3].objtype
						&& dynamic_cast<TemplatedClass<Vector>*>(state.operandlist[state.operandlist.size()-3].objtype))
					{
						TemplatedClass<Vector>* cls=state.operandlist[state.operandlist.size()-3].objtype->as<TemplatedClass<Vector>>();
						Class_base* vectype = cls->getTypes().size() > 0 ? (Class_base*)cls->getTypes()[0] : nullptr;
						if (checkmatchingLastObjtype(state,state.operandlist[state.operandlist.size()-1].objtype,vectype))
						{
							// use special fast setproperty without coercing for Vector
							startopcode = ABC_OP_OPTIMZED_SETPROPERTY_INTEGER_VECTOR;
						}
					}
					if (state.operandlist[state.operandlist.size()-3].type == OP_LOCAL || state.operandlist[state.operandlist.size()-3].type == OP_CACHED_SLOT)
					{
						startopcode+= 4;
						int index = state.operandlist[state.operandlist.size()-3].index;
						bool cachedslot = state.operandlist[state.operandlist.size()-3].type == OP_CACHED_SLOT;
						setupInstructionTwoArgumentsNoResult(state,startopcode,opcode,code);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos=index;
						state.preloadedcode.at(state.preloadedcode.size()-1).cachedslot3 = cachedslot;
						state.preloadedcode.push_back(0);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos = opcode; // use local3.pos as indicator for setproperty/initproperty
						state.operandlist.back().removeArg(state);
						setOperandModified(state,cachedslot ? OP_CACHED_SLOT : OP_LOCAL,index);
						clearOperands(state,false,lastlocalresulttype);
					}
					else
					{
						asAtom* arg = state.mi->context->getConstantAtom(state.operandlist[state.operandlist.size()-3].type,state.operandlist[state.operandlist.size()-3].index);
						setupInstructionTwoArgumentsNoResult(state,startopcode,opcode,code);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.arg3_constant=arg;
						state.preloadedcode.push_back(0);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos = opcode; // use local3.pos as indicator for setproperty/initproperty
						state.operandlist.back().removeArg(state);
						clearOperands(state,false,lastlocalresulttype);
					}
				}
				else
				{
					if (typestack.size() > 2 && typestack[typestack.size()-2].obj == Class<Integer>::getRef(state.function->getSystemState()).getPtr())
					{
						state.preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_SETPROPERTY_INTEGER_SIMPLE);
						state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags = opcode; // use local3.flags as indicator for setproperty/initproperty
					}
					else
					{
						state.preloadedcode.push_back((uint32_t)opcode);
						state.preloadedcode.back().pcode.local3.pos=t;
					}
					clearOperands(state,false,lastlocalresulttype);
				}
				removetypestack(typestack,state.mi->context->constant_pool.multinames[t].runtimeargs+1);
				break;
			default:
				state.preloadedcode.push_back((uint32_t)opcode);
				state.preloadedcode.back().pcode.local3.pos=t;
				clearOperands(state,false,lastlocalresulttype);
				removetypestack(typestack,state.mi->context->constant_pool.multinames[t].runtimeargs+1);
				break;
		}
	}
	else
	{
		switch (state.mi->context->constant_pool.multinames[t].runtimeargs)
		{
			case 0:
			{
				multiname* name =  state.mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
				state.preloadedcode.push_back((uint32_t)ABC_OP_OPTIMZED_SETPROPERTY_STATICNAME_SIMPLE);
				state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 =name;
				state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.pos = opcode; // use local3.pos as indicator for setproperty/initproperty
				if ((simple_setter_opcode_pos != UINT32_MAX) // function is simple setter
					&& state.function->inClass->isFinal // TODO also enable optimization for classes where it is guarranteed that the method is not overridden in derived classes
					&& state.function->inClass->getInterfaces().empty()) // class doesn't implement any interfaces
				{
					variable* v = state.function->inClass->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,state.worker);
					if (v && v->kind == TRAIT_KIND::INSTANCE_TRAIT)
						state.function->setSimpleGetterSetter(name);
				}
				removetypestack(typestack,state.mi->context->constant_pool.multinames[t].runtimeargs+1);
				break;
			}
			default:
				state.preloadedcode.push_back((uint32_t)opcode);
				state.preloadedcode.back().pcode.local3.pos=t;
				clearOperands(state,false,lastlocalresulttype);
				removetypestack(typestack,state.mi->context->constant_pool.multinames[t].runtimeargs+1);
				break;
		}
	}
#else
	state.preloadedcode.push_back((uint32_t)opcode);
	state.preloadedcode.back().pcode.local3.pos=t;
	clearOperands(state,false,lastlocalresulttype);
	removetypestack(typestack,state.mi->context->constant_pool.multinames[t].runtimeargs+1);
#endif
}

}
