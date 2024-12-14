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

#include "avm1array.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Integer.h"

using namespace std;
using namespace lightspark;

AVM1Array::AVM1Array(ASWorker* wrk, Class_base* c):Array(wrk,c),avm1_currentsize(0)
{
	resetEnumerator();
}

void AVM1Array::sinit(Class_base* c)
{
	Array::sinit(c);
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(AVM1_getLength,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(AVM1_setLength),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("pop","",c->getSystemState()->getBuiltinFunction(AVM1_pop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("push","",c->getSystemState()->getBuiltinFunction(AVM1_push,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("shift","",c->getSystemState()->getBuiltinFunction(AVM1_shift,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unshift","",c->getSystemState()->getBuiltinFunction(AVM1_unshift,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("splice","",c->getSystemState()->getBuiltinFunction(AVM1_splice,1),NORMAL_METHOD,true);

	c->prototype->setVariableByQName("call","",c->getSystemState()->getBuiltinFunction(AVM1_call,1,Class<AVM1Array>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("apply","",c->getSystemState()->getBuiltinFunction(AVM1_apply,1,Class<AVM1Array>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
}

bool AVM1Array::destruct()
{
	avm1_currentsize=0;
	name_enumeration.clear();
	resetEnumerator();
	return Array::destruct();
}

void AVM1Array::setCurrentSize(int64_t size)
{
	avm1_currentsize=size;
}

multiname* AVM1Array::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	// AVM1 needs special detection of integer name for negative indexes
	int32_t index=0;
	bool valid = false;
	switch (name.name_type)
	{
		case multiname::NAME_UINT:
			index = name.name_ui;
			valid=true;
			break;
		case multiname::NAME_INT:
			index = name.name_i;
			valid=true;
			break;
		case multiname::NAME_NUMBER:
			if(!Number::isInteger(name.name_d))
				break;
			valid=true;;
			index = (int32_t)uint32_t(name.name_d);
			if (name.name_d> UINT32_MAX)
			{
				// special case name is number and > UINT32_MAX, it's not a valid index, but the AVM1 length is adjusted ??? (see ruffle test avm1/array_length
				valid=false;
				if (index >= (int64_t)size())
					resize(index+1);
			}
			break;
		default:
			{
				uint32_t idx=0;
				valid = name.toUInt(getSystemState(),idx,false,nullptr,true);
				if (valid)
					index = (int32_t)idx;
			}
			break;
	}
	if(valid && index>=this->avm1_currentsize)
	{
		uint32_t oldsize = size();
		if (index == INT32_MAX)
		{
			// corner case index INT32_MAX leads to integer overflow for size, so size is set to 0
			resize(0);
			shrinkEnumeration(oldsize);
		}
		else
		{
			resize((uint64_t)index+1);
		}
		setCurrentSize(index+1);
	}
	if (!valid || index < 0)
	{
		if (!ASObject::hasPropertyByMultiname(name,true,true,getInstanceWorker()))
		{
			uint32_t nameid = name.normalizedNameId(getSystemState());
			name_enumeration.push_back(make_pair(nameid,false));
		}
		return ASObject::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
	}
	if (index >=(int32_t)this->size() || !this->hasEntry(index) )
		name_enumeration.push_back(make_pair(index,true));
	if (!set(index, o,false,false))
	{
		if (alreadyset)
			*alreadyset=true;
	}
	return nullptr;
}

bool AVM1Array::deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
{
	unsigned int index=0;
	bool validindex = isValidMultiname(getSystemState(),name,index);
	validindex = validindex && index < size();
	for (auto it = name_enumeration.begin(); it != name_enumeration.end(); it++)
	{
		if (it->first == name.name_s_id && it->second == validindex)
		{
			name_enumeration.erase(it);
			resetEnumerator();
			break;
		}
	}
	return Array::deleteVariableByMultiname(name,wrk);
}

uint32_t AVM1Array::nextNameIndex(uint32_t cur_index)
{
	auto it=currentEnumerationIndex<=cur_index ? currentEnumerationIterator : name_enumeration.cbegin();
	unsigned int i=currentEnumerationIndex<=cur_index ? currentEnumerationIndex : 0;
	while (i < cur_index)
	{
		++i;
		++it;
		if (it == name_enumeration.cend())
			return 0;
	}
	currentEnumerationIndex=i;
	currentEnumerationIterator=it;
	return i+1;
}

void AVM1Array::nextName(asAtom& ret, uint32_t index)
{
	index--;
	auto it=currentEnumerationIndex<=index ? currentEnumerationIterator : name_enumeration.cbegin();
	uint32_t i = currentEnumerationIndex <= index ? currentEnumerationIndex : 0;
	while (i < index)
	{
		++i;
		++it;
		if (it==name_enumeration.cend())
		{
			LOG(LOG_ERROR,"invalid nameindex in AVM1Array.nextName");
			ret = asAtomHandler::undefinedAtom;
			return;
		}
	}
	currentEnumerationIndex=index;
	currentEnumerationIterator=it;

	if (currentEnumerationIterator->second)
		ret = asAtomHandler::fromUInt(currentEnumerationIterator->first);
	else
		ret = asAtomHandler::fromStringID(currentEnumerationIterator->first);
}

void AVM1Array::nextValue(asAtom& ret, uint32_t index)
{
	if (currentEnumerationIterator == name_enumeration.cend())
	{
		LOG(LOG_ERROR,"invalid enumerator position in AVM1Array::nextValue");
		ret = asAtomHandler::undefinedAtom;
		return;
	}
	if (currentEnumerationIterator->second)
		this->at_nocheck(ret,currentEnumerationIterator->first);
	else
	{
		multiname m(nullptr);
		m.name_type = multiname::NAME_STRING;
		m.name_s_id = currentEnumerationIterator->first;
		Array::getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
	}
}

void AVM1Array::resetEnumerator()
{
	currentEnumerationIndex=UINT32_MAX;
	currentEnumerationIterator = name_enumeration.end();
}

void AVM1Array::shrinkEnumeration(uint32_t oldsize)
{
	int deletecount = oldsize - size();
	auto it = name_enumeration.rbegin();
	while (it != name_enumeration.rend() && deletecount)
	{
		if (it->second)
		{
			it = decltype(it)(name_enumeration.erase(std::next(it).base()));
			deletecount--;
		}
		else
			++it;
	}
	resetEnumerator();
}

Array* AVM1Array::createInstance()
{
	return Class<AVM1Array>::getInstanceSNoArgs(getInstanceWorker());
}

// TODO: It appears that in AVM1, most of `Array`'s methods will treat a
// non-array `this` object with numeric properties as if it were an array.

ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_getLength)
{
	AVM1Array* th=asAtomHandler::as<AVM1Array>(obj);
	if (th->avm1_currentsize<0) // AVM1 allows negative length
		asAtomHandler::setInt(ret,wrk,th->avm1_currentsize);
	else
		asAtomHandler::setInt(ret,wrk,th->currentsize);
}

ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_setLength)
{
	int64_t newLen;
	ARG_CHECK(ARG_UNPACK(newLen));
	AVM1Array* th=asAtomHandler::as<AVM1Array>(obj);
	uint32_t oldsize = th->size();
	th->avm1_currentsize=newLen;
	if (newLen < 0)
		th->resize(0);
	else
		Array::_setLength(ret,wrk,obj,args,argslen);
	if (oldsize > th->size())
		th->shrinkEnumeration(oldsize);
}

ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_call)
{
	// implements Function.call as generator
	if (argslen < 1)
		return;
	AVM1Array* th=Class<AVM1Array>::getInstanceSNoArgs(wrk);
	th->constructorImpl(args+1,argslen-1); // ignore first argument, as it is the "this" object for the call
	ret = asAtomHandler::fromObjectNoPrimitive(th);
}
ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_apply)
{
	// implements Function.apply as generator
	if (argslen < 1)
		return;
	if (argslen > 1 && !asAtomHandler::isArray(args[1]))
		return;
	AVM1Array* th=Class<AVM1Array>::getInstanceSNoArgs(wrk);

	Array* argarray = asAtomHandler::as<Array>(args[1]);
	th->resize(argarray->size());
	for(unsigned int i=1;i<argarray->size();i++)
	{
		asAtom a= argarray->at(i);
		th->set(i,a,false);
	}
	ret = asAtomHandler::fromObjectNoPrimitive(th);
}

ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_pop)
{
	Array::_pop(ret,wrk,obj,args,argslen);
	AVM1Array* th=asAtomHandler::as<AVM1Array>(obj);
	auto it = th->name_enumeration.rbegin();
	while (it != th->name_enumeration.rend())
	{
		if (it->second)
		{
			th->name_enumeration.erase(std::next(it).base());
			th->resetEnumerator();
			break;
		}
		it++;
	}
}
ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_push)
{
	AVM1Array* th=asAtomHandler::as<AVM1Array>(obj);
	uint32_t size = th->size();
	Array::_push(ret,wrk,obj,args,argslen);
	for (uint32_t i = size; i < th->size(); i++)
	{
		th->name_enumeration.push_back(make_pair(i,true));
	}
}
ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_shift)
{
	AVM1Array* th=asAtomHandler::as<AVM1Array>(obj);
	uint32_t size = th->size();
	Array::shift(ret,wrk,obj,args,argslen);
	if (size)
	{
		auto it = th->name_enumeration.begin();
		while (it != th->name_enumeration.end())
		{
			if (it->second)
			{
				th->name_enumeration.erase(it);
				th->resetEnumerator();
				break;
			}
			it++;
		}

	}
}
ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_unshift)
{
	AVM1Array* th=asAtomHandler::as<AVM1Array>(obj);
	uint32_t size = th->size();
	Array::unshift(ret,wrk,obj,args,argslen);
	if (size < th->size())
	{
		th->name_enumeration.push_back(make_pair(size,true));
	}
}
ASFUNCTIONBODY_ATOM(AVM1Array,AVM1_splice)
{
	AVM1Array* th=asAtomHandler::as<AVM1Array>(obj);
	uint32_t oldsize = th->size();

	Array::splice(ret,wrk,obj,args,argslen);

	if (oldsize > th->size())
	{
		th->shrinkEnumeration(oldsize);
	}
	else
	{
		// Array has grown
		for (uint32_t i = oldsize; i < th->size(); i++)
			th->name_enumeration.push_back(make_pair(i,true));
	}
}
