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
	LOG(CALLS,"bitAnd " << hex << i1 << '&' << i2);
	return i1&i2;
}

uintptr_t ABCVm::bitAnd_oi(ISWFObject* val1, intptr_t val2)
{
	uintptr_t i1=val1->toInt();
	uintptr_t i2=val2;
	val1->decRef();
	LOG(CALLS,"bitAnd " << hex << i1 << '&' << i2);
	return i1&i2;
}

void ABCVm::setProperty(ISWFObject* value,ISWFObject* obj,multiname* name)
{
	LOG(CALLS,"setProperty " << *name);

	//Check to see if a proper setter method is available
	ISWFObject* owner;
/*	if(name->namert)
		name->name=name->namert->toString();
	IFunction* f=obj->getSetterByName(name->name,owner);
	if(owner)
	{
		abort();
		//Call the setter
		LOG(CALLS,"Calling the setter");
		arguments args(1);
		args.set(0,value);
		//TODO: check
		value->incRef();
		//

		f->call(owner,&args);
		LOG(CALLS,"End of setter");
	}
	else*/
	{
		//No setter, just assign the variable
		obj->setVariableByMultiname(*name,value);
	}
	obj->decRef();

	if(name->namert)
	{
		name->namert->decRef();
		name->namert=NULL;
	}
}

ISWFObject* ABCVm::increment(ISWFObject* o)
{
	LOG(CALLS,"increment");

	int n=o->toInt();
	cout << "ref_count " << o->ref_count << endl;
	cout << "incrementing " << n << endl;
	o->decRef();
	ISWFObject* ret=new Integer(n+1);
	return ret;
}

void ABCVm::setProperty_i(intptr_t value,ISWFObject* obj,multiname* name)
{
	LOG(CALLS,"setProperty_i " << *name);

	//Check to see if a proper setter method is available
	ISWFObject* owner;
/*	if(name->namert)
		name->name=name->namert->toString();
	IFunction* f=obj->getSetterByName(name->name,owner);
	if(owner)
	{
		abort();
		//Call the setter
		LOG(CALLS,"Calling the setter");
		arguments args(1);
		args.set(0,value);
		//TODO: check
		value->incRef();
		//

		f->call(owner,&args);
		LOG(CALLS,"End of setter");
	}
	else*/
	{
		//No setter, just assign the variable
		obj->setVariableByMultiname_i(*name,value);
	}
	obj->decRef();

	if(name->namert)
	{
		name->namert->decRef();
		name->namert=NULL;
	}
}

ISWFObject* ABCVm::convert_d(ISWFObject* o)
{
	LOG(NOT_IMPLEMENTED, "convert_d" );
	return o;
}

ISWFObject* ABCVm::convert_b(ISWFObject* o)
{
	LOG(NOT_IMPLEMENTED, "convert_b" );
	return o;
}

ISWFObject* ABCVm::convert_i(ISWFObject* o)
{
	LOG(NOT_IMPLEMENTED, "convert_i" );
	return o;
}

void ABCVm::coerce_a()
{
	LOG(NOT_IMPLEMENTED, "coerce_a" );
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

ISWFObject* ABCVm::setSlot(ISWFObject* value, ISWFObject* obj, int n)
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

ISWFObject* ABCVm::bitXor(ISWFObject* val2, ISWFObject* val1)
{
	int i1=val1->toInt();
	int i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"bitXor " << hex << i1 << '^' << i2);
	return new Integer(i1^i2);
}

ISWFObject* ABCVm::bitOr(ISWFObject* val2, ISWFObject* val1)
{
	int i1=val1->toInt();
	int i2=val2->toInt();
	val1->decRef();
	val2->decRef();
	LOG(CALLS,"bitOr " << hex << i1 << '|' << i2);
	return new Integer(i1|i2);
}

void ABCVm::callProperty(call_context* th, int n, int m)
{
	arguments args(m);
	for(int i=0;i<m;i++)
		args.set(m-i-1,th->runtime_stack_pop());

	multiname name=th->context->getMultiname(n);
	LOG(CALLS,"callProperty " << name << ' ' << m);

	ISWFObject* obj=th->runtime_stack_pop();
	ISWFObject* owner;
	ISWFObject* o=obj->getVariableByMultiname(name,owner);
	if(owner)
	{
		//If o is already a function call it, otherwise find the Call method
		if(o->getObjectType()==T_FUNCTION)
		{
			IFunction* f=dynamic_cast<IFunction*>(o);
			ISWFObject* ret=f->call(obj,&args);
			th->runtime_stack_push(ret);
		}
		else if(o->getObjectType()==T_UNDEFINED)
		{
			LOG(NOT_IMPLEMENTED,"We got a Undefined function");
			th->runtime_stack_push(new Undefined);
		}
		else if(o->getObjectType()==T_DEFINABLE)
		{
			LOG(NOT_IMPLEMENTED,"We got a function not yet valid");
			Definable* d=static_cast<Definable*>(o);
			d->define(obj);
			IFunction* f=obj->getVariableByMultiname(name,owner)->toFunction();
			if(f)
			{
				ISWFObject* ret=f->call(owner,&args);
				th->runtime_stack_push(ret);
			}
			else
				abort();
		}
		else
		{
			IFunction* f=dynamic_cast<IFunction*>(o->getVariableByName("Call",owner));
			if(f)
			{
				ISWFObject* ret=f->call(o,&args);
				th->runtime_stack_push(ret);
			}
			else
			{
				LOG(CALLS,"No such function, returning Undefined");
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
	obj->decRef();
}

void ABCVm::getProperty(call_context* th, int n)
{
	multiname name=th->context->getMultiname(n,th);
	LOG(CALLS, "getProperty " << name );

	ISWFObject* obj=th->runtime_stack_pop();

	ISWFObject* owner;
	//Check to see if a proper getter method is available
	IFunction* f=obj->getGetterByName(name.name,owner);
	if(owner)
	{
		//Call the getter
		LOG(CALLS,"Calling the getter");
		//arguments args;
		//args.push(value);
		//value->incRef();
		th->runtime_stack_push(f->call(owner,NULL));
		LOG(CALLS,"End of getter");
		obj->decRef();
		return;
	}
	
	ISWFObject* ret=obj->getVariableByMultiname(name,owner);
	if(!owner)
	{
		LOG(NOT_IMPLEMENTED,"Property not found " << name);
		th->runtime_stack_push(new Undefined);
	}
	else
	{
		if(ret->getObjectType()==T_DEFINABLE)
		{
			LOG(ERROR,"Property " << name << " is not yet valid");
			Definable* d=static_cast<Definable*>(ret);
			d->define(obj);
			ret=obj->getVariableByMultiname(name,owner);
		}
		th->runtime_stack_push(ret);
		ret->incRef();
	}
	obj->decRef();
}

ISWFObject* ABCVm::divide(ISWFObject* val2, ISWFObject* val1)
{
	Number num2(val2);
	Number num1(val1);

	val1->decRef();
	val2->decRef();
	LOG(CALLS,"divide "  << num1 << '/' << num2);
	return new Number(num1/num2);
}

ISWFObject* ABCVm::getGlobalScope(call_context* th)
{
	LOG(CALLS,"getGlobalScope: " << &th->context->Global);
	th->context->Global->incRef();
	return th->context->Global;
}

ISWFObject* ABCVm::decrement(ISWFObject* o)
{
	LOG(CALLS,"decrement");

	int n=o->toInt();
	o->decRef();
	return new Integer(n-1);
}

ISWFObject* ABCVm::decrement_i(ISWFObject* o)
{
	LOG(NOT_IMPLEMENTED,"decrement_i");
	abort();
	return o;
}

