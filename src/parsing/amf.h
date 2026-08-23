/**************************************************************************
    Lightspark, a free flash player implementation

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

#ifndef PARSING_AMF_H
#define PARSING_AMF_H 1

#include <cstdint>
#include <vector>

#include "smartrefs.h"
#include "tiny_string.h"
#include "utils/span.h"

namespace lightspark
{

class AMFValue;
class AMF0Value;
class AMF3Value;

template<typename T = AMFValue>;
using AMFElement = std::pair<tiny_string, T>;
using AMF0Element = AMFElement<AMF0Value>;
using AMF3Element = AMFElement<AMF3Value>;

enum class AMF0TypeMarker : uint8_t
{
	Number = 0x00,
	Boolean = 0x01,
	String = 0x02,
	Object = 0x03,
	MovieClip = 0x04,
	Null = 0x05,
	Undefined = 0x06,
	Reference = 0x07,
	ECMAArray = 0x08,
	ObjectEnd = 0x09,
	StrictArray = 0x0A,
	Date = 0x0B,
	LongString = 0x0C,
	Unsupported = 0x0D,
	RecordSet = 0x0E,
	XML = 0x0F,
	TypedObject = 0x10,
	AMF3 = 0x11,
};

// Used for visiting.
struct AMF0Null {};
struct AMF0Undefined {};
struct AMF0Unsupported {};
struct AMF0Object
{
	Span<const AMFElement> elems;
};

struct AMF0ECMAArray
{
	size_t size;
	std::vector<AMFElement> elems;

	AMF0ECMAArray
	(
		Span<const AMFElement> _elems,
		size_t _size
	) : size(_size), elems(_elems.begin(), _elems.end()) {}
};

struct AMF0Date
{
	number_t date;
	uint16_t timeZone;
};

struct AMF0XML
{
	tiny_string data;
};

struct AMF0TypedObject
{
	tiny_string name;
	std::vector<AMFElement> elems;

	AMF0TypedObject
	(
		const tiny_string& _name
		Span<const AMFElement> _elems
	) : name(_name), elems(_elems.begin(), _elems.end()) {}
};

class AMF0Value
{
public:
	enum class Type
	{
		Number,
		Bool,
		String,
		Object,
		Null,
		Undefined,
		Ref,
		ECMAArray,
		StrictArray,
		Date,
		Unsupported,
		XML,
		TypedObject,
		AMF3,
	};
private:
	Type type;
	union
	{
		number_t num;
		bool _bool;
		tiny_string str;
		std::vector<AMFElement> obj;
		uint16_t ref;
		AMF0ECMAArray arr;
		std::vector<AMF0Value> strictArr;
		AMF0Date date;
		AMF0TypedObject typedObj;
		_R<AMF3Value> amf3Val;
	};
public:
	AMF0Value(const AMF0Value& other);
	AMF0Value(const Type& _type) : type(_type) {}
	AMF0Value(number_t val) : type(Type::Number), num(val) {}
	AMF0Value(bool flag) : type(Type::Bool), _bool(flag) {}
	AMF0Value(const tiny_string& _str) :
	type(Type::String),
	str(_str) {}

	AMF0Value(Span<const AMFElement> _obj) :
	type(Type::Object),
	obj(_obj.begin(), _obj.end()) {}

	AMF0Value(uint16_t _ref) : type(Type::Ref), ref(_ref) {}
	AMF0Value
	(
		Span<const AMFElement> elems,
		size_t size
	) : type(Type::ECMAArray), arr(elems, size) {}

	AMF0Value(Span<const AMF0Value> elems) :
	type(Type::StrictArray),
	strictArr(elems.begin(), elems.end()) {}

	AMF0Value
	(
		number_t ms,
		uint16_t timeZone
	) : type(Type::Date), date(ms, timeZone) {}

	AMF0Value(const AMF0XML& xml) : type(Type::XML), str(xml.data) {}
	AMF0Value
	(
		const tiny_string& name,
		Span<const AMFElement> elems
	) : type(Type::TypedObject), typedObj(name, elems) {}

	AMF0Value(_R<AMF3Value> val) : type(Type::AMF3), amf3Val(val) {}

	~AMF0Value();

	const Type& getType() const { return type; }
	template<typename V>
	constexpr auto visit(V&& visitor) const;
};

enum class AMF3TypeMarker : uint8_t
{
	Undefined = 0x00,
	Null = 0x01,
	False = 0x02,
	True = 0x03,
	Integer = 0x04,
	Double = 0x05,
	String = 0x06,
	XMLDoc = 0x07,
	Date = 0x08,
	Array = 0x09,
	Object = 0x0A,
	XML = 0x0B,
	ByteArray = 0x0C,
	VectorInt = 0x0D,
	VectorUInt = 0x0E,
	VectorDouble = 0x0F,
	VectorObject = 0x10,
	Dictionary = 0x11,
};

// Used for visiting.
struct AMF3Null {};
struct AMF3Undefined {};
struct AMF3XML
{
	tiny_string data;
	bool isStr;
};

struct AMF3Date
{
	number_t date;
};

struct AMF3Array
{
	size_t id;
	std::vector<AMFElement> elems;
	std::vector<AMF3Value> denseElems;

	AMF3Array
	(
		size_t _id,
		Span<const AMFElement> _elems,
		Span<const AMF3Value> _denseElems
	) :
	id(_id),
	elems(_elems.begin(), _elems.end()),
	denseElems(_denseElems.begin(), _denseElems.end()) {}
};

struct AMF3DenseArray
{
	size_t id;
	std::vector<AMF3Value> elems;

	AMF3Array
	(
		size_t _id,
		Span<const AMF3Value> _elems
	) : id(_id), elems(_elems.begin(), _elems.end()) {}
};

struct AMF3Object
{
	size_t id;
	TraitsRef traits;
	std::vector<AMFElement> props;

	AMF3Object
	(
		size_t _id,
		const TraitsRef& _traits,
		Span<const AMFElement> _props
	) :
	id(_id),
	traits(_traits),
	props(_props.begin(), _props.end()) {}
};

struct AMF3CustomObject : AMF3Object
{
	std::vector<AMFElement> customProps;

	AMF3Object
	(
		size_t _id,
		const TraitsRef& _traits,
		Span<const AMFElement> _props,
		Span<const AMFElement> _customProps,
	) : AMF3Object(_id, _traits, _props), customProps
	(
		_customProps.begin(),
		_customProps.end()
	) {}
};

template<typename T>
struct AMF3Vec
{
	static constexpr AMF3TypeMarker typeMarker = getTypeMarker();

	std::vector<T> elems;
	bool fixedLen;

	AMF3Vec(Span<const T> _elems, bool _fixedLen) : elems
	(
		_elems.begin(),
		_elems.end()
	), fixedLen(_fixedLen) {}

	static constexpr AMF3TypeMarker getTypeMarker();
};

template<>
struct AMF3Vec<AMF3Value>
{
	static constexpr AMF3TypeMarker typeMarker = getTypeMarker();

	size_t id;
	std::vector<_R<AMF3Value>> elems;
	bool fixedLen;

	AMF3Vec
	(
		size_t _id,
		Span<const _R<AMF3Value>> _elems,
		bool _fixedLen
	) :
	id(_id),
	elems(_elems.begin(), _elems.end()),
	fixedLen(_fixedLen) {}

	static constexpr AMF3TypeMarker getTypeMarker()
	{
		return AMF3TypeMarker::VectorObject;
	}
};

template<>
constexpr AMF3TypeMarker AMF3Vec<int32_t>::getTypeMarker()
{
	return AMF3TypeMarker::VectorInt;
}

template<>
constexpr AMF3TypeMarker AMF3Vec<uint32_t>::getTypeMarker()
{
	return AMF3TypeMarker::VectorUInt;
}

template<>
constexpr AMF3TypeMarker AMF3Vec<number_t>::getTypeMarker()
{
	return AMF3TypeMarker::VectorDouble;
}

using AMF3IntVector = AMF3Vec<int32_t>;
using AMF3UIntVector = AMF3Vec<uint32_t>;
using AMF3NumVector = AMF3Vec<number_t>;
using AMF3ObjVector = AMF3Vec<AMF3Value>;

struct AMF3Dict
{
	using DictPair = std::pair<_R<AMF3Value>, _R<AMF3Value>>;

	size_t id;
	std::vector<DictPair> elems;
	bool weakKeys;

	AMFDict
	(
		size_t _id,
		Span<const DictPair> _elems,
		bool _weakKeys
	) :
	id(_id),
	elems(_elems.begin(), _elems.end()),
	weakKeys(_weakKeys) {}
};

class AMF3Value : public RefCountable
{
public:
	enum class Type
	{
		Undefined,
		Null,
		Bool,
		Int,
		Number,
		String,
		XMLDoc,
		Date,
		Array,
		DenseArray,
		Object,
		CustomObj,
		XML,
		ByteArray,
		VectorInt,
		VectorUInt,
		VectorNumber,
		VectorObject,
		Dict,
		Ref,
	};
private:
	Type type;
	union
	{
		bool _bool;
		int32_t _int;
		number_t num;
		tiny_string str;
		AMF3Array arr;
		AMF3DenseArray denseArr;
		AMF3Object obj;
		AMF3CustomObject customObj;
		std::vector<uint8_t> byteArr;
		AMF3Vec<int32_t> intVec;
		AMF3Vec<uint32_t> uintVec;
		AMF3Vec<number_t> numVec;
		AMF3Vec<AMF3Value> objVec;
		AMF3Dict dict;
		size_t ref;
	};
public:
	AMF3Value(const AMF3Value& other);
	AMF3Value(const Type& _type) : type(_type) {}
	AMF3Value(bool flag) : type(Type::Bool), _bool(flag) {}
	AMF3Value(int32_t val) : type(Type::Int), _int(val) {}
	AMF3Value(number_t val) : type(Type::Number), num(val) {}
	AMF3Value(const tiny_string& _str) :
	type(Type::String),
	str(_str) {}

	AMF3Value
	(
		const tiny_string& _str,
		bool isStr
	) : type(isStr ? Type::XML : Type::XMLDoc), str(_str) {}

	AMF3Value(const AMF3Date& date) :
	type(Type::Date),
	num(date.date) {}

	AMF3Value
	(
		size_t id,
		Span<const AMFElement> elems,
		Span<const AMF3Value> denseElems
	) : type(Type::Array), arr(id, elems, denseElems) {}

	AMF3Value
	(
		size_t id,
		Span<const AMF3Value> elems
	) : type(Type::DenseArray), denseArr(id, elems) {}

	AMF3Value
	(
		size_t id,
		const TraitRef& traits,
		Span<const AMFElement> props
	) : type(Type::Object), obj(id, traits, props) {}

	AMF3Value
	(
		size_t id,
		const TraitRef& traits,
		Span<const AMFElement> props,
		Span<const AMFElement> customProps
	) :
	type(Type::CustomObj),
	customObj(id, traits, props, customProps) {}

	AMF3Value(Span<const uint8_t> data) :
	type(Type::ByteArray),
	byteArr(data.begin(), data.end()) {}

	AMF3Value
	(
		Span<const int32_t> elems,
		bool fixedLen
	) : type(Type::VectorInt), intVec(elems, fixedLen) {}

	AMF3Value
	(
		Span<const uint32_t> elems,
		bool fixedLen
	) : type(Type::VectorUInt), uintVec(elems, fixedLen) {}

	AMF3Value
	(
		Span<const number_t> elems,
		bool fixedLen
	) : type(Type::VectorNumber), numVec(elems, fixedLen) {}

	AMF3Value
	(
		Span<const _R<AMF3Value>> elems,
		bool fixedLen
	) : type(Type::VectorObject), objVec(id, elems, fixedLen) {}

	AMF3Value
	(
		size_t id,
		Span<const AMF3Dict::DictPair> elems,
		bool weakKeys
	) : type(Type::Dict), dict(id, elems, weakKeys) {}

	AMF3Value(size_t _ref) : type(Type::Ref), ref(_ref) {}

	~AMF3Value();

	const Type& getType() const { return type; }
	template<typename V>
	constexpr auto visit(V&& visitor) const;
};

class AMFValue
{
private:
	bool _isAMF3;
	union
	{
		AMF0Value amf0Val;
		_R<AMF3Value> amf3Val;
	};
public:
	AMFValue(const AMF0Value& val) : _isAMF3(false), amf0Val(val) {}
	AMFValue(const _R<AMF3Value>& val) : _isAMF3(true), amf3Val(val) {}
	AMFValue(const AMFValue& other) : _isAMF3(other._isAMF3)
	{
		if (_isAMF3)
			new (&amf3Val) _R<AMF3Value>(other.amf3Val);
		else
			new (&amf0Val) AMF0Value(other.amf0Val);
	}

	~AMFValue()
	{
		if (_isAMF3)
			amf3Val.~_R();
		else
			amf0Val.~AMF0Value();
	}

	constexpr bool isAMF3() const { return _isAMF3; }
	template<typename V>
	constexpr auto visit(V&& visitor) const
	{
		return _isAMF3 ? visitor(*amf3Val) : visitor(amf0Val);
	}
};

AMF0Value::AMF0Value(const AMF0Value& other) : type(other.type)
{
	switch (type)
	{
		case Type::Number: num = other.num; break;
		case Type::Bool: _bool = other._bool; break;
		case Type::XML:
		case Type::String: new (&str) tiny_string(other.str); break;
		case Type::Object:
			new (&obj) std::vector<AMF0Element>(other.obj);
			break;
		case Type::Ref: ref = other.ref;
		case Type::ECMAArray:
			new (&arr) AMF0ECMAArray(other.arr);
			break;
		case Type::StrictArray:
			new (&strictArr) std::vector
			<
				AMF0Value
			>(other.strictArr);
			break;
		case Type::Date: date = other.date;
		case Type::TypedObject:
			new (&typedObj) AMF0TypedObject(other.typedObj);
			break;
		case Type::AMF3:
			new (&amf3Val) _R<AMF3Value>(other.amf3Val);
			break;
	}
}

AMF0Value::~AMF0Value()
{
	switch (type)
	{
		case Type::XML:
		case Type::String: str.~tiny_string(); break;
		case Type::Object: obj.~vector; break;
		case Type::ECMAArray: arr.~AMF0ECMAArray(); break;
		case Type::StrictArray: strictArr.~vector; break;
		case Type::TypedObject: typedObj.~AMF0TypedObject(); break;
		case Type::AMF3: amf3Val.~_R(); break;
	}
}

template<typename V>
constexpr auto AMF0Value::visit(V&& visitor) const
{
	switch (type)
	{
		case Type::Number: return visitor(num);
		case Type::Bool: return visitor(_bool);
		case Type::String: return visitor(str);
		case Type::Object: return visitor(makeSpan(obj));
		case Type::Null: return visitor(AMF0Null());
		case Type::Undefined: return visitor(AMF0Undefined());
		case Type::Ref: return visitor(ref);
		case Type::ECMAArray: return visitor(arr);
		case Type::StrictArray: return visitor(makeSpan(strictArr));
		case Type::Date: return visitor(date);
		case Type::Unsupported: return visitor(AMF0Unsupported());
		case Type::XML: return visitor(AMF0XML(str));
		case Type::TypedObject: return visitor(typedObj);
		case Type::AMF3: return visitor(*amf3Val);
	}
}

AMF3Value::AMF3Value(const AMF3Value& other) : type(other.type)
{
	switch (type)
	{
		case Type::Bool: _bool = other._bool; break;
		case Type::Int: _int = other._int; break;
		case Type::Date:
		case Type::Number: num = other.num; break;
		case Type::XMLDoc:
		case Type::XML:
		case Type::String: new (&str) tiny_string(other.str); break;
		case Type::Array: new (&arr) AMF3Array(other.arr); break;
		case Type::DenseArray:
			new (&denseArr) AMF3DenseArray(other.denseArr);
			break;
		case Type::Object: new (&obj) AMF3Object(other.obj); break;
		case Type::CustomObj:
			new (&customObj) AMF3CustomObject(other.customObj);
			break;
		case Type::ByteArray:
			new (&byteArr) std::vector<uint8_t>(other.byteArr);
			break;
		case Type::VectorInt:
			new (&intVec) AMF3Vec<int32_t>(other.intVec);
			break;
		case Type::VectorUInt: uintVec.~AMF3Vec<uint32_t>(); break;
			new (&uintVec) AMF3Vec<uint32_t>(other.uintVec);
			break;
		case Type::VectorNumber:
			new (&numVec) AMF3Vec<number_t>(other.numVec);
			break;
		case Type::VectorObject:
			new (&objVec) AMF3Vec<AMF3Value>(other.objVec);
			break;
		case Type::Dict: new (&dict) AMF3Dict(other.dict); break;
	}
}

AMF3Value::~AMF3Value()
{
	switch (type)
	{
		case Type::XMLDoc:
		case Type::XML:
		case Type::String: str.~tiny_string(); break;
		case Type::Array: arr.~AMF3Array(); break;
		case Type::DenseArray: denseArr.~AMF3DenseArray(); break;
		case Type::Object: obj.~AMF3Object(); break;
		case Type::CustomObj: customObj.~AMF3CustomObject(); break;
		case Type::ByteArray: byteArr.~vector(); break;
		case Type::VectorInt: intVec.~AMF3Vec(); break;
		case Type::VectorUInt: uintVec.~AMF3Vec(); break;
		case Type::VectorNumber: numVec.~AMF3Vec(); break;
		case Type::VectorObject: objVec.~AMF3Vec(); break;
		case Type::Dict: dict.~AMF3Dict(); break;
	}

}

template<typename V>
constexpr auto AMF3Value::visit(V&& visitor) const
{
	switch (type)
	{
		case Type::Undefined: return visitor(AMF3Undefined());
		case Type::Null: return visitor(AMF3Null());
		case Type::Bool: return visitor(_bool);
		case Type::Int: return visitor(_int);
		case Type::Number: return visitor(num);
		case Type::String: return visitor(str);
		case Type::XMLDoc:
		case Type::XML:
			return visitor(AMF3XML(str, type == Type::XML));
		case Type::Date: return visitor(AMF3Date(num));
		case Type::Array: return visitor(arr);
		case Type::DenseArray: return visitor(denseArr);
		case Type::Object: return visitor(obj);
		case Type::CustomObj: return visitor(customObj);
		case Type::ByteArray: return visitor(byteArr);
		case Type::VectorInt: return visitor(intVec);
		case Type::VectorUInt: return visitor(uintVec);
		case Type::VectorNumber: return visitor(numVec);
		case Type::VectorObject: return visitor(objVec);
		case Type::Dict: return visitor(dict);
		case Type::Ref: return visitor(ref);
	}
}

}
#endif /* PARSING_AMF_H */
