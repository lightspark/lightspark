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
	auto val = parseUInt29(data);
	return { !(val & 1), val >> 1 };
}

uint32_t Amf3Deserializer::parseUInt29(Span<const uint8_t>& data)
{
	uint32_t ret = 0;
	for (size_t i = 0; i < 3; ++i)
	{
		ret = (ret << 7) | data[0] & 0x7f;
		if (!(data.read() & 0x80))
			return ret;
	}

	return (ret << 8) | data.read();
}

int32_t Amf3Deserializer::parseInteger(Span<const uint8_t>& data)
{
	return int32_t(parseUInt29(data) << 3) >> 3;
}

number_t Amf3Deserializer::parseDouble(Span<const uint8_t>& data)
{
	return data.readBE<number_t>();
}

_R<AMF3Value> Amf3Deserializer::parseDate(Span<const uint8_t>& data)
{
	return parseRefOrVal
	(
		data,
		[] { return AMF3Date(); },
		[&](size_t size, size_t idx)
		{
			return _MR(new AMF3Value(parseDouble(data), {}));
		}
	);
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
	auto pair = readSize(data);
	if (!pair.first)
		return stringMap.at(pair.second);

	if (!pair.second)
		return "";

	stringMap.emplace_back(data.readBytes(pair.second));
	return stringMap.back();
}

_R<AMF3Value> Amf3Deserializer::parseArrayImpl
(
	Span<const uint8_t>& data,
	size_t size,
	size_t idx
)
{
	if (data.getSize() < size)
		throw AMFException("Size of `Array` is too large.");
	auto parseDenseElems = [&]
	{
		std::vector<_R<AMF3Value>> ret;
		ret.reserve(size);
		for (size_t i = 0; i < size; ++i)
			ret.emplace_back(parseValue(data));
		return ret;
	};

	auto arr = objMap.back()->tryAs<AMF3Array>();
	auto key = parseString(data);
	if (key.empty())
	{
		auto elems = parseDenseElems();
		assert_and_throw(arr.hasValue());
		return _MR(new AMF3Value(arr->id, elems));
	}

	std::vector<AMFElement> elems;
	for (; !key.empty(); key = parseString(data))
		elems.emplace_back(key, parseValue(data));

	auto denseElems = parseDenseElems();
	assert_and_throw(arr.hasValue());
	return _MR(new AMF3Value(arr->id, elems, denseElems));
}

_R<AMF3Value> Amf3Deserializer::parseArray(Span<const uint8_t>& data)
{
	return parseRefOrVal
	(
		data,
		[&] { return AMF3Array(++objId); },
		[&](size_t size, size_t idx)
		{
			return parseArrayImpl(data, size, idx);
		}
	);
}

static constexpr bool isVectorType(const AMF3TypeMarker& type)
{
	switch (type)
	{
		case AMF3TypeMarker::VectorInt:
		case AMF3TypeMarker::VectorUInt:
		case AMF3TypeMarker::VectorDouble:
		case AMF3TypeMarker::VectorObject:
			return true;
		default:
			return false;
	}
}

template<typename T>
_R<AMF3Value> Amf3Deserializer::parseVec
(
	Span<const uint8_t>& data,
	size_t size
)
{
	if (data.size() < size * sizeof(T))
		throw AMFException("Size of `Vector` is too large.");

	bool fixedLen = data.read<bool>();

	std::vector<T> vec;
	vec.reserve(size);
	for (size_t i = 0; i < size; ++i)
		vec.emplace_back(data.readBE<T>());

	return _MR(new AMF3Value(vec, fixedLen));
}

_R<AMF3Value> Amf3Deserializer::parseObjVec
(
	Span<const uint8_t>& data,
	size_t size
)
{
	auto objVec = objMap.back()->tryAs<AMF3ObjVector>();
	bool fixedLen = data.read<bool>();
	auto name = parseString(data);

	std::vector<_R<AMF3Value>> vec;
	vec.reserve(size);
	for (size_t i = 0; i < size; ++i)
		vec.emplace_back(parseValue(data));

	assert_and_throw(objVec.hasValue());
	return _MR(new AMF3Value(objVec->id, name, vec, fixedLen));
}

_R<AMF3Value> Amf3Deserializer::parseVector
(
	const AMF3TypeMarker& type,
	Span<const uint8_t>& data
)
{
	using Type = AMF3TypeMarker;

	assert_and_throw(isVectorType(type));
	auto makeVector = [&] -> AMF3Value
	{
		switch (type)
		{
			case Type::VectorInt: return AMF3IntVector();
			case Type::VectorUInt: return AMF3UIntVector();
			case Type::VectorDouble: return AMF3DoubleVector();
			case Type::VectorObject:
				return AMF3ObjVector(++objId);
		}
	};

	return parseRefOrVal(data, makeVector, [&](size_t size, size_t)
	{
		switch (type)
		{
			case Type::VectorInt:
				return parseVec<int32_t>(data, size);
			case Type::VectorUInt:
				return parseVec<uint32_t>(data, size);
			case Type::VectorDouble:
				return parseVec<number_t>(data, size);
			case Type::VectorObject:
				return parseObjVec(data, size);
		}
	});
}


_R<AMF3Value> Amf3Deserializer::parseDictionaryImpl
(
	Span<const uint8_t>& data,
	size_t size,
	size_t idx
)
{
	using DictPair = std::pair<_R<AMF3Value>, _R<AMF3Value>>;
	bool weakKeys = data.read<bool>();

	if (data.size() < size * 2)
		throw AMFException("Size of `Dictionary` is too large.");

	std::vector<DictPair> pairs;
	pairs.reserve(size * 2);
	for (size_t i = 0; i < size; ++i)
		pairs.emplace_back(parseValue(data), parseValue(data));

	auto dict = objMap.at(idx)->tryAs<AMF3Dict>();
	assert_and_throw(dict.hasValue());
	return _MR(new AMF3Value(dict->id, pairs, weakKeys));
}

_R<AMF3Value> Amf3Deserializer::parseDictionary(Span<const uint8_t>& data)
{
	return parseRefOrVal
	(
		data,
		[&] { return AMF3Dict(++objId); },
		[&](size_t size, size_t idx)
		{
			return parseDictionaryImpl(data, size, idx);
		}
	);
}

_R<AMF3Value> Amf3Deserializer::parseByteArray(Span<const uint8_t>& data)
{
	using ByteVector = std::vector<uint8_t>;
	return parseRefOrVal
	(
		data,
		[] { return ByteVector {}; },
		[&](size_t size, size_t idx)
		{
			return _MR(new AMF3Value(data.readBytes(size)));
		}
	);
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
	);
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

const TraitsRef& Amf3Deserializer::parseTraits
(
	Span<const uint8_t>& data,
	size_t size,
	size_t idx
)
{
	if (!(size & 1))
		return traitsMap.at(size >> 1);

	size >>= 1;
	auto name = parseString(data);
	auto attrCount = size >> 2;
	bool isExternal = size & 1;
	bool isDynamic = size & 2;

	std::vector<tiny_string> staticProps;
	staticProps.reserve(attrCount);
	for (size_t i = 0; i < attrCount; ++i)
		staticProps.push_back(parseString(data));
	traitsMap.emplace_back(name, staticProps, isExternal, isDynamic);
	return traitsMap.back();
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

void Amf3Serializer::writeUInt29(IAMFWriter& writer, uint32_t val)
{
}

void Amf3Serializer::writeInt(IAMFWriter& writer, int32_t val)
{
}

void Amf3Serializer::writeBool(IAMFWriter& writer, bool val)
{
}

void Amf3Serializer::writeDouble(IAMFWriter& writer, number_t val)
{
}

void Amf3Serializer::writeStringVal
(
	IAMFWriter& writer,
	const tiny_string& str
)
{
}

void Amf3Serializer::writeString
(
	IAMFWriter& writer,
	const tiny_string& str
)
{
}

void Amf3Serializer::writeDate(IAMFWriter& writer, size_t id, number_t time)
{
}

void Amf3Serializer::writeArray
(
	IAMFWriter& writer,
	size_t id,
	Span<const AMFElement> elems,
	Span<const AMF3Value> denseElems
)
{
}

void Amf3Serializer::writeObject
(
	IAMFWriter& writer,
	size_t id,
	Span<const AMFElement> props,
	Span<const AMFElement> customProps
)
{
}

void Amf3Serializer::writeXML
(
	IAMFWriter& writer,
	const tiny_string& str,
	bool isStr
)
{
}

void Amf3Serializer::writeByteArray
(
	IAMFWriter& writer,
	Span<const uint8_t> data
)
{
}

template<typename T>
void Amf3Serializer::writeVector
(
	IAMFWriter& writer,
	size_t id,
	Span<const T> elems,
	bool fixedLen
)
{
}

void Amf3Serializer::writeDictionary
(
	IAMFWriter& writer,
	size_t id,
	Span<const DictPair> elems,
	bool weakKeys
)
{
}

void Amf3Serializer::writeRef
(
	IAMFWriter& writer,
	const AMF3TypeMarker& type,
	size_t ref
)
{
}

void Amf3Serializer::writeTraits
(
	IAMFWriter& writer,
	const TraitsRef& traits
)
{
}

void Amf3Serializer::writeTraitRef
(
	IAMFWriter& writer,
	const TraitsRef& traits
)
{
}

void Amf3Serializer::writeElem
(
	IAMFWriter& writer,
	const tiny_string& name,
	const AMF3Value& val
)
{
	writeString(writer, name);
	writeValue(writer, val);
}

void Amf3Serializer::writeValue(IAMFWriter& writer, const AMF3Value& val)
{
	using Type = AMF3TypeMarker;
	val.visit(makeVisitor
	(
		[&](AMF3Undefined) { writer.writeUInt8(Type::Undefined); },
		[&](AMF3Null) { writer.writeUInt8(Type::Null); },
		[&](bool flag) { writeBool(writer, flag); },
		[&](int32_t val) { writeInt(writer, val); },
		[&](number_t num) { writeNumber(writer, num); },
		[&](const tiny_string& str) { writeStringVal(writer, str); },
		[&](const AMF3Date& date)
		{
			writeDate(writer, date.id, date.date);
		},
		[&](const AMF3Array& arr)
		{
			auto pair = objMap.toLengthAdd(val);
			if (pair.first)
			{
				refMap.emplace(arr.id,
				{
					Type::Array,
					pair.second
				});
			}

			writeArray(writer, arr.elems, arr.denseElems);
		},
		[&](const AMF3DenseArray& arr)
		{
			auto pair = objMap.toLengthAdd(val);
			if (pair.first)
			{
				refMap.emplace(arr.id,
				{
					Type::Array,
					pair.second
				});
			}

			writeArray(writer, {}, arr.elems);
		},
		[&](const AMF3Object& obj)
		{
			writeObject(writer, obj.id, obj.traits, obj.props);
		},
		[&](const AMF3CustomObject& obj)
		{
			writeObject
			(
				writer,
				obj.id,
				obj.traits,
				obj.props,
				obj.customProps
			);
		},
		[&](const AMF3XML& xml)
		{
			writeXML(writer, xml.data, xml.isStr);
		},
		[&](const AMF3IntVector& vec)
		{
			auto span = makeSpan(vec.elems);
			writeVector(writer, span, vec.fixedLen);
		},
		[&](const AMF3UIntVector& vec)
		{
			auto span = makeSpan(vec.elems);
			writeVector(writer, span, vec.fixedLen);
		},
		[&](const AMF3DoubleVector& vec)
		{
			auto span = makeSpan(vec.elems);
			writeVector(writer, span, vec.fixedLen);
		},
		[&](const AMF3ObjVector& vec)
		{
			auto span = makeSpan(vec.elems);
			writeObjVector(writer, vec.id, span, vec.fixedLen);
		},
		[&](Span<const uint8_t> data)
		{
			writeByteArray(writer, data);
		},
		[&](size_t ref)
		{
			const auto& pair = *refMap.nth(ref);
			writeRef(writer, pair.first, pair.second);
		}
	));
}

void Amf3Serializer::writeLSOBody(IAMFWriter& writer, const LSO& lso)
{
	for (const auto& elem : lso.getBody())
	{
		writeElem(writer, elem.first, elem.second);
		writer.write(0);
	}
}
