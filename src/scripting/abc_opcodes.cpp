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

#include "scripting/abc.h"
#include <limits>
#include "scripting/class.h"
#include "exceptions.h"
#include "compat.h"
#include "scripting/abcutils.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/Namespace.h"
#include "scripting/toplevel/Null.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/toplevel/Undefined.h"
#include "scripting/flash/utils/Proxy.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "parsing/streams.h"

using namespace std;
using namespace lightspark;

int32_t ABCVm::bitAnd(ASObject* val2, ASObject* val1)
{
	int32_t i1=val1->toInt();
	int32_t i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG_CALL("bitAnd_oo " << hex << i1 << '&' << i2 << dec);
	return i1&i2;
}

int32_t ABCVm::bitAnd_oi(ASObject* val1, int32_t val2)
{
	int32_t i1=val1->toInt();
	int32_t i2=val2;
	val1->decRef();
	LOG_CALL("bitAnd_oi " << hex << i1 << '&' << i2 << dec);
	return i1&i2;
}

void ABCVm::setProperty(ASObject* value,ASObject* obj,multiname* name)
{
	LOG_CALL("setProperty " << *name << ' ' << obj<<" "<<obj->toDebugString()<<" " <<value);

	if(obj->is<Null>())
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << obj->toDebugString()<<" " <<value->toDebugString());
		createError<TypeError>(getWorker(),kConvertNullToObjectError);
		obj->decRef();
		return;
	}
	if (obj->is<Undefined>())
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << obj->toDebugString()<<" " <<value->toDebugString());
		createError<TypeError>(getWorker(),kConvertUndefinedToObjectError);
		obj->decRef();
		return;
	}
	//Do not allow to set contant traits
	asAtom v = asAtomHandler::fromObject(value);
	bool alreadyset=false;
	obj->setVariableByMultiname(*name,v,ASObject::CONST_NOT_ALLOWED,&alreadyset,obj->getInstanceWorker());
	if (alreadyset || (obj->getInstanceWorker()->currentCallContext && obj->getInstanceWorker()->currentCallContext->exceptionthrown))
		value->decRef();
	obj->decRef();
}

void ABCVm::setProperty_i(int32_t value,ASObject* obj,multiname* name)
{
	LOG_CALL("setProperty_i " << *name << ' ' <<obj);
	if(obj->is<Null>())
	{
		LOG(LOG_ERROR,"calling setProperty_i on null:" << *name << ' ' << obj->toDebugString()<<" " << value);
		createError<TypeError>(getWorker(),kConvertNullToObjectError);
	}
	else if (obj->is<Undefined>())
	{
		LOG(LOG_ERROR,"calling setProperty_i on undefined:" << *name << ' ' << obj->toDebugString()<<" " << value);
		createError<TypeError>(getWorker(),kConvertUndefinedToObjectError);
	}
	else
		obj->setVariableByMultiname_i(*name,value,obj->getInstanceWorker());
	obj->decRef();
}

number_t ABCVm::convert_d(ASObject* o)
{
	LOG_CALL( "convert_d" );
	number_t ret=o->toNumber();
	o->decRef();
	return ret;
}

bool ABCVm::convert_b(ASObject* o)
{
	LOG_CALL( "convert_b" );
	bool ret=Boolean_concrete(o);
	o->decRef();
	return ret;
}

uint32_t ABCVm::convert_u(ASObject* o)
{
	LOG_CALL( "convert_u" );
	uint32_t ret=o->toUInt();
	o->decRef();
	return ret;
}

int32_t ABCVm::convert_i(ASObject* o)
{
	LOG_CALL( "convert_i" );
	int32_t ret=o->toInt();
	o->decRef();
	return ret;
}

int64_t ABCVm::convert_di(ASObject* o)
{
	LOG_CALL( "convert_di" );
	int64_t ret=o->toInt64();
	o->decRef();
	return ret;
}

ASObject* ABCVm::convert_s(ASObject* o)
{
	LOG_CALL( "convert_s" );
	ASObject* ret=o;
	if(o->getObjectType()!=T_STRING)
	{
		ret=abstract_s(o->getInstanceWorker(),o->toString());
		o->decRef();
	}
	return ret;
}

void ABCVm::label()
{
	LOG_CALL( "label" );
}

void ABCVm::lookupswitch()
{
	LOG_CALL( "lookupswitch" );
}

ASObject* ABCVm::pushUndefined()
{
	LOG_CALL( "pushUndefined" );
	return getSys()->getUndefinedRef();
}

ASObject* ABCVm::pushNull()
{
	LOG_CALL( "pushNull" );
	return getSys()->getNullRef();
}

void ABCVm::coerce_a()
{
	LOG_CALL( "coerce_a" );
}

ASObject* ABCVm::checkfilter(ASObject* o)
{
	LOG_CALL( "checkfilter" );
	if (o->is<Null>())
		createError<TypeError>(getWorker(),kConvertNullToObjectError);
	else if (o->is<Undefined>())
		createError<TypeError>(getWorker(),kConvertUndefinedToObjectError);
	else if (!o->is<XML>() && !o->is<XMLList>())
		createError<TypeError>(getWorker(),kFilterError, o->getClassName());
	return o;
}

ASObject* ABCVm::coerce_s(ASObject* o)
{
	asAtom v = asAtomHandler::fromObject(o);
	if (Class<ASString>::getClass(o->getSystemState())->coerce(o->getInstanceWorker(),v))
		o->decRef();
	return asAtomHandler::toObject(v,o->getInstanceWorker());
}

void ABCVm::coerce(call_context* th, int n)
{
	multiname* mn = th->mi->context->getMultiname(n,nullptr);
	LOG_CALL("coerce " << *mn);

	RUNTIME_STACK_POINTER_CREATE(th,o);

	Type* type = mn->cachedType != nullptr ? mn->cachedType : Type::getTypeFromMultiname(mn, th->mi->context);
	if (type == nullptr)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		createError<TypeError>(th->worker,kClassNotFoundError,mn->qualifiedString(getSys()));
		return;
	}
	asAtom v= *o;
	if (type->coerce(th->worker,*o))
		ASATOM_DECREF(v);
}

void ABCVm::pop()
{
	LOG_CALL( "pop: DONE" );
}

void ABCVm::getLocal_int(int n, int v)
{
	LOG_CALL("getLocal[" << n << "] (int)= " << dec << v);
}

void ABCVm::getLocal(ASObject* o, int n)
{
	LOG_CALL("getLocal[" << n << "] (" << o << ") " << o->toDebugString());
}

void ABCVm::getLocal_short(int n)
{
	LOG_CALL("getLocal[" << n << "]");
}

void ABCVm::setLocal(int n)
{
	LOG_CALL("setLocal[" << n << "]");
}

void ABCVm::setLocal_int(int n, int v)
{
	LOG_CALL("setLocal[" << n << "] (int)= " << dec << v);
}

void ABCVm::setLocal_obj(int n, ASObject* v)
{
	LOG_CALL("setLocal[" << n << "] = " << v->toDebugString());
}

int32_t ABCVm::pushShort(intptr_t n)
{
	LOG_CALL( "pushShort " << n );
	return n;
}

void ABCVm::setSlot(call_context *th,ASObject* value, ASObject* obj, int n)
{
	LOG_CALL("setSlot " << n << " "<< obj<<" " <<obj->toDebugString() << " "<< value->toDebugString()<<" "<<value);
	obj->setSlot(th->worker,n-1,asAtomHandler::fromObject(value));
	obj->decRef();
}

ASObject* ABCVm::getSlot(call_context *th,ASObject* obj, int n)
{
	asAtom ret=obj->getSlot(n);
	LOG_CALL("getSlot " << n << " " << asAtomHandler::toDebugString(ret));
	//getSlot can only access properties defined in the current
	//script, so they should already be defind by this script
	ASATOM_INCREF(ret);
	obj->decRef();
	return asAtomHandler::toObject(ret,th->worker);
}

number_t ABCVm::negate(ASObject* v)
{
	LOG_CALL( "negate" );
	number_t ret=-(v->toNumber());
	v->decRef();
	return ret;
}

int32_t ABCVm::negate_i(ASObject* o)
{
	LOG_CALL("negate_i");

	int n=o->toInt();
	o->decRef();
	return -n;
}

int32_t ABCVm::bitNot(ASObject* val)
{
	int32_t i1=val->toInt();
	val->decRef();
	LOG_CALL("bitNot " << hex << i1 << dec);
	return ~i1;
}

int32_t ABCVm::bitXor(ASObject* val2, ASObject* val1)
{
	int32_t i1=val1->toInt();
	int32_t i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG_CALL("bitXor " << hex << i1 << '^' << i2 << dec);
	return i1^i2;
}

int32_t ABCVm::bitOr_oi(ASObject* val2, int32_t val1)
{
	int32_t i1=val1;
	int32_t i2=val2->toInt();
	val2->decRef();
	LOG_CALL("bitOr " << hex << i1 << '|' << i2 << dec);
	return i1|i2;
}

int32_t ABCVm::bitOr(ASObject* val2, ASObject* val1)
{
	int32_t i1=val1->toInt();
	int32_t i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG_CALL("bitOr " << hex << i1 << '|' << i2 << dec);
	return i1|i2;
}

void ABCVm::callProperty(call_context* th, int n, int m, method_info** called_mi, bool keepReturn)
{
	callPropIntern(th, n, m, keepReturn, false,nullptr);
}

void ABCVm::callPropLex(call_context *th, int n, int m, method_info **called_mi, bool keepReturn)
{
	callPropIntern(th, n, m, keepReturn, true,nullptr);
}

void ABCVm::callPropIntern(call_context *th, int n, int m, bool keepReturn, bool callproplex,preloadedcodedata* instrptr)
{
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
		RUNTIME_STACK_POP(th,args[m-i-1]);

	asAtom obj=asAtomHandler::invalidAtom;
	if (instrptr && (instrptr->local3.flags&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		RUNTIME_STACK_POP(th,obj);
		if (asAtomHandler::isObject(obj) && asAtomHandler::getObjectNoCheck(obj)->getClass() == instrptr->cacheobj1)
		{
			asAtom o = asAtomHandler::fromObject(instrptr->cacheobj2);
			ASATOM_INCREF(o);
			LOG_CALL( (callproplex ? (keepReturn ? "callPropLex " : "callPropLexVoid") : (keepReturn ? "callProperty " : "callPropVoid")) << " from cache:"<<*th->mi->context->getMultiname(n,th)<<" "<<asAtomHandler::toDebugString(obj)<<" "<<asAtomHandler::toDebugString(o));
			callImpl(th, o, obj, args, m, keepReturn);
			LOG_CALL("End of calling cached property "<<*th->mi->context->getMultiname(n,th));
			return;
		}
		else
		{
			if (instrptr->cacheobj2 && instrptr->cacheobj2->is<Function>() && instrptr->cacheobj2->as<IFunction>()->clonedFrom)
				instrptr->cacheobj2->decRef();
			instrptr->local3.flags |= ABC_OP_NOTCACHEABLE;
			instrptr->local3.flags &= ~ABC_OP_CACHED;
		}
	}
	
	multiname* name=th->mi->context->getMultiname(n,th);
	
	LOG_CALL( (callproplex ? (keepReturn ? "callPropLex " : "callPropLexVoid") : (keepReturn ? "callProperty " : "callPropVoid")) << *name << ' ' << m);

	if (asAtomHandler::isInvalid(obj))
	{
		RUNTIME_STACK_POP(th,obj);
	}

	if(asAtomHandler::is<Null>(obj))
	{
		LOG(LOG_ERROR,"trying to call property on null:"<<*name);
		createError<TypeError>(th->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		LOG(LOG_ERROR,"trying to call property on undefined:"<<*name);
		createError<TypeError>(th->worker,kConvertUndefinedToObjectError);
		return;
	}
	bool canCache = false;
	ASObject* pobj = asAtomHandler::getObject(obj);
	asAtom o=asAtomHandler::invalidAtom;
	if (!pobj)
	{
		// fast path for primitives to avoid creation of ASObjects
		asAtomHandler::getVariableByMultiname(obj,o,th->sys,*name,th->worker);
		canCache = asAtomHandler::isValid(o);
	}
	if(asAtomHandler::isInvalid(o))
	{
		pobj = asAtomHandler::toObject(obj,th->worker);
		//We should skip the special implementation of get
		canCache = pobj->getVariableByMultiname(o,*name, SKIP_IMPL,th->worker) & GET_VARIABLE_RESULT::GETVAR_CACHEABLE;
	}
	name->resetNameIfObject();
	if(asAtomHandler::isInvalid(o) && asAtomHandler::is<Class_base>(obj))
	{
		// check super classes
		_NR<Class_base> tmpcls = asAtomHandler::as<Class_base>(obj)->super;
		while (tmpcls && !tmpcls.isNull())
		{
			tmpcls->getVariableByMultiname(o,*name, GET_VARIABLE_OPTION::SKIP_IMPL,th->worker);
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
		if (canCache
				&& instrptr
				&& name->isStatic
				&& (instrptr->local3.flags & ABC_OP_NOTCACHEABLE)==0
				&& asAtomHandler::canCacheMethod(obj,name)
				&& asAtomHandler::getObject(o)
				&& !asAtomHandler::as<IFunction>(o)->clonedFrom
				&& ((asAtomHandler::is<Class_base>(obj) && asAtomHandler::as<IFunction>(o)->inClass == asAtomHandler::as<Class_base>(obj))
					|| (asAtomHandler::as<IFunction>(o)->inClass && asAtomHandler::getClass(obj,th->sys)->isSubClass(asAtomHandler::as<IFunction>(o)->inClass))))
		{
			// cache method if multiname is static and it is a method of a sealed class
			instrptr->local3.flags |= ABC_OP_CACHED;
			instrptr->cacheobj1 = asAtomHandler::getClass(obj,th->sys);
			instrptr->cacheobj2 = asAtomHandler::getObjectNoCheck(o);
			LOG_CALL("caching callproperty:"<<*name<<" "<<instrptr->cacheobj1->toDebugString()<<" "<<instrptr->cacheobj2->toDebugString());
		}
//		else
//			LOG(LOG_ERROR,"callprop caching failed:"<<canCache<<" "<<*name<<" "<<name->isStatic<<" "<<asAtomHandler::toDebugString(obj));
		asAtom obj2 = obj;
		if (callproplex && asAtomHandler::is<IFunction>(o) && !asAtomHandler::as<IFunction>(o)->inClass)
		{
			// according to spec, callproplex should use null as the "this" if the function is not a method, but
			// using null or undefined as "this" indicates use of the global object
			// see avm2overview chapter 2.4
			obj2 = asAtomHandler::fromObject(getGlobalScope(th));
		}
		else
			obj2 = asAtomHandler::getClosureAtom(o,obj);
		if (obj.uintval != obj2.uintval)
		{
			ASATOM_INCREF(obj2);
			ASATOM_DECREF(obj);
		}
		callImpl(th, o, obj2, args, m, keepReturn);
	}
	else
	{
		//If the object is a Proxy subclass, try to invoke callProperty
		if(asAtomHandler::is<Proxy>(obj))
		{
			//Check if there is a custom caller defined, skipping implementation to avoid recursive calls
			multiname callPropertyName(nullptr);
			callPropertyName.name_type=multiname::NAME_STRING;
			callPropertyName.name_s_id=th->sys->getUniqueStringId("callProperty");
			callPropertyName.ns.emplace_back(th->sys,flash_proxy,NAMESPACE);
			asAtom oproxy=asAtomHandler::invalidAtom;
			pobj->getVariableByMultiname(oproxy,callPropertyName,GET_VARIABLE_OPTION::SKIP_IMPL,th->worker);
			if(asAtomHandler::isValid(oproxy))
			{
				assert_and_throw(asAtomHandler::isFunction(oproxy));
				if(asAtomHandler::isValid(o))
				{
					callImpl(th, o, obj, args, m,keepReturn);
				}
				else
				{
					//Create a new array
					asAtom* proxyArgs=g_newa(asAtom, m+1);
					ASObject* namearg = abstract_s(th->worker,name->normalizedName(th->sys));
					namearg->setProxyProperty(*name);
					proxyArgs[0]=asAtomHandler::fromObject(namearg);
					for(int i=0;i<m;i++)
						proxyArgs[i+1]=args[i];
					
					//We now suppress special handling
					LOG_CALL("Proxy::callProperty");
					ASATOM_INCREF(oproxy);
					asAtom ret=asAtomHandler::invalidAtom;
					asAtomHandler::callFunction(oproxy,th->worker,ret,obj,proxyArgs,m+1,true);
					ASATOM_DECREF(oproxy);
					if(keepReturn)
						RUNTIME_STACK_PUSH(th,ret);
					else
					{
						ASATOM_DECREF(ret);
					}
				}
				LOG_CALL("End of calling proxy custom caller " << *name);
				ASATOM_DECREF(oproxy);
				return;
			}
			else if(asAtomHandler::isValid(o))
			{
				callImpl(th, o, obj, args, m, keepReturn);
				LOG_CALL("End of calling proxy " << *name);
				return;
			}
		}
		for(int i=0;i<m;i++)
			ASATOM_DECREF(args[i]);
		//LOG(LOG_NOT_IMPLEMENTED,"callProperty: " << name->qualifiedString(th->sys) << " not found on " << obj->toDebugString());
		if (pobj->hasPropertyByMultiname(*name,true,true,th->worker))
		{
			tiny_string clsname = pobj->getClass()->getQualifiedClassName();
			createError<ReferenceError>(th->worker,kWriteOnlyError, name->normalizedName(th->sys), clsname);
		}
		if (pobj->getClass() && pobj->getClass()->isSealed)
		{
			tiny_string clsname = pobj->getClass()->getQualifiedClassName();
			createError<ReferenceError>(th->worker,kReadSealedError, name->normalizedName(th->sys), clsname);
		}
		if (asAtomHandler::is<Class_base>(obj))
		{
			tiny_string clsname = asAtomHandler::as<Class_base>(obj)->class_name.getQualifiedName(th->sys);
			createError<TypeError>(th->worker,kCallNotFoundError, name->qualifiedString(th->sys), clsname);
		}
		else
		{
			tiny_string clsname = pobj->getClassName();
			createError<TypeError>(th->worker,kCallNotFoundError, name->qualifiedString(th->sys), clsname);
		}

		ASATOM_DECREF(obj);
		if(keepReturn)
		{
			RUNTIME_STACK_PUSH(th,asAtomHandler::undefinedAtom);
		}

	}
	LOG_CALL("End of calling " << *name);
}

void ABCVm::callMethod(call_context* th, int n, int m)
{
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	LOG_CALL( "callMethod " << n << ' ' << m);

	asAtom obj=asAtomHandler::invalidAtom;
	RUNTIME_STACK_POP(th,obj);

	if(asAtomHandler::is<Null>(obj))
	{
		LOG(LOG_ERROR,"trying to call method on null:"<<n);
		createError<TypeError>(th->worker,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::is<Undefined>(obj))
	{
		LOG(LOG_ERROR,"trying to call method on undefined:"<<n);
		createError<TypeError>(th->worker,kConvertUndefinedToObjectError);
		return;
	}

	asAtom o=asAtomHandler::getObject(obj)->getSlot(n);
	if(asAtomHandler::isValid(o))
	{
		ASATOM_INCREF(o);
		callImpl(th, o, obj, args, m, true);
	}
	else
	{
		tiny_string clsname = asAtomHandler::getObject(obj)->getClassName();
		for(int i=0;i<m;i++)
		{
			ASATOM_DECREF(args[i]);
		}
		ASATOM_DECREF(obj);
		createError<TypeError>(th->worker,kCallNotFoundError, "", clsname);
	}
	LOG_CALL("End of calling method " << n);
}

int32_t ABCVm::getProperty_i(ASObject* obj, multiname* name)
{
	LOG_CALL( "getProperty_i " << *name );

	//TODO: implement exception handling to find out if no integer can be returned
	int32_t ret=obj->getVariableByMultiname_i(*name,nullptr);

	obj->decRef();
	return ret;
}

ASObject* ABCVm::getProperty(ASObject* obj, multiname* name)
{
	LOG_CALL( "getProperty " << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());

	asAtom prop=asAtomHandler::invalidAtom;
	obj->getVariableByMultiname(prop,*name,GET_VARIABLE_OPTION::NONE,obj->getInstanceWorker());
	ASObject *ret;

	if(asAtomHandler::isInvalid(prop))
	{
		if (obj->getClass() && obj->getClass()->isSealed)
			createError<ReferenceError>(obj->getInstanceWorker(),kReadSealedError, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClass()->getQualifiedClassName());
		else if (name->isEmpty())
			createError<ReferenceError>(obj->getInstanceWorker(),kReadSealedErrorNs, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClassName());
		else if (obj->is<Undefined>())
			createError<TypeError>(obj->getInstanceWorker(),kConvertUndefinedToObjectError);
		else if (Log::getLevel() >= LOG_NOT_IMPLEMENTED && (!obj->getClass() || obj->getClass()->isSealed))
			LOG(LOG_NOT_IMPLEMENTED,"getProperty: " << name->normalizedNameUnresolved(obj->getSystemState()) << " not found on " << obj->toDebugString() << " "<<obj->getClassName());
		ret = obj->getSystemState()->getUndefinedRef();
	}
	else
	{
		ret=asAtomHandler::toObject(prop,obj->getInstanceWorker());
		ret->incRef();
	}
	obj->decRef();
	return ret;
}

number_t ABCVm::divide(ASObject* val2, ASObject* val1)
{
	double num1=val1->toNumber();
	double num2=val2->toNumber();

	val1->decRef();
	val2->decRef();
	LOG_CALL("divide "  << num1 << '/' << num2);
	return num1/num2;
}

void ABCVm::pushWith(call_context* th)
{
	RUNTIME_STACK_POP_CREATE(th,t);
	LOG_CALL( "pushWith " << asAtomHandler::toDebugString(*t) );
	assert_and_throw(th->curr_scope_stack < th->mi->body->max_scope_depth);
	if (asAtomHandler::isObject(*t))
		asAtomHandler::getObjectNoCheck(*t)->addStoredMember();
	th->scope_stack[th->curr_scope_stack] = *t;
	th->scope_stack_dynamic[th->curr_scope_stack] = true;
	th->curr_scope_stack++;
}

void ABCVm::pushScope(call_context* th)
{
	RUNTIME_STACK_POP_CREATE(th,t);
	LOG_CALL( "pushScope " << asAtomHandler::toDebugString(*t) );
	assert_and_throw(th->curr_scope_stack < th->mi->body->max_scope_depth);
	if (asAtomHandler::isObject(*t))
		asAtomHandler::getObjectNoCheck(*t)->addStoredMember();
	th->scope_stack[th->curr_scope_stack] = *t;
	th->scope_stack_dynamic[th->curr_scope_stack] = false;
	th->curr_scope_stack++;
}

Global* ABCVm::getGlobalScope(call_context* th)
{
	ASObject* ret;
	if (th->parent_scope_stack && th->parent_scope_stack->scope.size() > 0)
		ret =asAtomHandler::toObject(th->parent_scope_stack->scope.begin()->object,th->worker);
	else
	{
		assert_and_throw(th->curr_scope_stack > 0);
		ret =asAtomHandler::getObject(th->scope_stack[0]);
	}
	assert_and_throw(ret->is<Global>());
	LOG_CALL("getGlobalScope: " << ret->toDebugString());
	return ret->as<Global>();
}

number_t ABCVm::decrement(ASObject* o)
{
	LOG_CALL("decrement");

	number_t n=o->toNumber();
	o->decRef();
	return n-1;
}

uint32_t ABCVm::decrement_i(ASObject* o)
{
	LOG_CALL("decrement_i");

	int32_t n=o->toInt();
	o->decRef();
	return n-1;
}

uint64_t ABCVm::decrement_di(ASObject* o)
{
	LOG_CALL("decrement_di");

	int64_t n=o->toInt64();
	o->decRef();
	return n-1;
}

bool ABCVm::ifNLT(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=!(obj1->isLess(obj2)==TTRUE);
	LOG_CALL("ifNLT (" << ((ret)?"taken)":"not taken)"));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLT(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TTRUE);
	LOG_CALL("ifLT (" << ((ret)?"taken)":"not taken)"));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLT_oi(ASObject* obj2, int32_t val1)
{
	LOG_CALL("ifLT_oi");

	//As ECMA said, on NaN return undefined... and undefined means not jump
	bool ret;
	if(obj2->getObjectType()==T_UNDEFINED)
		ret=false;
	else
		ret=val1<obj2->toInt();

	obj2->decRef();
	return ret;
}

bool ABCVm::ifLT_io(int32_t val2, ASObject* obj1)
{
	LOG_CALL("ifLT_io ");

	bool ret=obj1->toInt()<val2;

	obj1->decRef();
	return ret;
}

bool ABCVm::ifNE(ASObject* obj1, ASObject* obj2)
{
	//Real comparision demanded to object
	bool ret=!(obj1->isEqual(obj2));
	LOG_CALL("ifNE (" << ((ret)?"taken)":"not taken)"));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifNE_oi(ASObject* obj1, int32_t val2)
{
	//HACK
	if(obj1->getObjectType()==T_UNDEFINED)
		return false;
	bool ret=obj1->toInt()!=val2;
	LOG_CALL("ifNE (" << ((ret)?"taken)":"not taken)"));

	obj1->decRef();
	return ret;
}

int32_t ABCVm::pushByte(intptr_t n)
{
	LOG_CALL( "pushByte " << n );
	return n;
}

number_t ABCVm::multiply_oi(ASObject* val2, int32_t val1)
{
	double num1=val1;
	double num2=val2->toNumber();
	val2->decRef();
	LOG_CALL("multiply_oi "  << num1 << '*' << num2);
	return num1*num2;
}

number_t ABCVm::multiply(ASObject* val2, ASObject* val1)
{
	double num1=val1->toNumber();
	double num2=val2->toNumber();
	val1->decRef();
	val2->decRef();
	LOG_CALL("multiply_i "  << num1 << '*' << num2);
	return num1*num2;
}

int32_t ABCVm::multiply_i(ASObject* val2, ASObject* val1)
{
	int num1=val1->toInt();
	int num2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG_CALL("multiply "  << num1 << '*' << num2);
	return num1*num2;
}

void ABCVm::incLocal(call_context* th, int n)
{
	LOG_CALL( "incLocal " << n );
	number_t tmp=asAtomHandler::toNumber(th->locals[n]);
	ASATOM_DECREF(th->locals[n]);
	asAtomHandler::setNumber(th->locals[n],th->worker,tmp+1);
}

void ABCVm::incLocal_i(call_context* th, int n)
{
	LOG_CALL( "incLocal_i " << n );
	int32_t tmp=asAtomHandler::toInt(th->locals[n]);
	ASATOM_DECREF(th->locals[n]);
	asAtomHandler::setInt(th->locals[n],th->worker,tmp+1);
}

void ABCVm::decLocal(call_context* th, int n)
{
	LOG_CALL( "decLocal " << n );
	number_t tmp=asAtomHandler::toNumber(th->locals[n]);
	ASATOM_DECREF(th->locals[n]);
	asAtomHandler::setNumber(th->locals[n],th->worker,tmp-1);
}

void ABCVm::decLocal_i(call_context* th, int n)
{
	LOG_CALL( "decLocal_i " << n );
	int32_t tmp=asAtomHandler::toInt(th->locals[n]);
	ASATOM_DECREF(th->locals[n]);
	asAtomHandler::setInt(th->locals[n],th->worker,tmp-1);
}

/* This is called for expressions like
 * function f() { ... }
 * var v = new f();
 */
void ABCVm::constructFunction(asAtom &ret, call_context* th, asAtom &f, asAtom *args, int argslen)
{
	//See ECMA 13.2.2
	if(asAtomHandler::as<IFunction>(f)->inClass)
	{
		createError<TypeError>(th->worker,kCannotCallMethodAsConstructor, "");
		return;
	}

	assert(asAtomHandler::as<IFunction>(f)->prototype);
	ret=asAtomHandler::fromObject(new_functionObject(asAtomHandler::as<IFunction>(f)->prototype));
#ifndef NDEBUG
	asAtomHandler::getObject(ret)->initialized=false;
#endif
	if (asAtomHandler::is<SyntheticFunction>(f))
	{
		SyntheticFunction* sf=asAtomHandler::as<SyntheticFunction>(f);
		if (sf->mi->body && !sf->mi->needsActivation())
		{
			LOG_CALL("Building method traits " <<sf->mi->body->trait_count);
			std::vector<multiname*> additionalslots;
			for(unsigned int i=0;i<sf->mi->body->trait_count;i++)
				th->mi->context->buildTrait(asAtomHandler::getObject(ret),additionalslots,&sf->mi->body->traits[i],false);
			asAtomHandler::getObject(ret)->initAdditionalSlots(additionalslots);
		}
	}
#ifndef NDEBUG
	asAtomHandler::getObject(ret)->initialized=true;
#endif

	ASObject* constructor = new_functionObject(asAtomHandler::as<IFunction>(f)->prototype);
	if (asAtomHandler::as<IFunction>(f)->prototype->subtype != SUBTYPE_FUNCTIONOBJECT)
	{
		asAtomHandler::as<IFunction>(f)->prototype->incRef();
		constructor->setVariableByQName("prototype","",asAtomHandler::as<IFunction>(f)->prototype.getPtr(),DECLARED_TRAIT);
	}
	else
	{
		Class<ASObject>::getRef(asAtomHandler::as<IFunction>(f)->getSystemState())->getPrototype(th->worker)->incRef();
		constructor->setVariableByQName("prototype","",Class<ASObject>::getRef(asAtomHandler::as<IFunction>(f)->getSystemState())->getPrototype(th->worker)->getObj(),DECLARED_TRAIT);
	}
	
	asAtomHandler::getObject(ret)->setVariableByQName("constructor","",constructor,DECLARED_TRAIT);

	ASATOM_INCREF(ret);
	ASATOM_INCREF(f);
	asAtom ret2=asAtomHandler::invalidAtom;
	asAtomHandler::callFunction(f,th->worker,ret2,ret,args,argslen,true);
	ASATOM_DECREF(f);

	//ECMA: "return ret2 if it is an object, else ret"
	if(!asAtomHandler::isPrimitive(ret2))
	{
		ASATOM_DECREF(ret);
		ret = ret2;
	}
	else
	{
		ASATOM_DECREF(ret2);
	}
}

void ABCVm::construct(call_context* th, int m)
{
	LOG_CALL( "construct " << m);
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	asAtom obj=asAtomHandler::invalidAtom;
	RUNTIME_STACK_POP(th,obj);

	LOG_CALL("Constructing");

	asAtom ret=asAtomHandler::invalidAtom;
	switch(asAtomHandler::getObjectType(obj))
	{
		case T_CLASS:
		{
			Class_base* o_class=asAtomHandler::as<Class_base>(obj);
			o_class->getInstance(th->worker,ret,true,args,m);
			break;
		}
/*
		case T_OBJECT:
		{
			Class_base* o_class=static_cast<Class_base*>(obj->getClass());
			assert(o_class);
			ret=o_class->getInstance(true,args,m);
			break;
		}
*/
		case T_FUNCTION:
		{
			constructFunction(ret,th, obj, args, m);
			break;
		}

		default:
		{
			createError<TypeError>(th->worker,kConstructOfNonFunctionError);
			ASATOM_DECREF(obj);
			return;
		}
	}
	if (asAtomHandler::getObject(ret))
		asAtomHandler::getObject(ret)->setConstructorCallComplete();
	ASATOM_DECREF(obj);
	LOG_CALL("End of constructing " << asAtomHandler::toDebugString(ret));
	RUNTIME_STACK_PUSH(th,ret);
}
asAtom ABCVm::constructGenericType_intern(ABCContext* context, ASObject* obj, int m, ASObject** args)
{
	if(obj->getObjectType() != T_TEMPLATE)
	{
		LOG(LOG_NOT_IMPLEMENTED, "constructGenericType of " << obj->getObjectType());
		obj->decRef();
		for(int i=0;i<m;i++)
			args[i]->decRef();
		return asAtomHandler::undefinedAtom;
	}

	Template_base* o_template=static_cast<Template_base*>(obj);
	/* Instantiate the template to obtain a class */

	std::vector<Type*> t(m);
	for(int i=0;i<m;++i)
	{
		if(args[i]->is<Class_base>())
			t[i] = args[i]->as<Class_base>();
		else if(args[i]->is<Null>())
			t[i] = Type::anyType;
		else
		{
			obj->decRef();
			for(int i=0;i<m;i++)
				args[i]->decRef();
			return asAtomHandler::invalidAtom;
		}
	}
	Class_base* o_class = o_template->applyType(t,context->root->applicationDomain);
	return asAtomHandler::fromObjectNoPrimitive(o_class);

}
void ABCVm::constructGenericType(call_context* th, int m)
{

	th->explicitConstruction = true;
	LOG_CALL( "constructGenericType " << m);
	if (m != 1)
	{
		th->explicitConstruction = false;
		createError<TypeError>(th->worker,kWrongTypeArgCountError, "function", "1", Integer::toString(m));
		return;
	}
	ASObject** args=g_newa(ASObject*, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP_ASOBJECT(th,args[m-i-1]);
	}

	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,obj);

	asAtom res = constructGenericType_intern(th->mi->context, obj, m, args);
	RUNTIME_STACK_PUSH(th,res);
	if (asAtomHandler::isInvalid(res))
	{
		th->explicitConstruction = false;
		createError<TypeError>(th->worker,0,"Wrong type in applytype");
		return;
	}
	if (asAtomHandler::isUndefined(res))
	{
		th->explicitConstruction = false;
		return;
	}

	// Register the type name in the global scope. The type name
	// is later used by the coerce opcode.
	ASObject* global;
	if (th->parent_scope_stack && th->parent_scope_stack->scope.size() > 0)
		global =asAtomHandler::toObject(th->parent_scope_stack->scope.begin()->object,th->worker);
	else
	{
		assert_and_throw(th->curr_scope_stack > 0);
		global =asAtomHandler::getObject(th->scope_stack[0]);
	}
	QName qname = asAtomHandler::as<Class_base>(res)->class_name;
	if (!global->hasPropertyByMultiname(qname, false, false,th->worker))
		global->setVariableAtomByQName(global->getSystemState()->getStringFromUniqueId(qname.nameId),nsNameAndKind(global->getSystemState(),qname.nsStringId,NAMESPACE),res,DECLARED_TRAIT);
	th->explicitConstruction = false;
}

ASObject* ABCVm::typeOf(ASObject* obj)
{
	LOG_CALL("typeOf");
	string ret;
	switch(obj->getObjectType())
	{
		case T_UNDEFINED:
			ret="undefined";
			break;
		case T_OBJECT:
			if(obj->is<XML>() || obj->is<XMLList>())
			{
				ret = "xml";
				break;
			}
			//fallthrough
		case T_NULL:
		case T_ARRAY:
		case T_CLASS: //this is not clear from spec, but was tested
		case T_QNAME:
		case T_NAMESPACE:
			ret="object";
			break;
		case T_BOOLEAN:
			ret="boolean";
			break;
		case T_NUMBER:
		case T_INTEGER:
		case T_UINTEGER:
			ret="number";
			break;
		case T_STRING:
			ret="string";
			break;
		case T_FUNCTION:
			ret="function";
			break;
		default:
			assert_and_throw(false);
	}
	ASObject* res = abstract_s(obj->getInstanceWorker(),ret);
	obj->decRef();
	return res;
}

void ABCVm::jump(int offset)
{
	LOG_CALL("jump " << offset);
}

bool ABCVm::ifTrue(ASObject* obj1)
{
	bool ret=Boolean_concrete(obj1);
	LOG_CALL("ifTrue (" << ((ret)?"taken)":"not taken)"));

	obj1->decRef();
	return ret;
}

number_t ABCVm::modulo(ASObject* val1, ASObject* val2)
{
	number_t num1=val1->toNumber();
	number_t num2=val2->toNumber();

	val1->decRef();
	val2->decRef();
	LOG_CALL("modulo "  << num1 << '%' << num2);
	/* fmod returns NaN if num2 == 0 as the spec mandates */
	return ::fmod(num1,num2);
}

number_t ABCVm::subtract_oi(ASObject* val2, int32_t val1)
{
	int num2=val2->toInt();
	int num1=val1;

	val2->decRef();
	LOG_CALL("subtract_oi " << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_do(number_t val2, ASObject* val1)
{
	number_t num2=val2;
	number_t num1=val1->toNumber();

	val1->decRef();
	LOG_CALL("subtract_do " << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_io(int32_t val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(LOG_NOT_IMPLEMENTED,"subtract: HACK");
		return 0;
	}
	int num2=val2;
	int num1=val1->toInt();

	val1->decRef();
	LOG_CALL("subtract_io " << dec << num1 << '-' << num2);
	return num1-num2;
}

int32_t ABCVm::subtract_i(ASObject* val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED ||
		val2->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(LOG_NOT_IMPLEMENTED,"subtract_i: HACK");
		return 0;
	}
	int num2=val2->toInt();
	int num1=val1->toInt();

	val1->decRef();
	val2->decRef();
	LOG_CALL("subtract_i " << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract(ASObject* val2, ASObject* val1)
{
	number_t num2=val2->toNumber();
	number_t num1=val1->toNumber();

	val1->decRef();
	val2->decRef();
	LOG_CALL("subtract " << num1 << '-' << num2);
	return num1-num2;
}

void ABCVm::pushUInt(call_context* th, uint32_t i)
{
	LOG_CALL( "pushUInt " << i);
}

void ABCVm::pushInt(call_context* th, int32_t i)
{
	LOG_CALL( "pushInt " << i);
}

void ABCVm::pushDouble(call_context* th, double d)
{
	LOG_CALL( "pushDouble " << d);
}

void ABCVm::kill(int n)
{
	LOG_CALL( "kill " << n );
}

ASObject* ABCVm::add(ASObject* val2, ASObject* val1)
{
	//Implement ECMA add algorithm, for XML and default (see avm2overview)
	
	ASObject* res = nullptr;
	// if both values are Integers or int Numbers the result is also an int Number
	if( (val1->is<Integer>() || val1->is<UInteger>() || (val1->is<Number>() && !val1->as<Number>()->isfloat)) &&
		(val2->is<Integer>() || val2->is<UInteger>() || (val2->is<Number>() && !val2->as<Number>()->isfloat)))
	{
		int64_t num1=val1->toInt64();
		int64_t num2=val2->toInt64();
		LOG_CALL("addI " << num1 << '+' << num2);
		res = abstract_di(val1->getInstanceWorker(), num1+num2);
		val1->decRef();
		val2->decRef();
		return res;
	}
	else if(val1->is<Number>() && val2->is<Number>())
	{
		double num1=val1->as<Number>()->toNumber();
		double num2=val2->as<Number>()->toNumber();
		LOG_CALL("addN " << num1 << '+' << num2);
		res = abstract_d(val1->getInstanceWorker(), num1+num2);
		val1->decRef();
		val2->decRef();
		return res;
	}
	else if(val1->is<ASString>() || val2->is<ASString>())
	{
		tiny_string a = val1->toString();
		tiny_string b = val2->toString();
		LOG_CALL("add " << a << '+' << b);
		res = abstract_s(val1->getInstanceWorker(),a + b);
		val1->decRef();
		val2->decRef();
		return res;
	}
	else if( (val1->is<XML>() || val1->is<XMLList>()) && (val2->is<XML>() || val2->is<XMLList>()) )
	{
		//Check if the objects are both XML or XMLLists
		Class_base* xmlClass=Class<XML>::getClass(val1->getSystemState());

		XMLList* newList=Class<XMLList>::getInstanceS(val1->getInstanceWorker(),true);
		if(val1->getClass()==xmlClass)
			newList->append(_MNR(static_cast<XML*>(val1)));
		else //if(val1->getClass()==xmlListClass)
			newList->append(_MNR(static_cast<XMLList*>(val1)));

		if(val2->getClass()==xmlClass)
			newList->append(_MNR(static_cast<XML*>(val2)));
		else //if(val2->getClass()==xmlListClass)
			newList->append(_MNR(static_cast<XMLList*>(val2)));

		//The references of val1 and val2 have been passed to the smart references
		//no decRef is needed
		return newList;
	}
	else
	{//If none of the above apply, convert both to primitives with no hint
		asAtom val1p=asAtomHandler::invalidAtom;
		bool isrefcounted1;
		val1->toPrimitive(val1p,isrefcounted1);
		asAtom val2p=asAtomHandler::invalidAtom;
		bool isrefcounted2;
		val2->toPrimitive(val2p,isrefcounted2);
		if(asAtomHandler::is<ASString>(val1p) || asAtomHandler::is<ASString>(val2p))
		{//If one is String, convert both to strings and concat
			string a(asAtomHandler::toString(val1p,val1->getInstanceWorker()).raw_buf());
			string b(asAtomHandler::toString(val2p,val1->getInstanceWorker()).raw_buf());
			if (isrefcounted1)
				ASATOM_DECREF(val1p);
			if (isrefcounted2)
				ASATOM_DECREF(val2p);
			LOG_CALL("add " << a << '+' << b);
			res = abstract_s(val1->getInstanceWorker(),a+b);
			val1->decRef();
			val2->decRef();
			return res;
		}
		else
		{//Convert both to numbers and add
			number_t num1=asAtomHandler::toNumber(val1p);
			number_t num2=asAtomHandler::toNumber(val2p);
			if (isrefcounted1)
				ASATOM_DECREF(val1p);
			if (isrefcounted2)
				ASATOM_DECREF(val2p);
			LOG_CALL("addN " << num1 << '+' << num2);
			number_t result = num1 + num2;
			res = abstract_d(val1->getInstanceWorker(),result);
			val1->decRef();
			val2->decRef();
			return res;
		}
	}

}

int32_t ABCVm::add_i(ASObject* val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED ||
		val2->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(LOG_NOT_IMPLEMENTED,"add_i: HACK");
		return 0;
	}
	int32_t num2=val2->toInt();
	int32_t num1=val1->toInt();

	val1->decRef();
	val2->decRef();
	LOG_CALL("add_i " << num1 << '+' << num2);
	return num1+num2;
}

ASObject* ABCVm::add_oi(ASObject* val2, int32_t val1)
{
	ASObject* res =nullptr;
	//Implement ECMA add algorithm, for XML and default
	if(val2->getObjectType()==T_INTEGER)
	{
		Integer* ip=static_cast<Integer*>(val2);
		int32_t num2=ip->val;
		int32_t num1=val1;
		res = abstract_i(val2->getInstanceWorker(),num1+num2);
		val2->decRef();
		LOG_CALL("add " << num1 << '+' << num2);
		return res;
	}
	else if(val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		res = abstract_d(val2->getInstanceWorker(),num1+num2);
		val2->decRef();
		LOG_CALL("add " << num1 << '+' << num2);
		return res;
	}
	else if(val2->getObjectType()==T_STRING)
	{
		//Convert argument to int32_t
		tiny_string a = Integer::toString(val1);
		const tiny_string& b=val2->toString();
		res = abstract_s(val2->getInstanceWorker(), a+b);
		val2->decRef();
		LOG_CALL("add " << a << '+' << b);
		return res;
	}
	else
	{
		return add(val2,abstract_i(val2->getInstanceWorker(),val1));
	}

}

ASObject* ABCVm::add_od(ASObject* val2, number_t val1)
{
	ASObject* res = nullptr;
	//Implement ECMA add algorithm, for XML and default
	if(val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		res = abstract_d(val2->getInstanceWorker(),num1+num2);
		val2->decRef();
		LOG_CALL("add " << num1 << '+' << num2);
		return res;
	}
	else if(val2->getObjectType()==T_INTEGER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		res = abstract_d(val2->getInstanceWorker(),num1+num2);
		val2->decRef();
		LOG_CALL("add " << num1 << '+' << num2);
		return res;
	}
	else if(val2->getObjectType()==T_STRING)
	{
		tiny_string a = Number::toString(val1);
		const tiny_string& b=val2->toString();
		res = abstract_s(val2->getInstanceWorker(),a+b);
		val2->decRef();
		LOG_CALL("add " << a << '+' << b);
		return res;
	}
	else
	{
		return add(val2,abstract_d(val2->getInstanceWorker(),val1));
	}

}

int32_t ABCVm::lShift(ASObject* val1, ASObject* val2)
{
	int32_t i2=val2->toInt();
	uint32_t i1=val1->toUInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG_CALL("lShift "<<hex<<i2<<"<<"<<i1<<dec);
	//Left shift are supposed to always work in 32bit
	int32_t ret=i2<<i1;
	return ret;
}

int32_t ABCVm::lShift_io(uint32_t val1, ASObject* val2)
{
	int32_t i2=val2->toInt();
	uint32_t i1=val1&0x1f;
	val2->decRef();
	LOG_CALL("lShift "<<hex<<i2<<"<<"<<i1<<dec);
	//Left shift are supposed to always work in 32bit
	int32_t ret=i2<<i1;
	return ret;
}

int32_t ABCVm::rShift(ASObject* val1, ASObject* val2)
{
	int32_t i2=val2->toInt();
	uint32_t i1=val1->toUInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG_CALL("rShift "<<hex<<i2<<">>"<<i1<<dec);
	return i2>>i1;
}

uint32_t ABCVm::urShift(ASObject* val1, ASObject* val2)
{
	uint32_t i2=val2->toUInt();
	uint32_t i1=val1->toUInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG_CALL("urShift "<<hex<<i2<<">>"<<i1<<dec);
	return i2>>i1;
}

uint32_t ABCVm::urShift_io(uint32_t val1, ASObject* val2)
{
	uint32_t i2=val2->toUInt();
	uint32_t i1=val1&0x1f;
	val2->decRef();
	LOG_CALL("urShift "<<hex<<i2<<">>"<<i1<<dec);
	return i2>>i1;
}

bool ABCVm::_not(ASObject* v)
{
	LOG_CALL( "not" );
	bool ret=!Boolean_concrete(v);
	v->decRef();
	return ret;
}

bool ABCVm::equals(ASObject* val2, ASObject* val1)
{
	bool ret=val1->isEqual(val2);
	LOG_CALL( "equals " << ret);
	val1->decRef();
	val2->decRef();
	return ret;
}

bool ABCVm::strictEqualImpl(ASObject* obj1, ASObject* obj2)
{
	SWFOBJECT_TYPE type1=obj1->getObjectType();
	SWFOBJECT_TYPE type2=obj2->getObjectType();
	if(type1!=type2)
	{
		//Type conversions are ok only for numeric types
		switch(type1)
		{
			case T_NUMBER:
			case T_INTEGER:
			case T_UINTEGER:
				break;
			case T_NULL:
				if (!obj2->isConstructed() && !obj2->is<Class_base>())
					return true;
				return false;
			default:
				return false;
		}
		switch(type2)
		{
			case T_NUMBER:
			case T_INTEGER:
			case T_UINTEGER:
				break;
			case T_NULL:
				if (!obj1->isConstructed() && !obj1->is<Class_base>())
					return true;
				return false;
			default:
				return false;
		}
	}
	return obj1->isEqual(obj2);
}

bool ABCVm::strictEquals(ASObject* obj2, ASObject* obj1)
{
	LOG_CALL( "strictEquals" );
	bool ret=strictEqualImpl(obj1, obj2);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::dup()
{
	LOG_CALL( "dup: DONE" );
}

bool ABCVm::pushTrue()
{
	LOG_CALL( "pushTrue" );
	return true;
}

bool ABCVm::pushFalse()
{
	LOG_CALL( "pushFalse" );
	return false;
}

ASObject* ABCVm::pushNaN()
{
	LOG_CALL( "pushNaN" );
	return abstract_d(getWorker(),Number::NaN);
}

bool ABCVm::ifGT(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TTRUE);
	LOG_CALL("ifGT (" << ((ret)?"taken)":"not taken)"));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifNGT(ASObject* obj2, ASObject* obj1)
{

	//Real comparision demanded to object
	bool ret=!(obj2->isLess(obj1)==TTRUE);
	LOG_CALL("ifNGT (" << ((ret)?"taken)":"not taken)"));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TFALSE);
	LOG_CALL("ifLE (" << ((ret)?"taken)":"not taken)"));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifNLE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=!(obj2->isLess(obj1)==TFALSE);
	LOG_CALL("ifNLE (" << ((ret)?"taken)":"not taken)"));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifGE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TFALSE);
	LOG_CALL("ifGE (" << ((ret)?"taken)":"not taken)"));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifNGE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=!(obj1->isLess(obj2)==TFALSE);
	LOG_CALL("ifNGE (" << ((ret)?"taken)":"not taken)"));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::_throw(call_context* th)
{
	LOG_CALL("throw");
	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,exc);
	setError(exc);
}

void ABCVm::setSuper(call_context* th, int n)
{
	RUNTIME_STACK_POP_CREATE(th,value);
	multiname* name=th->mi->context->getMultiname(n,th);
	LOG_CALL("setSuper " << *name);

	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,obj);

	assert_and_throw(th->function->inClass)
	assert_and_throw(th->function->inClass->super);
	assert_and_throw(obj->getClass());
	assert_and_throw(obj->getClass()->isSubClass(th->function->inClass));

	bool alreadyset=false;
	obj->setVariableByMultiname_intern(*name,*value,ASObject::CONST_NOT_ALLOWED,th->function->inClass->super.getPtr(),&alreadyset,th->worker);
	if (alreadyset)
		ASATOM_DECREF_POINTER(value);
	name->resetNameIfObject();
	obj->decRef();
}

void ABCVm::getSuper(call_context* th, int n)
{
	multiname* name=th->mi->context->getMultiname(n,th);
	LOG_CALL("getSuper " << *name);

	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,obj);

	if(obj->is<Null>())
	{
		LOG(LOG_ERROR,"calling getSuper on null:" << *name << ' ' << obj->toDebugString());
		name->resetNameIfObject();
		obj->decRef();
		createError<TypeError>(th->worker,kConvertNullToObjectError);
		return;
	}
	if (obj->is<Undefined>())
	{
		LOG(LOG_ERROR,"calling getSuper on undefined:" << *name << ' ' << obj->toDebugString());
		name->resetNameIfObject();
		obj->decRef();
		createError<TypeError>(th->worker,kConvertUndefinedToObjectError);
		return;
	}

	Class_base* cls = nullptr;
	if (th->function->inClass && !th->function->inClass->super.isNull())
		cls = th->function->inClass->super.getPtr();
	else if (obj->getClass() && !obj->getClass()->super.isNull())
		cls = obj->getClass()->super.getPtr();
	assert_and_throw(cls);

	asAtom ret=asAtomHandler::invalidAtom;
	obj->getVariableByMultinameIntern(ret,*name,cls,GET_VARIABLE_OPTION::NONE,th->worker);
	if (asAtomHandler::isInvalid(ret))
	{
		createError<ReferenceError>(th->worker,kCallOfNonFunctionError,name->normalizedNameUnresolved(obj->getSystemState()));
		name->resetNameIfObject();
		obj->decRef();
		return;
	}
	name->resetNameIfObject();
	obj->decRef();
	RUNTIME_STACK_PUSH(th,ret);
}

bool ABCVm::getLex(call_context* th, int n)
{
	multiname* name=th->mi->context->getMultiname(n,nullptr);
	//getlex is specified not to allow runtime multinames
	assert_and_throw(name->isStatic);
	return getLex_multiname(th,name,UINT32_MAX);
}
bool ABCVm::getLex_multiname(call_context* th, multiname* name,uint32_t localresult)
{
	LOG_CALL( "getLex"<<(localresult!=UINT32_MAX ? "_l:":":") << *name );
	vector<scope_entry>::reverse_iterator it;
	// o will be a reference owned by this function (or NULL). At
	// the end the reference will be handed over to the runtime
	// stack.
	asAtom o=asAtomHandler::invalidAtom;

	bool canCache = true;
	
	//Find out the current 'this', when looking up over it, we have to consider all of it
	for(uint32_t i = th->curr_scope_stack; i > 0; i--)
	{
		ASObject* s = asAtomHandler::toObject(th->scope_stack[i-1],th->worker);
		// FROM_GETLEX flag tells getVariableByMultiname to
		// ignore non-existing properties in XML obejcts
		// (normally it would return an empty XMLList if the
		// property does not exist).
		// And this ensures dynamic properties are also searched
		GET_VARIABLE_OPTION opt=GET_VARIABLE_OPTION(FROM_GETLEX);
		if(!th->scope_stack_dynamic[i-1])
			opt=(GET_VARIABLE_OPTION)(opt | SKIP_IMPL);
		else
			canCache = false;

		asAtom prop=asAtomHandler::invalidAtom;
		if (s->is<Null>()) // avoid exception
			continue;
		s->getVariableByMultiname(prop,*name, opt,th->worker);
		if(asAtomHandler::isValid(prop))
		{
			o=prop;
			break;
		}
	}
	if(asAtomHandler::isInvalid(o) && th->parent_scope_stack) // check parent scope stack
	{
		for(it=th->parent_scope_stack->scope.rbegin();it!=th->parent_scope_stack->scope.rend();++it)
		{
			// XML_STRICT flag tells getVariableByMultiname to
			// ignore non-existing properties in XML obejcts
			// (normally it would return an empty XMLList if the
			// property does not exist).
			// And this ensures dynamic properties are also searched
			GET_VARIABLE_OPTION opt=GET_VARIABLE_OPTION(FROM_GETLEX);
			if(!it->considerDynamic)
				opt=(GET_VARIABLE_OPTION)(opt | SKIP_IMPL);
			else
				canCache = false;
	
			asAtom prop=asAtomHandler::invalidAtom;
			asAtomHandler::toObject(it->object,th->worker)->getVariableByMultiname(prop,*name, opt,th->worker);
			if(asAtomHandler::isValid(prop))
			{
				o=prop;
				break;
			}
		}
	}
	if(asAtomHandler::isInvalid(o))
	{
		if (name->cachedType && dynamic_cast<const Class_base*>(name->cachedType))
		{
			o = asAtomHandler::fromObjectNoPrimitive((Class_base*)dynamic_cast<const Class_base*>(name->cachedType));
		}
	}
	if(asAtomHandler::isInvalid(o))
	{
		ASObject* target;
		getCurrentApplicationDomain(th)->getVariableAndTargetByMultiname(o,*name, target,th->worker);
		if(asAtomHandler::isInvalid(o))
		{
			LOG(LOG_NOT_IMPLEMENTED,"getLex: " << *name<< " not found");
			createError<ReferenceError>(th->worker,kUndefinedVarError,name->normalizedNameUnresolved(th->sys));
			RUNTIME_STACK_PUSH(th,asAtomHandler::fromObject(th->worker->getSystemState()->getUndefinedRef()));
			name->resetNameIfObject();
			return false;
		}
		ASATOM_INCREF(o);
	}
	else
		// TODO can we cache objects found in the scope_stack? 
		canCache = false;
	name->resetNameIfObject();
	if (localresult!= UINT32_MAX)
	{
		ASATOM_DECREF(th->locals[localresult]);
		asAtomHandler::set(th->locals[localresult],o);
	}
	else
		RUNTIME_STACK_PUSH(th,o);
	if (asAtomHandler::is<Class_inherit>(o))
		asAtomHandler::as<Class_inherit>(o)->checkScriptInit();
	return canCache && !asAtomHandler::isPrimitive(o); // don't cache primitive values as they may change
}

void ABCVm::constructSuper(call_context* th, int m)
{
	LOG_CALL( "constructSuper " << m);
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	asAtom obj=asAtomHandler::invalidAtom;
	RUNTIME_STACK_POP(th,obj);

	assert_and_throw(th->function->inClass);
	assert_and_throw(th->function->inClass->super);
	assert_and_throw(asAtomHandler::getObject(obj)->getClass());
	assert_and_throw(asAtomHandler::getObject(obj)->getClass()->isSubClass(th->function->inClass));
	LOG_CALL("Super prototype name " << th->function->inClass->super->class_name);

	th->function->inClass->super->handleConstruction(obj,args, m, false);
	ASATOM_DECREF(obj);
	LOG_CALL("End super construct "<<asAtomHandler::toDebugString(obj));
}

ASObject* ABCVm::findProperty(call_context* th, multiname* name)
{
	LOG_CALL( "findProperty " << *name );

	vector<scope_entry>::reverse_iterator it;
	bool found=false;
	ASObject* ret=nullptr;
	for(uint32_t i = th->curr_scope_stack; i > 0; i--)
	{
		found=asAtomHandler::hasPropertyByMultiname(th->scope_stack[i-1],*name, th->scope_stack_dynamic[i-1], true,th->worker);

		if(found)
		{
			//We have to return the object, not the property
			ret=asAtomHandler::toObject(th->scope_stack[i-1],th->worker);
			break;
		}
	}
	if(!found && th->parent_scope_stack) // check parent scope stack
	{
		for(it=th->parent_scope_stack->scope.rbegin();it!=th->parent_scope_stack->scope.rend();++it)
		{
			found=asAtomHandler::hasPropertyByMultiname(it->object,*name, it->considerDynamic, true,th->worker);
	
			if(found)
			{
				//We have to return the object, not the property
				ret=asAtomHandler::toObject(it->object,th->worker);
				break;
			}
		}
	}
	if(!found)
	{
		//try to find a global object where this is defined
		ASObject* target;
		asAtom o=asAtomHandler::invalidAtom;
		getCurrentApplicationDomain(th)->getVariableAndTargetByMultiname(o,*name, target,th->worker);
		if(asAtomHandler::isValid(o))
			ret=target;
		else //else push the current global object
		{
			if (th->parent_scope_stack && th->parent_scope_stack->scope.size() > 0)
				ret =asAtomHandler::toObject(th->parent_scope_stack->scope[0].object,th->worker);
			else
			{
				assert_and_throw(th->curr_scope_stack > 0);
				ret =asAtomHandler::toObject(th->scope_stack[0],th->worker);
			}
		}
	}

	//TODO: make this a regular assert
	assert_and_throw(ret);
	ret->incRef();
	return ret;
}

ASObject* ABCVm::findPropStrict(call_context* th, multiname* name)
{
	LOG_CALL( "findPropStrict " << *name );

	vector<scope_entry>::reverse_iterator it;
	bool found=false;
	ASObject* ret=nullptr;

	for(uint32_t i = th->curr_scope_stack; i > 0; i--)
	{
		found=asAtomHandler::hasPropertyByMultiname(th->scope_stack[i-1],*name, th->scope_stack_dynamic[i-1], true,th->worker);
		if(found)
		{
			//We have to return the object, not the property
			ret=asAtomHandler::toObject(th->scope_stack[i-1],th->worker);
			break;
		}
	}
	if(!found && th->parent_scope_stack) // check parent scope stack
	{
		for(it =th->parent_scope_stack->scope.rbegin();it!=th->parent_scope_stack->scope.rend();++it)
		{
			found=asAtomHandler::hasPropertyByMultiname(it->object,*name, it->considerDynamic, true,th->worker);
			if(found)
			{
				//We have to return the object, not the property
				ret=asAtomHandler::toObject(it->object,th->worker);
				break;
			}
		}
	}
	if(!found)
	{
		ASObject* target;
		asAtom o=asAtomHandler::invalidAtom;
		getCurrentApplicationDomain(th)->getVariableAndTargetByMultiname(o,*name, target,th->worker);
		if(asAtomHandler::isValid(o))
			ret=target;
		else
		{
			LOG(LOG_NOT_IMPLEMENTED,"findPropStrict: " << *name << " not found");
			multiname m(nullptr);
			m.name_type = multiname::NAME_STRING;
			m.name_s_id = name->name_s_id;
			m.isAttribute = false;
			for(uint32_t i = th->curr_scope_stack; i > 0; i--)
			{
				ASObject* r = asAtomHandler::toObject(th->scope_stack[i-1],th->worker);
				if (!r->is<Class_base>())
					continue;
				if (r->as<Class_base>()->checkExistingFunction(m))
				{
					createError<TypeError>(th->worker,kCallOfNonFunctionError,name->normalizedNameUnresolved(th->sys));
					return ret;
				}
			}
			createError<ReferenceError>(th->worker,kUndefinedVarError,name->normalizedNameUnresolved(th->sys));
			return th->sys->getUndefinedRef();
		}
	}

	assert_and_throw(ret);
	ret->incRef();
	return ret;
}
void ABCVm::findPropStrictCache(asAtom &ret, call_context* th)
{
	preloadedcodedata* instrptr = th->exec_pos;
	uint32_t t = th->exec_pos->local3.pos;

	if ((instrptr->local3.flags&ABC_OP_CACHED) == ABC_OP_CACHED)
	{
		if(instrptr->cacheobj2)
			instrptr->cacheobj2->incRef();
		if (instrptr->cacheobj1->is<IFunction>())
			asAtomHandler::setFunction(ret,instrptr->cacheobj1,instrptr->cacheobj2,th->worker);
		else
		{
			instrptr->cacheobj1->incRef();
			ret = asAtomHandler::fromObject(instrptr->cacheobj1);
		}
		return;
	}
	multiname* name=th->mi->context->getMultiname(t,th);
	LOG_CALL( "findPropStrictCache " << *name );

	vector<scope_entry>::reverse_iterator it;
	bool hasdynamic=false;
	bool found=false;

	for(uint32_t i = th->curr_scope_stack; i > 0; i--)
	{
		found=asAtomHandler::hasPropertyByMultiname(th->scope_stack[i-1], *name, th->scope_stack_dynamic[i-1], true,th->worker);
		if (th->scope_stack_dynamic[i-1])
			hasdynamic = true;
		if(found)
		{
			//We have to return the object, not the property
			ret=th->scope_stack[i-1];
			// we can cache the property if the found scope_stack object is a non-dynamic class
			// and no dynamic objects are at higher levels of the scope_stack
			if (asAtomHandler::is<Class_base>(ret) && !hasdynamic)
			{
				// put object in cache
				instrptr->local3.flags = ABC_OP_CACHED;
				instrptr->cacheobj1 = asAtomHandler::toObject(ret,th->worker);
				instrptr->cacheobj2 = asAtomHandler::getClosure(ret);
				if (instrptr->cacheobj1->is<IFunction>())
					instrptr->cacheobj1->as<IFunction>()->clonedFrom=nullptr;
			}
			break;
		}
	}
	if(!found && th->parent_scope_stack) // check parent scope stack
	{
		for(it =th->parent_scope_stack->scope.rbegin();it!=th->parent_scope_stack->scope.rend();++it)
		{
			found=asAtomHandler::hasPropertyByMultiname(it->object,*name, it->considerDynamic, true,th->worker);
			if (it->considerDynamic)
				hasdynamic = true;
			if(found)
			{
				//We have to return the object, not the property
				ret=it->object;
				break;
			}
		}
	}
	if(!found)
	{
		ASObject* target;
		asAtom o=asAtomHandler::invalidAtom;
		getCurrentApplicationDomain(th)->getVariableAndTargetByMultiname(o,*name, target,th->worker);
		if(asAtomHandler::isValid(o))
		{
			ret=asAtomHandler::fromObject(target);
		}
		else
		{
			LOG(LOG_NOT_IMPLEMENTED,"findPropStrict: " << *name << " not found");
			multiname m(nullptr);
			m.name_type = multiname::NAME_STRING;
			m.name_s_id = name->name_s_id;
			m.isAttribute = false;
			for(uint32_t i = th->curr_scope_stack; i > 0; i--)
			{
				ASObject* r = asAtomHandler::toObject(th->scope_stack[i-1],th->worker);
				if (!r->is<Class_base>())
					continue;
				if (r->as<Class_base>()->checkExistingFunction(m))
				{
					createError<TypeError>(th->worker,kCallOfNonFunctionError,name->normalizedNameUnresolved(th->sys));
					name->resetNameIfObject();
					return;
				}
			}
			createError<ReferenceError>(th->worker,kUndefinedVarError,name->normalizedNameUnresolved(th->sys));
			name->resetNameIfObject();
			return;
		}
		// we only cache the property if
		// - the property was not found in the scope_stack and
		// - the scope_stack does not contain any dynamic objects and
		// - the property was found in one of the global scopes
		if (!hasdynamic)
		{
			// put object in cache
			instrptr->local3.flags = ABC_OP_CACHED;
			instrptr->cacheobj1 = asAtomHandler::toObject(ret,th->worker);
			instrptr->cacheobj2 = asAtomHandler::getClosure(ret);
			if (instrptr->cacheobj1->is<IFunction>())
				instrptr->cacheobj1->as<IFunction>()->clonedFrom=nullptr;
		}
	}
	name->resetNameIfObject();
	assert_and_throw(asAtomHandler::isValid(ret));
	ASATOM_INCREF(ret);
}

bool ABCVm::greaterThan(ASObject* obj1, ASObject* obj2)
{
	LOG_CALL("greaterThan");

	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TTRUE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::greaterEquals(ASObject* obj1, ASObject* obj2)
{
	LOG_CALL("greaterEquals");

	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TFALSE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::lessEquals(ASObject* obj1, ASObject* obj2)
{
	LOG_CALL("lessEquals");

	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TFALSE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::initProperty(ASObject* obj, ASObject* value, multiname* name)
{
	//Allow to set contant traits
	asAtom v = asAtomHandler::fromObject(value);
	bool alreadyset=false;
	obj->setVariableByMultiname(*name,v,ASObject::CONST_ALLOWED,&alreadyset,obj->getInstanceWorker());
	if (alreadyset)
		value->decRef();
	obj->decRef();
}

void ABCVm::callStatic(call_context* th, int n, int m, method_info** called_mi, bool keepReturn)
{
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	asAtom obj=asAtomHandler::invalidAtom;
	RUNTIME_STACK_POP(th,obj);
	if(asAtomHandler::isNull(obj))
	{
		LOG(LOG_ERROR,"trying to callStatic on null");
		ASATOM_DECREF(obj);
		for(int i=0;i<m;i++)
			ASATOM_DECREF(args[i]);
		createError<TypeError>(th->worker,kConvertNullToObjectError);
	}
	else if (asAtomHandler::isUndefined(obj))
	{
		LOG(LOG_ERROR,"trying to callStatic on undefined");
		ASATOM_DECREF(obj);
		for(int i=0;i<m;i++)
			ASATOM_DECREF(args[i]);
		createError<TypeError>(th->worker,kConvertUndefinedToObjectError);
	}
	else
	{
		method_info* mi = th->mi->context->get_method(n);
		assert_and_throw(mi);
		SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(th->worker,mi,mi->numArgs()-mi->numOptions());
		if(f)
		{
			asAtom v = asAtomHandler::fromObject(f);
			callImpl(th, v, obj, args, m, keepReturn);
		}
		else
		{
			tiny_string clsname = asAtomHandler::toObject(obj,th->worker)->getClassName();
			ASATOM_DECREF(obj);
			for(int i=0;i<m;i++)
				ASATOM_DECREF(args[i]);
			createError<ReferenceError>(th->worker,kCallNotFoundError, "?", clsname);
		}
	}
	LOG_CALL("End of callStatic ");
}

void ABCVm::callSuper(call_context* th, int n, int m, method_info** called_mi, bool keepReturn)
{
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	multiname* name=th->mi->context->getMultiname(n,th);
	LOG_CALL((keepReturn ? "callSuper " : "callSuperVoid ") << *name << ' ' << m);

	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,obj);
	if(obj->is<Null>())
	{
		obj->decRef();
		for(int i=0;i<m;++i)
			ASATOM_DECREF(args[i]);
		LOG(LOG_ERROR,"trying to call super on null:"<<*name);
		createError<TypeError>(th->worker,kConvertNullToObjectError);
		return;
	}
	if (obj->is<Undefined>())
	{
		obj->decRef();
		for(int i=0;i<m;++i)
			ASATOM_DECREF(args[i]);
		LOG(LOG_ERROR,"trying to call super on undefined:"<<*name);
		createError<TypeError>(th->worker,kConvertUndefinedToObjectError);
		return;
	}

	assert_and_throw(th->function->inClass);
	assert_and_throw(th->function->inClass->super);
	assert_and_throw(obj->getClass());
	assert_and_throw(obj->getClass()->isSubClass(th->function->inClass));
	asAtom f=asAtomHandler::invalidAtom;
	obj->getVariableByMultinameIntern(f,*name,th->function->inClass->super.getPtr(),GET_VARIABLE_OPTION::NONE, th->worker);
	name->resetNameIfObject();
	if(asAtomHandler::isValid(f))
	{
		asAtom v = asAtomHandler::fromObject(obj);
		callImpl(th, f, v, args, m, keepReturn);
	}
	else
	{
		tiny_string clsname = obj->getClassName();
		obj->decRef();
		for(int i=0;i<m;i++)
			ASATOM_DECREF(args[i]);
		//LOG(LOG_ERROR,"Calling an undefined function " << th->sys->getStringFromUniqueId(name->name_s_id));
		createError<ReferenceError>(th->worker,kCallNotFoundError, name->qualifiedString(th->sys), clsname);
	}
	LOG_CALL("End of callSuper " << *name);
}

bool ABCVm::isType(ABCContext* context, ASObject* obj, multiname* name)
{
	bool ret = context->isinstance(obj, name);
	obj->decRef();
	return ret;
}

bool ABCVm::isTypelate(ASObject* type, ASObject* obj)
{
	LOG_CALL("isTypelate");
	bool real_ret=false;

	Class_base* objc=nullptr;
	Class_base* c=nullptr;
	switch (type->getObjectType())
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_OBJECT:
		case T_STRING:
			LOG(LOG_ERROR,"trying to call isTypelate on object:"<<obj->toDebugString());
			createError<TypeError>(type->getInstanceWorker(),kIsTypeMustBeClassError);
			obj->decRef();
			type->decRef();
			return real_ret;
		case T_NULL:
			LOG(LOG_ERROR,"trying to call isTypelate on null:"<<obj->toDebugString());
			createError<TypeError>(type->getInstanceWorker(),kConvertNullToObjectError);
			obj->decRef();
			type->decRef();
			return real_ret;
		case T_UNDEFINED:
			LOG(LOG_ERROR,"trying to call isTypelate on undefined:"<<obj->toDebugString());
			createError<TypeError>(type->getInstanceWorker(),kConvertUndefinedToObjectError);
			obj->decRef();
			type->decRef();
			return real_ret;
		case T_CLASS:
			break;
		default:
			createError<TypeError>(type->getInstanceWorker(),kIsTypeMustBeClassError);
			obj->decRef();
			type->decRef();
			return real_ret;
	}

	c=static_cast<Class_base*>(type);
	//Special case numeric types
	if(obj->getObjectType()==T_INTEGER || obj->getObjectType()==T_UINTEGER || obj->getObjectType()==T_NUMBER)
	{
		if(c==Class<Number>::getClass(c->getSystemState()) || c==c->getSystemState()->getObjectClassRef())
			real_ret=true;
		else if(c==Class<Integer>::getClass(c->getSystemState()))
			real_ret=(obj->toNumber()==obj->toInt());
		else if(c==Class<UInteger>::getClass(c->getSystemState()))
			real_ret=(obj->toNumber()==obj->toUInt());
		else
			real_ret=false;
		LOG_CALL("Numeric type is " << ((real_ret)?"":"not ") << "subclass of " << c->class_name);
		obj->decRef();
		type->decRef();
		return real_ret;
	}

	if(obj->classdef)
	{
		objc=obj->classdef;
	}
	else
	{
		real_ret=obj->getObjectType()==type->getObjectType();
		LOG_CALL("isTypelate on non classed object " << real_ret <<" "<<obj->toDebugString()<<" "<<type->toDebugString());
		obj->decRef();
		type->decRef();
		return real_ret;
	}

	real_ret=objc->isSubClass(c);
	LOG_CALL("Type " << objc->class_name << " is " << ((real_ret)?"":"not ") 
			<< "subclass of " << c->class_name);
	obj->decRef();
	type->decRef();
	return real_ret;
}

ASObject* ABCVm::asType(ABCContext* context, ASObject* obj, multiname* name)
{
	bool ret = context->isinstance(obj, name);
	LOG_CALL("asType "<<*name<<" "<<obj->toDebugString());
	
	if(ret)
		return obj;
	else
	{
		ASObject* res = obj->getSystemState()->getNullRef();
		obj->decRef();
		return res;
	}
}

ASObject* ABCVm::asTypelate(ASObject* type, ASObject* obj)
{
	LOG_CALL("asTypelate");

	if(!type->is<Class_base>())
	{
		LOG(LOG_ERROR,"trying to call asTypelate on non class object:"<<obj->toDebugString());
		createError<TypeError>(type->getInstanceWorker(),kConvertNullToObjectError);
		obj->decRef();
		return nullptr;
	}
	Class_base* c=static_cast<Class_base*>(type);
	//Special case numeric types
	if(obj->getObjectType()==T_INTEGER || obj->getObjectType()==T_UINTEGER || obj->getObjectType()==T_NUMBER)
	{
		bool real_ret;
		if(c==Class<Number>::getClass(c->getSystemState()) || c==c->getSystemState()->getObjectClassRef())
			real_ret=true;
		else if(c==Class<Integer>::getClass(c->getSystemState()))
			real_ret=(obj->toNumber()==obj->toInt());
		else if(c==Class<UInteger>::getClass(c->getSystemState()))
			real_ret=(obj->toNumber()==obj->toUInt());
		else
			real_ret=false;
		LOG_CALL("Numeric type is " << ((real_ret)?"":"not ") << "subclass of " << c->class_name);
		type->decRef();
		if(real_ret)
			return obj;
		else
		{
			ASObject* res = obj->getSystemState()->getNullRef();
			obj->decRef();
			return res;
		}
	}

	Class_base* objc;
	if(obj->classdef)
		objc=obj->classdef;
	else
	{
		ASObject* res = obj->getSystemState()->getNullRef();
		obj->decRef();
		type->decRef();
		return res;
	}

	bool real_ret=objc->isSubClass(c);
	LOG_CALL("Type " << objc->class_name << " is " << ((real_ret)?" ":"not ")
			<< "subclass of " << c->class_name);
	type->decRef();
	if(real_ret)
		return obj;
	else
	{
		ASObject* res = obj->getSystemState()->getNullRef();
		obj->decRef();
		return res;
	}
}

bool ABCVm::ifEq(ASObject* obj1, ASObject* obj2)
{
	bool ret=obj1->isEqual(obj2);
	LOG_CALL("ifEq (" << ((ret)?"taken)":"not taken)"));

	//Real comparision demanded to object
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifStrictEq(ASObject* obj2, ASObject* obj1)
{
	bool ret=strictEqualImpl(obj1,obj2);
	LOG_CALL("ifStrictEq "<<ret);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifStrictNE(ASObject* obj2, ASObject* obj1)
{
	bool ret=!strictEqualImpl(obj1,obj2);
	LOG_CALL("ifStrictNE "<<ret);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::in(ASObject* val2, ASObject* val1)
{
	LOG_CALL( "in" );
	if(val2->is<Null>())
	{
		createError<TypeError>(getWorker(),kConvertNullToObjectError);
		val1->decRef();
		return false;
	}

	multiname name(nullptr);
	name.name_type=multiname::NAME_OBJECT;
	//Acquire the reference
	name.name_o=val1;
	name.ns.emplace_back(val2->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	bool ret=val2->hasPropertyByMultiname(name, true, true,val2->getInstanceWorker());
	name.name_o=nullptr;
	val1->decRef();
	val2->decRef();
	return ret;
}

bool ABCVm::ifFalse(ASObject* obj1)
{
	bool ret=!Boolean_concrete(obj1);
	LOG_CALL("ifFalse (" << ((ret)?"taken":"not taken") << ')');

	obj1->decRef();
	return ret;
}

void ABCVm::constructProp(call_context* th, int n, int m)
{
	th->explicitConstruction = true;
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	multiname* name=th->mi->context->getMultiname(n,th);

	LOG_CALL("constructProp "<< *name << ' ' << m);

	asAtom obj=asAtomHandler::invalidAtom;
	RUNTIME_STACK_POP(th,obj);

	asAtom o=asAtomHandler::invalidAtom;
	asAtomHandler::toObject(obj,th->worker)->getVariableByMultiname(o,*name,GET_VARIABLE_OPTION::NONE, th->worker);

	name->resetNameIfObject();
	if(asAtomHandler::isInvalid(o))
	{
		for(int i=0;i<m;++i)
			ASATOM_DECREF(args[i]);
		ASATOM_DECREF(obj);
		if (asAtomHandler::is<Undefined>(obj))
			createError<TypeError>(th->worker,kConvertUndefinedToObjectError);
		else if (asAtomHandler::isPrimitive(obj))
			createError<TypeError>(th->worker,kConstructOfNonFunctionError);
		else
			createError<ReferenceError>(th->worker,kUndefinedVarError, name->normalizedNameUnresolved(th->sys));
		return;
	}
	LOG_CALL("Constructing "<<asAtomHandler::toDebugString(o));
	asAtom ret=asAtomHandler::invalidAtom;
	if(asAtomHandler::isClass(o))
	{
		Class_base* o_class=asAtomHandler::as<Class_base>(o);
		o_class->getInstance(th->worker,ret,true,args,m);
	}
	else if(asAtomHandler::isFunction(o))
	{
		constructFunction(ret,th, o, args, m);
	}
	else if (asAtomHandler::isTemplate(o))
	{
		th->explicitConstruction = false;
		for(int i=0;i<m;++i)
			ASATOM_DECREF(args[i]);
		ASATOM_DECREF(o);
		ASATOM_DECREF(obj);
		createError<TypeError>(th->worker,kConstructOfNonFunctionError);
		return;
	}
	else
	{
		th->explicitConstruction = false;
		for(int i=0;i<m;++i)
			ASATOM_DECREF(args[i]);
		ASATOM_DECREF(o);
		ASATOM_DECREF(obj);
		createError<TypeError>(th->worker,kNotConstructorError);
		return;
	}
	RUNTIME_STACK_PUSH(th,ret);
	if (asAtomHandler::getObject(ret))
		asAtomHandler::getObject(ret)->setConstructorCallComplete();

	ASATOM_DECREF(o);
	ASATOM_DECREF(obj);
	LOG_CALL("End of constructing " << asAtomHandler::toDebugString(ret));
	th->explicitConstruction = false;
}

bool ABCVm::hasNext2(call_context* th, int n, int m)
{
	LOG_CALL("hasNext2 " << n << ' ' << m);
	ASObject* obj=asAtomHandler::getObject(th->locals[n]);
	//If the local is not assigned or is a primitive bail out
	if(obj==nullptr)
		return false;

	uint32_t curIndex=asAtomHandler::toUInt(th->locals[m]);

	uint32_t newIndex=obj->nextNameIndex(curIndex);
	ASATOM_DECREF(th->locals[m]);
	asAtomHandler::setUInt(th->locals[m],th->worker,newIndex);
	if(newIndex==0)
	{
		ASATOM_DECREF(th->locals[n]);
		th->locals[n]=asAtomHandler::nullAtom;
		return false;
	}
	return true;
}

void ABCVm::newObject(call_context* th, int n)
{
	th->explicitConstruction = true;
	LOG_CALL("newObject " << n);
	ASObject* ret=Class<ASObject>::getInstanceS(th->worker);
	//Duplicated keys overwrite the previous value
	multiname propertyName(nullptr);
	propertyName.name_type=multiname::NAME_STRING;
	for(int i=0;i<n;i++)
	{
		RUNTIME_STACK_POP_CREATE(th,value);
		RUNTIME_STACK_POP_CREATE(th,name);
		uint32_t nameid=asAtomHandler::toStringId(*name,th->worker);
		ASATOM_DECREF_POINTER(name);
		ret->setDynamicVariableNoCheck(nameid,*value);
	}

	RUNTIME_STACK_PUSH(th,asAtomHandler::fromObject(ret));
	th->explicitConstruction = false;
}

void ABCVm::getDescendants(call_context* th, int n)
{
	multiname* name=th->mi->context->getMultiname(n,th);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,obj);
	LOG_CALL("getDescendants " << *name << " " <<name->isAttribute<< " "<<obj->getClassName());
	XML::XMLVector ret;
	XMLList* targetobject = nullptr;
	if(obj->getClass()==Class<XML>::getClass(obj->getSystemState()))
	{
		XML* xmlObj=Class<XML>::cast(obj);
		targetobject = xmlObj->getChildrenlist();
		xmlObj->getDescendantsByQName(*name, ret);
	}
	else if(obj->getClass()==Class<XMLList>::getClass(th->sys))
	{
		XMLList* xmlObj=Class<XMLList>::cast(obj);
		targetobject = xmlObj;
		xmlObj->getDescendantsByQName(*name, ret);
	}
	else if(obj->is<Proxy>())
	{
		multiname callPropertyName(nullptr);
		callPropertyName.name_type=multiname::NAME_STRING;
		callPropertyName.name_s_id=obj->getSystemState()->getUniqueStringId("getDescendants");
		callPropertyName.ns.emplace_back(th->sys,flash_proxy,NAMESPACE);
		asAtom o=asAtomHandler::invalidAtom;
		GET_VARIABLE_RESULT res = obj->getVariableByMultiname(o,callPropertyName,SKIP_IMPL,th->worker);
		
		if(asAtomHandler::isValid(o))
		{
			assert_and_throw(asAtomHandler::isFunction(o));
			
			//Create a new array
			asAtom* proxyArgs=g_newa(asAtom, 1);
			ASObject* namearg = abstract_s(th->worker, name->normalizedName(th->sys));
			namearg->setProxyProperty(*name);
			proxyArgs[0]=asAtomHandler::fromObject(namearg);

			//We now suppress special handling
			LOG_CALL("Proxy::getDescendants "<<*name<<" "<<asAtomHandler::toDebugString(o));
			ASATOM_INCREF(o);
			asAtom v = asAtomHandler::fromObject(obj);
			asAtom ret=asAtomHandler::invalidAtom;
			asAtomHandler::callFunction(o,th->worker,ret,v,proxyArgs,1,true);
			LOG_CALL("Proxy::getDescendants done " << *name<<" "<<asAtomHandler::toDebugString(o));
			ASATOM_DECREF(o);
			RUNTIME_STACK_PUSH(th,ret);
			if (res & GET_VARIABLE_RESULT::GETVAR_ISNEWOBJECT)
				ASATOM_DECREF(o);
			return;
		}
		else
		{
			tiny_string objName = obj->getClassName();
			obj->decRef();
			createError<TypeError>(th->worker,kDescendentsError, objName);
			return;
		}
	}
	else
	{
		tiny_string objName = obj->getClassName();
		obj->decRef();
		createError<TypeError>(th->worker,kDescendentsError, objName);
		return;
	}
	XMLList* retObj=XMLList::create(th->worker,ret,targetobject,*name);
	RUNTIME_STACK_PUSH(th,asAtomHandler::fromObject(retObj));
	obj->decRef();
}

number_t ABCVm::increment(ASObject* o)
{
	LOG_CALL("increment");

	number_t n=o->toNumber();
	o->decRef();
	return n+1;
}

uint32_t ABCVm::increment_i(ASObject* o)
{
	LOG_CALL("increment_i");

	int n=o->toInt();
	o->decRef();
	return n+1;
}

uint64_t ABCVm::increment_di(ASObject* o)
{
	LOG_CALL("increment_di");

	int64_t n=o->toInt64();
	o->decRef();
	return n+1;
}

ASObject* ABCVm::nextValue(ASObject* index, ASObject* obj)
{
	LOG_CALL("nextValue");
	if(index->getObjectType()!=T_UINTEGER)
		throw UnsupportedException("Type mismatch in nextValue");

	asAtom ret=asAtomHandler::invalidAtom;
	obj->nextValue(ret,index->toUInt());
	obj->decRef();
	index->decRef();
	return asAtomHandler::toObject(ret,obj->getInstanceWorker());
}

ASObject* ABCVm::nextName(ASObject* index, ASObject* obj)
{
	LOG_CALL("nextName");
	if(index->getObjectType()!=T_UINTEGER)
		throw UnsupportedException("Type mismatch in nextName");

	asAtom ret=asAtomHandler::invalidAtom;
	obj->nextName(ret,index->toUInt());
	obj->decRef();
	index->decRef();
	return asAtomHandler::toObject(ret,obj->getInstanceWorker());
}
ASObject* ABCVm::hasNext(ASObject* obj,ASObject* cur_index)
{
	LOG_CALL("hasNext " << obj->toDebugString() << ' ' << cur_index->toDebugString());

	uint32_t curIndex=cur_index->toUInt();

	uint32_t newIndex=obj->nextNameIndex(curIndex);
	return abstract_i(obj->getInstanceWorker(),newIndex);
}

void ABCVm::newClass(call_context* th, int n)
{
	int name_index=th->mi->context->instances[n].name;
	assert_and_throw(name_index);
	const multiname* mname=th->mi->context->getMultiname(name_index,nullptr);
	LOG_CALL( "newClass " << *mname );

	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,baseClass);

	assert_and_throw(mname->ns.size()==1);
	QName className(mname->name_s_id,mname->ns[0].nsNameId);

	Class_inherit* ret = nullptr;
	auto i = th->mi->context->root->applicationDomain->classesBeingDefined.cbegin();
	while (i != th->mi->context->root->applicationDomain->classesBeingDefined.cend())
	{
		if(i->first->name_s_id == mname->name_s_id && (i->first->ns.empty() || mname->ns.empty() || (i->first->ns[0].nsRealId == mname->ns[0].nsRealId)))
		{
			ret = (Class_inherit*)i->second;
			ret->incRef();
			break;
		}
		i++;
	}

	if (ret == nullptr)
	{
		//Check if this class has been already defined
		_NR<ApplicationDomain> domain = getCurrentApplicationDomain(th);
		ASObject* target;
		asAtom oldDefinition=asAtomHandler::invalidAtom;
		domain->getVariableAndTargetByMultiname(oldDefinition,*mname, target,th->worker);
		if(asAtomHandler::isClass(oldDefinition))
		{
			LOG_CALL("Class " << className << " already defined. Pushing previous definition");
			baseClass->decRef();
			ASATOM_INCREF(oldDefinition);
			RUNTIME_STACK_PUSH(th,oldDefinition);
			// ensure that this interface is linked to all previously defined classes implementing this interface
			if (th->mi->context->instances[n].isInterface())
				th->mi->context->root->applicationDomain->SetAllClassLinks();
			return;
		}
		MemoryAccount* m = th->sys->allocateMemoryAccount(className.getQualifiedName(th->sys));
		ret=new (m) Class_inherit(className, m,nullptr,nullptr);
		ret->setWorker(th->worker);
		ret->setRefConstant();

		LOG_CALL("add classes defined:"<<*mname<<" "<<th->mi->context);
		//Add the class to the ones being currently defined in this context
		th->mi->context->root->applicationDomain->classesBeingDefined.insert(make_pair(mname, ret));
		th->mi->context->root->customClasses.insert(make_pair(mname->name_s_id,ret));
	}
	ret->isFinal = th->mi->context->instances[n].isFinal();
	ret->isSealed = th->mi->context->instances[n].isSealed();
	ret->isInterface = th->mi->context->instances[n].isInterface();

	assert_and_throw(th->mi->context);
	ret->context=th->mi->context;

	// baseClass may be Null if we are currently also executing the newClass opcode of the base class
	if(baseClass->getObjectType()==T_NULL && th->mi->context->instances[n].supername)
	{
		multiname* mnsuper = th->mi->context->getMultiname(th->mi->context->instances[n].supername,nullptr);
		auto i = th->mi->context->root->applicationDomain->classesBeingDefined.cbegin();
		while (i != th->mi->context->root->applicationDomain->classesBeingDefined.cend())
		{
			if(i->first->name_s_id == mnsuper->name_s_id && i->first->ns[0].nsRealId == mnsuper->ns[0].nsRealId)
			{
				baseClass = i->second;
				break;
			}
			i++;
		}
	}

	//Null is a "valid" base class
	if(baseClass->getObjectType()!=T_NULL)
	{
		assert_and_throw(baseClass->is<Class_base>());
		Class_base* base = baseClass->as<Class_base>();
		if (base->is<Class_inherit>())
			base->as<Class_inherit>()->checkScriptInit();
		assert(!base->isFinal);
		if (ret->super.isNull())
			ret->setSuper(_MR(base));
		else if (base != ret->super.getPtr())
		{
			LOG(LOG_ERROR,"resetting super class from "<<ret->super->toDebugString() <<" to "<< base->toDebugString());
			ret->setSuper(_MR(base));
		}
		th->mi->context->root->applicationDomain->addSuperClassNotFilled(ret);
	}

	//Add protected namespace if needed
	if(th->mi->context->instances[n].isProtectedNs())
	{
		ret->use_protected=true;
		int ns=th->mi->context->instances[n].protectedNs;
		const namespace_info& ns_info=th->mi->context->constant_pool.namespaces[ns];
		ret->initializeProtectedNamespace(th->mi->context->getString(ns_info.name),ns_info,th->mi->context->root);
	}

	IFunction* f = Class<IFunction>::getFunction(ret->getSystemState(),Class_base::_toString);
	f->setRefConstant();
	ret->setDeclaredMethodByQName(BUILTIN_STRINGS::STRING_TOSTRING,nsNameAndKind(ret->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),f,NORMAL_METHOD,false);

	if (th->parent_scope_stack)
		ret->class_scope = th->parent_scope_stack->scope;
	for(uint32_t i = 0 ; i < th->curr_scope_stack; i++)
	{
		ret->class_scope.push_back(scope_entry(th->scope_stack[i],th->scope_stack_dynamic[i]));
	}

	// set constructor property first, as it seems to be allowed to override the constructor property by a class trait
	ret->addConstructorGetter();

	LOG_CALL("Building class traits");
	std::vector<multiname*> additionalslots;
	for(unsigned int i=0;i<th->mi->context->classes[n].trait_count;i++)
		th->mi->context->buildTrait(ret,additionalslots,&th->mi->context->classes[n].traits[i],false);

	LOG_CALL("Adding immutable object traits to class");
	//Class objects also contains all the methods/getters/setters declared for instances
	instance_info* cur=&th->mi->context->instances[n];
	for(unsigned int i=0;i<cur->trait_count;i++)
	{
		// don't build slot traits as they mess up protected namespaces
		int kind=cur->traits[i].kind&0xf;
		if(kind==traits_info::Method || kind==traits_info::Setter || kind==traits_info::Getter)
			th->mi->context->buildTrait(ret,additionalslots,&cur->traits[i],true);
	}
	ret->initAdditionalSlots(additionalslots);

	method_info* constructor=&th->mi->context->methods[th->mi->context->instances[n].init];
	if(constructor->body) /* e.g. interfaces have no valid constructor */
	{
#ifdef PROFILING_SUPPORT
		if(!constructor->validProfName)
		{
			constructor->profName=mname->normalizedName(th->sys)+"::__CONSTRUCTOR__";
			constructor->validProfName=true;
		}
#endif
		SyntheticFunction* constructorFunc=Class<IFunction>::getSyntheticFunction(th->worker,constructor,constructor->numArgs()-constructor->numOptions());
		constructorFunc->setRefConstant();
		constructorFunc->acquireScope(ret->class_scope);
		constructorFunc->addToScope(scope_entry(asAtomHandler::fromObject(ret),false));
		constructorFunc->inClass = ret;
		//add Constructor the the class methods
		ret->constructor=constructorFunc;
	}
	ret->class_index=n;
	th->mi->context->root->bindClass(className,ret);

	//Add prototype variable
	ret->prototype = _MNR(new_objectPrototype(ret->getInstanceWorker()));
	ret->prototype->getObj()->setRefConstant();
	//Add the constructor variable to the class prototype
	ret->prototype->setVariableByQName(BUILTIN_STRINGS::STRING_CONSTRUCTOR,nsNameAndKind(),ret, DECLARED_TRAIT);
	if(ret->super)
		ret->prototype->prevPrototype=ret->super->prototype;
	ret->addPrototypeGetter();
	if (constructor->body)
		ret->constructorprop = _NR<ObjectConstructor>(new_objectConstructor(ret,ret->constructor->length));
	else
		ret->constructorprop = _NR<ObjectConstructor>(new_objectConstructor(ret,0));

	//add implemented interfaces
	for(unsigned int i=0;i<th->mi->context->instances[n].interface_count;i++)
	{
		multiname* name=th->mi->context->getMultiname(th->mi->context->instances[n].interfaces[i],nullptr);
		ret->addImplementedInterface(*name);

		//Make the class valid if needed
		ASObject* target;
		asAtom obj=asAtomHandler::invalidAtom;
		getCurrentApplicationDomain(th)->getVariableAndTargetByMultiname(obj,*name, target,th->worker);

		//Named only interfaces seems to be allowed 
		if(asAtomHandler::isInvalid(obj))
			continue;

	}
	
	LOG_CALL("Calling Class init " << ret);
	//Class init functions are called with global as this
	method_info* m=&th->mi->context->methods[th->mi->context->classes[n].cinit];
	SyntheticFunction* cinit=Class<IFunction>::getSyntheticFunction(th->worker,m,m->numArgs()-m->numOptions());
	cinit->fromNewFunction=true;
	cinit->inClass = ret;
	cinit->setRefConstant();
	//cinit must inherit the current scope
	if (th->parent_scope_stack)
		cinit->acquireScope(th->parent_scope_stack->scope);
	for(uint32_t i = 0 ; i < th->curr_scope_stack; i++)
	{
		cinit->addToScope(scope_entry(th->scope_stack[i],th->scope_stack_dynamic[i]));
	}
	
	asAtom ret2=asAtomHandler::invalidAtom;
	try
	{
		asAtom v = asAtomHandler::fromObject(ret);
		asAtom f = asAtomHandler::fromObject(cinit);
		asAtomHandler::callFunction(f,th->worker,ret2,v,nullptr,0,true,false,false);
	}
	catch(ASObject* exc)
	{
		LOG_CALL("Exception during class initialization. Returning Undefined");
		//Handle eventual exceptions from the constructor, to fix the stack
		RUNTIME_STACK_PUSH(th,asAtomHandler::fromObject(th->mi->context->root->applicationDomain->getSystemState()->getUndefinedRef()));
		cinit->decRef();

		//Remove the class to the ones being currently defined in this context
		th->mi->context->root->applicationDomain->classesBeingDefined.erase(mname);
		throw;
	}
	assert_and_throw(asAtomHandler::isUndefined(ret2));
	ASATOM_DECREF(ret2);
	LOG_CALL("End of Class init " << *mname <<" " <<ret);
	RUNTIME_STACK_PUSH(th,asAtomHandler::fromObjectNoPrimitive(ret));
	cinit->decRef();
	if (ret->getGlobalScope()) // the variable on the Definition Object was set to null in class definition, but can be set to the real object now that the class init function was called
		ret->getGlobalScope()->setVariableByQName(mname->name_s_id,mname->ns[0], ret,DECLARED_TRAIT);
	th->mi->context->root->bindClass(className,ret);
	
	th->mi->context->root->applicationDomain->copyBorrowedTraitsFromSuper(ret);
	
	//If the class is not an interface itself and has no super class, link the traits here (if it has a super class, the traits are linked in copyBorrowedTraitsFromSuper
	if(ret->super.isNull() && !th->mi->context->instances[n].isInterface())
	{
		//Link all the interfaces for this class and all the bases
		if (!th->mi->context->root->applicationDomain->newClassRecursiveLink(ret, ret))
		{
			// remember classes where not all interfaces are defined yet
			th->mi->context->root->applicationDomain->AddClassLinks(ret);
		}
	}
	// ensure that this interface is linked to all previously defined classes implementing this interface
	if (th->mi->context->instances[n].isInterface())
		th->mi->context->root->applicationDomain->SetAllClassLinks();
	ret->setConstructIndicator();
	//Remove the class to the ones being currently defined in this context
	th->mi->context->root->applicationDomain->classesBeingDefined.erase(mname);
}

void ABCVm::swap()
{
	LOG_CALL("swap");
}

ASObject* ABCVm::newActivation(call_context* th, method_info* mi)
{
	LOG_CALL("newActivation");
	//TODO: Should method traits be added to the activation context?
	ASObject* act= nullptr;
	ASObject* caller = asAtomHandler::getObject(th->locals[0]);
	if (caller != nullptr && caller->is<Function_object>())
		act = new_functionObject(caller->as<Function_object>()->functionPrototype);
	else
		act = new_activationObject(th->worker);
#ifndef NDEBUG
	act->initialized=false;
#endif
	act->Variables.Variables.reserve(mi->body->trait_count);
	std::vector<multiname*> additionalslots;
	for(unsigned int i=0;i<mi->body->trait_count;i++)
		th->mi->context->buildTrait(act,additionalslots,&mi->body->traits[i],false,-1,false);
	act->initAdditionalSlots(additionalslots);
#ifndef NDEBUG
	act->initialized=true;
#endif
	return act;
}

void ABCVm::popScope(call_context* th)
{
	LOG_CALL("popScope");
	assert_and_throw(th->curr_scope_stack);
	th->curr_scope_stack--;
	if (asAtomHandler::isObject(th->scope_stack[th->curr_scope_stack]))
		asAtomHandler::getObjectNoCheck(th->scope_stack[th->curr_scope_stack])->removeStoredMember();
}

bool ABCVm::lessThan(ASObject* obj1, ASObject* obj2)
{
	LOG_CALL("lessThan");

	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TTRUE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::call(call_context* th, int m, method_info** called_mi)
{
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	asAtom obj=asAtomHandler::invalidAtom;
	RUNTIME_STACK_POP(th,obj);
	asAtom f=asAtomHandler::invalidAtom;
	RUNTIME_STACK_POP(th,f);
	LOG_CALL("call " << m << ' ' << asAtomHandler::toDebugString(f));
	asAtom obj2 = asAtomHandler::getClosureAtom(f,obj);
	if (obj.uintval != obj2.uintval)
	{
		ASATOM_INCREF(obj2);
		ASATOM_DECREF(obj);
	}
	callImpl(th, f, obj2, args, m, true);
}
// this consumes one reference of f, obj and of each arg
void ABCVm::callImpl(call_context* th, asAtom& f, asAtom& obj, asAtom* args, int m, bool keepReturn)
{
	if(asAtomHandler::is<IFunction>(f))
	{
		asAtom ret=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(f,th->worker,ret,obj,args,m,true);
		if(keepReturn)
			RUNTIME_STACK_PUSH(th,ret);
		else
			ASATOM_DECREF(ret);
	}
	else if(asAtomHandler::is<Class_base>(f))
	{
		Class_base* c=asAtomHandler::as<Class_base>(f);
		asAtom ret=asAtomHandler::invalidAtom;
		c->generator(th->worker,ret,args,m);
		for(int i=0;i<m;i++)
			ASATOM_DECREF(args[i]);
		if(keepReturn)
			RUNTIME_STACK_PUSH(th,ret);
		else
			ASATOM_DECREF(ret);
		ASATOM_DECREF(obj);
	}
	else if(asAtomHandler::is<RegExp>(f))
	{
		asAtom ret=asAtomHandler::invalidAtom;
		RegExp::exec(ret,th->worker,f,args,m);
		if(keepReturn)
			RUNTIME_STACK_PUSH(th,ret);
		else
			ASATOM_DECREF(ret);
		for(int i=0;i<m;++i)
			ASATOM_DECREF(args[i]);
		ASATOM_DECREF(obj);
	}
	else
	{
		LOG(LOG_ERROR,"trying to call an object as a function:"<<asAtomHandler::toDebugString(f) <<" on "<<asAtomHandler::toDebugString(obj));
		ASATOM_DECREF(f);
		ASATOM_DECREF(obj);
		for(int i=0;i<m;++i)
			ASATOM_DECREF(args[i]);
		createError<TypeError>(th->worker,kCallOfNonFunctionError, "Object");
	}
	LOG_CALL("End of call " << m << ' ' << asAtomHandler::toDebugString(f));
	ASATOM_DECREF(f);
}

bool ABCVm::deleteProperty(ASObject* obj, multiname* name)
{
	LOG_CALL("deleteProperty " << *name<<" "<<obj->toDebugString());
	if (name->name_type == multiname::NAME_OBJECT && name->name_o)
	{
		if (name->name_o->is<XMLList>())
		{
			createError<TypeError>(getWorker(),kDeleteTypeError,name->name_o->getClassName());
			name->resetNameIfObject();
			return false;
		}
	}
	bool ret = obj->deleteVariableByMultiname(*name,obj->getInstanceWorker());

	obj->decRef();
	return ret;
}

ASObject* ABCVm::newFunction(call_context* th, int n)
{
	LOG_CALL("newFunction " << n);

	method_info* m=&th->mi->context->methods[n];
	SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(th->worker,m,m->numArgs()-m->numOptions());
	f->func_scope = _R<scope_entry_list>(new scope_entry_list());
	f->fromNewFunction=true;
	if (th->parent_scope_stack)
	{
		f->func_scope->scope=th->parent_scope_stack->scope;
		for (auto it = f->func_scope->scope.begin(); it != f->func_scope->scope.end(); it++)
		{
			ASObject* o = asAtomHandler::getObject(it->object);
			if (o && !o->is<Global>())
			{
				o->incRef();
				o->addStoredMember();
			}
		}
	}
	for(uint32_t i = 0 ; i < th->curr_scope_stack; i++)
	{
		ASObject* o = asAtomHandler::getObject(th->scope_stack[i]);
		if (o && !o->is<Global>())
		{
			o->incRef();
			o->addStoredMember();
		}
		f->addToScope(scope_entry(th->scope_stack[i],th->scope_stack_dynamic[i]));
	}
	//Create the prototype object
	f->prototype = _MR(new_asobject(th->worker));
	f->prototype->setVariableAtomByQName(BUILTIN_STRINGS::STRING_CONSTRUCTOR,nsNameAndKind(),asAtomHandler::fromObject(f),DECLARED_TRAIT,true,false);
	return f;
}

ASObject* ABCVm::getScopeObject(call_context* th, int n)
{
	assert_and_throw(th->curr_scope_stack > (size_t)n);
	ASObject* ret=asAtomHandler::toObject(th->scope_stack[(size_t)n],th->worker);
	ret->incRef();
	LOG_CALL( "getScopeObject: " << ret->toDebugString());
	return ret;
}

ASObject* ABCVm::pushString(call_context* th, int n)
{
	LOG_CALL( "pushString " << th->sys->getStringFromUniqueId(th->mi->context->getString(n)) );
	return abstract_s(th->worker,th->mi->context->getString(n));
}

ASObject* ABCVm::newCatch(call_context* th, int n)
{
	ASObject* catchScope = Class<ASObject>::getInstanceS(th->worker);
	assert_and_throw(n >= 0 && (unsigned int)n < th->mi->body->exceptions.size());
	multiname* name = th->mi->context->getMultiname(th->mi->body->exceptions[n].var_name, nullptr);
	catchScope->setVariableByMultiname(*name, asAtomHandler::undefinedAtom,ASObject::CONST_NOT_ALLOWED,nullptr,th->worker);
	variable* v = catchScope->findVariableByMultiname(*name,nullptr,nullptr,nullptr,true,th->worker);
	catchScope->initSlot(1, v);
	return catchScope;
}

void ABCVm::newArray(call_context* th, int n)
{
	th->explicitConstruction = true;
	LOG_CALL( "newArray " << n );
	Array* ret=Class<Array>::getInstanceSNoArgs(th->worker);
	ret->resize(n);
	for(int i=0;i<n;i++)
	{
		RUNTIME_STACK_POP_CREATE(th,obj);
		ret->set(n-i-1,*obj,false,false);
	}

	RUNTIME_STACK_PUSH(th,asAtomHandler::fromObject(ret));
	th->explicitConstruction = false;
}

ASObject* ABCVm::esc_xattr(ASObject* o)
{
	tiny_string t;
	if (o->is<XML>())
		t = o->as<XML>()->toXMLString_internal();
	else if (o->is<XMLList>())
		t = o->as<XMLList>()->toXMLString_internal();
	else
		t = XML::encodeToXML(o->toString(),true);
	ASObject* res = abstract_s(o->getInstanceWorker(),t);
	o->decRef();
	return res;
}

ASObject* ABCVm::esc_xelem(ASObject* o)
{
	tiny_string t;
	if (o->is<XML>())
		t = o->as<XML>()->toXMLString_internal();
	else if (o->is<XMLList>())
		t = o->as<XMLList>()->toXMLString_internal();
	else
		t = XML::encodeToXML(o->toString(),false);
	ASObject* res = abstract_s(o->getInstanceWorker(),t);
	o->decRef();
	return res;
}

/* This should walk prototype chain of value, trying to find type. See ECMA.
 * Its usage is disouraged in AS3 in favour of 'is' and 'as' (opcodes isType and asType)
 */
bool ABCVm::instanceOf(ASObject* value, ASObject* type)
{
	LOG_CALL("instanceOf "<<value->toDebugString()<<" "<<type->toDebugString());
	if(type->is<IFunction>())
	{
		IFunction* t=static_cast<IFunction*>(type);
		ASObject* functionProto=t->prototype.getPtr();
		//Only Function_object instances may come from functions
		ASObject* proto=value;
		while(proto->is<Function_object>())
		{
			proto=proto->as<Function_object>()->functionPrototype.getPtr();
			if(proto==functionProto)
				return true;
		}
		if (value->is<DisplayObject>())
		{
			AVM1Function* constr = value->as<DisplayObject>()->loadedFrom->AVM1getClassConstructor(value->as<DisplayObject>()->getTagID());
			bool res = type== constr;
			if (!res && constr)
			{
				ASObject* pr = constr->prototype.getPtr();
				while (pr)
				{
					if (pr == functionProto)
						return true;
					pr=pr->getprop_prototype();
				}
			}
			return type== value->as<DisplayObject>()->loadedFrom->AVM1getClassConstructor(value->as<DisplayObject>()->getTagID());
		}
		proto = value->getprop_prototype();
		while(proto)
		{
			if(proto==functionProto)
				return true;
			proto=proto->getprop_prototype();
		}
		return false;
	}

	if(!type->is<Class_base>())
	{
		createError<TypeError>(value->getInstanceWorker(),kCantUseInstanceofOnNonObjectError);
		return false;
	}

	if(value->is<Null>())
		return false;


	if(value->is<Class_base>())
		// Classes are instance of Class and Object but not
		// itself or super classes
		return type == Class_object::getClass(type->getSystemState()) || 
			type == Class<ASObject>::getClass(type->getSystemState());
	if (value->is<Function_object>())
	{
		Function_object* t=static_cast<Function_object*>(value);
		value = t->functionPrototype.getPtr();
	}
	return value->getClass() && value->getClass()->isSubClass(type->as<Class_base>(), false);
}

Namespace* ABCVm::pushNamespace(call_context* th, int n)
{
	const namespace_info& ns_info=th->mi->context->constant_pool.namespaces[n];
	LOG_CALL( "pushNamespace " << th->sys->getStringFromUniqueId(th->mi->context->getString(ns_info.name)) );
	return Class<Namespace>::getInstanceS(th->worker,th->mi->context->getString(ns_info.name),BUILTIN_STRINGS::EMPTY,(NS_KIND)(int)ns_info.kind);
}

/* @spec-checked avm2overview */
void ABCVm::dxns(call_context* th, int n)
{
	if(!th->mi->hasDXNS())
	{
		createError<VerifyError>(th->worker,0,"dxns without SET_DXNS");
		return;
	}

	th->defaultNamespaceUri = th->mi->context->getString(n);
}

/* @spec-checked avm2overview */
void ABCVm::dxnslate(call_context* th, ASObject* o)
{
	if(!th->mi->hasDXNS())
	{
		createError<VerifyError>(th->worker,0,"dxnslate without SET_DXNS");
		return;
	}

	th->defaultNamespaceUri = o->toStringId();
	o->decRef();
}
