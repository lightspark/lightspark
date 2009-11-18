/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "abc.h"
#include <typeinfo>
using namespace std;

uintptr_t ABCVm::bitAnd(ASObject* val2, ASObject* val1)
{
	uintptr_t i1=val1->toInt();
	uintptr_t i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"bitAnd_oo " << hex << i1 << '&' << i2);
	return i1&i2;
}

uintptr_t ABCVm::increment(ASObject* o)
{
	LOG(CALLS,"increment");

	int n=o->toInt();
	o->decRef();
	return n+1;
}

uintptr_t ABCVm::bitAnd_oi(ASObject* val1, intptr_t val2)
{
	uintptr_t i1=val1->toInt();
	uintptr_t i2=val2;
	val1->decRef();
	LOG(CALLS,"bitAnd_oi " << hex << i1 << '&' << i2);
	return i1&i2;
}

void ABCVm::setProperty(ASObject* value,ASObject* obj,multiname* name)
{
	LOG(CALLS,"setProperty " << *name);

	obj->setVariableByMultiname(*name,value);
	obj->decRef();
}

void ABCVm::setProperty_i(intptr_t value,ASObject* obj,multiname* name)
{
	LOG(CALLS,"setProperty_i " << *name);

	obj->setVariableByMultiname_i(*name,value);
	obj->decRef();
}

ASObject* ABCVm::convert_d(ASObject* o)
{
	LOG(NOT_IMPLEMENTED, "convert_d" );
	return o;
}

ASObject* ABCVm::convert_b(ASObject* o)
{
	LOG(TRACE, "convert_b" );
	return o;
}

ASObject* ABCVm::convert_u(ASObject* o)
{
	LOG(NOT_IMPLEMENTED, "convert_u" );
	return o;
}

ASObject* ABCVm::convert_i(ASObject* o)
{
	LOG(NOT_IMPLEMENTED, "convert_i" );
	return o;
}

void ABCVm::label()
{
	LOG(CALLS, "label" );
}

void ABCVm::lookupswitch()
{
	LOG(CALLS, "lookupswitch" );
}

void ABCVm::coerce_a()
{
	LOG(NOT_IMPLEMENTED, "coerce_a" );
}

ASObject* ABCVm::checkfilter(ASObject* o)
{
	LOG(NOT_IMPLEMENTED, "checkfilter" );
	return o;
}

ASObject* ABCVm::coerce_s(ASObject* o)
{
	LOG(NOT_IMPLEMENTED, "coerce_s" );
	return o;
}

void ABCVm::pop(call_context* th)
{
	LOG(CALLS, "pop: DONE" );
}

void ABCVm::getLocal(call_context* th, int n)
{
	LOG(CALLS,"getLocal: DONE " << n);
}

void ABCVm::setLocal(call_context* th, int n)
{
	LOG(CALLS,"setLocal: DONE " << n);
}

intptr_t ABCVm::pushShort(intptr_t n)
{
	LOG(CALLS, "pushShort " << n );
	return n;
}

void ABCVm::setSlot(ASObject* value, ASObject* obj, int n)
{
	LOG(CALLS,"setSlot " << dec << n);
	obj->setSlot(n,value);
	obj->decRef();
}

ASObject* ABCVm::getSlot(ASObject* obj, int n)
{
	LOG(CALLS,"getSlot " << dec << n);
	ASObject* ret=obj->getSlot(n);
	ret->incRef();
	obj->decRef();
	return ret;
}

ASObject* ABCVm::negate(ASObject* v)
{
	LOG(CALLS, "negate" );
	ASObject* ret=new Number(-(v->toNumber()));
	v->decRef();
	return ret;
}

uintptr_t ABCVm::bitNot(ASObject* val)
{
	uintptr_t i1=val->toInt();
	val->decRef();
	LOG(CALLS,"bitNot " << hex << i1);
	return ~i1;
}

uintptr_t ABCVm::bitXor(ASObject* val2, ASObject* val1)
{
	int i1=val1->toInt();
	int i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"bitXor " << hex << i1 << '^' << i2);
	return i1^i2;
}

uintptr_t ABCVm::bitOr_oi(ASObject* val2, uintptr_t val1)
{
	int i1=val1;
	int i2=val2->toInt();
	val2->decRef();
	LOG(CALLS,"bitOr " << hex << i1 << '|' << i2);
	return i1|i2;
}

uintptr_t ABCVm::bitOr(ASObject* val2, ASObject* val1)
{
	int i1=val1->toInt();
	int i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"bitOr " << hex << i1 << '|' << i2);
	return i1|i2;
}

void ABCVm::callProperty(call_context* th, int n, int m)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th);
	LOG(CALLS,"callProperty " << *name << ' ' << m);

	ASObject* obj=th->runtime_stack_pop();

	//HACK osceno
	if(name->name_s=="isWhiteListedUrl" || name->name_s=="isAtLeastVersion")
	{
		LOG(NOT_IMPLEMENTED,"HACK osceno per youtube");
		th->runtime_stack_push(new Boolean(true));
		return;
	}

	//Here overriding is supposed to work, so restore prototype and max_level
	int oldlevel=obj->max_level;
	if(obj->prototype)
		obj->max_level=obj->prototype->max_level;
	//Store the old prototype
	Class_base* old_prototype=obj->actualPrototype;
	obj->actualPrototype=obj->prototype;

	ASObject* owner;
	ASObject* o=obj->getVariableByMultiname(*name,owner);
	if(owner)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=static_cast<IFunction*>(o);
			//Methods as to be runned with their own class this
			ASObject* ret=f->fast_call(owner,args,m);
			th->runtime_stack_push(ret);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function on obj " << ((obj->prototype)?obj->prototype->class_name:"Object"));
			th->runtime_stack_push(new Undefined);
		}
		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(CALLS,"We got a function not yet valid");
			Definable* d=static_cast<Definable*>(o);
			d->define(obj);
			o=obj->getVariableByMultiname(*name,owner);
			if(o->getObjectType()==T_OBJECT)
			{
				//Could be a interface upcasting, return the argument
				LOG(NOT_IMPLEMENTED,"Interface upcating?, return arg");
				if(m!=1)
					abort();
				th->runtime_stack_push(args[0]);
			}
			else
			{
				IFunction* f=o->toFunction();
				if(f)
				{
					ASObject* ret=f->fast_call(owner,args,m);
					th->runtime_stack_push(ret);
				}
				else
				{
					LOG(NOT_IMPLEMENTED,"No such function");
					th->runtime_stack_push(new Undefined);
					//abort();
				}
			}
		}
		else
		{
			if(m==1) //Assume this is a constructor
			{
				LOG(CALLS,"Passthorugh of " << args[0]);
				th->runtime_stack_push(args[0]);
			}
			else
				abort();

			/*
			IFunction* f=static_cast<IFunction*>(o->getVariableByQName("Call","",owner));
			if(f)
			{
				ASObject* ret=f->fast_call(o,args,m);
				th->runtime_stack_push(ret);
			}
			else
			{
				LOG(NOT_IMPLEMENTED,"No such function, returning Undefined");
				th->runtime_stack_push(new Undefined);
			}*/
		}
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Calling an undefined function on obj " << ((obj->prototype)?obj->prototype->class_name:"Object"));
		th->runtime_stack_push(new Undefined);
	}
	LOG(CALLS,"End of calling " << *name);

	obj->actualPrototype=old_prototype;
	obj->max_level=oldlevel;
	obj->decRef();
	delete[] args;
}

intptr_t ABCVm::getProperty_i(ASObject* obj, multiname* name)
{
	LOG(CALLS, "getProperty_i " << *name );

	ASObject* owner;
	intptr_t ret=obj->getVariableByMultiname_i(*name,owner);
	if(owner==NULL)
	{
		LOG(NOT_IMPLEMENTED,"Property not found " << *name);
		return 0;
	}
	obj->decRef();
	return ret;
}

ASObject* ABCVm::getProperty(ASObject* obj, multiname* name)
{
	LOG(CALLS, "getProperty " << *name );

	ASObject* owner;
	ASObject* ret=obj->getVariableByMultiname(*name,owner);
	if(owner==NULL)
	{
		LOG(NOT_IMPLEMENTED,"Property not found " << *name);
		ret=new Undefined;
	}
	else
	{
		//If we are getting a function object attach the the current scope
		if(ret->getObjectType()==T_FUNCTION)
		{
			LOG(CALLS,"Attaching this to function " << name);
			IFunction* f=ret->toFunction()->clone();
			f->bind(owner);
			ret=f;
		}
		else if(ret->getObjectType()==T_DEFINABLE)
		{
			//LOG(ERROR,"Property " << name << " is not yet valid");
			abort();
			Definable* d=static_cast<Definable*>(ret);
			d->define(obj);
			ret=obj->getVariableByMultiname(*name,owner);
		}
		ret->incRef();
	}
	obj->decRef();
	return ret;
}

number_t ABCVm::divide(ASObject* val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED ||
		val2->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(NOT_IMPLEMENTED,"subtract: HACK");
		return 0;
	}
	double num1=val1->toNumber();
	double num2=val2->toNumber();

	val1->decRef();
	val2->decRef();
	LOG(CALLS,"divide "  << num1 << '/' << num2);
	return num1/num2;
}

ASObject* ABCVm::getGlobalScope(call_context* th)
{
	LOG(CALLS,"getGlobalScope: " << &th->context->Global);
	th->context->Global->incRef();
	return th->context->Global;
}

uintptr_t ABCVm::decrement(ASObject* o)
{
	LOG(CALLS,"decrement");

	int n=o->toInt();
	o->decRef();
	return n-1;
}

ASObject* ABCVm::decrement_i(ASObject* o)
{
	LOG(NOT_IMPLEMENTED,"decrement_i");
	abort();
	return o;
}

bool ABCVm::ifLT(ASObject* obj2, ASObject* obj1)
{
	LOG(CALLS,"ifLT");

	//Real comparision demanded to object
	bool ret=obj1->isLess(obj2);
	cout << obj1->toInt() << "<" << obj2->toInt() << endl;

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLT_oi(ASObject* obj2, intptr_t val1)
{
	LOG(CALLS,"ifLT_oi");

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
	LOG(CALLS,"ifLT_io ");

	bool ret=obj1->toInt()<val2;

	obj1->decRef();
	return ret;
}

bool ABCVm::ifNE(ASObject* obj1, ASObject* obj2)
{
	LOG(CALLS,"ifNE");

	//Real comparision demanded to object
	bool ret=!(obj1->isEqual(obj2));
	cout << "compare " << ret << endl;

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifNE_oi(ASObject* obj1, intptr_t val2)
{
	LOG(CALLS,"ifNE");

	bool ret=obj1->toInt()!=val2;

	obj1->decRef();
	return ret;
}

intptr_t ABCVm::pushByte(intptr_t n)
{
	LOG(CALLS, "pushByte " << n );
	return n;
}

number_t ABCVm::multiply_oi(ASObject* val2, intptr_t val1)
{
	double num1=val1;
	double num2=val2->toNumber();
	val2->decRef();
	LOG(CALLS,"multiply_oi "  << num1 << '*' << num2);
	return num1*num2;
}

number_t ABCVm::multiply(ASObject* val2, ASObject* val1)
{
	double num1=val1->toNumber();
	double num2=val2->toNumber();
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"multiply "  << num1 << '*' << num2);
	return num1*num2;
}

void ABCVm::incLocal_i(call_context* th, int n)
{
	LOG(CALLS, "incLocal_i " << n );
	if(th->locals[n]->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(th->locals[n]);
		i->val++;
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Cannot increment type " << th->locals[n]->getObjectType());
	}

}

void ABCVm::construct(call_context* th, int m)
{
	LOG(CALLS, "construct " << m);
	arguments args(m);
	for(int i=0;i<m;i++)
		args.set(m-i-1,th->runtime_stack_pop());

	ASObject* obj=th->runtime_stack_pop();

	if(obj->getObjectType()==T_DEFINABLE)
	{
		LOG(ERROR,"Check");
		abort();
	/*	LOG(CALLS,"Deferred definition of property " << name);
		Definable* d=static_cast<Definable*>(o);
		d->define(obj);
		o=obj->getVariableByMultiname(name,owner);
		LOG(CALLS,"End of deferred definition of property " << name);*/
	}

	LOG(CALLS,"Constructing");
	Class_base* o_class=static_cast<Class_base*>(obj);
	assert(o_class->getObjectType()==T_CLASS);
	ASObject* ret=o_class->getInstance()->obj;

	ret->handleConstruction(th->context, &args, true);

//	args.decRef();
	obj->decRef();
	LOG(CALLS,"End of constructing");
	th->runtime_stack_push(ret);
}

ASObject* ABCVm::typeOf(ASObject* obj)
{
	LOG(CALLS,"typeOf");
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
	return new ASString(ret);
}

void ABCVm::callPropVoid(call_context* th, int n, int m)
{
	multiname* name=th->context->getMultiname(n,th); 
	LOG(CALLS,"callPropVoid " << *name << ' ' << m);

	arguments args(m);
	for(int i=0;i<m;i++)
		args.set(m-i-1,th->runtime_stack_pop());
	ASObject* obj=th->runtime_stack_pop();

	//Here overriding is supposed to work, so restore prototype and max_level
	int oldlevel=obj->max_level;
	if(obj->prototype)
		obj->max_level=obj->prototype->max_level;
	//Store the old prototype
	Class_base* old_prototype=obj->actualPrototype;
	obj->actualPrototype=obj->prototype;

	ASObject* owner;
	ASObject* o=obj->getVariableByMultiname(*name,owner);
	if(owner)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=static_cast<IFunction*>(o);
			f->call(obj,&args);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function onj type " << obj->prototype->class_name);
		}
		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(NOT_IMPLEMENTED,"We got a function not yet valid");
		}
		else
		{
			abort();
/*			IFunction* f=static_cast<IFunction*>(o->getVariableByQName(".Call","",owner));
			f->call(owner,&args);*/
		}
	}
	else
	{
		if(obj->prototype)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function obj type " << obj->prototype->class_name);
		}
		else
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function");
		}
	}

	obj->actualPrototype=old_prototype;
	obj->max_level=oldlevel;
	obj->decRef();
	LOG(CALLS,"End of calling " << *name);
}

void ABCVm::jump(call_context* th, int offset)
{
	LOG(CALLS,"jump " << offset);
}

bool ABCVm::ifTrue(ASObject* obj1, int offset)
{
	LOG(CALLS,"ifTrue " << offset);

	bool ret=Boolean_concrete(obj1);
	obj1->decRef();
	return ret;
}

intptr_t ABCVm::modulo(ASObject* val1, ASObject* val2)
{
	int num2=val2->toInt();
	int num1=val1->toInt();

	val1->decRef();
	val2->decRef();
	LOG(CALLS,"modulo "  << num1 << '%' << num2);
	return num1%num2;
}

number_t ABCVm::subtract_oi(ASObject* val2, intptr_t val1)
{
	int num2=val2->toInt();
	int num1=val1;

	val2->decRef();
	LOG(CALLS,"subtract_oi " << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_do(number_t val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(NOT_IMPLEMENTED,"subtract: HACK");
		return 0;
	}
	number_t num2=val2;
	number_t num1=val1->toNumber();

	val1->decRef();
	LOG(CALLS,"subtract_do " << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_io(intptr_t val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(NOT_IMPLEMENTED,"subtract: HACK");
		return 0;
	}
	int num2=val2;
	int num1=val1->toInt();

	val1->decRef();
	LOG(CALLS,"subtract_io " << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract(ASObject* val2, ASObject* val1)
{
	if(val1->getObjectType()==T_UNDEFINED ||
		val2->getObjectType()==T_UNDEFINED)
	{
		//HACK
		LOG(NOT_IMPLEMENTED,"subtract: HACK");
		return 0;
	}
	int num2=val2->toInt();
	int num1=val1->toInt();

	val1->decRef();
	val2->decRef();
	LOG(CALLS,"subtract " << num1 << '-' << num2);
	return num1-num2;
}

ASObject* ABCVm::pushInt(call_context* th, int n)
{
	s32 i=th->context->constant_pool.integer[n];
	LOG(CALLS, "pushInt [" << dec << n << "] " << i);
}

void ABCVm::kill(call_context* th, int n)
{
	LOG(CALLS, "kill " << n );
}

ASObject* ABCVm::add(ASObject* val2, ASObject* val1)
{
	//Implement ECMA add algorithm, for XML and default
	if(val1->getObjectType()==T_NUMBER && val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1->toNumber();
		val1->decRef();
		val2->decRef();
		LOG(CALLS,"add " << num1 << '+' << num2);
		return abstract_d(num1+num2);
	}
	else if(val1->getObjectType()==T_INTEGER && val2->getObjectType()==T_INTEGER)
	{
		double num2=val2->toNumber();
		double num1=val1->toNumber();
		val1->decRef();
		val2->decRef();
		LOG(CALLS,"add " << num1 << '+' << num2);
		return abstract_d(num1+num2);
	}
	else if(val1->getObjectType()==T_INTEGER && val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1->toNumber();
		val1->decRef();
		val2->decRef();
		LOG(CALLS,"add " << num1 << '+' << num2);
		return abstract_d(num1+num2);
	}
	else if(val1->getObjectType()==T_NUMBER && val2->getObjectType()==T_INTEGER)
	{
		double num2=val2->toNumber();
		double num1=val1->toNumber();
		val1->decRef();
		val2->decRef();
		LOG(CALLS,"add " << num1 << '+' << num2);
		return abstract_d(num1+num2);
	}
	else if(val1->getObjectType()==T_STRING || val2->getObjectType()==T_STRING)
	{
		string a(val1->toString().raw_buf());
		string b(val2->toString().raw_buf());
		val1->decRef();
		val2->decRef();
		LOG(CALLS,"add " << a << '+' << b);
		return new ASString(a+b);
	}
	else if(val1->getObjectType()==T_ARRAY)
	{
		//Array concatenation
		Array* ar=static_cast<Array*>(val1->interface);
		ar->push(val2);
		return val1;
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Add between types " << val1->getObjectType() << ' ' << val2->getObjectType());
		return new Undefined;
	}

}

ASObject* ABCVm::add_oi(ASObject* val2, intptr_t val1)
{
	//Implement ECMA add algorithm, for XML and default
	if(val2->getObjectType()==T_INTEGER)
	{
		Integer* ip=static_cast<Integer*>(val2);
		int num2=ip->val;
		int num1=val1;
		val2->decRef();
		LOG(CALLS,"add " << num1 << '+' << num2);
		return abstract_i(num1+num2);
	}
	else if(val2->getObjectType()==T_NUMBER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		val2->decRef();
		LOG(CALLS,"add " << num1 << '+' << num2);
		return abstract_d(num1+num2);
	}
	else if(val2->getObjectType()==T_STRING)
	{
		tiny_string tmp(val1);
		string a(tmp.raw_buf());
		string b(val2->toString().raw_buf());
		val2->decRef();
		LOG(CALLS,"add " << a << '+' << b);
		return new ASString(a+b);
	}
	else if(val2->getObjectType()==T_ARRAY)
	{
		abort();
		/*//Array concatenation
		ASArray* ar=static_cast<ASArray*>(val1);
		ar->push(val2);
		return ar;*/
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Add between integer and " << val2->getObjectType());
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
		LOG(CALLS,"add " << num1 << '+' << num2);
		return new Number(num1+num2);
	}
	else if(val2->getObjectType()==T_INTEGER)
	{
		double num2=val2->toNumber();
		double num1=val1;
		val2->decRef();
		LOG(CALLS,"add " << num1 << '+' << num2);
		return new Number(num1+num2);
	}
	else if(val2->getObjectType()==T_STRING)
	{
		abort();
		/*string a=val1->toString();
		string b=val2->toString();
		val1->decRef();
		val2->decRef();
		LOG(CALLS,"add " << a << '+' << b);
		return new ASString(a+b);*/
	}
	else if(val2->getObjectType()==T_ARRAY)
	{
		abort();
		/*//Array concatenation
		ASArray* ar=static_cast<ASArray*>(val1);
		ar->push(val2);
		return ar;*/
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Add between types number and " << val2->getObjectType());
		return new Undefined;
	}

}

uintptr_t ABCVm::lShift(ASObject* val1, ASObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"lShift "<<i2<<"<<"<<i1);
	return i2<<i1;
}

uintptr_t ABCVm::lShift_io(uintptr_t val1, ASObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1&0x1f;
	val2->decRef();
	LOG(CALLS,"lShift "<<i2<<"<<"<<i1);
	return i2<<i1;
}

uintptr_t ABCVm::rShift(ASObject* val1, ASObject* val2)
{
	int32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"rShift "<<i2<<">>"<<i1);
	return i2>>i1;
}

uintptr_t ABCVm::urShift(ASObject* val1, ASObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"urShift "<<i2<<">>"<<i1);
	return i2>>i1;
}

uintptr_t ABCVm::urShift_io(uintptr_t val1, ASObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1&0x1f;
	val2->decRef();
	LOG(CALLS,"urShift "<<i2<<">>"<<i1);
	return i2>>i1;
}

bool ABCVm::_not(ASObject* v)
{
	LOG(CALLS, "not" );
	bool ret=!Boolean_concrete(v);
	v->decRef();
	return ret;
}

bool ABCVm::equals(ASObject* val2, ASObject* val1)
{
	LOG(CALLS, "equals" );
	bool ret=val1->isEqual(val2);
	val1->decRef();
	val2->decRef();
	return ret;
}

void ABCVm::dup(call_context* th)
{
	LOG(CALLS, "dup: DONE" );
}

bool ABCVm::pushTrue()
{
	LOG(CALLS, "pushTrue" );
	return true;
}

bool ABCVm::pushFalse()
{
	LOG(CALLS, "pushFalse" );
	return false;
}

ASObject* ABCVm::pushNaN(call_context* th)
{
	LOG(NOT_IMPLEMENTED, "pushNaN" );
	return new Undefined;
}

bool ABCVm::ifGT(ASObject* obj2, ASObject* obj1)
{
	LOG(CALLS,"ifGT");

	//Real comparision demanded to object
	bool ret=obj1->isGreater(obj2);

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifNGT(ASObject* obj2, ASObject* obj1, int offset)
{
	LOG(CALLS,"ifNGT " << offset);

	//Real comparision demanded to object
	bool ret= !(obj1->isGreater(obj2));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLE(ASObject* obj2, ASObject* obj1)
{
	LOG(CALLS,"ifLE");

	//Real comparision demanded to object
	bool ret=obj1->isLess(obj2) || obj1->isEqual(obj2);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifNLE(ASObject* obj2, ASObject* obj1, int offset)
{
	LOG(CALLS,"ifNLE " << offset);

	//Real comparision demanded to object
	bool ret=!(obj1->isLess(obj2) || obj1->isEqual(obj2));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifGE(ASObject* obj2, ASObject* obj1)
{
	LOG(CALLS,"ifGE");
	abort();
}

bool ABCVm::ifNGE(ASObject* obj2, ASObject* obj1, int offset)
{
	LOG(CALLS,"ifNGE " << offset);

	//Real comparision demanded to object
	bool ret=!(obj1->isGreater(obj2) || obj1->isEqual(obj2));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

void ABCVm::_throw(call_context* th)
{
	LOG(NOT_IMPLEMENTED, "throw" );
	abort();
}

void ABCVm::setSuper(call_context* th, int n)
{
	abort();
	ASObject* value=th->runtime_stack_pop();
	multiname* name=th->context->getMultiname(n,th); 

	LOG(NOT_IMPLEMENTED,"setSuper " << *name);

/*	ASObject* obj=th->runtime_stack_pop();
	if(obj->super)
		obj=obj->super;

	obj->setVariableByMultiname(*name,value);

	//TODO: decRef object*/
}

void ABCVm::getSuper(call_context* th, int n)
{
	abort();
	multiname* name=th->context->getMultiname(n,th); 

	LOG(NOT_IMPLEMENTED,"getSuper " << *name);

	ASObject* obj=th->runtime_stack_pop();
/*	if(obj->super)
		obj=obj->super;

	ASObject* owner;
	ASObject* ret=obj->getVariableByMultiname(*name,owner);

	if(owner)
	{
		ret->incRef();
		th->runtime_stack_push(ret);
	}
	else
		th->runtime_stack_push(new Undefined);

	//TODO: decRef object*/
}

void ABCVm::getLex(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(CALLS, "getLex: " << *name );
	vector<ASObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ASObject* owner;
	for(it;it!=th->scope_stack.rend();it++)
	{
		ASObject* o=(*it)->getVariableByMultiname(*name,owner);
		if(owner)
		{
			//If we are getting a function object attach the the current scope
			if(o->getObjectType()==T_FUNCTION)
			{
				LOG(CALLS,"Attaching this to function " << name);
				IFunction* f=o->toFunction()->clone();
				f->bind(*it);
				o=f;
			}
			else if(o->getObjectType()==T_DEFINABLE)
			{
				LOG(CALLS,"Deferred definition of property " << *name);
				Definable* d=static_cast<Definable*>(o);
				d->define(*it);
				o=(*it)->getVariableByMultiname(*name,owner);
				LOG(CALLS,"End of deferred definition of property " << *name);
			}
			th->runtime_stack_push(o);
			o->incRef();
			break;
		}
	}
	if(!owner)
	{
		LOG(CALLS, "NOT found, trying Global" );
		ASObject* o2=th->context->vm->Global.getVariableByMultiname(*name,owner);
		if(owner)
		{
			if(o2->getObjectType()==T_DEFINABLE)
			{
				LOG(CALLS,"Deferred definition of property " << *name);
				Definable* d=static_cast<Definable*>(o2);
				d->define(th->context->Global);
				o2=th->context->Global->getVariableByMultiname(*name,owner);
				LOG(CALLS,"End of deferred definition of property " << *name);
			}

			th->runtime_stack_push(o2);
			o2->incRef();
		}
		else
		{
			LOG(NOT_IMPLEMENTED, "NOT found, pushing Undefined" );
			th->runtime_stack_push(new Undefined);
		}
	}
}

void ABCVm::constructSuper(call_context* th, int n)
{
	LOG(CALLS, "constructSuper " << n);
	arguments args(n);
	for(int i=0;i<n;i++)
		args.set(n-i-1,th->runtime_stack_pop());

	ASObject* obj=th->runtime_stack_pop();

	if(obj->actualPrototype==NULL)
	{
		LOG(CALLS,"No prototype. Returning");
		abort();
		return;
	}

	//Store the old prototype
	Class_base* old_prototype=obj->actualPrototype;
	cout << "Cur prototype name " << obj->actualPrototype->class_name << endl;
	//Move the object protoype and level up
	obj->actualPrototype=old_prototype->super;
	assert(obj->actualPrototype);

	//multiname* name=th->context->getMultiname(th->context->instances[obj->actualPrototype->class_index].name,NULL);
	//LOG(CALLS,"Constructing super " << *name);

	obj->handleConstruction(th->context,&args, false);
	LOG(CALLS,"End super construct ");

	//Reset prototype to its previous value
	assert(obj->actualPrototype==old_prototype->super);
	obj->actualPrototype=old_prototype;

	obj->decRef();
}

void ABCVm::findProperty(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(CALLS, "findProperty " << *name );

	vector<ASObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ASObject* owner;
	for(it;it!=th->scope_stack.rend();it++)
	{
		(*it)->getVariableByMultiname(*name,owner);
		if(owner)
		{
			//We have to return the object, not the property
			th->runtime_stack_push(owner);
			owner->incRef();
			break;
		}
	}
	if(!owner)
	{
		LOG(CALLS, "NOT found, pushing global" );
		th->runtime_stack_push(&th->context->vm->Global);
		th->context->vm->Global.incRef();
	}
}

void ABCVm::findPropStrict(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(CALLS, "findPropStrict " << *name );

	vector<ASObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ASObject* owner;
	for(it;it!=th->scope_stack.rend();it++)
	{
		(*it)->getVariableByMultiname(*name,owner);
		if(owner)
		{
			//We have to return the object, not the property
			th->runtime_stack_push(owner);
			owner->incRef();
			break;
		}
	}
	if(!owner)
	{
		LOG(CALLS, "NOT found, trying Global" );
		//TODO: to multiname
		th->context->vm->Global.getVariableByMultiname(*name,owner);
		if(owner)
		{
			th->runtime_stack_push(owner);
			owner->incRef();
		}
		else
		{
			LOG(CALLS, "NOT_IMPLEMENTED found, pushing Undefined" );
			th->runtime_stack_push(new Undefined);
		}
	}
}

ASObject* ABCVm::greaterThan(ASObject* obj1, ASObject* obj2)
{
	LOG(CALLS,"greaterThan");

	//Real comparision demanded to object
	bool ret=obj1->isGreater(obj2);
	obj1->decRef();
	obj2->decRef();
	return new Boolean(ret);
}

ASObject* ABCVm::greaterEquals(ASObject* obj1, ASObject* obj2)
{
	LOG(CALLS,"greaterEquals");

	//Real comparision demanded to object
	bool ret=(obj1->isGreater(obj2) || obj1->isEqual(obj2));
	obj1->decRef();
	obj2->decRef();
	return new Boolean(ret);
}

ASObject* ABCVm::lessEquals(ASObject* obj1, ASObject* obj2)
{
	LOG(CALLS,"lessEquals");

	//Real comparision demanded to object
	bool ret=(obj1->isLess(obj2) || obj1->isEqual(obj2));
	obj1->decRef();
	obj2->decRef();
	return new Boolean(ret);
}

void ABCVm::initProperty(call_context* th, int n)
{
	ASObject* value=th->runtime_stack_pop();
	multiname* name=th->context->getMultiname(n,th);
	LOG(CALLS, "initProperty " << *name );

	ASObject* obj=th->runtime_stack_pop();

	obj->setVariableByMultiname(*name,value);
	obj->decRef();
}

void ABCVm::callSuper(call_context* th, int n, int m)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th); 
	LOG(NOT_IMPLEMENTED,"callSuper " << *name << ' ' << m);

	ASObject* obj=th->runtime_stack_pop();
	//HACK (nice) set the max level to the current actual prototype before looking up the member
	assert(obj->actualPrototype);
	int oldlevel=obj->max_level;
	obj->max_level=obj->actualPrototype->max_level-1;
	//Store the old prototype
	Class_base* old_prototype=obj->actualPrototype;

	//Move the object protoype and level up
	obj->actualPrototype=old_prototype->super;

	ASObject* owner;
	ASObject* o=obj->getVariableByMultiname(*name,owner);

	//Set back the original max_level
	obj->max_level=oldlevel;

	if(owner)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=static_cast<IFunction*>(o);
			ASObject* ret=f->fast_call(owner,args,m);
			th->runtime_stack_push(ret);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function");
			th->runtime_stack_push(new Undefined);
		}
/*		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(CALLS,"We got a function not yet valid");
			Definable* d=static_cast<Definable*>(o);
			d->define(obj);
			IFunction* f=obj->getVariableByMultiname(*name,owner)->toFunction();
			if(f)
			{
				ASObject* ret=f->fast_call(owner,args,m);
				th->runtime_stack_push(ret);
			}
			else
			{
				LOG(NOT_IMPLEMENTED,"No such function");
				th->runtime_stack_push(new Undefined);
				//abort();
			}
		}
		else
		{
			//IFunction* f=static_cast<IFunction*>(o->getVariableByQName("Call","",owner));
			//if(f)
			//{
			//	ASObject* ret=f->fast_call(o,args,m);
			//	th->runtime_stack_push(ret);
			//}
			//else
			//{
			//	LOG(NOT_IMPLEMENTED,"No such function, returning Undefined");
			//	th->runtime_stack_push(new Undefined);
			//}
		}*/
		else
			abort();
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Calling an undefined function");
		th->runtime_stack_push(new Undefined);
	}
	LOG(CALLS,"End of calling " << *name);

	//Reset prototype to its previous value
	assert(obj->actualPrototype==old_prototype->super);
	obj->actualPrototype=old_prototype;

	obj->decRef();
	delete[] args;
}

void ABCVm::callSuperVoid(call_context* th, int n, int m)
{
	ASObject** args=new ASObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th); 
	LOG(CALLS,"callSuperVoid " << *name << ' ' << m);

	ASObject* obj=th->runtime_stack_pop();

	cout << "super name " << obj->actualPrototype->super->class_name << endl;
	//HACK (nice) set the max level to the current actual prototype before looking up the member
	assert(obj->actualPrototype);
	int oldlevel=obj->max_level;
	obj->max_level=obj->actualPrototype->max_level-1;
	//Store the old prototype
	Class_base* old_prototype=obj->actualPrototype;

	//Move the object protoype and level up
	obj->actualPrototype=old_prototype->super;

	ASObject* owner;
	ASObject* o=obj->getVariableByMultiname(*name,owner);

	//Set back the original max_level
	obj->max_level=oldlevel;

	if(owner)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=static_cast<IFunction*>(o);
			f->fast_call(obj,args,m);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function");
		}
/*		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(CALLS,"We got a function not yet valid");
			Definable* d=static_cast<Definable*>(o);
			d->define(obj);
			IFunction* f=obj->getVariableByMultiname(*name,owner)->toFunction();
			if(f)
				f->fast_call(owner,args,m);
			else
			{
				LOG(NOT_IMPLEMENTED,"No such function");
			//	abort();
			}
		}
		else
		{
			//IFunction* f=static_cast<IFunction*>(o->getVariableByQName("Call","",owner));
			//if(f)
			//	f->fast_call(o,args,m);
			//else
			//{
			//	LOG(NOT_IMPLEMENTED,"No such function, returning Undefined");
			//}
		}*/
		else
			abort();
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Calling an undefined function");
	}
	LOG(CALLS,"End of calling " << *name);

	//Reset prototype to its previous value
	assert(obj->actualPrototype==old_prototype->super);
	obj->actualPrototype=old_prototype;

	obj->decRef();
	delete[] args;
}

void ABCVm::isTypelate(call_context* th)
{
	LOG(NOT_IMPLEMENTED,"isTypelate");
	ASObject* type=th->runtime_stack_pop();
	ASObject* obj=th->runtime_stack_pop();
	if(obj->getObjectType()==T_UNDEFINED)
	{
		th->runtime_stack_push(new Boolean(false));
		cout << "false" << endl;
	}
	else
	{
		cout << "true" << endl;
		th->runtime_stack_push(new Boolean(true));
	}
//	cout << "Name " << type->class_name << " type " << type->getObjectType() << endl;
//	cout << "Name " << obj->class_name << " type " << obj->getObjectType() << endl;
//	if(type->class_name==obj->class_name)
//	else
//		th->runtime_stack_push(new Boolean(false));
}

bool ABCVm::ifStrictNE(ASObject* obj2, ASObject* obj1)
{
	LOG(CALLS,"ifStrictNE");
	if(obj1->getObjectType()!=obj2->getObjectType())
		return false;
	return ifNE(obj2,obj1);
}

bool ABCVm::in(ASObject* val2, ASObject* val1)
{
	LOG(CALLS, "in" );
	bool ret=val2->hasProperty(val1->toString());
	val1->decRef();
	val2->decRef();
	return ret;
}

bool ABCVm::ifFalse(ASObject* obj1, int offset)
{
	LOG(CALLS,"ifFalse " << offset);

	bool ret=!Boolean_concrete(obj1);
	cout << "return " << ret << endl;
	obj1->decRef();
	return ret;
}

void ABCVm::constructProp(call_context* th, int n, int m)
{
	arguments args(m);
	for(int i=0;i<m;i++)
		args.set(m-i-1,th->runtime_stack_pop());

	multiname* name=th->context->getMultiname(n,th);

	LOG(CALLS,"constructProp "<< *name << ' ' << m);

	ASObject* obj=th->runtime_stack_pop();
	ASObject* owner;
	ASObject* o=obj->getVariableByMultiname(*name,owner);
	if(!owner)
	{
		LOG(ERROR,"Could not resolve property");
		abort();
	}

	if(o->getObjectType()==T_DEFINABLE)
	{
		LOG(CALLS,"Deferred definition of property " << *name);
		Definable* d=static_cast<Definable*>(o);
		d->define(obj);
		o=obj->getVariableByMultiname(*name,owner);
		LOG(CALLS,"End of deferred definition of property " << *name);
	}

	LOG(CALLS,"Constructing");
	if(o->getObjectType()==T_CLASS)
	{
		Class_base* o_class=static_cast<Class_base*>(o);
		ASObject* ret=o_class->getInstance()->obj;;

		ret->handleConstruction(th->context, &args, true);
		th->runtime_stack_push(ret);
	}
	else if(o->getObjectType()==T_FUNCTION)
	{
		SyntheticFunction* sf=static_cast<SyntheticFunction*>(o);
		ASObject* ret=new Function_Object;
		if(sf->mi->body)
		{
			LOG(CALLS,"Building method traits");
			for(int i=0;i<sf->mi->body->trait_count;i++)
				th->context->buildTrait(ret,&sf->mi->body->traits[i]);
			sf->call(ret,&args);
		}
		th->runtime_stack_push(ret);
	}
	else
		abort();

	obj->decRef();
	LOG(CALLS,"End of constructing");
}

ASObject* ABCVm::hasNext2(call_context* th, int n, int m)
{
	LOG(NOT_IMPLEMENTED,"hasNext2 " << n << ' ' << m);
	ASObject* obj=th->locals[n];
	int cur_index=th->locals[m]->toInt()+1;
	//Look up if there is a following index which is still an object
	//(not a method)
	for(cur_index;cur_index<obj->numVariables();cur_index++)
	{
		obj_var* var=obj->Variables.getValueAt(cur_index);
		if(var->var && var->var->getObjectType()!=T_FUNCTION)
			break;
	}

	//Our references are 0 based, the AS ones are 1 based
	//what a mess
	if(cur_index<obj->numVariables())
	{
		if(obj->getNameAt(cur_index)=="toString")
			abort();
		th->locals[m]->decRef();
		th->locals[m]=new Integer(cur_index+1);
		return new Boolean(true);
	}
	else
	{
		th->locals[n]->decRef();
		th->locals[n]=new Null;
		th->locals[m]->decRef();
		th->locals[m]=new Integer(0);
		return new Boolean(false);
	}
}

void ABCVm::newObject(call_context* th, int n)
{
	LOG(CALLS,"newObject " << n);
	ASObject* ret=new ASObject;
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
	abort();
/*	LOG(CALLS,"newObject " << n);
	ASObject* ret=new ASObject;
	for(int i=0;i<n;i++)
	{
		ASObject* value=th->runtime_stack_pop();
		ASObject* name=th->runtime_stack_pop();
		ret->setVariableByQName(name->toString(),"",value);
		name->decRef();
	}

	th->runtime_stack_push(ret);*/
}

uintptr_t ABCVm::increment_i(ASObject* o)
{
	LOG(CALLS,"increment_i");

	int n=o->toInt();
	o->decRef();
	return n+1;
}

ASObject* ABCVm::nextName(ASObject* index, ASObject* obj)
{
	LOG(CALLS,"nextName");
	if(index->getObjectType()!=T_INTEGER)
	{
		LOG(ERROR,"Type mismatch");
		abort();
	}

	ASObject* ret=new ASString(obj->getNameAt(index->toInt()-1));
	obj->decRef();
	index->decRef();
	return ret;
}

void ABCVm::newClass(call_context* th, int n)
{
	LOG(CALLS, "newClass " << n );
	method_info* constructor=&th->context->methods[th->context->instances[n].init];
	int name_index=th->context->instances[n].name;
	assert(name_index);
	const multiname* mname=th->context->getMultiname(name_index,NULL);

	Class_base* ret=new Class_inherit(mname->name_s);
//	Class_base* ret=Class<IInterface>::getClass(mname->name_s);
	ASObject* tmp=th->runtime_stack_pop();

	//Null is a "valid" base class
	if(tmp->getObjectType()!=T_NULL)
	{
		assert(tmp->getObjectType()==T_CLASS);
		ret->super=static_cast<Class_base*>(tmp);
		ret->max_level=tmp->max_level+1;
	}

	method_info* m=&th->context->methods[th->context->classes[n].cinit];
	IFunction* cinit=new SyntheticFunction(m);
	LOG(CALLS,"Building class traits");
	for(int i=0;i<th->context->classes[n].trait_count;i++)
		th->context->buildTrait(ret,&th->context->classes[n].traits[i]);

	//add Constructor the the class methods
	ret->constructor=new SyntheticFunction(constructor);
	ret->class_index=n;

	//add implemented interfaces
	for(int i=0;i<th->context->instances[n].interface_count;i++)
	{
		multiname* name=th->context->getMultiname(th->context->instances[n].interfaces[i],NULL);
		ret->addImplementedInterface(*name);

		//Mke the class valid if needed
		ASObject* owner;
		ASObject* obj=th->context->Global->getVariableByMultiname(*name,owner);
		assert(owner);
		if(obj->getObjectType()==T_DEFINABLE)
		{
			LOG(CALLS,"Class " << *name << " is not yet valid (as interface)");
			Definable* d=static_cast<Definable*>(obj);
			d->define(th->context->Global);
			LOG(CALLS,"End of deferred init of class " << *name);
			obj=th->context->Global->getVariableByMultiname(*name,owner);
			assert(owner);
		}
	}

	LOG(CALLS,"Calling Class init " << ret);
	cinit->call(ret,NULL);
	LOG(CALLS,"End of Class init " << ret);
	th->runtime_stack_push(ret);
}

void ABCVm::asTypelate(call_context* th)
{
	LOG(NOT_IMPLEMENTED,"asTypelate");
	ASObject* c=th->runtime_stack_pop();
	c->decRef();
//	ASObject* v=th->runtime_stack_pop();
//	th->runtime_stack_push(v);
}

ASObject* ABCVm::nextValue(ASObject* index, ASObject* obj)
{
	LOG(NOT_IMPLEMENTED,"nextValue");
	if(index->getObjectType()!=T_INTEGER)
	{
		LOG(ERROR,"Type mismatch");
		abort();
	}

	ASObject* ret=obj->getValueAt(index->toInt()-1);
	obj->decRef();
	index->decRef();
	ret->incRef();
	return ret;
}

void ABCVm::swap(call_context* th)
{
	LOG(CALLS,"swap");
}

ASObject* ABCVm::newActivation(call_context* th,method_info* info)
{
	LOG(CALLS,"newActivation");
	//TODO: Should create a real activation object
	//TODO: Should method traits be added to the activation context?
	ASObject* act=new ASObject;
	for(int i=0;i<info->body->trait_count;i++)
		th->context->buildTrait(act,&info->body->traits[i]);

	return act;
}

void ABCVm::popScope(call_context* th)
{
	LOG(CALLS,"popScope");
	th->scope_stack.back()->decRef();
	th->scope_stack.pop_back();
}

