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

#include "parsing/amf0_generator.h"
#include "parsing/lso.h"
#include "utils/visitor.h"

using namespace lightspark;

// Based on Ruffle's `rust-flash-lso` crate.

AMF0Value Amf0Deserializer::parseNumber(Span<const uint8_t>& data)
{
	return AMF0Value(data.readBE<number_t>());
}

AMF0Value Amf0Deserializer::parseBool(Span<const uint8_t>& data)
{
	return AMF0Value(data.read() != 0);
}

AMF0Value Amf0Deserializer::parseStringVal(Span<const uint8_t>& data)
{
	return AMF0Value(parseString(data));
}

AMF0Value Amf0Deserializer::parseObject(Span<const uint8_t>& data)
{
	return AMF0Value(AMF0Object(parseArrayElem(data)));
}

AMF0Value Amf0Deserializer::parseRef(Span<const uint8_t>& data)
{
	return AMF0Ref(data.readBE<uint16_t>());
}

AMF0Value Amf0Deserializer::parseECMAArray(Span<const uint8_t>& data)
{

	size_t size = data.readBE<uint32_t>();
	return AMF0Value(parseArrayElem(data), size);
}

AMF0Value Amf0Deserializer::parseStrictArray(Span<const uint8_t>& data)
{
	size_t size = data.readBE<uint32_t>();

	if (data.getSize() < size)
		throw AMFException("Size of `StrictArray` is too large.");

	std::vector<AMF0Value> arr;
	arr.reserve(size);
	for (size_t i = 0; i < size; ++i)
		arr.push_back(parseValue(data));

	return AMF0Value(arr);
}

AMF0Value Amf0Deserializer::parseDate(Span<const uint8_t>& data)
{
	auto ms = data.readBE<number_t>();
	auto timeZone = data.readBE<uint16_t>();
	return AMF0Value(ms, timeZone);
}

tiny_string Amf0Deserializer::parseLongStringImpl(Span<const uint8_t>& data)
{
	size_t size = data.readBE<uint32_t>();
	return data.readBytes(size);
}

AMF0Value Amf0Deserializer::parseLongString(Span<const uint8_t>& data)
{
	return AMF0Value(parseLongStringImpl(data));
}

AMF0Value Amf0Deserializer::parseXML(Span<const uint8_t>& data)
{
	return AMF0Value(AMF0XML(parseLongStringImpl(data)));
}

AMF0Value Amf0Deserializer::parseTypedObject(Span<const uint8_t>& data)
{
	auto name = parseString(data);
	return AMF0Value(name, parseArrayElem(data));
}

AMFValue Amf0Deserializer::parseAMF3Value(Span<const uint8_t>& data)
{
	return amf3Deserializer.parseValue(data);
}

AMFValue Amf0Deserializer::parseValue(Span<const uint8_t>& data)
{
	auto type = data.read<AMF0TypeMarker>();
	switch (type)
	{
		case AMF0TypeMarker::Number: return parseNumber(data);
		case AMF0TypeMarker::Bool: return parseBool(data);
		case AMF0TypeMarker::String: return parseString(data);
		case AMF0TypeMarker::Object: return parseObject(data);
		case AMF0TypeMarker::Null: return AMF0Value::nullVal;
		case AMF0TypeMarker::Undefined: return AMF0Value::undefinedVal;
		case AMF0TypeMarker::Reference: return parseRef(data);
		case AMF0TypeMarker::ECMAArray: return parseECMAArray(data);
		case AMF0TypeMarker::StrictArray: return parseStrictArray(data);
		case AMF0TypeMarker::Date: return parseDate(data);
		case AMF0TypeMarker::LongString: return parseLongString(data);
		case AMF0TypeMarker::XML: return parseXML(data);
		case AMF0TypeMarker::TypedObject: return parseTypedObject(data);
		case AMF0TypeMarker::AMF3: return parseAMF3(data);
		default:
			throw AMFException("Unsupported type");
			break;
	}
}

AMFElement Amf0Deserializer::parseElement(Span<const uint8_t>& data)
{
	auto name = parseString(data);
	return AMFElement(name, parseValue(data));
}

std::vector<AMFElement> Amf0Deserializer::parseBody(Span<const uint8_t>& data)
{
	std::vector<AMFElement> ret;
	while (!data.empty())
	{
		ret.push_back(parseElement(data));
		(void)data.read();
	}
	return ret;
}

tiny_string Amf0Deserializer::parseString(Span<const uint8_t>& data)
{
	size_t size = data.readBE<uint16_t>();
	return data.readBytes(size);
}

void Amf0SerializerBase::writeNumber(const tiny_string& name, number_t val)
{
	addElem(name, AMF0Value(val));
}

void Amf0SerializerBase::writeBool(const tiny_string& name, bool val)
{
	addElem(name, AMF0Value(val));
}

void Amf0SerializerBase::writeString(const tiny_string& name, const tiny_string& str)
{
	addElem(name, AMF0Value(str));
}

WriteObjType Amf0Serializer::writeObject
(
	const tiny_string& name,
	void* obj
)
{
	auto ref = tryGetRef(obj);
	if (ref.hasValue())
		return WriteObjType({}, ref);
	auto _ref = makeRef();
	addRef(obj, _ref);
	return WriteObjType(*this, _ref);
}

void Amf0SerializerBase::writeNull(const tiny_string& name)
{
	addElem(name, AMF0Value::nullVal);
}

void Amf0SerializerBase::writeUndefined(const tiny_string& name)
{
	addElem(name, AMF0Value::undefinedVal);
}

void Amf0SerializerBase::writeRef(const tiny_string& name, uint16_t ref)
{
	addElem(name, AMF0Ref(ref), false);
}

WriteArrayType Amf0SerializerBase::writeArray
(
	const tiny_string& name,
	void* arr
)
{
	auto ref = tryGetRef(obj);
	if (ref.hasValue())
		return WriteArrayType({}, ref);
	auto _ref = makeRef();
	addRef(arr, _ref);
	return WriteArrayType(*this, _ref);
}

void Amf0SerializerBase::writeDate
(
	const tiny_string& name,
	number_t val,
	uint16_t timeZone
);
{
	addElem(name, AMF0Value(val, timeZone));
}

void Amf0SerializerBase::writeXML(const tiny_string& name, const tiny_string& str)
{
	addElem(name, AMF0XML(str));
}

using WriteObjType = Amf0SerializerBase::WriteObjType;
using WriteArrayType = Amf0SerializerBase::WriteArrayType;

Optional<uint16_t> Amf0Serializer::tryGetRef(void* ptr) const
{
	auto it = refs.find(ptr);
	if (it == refs.end())
		return {};
	return it->second;
}

void Amf0Serializer::addRef(void* ptr, uint16_t ref)
{
	refs.emplace(ptr, ref);
}

void Amf0Serializer::addElem
(
	const tiny_string& name,
	const AMFValue& val,
	bool incRef
)
{
	curRef += incRef;
	elems.emplace_back(name, val);
}

void Amf0Serializer::writeNumber(IAMFWriter& writer, number_t val)
{
	writer.writeUInt8(AMF0TypeMarker::Number);
	writer.writeDouble(val);
}

void Amf0Serializer::writeBool(IAMFWriter& writer, bool val)
{
	writer.writeUInt8(AMF0TypeMarker::Bool);
	writer.writeUInt8(val);
}

void Amf0Serializer::writeStringVal(IAMFWriter& writer, const tiny_string& str)
{
	writer.writeUInt8(AMF0TypeMarker::String);
	writeString(writer, str);
}

void Amf0Serializer::writeString(IAMFWriter& writer, const tiny_string& str)
{
	writer.writeString(str);
}

void Amf0Serializer::writeObject
(
	IAMFWriter& writer,
	Span<const AMFElement> elems
)
{
	writer.writeUInt8(AMF0TypeMarker::Object);
	for (const auto& val : elems)
		writeElem(writer, val.first, val.second);

	writer.writeUInt16(0);
	writer.writeUInt8(AMF0TypeMarker::ObjectEnd);
}

void Amf0Serializer::writeRef(IAMFWriter& writer, uint16_t ref)
{
	writer.writeUInt8(AMF0TypeMarker::Ref);
	writer.writeUInt16(ref);
}

void Amf0Serializer::writeECMAArray
(
	IAMFWriter& writer,
	Span<const AMFElement> elems,
	size_t size
)
{
	writer.writeUInt8(AMF0TypeMarker::ECMAArray);
	writer.writeUInt32(size);

	for (const auto& val : elems)
		writeElem(writer, val.first, val.second);

	writer.writeUInt16(0);
	writer.writeUInt8(AMF0TypeMarker::ObjectEnd);
}

void Amf0Serializer::writeStrictArray
(
	IAMFWriter& writer,
	Span<const AMF0Value> elems
)
{
	writer.writeUInt8(AMF0TypeMarker::StrictArray);
	writer.writeUInt32(elems.getSize());

	for (const auto& val : elems)
		writeValue(writer, val);
}

void Amf0Serializer::writeDate
(
	IAMFWriter& writer,
	number_t val,
	uint16_t timeZone
)
{
	writer.writeUInt8(AMF0TypeMarker::Date);
	writer.writeDouble(val);
	writer.writeUInt16(timeZone);
}

void Amf0Serializer::writeLongStringVal
(
	IAMFWriter& writer,
	const tiny_string& str
)
{
	writer.writeUInt8(AMF0TypeMarker::LongString);
	writeLongString(writer, str);
}

void Amf0Serializer::writeLongString
(
	IAMFWriter& writer,
	const tiny_string& str
)
{
	writer.writeString(str, true);
}

void Amf0Serializer::writeXML(IAMFWriter& writer, const tiny_string& str)
{
	writer.writeUInt8(AMF0TypeMarker::XML);
	writeLongString(writer, str);
}

void Amf0Serializer::writeTypedObject
(
	IAMFWriter& writer,
	const tiny_string& name,
	Span<const AMFElement> elems
)
{
	writer.writeUInt8(AMF0TypeMarker::TypedObject);
	writeString(writer, name);

	for (const auto& val : elems)
		writeElem(writer, val.first, val.second);

	writer.writeUInt16(0);
	writer.writeUInt8(AMF0TypeMarker::ObjectEnd);
}

void Amf0Serializer::writeAMF3Value(IAMFWriter& writer, const AMF3Value& val)
{
}

void Amf0Serializer::writeElem
(
	IAMFWriter& writer,
	const tiny_string& name,
	const AMF0Value& val
)
{
	writeString(writer, name);
	writeValue(writer, val);
}

void Amf0Serializer::writeValue(IAMFWriter& writer, const AMF0Value& val)
{
	using Type = AMF0TypeMarker;
	val.visit(makeVisitor
	(
		[&](number_t num) { writeNumber(writer, num); },
		[&](bool flag) { writeBool(writer, flag); },
		[&](const tiny_string& str)
		{
			if (str.numBytes() > 65535)
			{
				writeLongStringVal(writer, str);
				return;
			}
			writeString(writer, str);
		},
		[&](const AMF0Object& obj)
		{
			writeObject(writer, obj.elems);
		},
		[&](AMF0Null) { writer.writeUInt8(Type::Null); },
		[&](AMF0Undefined) { writer.writeUInt8(Type::Undefined); },
		[&](uint16_t ref) { writeRef(writer, ref); },
		[&](const AMF0ECMAArray& arr)
		{
			writeECMAArray(writer, arr.elems, arr.size);
		},
		[&](Span<const AMF0Value> elems)
		{
			writeStrictArray(writer, elems);
		},
		[&](const AMF0Date& date)
		{
			writeDate(writer, date.date, date.timeZone);
		},
		[&](AMF0Unsupported)
		{
			writer.writeUInt8(Type::Unsupported);
		},
		[&](const AMF0XML& xml) { writeXML(writer, xml.data); },
		[&](const AMF0TypeObject& obj)
		{
			writeTypedObject(writer, obj.name, obj.elems);
		},
		[&](const AMF3Value& val)
		{
			writer.writeUInt8(Type::AMF3);
			AMF3Serializer().writeValue(writer, val);
		}
	));
}

LSO Amf0Serializer::makeLSO(const tiny_string& name)
{
	return LSO(elems, name, AMFVersion::AMF0);
}

void Amf0ObjectSerializer::addElem
(
	const tiny_string& name,
	const AMFValue& val,
	bool incRef
)
{
	elems.emplace_back(name, val);
}

void Amf0ObjectSerializer::commit(const tiny_string& name)
{
	addElem(name, AMF0Object(elems));
}

void Amf0ArraySerializer::addElem
(
	const tiny_string& name,
	const AMFValue& val,
	bool incRef
)
{
	elems.emplace_back(name, val);
}

void Amf0ArraySerializer::commit(const tiny_string& name, size_t size)
{
	addElem(name, AMF0Value(elems, size));
}
