/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010  Ennio Barbaro (e.barbaro@sssup.it)
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

#ifndef _AMF3_GENERATOR_H

#include <string>
#include <vector>
#include "compat.h"

namespace lightspark
{

class ASObject;
class ByteArray;

namespace amf3
{

template<class T>
class ReferenceOrValue
{
public:
	enum CONTENTS { NONE=0, REFERENCE, VALUE };
	CONTENTS contents;
	uint32_t reference;
	T value;
	ReferenceOrValue():contents(NONE){}
	ReferenceOrValue<T>& operator=(uint32_t r)
	{
		contents=REFERENCE;
		reference=r;
		return *this;
	}
	//TODO: make movable
	ReferenceOrValue<T>& operator=(const T& r)
	{
		contents=VALUE;
		value=r;
		return *this;
	}
};

struct NullType {};

struct UndefinedType {};

struct BoolType
{
	bool val;
	explicit BoolType(bool v):val(v){}
	BoolType():val(0){}
};

struct Integer
{
	int32_t val;
	explicit Integer(int32_t v):val(v){}
	Integer():val(0){}
};

struct Double
{
	union
	{
		double val;
		uint64_t dummy;
	};
	explicit Double(uint64_t v):dummy(v){}
	Double():val(0){}
};

typedef ReferenceOrValue<std::string> Utf8String;
typedef ReferenceOrValue<uint64_t> Date;

class NameAndValue;
class ValueType;

struct Array
{
	std::vector<NameAndValue> m_associativeSection;
	std::vector<ValueType> m_denseSection;
};

struct Object
{
	std::vector<NameAndValue> m_associativeSection;
};

typedef ReferenceOrValue<Array> ArrayType;
typedef ReferenceOrValue<Object> ObjectType;

class ValueType
{
public:
	enum CONTENTS { NONE=0, NULLTYPE, UNDEFINEDTYPE, BOOL, INTEGER, DOUBLE, UTF8STRING, DATE, ARRAYTYPE, OBJECTTYPE };
	CONTENTS contents;
	//NullType,UndefinedType have no storage
	BoolType boolVal;
	Integer intVal;
	Double doubleVal;
	Utf8String stringVal;
	//Date dateVal;
	ArrayType arrayVal;
	ObjectType objVal;
	ValueType():contents(NONE){}
	ValueType& operator=(NullType r)
	{
		contents=NULLTYPE;
		return *this;
	}
	ValueType& operator=(UndefinedType r)
	{
		contents=UNDEFINEDTYPE;
		return *this;
	}
	//TODO: make movable
	ValueType& operator=(const BoolType& r)
	{
		contents=BOOL;
		boolVal=r;
		return *this;
	}
	ValueType& operator=(const Integer& r)
	{
		contents=INTEGER;
		intVal=r;
		return *this;
	}
	ValueType& operator=(const Double& r)
	{
		contents=DOUBLE;
		doubleVal=r;
		return *this;
	}
	ValueType& operator=(const Utf8String& r)
	{
		contents=UTF8STRING;
		stringVal=r;
		return *this;
	}
	ValueType& operator=(const ArrayType& r)
	{
		contents=ARRAYTYPE;
		arrayVal=r;
		return *this;
	}
	ValueType& operator=(const ObjectType& r)
	{
		contents=OBJECTTYPE;
		objVal=r;
		return *this;
	}
};

struct NameAndValue
{
	Utf8String name;
	ValueType value;
};

enum markers_type
{
	undefined_marker = 0x0,
	null_marker = 0x1,
	false_marker = 0x2,
	true_marker = 0x3,
	integer_marker = 0x4,
	double_marker = 0x5,
	string_marker = 0x6,
	xml_doc_marker = 0x7,
	date_marker = 0x8,
	array_marker = 0x9,
	object_marker = 0xa,
	xml_marker = 0xb
};

}; //namespace amf3

class Amf3Deserializer
{
private:
	ByteArray* input;
	amf3::Utf8String parseStringVR() const;
	amf3::ObjectType parseObject() const;
	amf3::ArrayType parseArray() const;
	amf3::ValueType parseValue() const;
	amf3::Integer parseInteger() const;
	amf3::Double parseDouble() const;
public:
	Amf3Deserializer(ByteArray* i):input(i) {}
	bool generateObjects(std::vector<ASObject*>& objs);
};

};
#endif
