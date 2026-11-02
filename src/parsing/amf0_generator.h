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

#ifndef PARSING_AMF0_GENERATOR_H
#define PARSING_AMF0_GENERATOR_H 1

#include <cstdint>
#include <tsl/ordered_map.h>
#include <vector>

#include "parsing/amf.h"
#include "parsing/amf3_generator.h"
#include "utils/span.h"

// Based on Ruffle's `rust-flash-lso` crate.

namespace lightspark
{

class tiny_string;
class LSO;

class Amf0Deserializer
{
private:
	Amf3Deserializer amf3Deserializer;

	AMF0Value parseNumber(Span<const uint8_t>& data);
	AMF0Value parseBool(Span<const uint8_t>& data);
	AMF0Value parseStringVal(Span<const uint8_t>& data);
	AMF0Value parseObject(Span<const uint8_t>& data);
	AMF0Value parseRef(Span<const uint8_t>& data);
	AMF0Value parseECMAArray(Span<const uint8_t>& data);
	AMF0Value parseStrictArray(Span<const uint8_t>& data);
	AMF0Value parseDate(Span<const uint8_t>& data);
	static tiny_string parseLongStringImpl(Span<const uint8_t>& data);
	AMF0Value parseLongString(Span<const uint8_t>& data);
	AMF0Value parseXML(Span<const uint8_t>& data);
	AMF0Value parseTypedObject(Span<const uint8_t>& data);
	AMFValue parseAMF3Value(Span<const uint8_t>& data);
	std::vector<AMFElement> parseArrayElem(Span<const uint8_t>& data);
public:
	AMFValue parseValue(Span<const uint8_t>& data);
	AMFElement parseElement(Span<const uint8_t>& data);
	std::vector<AMFElement> parseBody(Span<const uint8_t>& data);
	static tiny_string parseString(Span<const uint8_t>& data);
};

class Amf0ArraySerializer;
class Amf0ObjectSerializer;

class Amf0SerializerBase
{
public:
	using WriteObjType = std::pair
	<
		Optional<Amf0ObjectSerializer>,
		uint16_t
	>;

	using WriteArrayType = std::pair
	<
		Optional<Amf0ArraySerializer>,
		uint16_t
	>;
protected:
	virtual uint16_t makeRef() = 0;
	virtual Optional<uint16_t> tryGetRef(void* ptr) const  = 0;
	virtual void addRef(void* ptr, uint16_t ref)  = 0;
	virtual void addElem
	(
		const tiny_string& name,
		const AMFValue& val,
		bool incRef = true
	) = 0;

	void writeNumber(const tiny_string& name, number_t val);
	void writeBool(const tiny_string& name, bool val);
	void writeString(const tiny_string& name, const tiny_string& str);
	WriteObjType writeObject(const tiny_string& name, void* obj);
	void writeNull(const tiny_string& name);
	void writeUndefined(const tiny_string& name);
	void writeRef(const tiny_string& name, uint16_t ref);
	WriteArrayType writeArray(const tiny_string& name, void* arr);
	void writeDate
	(
		const tiny_string& name,
		number_t val,
		uint16_t timeZone
	);

	void writeXML(const tiny_string& name, const tiny_string& str);
};

class Amf0Serializer : public Amf0SerializerBase
{
private:
	std::vector<AMFElement> elems;
	uint16_t curRef;
	tsl::ordered_map<void*, uint16_t> refs;

	static void writeNumber(IAMFWriter& writer, number_t val);
	static void writeBool(IAMFWriter& writer, bool val);
	static void writeStringVal(IAMFWriter& writer, const tiny_string& str);
	static void writeString(IAMFWriter& writer, const tiny_string& str);
	static void writeObject
	(
		IAMFWriter& writer,
		Span<const AMFElement> elems
	);

	static void writeRef(IAMFWriter& writer, uint16_t ref);
	static void writeECMAArray
	(
		IAMFWriter& writer,
		Span<const AMFElement> elems,
		size_t size
	);

	static void writeStrictArray
	(
		IAMFWriter& writer,
		Span<const AMF0Value> elems
	);

	static void writeDate
	(
		IAMFWriter& writer,
		number_t val,
		uint16_t timeZone
	);

	static void writeLongStringVal(IAMFWriter& writer, const tiny_string& str);
	static void writeLongString(IAMFWriter& writer, const tiny_string& str);
	static void writeXML(IAMFWriter& writer, const tiny_string& str);

	static void writeTypedObject
	(
		IAMFWriter& writer,
		const tiny_string& name,
		Span<const AMFElement> elems
	);

	static void writeAMF3Value(IAMFWriter& writer, const AMF3Value& val);
	static void writeElem
	(
		IAMFWriter& writer,
		const tiny_string& name,
		const AMF0Value& val
	);
protected:
	uint16_t makeRef() override { return curRef++; }
	Optional<uint16_t> tryGetRef(void* ptr) const override;
	void addRef(void* ptr, uint16_t ref) override;
	void addElem
	(
		const tiny_string& name,
		const AMFValue& val,
		bool incRef = true
	) override;
public:
	static void writeValue(IAMFWriter& writer, const AMF0Value& val);
	LSO makeLSO(const tiny_string& name);
};

class Amf0ObjectSerializer : public Amf0SerializerBase
{
	using Base = Amf0SerializerBase;
private:
	std::vector<AMFElement> elems;
	Base& parent;
protected:
	uint16_t makeRef() override { return parent.makeRef(); }
	Optional<uint16_t> tryGetRef(void* ptr) const override
	{
		return parent.tryGetRef(ptr);
	}

	void addRef(void* ptr, uint16_t ref) override
	{
		parent.addRef(ptr, ref);
	}

	void addElem
	(
		const tiny_string& name,
		const AMFValue& val,
		bool incRef = true
	) override;
public:
	Amf0ObjectSerializer(Base& _parent) : parent(_parent) {}
	void commit(const tiny_string& name);
};

class Amf0ArraySerializer : public Amf0SerializerBase
{
	using Base = Amf0SerializerBase;
private:
	std::vector<AMFElement> elems;
	Base& parent;
protected:
	uint16_t makeRef() override { return parent.makeRef(); }
	Optional<uint16_t> tryGetRef(void* ptr) const override
	{
		return parent.tryGetRef(ptr);
	}

	void addRef(void* ptr, uint16_t ref) override
	{
		parent.addRef(ptr, ref);
	}

	void addElem
	(
		const tiny_string& name,
		const AMFValue& val,
		bool incRef = true
	) override;
public:
	Amf0ArraySerializer(Base& _parent) : parent(_parent) {}
	void commit(const tiny_string& name, size_t size);
};

}
#endif /* PARSING_AMF0_GENERATOR_H */
