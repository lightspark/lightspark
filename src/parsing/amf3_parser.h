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

#ifndef _AMF3_PARSER_H

#include <stdint.h>
#include <iostream>
#include <boost/fusion/include/tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/qi.hpp>

namespace lightspark
{

namespace amf3
{

template<class T>
class ReferenceOrValue
{
template<class Y> friend std::ostream& operator<<(std::ostream& o, const ReferenceOrValue<Y>& r);
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

struct Bool
{
	bool val;
	explicit Bool(bool v):val(v){}
	Bool():val(0){}
};

struct Integer
{
	uint32_t val;
	explicit Integer(uint32_t v):val(v){}
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
struct Array;
typedef ReferenceOrValue<boost::recursive_wrapper<Array> > ArrayType;

class ValueType
{
public:
	enum CONTENTS { NONE=0, NULLTYPE, UNDEFINEDTYPE, BOOL, INTEGER, DOUBLE, UTF8STRING, DATE, ARRAYTYPE };
	CONTENTS contents;
	//NullType,UndefinedType have no storage
	Bool boolVal;
	Integer intVal;
	//Double doubleVal;
	Utf8String stringVal;
	//Date dateVal;
	ArrayType arrayVal;
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
	ValueType& operator=(const Bool& r)
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
};

typedef boost::fusion::tuple<Utf8String,ValueType> NameAndValue;

struct Array
{
	typedef std::vector<NameAndValue> AssociativeArray;
	AssociativeArray m_associativeSection;
	std::vector<ValueType> m_denseSection;
};

std::ostream& operator<<(std::ostream& o, const boost::recursive_wrapper<Array>& r);
std::ostream& operator<<(std::ostream& o, const ValueType& r);

template<class T>
std::ostream& operator<<(std::ostream& o, const ReferenceOrValue<T>& r)
{
	switch(r.contents)
	{
		case ReferenceOrValue<T>::NONE:
			o << "NONE";
			break;
		case ReferenceOrValue<T>::REFERENCE:
			o << "Ref("<<r.reference<<')';
			break;
		case ReferenceOrValue<T>::VALUE:
			o << r.value;
			break;
	}
	return o;
}

struct Amf3ParserGrammar : boost::spirit::qi::grammar<char*,std::vector<ValueType>()>
{
	boost::spirit::qi::rule<char*,uint8_t(uint8_t,uint8_t)> byterange; //Parse a byte in a range;
	boost::spirit::qi::rule<char*,uint32_t(),boost::spirit::qi::locals<uint32_t,uint32_t> > U29_;
	boost::spirit::qi::rule<char*,uint64_t()> DOUBLE_;

	boost::spirit::qi::rule<char*> UTF8_empty_;
	boost::spirit::qi::rule<char*,std::string(uint32_t),boost::spirit::qi::locals<uint32_t> > UTF8_string_;
	boost::spirit::qi::rule<char*,Utf8String(bool),boost::spirit::qi::locals<uint32_t> > UTF8_vr_;

	boost::spirit::qi::rule<char*,uint64_t()> date_time_;

	boost::spirit::qi::rule<char*,NameAndValue() > assoc_value_;
	
	boost::spirit::qi::rule<char*,UndefinedType()> undefined_type;
	boost::spirit::qi::rule<char*,NullType()> null_type;
	boost::spirit::qi::rule<char*,Bool()> false_type;
	boost::spirit::qi::rule<char*,Bool()> true_type;
	boost::spirit::qi::rule<char*,Integer()> integer_type;
	boost::spirit::qi::rule<char*,Double()> double_type;
	boost::spirit::qi::rule<char*,Utf8String()> string_type;
	//boost::spirit::qi::rule<char*,std::string()> XMLDocument_type;
	boost::spirit::qi::rule<char*,Date(),boost::spirit::qi::locals<uint32_t> > date_type;
	boost::spirit::qi::rule<char*,ArrayType(),boost::spirit::qi::locals<uint32_t> > array_type;
	boost::spirit::qi::rule<char*,ValueType()> value_type;
	boost::spirit::qi::rule<char*,std::vector<ValueType>() > entry_rule;
	boost::spirit::qi::rule<char*,Array(uint32_t)> array_struct_value;

	enum markers_type {
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
	Amf3ParserGrammar();
};

}; //namespace amf3

}; //namespace lightspark
BOOST_FUSION_ADAPT_STRUCT(
	lightspark::amf3::Array,
	(lightspark::amf3::Array::AssociativeArray, m_associativeSection)
	(std::vector<lightspark::amf3::ValueType>, m_denseSection)
)

namespace boost { namespace spirit { namespace traits {

//Make sure the parsed types are not wrongly used as containers by spirit
template <typename Enable> struct is_container<std::string, Enable>: mpl::false_ {};

};};};


#endif
