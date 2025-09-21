/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "parsing/amf3_generator.h"
#include "scripting/toplevel/IFunction.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Date.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/class.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/xml/flashxml.h"
#include "toplevel/XML.h"
#include <iostream>
#include <fstream>
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/utils/Dictionary.h"

using namespace std;
using namespace lightspark;

asAtom Amf3Deserializer::readObject()
{
	vector<tiny_string> stringMap;
	vector<asAtom> objMap;
	vector<TraitsRef> traitsMap;
	asAtom ret = parseValue(stringMap, objMap, traitsMap);
	if (!parserError.empty())
		LOG(LOG_ERROR,"Amf3Deserializer::readObject error:"<<parserError);
	return ret;
}

void Amf3Deserializer::readSharedObject(ASObject* ret)
{
	// skip the amf header of the file
	// -> 2 byte always 0x00, 0xbf
	// -> 4 byte filesize-6
	// -> 10 byte always 'T', 'C', 'S', 'O', 0x00, 0x04, 0x00, 0x00, 0x00, 0x00
	input->setPosition(16);
	tiny_string name;
	bool ok=false;
	ok = input->readUTF(name);
	uint32_t amfversion;
	ok = ok && input->readUnsignedInt(amfversion);
	if (ok)
	{
		vector<tiny_string> stringMap;
		vector<asAtom> objMap;
		vector<TraitsRef> traitsMap;
		if (amfversion!= 3 && amfversion!=0)
			LOG(LOG_ERROR,"invalid amf version for sharedObject:"<<name<<" "<<amfversion);
		input->setCurrentObjectEncoding(amfversion==3 ? OBJECT_ENCODING::AMF3 : OBJECT_ENCODING::AMF0);
		while (input->getPosition() < input->getLength())
		{
			tiny_string key = amfversion==3 ? parseStringVR(stringMap) : parseStringAMF0();
			asAtom value = parseValue(stringMap,objMap,traitsMap);
			if (!parserError.empty())
			{
				LOG(LOG_ERROR,"Amf3Deserializer::readSharedObject error:"<<parserError);
				break;
			}
			if (asAtomHandler::isValid(value))
			{
				multiname m(nullptr);
				m.name_type=multiname::NAME_STRING;
				m.name_s_id=input->getSystemState()->getUniqueStringId(key);
				ret->setVariableByMultiname(m,value,CONST_ALLOWED,nullptr,input->getInstanceWorker());
			}
			uint8_t b;
			input->readByte(b); // skip 0 byte
		}
	}
}

asAtom Amf3Deserializer::parseInteger()
{
	uint32_t tmp;
	if(!input->readU29(tmp))
	{
		parserError="Not enough data to parse integer";
		return asAtomHandler::invalidAtom;
	}
	return asAtomHandler::fromInt((int32_t)tmp);
}

asAtom Amf3Deserializer::parseDouble()
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
		{
			parserError="Not enough data to parse double";
			return asAtomHandler::invalidAtom;
		}
	}
	tmp.dummy=GINT64_FROM_BE(tmp.dummy);
	
	return asAtomHandler::fromNumber(input->getInstanceWorker(),tmp.val,false);
}

asAtom Amf3Deserializer::parseDate()
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
		{
			parserError="Not enough data to parse date";
			return asAtomHandler::invalidAtom;
		}
	}
	tmp.dummy=GINT64_FROM_BE(tmp.dummy);
	Date* dt = Class<Date>::getInstanceS(input->getInstanceWorker());
	dt->MakeDateFromMilliseconds((int64_t)tmp.val);
	return asAtomHandler::fromObject(dt);
}

tiny_string Amf3Deserializer::parseStringVR(std::vector<tiny_string>& stringMap)
{
	uint32_t strRef;
	if(!input->readU29(strRef))
	{
		parserError="Not enough data to parse string";
		return "";
	}

	if((strRef&0x01)==0)
	{
		//Just a reference
		if(stringMap.size() <= (strRef >> 1))
		{
			parserError="Invalid string reference in AMF3 data";
			return "";
		}
		return stringMap[strRef >> 1];
	}

	uint32_t strLen=strRef>>1;
	string retStr;
	for(uint32_t i=0;i<strLen;i++)
	{
		uint8_t c;
		if(!input->readByte(c))
		{
			parserError="Not enough data to parse string";
			return "";
		}
		retStr.push_back(c);
	}
	//Add string to the map, if it's not the empty one
	if(retStr.size())
		stringMap.emplace_back(retStr);
	return retStr;
}

asAtom Amf3Deserializer::parseArray(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap)
{
	uint32_t arrayRef;
	if(!input->readU29(arrayRef))
	{
		parserError="Not enough data to parse AMF3 array";
		return asAtomHandler::invalidAtom;
	}

	if((arrayRef&0x01)==0)
	{
		//Just a reference
		if(objMap.size() <= (arrayRef >> 1))
		{
			parserError="Invalid object reference in AMF3 data";
			return asAtomHandler::invalidAtom;
		}
		asAtom ret=objMap[arrayRef >> 1];
		ASATOM_INCREF(ret);
		return ret;
	}

	Array* ret=Class<lightspark::Array>::getInstanceS(input->getInstanceWorker());
	//Add object to the map
	objMap.push_back(asAtomHandler::fromObject(ret));

	uint32_t denseCount = arrayRef >> 1;

	//Read name, value pairs
	while(1)
	{
		const tiny_string& varName=parseStringVR(stringMap);
		if(varName=="")
			break;
		asAtom value=parseValue(stringMap, objMap, traitsMap);
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=input->getSystemState()->getUniqueStringId(varName);
		m.isInteger=Array::isIntegerWithoutLeadingZeros(varName);
		ret->setVariableByMultiname(m,value,CONST_ALLOWED,nullptr,input->getInstanceWorker());
	}

	//Read the dense portion
	if (ret->size() < denseCount)
		ret->resize(denseCount);
	for(uint32_t i=0;i<denseCount;i++)
	{
		asAtom value=parseValue(stringMap, objMap, traitsMap);
		ret->set(i,value,false,false);
	}
	return asAtomHandler::fromObject(ret);
}

asAtom Amf3Deserializer::parseVector(uint8_t marker, std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap)
{
	uint32_t vectorRef;
	if(!input->readU29(vectorRef))
	{
		parserError="Not enough data to parse AMF3 vector";
		return asAtomHandler::invalidAtom;
	}

	if((vectorRef&0x01)==0)
	{
		//Just a reference
		if(objMap.size() <= (vectorRef >> 1))
		{
			parserError="Invalid object reference in AMF3 data";
			return asAtomHandler::invalidAtom;
		}
		asAtom ret=objMap[vectorRef >> 1];
		ASATOM_INCREF(ret);
		return ret;
	}

	uint8_t b;
	if (!input->readByte(b))
	{
		parserError="Not enough data to parse AMF3 vector";
		return asAtomHandler::invalidAtom;
	}
	Type* type =nullptr;
	switch (marker)
	{
		case vector_int_marker:
			type = Class<Integer>::getClass(input->getSystemState());
			break;
		case vector_uint_marker:
			type = Class<UInteger>::getClass(input->getSystemState());
			break;
		case vector_double_marker:
			type = Class<Number>::getClass(input->getSystemState());
			break;
		case vector_object_marker:
		{
			tiny_string aliasname;
			aliasname = parseStringVR(stringMap);
			type = input->getSystemState()->getObjectClassRef();
			if (!aliasname.empty())
			{
				ApplicationDomain* appdomain = input->getInstanceWorker()->rootClip->applicationDomain.getPtr();
				auto it=appdomain->aliasMap.find(aliasname);
				if(it==appdomain->aliasMap.end())
					LOG(LOG_ERROR,"unknown  vector alias when parsing vector:"<<aliasname);
				else
					type = it->second;
			}
			break;
		}
		default:
			LOG(LOG_ERROR,"invalid marker during deserialization of vector:"<<marker);
			parserError="invalid marker in AMF3 vector";
			return asAtomHandler::invalidAtom;
	}
	asAtom v=asAtomHandler::invalidAtom;
	Template<Vector>::getInstanceS(input->getInstanceWorker(),v,
								   type,
								   ABCVm::getCurrentApplicationDomain(input->getInstanceWorker()->currentCallContext));
	Vector* ret= asAtomHandler::as<Vector>(v);
	//Add object to the map
	objMap.push_back(asAtomHandler::fromObject(ret));

	
	int32_t count = vectorRef >> 1;

	for(int32_t i=0;i<count;i++)
	{
		switch (marker)
		{
			case vector_int_marker:
			{
				uint32_t value = 0;
				if (!input->readUnsignedInt(value))
				{
					parserError="Not enough data to parse AMF3 vector";
					return asAtomHandler::fromObjectNoPrimitive(ret);
				}
				asAtom v=asAtomHandler::fromInt((int32_t)value);
				ret->append(v);
				break;
			}
			case vector_uint_marker:
			{
				uint32_t value = 0;
				if (!input->readUnsignedInt(value))
				{
					parserError="Not enough data to parse AMF3 vector";
					return asAtomHandler::fromObjectNoPrimitive(ret);
				}
				asAtom v=asAtomHandler::fromUInt(value);
				ret->append(v);
				break;
			}
			case vector_double_marker:
			{
				asAtom v = parseDouble();
				ret->append(v);
				break;
			}
			case vector_object_marker:
			{
				asAtom value=parseValue(stringMap, objMap, traitsMap);
				ret->checkValue(value,true);
				ret->append(value);
				break;
			}
		}
	}
	// set fixed at last to avoid rangeError
	ret->setFixed(b == 0x01);
	return asAtomHandler::fromObject(ret);
}


asAtom Amf3Deserializer::parseDictionary(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap)
{
	uint32_t dictRef;
	if(!input->readU29(dictRef))
	{
		parserError="Not enough data to parse AMF3 dictionary";
		return asAtomHandler::invalidAtom;
	}

	if((dictRef&0x01)==0)
	{
		//Just a reference
		if(objMap.size() <= (dictRef >> 1))
		{
			parserError="Invalid object reference in AMF3 data";
			return asAtomHandler::invalidAtom;
		}
		asAtom ret=objMap[dictRef >> 1];
		ASATOM_INCREF(ret);
		return ret;
	}

	uint8_t weakkeys;
	if (!input->readByte(weakkeys))
	{
		parserError="Not enough data to parse AMF3 dictionary";
		return asAtomHandler::invalidAtom;
	}
	if (weakkeys)
		LOG(LOG_NOT_IMPLEMENTED,"handling of weak keys in Dictionary");
	Dictionary* ret=Class<Dictionary>::getInstanceS(input->getInstanceWorker());
	//Add object to the map
	objMap.push_back(asAtomHandler::fromObject(ret));

	
	int32_t count = dictRef >> 1;

	for(int32_t i=0;i<count;i++)
	{
		asAtom key=parseValue(stringMap, objMap, traitsMap);
		asAtom value=parseValue(stringMap, objMap, traitsMap);
		multiname name(nullptr);
		if (asAtomHandler::isString(key))
		{
			name.name_type=multiname::NAME_STRING;
			name.name_s_id=asAtomHandler::toStringId(key,input->getInstanceWorker());
			ASATOM_DECREF(key);
		}
		else if (asAtomHandler::isInteger(key))
		{
			name.name_type=multiname::NAME_INT;
			name.name_i=asAtomHandler::getInt(key);
		}
		else if (asAtomHandler::isUInteger(key))
		{
			name.name_type=multiname::NAME_UINT;
			name.name_ui=asAtomHandler::getUInt(key);
		}
		else if (asAtomHandler::isNumber(key))
		{
			name.name_type=multiname::NAME_NUMBER;
			name.name_d=asAtomHandler::toNumber(key);
		}
		else
		{
			name.name_type=multiname::NAME_OBJECT;
			name.name_o = key;
		}
		name.ns.push_back(nsNameAndKind(input->getSystemState(),"",NAMESPACE));
		ret->setVariableByMultiname(name,value,CONST_ALLOWED,nullptr,input->getInstanceWorker());
	}
	return asAtomHandler::fromObject(ret);
}

asAtom Amf3Deserializer::parseByteArray(std::vector<asAtom>& objMap)
{
	uint32_t bytearrayRef;
	if(!input->readU29(bytearrayRef))
	{
		parserError="Not enough data to parse AMF3 bytearray";
		return asAtomHandler::invalidAtom;
	}

	if((bytearrayRef&0x01)==0)
	{
		//Just a reference
		if(objMap.size() <= (bytearrayRef >> 1))
		{
			parserError="Invalid object reference in AMF3 data";
			return asAtomHandler::invalidAtom;
		}
		asAtom ret=objMap[bytearrayRef >> 1];
		ASATOM_INCREF(ret);
		return ret;
	}

	ByteArray* ret=Class<ByteArray>::getInstanceS(input->getInstanceWorker());
	//Add object to the map
	objMap.push_back(asAtomHandler::fromObject(ret));

	uint32_t count = bytearrayRef >> 1;

	for(uint32_t i=0;i<count;i++)
	{
		uint8_t b;
		if (!input->readByte(b))
		{
			parserError="Not enough data to parse AMF3 bytearray";
			return asAtomHandler::fromObjectNoPrimitive(ret);
		}
		ret->writeByte(b);
	}
	ret->setPosition(0);
	return asAtomHandler::fromObject(ret);
}

asAtom Amf3Deserializer::parseObject(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap)
{
	uint32_t objRef;
	if(!input->readU29(objRef))
	{
		parserError="Not enough data to parse AMF3 object";
		return asAtomHandler::invalidAtom;
	}
	if((objRef&0x01)==0)
	{
		//Just a reference
		if(objMap.size() <= (objRef >> 1))
		{
			parserError="Invalid object reference in AMF3 data";
			return asAtomHandler::invalidAtom;
		}
		asAtom ret=objMap[objRef >> 1];
		ASATOM_INCREF(ret);
		return ret;
	}

	if((objRef&0x07)==0x07)
	{
		//Custom serialization
		const tiny_string& className=parseStringVR(stringMap);
		assert_and_throw(!className.empty());
		ApplicationDomain* appdomain = input->getInstanceWorker()->rootClip->applicationDomain.getPtr();
		const auto it=appdomain->aliasMap.find(className);
		assert_and_throw(it!=appdomain->aliasMap.end());

		Class_base* type=it->second;
		traitsMap.push_back(TraitsRef(type));

		asAtom ret=asAtomHandler::invalidAtom;
		type->getInstance(input->getInstanceWorker(),ret,true, nullptr, 0);
		//Invoke readExternal
		multiname readExternalName(nullptr);
		readExternalName.name_type=multiname::NAME_STRING;
		readExternalName.name_s_id=input->getSystemState()->getUniqueStringId("readExternal");
		readExternalName.ns.push_back(nsNameAndKind(input->getSystemState(),"",NAMESPACE));
		readExternalName.isAttribute = false;

		asAtom o=asAtomHandler::invalidAtom;
		asAtomHandler::getObject(ret)->getVariableByMultiname(o,readExternalName,GET_VARIABLE_OPTION::SKIP_IMPL,input->getInstanceWorker());
		assert_and_throw(asAtomHandler::isFunction(o));
		asAtom tmpArg[1] = { asAtomHandler::fromObject(input) };
		asAtom r=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(o,input->getInstanceWorker(),r,ret, tmpArg, 1,false);
		ASATOM_DECREF(o);
		return ret;
	}

	TraitsRef traits(nullptr);
	if((objRef&0x02)==0)
	{
		uint32_t traitsRef=objRef>>2;
		if(traitsMap.size() <= traitsRef)
		{
			parserError="Invalid traits reference in AMF3 data";
			return asAtomHandler::invalidAtom;
		}
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

		ApplicationDomain* appdomain = input->getInstanceWorker()->rootClip->applicationDomain.getPtr();
		const auto it=appdomain->aliasMap.find(className);
		if(it!=appdomain->aliasMap.end())
			traits.type=it->second;
		traitsMap.emplace_back(traits);
	}

	asAtom ret=asAtomHandler::invalidAtom;
	if (traits.type)
	{
		traits.type->getInstance(input->getInstanceWorker(),ret,true, nullptr, 0,nullptr,false);
		if (input->getInstanceWorker()->currentCallContext->exceptionthrown)
		{
			// exception thrown during construction
			// adobe seems to just trace the error and continue;
			input->getSystemState()->trace(input->getInstanceWorker()->currentCallContext->exceptionthrown->toString());
			input->getInstanceWorker()->currentCallContext->exceptionthrown->decRef();
			input->getInstanceWorker()->currentCallContext->exceptionthrown=nullptr;
		}
	}
	else
		ret =asAtomHandler::fromObject(new_asobject(input->getInstanceWorker()));
	//Add object to the map
	objMap.push_back(ret);

	for(uint32_t i=0;i<traits.traitsNames.size();i++)
	{
		asAtom value=parseValue(stringMap, objMap, traitsMap);

		multiname name(nullptr);
		name.name_type=multiname::NAME_STRING;
		name.name_s_id=input->getSystemState()->getUniqueStringId(traits.traitsNames[i]);
		name.ns.push_back(nsNameAndKind(input->getSystemState(),"",NAMESPACE));
		name.isAttribute=false;
		asAtomHandler::getObject(ret)->setVariableByMultiname_intern(name,value,CONST_ALLOWED,traits.type,nullptr,input->getInstanceWorker());
		if (input->getInstanceWorker()->currentCallContext->exceptionthrown)
		{
			// exception thrown during setter call
			// adobe seems to just trace the error and continue;
			input->getSystemState()->trace(input->getInstanceWorker()->currentCallContext->exceptionthrown->toString());
			input->getInstanceWorker()->currentCallContext->exceptionthrown->decRef();
			input->getInstanceWorker()->currentCallContext->exceptionthrown=nullptr;
			ASATOM_DECREF(value);
		}
	}

	//Read dynamic name, value pairs
	while(traits.dynamic)
	{
		const tiny_string& varName=parseStringVR(stringMap);
		if(varName=="")
			break;
		asAtom value=parseValue(stringMap, objMap, traitsMap);
		asAtomHandler::getObject(ret)->setVariableAtomByQName(varName,nsNameAndKind(),value,DYNAMIC_TRAIT);
	}
	return ret;
}

asAtom Amf3Deserializer::parseXML(std::vector<asAtom>& objMap, bool legacyXML)
{
	uint32_t xmlRef;
	if(!input->readU29(xmlRef))
	{
		parserError="Not enough data to parse XML";
		return asAtomHandler::invalidAtom;
	}

	if((xmlRef&0x01)==0)
	{
		//Just a reference
		if(objMap.size() <= (xmlRef >> 1))
		{
			parserError="Invalid XML reference in AMF3 data";
			return asAtomHandler::invalidAtom;
		}
		asAtom xmlObj = objMap[xmlRef >> 1];
		ASATOM_INCREF(xmlObj);
		return xmlObj;
	}

	uint32_t strLen=xmlRef>>1;
	string xmlStr;
	for(uint32_t i=0;i<strLen;i++)
	{
		uint8_t c;
		if(!input->readByte(c))
		{
			parserError="Not enough data to parse XML string";
			return asAtomHandler::invalidAtom;
		}
		xmlStr.push_back(c);
	}

	ASObject *xmlObj;
	if(legacyXML)
		xmlObj=Class<XMLDocument>::getInstanceS(input->getInstanceWorker(),xmlStr);
	else
		xmlObj=XML::createFromString(input->getInstanceWorker(),xmlStr);
	objMap.push_back(asAtomHandler::fromObject(xmlObj));
	return asAtomHandler::fromObject(xmlObj);
}


asAtom Amf3Deserializer::parseValue(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap)
{
	//Read the first byte as it contains the object marker
	uint8_t marker;
	if(!input->readByte(marker))
	{
		parserError="Not enough data to parse AMF value";
		return asAtomHandler::invalidAtom;
	}
	if (input->getCurrentObjectEncoding() == OBJECT_ENCODING::AMF3)
	{
		switch(marker)
		{
			case null_marker:
				return asAtomHandler::nullAtom;
			case undefined_marker:
				return asAtomHandler::undefinedAtom;
			case false_marker:
				return asAtomHandler::falseAtom;
			case true_marker:
				return asAtomHandler::trueAtom;
			case integer_marker:
				return parseInteger();
			case double_marker:
				return parseDouble();
			case date_marker:
				return parseDate();
			case string_marker:
				return asAtomHandler::fromObject(abstract_s(input->getInstanceWorker(),parseStringVR(stringMap)));
			case xml_doc_marker:
				return parseXML(objMap, true);
			case array_marker:
				return parseArray(stringMap, objMap, traitsMap);
			case object_marker:
				return parseObject(stringMap, objMap, traitsMap);
			case xml_marker:
				return parseXML(objMap, false);
			case byte_array_marker:
				return parseByteArray(objMap);
			case vector_int_marker:
			case vector_uint_marker:
			case vector_double_marker:
			case vector_object_marker:
				return parseVector(marker, stringMap, objMap, traitsMap);
			case dictionary_marker:
				return parseDictionary(stringMap, objMap, traitsMap);
			default:
				LOG(LOG_ERROR,"Unsupported marker " << (uint32_t)marker);
				parserError="unsupported marker";
				return asAtomHandler::invalidAtom;
		}
	
	}
	else
	{
		switch(marker)
		{
			case amf0_number_marker:
				return parseDouble();
			case amf0_boolean_marker:
			{
				uint8_t b;
				input->readByte(b);
				return asAtomHandler::fromBool((bool)b);
			}
			case amf0_string_marker:
				return asAtomHandler::fromObject(abstract_s(input->getInstanceWorker(),parseStringAMF0()));
			case amf0_object_marker:
				return parseObjectAMF0(stringMap,objMap,traitsMap);
			case amf0_null_marker:
				return asAtomHandler::nullAtom;
			case amf0_undefined_marker:
				return asAtomHandler::undefinedAtom;
			case amf0_reference_marker:
				LOG(LOG_ERROR,"unimplemented marker " << (uint32_t)marker);
				parserError="unimplemented marker amf0_reference_marker";
				return asAtomHandler::invalidAtom;
			case amf0_ecma_array_marker:
				return parseECMAArrayAMF0(stringMap,objMap,traitsMap);
			case amf0_strict_array_marker:
				return parseStrictArrayAMF0(stringMap,objMap,traitsMap);
			case amf0_date_marker:
				LOG(LOG_ERROR,"unimplemented marker " << (uint32_t)marker);
				parserError="unimplemented marker amf0_date_marker";
				return asAtomHandler::invalidAtom;
			case amf0_long_string_marker:
				LOG(LOG_ERROR,"unimplemented marker " << (uint32_t)marker);
				parserError="unimplemented marker amf0_long_string_marker";
				return asAtomHandler::invalidAtom;
			case amf0_xml_document_marker:
				return parseXML(objMap, false);
			case amf0_typed_object_marker:
			{
				tiny_string class_name = parseStringAMF0();
				return parseObjectAMF0(stringMap,objMap,traitsMap, class_name);
			}
			case amf0_avmplus_object_marker:
				input->setCurrentObjectEncoding(OBJECT_ENCODING::AMF3);
				return parseValue(stringMap, objMap, traitsMap);
			default:
				LOG(LOG_ERROR,"Unsupported marker " << (uint32_t)marker);
				parserError="Unsupported marker";
				return asAtomHandler::invalidAtom;
		}
	}
}
tiny_string Amf3Deserializer::parseStringAMF0()
{
	uint16_t strLen;
	if(!input->readShort(strLen))
	{
		parserError="Not enough data to parse string length";
		return "";
	}

	string retStr;
	for(uint32_t i=0;i<strLen;i++)
	{
		uint8_t c;
		if(!input->readByte(c))
		{
			parserError="Not enough data to parse string";
			return retStr;
		}
		retStr.push_back(c);
	}
	return retStr;
}
asAtom Amf3Deserializer::parseECMAArrayAMF0(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap)
{
	uint32_t count;
	if(!input->readUnsignedInt(count))
	{
		parserError="Not enough data to parse AMF0 ECMA array length";
		return asAtomHandler::invalidAtom;
	}

	Array* ar = Class<Array>::getInstanceS(input->getInstanceWorker());
	ar->resize(count);
	asAtom ret=asAtomHandler::fromObject(ar);

	//Read name, value pairs
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	while(true)
	{
		tiny_string varName = parseStringAMF0();
		if (varName == "")
		{
			uint8_t marker = 0;
			input->readByte(marker);
			if (marker == amf0_object_end_marker )
				break;
			parserError="empty key in AMF0 ECMA array";
			return asAtomHandler::fromObjectNoPrimitive(ar);
		}
		asAtom value=parseValue(stringMap, objMap, traitsMap);
		// contrary to Adobe AMF specs integer names are treated as indexes inside the array
		m.name_s_id = ar->getSystemState()->getUniqueStringId(varName);
		m.isInteger=Array::isIntegerWithoutLeadingZeros(varName);
		ar->setVariableByMultiname(m,value,CONST_ALLOWED,nullptr,input->getInstanceWorker());
		count--;
	}
	return ret;
}
asAtom Amf3Deserializer::parseStrictArrayAMF0(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap)
{
	uint32_t count;
	if(!input->readUnsignedInt(count))
	{
		parserError="Not enough data to parse AMF3 strict array length";
		return asAtomHandler::invalidAtom;
	}

	lightspark::Array* ret=Class<lightspark::Array>::getInstanceS(input->getInstanceWorker());

	while(count)
	{
		asAtom value=parseValue(stringMap, objMap, traitsMap);
		ret->push(value);
		count--;
	}
	return asAtomHandler::fromObject(ret);
}

asAtom Amf3Deserializer::parseObjectAMF0(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap,
			const tiny_string &clsname)
{
	asAtom ret = asAtomHandler::invalidAtom;
	if (clsname == "")
		ret=asAtomHandler::fromObject(new_asobject(input->getInstanceWorker()));
	else
	{
		input->getSystemState()->getClassInstanceByName(input->getInstanceWorker(),ret,clsname);
	}

	while (true)
	{
		tiny_string varName = parseStringAMF0();
		if (varName == "")
		{
			uint8_t marker = 0;
			input->readByte(marker);
			if (marker == amf0_object_end_marker )
				return ret;
			parserError="empty key in AMF0 object";
			return ret;
		}
		asAtom value=parseValue(stringMap, objMap, traitsMap);

		if (clsname == "")
			asAtomHandler::getObjectNoCheck(ret)->setVariableAtomByQName(varName,nsNameAndKind(),value,DYNAMIC_TRAIT);
		else
		{
			multiname m(nullptr);
			m.name_type = multiname::NAME_STRING;
			m.name_s_id = input->getSystemState()->getUniqueStringId(varName);
			asAtomHandler::getObjectNoCheck(ret)->setVariableByMultiname(m,value,CONST_ALLOWED,nullptr,input->getInstanceWorker());
		}
	}
	return ret;
}

