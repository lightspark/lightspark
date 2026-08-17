/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include <fstream>
#include <iostream>

#include "parsing/amf3_generator.h"

using namespace lightspark;

std::pair<bool, size_t> Amf3Deserializer::parseSize(Span<const uint8_t>& data)
{
}

uint32_t Amf3Deserializer::parseInt(Span<const uint8_t>& data)
{
	uint32_t tmp;
	if(!input->readU29(tmp))
	{
		parserError="Not enough data to parse integer";
		return asAtomHandler::invalidAtom;
	}
	return asAtomHandler::fromInt((int32_t)tmp);
}

int32_t Amf3Deserializer::parseInteger(Span<const uint8_t>& data)
{
	return int32_t(parseInt(data));
}

number_t Amf3Deserializer::parseDouble(Span<const uint8_t>& data)
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
	tmp.dummy=LS_UINT64_TO_BE(tmp.dummy);
	
	return asAtomHandler::fromNumber(tmp.val);
}

_R<AMF3Value> Amf3Deserializer::parseDate(Span<const uint8_t>& data)
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
	tmp.dummy=LS_UINT64_TO_BE(tmp.dummy);
	Date* dt = Class<Date>::getInstanceS(input->getInstanceWorker());
	dt->MakeDateFromMilliseconds((int64_t)tmp.val);
	return asAtomHandler::fromObject(dt);
}

tiny_string Amf3Deserializer::parseString(Span<const uint8_t>& data)
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

_R<AMF3Value> Amf3Deserializer::parseXMLDoc(Span<const uint8_t>& data)
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

_R<AMF3Value> Amf3Deserializer::parseArray(Span<const uint8_t>& data)
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

_R<AMF3Value> Amf3Deserializer::parseVector
(
	const AMF3TypeMarker& type,
	Span<const uint8_t>& data
)
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


_R<AMF3Value> Amf3Deserializer::parseDictionary(Span<const uint8_t>& data)
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

_R<AMF3Value> Amf3Deserializer::parseByteArray(Span<const uint8_t>& data)
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

_R<AMF3Value> Amf3Deserializer::parseObjectImpl
(
	Span<const uint8_t>& data,
	size_t size,
	size_t idx
)
{
	auto traits = parseTraits(data, size);
	auto ref = objMap.back();
	auto obj = ref->tryAs<AMF3Object>();
	if (obj.hasValue())
		obj->traits = traits;

	if (traits.external)
	{
		auto it = extDecoders.find(traitRef);
		assert_and_throw(it != extDecoders.end());
		return _MR(new AMF3Value(AMF3Custom
		(
			it->second(data, *this),
			{},
			traits
		)));
	}

	std::vector<AMFElement> elems;
	elems.reserve(size);
	for (const auto& name : traits.staticProps)
		elems.emplace_back(name, parseValue(data));

	if (traits.dynamic)
	{
		auto name = parseString(data);
		for (; !name.empty(); name = parseString(data))
			elems.emplace_back(name, parseValue(data));
	}

	if (obj.hasValue())
		obj.elems = elems;
	return ref;
}

_R<AMF3Value> Amf3Deserializer::parseObject(Span<const uint8_t>& data)
{
	return parseRefOrVal
	(
		data,
		[&] { return AMF3Object(++objId); },
		[&](size_t size, size_t idx)
		{
			return parseObjectImpl(data, size, idx);
		}
	);
}

_R<AMF3Value> Amf3Deserializer::parseXML
(
	Span<const uint8_t>& data,
	bool isStr
)
{
	return parseRefOrVal
	(
		data,
		[] { return AMF3Value("", false); },
		[&](size_t size, size_t idx)
		{
			return _MR(new AMF3Value
			(
				data.readBytes(size),
				isStr
			));
		}
	)
}

_R<AMF3Value> Amf3Deserializer::parseRefOrVal
(
	Span<const uint8_t>& data,
	std::function<AMF3Value()> makeValue,
	std::function<_R<AMF3Value>(size_t, size_t)> parseVal
)
{
	auto pair = parseSize(data);
	if (!pair.first)
	{
		auto idx = objMap.size();
		objMap.emplace_back(new AMF3Value(makeValue()));
		return objMap.at(idx) = parseVal(pair.second, idx);
	}

	auto ref = objMap.at(pair.second);
	auto id = ref->getRefID();
	return id != -1 ? _MR(new AMF3Value(id)) : ref;
}

TraitsRef Amf3Deserializer::parseTraits
(
	Span<const uint8_t>& data,
	size_t size,
	size_t idx
)
{
}

AMFElement Amf3Deserializer::parseElement(Span<const uint8_t>& data)
{
	auto name = parseString(data);
	return AMFElement(name, parseValue(data));
}

_R<AMF3Value> Amf3Deserializer::parseValue(Span<const uint8_t>& data)
{
	const auto undefVal = AMF3Value::undefinedVal;
	const auto nullVal = AMF3Value::nullVal;

	auto type = data.read<AMF3TypeMarker>();
	switch (type)
	{
		case AMF3TypeMarker::Undefined:
			return _MR(new AMF3Value(undefVal));
		case AMF3TypeMarker::Null:
			return _MR(new AMF3Value(nullVal);
		case AMF3TypeMarker::False:
			return _MR(new AMF3Value(false));
		case AMF3TypeMarker::True:
			return _MR(new AMF3Value(true));
		case AMF3TypeMarker::Integer:
			return _MR(new AMF3Value(parseInteger(data)));
		case AMF3TypeMarker::Double:
			return _MR(new AMF3Value(parseDouble(data)));
		case AMF3TypeMarker::String:
			return _MR(new AMF3Value(parseString(data)));
		case AMF3TypeMarker::XMLDoc: return parseXML(data, false);
		case AMF3TypeMarker::Date: return parseDate(data);
		case AMF3TypeMarker::Array: return parseArray(data);
		case AMF3TypeMarker::Object: return parseObject(data);
		case AMF3TypeMarker::XML: return parseXML(data, true);
		case AMF3TypeMarker::ByteArray:
			return parseByteArray(data);
		case AMF3TypeMarker::VectorInt:
		case AMF3TypeMarker::VectorUInt:
		case AMF3TypeMarker::VectorDouble:
		case AMF3TypeMarker::VectorObject:
			return parseVector(type, data);
		case AMF3TypeMarker::Dictionary:
			return parseDictionary(data);
		default:
			throw AMFException("Unsupported type");
			break;
	}
}

std::vector<AMFElement> Amf3Deserializer::parseBody(Span<const uint8_t>& data)
{
	std::vector<AMFElement> ret;
	while (!data.empty())
	{
		ret.push_back(parseElement(data));
		(void)data.read();
	}
	return ret;
}
