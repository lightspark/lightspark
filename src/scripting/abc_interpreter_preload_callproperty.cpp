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
#include "scripting/class.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Global.h"
#include "parsing/streams.h"

using namespace std;
using namespace lightspark;

namespace lightspark
{
void setUnoptimizedCall(preloadstate& state,std::vector<typestackentry>& typestack,int opcode,Class_base** lastlocalresulttype, uint32_t argcount,Class_base* resulttype, bool needResult,uint32_t t)
{
	state.preloadedcode.push_back((uint32_t)opcode);
	clearOperands(state,true,lastlocalresulttype);
	state.preloadedcode.push_back(0);
	state.preloadedcode.back().pcode.arg1_uint=t;
	state.preloadedcode.back().pcode.arg2_uint=argcount;
	if (needResult)
		typestack.push_back(typestackentry(resulttype,false));

}
void preload_callprop(preloadstate& state,std::vector<typestackentry>& typestack,memorystream& code,int opcode,Class_base** lastlocalresulttype,uint8_t prevopcode)
{
	int32_t p = code.tellg();
	state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
	uint32_t t = code.readu30();
	assert_and_throw(t < state.mi->context->constant_pool.multiname_count);
	uint32_t argcount = code.readu30();
	bool needResult =
		opcode != 0x4f && //callpropvoid
		opcode != 0x4e; //callsupervoid
	bool needSuper =
		opcode == 0x45 || //callsuper
		opcode == 0x4e; //callsupervoid
#ifdef ENABLE_OPTIMIZATION
	if (needResult && code.peekbyte() == 0x29 && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
	{
		// callproperty is followed by pop
		if (opcode == 0x45)
			opcode = 0x4e; // treat call as callsupervoid
		else
			opcode = 0x4f; // treat call as callpropvoid
		needResult=false;
		state.oldnewpositions[code.tellg()] = (int32_t)state.preloadedcode.size()+1;
		code.readbyte(); // skip pop
	}
	// TODO optimize non-static multinames
	if (state.jumptargets.find(p) == state.jumptargets.end())
	{
		multiname* name =  state.mi->context->getMultinameImpl(asAtomHandler::nullAtom,nullptr,t,false);
		Class_base* resulttype=nullptr;
		bool skipcoerce = false;
		SyntheticFunction* func= nullptr;
		variable* v = nullptr;
		uint32_t operationcount = argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1;
		ASObject* cls = nullptr;
		if (needSuper)
		{
			if (state.function->inClass)
				cls = state.function->inClass->super.getPtr();
			else	if (typestack.size() >= operationcount && typestack[typestack.size()-operationcount].obj)
				cls = typestack[typestack.size()-operationcount].obj;
		}
		else
		{
			if (typestack.size() >= operationcount && typestack[typestack.size()-operationcount].obj)
				cls = typestack[typestack.size()-operationcount].obj;
		}
		bool fromglobal = false;
		if (cls && cls->is<Global>())
		{
			v = cls->findVariableByMultiname(*name,nullptr,nullptr,nullptr,false,state.worker);
			if (v)
			{
				if ((v->kind & TRAIT_KIND::DECLARED_TRAIT) == 0 || asAtomHandler::is<Class_base>(v->var))
					v=nullptr;
			}
			fromglobal= v != nullptr;
		}
		uint32_t instancevarSlotID=0;
		uint32_t classBorrowedSlotID=UINT32_MAX;
		if (!v && typestack.size() >= operationcount)
		{
			bool isoverridden=false;
			if (cls && canCallFunctionDirect(cls,name,false))
			{
				v = needSuper || typestack[typestack.size()-operationcount].classvar ?
							cls->findVariableByMultiname(
							*name,
							cls->as<Class_base>(),
							nullptr,nullptr,false,state.worker):
							cls->as<Class_base>()->getBorrowedVariableByMultiname(*name);
			}
			else if (cls && canCallFunctionDirect(cls,name,true))
			{
				isoverridden=true;
				v = needSuper || typestack[typestack.size()-operationcount].classvar ?
							cls->findVariableByMultiname(
								*name,
								cls->as<Class_base>(),
								nullptr,nullptr,false,state.worker):
							cls->as<Class_base>()->getBorrowedVariableByMultiname(*name);
			}
			if (!v
					&& state.operandlist.size()>=operationcount
					&& (state.operandlist[state.operandlist.size()-operationcount].type == OP_LOCAL
						|| state.operandlist[state.operandlist.size()-operationcount].type == OP_CACHED_CONSTANT)
					&& state.operandlist[state.operandlist.size()-operationcount].objtype && state.operandlist[state.operandlist.size()-operationcount].objtype->is<Class_base>()
					&& canCallFunctionDirect(state.operandlist.at(state.operandlist.size()-operationcount),name))
			{
				cls = state.operandlist[state.operandlist.size()-operationcount].objtype;
				if (needSuper)
					cls = cls->as<Class_base>()->super.getPtr();
				v=cls->findVariableByMultiname(*name,cls->as<Class_base>(),nullptr,nullptr,false,state.worker);
				if (v && !asAtomHandler::is<SyntheticFunction>(v->var))
					v=nullptr;
			}
			if (!v && cls && cls->is<Class_base>())
			{
				asAtom otmp = asAtomHandler::invalidAtom;
				// property may be a slot variable
				v = getTempVariableFromClass(cls->as<Class_base>(),otmp,name,state.worker);
				if (v && v->slotid)
					instancevarSlotID=v->slotid;
				v=nullptr;
				ASATOM_DECREF(otmp);
			}
			if (v)
			{
				if (asAtomHandler::is<SyntheticFunction>(v->var) && (fromglobal || (asAtomHandler::as<SyntheticFunction>(v->var)->inClass && asAtomHandler::as<SyntheticFunction>(v->var)->inClass == cls)))
				{
					func = asAtomHandler::as<SyntheticFunction>(v->var);
					resulttype = func->getReturnType();
					if (func->getMethodInfo()->paramTypes.size() >= argcount)
					{
						skipcoerce=true;
						auto it2 = typestack.rbegin();
						for(uint32_t i= argcount; i > 0; i--)
						{
							if (!(*it2).obj || !(*it2).obj->is<Class_base>() || !func->canSkipCoercion(i-1,(*it2).obj->as<Class_base>()))
							{
								skipcoerce=false;
								break;
							}
							it2++;
						}
					}
				}
				if (isoverridden && !needSuper)
				{
					if (cls && cls->is<Class_base>())
						classBorrowedSlotID = cls->as<Class_base>()->findBorrowedSlotID(v);
					func=nullptr;
					v=nullptr;
				}
			}
		}
		// if variable is taken from Global object, force handling of 1-argument-optimization as multi-argument optimization (for proper handling of variable pointer)
		int argcountcheckglobal = fromglobal && argcount == 1 ? 2 : argcount;
		switch (state.mi->context->constant_pool.multinames[t].runtimeargs)
		{
			case 0:
				switch (argcountcheckglobal)
				{
					case 0:
						if (instancevarSlotID)
						{
							if ((!needResult && setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_CALLPROPVOID_SLOTVAR_NOARGS,opcode,code,p)) ||
							   ((needResult && setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLPROPERTY_SLOTVAR_NOARGS,opcode,code,true,false,resulttype,p,true))))
							{
								state.preloadedcode.push_back(0);
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local2.pos = instancevarSlotID-1;
							}
							removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
							if (needResult)
								typestack.push_back(typestackentry(resulttype,false));
							break;
						}
						if (classBorrowedSlotID != UINT32_MAX)
						{
							if ((!needResult && setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_CALLPROPVOID_BORROWEDSLOT_NOARGS,opcode,code,p)) ||
							   ((needResult && setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLPROPERTY_BORROWEDSLOT_NOARGS,opcode,code,true,false,resulttype,p,true))))
							{
								state.preloadedcode.push_back(0);
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local2.pos = classBorrowedSlotID;
							}
							removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
							if (needResult)
								typestack.push_back(typestackentry(resulttype,false));
							break;
						}

						if (state.operandlist.size() > 0
								&& (state.operandlist.back().type == OP_LOCAL || state.operandlist.back().type == OP_CACHED_CONSTANT || state.operandlist.back().type == OP_CACHED_SLOT)
								&& state.operandlist.back().objtype)
						{
							if (canCallFunctionDirect(state.operandlist.back(),name,needSuper))
							{
								if (v && asAtomHandler::is<IFunction>(v->var) && asAtomHandler::isInvalid(asAtomHandler::as<IFunction>(v->var)->closure_this))
								{
									ASObject* cls = state.operandlist.back().objtype;
									if (needResult)
									{
										resulttype = asAtomHandler::as<IFunction>(v->var)->getReturnType();
										if (!resulttype && asAtomHandler::is<Function>(v->var))
											LOG(LOG_NOT_IMPLEMENTED,"missing result type for builtin method1:"<<*name<<" "<<cls->toDebugString());
										setupInstructionOneArgument(state,asAtomHandler::is<Function>(v->var) ? ABC_OP_OPTIMZED_CALLBUILTINFUNCTION_NOARGS : ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS ,opcode,code,true, false,resulttype,p,true,false,false,true,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_SETSLOT);
									}
									else
										setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_CALLFUNCTION_NOARGS_VOID,opcode,code,p);
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj2 = asAtomHandler::getObject(v->var);
									removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
									if (needResult)
									{
										if (resulttype == nullptr && asAtomHandler::is<Function>(v->var))
										{
											resulttype = asAtomHandler::as<Function>(v->var)->getReturnType();
											if (resulttype == nullptr)
												LOG(LOG_NOT_IMPLEMENTED,"missing result type for builtin method2:"<<*name<<" "<<cls->toDebugString());
										}
										typestack.push_back(typestackentry(resulttype,false));
									}
									break;
								}
							}
						}
						if (needSuper)
						{
							// no super method found, use standard call
							setUnoptimizedCall(state,typestack,opcode,lastlocalresulttype,argcount,resulttype,needResult,t);
							break;
						}

						if ((!needResult && setupInstructionOneArgumentNoResult(state,ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_NOARGS,opcode,code,p)) ||
						   ((needResult && setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_NOARGS,opcode,code,true,false,resulttype,p,true))))
						{
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
							state.preloadedcode.push_back(0);
							if (fromglobal && v)
							{
								state.preloadedcode.back().pcode.local2.flags |= ABC_OP_FROMGLOBAL;
								state.preloadedcode.back().pcode.cachedvar3 = v;
							}
						}
						else
						{
							if (func)
							{
								state.preloadedcode.at(state.preloadedcode.size()-1).opcode= (!needResult ? ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS);
								state.preloadedcode.back().pcode.local2.pos = argcount;
								if (skipcoerce)
									state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
								if (fromglobal && v)
								{
									state.preloadedcode.back().pcode.local2.flags |= ABC_OP_FROMGLOBAL;
									state.preloadedcode.back().pcode.cachedvar3 = v;
								}
								else
									state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj3 = func;
							}
							else
							{
								state.preloadedcode.at(state.preloadedcode.size()-1).opcode= (!needResult ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS);
								state.preloadedcode.back().pcode.local2.pos = argcount;
								if (skipcoerce)
									state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
								if (fromglobal && v)
								{
									state.preloadedcode.back().pcode.local2.flags |= ABC_OP_FROMGLOBAL;
									state.preloadedcode.back().pcode.cachedvar3 = v;
								}
								state.preloadedcode.push_back(0);
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
							}
							clearOperands(state,true,lastlocalresulttype);
						}
						removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
						if (needResult)
							typestack.push_back(typestackentry(resulttype,false));
						break;
					case 1:
					{
						if (instancevarSlotID)
						{
							if ((!needResult && setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLPROPVOID_SLOTVAR,opcode,code)) ||
							   ((needResult && setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_CALLPROPERTY_SLOTVAR,opcode,code,false,false,true,p,resulttype))))
							{
								state.preloadedcode.push_back(0);
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local2.pos = instancevarSlotID-1;
							}
							removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
							if (needResult)
								typestack.push_back(typestackentry(resulttype,false));
							break;
						}
						if (classBorrowedSlotID != UINT32_MAX)
						{
							if ((!needResult && setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLPROPVOID_BORROWEDSLOT,opcode,code)) ||
							   ((needResult && setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_CALLPROPERTY_BORROWEDSLOT,opcode,code,false,false,true,p,resulttype))))
							{
								state.preloadedcode.push_back(0);
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local2.pos = classBorrowedSlotID;
							}
							removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
							if (needResult)
								typestack.push_back(typestackentry(resulttype,false));
							break;
						}
						bool isGenerator=false;
						bool generatorneedsconversion=false;
						if (typestack.size() > 1 &&
								typestack[typestack.size()-2].obj != nullptr &&
								(typestack[typestack.size()-2].obj->is<Global>() ||
								 (typestack[typestack.size()-2].obj->is<Class_base>() && typestack[typestack.size()-2].obj->as<Class_base>()->isSealed)))
						{
							asAtom func = asAtomHandler::invalidAtom;
							GET_VARIABLE_RESULT r = typestack[typestack.size()-2].obj->getVariableByMultiname(func,*name,GET_VARIABLE_OPTION(DONT_CALL_GETTER|FROM_GETLEX|NO_INCREF),state.worker);
							if (asAtomHandler::isInvalid(func) && typestack[typestack.size()-2].obj->is<Class_base>())
							{
								variable* mvar = typestack[typestack.size()-2].obj->as<Class_base>()->getBorrowedVariableByMultiname(*name);
								if (mvar)
									func = mvar->var;
							}
							if (asAtomHandler::isClass(func) && asAtomHandler::as<Class_base>(func)->isBuiltin())
							{
								// function is a class generator, we can use it as the result type
								resulttype = asAtomHandler::as<Class_base>(func);
								if (resulttype == Class<ASObject>::getRef(state.function->getSystemState()).getPtr())
								{
									// Object generator can be skipped
									if (state.operandlist.size() > 1 && state.operandlist.back().objtype)
									{
										isGenerator=true;
										generatorneedsconversion = false;
									}
								}
								else if (resulttype == Class<Boolean>::getRef(state.function->getSystemState()).getPtr())
								{
									// Boolean generator can be skipped or turned into convert_b
									if (state.operandlist.size() > 1)
									{
										isGenerator=true;
										generatorneedsconversion = !(code.peekbyte() == 0x11 || //iftrue
												code.peekbyte() == 0x12 || //iffalse
												code.peekbyte() == 0x96    //not
												);
									}
								}
								if (resulttype == Class<Integer>::getRef(state.function->getSystemState()).getPtr()
										|| resulttype == Class<UInteger>::getRef(state.function->getSystemState()).getPtr()
										|| resulttype == Class<Number>::getRef(state.function->getSystemState()).getPtr())
								{
									if (state.operandlist.size() > 2 && typestack.size() > 0 && typestack.back().obj == resulttype)
										isGenerator=true; // generator can be skipped
									else if (state.operandlist.size() > 1)
									{
										// generator will be replaced by a conversion operator
										isGenerator=true;
										if (resulttype != Class<Number>::getRef(state.function->getSystemState()).getPtr()
												|| (state.operandlist.back().objtype != Class<Integer>::getRef(state.function->getSystemState()).getPtr()
													&& state.operandlist.back().objtype != Class<UInteger>::getRef(state.function->getSystemState()).getPtr()
													&& state.operandlist.back().objtype != Class<Number>::getRef(state.function->getSystemState()).getPtr()))
											generatorneedsconversion=true;
									}
								}
							}
							else if (asAtomHandler::is<SyntheticFunction>(func) && needResult)
							{
								SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(func);
								resulttype = f->getReturnType();
							}
							else if (asAtomHandler::is<Function>(func) && needResult)
							{
								Function* f = asAtomHandler::as<Function>(func);
								resulttype = f->getReturnType();
								if (!resulttype)
									LOG(LOG_NOT_IMPLEMENTED,"missing result type for builtin method3:"<<*name<<" "<<typestack[typestack.size()-2].obj->toDebugString());
							}
							if (r & GETVAR_ISNEWOBJECT)
							{
								ASATOM_DECREF(func);
							}
						}
						if (state.operandlist.size() > 1 && !isGenerator)
						{
							auto it = state.operandlist.rbegin();
							Class_base* argtype = it->objtype;
							it++;
							if (canCallFunctionDirect((*it),name,needSuper))
							{
								if (!fromglobal
									&& v
									&& asAtomHandler::is<IFunction>(v->var)
									&& (
											(state.function->inClass && state.function->inClass == asAtomHandler::as<IFunction>(v->var)->inClass)
											||
											(asAtomHandler::isInvalid(asAtomHandler::as<IFunction>(v->var)->closure_this)
											 && (!asAtomHandler::as<IFunction>(v->var)->inClass
												 || !state.function->inClass
												 || (state.function->inClass->getConstructIndicator() && state.function->inClass->isSubClass(asAtomHandler::as<IFunction>(v->var)->inClass))
												 || !state.function->inClass->isSubClass(asAtomHandler::as<IFunction>(v->var)->inClass) // function is from a subclass of the caller, so it may not be setup yet if we are currently executing the constructor
												 )
											 )
										 )
										)
								{
									if (asAtomHandler::is<SyntheticFunction>(v->var))
									{
										if (!needResult)
											setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG_VOID,opcode,code);
										else
											setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_ONEARG,opcode,code,false,false,true,p,resulttype);
										if (argtype)
										{
											SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(v->var);
											if (!f->getMethodInfo()->returnType)
												f->checkParamTypes();
											if (f->getMethodInfo()->paramTypes.size() && f->canSkipCoercion(0,argtype))
												state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local3.flags = ABC_OP_COERCED;
										}
										state.preloadedcode.push_back(0);
										if (fromglobal)
										{
											state.preloadedcode.back().pcode.local2.flags |= ABC_OP_FROMGLOBAL;
											state.preloadedcode.back().pcode.cachedvar3 = v;
										}
										else
											state.preloadedcode.back().pcode.cacheobj3 = asAtomHandler::getObject(v->var);
										removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
										if (needResult)
											typestack.push_back(typestackentry(resulttype,false));
										break;
									}
									if (asAtomHandler::is<Function>(v->var))
									{
										if (!needResult)
										{
											setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG_VOID,opcode,code);
										}
										else
										{
											setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_ONEARG,opcode,code,false,false,true,p,resulttype);
											state.preloadedcode.push_back(0);
										}
										state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj3 = asAtomHandler::getObject(v->var);
										removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
										if (needResult)
											typestack.push_back(typestackentry(resulttype,false));
										break;
									}
								}
							}
						}

						if (needSuper)
						{
							// no super method found, use standard call
							setUnoptimizedCall(state,typestack,opcode,lastlocalresulttype,argcount,resulttype,needResult,t);
							break;
						}
						// remember operand for isGenerator
						operands op;
						if (state.operandlist.size()>0)
							op = state.operandlist.back();
						bool reuseoperand = prevopcode == 0x62//getlocal
								|| prevopcode == 0xd0//getlocal_0
								|| prevopcode == 0xd1//getlocal_1
								|| prevopcode == 0xd2//getlocal_2
								|| prevopcode == 0xd3//getlocal_3
								|| prevopcode == 0x66//getproperty
								|| state.preloadedcode.back().hasLocalResult
								|| (op.type != OP_LOCAL && op.type != OP_CACHED_SLOT)
							;
						uint32_t opsize=state.operandlist.size();
						if ((!needResult && setupInstructionTwoArgumentsNoResult(state,ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME,opcode,code)) ||
						   ((needResult && setupInstructionTwoArguments(state,ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME,opcode,code,false,false,!(reuseoperand || generatorneedsconversion) || !isGenerator,p,resulttype))))
						{
							// generator for Integer/UInteger/Number can be skipped if argument is already an Integer/UInteger/Number and the result will be used as local result
							if (isGenerator && (reuseoperand || generatorneedsconversion))
							{
								removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
								// remove caller
								if (needResult && state.preloadedcode.back().opcode >= ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME+4 // caller has localresult
										&& state.operandlist.size() > opsize-2) // localresult was added to operandlist
								{
									auto it =state.operandlist.end();
									(--it)->removeArg(state);// remove arg2
									state.operandlist.pop_back();
								}
								state.preloadedcode.pop_back();
								// re-add last operand if it is not the result of the previous operation
								if (reuseoperand || generatorneedsconversion)
									addOperand(state,op,code);
								state.refreshOldNewPosition(code);
								if (generatorneedsconversion)
								{
									// replace call to generator with optimized convert_i/convert_d
									if (resulttype == Class<Integer>::getRef(state.function->getSystemState()).getPtr())
										setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTI,
																	0x73 //convert_i
																	,code,true,true,resulttype,code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTI_SETSLOT);
									else if (resulttype == Class<UInteger>::getRef(state.function->getSystemState()).getPtr())
										setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTU,
																	0x74 //convert_u
																	,code,true,true,resulttype,code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTU_SETSLOT);
									else if (resulttype == Class<Boolean>::getRef(state.function->getSystemState()).getPtr())
										setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTB,
																	0x76 //convert_b
																	,code,true,true,resulttype,code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTB_SETSLOT);
									else
										setupInstructionOneArgument(state,ABC_OP_OPTIMZED_CONVERTD,
																	0x75 //convert_d
																	,code,true,true,resulttype,code.tellg(),true,false,false,true,ABC_OP_OPTIMZED_CONVERTD_SETSLOT);
								}
								typestack.push_back(typestackentry(resulttype,false));
								break;
							}
							state.preloadedcode.push_back(0);
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
							state.preloadedcode.push_back(0);
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.local2.flags =(skipcoerce ? ABC_OP_COERCED : 0);
						}
						else if (needResult && checkForLocalResult(state,code,0,resulttype))
						{
							state.preloadedcode.at(state.preloadedcode.size()-1).opcode=ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_LOCALRESULT;
							state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
							state.preloadedcode.push_back(0);
							state.preloadedcode.back().pcode.local2.flags = (skipcoerce ? ABC_OP_COERCED : 0);
						}
						else
						{
							if (func)
							{
								state.preloadedcode.at(state.preloadedcode.size()-1).opcode= (!needResult ? ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS);
								state.preloadedcode.back().pcode.local2.pos = argcount;
								if (skipcoerce)
									state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj3 = func;
							}
							else
							{
								state.preloadedcode.at(state.preloadedcode.size()-1).opcode= (!needResult ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS);
								state.preloadedcode.back().pcode.local2.pos = argcount;
								if (skipcoerce)
									state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
								state.preloadedcode.push_back(0);
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cachedmultiname2 = name;
							}
							clearOperands(state,true,lastlocalresulttype);
						}
						removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
						if (needResult)
							typestack.push_back(typestackentry(resulttype,false));
						break;
					}
					default:
						if (state.operandlist.size() >= argcount)
						{
							state.preloadedcode.push_back((uint32_t)(!needResult ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED));
							bool allargsint=true;
							auto it = state.operandlist.rbegin();
							for(uint32_t i= 0; i < argcount; i++)
							{
								it->removeArg(state);
								state.preloadedcode.push_back(0);
								it->fillCode(state,0,state.preloadedcode.size()-1,false);
								state.preloadedcode.back().pcode.arg2_uint = it->type;
								if (!it->objtype || it->objtype != Class<Integer>::getRef(state.function->getSystemState()).getPtr())
									allargsint=false;
								it++;
							}

							uint32_t oppos = state.preloadedcode.size()-1-argcount;
							state.preloadedcode.at(oppos+1).pcode.cachedmultiname3 = name;
							if (state.operandlist.size() > argcount)
							{
								if (canCallFunctionDirect((*it),name,needSuper))
								{
									if (v && asAtomHandler::is<IFunction>(v->var) && asAtomHandler::isInvalid(asAtomHandler::as<IFunction>(v->var)->closure_this))
									{
										if (asAtomHandler::is<SyntheticFunction>(v->var) && (asAtomHandler::as<SyntheticFunction>(v->var)->inClass || fromglobal))
										{
											state.preloadedcode.at(oppos).opcode = (!needResult ? ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS_VOID : ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_MULTIARGS);
											state.preloadedcode.at(oppos).pcode.local2.pos = argcount;
											if (skipcoerce)
												state.preloadedcode.at(oppos).pcode.local2.flags = ABC_OP_COERCED;
											it->fillCode(state,0,oppos,true);
											it->removeArg(state);
											oppos = state.preloadedcode.size()-1-argcount;
											if (fromglobal && v)
											{
												state.preloadedcode.at(oppos).pcode.local2.flags |= ABC_OP_FROMGLOBAL;
												state.preloadedcode.at(oppos).pcode.cachedvar3 = v;
											}
											else
												state.preloadedcode.at(oppos).pcode.cacheobj3 = asAtomHandler::getObject(v->var);
											removeOperands(state,true,lastlocalresulttype,argcount+1);
											if (needResult)
												checkForLocalResult(state,code,2,resulttype,oppos,state.preloadedcode.size()-1);
											removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
											if (needResult)
												typestack.push_back(typestackentry(resulttype,false));
											break;
										}
										else if (asAtomHandler::is<Function>(v->var))
										{
											if (needResult)
												resulttype = asAtomHandler::as<Function>(v->var)->getArgumentDependentReturnType(allargsint);
											state.preloadedcode.at(oppos).opcode = (!needResult ? ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS_VOID : ABC_OP_OPTIMZED_CALLFUNCTIONBUILTIN_MULTIARGS);
											state.preloadedcode.at(oppos).pcode.local2.pos = argcount;
											if (skipcoerce)
												state.preloadedcode.at(oppos).pcode.local2.flags = ABC_OP_COERCED;
											it->fillCode(state,0,oppos,true);
											it->removeArg(state);
											oppos = state.preloadedcode.size()-1-argcount;
											state.preloadedcode.at(oppos).pcode.cacheobj3 = asAtomHandler::getObject(v->var);
											removeOperands(state,true,lastlocalresulttype,argcount+1);
											if (needResult)
												checkForLocalResult(state,code,2,resulttype,oppos,state.preloadedcode.size()-1);
											removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
											if (needResult)
												typestack.push_back(typestackentry(resulttype,false));
											break;
										}
									}
								}
								if (instancevarSlotID)
								{
									assert (!needSuper);
									state.preloadedcode.at(oppos).opcode = (!needResult ? ABC_OP_OPTIMZED_CALLPROPVOID_SLOTVAR_MULTIARGS_CACHED_CALLER:ABC_OP_OPTIMZED_CALLPROPERTY_SLOTVAR_MULTIARGS_CACHED_CALLER);
									state.preloadedcode.at(oppos).pcode.local2.pos = argcount;
									state.preloadedcode.at(oppos).pcode.local2.flags = (skipcoerce ? ABC_OP_COERCED : 0);
									state.preloadedcode.at(oppos).pcode.local3.pos = instancevarSlotID-1;
								}
								else if (classBorrowedSlotID != UINT32_MAX)
								{
									state.preloadedcode.at(oppos).opcode = (!needResult ? ABC_OP_OPTIMZED_CALLPROPVOID_BORROWEDSLOT_MULTIARGS_CACHED_CALLER:ABC_OP_OPTIMZED_CALLPROPERTY_BORROWEDSLOT_MULTIARGS_CACHED_CALLER);
									state.preloadedcode.at(oppos).pcode.local2.pos = argcount;
									state.preloadedcode.at(oppos).pcode.local2.flags = (skipcoerce ? ABC_OP_COERCED : 0);
									state.preloadedcode.at(oppos).pcode.local3.pos = classBorrowedSlotID;
								}
								else
								{
									if (needSuper)
									{
										// no super method found, use standard call
										setUnoptimizedCall(state,typestack,opcode,lastlocalresulttype,argcount,resulttype,needResult,t);
										break;
									}

									state.preloadedcode.at(oppos).opcode = (!needResult ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS_CACHED_CALLER:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS_CACHED_CALLER);
									state.preloadedcode.at(oppos).pcode.local2.pos = argcount;
									state.preloadedcode.at(oppos).pcode.local2.flags = (skipcoerce ? ABC_OP_COERCED : 0);
									if (fromglobal && v)
									{
										state.preloadedcode.at(oppos).pcode.local2.flags |= ABC_OP_FROMGLOBAL;
										state.preloadedcode.at(oppos).pcode.cachedvar3 = v;
									}
								}
								it->removeArg(state);
								oppos = state.preloadedcode.size()-1-argcount;
								state.preloadedcode.push_back(0);
								if (it->fillCode(state,1,state.preloadedcode.size()-1,false))
									state.preloadedcode.at(oppos).opcode++;
								removeOperands(state,true,lastlocalresulttype,argcount+1);
								if (needResult)
									checkForLocalResult(state,code,2,nullptr,oppos,state.preloadedcode.size()-1);
								removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
								if (needResult)
									typestack.push_back(typestackentry(resulttype,false));
								break;
							}
							assert (!needSuper);
							state.preloadedcode.at(oppos).pcode.local2.pos = argcount;
							state.preloadedcode.at(oppos).pcode.local2.flags = (skipcoerce ? ABC_OP_COERCED : 0);
							clearOperands(state,true,lastlocalresulttype);
							if (needResult)
								checkForLocalResult(state,code,1,resulttype,state.preloadedcode.size()-1-argcount,state.preloadedcode.size()-1);
							removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
							if (needResult)
								typestack.push_back(typestackentry(resulttype,false));
						}
						else
						{
							assert (!needSuper);
							if (func)
							{
								state.preloadedcode.push_back((uint32_t)(!needResult ? ABC_OP_OPTIMZED_CALLFUNCTIONVOIDSYNTHETIC_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLFUNCTIONSYNTHETIC_STATICNAME_MULTIARGS));
								state.preloadedcode.back().pcode.local2.pos = argcount;
								if (skipcoerce)
									state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
								state.preloadedcode.at(state.preloadedcode.size()-1).pcode.cacheobj3 = func;
							}
							else
							{
								state.preloadedcode.push_back((uint32_t)(!needResult ? ABC_OP_OPTIMZED_CALLPROPVOID_STATICNAME_MULTIARGS:ABC_OP_OPTIMZED_CALLPROPERTY_STATICNAME_MULTIARGS));
								state.preloadedcode.back().pcode.local2.pos = argcount;
								if (skipcoerce)
									state.preloadedcode.back().pcode.local2.flags = ABC_OP_COERCED;
								if (fromglobal && v)
								{
									state.preloadedcode.back().pcode.local2.flags |= ABC_OP_FROMGLOBAL;
									state.preloadedcode.back().pcode.cachedvar3 = v;
								}
								state.preloadedcode.push_back(0);
								state.preloadedcode.back().pcode.cachedmultiname2 = name;
							}
							clearOperands(state,true,lastlocalresulttype);
							removetypestack(typestack,argcount+state.mi->context->constant_pool.multinames[t].runtimeargs+1);
							if (needResult)
								typestack.push_back(typestackentry(resulttype,false));
						}
						break;
				}
				if (needResult)
				{
					bool skip = false;
					switch (code.peekbyte())
					{
						case 0x73://convert_i
							skip = resulttype == Class<Integer>::getRef(state.function->getSystemState()).getPtr();
							break;
						case 0x74://convert_u
							skip = resulttype == Class<UInteger>::getRef(state.function->getSystemState()).getPtr();
							break;
						case 0x75://convert_d
							skip = resulttype == Class<Number>::getRef(state.function->getSystemState()).getPtr();
							break;
						case 0x76://convert_b
							skip = resulttype == Class<Boolean>::getRef(state.function->getSystemState()).getPtr();
							break;
					}
					if (skip && state.jumptargets.find(code.tellg()+1) == state.jumptargets.end())
						code.readbyte();
				}
				break;
			default:
				setUnoptimizedCall(state,typestack,opcode,lastlocalresulttype,argcount,resulttype,needResult,t);
				break;
		}
	}
	else
#endif
	{
		setUnoptimizedCall(state,typestack,opcode,lastlocalresulttype,argcount,nullptr,needResult,t);
	}
}

}
