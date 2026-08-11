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

#ifndef PARSING_LSO_H
#define PARSING_LSO_H 1

#include <cstdint>

#include "exceptions.h"
#include "parsing/amf.h"
#include "parsing/amf0_generator.h"
#include "parsing/amf3_generator.h"
#include "tiny_string.h"
#include "utils/span.h"

namespace lightspark
{

// Based on Ruffle's `rust-flash-lso` crate.

using LSOElement = std::pair<tiny_string, AMFValue>;

class LSOException : public LightsparkException
{
public:
	LSOException(const std::string& c) : LightsparkException(c) {}

	const char* what() const throw()
	{
		if (!cause.empty())
			return cause.c_str();
		return "LSO error";
	}
};

struct LSOHeader
{
	uint32_t size;
	tiny_string name;
	AMFVersion version;

	LSOHeader
	(
		const tiny_string& _name,
		const AMFVersion& _version,
		uint32_t _size = 0
	) : size(_size), name(_name), version(_version) {}
};

class LSO
{
private:
	LSOHeader header;
	std::vector<LSOElement> body;
public:
	LSO
	(
		const LSOHeader& _header,
		std::vector<LSOElement> _body = {}
	) : header(_header), body(_body) {}

	LSO
	(
		const LSOHeader& _header,
		Span<LSOElement> _body
	) : LSO(_header, _body.begin(), _body.end()) {}

	template<typename Iter>
	LSO
	(
		const LSOHeader& _header,
		Iter begin,
		Iter end
	) : header(_header), body(begin, end) {}

	LSO
	(
		const tiny_string& name,
		const AMFVersion& version,
		std::vector<LSOElement> _body = {}
	) : LSO(LSOHeader(name, version), _body) {}

	LSO
	(
		const tiny_string& name,
		const AMFVersion& version,
		Span<LSOElement> _body
	) : LSO(name, version, _body.begin(), _body.end()) {}

	template<typename Iter>
	LSO
	(
		const tiny_string& name,
		const AMFVersion& version,
		Iter begin,
		Iter end
	) : LSO(LSOHeader(name, version), begin, end) {}

	const LSOHeader& getHeader() const { return header; }
	Span<const LSOElement> getBody() const { return makeSpan(body); }
};

class LSOReader
{
private:
	Amf0Deserializer amf0Deserializer;
	Amf3Deserializer amf3Deserializer;
	Span<const uint8_t> data;
public:
	LSOReader(Span<const uint8_t> _data) : data(_data) {}
	static LSOHeader parseHeader(Span<const uint8_t>& data);
	Optional<LSO> tryParse();
	LSO parse();
};

class LSOWriter
{
private:
	Amf3Serializer amf3Serializer;
	Optional<bool> forceAMF3;

	void writeHeader
	(
		const LSOHeader& header,
		std::vector<uint8_t>& data
	);
public:
	LSOWriter() {}
	LSOWriter(const LSO& lso, std::vector<uint8_t>& data);
	LSOWriter
	(
		const LSO& lso,
		std::vector<uint8_t>& data,
		bool isAMF3
	);

	void writeToBytes(const LSO& lso, std::vector<uint8_t>& data);
};

}
#endif /* PARSING_LSO_H */
