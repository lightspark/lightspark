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
