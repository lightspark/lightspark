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
	uint32_t parseInt(Span<const uint8_t>& data);
	int32_t parseInteger(Span<const uint8_t>& data);
	number_t parseDouble(Span<const uint8_t>& data);
	tiny_string parseString(Span<const uint8_t>& data);
	_R<AMF3Value> parseDate(Span<const uint8_t>& data);
	_R<AMF3Value> parseArray(Span<const uint8_t>& data);
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

	_R<AMF3Value> parseDictionary(Span<const uint8_t>& data);
	_R<AMF3Value> parseRefOrVal
	(
		Span<const uint8_t>& data,
		std::function<AMF3Value()> makeValue,
		std::function<_R<AMF3Value>(size_t, size_t)> parseVal
	);

	TraitsRef parseTraits
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

}
#endif /* PARSING_AMF3_GENERATOR_H */
