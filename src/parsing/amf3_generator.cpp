/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "amf3_generator.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/Array.h"
#include "scripting/class.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace lightspark;
using namespace lightspark::amf3;

static tiny_string getString(Amf3Deserializer* th, const amf3::Utf8String& s)
{
	if(s.contents==amf3::Utf8String::REFERENCE)
		return "TODO_support_references";
	return s.value;
}

static ASObject* createArray(Amf3Deserializer* th, const amf3::ArrayType&);
static ASObject* createObject(Amf3Deserializer* th, const amf3::ObjectType&);

static ASObject* createValue(Amf3Deserializer* th, const amf3::ValueType& v)
{
	switch(v.contents)
	{
		case amf3::ValueType::NULLTYPE:
			return new Null;
		case amf3::ValueType::UNDEFINEDTYPE:
			return new Undefined;
		case amf3::ValueType::BOOL:
			return abstract_b(v.boolVal.val);
		case amf3::ValueType::INTEGER:
			return abstract_i(v.intVal.val);
		case amf3::ValueType::DOUBLE:
			return abstract_d(v.doubleVal.val);
		case amf3::ValueType::UTF8STRING:
			return Class<ASString>::getInstanceS(getString(th, v.stringVal));
		case amf3::ValueType::ARRAYTYPE:
			return createArray(th, v.arrayVal);
		case amf3::ValueType::OBJECTTYPE:
			return createObject(th, v.objVal);
		default:
			throw UnsupportedException("Unsupported type in AMF3");
	}
}

static ASObject* createObject(Amf3Deserializer* th, const amf3::ObjectType& t)
{
	ASObject* ret=Class<ASObject>::getInstanceS();
	if(t.contents==amf3::ObjectType::REFERENCE)
	{
		LOG(LOG_NOT_IMPLEMENTED,"References in AMF3 not supported");
		return ret;
	}
	const amf3::Object& o=t.value;
	for(uint32_t i=0;i<o.m_associativeSection.size();i++)
	{
		const tiny_string& varName=getString(th, o.m_associativeSection[i].name);
		ASObject* obj=createValue(th, o.m_associativeSection[i].value);
		ret->setVariableByQName(varName,"",obj,DYNAMIC_TRAIT);
	}
	return ret;
}

static ASObject* createArray(Amf3Deserializer* th, const amf3::ArrayType& t)
{
	lightspark::Array* ret=Class<lightspark::Array>::getInstanceS();
	if(t.contents==amf3::ArrayType::REFERENCE)
	{
		LOG(LOG_NOT_IMPLEMENTED,"References in AMF3 not supported");
		return ret;
	}
	const amf3::Array& a=t.value;
	for(uint32_t i=0;i<a.m_associativeSection.size();i++)
	{
		const tiny_string& varName=getString(th, a.m_associativeSection[i].name);
		ASObject* obj=createValue(th, a.m_associativeSection[i].value);
		ret->setVariableByQName(varName,"",obj, DYNAMIC_TRAIT);
	}
	for(uint32_t i=0;i<a.m_denseSection.size();i++)
	{
		ASObject* obj=createValue(th, a.m_denseSection[i]);
		ret->push(obj);
	}
	return ret;
}

bool Amf3Deserializer::generateObjects(std::vector<ASObject*>& objs)
{
	amf3::ValueType ans=parseValue();

	//Now create an object for the parsed value
	objs.push_back(createValue(this, ans));
	return true;
}

amf3::Integer Amf3Deserializer::parseInteger() const
{
	amf3::Integer ret;
	if(!input->readU29(ret.val))
		throw ParseException("Not enough data to parse integer");
	return ret;
}

Double Amf3Deserializer::parseDouble() const
{
	uint64_t tmp;
	uint8_t* tmpPtr=reinterpret_cast<uint8_t*>(&tmp);

	for(uint32_t i=0;i<8;i++)
	{
		if(!input->readByte(tmpPtr[i]))
			throw ParseException("Not enough data to parse double");
	}
	Double ret;
	ret.dummy=GINT64_FROM_BE(tmp);
	return ret;
}

Utf8String Amf3Deserializer::parseStringVR() const
{
	Utf8String ret;
	int32_t strRef;
	if(!input->readU29(strRef))
		throw ParseException("Not enough data to parse string");

	if((strRef&0x01)==0)
		throw UnsupportedException("References not supported in parseStringVR");

	int32_t strLen=strRef>>1;
	string retStr;
	for(int32_t i=0;i<strLen;i++)
	{
		uint8_t c;
		if(!input->readByte(c))
			throw ParseException("Not enough data to parse string");
		retStr.push_back(c);
	}
	ret=retStr;
	return ret;
}

ArrayType Amf3Deserializer::parseArray() const
{
	ArrayType ret;
	int32_t arrayRef;
	if(!input->readU29(arrayRef))
		throw ParseException("Not enough data to parse AMF3 array");

	if((arrayRef&0x01)==0)
		throw UnsupportedException("References not supported in parseArray");

	amf3::Array arrayRet;

	int32_t denseCount = arrayRef >> 1;

	//Read name, value pairs
	while(1)
	{
		NameAndValue prop;
		prop.name=parseStringVR();
		if(prop.name.contents==Utf8String::VALUE && prop.name.value=="")
			break;
		prop.value=parseValue();
		arrayRet.m_associativeSection.emplace_back(prop);
	}

	//Read the dense portion
	for(int32_t i=0;i<denseCount;i++)
	{
		ValueType val=parseValue();
		arrayRet.m_denseSection.emplace_back(val);
	}
	ret=arrayRet;
	return ret;
}

ObjectType Amf3Deserializer::parseObject() const
{
	ObjectType ret;
	//TODO: Use U29
	uint8_t objRef;
	if(!input->readByte(objRef))
		throw ParseException("Not enough data to parse AMF3 object");

	assert_and_throw((objRef&0x80)==0);

	if((objRef&0x01)==0)
		throw UnsupportedException("References not supported in parseObject");

	Object objRet;
	if((objRef&0x02)==0)
		throw UnsupportedException("Traits references not supported in parseObject");

	if((objRef&0x04)==1)
		throw UnsupportedException("Custom externalizable objects not supported in parseObject");

	bool dynamic=objRef&0x08;
	uint32_t traitsCount=objRef>>4;
	assert_and_throw(traitsCount==0);
	/*ret.className=*/parseStringVR();
	//TODO: read traits and traits value

	//Read dynamic name, value pairs
	while(dynamic)
	{
		NameAndValue prop;
		prop.name=parseStringVR();
		if(prop.name.contents==Utf8String::VALUE && prop.name.value=="")
			break;
		prop.value=parseValue();
		objRet.m_associativeSection.emplace_back(prop);
	}
	ret=objRet;
	return ret;
}

ValueType Amf3Deserializer::parseValue() const
{
	//Read the first byte as it contains the object marker
	ValueType ret;
	uint8_t marker;
	if(!input->readByte(marker))
		throw ParseException("Not enough data to parse AMF3 object");

	switch(marker)
	{
		case integer_marker:
		{
			ret=parseInteger();
			break;
		}
		case double_marker:
		{
			ret=parseDouble();
			break;
		}
		case string_marker:
		{
			ret=parseStringVR();
			break;
		}
		case array_marker:
		{
			ret=parseArray();
			break;
		}
		case object_marker:
		{
			ret=parseObject();
			break;
		}
		default:
			LOG(LOG_ERROR,"Unsupported marker " << (uint32_t)marker);
			throw UnsupportedException("Unsupported marker");
	}
	return ret;
}
