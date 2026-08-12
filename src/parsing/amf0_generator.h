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

	virtual uint16_t makeRef() = 0;
	virtual Optional<uint16_t> tryGetRef(void* ptr) const  = 0;
	virtual void addRef(void* ptr, uint16_t ref)  = 0;
	virtual void addElem
	(
		const tiny_string& name,
		const AMFValue& val,
		bool incRef
	)  = 0;

	void writeNumber(const tiny_string& name, number_t val);
	void writeBool(const tiny_string& name, bool val);
	void writeString(const tiny_string& name, const tiny_string& str);
	virtual WriteObjType writeObject(const tiny_string& name, void* obj);
	void writeRef(const tiny_string& name, uint16_t ref);
	virtual WriteArrayType writeECMAArray
	(
		const tiny_string& name,
		void* arr
	)  = 0;

	virtual WriteArrayType writeStrictArray
	(
		const tiny_string& name,
		void* arr
	)  = 0;

	void writeDate(const tiny_string& name, number_t val);
	void writeLongString(const tiny_string& name, const tiny_string& str);
	void writeXML(const tiny_string& name, const tiny_string& str);
	virtual WriteObjType writeTypedObject
	(
		const tiny_string& name,
		void* obj
	) = 0;
};

class Amf0Serializer : public Amf0SerializerBase
{
private:
	std::vector<AMFElement> elems;
	uint16_t curRef;
	tsl::ordered_map<void*, uint16_t> refs;
public:
	virtual uint16_t makeRef() override;
	virtual Optional<uint16_t> tryGetRef(void* ptr) const override;
	virtual void addRef(void* ptr, uint16_t ref) override;
	virtual void addElem
	(
		const tiny_string& name,
		const AMFValue& val,
		bool incRef
	) override;

	virtual WriteArrayType writeECMAArray
	(
		const tiny_string& name,
		void* arr
	) override;

	virtual WriteArrayType writeStrictArray
	(
		const tiny_string& name,
		void* arr
	) override;

	virtual WriteObjType writeTypedObject
	(
		const tiny_string& name,
		void* obj
	) override;

	LSO makeLSO(const tiny_string& name);
};

class Amf0ObjectSerializer : public Amf0SerializerBase
{
	using Base = Amf0SerializerBase;
private:
	std::vector<AMFElement> elems;
	Base& parent;
public:
	Amf0ObjectSerializer(Base& _parent) : parent(_parent) {}

	virtual uint16_t makeRef() override { return parent.makeRef(); }
	virtual Optional<uint16_t> tryGetRef(void* ptr) const override
	{
		return parent.tryGetRef(ptr);
	}

	virtual void addRef(void* ptr, uint16_t ref) override
	{
		parent.addRef(ptr, ref);
	}

	virtual void addElem
	(
		const tiny_string& name,
		const AMFValue& val,
		bool incRef
	) override;

	virtual WriteArrayType writeECMAArray
	(
		const tiny_string& name,
		void* arr
	) override;

	virtual WriteArrayType writeStrictArray
	(
		const tiny_string& name,
		void* arr
	) override;

	virtual WriteObjType writeTypedObject
	(
		const tiny_string& name,
		void* obj
	) override;

	void commit(const tiny_string& name);
};

class Amf0ArraySerializer : public Amf0SerializerBase
{
	using Base = Amf0SerializerBase;
private:
	std::vector<AMFElement> elems;
	Base& parent;
public:
	Amf0ArraySerializer(Base& _parent) : parent(_parent) {}

	virtual uint16_t makeRef() override { return parent.makeRef(); }
	virtual Optional<uint16_t> tryGetRef(void* ptr) const override
	{
		return parent.tryGetRef(ptr);
	}

	virtual void addRef(void* ptr, uint16_t ref) override
	{
		parent.addRef(ptr, ref);
	}

	virtual void addElem
	(
		const tiny_string& name,
		const AMFValue& val,
		bool incRef
	) override;

	virtual WriteArrayType writeECMAArray
	(
		const tiny_string& name,
		void* arr
	) override;

	virtual WriteArrayType writeStrictArray
	(
		const tiny_string& name,
		void* arr
	) override;

	virtual WriteObjType writeTypedObject
	(
		const tiny_string& name,
		void* obj
	) override;

	void commit(const tiny_string& name);
};

}
#endif /* PARSING_AMF0_GENERATOR_H */
