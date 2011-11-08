/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "abc.h"
#include <limits>
#include "class.h"
#include "exceptions.h"
#include "compat.h"

using namespace std;
using namespace lightspark;

uint32_t ABCVm::bitAnd(ASObject* val2, ASObject* val1)
{
	uint32_t i1=val1->toUInt();
	uint32_t i2=val2->toUInt();
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("bitAnd_oo ") << hex << i1 << '&' << i2 << dec);
	return i1&i2;
}

uint32_t ABCVm::bitAnd_oi(ASObject* val1, int32_t val2)
{
	uint32_t i1=val1->toUInt();
	uint32_t i2=val2;
	val1->decRef();
	LOG(LOG_CALLS,_("bitAnd_oi ") << hex << i1 << '&' << i2 << dec);
	return i1&i2;
}

void ABCVm::setProperty(ASObject* value,ASObject* obj,multiname* name)
{
	LOG(LOG_CALLS,_("setProperty ") << *name << ' ' << obj);

	//We have to reset the level before finding a variable
	obj->setVariableByMultiname(*name,value);
	obj->decRef();
}

void ABCVm::setProperty_i(int32_t value,ASObject* obj,multiname* name)
{
	LOG(LOG_CALLS,_("setProperty_i ") << *name << ' ' <<obj);

	obj->setVariableByMultiname_i(*name,value);
	obj->decRef();
}

number_t ABCVm::convert_d(ASObject* o)
{
	LOG(LOG_CALLS, _("convert_d") );
	number_t ret=o->toNumber();
	o->decRef();
	return ret;
}

bool ABCVm::convert_b(ASObject* o)
{
	LOG(LOG_CALLS, _("convert_b") );
	bool ret=Boolean_concrete(o);
	o->decRef();
	return ret;
}

uint32_t ABCVm::convert_u(ASObject* o)
{
	LOG(LOG_CALLS, _("convert_u") );
	uint32_t ret=o->toUInt();
	o->decRef();
	return ret;
}

int32_t ABCVm::convert_i(ASObject* o)
{
	LOG(LOG_CALLS, _("convert_i") );
	int32_t ret=o->toInt();
	o->decRef();
	return ret;
}

ASObject* ABCVm::convert_s(ASObject* o)
{
	LOG(LOG_CALLS, _("convert_s") );
	ASObject* ret=o;
	if(o->getObjectType()!=T_STRING)
	{
		ret=Class<ASString>::getInstanceS(o->toString());
		o->decRef();
	}
	return ret;
}

void ABCVm::label()
{
	LOG(LOG_CALLS, _("label") );
}

void ABCVm::lookupswitch()
{
	LOG(LOG_CALLS, _("lookupswitch") );
}

ASObject* ABCVm::pushUndefined()
{
	LOG(LOG_CALLS, _("pushUndefined") );
	return new Undefined;
}

ASObject* ABCVm::pushNull()
{
	LOG(LOG_CALLS, _("pushNull") );
	return new Null;
}

void ABCVm::coerce_a()
{
	LOG(LOG_CALLS, _("coerce_a") );
}

ASObject* ABCVm::checkfilter(ASObject* o)
{
	Class_base* xmlClass=Class<XML>::getClass();
	Class_base* xmlListClass=Class<XMLList>::getClass();

	if (o->getClass()!=xmlClass && o->getClass()!=xmlListClass)
		throw Class<TypeError>::getInstanceS();

	return o;
}

ASObject* ABCVm::coerce_s(ASObject* o)
{
	return Class<ASString>::getClass()->coerce(o);
}

void ABCVm::coerce(call_context* th, int n)
{
	multiname* mn = th->context->getMultiname(n,NULL);
	LOG(LOG_CALLS,_("coerce ") << *mn);

	const Type* type = Type::getTypeFromMultiname(mn);

	ASObject* o=th->runtime_stack_pop();
	o=type->coerce(o);
	th->runtime_stack_push(o);
}

void ABCVm::pop()
{
	LOG(LOG_CALLS, _("pop: DONE") );
}

void ABCVm::getLocal_int(int n, int v)
{
	LOG(LOG_CALLS,_("getLocal[") << n << _("] (int)= ") << dec << v);
}

void ABCVm::getLocal(ASObject* o, int n)
{
	LOG(LOG_CALLS,_("getLocal[") << n << _("] (") << o << _(") ") << o->toDebugString());
}

void ABCVm::getLocal_short(int n)
{
	LOG(LOG_CALLS,_("getLocal[") << n << _("]"));
}

void ABCVm::setLocal(int n)
{
	LOG(LOG_CALLS,_("setLocal[") << n << _("]"));
}

void ABCVm::setLocal_int(int n, int v)
{
	LOG(LOG_CALLS,_("setLocal[") << n << _("] (int)= ") << dec << v);
}

void ABCVm::setLocal_obj(int n, ASObject* v)
{
	LOG(LOG_CALLS,_("setLocal[") << n << _("] = ") << v->toDebugString());
}

int32_t ABCVm::pushShort(intptr_t n)
{
	LOG(LOG_CALLS, _("pushShort ") << n );
	return n;
}

void ABCVm::setSlot(ASObject* value, ASObject* obj, int n)
{
	LOG(LOG_CALLS,"setSlot " << n);
	if(value==NULL)
		value=new Undefined;
	obj->setSlot(n,value);
	obj->decRef();
}

ASObject* ABCVm::getSlot(ASObject* obj, int n)
{
	ASObject* ret=obj->getSlot(n);
	LOG(LOG_CALLS,"getSlot " << n << " " << ret << "=" << ret->toDebugString());
	//getSlot can only access properties defined in the current
	//script, so they should already be defind by this script
	assert(!ret->is<Definable>());
	ret->incRef();
	obj->decRef();
	return ret;
}

number_t ABCVm::negate(ASObject* v)
{
	LOG(LOG_CALLS, _("negate") );
	number_t ret=-(v->toNumber());
	v->decRef();
	return ret;
}

int32_t ABCVm::negate_i(ASObject* o)
{
	LOG(LOG_CALLS,_("negate_i"));

	int n=o->toInt();
	o->decRef();
	return -n;
}

uint32_t ABCVm::bitNot(ASObject* val)
{
	uint32_t i1=val->toUInt();
	val->decRef();
	LOG(LOG_CALLS,_("bitNot ") << hex << i1 << dec);
	return ~i1;
}

uint32_t ABCVm::bitXor(ASObject* val2, ASObject* val1)
{
	int i1=val1->toUInt();
	int i2=val2->toUInt();
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("bitXor ") << hex << i1 << '^' << i2 << dec);
	return i1^i2;
}

uint32_t ABCVm::bitOr_oi(ASObject* val2, uint32_t val1)
{
	int i1=val1;
	int i2=val2->toUInt();
	val2->decRef();
	LOG(LOG_CALLS,_("bitOr ") << hex << i1 << '|' << i2 << dec);
	return i1|i2;
}

uint32_t ABCVm::bitOr(ASObject* val2, ASObject* val1)
{
	int i1=val1->toUInt();
	int i2=val2->toUInt();
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("bitOr ") << hex << i1 << '|' << i2 << dec);
	return i1|i2;
}

void ABCVm::callProperty(call_context* th, int n, int m, method_info** called_mi, bool keepReturn)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_CALLS, (keepReturn ? "callProperty " : "callPropVoid") << *name << ' ' << m);

	ASObject* obj=th->runtime_stack_pop();
	if(obj->classdef)
		LOG(LOG_CALLS,obj->classdef->class_name);

	//We should skip the special implementation of get
	_NR<ASObject> o=obj->getVariableByMultiname(*name, ASObject::SKIP_IMPL);

	if(!o.isNull())
	{
		//Run the deferred initialization if needed
		if(o->is<Definable>())
		{
			LOG(LOG_CALLS,_("We got a function not yet valid"));
			ASObject *defined=o->as<Definable>()->define();
			defined->incRef();
			o=_MNR(defined);
		}
		o->incRef();
		callImpl(th, o.getPtr(), obj, args, m, called_mi, keepReturn);
	}
	else
	{
		//If the object is a Proxy subclass, try to invoke callProperty
		if(obj->classdef && obj->classdef->isSubClass(Class<Proxy>::getClass()))
		{
			//Check if there is a custom caller defined, skipping implementation to avoid recursive calls
			multiname callPropertyName;
			callPropertyName.name_type=multiname::NAME_STRING;
			callPropertyName.name_s="callProperty";
			callPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
			_NR<ASObject> o=obj->getVariableByMultiname(callPropertyName,ASObject::SKIP_IMPL);

			if(!o.isNull())
			{
				assert_and_throw(o->getObjectType()==T_FUNCTION);

				IFunction* f=static_cast<IFunction*>(o.getPtr());
				//Create a new array
				ASObject** proxyArgs=new ASObject*[m+1];
				//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
				proxyArgs[0]=Class<ASString>::getInstanceS(name->name_s);
				for(int i=0;i<m;i++)
					proxyArgs[i+1]=args[i];
				delete[] args;

				//We now suppress special handling
				LOG(LOG_CALLS,_("Proxy::callProperty"));
				f->incRef();
				obj->incRef();
				ASObject* ret=f->call(obj,proxyArgs,m+1);
				//call getMethodInfo only after the call, so it's updated
				if(called_mi)
					*called_mi=f->getMethodInfo();
				f->decRef();
				if(keepReturn)
					th->runtime_stack_push(ret);
				else
					ret->decRef();

				obj->decRef();
				LOG(LOG_CALLS,_("End of calling ") << *name);
				delete[] proxyArgs;
				return;
			}
		}

		LOG(LOG_NOT_IMPLEMENTED,"callProperty: " << name->normalizedName() << " not found on " << obj->toDebugString());
		if(keepReturn)
			th->runtime_stack_push(new Undefined);

		obj->decRef();
		delete[] args;
	}
	LOG(LOG_CALLS,_("End of calling ") << *name);
}

int32_t ABCVm::getProperty_i(ASObject* obj, multiname* name)
{
	LOG(LOG_CALLS, _("getProperty_i ") << *name );

	//TODO: implement exception handling to find out if no integer can be returned
	int32_t ret=obj->getVariableByMultiname_i(*name);

	obj->decRef();
	return ret;
}

ASObject* ABCVm::getProperty(ASObject* obj, multiname* name)
{
	LOG(LOG_CALLS, _("getProperty ") << *name << ' ' << obj);

	_NR<ASObject> prop=obj->getVariableByMultiname(*name);
	ASObject *ret;

	if(prop.isNull())
	{
		LOG(LOG_NOT_IMPLEMENTED,"getProperty: " << name->normalizedName() << " not found on " << obj->toDebugString());
		ret = new Undefined;
	}
	else
	{
		if(prop->is<Definable>())
		{
			LOG(LOG_CALLS,_("Property ") << name << _(" is not yet valid"));
			ret=prop->as<Definable>()->define();
			ret->incRef();
		}
		else
		{
			prop->incRef();
			ret=prop.getPtr();
		}
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
	LOG(LOG_CALLS,_("divide ")  << num1 << '/' << num2);
	return num1/num2;
}

void ABCVm::pushWith(call_context* th)
{
	ASObject* t=th->runtime_stack_pop();
	LOG(LOG_CALLS, _("pushWith ") << t );
	th->scope_stack.emplace_back(scope_entry(_MR(t), true));
}

void ABCVm::pushScope(call_context* th)
{
	ASObject* t=th->runtime_stack_pop();
	LOG(LOG_CALLS, _("pushScope ") << t );
	th->scope_stack.emplace_back(scope_entry(_MR(t), false));
}

Global* ABCVm::getGlobalScope(call_context* th)
{
	assert_and_throw(th->scope_stack.size() > 0);
	ASObject* ret=th->scope_stack[0].object.getPtr();
	assert_and_throw(ret->is<Global>());
	LOG(LOG_CALLS,_("getGlobalScope: ") << ret);
	ret->incRef();
	return ret->as<Global>();
}

number_t ABCVm::decrement(ASObject* o)
{
	LOG(LOG_CALLS,_("decrement"));

	number_t n=o->toNumber();
	o->decRef();
	return n-1;
}

uint32_t ABCVm::decrement_i(ASObject* o)
{
	LOG(LOG_CALLS,_("decrement_i"));

	int n=o->toInt();
	o->decRef();
	return n-1;
}

bool ABCVm::ifNLT(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=!(obj1->isLess(obj2)==TTRUE);
	LOG(LOG_CALLS,_("ifNLT (") << ((ret)?_("taken)"):_("not taken)")));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLT(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TTRUE);
	LOG(LOG_CALLS,_("ifLT (") << ((ret)?_("taken)"):_("not taken)")));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLT_oi(ASObject* obj2, int32_t val1)
{
	LOG(LOG_CALLS,_("ifLT_oi"));

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
	LOG(LOG_CALLS,_("ifLT_io "));

	bool ret=obj1->toInt()<val2;

	obj1->decRef();
	return ret;
}

bool ABCVm::ifNE(ASObject* obj1, ASObject* obj2)
{
	//Real comparision demanded to object
	bool ret=!(obj1->isEqual(obj2));
	LOG(LOG_CALLS,_("ifNE (") << ((ret)?_("taken)"):_("not taken)")));

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
	LOG(LOG_CALLS,_("ifNE (") << ((ret)?_("taken)"):_("not taken)")));

	obj1->decRef();
	return ret;
}

int32_t ABCVm::pushByte(intptr_t n)
{
	LOG(LOG_CALLS, _("pushByte ") << n );
	return n;
}

number_t ABCVm::multiply_oi(ASObject* val2, int32_t val1)
{
	double num1=val1;
	double num2=val2->toNumber();
	val2->decRef();
	LOG(LOG_CALLS,_("multiply_oi ")  << num1 << '*' << num2);
	return num1*num2;
}

number_t ABCVm::multiply(ASObject* val2, ASObject* val1)
{
	double num1=val1->toNumber();
	double num2=val2->toNumber();
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("multiply ")  << num1 << '*' << num2);
	return num1*num2;
}

int32_t ABCVm::multiply_i(ASObject* val2, ASObject* val1)
{
	int num1=val1->toInt();
	int num2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("multiply ")  << num1 << '*' << num2);
	return num1*num2;
}

void ABCVm::incLocal(call_context* th, int n)
{
	LOG(LOG_CALLS, _("incLocal ") << n );
	if(th->locals[n]->getObjectType()==T_NUMBER)
	{
		th->locals[n]->as<Number>()->val++;
	}
	else
	{
		number_t tmp=th->locals[n]->toNumber();
		th->locals[n]->decRef();
		th->locals[n]=abstract_d(tmp+1);
	}

}

void ABCVm::incLocal_i(call_context* th, int n)
{
	LOG(LOG_CALLS, _("incLocal_i ") << n );
	if(th->locals[n]->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(th->locals[n]);
		i->val++;
	}
	else
	{
		int32_t tmp=th->locals[n]->toInt();
		th->locals[n]->decRef();
		th->locals[n]=abstract_i(tmp+1);
	}

}

void ABCVm::decLocal(call_context* th, int n)
{
	LOG(LOG_CALLS, _("decLocal ") << n );
	if(th->locals[n]->getObjectType()==T_NUMBER)
	{
		th->locals[n]->as<Number>()->val--;
	}
	else
	{
		number_t tmp=th->locals[n]->toNumber();
		th->locals[n]->decRef();
		th->locals[n]=abstract_d(tmp-1);
	}

}

void ABCVm::decLocal_i(call_context* th, int n)
{
	LOG(LOG_CALLS, _("decLocal_i ") << n );
	if(th->locals[n]->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(th->locals[n]);
		i->val--;
	}
	else
	{
		int32_t tmp=th->locals[n]->toInt();
		th->locals[n]->decRef();
		th->locals[n]=abstract_i(tmp-1);
	}

}

/* This is called for expressions like
 * function f() { ... }
 * var v = new f();
 */
ASObject* ABCVm::constructFunction(call_context* th, IFunction* f, ASObject** args, int argslen)
{
	//See ECMA 13.2.2
	assert(f->is<SyntheticFunction>());
	SyntheticFunction* sf=f->as<SyntheticFunction>();

	ASObject* ret=Class<ASObject>::getInstanceS();
	assert(sf->mi->body);
#ifndef NDEBUG
	ret->initialized=false;
#endif
	LOG(LOG_CALLS,_("Building method traits"));
	for(unsigned int i=0;i<sf->mi->body->trait_count;i++)
		th->context->buildTrait(ret,&sf->mi->body->traits[i],false);
#ifndef NDEBUG
	ret->initialized=true;
#endif
	//Now add our classdef
	ret->setClass(new Class_function());
	ret->getClass()->prototype = sf->prototype;

	sf->incRef();
	ret->setVariableByQName("constructor","",sf,DYNAMIC_TRAIT);

	ret->incRef();
	assert_and_throw(!sf->closure_this);
	sf->incRef();
	ASObject* ret2=sf->call(ret,args,argslen);
	sf->decRef();

	//ECMA: "return ret2 if it is an object, else ret"
	if(ret2 && !ret2->is<Undefined>())
	{
		ret->decRef();
		ret = ret2;
	}
	else if(ret2)
		ret2->decRef();

	return ret;
}

void ABCVm::construct(call_context* th, int m)
{
	LOG(LOG_CALLS, _("construct ") << m);
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	ASObject* obj=th->runtime_stack_pop();

	if(obj->getObjectType()==T_DEFINABLE)
	{
		throw UnsupportedException("Definable not supported in construct");
	/*	LOG(LOG_CALLS,_("Deferred definition of property ") << name);
		Definable* d=static_cast<Definable*>(o);
		d->define(obj);
		o=obj->getVariableByMultiname(name,owner);
		LOG(LOG_CALLS,_("End of deferred definition of property ") << name);*/
	}

	LOG(LOG_CALLS,_("Constructing"));

	ASObject* ret;
	switch(obj->getObjectType())
	{
		case T_CLASS:
		{
			Class_base* o_class=static_cast<Class_base*>(obj);
			ret=o_class->getInstance(true,args,m);
			break;
		}

		case T_OBJECT:
		{
			Class_base* o_class=static_cast<Class_base*>(obj->getClass());
			assert(o_class);
			ret=o_class->getInstance(true,args,m);
			break;
		}

		case T_UNDEFINED:
		case T_NULL:
		{
			//Inc ref count to make up for decremnt later
			obj->incRef();
			ret=obj;
			break;
		}

		case T_FUNCTION:
		{
			ret = constructFunction(th, obj->as<IFunction>(), args, m);
			break;
		}

		default:
		{
			LOG(LOG_ERROR,_("Object type ") << obj->getObjectType() << _(" not supported in construct"));
			throw UnsupportedException("This object is not supported in construct");
			break;
		}
	}

	obj->decRef();
	LOG(LOG_CALLS,_("End of constructing ") << ret);
	th->runtime_stack_push(ret);
	delete[] args;
}

void ABCVm::constructGenericType(call_context* th, int m)
{
	LOG(LOG_CALLS, _("constructGenericType ") << m);
	assert_and_throw(m==1);
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	ASObject* obj=th->runtime_stack_pop();

	if(obj->getObjectType() != T_TEMPLATE)
	{
		LOG(LOG_NOT_IMPLEMENTED, "constructGenericType of " << obj->getObjectType());
		obj->decRef();
		obj = new Undefined();
		th->runtime_stack_push(obj);
		for(int i=0;i<m;i++)
			args[i]->decRef();
		delete[] args;
		return;
	}

	Template_base* o_template=static_cast<Template_base*>(obj);

	/* Instantiate the template to obtain a class */
	Class_base* o_class = o_template->applyType(args,m);

	th->runtime_stack_push(o_class);
	delete[] args;
}

ASObject* ABCVm::typeOf(ASObject* obj)
{
	LOG(LOG_CALLS,_("typeOf"));
	string ret;
	switch(obj->getObjectType())
	{
		case T_UNDEFINED:
			ret="undefined";
			break;
		case T_OBJECT:
		case T_NULL:
		case T_ARRAY:
		case T_CLASS: //this is not clear from spec, but was tested
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
	obj->decRef();
	return Class<ASString>::getInstanceS(ret);
}

void ABCVm::jump(int offset)
{
	LOG(LOG_CALLS,_("jump ") << offset);
}

bool ABCVm::ifTrue(ASObject* obj1)
{
	bool ret=Boolean_concrete(obj1);
	LOG(LOG_CALLS,_("ifTrue (") << ((ret)?_("taken)"):_("not taken)")));

	obj1->decRef();
	return ret;
}

int32_t ABCVm::modulo(ASObject* val1, ASObject* val2)
{
	uint32_t num1=val1->toUInt();
	uint32_t num2=val2->toUInt();

	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("modulo ")  << num1 << '%' << num2);
	//Special case 0%0
	//TODO: should be NaN
	if(num1==0 && num2==0)
		return 0;
	return num1%num2;
}

number_t ABCVm::subtract_oi(ASObject* val2, int32_t val1)
{
	int num2=val2->toInt();
	int num1=val1;

	val2->decRef();
	LOG(LOG_CALLS,_("subtract_oi ") << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_do(number_t val2, ASObject* val1)
{
	number_t num2=val2;
	number_t num1=val1->toNumber();

	val1->decRef();
	LOG(LOG_CALLS,_("subtract_do ") << num1 << '-' << num2);
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
	LOG(LOG_CALLS,_("subtract_io ") << dec << num1 << '-' << num2);
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
	LOG(LOG_CALLS,_("subtract_i ") << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract(ASObject* val2, ASObject* val1)
{
	number_t num2=val2->toNumber();
	number_t num1=val1->toNumber();

	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("subtract ") << num1 << '-' << num2);
	return num1-num2;
}

void ABCVm::pushUInt(call_context* th, int n)
{
	u32 i=th->context->constant_pool.uinteger[n];
	LOG(LOG_CALLS, _("pushUInt [") << dec << n << _("] ") << i);
}

void ABCVm::pushInt(call_context* th, int n)
{
	s32 i=th->context->constant_pool.integer[n];
	LOG(LOG_CALLS, _("pushInt [") << dec << n << _("] ") << i);
}

void ABCVm::pushDouble(call_context* th, int n)
{
	d64 d=th->context->constant_pool.doubles[n];
	LOG(LOG_CALLS, _("pushDouble [") << dec << n << _("] ") << d);
}

void ABCVm::kill(int n)
{
	LOG(LOG_CALLS, _("kill ") << n );
}

ASObject* ABCVm::add(ASObject* val2, ASObject* val1)
{
	//Implement ECMA add algorithm, for XML and default (see avm2overview)
	if(val1->is<Number>() && val2->is<Number>())
	{
		double num1=val1->as<Number>()->val;
		double num2=val2->as<Number>()->val;
		LOG(LOG_CALLS,"addN " << num1 << '+' << num2);
		val1->decRef();
		val2->decRef();
		return abstract_d(num1+num2);
	}
	else if(val1->is<ASString>() || val2->is<ASString>())
	{
		tiny_string a = val1->toString();
		tiny_string b = val2->toString();
		LOG(LOG_CALLS,"add " << a << '+' << b);
		val1->decRef();
		val2->decRef();
		return Class<ASString>::getInstanceS(a + b);
	}
	else if( (val1->is<XML>() || val1->is<XMLList>()) && (val2->is<XML>() || val2->is<XMLList>()) )
	{
		//Check if the objects are both XML or XMLLists
		Class_base* xmlClass=Class<XML>::getClass();

		XMLList* newList=Class<XMLList>::getInstanceS(true);
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
		val1->decRef();
		val2->decRef();
		if(val1p->is<ASString>() || val2p->is<ASString>())
		{//If one is String, convert both to strings and concat
			string a(val1p->toString().raw_buf());
			string b(val2p->toString().raw_buf());
			LOG(LOG_CALLS,"add " << a << '+' << b);
			return Class<ASString>::getInstanceS(a+b);
		}
		else
		{//Convert both to numbers and add
			number_t result = val2p->toNumber() + val1p->toNumber();
			return abstract_d(result);
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
	LOG(LOG_CALLS,_("add_i ") << num1 << '-' << num2);
	return num1+num2;
}

ASObject* ABCVm::add_oi(ASObject* val2, int32_t val1)
{
	//Implement ECMA add algorithm, for XML and default
	if(val2->getObjectType()==T_INTEGER)
	{
		Integer* ip=static_cast<Integer*>(val2);
		int32_t num2=ip->val;
		int32_t num1=val1;
		val2->decRef();
		LOG(LOG_CALLS,_("add ") << num1 << '+' << num2);
		return abstract_i(num1+num2);
	}
	else if(val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		val2->decRef();
		LOG(LOG_CALLS,_("add ") << num1 << '+' << num2);
		return abstract_d(num1+num2);
	}
	else if(val2->getObjectType()==T_STRING)
	{
		//Convert argument to int32_t
		tiny_string a = Integer::toString(val1);
		const tiny_string& b=val2->toString();
		val2->decRef();
		LOG(LOG_CALLS,_("add ") << a << '+' << b);
		return Class<ASString>::getInstanceS(a+b);
	}
	else
	{
		return add(val2,abstract_i(val1));
	}

}

ASObject* ABCVm::add_od(ASObject* val2, number_t val1)
{
	//Implement ECMA add algorithm, for XML and default
	if(val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		val2->decRef();
		LOG(LOG_CALLS,_("add ") << num1 << '+' << num2);
		return abstract_d(num1+num2);
	}
	else if(val2->getObjectType()==T_INTEGER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		val2->decRef();
		LOG(LOG_CALLS,_("add ") << num1 << '+' << num2);
		return abstract_d(num1+num2);
	}
	else if(val2->getObjectType()==T_STRING)
	{
		tiny_string a = Number::toString(val1);
		const tiny_string& b=val2->toString();
		val2->decRef();
		LOG(LOG_CALLS,_("add ") << a << '+' << b);
		return Class<ASString>::getInstanceS(a+b);
	}
	else
	{
		return add(val2,abstract_d(val1));
	}

}

uint32_t ABCVm::lShift(ASObject* val1, ASObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("lShift ")<<hex<<i2<<_("<<")<<i1<<dec);
	//Left shift are supposed to always work in 32bit
	uint32_t ret=i2<<i1;
	return ret;
}

uint32_t ABCVm::lShift_io(uint32_t val1, ASObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1&0x1f;
	val2->decRef();
	LOG(LOG_CALLS,_("lShift ")<<hex<<i2<<_("<<")<<i1<<dec);
	//Left shift are supposed to always work in 32bit
	uint32_t ret=i2<<i1;
	return ret;
}

uint32_t ABCVm::rShift(ASObject* val1, ASObject* val2)
{
	int32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("rShift ")<<hex<<i2<<_(">>")<<i1<<dec);
	return i2>>i1;
}

uint32_t ABCVm::urShift(ASObject* val1, ASObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("urShift ")<<hex<<i2<<_(">>")<<i1<<dec);
	return i2>>i1;
}

uint32_t ABCVm::urShift_io(uint32_t val1, ASObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1&0x1f;
	val2->decRef();
	LOG(LOG_CALLS,_("urShift ")<<hex<<i2<<_(">>")<<i1<<dec);
	return i2>>i1;
}

bool ABCVm::_not(ASObject* v)
{
	LOG(LOG_CALLS, _("not") );
	bool ret=!Boolean_concrete(v);
	v->decRef();
	return ret;
}

bool ABCVm::equals(ASObject* val2, ASObject* val1)
{
	bool ret=val1->isEqual(val2);
	LOG(LOG_CALLS, _("equals ") << ret);
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
			default:
				return false;
		}
		switch(type2)
		{
			case T_NUMBER:
			case T_INTEGER:
			case T_UINTEGER:
				break;
			default:
				return false;
		}
	}
	return obj1->isEqual(obj2);
}

bool ABCVm::strictEquals(ASObject* obj2, ASObject* obj1)
{
	LOG(LOG_CALLS, _("strictEquals") );
	bool ret=strictEqualImpl(obj1, obj2);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::dup()
{
	LOG(LOG_CALLS, _("dup: DONE") );
}

bool ABCVm::pushTrue()
{
	LOG(LOG_CALLS, _("pushTrue") );
	return true;
}

bool ABCVm::pushFalse()
{
	LOG(LOG_CALLS, _("pushFalse") );
	return false;
}

ASObject* ABCVm::pushNaN()
{
	LOG(LOG_CALLS, _("pushNaN") );
	//Not completely correct, but mostly ok
	return new Undefined;
}

bool ABCVm::ifGT(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TTRUE);
	LOG(LOG_CALLS,_("ifGT (") << ((ret)?_("taken)"):_("not taken)")));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifNGT(ASObject* obj2, ASObject* obj1)
{

	//Real comparision demanded to object
	bool ret=!(obj2->isLess(obj1)==TTRUE);
	LOG(LOG_CALLS,_("ifNGT (") << ((ret)?_("taken)"):_("not taken)")));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TFALSE);
	LOG(LOG_CALLS,_("ifLE (") << ((ret)?_("taken)"):_("not taken)")));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifNLE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=!(obj2->isLess(obj1)==TFALSE);
	LOG(LOG_CALLS,_("ifNLE (") << ((ret)?_("taken)"):_("not taken)")));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifGE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TFALSE);
	LOG(LOG_CALLS,_("ifGE (") << ((ret)?_("taken)"):_("not taken)")));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifNGE(ASObject* obj2, ASObject* obj1)
{
	//Real comparision demanded to object
	bool ret=!(obj1->isLess(obj2)==TFALSE);
	LOG(LOG_CALLS,_("ifNGE (") << ((ret)?_("taken)"):_("not taken)")));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::_throw(call_context* th)
{
	LOG(LOG_CALLS,_("throw"));
	throw th->runtime_stack_pop();
}

void ABCVm::setSuper(call_context* th, int n)
{
	ASObject* value=th->runtime_stack_pop();
	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_CALLS,_("setSuper ") << *name);

	ASObject* obj=th->runtime_stack_pop();

	assert_and_throw(th->inClass)
	assert_and_throw(th->inClass->super);
	assert_and_throw(obj->getClass());
	assert_and_throw(obj->getClass()->isSubClass(th->inClass));

	obj->setVariableByMultiname(*name,value,th->inClass->super.getPtr());
	obj->decRef();
}

void ABCVm::getSuper(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_CALLS,_("getSuper ") << *name);

	ASObject* obj=th->runtime_stack_pop();

	assert_and_throw(th->inClass);
	assert_and_throw(th->inClass->super);
	assert_and_throw(obj->getClass());
	assert_and_throw(obj->getClass()->isSubClass(th->inClass));

	_NR<ASObject> ret = obj->getVariableByMultiname(*name,ASObject::NONE,th->inClass->super.getPtr());
	if(ret.isNull())
	{
		LOG(LOG_NOT_IMPLEMENTED,"getSuper: " << name->normalizedName() << " not found on " << obj->toDebugString());
		ret = _MNR(new Undefined);
	}
	else
	{
		if(ret->is<Definable>())
		{
			ASObject *defined = ret->as<Definable>()->define();
			defined->incRef();
			ret = _MNR(defined);
		}
	}

	obj->decRef();
	ret->incRef();
	th->runtime_stack_push(ret.getPtr());
}

void ABCVm::getLex(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_CALLS, _("getLex: ") << *name );
	vector<scope_entry>::reverse_iterator it=th->scope_stack.rbegin();
	ASObject* o = NULL;

	//Find out the current 'this', when looking up over it, we have to consider all of it
	ASObject* target;
	for(;it!=th->scope_stack.rend();++it)
	{
		// XML_STRICT flag tells getVariableByMultiname to
		// ignore non-existing properties in XML obejcts
		// (normally it would return an empty XMLList if the
		// property does not exist).
		ASObject::GET_VARIABLE_OPTION opt=ASObject::XML_STRICT;
		if(!it->considerDynamic)
			opt=(ASObject::GET_VARIABLE_OPTION)(opt | ASObject::SKIP_IMPL);

		_NR<ASObject> prop=it->object->getVariableByMultiname(*name, opt);
		if(!prop.isNull())
		{
			prop->incRef();
			o=prop.getPtr();
			target=it->object.getPtr();
			break;
		}
	}

	if(o==NULL)
	{
		o=getGlobal()->getVariableAndTargetByMultiname(*name, target);
		if(o==NULL)
		{
			LOG(LOG_NOT_IMPLEMENTED,"getLex: " << *name<< "was not found, pushing Undefined");
			o=new Undefined;
		}
	}

	if(o->is<Definable>())
	{
		LOG(LOG_CALLS,_("Deferred definition of property ") << *name);
		o=o->as<Definable>()->define();
		LOG(LOG_CALLS,_("End of deferred definition of property ") << *name);
	}
	th->runtime_stack_push(o);
	o->incRef();
}

void ABCVm::constructSuper(call_context* th, int m)
{
	LOG(LOG_CALLS, _("constructSuper ") << m);
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	ASObject* obj=th->runtime_stack_pop();

	assert_and_throw(th->inClass);
	assert_and_throw(th->inClass->super);
	assert_and_throw(obj->getClass());
	assert_and_throw(obj->getClass()->isSubClass(th->inClass));
	LOG(LOG_CALLS,_("Super prototype name ") << th->inClass->super->class_name);

	th->inClass->super->handleConstruction(obj,args, m, false);
	LOG(LOG_CALLS,_("End super construct "));

	delete[] args;
}

ASObject* ABCVm::findProperty(call_context* th, multiname* name)
{
	LOG(LOG_CALLS, _("findProperty ") << *name );

	vector<scope_entry>::reverse_iterator it=th->scope_stack.rbegin();
	bool found=false;
	ASObject* ret=NULL;
	for(;it!=th->scope_stack.rend();++it)
	{
		found=it->object->hasPropertyByMultiname(*name, it->considerDynamic);

		if(found)
		{
			//We have to return the object, not the property
			ret=it->object.getPtr();
			break;
		}
	}
	if(!found)
	{
		//try to find a global object where this is defined
		ASObject* target;
		ASObject* o=getGlobal()->getVariableAndTargetByMultiname(*name, target);
		if(o)
			ret=target;
		else //else push the current global object
			ret=th->scope_stack[0].object.getPtr();
	}

	//TODO: make this a regular assert
	assert_and_throw(ret);
	ret->incRef();
	return ret;
}

ASObject* ABCVm::findPropStrict(call_context* th, multiname* name)
{
	LOG(LOG_CALLS, _("findPropStrict ") << *name );

	vector<scope_entry>::reverse_iterator it=th->scope_stack.rbegin();
	bool found=false;
	ASObject* ret=NULL;

	for(;it!=th->scope_stack.rend();++it)
	{
		found=it->object->hasPropertyByMultiname(*name, it->considerDynamic);
		if(found)
		{
			//We have to return the object, not the property
			ret=it->object.getPtr();
			break;
		}
	}
	if(!found)
	{
		ASObject* target;
		ASObject* o=getGlobal()->getVariableAndTargetByMultiname(*name, target);
		if(o)
			ret=target;
		else
		{
			LOG(LOG_NOT_IMPLEMENTED,"findPropStrict: " << *name << " not found, pushing Undefined");
			ret=new Undefined;
		}
	}

	assert_and_throw(ret);
	ret->incRef();
	return ret;
}

bool ABCVm::greaterThan(ASObject* obj1, ASObject* obj2)
{
	LOG(LOG_CALLS,_("greaterThan"));

	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TTRUE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::greaterEquals(ASObject* obj1, ASObject* obj2)
{
	LOG(LOG_CALLS,_("greaterEquals"));

	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TFALSE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::lessEquals(ASObject* obj1, ASObject* obj2)
{
	LOG(LOG_CALLS,_("lessEquals"));

	//Real comparision demanded to object
	bool ret=(obj2->isLess(obj1)==TFALSE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::initProperty(ASObject* obj, ASObject* value, multiname* name)
{
	LOG(LOG_CALLS, _("initProperty ") << *name << ' ' << obj);

	obj->setVariableByMultiname(*name,value);

	obj->decRef();
}

void ABCVm::callSuper(call_context* th, int n, int m, method_info** called_mi, bool keepReturn)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_CALLS,(keepReturn ? "callSuper " : "callSuperVoid ") << *name << ' ' << m);

	ASObject* obj=th->runtime_stack_pop();

	assert_and_throw(th->inClass);
	assert_and_throw(th->inClass->super);
	assert_and_throw(obj->getClass());
	assert_and_throw(obj->getClass()->isSubClass(th->inClass));
	_NR<ASObject> f = obj->getVariableByMultiname(*name,ASObject::NONE,th->inClass->super.getPtr());
	if(!f.isNull())
	{
		f->incRef();
		callImpl(th, f.getPtr(), obj, args, m, called_mi, keepReturn);
	}
	else
	{
		LOG(LOG_ERROR,_("Calling an undefined function ") << name->name_s);
		if(keepReturn)
			th->runtime_stack_push(new Undefined);
	}
	LOG(LOG_CALLS,_("End of callSuper ") << *name);
}

bool ABCVm::isType(ASObject* obj, multiname* name)
{
	bool ret = ABCContext::isinstance(obj, name);
	obj->decRef();
	return ret;
}

bool ABCVm::isTypelate(ASObject* type, ASObject* obj)
{
	LOG(LOG_CALLS,_("isTypelate"));
	bool real_ret=false;

	Class_base* objc=NULL;
	Class_base* c=NULL;
	assert_and_throw(type->getObjectType()==T_CLASS);
	c=static_cast<Class_base*>(type);
	//Special case numeric types
	if(obj->getObjectType()==T_INTEGER || obj->getObjectType()==T_UINTEGER || obj->getObjectType()==T_NUMBER)
	{
		obj->decRef();
		real_ret=(c==Class<Integer>::getClass() || c==Class<Number>::getClass() || c==Class<UInteger>::getClass());
		LOG(LOG_CALLS,_("Numeric type is ") << ((real_ret)?"":_("not ")) << _("subclass of ") << c->class_name);
		type->decRef();
		return real_ret;
	}

	if(obj->classdef)
	{
		assert_and_throw(type->getObjectType()==T_CLASS);

		objc=obj->classdef;
	}
	else if(obj->getObjectType()==T_CLASS)
	{
		assert_and_throw(type->getObjectType()==T_CLASS);

		//Special case for Class
		if(c->class_name.name=="Class" && c->class_name.ns=="")
		{
			type->decRef();
			obj->decRef();
			return true;
		}
		else
		{
			type->decRef();
			obj->decRef();
			return false;
		}
	}
	else
	{
		real_ret=obj->getObjectType()==type->getObjectType();
		LOG(LOG_CALLS,_("isTypelate on non classed object ") << real_ret);
		obj->decRef();
		type->decRef();
		return real_ret;
	}

	real_ret=objc->isSubClass(c);
	LOG(LOG_CALLS,_("Type ") << objc->class_name << _(" is ") << ((real_ret)?"":_("not ")) 
			<< "subclass of " << c->class_name);
	obj->decRef();
	type->decRef();
	return real_ret;
}

ASObject* ABCVm::asType(ASObject* obj, multiname* name)
{
	bool ret = ABCContext::isinstance(obj, name);	
	LOG(LOG_CALLS,_("asType"));
	
	if(ret)
		return obj;
	else
	{
		obj->decRef();
		return new Null;
	}
}

ASObject* ABCVm::asTypelate(ASObject* type, ASObject* obj)
{
	LOG(LOG_CALLS,_("asTypelate"));

	//HACK: until we have implemented all flash classes
	if(type->is<Undefined>())
	{
		LOG(LOG_NOT_IMPLEMENTED,"asTypelate with undefined");
		type->decRef();
		return obj;
	}

	if(!type->is<Class_base>())
	{
		obj->decRef();
		type->decRef();
		throw Class<TypeError>::getInstanceS("Error #1009: Not a type in asTypelate");
	}
	Class_base* c=static_cast<Class_base*>(type);
	//Special case numeric types
	if(obj->getObjectType()==T_INTEGER || obj->getObjectType()==T_UINTEGER || obj->getObjectType()==T_NUMBER)
	{
		bool real_ret=(c==Class<Integer>::getClass() || c==Class<Number>::getClass() || c==Class<UInteger>::getClass());
		LOG(LOG_CALLS,_("Numeric type is ") << ((real_ret)?"":_("not ")) << _("subclass of ") << c->class_name);
		type->decRef();
		if(real_ret)
			return obj;
		else
		{
			obj->decRef();
			return new Null;
		}
	}

	Class_base* objc;
	if(obj->classdef)
		objc=obj->classdef;
	else if(obj->getObjectType()==T_CLASS)
	{
		//Special case for Class
		if(c->class_name.name=="Class" && c->class_name.ns=="")
		{
			type->decRef();
			return obj;
		}
		obj->decRef();
		type->decRef();
		return new Null;
	}
	else
	{
		obj->decRef();
		type->decRef();
		return new Null;
	}

	bool real_ret=objc->isSubClass(c);
	LOG(LOG_CALLS,_("Type ") << objc->class_name << _(" is ") << ((real_ret)?_(" "):_("not ")) 
			<< _("subclass of ") << c->class_name);
	type->decRef();
	if(real_ret)
		return obj;
	else
	{
		obj->decRef();
		return new Null;
	}
}

bool ABCVm::ifEq(ASObject* obj1, ASObject* obj2)
{
	bool ret=obj1->isEqual(obj2);
	LOG(LOG_CALLS,_("ifEq (") << ((ret)?_("taken)"):_("not taken)")));

	//Real comparision demanded to object
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifStrictEq(ASObject* obj2, ASObject* obj1)
{
	bool ret=strictEqualImpl(obj1,obj2);
	LOG(LOG_CALLS,_("ifStrictEq ")<<ret);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifStrictNE(ASObject* obj2, ASObject* obj1)
{
	bool ret=!strictEqualImpl(obj1,obj2);
	LOG(LOG_CALLS,_("ifStrictNE ")<<ret);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::in(ASObject* val2, ASObject* val1)
{
	LOG(LOG_CALLS, _("in") );
	multiname name;
	name.name_type=multiname::NAME_STRING;
	name.name_s=val1->toString();
	name.ns.push_back(nsNameAndKind("",NAMESPACE));
	bool ret=val2->hasPropertyByMultiname(name, true);
	val1->decRef();
	val2->decRef();
	return ret;
}

bool ABCVm::ifFalse(ASObject* obj1)
{
	bool ret=!Boolean_concrete(obj1);
	LOG(LOG_CALLS,_("ifFalse (") << ((ret)?_("taken"):_("not taken")) << ')');

	obj1->decRef();
	return ret;
}

void ABCVm::constructProp(call_context* th, int n, int m)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th);

	LOG(LOG_CALLS,_("constructProp ")<< *name << ' ' << m);

	ASObject* obj=th->runtime_stack_pop();

	_NR<ASObject> o=obj->getVariableByMultiname(*name);

	if(o.isNull())
	{
		for(int i=0;i<m;++i)
			args[i]->decRef();
		obj->decRef();
		throw Class<ReferenceError>::getInstanceS("Error #1065: Variable is not defined.");
	}

	if(o->is<Definable>())
	{
		LOG(LOG_CALLS,_("Deferred definition of property ") << *name);
		ASObject *defined=o->as<Definable>()->define();
		defined->incRef();
		o=_MNR(defined);
		LOG(LOG_CALLS,_("End of deferred definition of property ") << *name);
	}

	LOG(LOG_CALLS,_("Constructing"));
	ASObject* ret;
	if(o->getObjectType()==T_CLASS)
	{
		Class_base* o_class=static_cast<Class_base*>(o.getPtr());
		ret=o_class->getInstance(true,args,m);
	}
	else if(o->getObjectType()==T_FUNCTION)
	{
		ret = constructFunction(th, o->as<IFunction>(), args, m);
	}
	else
		throw RunTimeException("Cannot construct such an object in constructProp");

	th->runtime_stack_push(ret);
	obj->decRef();
	delete[] args;
	LOG(LOG_CALLS,_("End of constructing ") << ret);
}

bool ABCVm::hasNext2(call_context* th, int n, int m)
{
	LOG(LOG_CALLS,"hasNext2 " << n << ' ' << m);
	ASObject* obj=th->locals[n];
	//If the local is not assigned bail out
	if(obj==NULL)
		return false;

	uint32_t curIndex=th->locals[m]->toUInt();

	uint32_t newIndex=obj->nextNameIndex(curIndex);
	th->locals[m]->decRef();
	th->locals[m]=abstract_i(newIndex);
	if(newIndex==0)
	{
		th->locals[n]->decRef();
		th->locals[n]=new Null;
		return false;
	}
	return true;
}

void ABCVm::newObject(call_context* th, int n)
{
	LOG(LOG_CALLS,_("newObject ") << n);
	ASObject* ret=Class<ASObject>::getInstanceS();
	//Duplicated keys overwrite the previous value
	multiname propertyName;
	propertyName.name_type=multiname::NAME_STRING;
	propertyName.ns.push_back(nsNameAndKind("",NAMESPACE));
	for(int i=0;i<n;i++)
	{
		ASObject* value=th->runtime_stack_pop();
		ASObject* name=th->runtime_stack_pop();
		propertyName.name_s=name->toString();
		name->decRef();
		ret->setVariableByMultiname(propertyName, value);
	}

	th->runtime_stack_push(ret);
}

void ABCVm::getDescendants(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_CALLS,"getDescendants " << *name);
	ASObject* obj=th->runtime_stack_pop();
	//The name must be a QName
	assert_and_throw(name->name_type==multiname::NAME_STRING);
	vector<_R<XML> > ret;
	//TODO: support multiname and namespaces
	if(obj->getClass()==Class<XML>::getClass())
	{
		XML* xmlObj=Class<XML>::cast(obj);
		xmlObj->getDescendantsByQName(name->name_s, "", ret);
	}
	else if(obj->getClass()==Class<XMLList>::getClass())
	{
		XMLList* xmlObj=Class<XMLList>::cast(obj);
		xmlObj->getDescendantsByQName(name->name_s, "", ret);
	}
	else
	{
		obj->decRef();
		throw Class<TypeError>::getInstanceS("Only XML and XMLList objects have descendants");
	}
	obj->decRef();
	XMLList* retObj=Class<XMLList>::getInstanceS(ret);
	th->runtime_stack_push(retObj);
}

number_t ABCVm::increment(ASObject* o)
{
	LOG(LOG_CALLS,"increment");

	number_t n=o->toNumber();
	o->decRef();
	return n+1;
}

uint32_t ABCVm::increment_i(ASObject* o)
{
	LOG(LOG_CALLS,_("increment_i"));

	int n=o->toInt();
	o->decRef();
	return n+1;
}

ASObject* ABCVm::nextValue(ASObject* index, ASObject* obj)
{
	LOG(LOG_CALLS,"nextValue");
	if(index->getObjectType()!=T_INTEGER)
		throw UnsupportedException("Type mismatch in nextValue");

	_R<ASObject> ret=obj->nextValue(index->toInt());
	obj->decRef();
	index->decRef();
	ret->incRef();
	return ret.getPtr();
}

ASObject* ABCVm::nextName(ASObject* index, ASObject* obj)
{
	LOG(LOG_CALLS,"nextName");
	if(index->getObjectType()!=T_INTEGER)
		throw UnsupportedException("Type mismatch in nextName");

	_R<ASObject> ret=obj->nextName(index->toInt());
	obj->decRef();
	index->decRef();
	ret->incRef();
	return ret.getPtr();
}

void ABCVm::newClassRecursiveLink(Class_base* target, Class_base* c)
{
	if(c->super)
		newClassRecursiveLink(target, c->super.getPtr());

	const vector<Class_base*>& interfaces=c->getInterfaces();
	for(unsigned int i=0;i<interfaces.size();i++)
	{
		LOG(LOG_CALLS,_("Linking with interface ") << interfaces[i]->class_name);
		interfaces[i]->linkInterface(target);
	}
}

void ABCVm::newClass(call_context* th, int n)
{
	LOG(LOG_CALLS, "newClass " << n );
	method_info* constructor=&th->context->methods[th->context->instances[n].init];
	int name_index=th->context->instances[n].name;
	assert_and_throw(name_index);
	const multiname* mname=th->context->getMultiname(name_index,NULL);

	ASObject* baseClass=th->runtime_stack_pop();

	assert_and_throw(mname->ns.size()==1);
	QName className(mname->name_s,mname->ns[0].name);
	//Check if this class has been already defined
	auto oldDefinition=sys->classes.find(className);
	if(oldDefinition!=sys->classes.end())
	{
		LOG(LOG_CALLS,_("Class ") << className << _(" already defined. Pushing previous definition"));
		baseClass->decRef();
		oldDefinition->second->incRef();
		th->runtime_stack_push(oldDefinition->second);
		return;
	}

	Class_inherit* ret=new Class_inherit(className);
	ret->isFinal = th->context->instances[n].isFinal();
	ret->isSealed = th->context->instances[n].isSealed();
#ifdef PROFILING_SUPPORT
	if(!constructor->validProfName)
	{
		constructor->profName=mname->name_s+"::__CONSTRUCTOR__";
		constructor->validProfName=true;
	}
#endif
	assert_and_throw(th->context);
	ret->context=th->context;

	//Add protected namespace if needed
	if(th->context->instances[n].isProtectedNs())
	{
		ret->use_protected=true;
		int ns=th->context->instances[n].protectedNs;
		const namespace_info& ns_info=th->context->constant_pool.namespaces[ns];
		ret->protected_ns=nsNameAndKind(th->context->getString(ns_info.name),(NS_KIND)(int)ns_info.kind);
	}

	//Null is a "valid" base class
	if(baseClass->getObjectType()!=T_NULL)
	{
		assert_and_throw(baseClass->is<Class_base>());
		Class_base* base = baseClass->as<Class_base>();
		assert(!base->isFinal);
		ret->setSuper(_MR(base));
	}

	ret->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(Class_base::_toString),NORMAL_METHOD,false);

	ret->class_scope=th->scope_stack;
	ret->incRef();
	ret->class_scope.emplace_back(_MR(ret),false);

	LOG(LOG_CALLS,_("Building class traits"));
	for(unsigned int i=0;i<th->context->classes[n].trait_count;i++)
		th->context->buildTrait(ret,&th->context->classes[n].traits[i],false);

	LOG(LOG_CALLS,_("Adding immutable object traits to class"));
	//Class objects also contains all the methods/getters/setters declared for instances
	instance_info* cur=&th->context->instances[n];
	for(unsigned int i=0;i<cur->trait_count;i++)
	{
		int kind=cur->traits[i].kind&0xf;
		if(kind==traits_info::Method || kind==traits_info::Setter || kind==traits_info::Getter)
			th->context->buildTrait(ret,&cur->traits[i],true);
	}

	SyntheticFunction* constructorFunc=Class<IFunction>::getSyntheticFunction(constructor);
	constructorFunc->acquireScope(ret->class_scope);
	constructorFunc->inClass = ret;
	//add Constructor the the class methods
	ret->constructor=constructorFunc;
	ret->class_index=n;

	//Add prototype variable
	ret->prototype = _MNR(new_asobject());
	//Add the constructor variable to the class prototype
	ret->incRef();
	ret->prototype->setVariableByQName("constructor","",ret, DECLARED_TRAIT);
	if(ret->super)
		ret->prototype->setprop_prototype(ret->super->prototype);
	ret->addPrototypeGetter();

	//add implemented interfaces
	for(unsigned int i=0;i<th->context->instances[n].interface_count;i++)
	{
		multiname* name=th->context->getMultiname(th->context->instances[n].interfaces[i],NULL);
		ret->addImplementedInterface(*name);

		//Make the class valid if needed
		ASObject* target;
		ASObject* obj=getGlobal()->getVariableAndTargetByMultiname(*name, target);

		//Named only interfaces seems to be allowed 
		if(obj==NULL)
			continue;

		if(obj->is<Definable>())
		{
			LOG(LOG_CALLS,_("Class ") << *name << _(" is not yet valid (as interface)"));
			obj=obj->as<Definable>()->define();
			LOG(LOG_CALLS,_("End of deferred init of class ") << *name);
			assert_and_throw(obj);
		}
	}
	//If the class is not an interface itself, link the traits
	if(!th->context->instances[n].isInterface())
	{
		//Link all the interfaces for this class and all the bases
		newClassRecursiveLink(ret, ret);
	}

	LOG(LOG_CALLS,_("Calling Class init ") << ret);
	ret->incRef();
	//Class init functions are called with global as this
	method_info* m=&th->context->methods[th->context->classes[n].cinit];
	SyntheticFunction* cinit=Class<IFunction>::getSyntheticFunction(m);
	//cinit must inherit the current scope
	cinit->acquireScope(th->scope_stack);
	ASObject* ret2=cinit->call(ret,NULL,0);
	assert_and_throw(ret2->is<Undefined>());
	ret2->decRef();
	LOG(LOG_CALLS,_("End of Class init ") << ret);
	th->runtime_stack_push(ret);
	cinit->decRef();
}

void ABCVm::swap()
{
	LOG(LOG_CALLS,_("swap"));
}

ASObject* ABCVm::newActivation(call_context* th,method_info* info)
{
	LOG(LOG_CALLS,_("newActivation"));
	//TODO: Should create a real activation object
	//TODO: Should method traits be added to the activation context?
	ASObject* act=Class<ASObject>::getInstanceS();
#ifndef NDEBUG
	act->initialized=false;
#endif
	for(unsigned int i=0;i<info->body->trait_count;i++)
		th->context->buildTrait(act,&info->body->traits[i],false);
#ifndef NDEBUG
	act->initialized=true;
#endif

	return act;
}

void ABCVm::popScope(call_context* th)
{
	LOG(LOG_CALLS,_("popScope"));
	th->scope_stack.pop_back();
}

bool ABCVm::lessThan(ASObject* obj1, ASObject* obj2)
{
	LOG(LOG_CALLS,_("lessThan"));

	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2)==TTRUE);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::call(call_context* th, int m, method_info** called_mi)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	ASObject* obj=th->runtime_stack_pop();
	ASObject* f=th->runtime_stack_pop();
	LOG(LOG_CALLS,_("call ") << m << ' ' << f);
	callImpl(th, f, obj, args, m, called_mi, true);
}

void ABCVm::callImpl(call_context* th, ASObject* f, ASObject* obj, ASObject** args, int m, method_info** called_mi, bool keepReturn)
{
	if(f->is<Function>())
	{
		IFunction* func=f->as<Function>();
		func->incRef();
		ASObject* ret=func->call(obj,args,m);
		//call getMethodInfo only after the call, so it's updated
		if(called_mi)
			*called_mi=func->getMethodInfo();
		func->decRef();
		if(keepReturn)
			th->runtime_stack_push(ret);
		else
			ret->decRef();
	}
	else if(f->is<Class_base>())
	{
		obj->decRef();
		Class_base* c=f->as<Class_base>();
		ASObject* ret=c->generator(args,m);
		assert_and_throw(ret);
		if(keepReturn)
			th->runtime_stack_push(ret);
		else
			ret->decRef();
	}
	else
	{
		obj->decRef();
		for(int i=0;i<m;++i)
			args[i]->decRef();
		//HACK: Until we have implemented the whole flash api
		//we silently ignore calling undefined functions
		if(f->is<Undefined>())
		{
			if(keepReturn)
				th->runtime_stack_push(new Undefined);
		}
		else
			throw Class<TypeError>::getInstanceS("Error #1006: Tried to call something that is not a function");
	}
	LOG(LOG_CALLS,_("End of call ") << m << ' ' << f);
	delete[] args;
}

bool ABCVm::deleteProperty(ASObject* obj, multiname* name)
{
	LOG(LOG_CALLS,_("deleteProperty ") << *name);
	bool ret = obj->deleteVariableByMultiname(*name);

	obj->decRef();
	return ret;
}

ASObject* ABCVm::newFunction(call_context* th, int n)
{
	LOG(LOG_CALLS,_("newFunction ") << n);

	method_info* m=&th->context->methods[n];
	SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(m);
	f->func_scope=th->scope_stack;

	//Bind the function to null, as this is not a class method
	f->bind(NullRef,-1);
	return f;
}

ASObject* ABCVm::getScopeObject(call_context* th, int n)
{
	ASObject* ret=th->scope_stack[n+th->initialScopeStack].object.getPtr();
	ret->incRef();
	LOG(LOG_CALLS, _("getScopeObject: ") << ret );
	return ret;
}

ASObject* ABCVm::pushString(call_context* th, int n)
{
	tiny_string s=th->context->getString(n); 
	LOG(LOG_CALLS, _("pushString ") << s );
	return Class<ASString>::getInstanceS(s);
}

ASObject* ABCVm::newCatch(call_context* th, int n)
{
	ASObject* catchScope = Class<ASObject>::getInstanceS();
	assert_and_throw(n >= 0 && (unsigned int)n < th->mi->body->exception_count);
	multiname* name = th->context->getMultiname(th->mi->body->exceptions[n].var_name, th);
	catchScope->setVariableByMultiname(*name, new Undefined);
	catchScope->initSlot(1, *name);
	return catchScope;
}

void ABCVm::newArray(call_context* th, int n)
{
	LOG(LOG_CALLS, _("newArray ") << n );
	Array* ret=Class<Array>::getInstanceS();
	ret->resize(n);
	for(int i=0;i<n;i++)
		ret->set(n-i-1,th->runtime_stack_pop());

	th->runtime_stack_push(ret);
}

ASObject* ABCVm::esc_xattr(ASObject* o)
{
	/* TODO: implement correct escaping according to E4X
	 * For now we just cut the string at the first \0 byte, which is wrong
	 * but suppresses more errors */
	tiny_string t = o->toString();
	LOG(LOG_NOT_IMPLEMENTED,"esc_xattr on " << t);
	o->decRef();
	auto i=t.begin();
	for(;i!=t.end();++i)
	{
		if(*i == '\0')
			break;
	}
	if(i == t.end())
		return Class<ASString>::getInstanceS(t);
	else
		return Class<ASString>::getInstanceS(t.substr(0,i));
}
