/**************************************************************************
    Lightspark, a free flash player implementation

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
#include "scripting/abc_optimized.h"
#include "scripting/abc_optimized_callproperty.h"
#include "scripting/class.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/RegExp.h"

using namespace std;
using namespace lightspark;

void setCallException(const asAtom& obj, multiname* name, ASWorker* wrk)
{
	ASObject* pobj = asAtomHandler::getObject(obj);
	Class_base* cls = asAtomHandler::getClass(obj,wrk->getSystemState(),false) ;
	if (asAtomHandler::isPrimitive(obj))
	{
		tiny_string clsname = cls ? cls->getQualifiedClassName() : asAtomHandler::toString(obj,wrk);
		createError<TypeError>(wrk,kCallOfNonFunctionError, name->qualifiedString(wrk->getSystemState()), clsname);
	}
	else if (pobj && pobj->hasPropertyByMultiname(*name,true,true,wrk))
	{
		tiny_string clsname = pobj->getClass()->getQualifiedClassName();
		createError<ReferenceError>(wrk,kWriteOnlyError, name->normalizedName(wrk), clsname);
	}
	else if (cls && cls->isSealed)
	{
		tiny_string clsname = cls->getQualifiedClassName();
		createError<ReferenceError>(wrk,kReadSealedError, name->normalizedName(wrk), clsname);
	}
	else if (pobj && pobj->is<Class_base>())
	{
		tiny_string clsname = pobj->as<Class_base>()->class_name.getQualifiedName(wrk->getSystemState(),true);
		createError<TypeError>(wrk,kCallOfNonFunctionError, name->qualifiedString(wrk->getSystemState()), clsname);
	}
	else
	{
		tiny_string clsname = cls ? cls->getQualifiedClassName() : asAtomHandler::toString(obj,wrk);
		createError<TypeError>(wrk,kCallNotFoundError, name->qualifiedString(wrk->getSystemState()), clsname);
	}
}

FORCE_INLINE void callprop_intern(call_context* context,asAtom& ret,asAtom& obj,asAtom* args, uint32_t argsnum,multiname* name,preloadedcodedata* cacheptr,bool refcounted, bool needreturn, bool coercearguments)
{
	assert(context->worker==getWorker());
	if ((cacheptr->local2.flags&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		if (asAtomHandler::isObject(obj) &&
				((asAtomHandler::is<Class_base>(obj) && asAtomHandler::getObjectNoCheck(obj) == cacheptr->cacheobj1)
				|| asAtomHandler::getObjectNoCheck(obj)->getClass() == cacheptr->cacheobj1))
		{
			asAtom o = asAtomHandler::fromObjectNoPrimitive(cacheptr->cacheobj3);
			LOG_CALL( "callProperty from cache:"<<*name<<" "<<asAtomHandler::toDebugString(obj)<<" "<<asAtomHandler::toDebugString(o)<<" "<<coercearguments);
			if(asAtomHandler::is<IFunction>(o))
				asAtomHandler::callFunction(o,context->worker,ret,obj,args,argsnum,refcounted,needreturn && coercearguments,coercearguments);
			else if(asAtomHandler::is<Class_base>(o))
			{
				asAtomHandler::as<Class_base>(o)->generator(context->worker,ret,args,argsnum);
				if (refcounted)
				{
					for(uint32_t i=0;i<argsnum;i++)
						ASATOM_DECREF(args[i]);
					ASATOM_DECREF(obj);
				}
			}
			else if(asAtomHandler::is<RegExp>(o))
				RegExp::exec(ret,context->worker,o,args,argsnum);
			else
			{
				LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
				createError<TypeError>(context->worker,kCallOfNonFunctionError, "Object");
			}
			if (needreturn && asAtomHandler::isInvalid(ret))
				ret = asAtomHandler::undefinedAtom;
			LOG_CALL("End of calling cached property "<<*name<<" "<<asAtomHandler::toDebugString(ret));
			return;
		}
		else
		{
			if (cacheptr->cacheobj3 && cacheptr->cacheobj3->is<Function>() && cacheptr->cacheobj3->as<IFunction>()->clonedFrom)
				cacheptr->cacheobj3->decRef();
			cacheptr->local2.flags |= ABC_OP_NOTCACHEABLE;
			cacheptr->local2.flags &= ~ABC_OP_CACHED;
		}
	}
	if(asAtomHandler::is<Null>(obj))
	{
		LOG(LOG_ERROR,"trying to call property on null:"<<*name);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		if (refcounted)
		{
			for(uint32_t i=0;i<argsnum;i++)
				ASATOM_DECREF(args[i]);
			ASATOM_DECREF(obj);
		}
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		LOG(LOG_ERROR,"trying to call property on undefined2:"<<*name);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		if (refcounted)
		{
			for(uint32_t i=0;i<argsnum;i++)
				ASATOM_DECREF(args[i]);
			ASATOM_DECREF(obj);
		}
		return;
	}
	asAtom o=asAtomHandler::invalidAtom;
	bool canCache = false;
	asAtomHandler::getVariableByMultiname(obj,o,*name,context->worker,canCache,GET_VARIABLE_OPTION(GET_VARIABLE_OPTION::SKIP_IMPL|GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE));
	name->resetNameIfObject();
	if(asAtomHandler::isInvalid(o) && asAtomHandler::is<Class_base>(obj))
	{
		// check super classes
		_NR<Class_base> tmpcls = asAtomHandler::as<Class_base>(obj)->super;
		while (tmpcls && !tmpcls.isNull())
		{
			tmpcls->getVariableByMultiname(o,*name, GET_VARIABLE_OPTION(SKIP_IMPL),context->worker);
			if(asAtomHandler::isValid(o))
			{
				canCache = true;
				break;
			}
			tmpcls = tmpcls->super;
		}
	}
	if(asAtomHandler::isValid(o) && !asAtomHandler::is<Proxy>(obj))
	{
		if(asAtomHandler::is<IFunction>(o))
		{
			if (canCache
					&& (cacheptr->local2.flags & ABC_OP_NOTCACHEABLE)==0
					&& asAtomHandler::canCacheMethod(obj,name)
					&& asAtomHandler::isObject(o)
					&& !asAtomHandler::as<IFunction>(o)->clonedFrom
					&& ((asAtomHandler::is<Class_base>(obj) && asAtomHandler::as<IFunction>(o)->inClass == asAtomHandler::as<Class_base>(obj))
						|| (asAtomHandler::as<IFunction>(o)->inClass && asAtomHandler::getClass(obj,context->sys)->isSubClass(asAtomHandler::as<IFunction>(o)->inClass))))
			{
				// cache method if multiname is static and it is a method of a sealed class
				cacheptr->local2.flags |= ABC_OP_CACHED;
				if (argsnum==2 && asAtomHandler::is<SyntheticFunction>(o) && cacheptr->cacheobj1 && cacheptr->cacheobj3) // special case 2 parameters with known parameter types: check if coercion can be skipped
				{
					SyntheticFunction* f = asAtomHandler::as<SyntheticFunction>(o);
					if (!f->getMethodInfo()->returnType)
						f->checkParamTypes();
					if (f->getMethodInfo()->paramTypes.size()==2 &&
						f->canSkipCoercion(0,(Class_base*)cacheptr->cacheobj1) &&
						f->canSkipCoercion(1,(Class_base*)cacheptr->cacheobj3))
					{
						cacheptr->local2.flags |= ABC_OP_COERCED;
					}
				}
				cacheptr->cacheobj1 = asAtomHandler::getClass(obj,context->sys);
				cacheptr->cacheobj3 = asAtomHandler::getObjectNoCheck(o);
				LOG_CALL("caching callproperty:"<<*name<<" "<<cacheptr->cacheobj1->toDebugString()<<" "<<cacheptr->cacheobj3->toDebugString());
			}
			else
			{
				cacheptr->local2.flags |= ABC_OP_NOTCACHEABLE;
				cacheptr->local2.flags &= ~ABC_OP_CACHED;
			}
			asAtom closure = asAtomHandler::getClosureAtom(o,obj);
			if (refcounted && closure.uintval != obj.uintval && !(cacheptr->local2.flags & ABC_OP_CACHED) && asAtomHandler::as<IFunction>(o)->clonedFrom)
				ASATOM_INCREF(closure);
			asAtomHandler::callFunction(o,context->worker,ret,closure,args,argsnum,refcounted,needreturn && coercearguments,coercearguments);
			if (needreturn && asAtomHandler::isInvalid(ret))
				ret = asAtomHandler::undefinedAtom;
			if (refcounted && closure.uintval != obj.uintval)
				ASATOM_DECREF(obj);
		}
		else if(asAtomHandler::is<Class_base>(o))
		{
			asAtomHandler::as<Class_base>(o)->generator(context->worker,ret,args,argsnum);
			if (refcounted)
			{
				for(uint32_t i=0;i<argsnum;i++)
					ASATOM_DECREF(args[i]);
				ASATOM_DECREF(obj);
			}
		}
		else if(asAtomHandler::is<RegExp>(o))
			RegExp::exec(ret,context->worker,o,args,argsnum);
		else
		{
			LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
			createError<TypeError>(context->worker,kCallOfNonFunctionError, "Object");
			ASATOM_DECREF(o);
			return;
		}
	}
	else
	{
		//If the object is a Proxy subclass, try to invoke callProperty
		if(asAtomHandler::is<Proxy>(obj))
		{
			//Check if there is a custom caller defined, skipping implementation to avoid recursive calls
			multiname callPropertyName(nullptr);
			callPropertyName.name_type=multiname::NAME_STRING;
			callPropertyName.name_s_id=context->sys->getUniqueStringId("callProperty");
			callPropertyName.ns.emplace_back(context->sys,flash_proxy,NAMESPACE);
			asAtom oproxy=asAtomHandler::invalidAtom;
			ASObject* pobj = asAtomHandler::getObjectNoCheck(obj);
			GET_VARIABLE_RESULT res = pobj->getVariableByMultiname(oproxy,callPropertyName,SKIP_IMPL,context->worker);
			if(asAtomHandler::isValid(oproxy))
			{
				assert_and_throw(asAtomHandler::isFunction(oproxy));
				if(asAtomHandler::isValid(o))
				{
					if(asAtomHandler::is<IFunction>(o))
						asAtomHandler::callFunction(o,context->worker,ret,obj,args,argsnum,refcounted,needreturn && coercearguments,coercearguments);
					else if(asAtomHandler::is<Class_base>(o))
					{
						asAtomHandler::as<Class_base>(o)->generator(context->worker,ret,args,argsnum);
						if (refcounted)
						{
							for(uint32_t i=0;i<argsnum;i++)
								ASATOM_DECREF(args[i]);
							ASATOM_DECREF(obj);
						}
					}
					else if(asAtomHandler::is<RegExp>(o))
						RegExp::exec(ret,context->worker,o,args,argsnum);
					else
					{
						LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
						createError<TypeError>(context->worker,kCallOfNonFunctionError, "Object");
						if (res & GET_VARIABLE_RESULT::GETVAR_ISNEWOBJECT)
							ASATOM_DECREF(oproxy);
						ASATOM_DECREF(o);
						return;
					}
					ASATOM_DECREF(o);
				}
				else
				{
					//Create a new array
					asAtom* proxyArgs=g_newa(asAtom,argsnum+1);
					ASObject* namearg = abstract_s(context->worker,name->normalizedName(context->worker));
					namearg->setProxyProperty(*name);
					proxyArgs[0]=asAtomHandler::fromObject(namearg);
					for(uint32_t i=0;i<argsnum;i++)
						proxyArgs[i+1]=args[i];

					//We now suppress special handling
					LOG_CALL("Proxy::callProperty");
					ASATOM_INCREF(oproxy);
					if (!refcounted)
					 	ASATOM_INCREF(obj);
					asAtomHandler::callFunction(oproxy,context->worker,ret,obj,proxyArgs,argsnum+1,true,needreturn && coercearguments,coercearguments);
					ASATOM_DECREF(oproxy);
				}
				LOG_CALL("End of calling proxy custom caller " << *name << " "<<asAtomHandler::toDebugString(oproxy)<<" "<<res);
				if (res & GET_VARIABLE_RESULT::GETVAR_ISNEWOBJECT)
					ASATOM_DECREF(oproxy);
				return;
			}
			else if(asAtomHandler::isValid(o))
			{
				if(asAtomHandler::is<IFunction>(o))
					asAtomHandler::callFunction(o,context->worker,ret,obj,args,argsnum,refcounted,needreturn && coercearguments,coercearguments);
				else if(asAtomHandler::is<Class_base>(o))
				{
					asAtomHandler::as<Class_base>(o)->generator(context->worker,ret,args,argsnum);
					if (refcounted)
					{
						for(uint32_t i=0;i<argsnum;i++)
							ASATOM_DECREF(args[i]);
						ASATOM_DECREF(obj);
					}
				}
				else if(asAtomHandler::is<RegExp>(o))
					RegExp::exec(ret,context->worker,o,args,argsnum);
				else
				{
					LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
					createError<TypeError>(context->worker,kCallOfNonFunctionError, "Object");
					return;
				}
				LOG_CALL("End of calling proxy " << *name);
			}
		}
		if (asAtomHandler::isInvalid(ret))
		{
			setCallException(obj,name,context->worker);
			asAtomHandler::setUndefined(ret);
		}
	}

	ASATOM_DECREF(o);
	LOG_CALL("End of calling " << *name<<" "<<asAtomHandler::toDebugString(ret));
}

FORCE_INLINE void callprop_intern_slotvar(call_context* context,asAtom& ret,asAtom& obj,asAtom* args, uint32_t argsnum,uint32_t slot_id,preloadedcodedata* cacheptr,bool refcounted, bool needreturn, bool coercearguments)
{
	assert(context->worker==getWorker());
	if(asAtomHandler::is<Null>(obj))
	{
		LOG(LOG_ERROR,"trying to call property on null:"<<slot_id);
		createError<TypeError>(context->worker,kConvertNullToObjectError);
		if (refcounted)
		{
			for(uint32_t i=0;i<argsnum;i++)
				ASATOM_DECREF(args[i]);
			ASATOM_DECREF(obj);
		}
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		LOG(LOG_ERROR,"trying to call property on undefined3:"<<slot_id);
		createError<TypeError>(context->worker,kConvertUndefinedToObjectError);
		if (refcounted)
		{
			for(uint32_t i=0;i<argsnum;i++)
				ASATOM_DECREF(args[i]);
			ASATOM_DECREF(obj);
		}
		return;
	}
	assert(asAtomHandler::isObject(obj));
	asAtom o=asAtomHandler::invalidAtom;
	o = asAtomHandler::getObject(obj)->getSlotNoCheck(slot_id);
	ASATOM_INCREF(o);

	if(asAtomHandler::is<IFunction>(o))
	{
		asAtom closure = asAtomHandler::getClosureAtom(o,obj);
		if (refcounted && closure.uintval != obj.uintval && asAtomHandler::as<IFunction>(o)->clonedFrom)
			ASATOM_INCREF(closure);
		asAtomHandler::callFunction(o,context->worker,ret,closure,args,argsnum,refcounted,needreturn && coercearguments,coercearguments);
		if (needreturn && asAtomHandler::isInvalid(ret))
			ret = asAtomHandler::undefinedAtom;
		if (refcounted && closure.uintval != obj.uintval)
			ASATOM_DECREF(obj);
	}
	else if(asAtomHandler::is<Class_base>(o))
	{
		asAtomHandler::as<Class_base>(o)->generator(context->worker,ret,args,argsnum);
		if (refcounted)
		{
			for(uint32_t i=0;i<argsnum;i++)
				ASATOM_DECREF(args[i]);
			ASATOM_DECREF(obj);
		}
	}
	else if(asAtomHandler::is<RegExp>(o))
		RegExp::exec(ret,context->worker,o,args,argsnum);
	else
	{
		LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(o) <<" on "<<asAtomHandler::toDebugString(obj));
		createError<TypeError>(context->worker,kCallOfNonFunctionError, "Object");
		ASATOM_DECREF(o);
		return;
	}

	LOG_CALL("End of calling " << asAtomHandler::toDebugString(o)<<" "<<asAtomHandler::toDebugString(ret));
	ASATOM_DECREF(o);
}

void lightspark::abc_callpropertyStaticName(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = context->exec_pos->local2.pos;
	multiname* name=(++context->exec_pos)->cachedmultiname2;
	LOG_CALL( "callProperty_staticname " << *name<<" "<<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*args,args+1,argcount,name,instrptr,true,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	RUNTIME_STACK_POP_CREATE(context,args);
	multiname* name=instrptr->cachedmultiname2;

	LOG_CALL( "callProperty_l " << *name);
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,*obj,args,1,name,context->exec_pos,true,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticNameCached(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom ret=asAtomHandler::invalidAtom;
	if (instrptr->local2.flags&ABC_OP_FROMGLOBAL && instrptr->cachedvar3 && asAtomHandler::is<SyntheticFunction>(instrptr->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(instrptr->cachedvar3->var,*obj);
		asAtomHandler::callFunction(instrptr->cachedvar3->var,context->worker,ret,closure,args,argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern(context,ret,*obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(*obj);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticNameCached_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached_lr " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom ret=asAtomHandler::invalidAtom;
	if (instrptr->local2.flags&ABC_OP_FROMGLOBAL && instrptr->cachedvar3 && asAtomHandler::is<SyntheticFunction>(instrptr->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(instrptr->cachedvar3->var,*obj);
		asAtomHandler::callFunction(instrptr->cachedvar3->var,context->worker,ret,closure,args,argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern(context,ret,*obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	REPLACELOCALRESULT(context,context->exec_pos->local3.pos,ret);
	ASATOM_DECREF(*obj);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticNameCached_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached_c " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *(++context->exec_pos)->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	if (instrptr->local2.flags&ABC_OP_FROMGLOBAL && instrptr->cachedvar3 && asAtomHandler::is<SyntheticFunction>(instrptr->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(instrptr->cachedvar3->var,obj);
		asAtomHandler::callFunction(instrptr->cachedvar3->var,context->worker,ret,closure,args,argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticNameCached_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached_l " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = CONTEXT_GETLOCAL(context,(++context->exec_pos)->local_pos2);
	asAtom ret=asAtomHandler::invalidAtom;
	if (instrptr->local2.flags&ABC_OP_FROMGLOBAL && instrptr->cachedvar3 && asAtomHandler::is<SyntheticFunction>(instrptr->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(instrptr->cachedvar3->var,obj);
		asAtomHandler::callFunction(instrptr->cachedvar3->var,context->worker,ret,closure,args,argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticNameCached_constant_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached_cl " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *(++context->exec_pos)->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	if (instrptr->local2.flags&ABC_OP_FROMGLOBAL && instrptr->cachedvar3 && asAtomHandler::is<SyntheticFunction>(instrptr->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(instrptr->cachedvar3->var,obj);
		asAtomHandler::callFunction(instrptr->cachedvar3->var,context->worker,ret,closure,args,argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	REPLACELOCALRESULT(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticNameCached_local_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callProperty_staticnameCached_ll " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = CONTEXT_GETLOCAL(context,(++context->exec_pos)->local_pos2);
	asAtom ret=asAtomHandler::invalidAtom;
	if (instrptr->local2.flags&ABC_OP_FROMGLOBAL && instrptr->cachedvar3 && asAtomHandler::is<SyntheticFunction>(instrptr->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(instrptr->cachedvar3->var,obj);
		asAtomHandler::callFunction(instrptr->cachedvar3->var,context->worker,ret,closure,args,argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	REPLACELOCALRESULT(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=*instrptr->arg2_constant;
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=*instrptr->arg2_constant;

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_lc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cl " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_ll " << *name<<" "<<instrptr->local_pos1<<" "<<instrptr->local_pos2<<" "<<asAtomHandler::toDebugString(obj)<<" "<<asAtomHandler::toDebugString(args));
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=*instrptr->arg2_constant;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_ccl " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,&args,1,name,context->exec_pos,false,true,coerce);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=*instrptr->arg2_constant;

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_lcl " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,&args,1,name,context->exec_pos,false,true,coerce);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_cll " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,&args,1,name,context->exec_pos,false,true,coerce);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=(++(context->exec_pos))->cachedmultiname2;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;

	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_lll " << *name);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern(context,res,obj,&args,1,name,context->exec_pos,false,true,coerce);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));


	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_noargs_c " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,true,false);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));


	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_noargs_l " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	if (context->exec_pos->local2.flags&ABC_OP_FROMGLOBAL && context->exec_pos->cachedvar3 && asAtomHandler::is<SyntheticFunction>(context->exec_pos->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(context->exec_pos->cachedvar3->var,obj);
		asAtomHandler::callFunction(context->exec_pos->cachedvar3->var,context->worker,ret,closure,nullptr,0,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,true,false);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));


	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callProperty_noargs_c_lr " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	if (context->exec_pos->local2.flags&ABC_OP_FROMGLOBAL && context->exec_pos->cachedvar3 && asAtomHandler::is<SyntheticFunction>(context->exec_pos->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(context->exec_pos->cachedvar3->var,obj);
		asAtomHandler::callFunction(context->exec_pos->cachedvar3->var,context->worker,ret,closure,nullptr,0,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,true,false);
	REPLACELOCALRESULT(context,instrptr->local3.pos,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertyStaticName_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callProperty_noargs_l_lr " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	if (context->exec_pos->local2.flags&ABC_OP_FROMGLOBAL && context->exec_pos->cachedvar3 && asAtomHandler::is<SyntheticFunction>(context->exec_pos->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(context->exec_pos->cachedvar3->var,obj);
		asAtomHandler::callFunction(context->exec_pos->cachedvar3->var,context->worker,ret,closure,nullptr,0,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,true,false);
	REPLACELOCALRESULT(context,instrptr->local3.pos,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropvoidStaticName(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	multiname* name=(++context->exec_pos)->cachedmultiname2;
	LOG_CALL( "callPropvoid_staticname " << *name<<" "<<argcount);
	RUNTIME_STACK_POP_N_CREATE(context,argcount+1,args);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*args,args+1,argcount,name,instrptr,true,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropvoidStaticNameCached(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callPropvoid_staticnameCached " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	RUNTIME_STACK_POP_CREATE(context,obj);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,*obj,args,argcount,name,instrptr,false,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	ASATOM_DECREF_POINTER(obj);
	++(context->exec_pos);
}
void lightspark::abc_callpropvoidStaticNameCached_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callPropvoid_staticnameCached_c " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *(++context->exec_pos)->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropvoidStaticNameCached_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	multiname* name=(context->exec_pos+1)->cachedmultiname3;
	LOG_CALL( "callPropvoid_staticnameCached_l " << *name<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = CONTEXT_GETLOCAL(context,(++context->exec_pos)->local_pos2);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,args,argcount,name,instrptr,false,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropvoidStaticName_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=*instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoidStaticName_cc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropvoidStaticName_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=*instrptr->arg2_constant;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropvoidStaticName_lc " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropvoidStaticName_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoidStaticName_cl " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropvoidStaticName_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));

	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropvoidStaticName_ll " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,&args,1,name,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropvoidStaticName_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoidStaticName_noargs_c " << *name);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,false,false);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropvoidStaticName_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	multiname* name=context->exec_pos->cachedmultiname2;
	(++(context->exec_pos));

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropvoid_noargs_l " << *name<<" "<<asAtomHandler::toDebugString(obj));
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern(context,ret,obj,nullptr,0,name,context->exec_pos,false,false,false);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}


void lightspark::abc_callpropvoidSlotVarCached_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	uint32_t slot_id=instrptr->local3.pos;
	LOG_CALL( "callPropvoid_SlotVarCached_c " << slot_id<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *(++context->exec_pos)->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,args,argcount,slot_id,instrptr,false,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropvoidSlotVarCached_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	uint32_t slot_id=instrptr->local3.pos;
	LOG_CALL( "callPropvoid_SlotVarCached_l " << slot_id<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = CONTEXT_GETLOCAL(context,(++context->exec_pos)->local_pos2);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,args,argcount,slot_id,instrptr,false,false,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropvoidSlotVar_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom args=*instrptr->arg2_constant;
	(++(context->exec_pos));
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoidSlotVar_cc " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,&args,1,slot_id,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropvoidSlotVar_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom args=*instrptr->arg2_constant;
	(++(context->exec_pos));
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropvoidSlotVar_lc " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,&args,1,slot_id,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropvoidSlotVar_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	(++(context->exec_pos));
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoidSlotVar_cl " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,&args,1,slot_id,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropvoidSlotVar_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);
	(++(context->exec_pos));
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropvoidSlotVar_ll " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,&args,1,slot_id,context->exec_pos,false,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropvoidSlotVar_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropvoidSlotVar_noargs_c " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,nullptr,0,slot_id,context->exec_pos,false,false,false);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropvoidSlotVar_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropvoidSlotVar_noargs_l " <<asAtomHandler::toDebugString(obj)<< " " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,nullptr,0,slot_id,context->exec_pos,false,false,false);
	ASATOM_DECREF(ret);

	++(context->exec_pos);
}

void lightspark::abc_callpropertySlotVar_constant_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom args=*instrptr->arg2_constant;
	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropertySlotVar_cc " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,&args,1,slot_id,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_local_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom args=*instrptr->arg2_constant;

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropertySlotVar_lc " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,&args,1,slot_id,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_constant_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropertySlotVar_cl " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,&args,1,slot_id,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_local_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropertySlotVar_ll " << slot_id<<" "<<instrptr->local_pos1<<" "<<instrptr->local_pos2<<" "<<asAtomHandler::toDebugString(obj)<<" "<<asAtomHandler::toDebugString(args));
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,&args,1,slot_id,context->exec_pos,false,true,coerce);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_constant_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom args=*instrptr->arg2_constant;

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropertySlotVar_ccl " << slot_id);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,res,obj,&args,1,slot_id,context->exec_pos,false,true,coerce);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_local_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom args=*instrptr->arg2_constant;

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropertySlotVar_lcl " << slot_id);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,res,obj,&args,1,slot_id,context->exec_pos,false,true,coerce);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_constant_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropertySlotVar_cll " << slot_id);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,res,obj,&args,1,slot_id,context->exec_pos,false,true,coerce);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_local_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	bool coerce = ((++(context->exec_pos))->local2.flags&ABC_OP_COERCED)==0;
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom args=CONTEXT_GETLOCAL(context,instrptr->local_pos2);

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropertySlotVar_lll " << slot_id);
	asAtom res=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,res,obj,&args,1,slot_id,context->exec_pos,false,true,coerce);
	REPLACELOCALRESULT(context,instrptr->local3.pos,res);
	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	uint32_t slot_id=context->exec_pos->local2.pos;


	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropertySlotVar_noargs_c " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	callprop_intern_slotvar(context,ret,obj,nullptr,0,slot_id,context->exec_pos,false,true,false);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	uint32_t slot_id=context->exec_pos->local2.pos;


	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropertySlotVar_noargs_l " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	if (context->exec_pos->local2.flags&ABC_OP_FROMGLOBAL && context->exec_pos->cachedvar3 && asAtomHandler::is<SyntheticFunction>(context->exec_pos->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(context->exec_pos->cachedvar3->var,obj);
		asAtomHandler::callFunction(context->exec_pos->cachedvar3->var,context->worker,ret,closure,nullptr,0,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern_slotvar(context,ret,obj,nullptr,0,slot_id,context->exec_pos,false,true,false);
	RUNTIME_STACK_PUSH(context,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_constant_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	uint32_t slot_id=context->exec_pos->local2.pos;


	asAtom obj= *instrptr->arg1_constant;
	LOG_CALL( "callPropertySlotVar_noargs_c_lr " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	if (context->exec_pos->local2.flags&ABC_OP_FROMGLOBAL && context->exec_pos->cachedvar3 && asAtomHandler::is<SyntheticFunction>(context->exec_pos->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(context->exec_pos->cachedvar3->var,obj);
		asAtomHandler::callFunction(context->exec_pos->cachedvar3->var,context->worker,ret,closure,nullptr,0,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern_slotvar(context,ret,obj,nullptr,0,slot_id,context->exec_pos,false,true,false);
	REPLACELOCALRESULT(context,instrptr->local3.pos,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVar_local_localresult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	(++(context->exec_pos));
	uint32_t slot_id=context->exec_pos->local2.pos;

	asAtom obj= CONTEXT_GETLOCAL(context,instrptr->local_pos1);
	LOG_CALL( "callPropertySlotVar_noargs_l_lr " << slot_id);
	asAtom ret=asAtomHandler::invalidAtom;
	if (context->exec_pos->local2.flags&ABC_OP_FROMGLOBAL && context->exec_pos->cachedvar3 && asAtomHandler::is<SyntheticFunction>(context->exec_pos->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(context->exec_pos->cachedvar3->var,obj);
		asAtomHandler::callFunction(context->exec_pos->cachedvar3->var,context->worker,ret,closure,nullptr,0,false,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0,(context->exec_pos->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern_slotvar(context,ret,obj,nullptr,0,slot_id,context->exec_pos,false,true,false);
	REPLACELOCALRESULT(context,instrptr->local3.pos,ret);

	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVarCached_constant(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	uint32_t slot_id=instrptr->local3.pos;
	LOG_CALL( "callProperty_SlotVarCached_c " << slot_id<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *(++context->exec_pos)->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	if (instrptr->local2.flags&ABC_OP_FROMGLOBAL && instrptr->cachedvar3 && asAtomHandler::is<SyntheticFunction>(instrptr->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(instrptr->cachedvar3->var,obj);
		asAtomHandler::callFunction(instrptr->cachedvar3->var,context->worker,ret,closure,args,argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern_slotvar(context,ret,obj,args,argcount,slot_id,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVarCached_local(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	uint32_t slot_id=instrptr->local3.pos;
	LOG_CALL( "callProperty_SlotVarCached_l " << slot_id<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = CONTEXT_GETLOCAL(context,(++context->exec_pos)->local_pos2);
	asAtom ret=asAtomHandler::invalidAtom;
	if (instrptr->local2.flags&ABC_OP_FROMGLOBAL && instrptr->cachedvar3 && asAtomHandler::is<SyntheticFunction>(instrptr->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(instrptr->cachedvar3->var,obj);
		asAtomHandler::callFunction(instrptr->cachedvar3->var,context->worker,ret,closure,args,argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern_slotvar(context,ret,obj,args,argcount,slot_id,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	RUNTIME_STACK_PUSH(context,ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVarCached_constant_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	uint32_t slot_id=instrptr->local3.pos;
	LOG_CALL( "callProperty_SlotVarCached_cl " << slot_id<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = *(++context->exec_pos)->arg2_constant;
	asAtom ret=asAtomHandler::invalidAtom;
	if (instrptr->local2.flags&ABC_OP_FROMGLOBAL && instrptr->cachedvar3 && asAtomHandler::is<SyntheticFunction>(instrptr->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(instrptr->cachedvar3->var,obj);
		asAtomHandler::callFunction(instrptr->cachedvar3->var,context->worker,ret,closure,args,argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern_slotvar(context,ret,obj,args,argcount,slot_id,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	REPLACELOCALRESULT(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
void lightspark::abc_callpropertySlotVarCached_local_localResult(call_context* context)
{
	preloadedcodedata* instrptr = context->exec_pos;
	uint32_t argcount = instrptr->local2.pos;
	asAtom* args = g_newa(asAtom,argcount);
	uint32_t slot_id=instrptr->local3.pos;
	LOG_CALL( "callProperty_SlotVarCached_ll " << slot_id<<" "<<argcount);
	for (uint32_t i = argcount; i > 0 ; i--)
	{
		(++(context->exec_pos));
		if (context->exec_pos->arg2_uint == OPERANDTYPES::OP_LOCAL || context->exec_pos->arg2_uint == OPERANDTYPES::OP_CACHED_SLOT)
			args[i-1] = CONTEXT_GETLOCAL(context,context->exec_pos->local_pos1);
		else
			args[i-1] = *context->exec_pos->arg1_constant;
	}
	asAtom obj = CONTEXT_GETLOCAL(context,(++context->exec_pos)->local_pos2);
	asAtom ret=asAtomHandler::invalidAtom;
	if (instrptr->local2.flags&ABC_OP_FROMGLOBAL && instrptr->cachedvar3 && asAtomHandler::is<SyntheticFunction>(instrptr->cachedvar3->var))
	{
		asAtom closure = asAtomHandler::getClosureAtom(instrptr->cachedvar3->var,obj);
		asAtomHandler::callFunction(instrptr->cachedvar3->var,context->worker,ret,closure,args,argcount,false,(instrptr->local2.flags&ABC_OP_COERCED)==0,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	}
	else
		callprop_intern_slotvar(context,ret,obj,args,argcount,slot_id,instrptr,false,true,(instrptr->local2.flags&ABC_OP_COERCED)==0);
	REPLACELOCALRESULT(context,context->exec_pos->local3.pos,ret);
	++(context->exec_pos);
}
