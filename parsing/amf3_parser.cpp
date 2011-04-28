/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010  Ennio Barbaro (e.barbaro@sssup.it)
    Copyright (C) 2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "amf3_parser.h"

#include <string>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_function.hpp>

using namespace std;
using namespace lightspark::amf3;
namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;

Amf3ParserGrammar::Amf3ParserGrammar() : Amf3ParserGrammar::base_type(entry_rule)
{
	using namespace qi::labels;
	using qi::_pass;

	U29_ = (qi::byte_[_a = _1,_b = _a&0x7f] >> 
		-( qi::eps(_a&0x80) >> qi::byte_[_a = _1,_b = (_b<<7)|(_a&0x7f)] >>
		-( qi::eps(_a&0x80) >> qi::byte_[_a = _1,_b = (_b<<7)|(_a&0x7f)] >>
		-( qi::eps(_a&0x80) >> qi::byte_[_a = _1,_b = (_b<<7)|_a]))))[_val = _b];
	DOUBLE_ = qi::big_qword;

	UTF8_empty_ = qi::byte_(0x01);
	UTF8_string_ %= qi::omit[qi::eps[phoenix::bind(&std::string::reserve,_val,_r1+1)]] >> (qi::repeat(_r1)[qi::char_]);
	//The argument signals if 0x01 (empty string) is legal or not
	UTF8_vr_ = qi::omit[U29_[_a = _1]] >>
		( ( qi::eps((_a & 1)==0) >> qi::attr(_a>>1))[_val = _1] | //If it's a reference stop here 
		  ( qi::eps((_a & 1)==1)[_pass=(!_r1 || _a!=0x1),_a=_a>>1] >> (UTF8_string_(_a)))[_val = _1]); //Read a string

	date_time_ %= DOUBLE_;

	assoc_value_ %= UTF8_vr_(true) >> value_type;
	
	undefined_type = qi::byte_(undefined_marker) [_val = phoenix::construct<UndefinedType>()];
	null_type = qi::byte_(null_marker) [_val = phoenix::construct<NullType>()];
	false_type = qi::byte_(false_marker)[_val = phoenix::construct<Bool>(false)];
	true_type = qi::byte_(true_marker)[_val = phoenix::construct<Bool>(true)];
	integer_type = (qi::omit[qi::byte_(integer_marker)] >> U29_)[_val = phoenix::construct<Integer>(_1)];
	double_type = (qi::omit[qi::byte_(double_marker)] >>  DOUBLE_)[_val = phoenix::construct<Double>(_1)];
	string_type %= qi::omit[qi::byte_(string_marker)] >> UTF8_vr_(false);
	date_type = qi::omit[qi::byte_(date_marker)] >> qi::omit[U29_[_a = _1]] >>
		( ( qi::eps((_a & 1)==0) >> qi::attr(_a>>1))[_val = _1] | //If it's a reference stop here 
		  ( qi::eps((_a & 1)==1) >> date_time_)); //Read a string

	array_struct_value %= *assoc_value_ >> qi::omit[UTF8_empty_] >> //Get the associative part
				qi::repeat(_r1)[value_type]; //Get the dense part

	array_type = qi::omit[qi::byte_(array_marker)] >> //Get the marker
		qi::omit[U29_[_a = _1]] >> //Get the reference/value flag into a local variable
		( ( qi::eps((_a & 1)==0) >> qi::attr(_a>>1))[_val = _1] | //If it's a reference stop here 
		  ( qi::eps((_a & 1)==1)[_a=_a>>1] >> array_struct_value(_a)[_val = _1]) //Read the array
		);

	value_type = undefined_type[_val=_1] | null_type[_val=_1] | false_type[_val=_1] | true_type[_val=_1] | integer_type[_val=_1] | 
		/*double_type |*/ string_type[_val = _1] | /*date_type |*/ array_type[_val = _1];
	entry_rule %= *value_type;
}

std::ostream& lightspark::amf3::operator<<(std::ostream& o, const ValueType& r)
{
	switch(r.contents)
	{
		case ValueType::NONE:
			o << "NONE";
			break;
		case ValueType::NULLTYPE:
			o << "Null";
			break;
		case ValueType::UNDEFINEDTYPE:
			o << "Undefined";
			break;
		case ValueType::BOOL:
			o << "Bool: " << r.boolVal.val;
			break;
		case ValueType::INTEGER:
			o << "Integer: " << r.intVal.val;
			break;
		case ValueType::DOUBLE:
		//	o << "Double: " << r.doubleVal;
			break;
		case ValueType::UTF8STRING:
			o << "String: " << r.stringVal;
			break;
		case ValueType::DATE:
		//	o << "Date: " << r.dateVal;
			break;
		case ValueType::ARRAYTYPE:
			o << "Array:" << endl << r.arrayVal;
			break;
	}
	return o;
}

std::ostream& lightspark::amf3::operator<<(std::ostream& o, const boost::recursive_wrapper<Array>& w)
{
	const Array& r=w.get();
	o << "=========" << std::endl;
	o << "Associative array " << std::endl;
	for(uint32_t i=0;i<r.m_associativeSection.size();i++)
	{
		o << boost::fusion::at_c<0>(r.m_associativeSection[i]) << std::endl;
		o << boost::fusion::at_c<1>(r.m_associativeSection[i]) << endl;
	}
	o << "Dense array " << std::endl;
	for(uint32_t i=0;i<r.m_denseSection.size();i++)
	{
		o << r.m_denseSection[i] << endl;
	}
	o << "=========" << std::endl;
	return o;
}
