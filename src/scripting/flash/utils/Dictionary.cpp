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

using namespace std;
using namespace lightspark;

Dictionary::Dictionary(Class_base* c):ASObject(c),
	data(std::less<dictType::key_type>(), reporter_allocator<dictType::value_type>(c->memoryAccount)),weakkeys(false)
{
}

void Dictionary::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->prototype->setVariableByQName("toJSON",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toJSON),DYNAMIC_TRAIT);
}

void Dictionary::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(Dictionary,_constructor)
{
	Dictionary* th=obj.as<Dictionary>();
	ARG_UNPACK_ATOM(th->weakkeys, false);
}

ASFUNCTIONBODY_ATOM(Dictionary,_toJSON)
{
	ret = asAtom::fromString(sys,"Dictionary");
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

void Dictionary::setVariableByMultiname_i(const multiname& name, int32_t value)
{
	assert_and_throw(implEnable);
	asAtom v(value);
	Dictionary::setVariableByMultiname(name,v,CONST_NOT_ALLOWED);
}

void Dictionary::setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst)
{
	assert_and_throw(implEnable);
	if(name.name_type==multiname::NAME_OBJECT)
	{
		multiname tmpname(NULL);
		tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		switch (name.name_o->getObjectType())
		{
			case T_BOOLEAN:
			case T_INTEGER:
				tmpname.name_type=multiname::NAME_INT;
				tmpname.name_i = name.name_o->toInt();
				ASObject::setVariableByMultiname(tmpname, o, allowConst);
				return;
			case T_UINTEGER:
				tmpname.name_type=multiname::NAME_UINT;
				tmpname.name_ui = name.name_o->toUInt();
				ASObject::setVariableByMultiname(tmpname, o, allowConst);
				return;
			case T_NUMBER:
				tmpname.name_type=multiname::NAME_NUMBER;
				tmpname.name_d = name.name_o->toNumber();
				ASObject::setVariableByMultiname(tmpname, o, allowConst);
				return;
			case T_STRING:
				tmpname.name_type=multiname::NAME_STRING;
				tmpname.name_s_id = name.name_o->toStringId();
				ASObject::setVariableByMultiname(tmpname, o, allowConst);
				return;
			default:
				break;
		}
		name.name_o->incRef();
		_R<ASObject> name_o(name.name_o);

		Dictionary::dictType::iterator it=findKey(name_o.getPtr());
		if(it!=data.end())
		{
			ASATOM_DECREF(it->second);
			it->second=o;
		}
		else
			data.insert(make_pair(name_o,o));
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_UINT ||
			name.name_type==multiname::NAME_NUMBER);
		ASObject::setVariableByMultiname(name, o, allowConst);
	}
}

bool Dictionary::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(implEnable);

	if(name.name_type==multiname::NAME_OBJECT)
	{
		multiname tmpname(NULL);
		tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		switch (name.name_o->getObjectType())
		{
			case T_BOOLEAN:
			case T_INTEGER:
				tmpname.name_type=multiname::NAME_INT;
				tmpname.name_i = name.name_o->toInt();
				return ASObject::deleteVariableByMultiname(tmpname);
			case T_UINTEGER:
				tmpname.name_type=multiname::NAME_UINT;
				tmpname.name_ui = name.name_o->toUInt();
				return ASObject::deleteVariableByMultiname(tmpname);
			case T_NUMBER:
				tmpname.name_type=multiname::NAME_NUMBER;
				tmpname.name_d = name.name_o->toNumber();
				return ASObject::deleteVariableByMultiname(tmpname);
			case T_STRING:
				tmpname.name_type=multiname::NAME_STRING;
				tmpname.name_s_id = name.name_o->toStringId();
				return ASObject::deleteVariableByMultiname(tmpname);
			default:
				break;
		}
		name.name_o->incRef();
		_R<ASObject> name_o(name.name_o);

		Dictionary::dictType::iterator it=findKey(name_o.getPtr());
		if(it != data.end())
		{
			ASATOM_DECREF(it->second);
			data.erase(it);
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
		return ASObject::deleteVariableByMultiname(name);
	}
}

GET_VARIABLE_RESULT Dictionary::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & ASObject::SKIP_IMPL)==0 && implEnable)
	{
		if(name.name_type==multiname::NAME_OBJECT)
		{
			multiname tmpname(NULL);
			tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
			switch (name.name_o->getObjectType())
			{
				case T_BOOLEAN:
				case T_INTEGER:
					tmpname.name_type=multiname::NAME_INT;
					tmpname.name_i = name.name_o->toInt();
					return getVariableByMultinameIntern(ret,tmpname,this->getClass(),opt);
				case T_UINTEGER:
					tmpname.name_type=multiname::NAME_UINT;
					tmpname.name_ui = name.name_o->toUInt();
					return getVariableByMultinameIntern(ret,tmpname,this->getClass(),opt);
				case T_NUMBER:
					tmpname.name_type=multiname::NAME_NUMBER;
					tmpname.name_d = name.name_o->toNumber();
					return getVariableByMultinameIntern(ret,tmpname,this->getClass(),opt);
				case T_STRING:
					tmpname.name_type=multiname::NAME_STRING;
					tmpname.name_s_id = name.name_o->toStringId();
					return getVariableByMultinameIntern(ret,tmpname,this->getClass(),opt);
				default:
					break;
			}
			bool islastref = name.name_o->isLastRef();
			name.name_o->incRef();
			_R<ASObject> name_o(name.name_o);

			Dictionary::dictType::iterator it=findKey(name_o.getPtr());
			if(it != data.end())
			{
				ret = it->second;
				if (islastref)
				{
					LOG(LOG_INFO,"erasing weak key from dictionary:"<< name.name_o->toDebugString());
					name.name_o->decRef();
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
			return getVariableByMultinameIntern(ret,name,this->getClass(),opt);
		}
	}
	//Try with the base implementation
	return getVariableByMultinameIntern(ret,name,this->getClass(),opt);
}

bool Dictionary::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(considerDynamic==false)
	{
		if(name.name_type==multiname::NAME_OBJECT)
			return false;
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
	}
	if (!isConstructed())
		return false;

	if(name.name_type==multiname::NAME_OBJECT)
	{
		multiname tmpname(NULL);
		tmpname.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
		switch (name.name_o->getObjectType())
		{
			case T_BOOLEAN:
			case T_INTEGER:
				tmpname.name_type=multiname::NAME_INT;
				tmpname.name_i = name.name_o->toInt();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype);
			case T_UINTEGER:
				tmpname.name_type=multiname::NAME_UINT;
				tmpname.name_ui = name.name_o->toUInt();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype);
			case T_NUMBER:
				tmpname.name_type=multiname::NAME_NUMBER;
				tmpname.name_d = name.name_o->toNumber();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype);
			case T_STRING:
				tmpname.name_type=multiname::NAME_STRING;
				tmpname.name_s_id = name.name_o->toStringId();
				return ASObject::hasPropertyByMultiname(tmpname, considerDynamic, considerPrototype);
			default:
				break;
		}

		name.name_o->incRef();
		_R<ASObject> name_o(name.name_o);

		Dictionary::dictType::iterator it=findKey(name_o.getPtr());
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
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
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
		ret = asAtom::fromObject(it->first.getPtr());
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

tiny_string Dictionary::toString()
{
	std::stringstream retstr;
	retstr << "{";
	auto it=data.begin();
	while(it != data.end())
	{
		if(it != data.begin())
			retstr << ", ";
		retstr << "{" << it->first->toString() << ", " << it->second.toString(getSystemState()) << "}";
		++it;
	}
	retstr << "}";

	return retstr.str();
}


void Dictionary::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	if (out->getObjectEncoding() == ObjectEncoding::AMF0)
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
			asAtom v;
			nextName(v,tmp);
			v.toObject(getSystemState())->serialize(out, stringMap, objMap, traitsMap);
			nextValue(v,tmp);
			v.toObject(getSystemState())->serialize(out, stringMap, objMap, traitsMap);
		}
	}
}

