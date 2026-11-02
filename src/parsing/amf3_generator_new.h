/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010  Ennio Barbaro (e.barbaro@sssup.it)
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

#ifndef PARSING_AMF3_GENERATOR_H
#define PARSING_AMF3_GENERATOR_H 1

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <tsl/ordered_set.h>
#include <vector>

#include "compat.h"
#include "parsing/amf.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/span.h"

namespace lightspark
{

enum AMF3TypeMarker;
class AMF3Value;

struct TraitsRef
{
	tiny_string name;
	std::vector<tiny_string> staticProps;
	bool external;
	bool dynamic;

	TraitsRef
	(
		const tiny_string& _name = "",
		const std::vector<tiny_string>& _staticProps = {},
		bool _external = false,
		bool _dynamic = false
	) :
	name(_name),
	staticProps(_staticProps),
	external(_external),
	dynamic(_dynamic) {}
};

class Amf3Deserializer
{
public:
	using ExtDecoderFunc = std::function<std::vector<AMFElement>
	(
		Span<const uint8_t>&,
		Amf3Deserializer&
	)>;
private:
	std::vector<tiny_string> stringMap;
	std::vector<_R<AMF3Value>> objMap;
	std::vector<TraitsRef> traitsMap;
	std::unordered_map<tiny_string, ExtDecoderFunc> extDecoders;
	size_t objId { 0 };

	std::pair<bool, size_t> parseSize(Span<const uint8_t>& data);
	uint32_t parseUInt29(Span<const uint8_t>& data);
	int32_t parseInteger(Span<const uint8_t>& data);
	number_t parseDouble(Span<const uint8_t>& data);
	tiny_string parseString(Span<const uint8_t>& data);
	_R<AMF3Value> parseDate(Span<const uint8_t>& data);
	_R<AMF3Value> parseArray(Span<const uint8_t>& data);
	_R<AMF3Value> parseArrayImpl
	(
		Span<const uint8_t>& data,
		size_t size,
		size_t idx
	);

	_R<AMF3Value> parseObject(Span<const uint8_t>& data);
	_R<AMF3Value> parseObjectImpl
	(
		Span<const uint8_t>& data,
		size_t size,
		size_t idx
	);

	_R<AMF3Value> parseXML(Span<const uint8_t>& data, bool isStr);
	_R<AMF3Value> parseByteArray(Span<const uint8_t>& data);
	_R<AMF3Value> parseVector
	(
		const AMF3TypeMarker& type,
		Span<const uint8_t>& data
	);

	template<typename T>
	_R<AMF3Value> parseVec(Span<const uint8_t>& data, size_t size);
	_R<AMF3Value> parseObjVec(Span<const uint8_t>& data, size_t size);
	_R<AMF3Value> parseDictionary(Span<const uint8_t>& data);
	_R<AMF3Value> parseDictionaryImpl
	(
		Span<const uint8_t>& data,
		size_t size,
		size_t idx
	);

	_R<AMF3Value> parseRefOrVal
	(
		Span<const uint8_t>& data,
		std::function<AMF3Value()> makeValue,
		std::function<_R<AMF3Value>(size_t, size_t)> parseVal
	);

	const TraitsRef& parseTraits
	(
		Span<const uint8_t>& data,
		size_t size,
		size_t idx
	);

	AMFElement parseElement(Span<const uint8_t>& data);
	AMF3Value parseValueImpl(Span<const uint8_t>& data);
public:
	_R<AMF3Value> parseValue(Span<const uint8_t>& data);
	std::vector<AMFElement> parseBody(Span<const uint8_t>& data);
};

template<typename T>
class ElemVec
{
	using VecType = tsl::ordered_set<T>;
private:
	VecType elems;
public:
	using value_type = VecType::value_type;
	using size_type = VecType::size_type;
	using difference_type = VecType::difference_type;
	using iterator = VecType::iterator;
	using const_iterator = VecType::iterator;

	using Iter = VecType::iterator;
	using ConstIter = VecType::const_iterator;
	using SizePair = std::pair<bool, size_t>;

	bool hasElem(const T& val) const { return elems.contains(val); }
	void addElem(const T& val) { elems.insert(val); }
	template<typename... Args>
	void emplaceElem(Args&&... args) { elems.emplace(args...); }
	Optional<const T&> getElem(size_t i) const
	{
		if (i >= elems.size())
			return {};
		return makeOptionalRef(elems.nth(i));
	}

	Optional<size_t> getIndex(const T& val) const
	{
		auto it = elems.find(val);
		if (it == elems.end())
			return {};
		return it - elems.begin();
	}

	SizePair toSize(const T& val, size_t size = 0) const
	{
		auto idx = getIndex(val);
		return { idx.hasValue(), idx.valueOr(size) };
	}

	SizePair toSizeAdd(const T& val, size_t size = 0)
	{
		auto pair = toSize(val, size);
		addElem(val);
		return pair;
	}

	Iter begin() { return elems.begin(); }
	ConstIter begin() { return elems.begin(); }
	ConstIter cbegin() { return elems.begin(); }
	Iter end() { return elems.end(); }
	ConstIter end() { return elems.end(); }
	ConstIter cend() { return elems.end(); }
};

class Amf3Serializer
{
	using RefPair = std::pair<AMF3TypeMarker, size_t>;
	using DictPair = std::pair<_R<AMF3Value>, _R<AMF3Value>>;
private:
	ElemVec<tiny_string> stringMap;
	ElemVec<AMF3Value> objMap;
	std::vector<TraitsRef> traitsMap;
	std::unordered_map<tiny_string, IExternalEncoder> extEncoders;
	std::unordered_map<size_t, RefPair> refMap;

	void writeUInt29(IAMFWriter& writer, uint32_t val);
	void writeInt(IAMFWriter& writer, int32_t val);
	void writeBool(IAMFWriter& writer, bool val);
	void writeDouble(IAMFWriter& writer, number_t val);
	void writeStringVal(IAMFWriter& writer, const tiny_string& str);
	void writeString(IAMFWriter& writer, const tiny_string& str);
	void writeDate(IAMFWriter& writer, number_t time);
	void writeArray
	(
		IAMFWriter& writer,
		Span<const AMFElement> elems,
		Span<const AMF3Value> denseElems
	);

	void writeObject
	(
		IAMFWriter& writer,
		size_t id,
		Optional<const TraitsRef&> traits,
		Span<const AMFElement> props,
		Span<const AMFElement> customProps = {}
	);

	void writeXML
	(
		IAMFWriter& writer,
		const tiny_string& str,
		bool isStr
	);

	void writeByteArray(IAMFWriter& writer, Span<const uint8_t> data);
	template<typename T, typename = void>
	void writeVector(IAMFWriter& writer, const T& vec);
	void writeDictionary(IAMFWriter& writer, const AMF3Dict& dict);

	std::pair<bool, size_t> addRef
	(
		const AMF3Value& val
		const AMF3TypeMarker& type,
		size_t id,
		size_t size
	);

	void writeRef
	(
		IAMFWriter& writer,
		const AMF3TypeMarker& type,
		size_t ref
	);

	void writeTraits(IAMFWriter& writer, const TraitsRef& traits);
	void writeTraitRef
	(
		IAMFWriter& writer,
		size_t idx,
		Span<const AMFElement> props,
		Span<const AMFElement> customProps,
		const TraitsRef& ref
	);

	void writeElem
	(
		IAMFWriter& writer,
		const tiny_string& name,
		const AMF3Value& val
	);
public:
	void writeValue(IAMFWriter& writer, const AMF3Value& val);
	void writeLSOBody(IAMFWriter& writer, const LSO& lso);
};

}
#endif /* PARSING_AMF3_GENERATOR_H */
