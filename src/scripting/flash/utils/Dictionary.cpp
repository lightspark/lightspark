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

#include "scripting/abc.h"
#include "asobject.h"
#include "scripting/class.h"
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/Dictionary.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Number.h"

using namespace std;
using namespace lightspark;

Dictionary::Dictionary(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_DICTIONARY),
	weakkeys(false)
{
}

void Dictionary::finalize()
{
	auto it=data.begin();
	while (it != data.end())
	{
		asAtom key = it->first.key;
		ASObject* obj = asAtomHandler::getObject(it->second);
		it = data.erase(it);
		ASATOM_REMOVESTOREDMEMBER(key);
		if (obj)
			obj->removeStoredMember();
	}
	enumeratedKeys.clear();
}

bool Dictionary::destruct()
{
	auto it=data.begin();
	while (it != data.end())
	{
		asAtom key = it->first.key;
		ASObject* obj = asAtomHandler::getObject(it->second);
		it = data.erase(it);
		ASATOM_REMOVESTOREDMEMBER(key);
		if (obj)
			obj->removeStoredMember();
	}
	enumeratedKeys.clear();
	weakkeys=false;
	return destructIntern();
}
void Dictionary::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	for (auto it=data.begin() ; it != data.end(); ++it)
	{
		ASObject* k = asAtomHandler::getObject(it->first.key);
		if (k)
			k->prepareShutdown();
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			o->prepareShutdown();
	}
}

void Dictionary::sinit(Class_base* c)
{
	CLASS_SETUP_CONSTRUCTOR_1_PARAMETER(c, ASObject, _constructor, 1, CLASS_GETREF(c,Boolean),CLASS_DYNAMIC_NOT_FINAL);
	c->length=-1; // Dictionary class has no length constant

	c->isReusable=true;

	c->prototype->setVariableByQName("toJSON",AS3,c->getSystemState()->getBuiltinFunction(_toJSON),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Dictionary,_constructor)
{
	Dictionary* th=asAtomHandler::as<Dictionary>(obj);
	ARG_CHECK(ARG_UNPACK(th->weakkeys, false));
}

ASFUNCTIONBODY_ATOM(Dictionary,_toJSON)
{
	ret = asAtomHandler::fromString(wrk->getSystemState(),"Dictionary");
}

Dictionary::dictType::iterator Dictionary::findKey(asAtom o)
{
	asAtomStrict a;
	a.key=o;
	a.wrk=getInstanceWorker();
	return data.find(a);
}

void Dictionary::setVariableByMultiname_i(multiname& name, int32_t value,ASWorker* wrk)
{
	assert_and_throw(implEnable);
	asAtom v = asAtomHandler::fromInt(value);
	Dictionary::setVariableByMultiname(name,v,CONST_NOT_ALLOWED,nullptr,wrk);
}

void Dictionary::getKeyFromMultiname(const multiname& name,asAtom& key)
{
	switch (name.name_type)
	{
		case multiname::NAME_STRING:
			if (name.isInteger)
			{
				number_t  ret;
				Integer::fromStringFlashCompatible(getSystemState()->getStringFromUniqueId(name.name_s_id).raw_buf(),ret,10);
				if (ret >= 0)
				{
					key = asAtomHandler::fromInt(ret);
					break;
				}
			}
			key = asAtomHandler::fromStringID(name.name_s_id);
			break;
		case multiname::NAME_INT:
			if (name.name_i >= 0)
				key = asAtomHandler::fromInt(name.name_i);
			else
			{
				uint32_t id = getSystemState()->getUniqueStringId(Integer::toString(name.name_i));
				key = asAtomHandler::fromStringID(id);
			}
			break;
		case multiname::NAME_UINT:
			key = asAtomHandler::fromUInt(name.name_ui);
			break;
		case multiname::NAME_NUMBER:
		{
			uint32_t id = getSystemState()->getUniqueStringId(Number::toString(name.name_d));
			key = asAtomHandler::fromStringID(id);
			break;
		}
		case multiname::NAME_OBJECT:
		{
			if (asAtomHandler::isUndefined(name.name_o))
				key = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_UNDEFINED);
			else if (asAtomHandler::isNull(name.name_o))
				key = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_NULL);
			else if (name.name_o.uintval == asAtomHandler::trueAtom.uintval)
				key = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_TRUE);
			else if (name.name_o.uintval == asAtomHandler::falseAtom.uintval)
				key = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_FALSE);
			else
				key = name.name_o;
			break;
		}
	}
}
multiname *Dictionary::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	asAtomHandler::localNumberToGlobalNumber(getInstanceWorker(),o);
	assert_and_throw(implEnable);
	asAtom key=asAtomHandler::invalidAtom;
	getKeyFromMultiname(name,key);
	dictType::iterator it=findKey(key);
	if(it!=data.end())
	{
		if (alreadyset && it->second.uintval == o.uintval)
			*alreadyset=true;
		else
		{
			asAtom oldvar = it->second;
			ASObject* obj = asAtomHandler::getObject(oldvar);
			if (obj)
				obj->removeStoredMember();
			it->second=o;
			obj = asAtomHandler::getObject(o);
			if (obj)
				obj->addStoredMember();
		}
	}
	else
	{
		// Derived classes may be sealed!
		if (name.name_type==multiname::NAME_STRING && getClass() && getClass()->isSealed )
		{
			createError<ReferenceError>(getInstanceWorker(),kWriteSealedError,
										name.normalizedNameUnresolved(getSystemState()),
										getClass()->getQualifiedClassName());
			return nullptr;
		}

		ASATOM_ADDSTOREDMEMBER(key);
		ASObject* obj = asAtomHandler::getObject(o);
		if (obj)
			obj->addStoredMember();
		data.insert(make_pair(asAtomStrict { key, getInstanceWorker() },o));
		enumeratedKeys.push_back(key);
	}
	return nullptr;
}

bool Dictionary::deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
{
	assert_and_throw(implEnable);
	asAtom key=asAtomHandler::invalidAtom;
	getKeyFromMultiname(name,key);

	Dictionary::dictType::iterator it=findKey(key);
	if(it != data.end())
	{
		ASObject* obj = asAtomHandler::getObject(it->second);
		if (obj)
			obj->removeStoredMember();
		for (auto iten = enumeratedKeys.begin(); iten != enumeratedKeys.end(); iten++)
		{
			if ((*iten).uintval == it->first.key.uintval)
			{
				// don't erase, as we might be inside a "hasnext" loop and have to keep the index valid
				*iten = asAtomHandler::invalidAtom;
				break;
			}
		}
		ASATOM_REMOVESTOREDMEMBER(it->first.key);
		data.erase(it);
	}
	else if (name.name_type==multiname::NAME_STRING && getClass() && getClass()->isSealed)
		return false;
	return true;
}
GET_VARIABLE_RESULT Dictionary::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt,ASWorker* wrk)
{
	if((opt & GET_VARIABLE_OPTION::SKIP_IMPL)==0 && implEnable)
	{
		asAtom key=asAtomHandler::invalidAtom;
		getKeyFromMultiname(name,key);
		bool islastref = name.name_type==multiname::NAME_OBJECT && asAtomHandler::isObject(name.name_o) && asAtomHandler::getObjectNoCheck(name.name_o)->isLastRef();
		Dictionary::dictType::iterator it=findKey(key);
		if(it != data.end())
		{
			ret = it->second;
			if (islastref && weakkeys)
			{
				ASObject* o = asAtomHandler::getObject(ret);
				if (o)
				{
					o->incRef();
					o->removeStoredMember();
				}
				data.erase(it);
				ASATOM_REMOVESTOREDMEMBER(name.name_o);
			}
			else
				ASATOM_INCREF(ret);
			return GET_VARIABLE_RESULT::GETVAR_NORMAL;
		}
		if (asAtomHandler::isPrimitive(key))
		{
			// primitive key, fallback to base implementation
			return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
		}
		else
		{
			ret = asAtomHandler::undefinedAtom;
			return GET_VARIABLE_RESULT::GETVAR_NORMAL;
		}
	}
	//Try with the base implementation
	return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
}

asAtomWithNumber Dictionary::getAtomWithNumberByMultiname(const multiname& name, ASWorker* wrk, GET_VARIABLE_OPTION opt)
{
	asAtom key=asAtomHandler::invalidAtom;
	getKeyFromMultiname(name,key);
	Dictionary::dictType::iterator it=findKey(key);
	if(it != data.end())
	{
		asAtomWithNumber res;
		res.value = it->second;
		return res;
	}
	if (asAtomHandler::isPrimitive(key))
	{
		// primitive key, fallback to base implementation
		return ASObject::getAtomWithNumberByMultiname(name,wrk,opt);
	}
	else
	{
		asAtomWithNumber res;
		res.value = asAtomHandler::undefinedAtom;
		return res;
	}
}

bool Dictionary::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype,ASWorker* wrk)
{
	if(considerDynamic==false)
	{
		if(name.name_type==multiname::NAME_OBJECT)
			return false;
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
	}
	if (!isConstructed())
		return false;

	asAtom key=asAtomHandler::invalidAtom;
	getKeyFromMultiname(name,key);
	Dictionary::dictType::iterator it=findKey(key);
	return it != data.end();
}

uint32_t Dictionary::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	while (cur_index < enumeratedKeys.size() && asAtomHandler::isInvalid(enumeratedKeys[cur_index]))
		cur_index++;
	if (cur_index >= enumeratedKeys.size())
		return 0;
	return cur_index+1;
}

void Dictionary::nextName(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	if (index > enumeratedKeys.size())
		ret = asAtomHandler::undefinedAtom;
	else
	{
		ret = enumeratedKeys[index-1];
		ASATOM_INCREF(ret);
	}
}

void Dictionary::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	if (index > enumeratedKeys.size())
		ret = asAtomHandler::undefinedAtom;
	else
	{
		asAtom key = enumeratedKeys[index-1];
		ret = findKey(key)->second;
		ASATOM_INCREF(ret);
	}
}

bool Dictionary::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	for (auto it = data.begin(); it != data.end(); it++)
	{
		if (asAtomHandler::isObject(it->first.key))
			ret = asAtomHandler::getObjectNoCheck(it->first.key)->countAllCylicMemberReferences(gcstate) || ret;
		if (asAtomHandler::isObject(it->second))
			ret = asAtomHandler::getObjectNoCheck(it->second)->countAllCylicMemberReferences(gcstate) || ret;
	}
	return ret;
}

tiny_string Dictionary::toString()
{
	std::stringstream retstr;
	retstr << "{";
	auto it=data.begin();
	while(it != data.end())
	{
		if(it != data.begin())
			retstr << ", ";
		retstr << "{" << asAtomHandler::toString(it->first.key,getInstanceWorker()) << ", " << asAtomHandler::toString(it->second,getInstanceWorker()) << "}";
		++it;
	}
	retstr << "}";

	return retstr.str();
}


void Dictionary::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing Dictionary in AMF0 not implemented");
		return;
	}
	assert_and_throw(objMap.find(this)==objMap.end());
	out->writeByte(dictionary_marker);
	//Check if the dictionary has been already serialized
	auto it=objMap.find(this);
	if(it!=objMap.end())
	{
		//The least significant bit is 0 to signal a reference
		out->writeU29(it->second << 1);
	}
	else
	{
		//Add the dictionary to the map
		objMap.insert(make_pair(this, objMap.size()));

		uint32_t count = 0;
		uint32_t tmp;
		while ((tmp = nextNameIndex(count)) != 0)
		{
			count = tmp;
		}
		assert_and_throw(count<0x20000000);
		uint32_t value = (count << 1) | 1;
		out->writeU29(value);
		out->writeByte(0x00); // TODO handle weak keys
		
		tmp = 0;
		while ((tmp = nextNameIndex(tmp)) != 0)
		{
			asAtom v=asAtomHandler::invalidAtom;
			nextName(v,tmp);
			asAtomHandler::serialize(out, stringMap, objMap, traitsMap,wrk,v);
			nextValue(v,tmp);
			asAtomHandler::serialize(out, stringMap, objMap, traitsMap,wrk,v);
		}
	}
}

