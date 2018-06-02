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
#include "scripting/toplevel/XML.h"
#include <3rdparty/pugixml/src/pugixml.hpp>

using namespace std;
using namespace lightspark;

void Vector::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),getLength,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),setLength,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fixed","",Class<IFunction>::getFunction(c->getSystemState(),getFixed,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fixed","",Class<IFunction>::getFunction(c->getSystemState(),setFixed,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),SETTER_METHOD,true);
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

Vector::Vector(Class_base* c, const Type *vtype):ASObject(c,T_OBJECT,SUBTYPE_VECTOR),vec_type(vtype),fixed(false),vec(reporter_allocator<asAtom>(c->memoryAccount))
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

void Vector::generator(asAtom& ret,SystemState *sys, asAtom &o_class, asAtom* args, const unsigned int argslen)
{
	assert_and_throw(argslen == 1);
	assert_and_throw(args[0].toObject(sys)->getClass());
	assert_and_throw(o_class.as<TemplatedClass<Vector>>()->getTypes().size() == 1);

	const Type* type = o_class.as<TemplatedClass<Vector>>()->getTypes()[0];

	if(args[0].is<Array>())
	{
		//create object without calling _constructor
		o_class.as<TemplatedClass<Vector>>()->getInstance(ret,false,NULL,0);
		Vector* res = ret.as<Vector>();

		Array* a = args[0].as<Array>();
		for(unsigned int i=0;i<a->size();++i)
		{
			asAtom obj = a->at(i);
			ASATOM_INCREF(obj);
			//Convert the elements of the array to the type of this vector
			type->coerce(sys,obj);
			res->vec.push_back(obj);
		}
	}
	else if(args[0].getObject()->getClass()->getTemplate() == Template<Vector>::getTemplate(sys))
	{
		Vector* arg = args[0].as<Vector>();

		Vector* res = NULL;
		if (arg->vec_type == type)
		{
			// according to specs, the argument is returned if it is a vector of the same type as the provided class
			res = arg;
			res->incRef();
			ret = asAtom::fromObject(res);
		}
		else
		{
			//create object without calling _constructor
			o_class.as<TemplatedClass<Vector>>()->getInstance(ret,false,NULL,0);
			res = ret.as<Vector>();
			for(auto i = arg->vec.begin(); i != arg->vec.end(); ++i)
			{
				ASATOM_INCREF((*i));
				type->coerce(sys,*i);
				res->vec.push_back( *i );
			}
		}
	}
	else
	{
		throwError<ArgumentError>(kCheckTypeFailedError, args[0].toObject(sys)->getClassName(), "Vector");
	}
}

ASFUNCTIONBODY_ATOM(Vector,_constructor)
{
	uint32_t len;
	bool fixed;
	ARG_UNPACK_ATOM (len, 0) (fixed, false);
	assert_and_throw(argslen <= 2);

	Vector* th=obj.as< Vector>();
	assert(th->vec_type);
	th->fixed = fixed;
	th->vec.resize(len, asAtom::invalidAtom);
}

ASFUNCTIONBODY_ATOM(Vector,_concat)
{
	Vector* th=obj.as<Vector>();
	th->getClass()->getInstance(ret,true,NULL,0);
	Vector* res = ret.as<Vector>();
	// copy values into new Vector
	res->vec.resize(th->size(), asAtom::invalidAtom);
	auto it=th->vec.begin();
	uint32_t index = 0;
	for(;it != th->vec.end();++it)
	{
		res->vec[index]=*it;
		ASATOM_INCREF(res->vec[index]);
		index++;
	}
	//Insert the arguments in the vector
	int pos = sys->getSwfVersion() < 11 ? argslen-1 : 0;
	for(unsigned int i=0;i<argslen;i++)
	{
		if (args[pos].is<Vector>())
		{
			Vector* arg=args[pos].as<Vector>();
			res->vec.resize(index+arg->size(), asAtom::invalidAtom);
			auto it=arg->vec.begin();
			for(;it != arg->vec.end();++it)
			{
				if (it->type != T_INVALID)
				{
					res->vec[index]= *it;
					th->vec_type->coerceForTemplate(sys,res->vec[index]);
					ASATOM_INCREF(res->vec[index]);
				}
				index++;
			}
		}
		else
		{
			th->vec_type->coerce(sys,args[pos]);
			res->vec[index] = args[pos];
			ASATOM_INCREF(res->vec[index]);
			index++;
		}
		pos += (sys->getSwfVersion() < 11 ?-1 : 1);
	}	
}

ASFUNCTIONBODY_ATOM(Vector,filter)
{
	if (argslen < 1 || argslen > 2)
		throwError<ArgumentError>(kWrongArgumentCountError, "Vector.filter", "1", Integer::toString(argslen));
	if (!args[0].is<IFunction>())
		throwError<TypeError>(kCheckTypeFailedError, args[0].toObject(sys)->getClassName(), "Function");
	Vector* th=obj.as<Vector>();
	  
	asAtom f = args[0];
	asAtom params[3];
	th->getClass()->getInstance(ret,true,NULL,0);
	Vector* res= ret.as<Vector>();
	asAtom funcRet;

	for(unsigned int i=0;i<th->size();i++)
	{
		if (th->vec[i].type == T_INVALID)
			continue;
		params[0] = th->vec[i];
		params[1] = asAtom(i);
		params[2] = asAtom::fromObject(th);

		if(argslen==1)
		{
			f.callFunction(funcRet,asAtom::nullAtom, params, 3,false);
		}
		else
		{
			f.callFunction(funcRet,args[1], params, 3,false);
		}
		if(funcRet.type != T_INVALID)
		{
			if(funcRet.Boolean_concrete())
			{
				ASATOM_INCREF(th->vec[i]);
				res->vec.push_back(th->vec[i]);
			}
			ASATOM_DECREF(funcRet);
		}
	}
}

ASFUNCTIONBODY_ATOM(Vector, some)
{
	if (argslen < 1)
		throwError<ArgumentError>(kWrongArgumentCountError, "Vector.some", "1", Integer::toString(argslen));
	if (!args[0].is<IFunction>())
		throwError<TypeError>(kCheckTypeFailedError, args[0].toObject(sys)->getClassName(), "Function");
	Vector* th=static_cast<Vector*>(obj.getObject());
	asAtom f = args[0];
	asAtom params[3];

	for(unsigned int i=0; i < th->size(); i++)
	{
		if (th->vec[i].type == T_INVALID)
			continue;
		params[0] = th->vec[i];
		params[1] = asAtom(i);
		params[2] = asAtom::fromObject(th);

		if(argslen==1)
		{
			f.callFunction(ret,asAtom::nullAtom, params, 3,false);
		}
		else
		{
			f.callFunction(ret,args[1], params, 3,false);
		}
		if(ret.type != T_INVALID)
		{
			if(ret.Boolean_concrete())
			{
				return;
			}
			ASATOM_DECREF(ret);
		}
	}
	ret.setBool(false);
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

	for(unsigned int i=0; i < th->size(); i++)
	{
		if (th->vec[i].type != T_INVALID)
			params[0] = th->vec[i];
		else
			params[0] = asAtom::nullAtom;
		params[1] = asAtom(i);
		params[2] = asAtom::fromObject(th);

		if(argslen==1)
		{
			f.callFunction(ret,asAtom::nullAtom, params, 3,false);
		}
		else
		{
			f.callFunction(ret,args[1], params, 3,false);
		}
		if(ret.type != T_INVALID)
		{
			if (ret.type == T_UNDEFINED || ret.type == T_NULL)
				throwError<TypeError>(kCallOfNonFunctionError, ret.toString(sys));
			if(!ret.Boolean_concrete())
			{
				return;
			}
			ASATOM_DECREF(ret);
		}
	}
	ret.setBool(true);
}

void Vector::append(asAtom &o)
{
	if (fixed)
	{
		ASATOM_DECREF(o);
		throwError<RangeError>(kVectorFixedError);
	}
	vec_type->coerce(getSystemState(),o);
	vec.push_back(o);
}

ASObject *Vector::describeType() const
{
	pugi::xml_document p;
	pugi::xml_node root = p.append_child("type");

	// type attributes
	Class_base* prot=getClass();
	if(prot)
	{
		root.append_attribute("name").set_value(prot->getQualifiedClassName(true).raw_buf());
		root.append_attribute("base").set_value("__AS3__.vec::Vector.<*>");
	}
	bool isDynamic = getClass() && !getClass()->isSealed;
	root.append_attribute("isDynamic").set_value(isDynamic);
	root.append_attribute("isFinal").set_value(false);
	root.append_attribute("isStatic").set_value(false);
	pugi::xml_node node=root.append_child("extendsClass");
	node.append_attribute("type").set_value("__AS3__.vec::Vector.<*>");

	if(prot)
		prot->describeInstance(root,true);

	//LOG(LOG_INFO,"describeType:"<< Class<XML>::getInstanceS(getSystemState(),root)->toXMLString_internal());

	return XML::createFromNode(root);
	
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
		th->vec_type->coerce(th->getSystemState(),args[i]);
		th->vec.push_back(args[i]);
	}
	ret.setUInt((uint32_t)th->vec.size());
}

ASFUNCTIONBODY_ATOM(Vector,_pop)
{
	Vector* th=obj.as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	uint32_t size =th->size();
	if (size == 0)
	{
		ret.setNull();
		th->vec_type->coerce(th->getSystemState(),ret);
		return;
	}
	ret = th->vec[size-1];
	if (ret.type == T_INVALID)
	{
		ret.setNull();
		th->vec_type->coerce(th->getSystemState(),ret);
	}
	th->vec.pop_back();
}

ASFUNCTIONBODY_ATOM(Vector,getLength)
{
	ret.setUInt((uint32_t)obj.as<Vector>()->vec.size());
}

ASFUNCTIONBODY_ATOM(Vector,setLength)
{
	Vector* th = obj.as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	uint32_t len;
	ARG_UNPACK_ATOM (len);
	if(len <= th->vec.size())
	{
		for(size_t i=len; i< th->vec.size(); ++i)
			ASATOM_DECREF(th->vec[i]);
	}
	th->vec.resize(len, asAtom::invalidAtom);
}

ASFUNCTIONBODY_ATOM(Vector,getFixed)
{
	ret.setBool(obj.as<Vector>()->fixed);
}

ASFUNCTIONBODY_ATOM(Vector,setFixed)
{
	Vector* th = obj.as<Vector>();
	bool fixed;
	ARG_UNPACK_ATOM (fixed);
	th->fixed = fixed;
}

ASFUNCTIONBODY_ATOM(Vector,forEach)
{
	Vector* th=obj.as<Vector>();
	if (argslen < 1)
		throwError<ArgumentError>(kWrongArgumentCountError, "Vector.forEach", "1", Integer::toString(argslen));
	if (!args[0].is<IFunction>())
		throwError<TypeError>(kCheckTypeFailedError, args[0].toObject(th->getSystemState())->getClassName(), "Function");
	asAtom f = args[0];
	asAtom params[3];

	for(unsigned int i=0; i < th->size(); i++)
	{
		if (th->vec[i].type == T_INVALID)
			continue;
		params[0] = th->vec[i];
		params[1] = asAtom(i);
		params[2] = asAtom::fromObject(th);

		asAtom funcret;
		if( argslen == 1 )
		{
			f.callFunction(funcret,asAtom::nullAtom, params, 3,false);
		}
		else
		{
			f.callFunction(funcret,args[1], params, 3,false);
		}
		ASATOM_DECREF(funcret);
	}
}

ASFUNCTIONBODY_ATOM(Vector, _reverse)
{
	Vector* th = obj.as<Vector>();

	std::vector<asAtom> tmp = std::vector<asAtom>(th->vec.begin(),th->vec.end());
	uint32_t size = th->size();
	th->vec.clear();
	th->vec.resize(size, asAtom::invalidAtom);
	std::vector<asAtom>::iterator it=tmp.begin();
	uint32_t index = size-1;
	for(;it != tmp.end();++it)
 	{
		th->vec[index]=*it;
		index--;
	}
	th->incRef();
	ret = asAtom::fromObject(th);
}

ASFUNCTIONBODY_ATOM(Vector,lastIndexOf)
{
	Vector* th=obj.as<Vector>();
	assert_and_throw(argslen==1 || argslen==2);
	int32_t res=-1;
	asAtom arg0=args[0];

	if(th->vec.size() == 0)
	{
		ret.setInt((int32_t)-1);
		return;
	}

	size_t i = th->size()-1;

	if(argslen == 2 && std::isnan(args[1].toNumber()))
	{
		ret.setInt(0);
		return;
	}

	if(argslen == 2 && args[1].type != T_UNDEFINED && !std::isnan(args[1].toNumber()))
	{
		int j = args[1].toInt(); //Preserve sign
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
		if (th->vec[i].type == T_INVALID)
		    continue;
		if (th->vec[i].isEqualStrict(th->getSystemState(),arg0))
		{
			res=i;
			break;
		}
	}
	while(i--);

	ret.setInt(res);
}

ASFUNCTIONBODY_ATOM(Vector,shift)
{
	Vector* th=obj.as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	if(!th->size())
	{
		ret.setNull();
		th->vec_type->coerce(th->getSystemState(),ret);
		return;
	}
	if(th->vec[0].type !=T_INVALID)
		ret=th->vec[0];
	else
	{
		ret.setNull();
		th->vec_type->coerce(th->getSystemState(),ret);
	}
	for(uint32_t i= 1;i< th->size();i++)
	{
		th->vec[i-1]=th->vec[i];
	}
	th->vec.resize(th->size()-1, asAtom::invalidAtom);
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
	th->getClass()->getInstance(ret,true,NULL,0);
	Vector* res= ret.as<Vector>();
	res->vec.resize(endIndex-startIndex, asAtom::invalidAtom);
	int j = 0;
	for(int i=startIndex; i<endIndex; i++) 
	{
		if (th->vec[i].type != T_INVALID)
		{
			ASATOM_INCREF(th->vec[i]);
			res->vec[j] =th->vec[i];
			th->vec_type->coerce(th->getSystemState(),th->vec[j]);
		}
		j++;
	}
}

ASFUNCTIONBODY_ATOM(Vector,splice)
{
	Vector* th=obj.as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	int startIndex=args[0].toInt();
	//By default, delete all the element up to the end
	//Use the array len, it will be capped below
	int deleteCount=th->size();
	if(argslen > 1)
		deleteCount=args[1].toUInt();
	int totalSize=th->size();
	th->getClass()->getInstance(ret,true,NULL,0);
	Vector* res= ret.as<Vector>();

	startIndex=th->capIndex(startIndex);

	if((startIndex+deleteCount)>totalSize)
		deleteCount=totalSize-startIndex;

	res->vec.resize(deleteCount, asAtom::invalidAtom);
	if(deleteCount)
	{
		// write deleted items to return array
		for(int i=0;i<deleteCount;i++)
		{
			res->vec[i] = th->vec[startIndex+i];
		}
		// delete items from current array
		for (int i = 0; i < deleteCount; i++)
		{
			th->vec[startIndex+i] = asAtom::invalidAtom;
		}
	}
	// remember items in current array that have to be moved to new position
	vector<asAtom> tmp = vector<asAtom>(totalSize- (startIndex+deleteCount));
	tmp.resize(totalSize- (startIndex+deleteCount), asAtom::invalidAtom);
	for (int i = startIndex+deleteCount; i < totalSize ; i++)
	{
		if (th->vec[i].type != T_INVALID)
		{
			tmp[i-(startIndex+deleteCount)] = th->vec[i];
			th->vec[i] = asAtom::invalidAtom;
		}
	}
	th->vec.resize(startIndex, asAtom::invalidAtom);

	
	//Insert requested values starting at startIndex
	for(unsigned int i=2;i<argslen;i++)
	{
		ASATOM_INCREF(args[i]);
		th->vec.push_back(args[i]);
	}
	// move remembered items to new position
	th->vec.resize((totalSize-deleteCount)+(argslen > 2 ? argslen-2 : 0), asAtom::invalidAtom);
	for(int i=0;i<totalSize- (startIndex+deleteCount);i++)
	{
		th->vec[startIndex+i+(argslen > 2 ? argslen-2 : 0)] = tmp[i];
	}
}

ASFUNCTIONBODY_ATOM(Vector,join)
{
	Vector* th=obj.as<Vector>();
	
	tiny_string del = ",";
	if (argslen == 1)
		  del=args[0].toString(sys);
	string res;
	for(uint32_t i=0;i<th->size();i++)
	{
		if (th->vec[i].type != T_INVALID)
			res+=th->vec[i].toString(sys).raw_buf();
		if(i!=th->size()-1)
			res+=del.raw_buf();
	}
	ret = asAtom::fromObject(abstract_s(th->getSystemState(),res));
}

ASFUNCTIONBODY_ATOM(Vector,indexOf)
{
	Vector* th=obj.as<Vector>();
	assert_and_throw(argslen==1 || argslen==2);
	int32_t res=-1;
	asAtom arg0=args[0];

	int unsigned i = 0;
	if(argslen == 2)
	{
		i = args[1].toInt();
	}

	for(;i<th->size();i++)
	{
		if (th->vec[i].type ==T_INVALID)
			continue;
		if(th->vec[i].isEqualStrict(th->getSystemState(),arg0))
		{
			res=i;
			break;
		}
	}
	ret.setInt(res);
}
bool Vector::sortComparatorDefault::operator()(const asAtom& d1, const asAtom& d2)
{
	asAtom o1 = d1;
	asAtom o2 = d2;
	if(isNumeric)
	{
		number_t a=o1.toNumber();

		number_t b=o2.toNumber();

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
		tiny_string s1 = o1.toString(getSys());
		tiny_string s2 = o2.toString(getSys());

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
number_t Vector::sortComparatorWrapper::compare(const asAtom& d1, const asAtom& d2)
{
	asAtom objs[2];
	if (d1.type != T_INVALID)
		objs[0] = d1;
	else
		objs[0] = asAtom::nullAtom;
	if (d2.type != T_INVALID)
		objs[1] = d2;
	else
		objs[1] = asAtom::nullAtom;
	asAtom ret;
	// don't coerce the result, as it may be an int that would loose it's sign through coercion
	comparator.callFunction(ret,asAtom::nullAtom, objs, 2,false,false);
	assert_and_throw(ret.type != T_INVALID);
	return ret.toNumber();
}

// std::sort expects strict weak ordering for the comparison function
// this is not guarranteed by user defined comparison functions, so we need our own sorting method.
// TODO this is just a simple quicksort implementation, not optimized for performance
void simplequicksortVector(std::vector<asAtom>& v, Vector::sortComparatorWrapper& comp, int32_t lo, int32_t hi)
{
	if (v.size() == 2)
	{
		if (comp.compare(v[0],v[1]) > 0)
			std::swap(v[0],v[1]);
		return;
	}
	if (lo < hi)
	{
		asAtom pivot = v[lo];
		int32_t i = lo - 1;
		int32_t j = hi + 1;
		while (true)
		{
			do
			{
				i++;
			}
			while (i < hi && (comp.compare(v[i],pivot) < 0));
			
			do
			{
				j--;
			}
			while (j >= 0 && (comp.compare(v[j],pivot) > 0));
	
			if (i >= j)
				break;
	
			std::swap(v[i],v[j]);
		}
		if (j >= lo)
		{
			simplequicksortVector(v, comp, lo, j);
			simplequicksortVector(v, comp, j + 1, hi);
		}
	}
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
	std::vector<asAtom> tmp = vector<asAtom>(th->vec.size());
	int i = 0;
	for(auto it=th->vec.begin();it != th->vec.end();++it)
	{
		tmp[i++]= *it;
	}
	
	if(comp.type != T_INVALID)
	{
		sortComparatorWrapper c(comp);
		simplequicksortVector(tmp,c,0,tmp.size()-1);
	}
	else
		sort(tmp.begin(),tmp.end(),sortComparatorDefault(isNumeric,isCaseInsensitive,isDescending));

	th->vec.clear();
	for(auto ittmp=tmp.begin();ittmp != tmp.end();++ittmp)
	{
		th->vec.push_back(*ittmp);
	}
	ASATOM_INCREF(obj);
	ret = obj;
}

ASFUNCTIONBODY_ATOM(Vector,unshift)
{
	Vector* th=obj.as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kVectorFixedError);
	if (argslen > 0)
	{
		uint32_t s = th->size();
		th->vec.resize(th->size()+argslen, asAtom::invalidAtom);
		for(uint32_t i=s;i> 0;i--)
		{
			th->vec[(i-1)+argslen]=th->vec[i-1];
			th->vec[i-1] = asAtom::invalidAtom;
		}
		
		for(uint32_t i=0;i<argslen;i++)
		{
			ASATOM_INCREF(args[i]);
			th->vec[i] = args[i];
			th->vec_type->coerce(th->getSystemState(),th->vec[i]);
		}
	}
	ret.setInt((int32_t)th->size());
}

ASFUNCTIONBODY_ATOM(Vector,_map)
{
	Vector* th=obj.as<Vector>();
	asAtom func(T_FUNCTION);
	asAtom thisObject;
	
	if (argslen >= 1 && !args[0].is<IFunction>())
		throwError<TypeError>(kCheckTypeFailedError, args[0].toObject(th->getSystemState())->getClassName(), "Function");

	ARG_UNPACK_ATOM(func)(thisObject,asAtom::nullAtom);
	th->getClass()->getInstance(ret,true,NULL,0);
	Vector* res= ret.as<Vector>();

	for(uint32_t i=0;i<th->size();i++)
	{
		asAtom funcArgs[3];
		funcArgs[0]=th->vec[i];
		funcArgs[1]=asAtom(i);
		funcArgs[2]=asAtom::fromObject(th);
		asAtom funcRet;
		func.callFunction(funcRet,thisObject, funcArgs, 3,false);
		assert_and_throw(funcRet.type != T_INVALID);
		ASATOM_INCREF(funcRet);
		res->vec.push_back(funcRet);
	}

	ret = asAtom::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Vector,_toString)
{
	tiny_string res;
	Vector* th = obj.as<Vector>();
	for(size_t i=0; i < th->vec.size(); ++i)
	{
		if (th->vec[i].type != T_INVALID)
			res += th->vec[i].toString(sys);
		else
		{
			// use the type's default value
			asAtom natom(T_NULL);
			th->vec_type->coerce(th->getSystemState(), natom);
			res += natom.toString(sys);
		}

		if(i!=th->vec.size()-1)
			res += ',';
	}
	ret = asAtom::fromObject(abstract_s(th->getSystemState(),res));
}

ASFUNCTIONBODY_ATOM(Vector,insertAt)
{
	Vector* th=obj.as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kOutOfRangeError);
	int32_t index;
	asAtom o;
	ARG_UNPACK_ATOM(index)(o);

	if (index < 0 && th->vec.size() >= (uint32_t)(-index))
		index = th->vec.size()+(index);
	if (index < 0)
		index = 0;
	if ((uint32_t)index >= th->vec.size())
	{
		ASATOM_INCREF(o);
		th->vec.push_back(o);
	}
	else
	{
		ASATOM_INCREF(o);
		th->vec.insert(th->vec.begin()+index,o);
	}
}

ASFUNCTIONBODY_ATOM(Vector,removeAt)
{
	Vector* th=obj.as<Vector>();
	if (th->fixed)
		throwError<RangeError>(kOutOfRangeError);
	int32_t index;
	ARG_UNPACK_ATOM(index);
	if (index < 0)
		index = th->vec.size()+index;
	if (index < 0)
		index = 0;
	if ((uint32_t)index < th->vec.size())
	{
		ret = th->vec[index];
		th->vec.erase(th->vec.begin()+index);
	}
	else
		throwError<RangeError>(kOutOfRangeError);
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
GET_VARIABLE_RESULT Vector::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt);
	}

	assert_and_throw(name.ns.size()>0);
	if(!name.hasEmptyNS)
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt);
	}

	unsigned int index=0;
	bool isNumber =false;
	if(!Vector::isValidMultiname(getSystemState(),name,index,&isNumber) || index > vec.size())
	{
		switch(name.name_type) 
		{
			case multiname::NAME_NUMBER:
				if (getSystemState()->getSwfVersion() >= 11 
						|| (uint32_t(name.name_d) == name.name_d && name.name_d < UINT32_MAX))
					throwError<RangeError>(kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				else
					throwError<ReferenceError>(kReadSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
				break;
			case multiname::NAME_INT:
				if (getSystemState()->getSwfVersion() >= 11
						|| name.name_i >= (int32_t)vec.size())
					throwError<RangeError>(kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				else
					throwError<ReferenceError>(kReadSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
				break;
			case multiname::NAME_UINT:
				throwError<RangeError>(kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				break;
			case multiname::NAME_STRING:
				if (isNumber)
				{
					if (getSystemState()->getSwfVersion() >= 11 )
						throwError<RangeError>(kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
					else
						throwError<ReferenceError>(kReadSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
				}
				break;
			default:
				break;
		}

		GET_VARIABLE_RESULT res = getVariableByMultinameIntern(ret,name,this->getClass(),opt);
		if (ret.type == T_INVALID)
			throwError<ReferenceError>(kReadSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
		return res;
	}
	if(index < vec.size())
	{
		if (vec[index].type !=T_INVALID)
		{
			ASATOM_INCREF(vec[index]);
			ret = vec[index];
		}
		else
		{
			ret.setNull();
			vec_type->coerce(getSystemState(), ret );
		}
	}
	else
	{
		throwError<RangeError>(kOutOfRangeError,
				       Integer::toString(index),
				       Integer::toString(vec.size()));
	}
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}

void Vector::setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst)
{
	assert_and_throw(name.ns.size()>0);
	if(!name.hasEmptyNS)
		return ASObject::setVariableByMultiname(name, o, allowConst);

	unsigned int index=0;
	if(!Vector::isValidMultiname(getSystemState(),name,index))
	{
		switch(name.name_type) 
		{
			case multiname::NAME_NUMBER:
				if (getSystemState()->getSwfVersion() >= 11 
						|| (int32_t(name.name_d) == name.name_d && name.name_d < UINT32_MAX))
					throwError<RangeError>(kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				else
					throwError<ReferenceError>(kWriteSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
				break;
			case multiname::NAME_INT:
				if (getSystemState()->getSwfVersion() >= 11
						|| name.name_i >= (int32_t)vec.size())
					throwError<RangeError>(kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				else
					throwError<ReferenceError>(kWriteSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
				break;
			case multiname::NAME_UINT:
				throwError<RangeError>(kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				break;
			default:
				break;
		}
		
		if (!ASObject::hasPropertyByMultiname(name,false,true))
			throwError<ReferenceError>(kWriteSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
		return ASObject::setVariableByMultiname(name, o, allowConst);
	}
	this->vec_type->coerce(getSystemState(), o);
	  
	if(index < vec.size())
	{
		ASATOM_DECREF(vec[index]);
		vec[index] = o;
	}
	else if(!fixed && index == vec.size())
	{
		vec.push_back( o );
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
	for(size_t i = 0; i < vec.size(); ++i)
	{
		if( i )
			t += ",";
		if (vec[i].type != T_INVALID) 
			t += vec[i].toString(getSystemState());
		else
		{
			asAtom natom(T_NULL);
			vec_type->coerce(getSystemState(), natom );
			t += natom.toString(getSystemState());
		}
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

void Vector::nextName(asAtom& ret,uint32_t index)
{
	if(index<=vec.size())
		ret.setUInt(index-1);
	else
		throw RunTimeException("Vector::nextName out of bounds");
}

void Vector::nextValue(asAtom& ret,uint32_t index)
{
	if(index<=vec.size())
	{
		if (vec[index-1].type != T_INVALID)
		{
			ASATOM_INCREF(vec[index-1]);
			ret = vec[index-1];
		}
		else
		{
			ret.setNull();
			vec_type->coerce(getSystemState(), ret);
		}
	}
	else
		throw RunTimeException("Vector::nextValue out of bounds");
}

bool Vector::isValidMultiname(SystemState* sys,const multiname& name, uint32_t& index, bool* isNumber)
{
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
	assert_and_throw(name.ns.size()!=0);
	if(!name.hasEmptyNS)
		return false;

	bool validIndex=name.toUInt(sys,index, true,isNumber) && (index != UINT32_MAX);

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
		asAtom o = vec[i];
		if (o.type == T_INVALID)
			o= asAtom::nullAtom;
		if (replacer.type != T_INVALID)
		{
			asAtom params[2];
			
			params[0] = asAtom(i);
			params[1] = o;
			asAtom funcret;
			replacer.callFunction(funcret,asAtom::nullAtom, params, 2,false);
			if (funcret.type != T_INVALID)
				subres = funcret.toObject(getSystemState())->toJSON(path,asAtom::invalidAtom,spaces,filter);
		}
		else
		{
			subres = o.toObject(getSystemState())->toJSON(path,replacer,spaces,filter);
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

asAtom Vector::at(unsigned int index, asAtom defaultValue) const
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
			if (vec[i].type == T_INVALID)
			{
				//TODO should we write a null_marker here?
				LOG(LOG_NOT_IMPLEMENTED,"serialize unset vector objects");
				continue;
			}
			switch (marker)
			{
				case vector_int_marker:
					out->writeUnsignedInt(out->endianIn((uint32_t)vec[i].toInt()));
					break;
				case vector_uint_marker:
					out->writeUnsignedInt(out->endianIn(vec[i].toUInt()));
					break;
				case vector_double_marker:
					out->serializeDouble(vec[i].toNumber());
					break;
				case vector_object_marker:
					vec[i].toObject(getSystemState())->serialize(out, stringMap, objMap, traitsMap);
					break;
			}
		}
	}
}
