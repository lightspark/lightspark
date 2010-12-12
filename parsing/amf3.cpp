#include <boost/spirit/include/qi.hpp>

#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/tuple.hpp>
#include <iostream>
#include <string>
#include <fstream>
#include <map>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phoenix = boost::phoenix;

template<class T>
class ReferenceOrValue
{
template<class Y> friend std::ostream& operator<<(std::ostream& o, const ReferenceOrValue<Y>& r);
private:
	enum CONTENTS { NONE=0, REFERENCE, VALUE };
	CONTENTS contents;
public:
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

typedef ReferenceOrValue<std::string> Utf8String;
typedef ReferenceOrValue<uint64_t> Date;

struct NullType
{
};

struct UndefinedType
{
};

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

struct Array;
typedef ReferenceOrValue<boost::recursive_wrapper<Array> > ArrayType;

class ValueType
{
friend std::ostream& operator<<(std::ostream& o, const ValueType& r);
private:
	enum CONTENTS { NONE=0, NULLTYPE, UNDEFINEDTYPE,BOOL,INTEGER,DOUBLE,UTF8STRING,DATE,ARRAYTYPE };
	CONTENTS contents;
public:
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

BOOST_FUSION_ADAPT_STRUCT(
	Array,
	(Array::AssociativeArray, m_associativeSection)
	(std::vector<ValueType>, m_denseSection)
)

template<typename Iterator>
struct AmfParserGrammar : qi::grammar<Iterator,std::vector<ValueType>()>
{
	qi::rule<Iterator,uint8_t(uint8_t,uint8_t)> byterange; //Parse a byte in a range;
	qi::rule<Iterator,uint32_t(),qi::locals<uint32_t,uint32_t> > U29_;
	qi::rule<Iterator,uint64_t()> DOUBLE_;

	/** Unused **/
	qi::rule<Iterator,uint32_t()> UTF8_char_;

	qi::rule<Iterator> UTF8_empty_;
	qi::rule<Iterator,std::string(uint32_t),qi::locals<uint32_t> > UTF8_string_;
	qi::rule<Iterator,Utf8String(bool),qi::locals<uint32_t> > UTF8_vr_;

	qi::rule<Iterator,uint64_t()> date_time_;

	qi::rule<Iterator,NameAndValue() > assoc_value_;
	
	qi::rule<Iterator,UndefinedType()> undefined_type;
	qi::rule<Iterator,NullType()> null_type;
	qi::rule<Iterator,Bool()> false_type;
	qi::rule<Iterator,Bool()> true_type;
	qi::rule<Iterator,Integer()> integer_type;
	qi::rule<Iterator,Double()> double_type;
	qi::rule<Iterator,Utf8String()> string_type;
	qi::rule<Iterator,std::string()> XMLDocument_type;
	qi::rule<Iterator,Date(),qi::locals<uint32_t> > date_type;
	qi::rule<Iterator,ArrayType(),qi::locals<uint32_t> > array_type;
	qi::rule<Iterator,ValueType()> value_type;
	qi::rule<Iterator,std::vector<ValueType>() > entry_rule;
	qi::rule<Iterator,Array(uint32_t)> array_stuct_value;

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

	AmfParserGrammar() : AmfParserGrammar::base_type(entry_rule)
	{
		using namespace qi::labels;
		using qi::_pass;

		U29_ = (qi::byte_[_a = _1,_b = _a&0x7f] >> 
			-( qi::eps(_a&0x80) >> qi::byte_[_a = _1,_b = (_b<<7)|(_a&0x7f)] >>
			-( qi::eps(_a&0x80) >> qi::byte_[_a = _1,_b = (_b<<7)|(_a&0x7f)] >>
			-( qi::eps(_a&0x80) >> qi::byte_[_a = _1,_b = (_b<<7)|_a]))))[_val = _b];
		DOUBLE_ = qi::big_qword;

		UTF8_char_ = qi::byte_;

		UTF8_empty_ = qi::byte_(0x01);
		UTF8_string_ %= qi::omit[qi::eps[phoenix::bind(&std::string::reserve,_val,_r1+1)]] >> (qi::repeat(_r1)[qi::char_]);
		//The argument signals if 0x01 (empty string) is legal or not
		UTF8_vr_ = qi::omit[U29_[_a = _1]] >>
			( ( qi::eps((_a & 1)==0) >> qi::attr(_a>>1))[_val = _1] | //If it's a reference stop here 
			  ( qi::eps((_a & 1)==1)[_pass=(!_r1 || _a!=0x1),_a=_a>>1] >> (UTF8_string_(_a)))[_val = _1]); //Read a string

		date_time_ %= DOUBLE_;

		assoc_value_ %= UTF8_vr_(true) >> value_type;
		
		undefined_type = qi::byte_(undefined_marker) [_val = boost::phoenix::construct<UndefinedType>()];
		null_type = qi::byte_(null_marker) [_val = boost::phoenix::construct<NullType>()];
		false_type = qi::byte_(false_marker)[_val = boost::phoenix::construct<Bool>(false)];
		true_type = qi::byte_(true_marker)[_val = boost::phoenix::construct<Bool>(true)];
		integer_type = (qi::omit[qi::byte_(integer_marker)] >> U29_)[_val = boost::phoenix::construct<Integer>(_1)];
		double_type = (qi::omit[qi::byte_(double_marker)] >>  DOUBLE_)[_val = boost::phoenix::construct<Double>(_1)];
		string_type %= qi::omit[qi::byte_(string_marker)] >> UTF8_vr_(false);
		date_type = qi::omit[qi::byte_(date_marker)] >> qi::omit[U29_[_a = _1]] >>
			( ( qi::eps((_a & 1)==0) >> qi::attr(_a>>1))[_val = _1] | //If it's a reference stop here 
			  ( qi::eps((_a & 1)==1) >> date_time_)); //Read a string

		array_stuct_value %= *assoc_value_ >> qi::omit[UTF8_empty_] >> //Get the associative part
					qi::repeat(_r1)[value_type]; //Get the dense part

		array_type = qi::omit[qi::byte_(array_marker)] >> //Get the marker
			qi::omit[U29_[_a = _1]] >> //Get the reference/value flag into a local variable
			( ( qi::eps((_a & 1)==0) >> qi::attr(_a>>1))[_val = _1] | //If it's a reference stop here 
			  ( qi::eps((_a & 1)==1)[_a=_a>>1] >> array_stuct_value(_a)[_val = _1]) //Read the array
			);

		value_type = undefined_type[_val=_1] | null_type[_val=_1] | false_type[_val=_1] | true_type[_val=_1] | integer_type[_val=_1] | 
			/*double_type |*/ string_type[_val = _1] | /*date_type |*/ array_type[_val = _1];
		entry_rule %= *value_type;
	}
};

using namespace std;

std::ostream& operator<<(std::ostream& o, const ValueType& r)
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
			//o << "Date: " << r.dateVal;
			break;
		case ValueType::ARRAYTYPE:
			o << "Array:" << endl << r.arrayVal;
			break;
	}
	return o;
}

std::ostream& operator<<(std::ostream& o, const boost::recursive_wrapper<Array>& w)
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
}

int main(int argc, char **argv) {
	std::ifstream input ("amf3.bin");
	input.unsetf(std::ios::skipws);
	assert(input);
	char buf[4096];
	input.read(buf,4096);
	int available=input.gcount();
	assert(available<4096);
	input.close();
	char* start=buf;
	char* end=buf+available;

	std::vector<ValueType> ans;
	AmfParserGrammar<char*> theParser;

	bool r = qi::parse(start, end, theParser,ans);
 	assert(r);
// 	assert(init == end);

	for (int i=0;i < ans.size();++i)
	{
		cout << ans[i] << endl;
	}
	
    return 0;
}
