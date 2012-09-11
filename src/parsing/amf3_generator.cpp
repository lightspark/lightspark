/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "parsing/amf3_generator.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/class.h"
#include "scripting/flash/xml/flashxml.h"
#include "toplevel/XML.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace lightspark;

_R<ASObject> Amf3Deserializer::readObject() const
{
	vector<tiny_string> stringMap;
	vector<ASObject*> objMap;
	vector<TraitsRef> traitsMap;
	return parseValue(stringMap, objMap, traitsMap);
}

_R<ASObject> Amf3Deserializer::parseInteger() const
{
	uint32_t tmp;
	if(!input->readU29(tmp))
		throw ParseException("Not enough data to parse integer");
	return _MR(abstract_i(tmp));
}

_R<ASObject> Amf3Deserializer::parseDouble() const
{
	union
	{
		uint64_t dummy;
		double val;
	} tmp;
	uint8_t* tmpPtr=reinterpret_cast<uint8_t*>(&tmp.dummy);

	for(uint32_t i=0;i<8;i++)
	{
		if(!input->readByte(tmpPtr[i]))
			throw ParseException("Not enough data to parse double");
	}
	tmp.dummy=GINT64_FROM_BE(tmp.dummy);
	return _MR(abstract_d(tmp.val));
}

tiny_string Amf3Deserializer::parseStringVR(std::vector<tiny_string>& stringMap) const
{
	uint32_t strRef;
	if(!input->readU29(strRef))
		throw ParseException("Not enough data to parse string");

	if((strRef&0x01)==0)
	{
		//Just a reference
		if(stringMap.size() <= (strRef >> 1))
			throw ParseException("Invalid string reference in AMF3 data");
		return stringMap[strRef >> 1];
	}

	uint32_t strLen=strRef>>1;
	string retStr;
	for(uint32_t i=0;i<strLen;i++)
	{
		uint8_t c;
		if(!input->readByte(c))
			throw ParseException("Not enough data to parse string");
		retStr.push_back(c);
	}
	//Add string to the map, if it's not the empty one
	if(retStr.size())
		stringMap.emplace_back(retStr);
	return retStr;
}

_R<ASObject> Amf3Deserializer::parseArray(std::vector<tiny_string>& stringMap,
			std::vector<ASObject*>& objMap,
			std::vector<TraitsRef>& traitsMap) const
{
	uint32_t arrayRef;
	if(!input->readU29(arrayRef))
		throw ParseException("Not enough data to parse AMF3 array");

	if((arrayRef&0x01)==0)
	{
		//Just a reference
		if(objMap.size() <= (arrayRef >> 1))
			throw ParseException("Invalid object reference in AMF3 data");
		ASObject* ret=objMap[arrayRef >> 1];
		ret->incRef();
		return _MR(ret);
	}

	_R<lightspark::Array> ret=_MR(Class<lightspark::Array>::getInstanceS());
	//Add object to the map
	objMap.push_back(ret.getPtr());

	int32_t denseCount = arrayRef >> 1;

	//Read name, value pairs
	while(1)
	{
		const tiny_string& varName=parseStringVR(stringMap);
		if(varName=="")
			break;
		_R<ASObject> value=parseValue(stringMap, objMap, traitsMap);
		value->incRef();
		ret->setVariableByQName(varName,"",value.getPtr(), DYNAMIC_TRAIT);
	}

	//Read the dense portion
	for(int32_t i=0;i<denseCount;i++)
	{
		_R<ASObject> value=parseValue(stringMap, objMap, traitsMap);
		ret->push(value);
	}
	return ret;
}

_R<ASObject> Amf3Deserializer::parseObject(std::vector<tiny_string>& stringMap,
			std::vector<ASObject*>& objMap,
			std::vector<TraitsRef>& traitsMap) const
{
	uint32_t objRef;
	if(!input->readU29(objRef))
		throw ParseException("Not enough data to parse AMF3 object");
	if((objRef&0x01)==0)
	{
		//Just a reference
		if(objMap.size() <= (objRef >> 1))
			throw ParseException("Invalid object reference in AMF3 data");
		ASObject* ret=objMap[objRef >> 1];
		ret->incRef();
		return _MR(ret);
	}

	if((objRef&0x07)==0x07)
	{
		//Custom serialization
		const tiny_string& className=parseStringVR(stringMap);
		assert_and_throw(!className.empty());
		const auto it=getSys()->aliasMap.find(className);
		assert_and_throw(it!=getSys()->aliasMap.end());

		Class_base* type=it->second.getPtr();
		traitsMap.push_back(TraitsRef(type));

		_R<ASObject> ret=_MR(type->getInstance(true, NULL, 0));
		//Invoke readExternal
		multiname readExternalName(NULL);
		readExternalName.name_type=multiname::NAME_STRING;
		readExternalName.name_s_id=getSys()->getUniqueStringId("readExternal");
		readExternalName.ns.push_back(nsNameAndKind("",NAMESPACE));
		readExternalName.isAttribute = false;

		_NR<ASObject> o=ret->getVariableByMultiname(readExternalName,ASObject::SKIP_IMPL);
		assert_and_throw(!o.isNull() && o->getObjectType()==T_FUNCTION);
		IFunction* f=o->as<IFunction>();
		ret->incRef();
		input->incRef();
		ASObject* const tmpArg[1] = {input};
		f->call(ret.getPtr(), tmpArg, 1);
		return ret;
	}

	TraitsRef traits(NULL);
	if((objRef&0x02)==0)
	{
		uint32_t traitsRef=objRef>>2;
		if(traitsMap.size() <= traitsRef)
			throw ParseException("Invalid traits reference in AMF3 data");
		traits=traitsMap[traitsRef];
	}
	else
	{
		traits.dynamic = objRef&0x08;
		uint32_t traitsCount=objRef>>4;
		const tiny_string& className=parseStringVR(stringMap);
		//Add the type to the traitsMap
		for(uint32_t i=0;i<traitsCount;i++)
			traits.traitsNames.emplace_back(parseStringVR(stringMap));

		const auto it=getSys()->aliasMap.find(className);
		if(it!=getSys()->aliasMap.end())
			traits.type=it->second.getPtr();
		traitsMap.emplace_back(traits);
	}

	_R<ASObject> ret=_MR((traits.type)?traits.type->getInstance(true, NULL, 0):
		Class<ASObject>::getInstanceS());
	//Add object to the map
	objMap.push_back(ret.getPtr());

	for(uint32_t i=0;i<traits.traitsNames.size();i++)
	{
		_R<ASObject> value=parseValue(stringMap, objMap, traitsMap);
		value->incRef();

		multiname name(NULL);
		name.name_type=multiname::NAME_STRING;
		name.name_s_id=getSys()->getUniqueStringId(traits.traitsNames[i]);
		name.ns.push_back(nsNameAndKind("",NAMESPACE));
		name.isAttribute=false;

		ret->setVariableByMultiname(name,value.getPtr(),ASObject::CONST_ALLOWED,traits.type);
	}

	//Read dynamic name, value pairs
	while(traits.dynamic)
	{
		const tiny_string& varName=parseStringVR(stringMap);
		if(varName=="")
			break;
		_R<ASObject> value=parseValue(stringMap, objMap, traitsMap);
		value->incRef();
		ret->setVariableByQName(varName,"",value.getPtr(),DYNAMIC_TRAIT);
	}
	return ret;
}

_R<ASObject> Amf3Deserializer::parseXML(std::vector<ASObject*>& objMap, bool legacyXML) const
{
	uint32_t xmlRef;
	if(!input->readU29(xmlRef))
		throw ParseException("Not enough data to parse XML");

	if((xmlRef&0x01)==0)
	{
		//Just a reference
		if(objMap.size() <= (xmlRef >> 1))
			throw ParseException("Invalid XML reference in AMF3 data");
		ASObject *xmlObj = objMap[xmlRef >> 1];
		xmlObj->incRef();
		return _MR(xmlObj);
	}

	uint32_t strLen=xmlRef>>1;
	string xmlStr;
	for(uint32_t i=0;i<strLen;i++)
	{
		uint8_t c;
		if(!input->readByte(c))
			throw ParseException("Not enough data to parse string");
		xmlStr.push_back(c);
	}

	ASObject *xmlObj;
	if(legacyXML)
		xmlObj=Class<XMLDocument>::getInstanceS(xmlStr);
	else
		xmlObj=Class<XML>::getInstanceS(xmlStr);
	objMap.push_back(xmlObj);
	return _MR(xmlObj);
}

_R<ASObject> Amf3Deserializer::parseValue(std::vector<tiny_string>& stringMap,
			std::vector<ASObject*>& objMap,
			std::vector<TraitsRef>& traitsMap) const
{
	//Read the first byte as it contains the object marker
	uint8_t marker;
	if(!input->readByte(marker))
		throw ParseException("Not enough data to parse AMF3 object");

	switch(marker)
	{
		case null_marker:
			return _MR(getSys()->getNullRef());
		case undefined_marker:
			return _MR(getSys()->getUndefinedRef());
		case false_marker:
			return _MR(abstract_b(false));
		case true_marker:
			return _MR(abstract_b(true));
		case integer_marker:
			return parseInteger();
		case double_marker:
			return parseDouble();
		case string_marker:
			return _MR(Class<ASString>::getInstanceS(parseStringVR(stringMap)));
		case xml_doc_marker:
			return parseXML(objMap, true);
		case array_marker:
			return parseArray(stringMap, objMap, traitsMap);
		case object_marker:
			return parseObject(stringMap, objMap, traitsMap);
		case xml_marker:
			return parseXML(objMap, false);
		default:
			LOG(LOG_ERROR,"Unsupported marker " << (uint32_t)marker);
			throw UnsupportedException("Unsupported marker");
	}
}
