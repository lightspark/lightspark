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

void ABCVm::setProperty_i(intptr_t value,ISWFObject* obj,multiname* name)
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

