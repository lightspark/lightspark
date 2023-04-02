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
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/XML.h"

using namespace std;
using namespace lightspark;

Any* const Type::anyType = new Any();
Void* const Type::voidType = new Void();

Null::Null():ASObject(nullptr,nullptr,T_NULL)
{
	traitsInitialized = true;
	constructIndicator = true;
	constructorCallComplete = true;
}

Undefined::Undefined():ASObject(nullptr,nullptr,T_UNDEFINED)
{
	traitsInitialized = true;
	constructIndicator = true;
	constructorCallComplete = true;
}

ASFUNCTIONBODY_ATOM(Undefined,call)
{
	LOG_CALL("Undefined function");
}

TRISTATE Undefined::isLess(ASObject* r)
{
	//ECMA-262 complaiant
	//As undefined became NaN when converted to number the operation is undefined
	return TUNDEFINED;
}
TRISTATE Undefined::isLessAtom(asAtom& r)
{
	//ECMA-262 complaiant
	//As undefined became NaN when converted to number the operation is undefined
	return TUNDEFINED;
}

bool Undefined::isEqual(ASObject* r)
{
	switch(r->getObjectType())
	{
		case T_UNDEFINED:
		case T_NULL:
			return true;
		case T_NUMBER:
		case T_INTEGER:
		case T_UINTEGER:
		case T_BOOLEAN:
			return false;
		case T_FUNCTION:
		case T_STRING:
			if (!r->isConstructed())
				return true;
			return false;
		default:
			return r->isEqual(this);
	}
}

int32_t Undefined::toInt()
{
	return 0;
}
int64_t Undefined::toInt64()
{
	return 0;
}

ASObject *Undefined::describeType(ASWorker* wrk) const
{
	return ASObject::describeType(wrk);
}

void Undefined::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
		out->writeByte(amf0_undefined_marker);
	else
		out->writeByte(undefined_marker);
}

multiname *Undefined::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	LOG(LOG_ERROR,"trying to set variable on undefined:"<<name <<" "<<asAtomHandler::toDebugString(o));
	createError<TypeError>(getInstanceWorker(),kConvertUndefinedToObjectError);
	return nullptr;
}

IFunction::IFunction(ASWorker* wrk,Class_base* c,CLASS_SUBTYPE st):ASObject(wrk,c,T_FUNCTION,st),length(0),closure_this(nullptr),inClass(nullptr),isStatic(false),clonedFrom(nullptr),functionname(0)
{
}

void IFunction::sinit(Class_base* c)
{
	c->isReusable=true;
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),IFunction::_toString),DYNAMIC_TRAIT);

	c->setDeclaredMethodByQName("call","",Class<IFunction>::getFunction(c->getSystemState(),IFunction::_call,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("call",AS3,Class<IFunction>::getFunction(c->getSystemState(),IFunction::_call,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("apply","",Class<IFunction>::getFunction(c->getSystemState(),IFunction::apply,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("apply",AS3,Class<IFunction>::getFunction(c->getSystemState(),IFunction::apply,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),IFunction::_length),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),IFunction::_length),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),IFunction::_toString),NORMAL_METHOD,false);
}

void IFunction::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (clonedFrom)
		clonedFrom->prepareShutdown();
	if (closure_this)
		closure_this->prepareShutdown();
	if (prototype)
		prototype->prepareShutdown();
}

bool IFunction::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	if (closure_this)
		ret = closure_this->countAllCylicMemberReferences(gcstate) || ret;
	if (prototype)
		ret = prototype->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

ASFUNCTIONBODY_GETTER_SETTER(IFunction,prototype)
ASFUNCTIONBODY_ATOM(IFunction,_length)
{
	if (asAtomHandler::is<IFunction>(obj))
	{
		IFunction* th=asAtomHandler::as<IFunction>(obj);
		asAtomHandler::setUInt(ret,wrk,th->length);
	}
	else
		asAtomHandler::setUInt(ret,wrk,1);
}

ASFUNCTIONBODY_ATOM(IFunction,apply)
{
	/* This function never changes the 'this' pointer of a method closure */
	IFunction* th=asAtomHandler::as<IFunction>(obj);
	assert_and_throw(argslen<=2);

	asAtom newObj=asAtomHandler::invalidAtom;
	asAtom* newArgs=nullptr;
	int newArgsLen=0;
	if(th->inClass && th->closure_this)
		newObj = asAtomHandler::fromObject(th->closure_this);
	else
	{
		//Validate parameters
		if(argslen==0 || asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
		{
			//get the current global object
			call_context* cc = wrk->currentCallContext;
			if (!cc)
			{
				if (argslen == 0)
					newObj=asAtomHandler::nullAtom;
				else
					newObj=args[0];
			}
			else if (cc->parent_scope_stack && cc->parent_scope_stack->scope.size() > 0)
				newObj = cc->parent_scope_stack->scope[0].object;
			else
			{
				assert_and_throw(cc->curr_scope_stack > 0);
				newObj = cc->scope_stack[0];
			}
		}
		else
		{
			newObj=args[0];
		}
	}
	if(argslen == 2 && asAtomHandler::isArray(args[1]))
	{
		Array* array=Class<Array>::cast(asAtomHandler::getObject(args[1]));
		newArgsLen=array->size();
		newArgs=new asAtom[newArgsLen];
		for(int i=0;i<newArgsLen;i++)
		{
			asAtom val = array->at(i);
			newArgs[i]=val;
		}
	}
	if (asAtomHandler::is<AVM1Function>(obj))
		asAtomHandler::as<AVM1Function>(obj)->call(&ret,&newObj,newArgs,newArgsLen);
	else
		asAtomHandler::callFunction(obj,wrk,ret,newObj,newArgs,newArgsLen,false);
	if (newArgs)
		delete[] newArgs;
}

ASFUNCTIONBODY_ATOM(IFunction,_call)
{
	/* This function never changes the 'this' pointer of a method closure */
	IFunction* th=static_cast<IFunction*>(asAtomHandler::getObject(obj));
	asAtom newObj=asAtomHandler::invalidAtom;
	asAtom* newArgs=nullptr;
	uint32_t newArgsLen=0;
	if(th->inClass && th->closure_this)
		newObj = asAtomHandler::fromObject(th->closure_this);
	else
	{
		if(argslen==0 || asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
		{
			//get the current global object
			call_context* cc = wrk->currentCallContext;
			if (!cc)
			{
				if (argslen == 0)
					newObj=asAtomHandler::nullAtom;
				else
					newObj=args[0];
			}
			else if (cc->parent_scope_stack && cc->parent_scope_stack->scope.size() > 0)
				newObj = cc->parent_scope_stack->scope[0].object;
			else
			{
				assert_and_throw(cc->curr_scope_stack > 0);
				newObj = cc->scope_stack[0];
			}
		}
		else
		{
			newObj=args[0];
		}
	}
	if(argslen > 1)
	{
		newArgsLen=argslen-1;
		newArgs=g_newa(asAtom, newArgsLen);
		for(unsigned int i=0;i<newArgsLen;i++)
		{
			newArgs[i]=args[i+1];
		}
	}
	if (asAtomHandler::is<AVM1Function>(obj))
		asAtomHandler::as<AVM1Function>(obj)->call(&ret,&newObj,newArgs,newArgsLen);
	else
		asAtomHandler::callFunction(obj,wrk,ret,newObj,newArgs,newArgsLen,false);
}

ASFUNCTIONBODY_ATOM(IFunction,_toString)
{
	if (asAtomHandler::is<Class_base>(obj))
		ret = asAtomHandler::fromString(wrk->getSystemState(),"[class Function]");
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),"function Function() {}");
}

void Class<IFunction>::generator(ASWorker* wrk, asAtom& ret,asAtom* args, const unsigned int argslen)
{
	if (argslen > 0)
		createError<EvalError>(wrk,kFunctionConstructorError);
	ret = asAtomHandler::fromObject(getNopFunction());
}

ASObject *IFunction::describeType(ASWorker* wrk) const
{
	pugi::xml_document p;
	pugi::xml_node root = p.append_child("type");
	root.append_attribute("name").set_value("Function");
	root.append_attribute("base").set_value("Object");
	root.append_attribute("isDynamic").set_value("true");
	root.append_attribute("isFinal").set_value("false");
	root.append_attribute("isStatic").set_value("false");

	pugi::xml_node node=root.append_child("extendsClass");
	node.append_attribute("type").set_value("Object");

	// TODO: accessor
	LOG(LOG_NOT_IMPLEMENTED, "describeType for Function not completely implemented");

	return XML::createFromNode(wrk, root);
}

std::string IFunction::toDebugString() const
{
	string ret = ASObject::toDebugString()+(closure_this ? "(closure:"+closure_this->toDebugString()+")":"")+(clonedFrom ?" cloned":"");
#ifndef _NDEBUG
	if (functionname)
	{
		ret +=" n:";
		ret += getSystemState()->getStringFromUniqueId(functionname);
	}
	if (inClass)
	{
		ret += " c:";
		ret += inClass->toDebugString();
	}
#endif
	return ret;
}
void IFunction::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	// according to avmplus functions are "serialized" as undefined
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
		out->writeByte(amf0_undefined_marker);
	else
		out->writeByte(undefined_marker);
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

/**
 * This prepares a new call_context and then executes the ABC bytecode function
 * by ABCVm::executeFunction() or through JIT.
 * It consumes one reference of obj and one of each arg
 */
void SyntheticFunction::call(ASWorker* wrk,asAtom& ret, asAtom& obj, asAtom *args, uint32_t numArgs, bool coerceresult, bool coercearguments)
{
	const method_body_info::CODE_STATUS& codeStatus = mi->body->codeStatus;
	if (codeStatus == method_body_info::PRELOADING)
	{
		// this is a call to this method during preloading, it can happen when constructing objects for optimization detection
		return;
	}
	assert(wrk == getWorker());
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
		return;

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
			return;
		}
	}
	if ((isMethod() || mi->hasExplicitTypes) && numArgs > mi->numArgs() && !mi->needsArgs() && !mi->needsRest() && !mi->hasOptional())
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError,getSystemState()->getStringFromUniqueId(functionname),Integer::toString(mi->numArgs()),Integer::toString(numArgs));
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
		incRef();
		argumentsArray->setVariableByQName("callee","",this,DECLARED_TRAIT);
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
	cc->inClass = this->inClass;
	cc->stackp = cc->stack;

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
			*i = *argp++;
			(*itpartype++)->coerce(getInstanceWorker(),*i);
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
	if (!isMethod())
	{
		this->incRef();
		this->addStoredMember();
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
				exception_info_abc exc=mi->body->exceptions[i];
				multiname* name=mi->context->getMultiname(exc.exc_type, NULL);
				LOG_CALL("f=" << exc.from << " t=" << exc.to << " type=" << *name);
				if (pos >= exc.from && pos <= exc.to && mi->context->isinstance(excobj, name))
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
	if (!isMethod())
		this->removeStoredMember();//free local ref
	if (recursive_call)
		delete cc;
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
		const Type* t = Type::getTypeFromMultiname(mi->paramTypeName(i), mi->context,opportunistic);
		if (!t)
			return;
		mi->paramTypes.push_back(t);
		if(t != Type::anyType)
			mi->hasExplicitTypes = true;
	}

	const Type* t = Type::getTypeFromMultiname(mi->returnTypeName(), mi->context,opportunistic);
	mi->returnType = t;
}

bool SyntheticFunction::canSkipCoercion(int param, Class_base *cls)
{
	assert(mi->returnType && param < (int)mi->numArgs());
	
	return mi->paramTypes[param] == cls ||(
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

bool Null::isEqual(ASObject* r)
{
	switch(r->getObjectType())
	{
		case T_NULL:
		case T_UNDEFINED:
			return true;
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_BOOLEAN:
			return false;
		case T_FUNCTION:
		case T_STRING:
			if (!r->isConstructed())
				return true;
			return false;
		default:
			return r->isEqual(this);
	}
}

TRISTATE Null::isLess(ASObject* r)
{
	if(r->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(r);
		return (0<i->toInt())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_UINTEGER)
	{
		UInteger* i=static_cast<UInteger*>(r);
		return (0<i->toUInt())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NUMBER)
	{
		Number* i=static_cast<Number*>(r);
		if(std::isnan(i->toNumber())) return TUNDEFINED;
		return (0<i->toNumber())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_BOOLEAN)
	{
		Boolean* i=static_cast<Boolean*>(r);
		return (0<i->val)?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NULL)
	{
		return TFALSE;
	}
	else if(r->getObjectType()==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else if(r->getObjectType()==T_STRING)
	{
		double val2=r->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
	else
	{
		asAtom val2p=asAtomHandler::invalidAtom;
		bool isrefcounted;
		r->toPrimitive(val2p,isrefcounted,NUMBER_HINT);
		double val2=asAtomHandler::toNumber(val2p);
		if (isrefcounted)
			ASATOM_DECREF(val2p);
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
}

TRISTATE Null::isLessAtom(asAtom& r)
{
	if(asAtomHandler::getObjectType(r)==T_INTEGER)
	{
		return (0<asAtomHandler::toInt(r))?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_UINTEGER)
	{
		return (0<asAtomHandler::toUInt(r))?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_NUMBER)
	{
		if(std::isnan(asAtomHandler::toNumber(r))) return TUNDEFINED;
		return (0<asAtomHandler::toNumber(r))?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_BOOLEAN)
	{
		return (0<asAtomHandler::toInt(r))?TTRUE:TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_NULL)
	{
		return TFALSE;
	}
	else if(asAtomHandler::getObjectType(r)==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else if(asAtomHandler::getObjectType(r)==T_STRING)
	{
		double val2=asAtomHandler::toNumber(r);
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
	else
	{
		asAtom val2p=asAtomHandler::invalidAtom;
		bool isrefcounted;
		asAtomHandler::getObject(r)->toPrimitive(val2p,isrefcounted,NUMBER_HINT);
		double val2=asAtomHandler::toNumber(val2p);
		if (isrefcounted)
			ASATOM_DECREF(val2p);
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
}

int32_t Null::getVariableByMultiname_i(const multiname& name, ASWorker* wrk)
{
	LOG(LOG_ERROR,"trying to get variable on null:"<<name);
	createError<TypeError>(wrk,kConvertNullToObjectError);
	return 0;
}

GET_VARIABLE_RESULT Null::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	LOG(LOG_ERROR,"trying to get variable on null:"<<name);
	createError<TypeError>(wrk,kConvertNullToObjectError);
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}

int32_t Null::toInt()
{
	return 0;
}
int64_t Null::toInt64()
{
	return 0;
}

void Null::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
		out->writeByte(amf0_null_marker);
	else
		out->writeByte(null_marker);
}

multiname *Null::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	LOG(LOG_ERROR,"trying to set variable on null:"<<name<<" value:"<<asAtomHandler::toDebugString(o));
	ASATOM_DECREF(o);
	createError<TypeError>(wrk,kConvertNullToObjectError);
	return nullptr;
}

const Type* Type::getBuiltinType(ASWorker* wrk, multiname* mn)
{
	if(mn->isStatic && mn->cachedType)
		return mn->cachedType;
	assert_and_throw(mn->isQName());
	assert(mn->name_type == multiname::NAME_STRING);
	if(mn == 0)
		return Type::anyType; //any
	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::ANY
		&& mn->hasEmptyNS)
		return Type::anyType;
	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::VOID
		&& mn->hasEmptyNS)
		return Type::voidType;

	//Check if the class has already been defined
	ASObject* target;
	asAtom tmp=asAtomHandler::invalidAtom;
	wrk->getSystemState()->systemDomain->getVariableAndTargetByMultiname(tmp,*mn, target,wrk);
	if(asAtomHandler::isClass(tmp))
	{
		if (mn->isStatic)
			mn->cachedType = dynamic_cast<const Class_base*>(asAtomHandler::getObjectNoCheck(tmp));
		return dynamic_cast<const Class_base*>(asAtomHandler::getObjectNoCheck(tmp));
	}
	else
		return nullptr;
}

/*
 * This should only be called after all global objects have been created
 * by running ABCContext::exec() for all ABCContexts.
 * Therefore, all classes are at least declared.
 */
const Type* Type::getTypeFromMultiname(multiname* mn, ABCContext* context, bool opportunistic)
{
	if(mn == 0) //multiname idx zero indicates any type
		return Type::anyType;

	if(mn->isStatic && mn->cachedType)
		return mn->cachedType;

	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::ANY
		&& mn->ns.size() == 1 && mn->hasEmptyNS)
		return Type::anyType;

	if(mn->name_type == multiname::NAME_STRING && mn->name_s_id==BUILTIN_STRINGS::VOID
		&& mn->ns.size() == 1 && mn->hasEmptyNS)
		return Type::voidType;

	ASObject* typeObject = nullptr;
	/*
	 * During the newClass opcode, the class is added to context->root->applicationDomain->classesBeingDefined.
	 * The class variable in the global scope is only set a bit later.
	 * When the class has to be resolved in between (for example, the
	 * class has traits of the class's type), then we'll find it in
	 * classesBeingDefined, but context->root->getVariableAndTargetByMultiname()
	 * would still return "Undefined".
	 */
	auto i = context->root->applicationDomain->classesBeingDefined.find(mn);
	if(i != context->root->applicationDomain->classesBeingDefined.end())
		typeObject = i->second;
	else
	{
		if (opportunistic)
			typeObject = context->root->applicationDomain->getVariableByMultinameOpportunistic(*mn,context->root->getInstanceWorker());
		else
		{
			ASObject* target;
			asAtom o=asAtomHandler::invalidAtom;
			context->root->applicationDomain->getVariableAndTargetByMultiname(o,*mn,target,context->root->getInstanceWorker());
			if (asAtomHandler::isValid(o))
				typeObject=asAtomHandler::toObject(o,context->root->getInstanceWorker());
		}
	}
	if(!typeObject)
	{
		if (mn->ns.size() >= 1 && mn->ns[0].nsNameId == BUILTIN_STRINGS::STRING_AS3VECTOR)
		{
			if (mn->templateinstancenames.size() == 1)
			{
				const Type* instancetype = getTypeFromMultiname(mn->templateinstancenames.front(),context,opportunistic);
				if (instancetype==nullptr)
				{
					multiname* mti = mn->templateinstancenames.front();
					auto it = context->root->applicationDomain->classesBeingDefined.begin();
					while(it != context->root->applicationDomain->classesBeingDefined.end())
					{
						const multiname* m = it->first;
						if (mti->name_type == multiname::NAME_STRING && m->name_type == multiname::NAME_STRING
								&& mti->name_s_id == m->name_s_id
								&& mti->ns == m->ns)
						{
							instancetype = it->second;
							break;
						}
						it++;
					}
				}
				if (instancetype)
					typeObject = Template<Vector>::getTemplateInstance(context->root,instancetype,context->root->applicationDomain).getPtr();
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,"getTypeFromMultiname with "<<mn->templateinstancenames.size()<<" instance types");
				QName qname(mn->name_s_id,mn->ns[0].nsNameId);
				typeObject = Template<Vector>::getTemplateInstance(context->root,qname,context,context->root->applicationDomain).getPtr();
			}
		}
	}
	if (typeObject && typeObject->is<Class_base>())
		mn->cachedType = typeObject->as<Type>();
	return typeObject ? typeObject->as<Type>() : nullptr;
}

Class_base::Class_base(const QName& name, uint32_t _classID, MemoryAccount* m):ASObject(getSys()->worker,Class_object::getClass(getSys()),T_CLASS),protected_ns(getSys(),"",NAMESPACE),constructor(nullptr),
	qualifiedClassnameID(UINT32_MAX),global(nullptr),
	context(nullptr),class_name(name),memoryAccount(m),length(1),class_index(-1),isFinal(false),isSealed(false),isInterface(false),isReusable(false),use_protected(false),classID(_classID)
{
	setSystemState(getSys());
	setRefConstant();
}

Class_base::Class_base(const Class_object* c):ASObject((MemoryAccount*)nullptr),protected_ns(getSys(),BUILTIN_STRINGS::EMPTY,NAMESPACE),constructor(nullptr),
	qualifiedClassnameID(UINT32_MAX),global(nullptr),
	context(nullptr),class_name(BUILTIN_STRINGS::STRING_CLASS,BUILTIN_STRINGS::EMPTY),memoryAccount(nullptr),length(1),class_index(-1),isFinal(false),isSealed(false),isInterface(false),isReusable(false),use_protected(false),classID(UINT32_MAX)
{
	type=T_CLASS;
	//We have tested that (Class is Class == true) so the classdef is 'this'
	setClass(this);
	//The super is Class<ASObject> but we set it in SystemState constructor to avoid an infinite loop

	setSystemState(getSys());
}

/*
 * This copies the non-static traits of the super class to this
 * class.
 *
 * If a property is in the protected namespace of the super class, a copy is
 * created with the protected namespace of this class.
 * That is necessary, because superclass methods are called with the protected ns
 * of the current class.
 *
 * use_protns and protectedns must be set before this function is called
 */
void Class_base::copyBorrowedTraitsFromSuper()
{
	//assert(borrowedVariables.Variables.empty());
	variables_map::var_iterator i = super->borrowedVariables.Variables.begin();
	for(;i != super->borrowedVariables.Variables.end(); ++i)
	{
		variable& v = i->second;
		if (borrowedVariables.findObjVar(i->first,v.ns,TRAIT_KIND::NO_CREATE_TRAIT,TRAIT_KIND::DECLARED_TRAIT))
			continue; // variable is already overwritten in this class
		v.issealed = super->isSealed;
		ASATOM_INCREF(v.var);
		ASATOM_INCREF(v.getter);
		ASATOM_INCREF(v.setter);
		borrowedVariables.Variables.insert(make_pair(i->first,v));
	}
}

void Class_base::initStandardProps()
{
	constructorprop = _NR<ObjectConstructor>(new_objectConstructor(this,0));
	addConstructorGetter();

	Function* f = Class<IFunction>::getFunction(getSystemState(),Class_base::_toString);
	f->setRefConstant();
	setDeclaredMethodByQName("toString","",f,NORMAL_METHOD,false);
	prototype->setVariableByQName("constructor","",this,DECLARED_TRAIT);

	if(super)
		prototype->prevPrototype=super->prototype;
	addPrototypeGetter();
	addLengthGetter();
}


bool Class_base::coerce(ASWorker* wrk, asAtom& o) const
{
	switch (asAtomHandler::getObjectType(o))
	{
		case T_UNDEFINED:
			asAtomHandler::setNull(o);
			return true;
		case T_NULL:
			return false;
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
			// int/uint/number are interchangeable
			if(this == Class<Number>::getRef(getSystemState()).getPtr() || this == Class<UInteger>::getRef(getSystemState()).getPtr() || this == Class<Integer>::getRef(getSystemState()).getPtr())
				return false;
			break;
		default:
			break;
	}
	if(this ==getSystemState()->getObjectClassRef())
		return false;
	if(asAtomHandler::isClass(o))
	{ /* classes can be cast to the type 'Object' or 'Class' */
		if(this == getSystemState()->getObjectClassRef()
		|| (class_name.nameId==BUILTIN_STRINGS::STRING_CLASS && class_name.nsStringId==BUILTIN_STRINGS::EMPTY))
			return false; /* 'this' is the type of a class */
		else
		{
			createError<TypeError>(wrk,kCheckTypeFailedError, asAtomHandler::toObject(o,wrk)->getClassName(), getQualifiedClassName());
			return false;
		}
	}
	if (asAtomHandler::getObject(o) && asAtomHandler::getObject(o)->is<ObjectConstructor>())
		return false;

	//o->getClass() == NULL for primitive types
	//those are handled in overloads Class<Number>::coerce etc.
	if(!asAtomHandler::getObject(o) ||  !asAtomHandler::getObject(o)->getClass() || !asAtomHandler::getObject(o)->getClass()->isSubClass(this))
		createError<TypeError>(wrk,kCheckTypeFailedError, asAtomHandler::toObject(o,wrk)->getClassName(), getQualifiedClassName());
	return false;
}

void Class_base::coerceForTemplate(ASWorker* wrk, asAtom &o) const
{
	switch (asAtomHandler::getObjectType(o))
	{
		case T_UNDEFINED:
			asAtomHandler::setNull(o);
			return;
		case T_NULL:
			return;
		case T_UINTEGER:
		case T_INTEGER:
			if(this == Class<Integer>::getRef(getSystemState()).getPtr() || this == Class<UInteger>::getRef(getSystemState()).getPtr())
				return;
			break;
		case T_NUMBER:
			if(this == Class<Number>::getRef(getSystemState()).getPtr())
				return;
			break;
		case T_STRING:
			if(this == Class<ASString>::getRef(getSystemState()).getPtr())
				return;
			break;
		default:
			break;
	}
	if (asAtomHandler::getObject(o) && asAtomHandler::getObject(o)->is<ObjectConstructor>())
		return;

	if(!asAtomHandler::getObject(o) || !asAtomHandler::getObject(o)->getClass() || !asAtomHandler::getObject(o)->getClass()->isSubClass(this))
		createError<TypeError>(wrk,kCheckTypeFailedError, asAtomHandler::toObject(o,wrk)->getClassName(), getQualifiedClassName());
}

void Class_base::setSuper(Ref<Class_base> super_)
{
	assert(!super);
	super = super_;
	copyBorrowedTraitsFromSuper();
}

ASFUNCTIONBODY_ATOM(Class_base,_toString)
{
	Class_base* th = asAtomHandler::as<Class_base>(obj);
	tiny_string res;
	res = "[class ";
	res += wrk->getSystemState()->getStringFromUniqueId(th->class_name.nameId);
	res += "]";
	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
}

void Class_base::addConstructorGetter()
{
	setDeclaredMethodByQName("constructor","",Class<IFunction>::getFunction(getSystemState(),_getter_constructorprop),GETTER_METHOD,false);
}

void Class_base::addPrototypeGetter()
{
	setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(getSystemState(),_getter_prototype),GETTER_METHOD,false);
}

void Class_base::addLengthGetter()
{
	setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(getSystemState(),_getter_length),GETTER_METHOD,false);
}

Class_base::~Class_base()
{
}

void Class_base::_getter_constructorprop(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen)
{
	Class_base* th = nullptr;
	if(asAtomHandler::is<Class_base>(obj))
		th = asAtomHandler::as<Class_base>(obj);
	else
		th = asAtomHandler::getObject(obj)->getClass();
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,0,"Arguments provided in getter");
		return;
	}
	ASObject* res=th->constructorprop.getPtr();
	res->incRef();
	ret = asAtomHandler::fromObject(res);
}

Prototype* Class_base::getPrototype(ASWorker* wrk) const
{
	assert(wrk==getWorker() && !wrk->inFinalization());
	// workers need their own prototype objects for every class
	if (prototype.isNull() || this->is<Class_inherit>() || wrk->isPrimordial)
		return prototype.getPtr();
	else
		return wrk->getClassPrototype(this);
}


void Class_base::_getter_prototype(asAtom& ret, ASWorker* wrk,asAtom& obj, asAtom* args, const unsigned int argslen)
{
	if(!asAtomHandler::is<Class_base>(obj))
	{
		createError<ArgumentError>(wrk,0,"Function applied to wrong object");
		return;
	}
	Class_base* th = asAtomHandler::as<Class_base>(obj);
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,0,"Arguments provided in getter");
		return;
	}
	
	ret = asAtomHandler::fromObject(th->getPrototype(wrk)->getObj());
	ASATOM_INCREF(ret);
}
ASFUNCTIONBODY_GETTER(Class_base, length)

void Class_base::generator(ASWorker* wrk, asAtom& ret, asAtom* args, const unsigned int argslen)
{
	ASObject::generator(ret,wrk,asAtomHandler::invalidAtom, args, argslen);
}

void Class_base::addImplementedInterface(const multiname& i)
{
	interfaces.push_back(i);
}

void Class_base::addImplementedInterface(Class_base* i)
{
	interfaces_added.push_back(i);
}

tiny_string Class_base::toString()
{
	tiny_string ret="[class ";
	ret+=getSystemState()->getStringFromUniqueId(class_name.nameId);
	ret+="]";
	return ret;
}

void Class_base::setConstructor(IFunction* c)
{
	assert_and_throw(constructor==nullptr);
	if (c)
		c->setRefConstant();
	constructor=c;
}

void Class_base::handleConstruction(asAtom& target, asAtom* args, unsigned int argslen, bool buildAndLink)
{
	if (!asAtomHandler::isObject(target))
		return;
	ASObject* t = asAtomHandler::getObjectNoCheck(target);
	if(buildAndLink)
	{
		setupDeclaredTraits(t);
	}

	if(constructor)
	{
		LOG_CALL("handleConstruction for "<<asAtomHandler::toDebugString(target));
		t->incRef();
		asAtom ret=asAtomHandler::invalidAtom;
		asAtom c = asAtomHandler::fromObject(constructor);
		asAtomHandler::callFunction(c,asAtomHandler::getObjectNoCheck(target)->getInstanceWorker(),ret,target,args,argslen,true);
		t->constructIndicator = true;
		target = asAtomHandler::fromObject(asAtomHandler::getObjectNoCheck(target));
		LOG_CALL("handleConstruction done for "<<asAtomHandler::toDebugString(target));
	}
	else
	{
		t->constructIndicator = true;
		for(uint32_t i=0;i<argslen;i++)
			ASATOM_DECREF(args[i]);
	}
	if (asAtomHandler::isObject(target))
	{
		// Tell the object that the constructor of the builtin object has been called
		if (this->isBuiltin())
			asAtomHandler::getObjectNoCheck(target)->constructionComplete();
		if(buildAndLink)
			asAtomHandler::getObjectNoCheck(target)->afterConstruction();
	}
	else
		t->decRef();
}


void Class_base::finalize()
{
	borrowedVariables.destroyContents();
	super.reset();
	prototype.reset();
	interfaces_added.clear();
	protected_ns = nsNameAndKind(getSystemState(),"",NAMESPACE);
	ASObject* p =constructorprop.getPtr();
	constructorprop.reset();
	if (p)
		p->decRef();
	if(constructor)
		constructor=nullptr;
	context = nullptr;
	global = nullptr;
	length = 1;
	class_index = -1;
	isFinal = false;
	isSealed = false;
	isInterface = false;
	use_protected = false;
}
void Class_base::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	this->isReusable=false;
	ASObject::prepareShutdown();
	borrowedVariables.prepareShutdown();
	if(constructor)
		constructor->prepareShutdown();
	constructor=nullptr;
	if (constructorprop)
		constructorprop->prepareShutdown();
	if (prototype && prototype->getObj())
		prototype->getObj()->prepareShutdown();
	if (super)
		super->prepareShutdown();
}
Template_base::Template_base(ASWorker* wrk,QName name) : ASObject(wrk,(Class_base*)(nullptr)),template_name(name)
{
	type = T_TEMPLATE;
}
void Template_base::addPrototypeGetter(SystemState* sys)
{
	this->setSystemState(sys);
	setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(sys,_getter_prototype),GETTER_METHOD,false);
}
void Template_base::_getter_prototype(asAtom& ret, ASWorker* wrk,asAtom& obj, asAtom* args, const unsigned int argslen)
{
	if(!asAtomHandler::is<Template_base>(obj))
	{
		createError<ArgumentError>(wrk,0,"Function applied to wrong object");
		return;
	}
	Template_base* th = asAtomHandler::as<Template_base>(obj);
	if(argslen != 0)
	{
		createError<ArgumentError>(wrk,0,"Arguments provided in getter");
		return;
	}
	ASObject* res=th->prototype->getObj();
	res->incRef();
	ret = asAtomHandler::fromObject(res);
}

Class_object* Class_object::getClass(SystemState *sys)
{
	//We check if we are registered already
	//if not we register ourselves (see also Class<T>::getClass)
	//Class object position in the map is hardwired to 0
	uint32_t classId=0;
	Class_object* ret=nullptr;
	Class_base** retAddr=&sys->builtinClasses[classId];
	if(*retAddr==nullptr)
	{
		//Create the class
		ret=new (sys->unaccountedMemory) Class_object();
		ret->setWorker(sys->worker);
		ret->setSystemState(sys);
		ret->incRef();
		*retAddr=ret;
	}
	else
		ret=static_cast<Class_object*>(*retAddr);

	return ret;
}

const std::vector<Class_base*>& Class_base::getInterfaces(bool *alldefined) const
{
	if (alldefined)
		*alldefined = true;
	if(!interfaces.empty())
	{
		//Recursively get interfaces implemented by this interface
		std::vector<multiname>::iterator it = interfaces.begin();
		while (it !=interfaces.end())
		{
			ASObject* target;
			asAtom interface_obj=asAtomHandler::invalidAtom;
			this->context->root->applicationDomain->getVariableAndTargetByMultiname(interface_obj,*it, target,context->root->getInstanceWorker());
			if (asAtomHandler::isValid(interface_obj))
			{
				assert_and_throw(asAtomHandler::isClass(interface_obj));
				Class_base* inter=static_cast<Class_base*>(asAtomHandler::getObject(interface_obj));
				//Probe the interface for its interfaces
				bool bAllDefinedSub;
				inter->getInterfaces(&bAllDefinedSub);

				if (bAllDefinedSub)
				{
					interfaces_added.push_back(inter);
					interfaces.erase(it);
					continue;
				}
				else if (alldefined)
					*alldefined = false;
			}
			else if (alldefined)
				*alldefined = false;
			it++;
		}
	}
	return interfaces_added;
}

void Class_base::linkInterface(Class_base* c) const
{
	assert(class_index!=-1);
	//Recursively link interfaces implemented by this interface
	const std::vector<Class_base*> interfaces = getInterfaces();
	for(unsigned int i=0;i<interfaces.size();i++)
		interfaces[i]->linkInterface(c);

	assert_and_throw(context);

	//Link traits of this interface
	for(unsigned int j=0;j<context->instances[class_index].trait_count;j++)
	{
		traits_info* t=&context->instances[class_index].traits[j];
		context->linkTrait(c,t);
	}

	if(constructor)
	{
		LOG_CALL("Calling interface init for " << class_name);
		asAtom v = asAtomHandler::fromObject(c);
		asAtom ret=asAtomHandler::invalidAtom;
		asAtom constr = asAtomHandler::fromObject(constructor);
		asAtomHandler::callFunction(constr,context->root->getInstanceWorker(),ret,v,nullptr,0,false);
		assert_and_throw(asAtomHandler::isInvalid(ret));
	}
}

bool Class_base::isSubClass(const Class_base* cls, bool considerInterfaces) const
{
//	check();
	if(cls==this || cls==cls->getSystemState()->getObjectClassRef())
		return true;

	//Now check the interfaces
	//an interface can't be subclass of a normal class, we only check the interfaces if cls is an interface itself
	if (considerInterfaces && cls->isInterface)
	{
		const std::vector<Class_base*> interfaces = getInterfaces();
		for(unsigned int i=0;i<interfaces.size();i++)
		{
			if(interfaces[i]->isSubClass(cls, considerInterfaces))
				return true;
		}
	}

	//Now ask the super
	if(super && super->isSubClass(cls, considerInterfaces))
		return true;
	return false;
}

const tiny_string Class_base::getQualifiedClassName(bool forDescribeType) const
{
	if (qualifiedClassnameID != UINT32_MAX && !forDescribeType)
		return getSystemState()->getStringFromUniqueId(qualifiedClassnameID);
	if(class_index==-1)
		return class_name.getQualifiedName(getSystemState(),forDescribeType);
	else
	{
		assert_and_throw(context);
		int name_index=context->instances[class_index].name;
		assert_and_throw(name_index);
		const multiname* mname=context->getMultiname(name_index,nullptr);
		return mname->qualifiedString(getSystemState(),forDescribeType);
	}
}
uint32_t Class_base::getQualifiedClassNameID()
{
	if (qualifiedClassnameID == UINT32_MAX)
	{
		if(class_index==-1)
		{
			qualifiedClassnameID = getSystemState()->getUniqueStringId(class_name.getQualifiedName(getSystemState(),false));
		}
		else
		{
			assert_and_throw(context);
			int name_index=context->instances[class_index].name;
			assert_and_throw(name_index);
			const multiname* mname=context->getMultiname(name_index,nullptr);
			qualifiedClassnameID=getSystemState()->getUniqueStringId(mname->qualifiedString(getSystemState(),false));
		}
	}
	return qualifiedClassnameID;
}

tiny_string Class_base::getName() const
{
	return (class_name.nsStringId == BUILTIN_STRINGS::EMPTY ?
				this->getSystemState()->getStringFromUniqueId(class_name.nameId)
			  : this->getSystemState()->getStringFromUniqueId(class_name.nsStringId) +"$"+ this->getSystemState()->getStringFromUniqueId(class_name.nameId));
}

ASObject *Class_base::describeType(ASWorker* wrk) const
{
	pugi::xml_document p;
	pugi::xml_node root = p.append_child("type");

	root.append_attribute("name").set_value(getQualifiedClassName(true).raw_buf());
	root.append_attribute("base").set_value("Class");
	root.append_attribute("isDynamic").set_value("true");
	root.append_attribute("isFinal").set_value("true");
	root.append_attribute("isStatic").set_value("true");

	// extendsClass
	pugi::xml_node node=root.append_child("extendsClass");
	node.append_attribute("type").set_value("Class");
	node=root.append_child("extendsClass");
	node.append_attribute("type").set_value("Object");

	// prototype
	pugi::xml_node prototypenode=root.append_child("accessor");
	prototypenode.append_attribute("name").set_value("prototype");
	prototypenode.append_attribute("access").set_value("readonly");
	prototypenode.append_attribute("type").set_value("*");
	prototypenode.append_attribute("declaredBy").set_value("Class");

	std::map<varName,pugi::xml_node> propnames;
	// variable
	if(class_index>=0)
		describeTraits(root, context->classes[class_index].traits,propnames,true);

	// factory
	node=root.append_child("factory");
	node.append_attribute("type").set_value(getQualifiedClassName().raw_buf());
	describeInstance(node,false,false);
	return XML::createFromNode(wrk,root);
}

void Class_base::describeInstance(pugi::xml_node& root, bool istemplate,bool forinstance) const
{
	// extendsClass
	const Class_base* c=super.getPtr();
	while(c)
	{
		pugi::xml_node node=root.append_child("extendsClass");
		node.append_attribute("type").set_value(c->getQualifiedClassName(true).raw_buf());
		c=c->super.getPtr();
		if (istemplate)
			break;
	}
	this->describeClassMetadata(root);

	// implementsInterface
	std::set<Class_base*> allinterfaces;
	c=this;
	while(c && c->class_index>=0)
	{
		const std::vector<Class_base*>& interfaces=c->getInterfaces();
		auto it=interfaces.begin();
		for(; it!=interfaces.end(); ++it)
		{
			if (allinterfaces.find(*it)!= allinterfaces.end())
				continue;
			allinterfaces.insert(*it);
			pugi::xml_node node=root.append_child("implementsInterface");
			node.append_attribute("type").set_value((*it)->getQualifiedClassName().raw_buf());
		}
		c=c->super.getPtr();
	}
	describeConstructor(root);

	// variables, methods, accessors
	c=this;
	if (c->class_index<0)
	{
		// builtin class
		LOG(LOG_NOT_IMPLEMENTED, "describeType for builtin classes not completely implemented:"<<this->class_name);
		std::map<tiny_string, pugi::xml_node*> instanceNodes;
		if(!istemplate)
			describeVariables(root,c,instanceNodes,Variables,false,forinstance);
		describeVariables(root,c,instanceNodes,borrowedVariables,istemplate,false);
	}
	std::map<varName,pugi::xml_node> propnames;
	bool bfirst = true;
	while(c && c->class_index>=0)
	{
		c->describeTraits(root, c->context->instances[c->class_index].traits,propnames,bfirst);
		bfirst = false;
		c=c->super.getPtr();
		if (istemplate)
			break;
	}
}

void Class_base::describeVariables(pugi::xml_node& root, const Class_base* c, std::map<tiny_string, pugi::xml_node*>& instanceNodes, const variables_map& map, bool isTemplate,bool forinstance) const
{
	variables_map::const_var_iterator it=map.Variables.cbegin();
	for(;it!=map.Variables.cend();++it)
	{
		const char* nodename;
		const char* access = nullptr;
		switch (it->second.kind)
		{
			case CONSTANT_TRAIT:
				nodename = "constant";
				break;
			case INSTANCE_TRAIT:
			case DECLARED_TRAIT:
				if (asAtomHandler::isValid(it->second.var))
				{
					if (isTemplate)
						continue;
					nodename="method";
				}
				else
				{
					if (!isTemplate && forinstance && it->second.kind != INSTANCE_TRAIT)
						continue;
					nodename="accessor";
					if (asAtomHandler::isValid(it->second.getter) && asAtomHandler::isValid(it->second.setter))
						access = "readwrite";
					else if (asAtomHandler::isValid(it->second.getter))
						access = "readonly";
					else if (asAtomHandler::isValid(it->second.setter))
						access = "writeonly";
				}
				break;
			default:
				continue;
		}
		tiny_string name = getSystemState()->getStringFromUniqueId(it->first);
		if (name == "constructor")
			continue;
		auto existing=instanceNodes.find(name);
		if(existing != instanceNodes.cend())
			continue;

		pugi::xml_node node=root.append_child(nodename);
		node.append_attribute("name").set_value(name.raw_buf());
		if (access)
			node.append_attribute("access").set_value(access);
		if (isTemplate)
		{
			ASObject* obj = NULL;
			if (asAtomHandler::isFunction(it->second.getter))
				obj = asAtomHandler::getObject(it->second.getter);
			else if (asAtomHandler::isFunction(it->second.setter))
				obj = asAtomHandler::getObject(it->second.setter);
			if (obj)
			{
				if (obj->is<SyntheticFunction>())
					node.append_attribute("type").set_value(obj->as<SyntheticFunction>()->getMethodInfo()->returnTypeName()->qualifiedString(getSystemState(),true).raw_buf());
				else if (obj->is<Function>())
				{
					if (obj->as<Function>()->returnType)
						node.append_attribute("type").set_value(obj->as<Function>()->returnType->getQualifiedClassName(true).raw_buf());
					else
						LOG(LOG_NOT_IMPLEMENTED,"describeType: return type not known:"<<this->class_name<<"  property "<<name);
				}
			}
			node.append_attribute("declaredBy").set_value("__AS3__.vec::Vector.<*>");
		}
		instanceNodes[name] = &node;
	}
}
void Class_base::describeConstructor(pugi::xml_node &root) const
{
	if (!this->constructor)
		return;
	if (!this->constructor->is<SyntheticFunction>())
	{
		if (!this->getTemplate())
		{
			if (this->constructor->as<IFunction>()->length == 0)
				return;
			LOG(LOG_NOT_IMPLEMENTED,"describeConstructor for builtin classes is not completely implemented");
			pugi::xml_node node=root.append_child("constructor");
			for (uint32_t i = 0; i < this->constructor->as<IFunction>()->length; i++)
			{
				pugi::xml_node paramnode = node.append_child("parameter");
				paramnode.append_attribute("index").set_value(i+1);
				paramnode.append_attribute("type").set_value("*"); // TODO
				paramnode.append_attribute("optional").set_value(false); // TODO
			}
		}
		return;
	}
	method_info* mi = this->constructor->getMethodInfo();
	if (mi->numArgs() == 0)
		return;
	pugi::xml_node node=root.append_child("constructor");

	for (uint32_t i = 0; i < mi->numArgs(); i++)
	{
		pugi::xml_node paramnode = node.append_child("parameter");
		paramnode.append_attribute("index").set_value(i+1);
		paramnode.append_attribute("type").set_value(mi->paramTypeName(i)->qualifiedString(getSystemState(),true).raw_buf());
		paramnode.append_attribute("optional").set_value(i >= mi->numArgs()-mi->numOptions());
	}
}

void Class_base::describeTraits(pugi::xml_node &root, std::vector<traits_info>& traits, std::map<varName,pugi::xml_node> &propnames, bool first) const
{
	std::map<u30, pugi::xml_node> accessorNodes;
	for(unsigned int i=0;i<traits.size();i++)
	{
		traits_info& t=traits[i];
		int kind=t.kind&0xf;
		multiname* mname=context->getMultiname(t.name,NULL);
		if (mname->name_type!=multiname::NAME_STRING ||
			(mname->ns.size()==1 && (mname->ns[0].kind != NAMESPACE)) ||
			mname->ns.size() > 1)
			continue;
		pugi::xml_node node;
		varName vn(mname->name_s_id,mname->ns.size()==1 && first && !(kind==traits_info::Getter || kind==traits_info::Setter) ? mname->ns[0] : nsNameAndKind());
		auto existing=accessorNodes.find(t.name);
		if(existing==accessorNodes.end())
		{
			auto it = propnames.find(vn);
			if (it != propnames.end())
			{
				if (!(kind==traits_info::Getter || kind==traits_info::Setter))
					describeMetadata(it->second,t);
				continue;
			}
		}
		if(kind==traits_info::Slot || kind==traits_info::Const)
		{
			multiname* type=context->getMultiname(t.type_name,NULL);
			const char *nodename=kind==traits_info::Const?"constant":"variable";
			node=root.append_child(nodename);
			node.append_attribute("name").set_value(getSystemState()->getStringFromUniqueId(mname->name_s_id).raw_buf());
			node.append_attribute("type").set_value(type->qualifiedString(getSystemState(),true).raw_buf());
			if (mname->ns.size() > 0 && !mname->ns[0].hasEmptyName())
				node.append_attribute("uri").set_value(getSystemState()->getStringFromUniqueId(mname->ns[0].nsNameId).raw_buf());
			describeMetadata(node, t);
		}
		else if (kind==traits_info::Method)
		{
			node=root.append_child("method");
			node.append_attribute("name").set_value(getSystemState()->getStringFromUniqueId(mname->name_s_id).raw_buf());
			node.append_attribute("declaredBy").set_value(getQualifiedClassName().raw_buf());

			method_info& method=context->methods[t.method];
			const multiname* rtname=method.returnTypeName();
			node.append_attribute("returnType").set_value(rtname->qualifiedString(getSystemState(),true).raw_buf());
			if (mname->ns.size() > 0 && !mname->ns[0].hasEmptyName())
				node.append_attribute("uri").set_value(getSystemState()->getStringFromUniqueId(mname->ns[0].nsNameId).raw_buf());

			assert(method.numArgs() >= method.numOptions());
			uint32_t firstOpt=method.numArgs() - method.numOptions();
			for(uint32_t j=0;j<method.numArgs(); j++)
			{
				pugi::xml_node param=node.append_child("parameter");
				param.append_attribute("index").set_value(UInteger::toString(j+1).raw_buf());
				param.append_attribute("type").set_value(method.paramTypeName(j)->qualifiedString(getSystemState(),true).raw_buf());
				param.append_attribute("optional").set_value(j>=firstOpt?"true":"false");
			}

			describeMetadata(node, t);
		}
		else if (kind==traits_info::Getter || kind==traits_info::Setter)
		{
			// The getters and setters are separate
			// traits. Check if we have already created a
			// node for this multiname with the
			// complementary accessor. If we have, update
			// the access attribute to "readwrite".
			if(existing==accessorNodes.end())
			{
				node=root.append_child("accessor");
				node.append_attribute("name").set_value(getSystemState()->getStringFromUniqueId(mname->name_s_id).raw_buf());
			}
			else
			{
				node=existing->second;
			}

			const char* access=NULL;
			pugi::xml_attribute oldAttr=node.attribute("access");
			tiny_string oldAccess=oldAttr.value();

			if(kind==traits_info::Getter && oldAccess=="")
				access="readonly";
			else if(kind==traits_info::Setter && oldAccess=="")
				access="writeonly";
			else if((kind==traits_info::Getter && oldAccess=="writeonly") ||
				(kind==traits_info::Setter && oldAccess=="readonly"))
				access="readwrite";

			node.remove_attribute("access");
			node.append_attribute("access").set_value(access);

			tiny_string type;
			method_info& method=context->methods[t.method];
			if(kind==traits_info::Getter)
			{
				const multiname* rtname=method.returnTypeName();
				type=rtname->qualifiedString(getSystemState(),true);
			}
			else if(method.numArgs()>0) // setter
			{
				type=method.paramTypeName(0)->qualifiedString(getSystemState(),true);
			}
			if(!type.empty())
			{
				node.remove_attribute("type");
				node.append_attribute("type").set_value(type.raw_buf());
			}
			if (mname->ns.size() > 0 && !mname->ns[0].hasEmptyName())
				node.append_attribute("uri").set_value(getSystemState()->getStringFromUniqueId(mname->ns[0].nsNameId).raw_buf());

			node.remove_attribute("declaredBy");
			node.append_attribute("declaredBy").set_value(getQualifiedClassName().raw_buf());

			describeMetadata(node, t);
			accessorNodes[t.name]=node;
		}
		propnames.insert(make_pair(vn,node));
	}
}

void Class_base::describeMetadata(pugi::xml_node& root, const traits_info& trait) const
{
	if((trait.kind&traits_info::Metadata) == 0)
		return;

	for(unsigned int i=0;i<trait.metadata_count;i++)
	{
		pugi::xml_node metadata_node=root.append_child("metadata");
		metadata_info& minfo = context->metadata[trait.metadata[i]];
		metadata_node.append_attribute("name").set_value(context->root->getSystemState()->getStringFromUniqueId(context->getString(minfo.name)).raw_buf());

		for(unsigned int j=0;j<minfo.item_count;++j)
		{
			pugi::xml_node arg_node=metadata_node.append_child("arg");
			arg_node.append_attribute("key").set_value(context->root->getSystemState()->getStringFromUniqueId(context->getString(minfo.items[j].key)).raw_buf());
			arg_node.append_attribute("value").set_value(context->root->getSystemState()->getStringFromUniqueId(context->getString(minfo.items[j].value)).raw_buf());
		}
	}
}

void Class_base::initializeProtectedNamespace(uint32_t nameId, const namespace_info& ns, RootMovieClip *root)
{
	Class_inherit* cur=dynamic_cast<Class_inherit*>(super.getPtr());
	nsNameAndKind* baseNs=NULL;
	while(cur)
	{
		if(cur->use_protected)
		{
			baseNs=&cur->protected_ns;
			break;
		}
		cur=dynamic_cast<Class_inherit*>(cur->super.getPtr());
	}
	if(baseNs==NULL)
		protected_ns=nsNameAndKind(getSystemState(),nameId,(NS_KIND)(int)ns.kind,root);
	else
		protected_ns=nsNameAndKind(getSystemState(),nameId,baseNs->nsId,(NS_KIND)(int)ns.kind,root);
}

variable* Class_base::findBorrowedSettable(const multiname& name, bool* has_getter)
{
	return ASObject::findSettableImpl(getSystemState(),borrowedVariables,name,has_getter);
}

variable* Class_base::findSettableInPrototype(const multiname& name)
{
	Prototype* proto = prototype.getPtr();
	while(proto)
	{
		variable *obj = proto->getObj()->findSettable(name);
		if (obj)
			return obj;

		proto = proto->prevPrototype.getPtr();
	}

	return NULL;
}

EARLY_BIND_STATUS Class_base::resolveMultinameStatically(const multiname& name) const
{
	if(findBorrowedGettable(name)!=NULL)
		return BINDED;
	else
		return NOT_BINDED;
}

bool Class_base::checkExistingFunction(const multiname &name)
{
	variable* v = Variables.findObjVar(getSystemState(),name, DECLARED_TRAIT);
	if (!v)
		v = borrowedVariables.findObjVar(getSystemState(),name, DECLARED_TRAIT);
	if (v && asAtomHandler::isValid(v->var))
		return this->isSealed;
	if (!super.isNull())
		return super->checkExistingFunction(name);
	return false;
}

void Class_base::getClassVariableByMultiname(asAtom& ret, const multiname &name, ASWorker* wrk)
{
	uint32_t nsRealId;
	variable* obj = ASObject::findGettableImpl(getSystemState(), borrowedVariables,name,&nsRealId);
	if(!obj && name.hasEmptyNS)
	{
		//Check prototype chain
		Prototype* proto = prototype.getPtr();
		while(proto)
		{
			obj=proto->getObj()->Variables.findObjVar(getSystemState(),name,DECLARED_TRAIT|DYNAMIC_TRAIT,&nsRealId);
			if(obj)
			{
				//It seems valid for a class to redefine only the setter, so if we can't find
				//something to get, it's ok
				if(!(asAtomHandler::isValid(obj->getter) || asAtomHandler::isValid(obj->var)))
					obj=NULL;
			}
			if(obj)
				break;
			proto = proto->prevPrototype.getPtr();
		}
	}
	if(!obj)
		return;


	if (obj->kind == INSTANCE_TRAIT)
	{
		if (getSystemState()->getNamespaceFromUniqueId(nsRealId).kind != STATIC_PROTECTED_NAMESPACE)
		{
			createError<TypeError>(wrk,kCallOfNonFunctionError,name.normalizedNameUnresolved(getSystemState()));
			return;
		}
	}

	if(asAtomHandler::isValid(obj->getter))
	{
		//Call the getter
		LOG_CALL("Calling the getter for " << name << " on " << asAtomHandler::toDebugString(obj->getter));
		assert(asAtomHandler::isFunction(obj->getter));
		asAtomHandler::as<IFunction>(obj->getter)->callGetter(ret,asAtomHandler::getClosure(obj->getter) ? asAtomHandler::getClosure(obj->getter) : this,wrk);
		LOG_CALL("End of getter"<< ' ' << asAtomHandler::toDebugString(obj->getter)<<" result:"<<asAtomHandler::toDebugString(ret));
		return;
	}
	else
	{
		assert_and_throw(asAtomHandler::isInvalid(obj->setter));
		ASATOM_INCREF(obj->var);
		if(asAtomHandler::isFunction(obj->var) && asAtomHandler::getObject(obj->var)->as<IFunction>()->isMethod())
		{
			if (asAtomHandler::as<IFunction>(obj->var)->closure_this)
			{
				LOG_CALL("class function " << name << " is already bound to "<<asAtomHandler::toDebugString(obj->var) );
				ret = obj->var;
			}
			else
			{
				LOG_CALL("Attaching this class " << this->toDebugString() << " to function " << name << " "<<asAtomHandler::toDebugString(obj->var));
				asAtomHandler::setFunction(ret,asAtomHandler::getObject(obj->var),nullptr,wrk);
			}
		}
		else
			ret = obj->var;
	}
}

bool Class_base::isInterfaceMethod(const multiname& name)
{
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = name.name_s_id;
	const std::vector<Class_base*> interfaces = getInterfaces();
	for(unsigned int i=0;i<interfaces.size();i++)
	{
		variable* v = interfaces[i]->borrowedVariables.findObjVar(getSystemState(),m,DECLARED_TRAIT);
		if (v)
			return true;
	}
	return false;
}

void Class_base::removeAllDeclaredProperties()
{
	Variables.removeAllDeclaredProperties();
	borrowedVariables.removeAllDeclaredProperties();
}

multiname* Class_base::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	Prototype* pr = this->getPrototype(wrk);
	ASObject* dynvars =pr ? pr->getWorkerDynamicClassVars() : nullptr;
	if (dynvars)
	{
		if (ASObject::hasPropertyByMultiname(name,false,false,wrk))
			return setVariableByMultiname_intern(name,o,allowConst,this->getClass(),alreadyset,wrk);
		else 
			return dynvars->setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
	}
	return ASObject::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
}

GET_VARIABLE_RESULT Class_base::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	Prototype* pr = this->getPrototype(wrk);
	ASObject* dynvars =pr ? pr->getWorkerDynamicClassVars() : nullptr;
	if (dynvars == nullptr)
		return ASObject::getVariableByMultiname(ret,name,opt,wrk);
	GET_VARIABLE_RESULT res = dynvars->getVariableByMultiname(ret,name,opt,wrk);
	if (asAtomHandler::isValid(ret))
		return res;
	return ASObject::getVariableByMultiname(ret,name,opt,wrk);
}

bool Class_base::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype,ASWorker* wrk)
{
	Prototype* pr = this->getPrototype(wrk);
	ASObject* dynvars =pr ? pr->getWorkerDynamicClassVars() : nullptr;
	if (dynvars && considerDynamic && dynvars->hasPropertyByMultiname(name,considerDynamic, false,wrk))
		return true;
	return ASObject::hasPropertyByMultiname(name,considerDynamic, considerPrototype,wrk);
}

variable* Class_base::findVariableByMultiname(const multiname& name, Class_base* cls, uint32_t* nsRealID, bool* isborrowed, bool considerdynamic, ASWorker* wrk)
{
	variable* v=nullptr;
	Prototype* pr = this->getPrototype(wrk);
	ASObject* dynvars =pr ? pr->getWorkerDynamicClassVars() : nullptr;
	if (dynvars)
		v = dynvars->findVariableByMultiname(name,cls,nsRealID,isborrowed,true,wrk);
	if (!v)
		v = ASObject::findVariableByMultiname(name,cls,nsRealID,isborrowed,considerdynamic,wrk);
	return v;
}

ASQName::ASQName(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_QNAME),uri_is_null(false),uri(0),local_name(0)
{
}
void ASQName::setByXML(XML* node)
{
	uri_is_null=false;
	local_name = getSystemState()->getUniqueStringId(node->getName());
	uri=node->getNamespaceURI();
}

void ASQName::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(c->getSystemState(),_getURI),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localName","",Class<IFunction>::getFunction(c->getSystemState(),_getLocalName),GETTER_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(ASQName,_constructor)
{
	ASQName* th=asAtomHandler::as<ASQName>(obj);
	assert_and_throw(argslen<3);

	asAtom nameval=asAtomHandler::invalidAtom;
	asAtom namespaceval=asAtomHandler::invalidAtom;

	if(argslen==0)
	{
		th->local_name=BUILTIN_STRINGS::EMPTY;
		th->uri_is_null=false;
		th->uri=wrk->getSystemState()->getUniqueStringId(wrk->getDefaultXMLNamespace());
		return;
	}
	if(argslen==1)
	{
		nameval=args[0];
	}
	else if(argslen==2)
	{
		namespaceval=args[0];
		nameval=args[1];
	}

	// Set local_name
	if(asAtomHandler::isQName(nameval))
	{
		ASQName *q=asAtomHandler::as<ASQName>(nameval);
		th->local_name=q->local_name;
		if(asAtomHandler::isInvalid(namespaceval))
		{
			th->uri_is_null=q->uri_is_null;
			th->uri=q->uri;
			return;
		}
	}
	else if(asAtomHandler::isUndefined(nameval))
		th->local_name=BUILTIN_STRINGS::EMPTY;
	else
		th->local_name=asAtomHandler::toStringId(nameval,wrk);

	// Set uri
	th->uri_is_null=false;
	if(asAtomHandler::isInvalid(namespaceval) || asAtomHandler::isUndefined(namespaceval))
	{
		if(th->local_name==BUILTIN_STRINGS::STRING_WILDCARD)
		{
			th->uri_is_null=true;
			th->uri=BUILTIN_STRINGS::EMPTY;
		}
		else
		{
			th->uri=wrk->getDefaultXMLNamespaceID();
		}
	}
	else if(asAtomHandler::isNull(namespaceval))
	{
		th->uri_is_null=true;
		th->uri=BUILTIN_STRINGS::EMPTY;
	}
	else
	{
		if(asAtomHandler::isQName(namespaceval) &&
		   !(asAtomHandler::as<ASQName>(namespaceval)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(namespaceval);
			th->uri=q->uri;
		}
		else if (asAtomHandler::isValid(namespaceval))
			th->uri=asAtomHandler::toStringId(namespaceval,wrk);
	}
}
ASFUNCTIONBODY_ATOM(ASQName,generator)
{
	ASQName* th=Class<ASQName>::getInstanceS(wrk);
	assert_and_throw(argslen<3);

	asAtom nameval=asAtomHandler::invalidAtom;
	asAtom namespaceval=asAtomHandler::invalidAtom;

	if(argslen==0)
	{
		th->local_name=BUILTIN_STRINGS::EMPTY;
		th->uri_is_null=false;
		th->uri=wrk->getDefaultXMLNamespaceID();
		ret = asAtomHandler::fromObject(th);
		return;
	}
	if(argslen==1)
	{
		nameval=args[0];
		namespaceval=asAtomHandler::invalidAtom;
	}
	else if(argslen==2)
	{
		namespaceval=args[0];
		nameval=args[1];
	}

	// Set local_name
	if(asAtomHandler::isQName(nameval))
	{
		ASQName *q=asAtomHandler::as<ASQName>(nameval);
		th->local_name=q->local_name;
		if(asAtomHandler::isInvalid(namespaceval))
		{
			th->uri_is_null=q->uri_is_null;
			th->uri=q->uri;
			ret = asAtomHandler::fromObject(th);
			return;
		}
	}
	else if(asAtomHandler::isUndefined(nameval))
		th->local_name=BUILTIN_STRINGS::EMPTY;
	else
		th->local_name=asAtomHandler::toStringId(nameval,wrk);

	// Set uri
	th->uri_is_null=false;
	if(asAtomHandler::isInvalid(namespaceval) || asAtomHandler::isUndefined(namespaceval))
	{
		if(th->local_name==BUILTIN_STRINGS::STRING_WILDCARD)
		{
			th->uri_is_null=true;
			th->uri=BUILTIN_STRINGS::EMPTY;
		}
		else
		{
			th->uri=wrk->getDefaultXMLNamespaceID();
		}
	}
	else if(asAtomHandler::isNull(namespaceval))
	{
		th->uri_is_null=true;
		th->uri=BUILTIN_STRINGS::EMPTY;
	}
	else
	{
		if(asAtomHandler::isQName(namespaceval) &&
		   !(asAtomHandler::as<ASQName>(namespaceval)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(namespaceval);
			th->uri=q->uri;
		}
		else if (asAtomHandler::isValid(namespaceval))
			th->uri=asAtomHandler::toStringId(namespaceval,wrk);
	}
	ret = asAtomHandler::fromObject(th);
}

ASFUNCTIONBODY_ATOM(ASQName,_getURI)
{
	ASQName* th=asAtomHandler::as<ASQName>(obj);
	if(th->uri_is_null)
		asAtomHandler::setNull(ret);
	else
		ret = asAtomHandler::fromStringID(th->uri);
}

ASFUNCTIONBODY_ATOM(ASQName,_getLocalName)
{
	ASQName* th=asAtomHandler::as<ASQName>(obj);
	ret = asAtomHandler::fromStringID(th->local_name);
}

ASFUNCTIONBODY_ATOM(ASQName,_toString)
{
	if(!asAtomHandler::is<ASQName>(obj))
	{
		createError<TypeError>(wrk,0,"QName.toString is not generic");
		return;
	}
	ASQName* th=asAtomHandler::as<ASQName>(obj);
	ret = asAtomHandler::fromString(wrk->getSystemState(),th->toString());
}

bool ASQName::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_QNAME)
	{
		ASQName *q=static_cast<ASQName *>(o);
		return uri_is_null==q->uri_is_null && uri==q->uri && local_name==q->local_name;
	}

	return ASObject::isEqual(o);
}

tiny_string ASQName::toString()
{
	tiny_string s;
	if(uri_is_null)
		s = "*::";
	else if(uri!=BUILTIN_STRINGS::EMPTY)
		s = getSystemState()->getStringFromUniqueId(uri) + "::";

	return s + getSystemState()->getStringFromUniqueId(local_name);
}

uint32_t ASQName::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<2)
		return cur_index+1;
	else
	{
		//Fall back on object properties
		uint32_t ret=ASObject::nextNameIndex(cur_index-2);
		if(ret==0)
			return 0;
		else
			return ret+2;

	}
}

void ASQName::nextName(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	switch(index)
	{
		case 1:
			ret = asAtomHandler::fromString(getSystemState(),"uri");
			break;
		case 2:
			ret = asAtomHandler::fromString(getSystemState(),"localName");
			break;
		default:
			ASObject::nextName(ret,index-2);
			break;
	}
}

void ASQName::nextValue(asAtom &ret, uint32_t index)
{
	assert_and_throw(implEnable);
	switch(index)
	{
		case 1:
			if (uri_is_null)
				asAtomHandler::setNull(ret);
			else
				ret = asAtomHandler::fromStringID(this->uri);
			break;
		case 2:
			ret = asAtomHandler::fromStringID(this->local_name);
			break;
		default:
			ASObject::nextValue(ret,index-2);
			break;
	}
}

Namespace::Namespace(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_NAMESPACE),nskind(NAMESPACE),prefix_is_undefined(false),uri(BUILTIN_STRINGS::EMPTY),prefix(BUILTIN_STRINGS::EMPTY)
{
}

Namespace::Namespace(ASWorker* wrk,Class_base* c, uint32_t _uri, uint32_t _prefix, NS_KIND _nskind)
  : ASObject(wrk,c,T_NAMESPACE),nskind(_nskind),prefix_is_undefined(false),uri(_uri),prefix(_prefix)
{
}

void Namespace::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	//c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(c->getSystemState(),_setURI),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(c->getSystemState(),_getURI),GETTER_METHOD,true);
	//c->setDeclaredMethodByQName("prefix","",Class<IFunction>::getFunction(c->getSystemState(),_setPrefix),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("prefix","",Class<IFunction>::getFunction(c->getSystemState(),_getPrefix),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),_valueOf),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(c->getSystemState(),_ECMA_valueOf),DYNAMIC_TRAIT);
}

void Namespace::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(Namespace,_constructor)
{
	asAtom urival=asAtomHandler::invalidAtom;
	asAtom prefixval=asAtomHandler::invalidAtom;
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	assert_and_throw(argslen<3);

	if (argslen == 0)
	{
		//Return before resetting the value to preserve those eventually set by the C++ constructor
		return;
	}
	else if (argslen == 1)
	{
		urival = args[0];
		prefixval = asAtomHandler::invalidAtom;
	}
	else
	{
		prefixval = args[0];
		urival = args[1];
	}
	th->prefix_is_undefined=false;
	th->prefix = BUILTIN_STRINGS::EMPTY;
	th->uri = BUILTIN_STRINGS::EMPTY;
;

	if(asAtomHandler::isInvalid(prefixval))
	{
		if(asAtomHandler::isNamespace(urival))
		{
			Namespace* n=asAtomHandler::as<Namespace>(urival);
			th->uri=n->uri;
			th->prefix=n->prefix;
			th->prefix_is_undefined=n->prefix_is_undefined;
		}
		else if(asAtomHandler::isQName(urival) &&
		   !(asAtomHandler::as<ASQName>(urival)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=asAtomHandler::toStringId(urival,wrk);
			if(th->uri!=BUILTIN_STRINGS::EMPTY)
			{
				th->prefix_is_undefined=true;
				th->prefix=BUILTIN_STRINGS::EMPTY;
			}
		}
	}
	else // has both urival and prefixval
	{
		if(asAtomHandler::isQName(urival) &&
		   !(asAtomHandler::as<ASQName>(urival)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=asAtomHandler::toStringId(urival,wrk);
		}

		if(th->uri==BUILTIN_STRINGS::EMPTY)
		{
			if(asAtomHandler::isUndefined(prefixval) ||
			   asAtomHandler::toString(prefixval,wrk)=="")
				th->prefix=BUILTIN_STRINGS::EMPTY;
			else
			{
				tiny_string s = asAtomHandler::toString(prefixval,wrk);
				createError<TypeError>(wrk,kXMLNamespaceWithPrefixAndNoURI,s);
				return;
			}
		}
		else if(asAtomHandler::isUndefined(prefixval) ||
			!isXMLName(wrk,prefixval))
		{
			th->prefix_is_undefined=true;
			th->prefix=BUILTIN_STRINGS::EMPTY;
		}
		else
		{
			th->prefix=asAtomHandler::toStringId(prefixval,wrk);
		}
	}
}
ASFUNCTIONBODY_ATOM(Namespace,generator)
{
	Namespace* th=Class<Namespace>::getInstanceS(wrk);
	asAtom urival=asAtomHandler::invalidAtom;
	asAtom prefixval=asAtomHandler::invalidAtom;
	assert_and_throw(argslen<3);

	if (argslen == 0)
	{
		th->prefix_is_undefined=false;
		th->prefix = BUILTIN_STRINGS::EMPTY;
		th->uri = BUILTIN_STRINGS::EMPTY;
		ret = asAtomHandler::fromObject(th);
		return;
	}
	else if (argslen == 1)
	{
		urival = args[0];
		prefixval = asAtomHandler::invalidAtom;
	}
	else
	{
		prefixval = args[0];
		urival = args[1];
	}
	th->prefix_is_undefined=false;
	th->prefix = BUILTIN_STRINGS::EMPTY;
	th->uri = BUILTIN_STRINGS::EMPTY;

	if(asAtomHandler::isInvalid(prefixval))
	{
		if(asAtomHandler::isNamespace(urival))
		{
			Namespace* n=asAtomHandler::as<Namespace>(urival);
			th->uri=n->uri;
			th->prefix=n->prefix;
			th->prefix_is_undefined=n->prefix_is_undefined;
		}
		else if(asAtomHandler::isQName(urival) &&
		   !(asAtomHandler::as<ASQName>(urival)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=asAtomHandler::toStringId(urival,wrk);
			if(th->uri!=BUILTIN_STRINGS::EMPTY)
			{
				th->prefix_is_undefined=true;
				th->prefix=BUILTIN_STRINGS::EMPTY;
			}
		}
	}
	else // has both urival and prefixval
	{
		if(asAtomHandler::isQName(urival) &&
		   !(asAtomHandler::as<ASQName>(urival)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=asAtomHandler::toStringId(urival,wrk);
		}

		if(th->uri==BUILTIN_STRINGS::EMPTY)
		{
			if(asAtomHandler::isUndefined(prefixval) ||
			   asAtomHandler::toString(prefixval,wrk)=="")
				th->prefix=BUILTIN_STRINGS::EMPTY;
			else
			{
				createError<TypeError>(wrk,0,"Namespace prefix for empty uri not allowed");
				return;
			}
		}
		else if(asAtomHandler::isUndefined(prefixval) ||
			!isXMLName(wrk,prefixval))
		{
			th->prefix_is_undefined=true;
			th->prefix=BUILTIN_STRINGS::EMPTY;
		}
		else
		{
			th->prefix=asAtomHandler::toStringId(prefixval,wrk);
		}
	}
	ret = asAtomHandler::fromObject(th);
}
/*
ASFUNCTIONBODY_ATOM(Namespace,_setURI)
{
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	th->uri=asAtomHandler::toString(args[0],);
}
*/
ASFUNCTIONBODY_ATOM(Namespace,_getURI)
{
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	ret = asAtomHandler::fromStringID(th->uri);
}
/*
ASFUNCTIONBODY_ATOM(Namespace,_setPrefix)
{
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	if(asAtomHandler::isUndefined(args[0]))
	{
		th->prefix_is_undefined=true;
		th->prefix="";
	}
	else
	{
		th->prefix_is_undefined=false;
		th->prefix=asAtomHandler::toString(args[0],);
	}
}
*/
ASFUNCTIONBODY_ATOM(Namespace,_getPrefix)
{
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	if(th->prefix_is_undefined)
		asAtomHandler::setUndefined(ret);
	else
		ret = asAtomHandler::fromStringID(th->prefix);
}

ASFUNCTIONBODY_ATOM(Namespace,_toString)
{
	if(!asAtomHandler::is<Namespace>(obj))
	{
		createError<TypeError>(wrk,0,"Namespace.toString is not generic");
		return;
	}
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	ret = asAtomHandler::fromStringID(th->uri);
}

ASFUNCTIONBODY_ATOM(Namespace,_valueOf)
{
	ret = asAtomHandler::fromStringID(asAtomHandler::as<Namespace>(obj)->uri);
}

ASFUNCTIONBODY_ATOM(Namespace,_ECMA_valueOf)
{
	if(!asAtomHandler::is<Namespace>(obj))
	{
		createError<TypeError>(wrk,0,"Namespace.valueOf is not generic");
		return;
	}
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	ret = asAtomHandler::fromStringID(th->uri);
}

bool Namespace::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_NAMESPACE)
	{
		Namespace *n=static_cast<Namespace *>(o);
		return uri==n->uri;
	}

	return ASObject::isEqual(o);
}

uint32_t Namespace::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<2)
		return cur_index+1;
	else
	{
		//Fall back on object properties
		uint32_t ret=ASObject::nextNameIndex(cur_index-2);
		if(ret==0)
			return 0;
		else
			return ret+2;

	}
}

void Namespace::nextName(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	switch(index)
	{
		case 1:
			ret = asAtomHandler::fromString(getSystemState(),"uri");
			break;
		case 2:
			ret = asAtomHandler::fromString(getSystemState(),"prefix");
			break;
		default:
			ASObject::nextName(ret,index-2);
			break;
	}
}

void Namespace::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	switch(index)
	{
		case 1:
			ret = asAtomHandler::fromStringID(this->uri);
			break;
		case 2:
			if(prefix_is_undefined)
				asAtomHandler::setUndefined(ret);
			else
				ret = asAtomHandler::fromStringID(this->prefix);
			break;
		default:
			ASObject::nextValue(ret,index-2);
			break;
	}
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


Global::Global(ASWorker* wrk, Class_base* cb, ABCContext* c, int s, bool avm1):ASObject(wrk,cb,T_OBJECT,SUBTYPE_GLOBAL),scriptId(s),context(c),isavm1(avm1)
{
}

void Global::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef(c->getSystemState()));
}

void Global::getVariableByMultinameOpportunistic(asAtom& ret, const multiname& name,ASWorker* wrk)
{
	ASObject::getVariableByMultinameIntern(ret,name,this->getClass(),NO_INCREF,wrk);
	//Do not attempt to define the variable now in any case
}

GET_VARIABLE_RESULT Global::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	GET_VARIABLE_RESULT res = getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	/*
	 * All properties are registered by now, even if the script init has
	 * not been run. Thus if ret == NULL, we don't have to run the script init.
	 */
	if(asAtomHandler::isInvalid(ret) || !context || context->hasRunScriptInit[scriptId])
		return res;
	LOG_CALL("Access to " << name << ", running script init");
	asAtom v = asAtomHandler::fromObject(this);
	context->runScriptInit(scriptId,v);
	return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
}

multiname *Global::setVariableByMultiname(multiname &name, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool *alreadyset,ASWorker* wrk)
{
	if (context && !context->hasRunScriptInit[scriptId])
	{
		LOG_CALL("Access to " << name << ", running script init");
		asAtom v = asAtomHandler::fromObject(this);
		context->runScriptInit(scriptId,v);
	}
	return setVariableByMultiname_intern(name,o,allowConst,this->getClass(),alreadyset,wrk);
}

void Global::registerBuiltin(const char* name, const char* ns, _R<ASObject> o, NS_KIND nskind)
{
	o->incRef();
	o->setRefConstant();
	if (o->is<Class_base>())
		o->as<Class_base>()->setGlobalScope(this);
	setVariableByQName(name,nsNameAndKind(getSystemState(),ns,nskind),o.getPtr(),CONSTANT_TRAIT);
}

void Global::checkScriptInit()
{
	if(context->hasRunScriptInit[scriptId])
		return;
	LOG_CALL("running script init "<<scriptId);
	asAtom v = asAtomHandler::fromObject(this);
	context->runScriptInit(scriptId,v);
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

EARLY_BIND_STATUS ActivationType::resolveMultinameStatically(const multiname& name) const
{
	std::cerr << "Looking for " << name << std::endl;
	for(unsigned int i=0;i<mi->body->trait_count;i++)
	{
		const traits_info* t=&mi->body->traits[i];
		multiname* mname=mi->context->getMultiname(t->name,nullptr);
		std::cerr << "\t in " << *mname << std::endl;
		assert_and_throw(mname->ns.size()==1 && mname->name_type==multiname::NAME_STRING);
		if(mname->name_s_id!=name.normalizedNameId(mi->context->root->getSystemState()))
			continue;
		if(find(name.ns.begin(),name.ns.end(),mname->ns[0])==name.ns.end())
			continue;
		return BINDED;
	}
	return NOT_BINDED;
}

const multiname* ActivationType::resolveSlotTypeName(uint32_t slotId) const
{
	std::cerr << "Resolving type at id " << slotId << std::endl;
	for(unsigned int i=0;i<mi->body->trait_count;i++)
	{
		const traits_info* t=&mi->body->traits[i];
		if(t->slot_id!=slotId)
			continue;

		multiname* tname=mi->context->getMultiname(t->type_name,nullptr);
		return tname;
	}
	return nullptr;
}

void Prototype::setVariableByQName(const tiny_string &name, const tiny_string &ns, ASObject *o, TRAIT_KIND traitKind)
{
	if (o->is<Function>())
		o->as<Function>()->setRefConstant();
	obj->setVariableByQName(name,ns,o,traitKind);
	originalPrototypeVars->setVariableByQName(name,ns,o,traitKind);
}
void Prototype::setVariableByQName(const tiny_string &name, const nsNameAndKind &ns, ASObject *o, TRAIT_KIND traitKind)
{
	if (o->is<Function>())
		o->as<Function>()->setRefConstant();
	uint32_t nameID = obj->getSystemState()->getUniqueStringId(name);
	obj->setVariableByQName(nameID,ns,o,traitKind);
	originalPrototypeVars->setVariableByQName(nameID,ns,o,traitKind);
}
void Prototype::setVariableByQName(uint32_t nameID, const nsNameAndKind &ns, ASObject *o, TRAIT_KIND traitKind)
{
	if (o->is<Function>())
		o->as<Function>()->setRefConstant();
	obj->setVariableByQName(nameID,ns,o,traitKind);
	originalPrototypeVars->setVariableByQName(nameID,ns,o,traitKind);
}

void Prototype::setVariableAtomByQName(const tiny_string &name, const nsNameAndKind &ns, asAtom o, TRAIT_KIND traitKind)
{
	if (asAtomHandler::is<Function>(o))
		asAtomHandler::as<Function>(o)->setRefConstant();
	uint32_t nameID = obj->getSystemState()->getUniqueStringId(name);
	obj->setVariableAtomByQName(nameID,ns,o,traitKind);
	originalPrototypeVars->setVariableAtomByQName(nameID,ns,o,traitKind);
}

void Prototype::setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable)
{
	obj->setDeclaredMethodByQName(name, ns, o, type, isBorrowed, isEnumerable);
	o->setRefConstant();
}
void Prototype::copyOriginalValues(Prototype* target)
{
	if (this->prevPrototype)
		this->prevPrototype->copyOriginalValues(target);
	originalPrototypeVars->copyValues(target->getObj(),target->getObj()->getInstanceWorker());
	if (!target->workerDynamicClassVars)
		target->workerDynamicClassVars = new_asobject(target->getObj()->getInstanceWorker());
	target->getObj()->setRefConstant();
}

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
		ASATOM_DECREF(it->second);
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
		ASATOM_DECREF(it->second);
	}
	scopevariables.clear();
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
		ASATOM_DECREF(it->second);
	}
	scopevariables.clear();
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
		ASATOM_DECREF(scopevariables [it->first]);
		ASATOM_INCREF(it->second);
		scopevariables [it->first]=it->second;
	}
}
