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

using namespace std;
using namespace lightspark;

Dictionary::Dictionary(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_DICTIONARY),
	data(std::less<dictType::key_type>(), reporter_allocator<dictType::value_type>(c->memoryAccount)),weakkeys(false)
{
}

void Dictionary::finalize()
{
	auto it=data.begin();
	while (it != data.end())
	{
		ASObject* key = it->first;
		ASObject* obj = asAtomHandler::getObject(it->second);
		it = data.erase(it);
		key->removeStoredMember();
		if (obj)
			obj->removeStoredMember();
	}
}

bool Dictionary::destruct()
{
	auto it=data.begin();
	while (it != data.end())
	{
		ASObject* key = it->first;
		ASObject* obj = asAtomHandler::getObject(it->second);
		it = data.erase(it);
		key->removeStoredMember();
		if (obj)
			obj->removeStoredMember();
	}
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
		it->first->prepareShutdown();
		ASObject* o = asAtomHandler::getObject(it->second);
		if (o)
			o->prepareShutdown();
	}
}

void Dictionary::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
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

Dictionary::dictType::iterator Dictionary::findKey(ASObject *o)
{
	Dictionary::dictType::iterator it = data.begin();
	for(; it!=data.end(); ++it)
	{
		if (it->first->isEqualStrict(o))
			return it;
	}

	return data.end();
}

void Dictionary::setVariableByMultiname_i(multiname& name, int32_t value,ASWorker* wrk)
{
	assert_and_throw(implEnable);
	asAtom v = asAtomHandler::fromInt(value);
	Dictionary::setVariableByMultiname(name,v,CONST_NOT_ALLOWED,nullptr,wrk);
}

multiname *Dictionary::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	assert_and_throw(implEnable);
	if(name.name_type==multiname::NAME_OBJECT)
	{
		multiname tmpname(nullptr);
		tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		switch (name.name_o->getObjectType())
		{
			case T_BOOLEAN:
			case T_INTEGER:
				tmpname.name_type=multiname::NAME_INT;
				tmpname.name_i = name.name_o->toInt();
				return ASObject::setVariableByMultiname(tmpname, o, allowConst,alreadyset,wrk);
			case T_UINTEGER:
				tmpname.name_type=multiname::NAME_UINT;
				tmpname.name_ui = name.name_o->toUInt();
				return ASObject::setVariableByMultiname(tmpname, o, allowConst,alreadyset,wrk);
			case T_NUMBER:
				tmpname.name_type=multiname::NAME_NUMBER;
				tmpname.name_d = name.name_o->toNumber();
				return ASObject::setVariableByMultiname(tmpname, o, allowConst,alreadyset,wrk);
			case T_STRING:
				tmpname.name_type=multiname::NAME_STRING;
				tmpname.name_s_id = name.name_o->toStringId();
				return ASObject::setVariableByMultiname(tmpname, o, allowConst,alreadyset,wrk);
			default:
				break;
		}

		Dictionary::dictType::iterator it=findKey(name.name_o);
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
			name.name_o->incRef();
			name.name_o->addStoredMember();
			ASObject* obj = asAtomHandler::getObject(o);
			if (obj)
				obj->addStoredMember();
			data.insert(make_pair(name.name_o,o));
		}
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_UINT ||
			name.name_type==multiname::NAME_NUMBER);
		return ASObject::setVariableByMultiname(name, o, allowConst,alreadyset,wrk);
	}
	return nullptr;
}

bool Dictionary::deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
{
	assert_and_throw(implEnable);

	if(name.name_type==multiname::NAME_OBJECT)
	{
		multiname tmpname(nullptr);
		tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		switch (name.name_o->getObjectType())
		{
			case T_BOOLEAN:
			case T_INTEGER:
				tmpname.name_type=multiname::NAME_INT;
				tmpname.name_i = name.name_o->toInt();
				return ASObject::deleteVariableByMultiname(tmpname,wrk);
			case T_UINTEGER:
				tmpname.name_type=multiname::NAME_UINT;
				tmpname.name_ui = name.name_o->toUInt();
				return ASObject::deleteVariableByMultiname(tmpname,wrk);
			case T_NUMBER:
				tmpname.name_type=multiname::NAME_NUMBER;
				tmpname.name_d = name.name_o->toNumber();
				return ASObject::deleteVariableByMultiname(tmpname,wrk);
			case T_STRING:
				tmpname.name_type=multiname::NAME_STRING;
				tmpname.name_s_id = name.name_o->toStringId();
				return ASObject::deleteVariableByMultiname(tmpname,wrk);
			default:
				break;
		}

		Dictionary::dictType::iterator it=findKey(name.name_o);
		if(it != data.end())
		{
			ASObject* obj = asAtomHandler::getObject(it->second);
			if (obj)
				obj->removeStoredMember();
			data.erase(it);
			name.name_o->removeStoredMember();
			return true;
		}
		return false;
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_UINT ||
			name.name_type==multiname::NAME_NUMBER);
		return ASObject::deleteVariableByMultiname(name,wrk);
	}
}
GET_VARIABLE_RESULT Dictionary::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt,ASWorker* wrk)
{
	if((opt & GET_VARIABLE_OPTION::SKIP_IMPL)==0 && implEnable)
	{
		if(name.name_type==multiname::NAME_OBJECT)
		{
			multiname tmpname(nullptr);
			tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
			switch (name.name_o->getObjectType())
			{
				case T_BOOLEAN:
				case T_INTEGER:
					tmpname.name_type=multiname::NAME_INT;
					tmpname.name_i = name.name_o->toInt();
					return getVariableByMultinameIntern(ret,tmpname,this->getClass(),opt,wrk);
				case T_UINTEGER:
					tmpname.name_type=multiname::NAME_UINT;
					tmpname.name_ui = name.name_o->toUInt();
					return getVariableByMultinameIntern(ret,tmpname,this->getClass(),opt,wrk);
				case T_NUMBER:
					tmpname.name_type=multiname::NAME_NUMBER;
					tmpname.name_d = name.name_o->toNumber();
					return getVariableByMultinameIntern(ret,tmpname,this->getClass(),opt,wrk);
				case T_STRING:
					tmpname.name_type=multiname::NAME_STRING;
					tmpname.name_s_id = name.name_o->toStringId();
					return getVariableByMultinameIntern(ret,tmpname,this->getClass(),opt,wrk);
				default:
					break;
			}
			bool islastref = name.name_o->isLastRef();

			Dictionary::dictType::iterator it=findKey(name.name_o);
			if(it != data.end())
			{
				ret = it->second;
				if (islastref && weakkeys)
				{
					name.name_o->removeStoredMember();
					data.erase(it);
				}
				else
					ASATOM_INCREF(ret);
				return GET_VARIABLE_RESULT::GETVAR_NORMAL;
			}
			else
				return GET_VARIABLE_RESULT::GETVAR_NORMAL;
		}
		else
		{
			//Primitive types _must_ be handled by the normal ASObject path
			//REFERENCE: Dictionary Object on AS3 reference
			assert(name.name_type==multiname::NAME_STRING ||
				name.name_type==multiname::NAME_INT ||
				name.name_type==multiname::NAME_UINT ||
				name.name_type==multiname::NAME_NUMBER);
			return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
		}
	}
	//Try with the base implementation
	return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
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

	if(name.name_type==multiname::NAME_OBJECT)
	{
		multiname tmpname(nullptr);
		tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		switch (name.name_o->getObjectType())
		{
			case T_BOOLEAN:
			case T_INTEGER:
				tmpname.name_type=multiname::NAME_INT;
				tmpname.name_i = name.name_o->toInt();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype,wrk);
			case T_UINTEGER:
				tmpname.name_type=multiname::NAME_UINT;
				tmpname.name_ui = name.name_o->toUInt();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype,wrk);
			case T_NUMBER:
				tmpname.name_type=multiname::NAME_NUMBER;
				tmpname.name_d = name.name_o->toNumber();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype,wrk);
			case T_STRING:
				tmpname.name_type=multiname::NAME_STRING;
				tmpname.name_s_id = name.name_o->toStringId();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype,wrk);
			default:
				break;
		}
		Dictionary::dictType::iterator it=findKey(name.name_o);
		return it != data.end();
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_UINT ||
			name.name_type==multiname::NAME_NUMBER);
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
	}
}

uint32_t Dictionary::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<data.size())
		return cur_index+1;
	else
	{
		//Fall back on object properties
		uint32_t ret=ASObject::nextNameIndex(cur_index-data.size());
		if(ret==0)
			return 0;
		else
			return ret+data.size();

	}
}

void Dictionary::nextName(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
	{
		auto it=data.begin();
		for(unsigned int i=1;i<index;i++)
			++it;
		it->first->incRef();
		ret = asAtomHandler::fromObject(it->first);
	}
	else
	{
		//Fall back on object properties
		ASObject::nextName(ret,index-data.size());
	}
}

void Dictionary::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
	{
		auto it=data.begin();
		for(unsigned int i=1;i<index;i++)
			++it;

		ASATOM_INCREF(it->second);
		ret = it->second;
	}
	else
	{
		//Fall back on object properties
		ASObject::nextValue(ret,index-data.size());
	}
}

bool Dictionary::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	for (auto it = data.begin(); it != data.end(); it++)
	{
		ret = it->first->countAllCylicMemberReferences(gcstate) || ret;
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
		retstr << "{" << it->first->toString() << ", " << asAtomHandler::toString(it->second,getInstanceWorker()) << "}";
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

