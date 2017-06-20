/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/toplevel/Vector.h"
#include "scripting/abc.h"
#include "scripting/class.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

void Vector::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),getLength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),setLength),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fixed","",Class<IFunction>::getFunction(c->getSystemState(),getFixed),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fixed","",Class<IFunction>::getFunction(c->getSystemState(),setFixed),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("concat","",Class<IFunction>::getFunction(c->getSystemState(),_concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("concat",AS3,Class<IFunction>::getFunction(c->getSystemState(),_concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("every","",Class<IFunction>::getFunction(c->getSystemState(),every),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("every",AS3,Class<IFunction>::getFunction(c->getSystemState(),every),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("filter","",Class<IFunction>::getFunction(c->getSystemState(),filter),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("filter",AS3,Class<IFunction>::getFunction(c->getSystemState(),filter),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("forEach","",Class<IFunction>::getFunction(c->getSystemState(),forEach),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("forEach",AS3,Class<IFunction>::getFunction(c->getSystemState(),forEach),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf","",Class<IFunction>::getFunction(c->getSystemState(),indexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),indexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf","",Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("join","",Class<IFunction>::getFunction(c->getSystemState(),join),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("join",AS3,Class<IFunction>::getFunction(c->getSystemState(),join),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("map","",Class<IFunction>::getFunction(c->getSystemState(),_map),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("map",AS3,Class<IFunction>::getFunction(c->getSystemState(),_map),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pop","",Class<IFunction>::getFunction(c->getSystemState(),_pop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pop",AS3,Class<IFunction>::getFunction(c->getSystemState(),_pop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("push","",Class<IFunction>::getFunction(c->getSystemState(),push),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("push",AS3,Class<IFunction>::getFunction(c->getSystemState(),push),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reverse","",Class<IFunction>::getFunction(c->getSystemState(),_reverse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reverse",AS3,Class<IFunction>::getFunction(c->getSystemState(),_reverse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("shift","",Class<IFunction>::getFunction(c->getSystemState(),shift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("shift",AS3,Class<IFunction>::getFunction(c->getSystemState(),shift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice","",Class<IFunction>::getFunction(c->getSystemState(),slice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",AS3,Class<IFunction>::getFunction(c->getSystemState(),slice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("some","",Class<IFunction>::getFunction(c->getSystemState(),some),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("some",AS3,Class<IFunction>::getFunction(c->getSystemState(),some),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sort","",Class<IFunction>::getFunction(c->getSystemState(),_sort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sort",AS3,Class<IFunction>::getFunction(c->getSystemState(),_sort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("splice","",Class<IFunction>::getFunction(c->getSystemState(),splice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("splice",AS3,Class<IFunction>::getFunction(c->getSystemState(),splice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unshift","",Class<IFunction>::getFunction(c->getSystemState(),unshift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unshift",AS3,Class<IFunction>::getFunction(c->getSystemState(),unshift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("insertAt",AS3,Class<IFunction>::getFunction(c->getSystemState(),insertAt,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeAt",AS3,Class<IFunction>::getFunction(c->getSystemState(),removeAt,1),NORMAL_METHOD,true);
	

	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("concat",AS3,Class<IFunction>::getFunction(c->getSystemState(),_concat),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("every",AS3,Class<IFunction>::getFunction(c->getSystemState(),every),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("filter",AS3,Class<IFunction>::getFunction(c->getSystemState(),filter),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("forEach",AS3,Class<IFunction>::getFunction(c->getSystemState(),forEach),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("indexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),indexOf),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("join",AS3,Class<IFunction>::getFunction(c->getSystemState(),join),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("map",AS3,Class<IFunction>::getFunction(c->getSystemState(),_map),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("pop",AS3,Class<IFunction>::getFunction(c->getSystemState(),_pop),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("push",AS3,Class<IFunction>::getFunction(c->getSystemState(),push),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("reverse",AS3,Class<IFunction>::getFunction(c->getSystemState(),_reverse),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("shift",AS3,Class<IFunction>::getFunction(c->getSystemState(),shift),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("slice",AS3,Class<IFunction>::getFunction(c->getSystemState(),slice),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("some",AS3,Class<IFunction>::getFunction(c->getSystemState(),some),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("sort",AS3,Class<IFunction>::getFunction(c->getSystemState(),_sort),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("splice",AS3,Class<IFunction>::getFunction(c->getSystemState(),splice),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("unshift",AS3,Class<IFunction>::getFunction(c->getSystemState(),unshift),DYNAMIC_TRAIT);
}

Vector::Vector(Class_base* c, const Type *vtype):ASObject(c,T_OBJECT,SUBTYPE_VECTOR),vec_type(vtype),fixed(false),vec(reporter_allocator<ASObject*>(c->memoryAccount))
{
}

Vector::~Vector()
{
}

void Vector::setTypes(const std::vector<const Type *> &types)
{
	assert(vec_type == NULL);
	if(types.size() == 1)
		vec_type = types[0];
}
bool Vector::sameType(const Class_base *cls) const
{
	tiny_string clsname = this->getClass()->getQualifiedClassName();
	return (clsname.startsWith(cls->class_name.getQualifiedName(getSystemState()).raw_buf()));
}

asAtom Vector::generator(asAtom &o_class, asAtom* args, const unsigned int argslen)
{
	assert_and_throw(argslen == 1);
	assert_and_throw(args[0].toObject(o_class.getObject()->getSystemState())->getClass());
	assert_and_throw(o_class.as<TemplatedClass<Vector>>()->getTypes().size() == 1);

	const Type* type = o_class.as<TemplatedClass<Vector>>()->getTypes()[0];

	if(args[0].is<Array>())
	{
		//create object without calling _constructor
		Vector* ret = o_class.as<TemplatedClass<Vector>>()->getInstance(false,NULL,0).as<Vector>();

		Array* a = args[0].as<Array>();
		for(unsigned int i=0;i<a->size();++i)
		{
			asAtom obj = a->at(i);
			ASATOM_INCREF(obj);
			//Convert the elements of the array to the type of this vector
			ret->vec.push_back( type->coerce(o_class.getObject()->getSystemState(),obj).toObject(o_class.getObject()->getSystemState()) );
		}
		return ret;
	}
	else if(args[0].getObject()->getClass()->getTemplate() == Template<Vector>::getTemplate(o_class.getObject()->getSystemState()))
	{
		Vector* arg = args[0].as<Vector>();

		//create object without calling _constructor
		Vector* ret = o_class.as<TemplatedClass<Vector>>()->getInstance(false,NULL,0).as<Vector>();
		for(auto i = arg->vec.begin(); i != arg->vec.end(); ++i)
		{
			(*i)->incRef();
			asAtom v = asAtom::fromObject(*i);
			ret->vec.push_back( type->coerce(o_class.getObject()->getSystemState(),v).toObject(o_class.getObject()->getSystemState()) );
		}
		return ret;
	}
	else
	{
		throwError<ArgumentError>(kCheckTypeFailedError, args[0].toObject(o_class.getObject()->getSystemState())->getClassName(), "Vector");
	}

	return asAtom::invalidAtom;
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
	th->vec.resize(len, NULL);

	return NULL;
}

ASFUNCTIONBODY_ATOM(Vector,_concat)
{
	Vector* th=obj.as<Vector>();
	Vector* ret= th->getClass()->getInstance(true,NULL,0).as<Vector>();
	// copy values into new Vector
	ret->vec.resize(th->size(), NULL);
	auto it=th->vec.begin();
	uint32_t index = 0;
	for(;it != th->vec.end();++it)
	{
		if (*it)
		{
			ret->vec[index]=*it;
			ret->vec[index]->incRef();
		}
		index++;
	}
	//Insert the arguments in the vector
	for(unsigned int i=0;i<argslen;i++)
	{
		if (args[i].is<Vector>())
		{
			Vector* arg=args[i].as<Vector>();
			ret->vec.resize(index+arg->size(), NULL);
			auto it=arg->vec.begin();
			for(;it != arg->vec.end();++it)
			{
				if (*it)
				{
					// force Class_base to ensure that a TypeError is thrown 
					// if the object type does not match the base vector type
					asAtom v = asAtom::fromObject(*it);
					ret->vec[index]=((Class_base*)th->vec_type)->Class_base::coerce(th->getSystemState(),v).toObject(th->getSystemState());
					ret->vec[index]->incRef();
				}
				index++;
			}
		}
		else
		{
			ret->vec[index] = th->vec_type->coerce(th->getSystemState(),args[i]).toObject(th->getSystemState());
			ret->vec[index]->incRef();
			index++;
		}
	}	
	return asAtom::fromObject(ret);
}

ASFUNCTIONBODY_ATOM(Vector,filter)
{
	if (argslen < 1 || argslen > 2)
		throwError<ArgumentError>(kWrongArgumentCountError, "Vector.filter", "1", Integer::toString(argslen));
	if (!args[0].is<IFunction>())
		throwError<TypeError>(kCheckTypeFailedError, args[0].toObject(obj.getObject()->getSystemState())->getClassName(), "Function");
	Vector* th=static_cast<Vector*>(obj.getObject());
	  
	asAtom f = args[0];
	asAtom params[3];
	Vector* ret= th->getClass()->getInstance(true,NULL,0).as<Vector>();
	asAtom funcRet;

	for(unsigned int i=0;i<th->size();i++)
	{
		if (!th->vec[i])
			continue;
		params[0] = asAtom::fromObject(th->vec[i]);
		th->vec[i]->incRef();
		params[1] = asAtom(i);
		params[2] = asAtom::fromObject(th);
		th->incRef();

		if(argslen==1)
		{
			funcRet=f.callFunction(asAtom::nullAtom, params, 3);
		}
		else
		{
			funcRet=f.callFunction(args[1], params, 3);
		}
		if(funcRet.type != T_INVALID)
		{
			if(funcRet.Boolean_concrete())
			{
				th->vec[i]->incRef();
				ret->vec.push_back(th->vec[i]);
			}
			ASATOM_DECREF(funcRet);
		}
	}
	return asAtom::fromObject(ret);
}

ASFUNCTIONBODY_ATOM(Vector, some)
{
	if (argslen < 1)
		throwError<ArgumentError>(kWrongArgumentCountError, "Vector.some", "1", Integer::toString(argslen));
	if (!args[0].is<IFunction>())
		throwError<TypeError>(kCheckTypeFailedError, args[0].getObject()->getClassName(), "Function");
	Vector* th=static_cast<Vector*>(obj.getObject());
	asAtom f = args[0];
	asAtom params[3];
	asAtom funcRet;

	for(unsigned int i=0; i < th->size(); i++)
	{
		if (!th->vec[i])
			continue;
		params[0] = asAtom::fromObject(th->vec[i]);
		th->vec[i]->incRef();
		params[1] = asAtom(i);
		params[2] = asAtom::fromObject(th);
		th->incRef();

		if(argslen==1)
		{
			funcRet=f.callFunction(asAtom::nullAtom, params, 3);
		}
		else
		{
			funcRet=f.callFunction(args[1], params, 3);
		}
		if(funcRet.type != T_INVALID)
		{
			if(funcRet.Boolean_concrete())
			{
				return funcRet;
			}
			ASATOM_DECREF(funcRet);
		}
	}
	return asAtom::falseAtom;
}

ASFUNCTIONBODY_ATOM(Vector, every)
{
	Vector* th=static_cast<Vector*>(obj.getObject());
	if (argslen < 1)
		throwError<ArgumentError>(kWrongArgumentCountError, "Vector.some", "1", Integer::toString(argslen));
	if (!args[0].is<IFunction>())
		throwError<TypeError>(kCheckTypeFailedError, args[0].toObject(th->getSystemState())->getClassName(), "Function");
	asAtom f = args[0];
	asAtom params[3];
	asAtom funcRet;

	for(unsigned int i=0; i < th->size(); i++)
	{
		if (th->vec[i])
		{
			params[0] = asAtom::fromObject(th->vec[i]);
			th->vec[i]->incRef();
		}
		else
			params[0] = asAtom::nullAtom;
		params[1] = asAtom(i);
		params[2] = asAtom::fromObject(th);
		th->incRef();

		if(argslen==1)
		{
			funcRet=f.callFunction(asAtom::nullAtom, params, 3);
		}
		else
		{
			funcRet=f.callFunction(args[1], params, 3);
		}
		if(funcRet.type != T_INVALID)
		{
			if (funcRet.type == T_UNDEFINED || funcRet.type == T_NULL)
				throwError<TypeError>(kCallOfNonFunctionError, funcRet.toString());
			if(!funcRet.Boolean_concrete())
			{
				return funcRet;
			}
			ASATOM_DECREF(funcRet);
		}
	}
	return asAtom::trueAtom;
}

void Vector::append(ASObject *o)
{
	if (fixed)
	{
		o->decRef();
		throwError<RangeError>(kVectorFixedError);
	}

	asAtom v = asAtom::fromObject(o);
	vec.push_back(vec_type->coerce(getSystemState(),v).toObject(getSystemState()));
}

ASFUNCTIONBODY_ATOM(Vector,push)
{
	Vector* th=static_cast<Vector*>(obj.getObject());
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	for(size_t i = 0; i < argslen; ++i)
	{
		ASATOM_INCREF(args[i]);
		//The proprietary player violates the specification and allows elements of any type to be pushed;
		//they are converted to the vec_type
		th->vec.push_back( th->vec_type->coerce(th->getSystemState(),args[i]).toObject(th->getSystemState()) );
	}
	return asAtom((uint32_t)th->vec.size());
}

ASFUNCTIONBODY_ATOM(Vector,_pop)
{
	Vector* th=obj.as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	uint32_t size =th->size();
	asAtom natom(T_NULL);
	if (size == 0)
		return th->vec_type->coerce(th->getSystemState(),natom);
	ASObject* ret = th->vec[size-1];
	if (!ret)
		ret = th->vec_type->coerce(th->getSystemState(),natom).toObject(th->getSystemState());
	th->vec.pop_back();
	return asAtom::fromObject(ret);
}

ASFUNCTIONBODY(Vector,getLength)
{
	return abstract_ui(obj->getSystemState(),obj->as<Vector>()->vec.size());
}

ASFUNCTIONBODY(Vector,setLength)
{
	Vector* th = obj->as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	uint32_t len;
	ARG_UNPACK (len);
	if(len <= th->vec.size())
	{
		for(size_t i=len; i< th->vec.size(); ++i)
			if(th->vec[i])
				th->vec[i]->decRef();
	}
	th->vec.resize(len, NULL);
	return NULL;
}

ASFUNCTIONBODY(Vector,getFixed)
{
	return abstract_b(obj->getSystemState(),obj->as<Vector>()->fixed);
}

ASFUNCTIONBODY(Vector,setFixed)
{
	Vector* th = obj->as<Vector>();
	bool fixed;
	ARG_UNPACK (fixed);
	th->fixed = fixed;
	return NULL;
}

ASFUNCTIONBODY_ATOM(Vector,forEach)
{
	Vector* th=static_cast<Vector*>(obj.getObject());
	if (argslen < 1)
		throwError<ArgumentError>(kWrongArgumentCountError, "Vector.forEach", "1", Integer::toString(argslen));
	if (!args[0].is<IFunction>())
		throwError<TypeError>(kCheckTypeFailedError, args[0].toObject(th->getSystemState())->getClassName(), "Function");
	asAtom f = args[0];
	asAtom params[3];

	for(unsigned int i=0; i < th->size(); i++)
	{
		if (!th->vec[i])
			continue;
		params[0] = asAtom::fromObject(th->vec[i]);
		th->vec[i]->incRef();
		params[1] = asAtom(i);
		params[2] = th;
		th->incRef();

		asAtom funcret;
		if( argslen == 1 )
		{
			funcret=f.callFunction(asAtom::nullAtom, params, 3);
		}
		else
		{
			funcret=f.callFunction(args[1], params, 3);
		}
		ASATOM_DECREF(funcret);
	}

	return asAtom::invalidAtom;
}

ASFUNCTIONBODY(Vector, _reverse)
{
	Vector* th = static_cast<Vector*>(obj);

	std::vector<ASObject*> tmp = std::vector<ASObject*>(th->vec.begin(),th->vec.end());
	uint32_t size = th->size();
	th->vec.clear();
	th->vec.resize(size, NULL);
	std::vector<ASObject*>::iterator it=tmp.begin();
	uint32_t index = size-1;
	for(;it != tmp.end();++it)
 	{
		th->vec[index]=*it;
		index--;
	}
	th->incRef();
	return th;
}

ASFUNCTIONBODY(Vector,lastIndexOf)
{
	Vector* th=static_cast<Vector*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	int ret=-1;
	ASObject* arg0=args[0];

	if(th->vec.size() == 0)
		return abstract_di(obj->getSystemState(),-1);

	size_t i = th->size()-1;

	if(argslen == 2 && std::isnan(args[1]->toNumber()))
		return abstract_i(obj->getSystemState(),0);

	if(argslen == 2 && args[1]->getObjectType() != T_UNDEFINED && !std::isnan(args[1]->toNumber()))
	{
		int j = args[1]->toInt(); //Preserve sign
		if(j < 0) //Negative offset, use it as offset from the end of the array
		{
			if((size_t)-j > th->size())
				i = 0;
			else
				i = th->size()+j;
		}
		else //Positive offset, use it directly
		{
			if((size_t)j > th->size()) //If the passed offset is bigger than the array, cap the offset
				i = th->size()-1;
			else
				i = j;
		}
	}
	do
	{
		if (!th->vec[i])
		    continue;
		if (th->vec[i]->isEqualStrict(arg0))
		{
			ret=i;
			break;
		}
	}
	while(i--);

	return abstract_i(obj->getSystemState(),ret);
}

ASFUNCTIONBODY_ATOM(Vector,shift)
{
	Vector* th=obj.as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	asAtom natom(T_NULL);
	if(!th->size())
		return th->vec_type->coerce(th->getSystemState(),natom);
	asAtom ret;
	if(th->vec[0])
		ret=asAtom::fromObject(th->vec[0]);
	else
		ret=th->vec_type->coerce(th->getSystemState(),natom);
	for(uint32_t i= 1;i< th->size();i++)
	{
		if (th->vec[i])
		{
			th->vec[i-1]=th->vec[i];
		}
		else if (th->vec[i-1])
		{
			th->vec[i-1] = NULL;
		}
	}
	th->vec.resize(th->size()-1, NULL);
	return ret;
}

int Vector::capIndex(int i) const
{
	int totalSize=size();

	if(totalSize <= 0)
		return 0;
	else if(i < -totalSize)
		return 0;
	else if(i > totalSize)
		return totalSize;
	else if(i>=0)     // 0 <= i < totalSize
		return i;
	else              // -totalSize <= i < 0
	{
		//A negative i is relative to the end
		return i+totalSize;
	}
}

ASFUNCTIONBODY_ATOM(Vector,slice)
{
	Vector* th=obj.as<Vector>();

	int startIndex=0;
	int endIndex=16777215;
	if(argslen>0)
		startIndex=args[0].toInt();
	if(argslen>1)
		endIndex=args[1].toInt();

	startIndex=th->capIndex(startIndex);
	endIndex=th->capIndex(endIndex);
	Vector* ret= th->getClass()->getInstance(true,NULL,0).as<Vector>();
	ret->vec.resize(endIndex-startIndex, NULL);
	int j = 0;
	for(int i=startIndex; i<endIndex; i++) 
	{
		if (th->vec[i])
		{
			th->vec[i]->incRef();
			asAtom v = asAtom::fromObject(th->vec[i]);
			ret->vec[j] =th->vec_type->coerce(th->getSystemState(),v).toObject(th->getSystemState());
		}
		j++;
	}
	return asAtom::fromObject(ret);
}

ASFUNCTIONBODY(Vector,splice)
{
	Vector* th=static_cast<Vector*>(obj);
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	int startIndex=args[0]->toInt();
	//By default, delete all the element up to the end
	//Use the array len, it will be capped below
	int deleteCount=th->size();
	if(argslen > 1)
		deleteCount=args[1]->toUInt();
	int totalSize=th->size();
	Vector* ret= obj->getClass()->getInstance(true,NULL,0).as<Vector>();

	startIndex=th->capIndex(startIndex);

	if((startIndex+deleteCount)>totalSize)
		deleteCount=totalSize-startIndex;

	ret->vec.resize(deleteCount, NULL);
	if(deleteCount)
	{
		// write deleted items to return array
		for(int i=0;i<deleteCount;i++)
		{
			if (th->vec[startIndex+i])
				ret->vec[i] = th->vec[startIndex+i];
		}
		// delete items from current array
		for (int i = 0; i < deleteCount; i++)
		{
			if(th->vec[startIndex+i])
			{
				th->vec[startIndex+i] = NULL;
			}
		}
	}
	// remember items in current array that have to be moved to new position
	vector<ASObject*> tmp = vector<ASObject*>(totalSize- (startIndex+deleteCount));
	tmp.resize(totalSize- (startIndex+deleteCount), NULL);
	for (int i = startIndex+deleteCount; i < totalSize ; i++)
	{
		if (th->vec[i])
		{
			tmp[i-(startIndex+deleteCount)] = th->vec[i];
			th->vec[i] = NULL;
		}
	}
	th->vec.resize(startIndex, NULL);

	
	//Insert requested values starting at startIndex
	for(unsigned int i=2;i<argslen;i++)
	{
		args[i]->incRef();
		th->vec.push_back(args[i]);
	}
	// move remembered items to new position
	th->vec.resize((totalSize-deleteCount)+(argslen > 2 ? argslen-2 : 0), NULL);
	for(int i=0;i<totalSize- (startIndex+deleteCount);i++)
	{
		if (tmp[i])
			th->vec[startIndex+i+(argslen > 2 ? argslen-2 : 0)] = tmp[i];
	}
	return ret;
}

ASFUNCTIONBODY(Vector,join)
{
	Vector* th=static_cast<Vector*>(obj);
	
	tiny_string del = ",";
	if (argslen == 1)
	      del=args[0]->toString();
	string ret;
	for(uint32_t i=0;i<th->size();i++)
	{
		if (th->vec[i])
			ret+=th->vec[i]->toString().raw_buf();
		if(i!=th->size()-1)
			ret+=del.raw_buf();
	}
	return abstract_s(obj->getSystemState(),ret);
}

ASFUNCTIONBODY(Vector,indexOf)
{
	Vector* th=static_cast<Vector*>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	int ret=-1;
	ASObject* arg0=args[0];

	int unsigned i = 0;
	if(argslen == 2)
	{
		i = args[1]->toInt();
	}

	for(;i<th->size();i++)
	{
		if (!th->vec[i])
			continue;
		if(th->vec[i]->isEqualStrict(arg0))
		{
			ret=i;
			break;
		}
	}
	return abstract_i(obj->getSystemState(),ret);
}
bool Vector::sortComparatorDefault::operator()(ASObject* d1, ASObject* d2)
{
	if(isNumeric)
	{
		number_t a=d1->toNumber();

		number_t b=d2->toNumber();

		if(std::isnan(a) || std::isnan(b))
			throw RunTimeException("Cannot sort non number with Array.NUMERIC option");
		if(isDescending)
			return b>a;
		else
			return a<b;
	}
	else
	{
		//Comparison is always in lexicographic order
		tiny_string s1 = d1->toString();
		tiny_string s2 = d2->toString();

		if(isDescending)
		{
			//TODO: unicode support
			if(isCaseInsensitive)
				return s1.strcasecmp(s2)>0;
			else
				return s1>s2;
		}
		else
		{
			//TODO: unicode support
			if(isCaseInsensitive)
				return s1.strcasecmp(s2)<0;
			else
				return s1<s2;
		}
	}
}
bool Vector::sortComparatorWrapper::operator()(ASObject* d1, ASObject* d2)
{
	asAtom objs[2];
	if (d1)
	{
		objs[0] = asAtom::fromObject(d1);
		ASATOM_INCREF(objs[0]);
	}
	else
		objs[0] = asAtom::nullAtom;
	if (d2)
	{
		objs[1] = asAtom::fromObject(d2);
		ASATOM_INCREF(objs[1]);
	}
	else
		objs[1] = asAtom::nullAtom;

	asAtom ret= comparator.callFunction(asAtom::nullAtom, objs, 2);
	assert_and_throw(ret.type != T_INVALID);
	return (ret.toNumber()<0); //Less
}

ASFUNCTIONBODY_ATOM(Vector,_sort)
{
	if (argslen != 1)
		throwError<ArgumentError>(kWrongArgumentCountError, "Vector.sort", "1", Integer::toString(argslen));
	Vector* th=static_cast<Vector*>(obj.getObject());
	
	asAtom comp;
	bool isNumeric=false;
	bool isCaseInsensitive=false;
	bool isDescending=false;
	if(args[0].type==T_FUNCTION) //Comparison func
	{
		assert_and_throw(comp.type==T_INVALID);
		comp=args[0];
	}
	else
	{
		uint32_t options=args[0].toInt();
		if(options&Array::NUMERIC)
			isNumeric=true;
		if(options&Array::CASEINSENSITIVE)
			isCaseInsensitive=true;
		if(options&Array::DESCENDING)
			isDescending=true;
		if(options&(~(Array::NUMERIC|Array::CASEINSENSITIVE|Array::DESCENDING)))
			throw UnsupportedException("Vector::sort not completely implemented");
	}
	std::vector<ASObject*> tmp = vector<ASObject*>(th->vec.size());
	int i = 0;
	for(auto it=th->vec.begin();it != th->vec.end();++it)
	{
		tmp[i++]= *it;
	}
	
	if(comp.type != T_INVALID)
		sort(tmp.begin(),tmp.end(),sortComparatorWrapper(comp,th->vec_type));
	else
		sort(tmp.begin(),tmp.end(),sortComparatorDefault(isNumeric,isCaseInsensitive,isDescending));

	th->vec.clear();
	for(auto ittmp=tmp.begin();ittmp != tmp.end();++ittmp)
	{
		th->vec.push_back(*ittmp);
	}
	ASATOM_INCREF(obj);
	return obj;
}

ASFUNCTIONBODY_ATOM(Vector,unshift)
{
	Vector* th=obj.as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	if (argslen > 0)
	{
		th->vec.resize(th->size()+argslen, NULL);
		for(uint32_t i=th->size();i> 0;i--)
		{
			if (th->vec[i-1])
			{
				th->vec[(i-1)+argslen]=th->vec[i-1];
				th->vec[i-1] = NULL;
			}
			
		}
		
		for(uint32_t i=0;i<argslen;i++)
		{
			ASATOM_INCREF(args[i]);
			th->vec[i] = th->vec_type->coerce(th->getSystemState(),args[i]).toObject(th->getSystemState());
		}
	}
	return asAtom((int32_t)th->size());
}

ASFUNCTIONBODY_ATOM(Vector,_map)
{
	Vector* th=static_cast<Vector*>(obj.getObject());
	asAtom func(T_FUNCTION);
	asAtom thisObject;
	
	if (argslen >= 1 && !args[0].is<IFunction>())
		throwError<TypeError>(kCheckTypeFailedError, args[0].toObject(th->getSystemState())->getClassName(), "Function");

	ARG_UNPACK_ATOM(func)(thisObject,asAtom::nullAtom);
	
	Vector* ret= th->getClass()->getInstance(true,NULL,0).as<Vector>();

	for(uint32_t i=0;i<th->size();i++)
	{
		asAtom funcArgs[3];
		if (!th->vec[i])
			funcArgs[0]=asAtom::nullAtom;
		else
		{
			if(th->vec[i])
			{
				funcArgs[0]=asAtom::fromObject(th->vec[i]);
				ASATOM_INCREF(funcArgs[0]);
			}
			else
				funcArgs[0]=asAtom::undefinedAtom;
		}
		funcArgs[1]=asAtom(i);
		funcArgs[2]=asAtom::fromObject(th);
		ASATOM_INCREF(funcArgs[2]);
		asAtom funcRet=func.callFunction(thisObject, funcArgs, 3);
		assert_and_throw(funcRet.type != T_INVALID);
		ret->vec.push_back(funcRet.toObject(th->getSystemState()));
	}

	return asAtom::fromObject(ret);
}

ASFUNCTIONBODY_ATOM(Vector,_toString)
{
	tiny_string ret;
	Vector* th = obj.as<Vector>();
	asAtom natom(T_NULL);
	for(size_t i=0; i < th->vec.size(); ++i)
	{
		if (th->vec[i])
			ret += th->vec[i]->toString();
		else
			// use the type's default value
			ret += th->vec_type->coerce(th->getSystemState(), natom).toString();

		if(i!=th->vec.size()-1)
			ret += ',';
	}
	return asAtom::fromObject(abstract_s(th->getSystemState(),ret));
}

ASFUNCTIONBODY(Vector,insertAt)
{
	Vector* th=static_cast<Vector*>(obj);
	if (th->fixed)
		throwError<RangeError>(kOutOfRangeError);
	int32_t index;
	_NR<ASObject> o;
	ARG_UNPACK(index)(o);

	if (index < 0 && th->vec.size() >= (uint32_t)(-index))
		index = th->vec.size()+(index);
	if (index < 0)
		index = 0;
	LOG(LOG_ERROR,"insertat:"<<index<<" "<<th->vec.size()<<" "<<th->toString());
	if ((uint32_t)index >= th->vec.size())
	{
		o->incRef();
		th->vec.push_back(o.getPtr());
	}
	else
	{
		o->incRef();
		th->vec.insert(th->vec.begin()+index,o.getPtr());
	}
	return NULL;
}

ASFUNCTIONBODY(Vector,removeAt)
{
	Vector* th=static_cast<Vector*>(obj);
	if (th->fixed)
		throwError<RangeError>(kOutOfRangeError);
	int32_t index;
	ARG_UNPACK(index);
	if (index < 0)
		index = th->vec.size()+index;
	if (index < 0)
		index = 0;
	ASObject* o = NULL;
	if ((uint32_t)index < th->vec.size())
	{
		o = th->vec[index];
		th->vec.erase(th->vec.begin()+index);
	}
	else
		throwError<RangeError>(kOutOfRangeError);
		
	return o;
}

bool Vector::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(!considerDynamic)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	if(!name.hasEmptyNS)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	unsigned int index=0;
	if(!Vector::isValidMultiname(getSystemState(),name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	if(index < vec.size())
		return true;
	else
		return false;
}

/* this handles the [] operator, because vec[12] becomes vec.12 in bytecode */
asAtom Vector::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
		return ASObject::getVariableByMultiname(name,opt);

	assert_and_throw(name.ns.size()>0);
	if(!name.hasEmptyNS)
		return ASObject::getVariableByMultiname(name,opt);

	unsigned int index=0;
	if(!Vector::isValidMultiname(getSystemState(),name,index) || index == UINT32_MAX)
	{
		if (name.name_type == multiname::NAME_INT || name.name_type == multiname::NAME_UINT ||
				(name.name_type == multiname::NAME_NUMBER && Number::isInteger(name.name_d)))
			throwError<RangeError>(kOutOfRangeError,Integer::toString(name.name_i),Integer::toString(vec.size()));

		asAtom ret = ASObject::getVariableByMultiname(name,opt);
		if (ret.type == T_INVALID)
			throwError<ReferenceError>(kReadSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
		return ret;
	}
	if(index < vec.size())
	{
		if (vec[index])
		{
			vec[index]->incRef();
			return asAtom::fromObject(vec[index]);
		}
		else
		{
			asAtom natom(T_NULL);
			return vec_type->coerce(getSystemState(), natom );
		}
	}
	else
	{
		throwError<RangeError>(kOutOfRangeError,
				       Integer::toString(index),
				       Integer::toString(vec.size()));
	}

	return asAtom::invalidAtom;
}

void Vector::setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst)
{
	assert_and_throw(name.ns.size()>0);
	if(!name.hasEmptyNS)
		return ASObject::setVariableByMultiname(name, o, allowConst);

	unsigned int index=0;
	if(!Vector::isValidMultiname(getSystemState(),name,index))
	{
		if (name.name_type == multiname::NAME_INT || name.name_type == multiname::NAME_UINT ||
				(name.name_type == multiname::NAME_NUMBER && Number::isInteger(name.name_d)))
			throwError<RangeError>(kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
		
		if (!ASObject::hasPropertyByMultiname(name,false,true))
			throwError<ReferenceError>(kWriteSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
		return ASObject::setVariableByMultiname(name, o, allowConst);
	}
	ASObject* o2 = this->vec_type->coerce(getSystemState(), o).toObject(getSystemState());
	  
	if(index < vec.size())
	{
		if (vec[index])
			vec[index]->decRef();
		vec[index] = o2;
	}
	else if(!fixed && index == vec.size())
	{
		vec.push_back( o2 );
	}
	else
	{
		/* Spec says: one may not set a value with an index more than
		 * one beyond the current final index. */
		throwError<RangeError>(kOutOfRangeError,
				       Integer::toString(index),
				       Integer::toString(vec.size()));
	}
}

tiny_string Vector::toString()
{
	//TODO: test
	tiny_string t;
	asAtom natom(T_NULL);
	for(size_t i = 0; i < vec.size(); ++i)
	{
		if( i )
			t += ",";
		if (vec[i]) 
			t += vec[i]->toString();
		else
			t += vec_type->coerce(getSystemState(), natom ).toString();
	}
	return t;
}

uint32_t Vector::nextNameIndex(uint32_t cur_index)
{
	if(cur_index < vec.size())
		return cur_index+1;
	else
		return 0;
}

asAtom Vector::nextName(uint32_t index)
{
	if(index<=vec.size())
		return asAtom(index-1);
	else
		throw RunTimeException("Vector::nextName out of bounds");
}

asAtom Vector::nextValue(uint32_t index)
{
	if(index<=vec.size())
	{
		if (vec[index-1])
		{
			vec[index-1]->incRef();
			return asAtom::fromObject(vec[index-1]);
		}
		asAtom natom(T_NULL);
		return vec_type->coerce(getSystemState(), natom);
	}
	else
		throw RunTimeException("Vector::nextValue out of bounds");
}

bool Vector::isValidMultiname(SystemState* sys,const multiname& name, uint32_t& index)
{
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
	assert_and_throw(name.ns.size()!=0);
	if(!name.hasEmptyNS)
		return false;

	bool validIndex=name.toUInt(sys,index, true);

	return validIndex;
}

tiny_string Vector::toJSON(std::vector<ASObject *> &path, asAtom replacer, const tiny_string &spaces, const tiny_string &filter)
{
	bool ok;
	tiny_string res = call_toJSON(ok,path,replacer,spaces,filter);
	if (ok)
		return res;
	// check for cylic reference
	if (std::find(path.begin(),path.end(), this) != path.end())
		throwError<TypeError>(kJSONCyclicStructure);

	path.push_back(this);
	res += "[";
	bool bfirst = true;
	tiny_string newline = (spaces.empty() ? "" : "\n");
	for (unsigned int i =0;  i < vec.size(); i++)
	{
		tiny_string subres;
		ASObject* o = vec[i];
		if (!o)
			o = getSystemState()->getNullRef();
		if (replacer.type != T_INVALID)
		{
			asAtom params[2];
			
			params[0] = asAtom(i);
			ASATOM_INCREF(params[0]);
			params[1] = asAtom::fromObject(o);
			asAtom funcret=replacer.callFunction(asAtom::nullAtom, params, 2);
			if (funcret.type != T_INVALID)
				subres = funcret.toObject(getSystemState())->toJSON(path,asAtom::invalidAtom,spaces,filter);
		}
		else
		{
			subres = o->toJSON(path,replacer,spaces,filter);
		}
		if (!subres.empty())
		{
			if (!bfirst)
				res += ",";
			res += newline+spaces;
			
			bfirst = false;
			res += subres;
		}
	}
	if (!bfirst)
		res += newline+spaces.substr_bytes(0,spaces.numBytes()/2);
	res += "]";
	path.pop_back();
	return res;
}

ASObject* Vector::at(unsigned int index, ASObject *defaultValue) const
{
	if (index < vec.size())
		return vec.at(index);
	else
		return defaultValue;
}

void Vector::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	if (out->getObjectEncoding() == ObjectEncoding::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing Vector in AMF0 not implemented");
		return;
	}
	uint8_t marker = 0;
	if (vec_type == Class<Integer>::getClass(getSystemState()))
		marker = vector_int_marker;
	else if (vec_type == Class<UInteger>::getClass(getSystemState()))
		marker = vector_uint_marker;
	else if (vec_type == Class<Number>::getClass(getSystemState()))
		marker = vector_double_marker;
	else
		marker = vector_object_marker;
	out->writeByte(marker);
	//Check if the vector has been already serialized
	auto it=objMap.find(this);
	if(it!=objMap.end())
	{
		//The least significant bit is 0 to signal a reference
		out->writeU29(it->second << 1);
	}
	else
	{
		//Add the Vector to the map
		objMap.insert(make_pair(this, objMap.size()));

		uint32_t count = size();
		assert_and_throw(count<0x20000000);
		uint32_t value = (count << 1) | 1;
		out->writeU29(value);
		out->writeByte(fixed ? 0x01 : 0x00);
		if (marker == vector_object_marker)
		{
			out->writeStringVR(stringMap,vec_type->getName());
		}
		for(uint32_t i=0;i<count;i++)
		{
			if (!vec[i])
			{
				//TODO should we write a null_marker here?
				LOG(LOG_NOT_IMPLEMENTED,"serialize unset vector objects");
				continue;
			}
			switch (marker)
			{
				case vector_int_marker:
					out->writeUnsignedInt(out->endianIn((uint32_t)vec[i]->toInt()));
					break;
				case vector_uint_marker:
					out->writeUnsignedInt(out->endianIn(vec[i]->toUInt()));
					break;
				case vector_double_marker:
					out->serializeDouble(vec[i]->toNumber());
					break;
				case vector_object_marker:
					vec[i]->serialize(out, stringMap, objMap, traitsMap);
					break;
			}
		}
	}
}
