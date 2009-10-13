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

using namespace std;

uintptr_t ABCVm::bitAnd(ISWFObject* val2, ISWFObject* val1)
{
	uintptr_t i1=val1->toInt();
	uintptr_t i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"bitAnd_oo " << hex << i1 << '&' << i2);
	return i1&i2;
}

uintptr_t ABCVm::increment(ISWFObject* o)
{
	LOG(CALLS,"increment");

	int n=o->toInt();
	o->decRef();
	return n+1;
}

uintptr_t ABCVm::bitAnd_oi(ISWFObject* val1, intptr_t val2)
{
	uintptr_t i1=val1->toInt();
	uintptr_t i2=val2;
	val1->decRef();
/*	static int c=0;
	if(c%(1024*512)==0)
		cerr << "bitand " << c << endl;
	c++;*/
	LOG(CALLS,"bitAnd_oi " << hex << i1 << '&' << i2);
	return i1&i2;
}

void ABCVm::setProperty(ISWFObject* value,ISWFObject* obj,multiname* name)
{
	LOG(CALLS,"setProperty " << *name);

	obj->setVariableByMultiname(*name,value);
	obj->decRef();
}

void ABCVm::setProperty_i(intptr_t value,ISWFObject* obj,multiname* name)
{
	LOG(CALLS,"setProperty_i " << *name);

	obj->setVariableByMultiname_i(*name,value);
	obj->decRef();
}

ISWFObject* ABCVm::convert_d(ISWFObject* o)
{
	LOG(NOT_IMPLEMENTED, "convert_d" );
	return o;
}

ISWFObject* ABCVm::convert_b(ISWFObject* o)
{
	LOG(TRACE, "convert_b" );
	return o;
}

ISWFObject* ABCVm::convert_u(ISWFObject* o)
{
	LOG(NOT_IMPLEMENTED, "convert_u" );
	return o;
}

ISWFObject* ABCVm::convert_i(ISWFObject* o)
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

ISWFObject* ABCVm::checkfilter(ISWFObject* o)
{
	LOG(NOT_IMPLEMENTED, "checkfilter" );
	return o;
}

ISWFObject* ABCVm::coerce_s(ISWFObject* o)
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

void ABCVm::setSlot(ISWFObject* value, ISWFObject* obj, int n)
{
	LOG(CALLS,"setSlot " << dec << n);
	obj->setSlot(n,value);
	obj->decRef();
}

ISWFObject* ABCVm::getSlot(ISWFObject* obj, int n)
{
	LOG(CALLS,"getSlot " << dec << n);
	ISWFObject* ret=obj->getSlot(n);
	ret->incRef();
	obj->decRef();
	return ret;
}

ISWFObject* ABCVm::negate(ISWFObject* v)
{
	LOG(CALLS, "negate" );
	ISWFObject* ret=new Number(-(v->toNumber()));
	v->decRef();
	return ret;
}

uintptr_t ABCVm::bitNot(ISWFObject* val)
{
	uintptr_t i1=val->toInt();
	val->decRef();
	LOG(CALLS,"bitNot " << hex << i1);
	return ~i1;
}

uintptr_t ABCVm::bitXor(ISWFObject* val2, ISWFObject* val1)
{
	int i1=val1->toInt();
	int i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"bitXor " << hex << i1 << '^' << i2);
	return i1^i2;
}

uintptr_t ABCVm::bitOr_oi(ISWFObject* val2, uintptr_t val1)
{
	int i1=val1;
	int i2=val2->toInt();
	val2->decRef();
	LOG(CALLS,"bitOr " << hex << i1 << '|' << i2);
	return i1|i2;
}

uintptr_t ABCVm::bitOr(ISWFObject* val2, ISWFObject* val1)
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
	ISWFObject** args=new ISWFObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th);
	LOG(CALLS,"callProperty " << *name << ' ' << m);

	ISWFObject* obj=th->runtime_stack_pop();
	ISWFObject* owner;
	ISWFObject* o=obj->getVariableByMultiname(*name,owner);
	if(owner)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=static_cast<IFunction*>(o);
			//Methods as to be runned with their own class this
			ISWFObject* ret=f->fast_call(owner,args,m);
			th->runtime_stack_push(ret);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function");
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
					ISWFObject* ret=f->fast_call(owner,args,m);
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
			IFunction* f=dynamic_cast<IFunction*>(o->getVariableByQName("Call","",owner));
			if(f)
			{
				ISWFObject* ret=f->fast_call(o,args,m);
				th->runtime_stack_push(ret);
			}
			else
			{
				LOG(NOT_IMPLEMENTED,"No such function, returning Undefined");
				th->runtime_stack_push(new Undefined);
			}
		}
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Calling an undefined function");
		th->runtime_stack_push(new Undefined);
	}
	LOG(CALLS,"End of calling " << *name);
	obj->decRef();
	delete[] args;
}

intptr_t ABCVm::getProperty_i(ISWFObject* obj, multiname* name)
{
	LOG(CALLS, "getProperty_i " << *name );

	ISWFObject* owner;
	intptr_t ret=obj->getVariableByMultiname_i(*name,owner);
	if(owner==NULL)
	{
		LOG(NOT_IMPLEMENTED,"Property not found " << *name);
		return 0;
	}
	obj->decRef();
	return ret;
}

ISWFObject* ABCVm::getProperty(ISWFObject* obj, multiname* name)
{
	LOG(CALLS, "getProperty " << *name );

	ISWFObject* owner;
	ISWFObject* ret=obj->getVariableByMultiname(*name,owner);
	if(owner==NULL)
	{
		LOG(NOT_IMPLEMENTED,"Property not found " << *name);
		//abort();
		ret=new Undefined;
	}
	else
	{
		//If we are getting a function object attach the the current scope
		if(ret->getObjectType()==T_FUNCTION)
		{
			LOG(CALLS,"Attaching this to function " << name);
			IFunction* f=ret->clone()->toFunction();
			f->closure_this=owner;
			f->bind();
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

number_t ABCVm::divide(ISWFObject* val2, ISWFObject* val1)
{
	double num1=val1->toNumber();
	double num2=val2->toNumber();

	val1->decRef();
	val2->decRef();
	LOG(CALLS,"divide "  << num1 << '/' << num2);
	return num1/num2;
}

ISWFObject* ABCVm::getGlobalScope(call_context* th)
{
	LOG(CALLS,"getGlobalScope: " << &th->context->Global);
	th->context->Global->incRef();
	return th->context->Global;
}

uintptr_t ABCVm::decrement(ISWFObject* o)
{
	LOG(CALLS,"decrement");

	int n=o->toInt();
	o->decRef();
	return n-1;
}

ISWFObject* ABCVm::decrement_i(ISWFObject* o)
{
	LOG(NOT_IMPLEMENTED,"decrement_i");
	abort();
	return o;
}

bool ABCVm::ifLT(ISWFObject* obj2, ISWFObject* obj1)
{
	LOG(CALLS,"ifLT");

	//Real comparision demanded to object
	bool ret=obj1->isLess(obj2);

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLT_oi(ISWFObject* obj2, intptr_t val1)
{
	LOG(CALLS,"ifLT_oi");

	bool ret=val1<obj2->toInt();

	obj2->decRef();
	return ret;
}

bool ABCVm::ifLT_io(intptr_t val2, ISWFObject* obj1)
{
	LOG(CALLS,"ifLT_io ");

	bool ret=obj1->toInt()<val2;

	obj1->decRef();
	return ret;
}

bool ABCVm::ifNE(ISWFObject* obj1, ISWFObject* obj2)
{
	LOG(CALLS,"ifNE");

	//Real comparision demanded to object
	bool ret=!(obj1->isEqual(obj2));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifNE_oi(ISWFObject* obj1, intptr_t val2)
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

number_t ABCVm::multiply_oi(ISWFObject* val2, intptr_t val1)
{
	double num1=val1;
	double num2=val2->toNumber();
	val2->decRef();
	LOG(CALLS,"multiply "  << num1 << '*' << num2);
	return num1*num2;
}

number_t ABCVm::multiply(ISWFObject* val2, ISWFObject* val1)
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
		Integer* i=dynamic_cast<Integer*>(th->locals[n]);
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

	ISWFObject* o=th->runtime_stack_pop();

	if(o->getObjectType()==T_DEFINABLE)
	{
		LOG(ERROR,"Check");
		abort();
	/*	LOG(CALLS,"Deferred definition of property " << name);
		Definable* d=dynamic_cast<Definable*>(o);
		d->define(obj);
		o=obj->getVariableByMultiname(name,owner);
		LOG(CALLS,"End of deferred definition of property " << name);*/
	}

	LOG(CALLS,"Constructing " << o->class_name);
	//We get a shallow copy of the object, but clean out Variables
	//TODO: should be done in the copy constructor
	ASObject* ret=dynamic_cast<ASObject*>(o->clone());

	ASObject* aso=dynamic_cast<ASObject*>(o);
	ret->prototype=aso;
	aso->incRef();
	if(aso==NULL)
		LOG(ERROR,"Class is not as ASObject");

	if(o->class_index==-2)
	{
		//We have to build the method traits
		SyntheticFunction* sf=static_cast<SyntheticFunction*>(ret);
		LOG(CALLS,"Building method traits");
		for(int i=0;i<sf->mi->body->trait_count;i++)
			th->context->buildTrait(ret,&sf->mi->body->traits[i]);
		sf->call(ret,&args);

	}
	else if(o->class_index==-1)
	{
		//The class is builtin
		LOG(CALLS,"Building a builtin class");
	}
	else
	{
		//The class is declared in the script and has an index
		LOG(CALLS,"Building instance traits");
		for(int i=0;i<th->context->instances[o->class_index].trait_count;i++)
			th->context->buildTrait(ret,&th->context->instances[o->class_index].traits[i]);
	}

	if(o->constructor)
	{
		LOG(CALLS,"Calling Instance init");
		args.incRef();
		o->constructor->call(ret,&args);
//		args.decRef();
	}

	LOG(CALLS,"End of constructing");
	th->runtime_stack_push(ret);
	o->decRef();
}

ISWFObject* ABCVm::typeOf(ISWFObject* obj)
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
	ISWFObject* obj=th->runtime_stack_pop();
	ISWFObject* owner;
	ISWFObject* o=obj->getVariableByMultiname(*name,owner);
	if(owner)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=dynamic_cast<IFunction*>(o);
			f->call(obj,&args);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function");
			th->runtime_stack_push(new Undefined);
		}
		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(NOT_IMPLEMENTED,"We got a function not yet valid");
			th->runtime_stack_push(new Undefined);
		}
		else
		{
			IFunction* f=dynamic_cast<IFunction*>(o->getVariableByQName(".Call","",owner));
			f->call(owner,&args);
		}
	}
	else
		LOG(NOT_IMPLEMENTED,"Calling an undefined function");

	obj->decRef();
	LOG(CALLS,"End of calling " << *name);
}

void ABCVm::jump(call_context* th, int offset)
{
	LOG(CALLS,"jump " << offset);
}

bool ABCVm::ifTrue(ISWFObject* obj1, int offset)
{
	LOG(CALLS,"ifTrue " << offset);
}

intptr_t ABCVm::modulo(ISWFObject* val1, ISWFObject* val2)
{
	int num2=val2->toInt();
	int num1=val1->toInt();

	val1->decRef();
	val2->decRef();
	LOG(CALLS,"modulo "  << num1 << '%' << num2);
	return num1%num2;
}

number_t ABCVm::subtract_oi(ISWFObject* val2, intptr_t val1)
{
	int num2=val2->toInt();
	int num1=val1;

	val2->decRef();
	LOG(CALLS,"subtract " << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_do(number_t val2, ISWFObject* val1)
{
	number_t num2=val2;
	number_t num1=val1->toNumber();

	val1->decRef();
	LOG(CALLS,"subtract " << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract_io(intptr_t val2, ISWFObject* val1)
{
	int num2=val2;
	int num1=val1->toInt();

	val1->decRef();
	LOG(CALLS,"subtract " << num1 << '-' << num2);
	return num1-num2;
}

number_t ABCVm::subtract(ISWFObject* val2, ISWFObject* val1)
{
	int num2=val2->toInt();
	int num1=val1->toInt();

	val1->decRef();
	val2->decRef();
	LOG(CALLS,"subtract " << num1 << '-' << num2);
	return num1-num2;
}

ISWFObject* ABCVm::pushInt(call_context* th, int n)
{
	s32 i=th->context->constant_pool.integer[n];
	LOG(CALLS, "pushInt [" << dec << n << "] " << i);
}

void ABCVm::kill(call_context* th, int n)
{
	LOG(CALLS, "kill " << n );
}

ISWFObject* ABCVm::add(ISWFObject* val2, ISWFObject* val1)
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
		string a(val1->toString());
		string b(val2->toString());
		val1->decRef();
		val2->decRef();
		LOG(CALLS,"add " << a << '+' << b);
		return new ASString(a+b);
	}
	else if(val1->getObjectType()==T_ARRAY)
	{
		//Array concatenation
		ASArray* ar=static_cast<ASArray*>(val1);
		ar->push(val2);
		return ar;
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Add between types " << val1->getObjectType() << ' ' << val2->getObjectType());
		return new Undefined;
	}

}

ISWFObject* ABCVm::add_oi(ISWFObject* val2, intptr_t val1)
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
		LOG(NOT_IMPLEMENTED,"Add between integer and " << val2->getObjectType());
		return new Undefined;
	}

}

ISWFObject* ABCVm::add_od(ISWFObject* val2, number_t val1)
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

uintptr_t ABCVm::lShift(ISWFObject* val1, ISWFObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"lShift "<<i2<<"<<"<<i1);
	return i2<<i1;
}

uintptr_t ABCVm::lShift_io(uintptr_t val1, ISWFObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1&0x1f;
	val2->decRef();
	LOG(CALLS,"lShift "<<i2<<"<<"<<i1);
	return i2<<i1;
}

uintptr_t ABCVm::rShift(ISWFObject* val1, ISWFObject* val2)
{
	int32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"rShift "<<i2<<">>"<<i1);
	return i2>>i1;
}

uintptr_t ABCVm::urShift(ISWFObject* val1, ISWFObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1->toInt()&0x1f;
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"urShift "<<i2<<">>"<<i1);
	return i2>>i1;
}

uintptr_t ABCVm::urShift_io(uintptr_t val1, ISWFObject* val2)
{
	uint32_t i2=val2->toInt();
	int32_t i1=val1&0x1f;
	val2->decRef();
	LOG(CALLS,"urShift "<<i2<<">>"<<i1);
	return i2>>i1;
}

bool ABCVm::_not(ISWFObject* v)
{
	LOG(CALLS, "not" );
	bool ret=!Boolean_concrete(v);
	v->decRef();
	return ret;
}

bool ABCVm::equals(ISWFObject* val2, ISWFObject* val1)
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

ISWFObject* ABCVm::pushNaN(call_context* th)
{
	LOG(NOT_IMPLEMENTED, "pushNaN" );
	return new Undefined;
}

bool ABCVm::ifGT(ISWFObject* obj2, ISWFObject* obj1)
{
	LOG(CALLS,"ifGT");

	//Real comparision demanded to object
	bool ret=obj1->isGreater(obj2);

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifNGT(ISWFObject* obj2, ISWFObject* obj1, int offset)
{
	LOG(CALLS,"ifNGT " << offset);

	//Real comparision demanded to object
	bool ret= !(obj1->isGreater(obj2));

	obj2->decRef();
	obj1->decRef();
	return ret;
}

bool ABCVm::ifLE(ISWFObject* obj2, ISWFObject* obj1)
{
	LOG(CALLS,"ifLE");

	//Real comparision demanded to object
	bool ret=obj1->isLess(obj2) || obj1->isEqual(obj2);
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifNLE(ISWFObject* obj2, ISWFObject* obj1, int offset)
{
	LOG(CALLS,"ifNLE " << offset);

	//Real comparision demanded to object
	bool ret=!(obj1->isLess(obj2) || obj1->isEqual(obj2));
	obj1->decRef();
	obj2->decRef();
	return ret;
}

bool ABCVm::ifGE(ISWFObject* obj2, ISWFObject* obj1)
{
	LOG(CALLS,"ifGE");
	abort();
}

bool ABCVm::ifNGE(ISWFObject* obj2, ISWFObject* obj1, int offset)
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
	ISWFObject* value=th->runtime_stack_pop();
	multiname* name=th->context->getMultiname(n,th); 

	LOG(NOT_IMPLEMENTED,"setSuper " << *name);

	ISWFObject* obj=th->runtime_stack_pop();
	ASObject* o2=dynamic_cast<ASObject*>(obj);
	if(o2 && o2->super)
		obj=o2->super;

	obj->setVariableByMultiname(*name,value);

	//TODO: decRef object
}

void ABCVm::getSuper(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th); 

	LOG(NOT_IMPLEMENTED,"getSuper " << *name);

	ISWFObject* obj=th->runtime_stack_pop();
	ASObject* o2=dynamic_cast<ASObject*>(obj);
	if(o2 && o2->super)
		obj=o2->super;

	ISWFObject* owner;
	ISWFObject* ret=obj->getVariableByMultiname(*name,owner);

	if(owner)
	{
		ret->incRef();
		th->runtime_stack_push(ret);
	}
	else
		th->runtime_stack_push(new Undefined);

	//TODO: decRef object
}

void ABCVm::getLex(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(CALLS, "getLex: " << *name );
	vector<ISWFObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ISWFObject* owner;
	for(it;it!=th->scope_stack.rend();it++)
	{
		assert(!(*it)->class_name.empty());
		ISWFObject* o=(*it)->getVariableByMultiname(*name,owner);
		if(owner)
		{
			//If we are getting a function object attach the the current scope
			if(o->getObjectType()==T_FUNCTION)
			{
				LOG(CALLS,"Attaching this to function " << name);
				IFunction* f=o->clone()->toFunction();
				f->closure_this=(*it);
				f->bind();
			}
			else if(o->getObjectType()==T_DEFINABLE)
			{
				LOG(CALLS,"Deferred definition of property " << *name);
				Definable* d=dynamic_cast<Definable*>(o);
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
		ISWFObject* o2=th->context->vm->Global.getVariableByMultiname(*name,owner);
		if(owner)
		{
			if(o2->getObjectType()==T_DEFINABLE)
			{
				LOG(CALLS,"Deferred definition of property " << *name);
				Definable* d=dynamic_cast<Definable*>(o2);
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

	ASObject* obj=dynamic_cast<ASObject*>(th->runtime_stack_pop());

	if(obj==NULL)
	{
		LOG(CALLS,"Not an ASObject. Aborting");
		abort();
	}

	if(obj->prototype==NULL)
	{
		LOG(CALLS,"No prototype. Returning");
		return;
	}

	//The prototype of the new super instance is the super of the prototype
	ASObject* super=obj->prototype->super;

	//Check if the super is the one we expect
	int super_name=th->context->instances[obj->prototype->class_index].supername;
	if(super_name)
	{
		const multiname* mname=th->context->getMultiname(super_name,NULL);
		const tiny_string& sname=mname->name_s;
		ISWFObject* owner;
		ISWFObject* real_super=th->context->Global->getVariableByMultiname(*mname,owner);
		if(owner)
		{
			if(real_super==super)
			{
				LOG(CALLS,"Same super");
			}
			else
			{
				LOG(CALLS,"Changing super");
				super=dynamic_cast<ASObject*>(real_super);
			}
		}
		else
		{
			LOG(ERROR,"Super not found");
			abort();
		}
	}
	else
	{
		LOG(ERROR,"No super");
		abort();
	}

	if(super->class_index!=-1)
	{
		obj->super=new ASObject;
		//Propagate injection super
		obj->super->super_inject=obj->super_inject;
		obj->super->prototype=super;
		super->incRef();

		multiname* name=th->context->getMultiname(th->context->instances[super->class_index].name,th);
		LOG(CALLS,"Constructing super " << *name << " obj " << obj->super << " on obj " << obj);
		obj->super->class_name=name->name_s;
		LOG(CALLS,"Building instance traits");
		for(int i=0;i<th->context->instances[super->class_index].trait_count;i++)
			th->context->buildTrait(obj->super,&th->context->instances[super->class_index].traits[i]);
		LOG(CALLS,"Calling Instance init");
		//args.incRef();
		super->constructor->call(obj->super,&args);
		//args.decRef();

		LOG(CALLS,"End of constructing " << *name);
	}
	else
	{
		assert(!super->class_name.empty());
		LOG(CALLS,"Builtin super " << super->class_name);
		if(obj->super_inject)
		{
			//Check if we have to inject the super
			ASObject* cur=obj->super_inject;
			while(cur!=NULL)
			{
				assert(!cur->class_name.empty());
				if(cur->class_name==super->class_name)
				{
					//Ok, we have to inject the super here
					obj->super=obj->super_inject;
					obj->super_inject=NULL;
					break;
				}
				cur=cur->super;
			}
			//Assert if no suitable injection point has been found
			if(cur==NULL)
			{
				//HACK: insert here anyway
				obj->super=obj->super_inject;
				LOG(CALLS,"HACK: Injecting before " << obj->class_name);
			}
		}
		else
		{
			obj->super=dynamic_cast<ASObject*>(super->clone());
			LOG(CALLS,"Calling Instance init");
			//args.incRef();
			if(super->constructor)
				super->constructor->call(obj->super,&args);
			//args.decRef();
		}
	}
	LOG(CALLS,"End super construct ");
	obj->decRef();
}

void ABCVm::findPropStrict(call_context* th, int n)
{
	multiname* name=th->context->getMultiname(n,th);
	LOG(CALLS, "findPropStrict " << *name );

	vector<ISWFObject*>::reverse_iterator it=th->scope_stack.rbegin();
	ISWFObject* owner;
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

ISWFObject* ABCVm::greaterThan(ISWFObject* obj1, ISWFObject* obj2)
{
	LOG(CALLS,"greaterThan");

	//Real comparision demanded to object
	bool ret=obj1->isGreater(obj2);
	obj1->decRef();
	obj2->decRef();
	return new Boolean(ret);
}

ISWFObject* ABCVm::greaterEquals(ISWFObject* obj1, ISWFObject* obj2)
{
	LOG(CALLS,"greaterEquals");

	//Real comparision demanded to object
	bool ret=(obj1->isGreater(obj2) || obj1->isEqual(obj2));
	obj1->decRef();
	obj2->decRef();
	return new Boolean(ret);
}

ISWFObject* ABCVm::lessEquals(ISWFObject* obj1, ISWFObject* obj2)
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
	ISWFObject* value=th->runtime_stack_pop();
	multiname* name=th->context->getMultiname(n,th);
	LOG(CALLS, "initProperty " << *name );

	ISWFObject* obj=th->runtime_stack_pop();

	obj->setVariableByMultiname(*name,value);
	obj->decRef();
}

void ABCVm::callSuper(call_context* th, int n, int m)
{
	ISWFObject** args=new ISWFObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th); 
	LOG(NOT_IMPLEMENTED,"callSuper " << *name << ' ' << m);

	ISWFObject* receiver=th->runtime_stack_pop();
	ASObject* o2=dynamic_cast<ASObject*>(receiver);
	ISWFObject* obj;
	if(o2 && o2->super)
		obj=o2->super;
	else
		obj=receiver;

	ISWFObject* owner;
	ISWFObject* o=obj->getVariableByMultiname(*name,owner);
	if(owner)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=static_cast<IFunction*>(o);
			ISWFObject* ret=f->fast_call(owner,args,m);
			th->runtime_stack_push(ret);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function");
			th->runtime_stack_push(new Undefined);
		}
		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(CALLS,"We got a function not yet valid");
			Definable* d=static_cast<Definable*>(o);
			d->define(obj);
			IFunction* f=obj->getVariableByMultiname(*name,owner)->toFunction();
			if(f)
			{
				ISWFObject* ret=f->fast_call(owner,args,m);
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
			IFunction* f=dynamic_cast<IFunction*>(o->getVariableByQName("Call","",owner));
			if(f)
			{
				ISWFObject* ret=f->fast_call(o,args,m);
				th->runtime_stack_push(ret);
			}
			else
			{
				LOG(NOT_IMPLEMENTED,"No such function, returning Undefined");
				th->runtime_stack_push(new Undefined);
			}
		}
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Calling an undefined function");
		th->runtime_stack_push(new Undefined);
	}
	LOG(CALLS,"End of calling " << name);
	receiver->decRef();
	delete[] args;
}

void ABCVm::callSuperVoid(call_context* th, int n, int m)
{
	ISWFObject** args=new ISWFObject*[m];
	for(int i=0;i<m;i++)
		args[m-i-1]=th->runtime_stack_pop();

	multiname* name=th->context->getMultiname(n,th); 
	LOG(NOT_IMPLEMENTED,"callSuperVoid " << *name << ' ' << m);

	ISWFObject* receiver=th->runtime_stack_pop();
	ASObject* o2=dynamic_cast<ASObject*>(receiver);
	ISWFObject* obj;
	if(o2 && o2->super)
		obj=o2->super;
	else
		obj=receiver;

	ISWFObject* owner;
	ISWFObject* o=obj->getVariableByMultiname(*name,owner);
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
		else if(o->getObjectType()==T_DEFINABLE)
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
			IFunction* f=dynamic_cast<IFunction*>(o->getVariableByQName("Call","",owner));
			if(f)
				f->fast_call(o,args,m);
			else
			{
				LOG(NOT_IMPLEMENTED,"No such function, returning Undefined");
			}
		}
	}
	else
	{
		LOG(NOT_IMPLEMENTED,"Calling an undefined function");
	}
	LOG(CALLS,"End of calling " << name);
	receiver->decRef();
	delete[] args;
}

void ABCVm::isTypelate(call_context* th)
{
	LOG(NOT_IMPLEMENTED,"isTypelate");
	ISWFObject* type=th->runtime_stack_pop();
	ISWFObject* obj=th->runtime_stack_pop();
	cout << "Name " << type->class_name << " type " << type->getObjectType() << endl;
	cout << "Name " << obj->class_name << " type " << obj->getObjectType() << endl;
//	if(type->class_name==obj->class_name)
		th->runtime_stack_push(new Boolean(true));
//	else
//		th->runtime_stack_push(new Boolean(false));
}

bool ABCVm::ifStrictNE(ISWFObject* obj2, ISWFObject* obj1)
{
	LOG(NOT_IMPLEMENTED,"ifStrictNE");
	if(obj1->getObjectType()!=obj2->getObjectType())
		return false;
	return ifNE(obj2,obj1);
}

ISWFObject* ABCVm::in(ISWFObject* val2, ISWFObject* val1)
{
	LOG(NOT_IMPLEMENTED, "in" );
	return new Boolean(true);
}

