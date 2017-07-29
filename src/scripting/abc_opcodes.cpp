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
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/flash/utils/Proxy.h"
#include "parsing/streams.h"

using namespace std;
using namespace lightspark;

int32_t ABCVm::bitAnd(ASObject* val2, ASObject* val1)
{
	int32_t i1=val1->toInt();
	int32_t i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG_CALL(_("bitAnd_oo ") << hex << i1 << '&' << i2 << dec);
	return i1&i2;
}

int32_t ABCVm::bitAnd_oi(ASObject* val1, int32_t val2)
{
	int32_t i1=val1->toInt();
	int32_t i2=val2;
	val1->decRef();
	LOG_CALL(_("bitAnd_oi ") << hex << i1 << '&' << i2 << dec);
	return i1&i2;
}

void ABCVm::setProperty(ASObject* value,ASObject* obj,multiname* name)
{
	LOG_CALL(_("setProperty ") << *name << ' ' << obj<<" "<<obj->toDebugString()<<" " <<value);

	if(obj->is<Null>())
	{
		LOG(LOG_ERROR,"calling setProperty on null:" << *name << ' ' << obj->toDebugString()<<" " << value->toDebugString());
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (obj->is<Undefined>())
	{
		LOG(LOG_ERROR,"calling setProperty on undefined:" << *name << ' ' << obj->toDebugString()<<" " << value->toDebugString());
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	//Do not allow to set contant traits
	asAtom v = asAtom::fromObject(value);
	obj->setVariableByMultiname(*name,v,ASObject::CONST_NOT_ALLOWED);
	obj->decRef();
}

void ABCVm::setProperty_i(int32_t value,ASObject* obj,multiname* name)
{
	LOG_CALL(_("setProperty_i ") << *name << ' ' <<obj);
	if(obj->is<Null>())
	{
		LOG(LOG_ERROR,"calling setProperty_i on null:" << *name << ' ' << obj->toDebugString()<<" " << value);
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (obj->is<Undefined>())
	{
		LOG(LOG_ERROR,"calling setProperty_i on undefined:" << *name << ' ' << obj->toDebugString()<<" " << value);
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	obj->setVariableByMultiname_i(*name,value);
	obj->decRef();
}

number_t ABCVm::convert_d(ASObject* o)
{
	LOG_CALL( _("convert_d") );
	number_t ret=o->toNumber();
	o->decRef();
	return ret;
}

bool ABCVm::convert_b(ASObject* o)
{
	LOG_CALL( _("convert_b") );
	bool ret=Boolean_concrete(o);
	o->decRef();
	return ret;
}

uint32_t ABCVm::convert_u(ASObject* o)
{
	LOG_CALL( _("convert_u") );
	uint32_t ret=o->toUInt();
	o->decRef();
	return ret;
}

int32_t ABCVm::convert_i(ASObject* o)
{
	LOG_CALL( _("convert_i") );
	int32_t ret=o->toInt();
	o->decRef();
	return ret;
}

int64_t ABCVm::convert_di(ASObject* o)
{
	LOG_CALL( _("convert_di") );
	int64_t ret=o->toInt64();
	o->decRef();
	return ret;
}

ASObject* ABCVm::convert_s(ASObject* o)
{
	LOG_CALL( _("convert_s") );
	ASObject* ret=o;
	if(o->getObjectType()!=T_STRING)
	{
		ret=abstract_s(o->getSystemState(),o->toString());
		o->decRef();
	}
	return ret;
}

void ABCVm::label()
{
	LOG_CALL( _("label") );
}

void ABCVm::lookupswitch()
{
	LOG_CALL( _("lookupswitch") );
}

ASObject* ABCVm::pushUndefined()
{
	LOG_CALL( _("pushUndefined") );
	return getSys()->getUndefinedRef();
}

ASObject* ABCVm::pushNull()
{
	LOG_CALL( _("pushNull") );
	return getSys()->getNullRef();
}

void ABCVm::coerce_a()
{
	LOG_CALL( _("coerce_a") );
}

ASObject* ABCVm::checkfilter(ASObject* o)
{
	LOG_CALL( _("checkfilter") );
	if (o->is<Null>())
		throwError<TypeError>(kConvertNullToObjectError);
	if (o->is<Undefined>())
		throwError<TypeError>(kConvertUndefinedToObjectError);
	if (!o->is<XML>() && !o->is<XMLList>())
		throwError<TypeError>(kFilterError, o->getClassName());
	return o;
}

ASObject* ABCVm::coerce_s(ASObject* o)
{
	asAtom v = asAtom::fromObject(o);
	return Class<ASString>::getClass(o->getSystemState())->coerce(o->getSystemState(),v).toObject(o->getSystemState());
}

void ABCVm::coerce(call_context* th, int n)
{
	multiname* mn = th->context->getMultiname(n,NULL);
	LOG_CALL("coerce " << *mn);

	RUNTIME_STACK_POINTER_CREATE(th,o);

	const Type* type = mn->cachedType != NULL ? mn->cachedType : Type::getTypeFromMultiname(mn, th->context);
	if (type == NULL)
	{
		LOG(LOG_ERROR,"coerce: type not found:"<< *mn);
		throwError<TypeError>(kClassNotFoundError,mn->qualifiedString(getSys()));
	}
	if (mn->isStatic && mn->cachedType == NULL)
		mn->cachedType = type;
	*o = type->coerce(th->context->root->getSystemState(),*o);
}

void ABCVm::pop()
{
	LOG_CALL( _("pop: DONE") );
}

void ABCVm::getLocal_int(int n, int v)
{
	LOG_CALL(_("getLocal[") << n << _("] (int)= ") << dec << v);
}

void ABCVm::getLocal(ASObject* o, int n)
{
	LOG_CALL(_("getLocal[") << n << _("] (") << o << _(") ") << o->toDebugString());
}

void ABCVm::getLocal_short(int n)
{
	LOG_CALL(_("getLocal[") << n << _("]"));
}

void ABCVm::setLocal(int n)
{
	LOG_CALL(_("setLocal[") << n << _("]"));
}

void ABCVm::setLocal_int(int n, int v)
{
	LOG_CALL(_("setLocal[") << n << _("] (int)= ") << dec << v);
}

void ABCVm::setLocal_obj(int n, ASObject* v)
{
	LOG_CALL(_("setLocal[") << n << _("] = ") << v->toDebugString());
}

int32_t ABCVm::pushShort(intptr_t n)
{
	LOG_CALL( _("pushShort ") << n );
	return n;
}

void ABCVm::setSlot(ASObject* value, ASObject* obj, int n)
{
	LOG_CALL("setSlot " << n << " "<< obj<<" " <<obj->toDebugString() << " "<< value->toDebugString()<<" "<<value);
	obj->setSlot(n,asAtom::fromObject(value));
	obj->decRef();
}

ASObject* ABCVm::getSlot(ASObject* obj, int n)
{
	asAtom ret=obj->getSlot(n);
	LOG_CALL("getSlot " << n << " " << ret.toDebugString());
	//getSlot can only access properties defined in the current
	//script, so they should already be defind by this script
	ASATOM_INCREF(ret);
	obj->decRef();
	return ret.toObject(obj->getSystemState());
}

number_t ABCVm::negate(ASObject* v)
{
	LOG_CALL( _("negate") );
	number_t ret=-(v->toNumber());
	v->decRef();
	return ret;
}

int32_t ABCVm::negate_i(ASObject* o)
{
	LOG_CALL(_("negate_i"));

	int n=o->toInt();
	o->decRef();
	return -n;
}

int32_t ABCVm::bitNot(ASObject* val)
{
	int32_t i1=val->toInt();
	val->decRef();
	LOG_CALL(_("bitNot ") << hex << i1 << dec);
	return ~i1;
}

int32_t ABCVm::bitXor(ASObject* val2, ASObject* val1)
{
	int32_t i1=val1->toInt();
	int32_t i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG_CALL(_("bitXor ") << hex << i1 << '^' << i2 << dec);
	return i1^i2;
}

int32_t ABCVm::bitOr_oi(ASObject* val2, int32_t val1)
{
	int32_t i1=val1;
	int32_t i2=val2->toInt();
	val2->decRef();
	LOG_CALL(_("bitOr ") << hex << i1 << '|' << i2 << dec);
	return i1|i2;
}

int32_t ABCVm::bitOr(ASObject* val2, ASObject* val1)
{
	int32_t i1=val1->toInt();
	int32_t i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG_CALL(_("bitOr ") << hex << i1 << '|' << i2 << dec);
	return i1|i2;
}

void ABCVm::callProperty(call_context* th, int n, int m, method_info** called_mi, bool keepReturn)
{
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
		RUNTIME_STACK_POP(th,args[m-i-1]);

	multiname* name=th->context->getMultiname(n,th);
	LOG_CALL( (keepReturn ? "callProperty " : "callPropVoid") << *name << ' ' << m);

	RUNTIME_STACK_POP_CREATE(th,obj);

	if(obj.is<Null>())
	{
		LOG(LOG_ERROR,"trying to call property on null:"<<*name);
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (obj.is<Undefined>())
	{
		LOG(LOG_ERROR,"trying to call property on undefined:"<<*name);
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	ASObject* pobj = obj.toObject(th->context->root->getSystemState());
	checkDeclaredTraits(pobj);
	//We should skip the special implementation of get
	asAtom o=pobj->getVariableByMultiname(*name, ASObject::SKIP_IMPL);
	name->resetNameIfObject();
	if(o.type == T_INVALID && obj.is<Class_base>())
	{
		// check super classes
		_NR<Class_base> tmpcls = obj.as<Class_base>()->super;
		while (tmpcls && !tmpcls.isNull())
		{
			o=tmpcls->getVariableByMultiname(*name, ASObject::SKIP_IMPL);
			if(o.type != T_INVALID)
				break;
			tmpcls = tmpcls->super;
		}
	}
	if(o.type != T_INVALID && !obj.is<Proxy>())
	{
		callImpl(th, o, obj, args, m, keepReturn);
	}
	else
	{
		//If the object is a Proxy subclass, try to invoke callProperty
		if(obj.is<Proxy>())
		{
			//Check if there is a custom caller defined, skipping implementation to avoid recursive calls
			multiname callPropertyName(NULL);
			callPropertyName.name_type=multiname::NAME_STRING;
			callPropertyName.name_s_id=th->context->root->getSystemState()->getUniqueStringId("callProperty");
			callPropertyName.ns.emplace_back(th->context->root->getSystemState(),flash_proxy,NAMESPACE);
			asAtom oproxy=pobj->getVariableByMultiname(callPropertyName,ASObject::SKIP_IMPL);

			if(oproxy.type != T_INVALID)
			{
				assert_and_throw(oproxy.type==T_FUNCTION);
				if(o.type != T_INVALID)
				{
					callImpl(th, o, obj, args, m,keepReturn);
				}
				else
				{
					//Create a new array
					asAtom* proxyArgs=g_newa(asAtom, m+1);
					ASObject* namearg = abstract_s(th->context->root->getSystemState(),name->normalizedName(th->context->root->getSystemState()));
					namearg->setProxyProperty(*name);
					proxyArgs[0]=asAtom::fromObject(namearg);
					for(int i=0;i<m;i++)
						proxyArgs[i+1]=args[i];
					
					//We now suppress special handling
					LOG_CALL(_("Proxy::callProperty"));
					ASATOM_INCREF(oproxy);
					asAtom ret=oproxy.callFunction(obj,proxyArgs,m+1,true);
					ASATOM_DECREF(oproxy);
					if(keepReturn)
						RUNTIME_STACK_PUSH(th,ret);
					else
					{
						ASATOM_DECREF(ret);
					}
					
					ASATOM_DECREF(obj);
				}
				LOG_CALL(_("End of calling ") << *name);
				return;
			}
			else if(o.type != T_INVALID)
			{
				callImpl(th, o, obj, args, m, keepReturn);
				LOG_CALL(_("End of calling ") << *name);
				return;
			}
		}
		for(int i=0;i<m;i++)
			ASATOM_DECREF(args[i]);
		//LOG(LOG_NOT_IMPLEMENTED,"callProperty: " << name->qualifiedString(th->context->root->getSystemState()) << " not found on " << obj->toDebugString());
		if (pobj->hasPropertyByMultiname(*name,true,true))
		{
			tiny_string clsname = pobj->getClass()->getQualifiedClassName();
			ASATOM_DECREF(obj);
			throwError<ReferenceError>(kWriteOnlyError, name->normalizedName(th->context->root->getSystemState()), clsname);
		}
		if (pobj->getClass() && pobj->getClass()->isSealed)
		{
			tiny_string clsname = pobj->getClass()->getQualifiedClassName();
			ASATOM_DECREF(obj);
			throwError<ReferenceError>(kReadSealedError, name->normalizedName(th->context->root->getSystemState()), clsname);
		}
		if (obj.is<Class_base>())
		{
			tiny_string clsname = obj.as<Class_base>()->class_name.getQualifiedName(th->context->root->getSystemState());
			ASATOM_DECREF(obj);
			throwError<TypeError>(kCallNotFoundError, name->qualifiedString(th->context->root->getSystemState()), clsname);
		}
		else
		{
			tiny_string clsname = pobj->getClassName();
			ASATOM_DECREF(obj);
			throwError<TypeError>(kCallNotFoundError, name->qualifiedString(th->context->root->getSystemState()), clsname);
		}

		ASATOM_DECREF(obj);
		if(keepReturn)
		{
			RUNTIME_STACK_PUSH(th,asAtom::undefinedAtom);
		}

	}
	LOG_CALL(_("End of calling ") << *name);
}

void ABCVm::callMethod(call_context* th, int n, int m)
{
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	LOG_CALL( "callMethod " << n << ' ' << m);

	RUNTIME_STACK_POP_CREATE(th,obj);
	checkDeclaredTraits(obj.toObject(th->context->root->getSystemState()));

	if(obj.is<Null>())
	{
		LOG(LOG_ERROR,"trying to call method on null:"<<n);
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (obj.is<Undefined>())
	{
		LOG(LOG_ERROR,"trying to call method on undefined:"<<n);
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}

	asAtom o=obj.getObject()->getSlot(n);
	if(o.type != T_INVALID)
	{
		ASATOM_INCREF(o);
		callImpl(th, o, obj, args, m, true);
	}
	else
	{
		tiny_string clsname = obj.getObject()->getClassName();
		ASATOM_DECREF(obj);
		throwError<TypeError>(kCallNotFoundError, "", clsname);
	}
	LOG_CALL(_("End of calling method ") << n);
}

void ABCVm::checkDeclaredTraits(ASObject* obj)
{
	if(!obj->isInitialized() &&
			!obj->is<Null>() &&
			!obj->is<Undefined>() &&
			!obj->is<IFunction>() &&
			!obj->is<Function_object>() &&
			!obj->is<Class_base>() &&
			obj->getClass() &&
			(obj->getClass() != Class_object::getClass(obj->getSystemState())))
		obj->getClass()->setupDeclaredTraits(obj);
}

int32_t ABCVm::getProperty_i(ASObject* obj, multiname* name)
{
	LOG_CALL( _("getProperty_i ") << *name );
	checkDeclaredTraits(obj);

	//TODO: implement exception handling to find out if no integer can be returned
	int32_t ret=obj->getVariableByMultiname_i(*name);

	obj->decRef();
	return ret;
}

ASObject* ABCVm::getProperty(ASObject* obj, multiname* name)
{
	LOG_CALL( _("getProperty ") << *name << ' ' << obj->toDebugString() << ' '<<obj->isInitialized());
	checkDeclaredTraits(obj);

		
	asAtom prop=obj->getVariableByMultiname(*name);
	ASObject *ret;

	if(prop.type == T_INVALID)
	{
		if (obj->getClass() && obj->getClass()->isSealed)
			throwError<ReferenceError>(kReadSealedError, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClass()->getQualifiedClassName());
		if (name->isEmpty())
			throwError<ReferenceError>(kReadSealedErrorNs, name->normalizedNameUnresolved(obj->getSystemState()), obj->getClassName());
		if (obj->is<Undefined>())
			throwError<TypeError>(kConvertUndefinedToObjectError);
		if (Log::getLevel() >= LOG_NOT_IMPLEMENTED && (!obj->getClass() || obj->getClass()->isSealed))
			LOG(LOG_NOT_IMPLEMENTED,"getProperty: " << name->normalizedNameUnresolved(obj->getSystemState()) << " not found on " << obj->toDebugString() << " "<<obj->getClassName());
		ret = obj->getSystemState()->getUndefinedRef();
	}
	else
	{
		ret=prop.toObject(obj->getSystemState());
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
	LOG_CALL(_("divide ")  << num1 << '/' << num2);
	return num1/num2;
}

void ABCVm::pushWith(call_context* th)
{
	RUNTIME_STACK_POP_CREATE(th,t);
	LOG_CALL( _("pushWith ") << t.toDebugString() );
	ASATOM_INCREF(t);
	assert_and_throw(th->curr_scope_stack < th->max_scope_stack);
	th->scope_stack[th->curr_scope_stack] = t;
	th->scope_stack_dynamic[th->curr_scope_stack] = true;
	th->curr_scope_stack++;
}

void ABCVm::pushScope(call_context* th)
{
	RUNTIME_STACK_POP_CREATE(th,t);
	LOG_CALL( _("pushScope ") << t.toDebugString() );
	ASATOM_INCREF(t);
	assert_and_throw(th->curr_scope_stack < th->max_scope_stack);
	th->scope_stack[th->curr_scope_stack] = t;
	th->scope_stack_dynamic[th->curr_scope_stack] = false;
	th->curr_scope_stack++;
}

Global* ABCVm::getGlobalScope(call_context* th)
{
	ASObject* ret;
	if (!th->parent_scope_stack.isNull() && th->parent_scope_stack->scope.size() > 0)
		ret =th->parent_scope_stack->scope[0].object.toObject(th->context->root->getSystemState());
	else
	{
		assert_and_throw(th->curr_scope_stack > 0);
		ret =th->scope_stack[0].getObject();
	}
	assert_and_throw(ret->is<Global>());
	LOG_CALL(_("getGlobalScope: ") << ret);
	ret->incRef();
	return ret->as<Global>();
}

number_t ABCVm::decrement(ASObject* o)
{
	LOG_CALL(_("decrement"));

	number_t n=o->toNumber();
	o->decRef();
	return n-1;
}

uint32_t ABCVm::decrement_i(ASObject* o)
{
	LOG_CALL(_("decrement_i"));

	int32_t n=o->toInt();
	o->decRef();
	return n-1;
}

uint64_t ABCVm::decrement_di(ASObject* o)
{
	LOG_CALL(_("decrement_di"));

	int64_t n=o->toInt64();
	o->decRef();
	return n-1;
}

bool ABCVm::ifNLT(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=!(obj1->isLess(obj2)==TTRUE);
	LOG_CALL(_("ifNLT (") << ((ret)?_("taken)"):_("not taken)")));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLT(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TTRUE);
	LOG_CALL(_("ifLT (") << ((ret)?_("taken)"):_("not taken)")));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLT_oi(ASObject* obj2, int32_t val1)
{
	LOG_CALL(_("ifLT_oi"));

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
	LOG_CALL(_("ifLT_io "));

	bool ret=obj1->toInt()<val2;

	obj1->decRef();
	return ret;
}

bool ABCVm::ifNE(ASObject* obj1, ASObject* obj2)
{
	//Real comparision demanded to object
	bool ret=!(obj1->isEqual(obj2));
	LOG_CALL(_("ifNE (") << ((ret)?_("taken)"):_("not taken)")));

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
	LOG_CALL(_("ifNE (") << ((ret)?_("taken)"):_("not taken)")));

	obj1->decRef();
	return ret;
}

int32_t ABCVm::pushByte(intptr_t n)
{
	LOG_CALL( _("pushByte ") << n );
	return n;
}

number_t ABCVm::multiply_oi(ASObject* val2, int32_t val1)
{
	double num1=val1;
	double num2=val2->toNumber();
	val2->decRef();
	LOG_CALL(_("multiply_oi ")  << num1 << '*' << num2);
	return num1*num2;
}

number_t ABCVm::multiply(ASObject* val2, ASObject* val1)
{
	double num1=val1->toNumber();
	double num2=val2->toNumber();
	val1->decRef();
	val2->decRef();
	LOG_CALL(_("multiply ")  << num1 << '*' << num2);
	return num1*num2;
}

int32_t ABCVm::multiply_i(ASObject* val2, ASObject* val1)
{
	int num1=val1->toInt();
	int num2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG_CALL(_("multiply ")  << num1 << '*' << num2);
	return num1*num2;
}

void ABCVm::incLocal(call_context* th, int n)
{
	LOG_CALL( _("incLocal ") << n );
	number_t tmp=th->locals[n].toNumber();
	ASATOM_DECREF(th->locals[n]);
	th->locals[n].setNumber(tmp+1);
}

void ABCVm::incLocal_i(call_context* th, int n)
{
	LOG_CALL( _("incLocal_i ") << n );
	int32_t tmp=th->locals[n].toInt();
	ASATOM_DECREF(th->locals[n]);
	th->locals[n].setInt(tmp+1);
}

void ABCVm::decLocal(call_context* th, int n)
{
	LOG_CALL( _("decLocal ") << n );
	number_t tmp=th->locals[n].toNumber();
	ASATOM_DECREF(th->locals[n]);
	th->locals[n].setNumber(tmp-1);
}

void ABCVm::decLocal_i(call_context* th, int n)
{
	LOG_CALL( _("decLocal_i ") << n );
	int32_t tmp=th->locals[n].toInt();
	ASATOM_DECREF(th->locals[n]);
	th->locals[n].setInt(tmp-1);
}

/* This is called for expressions like
 * function f() { ... }
 * var v = new f();
 */
asAtom ABCVm::constructFunction(call_context* th, asAtom &f, asAtom *args, int argslen)
{
	//See ECMA 13.2.2
	if(f.as<IFunction>()->inClass)
		throwError<TypeError>(kCannotCallMethodAsConstructor, "");

	assert(f.as<IFunction>()->prototype);
	asAtom ret=asAtom::fromObject(new_functionObject(f.as<IFunction>()->prototype));
#ifndef NDEBUG
	ret.getObject()->initialized=false;
#endif
	if (f.is<SyntheticFunction>())
	{
		SyntheticFunction* sf=f.as<SyntheticFunction>();
		if (sf->mi->body && !sf->mi->needsActivation())
		{
			LOG_CALL(_("Building method traits"));
			for(unsigned int i=0;i<sf->mi->body->trait_count;i++)
				th->context->buildTrait(ret.getObject(),&sf->mi->body->traits[i],false);
		}
	}
#ifndef NDEBUG
	ret.getObject()->initialized=true;
#endif

	ASObject* constructor = new_functionObject(f.as<IFunction>()->prototype);
	if (f.as<IFunction>()->prototype->subtype != SUBTYPE_FUNCTIONOBJECT)
	{
		f.as<IFunction>()->prototype->incRef();
		constructor->setVariableByQName("prototype","",f.as<IFunction>()->prototype.getPtr(),DECLARED_TRAIT);
	}
	else
	{
		Class<ASObject>::getRef(f.as<IFunction>()->getSystemState())->prototype->getObj()->incRef();
		constructor->setVariableByQName("prototype","",Class<ASObject>::getRef(f.as<IFunction>()->getSystemState())->prototype->getObj(),DECLARED_TRAIT);
	}
	
	ret.getObject()->setVariableByQName("constructor","",constructor,DECLARED_TRAIT);

	ASATOM_INCREF(f);
	asAtom ret2=f.callFunction(ret,args,argslen,true);
	ASATOM_DECREF(f);

	//ECMA: "return ret2 if it is an object, else ret"
	if(!ret2.isPrimitive())
	{
		ASATOM_DECREF(ret);
		ret = ret2;
	}
	else
	{
		ASATOM_INCREF(ret);
		ASATOM_DECREF(ret2);
	}

	return ret;
}

void ABCVm::construct(call_context* th, int m)
{
	LOG_CALL( _("construct ") << m);
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	RUNTIME_STACK_POP_CREATE(th,obj);

	LOG_CALL(_("Constructing"));

	asAtom ret;
	switch(obj.type)
	{
		case T_CLASS:
		{
			Class_base* o_class=obj.as<Class_base>();
			ret=o_class->getInstance(true,args,m);
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
			ret = constructFunction(th, obj, args, m);
			break;
		}

		default:
		{
			throwError<TypeError>(kConstructOfNonFunctionError);
		}
	}
	if (ret.getObject())
		ret.getObject()->setConstructorCallComplete();
	ASATOM_DECREF(obj);
	LOG_CALL(_("End of constructing ") << ret.toDebugString());
	RUNTIME_STACK_PUSH(th,ret);
}

void ABCVm::constructGenericType(call_context* th, int m)
{
	LOG_CALL( _("constructGenericType ") << m);
	if (m != 1)
		throwError<TypeError>(kWrongTypeArgCountError, "function", "1", Integer::toString(m));
	ASObject** args=g_newa(ASObject*, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP_ASOBJECT(th,args[m-i-1], th->context->root->getSystemState());
	}

	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,obj, th->context->root->getSystemState());

	if(obj->getObjectType() != T_TEMPLATE)
	{
		LOG(LOG_NOT_IMPLEMENTED, "constructGenericType of " << obj->getObjectType());
		obj->decRef();
		obj = th->context->root->getSystemState()->getUndefinedRef();
		RUNTIME_STACK_PUSH(th,asAtom::fromObject(obj));
		for(int i=0;i<m;i++)
			args[i]->decRef();
		return;
	}

	Template_base* o_template=static_cast<Template_base*>(obj);

	/* Instantiate the template to obtain a class */

	std::vector<const Type*> t(m);
	for(int i=0;i<m;++i)
	{
		if(args[i]->is<Class_base>())
			t[i] = args[i]->as<Class_base>();
		else if(args[i]->is<Null>())
			t[i] = Type::anyType;
		else
			throw Class<TypeError>::getInstanceS(obj->getSystemState(),"Wrong type in applytype");
	}

	Class_base* o_class = o_template->applyType(t,th->context->root->applicationDomain);

	// Register the type name in the global scope. The type name
	// is later used by the coerce opcode.
	ASObject* global;
	if (!th->parent_scope_stack.isNull() && th->parent_scope_stack->scope.size() > 0)
		global =th->parent_scope_stack->scope[0].object.toObject(th->context->root->getSystemState());
	else
	{
		assert_and_throw(th->curr_scope_stack > 0);
		global =th->scope_stack[0].getObject();
	}
	QName qname = o_class->class_name;
	if (!global->hasPropertyByMultiname(qname, false, false))
	{
		o_class->incRef();
		global->setVariableByQName(global->getSystemState()->getStringFromUniqueId(qname.nameId),nsNameAndKind(global->getSystemState(),qname.nsStringId,NAMESPACE),o_class,DECLARED_TRAIT);
	}

	for(int i=0;i<m;++i)
		args[i]->decRef();

	RUNTIME_STACK_PUSH(th,asAtom::fromObject(o_class));
}

ASObject* ABCVm::typeOf(ASObject* obj)
{
	LOG_CALL(_("typeOf"));
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
	ASObject* res = abstract_s(obj->getSystemState(),ret);
	obj->decRef();
	return res;
}

void ABCVm::jump(int offset)
{
	LOG_CALL(_("jump ") << offset);
}

bool ABCVm::ifTrue(ASObject* obj1)
{
	bool ret=Boolean_concrete(obj1);
	LOG_CALL(_("ifTrue (") << ((ret)?_("taken)"):_("not taken)")));

	obj1->decRef();
	return ret;
}

number_t ABCVm::modulo(ASObject* val1, ASObject* val2)
{
	number_t num1=val1->toNumber();
	number_t num2=val2->toNumber();

	val1->decRef();
	val2->decRef();
	LOG_CALL(_("modulo ")  << num1 << '%' << num2);
	/* fmod returns NaN if num2 == 0 as the spec mandates */
	return ::fmod(num1,num2);
}

number_t ABCVm::subtract_oi(ASObject* val2, int32_t val1)
{
	int num2=val2->toInt();
	int num1=val1;

	val2->decRef();
	LOG_CALL(_("subtract_oi ") << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_do(number_t val2, ASObject* val1)
{
	number_t num2=val2;
	number_t num1=val1->toNumber();

	val1->decRef();
	LOG_CALL(_("subtract_do ") << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_io(int32_t val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(LOG_NOT_IMPLEMENTED,_("subtract: HACK"));
		return 0;
	}
	int num2=val2;
	int num1=val1->toInt();

	val1->decRef();
	LOG_CALL(_("subtract_io ") << dec << num1 << '-' << num2);
	return num1-num2;
}

int32_t ABCVm::subtract_i(ASObject* val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED ||
		val2->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(LOG_NOT_IMPLEMENTED,_("subtract_i: HACK"));
		return 0;
	}
	int num2=val2->toInt();
	int num1=val1->toInt();

	val1->decRef();
	val2->decRef();
	LOG_CALL(_("subtract_i ") << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract(ASObject* val2, ASObject* val1)
{
	number_t num2=val2->toNumber();
	number_t num1=val1->toNumber();

	val1->decRef();
	val2->decRef();
	LOG_CALL(_("subtract ") << num1 << '-' << num2);
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
	LOG_CALL( _("kill ") << n );
}

ASObject* ABCVm::add(ASObject* val2, ASObject* val1)
{
	//Implement ECMA add algorithm, for XML and default (see avm2overview)
	
	ASObject* res = NULL;
	// if both values are Integers or int Numbers the result is also an int Number
	if( (val1->is<Integer>() || val1->is<UInteger>() || (val1->is<Number>() && !val1->as<Number>()->isfloat)) &&
		(val2->is<Integer>() || val2->is<UInteger>() || (val2->is<Number>() && !val2->as<Number>()->isfloat)))
	{
		int64_t num1=val1->toInt64();
		int64_t num2=val2->toInt64();
		LOG_CALL("addI " << num1 << '+' << num2);
		res = abstract_di(val1->getSystemState(), num1+num2);
		val1->decRef();
		val2->decRef();
		return res;
	}
	else if(val1->is<Number>() && val2->is<Number>())
	{
		double num1=val1->as<Number>()->toNumber();
		double num2=val2->as<Number>()->toNumber();
		LOG_CALL("addN " << num1 << '+' << num2);
		res = abstract_d(val1->getSystemState(), num1+num2);
		val1->decRef();
		val2->decRef();
		return res;
	}
	else if(val1->is<ASString>() || val2->is<ASString>())
	{
		tiny_string a = val1->toString();
		tiny_string b = val2->toString();
		LOG_CALL("add " << a << '+' << b);
		res = abstract_s(val1->getSystemState(),a + b);
		val1->decRef();
		val2->decRef();
		return res;
	}
	else if( (val1->is<XML>() || val1->is<XMLList>()) && (val2->is<XML>() || val2->is<XMLList>()) )
	{
		//Check if the objects are both XML or XMLLists
		Class_base* xmlClass=Class<XML>::getClass(val1->getSystemState());

		XMLList* newList=Class<XMLList>::getInstanceS(val1->getSystemState(),true);
		if(val1->getClass()==xmlClass)
			newList->append(_MR(static_cast<XML*>(val1)));
		else //if(val1->getClass()==xmlListClass)
			newList->append(_MR(static_cast<XMLList*>(val1)));

		if(val2->getClass()==xmlClass)
			newList->append(_MR(static_cast<XML*>(val2)));
		else //if(val2->getClass()==xmlListClass)
			newList->append(_MR(static_cast<XMLList*>(val2)));

		//The references of val1 and val2 have been passed to the smart references
		//no decRef is needed
		return newList;
	}
	else
	{//If none of the above apply, convert both to primitives with no hint
		_R<ASObject> val1p = val1->toPrimitive(NO_HINT);
		_R<ASObject> val2p = val2->toPrimitive(NO_HINT);
		if(val1p->is<ASString>() || val2p->is<ASString>())
		{//If one is String, convert both to strings and concat
			string a(val1p->toString().raw_buf());
			string b(val2p->toString().raw_buf());
			LOG_CALL("add " << a << '+' << b);
			res = abstract_s(val1->getSystemState(),a+b);
			val1->decRef();
			val2->decRef();
			return res;
		}
		else
		{//Convert both to numbers and add
			number_t num1=val1p->toNumber();
			number_t num2=val2p->toNumber();
			LOG_CALL("addN " << num1 << '+' << num2);
			number_t result = num1 + num2;
			res = abstract_d(val1->getSystemState(),result);
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
		LOG(LOG_NOT_IMPLEMENTED,_("add_i: HACK"));
		return 0;
	}
	int32_t num2=val2->toInt();
	int32_t num1=val1->toInt();

	val1->decRef();
	val2->decRef();
	LOG_CALL(_("add_i ") << num1 << '+' << num2);
	return num1+num2;
}

ASObject* ABCVm::add_oi(ASObject* val2, int32_t val1)
{
	ASObject* res =NULL;
	//Implement ECMA add algorithm, for XML and default
	if(val2->getObjectType()==T_INTEGER)
	{
		Integer* ip=static_cast<Integer*>(val2);
		int32_t num2=ip->val;
		int32_t num1=val1;
		res = abstract_i(val2->getSystemState(),num1+num2);
		val2->decRef();
		LOG_CALL(_("add ") << num1 << '+' << num2);
		return res;
	}
	else if(val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		res = abstract_d(val2->getSystemState(),num1+num2);
		val2->decRef();
		LOG_CALL(_("add ") << num1 << '+' << num2);
		return res;
	}
	else if(val2->getObjectType()==T_STRING)
	{
		//Convert argument to int32_t
		tiny_string a = Integer::toString(val1);
		const tiny_string& b=val2->toString();
		res = abstract_s(val2->getSystemState(), a+b);
		val2->decRef();
		LOG_CALL(_("add ") << a << '+' << b);
		return res;
	}
	else
	{
		return add(val2,abstract_i(val2->getSystemState(),val1));
	}

}

ASObject* ABCVm::add_od(ASObject* val2, number_t val1)
{
	ASObject* res = NULL;
	//Implement ECMA add algorithm, for XML and default
	if(val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		res = abstract_d(val2->getSystemState(),num1+num2);
		val2->decRef();
		LOG_CALL(_("add ") << num1 << '+' << num2);
		return res;
	}
	else if(val2->getObjectType()==T_INTEGER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		res = abstract_d(val2->getSystemState(),num1+num2);
		val2->decRef();
		LOG_CALL(_("add ") << num1 << '+' << num2);
		return res;
	}
	else if(val2->getObjectType()==T_STRING)
	{
		tiny_string a = Number::toString(val1);
		const tiny_string& b=val2->toString();
		res = abstract_s(val2->getSystemState(),a+b);
		val2->decRef();
		LOG_CALL(_("add ") << a << '+' << b);
		return res;
	}
	else
	{
		return add(val2,abstract_d(val2->getSystemState(),val1));
	}

}

int32_t ABCVm::lShift(ASObject* val1, ASObject* val2)
{
	int32_t i2=val2->toInt();
	uint32_t i1=val1->toUInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG_CALL(_("lShift ")<<hex<<i2<<_("<<")<<i1<<dec);
	//Left shift are supposed to always work in 32bit
	int32_t ret=i2<<i1;
	return ret;
}

int32_t ABCVm::lShift_io(uint32_t val1, ASObject* val2)
{
	int32_t i2=val2->toInt();
	uint32_t i1=val1&0x1f;
	val2->decRef();
	LOG_CALL(_("lShift ")<<hex<<i2<<_("<<")<<i1<<dec);
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
	LOG_CALL(_("rShift ")<<hex<<i2<<_(">>")<<i1<<dec);
	return i2>>i1;
}

uint32_t ABCVm::urShift(ASObject* val1, ASObject* val2)
{
	uint32_t i2=val2->toUInt();
	uint32_t i1=val1->toUInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG_CALL(_("urShift ")<<hex<<i2<<_(">>")<<i1<<dec);
	return i2>>i1;
}

uint32_t ABCVm::urShift_io(uint32_t val1, ASObject* val2)
{
	uint32_t i2=val2->toUInt();
	uint32_t i1=val1&0x1f;
	val2->decRef();
	LOG_CALL(_("urShift ")<<hex<<i2<<_(">>")<<i1<<dec);
	return i2>>i1;
}

bool ABCVm::_not(ASObject* v)
{
	LOG_CALL( _("not") );
	bool ret=!Boolean_concrete(v);
	v->decRef();
	return ret;
}

bool ABCVm::equals(ASObject* val2, ASObject* val1)
{
	bool ret=val1->isEqual(val2);
	LOG_CALL( _("equals ") << ret);
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
	LOG_CALL( _("strictEquals") );
	bool ret=strictEqualImpl(obj1, obj2);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::dup()
{
	LOG_CALL( _("dup: DONE") );
}

bool ABCVm::pushTrue()
{
	LOG_CALL( _("pushTrue") );
	return true;
}

bool ABCVm::pushFalse()
{
	LOG_CALL( _("pushFalse") );
	return false;
}

ASObject* ABCVm::pushNaN()
{
	LOG_CALL( _("pushNaN") );
	return abstract_d(getSys(),Number::NaN);
}

bool ABCVm::ifGT(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TTRUE);
	LOG_CALL(_("ifGT (") << ((ret)?_("taken)"):_("not taken)")));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifNGT(ASObject* obj2, ASObject* obj1)
{

	//Real comparision demanded to object
	bool ret=!(obj2->isLess(obj1)==TTRUE);
	LOG_CALL(_("ifNGT (") << ((ret)?_("taken)"):_("not taken)")));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TFALSE);
	LOG_CALL(_("ifLE (") << ((ret)?_("taken)"):_("not taken)")));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifNLE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=!(obj2->isLess(obj1)==TFALSE);
	LOG_CALL(_("ifNLE (") << ((ret)?_("taken)"):_("not taken)")));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifGE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TFALSE);
	LOG_CALL(_("ifGE (") << ((ret)?_("taken)"):_("not taken)")));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifNGE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=!(obj1->isLess(obj2)==TFALSE);
	LOG_CALL(_("ifNGE (") << ((ret)?_("taken)"):_("not taken)")));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::_throw(call_context* th)
{
	LOG_CALL(_("throw"));
	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,exc, th->context->root->getSystemState());
	throw exc;
}

void ABCVm::setSuper(call_context* th, int n)
{
	RUNTIME_STACK_POP_CREATE(th,value);
	multiname* name=th->context->getMultiname(n,th);
	LOG_CALL(_("setSuper ") << *name);

	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,obj, th->context->root->getSystemState());

	assert_and_throw(th->inClass)
	assert_and_throw(th->inClass->super);
	assert_and_throw(obj->getClass());
	assert_and_throw(obj->getClass()->isSubClass(th->inClass));

	obj->setVariableByMultiname(*name,value,ASObject::CONST_NOT_ALLOWED,th->inClass->super.getPtr());
	name->resetNameIfObject();
	obj->decRef();
}

void ABCVm::getSuper(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG_CALL(_("getSuper ") << *name);

	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,obj, th->context->root->getSystemState());

	if(obj->is<Null>())
	{
		LOG(LOG_ERROR,"calling getSuper on null:" << *name << ' ' << obj->toDebugString());
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (obj->is<Undefined>())
	{
		LOG(LOG_ERROR,"calling getSuper on undefined:" << *name << ' ' << obj->toDebugString());
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}

	Class_base* cls = NULL;
	if (th->inClass && !th->inClass->super.isNull())
		cls = th->inClass->super.getPtr();
	else if (obj->getClass() && !obj->getClass()->super.isNull())
		cls = obj->getClass()->super.getPtr();
	assert_and_throw(cls);

	asAtom ret = obj->getVariableByMultiname(*name,ASObject::NONE,cls);
	if (ret.type == T_INVALID)
		throwError<ReferenceError>(kCallOfNonFunctionError,name->normalizedNameUnresolved(obj->getSystemState()));

	name->resetNameIfObject();

	obj->decRef();
	RUNTIME_STACK_PUSH(th,ret);
}

bool ABCVm::getLex(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,NULL);
	//getlex is specified not to allow runtime multinames
	assert_and_throw(name->isStatic);
	LOG_CALL( "getLex: " << *name );
	vector<scope_entry>::reverse_iterator it;
	// o will be a reference owned by this function (or NULL). At
	// the end the reference will be handed over to the runtime
	// stack.
	asAtom o;

	bool canCache = true;
	
	//Find out the current 'this', when looking up over it, we have to consider all of it
	for(uint32_t i = th->curr_scope_stack; i > 0; i--)
	{
		ASObject* s = th->scope_stack[i-1].toObject(th->context->root->getSystemState());
		// XML_STRICT flag tells getVariableByMultiname to
		// ignore non-existing properties in XML obejcts
		// (normally it would return an empty XMLList if the
		// property does not exist).
		ASObject::GET_VARIABLE_OPTION opt=ASObject::XML_STRICT;
		if(!th->scope_stack_dynamic[i-1])
			opt=(ASObject::GET_VARIABLE_OPTION)(opt | ASObject::SKIP_IMPL);
		else
			canCache = false;

		checkDeclaredTraits(s);
		asAtom prop=s->getVariableByMultiname(*name, opt);
		if(prop.type != T_INVALID)
		{
			ASATOM_INCREF(prop);
			o=prop;
			break;
		}
	}
	if(o.type == T_INVALID && !th->parent_scope_stack.isNull()) // check parent scope stack
	{
		for(it=th->parent_scope_stack->scope.rbegin();it!=th->parent_scope_stack->scope.rend();++it)
		{
			// XML_STRICT flag tells getVariableByMultiname to
			// ignore non-existing properties in XML obejcts
			// (normally it would return an empty XMLList if the
			// property does not exist).
			ASObject::GET_VARIABLE_OPTION opt=ASObject::XML_STRICT;
			if(!it->considerDynamic)
				opt=(ASObject::GET_VARIABLE_OPTION)(opt | ASObject::SKIP_IMPL);
			else
				canCache = false;
	
			checkDeclaredTraits(it->object.toObject(th->context->root->getSystemState()));
			asAtom prop=it->object.toObject(th->context->root->getSystemState())->getVariableByMultiname(*name, opt);
			if(prop.type != T_INVALID)
			{
				ASATOM_INCREF(prop);
				o=prop;
				break;
			}
		}
	}

	if(o.type == T_INVALID)
	{
		ASObject* target;
		o=asAtom::fromObject(getCurrentApplicationDomain(th)->getVariableAndTargetByMultiname(*name, target));
		if(o.type == T_INVALID)
		{
			LOG(LOG_NOT_IMPLEMENTED,"getLex: " << *name<< " not found");
			throwError<ReferenceError>(kUndefinedVarError,name->normalizedNameUnresolved(th->context->root->getSystemState()));
			RUNTIME_STACK_PUSH(th,asAtom::fromObject(th->context->root->getSystemState()->getUndefinedRef()));
			name->resetNameIfObject();
			return false;
		}
		ASATOM_INCREF(o);
	}
	else
		// TODO can we cache objects found in the scope_stack? 
		canCache = false;

	name->resetNameIfObject();
	RUNTIME_STACK_PUSH(th,o);
	return canCache;
}

void ABCVm::constructSuper(call_context* th, int m)
{
	LOG_CALL( _("constructSuper ") << m);
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	RUNTIME_STACK_POP_CREATE(th,obj);

	assert_and_throw(th->inClass);
	assert_and_throw(th->inClass->super);
	assert_and_throw(obj.getObject()->getClass());
	assert_and_throw(obj.getObject()->getClass()->isSubClass(th->inClass));
	LOG_CALL(_("Super prototype name ") << th->inClass->super->class_name);

	th->inClass->super->handleConstruction(obj,args, m, false);
	LOG_CALL(_("End super construct "));
}

ASObject* ABCVm::findProperty(call_context* th, multiname* name)
{
	LOG_CALL( _("findProperty ") << *name );

	vector<scope_entry>::reverse_iterator it;
	bool found=false;
	ASObject* ret=NULL;
	for(uint32_t i = th->curr_scope_stack; i > 0; i--)
	{
		found=th->scope_stack[i-1].toObject(th->context->root->getSystemState())->hasPropertyByMultiname(*name, th->scope_stack_dynamic[i-1], true);

		if(found)
		{
			//We have to return the object, not the property
			ret=th->scope_stack[i-1].toObject(th->context->root->getSystemState());
			break;
		}
	}
	if(!found && !th->parent_scope_stack.isNull()) // check parent scope stack
	{
		for(it=th->parent_scope_stack->scope.rbegin();it!=th->parent_scope_stack->scope.rend();++it)
		{
			found=it->object.toObject(th->context->root->getSystemState())->hasPropertyByMultiname(*name, it->considerDynamic, true);
	
			if(found)
			{
				//We have to return the object, not the property
				ret=it->object.toObject(th->context->root->getSystemState());
				break;
			}
		}
	}
	if(!found)
	{
		//try to find a global object where this is defined
		ASObject* target;
		ASObject* o=getCurrentApplicationDomain(th)->getVariableAndTargetByMultiname(*name, target);
		if(o)
			ret=target;
		else //else push the current global object
		{
			if (!th->parent_scope_stack.isNull() && th->parent_scope_stack->scope.size() > 0)
				ret =th->parent_scope_stack->scope[0].object.toObject(th->context->root->getSystemState());
			else
			{
				assert_and_throw(th->curr_scope_stack > 0);
				ret =th->scope_stack[0].toObject(th->context->root->getSystemState());
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
	ASObject* ret=NULL;

	for(uint32_t i = th->curr_scope_stack; i > 0; i--)
	{
		found=th->scope_stack[i-1].toObject(th->context->root->getSystemState())->hasPropertyByMultiname(*name, th->scope_stack_dynamic[i-1], true);
		if(found)
		{
			//We have to return the object, not the property
			ret=th->scope_stack[i-1].toObject(th->context->root->getSystemState());
			break;
		}
	}
	if(!found && !th->parent_scope_stack.isNull()) // check parent scope stack
	{
		for(it =th->parent_scope_stack->scope.rbegin();it!=th->parent_scope_stack->scope.rend();++it)
		{
			found=it->object.toObject(th->context->root->getSystemState())->hasPropertyByMultiname(*name, it->considerDynamic, true);
			if(found)
			{
				//We have to return the object, not the property
				ret=it->object.toObject(th->context->root->getSystemState());
				break;
			}
		}
	}
	if(!found)
	{
		ASObject* target;
		ASObject* o=getCurrentApplicationDomain(th)->getVariableAndTargetByMultiname(*name, target);
		if(o)
			ret=target;
		else
		{
			LOG(LOG_NOT_IMPLEMENTED,"findPropStrict: " << *name << " not found");
			for(uint32_t i = th->curr_scope_stack; i > 0; i--)
			{
				ASObject* r = th->scope_stack[i-1].toObject(th->context->root->getSystemState());
				if (!r->is<Class_base>())
					continue;
				if (r->as<Class_base>()->findBorrowedGettable(*name))
				{
					if (!r->as<Class_base>()->isSealed)
						break;
					throwError<TypeError>(kCallOfNonFunctionError,name->normalizedNameUnresolved(th->context->root->getSystemState()));
				}
			}
			throwError<ReferenceError>(kUndefinedVarError,name->normalizedNameUnresolved(th->context->root->getSystemState()));
			return th->context->root->getSystemState()->getUndefinedRef();
		}
	}

	assert_and_throw(ret);
	ret->incRef();
	return ret;
}

asAtom ABCVm::findPropStrictCache(call_context* th, memorystream& code)
{

	lightspark::method_body_info_cache* cachepos = code.tellcachepos();
	if (cachepos->type == method_body_info_cache::CACHE_TYPE_OBJECT)
	{
		code.seekcachepos(cachepos->nextcachepos);
		cachepos->obj->incRef();
		if (cachepos->obj->is<IFunction>())
			return asAtom::fromFunction(cachepos->obj,cachepos->closure);
		else
			return asAtom::fromObject(cachepos->obj);
	}
	uint32_t t = code.readu30();
	multiname* name=th->context->getMultiname(t,th);
	LOG_CALL( "findPropStrict " << *name );

	vector<scope_entry>::reverse_iterator it;
	bool hasdynamic=false;
	bool found=false;
	asAtom ret;

	for(uint32_t i = th->curr_scope_stack; i > 0; i--)
	{
		found=th->scope_stack[i-1].toObject(th->context->root->getSystemState())->hasPropertyByMultiname(*name, th->scope_stack_dynamic[i-1], true);
		if (th->scope_stack_dynamic[i-1])
			hasdynamic = true;
		if(found)
		{
			//We have to return the object, not the property
			ret=th->scope_stack[i-1];
			break;
		}
	}
	if(!found && !th->parent_scope_stack.isNull()) // check parent scope stack
	{
		for(it =th->parent_scope_stack->scope.rbegin();it!=th->parent_scope_stack->scope.rend();++it)
		{
			found=it->object.toObject(th->context->root->getSystemState())->hasPropertyByMultiname(*name, it->considerDynamic, true);
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
		ASObject* o=getCurrentApplicationDomain(th)->getVariableAndTargetByMultiname(*name, target);
		if(o)
		{
			ret=asAtom::fromObject(target);
		}
		else
		{
			LOG(LOG_NOT_IMPLEMENTED,"findPropStrict: " << *name << " not found");
			for(uint32_t i = th->curr_scope_stack; i > 0; i--)
			{
				ASObject* r = th->scope_stack[i-1].toObject(th->context->root->getSystemState());
				if (!r->is<Class_base>())
					continue;
				if (r->as<Class_base>()->findBorrowedGettable(*name))
				{
					if (!r->as<Class_base>()->isSealed)
						break;
					throwError<TypeError>(kCallOfNonFunctionError,name->normalizedNameUnresolved(th->context->root->getSystemState()));
				}
			}
			throwError<ReferenceError>(kUndefinedVarError,name->normalizedNameUnresolved(th->context->root->getSystemState()));
			return th->context->root->getSystemState()->getUndefinedRef();
		}
		// we only cache the property if
		// - the property was not found in the scope_stack and
		// - the scope_stack does not contain any dynamic objects and
		// - the property was found in one of the global scopes
		if (!hasdynamic)
		{
			// put object in cache
			cachepos->type =method_body_info_cache::CACHE_TYPE_OBJECT;
			cachepos->obj = ret.toObject(th->context->root->getSystemState());
			cachepos->closure =ret.getClosure();
			ASATOM_INCREF(ret);
		}
	}
	name->resetNameIfObject();

	assert_and_throw(ret.type != T_INVALID);
	ASATOM_INCREF(ret);
	return ret;
}

bool ABCVm::greaterThan(ASObject* obj1, ASObject* obj2)
{
	LOG_CALL(_("greaterThan"));

	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TTRUE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::greaterEquals(ASObject* obj1, ASObject* obj2)
{
	LOG_CALL(_("greaterEquals"));

	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TFALSE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::lessEquals(ASObject* obj1, ASObject* obj2)
{
	LOG_CALL(_("lessEquals"));

	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TFALSE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::initProperty(ASObject* obj, ASObject* value, multiname* name)
{
	checkDeclaredTraits(obj);

	//Allow to set contant traits
	asAtom v = asAtom::fromObject(value);
	obj->setVariableByMultiname(*name,v,ASObject::CONST_ALLOWED);

	obj->decRef();
}

void ABCVm::callStatic(call_context* th, int n, int m, method_info** called_mi, bool keepReturn)
{
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	RUNTIME_STACK_POP_CREATE(th,obj);
	if(obj.type == T_NULL)
	{
		LOG(LOG_ERROR,"trying to callStatic on null");
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (obj.type == T_UNDEFINED)
	{
		LOG(LOG_ERROR,"trying to callStatic on undefined");
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}
	method_info* mi = th->context->get_method(n);
	assert_and_throw(mi);
	SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(th->context->root->getSystemState(),mi);

	if(f)
	{
		asAtom v = asAtom::fromObject(f);
		callImpl(th, v, obj, args, m, keepReturn);
	}
	else
	{
		tiny_string clsname = obj.toObject(th->context->root->getSystemState())->getClassName();
		ASATOM_DECREF(obj);
		for(int i=0;i<m;i++)
			ASATOM_DECREF(args[i]);
		throwError<ReferenceError>(kCallNotFoundError, "?", clsname);
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

	multiname* name=th->context->getMultiname(n,th);
	LOG_CALL((keepReturn ? "callSuper " : "callSuperVoid ") << *name << ' ' << m);

	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,obj, th->context->root->getSystemState());
	if(obj->is<Null>())
	{
		LOG(LOG_ERROR,"trying to call super on null:"<<*name);
		throwError<TypeError>(kConvertNullToObjectError);
	}
	if (obj->is<Undefined>())
	{
		LOG(LOG_ERROR,"trying to call super on undefined:"<<*name);
		throwError<TypeError>(kConvertUndefinedToObjectError);
	}

	assert_and_throw(th->inClass);
	assert_and_throw(th->inClass->super);
	assert_and_throw(obj->getClass());
	assert_and_throw(obj->getClass()->isSubClass(th->inClass));
	asAtom f = obj->getVariableByMultiname(*name, ASObject::SKIP_IMPL,th->inClass->super.getPtr());
	name->resetNameIfObject();
	if(f.type != T_INVALID)
	{
		asAtom v = asAtom::fromObject(obj);
		callImpl(th, f, v, args, m, keepReturn);
	}
	else
	{
		tiny_string clsname = obj->getClassName();
		obj->decRef();
		for(int i=0;i<m;i++)
			ASATOM_DECREF(args[i]);
		//LOG(LOG_ERROR,_("Calling an undefined function ") << th->context->root->getSystemState()->getStringFromUniqueId(name->name_s_id));
		throwError<ReferenceError>(kCallNotFoundError, name->qualifiedString(th->context->root->getSystemState()), clsname);
	}
	LOG_CALL(_("End of callSuper ") << *name);
}

bool ABCVm::isType(ABCContext* context, ASObject* obj, multiname* name)
{
	bool ret = context->isinstance(obj, name);
	obj->decRef();
	return ret;
}

bool ABCVm::isTypelate(ASObject* type, ASObject* obj)
{
	LOG_CALL(_("isTypelate"));
	bool real_ret=false;

	Class_base* objc=NULL;
	Class_base* c=NULL;
	switch (type->getObjectType())
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_OBJECT:
		case T_STRING:
			LOG(LOG_ERROR,"trying to call isTypelate on object:"<<obj->toDebugString());
			throwError<TypeError>(kIsTypeMustBeClassError);
			break;
		case T_NULL:
			LOG(LOG_ERROR,"trying to call isTypelate on null:"<<obj->toDebugString());
			throwError<TypeError>(kConvertNullToObjectError);
			break;
		case T_UNDEFINED:
			LOG(LOG_ERROR,"trying to call isTypelate on undefined:"<<obj->toDebugString());
			throwError<TypeError>(kConvertUndefinedToObjectError);
			break;
		case T_CLASS:
			break;
		default:
			throwError<TypeError>(kIsTypeMustBeClassError);
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
		LOG_CALL(_("Numeric type is ") << ((real_ret)?"":_("not ")) << _("subclass of ") << c->class_name);
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
		LOG_CALL(_("isTypelate on non classed object ") << real_ret);
		obj->decRef();
		type->decRef();
		return real_ret;
	}

	real_ret=objc->isSubClass(c);
	LOG_CALL(_("Type ") << objc->class_name << _(" is ") << ((real_ret)?"":_("not ")) 
			<< "subclass of " << c->class_name);
	obj->decRef();
	type->decRef();
	return real_ret;
}

ASObject* ABCVm::asType(ABCContext* context, ASObject* obj, multiname* name)
{
	bool ret = context->isinstance(obj, name);
	LOG_CALL(_("asType"));
	
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
	LOG_CALL(_("asTypelate"));

	if(!type->is<Class_base>())
	{
		LOG(LOG_ERROR,"trying to call asTypelate on non class object:"<<obj->toDebugString());
		throwError<TypeError>(kConvertNullToObjectError);
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
		LOG_CALL(_("Numeric type is ") << ((real_ret)?"":_("not ")) << _("subclass of ") << c->class_name);
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
	LOG_CALL(_("Type ") << objc->class_name << _(" is ") << ((real_ret)?_(" "):_("not ")) 
			<< _("subclass of ") << c->class_name);
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
	LOG_CALL(_("ifEq (") << ((ret)?_("taken)"):_("not taken)")));

	//Real comparision demanded to object
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifStrictEq(ASObject* obj2, ASObject* obj1)
{
	bool ret=strictEqualImpl(obj1,obj2);
	LOG_CALL(_("ifStrictEq ")<<ret);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifStrictNE(ASObject* obj2, ASObject* obj1)
{
	bool ret=!strictEqualImpl(obj1,obj2);
	LOG_CALL(_("ifStrictNE ")<<ret);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::in(ASObject* val2, ASObject* val1)
{
	LOG_CALL( _("in") );
	if(val2->is<Null>())
		throwError<TypeError>(kConvertNullToObjectError);

	multiname name(NULL);
	name.name_type=multiname::NAME_OBJECT;
	//Acquire the reference
	name.name_o=val1;
	name.ns.emplace_back(val2->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	bool ret=val2->hasPropertyByMultiname(name, true, true);
	name.name_o=NULL;
	val1->decRef();
	val2->decRef();
	return ret;
}

bool ABCVm::ifFalse(ASObject* obj1)
{
	bool ret=!Boolean_concrete(obj1);
	LOG_CALL(_("ifFalse (") << ((ret)?_("taken"):_("not taken")) << ')');

	obj1->decRef();
	return ret;
}

void ABCVm::constructProp(call_context* th, int n, int m)
{
	asAtom* args=g_newa(asAtom, m);
	for(int i=0;i<m;i++)
	{
		RUNTIME_STACK_POP(th,args[m-i-1]);
	}

	multiname* name=th->context->getMultiname(n,th);

	LOG_CALL(_("constructProp ")<< *name << ' ' << m);

	RUNTIME_STACK_POP_CREATE(th,obj);

	checkDeclaredTraits(obj.toObject(th->context->root->getSystemState()));
	asAtom o=obj.toObject(th->context->root->getSystemState())->getVariableByMultiname(*name);

	if(o.type == T_INVALID)
	{
		for(int i=0;i<m;++i)
			ASATOM_DECREF(args[i]);
		if (obj.is<Undefined>())
		{
			ASATOM_DECREF(obj);
			throwError<TypeError>(kConvertUndefinedToObjectError);
		}
		if (obj.isPrimitive())
		{
			ASATOM_DECREF(obj);
			throwError<TypeError>(kConstructOfNonFunctionError);
		}
		throwError<ReferenceError>(kUndefinedVarError, name->normalizedNameUnresolved(th->context->root->getSystemState()));
	}

	name->resetNameIfObject();

	LOG_CALL(_("Constructing"));
	asAtom ret;
	try
	{
		if(o.type==T_CLASS)
		{
			Class_base* o_class=o.as<Class_base>();
			ret=o_class->getInstance(true,args,m);
		}
		else if(o.type==T_FUNCTION)
		{
			ret = constructFunction(th, o, args, m);
		}
		else
			throwError<TypeError>(kConstructOfNonFunctionError);
	}
	catch(ASObject* exc)
	{
		LOG_CALL(_("Exception during object construction. Returning Undefined"));
		//Handle eventual exceptions from the constructor, to fix the stack
		RUNTIME_STACK_PUSH(th,obj);
		throw;
	}
	RUNTIME_STACK_PUSH(th,ret);
	if (ret.getObject())
		ret.getObject()->setConstructorCallComplete();

	ASATOM_DECREF(obj);
	LOG_CALL(_("End of constructing ") << ret.toDebugString());
}

bool ABCVm::hasNext2(call_context* th, int n, int m)
{
	LOG_CALL("hasNext2 " << n << ' ' << m);
	ASObject* obj=th->locals[n].getObject();
	//If the local is not assigned or is a primitive bail out
	if(obj==NULL)
		return false;

	uint32_t curIndex=th->locals[m].toUInt();

	uint32_t newIndex=obj->nextNameIndex(curIndex);
	ASATOM_DECREF(th->locals[m]);
	th->locals[m].setUInt(newIndex);
	if(newIndex==0)
	{
		ASATOM_DECREF(th->locals[n]);
		th->locals[n]=asAtom::nullAtom;
		return false;
	}
	return true;
}

void ABCVm::newObject(call_context* th, int n)
{
	LOG_CALL(_("newObject ") << n);
	ASObject* ret=Class<ASObject>::getInstanceS(th->context->root->getSystemState());
	//Duplicated keys overwrite the previous value
	multiname propertyName(NULL);
	propertyName.name_type=multiname::NAME_STRING;
	for(int i=0;i<n;i++)
	{
		RUNTIME_STACK_POP_CREATE(th,value);
		RUNTIME_STACK_POP_CREATE(th,name);
		propertyName.name_s_id=name.toStringId(th->context->root->getSystemState());
		ASATOM_DECREF(name);
		ret->setVariableByMultiname(propertyName, value, ASObject::CONST_NOT_ALLOWED);
	}

	RUNTIME_STACK_PUSH(th,asAtom::fromObject(ret));
}

void ABCVm::getDescendants(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,obj, th->context->root->getSystemState());
	LOG_CALL("getDescendants " << *name << " " <<name->isAttribute<< " "<<obj->getClassName());
	XML::XMLVector ret;
	XMLList* targetobject = NULL;
	if(obj->getClass()==Class<XML>::getClass(obj->getSystemState()))
	{
		XML* xmlObj=Class<XML>::cast(obj);
		targetobject = xmlObj->getChildrenlist();
		uint32_t ns_uri = BUILTIN_STRINGS::EMPTY;
		if (name->ns.size() > 0)
		{
			ns_uri = name->ns[0].nsNameId;
			if (ns_uri == BUILTIN_STRINGS::EMPTY && name->ns.size() == 1)
				ns_uri=BUILTIN_STRINGS::STRING_WILDCARD;
		}
		xmlObj->getDescendantsByQName(name->normalizedName(th->context->root->getSystemState()), ns_uri,name->isAttribute, ret);
	}
	else if(obj->getClass()==Class<XMLList>::getClass(th->context->root->getSystemState()))
	{
		XMLList* xmlObj=Class<XMLList>::cast(obj);
		uint32_t ns_uri = BUILTIN_STRINGS::EMPTY;
		if (name->ns.size() > 0)
		{
			ns_uri = name->ns[0].nsNameId;
			if (ns_uri == BUILTIN_STRINGS::EMPTY && name->ns.size() == 1)
				ns_uri=BUILTIN_STRINGS::STRING_WILDCARD;
		}
		targetobject = xmlObj;
		xmlObj->getDescendantsByQName(name->normalizedName(th->context->root->getSystemState()), ns_uri,name->isAttribute, ret);
	}
	else if(obj->is<Proxy>())
	{
		multiname callPropertyName(NULL);
		callPropertyName.name_type=multiname::NAME_STRING;
		callPropertyName.name_s_id=obj->getSystemState()->getUniqueStringId("getDescendants");
		callPropertyName.ns.emplace_back(th->context->root->getSystemState(),flash_proxy,NAMESPACE);
		asAtom o=obj->getVariableByMultiname(callPropertyName,ASObject::SKIP_IMPL);
		
		if(o.type != T_INVALID)
		{
			assert_and_throw(o.type==T_FUNCTION);
			
			//Create a new array
			asAtom* proxyArgs=g_newa(asAtom, 1);
			ASObject* namearg = abstract_s(obj->getSystemState(), name->normalizedName(th->context->root->getSystemState()));
			namearg->setProxyProperty(*name);
			proxyArgs[0]=asAtom::fromObject(namearg);

			//We now suppress special handling
			LOG_CALL(_("Proxy::getDescendants"));
			ASATOM_INCREF(o);
			asAtom v = asAtom::fromObject(obj);
			asAtom ret=o.callFunction(v,proxyArgs,1,true);
			ASATOM_DECREF(o);
			RUNTIME_STACK_PUSH(th,ret);
			
			obj->decRef();
			LOG_CALL(_("End of calling ") << *name);
			return;
		}
		else
		{
			tiny_string objName = obj->getClassName();
			obj->decRef();
			throwError<TypeError>(kDescendentsError, objName);
		}
	}
	else
	{
		tiny_string objName = obj->getClassName();
		obj->decRef();
		throwError<TypeError>(kDescendentsError, objName);
	}
	XMLList* retObj=XMLList::create(th->context->root->getSystemState(),ret,targetobject,*name);
	RUNTIME_STACK_PUSH(th,asAtom::fromObject(retObj));
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
	LOG_CALL(_("increment_i"));

	int n=o->toInt();
	o->decRef();
	return n+1;
}

uint64_t ABCVm::increment_di(ASObject* o)
{
	LOG_CALL(_("increment_di"));

	int64_t n=o->toInt64();
	o->decRef();
	return n+1;
}

ASObject* ABCVm::nextValue(ASObject* index, ASObject* obj)
{
	LOG_CALL("nextValue");
	if(index->getObjectType()!=T_UINTEGER)
		throw UnsupportedException("Type mismatch in nextValue");

	asAtom ret=obj->nextValue(index->toUInt());
	obj->decRef();
	index->decRef();
	ASATOM_INCREF(ret);
	return ret.toObject(obj->getSystemState());
}

ASObject* ABCVm::nextName(ASObject* index, ASObject* obj)
{
	LOG_CALL("nextName");
	if(index->getObjectType()!=T_UINTEGER)
		throw UnsupportedException("Type mismatch in nextName");

	asAtom ret=obj->nextName(index->toUInt());
	obj->decRef();
	index->decRef();
	ASATOM_INCREF(ret);
	return ret.toObject(obj->getSystemState());
}
ASObject* ABCVm::hasNext(ASObject* obj,ASObject* cur_index)
{
	LOG_CALL("hasNext " << obj->toDebugString() << ' ' << cur_index->toDebugString());

	uint32_t curIndex=cur_index->toUInt();

	uint32_t newIndex=obj->nextNameIndex(curIndex);
	return abstract_i(obj->getSystemState(),newIndex);
}

std::vector<Class_base*> classesToLinkInterfaces;
void ABCVm::SetAllClassLinks()
{
	for (unsigned int i = 0; i < classesToLinkInterfaces.size(); i++)
	{
		Class_base* cls = classesToLinkInterfaces[i];
		if (!cls)
			continue;
		if (ABCVm::newClassRecursiveLink(cls, cls))
			classesToLinkInterfaces[i] = NULL;
	}
}
void ABCVm::AddClassLinks(Class_base* target)
{
	classesToLinkInterfaces.push_back(target);
}

bool ABCVm::newClassRecursiveLink(Class_base* target, Class_base* c)
{
	if(c->super)
	{
		if (!newClassRecursiveLink(target, c->super.getPtr()))
			return false;
	}
	bool bAllDefined = false;
	const vector<Class_base*>& interfaces=c->getInterfaces(&bAllDefined);
	if (!bAllDefined)
	{
		return false;
	}
	for(unsigned int i=0;i<interfaces.size();i++)
	{
		LOG_CALL(_("Linking with interface ") << interfaces[i]->class_name);
		interfaces[i]->linkInterface(target);
	}
	return true;
}

void ABCVm::newClass(call_context* th, int n)
{
	int name_index=th->context->instances[n].name;
	assert_and_throw(name_index);
	const multiname* mname=th->context->getMultiname(name_index,NULL);
	LOG_CALL( "newClass " << *mname );

	RUNTIME_STACK_POP_CREATE_ASOBJECT(th,baseClass, th->context->root->getSystemState());

	assert_and_throw(mname->ns.size()==1);
	QName className(mname->name_s_id,mname->ns[0].nsNameId);

	Class_inherit* ret = NULL;
	auto i = th->context->root->applicationDomain->classesBeingDefined.cbegin();
	while (i != th->context->root->applicationDomain->classesBeingDefined.cend())
	{
		if(i->first->name_s_id == mname->name_s_id && i->first->ns[0].nsRealId == mname->ns[0].nsRealId)
		{
			ret = (Class_inherit*)i->second;
			ret->incRef();
			break;
		}
		i++;
	}

	if (ret == NULL)
	{
		//Check if this class has been already defined
		_NR<ApplicationDomain> domain = getCurrentApplicationDomain(th);
		ASObject* target;
		ASObject* oldDefinition=domain->getVariableAndTargetByMultiname(*mname, target);
		if(oldDefinition && oldDefinition->getObjectType()==T_CLASS)
		{
			LOG_CALL(_("Class ") << className << _(" already defined. Pushing previous definition"));
			baseClass->decRef();
			oldDefinition->incRef();
			RUNTIME_STACK_PUSH(th,asAtom::fromObject(oldDefinition));
			// ensure that this interface is linked to all previously defined classes implementing this interface
			if (th->context->instances[n].isInterface())
				ABCVm::SetAllClassLinks();
			return;
		}
		
		ret=new (th->context->root->getSystemState()->unaccountedMemory) Class_inherit(className, th->context->root->getSystemState()->unaccountedMemory,NULL);

		LOG_CALL("add classes defined:"<<*mname<<" "<<th->context);
		//Add the class to the ones being currently defined in this context
		th->context->root->applicationDomain->classesBeingDefined.insert(make_pair(mname, ret));
	}
		
	ret->isFinal = th->context->instances[n].isFinal();
	ret->isSealed = th->context->instances[n].isSealed();
	ret->isInterface = th->context->instances[n].isInterface();

	assert_and_throw(th->context);
	ret->context=th->context;

	//Null is a "valid" base class
	if(baseClass->getObjectType()!=T_NULL)
	{
		assert_and_throw(baseClass->is<Class_base>());
		Class_base* base = baseClass->as<Class_base>();
		assert(!base->isFinal);
		if (ret->super.isNull())
			ret->setSuper(_MR(base));
		else if (base != ret->super.getPtr())
		{
			LOG(LOG_ERROR,"resetting super class from "<<ret->super->toDebugString() <<" to "<< base->toDebugString());
			ret->setSuper(_MR(base));
		}
		i = th->context->root->applicationDomain->classesBeingDefined.cbegin();
		while (i != th->context->root->applicationDomain->classesBeingDefined.cend())
		{
			if(i->second == base)
			{
				th->context->root->applicationDomain->classesSuperNotFilled.push_back(ret);
				break;
			}
			i++;
		}
		
	}

	//Add protected namespace if needed
	if(th->context->instances[n].isProtectedNs())
	{
		ret->use_protected=true;
		int ns=th->context->instances[n].protectedNs;
		const namespace_info& ns_info=th->context->constant_pool.namespaces[ns];
		ret->initializeProtectedNamespace(th->context->getString(ns_info.name),ns_info);
	}

	ret->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(ret->getSystemState(),Class_base::_toString),NORMAL_METHOD,false);

	if (!th->parent_scope_stack.isNull())
		ret->class_scope = th->parent_scope_stack->scope;
	for(uint32_t i = 0 ; i < th->curr_scope_stack; i++)
	{
		ASATOM_INCREF(th->scope_stack[i]);
		ret->class_scope.push_back(scope_entry(th->scope_stack[i],th->scope_stack_dynamic[i]));
	}

	LOG_CALL(_("Building class traits"));
	for(unsigned int i=0;i<th->context->classes[n].trait_count;i++)
		th->context->buildTrait(ret,&th->context->classes[n].traits[i],false);

	LOG_CALL(_("Adding immutable object traits to class"));
	//Class objects also contains all the methods/getters/setters declared for instances
	instance_info* cur=&th->context->instances[n];
	for(unsigned int i=0;i<cur->trait_count;i++)
	{
		//int kind=cur->traits[i].kind&0xf;
		//if(kind==traits_info::Method || kind==traits_info::Setter || kind==traits_info::Getter)
			th->context->buildTrait(ret,&cur->traits[i],true);
	}

	method_info* constructor=&th->context->methods[th->context->instances[n].init];
	if(constructor->body) /* e.g. interfaces have no valid constructor */
	{
#ifdef PROFILING_SUPPORT
		if(!constructor->validProfName)
		{
			constructor->profName=mname->normalizedName(th->context->root->getSystemState())+"::__CONSTRUCTOR__";
			constructor->validProfName=true;
		}
#endif
		SyntheticFunction* constructorFunc=Class<IFunction>::getSyntheticFunction(ret->getSystemState(),constructor);
		constructorFunc->acquireScope(ret->class_scope);
		ret->incRef();
		constructorFunc->addToScope(scope_entry(asAtom::fromObject(ret),false));
		constructorFunc->inClass = ret;
		//add Constructor the the class methods
		ret->constructor=constructorFunc;
	}
	ret->class_index=n;
	th->context->root->bindClass(className,ret);

	//Add prototype variable
	ret->prototype = _MNR(new_objectPrototype(ret->getSystemState()));
	//Add the constructor variable to the class prototype
	ret->prototype->setVariableByQName("constructor","",ret, DECLARED_TRAIT);
	if(ret->super)
		ret->prototype->prevPrototype=ret->super->prototype;
	ret->addPrototypeGetter();
	if (constructor->body)
		ret->constructorprop = _NR<ObjectConstructor>(new_objectConstructor(ret,ret->constructor->length));
	else
		ret->constructorprop = _NR<ObjectConstructor>(new_objectConstructor(ret,0));
	
	ret->addConstructorGetter();

	//add implemented interfaces
	for(unsigned int i=0;i<th->context->instances[n].interface_count;i++)
	{
		multiname* name=th->context->getMultiname(th->context->instances[n].interfaces[i],NULL);
		ret->addImplementedInterface(*name);

		//Make the class valid if needed
		ASObject* target;
		ASObject* obj=getCurrentApplicationDomain(th)->getVariableAndTargetByMultiname(*name, target);

		//Named only interfaces seems to be allowed 
		if(obj==NULL)
			continue;

	}
	//If the class is not an interface itself, link the traits
	if(!th->context->instances[n].isInterface())
	{
		//Link all the interfaces for this class and all the bases
		if (!newClassRecursiveLink(ret, ret))
		{
			// remember classes where not all interfaces are defined yet
			ABCVm::AddClassLinks(ret);
		}
	}
	// ensure that this interface is linked to all previously defined classes implementing this interface
	if (th->context->instances[n].isInterface())
		ABCVm::SetAllClassLinks();
	
	LOG_CALL(_("Calling Class init ") << ret);
	//Class init functions are called with global as this
	method_info* m=&th->context->methods[th->context->classes[n].cinit];
	SyntheticFunction* cinit=Class<IFunction>::getSyntheticFunction(ret->getSystemState(),m);
	//cinit must inherit the current scope
	if (!th->parent_scope_stack.isNull())
		cinit->acquireScope(th->parent_scope_stack->scope);
	for(uint32_t i = 0 ; i < th->curr_scope_stack; i++)
	{
		ASATOM_INCREF(th->scope_stack[i]);
		cinit->addToScope(scope_entry(th->scope_stack[i],th->scope_stack_dynamic[i]));
	}
	
	asAtom ret2;
	try
	{
		asAtom v = asAtom::fromObject(ret);
		ret2=asAtom::fromObject(cinit).callFunction(v,NULL,0,true);
	}
	catch(ASObject* exc)
	{
		LOG_CALL(_("Exception during class initialization. Returning Undefined"));
		//Handle eventual exceptions from the constructor, to fix the stack
		RUNTIME_STACK_PUSH(th,asAtom::fromObject(th->context->root->applicationDomain->getSystemState()->getUndefinedRef()));
		cinit->decRef();

		//Remove the class to the ones being currently defined in this context
		th->context->root->applicationDomain->classesBeingDefined.erase(mname);
		throw;
	}
	assert_and_throw(ret2.type == T_UNDEFINED);
	ASATOM_DECREF(ret2);
	LOG_CALL(_("End of Class init ") << *mname <<" " <<ret);
	RUNTIME_STACK_PUSH(th,asAtom::fromObject(ret));
	cinit->decRef();

	auto j = th->context->root->applicationDomain->classesSuperNotFilled.cbegin();
	while (j != th->context->root->applicationDomain->classesSuperNotFilled.cend())
	{
		if((*j)->super == ret)
		{
			(*j)->copyBorrowedTraitsFromSuper();
			th->context->root->applicationDomain->classesSuperNotFilled.remove(ret);
			break;
		}
		j++;
	}
	//Remove the class to the ones being currently defined in this context
	th->context->root->applicationDomain->classesBeingDefined.erase(mname);
}

void ABCVm::swap()
{
	LOG_CALL(_("swap"));
}

ASObject* ABCVm::newActivation(call_context* th, method_info* mi)
{
	LOG_CALL("newActivation");
	//TODO: Should create a real activation object
	//TODO: Should method traits be added to the activation context?
	ASObject* act= NULL;
	ASObject* caller = th->locals[0].getObject();
	if (caller != NULL && caller->is<Function_object>())
	{
		act = new_functionObject(caller->as<Function_object>()->functionPrototype);
		act->incRef();
	}
	else
		act = Class<ASObject>::getInstanceS(th->context->root->getSystemState());
#ifndef NDEBUG
	act->initialized=false;
#endif
	act->Variables.Variables.reserve(mi->body->trait_count);
	for(unsigned int i=0;i<mi->body->trait_count;i++)
		th->context->buildTrait(act,&mi->body->traits[i],false,-1,false);
#ifndef NDEBUG
	act->initialized=true;
#endif

	return act;
}

void ABCVm::popScope(call_context* th)
{
	LOG_CALL(_("popScope"));
	assert_and_throw(th->curr_scope_stack);
	th->curr_scope_stack--;
}

bool ABCVm::lessThan(ASObject* obj1, ASObject* obj2)
{
	LOG_CALL(_("lessThan"));

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

	RUNTIME_STACK_POP_CREATE(th,obj);
	RUNTIME_STACK_POP_CREATE(th,f);
	LOG_CALL(_("call ") << m << ' ' << f.type);
	callImpl(th, f, obj, args, m, true);
}
// this consumes one reference of obj and of each arg
void ABCVm::callImpl(call_context* th, asAtom& f, asAtom& obj, asAtom* args, int m, bool keepReturn)
{
	if(f.is<IFunction>())
	{
		asAtom ret=f.callFunction(obj,args,m,true);
		if(keepReturn)
			RUNTIME_STACK_PUSH(th,ret);
		else
			ASATOM_DECREF(ret);
	}
	else if(f.is<Class_base>())
	{
		Class_base* c=f.as<Class_base>();
		asAtom ret=c->generator(args,m);
		c->decRef();
		if(keepReturn)
			RUNTIME_STACK_PUSH(th,ret);
		else
			ASATOM_DECREF(ret);
	}
	else if(f.is<RegExp>())
	{
		asAtom ret=RegExp::exec(th->context->root->getSystemState(),f,args,m);
		if(keepReturn)
			RUNTIME_STACK_PUSH(th,ret);
		else
			ASATOM_DECREF(ret);
		for(int i=0;i<m;++i)
			ASATOM_DECREF(args[i]);
	}
	else
	{
		LOG(LOG_ERROR,"trying to call an object as a function:"<<f.toDebugString() <<" on "<<obj.toDebugString());
		ASATOM_DECREF(obj);
		for(int i=0;i<m;++i)
			ASATOM_DECREF(args[i]);
		throwError<TypeError>(kCallOfNonFunctionError, "Object");
	}
	LOG_CALL(_("End of call ") << m << ' ' << f.type);
}

bool ABCVm::deleteProperty(ASObject* obj, multiname* name)
{
	LOG_CALL(_("deleteProperty ") << *name<<" "<<obj->toDebugString());
	if (name->name_type == multiname::NAME_OBJECT && name->name_o)
	{
		if (name->name_o->is<XMLList>())
			throwError<TypeError>(kDeleteTypeError,name->name_o->getClassName());
	}
	bool ret = obj->deleteVariableByMultiname(*name);

	obj->decRef();
	return ret;
}

ASObject* ABCVm::newFunction(call_context* th, int n)
{
	LOG_CALL(_("newFunction ") << n);

	method_info* m=&th->context->methods[n];
	SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(th->context->root->applicationDomain->getSystemState(),m);
	f->func_scope = _R<scope_entry_list>(new scope_entry_list());
	if (!th->parent_scope_stack.isNull())
		f->func_scope->scope=th->parent_scope_stack->scope;
	for(uint32_t i = 0 ; i < th->curr_scope_stack; i++)
	{
		ASATOM_INCREF(th->scope_stack[i]);
		f->addToScope(scope_entry(th->scope_stack[i],th->scope_stack_dynamic[i]));
	}
	
	//Create the prototype object
	f->prototype = _MR(new_asobject(f->getSystemState()));
	f->incRef();
	f->prototype->setVariableByQName("constructor","",f,DECLARED_TRAIT);
	return f;
}

ASObject* ABCVm::getScopeObject(call_context* th, int n)
{
	assert_and_throw(th->curr_scope_stack > (size_t)n);
	ASObject* ret=th->scope_stack[(size_t)n].toObject(th->context->root->getSystemState());
	ret->incRef();
	LOG_CALL( _("getScopeObject: ") << ret->toDebugString());
	return ret;
}

ASObject* ABCVm::pushString(call_context* th, int n)
{
	LOG_CALL( _("pushString ") << th->context->root->getSystemState()->getStringFromUniqueId(th->context->getString(n)) );
	return abstract_s(th->context->root->applicationDomain->getSystemState(),th->context->getString(n));
}

ASObject* ABCVm::newCatch(call_context* th, int n)
{
	ASObject* catchScope = Class<ASObject>::getInstanceS(th->context->root->getSystemState());
	assert_and_throw(n >= 0 && (unsigned int)n < th->mi->body->exceptions.size());
	multiname* name = th->context->getMultiname(th->mi->body->exceptions[n].var_name, NULL);
	catchScope->setVariableByMultiname(*name, asAtom::undefinedAtom,ASObject::CONST_NOT_ALLOWED);
	catchScope->initSlot(1, *name);
	return catchScope;
}

void ABCVm::newArray(call_context* th, int n)
{
	LOG_CALL( _("newArray ") << n );
	Array* ret=Class<Array>::getInstanceSNoArgs(th->context->root->getSystemState());
	ret->resize(n);
	for(int i=0;i<n;i++)
	{
		RUNTIME_STACK_POP_CREATE(th,obj);
		ret->set(n-i-1,obj);
	}

	RUNTIME_STACK_PUSH(th,asAtom::fromObject(ret));
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
	ASObject* res = abstract_s(o->getSystemState(),t);
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
	ASObject* res = abstract_s(o->getSystemState(),t);
	o->decRef();
	return res;
}

/* This should walk prototype chain of value, trying to find type. See ECMA.
 * Its usage is disouraged in AS3 in favour of 'is' and 'as' (opcodes isType and asType)
 */
bool ABCVm::instanceOf(ASObject* value, ASObject* type)
{
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
		return false;
	}

	if(!type->is<Class_base>())
		throwError<TypeError>(kCantUseInstanceofOnNonObjectError);

	if(value->is<Null>())
		return false;


	if(value->is<Class_base>())
		// Classes are instance of Class and Object but not
		// itself or super classes
		return type == Class_object::getClass(type->getSystemState()) || 
			type == Class<ASObject>::getClass(type->getSystemState());
	else
		return value->getClass() && value->getClass()->isSubClass(type->as<Class_base>(), false);
}

Namespace* ABCVm::pushNamespace(call_context* th, int n)
{
	const namespace_info& ns_info=th->context->constant_pool.namespaces[n];
	LOG_CALL( _("pushNamespace ") << th->context->root->getSystemState()->getStringFromUniqueId(th->context->getString(ns_info.name)) );
	return Class<Namespace>::getInstanceS(th->context->root->getSystemState(),th->context->getString(ns_info.name),BUILTIN_STRINGS::EMPTY,(NS_KIND)(int)ns_info.kind);
}

/* @spec-checked avm2overview */
void ABCVm::dxns(call_context* th, int n)
{
	if(!th->mi->hasDXNS())
		throw Class<VerifyError>::getInstanceS(th->context->root->getSystemState(),"dxns without SET_DXNS");

	th->defaultNamespaceUri = th->context->getString(n);
}

/* @spec-checked avm2overview */
void ABCVm::dxnslate(call_context* th, ASObject* o)
{
	if(!th->mi->hasDXNS())
		throw Class<VerifyError>::getInstanceS(th->context->root->getSystemState(),"dxnslate without SET_DXNS");

	th->defaultNamespaceUri = o->toStringId();
	o->decRef();
}
