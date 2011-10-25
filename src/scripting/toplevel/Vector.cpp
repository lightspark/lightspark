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

#include "Vector.h"
#include "abc.h"
#include "class.h"
#include "parsing/amf3_generator.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("");
REGISTER_CLASS_NAME(Vector);

void Vector::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASObject>::getRef();
	c->setDeclaredMethodByQName("push",AS3,Class<IFunction>::getFunction(push),NORMAL_METHOD,true);
}

void Vector::setTypes(const std::vector<Class_base*>& types)
{
	assert(vec_type == NULL);
	assert_and_throw(types.size() == 1);
	vec_type = types[0];
}

ASObject* Vector::generator(TemplatedClass<Vector>* o_class, ASObject* const* args, const unsigned int argslen)
{
	assert_and_throw(argslen == 1);
	assert_and_throw(args[0]->getClass());
	assert_and_throw(o_class->getTypes().size() == 1);

	Class_base* type = o_class->getTypes()[0];

	if(args[0]->getClass() == Class<Array>::getClass())
	{
		//create object without calling _constructor
		Vector* ret = o_class->getInstance(false,NULL,0);

		Array* a = static_cast<Array*>(args[0]);
		for(int i=0;i<a->size();++i)
		{
			ASObject* obj = a->at(i);
			obj->incRef();
			//Convert the elements of the array to the type of this vector
			//by calling the type's generator
			ret->vec.push_back( type->generator(&obj,1) );
		}
		return ret;
	}
	else if(args[0]->getClass()->getTemplate() == Template<Vector>::getTemplate())
	{
		Vector* arg = static_cast<Vector*>(args[0]);
		TemplatedClass<Vector>* arg_class = static_cast<TemplatedClass<Vector>*>(arg->getClass());
		Class_base* arg_type = arg_class->getTypes()[0];

		if(!arg_type->isSubClass(type))
			throw ArgumentError("Cannot convert one Vector into another because type of first is not subclass of later");

		//create object without calling _constructor
		Vector* ret = o_class->getInstance(false,NULL,0);
		for(auto i = arg->vec.begin(); i != arg->vec.end(); ++i)
		{
			(*i)->incRef();
			ret->vec.push_back( *i );
		}
		return ret;
	}
	else
	{
		throw ArgumentError("global Vector() function takes Array or Vector");
	}
}

ASFUNCTIONBODY(Vector,_constructor)
{
	//length:uint = 0, fixed:Boolean = false
	assert_and_throw(argslen <= 2);
	ASObject::_constructor(obj,NULL,0);

	Vector* th=static_cast< Vector *>(obj);
	assert(th->vec_type);
	//TODO
	th->fixed = false;
	return NULL;
}

ASFUNCTIONBODY(Vector,push)
{
	Vector* th=static_cast<Vector*>(obj);
	for(size_t i = 0; i < argslen; ++i)
	{
		//The proprietary player violates the specification and allows elements of any type to be pushed;
		//they are converted to the vec_type
		if(args[i]->getClass()->isSubClass(th->vec_type))
			th->vec.push_back( th->vec_type->generator(&args[i],1) );
		else
		{
			args[i]->incRef();
			th->vec.push_back(args[i]);
		}
	}
	return abstract_ui(th->vec.size());
}

/* this handles the [] operator, because vec[12] becomes vec.12 in bytecode */
_NR<ASObject> Vector::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
		return ASObject::getVariableByMultiname(name,opt);

	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="")
		return ASObject::getVariableByMultiname(name,opt);

	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::getVariableByMultiname(name,opt);

	if(index < vec.size())
	{
		vec[index]->incRef();
		return _MNR(vec[index]);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"Vector: Access beyond size");
		return NullRef; //TODO: test if we should throw something or if we should return new Undefined();
	}
}

tiny_string Vector::toString(bool debugMsg)
{
	//TODO: test
	tiny_string t;
	for(size_t i = 0; i < vec.size(); ++i)
	{
		if( i )
			t += ",";
		t += vec[i]->toString();
	}
	return t;
}


