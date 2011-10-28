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
#include "argconv.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("__AS3__.vec");
REGISTER_CLASS_NAME(Vector);

void Vector::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("push",AS3,Class<IFunction>::getFunction(push),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(getLength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(setLength),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
}

void Vector::setTypes(const std::vector<Type*>& types)
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

	Type* type = o_class->getTypes()[0];

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
			ret->vec.push_back( type->coerce(obj) );
		}
		return ret;
	}
	else if(args[0]->getClass()->getTemplate() == Template<Vector>::getTemplate())
	{
		Vector* arg = static_cast<Vector*>(args[0]);

		//create object without calling _constructor
		Vector* ret = o_class->getInstance(false,NULL,0);
		for(auto i = arg->vec.begin(); i != arg->vec.end(); ++i)
		{
			(*i)->incRef();
			ret->vec.push_back( type->coerce(*i) );
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
	uint32_t len;
	bool fixed;
	ARG_UNPACK (len, 0) (fixed, false);
	assert_and_throw(argslen <= 2);
	ASObject::_constructor(obj,NULL,0);

	Vector* th=static_cast< Vector *>(obj);
	assert(th->vec_type);
	th->fixed = fixed;
	th->vec.resize(len);

	/* See setLength */
	for(size_t i=0;i < len; ++i)
		th->vec[i] = th->vec_type->coerce( new Null );

	return NULL;
}

ASFUNCTIONBODY(Vector,push)
{
	Vector* th=static_cast<Vector*>(obj);
	for(size_t i = 0; i < argslen; ++i)
	{
		args[i]->incRef();
		//The proprietary player violates the specification and allows elements of any type to be pushed;
		//they are converted to the vec_type
		th->vec.push_back( th->vec_type->coerce(args[i]) );
	}
	return abstract_ui(th->vec.size());
}

ASFUNCTIONBODY(Vector,getLength)
{
	return abstract_ui(obj->as<Vector>()->vec.size());
}

ASFUNCTIONBODY(Vector,setLength)
{
	Vector* th = obj->as<Vector>();
	uint32_t len;
	ARG_UNPACK (len);
	if(len <= th->vec.size())
	{
		for(size_t i=len; i< th->vec.size(); ++i)
			th->vec[i]->decRef();
		th->vec.resize(len);
	}
	else
	{
		/* We should enlarge the vector
		 * with the type's 'default' value.
		 * That's probably just 'Null' cast to that
		 * type. (Casting Undefined, would be wrong,
		 * because that gave 'NaN' for Number)
		 */
		while(th->vec.size() < len)
			th->vec.push_back( th->vec_type->coerce( new Null ) );
	}
        return NULL;
}

ASFUNCTIONBODY(Vector,_toString)
{
	tiny_string ret;
	Vector* th = obj->as<Vector>();
	for(size_t i=0; i < th->vec.size(); ++i)
	{
		ret += th->vec[i]->toString();
		if(i!=th->vec.size()-1)
			ret += ',';
	}
	return Class<ASString>::getInstanceS(ret);
}
bool Vector::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	if(!considerDynamic)
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	if(name.ns[0].name!="")
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	if(index < vec.size())
		return true;
	else
		return false;
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

void Vector::setVariableByMultiname(const multiname& name, ASObject* o)
{
	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="")
		return ASObject::setVariableByMultiname(name, o);

	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name, o);

	if(index < vec.size())
	{
		vec[index]->decRef();
		vec[index] = o;
	}
	else if(index == vec.size())
	{
		vec.push_back( o );
	}
	else
	{
		/* Spec says: one may not set a value with an index more than
		 * one beyond the current final index. */
		throw Class<ArgumentError>::getInstanceS("Vector accessed out-of-bounds");
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


