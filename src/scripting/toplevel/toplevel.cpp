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

#include <list>
#include <algorithm>
#include <cstring>
#include <sstream>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cerrno>

#include <glib.h>

#include "scripting/abc.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/flash/events/flashevents.h"
#include "swf.h"
#include "compat.h"
#include "scripting/class.h"
#include "exceptions.h"
#include "backends/urlutils.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/ASQName.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/XML.h"

using namespace std;
using namespace lightspark;

void Class<IFunction>::generator(ASWorker* wrk, asAtom& ret,asAtom* args, const unsigned int argslen)
{
	if (argslen > 0)
		createError<EvalError>(wrk,kFunctionConstructorError);
	ret = asAtomHandler::fromObject(getNopFunction());
}

SyntheticFunction::SyntheticFunction(ASWorker* wrk, Class_base* c, method_info* m):IFunction(wrk,c,SUBTYPE_SYNTHETICFUNCTION),mi(m),val(nullptr),simpleGetterOrSetterName(nullptr),fromNewFunction(false),func_scope(NullRef)
{
	if(mi)
		length = mi->numArgs();
	objfreelist = &wrk->freelist_syntheticfunction;
}

IFunction* SyntheticFunction::clone(ASWorker* wrk)
{
	SyntheticFunction* ret = wrk->freelist_syntheticfunction.getObjectFromFreeList()->as<SyntheticFunction>();
	if (!ret)
	{
		ret=new (getClass()->memoryAccount) SyntheticFunction(*this);
	}
	else
	{
		ret->resetCached();
		ret->mi = mi;
		ret->val = val;
		ret->length = length;
		ret->inClass = inClass;
		ret->func_scope = func_scope;
		ret->functionname = functionname;
		ret->fromNewFunction = fromNewFunction;
	}
	ret->setWorker(wrk);
	ret->objfreelist = &wrk->freelist_syntheticfunction;
	ret->subtype = this->subtype;
	ret->isStatic = this->isStatic;
	ret->storedmembercount=0;
	return ret;
}
struct funccallcount
{
	uint32_t callcount;
	uint64_t callduration;
	bool builtin;
	funccallcount(bool _builtin):callcount(0),callduration(0),builtin(_builtin){}
};


#ifdef PROFILING_SUPPORT
std::unordered_map<Class_base*,std::unordered_map<uint32_t,struct funccallcount*>> mapFunctionCallCount;
void lightspark::addFunctionCall(Class_base* cls, uint32_t functionname, uint64_t duration, bool builtin)
{
	auto it = mapFunctionCallCount.find(cls);
	if (it == mapFunctionCallCount.end())
		it = mapFunctionCallCount.insert(make_pair(cls,std::unordered_map<uint32_t,struct funccallcount*>())).first;
	auto it2 = (*it).second.find(functionname);
	if (it2 == (*it).second.end())
		it2 = (*it).second.insert(make_pair(functionname,new funccallcount(builtin))).first;
	it2->second->callcount++;
	it2->second->callduration+=duration;
}
void dumpFunctionCallCount(bool builtinonly = false,uint32_t mincallcount=0, uint32_t mincallduration=0, uint64_t minaverageduration=0)
{
	LOG(LOG_INFO,"function call count:");
	for (auto it = mapFunctionCallCount.begin();it != mapFunctionCallCount.end(); it++)
	{
		for (auto it2 = (*it).second.begin();it2 != (*it).second.end(); it2++)
		{
			if (builtinonly && !(*it2).second->builtin)
				continue;
			if ((*it2).second->callduration>mincallduration && (*it2).second->callcount>mincallcount && ((*it2).second->callduration/(*it2).second->callcount)>minaverageduration)
				LOG(LOG_INFO,(*it).first->toDebugString()<<" "<<getSys()->getStringFromUniqueId((*it2).first)<<":"<<(*it2).second->callduration<<"/"<<(*it2).second->callcount<<"="<<(float((*it2).second->callduration)/float((*it2).second->callcount)));
		}
	}
}
#endif
/**
 * This prepares a new call_context and then executes the ABC bytecode function
 * by ABCVm::executeFunction() or through JIT.
 * It consumes one reference of obj and one of each arg
 */
void SyntheticFunction::call(ASWorker* wrk,asAtom& ret, asAtom& obj, asAtom *args, uint32_t numArgs, bool coerceresult, bool coercearguments)
{
#ifdef PROFILING_SUPPORT
	uint64_t t1 = compat_get_thread_cputime_us();
#endif
	const method_body_info::CODE_STATUS& codeStatus = mi->body->codeStatus;
	if (codeStatus == method_body_info::PRELOADING)
	{
		// this is a call to this method during preloading, it can happen when constructing objects for optimization detection
		return;
	}
	assert(wrk == getWorker());
	auto prev_cur_recursion = wrk->cur_recursion;
	call_context* saved_cc = wrk->incStack(obj,this->functionname);
	if (codeStatus != method_body_info::PRELOADED && codeStatus != method_body_info::USED)
	{
		mi->body->codeStatus = method_body_info::PRELOADING;
		mi->cc.sys = getSystemState();
		mi->cc.worker=wrk;
		mi->cc.exceptionthrown = nullptr;
		ABCVm::preloadFunction(this,wrk);
		mi->body->codeStatus = method_body_info::PRELOADED;
		mi->cc.exec_pos = mi->body->preloadedcode.data();
		mi->cc.locals = new asAtom[mi->body->getReturnValuePos()+1+mi->body->localresultcount];
		mi->cc.stack = new asAtom[mi->body->max_stack+1];
		mi->cc.scope_stack = new asAtom[mi->body->max_scope_depth];
		mi->cc.scope_stack_dynamic = new bool[mi->body->max_scope_depth];
		mi->cc.max_stackp=mi->cc.stack+mi->cc.mi->body->max_stack;
		mi->cc.lastlocal = mi->cc.locals+mi->cc.mi->body->getReturnValuePos()+1+mi->body->localresultcount;
		mi->cc.localslots = new asAtom*[mi->body->localconstantslots.size()+mi->body->getReturnValuePos()+1+mi->body->localresultcount];
		for (uint32_t i = 0; i < uint32_t(mi->body->getReturnValuePos()+1+mi->body->localresultcount); i++)
		{
			mi->cc.localslots[i] = &mi->cc.locals[i];
		}
	}
	if (saved_cc && saved_cc->exceptionthrown)
	{
		if (prev_cur_recursion > 0)
		{
			wrk->decStack(saved_cc);
			return;
		}
		else
		{
			saved_cc->exceptionthrown->decRef();
			saved_cc->exceptionthrown = nullptr;
		}
	}

	/* resolve argument and return types */
	if(!mi->returnType)
	{
		checkParamTypes();
	}

	if(numArgs < mi->numArgs()-mi->numOptions())
	{
		/* Not enough arguments provided.
		 * We throw if this is a method.
		 * We won't throw if all arguments are of 'Any' type.
		 * This is in accordance with the proprietary player. */
		if(isMethod() || mi->hasExplicitTypes)
		{
			createError<ArgumentError>(wrk,kWrongArgumentCountError,
						  asAtomHandler::toObject(obj,wrk)->getClassName(),
						  Integer::toString(mi->numArgs()-mi->numOptions()),
						  Integer::toString(numArgs));
			wrk->decStack(saved_cc);
			return;
		}
	}
	if ((isMethod() || mi->hasExplicitTypes) && numArgs > mi->numArgs() && !mi->needsArgs() && !mi->needsRest() && !mi->hasOptional())
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError,getSystemState()->getStringFromUniqueId(functionname),Integer::toString(mi->numArgs()),Integer::toString(numArgs));
		wrk->decStack(saved_cc);
		return;
	}

#ifdef LLVM_ENABLED
	//Temporarily disable JITting
	const uint32_t jit_hit_threshold=20;
	if(getSystemState()->useJit && mi->body->exceptions.size()==0 && ((mi->body->hit_count>=jit_hit_threshold && codeStatus==method_body_info::OPTIMIZED) || getSystemState()->useInterpreter==false))
	{
		//We passed the hot function threshold, synt the function
		val=mi->synt_method(getSystemState());
		assert(val);
	}
	++mi->body->hit_count;
#endif

	//Prepare arguments
	uint32_t args_len=mi->numArgs();
	int passedToLocals=imin(numArgs,args_len);
	uint32_t passedToRest=(numArgs > args_len)?(numArgs-mi->numArgs()):0;

	/* setup argumentsArray if needed */
	Array* argumentsArray = nullptr;
	if(mi->needsArgs())
	{
		//The arguments does not contain default values of optional parameters,
		//i.e. f(a,b=3) called as f(7) gives arguments = { 7 }
		argumentsArray=Class<Array>::getInstanceSNoArgs(wrk);
		argumentsArray->resize(numArgs);
		for(uint32_t j=0;j<numArgs;j++)
		{
			argumentsArray->set(j,args[j]);
		}
		//Add ourself as the callee property
		
		// TODO not really sure how to set the closure of the callee
		// for now we just set it to the closure of this or the class, if no closure is present
		if (this->inClass)
			argumentsArray->setVariableByQName("callee","",this->bind(this->closure_this ? this->closure_this : this->inClass,wrk),DECLARED_TRAIT);
		else
		{
			incRef();
			argumentsArray->setVariableByQName("callee","",this,DECLARED_TRAIT);
		}
	}

	/* setup call_context */
	bool recursive_call = codeStatus == method_body_info::USED;
	call_context* cc = nullptr;
	if (recursive_call)
	{
		cc = new call_context(mi);
		cc->sys = getSystemState();
		cc->worker=wrk;
		cc->exceptionthrown = nullptr;
		cc->locals= g_newa(asAtom, mi->body->getReturnValuePos()+1+mi->body->localresultcount);
		cc->stack = g_newa(asAtom, mi->body->max_stack+1);
		cc->scope_stack=g_newa(asAtom, mi->body->max_scope_depth);
		cc->scope_stack_dynamic=g_newa(bool, mi->body->max_scope_depth);
		cc->max_stackp=cc->stackp+cc->mi->body->max_stack;
		cc->lastlocal = cc->locals+mi->body->getReturnValuePos()+1+mi->body->localresultcount;
		cc->localslots = g_newa(asAtom*,mi->body->localconstantslots.size()+mi->body->getReturnValuePos()+1+mi->body->localresultcount);
		for (uint32_t i = 0; i < uint32_t(mi->body->getReturnValuePos()+1+mi->body->localresultcount); i++)
		{
			cc->localslots[i] = &cc->locals[i];
		}
	}
	else
	{
		cc = &mi->cc;
		assert(cc->worker==wrk);
	}
	cc->exec_pos = mi->body->preloadedcode.data();
	cc->parent_scope_stack=func_scope.getPtr();
	cc->defaultNamespaceUri = saved_cc ? saved_cc->defaultNamespaceUri : (uint32_t)BUILTIN_STRINGS::EMPTY;
	cc->function = this;
	cc->stackp = cc->stack;
	if (wrk->currentCallContext != nullptr)
		cc->explicitConstruction = wrk->currentCallContext->explicitConstruction;

	wrk->callStack.push_back(cc);
	/* Set the current global object, each script in each DoABCTag has its own */
	wrk->currentCallContext = cc;

	cc->locals[0]=obj;

	if (coercearguments)
	{
		/* coerce arguments to expected types */
		auto itpartype = mi->paramTypes.begin();
		asAtom* argp = args;
		asAtom* lastlocalpasssed = cc->locals+1+passedToLocals;
		for(asAtom* i=cc->locals+1;i< lastlocalpasssed;++i)
		{
			*i = *argp;
			if ((*itpartype) == asAtomHandler::getClass(*i,getSystemState()))
			{
				argp++;
				itpartype++;
				continue;
			}
			if ((*itpartype++)->coerce(getInstanceWorker(),*i))
				ASATOM_DECREF_POINTER(argp);
			argp++;
		}
	}
	else if (args)
		memcpy(cc->locals+1,args,passedToLocals*sizeof(asAtom));

	//Fill missing parameters until optional parameters begin
	//like fun(a,b,c,d=3,e=5) called as fun(1,2) becomes
	//locals = {this, 1, 2, Undefined, 3, 5}
	for(uint32_t i=passedToLocals;i<args_len;++i)
	{
		int iOptional = mi->numOptions()-args_len+i;
		if(iOptional >= 0)
		{
			mi->getOptional(cc->locals[i+1],iOptional);
			mi->paramTypes[i]->coerce(wrk,cc->locals[i+1]);
		} else {
			assert(mi->paramTypes[i] == Type::anyType);
			cc->locals[i+1]=asAtomHandler::undefinedAtom;
		}
	}
	memset(cc->locals+args_len+1,ATOMTYPE_UNDEFINED_BIT,(mi->body->getReturnValuePos()+mi->body->localresultcount-(args_len))*sizeof(asAtom));
	if (mi->body->localsinitialvalues)
		memcpy(cc->locals+args_len+1,mi->body->localsinitialvalues,(mi->body->local_count-(args_len+1))*sizeof(asAtom));

	if(mi->needsArgs())
	{
		assert_and_throw(cc->mi->body->local_count>args_len);
		cc->locals[args_len+1]=asAtomHandler::fromObject(argumentsArray);
		cc->argarrayposition=args_len+1;
	}
	else if(mi->needsRest())
	{
		assert_and_throw(argumentsArray==nullptr);
		Array* rest=Class<Array>::getInstanceSNoArgs(wrk);
		rest->resize(passedToRest);
		//Give the reference of the other args to an array
		for(uint32_t j=0;j<passedToRest;j++)
		{
			rest->set(j,args[passedToLocals+j]);
		}

		assert_and_throw(cc->mi->body->local_count>args_len);
		cc->locals[args_len+1]=asAtomHandler::fromObject(rest);
		cc->argarrayposition= args_len+1;
	}
	//Parameters are ready

	//obtain a local reference to this function, as it may delete itself
	bool hasstoredmember = false;
	if (!isMethod() && this->fromNewFunction)
	{
		this->incRef();
		if (this->storedmembercount == 0)
		{
			hasstoredmember=true;
			this->addStoredMember();
		}
	}

#ifndef NDEBUG
	if (wrk->isPrimordial)
		Log::calls_indent++;
#endif
	while (true)
	{
		if(!mi->body->exceptions.empty() || (val==nullptr && getSystemState()->useInterpreter))
		{
			if(codeStatus == method_body_info::OPTIMIZED && getSystemState()->useFastInterpreter)
			{
				//This is a mildy hot function, execute it using the fast interpreter
				ret=asAtomHandler::fromObject(ABCVm::executeFunctionFast(this,cc,asAtomHandler::toObject(obj,wrk)));
			}
			else
			{
				// returnvalue
				cc->locals[mi->body->getReturnValuePos()]=asAtomHandler::invalidAtom;
				// fill additional locals with slots of objects that don't change during execution
				int i = mi->body->getReturnValuePos()+1+mi->body->localresultcount;
				for (auto it = mi->body->localconstantslots.begin(); it != mi->body->localconstantslots.end(); it++)
				{
					assert(it->local_pos < (mi->numArgs()-mi->numOptions())+1);
					if (asAtomHandler::isObject(cc->locals[it->local_pos]))
					{
						ASObject* o = asAtomHandler::getObjectNoCheck(cc->locals[it->local_pos]);
						cc->localslots[i] = &(o->getSlotVar(it->slot_number)->var);
					}
					else
						cc->localslots[i] = &asAtomHandler::nullAtom;
					i++;
				}
				//Switch the codeStatus to USED to make sure the method will not be optimized while being used
				const method_body_info::CODE_STATUS oldCodeStatus = codeStatus;
				mi->body->codeStatus = method_body_info::USED;
				
				if (mi->needsscope && cc->exec_pos == mi->body->preloadedcode.data())
				{
					ASObject* o = asAtomHandler::getObject(obj);
					if (o)
					{
						o->incRef();
						o->addStoredMember();
					}
					cc->scope_stack[0] = obj;
					cc->scope_stack_dynamic[0] = false;
					cc->curr_scope_stack++;
				}
				//This is not a hot function, execute it using the interpreter
				ABCVm::executeFunction(cc);
				//Restore the previous codeStatus
				mi->body->codeStatus = oldCodeStatus;
			}
		}
		else
			ret=asAtomHandler::fromObject(val(cc));
		if (cc->exceptionthrown)
		{
			ASObject* excobj = cc->exceptionthrown;
			unsigned int pos = cc->exec_pos-cc->mi->body->preloadedcode.data();
			bool no_handler = true;
			LOG_CALL("got an " << excobj->toDebugString());
			LOG_CALL("pos=" << pos);
			for (unsigned int i=0;i<mi->body->exceptions.size();i++)
			{
				exception_info_abc& exc=mi->body->exceptions[i];
				if (pos < exc.from || pos > exc.to)
					continue;
				bool ok = false;
				if (!exc.exc_class)
				{
					multiname* name=mi->context->getMultiname(exc.exc_type, nullptr);
					if(name->name_s_id == BUILTIN_STRINGS::ANY)
						ok = true;
					else
					{
						Type* t = Type::getTypeFromMultiname(name,mi->context,true);
						exc.exc_class = (Class_base*)dynamic_cast<const Class_base*>(t);
					}
				}
				if (!ok && exc.exc_class)
					ok = excobj->getClass() && excobj->getClass()->isSubClass(exc.exc_class);
				LOG_CALL("f=" << exc.from << " t=" << exc.to << " type=" << mi->context->getMultiname(exc.exc_type, nullptr));
				if (ok)
				{
					LOG_CALL("Exception caught in function "<<getSystemState()->getStringFromUniqueId(functionname) << " with closure "<< asAtomHandler::toDebugString(obj));
					no_handler = false;
					cc->exec_pos = mi->body->preloadedcode.data()+exc.target;
					cc->runtime_stack_clear();
					*(cc->stackp++)=asAtomHandler::fromObject(excobj);
					while (cc->curr_scope_stack)
					{
						--cc->curr_scope_stack;
						LOG_CALL("scopestack exception:"<<asAtomHandler::toDebugString(cc->scope_stack[cc->curr_scope_stack]));
						if (asAtomHandler::isObject(cc->scope_stack[cc->curr_scope_stack]))
							asAtomHandler::getObjectNoCheck(cc->scope_stack[cc->curr_scope_stack])->removeStoredMember();
					}
					break;
				}
			}
			cc->exceptionthrown=nullptr;
			if (no_handler)
			{
				if (saved_cc)
					saved_cc->exceptionthrown=excobj;
				else
					throw excobj;
				break;
			}
			continue;
		}
		break;
	}
	wrk->decStack(saved_cc);
	wrk->callStack.pop_back();
#ifndef NDEBUG
	if (wrk->isPrimordial)
		Log::calls_indent--;
#endif

	ret.uintval = cc->locals[mi->body->getReturnValuePos()].uintval;
	ASATOM_INCREF(ret);

	if (coerceresult || mi->needscoerceresult)
	{
		asAtom v = ret;
		if (asAtomHandler::isValid(ret) && mi->returnType->coerce(getInstanceWorker(),ret))
			ASATOM_DECREF(v);
	}

	//The stack may be not clean, is this a programmer/compiler error?
	if(cc->stackp != cc->stack)
	{
		LOG(LOG_ERROR,"Stack not clean at the end of function:"<<getSystemState()->getStringFromUniqueId(this->functionname));
		while(cc->stackp != cc->stack)
		{
			--cc->stackp;
			ASATOM_DECREF_POINTER(cc->stackp);
		}
	}
	for(asAtom* i=cc->locals+1;i< cc->lastlocal;++i)
	{
		LOG_CALL("locals:"<<asAtomHandler::toDebugString(*i));
		ASATOM_DECREF_POINTER(i);
	}
	if (cc->locals[0].uintval != obj.uintval)
	{
		LOG_CALL("locals0:"<<asAtomHandler::toDebugString(cc->locals[0]));
		ASATOM_DECREF_POINTER(cc->locals);
	}

	asAtom* lastscope=cc->scope_stack+cc->curr_scope_stack;
	for(asAtom* i=cc->scope_stack;i< lastscope;++i)
	{
		LOG_CALL("scopestack:"<<asAtomHandler::toDebugString(*i));
		if (asAtomHandler::isObject(*i))
			asAtomHandler::getObjectNoCheck(*i)->removeStoredMember();
	}
	if(!mi->needsRest())
	{
		for(uint32_t j=0;j<passedToRest;j++)
		{
			ASATOM_DECREF(args[passedToLocals+j]);
		}
	}
	cc->curr_scope_stack=0;
	if (!isMethod() && this->fromNewFunction)
	{
		//free local ref
		if (hasstoredmember)
			this->removeStoredMember();
		else
			this->decRef();
	}
	if (recursive_call)
		delete cc;
#ifdef PROFILING_SUPPORT
	uint64_t t2 = compat_get_thread_cputime_us();
	if (inClass)
		addFunctionCall(inClass,functionname,t2-t1,false);
#endif
}

bool SyntheticFunction::destruct()
{
	// the scope may contain objects that have pointers to this function
	// which may lead to calling destruct() recursively
	// so we have to make sure to cleanup the func_scope only once
	if (!func_scope.isNull() && fromNewFunction)
	{
		for (auto it = func_scope->scope.begin();it != func_scope->scope.end(); it++)
		{
			ASObject* o = asAtomHandler::getObject(it->object);
			if (o && !o->is<Global>())
				o->removeStoredMember();
		}
	}
	func_scope.reset();
	val = nullptr;
	mi = nullptr;
	fromNewFunction = false;
	return IFunction::destruct();
}

void SyntheticFunction::finalize()
{
	// the scope may contain objects that have pointers to this function
	// which may lead to calling destruct() recursively
	// so we have to make sure to cleanup the func_scope only once
	if (!func_scope.isNull() && fromNewFunction)
	{
		for (auto it = func_scope->scope.begin();it != func_scope->scope.end(); it++)
		{
			ASObject* o = asAtomHandler::getObject(it->object);
			if (o && !o->is<Global>())
				o->removeStoredMember();
		}
	}
	func_scope.reset();
	IFunction::finalize();
}

void SyntheticFunction::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	IFunction::prepareShutdown();
	if (!func_scope.isNull())
	{
		for (auto it = func_scope->scope.begin();it != func_scope->scope.end(); it++)
		{
			ASObject* o = asAtomHandler::getObject(it->object);
			if (o)
				o->prepareShutdown();
		}
	}
}
bool SyntheticFunction::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = IFunction::countCylicMemberReferences(gcstate);
	if (!func_scope.isNull())
	{
		for (auto it = func_scope->scope.begin();it != func_scope->scope.end(); it++)
		{
			if (asAtomHandler::isObject(it->object))
				ret = asAtomHandler::getObjectNoCheck(it->object)->countAllCylicMemberReferences(gcstate) || ret;
		}
	}
	return ret;
}

bool SyntheticFunction::isEqual(ASObject *r)
{
	return r == this || 
			(this->inClass && r->is<SyntheticFunction>() && 
			 this->val == r->as<SyntheticFunction>()->val &&
			 this->mi == r->as<SyntheticFunction>()->mi &&
			 this->inClass == r->as<SyntheticFunction>()->inClass);
}

Class_base *SyntheticFunction::getReturnType(bool opportunistic)
{
	if (!mi->returnType)
		checkParamTypes(opportunistic);
	return (Class_base*)dynamic_cast<const Class_base*>(mi->returnType);
}

void SyntheticFunction::checkParamTypes(bool opportunistic)
{
	assert(!mi->returnType);
	mi->hasExplicitTypes = false;
	mi->paramTypes.reserve(mi->numArgs());
	mi->paramTypes.clear();
	for(size_t i=0;i < mi->numArgs();++i)
	{
		Type* t = Type::getTypeFromMultiname(mi->paramTypeName(i), mi->context,opportunistic);
		if (!t)
			return;
		mi->paramTypes.push_back(t);
		if(t != Type::anyType)
			mi->hasExplicitTypes = true;
	}

	Type* t = Type::getTypeFromMultiname(mi->returnTypeName(), mi->context,opportunistic);
	mi->returnType = t;
}

bool SyntheticFunction::canSkipCoercion(int param, Class_base *cls)
{
	assert(mi->returnType && param < (int)mi->numArgs());
	
	return mi->paramTypes[param] == cls || mi->paramTypes[param] == Type::anyType || mi->paramTypes[param] == Type::voidType || (
		(cls == Class<Number>::getRef(getSystemState()).getPtr() ||
			cls == Class<Integer>::getRef(getSystemState()).getPtr() ||
			cls == Class<UInteger>::getRef(getSystemState()).getPtr()) &&
		(mi->paramTypes[param] == Class<Number>::getRef(getSystemState()).getPtr() ||
			mi->paramTypes[param] == Class<Integer>::getRef(getSystemState()).getPtr() ||
			mi->paramTypes[param] == Class<UInteger>::getRef(getSystemState()).getPtr()));
}

IFunction* Function::clone(ASWorker* wrk)
{
	Function* ret = wrk->freelist[getClass()->classID].getObjectFromFreeList()->as<Function>();
	if (!ret)
		ret=new (getClass()->memoryAccount) Function(*this);
	else
	{
		ret->val_atom = val_atom;
		ret->length = length;
		ret->inClass = inClass;
		ret->functionname = functionname;
		ret->resetCached();
		ret->returnType = returnType;
		ret->returnTypeAllArgsInt = returnTypeAllArgsInt;
	}
	ret->setWorker(wrk);
	ret->objfreelist = &wrk->freelist[getClass()->classID];
	ret->subtype = this->subtype;
	ret->isStatic = this->isStatic;
	ret->storedmembercount=0;
	return ret;
}

void Function::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	if (prototype)
		prototype->prepareShutdown();
	if (returnType)
		returnType->prepareShutdown();
	if (returnTypeAllArgsInt)
		returnTypeAllArgsInt->prepareShutdown();
	IFunction::prepareShutdown();
}

bool Function::isEqual(ASObject* r)
{
	if (!r->is<Function>())
		return false;
	Function* f=r->as<Function>();
	return (val_atom==f->val_atom);
}

Class_base *Function::getReturnType(bool opportunistic)
{
	if (!returnType && inClass && this->functionname)
		LOG(LOG_NOT_IMPLEMENTED,"no returntype given for "<<inClass->toDebugString()<<" "<<getSystemState()->getStringFromUniqueId(this->functionname));
	return returnType;
}

Class_base *Function::getArgumentDependentReturnType(bool allargsint)
{
	if (!returnType && inClass && this->functionname)
		LOG(LOG_NOT_IMPLEMENTED,"no arg dependent returntype given for "<<inClass->toDebugString()<<" "<<getSystemState()->getStringFromUniqueId(this->functionname));
	return allargsint && returnTypeAllArgsInt ? returnTypeAllArgsInt : returnType;
}

void ASNop(asAtom& ret, ASWorker* wrk, asAtom& , asAtom* args, const unsigned int argslen)
{
	asAtomHandler::setUndefined(ret);
}

IFunction* Class<IFunction>::getNopFunction()
{
	IFunction* ret=new (this->memoryAccount) Function(getInstanceWorker(),this, ASNop);
	//Similarly to newFunction, we must create a prototype object
	ret->prototype = _MR(new_asobject(getInstanceWorker()));
	ret->incRef();
	ret->prototype->setVariableByQName("constructor","",ret,DECLARED_TRAIT);
	return ret;
}

void Class<IFunction>::getInstance(ASWorker* worker,asAtom& ret,bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass)
{
	if (argslen > 0)
		createError<EvalError>(worker,kFunctionConstructorError);
	ASObject* res = getNopFunction();
	if (construct)
		res->setConstructIndicator();
	ret = asAtomHandler::fromObject(res);
}

Class<IFunction>* Class<IFunction>::getClass(SystemState* sys)
{
	uint32_t classId=ClassName<IFunction>::id;
	Class<IFunction>* ret=nullptr;
	SystemState* s = sys ? sys : getSys();
	Class_base** retAddr=&s->builtinClasses[classId];
	if(*retAddr==nullptr)
	{
		//Create the class
		MemoryAccount* m = s->allocateMemoryAccount(ClassName<IFunction>::name);
		ret=new (m) Class<IFunction>(m,ClassName<IFunction>::id);
		ret->classID=classId;
		ret->setWorker(sys->worker);
		ret->setSystemState(s);
		//This function is called from Class<ASObject>::getRef(),
		//so the Class<ASObject> we obtain will not have any
		//declared methods yet! Therefore, set super will not copy
		//up any borrowed traits from there. We do that by ourself.
		ret->setSuper(Class<ASObject>::getRef(s));
		//The prototype for Function seems to be a function object. Use the special FunctionPrototype
		ret->prototype = _MNR(new_functionPrototype(sys->worker,ret, ret->super->prototype));
		ret->prototype->getObj()->setVariableByQName("constructor","",ret,DECLARED_TRAIT);
		ret->prototype->getObj()->setConstructIndicator();
		ret->incRef();
		*retAddr = ret;

		//we cannot use sinit, as we need to setup 'this_class' before calling
		//addPrototypeGetter and setDeclaredMethodByQName.
		//Thus we make sure that everything is in order when getFunction() below is called
		ret->addPrototypeGetter();
		IFunction::sinit(ret);
		ret->constructorprop = _NR<ObjectConstructor>(new_objectConstructor(ret,ret->length));

		ret->addConstructorGetter();

		ret->setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(ret->getSystemState(),IFunction::_getter_prototype),GETTER_METHOD,true);
		ret->setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(ret->getSystemState(),IFunction::_setter_prototype),SETTER_METHOD,true);
	}
	else
		ret=static_cast<Class<IFunction>*>(*retAddr);

	return ret;
}

ASFUNCTIONBODY_ATOM(lightspark,eval)
{
	// eval is not allowed in AS3, but an exception should be thrown
	createError<EvalError>(wrk,0,"EvalError");
}

ASFUNCTIONBODY_ATOM(lightspark,parseInt)
{
	tiny_string str;
	int radix;
	ARG_CHECK(ARG_UNPACK (str, "") (radix, 0));

	if((radix != 0) && ((radix < 2) || (radix > 36)))
	{
		asAtomHandler::setNumber(ret,wrk,numeric_limits<double>::quiet_NaN());
		return;
	}

	const char* cur=str.raw_buf();
	number_t res;
	bool valid=Integer::fromStringFlashCompatible(cur,res,radix);

	if(valid==false)
		asAtomHandler::setNumber(ret,wrk,numeric_limits<double>::quiet_NaN());
	else if (res < INT32_MAX && res > INT32_MIN)
		asAtomHandler::setInt(ret,wrk,(int32_t)res);
	else
		asAtomHandler::setNumber(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(lightspark,parseFloat)
{
	tiny_string str;
	ARG_CHECK(ARG_UNPACK (str, ""));

	asAtomHandler::setNumber(ret,wrk,parseNumber(str,wrk->getSystemState()->getSwfVersion()<11));
}
number_t lightspark::parseNumber(const tiny_string str, bool useoldversion)
{
	const char *p;
	char *end;

	// parsing of hex numbers is not allowed
	char* p1 = str.strchr('x');
	if (p1) *p1='y';
	p1 = str.strchr('X');
	if (p1) *p1='Y';

	p=str.raw_buf();
	if (useoldversion)
	{
		tiny_string s = p;
		tiny_string s2;
		bool found = false;
		for (uint32_t i = s.numChars(); i>0 ; i--)
		{
			if (s.charAt(i-1) == '.')
			{
				if (!found)
				{
					tiny_string s3;
					s3 += s.charAt(i-1);
					s2 = s3 + s2;
					found = true;
				}
			}
			else
			{
				tiny_string s3;
				s3 += s.charAt(i-1);
				s2 = s3 + s2;
			}
		}
		p= s2.raw_buf();
		double d=strtod(p, &end);

		if (end==p)
			return numeric_limits<double>::quiet_NaN();
		return d;
	}
	double d=strtod(p, &end);

	if (end==p)
		return numeric_limits<double>::quiet_NaN();

	return d;
}

ASFUNCTIONBODY_ATOM(lightspark,isNaN)
{
	if(argslen==0)
		asAtomHandler::setBool(ret,true);
	else if(asAtomHandler::isUndefined(args[0]))
		asAtomHandler::setBool(ret,true);
	else if(asAtomHandler::isInteger(args[0]))
		asAtomHandler::setBool(ret,false);
	else if(asAtomHandler::isBool(args[0]))
		asAtomHandler::setBool(ret,false);
	else if(asAtomHandler::isNull(args[0]))
		asAtomHandler::setBool(ret,false); // because Number(null) == 0
	else
		asAtomHandler::setBool(ret,std::isnan(asAtomHandler::toNumber(args[0])));
}

ASFUNCTIONBODY_ATOM(lightspark,isFinite)
{
	if(argslen==0)
		asAtomHandler::setBool(ret,false);
	else
		asAtomHandler::setBool(ret,isfinite(asAtomHandler::toNumber(args[0])));
}

ASFUNCTIONBODY_ATOM(lightspark,encodeURI)
{
	tiny_string str;
	ARG_CHECK(ARG_UNPACK (str, "undefined"));
	ret = asAtomHandler::fromObject(abstract_s(wrk,URLInfo::encode(str, URLInfo::ENCODE_URI)));
}

ASFUNCTIONBODY_ATOM(lightspark,decodeURI)
{
	tiny_string str;
	ARG_CHECK(ARG_UNPACK (str, "undefined"));
	ret = asAtomHandler::fromObject(abstract_s(wrk,URLInfo::decode(str, URLInfo::ENCODE_URI)));
}

ASFUNCTIONBODY_ATOM(lightspark,encodeURIComponent)
{
	tiny_string str;
	ARG_CHECK(ARG_UNPACK (str, "undefined"));
	ret = asAtomHandler::fromObject(abstract_s(wrk,URLInfo::encode(str, URLInfo::ENCODE_URICOMPONENT)));
}

ASFUNCTIONBODY_ATOM(lightspark,decodeURIComponent)
{
	tiny_string str;
	ARG_CHECK(ARG_UNPACK (str, "undefined"));
	ret = asAtomHandler::fromObject(abstract_s(wrk,URLInfo::decode(str, URLInfo::ENCODE_URICOMPONENT)));
}

ASFUNCTIONBODY_ATOM(lightspark,escape)
{
	tiny_string str;
	ARG_CHECK(ARG_UNPACK (str, "undefined"));
	if (argslen > 0 && asAtomHandler::is<Undefined>(args[0]))
		ret = asAtomHandler::fromString(wrk->getSystemState(),"null");
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,URLInfo::encode(str, URLInfo::ENCODE_ESCAPE)));
}

ASFUNCTIONBODY_ATOM(lightspark,unescape)
{
	tiny_string str;
	ARG_CHECK(ARG_UNPACK (str, "undefined"));
	if (argslen > 0 && asAtomHandler::is<Undefined>(args[0]))
		ret = asAtomHandler::fromString(wrk->getSystemState(),"null");
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,URLInfo::decode(str, URLInfo::ENCODE_ESCAPE)));
}

ASFUNCTIONBODY_ATOM(lightspark,print)
{
	Log::print(asAtomHandler::toString(args[0],wrk));
	ret = asAtomHandler::undefinedAtom;
}

ASFUNCTIONBODY_ATOM(lightspark,trace)
{
	stringstream s;
	for(uint32_t i = 0; i< argslen;i++)
	{
		if(i > 0)
			s << " ";

		s << asAtomHandler::toString(args[i],wrk);
	}
	Log::print(s.str());
	ret = asAtomHandler::undefinedAtom;
}
ASFUNCTIONBODY_ATOM(lightspark,AVM1_updateAfterEvent)
{
	wrk->getSystemState()->stage->forceInvalidation();
}

ASFUNCTIONBODY_ATOM(lightspark,AVM1_ASSetPropFlags)
{
	_NR<ASObject> o;
	tiny_string names;
	uint32_t set_true;
	uint32_t set_false;
	ARG_CHECK(ARG_UNPACK(o)(names)(set_true)(set_false,0));
	if (o.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,Integer::toString(0));
		return;
	}
	std::list<uint32_t> nameIDlist;
	if (asAtomHandler::isNull(args[1]))
	{
		for (uint32_t i = 0; i < o->numVariables(); i++)
		{
			nameIDlist.push_back(o->getNameAt(i));
		}
	}
	else
	{
		std::list<tiny_string> namelist = names.split((uint32_t)',');
		for (auto it = namelist.begin();it != namelist.end(); it++)
		{
			nameIDlist.push_back(wrk->getSystemState()->getUniqueStringId(*it));
		}
	}
	for (auto it = nameIDlist.begin();it != nameIDlist.end(); it++)
	{
		if ((*it) == BUILTIN_STRINGS::PROTOTYPE || (*it)==BUILTIN_STRINGS::STRING_PROTO)
			continue;
		multiname name(nullptr);
		name.name_type=multiname::NAME_STRING;
		name.name_s_id=*it;
		name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		name.isAttribute=false;
		if (set_false & 0x01) // dontEnum
			o->setIsEnumerable(name, true);
		if (set_true & 0x01) // dontEnum
			o->setIsEnumerable(name, false);
	}
	if (set_true & ~0x0001)
		LOG(LOG_NOT_IMPLEMENTED,"AVM1_ASSetPropFlags with set_true flags "<<hex<<set_true);
	if (set_false & ~0x0001)
		LOG(LOG_NOT_IMPLEMENTED,"AVM1_ASSetPropFlags with set_false flags "<<hex<<set_false);
/* definition from gnash:
	enum Flags {
		/// Protect from enumeration
		dontEnum	= 1 << 0, // 1
		/// Protect from deletion
		dontDelete	= 1 << 1, // 2
		/// Protect from assigning a value
		readOnly	= 1 << 2, // 4
		/// Only visible by VM initialized for version 6 or higher 
		onlySWF6Up 	= 1 << 7, // 128
		/// Ignore in SWF6-initialized VM
		ignoreSWF6	= 1 << 8, // 256
		/// Only visible by VM initialized for version 7 or higher 
		onlySWF7Up 	= 1 << 10, // 1024
		/// Only visible by VM initialized for version 8 or higher 
		onlySWF8Up	= 1 << 12, // 4096
		/// Only visible by VM initialized for version 9 or higher 
		onlySWF9Up	= 1 << 13 // 8192
*/
}
bool lightspark::isXMLName(ASWorker* wrk, asAtom& obj)
{
	tiny_string name;

	if(asAtomHandler::isQName(obj))
	{
		ASQName *q=asAtomHandler::as<ASQName>(obj);
		name=wrk->getSystemState()->getStringFromUniqueId(q->getLocalName());
	}
	else if(asAtomHandler::isUndefined(obj) ||
		asAtomHandler::isNull(obj))
		name="";
	else
		name=asAtomHandler::toString(obj,wrk);

	if(name.empty())
		return false;

	// http://www.w3.org/TR/2006/REC-xml-names-20060816/#NT-NCName
	// Note: Flash follows the second edition (20060816) of the
	// standard. The character range definitions were changed in
	// the newer edition.
	#define NC_START_CHAR(x) \
	  ((0x0041 <= x && x <= 0x005A) || x == 0x5F || \
	  (0x0061 <= x && x <= 0x007A) || (0x00C0 <= x && x <= 0x00D6) || \
	  (0x00D8 <= x && x <= 0x00F6) || (0x00F8 <= x && x <= 0x00FF) || \
	  (0x0100 <= x && x <= 0x0131) || (0x0134 <= x && x <= 0x013E) || \
	  (0x0141 <= x && x <= 0x0148) || (0x014A <= x && x <= 0x017E) || \
	  (0x0180 <= x && x <= 0x01C3) || (0x01CD <= x && x <= 0x01F0) || \
	  (0x01F4 <= x && x <= 0x01F5) || (0x01FA <= x && x <= 0x0217) || \
	  (0x0250 <= x && x <= 0x02A8) || (0x02BB <= x && x <= 0x02C1) || \
	  x == 0x0386 || (0x0388 <= x && x <= 0x038A) || x == 0x038C || \
	  (0x038E <= x && x <= 0x03A1) || (0x03A3 <= x && x <= 0x03CE) || \
	  (0x03D0 <= x && x <= 0x03D6) || x == 0x03DA || x == 0x03DC || \
	  x == 0x03DE || x == 0x03E0 || (0x03E2 <= x && x <= 0x03F3) || \
	  (0x0401 <= x && x <= 0x040C) || (0x040E <= x && x <= 0x044F) || \
	  (0x0451 <= x && x <= 0x045C) || (0x045E <= x && x <= 0x0481) || \
	  (0x0490 <= x && x <= 0x04C4) || (0x04C7 <= x && x <= 0x04C8) || \
	  (0x04CB <= x && x <= 0x04CC) || (0x04D0 <= x && x <= 0x04EB) || \
	  (0x04EE <= x && x <= 0x04F5) || (0x04F8 <= x && x <= 0x04F9) || \
	  (0x0531 <= x && x <= 0x0556) || x == 0x0559 || \
	  (0x0561 <= x && x <= 0x0586) || (0x05D0 <= x && x <= 0x05EA) || \
	  (0x05F0 <= x && x <= 0x05F2) || (0x0621 <= x && x <= 0x063A) || \
	  (0x0641 <= x && x <= 0x064A) || (0x0671 <= x && x <= 0x06B7) || \
	  (0x06BA <= x && x <= 0x06BE) || (0x06C0 <= x && x <= 0x06CE) || \
	  (0x06D0 <= x && x <= 0x06D3) || x == 0x06D5 || \
	  (0x06E5 <= x && x <= 0x06E6) || (0x0905 <= x && x <= 0x0939) || \
	  x == 0x093D || (0x0958 <= x && x <= 0x0961) || \
	  (0x0985 <= x && x <= 0x098C) || (0x098F <= x && x <= 0x0990) || \
	  (0x0993 <= x && x <= 0x09A8) || (0x09AA <= x && x <= 0x09B0) || \
	  x == 0x09B2 || (0x09B6 <= x && x <= 0x09B9) || \
	  (0x09DC <= x && x <= 0x09DD) || (0x09DF <= x && x <= 0x09E1) || \
	  (0x09F0 <= x && x <= 0x09F1) || (0x0A05 <= x && x <= 0x0A0A) || \
	  (0x0A0F <= x && x <= 0x0A10) || (0x0A13 <= x && x <= 0x0A28) || \
	  (0x0A2A <= x && x <= 0x0A30) || (0x0A32 <= x && x <= 0x0A33) || \
	  (0x0A35 <= x && x <= 0x0A36) || (0x0A38 <= x && x <= 0x0A39) || \
	  (0x0A59 <= x && x <= 0x0A5C) || x == 0x0A5E || \
	  (0x0A72 <= x && x <= 0x0A74) || (0x0A85 <= x && x <= 0x0A8B) || \
	  x == 0x0A8D || (0x0A8F <= x && x <= 0x0A91) || \
	  (0x0A93 <= x && x <= 0x0AA8) || (0x0AAA <= x && x <= 0x0AB0) || \
	  (0x0AB2 <= x && x <= 0x0AB3) || (0x0AB5 <= x && x <= 0x0AB9) || \
	  x == 0x0ABD || x == 0x0AE0 || (0x0B05 <= x && x <= 0x0B0C) || \
	  (0x0B0F <= x && x <= 0x0B10) || (0x0B13 <= x && x <= 0x0B28) || \
	  (0x0B2A <= x && x <= 0x0B30) || (0x0B32 <= x && x <= 0x0B33) || \
	  (0x0B36 <= x && x <= 0x0B39) || x == 0x0B3D || \
	  (0x0B5C <= x && x <= 0x0B5D) || (0x0B5F <= x && x <= 0x0B61) || \
	  (0x0B85 <= x && x <= 0x0B8A) || (0x0B8E <= x && x <= 0x0B90) || \
	  (0x0B92 <= x && x <= 0x0B95) || (0x0B99 <= x && x <= 0x0B9A) || \
	  x == 0x0B9C || (0x0B9E <= x && x <= 0x0B9F) || \
	  (0x0BA3 <= x && x <= 0x0BA4) || (0x0BA8 <= x && x <= 0x0BAA) || \
	  (0x0BAE <= x && x <= 0x0BB5) || (0x0BB7 <= x && x <= 0x0BB9) || \
	  (0x0C05 <= x && x <= 0x0C0C) || (0x0C0E <= x && x <= 0x0C10) || \
	  (0x0C12 <= x && x <= 0x0C28) || (0x0C2A <= x && x <= 0x0C33) || \
	  (0x0C35 <= x && x <= 0x0C39) || (0x0C60 <= x && x <= 0x0C61) || \
	  (0x0C85 <= x && x <= 0x0C8C) || (0x0C8E <= x && x <= 0x0C90) || \
	  (0x0C92 <= x && x <= 0x0CA8) || (0x0CAA <= x && x <= 0x0CB3) || \
	  (0x0CB5 <= x && x <= 0x0CB9) || x == 0x0CDE || \
	  (0x0CE0 <= x && x <= 0x0CE1) || (0x0D05 <= x && x <= 0x0D0C) || \
	  (0x0D0E <= x && x <= 0x0D10) || (0x0D12 <= x && x <= 0x0D28) || \
	  (0x0D2A <= x && x <= 0x0D39) || (0x0D60 <= x && x <= 0x0D61) || \
	  (0x0E01 <= x && x <= 0x0E2E) || x == 0x0E30 || \
	  (0x0E32 <= x && x <= 0x0E33) || (0x0E40 <= x && x <= 0x0E45) || \
	  (0x0E81 <= x && x <= 0x0E82) || x == 0x0E84 || \
	  (0x0E87 <= x && x <= 0x0E88) || x == 0x0E8A || x == 0x0E8D || \
	  (0x0E94 <= x && x <= 0x0E97) || (0x0E99 <= x && x <= 0x0E9F) || \
	  (0x0EA1 <= x && x <= 0x0EA3) || x == 0x0EA5 || x == 0x0EA7 || \
	  (0x0EAA <= x && x <= 0x0EAB) || (0x0EAD <= x && x <= 0x0EAE) || \
	  x == 0x0EB0 || (0x0EB2 <= x && x <= 0x0EB3) || x == 0x0EBD || \
	  (0x0EC0 <= x && x <= 0x0EC4) || (0x0F40 <= x && x <= 0x0F47) || \
	  (0x0F49 <= x && x <= 0x0F69) || (0x10A0 <= x && x <= 0x10C5) || \
	  (0x10D0 <= x && x <= 0x10F6) || x == 0x1100 || \
	  (0x1102 <= x && x <= 0x1103) || (0x1105 <= x && x <= 0x1107) || \
	  x == 0x1109 || (0x110B <= x && x <= 0x110C) || \
	  (0x110E <= x && x <= 0x1112) || x == 0x113C || x == 0x113E || \
	  x == 0x1140 || x == 0x114C || x == 0x114E || x == 0x1150 || \
	  (0x1154 <= x && x <= 0x1155) || x == 0x1159 || \
	  (0x115F <= x && x <= 0x1161) || x == 0x1163 || x == 0x1165 || \
	  x == 0x1167 || x == 0x1169 || (0x116D <= x && x <= 0x116E) || \
	  (0x1172 <= x && x <= 0x1173) || x == 0x1175 || x == 0x119E || \
	  x == 0x11A8 || x == 0x11AB || (0x11AE <= x && x <= 0x11AF) || \
	  (0x11B7 <= x && x <= 0x11B8) || x == 0x11BA || \
	  (0x11BC <= x && x <= 0x11C2) || x == 0x11EB || x == 0x11F0 || \
	  x == 0x11F9 || (0x1E00 <= x && x <= 0x1E9B) || \
	  (0x1EA0 <= x && x <= 0x1EF9) || (0x1F00 <= x && x <= 0x1F15) || \
	  (0x1F18 <= x && x <= 0x1F1D) || (0x1F20 <= x && x <= 0x1F45) || \
	  (0x1F48 <= x && x <= 0x1F4D) || (0x1F50 <= x && x <= 0x1F57) || \
	  x == 0x1F59 || x == 0x1F5B || x == 0x1F5D || \
	  (0x1F5F <= x && x <= 0x1F7D) || (0x1F80 <= x && x <= 0x1FB4) || \
	  (0x1FB6 <= x && x <= 0x1FBC) || x == 0x1FBE || \
	  (0x1FC2 <= x && x <= 0x1FC4) || (0x1FC6 <= x && x <= 0x1FCC) || \
	  (0x1FD0 <= x && x <= 0x1FD3) || (0x1FD6 <= x && x <= 0x1FDB) || \
	  (0x1FE0 <= x && x <= 0x1FEC) || (0x1FF2 <= x && x <= 0x1FF4) || \
	  (0x1FF6 <= x && x <= 0x1FFC) || x == 0x2126 || \
	  (0x212A <= x && x <= 0x212B) || x == 0x212E || \
	  (0x2180 <= x && x <= 0x2182) || (0x3041 <= x && x <= 0x3094) || \
	  (0x30A1 <= x && x <= 0x30FA) || (0x3105 <= x && x <= 0x312C) || \
	  (0xAC00 <= x && x <= 0xD7A3) || (0x4E00 <= x && x <= 0x9FA5) || \
	  x == 0x3007 || (0x3021 <= x && x <= 0x3029))
	#define NC_CHAR(x) \
	  (NC_START_CHAR(x) || x == 0x2E || x == 0x2D || x == 0x5F || \
	  (0x0030 <= x && x <= 0x0039) || (0x0660 <= x && x <= 0x0669) || \
	  (0x06F0 <= x && x <= 0x06F9) || (0x0966 <= x && x <= 0x096F) || \
	  (0x09E6 <= x && x <= 0x09EF) || (0x0A66 <= x && x <= 0x0A6F) || \
	  (0x0AE6 <= x && x <= 0x0AEF) || (0x0B66 <= x && x <= 0x0B6F) || \
	  (0x0BE7 <= x && x <= 0x0BEF) || (0x0C66 <= x && x <= 0x0C6F) || \
	  (0x0CE6 <= x && x <= 0x0CEF) || (0x0D66 <= x && x <= 0x0D6F) || \
	  (0x0E50 <= x && x <= 0x0E59) || (0x0ED0 <= x && x <= 0x0ED9) || \
	  (0x0F20 <= x && x <= 0x0F29) || (0x0300 <= x && x <= 0x0345) || \
	  (0x0360 <= x && x <= 0x0361) || (0x0483 <= x && x <= 0x0486) || \
	  (0x0591 <= x && x <= 0x05A1) || (0x05A3 <= x && x <= 0x05B9) || \
	  (0x05BB <= x && x <= 0x05BD) || x == 0x05BF || \
	  (0x05C1 <= x && x <= 0x05C2) || x == 0x05C4 || \
	  (0x064B <= x && x <= 0x0652) || x == 0x0670 || \
	  (0x06D6 <= x && x <= 0x06DC) || (0x06DD <= x && x <= 0x06DF) || \
	  (0x06E0 <= x && x <= 0x06E4) || (0x06E7 <= x && x <= 0x06E8) || \
	  (0x06EA <= x && x <= 0x06ED) || (0x0901 <= x && x <= 0x0903) || \
	  x == 0x093C || (0x093E <= x && x <= 0x094C) || x == 0x094D || \
	  (0x0951 <= x && x <= 0x0954) || (0x0962 <= x && x <= 0x0963) || \
	  (0x0981 <= x && x <= 0x0983) || x == 0x09BC || x == 0x09BE || \
	  x == 0x09BF || (0x09C0 <= x && x <= 0x09C4) || \
	  (0x09C7 <= x && x <= 0x09C8) || (0x09CB <= x && x <= 0x09CD) || \
	  x == 0x09D7 || (0x09E2 <= x && x <= 0x09E3) || x == 0x0A02 || \
	  x == 0x0A3C || x == 0x0A3E || x == 0x0A3F || \
	  (0x0A40 <= x && x <= 0x0A42) || (0x0A47 <= x && x <= 0x0A48) || \
	  (0x0A4B <= x && x <= 0x0A4D) || (0x0A70 <= x && x <= 0x0A71) || \
	  (0x0A81 <= x && x <= 0x0A83) || x == 0x0ABC || \
	  (0x0ABE <= x && x <= 0x0AC5) || (0x0AC7 <= x && x <= 0x0AC9) || \
	  (0x0ACB <= x && x <= 0x0ACD) || (0x0B01 <= x && x <= 0x0B03) || \
	  x == 0x0B3C || (0x0B3E <= x && x <= 0x0B43) || \
	  (0x0B47 <= x && x <= 0x0B48) || (0x0B4B <= x && x <= 0x0B4D) || \
	  (0x0B56 <= x && x <= 0x0B57) || (0x0B82 <= x && x <= 0x0B83) || \
	  (0x0BBE <= x && x <= 0x0BC2) || (0x0BC6 <= x && x <= 0x0BC8) || \
	  (0x0BCA <= x && x <= 0x0BCD) || x == 0x0BD7 || \
	  (0x0C01 <= x && x <= 0x0C03) || (0x0C3E <= x && x <= 0x0C44) || \
	  (0x0C46 <= x && x <= 0x0C48) || (0x0C4A <= x && x <= 0x0C4D) || \
	  (0x0C55 <= x && x <= 0x0C56) || (0x0C82 <= x && x <= 0x0C83) || \
	  (0x0CBE <= x && x <= 0x0CC4) || (0x0CC6 <= x && x <= 0x0CC8) || \
	  (0x0CCA <= x && x <= 0x0CCD) || (0x0CD5 <= x && x <= 0x0CD6) || \
	  (0x0D02 <= x && x <= 0x0D03) || (0x0D3E <= x && x <= 0x0D43) || \
	  (0x0D46 <= x && x <= 0x0D48) || (0x0D4A <= x && x <= 0x0D4D) || \
	  x == 0x0D57 || x == 0x0E31 || (0x0E34 <= x && x <= 0x0E3A) || \
	  (0x0E47 <= x && x <= 0x0E4E) || x == 0x0EB1 || \
	  (0x0EB4 <= x && x <= 0x0EB9) || (0x0EBB <= x && x <= 0x0EBC) || \
	  (0x0EC8 <= x && x <= 0x0ECD) || (0x0F18 <= x && x <= 0x0F19) || \
	  x == 0x0F35 || x == 0x0F37 || x == 0x0F39 || x == 0x0F3E || \
	  x == 0x0F3F || (0x0F71 <= x && x <= 0x0F84) || \
	  (0x0F86 <= x && x <= 0x0F8B) || (0x0F90 <= x && x <= 0x0F95) || \
	  x == 0x0F97 || (0x0F99 <= x && x <= 0x0FAD) || \
	  (0x0FB1 <= x && x <= 0x0FB7) || x == 0x0FB9 || \
	  (0x20D0 <= x && x <= 0x20DC) || x == 0x20E1 || \
	  (0x302A <= x && x <= 0x302F) || x == 0x3099 || x == 0x309A || \
	  x == 0x00B7 || x == 0x02D0 || x == 0x02D1 || x == 0x0387 || \
	  x == 0x0640 || x == 0x0E46 || x == 0x0EC6 || x == 0x3005 || \
	  (0x3031 <= x && x <= 0x3035) || (0x309D <= x && x <= 0x309E) || \
	  (0x30FC <= x && x <= 0x30FE))

	auto it=name.begin();
	if(!NC_START_CHAR(*it))
		return false;
	++it;

	for(;it!=name.end(); ++it)
	{
		if(!(NC_CHAR(*it)))
			return false;
	}

	#undef NC_CHAR
	#undef NC_START_CHAR

	return true;
}

ASFUNCTIONBODY_ATOM(lightspark,_isXMLName)
{
	assert_and_throw(argslen <= 1);
	if(argslen==0)
		asAtomHandler::setBool(ret,false);
	else
		asAtomHandler::setBool(ret,isXMLName(wrk,args[0]));
}

ObjectPrototype::ObjectPrototype(ASWorker* wrk,Class_base* c):ASObject(wrk,c)
{
	traitsInitialized = true;
	constructIndicator = true;
	constructorCallComplete = true;
	obj = this;
	this->objfreelist=nullptr; // prototypes are not reusable
	originalPrototypeVars = new_asobject(wrk);
	originalPrototypeVars->objfreelist=nullptr;
	originalPrototypeVars->setRefConstant();
}

void ObjectPrototype::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	prepShutdown();
}
bool ObjectPrototype::isEqual(ASObject* r)
{
	if (r->is<ObjectPrototype>())
		return this->getClass() == r->getClass();
	return ASObject::isEqual(r);
}

Prototype* ObjectPrototype::clonePrototype(ASWorker* wrk)
{
	Prototype* res = new_objectPrototype(wrk);
	copyOriginalValues(res);
	return res;
}

GET_VARIABLE_RESULT ObjectPrototype::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	GET_VARIABLE_RESULT res = getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	if(asAtomHandler::isValid(ret) || prevPrototype.isNull())
		return res;

	return prevPrototype->getObj()->getVariableByMultiname(ret,name, opt,wrk);
}

multiname *ObjectPrototype::setVariableByMultiname(multiname &name, asAtom& o, ASObject::CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	if (this->isSealed && this->hasPropertyByMultiname(name,false,true,wrk))
	{
		createError<ReferenceError>(getInstanceWorker(),kCannotAssignToMethodError, name.normalizedNameUnresolved(getSystemState()), "");
		return nullptr;
	}
	if (asAtomHandler::isObject(o)) // value may be used in another worker, so reset it's freelist pointer
		asAtomHandler::getObjectNoCheck(o)->objfreelist=nullptr;
	return ASObject::setVariableByMultiname(name, o, allowConst,alreadyset,wrk);
}

ObjectConstructor::ObjectConstructor(ASWorker* wrk, Class_base* c, uint32_t length) : ASObject(wrk,c,T_OBJECT,SUBTYPE_OBJECTCONSTRUCTOR),_length(length)
{
	this->setRefConstant();
	Class<ASObject>::getRef(c->getSystemState())->prototype->incRef();
	this->prototype = Class<ASObject>::getRef(c->getSystemState())->prototype.getPtr();
}

ArrayPrototype::ArrayPrototype(ASWorker* wrk, Class_base* c) : Array(wrk,c)
{
	traitsInitialized = true;
	constructIndicator = true;
	constructorCallComplete = true;
	obj = this;
	this->objfreelist=nullptr; // prototypes are not reusable
	originalPrototypeVars = new_asobject(wrk);
	originalPrototypeVars->objfreelist=nullptr;
	originalPrototypeVars->setRefConstant();
}

void ArrayPrototype::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	Array::prepareShutdown();
	prepShutdown();
}

bool ArrayPrototype::isEqual(ASObject* r)
{
	if (r->is<ArrayPrototype>())
		return this->getClass() == r->getClass();
	return ASObject::isEqual(r);
}

Prototype* ArrayPrototype::clonePrototype(ASWorker* wrk)
{
	Prototype* res = new (getClass()->memoryAccount) ArrayPrototype(wrk,getClass());
	copyOriginalValues(res);
	return res;
}

GET_VARIABLE_RESULT ArrayPrototype::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	GET_VARIABLE_RESULT res = Array::getVariableByMultiname(ret,name,opt,wrk);
	if(asAtomHandler::isValid(ret) || prevPrototype.isNull())
		return res;

	return prevPrototype->getObj()->getVariableByMultiname(ret,name, opt,wrk);
}

multiname *ArrayPrototype::setVariableByMultiname(multiname &name, asAtom& o, ASObject::CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	if (this->isSealed && this->hasPropertyByMultiname(name,false,true,wrk))
	{
		createError<ReferenceError>(getInstanceWorker(),kCannotAssignToMethodError, name.normalizedNameUnresolved(getSystemState()), "");
		return nullptr;
	}
	if (asAtomHandler::isObject(o)) // value may be used in another worker, so reset it's freelist pointer
		asAtomHandler::getObjectNoCheck(o)->objfreelist=nullptr;
	return Array::setVariableByMultiname(name, o, allowConst,alreadyset,wrk);
}

GET_VARIABLE_RESULT ObjectConstructor::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	if (name.normalizedName(getSystemState()) == "prototype")
	{
		prototype->getObj()->incRef();
		ret = asAtomHandler::fromObject(prototype->getObj());
	}
	else if (name.normalizedName(getSystemState()) == "length")
		asAtomHandler::setUInt(ret,getInstanceWorker(),_length);
	else
		return getClass()->getVariableByMultiname(ret,name, opt,wrk);
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}
bool ObjectConstructor::isEqual(ASObject* r)
{
	return this == r || getClass() == r;
}

FunctionPrototype::FunctionPrototype(ASWorker* wrk, Class_base* c, _NR<Prototype> p) : Function(wrk,c, ASNop)
{
	prevPrototype=p;
	//Add the prototype to the Nop function
	this->prototype = _MR(new_asobject(wrk));
	obj = this;
	this->objfreelist=nullptr; // prototypes are not reusable
	originalPrototypeVars = new_asobject(wrk);
	originalPrototypeVars->objfreelist=nullptr;
	originalPrototypeVars->setRefConstant();
}

void FunctionPrototype::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	Function::prepareShutdown();
	prepShutdown();
}

GET_VARIABLE_RESULT FunctionPrototype::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	GET_VARIABLE_RESULT res =Function::getVariableByMultiname(ret,name, opt,wrk);
	if(asAtomHandler::isValid(ret) || prevPrototype.isNull())
		return res;

	return prevPrototype->getObj()->getVariableByMultiname(ret,name, opt,wrk);
}

multiname* FunctionPrototype::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	if (asAtomHandler::isObject(o)) // value may be used in another worker, so reset it's freelist pointer
		asAtomHandler::getObjectNoCheck(o)->objfreelist=nullptr;
	return Function::setVariableByMultiname(name, o, allowConst,alreadyset,wrk);
	
}

Prototype* FunctionPrototype::clonePrototype(ASWorker* wrk)
{
	Prototype* res = new_functionPrototype(wrk,getClass(),this->prevPrototype);
	copyOriginalValues(res);
	return res;
}

Function_object::Function_object(ASWorker* wrk,Class_base* c, _R<ASObject> p) : ASObject(wrk,c,T_OBJECT,SUBTYPE_FUNCTIONOBJECT), functionPrototype(p)
{
	traitsInitialized = true;
	constructIndicator = true;
	constructorCallComplete = true;
}

void Function_object::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	if (functionPrototype)
		functionPrototype->prepareShutdown();
	ASObject::prepareShutdown();
}

GET_VARIABLE_RESULT Function_object::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	GET_VARIABLE_RESULT res = getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	assert(!functionPrototype.isNull());
	if(asAtomHandler::isValid(ret))
		return res;

	return functionPrototype->getVariableByMultiname(ret,name, opt,wrk);
}
