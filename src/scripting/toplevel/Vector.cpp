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
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/XML.h"
#include <3rdparty/pugixml/src/pugixml.hpp>
#include <algorithm>

using namespace std;
using namespace lightspark;

void Vector::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("length",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),getLength,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),setLength,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fixed",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),getFixed,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fixed",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),setFixed,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("concat",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),_concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("concat",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("every",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),every),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("every",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),every),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("filter",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),filter),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("filter",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),filter),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("forEach",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),forEach),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("forEach",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),forEach),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),indexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),indexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("join",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),join),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("join",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),join),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("map",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),_map),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("map",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_map),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pop",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),_pop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pop",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_pop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("push",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),push),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("push",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),push),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reverse",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),_reverse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reverse",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_reverse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("shift",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),shift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("shift",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),shift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),slice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),slice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("some",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),some),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("some",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),some),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sort",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),_sort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sort",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_sort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("splice",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),splice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("splice",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),splice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unshift",nsNameAndKind(),Class<IFunction>::getFunction(c->getSystemState(),unshift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unshift",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),unshift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("insertAt",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),insertAt,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeAt",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),removeAt,1),NORMAL_METHOD,true);
	

	c->prototype->setVariableByQName(BUILTIN_STRINGS::STRING_TOSTRING,nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("concat",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_concat),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("every",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),every),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("filter",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),filter),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("forEach",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),forEach),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("indexOf",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),indexOf),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("lastIndexOf",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("join",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),join),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("map",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_map),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("pop",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_pop),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("push",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),push),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("reverse",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_reverse),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("shift",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),shift),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("slice",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),slice),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("some",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),some),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("sort",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_sort),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("splice",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),splice),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleString",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("unshift",nsNameAndKind(c->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE),Class<IFunction>::getFunction(c->getSystemState(),unshift),CONSTANT_TRAIT);
}

Vector::Vector(ASWorker* wrk, Class_base* c, const Type *vtype):ASObject(wrk,c,T_OBJECT,SUBTYPE_VECTOR),vec_type(vtype),fixed(false),vec(reporter_allocator<asAtom>(c->memoryAccount))
{
}

Vector::~Vector()
{
}

bool Vector::destruct()
{
	for(unsigned int i=0;i<size();i++)
	{
		ASObject* obj = asAtomHandler::getObject(vec[i]);
		vec[i]=asAtomHandler::invalidAtom;
		if (obj)
			obj->removeStoredMember();
	}
	vec.clear();
	vec_type=nullptr;
	return destructIntern();
}

void Vector::finalize()
{
	for(unsigned int i=0;i<size();i++)
	{
		ASObject* obj = asAtomHandler::getObject(vec[i]);
		vec[i]=asAtomHandler::invalidAtom;
		if (obj)
			obj->removeStoredMember();
	}
	vec.clear();
	vec_type=nullptr;
}

void Vector::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	for(unsigned int i=0;i<size();i++)
	{
		ASObject* v = asAtomHandler::getObject(vec[i]);
		if (v)
			v->prepareShutdown();
	}
}
bool Vector::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	for (auto it = vec.begin(); it != vec.end(); it++)
	{
		if (asAtomHandler::isObject(*it))
			ret = asAtomHandler::getObjectNoCheck(*it)->countAllCylicMemberReferences(gcstate) || ret;
	}
	return ret;
}

void Vector::setTypes(const std::vector<const Type *> &types)
{
	assert(vec_type == nullptr);
	if(types.size() == 1)
		vec_type = types[0];
}
bool Vector::sameType(const Class_base *cls) const
{
	if (this->getClass()->class_name == cls->class_name)
		return true;
	tiny_string clsname = this->getClass()->getQualifiedClassName();
	return (clsname.startsWith(cls->class_name.getQualifiedName(getSystemState()).raw_buf()));
}

void Vector::generator(asAtom& ret, ASWorker* wrk, asAtom &o_class, asAtom* args, const unsigned int argslen)
{
	assert_and_throw(argslen == 1);
	assert_and_throw(asAtomHandler::toObject(args[0],wrk)->getClass());
	assert_and_throw(asAtomHandler::as<TemplatedClass<Vector>>(o_class)->getTypes().size() == 1);

	const Type* type = asAtomHandler::as<TemplatedClass<Vector>>(o_class)->getTypes()[0];

	RootMovieClip* root = wrk->rootClip.getPtr();
	if(asAtomHandler::is<Array>(args[0]))
	{
		//create object without calling _constructor
		asAtomHandler::as<TemplatedClass<Vector>>(o_class)->getInstance(wrk,ret,false,nullptr,0);
		Vector* res = asAtomHandler::as<Vector>(ret);

		Array* a = asAtomHandler::as<Array>(args[0]);
		for(unsigned int i=0;i<a->size();++i)
		{
			asAtom o = a->at(i);
			//Convert the elements of the array to the type of this vector
			if (!type->coerce(wrk,o))
				ASATOM_INCREF(o);
			ASObject* obj = asAtomHandler::getObject(o);
			if (obj)
				obj->addStoredMember();
			res->vec.push_back(o);
		}
		res->setIsInitialized(true);
	}
	else if(asAtomHandler::getObject(args[0])->getClass()->getTemplate() == Template<Vector>::getTemplate(root))
	{
		Vector* arg = asAtomHandler::as<Vector>(args[0]);

		Vector* res = nullptr;
		if (arg->vec_type == type)
		{
			// according to specs, the argument is returned if it is a vector of the same type as the provided class
			res = arg;
			res->incRef();
			ret = asAtomHandler::fromObject(res);
		}
		else
		{
			//create object without calling _constructor
			asAtomHandler::as<TemplatedClass<Vector>>(o_class)->getInstance(wrk,ret,false,nullptr,0);
			res = asAtomHandler::as<Vector>(ret);
			for(auto i = arg->vec.begin(); i != arg->vec.end(); ++i)
			{
				asAtom o = *i;
				if (!type->coerce(wrk,o))
					ASATOM_INCREF(o);
				ASObject* obj = asAtomHandler::getObject(o);
				if (obj)
					obj->addStoredMember();
				res->vec.push_back(o);
			}
		}
	}
	else
	{
		createError<ArgumentError>(wrk,kCheckTypeFailedError, asAtomHandler::toObject(args[0],wrk)->getClassName(), "Vector");
	}
}

ASFUNCTIONBODY_ATOM(Vector,_constructor)
{
	uint32_t len;
	bool fixed;
	ARG_CHECK(ARG_UNPACK (len, 0) (fixed, false));
	assert_and_throw(argslen <= 2);

	Vector* th=asAtomHandler::as<Vector>(obj);
	assert(th->vec_type);
	th->fixed = fixed;
	th->vec.resize(len, th->getDefaultValue());
}

ASFUNCTIONBODY_ATOM(Vector,_concat)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	th->getClass()->getInstance(wrk,ret,true,nullptr,0);
	Vector* res = asAtomHandler::as<Vector>(ret);
	// copy values into new Vector
	res->vec.resize(th->size(), th->getDefaultValue());
	auto it=th->vec.begin();
	uint32_t index = 0;
	for(;it != th->vec.end();++it)
	{
		res->vec[index]=*it;
		ASObject* obj = asAtomHandler::getObject(*it);
		if (obj)
		{
			obj->incRef();
			obj->addStoredMember();
		}
		index++;
	}
	//Insert the arguments in the vector
	int pos = wrk->getSystemState()->getSwfVersion() < 11 ? argslen-1 : 0;
	for(unsigned int i=0;i<argslen;i++)
	{
		if (asAtomHandler::is<Vector>(args[pos]))
		{
			Vector* arg=asAtomHandler::as<Vector>(args[pos]);
			res->vec.resize(index+arg->size(), th->getDefaultValue());
			auto it=arg->vec.begin();
			for(;it != arg->vec.end();++it)
			{
				if (asAtomHandler::isValid(*it))
				{
					res->vec[index]= *it;
					th->vec_type->coerceForTemplate(th->getInstanceWorker(),res->vec[index]);
					ASObject* obj = asAtomHandler::getObject(*it);
					if (obj)
					{
						obj->incRef();
						obj->addStoredMember();
					}
				}
				index++;
			}
		}
		else
		{
			asAtom v = args[pos];
			if (!th->vec_type->coerce(th->getInstanceWorker(),v))
				ASATOM_INCREF(v);
			ASObject* obj = asAtomHandler::getObject(v);
			if (obj)
				obj->addStoredMember();
			res->vec[index] = v;
			index++;
		}
		pos += (wrk->getSystemState()->getSwfVersion() < 11 ?-1 : 1);
	}	
}

ASFUNCTIONBODY_ATOM(Vector,filter)
{
	if ((argslen < 1) || (argslen > 2))
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "Vector.filter", "1", Integer::toString(argslen));
		return;
	}
	if (!asAtomHandler::is<IFunction>(args[0]))
	{
		createError<TypeError>(wrk,kCheckTypeFailedError, asAtomHandler::toObject(args[0],wrk)->getClassName(), "Function");
		return;
	}
	Vector* th=asAtomHandler::as<Vector>(obj);
	  
	asAtom f = args[0];
	asAtom params[3];
	th->getClass()->getInstance(wrk,ret,true,nullptr,0);
	Vector* res= asAtomHandler::as<Vector>(ret);
	asAtom funcRet=asAtomHandler::invalidAtom;
	asAtom closure = asAtomHandler::getClosure(f) ? asAtomHandler::fromObject(asAtomHandler::getClosure(f)) : asAtomHandler::nullAtom;

	for(unsigned int i=0;i<th->size();i++)
	{
		params[0] = th->vec[i];
		params[1] = asAtomHandler::fromUInt(i);
		params[2] = asAtomHandler::fromObject(th);

		if(argslen==1)
		{
			asAtomHandler::callFunction(f,wrk,funcRet,closure, params, 3,false);
		}
		else
		{
			asAtomHandler::callFunction(f,wrk,funcRet,args[1], params, 3,false);
		}
		if(asAtomHandler::isValid(funcRet))
		{
			if(asAtomHandler::Boolean_concrete(funcRet))
			{
				ASObject* obj = asAtomHandler::getObject(th->vec[i]);
				if (obj)
				{
					obj->incRef();
					obj->addStoredMember();
				}
				res->vec.push_back(th->vec[i]);
			}
			ASATOM_DECREF(funcRet);
		}
	}
}

ASFUNCTIONBODY_ATOM(Vector, some)
{
	if (argslen < 1)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "Vector.some", "1", Integer::toString(argslen));
		return;
	}
	if (!asAtomHandler::is<IFunction>(args[0]))
	{
		createError<TypeError>(wrk,kCheckTypeFailedError, asAtomHandler::toObject(args[0],wrk)->getClassName(), "Function");
		return;
	}
	Vector* th=static_cast<Vector*>(asAtomHandler::getObject(obj));
	asAtom f = args[0];
	asAtom params[3];
	asAtom closure = asAtomHandler::getClosure(f) ? asAtomHandler::fromObject(asAtomHandler::getClosure(f)) : asAtomHandler::nullAtom;

	for(unsigned int i=0; i < th->size(); i++)
	{
		params[0] = th->vec[i];
		params[1] = asAtomHandler::fromUInt(i);
		params[2] = asAtomHandler::fromObject(th);

		if(argslen==1)
		{
			asAtomHandler::callFunction(f,wrk,ret,closure, params, 3,false);
		}
		else
		{
			asAtomHandler::callFunction(f,wrk,ret,args[1], params, 3,false);
		}
		if(asAtomHandler::isValid(ret))
		{
			if(asAtomHandler::Boolean_concrete(ret))
			{
				return;
			}
			ASATOM_DECREF(ret);
		}
	}
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(Vector, every)
{
	Vector* th=static_cast<Vector*>(asAtomHandler::getObject(obj));
	if (argslen < 1)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "Vector.every", "1", Integer::toString(argslen));
		return;
	}
	if (!asAtomHandler::is<IFunction>(args[0]))
	{
		createError<TypeError>(wrk,kCheckTypeFailedError, asAtomHandler::toObject(args[0],wrk)->getClassName(), "Function");
		return;
	}
	asAtom f = args[0];
	asAtom params[3];
	asAtom closure = asAtomHandler::getClosure(f) ? asAtomHandler::fromObject(asAtomHandler::getClosure(f)) : asAtomHandler::nullAtom;

	for(unsigned int i=0; i < th->size(); i++)
	{
		if (asAtomHandler::isValid(th->vec[i]))
			params[0] = th->vec[i];
		else
			params[0] = asAtomHandler::nullAtom;
		params[1] = asAtomHandler::fromUInt(i);
		params[2] = asAtomHandler::fromObject(th);

		if(argslen==1)
		{
			asAtomHandler::callFunction(f,wrk,ret,closure, params, 3,false);
		}
		else
		{
			asAtomHandler::callFunction(f,wrk,ret,args[1], params, 3,false);
		}
		if(asAtomHandler::isValid(ret))
		{
			if (asAtomHandler::isUndefined(ret) || asAtomHandler::isNull(ret))
			{
				createError<TypeError>(wrk,kCallOfNonFunctionError, asAtomHandler::toString(ret,wrk));
				return;
			}
			if(!asAtomHandler::Boolean_concrete(ret))
			{
				return;
			}
			ASATOM_DECREF(ret);
		}
	}
	asAtomHandler::setBool(ret,true);
}

void Vector::append(asAtom &o)
{
	if (fixed)
	{
		ASATOM_DECREF(o);
		createError<RangeError>(getInstanceWorker(),kVectorFixedError);
		return;
	}
	asAtom v = o;
	if (vec_type->coerce(getInstanceWorker(),v))
		ASATOM_DECREF(v);
	ASObject* obj = asAtomHandler::getObject(v);
	if (obj)
		obj->addStoredMember();
	vec.push_back(o);
}

void Vector::remove(ASObject *o)
{
	for (auto it = vec.begin(); it != vec.end(); it++)
	{
		if (asAtomHandler::getObject(*it) == o)
		{
			o->removeStoredMember();
			vec.erase(it);
			break;
		}
	}
}

ASObject *Vector::describeType(ASWorker* wrk) const
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
		prot->describeInstance(root,true,true);

	//LOG(LOG_INFO,"describeType:"<< Class<XML>::getInstanceS(getInstanceWorker(),root)->toXMLString_internal());

	return XML::createFromNode(wrk,root);
	
}

ASFUNCTIONBODY_ATOM(Vector,push)
{
	Vector* th=static_cast<Vector*>(asAtomHandler::getObject(obj));
	if (th->fixed)
	{
		createError<RangeError>(wrk,kVectorFixedError);
		return;
	}
	for(size_t i = 0; i < argslen; ++i)
	{
		//The proprietary player violates the specification and allows elements of any type to be pushed;
		//they are converted to the vec_type
		asAtom v = args[i];
		if (!th->vec_type->coerce(th->getInstanceWorker(),v))
			ASATOM_INCREF(v);
		ASObject* obj = asAtomHandler::getObject(v);
		if (obj)
			obj->addStoredMember();
		th->vec.push_back(v);
	}
	asAtomHandler::setUInt(ret,wrk,(uint32_t)th->vec.size());
}

ASFUNCTIONBODY_ATOM(Vector,_pop)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	if (th->fixed)
	{
		createError<RangeError>(wrk,kVectorFixedError);
		return;
	}
	uint32_t size =th->size();
	if (size == 0)
	{
		asAtomHandler::setNull(ret);
		th->vec_type->coerce(th->getInstanceWorker(),ret);
		return;
	}
	ret = th->vec[size-1];
	ASObject* ob = asAtomHandler::getObject(ret);
	if (ob)
	{
		ob->incRef(); // will be decreffed in removeStoreMember
		ob->removeStoredMember();
	}
	th->vec.pop_back();
}

ASFUNCTIONBODY_ATOM(Vector,getLength)
{
	asAtomHandler::setUInt(ret,wrk,(uint32_t)asAtomHandler::as<Vector>(obj)->vec.size());
}

ASFUNCTIONBODY_ATOM(Vector,setLength)
{
	Vector* th = asAtomHandler::as<Vector>(obj);
	if (th->fixed)
	{
		createError<RangeError>(wrk,kVectorFixedError);
		return;
	}
	uint32_t len;
	ARG_CHECK(ARG_UNPACK (len));
	if(len <= th->vec.size())
	{
		for(size_t i=len; i< th->vec.size(); ++i)
		{
			ASObject* ob = asAtomHandler::getObject(th->vec[i]);
			if (ob)
				ob->removeStoredMember();
		}
	}
	th->vec.resize(len, th->getDefaultValue());
}

ASFUNCTIONBODY_ATOM(Vector,getFixed)
{
	asAtomHandler::setBool(ret,asAtomHandler::as<Vector>(obj)->fixed);
}

ASFUNCTIONBODY_ATOM(Vector,setFixed)
{
	Vector* th = asAtomHandler::as<Vector>(obj);
	bool fixed;
	ARG_CHECK(ARG_UNPACK (fixed));
	th->fixed = fixed;
}

ASFUNCTIONBODY_ATOM(Vector,forEach)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	if (argslen < 1)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "Vector.forEach", "1", Integer::toString(argslen));
		return;
	}
	if (!asAtomHandler::is<IFunction>(args[0]))
	{
		createError<TypeError>(wrk,kCheckTypeFailedError, asAtomHandler::toObject(args[0],wrk)->getClassName(), "Function");
		return;
	}
	asAtom f = args[0];
	asAtom params[3];
	asAtom closure = asAtomHandler::getClosure(f) ? asAtomHandler::fromObject(asAtomHandler::getClosure(f)) : asAtomHandler::nullAtom;

	for(unsigned int i=0; i < th->size(); i++)
	{
		params[0] = th->vec[i];
		params[1] = asAtomHandler::fromUInt(i);
		params[2] = asAtomHandler::fromObject(th);

		asAtom funcret=asAtomHandler::invalidAtom;
		if( argslen == 1 )
		{
			asAtomHandler::callFunction(f,wrk,funcret,closure, params, 3,false);
		}
		else
		{
			asAtomHandler::callFunction(f,wrk,funcret,args[1], params, 3,false);
		}
		ASATOM_DECREF(funcret);
	}
}

ASFUNCTIONBODY_ATOM(Vector, _reverse)
{
	Vector* th = asAtomHandler::as<Vector>(obj);

	std::vector<asAtom> tmp = std::vector<asAtom>(th->vec.begin(),th->vec.end());
	uint32_t size = th->size();
	th->vec.clear();
	th->vec.resize(size, th->getDefaultValue());
	std::vector<asAtom>::iterator it=tmp.begin();
	uint32_t index = size-1;
	for(;it != tmp.end();++it)
 	{
		th->vec[index]=*it;
		index--;
	}
	th->incRef();
	ret = asAtomHandler::fromObject(th);
}

ASFUNCTIONBODY_ATOM(Vector,lastIndexOf)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	int32_t res=-1;
	asAtom arg0=args[0];

	if(th->vec.size() == 0)
	{
		asAtomHandler::setInt(ret,wrk,(int32_t)-1);
		return;
	}

	size_t i = th->size()-1;

	if(argslen == 2 && std::isnan(asAtomHandler::toNumber(args[1])))
	{
		asAtomHandler::setInt(ret,wrk,0);
		return;
	}

	if(argslen == 2 && !asAtomHandler::isUndefined(args[1]) && !std::isnan(asAtomHandler::toNumber(args[1])))
	{
		int j = asAtomHandler::toInt(args[1]); //Preserve sign
		if(j < 0) //Negative offset, use it as offset from the end of the array
		{
			if((size_t)-j > th->size())
				i = 0;
			else
				i = th->size()+j;
		}
		else //Positive offset, use it directly
		{
			if((size_t)j >= th->size()) //If the passed offset is bigger than the array, cap the offset
				i = th->size()-1;
			else
				i = j;
		}
	}
	do
	{
		if (asAtomHandler::isEqualStrict(th->vec[i],wrk,arg0))
		{
			res=i;
			break;
		}
	}
	while(i--);

	asAtomHandler::setInt(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(Vector,shift)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	if (th->fixed)
	{
		createError<RangeError>(wrk,kVectorFixedError);
		return;
	}
	if(!th->size())
	{
		asAtomHandler::setNull(ret);
		th->vec_type->coerce(th->getInstanceWorker(),ret);
		return;
	}
	if(asAtomHandler::isValid(th->vec[0]))
		ret=th->vec[0];
	else
	{
		asAtomHandler::setNull(ret);
		th->vec_type->coerce(th->getInstanceWorker(),ret);
	}
	for(uint32_t i= 1;i< th->size();i++)
	{
		th->vec[i-1]=th->vec[i];
	}
	th->vec.resize(th->size()-1, th->getDefaultValue());
	ASObject* ob = asAtomHandler::getObject(ret);
	if (ob)
	{
		ob->incRef(); // will be decreffed in removeStoreMember
		ob->removeStoredMember();
	}
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

asAtom Vector::getDefaultValue()
{
	if (vec_type == Class<Integer>::getClass(getSystemState()))
		return asAtomHandler::fromInt(0);
	else if (vec_type == Class<UInteger>::getClass(getSystemState()))
		return asAtomHandler::fromUInt(0);
	else if (vec_type == Class<Number>::getClass(getSystemState()))
		return asAtomHandler::fromInt(0);
	else
		return asAtomHandler::nullAtom;
}

ASFUNCTIONBODY_ATOM(Vector,slice)
{
	Vector* th=asAtomHandler::as<Vector>(obj);

	int startIndex=0;
	int endIndex=16777215;
	if(argslen>0)
		startIndex=asAtomHandler::toInt(args[0]);
	if(argslen>1)
		endIndex=asAtomHandler::toInt(args[1]);

	startIndex=th->capIndex(startIndex);
	endIndex=th->capIndex(endIndex);
	th->getClass()->getInstance(wrk,ret,true,nullptr,0);
	Vector* res= asAtomHandler::as<Vector>(ret);
	res->vec.resize(endIndex-startIndex, th->getDefaultValue());
	int j = 0;
	for(int i=startIndex; i<endIndex; i++) 
	{
		if (asAtomHandler::isValid(th->vec[i]))
		{
			res->vec[j] =th->vec[i];
			if (!th->vec_type->coerce(th->getInstanceWorker(),res->vec[j]))
			{
				ASATOM_INCREF(res->vec[j]);
				ASObject* obj = asAtomHandler::getObject(res->vec[j]);
				if (obj)
					obj->addStoredMember();
			}
		}
		j++;
	}
}

ASFUNCTIONBODY_ATOM(Vector,splice)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	if (th->fixed)
	{
		createError<RangeError>(wrk,kVectorFixedError);
		return;
	}
	int startIndex=asAtomHandler::toInt(args[0]);
	//By default, delete all the element up to the end
	//Use the array len, it will be capped below
	int deleteCount=th->size();
	if(argslen > 1)
		deleteCount=asAtomHandler::toUInt(args[1]);
	int totalSize=th->size();
	th->getClass()->getInstance(wrk,ret,true,nullptr,0);
	Vector* res= asAtomHandler::as<Vector>(ret);

	startIndex=th->capIndex(startIndex);

	if((startIndex+deleteCount)>totalSize)
		deleteCount=totalSize-startIndex;

	res->vec.resize(deleteCount, th->getDefaultValue());
	if(deleteCount)
	{
		// write deleted items to return vector
		for(int i=0;i<deleteCount;i++)
		{
			res->vec[i] = th->vec[startIndex+i];
		}
		// delete items from current vector (no need to decref/removemember, as they are added to the result)
		for (int i = 0; i < deleteCount; i++)
		{
			th->vec[startIndex+i] = th->getDefaultValue();
		}
	}
	// remember items in current array that have to be moved to new position
	vector<asAtom> tmp = vector<asAtom>(totalSize- (startIndex+deleteCount));
	tmp.resize(totalSize- (startIndex+deleteCount), th->getDefaultValue());
	for (int i = startIndex+deleteCount; i < totalSize ; i++)
	{
		if (asAtomHandler::isValid(th->vec[i]))
		{
			tmp[i-(startIndex+deleteCount)] = th->vec[i];
			th->vec[i] = th->getDefaultValue();
		}
	}
	th->vec.resize(startIndex, th->getDefaultValue());

	
	//Insert requested values starting at startIndex
	for(unsigned int i=2;i<argslen;i++)
	{
		ASObject* obj = asAtomHandler::getObject(args[i]);
		if (obj)
		{
			obj->incRef();
			obj->addStoredMember();
		}
		th->vec.push_back(args[i]);
	}
	// move remembered items to new position
	th->vec.resize((totalSize-deleteCount)+(argslen > 2 ? argslen-2 : 0), th->getDefaultValue());
	for(int i=0;i<totalSize- (startIndex+deleteCount);i++)
	{
		th->vec[startIndex+i+(argslen > 2 ? argslen-2 : 0)] = tmp[i];
	}
}

ASFUNCTIONBODY_ATOM(Vector,join)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	
	tiny_string del = ",";
	if (argslen == 1)
		  del=asAtomHandler::toString(args[0],wrk);
	string res;
	for(uint32_t i=0;i<th->size();i++)
	{
		if (asAtomHandler::isValid(th->vec[i]))
			res+=asAtomHandler::toString(th->vec[i],wrk).raw_buf();
		if(i!=th->size()-1)
			res+=del.raw_buf();
	}
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}

ASFUNCTIONBODY_ATOM(Vector,indexOf)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	assert_and_throw(argslen==1 || argslen==2);
	int32_t res=-1;
	asAtom arg0=args[0];

	int unsigned i = 0;
	if(argslen == 2)
	{
		i = asAtomHandler::toInt(args[1]);
	}

	for(;i<th->size();i++)
	{
		if(asAtomHandler::isEqualStrict(th->vec[i],wrk,arg0))
		{
			res=i;
			break;
		}
	}
	asAtomHandler::setInt(ret,wrk,res);
}
bool Vector::sortComparatorDefault::operator()(const asAtom& d1, const asAtom& d2)
{
	asAtom o1 = d1;
	asAtom o2 = d2;
	if(isNumeric)
	{
		number_t a=asAtomHandler::toNumber(o1);

		number_t b=asAtomHandler::toNumber(o2);

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
		tiny_string s1 = asAtomHandler::toString(o1,getWorker());
		tiny_string s2 = asAtomHandler::toString(o2,getWorker());

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
	if (asAtomHandler::isValid(d1))
		objs[0] = d1;
	else
		objs[0] = asAtomHandler::nullAtom;
	if (asAtomHandler::isValid(d2))
		objs[1] = d2;
	else
		objs[1] = asAtomHandler::nullAtom;
	asAtom ret=asAtomHandler::invalidAtom;
	asAtom obj = asAtomHandler::getClosureAtom(comparator);
	// don't coerce the result, as it may be an int that would loose it's sign through coercion
	asAtomHandler::callFunction(comparator,asAtomHandler::getObjectNoCheck(comparator)->getInstanceWorker(), ret,obj, objs, 2,false,false);
	assert_and_throw(asAtomHandler::isValid(ret));
	return asAtomHandler::toNumber(ret);
}

// std::sort expects strict weak ordering for the comparison function
// this is not guarranteed by user defined comparison functions, so we need our own sorting method.

// this is the quicksort algorithm used by avmplus
// see https://github.com/adobe-flash/avmplus/blob/master/core/ArrayClass.cpp
// TODO merge with qsort in Array.cpp
void qsortVector(std::vector<asAtom>& v, Vector::sortComparatorWrapper& comp, uint32_t lo, uint32_t hi)
{
	// This is an iterative implementation of the recursive quick sort.
	// Recursive implementations are basically storing nested (lo,hi) pairs
	// in the stack frame, so we can avoid the recursion by storing them
	// in an array.
	//
	// Once partitioned, we sub-partition the smaller half first. This means
	// the greatest stack depth happens with equal partitions, all the way down,
	// which would be 1 + log2(size), which could never exceed 33.

	uint32_t size;
	struct StackFrame { uint32_t lo, hi; };
	StackFrame stk[33];
	int stkptr = 0;

	// leave without doing anything if the array is empty (lo > hi) or only one element (lo == hi)
	if (lo >= hi)
		return;

	// code below branches to this label instead of recursively calling qsort()
recurse:

	size = (hi - lo) + 1; // number of elements in the partition

	if (size < 4) {

		// It is standard to use another sort for smaller partitions,
		// for instance c library source uses insertion sort for 8 or less.
		//
		// However, as our swap() is essentially free, the relative cost of
		// compare() is high, and with profiling, I found quicksort()-ing
		// down to four had better performance.
		//
		// Although verbose, handling the remaining cases explicitly is faster,
		// so I do so here.

		if (size == 3) {
			if (comp.compare(v[lo],v[lo + 1]) > 0) {
				std::swap(v[lo], v[lo + 1]);
				if (comp.compare(v[lo + 1], v[lo + 2]) > 0) {
					std::swap(v[lo + 1], v[lo + 2]);
					if (comp.compare(v[lo], v[lo + 1]) > 0) {
						std::swap(v[lo], v[lo + 1]);
					}
				}
			} else {
				if (comp.compare(v[lo + 1], v[lo + 2]) > 0) {
					std::swap(v[lo + 1], v[lo + 2]);
					if (comp.compare(v[lo], v[lo + 1]) > 0) {
						std::swap(v[lo], v[lo + 1]);
					}
				}
			}
		} else if (size == 2) {
			if (comp.compare(v[lo], v[lo + 1]) > 0)
				std::swap(v[lo], v[lo + 1]);
		} else {
			// size is one, zero or negative, so there isn't any sorting to be done
		}
	} else {
		// qsort()-ing a near or already sorted list goes much better if
		// you use the midpoint as the pivot, but the algorithm is simpler
		// if the pivot is at the start of the list, so move the middle
		// element to the front!
		uint32_t pivot = lo + (size / 2);
		std::swap(v[pivot], v[lo]);


		uint32_t left = lo;
		uint32_t right = hi + 1;

		for (;;) {
			// Move the left right until it's at an element greater than the pivot.
			// Move the right left until it's at an element less than the pivot.
			// If left and right cross, we can terminate, otherwise swap and continue.
			//
			// As each pass of the outer loop increments left at least once,
			// and decrements right at least once, this loop has to terminate.

			do  {
				left++;
			} while ((left <= hi) && (comp.compare(v[left], v[lo]) <= 0));

			do  {
				right--;
			} while ((right > lo) && (comp.compare(v[right], v[lo]) >= 0));

			if (right < left)
				break;

			std::swap(v[left], v[right]);
		}

		// move the pivot after the lower partition
		std::swap(v[lo], v[right]);

		// The array is now in three partions:
		//  1. left partition   : i in [lo, right), elements less than or equal to pivot
		//  2. center partition : i in [right, left], elements equal to pivot
		//  3. right partition  : i in (left, hi], elements greater than pivot
		// NOTE : [ means the range includes the lower bounds, ( means it excludes it, with the same for ] and ).

		// Many quick sorts recurse into the left partition, and then the right.
		// The worst case of this can lead to a stack depth of size -- for instance,
		// the left is empty, the center is just the pivot, and the right is everything else.
		//
		// If you recurse into the smaller partition first, then the worst case is an
		// equal partitioning, which leads to a depth of log2(size).
		if ((right - 1 - lo) >= (hi - left))
		{
			if ((lo + 1) < right)
			{
				stk[stkptr].lo = lo;
				stk[stkptr].hi = right - 1;
				++stkptr;
			}

			if (left < hi)
			{
				lo = left;
				goto recurse;
			}
		}
		else
		{
			if (left < hi)
			{
				stk[stkptr].lo = left;
				stk[stkptr].hi = hi;
				++stkptr;
			}

			if ((lo + 1) < right)
			{
				hi = right - 1;
				goto recurse;           /* do small recursion */
			}
		}
	}

	// we reached the bottom of the well, pop the nested stack frame
	if (--stkptr >= 0)
	{
		lo = stk[stkptr].lo;
		hi = stk[stkptr].hi;
		goto recurse;
	}

	// we've returned to the top, so we are done!
	return;
}

ASFUNCTIONBODY_ATOM(Vector,_sort)
{
	if (argslen != 1)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "Vector.sort", "1", Integer::toString(argslen));
		return;
	}
	Vector* th=static_cast<Vector*>(asAtomHandler::getObject(obj));
	
	asAtom comp=asAtomHandler::invalidAtom;
	bool isNumeric=false;
	bool isCaseInsensitive=false;
	bool isDescending=false;
	if(asAtomHandler::isFunction(args[0])) //Comparison func
	{
		assert_and_throw(asAtomHandler::isInvalid(comp));
		comp=args[0];
	}
	else
	{
		uint32_t options=asAtomHandler::toInt(args[0]);
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
	
	if(asAtomHandler::isValid(comp))
	{
		sortComparatorWrapper c(comp);
		qsortVector(tmp,c,0,tmp.size()-1);
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
	Vector* th=asAtomHandler::as<Vector>(obj);
	if (th->fixed)
	{
		createError<RangeError>(wrk,kVectorFixedError);
		return;
	}
	if (argslen > 0)
	{
		uint32_t s = th->size();
		th->vec.resize(th->size()+argslen, th->getDefaultValue());
		for(uint32_t i=s;i> 0;i--)
		{
			th->vec[(i-1)+argslen]=th->vec[i-1];
			th->vec[i-1] = th->getDefaultValue();
		}
		
		for(uint32_t i=0;i<argslen;i++)
		{
			th->vec[i] = args[i];
			if (!th->vec_type->coerce(th->getInstanceWorker(),th->vec[i]))
			{
				ASObject* obj = asAtomHandler::getObject(th->vec[i]);
				if (obj)
				{
					obj->incRef();
					obj->addStoredMember();
				}
			}
		}
	}
	asAtomHandler::setInt(ret,wrk,(int32_t)th->size());
}

ASFUNCTIONBODY_ATOM(Vector,_map)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	asAtom thisObject=asAtomHandler::invalidAtom;
	
	if (argslen >= 1 && !asAtomHandler::is<IFunction>(args[0]))
	{
		createError<TypeError>(wrk,kCheckTypeFailedError, asAtomHandler::toObject(args[0],wrk)->getClassName(), "Function");
		return;
	}
	asAtom func=asAtomHandler::invalidAtom;

	ARG_CHECK(ARG_UNPACK(func)(thisObject,asAtomHandler::nullAtom));
	th->getClass()->getInstance(wrk,ret,true,nullptr,0);
	Vector* res= asAtomHandler::as<Vector>(ret);

	for(uint32_t i=0;i<th->size();i++)
	{
		asAtom funcArgs[3];
		funcArgs[0]=th->vec[i];
		funcArgs[1]=asAtomHandler::fromUInt(i);
		funcArgs[2]=asAtomHandler::fromObject(th);
		asAtom funcRet=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(func,wrk,funcRet,thisObject, funcArgs, 3,false);
		assert_and_throw(asAtomHandler::isValid(funcRet));
		ASObject* obj = asAtomHandler::getObject(funcRet);
		if (obj)
		{
			obj->incRef();
			obj->addStoredMember();
		}
		res->vec.push_back(funcRet);
	}

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Vector,_toString)
{
	tiny_string res;
	Vector* th = asAtomHandler::as<Vector>(obj);
	for(size_t i=0; i < th->vec.size(); ++i)
	{
		if (asAtomHandler::isValid(th->vec[i]))
			res += asAtomHandler::toString(th->vec[i],wrk);
		else
		{
			// use the type's default value
			asAtom natom = asAtomHandler::nullAtom;
			th->vec_type->coerce(th->getInstanceWorker(), natom);
			res += asAtomHandler::toString(natom,wrk);
		}

		if(i!=th->vec.size()-1)
			res += ',';
	}
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}

ASFUNCTIONBODY_ATOM(Vector,insertAt)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	if (th->fixed)
	{
		createError<RangeError>(wrk,kOutOfRangeError);
		return;
	}
	int32_t index;
	asAtom o=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(index)(o));

	if (index < 0 && th->vec.size() >= (uint32_t)(-index))
		index = th->vec.size()+(index);
	if (index < 0)
		index = 0;
	ASObject* ob = asAtomHandler::getObject(o);
	if (ob)
	{
		ob->incRef();
		ob->addStoredMember();
	}
	if ((uint32_t)index >= th->vec.size())
		th->vec.push_back(o);
	else
		th->vec.insert(th->vec.begin()+index,o);
}

ASFUNCTIONBODY_ATOM(Vector,removeAt)
{
	Vector* th=asAtomHandler::as<Vector>(obj);
	if (th->fixed)
	{
		createError<RangeError>(wrk,kOutOfRangeError);
		return;
	}
	int32_t index;
	ARG_CHECK(ARG_UNPACK(index));
	if (index < 0)
		index = th->vec.size()+index;
	if (index < 0)
		index = 0;
	if ((uint32_t)index < th->vec.size())
	{
		ret = th->vec[index];
		ASObject* ob = asAtomHandler::getObject(ret);
		th->vec.erase(th->vec.begin()+index);
		if (ob)
			ob->removeStoredMember();
	}
	else
		createError<RangeError>(wrk,kOutOfRangeError);
}

bool Vector::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk)
{
	if(!considerDynamic)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);

	if(!name.hasEmptyNS)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);

	unsigned int index=0;
	if(!Vector::isValidMultiname(getSystemState(),name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);

	if(index < vec.size())
		return true;
	else
		return false;
}

/* this handles the [] operator, because vec[12] becomes vec.12 in bytecode */
GET_VARIABLE_RESULT Vector::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt,ASWorker* wrk)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	}

	if(!name.hasEmptyNS)
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
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
					createError<RangeError>(getInstanceWorker(),kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				else
					createError<ReferenceError>(getInstanceWorker(),kReadSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
				return GET_VARIABLE_RESULT::GETVAR_NORMAL;
			case multiname::NAME_INT:
				if (getSystemState()->getSwfVersion() >= 11
						|| name.name_i >= (int32_t)vec.size())
					createError<RangeError>(getInstanceWorker(),kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				else
					createError<ReferenceError>(getInstanceWorker(),kReadSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
				return GET_VARIABLE_RESULT::GETVAR_NORMAL;
			case multiname::NAME_UINT:
				createError<RangeError>(getInstanceWorker(),kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				return GET_VARIABLE_RESULT::GETVAR_NORMAL;
			case multiname::NAME_STRING:
				if (isNumber)
				{
					if (getSystemState()->getSwfVersion() >= 11 )
						createError<RangeError>(getInstanceWorker(),kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
					else
						createError<ReferenceError>(getInstanceWorker(),kReadSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
					return GET_VARIABLE_RESULT::GETVAR_NORMAL;
				}
				break;
			default:
				break;
		}

		GET_VARIABLE_RESULT res = getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
		if (asAtomHandler::isInvalid(ret))
			createError<ReferenceError>(getInstanceWorker(),kReadSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
		return res;
	}
	if(index < vec.size())
	{
		ret = vec[index];
		if (!(opt & NO_INCREF))
			ASATOM_INCREF(ret);
	}
	else
	{
		createError<RangeError>(getInstanceWorker(),kOutOfRangeError,
				       Integer::toString(index),
				       Integer::toString(vec.size()));
	}
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}
GET_VARIABLE_RESULT Vector::getVariableByInteger(asAtom &ret, int index, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	if (index >=0 && uint32_t(index) < size())
	{
		ret = vec[index];
		if (!(opt & NO_INCREF))
			ASATOM_INCREF(ret);
		return GET_VARIABLE_RESULT::GETVAR_NORMAL;
	}
	else
		return getVariableByIntegerIntern(ret,index,opt,wrk);
}

multiname *Vector::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst,bool* alreadyset,ASWorker* wrk)
{
	if(!name.hasEmptyNS)
		return ASObject::setVariableByMultiname(name, o, allowConst,alreadyset,wrk);

	unsigned int index=0;
	if(!Vector::isValidMultiname(getSystemState(),name,index))
	{
		switch(name.name_type) 
		{
			case multiname::NAME_NUMBER:
				if (getSystemState()->getSwfVersion() >= 11 
						|| (this->fixed && ((int32_t(name.name_d) != name.name_d) || name.name_d >= (int32_t)vec.size() || name.name_d < 0)))
					createError<RangeError>(getInstanceWorker(),kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				else
					createError<ReferenceError>(getInstanceWorker(),kWriteSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
				return nullptr;
			case multiname::NAME_INT:
				if (getSystemState()->getSwfVersion() >= 11
						|| (this->fixed && (name.name_i >= (int32_t)vec.size() || name.name_i < 0)))
					createError<RangeError>(getInstanceWorker(),kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				else
					createError<ReferenceError>(getInstanceWorker(),kWriteSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
				return nullptr;
			case multiname::NAME_UINT:
				createError<RangeError>(getInstanceWorker(),kOutOfRangeError,name.normalizedName(getSystemState()),Integer::toString(vec.size()));
				return nullptr;
			default:
				break;
		}
		
		if (!ASObject::hasPropertyByMultiname(name,false,true,wrk))
		{
			createError<ReferenceError>(getInstanceWorker(),kWriteSealedError, name.normalizedName(getSystemState()), this->getClass()->getQualifiedClassName());
			return nullptr;
		}
		return ASObject::setVariableByMultiname(name, o, allowConst,alreadyset,wrk);
	}
	asAtom v = o;
	if (this->vec_type->coerce(getInstanceWorker(), o))
		ASATOM_DECREF(v);
	if(index < vec.size())
	{
		if (vec[index].uintval == o.uintval)
		{
			if (alreadyset)
				*alreadyset = true;
		}
		else
		{
			ASObject* obj = asAtomHandler::getObject(vec[index]);
			if (obj)
				obj->removeStoredMember();
			obj = asAtomHandler::getObject(o);
			if (obj)
				obj->addStoredMember();
			vec[index] = o;
		}
	}
	else if(!fixed && index == vec.size())
	{
		ASObject* obj = asAtomHandler::getObject(o);
		if (obj)
			obj->addStoredMember();
		vec.push_back(o);
	}
	else
	{
		/* Spec says: one may not set a value with an index more than
		 * one beyond the current final index. */
		createError<RangeError>(getInstanceWorker(),kOutOfRangeError,
				       Integer::toString(index),
				       Integer::toString(vec.size()));
	}
	return nullptr;
}

void Vector::setVariableByInteger(int index, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	if (index < 0)
	{
		setVariableByInteger_intern(index,o,allowConst,alreadyset,wrk);
		return;
	}
	*alreadyset = false;
	asAtom v = o;
	if (this->vec_type->coerce(getInstanceWorker(), o))
		ASATOM_DECREF(v);
	if(size_t(index) < vec.size())
	{
		if (vec[index].uintval != o.uintval)
		{
			ASObject* obj = asAtomHandler::getObject(vec[index]);
			if (obj)
				obj->removeStoredMember();
			obj = asAtomHandler::getObject(o);
			if (obj)
				obj->addStoredMember();
			vec[index] = o;
		}
		else
			*alreadyset=true;
	}
	else if(!fixed && size_t(index) == vec.size())
	{
		ASObject* obj = asAtomHandler::getObject(o);
		if (obj)
			obj->addStoredMember();
		vec.push_back(o);
	}
	else
	{
		/* Spec says: one may not set a value with an index more than
		 * one beyond the current final index. */
		createError<RangeError>(getInstanceWorker(),kOutOfRangeError,
				       Integer::toString(index),
				       Integer::toString(vec.size()));
	}
}

void Vector::throwRangeError(int index)
{
	/* Spec says: one may not set a value with an index more than
	 * one beyond the current final index. */
	createError<RangeError>(getInstanceWorker(),kOutOfRangeError,
				   Integer::toString(index),
				   Integer::toString(vec.size()));
}

tiny_string Vector::toString()
{
	//TODO: test
	tiny_string t;
	for(size_t i = 0; i < vec.size(); ++i)
	{
		if( i )
			t += ",";
		t += asAtomHandler::toString(vec[i],getInstanceWorker());
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
		asAtomHandler::setUInt(ret,this->getInstanceWorker(),index-1);
	else
		throw RunTimeException("Vector::nextName out of bounds");
}

void Vector::nextValue(asAtom& ret,uint32_t index)
{
	if(index<=vec.size())
	{
		ASATOM_INCREF(vec[index-1]);
		ret = vec[index-1];
	}
	else
		throw RunTimeException("Vector::nextValue out of bounds");
}

bool Vector::isValidMultiname(SystemState* sys,const multiname& name, uint32_t& index, bool* isNumber)
{
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
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
	{
		createError<TypeError>(getInstanceWorker(),kJSONCyclicStructure);
		return res;
	}

	path.push_back(this);
	res += "[";
	bool bfirst = true;
	tiny_string newline = (spaces.empty() ? "" : "\n");
	asAtom closure = asAtomHandler::isValid(replacer) && asAtomHandler::getClosure(replacer) ? asAtomHandler::fromObject(asAtomHandler::getClosure(replacer)) : asAtomHandler::nullAtom;
	for (unsigned int i =0;  i < vec.size(); i++)
	{
		tiny_string subres;
		asAtom o = vec[i];
		if (asAtomHandler::isValid(replacer))
		{
			asAtom params[2];
			
			params[0] = asAtomHandler::fromUInt(i);
			params[1] = o;
			asAtom funcret=asAtomHandler::invalidAtom;
			asAtomHandler::callFunction(replacer,getInstanceWorker(),funcret,closure, params, 2,false);
			if (asAtomHandler::isValid(funcret))
			{
				subres = asAtomHandler::toObject(funcret,getInstanceWorker())->toJSON(path,asAtomHandler::invalidAtom,spaces,filter);
				ASATOM_DECREF(funcret);
			}
		}
		else
		{
			bool newobj = !asAtomHandler::isObject(o); // member is not a pointer to an ASObject, so toObject() will create a temporary ASObject that has to be decreffed after usage
			subres = asAtomHandler::toObject(o,getInstanceWorker())->toJSON(path,replacer,spaces,filter);
			if (newobj)
				ASATOM_DECREF(o);
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
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
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
			if (asAtomHandler::isInvalid(vec[i]))
			{
				//TODO should we write a null_marker here?
				LOG(LOG_NOT_IMPLEMENTED,"serialize unset vector objects");
				continue;
			}
			switch (marker)
			{
				case vector_int_marker:
					out->writeUnsignedInt(out->endianIn((uint32_t)asAtomHandler::toInt(vec[i])));
					break;
				case vector_uint_marker:
					out->writeUnsignedInt(out->endianIn(asAtomHandler::toUInt(vec[i])));
					break;
				case vector_double_marker:
					out->serializeDouble(asAtomHandler::toNumber(vec[i]));
					break;
				case vector_object_marker:
					asAtomHandler::serialize(out, stringMap, objMap, traitsMap,wrk,vec[i]);
					break;
			}
		}
	}
}
