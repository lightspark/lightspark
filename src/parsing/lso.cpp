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

#include <initializer_list>

#include "parsing/lso.h"

using namespace lightspark;

// Based on Ruffle's `rust-flash-lso` crate.

static constexpr std::initializer_list<uint8_t> headerSig =
{
	'T', 'C', 'S', 'O', '\0',
	0x04, 0x00, 0x00, 0x00, 0x00
};

LSOHeader LSOReader::parseHeader(Span<const uint8_t>& data)
{
	if (data.atBE<uint16_t>(0) != 0xbf)
		throw LSOException("Invalid header version.");
	auto size = data.atBE<uint32_t>(2);

	data = data.subSpan(6);
	if (!data.startsWith(makeSpan(headerSig)))
		throw LSOException("Invalid signature.");

	data = data.subSpan(headerSig.size());

	auto name = AMF0Deserializer::parseString(data);
	auto version = data.atBE<uint32_t>(0);
	if (version != 0 && version != 3)
		throw LSOException("Invalid AMF version.");

	data = data.subSpan(4);
	return LSOHeader(name, AMFVersion(version), size);
}

Optional<LSO> LSOReader::tryParse()
{
	try
	{
		return parse();
	}
	catch (...)
	{
		return {};
	}
}

LSO LSOReader::parse()
{
	auto _data = data;
	auto header = parseHeader(_data);

	switch (header.version)
	{
		case AMFVersion::AMF0:
			return LSO
			(
				header,
				amf0Deserializer.parseBody(_data)
			);
		case AMFVersion::AMF3:
			return LSO
			(
				header,
				amf3Deserializer.parseBody(_data)
			);
		default:
			throw LSOException("Invalid AMF version.");
	}
}

LSOWriter::LSOWriter(const LSO& lso, std::vector<uint8_t>& data)
{
	writeToBytes(lso, data);
}

LSOWriter::LSOWriter
(
	const LSO& lso,
	std::vector<uint8_t>& data,
	bool isAMF3
) : forceAMF3(isAMF3)
{
	writeToBytes(lso, data);
}

void LSOWriter::writeHeader
(
	const LSOHeader& header,
	std::vector<uint8_t>& data
)
{
	auto size = makeBE(header.size).getValue();
	auto sizeSpan = Span<const uint32_t>(&size, 1).asBytes();
	auto nameSpan = makeSpan(header.name);
	data.insert(data.end(), { 0, 0xbf });
	data.insert(data.end(), sizeSpan.begin(), sizeSpan.end())
	data.insert(data.end(), headerSig);

	data.insert(data.end(), nameSpan.begin(), nameSpan.end());
	data.insert(data.end(), { 0, 0, 0, header.version });
}

void LSOWriter::writeToBytes(const LSO& lso, std::vector<uint8_t>& data)
{
	data.reserve(header.size + 6);
	writeHeader(lso.header, data);

	switch (header.version)
	{
		case AMFVersion::AMF0:
			AMF0Writer::writeLSOBody(lso, data);
			break;
		case AMFVersion::AMF3:
			amf3Serializer.writeLSOBody(lso, data);
			break;
		default:
			throw LSOException("Invalid AMF version.");
	}
}
