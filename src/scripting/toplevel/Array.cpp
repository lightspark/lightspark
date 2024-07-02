/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/toplevel/Array.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "parsing/amf3_generator.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Null.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/toplevel/Undefined.h"
#include "scripting/flash/utils/flashutils.h"
#include <algorithm>

using namespace std;
using namespace lightspark;

Array::Array(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_ARRAY),currentsize(0)
{
}

void Array::finalize()
{
	for (auto it=data_first.begin() ; it != data_first.end(); ++it)
	{
		ASObject* obj = asAtomHandler::getObject(*it);
		*it = asAtomHandler::invalidAtom;
		if (obj)
			obj->removeStoredMember();
	}
	for (auto it=data_second.begin() ; it != data_second.end(); ++it)
	{
		ASObject* obj = asAtomHandler::getObject(it->second);
		it->second = asAtomHandler::invalidAtom;
		if (obj)
			obj->removeStoredMember();
	}
	data_first.clear();
	data_second.clear();
}

bool Array::destruct()
{
	for (auto it=data_first.begin() ; it != data_first.end(); ++it)
	{
		ASObject* obj = asAtomHandler::getObject(*it);
		*it = asAtomHandler::invalidAtom;
		if (obj)
			obj->removeStoredMember();
	}
	for (auto it=data_second.begin() ; it != data_second.end(); ++it)
	{
		ASObject* obj = asAtomHandler::getObject(it->second);
		it->second = asAtomHandler::invalidAtom;
		if (obj)
			obj->removeStoredMember();
	}
	data_first.clear();
	data_second.clear();
	currentsize=0;
	return destructIntern();
}

void Array::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	for (auto it=data_first.begin() ; it != data_first.end();it++)
	{
		ASObject* o = asAtomHandler::getObject(*it);
		*it = asAtomHandler::invalidAtom;
		if (o)
		{
			o->prepareShutdown();
			o->removeStoredMember();
		}
	}
	data_first.clear();
	for (auto it=data_second.begin() ; it != data_second.end();)
	{
		ASObject* o = asAtomHandler::getObject(it->second);
		it = data_second.erase(it);
		if (o)
		{
			o->prepareShutdown();
			o->removeStoredMember();
		}
	}
}

void Array::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->prototype = _MNR((Prototype*)new (c->memoryAccount) ArrayPrototype(c->getInstanceWorker(),c));
	c->isReusable = true;
	c->setVariableAtomByQName("CASEINSENSITIVE",nsNameAndKind(),asAtomHandler::fromUInt(CASEINSENSITIVE),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DESCENDING",nsNameAndKind(),asAtomHandler::fromUInt(DESCENDING),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMERIC",nsNameAndKind(),asAtomHandler::fromUInt(NUMERIC),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RETURNINDEXEDARRAY",nsNameAndKind(),asAtomHandler::fromUInt(RETURNINDEXEDARRAY),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNIQUESORT",nsNameAndKind(),asAtomHandler::fromUInt(UNIQUESORT),CONSTANT_TRAIT);

	// properties
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(_getLength,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(_setLength),SETTER_METHOD,true);

	// public functions
	c->setDeclaredMethodByQName("concat",AS3,c->getSystemState()->getBuiltinFunction(_concat,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("every",AS3,c->getSystemState()->getBuiltinFunction(every,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("filter",AS3,c->getSystemState()->getBuiltinFunction(filter,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("forEach",AS3,c->getSystemState()->getBuiltinFunction(forEach,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",AS3,c->getSystemState()->getBuiltinFunction(indexOf,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",AS3,c->getSystemState()->getBuiltinFunction(lastIndexOf,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("join",AS3,c->getSystemState()->getBuiltinFunction(join,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("map",AS3,c->getSystemState()->getBuiltinFunction(_map,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pop",AS3,c->getSystemState()->getBuiltinFunction(_pop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("push",AS3,c->getSystemState()->getBuiltinFunction(_push_as3,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reverse",AS3,c->getSystemState()->getBuiltinFunction(_reverse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("shift",AS3,c->getSystemState()->getBuiltinFunction(shift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",AS3,c->getSystemState()->getBuiltinFunction(slice,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("some",AS3,c->getSystemState()->getBuiltinFunction(some,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sort",AS3,c->getSystemState()->getBuiltinFunction(_sort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sortOn",AS3,c->getSystemState()->getBuiltinFunction(sortOn),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("splice",AS3,c->getSystemState()->getBuiltinFunction(splice,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString",AS3,c->getSystemState()->getBuiltinFunction(_toLocaleString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unshift",AS3,c->getSystemState()->getBuiltinFunction(unshift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("insertAt",AS3,c->getSystemState()->getBuiltinFunction(insertAt,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeAt",AS3,c->getSystemState()->getBuiltinFunction(removeAt,1),NORMAL_METHOD,true);

	c->prototype->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(_getLength,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->prototype->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(_setLength),SETTER_METHOD,false);
	c->prototype->setVariableByQName("concat","",c->getSystemState()->getBuiltinFunction(_concat,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("every","",c->getSystemState()->getBuiltinFunction(every,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("filter","",c->getSystemState()->getBuiltinFunction(filter,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("forEach","",c->getSystemState()->getBuiltinFunction(forEach,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("indexOf","",c->getSystemState()->getBuiltinFunction(indexOf,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("lastIndexOf","",c->getSystemState()->getBuiltinFunction(lastIndexOf,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("join","",c->getSystemState()->getBuiltinFunction(join,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("map","",c->getSystemState()->getBuiltinFunction(_map,1),DYNAMIC_TRAIT);
	// workaround, pop was encountered not in the AS3 namespace before, need to investigate it further
	c->setDeclaredMethodByQName("pop","",c->getSystemState()->getBuiltinFunction(_pop),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("pop","",c->getSystemState()->getBuiltinFunction(_pop),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("push","",c->getSystemState()->getBuiltinFunction(_push,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("reverse","",c->getSystemState()->getBuiltinFunction(_reverse),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("shift","",c->getSystemState()->getBuiltinFunction(shift),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("slice","",c->getSystemState()->getBuiltinFunction(slice,2),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("some","",c->getSystemState()->getBuiltinFunction(some,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("sort","",c->getSystemState()->getBuiltinFunction(_sort),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("sortOn","",c->getSystemState()->getBuiltinFunction(sortOn),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("splice","",c->getSystemState()->getBuiltinFunction(splice,2),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",c->getSystemState()->getBuiltinFunction(_toLocaleString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("unshift","",c->getSystemState()->getBuiltinFunction(unshift),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Array,_constructor)
{
	Array* th=asAtomHandler::as<Array>(obj);
	th->constructorImpl(args, argslen);
}

ASFUNCTIONBODY_ATOM(Array,generator)
{
	Array* th=Class<Array>::getInstanceSNoArgs(wrk);
	th->constructorImpl(args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void Array::constructorImpl(asAtom* args, const unsigned int argslen)
{
	if(argslen==1 && (asAtomHandler::is<Integer>(args[0]) || asAtomHandler::is<UInteger>(args[0]) || asAtomHandler::is<Number>(args[0])))
	{
		uint32_t size = asAtomHandler::toUInt(args[0]);
		if ((number_t)size != asAtomHandler::toNumber(args[0]))
		{
			createError<RangeError>(getInstanceWorker(),kArrayIndexNotIntegerError, Number::toString(asAtomHandler::toNumber(args[0])));
			return;
		}
		LOG_CALL("Creating array of length " << size);
		resize(size);
		for (uint32_t i=0; i <size && i < ARRAY_SIZE_THRESHOLD; i++)
		{
			set(i,asAtomHandler::invalidAtom,false);
		}
	}
	else
	{
		LOG_CALL("Called Array constructor");
		resize(argslen);
		for(unsigned int i=0;i<argslen;i++)
		{
			set(i,args[i],false);
		}
	}
}

ASFUNCTIONBODY_ATOM(Array,_concat)
{
	Array* th=asAtomHandler::as<Array>(obj);
	Array* res=Class<Array>::getInstanceSNoArgs(wrk);
	
	// copy values into new array
	res->resize(th->size());
	auto it1=th->data_first.begin();
	for(;it1 != th->data_first.end();++it1)
	{
		res->data_first.push_back(*it1);
		ASObject* ob = asAtomHandler::getObject(*it1);
		if (ob)
		{
			ob->incRef();
			ob->addStoredMember();
		}
	}
	auto it2=th->data_second.begin();
	for(;it2 != th->data_second.end();++it2)
	{
		res->data_second[it2->first]=it2->second;
		ASObject* ob = asAtomHandler::getObject(it2->second);
		if (ob)
		{
			ob->incRef();
			ob->addStoredMember();
		}
	}

	for(unsigned int i=0;i<argslen;i++)
	{
		if (asAtomHandler::is<Array>(args[i]))
		{
			// Insert the contents of the array argument
			uint64_t oldSize=res->currentsize;
			uint64_t newSize=oldSize;
			Array* otherArray=asAtomHandler::as<Array>(args[i]);
			auto itother1=otherArray->data_first.begin();
			for(;itother1!=otherArray->data_first.end(); ++itother1)
			{
				ASATOM_INCREF(*itother1);
				res->push(*itother1);
				newSize++;
			}
			auto itother2=otherArray->data_second.begin();
			for(;itother2!=otherArray->data_second.end(); ++itother2)
			{
				asAtom a = itother2->second;
				res->set(newSize+itother2->first, a,false);
			}
			res->resize(oldSize+otherArray->size());
		}
		else
		{
			//Insert the argument
			ASATOM_INCREF(args[i]);
			res->push(args[i]);
		}
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Array,filter)
{
	Array* th=asAtomHandler::as<Array>(obj);
	Array* res=Class<Array>::getInstanceSNoArgs(wrk);
	_NR<IFunction> func;
	asAtom f=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(func));
	if (func.isNull() || th->currentsize == 0)
	{
		ret = asAtomHandler::fromObject(res);
		return;
	}
	f = asAtomHandler::fromObject(func.getPtr());
	
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"filter",th->getClass()->getQualifiedClassName());
		return;
	}
	asAtom params[3];
	asAtom funcRet=asAtomHandler::invalidAtom;
	ASATOM_INCREF(f);
	uint32_t index = 0;
	asAtom closure = asAtomHandler::getClosure(f) ? asAtomHandler::fromObject(asAtomHandler::getClosure(f)) : asAtomHandler::nullAtom;
	while (index < th->currentsize)
	{
		index++;
		if (index <= ARRAY_SIZE_THRESHOLD)
		{
			asAtom& a =th->data_first.at(index-1);
			if (asAtomHandler::isInvalid(a))
				continue;
			params[0] = a;
		}
		else
		{
			auto it=th->data_second.find(index-1);
			if (it == th->data_second.end())
				continue;
			params[0] = it->second;
		}

		params[1] = asAtomHandler::fromUInt(index-1);
		params[2] = asAtomHandler::fromObject(th);

		// ensure that return values are the original values
		asAtom origval = params[0];
		ASATOM_INCREF(origval);
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
				ASATOM_INCREF(origval);
				res->push(origval);
			}
			ASATOM_DECREF(funcRet);
		}
		ASATOM_DECREF(origval);
	}
	ASATOM_DECREF(f);

	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Array, some)
{
	Array* th=asAtomHandler::as<Array>(obj);
	_NR<IFunction> func;
	asAtom f=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(func));
	if (func.isNull() || th->currentsize == 0)
	{
		asAtomHandler::setBool(ret,false);
		return;
	}
	f = asAtomHandler::fromObject(func.getPtr());
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"some",th->getClass()->getQualifiedClassName());
		return;
	}
	asAtom params[3];

	ASATOM_INCREF(f);
	uint32_t index = 0;
	asAtom closure = asAtomHandler::getClosure(f) ? asAtomHandler::fromObject(asAtomHandler::getClosure(f)) : asAtomHandler::nullAtom;
	while (index < th->currentsize)
	{
		index++;
		if (index <= ARRAY_SIZE_THRESHOLD)
		{
			asAtom& a =th->data_first.at(index-1);
			if (asAtomHandler::isInvalid(a))
				continue;
			params[0] = a;
		}
		else
		{
			auto it=th->data_second.find(index-1);
			if (it == th->data_second.end())
				continue;
			params[0] = it->second;
		}
		params[1] = asAtomHandler::fromUInt(index-1);
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
				ASATOM_DECREF(f);
				return;
			}
			ASATOM_DECREF(ret);
		}
	}
	ASATOM_DECREF(f);
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(Array, every)
{
	Array* th=asAtomHandler::as<Array>(obj);
	_NR<IFunction> func;
	asAtom f=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(func));
	if (func.isNull() || th->currentsize == 0)
	{
		asAtomHandler::setBool(ret,true);
		return;
	}
	f = asAtomHandler::fromObject(func.getPtr());
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"every",th->getClass()->getQualifiedClassName());
		return;
	}
	asAtom params[3];
	ASATOM_INCREF(f);
	asAtom closure = asAtomHandler::getClosure(f) ? asAtomHandler::fromObject(asAtomHandler::getClosure(f)) : asAtomHandler::nullAtom;

	uint32_t index = 0;
	while (index < th->currentsize)
	{
		index++;
		if (index <= ARRAY_SIZE_THRESHOLD)
		{
			asAtom& a =th->data_first.at(index-1);
			if (asAtomHandler::isInvalid(a))
				continue;
			params[0] = a;
		}
		else
		{
			auto it=th->data_second.find(index-1);
			if (it == th->data_second.end())
				continue;
			params[0] = it->second;
		}
		params[1] = asAtomHandler::fromUInt(index-1);
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
			if(!asAtomHandler::Boolean_concrete(ret))
			{
				ASATOM_DECREF(f);
				return;
			}
			ASATOM_DECREF(ret);
		}
	}
	ASATOM_DECREF(f);
	asAtomHandler::setBool(ret,true);
}

ASFUNCTIONBODY_ATOM(Array,_getLength)
{
	Array* th=asAtomHandler::as<Array>(obj);
	asAtomHandler::setUInt(ret,wrk,th->currentsize);
}

ASFUNCTIONBODY_ATOM(Array,_setLength)
{
	uint32_t newLen;
	ARG_CHECK(ARG_UNPACK(newLen));
	Array* th=asAtomHandler::as<Array>(obj);
	if (th->getClass() && th->getClass()->isSealed)
		return;
	//If newLen is equal to size do nothing
	if(newLen==th->size())
		return;
	th->resize(newLen);
}

ASFUNCTIONBODY_ATOM(Array,forEach)
{
	Array* th=asAtomHandler::as<Array>(obj);
	_NR<IFunction> func;
	asAtom f=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(func));
	asAtomHandler::setUndefined(ret);
	if (func.isNull() || th->currentsize == 0)
		return;
	f = asAtomHandler::fromObject(func.getPtr());
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"foreach",th->getClass()->getQualifiedClassName());
		return;
	}
	asAtom params[3];

	ASATOM_INCREF(f);
	asAtom closure = asAtomHandler::getClosure(f) ? asAtomHandler::fromObject(asAtomHandler::getClosure(f)) : asAtomHandler::nullAtom;
	uint32_t index = 0;
	uint32_t s = th->size(); // remember current size, as it may change inside the called function
	while (index < s)
	{
		index++;
		if (index <= ARRAY_SIZE_THRESHOLD)
		{
			asAtom& a =th->data_first.at(index-1);
			if (asAtomHandler::isInvalid(a))
				continue;
			else
				params[0] = a;
		}
		else
		{
			auto it=th->data_second.find(index-1);
			if (it == th->data_second.end())
				continue;
			else
				params[0]=it->second;
		}
		params[1] = asAtomHandler::fromUInt(index-1);
		params[2] = asAtomHandler::fromObject(th);

		asAtom funcret=asAtomHandler::invalidAtom;
		if(argslen == 1 )
		{
			asAtomHandler::callFunction(f,wrk,funcret,closure, params, 3,false);
		}
		else
		{
			asAtomHandler::callFunction(f,wrk,funcret,args[1], params, 3,false);
		}
		ASATOM_DECREF(funcret);
	}
	ASATOM_DECREF(f);
}

ASFUNCTIONBODY_ATOM(Array, _reverse)
{
	Array* th=asAtomHandler::as<Array>(obj);

	if (th->data_second.empty())
		std::reverse(th->data_first.begin(),th->data_first.end());
	else
	{
		std::unordered_map<uint32_t, asAtom> tmp = std::unordered_map<uint32_t, asAtom>(th->data_second.begin(),th->data_second.end());
		for (uint32_t i = 0; i < th->data_first.size(); i++)
		{
			tmp[i] = th->data_first[i];
		}
		uint32_t size = th->size();
		th->data_first.clear();
		th->data_second.clear();
		auto it=tmp.begin();
		for(;it != tmp.end();++it)
		{
			th->set(size-(it->first+1),it->second,false);
		}
	}
	th->incRef();
	ret = asAtomHandler::fromObject(th);
}

ASFUNCTIONBODY_ATOM(Array,lastIndexOf)
{
	Array* th=asAtomHandler::as<Array>(obj);
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"lastIndexOf",th->getClass()->getQualifiedClassName());
		return;
	}
	number_t index;
	asAtom arg0=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(arg0) (index, 0x7fffffff));
	int32_t res=-1;

	if(argslen == 1 && th->currentsize == 0)
	{
		asAtomHandler::setInt(ret,wrk,(int32_t)-1);
		return;
	}

	size_t i;

	if(std::isnan(index))
	{
		asAtomHandler::setInt(ret,wrk,(int32_t)0);
		return;
	}

	int j = index; //Preserve sign
	if(j < 0) //Negative offset, use it as offset from the end of the array
	{
		if((size_t)-j > th->size())
		{
			asAtomHandler::setInt(ret,wrk,(int32_t)-1);
			return;
		}
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
	do
	{
		asAtom a=asAtomHandler::invalidAtom;
		if ( i >= ARRAY_SIZE_THRESHOLD)
		{
			auto it = th->data_second.find(i);
			if (it == th->data_second.end())
				continue;
			a = it->second;
		}
		else
		{
			a = th->data_first[i];
			if (asAtomHandler::isInvalid(a))
				continue;
		}
		if(asAtomHandler::isEqualStrict(a,wrk,arg0))
		{
			res=i;
			break;
		}
	}
	while(i--);

	asAtomHandler::setInt(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(Array,shift)
{
	if (!asAtomHandler::is<Array>(obj))
	{
		// this seems to be how Flash handles the generic shift calls
		if (asAtomHandler::is<Vector>(obj))
		{
			Vector::shift(ret,wrk,obj,args,argslen);
			return;
		}
		if (asAtomHandler::is<ByteArray>(obj))
		{
			ByteArray::shift(ret,wrk,obj,args,argslen);
			return;
		}
		// for other objects we just decrease the length property
		multiname lengthName(nullptr);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(wrk->getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o=asAtomHandler::invalidAtom;
		asAtomHandler::getObject(obj)->getVariableByMultiname(o,lengthName,SKIP_IMPL,wrk);
		uint32_t res = asAtomHandler::toUInt(o);
		asAtom v = asAtomHandler::fromUInt(res-1);
		if (res > 0)
			asAtomHandler::getObject(obj)->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED,nullptr,wrk);
		asAtomHandler::setUndefined(ret);
		return;
	}
	Array* th=asAtomHandler::as<Array>(obj);
	if(!th->size())
	{
		asAtomHandler::setUndefined(ret);
		return;
	}
	if (th->data_first.size() > 0)
		ret = th->data_first[0];
	if (asAtomHandler::isInvalid(ret))
		ret = asAtomHandler::undefinedAtom;
	if (th->data_first.size() > 0)
		th->data_first.erase(th->data_first.begin());

	std::unordered_map<uint32_t,asAtom> tmp;
	auto it=th->data_second.begin();
	for (; it != th->data_second.end(); ++it )
	{
		if(it->first)
		{
			if (it->first == ARRAY_SIZE_THRESHOLD)
				th->data_first[ARRAY_SIZE_THRESHOLD-1] = it->second;
			else
				tmp[it->first-1]=it->second;
		}
	}
	th->data_second.clear();
	th->data_second.insert(tmp.begin(),tmp.end());
	th->resize(th->size()-1);
	ASObject* o = asAtomHandler::getObject(ret);
	if (o)
	{
		o->incRef();// will be decreffed in removeStoreMember
		o->removeStoredMember();
	}
}

int Array::capIndex(int i)
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

ASFUNCTIONBODY_ATOM(Array,slice)
{
	Array* th=asAtomHandler::as<Array>(obj);
	uint32_t startIndex;
	uint32_t endIndex;

	ARG_CHECK(ARG_UNPACK(startIndex, 0) (endIndex, 16777215));
	startIndex=th->capIndex(startIndex);
	endIndex=th->capIndex(endIndex);

	Array* res=Class<Array>::getInstanceSNoArgs(wrk);
	uint32_t j = 0;
	for(uint32_t i=startIndex; i<endIndex && i< th->currentsize; i++) 
	{
		asAtom a = th->at((uint32_t)i);
		ASATOM_INCREF(a);
		if (asAtomHandler::isValid(a))
			res->push(a);
		j++;
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Array,splice)
{
	Array* th=asAtomHandler::as<Array>(obj);
	int startIndex;
	int deleteCount;
	//By default, delete all the element up to the end
	//DeleteCount defaults to the array len, it will be capped below
	ARG_CHECK(ARG_UNPACK_MORE_ALLOWED(startIndex) (deleteCount, th->size()));

	uint32_t totalSize=th->size();
	Array* res=Class<Array>::getInstanceSNoArgs(wrk);

	startIndex=th->capIndex(startIndex);

	if((uint32_t)(startIndex+deleteCount)>totalSize)
		deleteCount=totalSize-startIndex;

	res->resize(deleteCount);
	if(deleteCount)
	{
		// Derived classes may be sealed!
		if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
		{
			createError<ReferenceError>(wrk,kReadSealedError,"splice",th->getClass()->getQualifiedClassName());
			return;
		}
		// write deleted items to return array
		for(int i=0;i<deleteCount;i++)
		{
			asAtom a = th->at((uint32_t)startIndex+i);
			if (asAtomHandler::isValid(a))
				res->set(i,a,false,false,false);
		}
		// delete items from current array (no need to decref/removemember, as they are added to the result)
		for (int i = 0; i < deleteCount; i++)
		{
			if (i+startIndex < ARRAY_SIZE_THRESHOLD)
			{
				if ((uint32_t)startIndex <th->data_first.size())
					th->data_first.erase(th->data_first.begin()+startIndex);
			}
			else
			{
				auto it = th->data_second.find(startIndex+i);
				if (it != th->data_second.end())
				{
					th->data_second.erase(it);
				}
			}
		}
	}
	// remember items in current array that have to be moved to new position
	vector<asAtom> tmp = vector<asAtom>(totalSize- (startIndex+deleteCount));
	for (uint32_t i = (uint32_t)startIndex+deleteCount; i < totalSize ; i++)
	{
		if (i < ARRAY_SIZE_THRESHOLD)
		{
			if ((uint32_t)startIndex < th->data_first.size())
			{
				tmp[i-(startIndex+deleteCount)] = th->data_first[(uint32_t)startIndex];
				th->data_first.erase(th->data_first.begin()+startIndex);
			}
		}
		else
		{
			auto it = th->data_second.find(i);
			if (it != th->data_second.end())
			{
				tmp[i-(startIndex+deleteCount)] = it->second;
				th->data_second.erase(it);
			}
		}
	}
	th->resize(startIndex,false);

	
	//Insert requested values starting at startIndex
	for(unsigned int i=2;i<argslen;i++)
	{
		ASATOM_INCREF(args[i]);
		th->push(args[i]);
	}
	// move remembered items to new position
	th->resize((totalSize-deleteCount)+(argslen > 2 ? argslen-2 : 0));
	for(uint32_t i=0;i<totalSize- (startIndex+deleteCount);i++)
	{
		if (asAtomHandler::isValid(tmp[i]))
			th->set(startIndex+i+(argslen > 2 ? argslen-2 : 0),tmp[i],false,false,false);
	}
	ret =asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Array,join)
{
	Array* th=asAtomHandler::as<Array>(obj);
	string res;
	tiny_string del;
	ARG_CHECK(ARG_UNPACK(del, ","));

	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"join",th->getClass()->getQualifiedClassName());
		return;
	}
	for(uint32_t i=0;i<th->size();i++)
	{
		asAtom o = th->at(i);
		if (!asAtomHandler::is<Undefined>(o) && !asAtomHandler::is<Null>(o))
			res+= asAtomHandler::toString(o,wrk).raw_buf();
		if(i!=th->size()-1)
			res+=del.raw_buf();
	}
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}

ASFUNCTIONBODY_ATOM(Array,indexOf)
{
	Array* th=asAtomHandler::as<Array>(obj);
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"indexOf",th->getClass()->getQualifiedClassName());
		return;
	}
	int32_t res=-1;
	int32_t index;
	asAtom arg0=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(arg0) (index, 0));
	if (index < 0) index = th->size()+ index;
	if (index < 0) index = 0;

	if ((uint32_t)index < th->data_first.size())
	{
		for (auto it=th->data_first.begin()+index ; it != th->data_first.end(); ++it )
		{
			if(asAtomHandler::isEqualStrict(*it,wrk,arg0))
			{
				res=it - th->data_first.begin();
				break;
			}
		}
	}
	if (res == -1)
	{
		for (auto it=th->data_second.begin() ; it != th->data_second.end(); ++it )
		{
			if (it->first < (uint32_t)index)
				continue;
			if(asAtomHandler::isEqualStrict(it->second,wrk,arg0))
			{
				res=it->first;
				break;
			}
		}
	}
	asAtomHandler::setInt(ret,wrk,res);
}

ASFUNCTIONBODY_ATOM(Array,_pop)
{
	if (!asAtomHandler::is<Array>(obj))
	{
		// this seems to be how Flash handles the generic pop calls
		if (asAtomHandler::is<Vector>(obj))
		{
			Vector::_pop(ret,wrk,obj,args,argslen);
			return;
		}
		if (asAtomHandler::is<ByteArray>(obj))
		{
			ByteArray::pop(ret,wrk,obj,args,argslen);
			return;
		}
		// for other objects we just decrease the length property
		multiname lengthName(nullptr);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(wrk->getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o=asAtomHandler::invalidAtom;
		asAtomHandler::getObject(obj)->getVariableByMultiname(o,lengthName,SKIP_IMPL,wrk);
		uint32_t res = asAtomHandler::toUInt(o);
		ASATOM_DECREF(o);
		asAtom v = asAtomHandler::fromUInt(res-1);
		if (res > 0)
			asAtomHandler::getObject(obj)->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED,nullptr,wrk);
		asAtomHandler::setUndefined(ret);
		return;
	}
	Array* th=asAtomHandler::as<Array>(obj);
	uint32_t size =th->size();
	asAtomHandler::setUndefined(ret);
	if (size == 0)
		return;
	
	if (size <= ARRAY_SIZE_THRESHOLD)
	{
		if (th->data_first.size() > 0)
		{
			ret = *th->data_first.rbegin();
			th->data_first.pop_back();
			if (asAtomHandler::isInvalid(ret))
				asAtomHandler::setUndefined(ret);
			else
			{
				ASObject* o = asAtomHandler::getObject(ret);
				if (o)
				{
					o->incRef();// will be decreffed in removeStoreMember
					o->removeStoredMember();
				}
			}
		}
	}
	else
	{
		auto it = th->data_second.find(size-1);
		if (it != th->data_second.end())
		{
			ret=it->second;
			th->data_second.erase(it);
			ASObject* o = asAtomHandler::getObject(ret);
			if (o)
			{
				o->incRef();// will be decreffed in removeStoreMember
				o->removeStoredMember();
			}
		}
		else
			asAtomHandler::setUndefined(ret);
	}
	th->currentsize--;
}


bool Array::sortComparatorDefault::operator()(const asAtom& d1, const asAtom& d2)
{
	asAtom o1 = d1;
	asAtom o2 = d2;
	if(isNumeric)
	{
		number_t a=numeric_limits<double>::quiet_NaN();
		number_t b=numeric_limits<double>::quiet_NaN();
		if (useoldversion)
		{
			a=asAtomHandler::toInt(o1) & 0x1fffffff;
			b=asAtomHandler::toInt(o2) & 0x1fffffff;
		}
		else
		{
			a=asAtomHandler::toNumber(o1);
			b=asAtomHandler::toNumber(o2);
		}
		if((!asAtomHandler::isNumeric(o1) && std::isnan(a)) || (!asAtomHandler::isNumeric(o2) && std::isnan(b)))
			throw RunTimeException("Cannot sort non number with Array.NUMERIC option");
		if(isDescending)
			return a>b;
		else
			return a<b;
	}
	else
	{
		//Comparison is always in lexicographic order
		tiny_string s1;
		tiny_string s2;
		s1=asAtomHandler::toString(o1,getWorker());
		s2=asAtomHandler::toString(o2,getWorker());

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
// std::sort expects strict weak ordering for the comparison function
// this is not guarranteed by user defined comparison functions, so we need our own sorting method.

// this is the quicksort algorithm used by avmplus
// see https://github.com/adobe-flash/avmplus/blob/master/core/ArrayClass.cpp
void qsort(std::vector<asAtom>& v, Array::sortComparatorWrapper& comp, uint32_t lo, uint32_t hi)
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

number_t Array::sortComparatorWrapper::compare(const asAtom& d1, const asAtom& d2)
{
	asAtom objs[2];
	objs[0]=d1;
	objs[1]=d2;

	assert(asAtomHandler::isFunction(comparator));
	asAtom ret=asAtomHandler::invalidAtom;
	asAtom obj = asAtomHandler::getClosureAtom(comparator);
	if (asAtomHandler::is<AVM1Function>(comparator))
		asAtomHandler::as<AVM1Function>(comparator)->call(&ret,&obj,objs,1);
	else
	{
		// don't coerce the result, as it may be an int that would loose it's sign through coercion
		asAtomHandler::callFunction(comparator,asAtomHandler::as<IFunction>(comparator)->getInstanceWorker(),ret,obj, objs, 2,false,false);
	}
	assert_and_throw(asAtomHandler::isValid(ret));
	return asAtomHandler::toNumber(ret);
}

ASFUNCTIONBODY_ATOM(Array,_sort)
{
	Array* th=asAtomHandler::as<Array>(obj);
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"sort",th->getClass()->getQualifiedClassName());
		return;
	}
	asAtom comp=asAtomHandler::invalidAtom;
	bool isNumeric=false;
	bool isCaseInsensitive=false;
	bool isDescending=false;
	for(uint32_t i=0;i<argslen;i++)
	{
		if(asAtomHandler::isFunction(args[i])) //Comparison func
		{
			assert_and_throw(asAtomHandler::isInvalid(comp));
			comp=args[i];
		}
		else
		{
			uint32_t options=asAtomHandler::toInt(args[i]);
			if(options&NUMERIC)
				isNumeric=true;
			if(options&CASEINSENSITIVE)
				isCaseInsensitive=true;
			if(options&DESCENDING)
				isDescending=true;
			if(options&(~(NUMERIC|CASEINSENSITIVE|DESCENDING)))
				throw UnsupportedException("Array::sort not completely implemented");
		}
	}
	std::vector<asAtom> tmp;
	auto it1=th->data_first.begin();
	for(;it1 != th->data_first.end();++it1)
	{
		if (asAtomHandler::isInvalid(*it1) || asAtomHandler::isUndefined(*it1))
			continue;
		tmp.push_back(*it1);
	}
	auto it2=th->data_second.begin();
	for(;it2 != th->data_second.end();++it2)
	{
		if (asAtomHandler::isInvalid(it2->second) || asAtomHandler::isUndefined(it2->second))
			continue;
		tmp.push_back(it2->second);
	}
	
	if(asAtomHandler::isValid(comp))
	{
		sortComparatorWrapper c(comp);
		qsort(tmp,c,0,tmp.size()-1);
	}
	else
		sort(tmp.begin(),tmp.end(),sortComparatorDefault(wrk->getSystemState()->getSwfVersion() < 11, isNumeric,isCaseInsensitive,isDescending));

	th->data_first.clear();
	th->data_second.clear();
	std::vector<asAtom>::iterator ittmp=tmp.begin();
	int i = 0;
	for(;ittmp != tmp.end();++ittmp)
	{
		if (i < ARRAY_SIZE_THRESHOLD)
			th->data_first.push_back(*ittmp);
		else
			th->data_second[i] = *ittmp;
		i++;
	}
	ASATOM_INCREF(obj);
	ret = obj;
}

bool Array::sortOnComparator::operator()(const sorton_value& d1, const sorton_value& d2)
{
	std::vector<sorton_field>::iterator it=fields.begin();
	uint32_t i = 0;
	for(;it != fields.end();++it)
	{
		asAtom obj1 = d1.sortvalues.at(i);
		asAtom obj2 = d2.sortvalues.at(i);
		i++;
		if(it->isNumeric)
		{
			number_t a=numeric_limits<double>::quiet_NaN();
			number_t b=numeric_limits<double>::quiet_NaN();
			a=asAtomHandler::toNumber(obj1);
			
			b=asAtomHandler::toNumber(obj2);
			
			if((!asAtomHandler::isNumeric(obj1) && std::isnan(a)) || (!asAtomHandler::isNumeric(obj2) && std::isnan(b)))
				throw RunTimeException("Cannot sort non number with Array.NUMERIC option");
			if(it->isDescending)
				return a>b;
			else
				return a<b;
		}
		else
		{
			//Comparison is always in lexicographic order
			tiny_string s1;
			tiny_string s2;
			s1=asAtomHandler::toString(obj1,getWorker());
			s2=asAtomHandler::toString(obj2,getWorker());
			if (s1 != s2)
			{
				if(it->isDescending)
				{
					if(it->isCaseInsensitive)
						return s1.strcasecmp(s2)>0;
					else
						return s1>s2;
				}
				else
				{
					if(it->isCaseInsensitive)
						return s1.strcasecmp(s2)<0;
					else
						return s1<s2;
				}
			}
		}
	}
	return false;
}

ASFUNCTIONBODY_ATOM(Array,sortOn)
{
	if (argslen != 1 && argslen != 2)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "1",
					  Integer::toString(argslen));
		return;
	}
	Array* th=asAtomHandler::as<Array>(obj);
	std::vector<sorton_field> sortfields;
	if(asAtomHandler::is<Array>(args[0]))
	{
		Array* obj=asAtomHandler::as<Array>(args[0]);
		int n = 0;
		for(uint32_t i = 0;i<obj->size();i++)
		{
			multiname sortfieldname(nullptr);
			sortfieldname.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
			asAtom atom = obj->at(i);
			sortfieldname.setName(atom,wrk);
			sorton_field sf(sortfieldname);
			sortfields.push_back(sf);
		}
		if (argslen == 2 && asAtomHandler::is<Array>(args[1]))
		{
			Array* opts=asAtomHandler::as<Array>(args[1]);
			auto itopt=opts->data_first.begin();
			int nopt = 0;
			for(;itopt != opts->data_first.end() && nopt < n;++itopt)
			{
				uint32_t options=0;
				options = asAtomHandler::toInt(*itopt);
				if(options&NUMERIC)
					sortfields[nopt].isNumeric=true;
				if(options&CASEINSENSITIVE)
					sortfields[nopt].isCaseInsensitive=true;
				if(options&DESCENDING)
					sortfields[nopt].isDescending=true;
				if(options&(~(NUMERIC|CASEINSENSITIVE|DESCENDING)))
					throw UnsupportedException("Array::sort not completely implemented");
				nopt++;
			}
		}
	}
	else
	{
		multiname sortfieldname(nullptr);
		asAtom atom = args[0];
		sortfieldname.setName(atom,wrk);
		sortfieldname.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
		sorton_field sf(sortfieldname);
		if (argslen == 2)
		{
			uint32_t options = asAtomHandler::toInt(args[1]);
			if(options&NUMERIC)
				sf.isNumeric=true;
			if(options&CASEINSENSITIVE)
				sf.isCaseInsensitive=true;
			if(options&DESCENDING)
				sf.isDescending=true;
			if(options&(~(NUMERIC|CASEINSENSITIVE|DESCENDING)))
				throw UnsupportedException("Array::sort not completely implemented");
		}
		sortfields.push_back(sf);
	}
	
	std::vector<sorton_value> tmp;
	auto it1=th->data_first.begin();
	for(;it1 != th->data_first.end();++it1)
	{
		if (asAtomHandler::isInvalid(*it1) || asAtomHandler::isUndefined(*it1))
			continue;
		// ensure ASObjects are created
		asAtomHandler::toObject(*it1,wrk);
		
		sorton_value v(*it1);
		for (auto itsf=sortfields.begin();itsf != sortfields.end(); itsf++)
		{
			asAtom tmpval=asAtomHandler::invalidAtom;
			asAtomHandler::getObject(*it1)->getVariableByMultiname(tmpval,itsf->fieldname,GET_VARIABLE_OPTION::NONE,wrk);
			v.sortvalues.push_back(tmpval);
		}
		tmp.push_back(v);
	}
	auto it2=th->data_second.begin();
	for(;it2 != th->data_second.end();++it2)
	{
		if (asAtomHandler::isInvalid(it2->second) || asAtomHandler::isUndefined(it2->second))
			continue;
		// ensure ASObjects are created
		asAtomHandler::toObject(it2->second,wrk);
		
		sorton_value v(it2->second);
		for (auto itsf=sortfields.begin();itsf != sortfields.end(); itsf++)
		{
			asAtom tmpval=asAtomHandler::invalidAtom;
			asAtomHandler::getObject(it2->second)->getVariableByMultiname(tmpval,itsf->fieldname,GET_VARIABLE_OPTION::NONE,wrk);
			v.sortvalues.push_back(tmpval);
		}
		tmp.push_back(v);
	}
	
	sort(tmp.begin(),tmp.end(),sortOnComparator(sortfields));

	th->data_first.clear();
	th->data_second.clear();
	std::vector<sorton_value>::iterator ittmp=tmp.begin();
	uint32_t i = 0;
	for(;ittmp != tmp.end();++ittmp)
	{
		if (i < ARRAY_SIZE_THRESHOLD)
			th->data_first.push_back(ittmp->dataAtom);
		else
			th->data_second[i] = ittmp->dataAtom;
		i++;
	}
	// according to spec sortOn should return "nothing"(?), but it seems that the array is returned
	ASATOM_INCREF(obj);
	ret = obj;
}

ASFUNCTIONBODY_ATOM(Array,unshift)
{
	if (!asAtomHandler::is<Array>(obj))
	{
		// this seems to be how Flash handles the generic unshift calls
		if (asAtomHandler::is<Vector>(obj))
		{
			Vector::unshift(ret,wrk,obj,args,argslen);
			return;
		}
		if (asAtomHandler::is<ByteArray>(obj))
		{
			ByteArray::unshift(ret,wrk,obj,args,argslen);
			return;
		}
		// for other objects we just increase the length property
		multiname lengthName(nullptr);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(wrk->getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o=asAtomHandler::invalidAtom;
		asAtomHandler::getObject(obj)->getVariableByMultiname(o,lengthName,SKIP_IMPL,wrk);
		uint32_t res = asAtomHandler::toUInt(o);
		asAtom v = asAtomHandler::fromUInt(res+argslen);
		asAtomHandler::getObject(obj)->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED,nullptr,wrk);
		asAtomHandler::setUndefined(ret);
		return;
	}
	Array* th=asAtomHandler::as<Array>(obj);
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() > 12 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kWriteSealedError,"unshift",th->getClass()->getQualifiedClassName());
		return;
	}
	if (argslen > 0)
	{
		th->resize(th->size()+argslen);
		std::map<uint32_t,asAtom> tmp;
		for (uint32_t i = 0; i< th->data_first.size(); i++)
		{
			tmp[i+argslen]=th->data_first[i];
		}
		for (auto it=th->data_second.begin(); it != th->data_second.end(); ++it )
		{
			tmp[it->first+argslen]=it->second;
		}
		
		for(uint32_t i=0;i<argslen;i++)
		{
			tmp[i] = args[i];
			ASObject* ob = asAtomHandler::getObject(args[i]);
			if (ob)
			{
				ob->incRef();
				ob->addStoredMember();
			}
		}
		th->data_first.clear();
		th->data_second.clear();
		for (auto it=tmp.begin(); it != tmp.end(); ++it )
		{
			th->set(it->first,it->second,false,false,false);
		}
	}
	asAtomHandler::setUInt(ret,wrk,(int32_t)th->size());
}

ASFUNCTIONBODY_ATOM(Array,_push)
{
	if (!asAtomHandler::is<Array>(obj))
	{
		// this seems to be how Flash handles the generic push calls
		if (asAtomHandler::is<Vector>(obj))
		{
			Vector::push(ret,wrk,obj,args,argslen);
			return;
		}
		if (asAtomHandler::is<ByteArray>(obj))
		{
			ByteArray::push(ret,wrk,obj,args,argslen);
			return;
		}
		// for other objects we just increase the length property
		multiname lengthName(nullptr);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(wrk->getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o=asAtomHandler::invalidAtom;
		asAtomHandler::getObject(obj)->getVariableByMultiname(o,lengthName,SKIP_IMPL,wrk);
		uint32_t res = asAtomHandler::toUInt(o);
		ASATOM_DECREF(o);
		asAtom v = asAtomHandler::fromUInt(res+argslen);
		asAtomHandler::getObject(obj)->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED,nullptr,wrk);
		asAtomHandler::setUndefined(ret);
		return;
	}
	Array* th=asAtomHandler::as<Array>(obj);
	uint64_t s = th->currentsize;
	for(unsigned int i=0;i<argslen;i++)
	{
		ASATOM_INCREF(args[i]);
		th->push(args[i]);
	}
	// currentsize is set even if push fails
	th->currentsize = s+argslen;
	asAtomHandler::setInt(ret,wrk,(int32_t)th->size());
}
// AS3 handles push on uint.MAX_VALUE differently than ECMA, so we need to push methods
ASFUNCTIONBODY_ATOM(Array,_push_as3)
{
	if (!asAtomHandler::is<Array>(obj))
	{
		// this seems to be how Flash handles the generic push calls
		if (asAtomHandler::is<Vector>(obj))
		{
			Vector::push(ret,wrk,obj,args,argslen);
			return;
		}
		if (asAtomHandler::is<ByteArray>(obj))
		{
			ByteArray::push(ret,wrk,obj,args,argslen);
			return;
		}
		// for other objects we just increase the length property
		multiname lengthName(nullptr);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(wrk->getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = true;
		asAtom o=asAtomHandler::invalidAtom;
		asAtomHandler::getObject(obj)->getVariableByMultiname(o,lengthName,SKIP_IMPL,wrk);
		uint32_t res = asAtomHandler::toUInt(o);
		asAtom v = asAtomHandler::fromUInt(res+argslen);
		asAtomHandler::getObject(obj)->setVariableByMultiname(lengthName,v,CONST_NOT_ALLOWED,nullptr,wrk);
		asAtomHandler::setUndefined(ret);
		return;
	}
	Array* th=asAtomHandler::as<Array>(obj);
	for(unsigned int i=0;i<argslen;i++)
	{
		if (th->size() >= UINT32_MAX)
			break;
		ASATOM_INCREF(args[i]);
		th->push(args[i]);
	}
	asAtomHandler::setInt(ret,wrk,(int32_t)th->size());
}

ASFUNCTIONBODY_ATOM(Array,_map)
{
	Array* th=asAtomHandler::as<Array>(obj);

	if(argslen < 1)
	{
		createError<ArgumentError>(wrk,kWrongArgumentCountError, "Array.map", "1", Integer::toString(argslen));
		return;
	}
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"map",th->getClass()->getQualifiedClassName());
		return;
	}
	asAtom func=asAtomHandler::invalidAtom;
	if (!asAtomHandler::is<RegExp>(args[0]))
	{
		assert_and_throw(asAtomHandler::isFunction(args[0]));
		func=args[0];
	}
	asAtom closure = asAtomHandler::isValid(func) && asAtomHandler::getClosure(func) ? asAtomHandler::fromObject(asAtomHandler::getClosure(func)) : asAtomHandler::nullAtom;
	
	Array* arrayRet=Class<Array>::getInstanceSNoArgs(wrk);

	asAtom params[3];
	uint32_t index = 0;
	uint32_t s = th->size(); // remember current size, as it may change inside the called function
	while (index < s)
	{
		index++;
		if (index <= ARRAY_SIZE_THRESHOLD)
		{
			asAtom& a =th->data_first.at(index-1);
			if(asAtomHandler::isValid(a))
				params[0] = a;
			else
				params[0]=asAtomHandler::undefinedAtom;
		}
		else
		{
			auto it=th->data_second.find(index);
			if(asAtomHandler::isValid(it->second))
				params[0]=it->second;
			else
				params[0]=asAtomHandler::undefinedAtom;
		}
		params[1] = asAtomHandler::fromUInt(index-1);
		params[2] = asAtomHandler::fromObject(th);
		asAtom funcRet=asAtomHandler::invalidAtom;
		if (asAtomHandler::isValid(func))
		{
			asAtomHandler::callFunction(func,wrk,funcRet,argslen > 1? args[1] : closure, params, 3,false);
		}
		else
		{
			RegExp::exec(funcRet,wrk,args[0],args,1);
		}
		assert_and_throw(asAtomHandler::isValid(funcRet));
		arrayRet->push(funcRet);
	}

	ret = asAtomHandler::fromObject(arrayRet);
}

ASFUNCTIONBODY_ATOM(Array,_toString)
{
	if(asAtomHandler::getObject(obj) == Class<Number>::getClass(wrk->getSystemState())->prototype->getObj())
	{
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	if(!asAtomHandler::is<Array>(obj))
	{
		LOG(LOG_NOT_IMPLEMENTED, "generic Array::toString");
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	
	Array* th=asAtomHandler::as<Array>(obj);
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"toString",th->getClass()->getQualifiedClassName());
		return;
	}
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv()));
}

ASFUNCTIONBODY_ATOM(Array,_toLocaleString)
{
	if(asAtomHandler::getObject(obj) == Class<Number>::getClass(wrk->getSystemState())->prototype->getObj())
	{
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	if(!asAtomHandler::is<Array>(obj))
	{
		LOG(LOG_NOT_IMPLEMENTED, "generic Array::toLocaleString");
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	
	Array* th=asAtomHandler::as<Array>(obj);
	// Derived classes may be sealed!
	if (th->getSystemState()->getSwfVersion() < 13 && th->getClass() && th->getClass()->isSealed)
	{
		createError<ReferenceError>(wrk,kReadSealedError,"toLocaleString",th->getClass()->getQualifiedClassName());
		return;
	}
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv(true)));
}

ASFUNCTIONBODY_ATOM(Array,insertAt)
{
	Array* th=asAtomHandler::as<Array>(obj);
	int32_t index;
	asAtom o=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(index)(o));
	if (index < 0 && th->currentsize >= (uint32_t)(-index))
		index = th->currentsize+(index);
	if (index < 0)
		index = 0;
	if ((uint32_t)index >= th->currentsize)
	{
		th->currentsize++;
		th->set(th->currentsize-1,o,false);
	}
	else
	{
		std::map<uint32_t,asAtom> tmp;
		if (index < ARRAY_SIZE_THRESHOLD)
			th->data_first.insert(th->data_first.begin()+index,o);
		auto it=th->data_second.begin();
		for (; it != th->data_second.end(); ++it )
		{
			tmp[it->first+(it->first >= (uint32_t)index ? 1 : 0)]=it->second;
		}
		if (th->data_first.size() > ARRAY_SIZE_THRESHOLD)
		{
			tmp[ARRAY_SIZE_THRESHOLD] = th->data_first[ARRAY_SIZE_THRESHOLD];
			th->data_first.pop_back();
		}
		th->data_second.clear();
		auto ittmp = tmp.begin();
		while (ittmp != tmp.end())
		{
			th->set(ittmp->first,ittmp->second,false);
			ittmp++;
		}
		th->currentsize++;
		th->set(index,o,false);
	}
}

ASFUNCTIONBODY_ATOM(Array,removeAt)
{
	Array* th=asAtomHandler::as<Array>(obj);
	int32_t index;
	ARG_CHECK(ARG_UNPACK(index));
	if (index < 0)
		index = th->currentsize+index;
	if (index < 0)
		index = 0;
	asAtomHandler::setUndefined(ret);
	if (index < ARRAY_SIZE_THRESHOLD)
	{
		if ((uint32_t)index < th->data_first.size())
		{
			ret = th->data_first[index];
			if (asAtomHandler::isInvalid(ret))
				asAtomHandler::setUndefined(ret);
			th->data_first.erase(th->data_first.begin()+index);
		}
	}
	else
	{
		auto it = th->data_second.find(index);
		if(it != th->data_second.end())
		{
			ret = it->second;
			if (asAtomHandler::isInvalid(ret))
				asAtomHandler::setUndefined(ret);
			th->data_second.erase(it);
		}
	}
	if ((uint32_t)index < th->currentsize)
		th->currentsize--;
	std::map<uint32_t,asAtom> tmp;
	auto it=th->data_second.begin();
	for (; it != th->data_second.end(); ++it )
	{
		if (it->first == ARRAY_SIZE_THRESHOLD)
			th->data_first[ARRAY_SIZE_THRESHOLD-1]=it->second;
		else
			tmp[it->first-(it->first > (uint32_t)index ? 1 : 0)]=it->second;
	}
	th->data_second.clear();
	th->data_second.insert(tmp.begin(),tmp.end());
	ASObject* o = asAtomHandler::getObject(ret);
	if (o)
	{
		o->incRef();
		o->removeStoredMember();
	}
}
int32_t Array::getVariableByMultiname_i(const multiname& name, ASWorker* wrk)
{
	assert_and_throw(implEnable);
	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::getVariableByMultiname_i(name,wrk);

	if(index<size())
	{
		if (index < ARRAY_SIZE_THRESHOLD)
		{
			return data_first.size() > index ? asAtomHandler::toInt(data_first.at(index)) : 0;
		}
		auto it = data_second.find(index);
		if (it == data_second.end())
			return 0;
		return asAtomHandler::toInt(it->second);
	}

	return ASObject::getVariableByMultiname_i(name,wrk);
}

GET_VARIABLE_RESULT Array::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	}

	if(!name.hasEmptyNS)
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	}

	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	}

	if (getClass() && getClass()->isSealed)
	{
		createError<ReferenceError>(getInstanceWorker(),kReadSealedError,name.normalizedNameUnresolved(getSystemState()),getClass()->getQualifiedClassName());
		return GET_VARIABLE_RESULT::GETVAR_NORMAL;
	}
	
	if (index < ARRAY_SIZE_THRESHOLD)
	{
		if (data_first.size() > index)
		{
			ret = data_first.at(index);
			if (!(opt & NO_INCREF))
				ASATOM_INCREF(ret);
			if (asAtomHandler::isValid(ret))
				return GET_VARIABLE_RESULT::GETVAR_NORMAL;
		}
	}
	auto it = data_second.find(index);
	if(it != data_second.end())
	{
		ret = it->second;
		if (!(opt & NO_INCREF))
			ASATOM_INCREF(ret);
		return GET_VARIABLE_RESULT::GETVAR_NORMAL;
	}
	if (name.hasEmptyNS)
	{
		//Check prototype chain
		Prototype* proto = this->getClass()->prototype.getPtr();
		while(proto && proto->getObj() != this)
		{
			GET_VARIABLE_RESULT res = proto->getObj()->getVariableByMultiname(ret,name, opt,wrk);
			if(asAtomHandler::isValid(ret))
				return res;
			proto = proto->prevPrototype.getPtr();
		}
	}
	if(index<size())
		asAtomHandler::setUndefined(ret);
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}

GET_VARIABLE_RESULT Array::getVariableByInteger(asAtom &ret, int index, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	if (getClass() && getClass()->isSealed)
	{
		tiny_string s = Integer::toString(index);
		createError<ReferenceError>(getInstanceWorker(),kReadSealedError,s,getClass()->getQualifiedClassName());
		return GET_VARIABLE_RESULT::GETVAR_NORMAL;
	}
	if (index >=0 && uint32_t(index) < size())
	{
		if (index < ARRAY_SIZE_THRESHOLD)
		{
			if (data_first.size() > uint32_t(index))
			{
				ret = data_first.at(index);
				if (!(opt & NO_INCREF))
					ASATOM_INCREF(ret);
				if (asAtomHandler::isValid(ret))
					return GET_VARIABLE_RESULT::GETVAR_NORMAL;
			}
		}
		auto it = data_second.find(index);
		if(it != data_second.end())
		{
			ret = it->second;
			if (!(opt & NO_INCREF))
				ASATOM_INCREF(ret);
			return GET_VARIABLE_RESULT::GETVAR_NORMAL;
		}
		//Check prototype chain
		multiname m(nullptr);
		m.name_type = multiname::NAME_INT;
		m.name_i = index;
		Prototype* proto = this->getClass()->prototype.getPtr();
		while(proto && proto->getObj() != this)
		{
			GET_VARIABLE_RESULT res = proto->getObj()->getVariableByMultiname(ret,m, opt,wrk);
			if(asAtomHandler::isValid(ret))
				return res;
			proto = proto->prevPrototype.getPtr();
		}
		asAtomHandler::setUndefined(ret);
		return GET_VARIABLE_RESULT::GETVAR_NORMAL;
	}
	else
		return getVariableByIntegerIntern(ret,index,opt,wrk);
}

void Array::setVariableByMultiname_i(multiname& name, int32_t value, ASWorker* wrk)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(getSystemState(),name,index))
	{
		ASObject::setVariableByMultiname_i(name,value,wrk);
		return;
	}
	if(index>=size())
		resize(index+1);
	asAtom v = asAtomHandler::fromInt(value);
	set(index,v,false);
}


bool Array::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
	if (!isConstructed())
		return false;

	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);

	// Derived classes may be sealed!
	if (getClass() && getClass()->isSealed)
		return false;
	if (index < ARRAY_SIZE_THRESHOLD)
	{
		return data_first.size() > index ? (asAtomHandler::isValid(data_first.at(index))) : false;
	}
	
	return (data_second.find(index) != data_second.end());
}

bool Array::isValidMultiname(SystemState* sys, const multiname& name, uint32_t& index)
{
	switch (name.name_type)
	{
		case multiname::NAME_UINT:
			index = name.name_ui;
			return true;
		case multiname::NAME_INT:
			if (name.name_i >= 0)
			{
				index = name.name_i;
				return true;
			}
			break;
		case multiname::NAME_STRING:
			if (name.name_s_id >= '0' && name.name_s_id <= '9')
			{
				index = name.name_s_id - '0';
				return true;
			}
			if (name.name_s_id < BUILTIN_STRINGS::LAST_BUILTIN_STRING ||
					 !name.isInteger)
				return false;
			break;
		default:
			break;
	}
	if(name.isEmpty())
		return false;
	if(!name.hasEmptyNS)
		return false;
	
	return name.toUInt(sys,index);
}

bool Array::isIntegerWithoutLeadingZeros(const tiny_string& value)
{
	if (value.empty())
		return false;
	else if (value == "0")
		return true;

	bool first = true;
	for (CharIterator it=value.begin(); it!=value.end(); ++it)
	{
		if (!it.isdigit() || (first && *it == '0'))
			return false;

		first = false;
	}
	
	return true;
}

multiname *Array::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	assert_and_throw(implEnable);
	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
	// Derived classes may be sealed!
	if (getClass() && getClass()->isSealed)
	{
		createError<ReferenceError>(getInstanceWorker(),kWriteSealedError,
					   name.normalizedNameUnresolved(getSystemState()),
					   getClass()->getQualifiedClassName());
		return nullptr;
	}
	if(index>=size())
		resize((uint64_t)index+1);

	if (!set(index, o,false,false))
	{
		if (alreadyset)
			*alreadyset=true;
	}
	return nullptr;
}

void Array::setVariableByInteger(int index, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	if (index < 0)
	{
		setVariableByInteger_intern(index,o,allowConst, alreadyset,wrk);
		return;
	}
	// Derived classes may be sealed!
	if (getClass() && getClass()->isSealed)
	{
		multiname m(nullptr);
		m.name_type = multiname::NAME_INT;
		m.name_i = index;
		createError<ReferenceError>(getInstanceWorker(),kWriteSealedError,
					   m.normalizedNameUnresolved(getSystemState()),
					   getClass()->getQualifiedClassName());
		return;
	}
	if(uint64_t(index)>=size())
		resize((uint64_t)index+1);
	*alreadyset = !set(index, o,false,false);
}

bool Array::deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::deleteVariableByMultiname(name,wrk);
	if (getClass() && getClass()->isSealed)
		return false;

	if(index>=size())
		return true;
	if (index < data_first.size())
	{
		ASObject* obj = asAtomHandler::getObject(data_first.at(index));
		if (obj)
			obj->removeStoredMember();
		data_first[index]=asAtomHandler::invalidAtom;
		return true;
	}
	
	auto it = data_second.find(index);
	if(it == data_second.end())
		return true;
	ASObject* obj = asAtomHandler::getObject(it->second);
	if (obj)
		obj->removeStoredMember();
	data_second.erase(it);
	return true;
}

bool Array::isValidQName(const tiny_string& name, const tiny_string& ns, unsigned int& index)
{
	if(ns!="")
		return false;
	assert_and_throw(!name.empty());
	index=0;
	//First we try to convert the string name to an index, at the first non-digit
	//we bail out
	for(auto i=name.begin(); i!=name.end(); ++i)
	{
		if(!i.isdigit())
			return false;

		index*=10;
		index+=i.digit_value();
	}
	return true;
}

tiny_string Array::toString()
{
	assert_and_throw(implEnable);
	return toString_priv();
}

tiny_string Array::toString_priv(bool localized)
{
	string ret;
	for(uint32_t i=0;i<size();i++)
	{
		asAtom sl=asAtomHandler::invalidAtom;
		if (i < ARRAY_SIZE_THRESHOLD)
		{
			if (i < data_first.size())
				sl = data_first[i];
		}
		else
		{
			auto it = data_second.find(i);
			if(it != data_second.end())
				sl = it->second;
		}
		if(asAtomHandler::isValid(sl) && !asAtomHandler::isNull(sl) && !asAtomHandler::isUndefined(sl))
		{
			if (localized)
				ret += asAtomHandler::toLocaleString(sl,getInstanceWorker()).raw_buf();
			else
				ret += asAtomHandler::toString(sl,getInstanceWorker()).raw_buf();
		}
		if(i!=size()-1)
			ret+=',';
	}
	return ret;
}

void Array::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=size())
	{
		--index;
		if (index < ARRAY_SIZE_THRESHOLD)
			ret = data_first.at(index);
		else
		{
			auto it = data_second.find(index);
			if(it == data_second.end() || it->first != index)
				asAtomHandler::setUndefined(ret);
			else
				ret = it->second;
		}
		if(asAtomHandler::isInvalid(ret))
			asAtomHandler::setUndefined(ret);
		else
		{
			ASATOM_INCREF(ret);
		}
	}
	else
	{
		//Fall back on object properties
		ASObject::nextValue(ret,index-size());
	}
}

uint32_t Array::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	uint32_t s = size();
	if(cur_index<s)
	{
		uint32_t firstsize = data_first.size();
		while (cur_index < ARRAY_SIZE_THRESHOLD && cur_index<s && cur_index < firstsize && asAtomHandler::isInvalid(data_first.at(cur_index)))
		{
			cur_index++;
		}
		if(cur_index<firstsize)
			return cur_index+1;
		
		while (!data_second.count(cur_index) && cur_index<s)
		{
			cur_index++;
		}
		if(cur_index<s)
			return cur_index+1;
	}
	//Fall back on object properties
	uint32_t ret=ASObject::nextNameIndex(cur_index-s);
	if(ret==0)
		return 0;
	else
		return ret+s;
	
}

void Array::nextName(asAtom& ret, uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=size())
		asAtomHandler::setUInt(ret,this->getInstanceWorker(),index-1);
	else
	{
		//Fall back on object properties
		ASObject::nextName(ret,index-size());
	}
}

asAtom Array::at(unsigned int index)
{
	if(size()<=index)
		outofbounds(index);
	
	asAtom ret=asAtomHandler::invalidAtom;
	if (index < ARRAY_SIZE_THRESHOLD)
	{
		if (index < data_first.size())
			ret = data_first.at(index);
	}
	else
	{
		auto it = data_second.find(index);
		if (it != data_second.end())
			ret = it->second;
	}
	if(asAtomHandler::isValid(ret))
	{
		return ret;
	}
	return asAtomHandler::undefinedAtom;
}

void Array::outofbounds(unsigned int index) const
{
	createError<RangeError>(getInstanceWorker(),kInvalidArrayLengthError, Number::toString(index));
}
bool Array::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	for (auto it = data_first.begin(); it != data_first.end(); it++)
	{
		if (asAtomHandler::isObject(*it))
			ret = asAtomHandler::getObjectNoCheck(*it)->countAllCylicMemberReferences(gcstate) || ret;
	}
	for (auto it = data_second.begin(); it != data_second.end(); it++)
	{
		if (asAtomHandler::isObject(it->second))
			ret = asAtomHandler::getObjectNoCheck(it->second)->countAllCylicMemberReferences(gcstate) || ret;
	}
	return ret;
}

void Array::resize(uint64_t n,bool removemember)
{
	if (n < currentsize)
	{
		if (n < data_first.size())
		{
			if (removemember)
			{
				auto it1 = data_first.begin()+n;
				while (it1 != data_first.end())
				{
					ASObject* o = asAtomHandler::getObject(*it1);
					it1++;
					if (o)
						o->removeStoredMember();
				}
			}
			data_first.resize(n);
		}
		auto it2=data_second.begin();
		while (it2 != data_second.end())
		{
			if (it2->first >= n)
			{
				auto it2a = it2;
				++it2;
				ASObject* o = asAtomHandler::getObject(it2a->second);
				data_second.erase(it2a);
				if (removemember && o)
					o->removeStoredMember();
			}
			else
				++it2;
		}
	}
	currentsize = n;
}

void Array::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing Array in AMF0 not implemented");
		return;
	}
	out->writeByte(array_marker);
	//Check if the array has been already serialized
	auto it=objMap.find(this);
	if(it!=objMap.end())
	{
		//The least significant bit is 0 to signal a reference
		out->writeU29(it->second << 1);
	}
	else
	{
		//Add the array to the map
		objMap.insert(make_pair(this, objMap.size()));

		uint32_t denseCount = currentsize;
		assert_and_throw(denseCount<0x20000000);
		uint32_t value = (denseCount << 1) | 1;
		out->writeU29(value);
		serializeDynamicProperties(out, stringMap, objMap, traitsMap,wrk);
		for(uint32_t i=0;i<denseCount;i++)
		{
			if (i < ARRAY_SIZE_THRESHOLD)
			{
				if (asAtomHandler::isInvalid(data_first.at(i)))
					out->writeByte(null_marker);
				else
					asAtomHandler::serialize(out,stringMap, objMap, traitsMap,wrk,data_first.at(i));
			}
			else
			{
				if (data_second.find(i) == data_second.end())
					out->writeByte(null_marker);
				else
					asAtomHandler::serialize(out,stringMap, objMap, traitsMap,wrk,data_second.at(i));
			}
		}
	}
}

tiny_string Array::toJSON(std::vector<ASObject *> &path, asAtom replacer, const tiny_string& spaces,const tiny_string& filter)
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
	uint32_t denseCount = currentsize;
	asAtom closure = asAtomHandler::isValid(replacer) && asAtomHandler::getClosure(replacer) ? asAtomHandler::fromObject(asAtomHandler::getClosure(replacer)) : asAtomHandler::nullAtom;
	
	for (uint32_t i=0 ; i < denseCount; i++)
	{
		asAtom a=asAtomHandler::invalidAtom;
		if ( i < ARRAY_SIZE_THRESHOLD)
			a = data_first.at(i);
		else
		{
			auto it = data_second.find(i);
			if (it != data_second.end())
				a = it->second;
		}
		tiny_string subres;
		if (asAtomHandler::isValid(replacer) && asAtomHandler::isValid(a))
		{
			asAtom params[2];
			
			params[0] = asAtomHandler::fromUInt(i);
			params[1] = a;
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
			asAtom tmp = a;
			bool newobj = !asAtomHandler::isInvalid(a) && !asAtomHandler::isObject(a); // member is not a pointer to an ASObject, so toObject() will create a temporary ASObject that has to be decreffed after usage
			ASObject* o = asAtomHandler::isInvalid(a) ? getSystemState()->getNullRef() : asAtomHandler::toObject(tmp,getInstanceWorker());
			if (o)
			{
				subres = o->toJSON(path,replacer,spaces,filter);
				if (newobj)
					o->decRef();
			}
			else
				continue;
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

Array::~Array()
{
}

bool Array::set(unsigned int index, asAtom& o, bool checkbounds, bool addref, bool addmember)
{
	bool ret = true;
	if(index<currentsize)
	{
		if (index < ARRAY_SIZE_THRESHOLD)
		{
			if (index < data_first.size())
			{
				if (data_first.at(index).uintval == o.uintval)
					ret = false;
				else
				{
					asAtom oldvar = data_first.at(index);
					ASObject* obj = asAtomHandler::getObject(oldvar);
					if (obj)
						obj->removeStoredMember();
				}
			}
			else
				data_first.resize(index+1);
			if (ret)
			{
				ASObject* obj = asAtomHandler::getObject(o);
				if (obj)
				{
					if (addref)
						obj->incRef();
					if (addmember)
						obj->addStoredMember();
				}
			}
			data_first[index]=o;
		}
		else
		{
			if(data_second.find(index) != data_second.end())
			{
				if (data_second[index].uintval == o.uintval)
					ret = false;
				else
				{
					asAtom oldvar = data_second[index];
					ASObject* obj = asAtomHandler::getObject(oldvar);
					if (obj)
						obj->removeStoredMember();
				}
			}
			if (ret)
			{
				ASObject* obj = asAtomHandler::getObject(o);
				if (obj)
				{
					if (addref)
						obj->incRef();
					if (addmember)
						obj->addStoredMember();
				}
			}
			data_second[index]=o;
		}
	}
	else if (checkbounds)
		outofbounds(index);
	return ret;
}

uint64_t Array::size()
{
	if (this->getClass() && this->getClass()->is<Class_inherit>())
	{
		multiname lengthName(nullptr);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=BUILTIN_STRINGS::STRING_LENGTH;
		lengthName.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = false;
		if (hasPropertyByMultiname(lengthName, true, true,getInstanceWorker()))
		{
			asAtom o=asAtomHandler::invalidAtom;
			getVariableByMultiname(o,lengthName,SKIP_IMPL,getInstanceWorker());
			return asAtomHandler::toUInt(o);
		}
	}
	return currentsize;
}

void Array::push(asAtom o)
{
	if (currentsize == UINT32_MAX)
		return;
	// Derived classes may be sealed!
	if (getSystemState()->getSwfVersion() > 12 && getClass() && getClass()->isSealed)
	{
		createError<ReferenceError>(getInstanceWorker(),kWriteSealedError,"push",getClass()->getQualifiedClassName());
		return;
	}
	currentsize++;
	set(currentsize-1,o,false,false);
}

