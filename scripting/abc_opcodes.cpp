/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

uintptr_t ABCVm::bitAnd(ASObject* val2, ASObject* val1)
{
	uintptr_t i1=val1->toUInt();
	uintptr_t i2=val2->toUInt();
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("bitAnd_oo ") << hex << i1 << '&' << i2);
	return i1&i2;
}

uintptr_t ABCVm::bitAnd_oi(ASObject* val1, intptr_t val2)
{
	uintptr_t i1=val1->toUInt();
	uintptr_t i2=val2;
	val1->decRef();
	LOG(LOG_CALLS,_("bitAnd_oi ") << hex << i1 << '&' << i2);
	return i1&i2;
}

void ABCVm::setProperty(ASObject* value,ASObject* obj,multiname* name)
{
	LOG(LOG_CALLS,_("setProperty ") << *name << ' ' << obj);

	//We have to reset the level before finding a variable
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	if(tl.cur_this==obj)
		obj->resetLevel();

	obj->setVariableByMultiname(*name,value);
	if(tl.cur_this==obj)
		obj->setLevel(tl.cur_level);

	obj->decRef();
}

void ABCVm::setProperty_i(intptr_t value,ASObject* obj,multiname* name)
{
	LOG(LOG_CALLS,_("setProperty_i ") << *name << ' ' <<obj);

	obj->setVariableByMultiname_i(*name,value);
	obj->decRef();
}

number_t ABCVm::convert_d(ASObject* o)
{
	LOG(LOG_CALLS, _("convert_d") );
	if(o->getObjectType()!=T_UNDEFINED)
	{
		number_t ret=o->toNumber();
		o->decRef();
		return ret;
	}
	else
		return numeric_limits<double>::quiet_NaN();
}

bool ABCVm::convert_b(ASObject* o)
{
	LOG(LOG_CALLS, _("convert_b") );
	bool ret=Boolean_concrete(o);
	o->decRef();
	return ret;
}

uintptr_t ABCVm::convert_u(ASObject* o)
{
	LOG(LOG_CALLS, _("convert_u") );
	uintptr_t ret=o->toUInt();
	o->decRef();
	return ret;
}

intptr_t ABCVm::convert_i(ASObject* o)
{
	LOG(LOG_CALLS, _("convert_i") );
	intptr_t ret=o->toInt();
	o->decRef();
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
	LOG(LOG_NOT_IMPLEMENTED,"checkfilter");
	return o;
}

ASObject* ABCVm::coerce_s(ASObject* o)
{
	LOG(LOG_CALLS, _("coerce_s") );
	ASObject* ret=o;

	if(o->getObjectType()!=T_STRING)
	{
		if(o->getObjectType()==T_UNDEFINED || o->getObjectType()==T_NULL)
			ret=new Null;
		else
			ret=Class<ASString>::getInstanceS(o->toString(false));

		o->decRef();
	}
	return ret;
}

void ABCVm::coerce(call_context* th, int n)
{
	LOG(LOG_CALLS,_("coerce ") << *th->context->getMultiname(n,NULL));
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
	LOG(LOG_CALLS,_("getLocal[") << n << _("] (") << o << _(") ") << o->toString(true));
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
	LOG(LOG_CALLS,_("setLocal[") << n << _("] = ") << v->toString(true));
}

intptr_t ABCVm::pushShort(intptr_t n)
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
	LOG(LOG_CALLS,"getSlot " << n << " " << ret->toString(true));
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

uintptr_t ABCVm::bitNot(ASObject* val)
{
	uintptr_t i1=val->toUInt();
	val->decRef();
	LOG(LOG_CALLS,_("bitNot ") << hex << i1);
	return ~i1;
}

uintptr_t ABCVm::bitXor(ASObject* val2, ASObject* val1)
{
	int i1=val1->toUInt();
	int i2=val2->toUInt();
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("bitXor ") << hex << i1 << '^' << i2);
	return i1^i2;
}

uintptr_t ABCVm::bitOr_oi(ASObject* val2, uintptr_t val1)
{
	int i1=val1;
	int i2=val2->toUInt();
	val2->decRef();
	LOG(LOG_CALLS,_("bitOr ") << hex << i1 << '|' << i2);
	return i1|i2;
}

uintptr_t ABCVm::bitOr(ASObject* val2, ASObject* val1)
{
	int i1=val1->toUInt();
	int i2=val2->toUInt();
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("bitOr ") << hex << i1 << '|' << i2);
	return i1|i2;
}

void ABCVm::callProperty(call_context* th, int n, int m)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_CALLS,_("callProperty ") << *name << ' ' << m);

	ASObject* obj=th->runtime_stack_pop();
	if(obj->prototype)
		LOG(LOG_CALLS,obj->prototype->class_name);

	//We have to reset the level, as we may be getting a function defined at a lower level
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	if(tl.cur_this==obj)
		obj->resetLevel();

	//We should skip the special implementation of get
	ASObject* o=obj->getVariableByMultiname(*name, true);

	if(tl.cur_this==obj)
		obj->setLevel(tl.cur_level);

	if(o)
	{
		//Run the deferred initialization if needed
		if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(LOG_CALLS,_("We got a function not yet valid"));
			Definable* d=static_cast<Definable*>(o);
			d->define(obj);
			o=obj->getVariableByMultiname(*name);
		}

		//If o is already a function call it
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=static_cast<IFunction*>(o);
			//Methods has to be runned with their own class this
			//The owner has to be increffed
			obj->incRef();
			ASObject* ret=f->call(obj,args,m);
			th->runtime_stack_push(ret);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(LOG_NOT_IMPLEMENTED,_("We got a Undefined function on obj ") << ((obj->prototype)?obj->prototype->class_name.name:_("Object")));
			th->runtime_stack_push(new Undefined);
		}
		else if(o->getObjectType()==T_CLASS)
		{
			Class_base* c=static_cast<Class_base*>(o);
			ASObject* ret=c->generator(args, m);
			th->runtime_stack_push(ret);
		}
		else
			throw UnsupportedException("Unexpected object to call");
	}
	else
	{
		//If the object is a Proxy subclass, try to invoke callProperty
		if(obj->prototype && obj->prototype->isSubClass(Class<Proxy>::getClass()))
		{
			//Check if there is a custom caller defined, skipping implementation to avoid recursive calls
			multiname callPropertyName;
			callPropertyName.name_type=multiname::NAME_STRING;
			callPropertyName.name_s="callProperty";
			callPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
			ASObject* o=obj->getVariableByMultiname(callPropertyName,true);

			if(o)
			{
				assert_and_throw(o->getObjectType()==T_FUNCTION);

				IFunction* f=static_cast<IFunction*>(o);

				//Create a new array
				ASObject** proxyArgs=new ASObject*[m+1];
				//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
				proxyArgs[0]=Class<ASString>::getInstanceS(name->name_s);
				for(int i=0;i<m;i++)
					proxyArgs[i+1]=args[i];
				delete[] args;

				//We now suppress special handling
				LOG(LOG_CALLS,_("Proxy::callProperty"));
				ASObject* ret=f->call(obj,proxyArgs,m+1);
				th->runtime_stack_push(ret);

				obj->decRef();
				LOG(LOG_CALLS,_("End of calling ") << *name);
				delete[] proxyArgs;
				return;
			}
		}

		LOG(LOG_NOT_IMPLEMENTED,_("Calling an undefined function ") << *name << _(" on obj ") << 
				((obj->prototype)?obj->prototype->class_name.name:"Object"));
		th->runtime_stack_push(new Undefined);
	}
	LOG(LOG_CALLS,_("End of calling ") << *name);

	obj->decRef();
	delete[] args;
}

intptr_t ABCVm::getProperty_i(ASObject* obj, multiname* name)
{
	LOG(LOG_CALLS, _("getProperty_i ") << *name );

	//TODO: implement exception handling to find out if no integer can be returned
	intptr_t ret=obj->getVariableByMultiname_i(*name);

	obj->decRef();
	return ret;
}

ASObject* ABCVm::getProperty(ASObject* obj, multiname* name)
{
	LOG(LOG_CALLS, _("getProperty ") << *name << ' ' << obj);

	//We have to reset the level before finding a variable
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	if(tl.cur_this==obj)
		obj->resetLevel();

	ASObject* ret=obj->getVariableByMultiname(*name);

	if(tl.cur_this==obj)
		obj->setLevel(tl.cur_level);
	
	if(ret==NULL)
	{
		if(obj->prototype)
		{
			LOG(LOG_NOT_IMPLEMENTED,_("Property not found ") << *name << _(" on type ") << obj->prototype->class_name);
		}
		else
		{
			LOG(LOG_NOT_IMPLEMENTED,_("Property not found ") << *name);
		}
		return new Undefined;
	}
	else
	{
		//If we are getting a function object attach the the current scope
		if(ret->getObjectType()==T_FUNCTION)
		{
			//TODO: maybe also the level should be binded
			LOG(LOG_CALLS,_("Attaching this to function ") << name);
			IFunction* f=static_cast<IFunction*>(ret)->bind(obj,-1);
			obj->decRef();
			//No incref is needed, as the function is a new instance
			return f;
		}
		else if(ret->getObjectType()==T_DEFINABLE)
		{
			//LOG(ERROR,_("Property ") << name << _(" is not yet valid"));
			throw UnsupportedException("Definable not supported in getProperty");
			/*Definable* d=static_cast<Definable*>(ret.obj);
			d->define(obj);
			ret=obj->getVariableByMultiname(*name,owner);
			ret->incRef();*/
		}
	}
	ret->incRef();
	obj->decRef();
	return ret;
}

number_t ABCVm::divide(ASObject* val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED ||
		val2->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(LOG_NOT_IMPLEMENTED,_("divide: HACK"));
		return 0;
	}
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
	th->scope_stack.push_back(t);
}

void ABCVm::pushScope(call_context* th)
{
	ASObject* t=th->runtime_stack_pop();
	LOG(LOG_CALLS, _("pushScope ") << t );
	th->scope_stack.push_back(t);
}

ASObject* ABCVm::getGlobalScope(call_context* th)
{
	ASObject* ret=th->scope_stack[0];
	LOG(LOG_CALLS,_("getGlobalScope: ") << ret);
	ret->incRef();
	return ret;
}

number_t ABCVm::decrement(ASObject* o)
{
	LOG(LOG_CALLS,_("decrement"));

	number_t n=o->toNumber();
	o->decRef();
	return n-1;
}

uintptr_t ABCVm::decrement_i(ASObject* o)
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

bool ABCVm::ifLT_oi(ASObject* obj2, intptr_t val1)
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

bool ABCVm::ifLT_io(intptr_t val2, ASObject* obj1)
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

bool ABCVm::ifNE_oi(ASObject* obj1, intptr_t val2)
{
	//HACK
	if(obj1->getObjectType()==T_UNDEFINED)
		return false;
	bool ret=obj1->toInt()!=val2;
	LOG(LOG_CALLS,_("ifNE (") << ((ret)?_("taken)"):_("not taken)")));

	obj1->decRef();
	return ret;
}

intptr_t ABCVm::pushByte(intptr_t n)
{
	LOG(LOG_CALLS, _("pushByte ") << n );
	return n;
}

number_t ABCVm::multiply_oi(ASObject* val2, intptr_t val1)
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
		LOG(LOG_NOT_IMPLEMENTED,_("Cannot increment type ") << th->locals[n]->getObjectType());
	}

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
			SyntheticFunction* sf=dynamic_cast<SyntheticFunction*>(obj);
			assert_and_throw(sf);
			ret=Class<ASObject>::getInstanceS();
			if(sf->mi->body)
			{
#ifndef NDEBUG
				ret->initialized=false;
#endif
				LOG(LOG_CALLS,_("Building method traits"));
				for(unsigned int i=0;i<sf->mi->body->trait_count;i++)
					th->context->buildTrait(ret,&sf->mi->body->traits[i],false);
#ifndef NDEBUG
				ret->initialized=true;
#endif
				ret->incRef();
				assert_and_throw(sf->closure_this==NULL);
				ASObject* ret2=sf->call(ret,args,m);
				if(ret2)
					ret2->decRef();

				//Let's see if an AS prototype has been defined on the function
				multiname prototypeName;
				prototypeName.name_type=multiname::NAME_STRING;
				prototypeName.name_s="prototype";
				prototypeName.ns.push_back(nsNameAndKind("",NAMESPACE));
				ASObject* asp=sf->getVariableByMultiname(prototypeName,true);
				if(asp)
					asp->incRef();

				//Now add our prototype
				sf->incRef();
				ret->setPrototype(new Class_function(sf,asp));
			}
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
//	throw UnsupportedException("constructGenericType not implement");
	LOG(LOG_CALLS, _("constructGenericType ") << m);
	assert_and_throw(m==1);
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	ASObject* obj=th->runtime_stack_pop();

/*	if(obj->getObjectType()==T_DEFINABLE)
	{
		LOG(LOG_ERROR,_("Check"));
		abort();
		LOG(LOG_CALLS,_("Deferred definition of property ") << name);
		Definable* d=static_cast<Definable*>(o);
		d->define(obj);
		o=obj->getVariableByMultiname(name,owner);
		LOG(LOG_CALLS,_("End of deferred definition of property ") << name);
	}

	LOG(LOG_CALLS,_("Constructing"));
	Class_base* o_class=static_cast<Class_base*>(obj);
	assert_and_throw(o_class->getObjectType()==T_CLASS);
	ASObject* ret=o_class->getInstance(true,args,m);*/
	ASObject* ret=new Undefined;

	obj->decRef();
	LOG(LOG_CALLS,_("End of constructing"));
	th->runtime_stack_push(ret);
	//TODO: remove when implemented
	for(int i=0;i<m;i++)
		args[i]->decRef();
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
			ret="object";
			break;
		case T_BOOLEAN:
			ret="boolean";
			break;
		case T_NUMBER:
		case T_INTEGER:
			ret="number";
			break;
		case T_STRING:
			ret="string";
			break;
		case T_FUNCTION:
			ret="function";
			break;
		default:
			return new Undefined;
	}
	obj->decRef();
	return Class<ASString>::getInstanceS(ret);
}

void ABCVm::callPropVoid(call_context* th, int n, int m)
{
	multiname* name=th->context->getMultiname(n,th); 
	LOG(LOG_CALLS,"callPropVoid " << *name << ' ' << m);

	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();
	ASObject* obj=th->runtime_stack_pop();

	if(obj->prototype)
		LOG(LOG_CALLS, obj->prototype->class_name);

	//We have to reset the level, as we may be getting a function defined at a lower level
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	if(tl.cur_this==obj)
		obj->resetLevel();

	//We should skip the special implementation of get
	ASObject* o=obj->getVariableByMultiname(*name,true);

	if(tl.cur_this==obj)
		obj->setLevel(tl.cur_level);

	if(o)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=static_cast<IFunction*>(o);
			obj->incRef();

			ASObject* ret=f->call(obj,args,m);
			if(ret)
				ret->decRef();
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(LOG_NOT_IMPLEMENTED,_("Calling an undefined function ") << *name << _(" on obj ") << 
					((obj->prototype)?obj->prototype->class_name.name:"Object"));
		}
		else
			throw UnsupportedException("Not callable object in callPropVoid");
	}
	else
	{
		//If the object is a Proxy subclass, try to use callProperty
		if(obj->prototype && obj->prototype->isSubClass(Class<Proxy>::getClass()))
		{
			//Check if there is a custom caller defined, skipping implementation to avoid recursive calls
			multiname callPropertyName;
			callPropertyName.name_type=multiname::NAME_STRING;
			callPropertyName.name_s="callProperty";
			callPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
			ASObject* o=obj->getVariableByMultiname(callPropertyName,true);
			if(o)
			{
				assert_and_throw(o->getObjectType()==T_FUNCTION);

				IFunction* f=static_cast<IFunction*>(o);

				//Create a new array
				ASObject** proxyArgs=new ASObject*[m+1];
				//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
				proxyArgs[0]=Class<ASString>::getInstanceS(name->name_s);
				for(int i=0;i<m;i++)
					proxyArgs[i+1]=args[i];
				delete[] args;

				//We now suppress special handling
				LOG(LOG_CALLS,_("Proxy::callProperty"));
				ASObject* ret=f->call(obj,proxyArgs,m+1);
				if(ret)
					ret->decRef();

				//No need to decRef obj, as it has been passed to the function
				LOG(LOG_CALLS,_("End of calling ") << *name);
				delete[] proxyArgs;
				return;
			}
		}

		if(obj->prototype)
		{
			LOG(LOG_NOT_IMPLEMENTED,_("We got a Undefined function ") << name->name_s << _(" on obj type ") << obj->prototype->class_name);
		}
		else
		{
			LOG(LOG_NOT_IMPLEMENTED,_("We got a Undefined function ") << name->name_s);
		}
	}

	obj->decRef();
	delete[] args;
	LOG(LOG_CALLS,_("End of calling ") << *name);
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

intptr_t ABCVm::modulo(ASObject* val1, ASObject* val2)
{
	uint32_t num1=val1->toUInt();
	uint32_t num2=val2->toUInt();

	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("modulo ")  << num1 << '%' << num2);
	return num1%num2;
}

number_t ABCVm::subtract_oi(ASObject* val2, intptr_t val1)
{
	int num2=val2->toInt();
	int num1=val1;

	val2->decRef();
	LOG(LOG_CALLS,_("subtract_oi ") << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_do(number_t val2, ASObject* val1)
{
	assert_and_throw(val1->getObjectType()!=T_UNDEFINED);
	number_t num2=val2;
	number_t num1=val1->toNumber();

	val1->decRef();
	LOG(LOG_CALLS,_("subtract_do ") << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_io(intptr_t val2, ASObject* val1)
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

number_t ABCVm::subtract(ASObject* val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED ||
		val2->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(LOG_NOT_IMPLEMENTED,_("subtract: HACK"));
		return 0;
	}
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
	//Implement ECMA add algorithm, for XML and default
	if(val1->getObjectType()==T_INTEGER && val2->getObjectType()==T_INTEGER)
	{
		intptr_t num2=val2->toInt();
		intptr_t num1=val1->toInt();
		LOG(LOG_CALLS,_("add ") << num1 << '+' << num2);
		val1->decRef();
		val2->decRef();
		return abstract_i(num1+num2);
	}
	else if(val1->getObjectType()==T_ARRAY)
	{
		//Array concatenation
		Array* ar=static_cast<Array*>(val1);
		ar->push(val2);
		return val1;
	}
	else if(val1->getObjectType()==T_STRING || val2->getObjectType()==T_STRING)
	{
		string a(val1->toString().raw_buf());
		string b(val2->toString().raw_buf());
		LOG(LOG_CALLS,_("add ") << a << '+' << b);
		val1->decRef();
		val2->decRef();
		return Class<ASString>::getInstanceS(a+b);
	}
	else if(val1->getObjectType()==T_NUMBER || val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1->toNumber();
		LOG(LOG_CALLS,_("add ") << num1 << '+' << num2);
		val1->decRef();
		val2->decRef();
		return abstract_d(num1+num2);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,_("Add between types ") << val1->getObjectType() << ' ' << val2->getObjectType());
		return new Undefined;
	}

}

ASObject* ABCVm::add_oi(ASObject* val2, intptr_t val1)
{
	//Implement ECMA add algorithm, for XML and default
	if(val2->getObjectType()==T_INTEGER)
	{
		Integer* ip=static_cast<Integer*>(val2);
		intptr_t num2=ip->val;
		intptr_t num1=val1;
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
		tiny_string a((int32_t)val1);
		const tiny_string& b=val2->toString();
		val2->decRef();
		LOG(LOG_CALLS,_("add ") << a << '+' << b);
		return Class<ASString>::getInstanceS(a+b);
	}
	else if(val2->getObjectType()==T_ARRAY)
	{
		throw UnsupportedException("Array add not supported in add_oi");
		/*//Array concatenation
		ASArray* ar=static_cast<ASArray*>(val1);
		ar->push(val2);
		return ar;*/
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,_("Add between integer and ") << val2->getObjectType());
		return new Undefined;
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
		tiny_string a(val1);
		const tiny_string& b=val2->toString();
		val2->decRef();
		LOG(LOG_CALLS,_("add ") << a << '+' << b);
		return Class<ASString>::getInstanceS(a+b);
	}
	else if(val2->getObjectType()==T_ARRAY)
	{
		throw UnsupportedException("Array add not supported in add_od");
		/*//Array concatenation
		ASArray* ar=static_cast<ASArray*>(val1);
		ar->push(val2);
		return ar;*/
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,_("Add between types number and ") << val2->getObjectType());
		return new Undefined;
	}

}

uintptr_t ABCVm::lShift(ASObject* val1, ASObject* val2)
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

uintptr_t ABCVm::lShift_io(uintptr_t val1, ASObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1&0x1f;
	val2->decRef();
	LOG(LOG_CALLS,_("lShift ")<<hex<<i2<<_("<<")<<i1<<dec);
	//Left shift are supposed to always work in 32bit
	uint32_t ret=i2<<i1;
	return ret;
}

uintptr_t ABCVm::rShift(ASObject* val1, ASObject* val2)
{
	int32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("rShift ")<<hex<<i2<<_(">>")<<i1<<dec);
	return i2>>i1;
}

uintptr_t ABCVm::urShift(ASObject* val1, ASObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(LOG_CALLS,_("urShift ")<<hex<<i2<<_(">>")<<i1<<dec);
	return i2>>i1;
}

uintptr_t ABCVm::urShift_io(uintptr_t val1, ASObject* val2)
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

	//We modify the cur_level of obj
	obj->decLevel();

	obj->setVariableByMultiname(*name, value);

	//And the reset it using the stack
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	//What if using [sg]etSuper not on this??
	assert_and_throw(tl.cur_this==obj);
	tl.cur_this->setLevel(tl.cur_level);

	obj->decRef();
}

void ABCVm::getSuper(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th); 
	LOG(LOG_CALLS,_("getSuper ") << *name);

	ASObject* obj=th->runtime_stack_pop();

	thisAndLevel tl=getVm()->getCurObjAndLevel();
	//What if using [sg]etSuper not on this??
	assert_and_throw(tl.cur_this==obj);

	//We modify the cur_level of obj
	obj->decLevel();

	//Should we skip implementation? I think it's reasonable
	ASObject* o=obj->getVariableByMultiname(*name, true);

	tl=getVm()->getCurObjAndLevel();
	//What if using [sg]etSuper not on this??
	assert_and_throw(tl.cur_this==obj);
	//And the reset it using the stack
	tl.cur_this->setLevel(tl.cur_level);

	if(o)
	{
		if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(LOG_CALLS,_("We got an object not yet valid"));
			Definable* d=static_cast<Definable*>(o);
			d->define(obj);
			o=obj->getVariableByMultiname(*name);
		}
		o->incRef();
		th->runtime_stack_push(o);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,_("NOT found ") << name->name_s<< _(", pushing Undefined"));
		th->runtime_stack_push(new Undefined);
	}

	obj->decRef();
}

void ABCVm::getLex(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_CALLS, _("getLex: ") << *name );
	vector<ASObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ASObject* o;

	//Find out the current 'this', when looking up over it, we have to consider all of it
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	ASObject* target;
	for(;it!=th->scope_stack.rend();++it)
	{
		if(*it==tl.cur_this)
			tl.cur_this->resetLevel();

		//Skip implementation
		ASObject* tmpo=(*it)->getVariableByMultiname(*name, true);
		if(*it==tl.cur_this)
			tl.cur_this->setLevel(tl.cur_level);

		o=tmpo;
		if(o)
		{
			target=*it;
			break;
		}
	}

	if(o==NULL)
	{
		LOG(LOG_CALLS, _("NOT found, trying Global") );
		o=getGlobal()->getVariableAndTargetByMultiname(*name, target);
		if(o==NULL)
		{
			LOG(LOG_NOT_IMPLEMENTED,_("NOT found ") << name->name_s<< _(", pushing Undefined"));
			o=new Undefined;
		}
	}

	//If we are getting a function object attach the the current scope
	if(o->getObjectType()==T_FUNCTION)
	{
		LOG(LOG_CALLS,_("Attaching this to function ") << name);
		IFunction* f=static_cast<IFunction*>(o)->bind(target,-1);
		o=f;
	}
	else if(o->getObjectType()==T_DEFINABLE)
	{
		LOG(LOG_CALLS,_("Deferred definition of property ") << *name);
		Definable* d=static_cast<Definable*>(o);
		d->define(target);
		o=target->getVariableByMultiname(*name);
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

	assert_and_throw(obj->getLevel()!=0);

	thisAndLevel tl=getVm()->getCurObjAndLevel();
	//Check that current 'this' is the object
	assert_and_throw(tl.cur_this==obj);
	assert_and_throw(tl.cur_level==obj->getLevel());

	LOG(LOG_CALLS,_("Cur prototype name ") << obj->getActualPrototype()->class_name);
	//Change current level
	obj->decLevel();
	LOG(LOG_CALLS,_("Super prototype name ") << obj->getActualPrototype()->class_name);

	Class_base* curProt=obj->getActualPrototype();
	curProt->handleConstruction(obj,args, m, false);
	LOG(LOG_CALLS,_("End super construct "));
	obj->setLevel(tl.cur_level);

	obj->decRef();
	delete[] args;
}

ASObject* ABCVm::findProperty(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_CALLS, _("findProperty ") << *name );

	vector<ASObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ASObject* o=NULL;
	ASObject* ret=NULL;
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	for(;it!=th->scope_stack.rend();++it)
	{
		if(*it==tl.cur_this)
			tl.cur_this->resetLevel();

		//Skip implementation
		o=(*it)->getVariableByMultiname(*name, true);
		if(*it==tl.cur_this)
			tl.cur_this->setLevel(tl.cur_level);

		if(o)
		{
			//We have to return the object, not the property
			ret=*it;
			break;
		}
	}
	if(o==NULL)
	{
		LOG(LOG_CALLS, _("NOT found, pushing global") );
		ret=th->scope_stack[0];
	}

	assert_and_throw(ret);
	ret->incRef();
	return ret;
}

ASObject* ABCVm::findPropStrict(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_CALLS, _("findPropStrict ") << *name );

	vector<ASObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ASObject* o=NULL;
	ASObject* ret=NULL;
	thisAndLevel tl=getVm()->getCurObjAndLevel();

	for(;it!=th->scope_stack.rend();++it)
	{
		if(*it==tl.cur_this)
			tl.cur_this->resetLevel();
		//Skip special behaviour
		o=(*it)->getVariableByMultiname(*name, true);
		if(*it==tl.cur_this)
			tl.cur_this->setLevel(tl.cur_level);
		if(o)
		{
			//We have to return the object, not the property
			ret=*it;
			break;
		}
	}
	if(o==NULL)
	{
		LOG(LOG_CALLS, _("NOT found, trying Global") );
		ASObject* target;
		o=getGlobal()->getVariableAndTargetByMultiname(*name, target);
		if(o)
			ret=target;
		if(o==NULL)
		{
			LOG(LOG_NOT_IMPLEMENTED, _("NOT found, pushing Undefined"));
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

void ABCVm::initProperty(call_context* th, int n)
{
	ASObject* value=th->runtime_stack_pop();
	multiname* name=th->context->getMultiname(n,th);
	ASObject* obj=th->runtime_stack_pop();

	LOG(LOG_CALLS, _("initProperty ") << *name << ' ' << obj);

	//We have to reset the level before finding a variable
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	if(tl.cur_this==obj)
		obj->resetLevel();

	obj->setVariableByMultiname(*name,value);
	if(tl.cur_this==obj)
		obj->setLevel(tl.cur_level);

	obj->decRef();
}

void ABCVm::callSuper(call_context* th, int n, int m)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th); 
	LOG(LOG_CALLS,_("callSuper ") << *name << ' ' << m);

	ASObject* obj=th->runtime_stack_pop();

	//We modify the cur_level of obj
	obj->decLevel();

	//We should skip the special implementation of get
	ASObject* o=obj->getVariableByMultiname(*name, true);

	//And the reset it using the stack
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	//What if using [sg]etSuper not on this??
	assert_and_throw(tl.cur_this==obj);
	tl.cur_this->setLevel(tl.cur_level);

	if(o)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=static_cast<IFunction*>(o);
			obj->incRef();
			ASObject* ret=f->call(obj,args,m);
			th->runtime_stack_push(ret);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(LOG_NOT_IMPLEMENTED,_("We got a Undefined function"));
			th->runtime_stack_push(new Undefined);
		}
/*		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(LOG_CALLS,_("We got a function not yet valid"));
			Definable* d=static_cast<Definable*>(o);
			d->define(obj);
			IFunction* f=obj->getVariableByMultiname(*name)->toFunction();
			if(f)
			{
				ASObject* ret=f->call(owner,args,m);
				th->runtime_stack_push(ret);
			}
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,_("No such function"));
				th->runtime_stack_push(new Undefined);
			}
		}
		else
		{
			//IFunction* f=static_cast<IFunction*>(o->getVariableByQName("Call","",owner));
			//if(f)
			//{
			//	ASObject* ret=f->call(o,args,m);
			//	th->runtime_stack_push(ret);
			//}
			//else
			//{
			//	LOG(LOG_NOT_IMPLEMENTED,_("No such function, returning Undefined"));
			//	th->runtime_stack_push(new Undefined);
			//}
		}*/
		else
			throw UnsupportedException("Not callable object in callSuper");
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,_("Calling an undefined function ") << name->name_s);
		th->runtime_stack_push(new Undefined);
	}
	LOG(LOG_CALLS,_("End of callSuper ") << *name);

	obj->decRef();
	delete[] args;
}

void ABCVm::callSuperVoid(call_context* th, int n, int m)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th); 
	LOG(LOG_CALLS,_("callSuperVoid ") << *name << ' ' << m);

	ASObject* obj=th->runtime_stack_pop();

	//We modify the cur_level of obj
	obj->decLevel();

	//We should skip the special implementation of get
	ASObject* o=obj->getVariableByMultiname(*name, true);

	//And the reset it using the stack
	thisAndLevel tl=getVm()->getCurObjAndLevel();
	//What if using [sg]etSuper not on this??
	assert_and_throw(tl.cur_this==obj);
	tl.cur_this->setLevel(tl.cur_level);

	if(o)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=static_cast<IFunction*>(o);
			obj->incRef();
			ASObject* ret=f->call(obj,args,m);
			if(ret)
				ret->decRef();
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(LOG_NOT_IMPLEMENTED,_("We got a Undefined function"));
		}
/*		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(LOG_CALLS,_("We got a function not yet valid"));
			Definable* d=static_cast<Definable*>(o);
			d->define(obj);
			IFunction* f=obj->getVariableByMultiname(*name,owner)->toFunction();
			if(f)
				f->call(owner,args,m);
			else
			{
				LOG(LOG_NOT_IMPLEMENTED,_("No such function"));
			}
		}
		else
		{
			//IFunction* f=static_cast<IFunction*>(o->getVariableByQName("Call","",owner));
			//if(f)
			//	f->call(o,args,m);
			//else
			//{
			//	LOG(LOG_NOT_IMPLEMENTED,_("No such function, returning Undefined"));
			//}
		}*/
		else
			throw UnsupportedException("Not callable object in callSuperVoid");
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,_("Calling an undefined function"));
	}
	LOG(LOG_CALLS,_("End of callSuperVoid ") << *name);

	obj->decRef();
	delete[] args;
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

	if(obj->prototype)
	{
		assert_and_throw(type->getObjectType()==T_CLASS);

		objc=obj->prototype;
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

ASObject* ABCVm::asTypelate(ASObject* type, ASObject* obj)
{
	LOG(LOG_CALLS,_("asTypelate"));

	assert_and_throw(type->getObjectType()==T_CLASS);
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
	if(obj->prototype)
		objc=obj->prototype;
	else if(obj->getObjectType()==T_CLASS)
	{
		//Special case for Class
		if(c->class_name.name=="Class" && c->class_name.ns=="")
		{
			type->decRef();
			return obj;
		}
		
		objc=static_cast<Class_base*>(obj);
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
	bool ret;
	//If we are dealing with objects, check the prototype
	if(obj1->prototype && obj2->prototype && obj1->prototype!=obj2->prototype)
			ret=false;
	else if(obj1->getObjectType()!=obj2->getObjectType())
			ret=false;
	else
		ret=ifEq(obj2,obj1);
	LOG(LOG_CALLS,_("ifStrictEq ")<<ret);
	return ret;
}

bool ABCVm::ifStrictNE(ASObject* obj2, ASObject* obj1)
{
	LOG(LOG_CALLS,_("ifStrictNE"));
	return !ifStrictEq(obj2,obj1);
}

bool ABCVm::in(ASObject* val2, ASObject* val1)
{
	LOG(LOG_CALLS, _("in") );
	multiname name;
	name.name_type=multiname::NAME_STRING;
	name.name_s=val1->toString();
	name.ns.push_back(nsNameAndKind("",NAMESPACE));
	bool ret=val2->hasPropertyByMultiname(name);
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

	thisAndLevel tl=getVm()->getCurObjAndLevel();
	if(tl.cur_this==obj)
		obj->resetLevel();

	ASObject* o=obj->getVariableByMultiname(*name);

	if(tl.cur_this==obj)
		obj->setLevel(tl.cur_level);

	if(o==NULL)
		throw RunTimeException("Could not resolve property in constructProp");

	//The get protocol expects that we incRef the var
	o->incRef();

	if(o->getObjectType()==T_DEFINABLE)
	{
		LOG(LOG_CALLS,_("Deferred definition of property ") << *name);
		Definable* d=static_cast<Definable*>(o);
		d->define(obj);
		o=obj->getVariableByMultiname(*name);
		LOG(LOG_CALLS,_("End of deferred definition of property ") << *name);
	}

	LOG(LOG_CALLS,_("Constructing"));
	ASObject* ret;
	if(o->getObjectType()==T_CLASS)
	{
		Class_base* o_class=static_cast<Class_base*>(o);
		ret=o_class->getInstance(true,args,m);
	}
	else if(o->getObjectType()==T_FUNCTION)
	{
		SyntheticFunction* sf=dynamic_cast<SyntheticFunction*>(o);
		assert_and_throw(sf);
		ret=Class<ASObject>::getInstanceS();
		if(sf->mi->body)
		{
#ifndef NDEBUG
			ret->initialized=false;
#endif
			LOG(LOG_CALLS,_("Building method traits"));
			for(unsigned int i=0;i<sf->mi->body->trait_count;i++)
				th->context->buildTrait(ret,&sf->mi->body->traits[i],false);
#ifndef NDEBUG
			ret->initialized=true;
#endif
			ret->incRef();
			assert_and_throw(sf->closure_this==NULL);
			ASObject* ret2=sf->call(ret,args,m);
			if(ret2)
				ret2->decRef();

			//Let's see if an AS prototype has been defined on the function
			multiname prototypeName;
			prototypeName.name_type=multiname::NAME_STRING;
			prototypeName.name_s="prototype";
			prototypeName.ns.push_back(nsNameAndKind("",NAMESPACE));
			ASObject* asp=sf->getVariableByMultiname(prototypeName,true);
			if(asp)
				asp->incRef();

			//Now add our prototype
			sf->incRef();
			ret->setPrototype(new Class_function(sf,asp));
		}
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
	LOG(LOG_CALLS,_("hasNext2 ") << n << ' ' << m);
	ASObject* obj=th->locals[n];
	unsigned int cur_index=th->locals[m]->toUInt();

	if(obj && obj->implEnable)
	{
		bool ret;
		//cur_index is modified with the next index
		if(obj->hasNext(cur_index,ret))
		{
			if(ret)
			{
				th->locals[m]->decRef();
				th->locals[m]=abstract_i(cur_index);
			}
			else
			{
				th->locals[n]->decRef();
				th->locals[n]=new Null;
				th->locals[m]->decRef();
				th->locals[m]=abstract_i(0);
			}
			return ret;
		}
	}

/*	//Look up if there is a following index which is still an object
	//(not a method)
	for(cur_index;cur_index<obj->numVariables();cur_index++)
	{
		obj_var* var=obj->Variables.getValueAt(cur_index);
		if(var->var && var->var->getObjectType()!=T_FUNCTION)
			break;
	}*/

	//Our references are 0 based, the AS ones are 1 based
	//what a mess
	if(cur_index<obj->numVariables())
	{
		th->locals[m]->decRef();
		th->locals[m]=new Integer(cur_index+1);
		return true;
	}
	else
	{
		th->locals[n]->decRef();
		th->locals[n]=new Null;
		th->locals[m]->decRef();
		th->locals[m]=new Integer(0);
		return false;
	}
}

void ABCVm::newObject(call_context* th, int n)
{
	LOG(LOG_CALLS,_("newObject ") << n);
	ASObject* ret=Class<ASObject>::getInstanceS();
	for(int i=0;i<n;i++)
	{
		ASObject* value=th->runtime_stack_pop();
		ASObject* name=th->runtime_stack_pop();
		ret->setVariableByQName(name->toString(),"",value);
		name->decRef();
	}

	th->runtime_stack_push(ret);
}

void ABCVm::getDescendants(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(LOG_NOT_IMPLEMENTED,"getDescendants " << *name);
	ASObject* obj=th->runtime_stack_pop();
	//HACK: to be removed when describeType is implemented
	if(obj->getObjectType()==T_UNDEFINED)
	{
		obj->decRef();
		th->runtime_stack_push(new Undefined);
		return;
	}
	assert_and_throw(obj->getPrototype()==Class<XML>::getClass());
	XML* xmlObj=Class<XML>::cast(obj);
	//The name must be a QName
	assert_and_throw(name->name_type==multiname::NAME_STRING);
	vector<XML*> ret;
	//TODO: support multiname and namespaces
	xmlObj->getDescendantsByQName(name->name_s, "", ret);
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

uintptr_t ABCVm::increment_i(ASObject* o)
{
	LOG(LOG_CALLS,_("increment_i"));

	int n=o->toInt();
	o->decRef();
	return n+1;
}

ASObject* ABCVm::nextValue(ASObject* index, ASObject* obj)
{
	LOG(LOG_CALLS,_("nextValue"));
	if(index->getObjectType()!=T_INTEGER)
		throw UnsupportedException("Type mismatch in nextValue");

	ASObject* ret=NULL;
	if(obj->implEnable)
	{ 
		if(obj->nextValue(index->toInt()-1,ret))
		{
			obj->decRef();
			index->decRef();
			ret->incRef();
			return ret;
		}
	}

	ret=obj->getValueAt(index->toInt()-1);
	obj->decRef();
	index->decRef();
	ret->incRef();
	return ret;
}

ASObject* ABCVm::nextName(ASObject* index, ASObject* obj)
{
	LOG(LOG_CALLS,_("nextName"));
	if(index->getObjectType()!=T_INTEGER)
		throw UnsupportedException("Type mismatch in nextName");

	ASObject* ret=NULL;
	if(obj->implEnable)
	{ 
		if(obj->nextName(index->toInt(),ret))
		{
			obj->decRef();
			index->decRef();
			return ret;
		}
	}

	ret=Class<ASString>::getInstanceS(obj->getNameAt(index->toInt()-1));
	obj->decRef();
	index->decRef();
	return ret;
}

void ABCVm::newClassRecursiveLink(Class_base* target, Class_base* c)
{
	if(c->super)
		newClassRecursiveLink(target, c->super);

	const vector<Class_base*>& interfaces=c->getInterfaces();
	for(unsigned int i=0;i<interfaces.size();i++)
	{
		LOG(LOG_CALLS,_("Linking with interface ") << interfaces[i]->class_name);
		interfaces[i]->linkInterface(target);
	}
}

void ABCVm::newClass(call_context* th, int n)
{
	LOG(LOG_CALLS, _("newClass ") << n );
	method_info* constructor=&th->context->methods[th->context->instances[n].init];
	int name_index=th->context->instances[n].name;
	assert_and_throw(name_index);
	const multiname* mname=th->context->getMultiname(name_index,NULL);

	assert_and_throw(mname->ns.size()==1);
	Class_inherit* ret=new Class_inherit(QName(mname->name_s,mname->ns[0].name));
	ret->class_scope=th->scope_stack;
	for(uint32_t i=0;i<ret->class_scope.size();i++)
		ret->class_scope[i]->incRef();
	ASObject* tmp=th->runtime_stack_pop();

	assert_and_throw(th->context);
	ret->context=th->context;

	//Null is a "valid" base class
	if(tmp->getObjectType()!=T_NULL)
	{
		assert_and_throw(tmp->getObjectType()==T_CLASS);
		ret->super=static_cast<Class_base*>(tmp);
		ret->max_level=ret->super->max_level+1;
		ret->setLevel(ret->max_level);
	}

	method_info* m=&th->context->methods[th->context->classes[n].cinit];
	SyntheticFunction* cinit=Class<IFunction>::getSyntheticFunction(m);
	//cinit must inherit the current scope
	cinit->acquireScope(th->scope_stack);
	//and the created class
	cinit->addToScope(ret);

	LOG(LOG_CALLS,_("Building class traits"));
	for(unsigned int i=0;i<th->context->classes[n].trait_count;i++)
		th->context->buildTrait(ret,&th->context->classes[n].traits[i],false);

	//Add protected namespace if needed
	if((th->context->instances[n].flags)&0x08)
	{
		ret->use_protected=true;
		int ns=th->context->instances[n].protectedNs;
		const namespace_info& ns_info=th->context->constant_pool.namespaces[ns];
		ret->protected_ns=nsNameAndKind(th->context->getString(ns_info.name),(NS_KIND)(int)ns_info.kind);
	}

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
	//Also the constructor should have the script as the global object
	constructorFunc->addToScope(th->scope_stack[0]);
	//add Constructor the the class methods
	ret->constructor=constructorFunc;
	ret->class_index=n;

	//Set the constructor variable to the class itself (this is accessed by object using the protoype)
	ret->incRef();
	ret->setVariableByQName("constructor","",ret);

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

		if(obj->getObjectType()==T_DEFINABLE)
		{
			LOG(LOG_CALLS,_("Class ") << *name << _(" is not yet valid (as interface)"));
			Definable* d=static_cast<Definable*>(obj);
			d->define(target);
			LOG(LOG_CALLS,_("End of deferred init of class ") << *name);
			obj=target->getVariableByMultiname(*name);
			assert_and_throw(obj);
		}
	}
	//If the class is not an interface itself, link the traits
	if(!((th->context->instances[n].flags)&0x04))
	{
		//Link all the interfaces for this class and all the bases
		newClassRecursiveLink(ret, ret);
	}

	LOG(LOG_CALLS,_("Calling Class init ") << ret);
	ret->incRef();
	//Class init functions are called with global as this
	ASObject* ret2=cinit->call(ret,NULL,0);
	assert_and_throw(ret2==NULL);
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
	th->scope_stack.back()->decRef();
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

void ABCVm::call(call_context* th, int m)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	ASObject* obj=th->runtime_stack_pop();
	ASObject* f=th->runtime_stack_pop();
	LOG(LOG_CALLS,_("call ") << m << ' ' << f);

	if(f->getObjectType()==T_FUNCTION)
	{
		IFunction* func=static_cast<IFunction*>(f);
		//TODO: check for correct level, member function are already binded
		ASObject* ret=func->call(obj,args,m);
		//Push the value only if not null
		if(ret)
			th->runtime_stack_push(ret);
		else
			th->runtime_stack_push(new Undefined);
		f->decRef();
	}
	else if(f->getObjectType()==T_CLASS)
	{
		assert_and_throw(m==1);
		Class_base* c=static_cast<Class_base*>(f);
		ASObject* ret=c->generator(args,1);
		assert_and_throw(ret);
		th->runtime_stack_push(ret);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,_("Function not good ") << f->getObjectType());
		th->runtime_stack_push(new Undefined);
	}
	LOG(LOG_CALLS,_("End of call ") << m << ' ' << f);
	delete[] args;

}

void ABCVm::deleteProperty(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th); 
	LOG(LOG_CALLS,_("deleteProperty ") << *name);
	ASObject* obj=th->runtime_stack_pop();
	obj->deleteVariableByMultiname(*name);

	//TODO: Now we assume that all objects are dynamic
	th->runtime_stack_push(abstract_b(true));

	obj->decRef();
}

ASObject* ABCVm::newFunction(call_context* th, int n)
{
	LOG(LOG_CALLS,_("newFunction ") << n);

	method_info* m=&th->context->methods[n];
	SyntheticFunction* f=Class<IFunction>::getSyntheticFunction(m);
	f->func_scope=th->scope_stack;
	for(unsigned int i=0;i<f->func_scope.size();i++)
		f->func_scope[i]->incRef();

	//Bind the function to null, as this is not a class method
	f->bind(NULL,-1);
	return f;
}

ASObject* ABCVm::getScopeObject(call_context* th, int n)
{
	ASObject* ret=th->scope_stack[n+th->initialScopeStack];
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
	ASObject* catchScope = new ASObject();
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

