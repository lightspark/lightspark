/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010  Ennio Barbaro (e.barbaro@sssup.it)

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

#include <string>
#include <vector>
#include "compat.h"
#include "swftypes.h"
#include "smartrefs.h"
#include "asobject.h"

namespace lightspark
{

class ASObject;
class ByteArray;

enum markers_type
{
	undefined_marker = 0x0,
	null_marker = 0x1,
	false_marker = 0x2,
	true_marker = 0x3,
	integer_marker = 0x4,
	double_marker = 0x5,
	string_marker = 0x6,
	xml_doc_marker = 0x7,
	date_marker = 0x8,
	array_marker = 0x9,
	object_marker = 0xA,
	xml_marker = 0xB,
	byte_array_marker = 0x0C,
	vector_int_marker = 0x0D,
	vector_uint_marker = 0x0E,
	vector_double_marker = 0x0F,
	vector_object_marker = 0x10,
	dictionary_marker = 0x11
};

enum amf0_markers_type
{
	amf0_number_marker       = 0x00,
	amf0_boolean_marker      = 0x01,
	amf0_string_marker       = 0x02,
	amf0_object_marker       = 0x03,
	amf0_movieclip_marker    = 0x04,
	amf0_null_marker         = 0x05,
	amf0_undefined_marker    = 0x06,
	amf0_reference_marker    = 0x07,
	amf0_ecma_array_marker   = 0x08,
	amf0_object_end_marker   = 0x09,
	amf0_strict_array_marker = 0x0A,
	amf0_date_marker         = 0x0B,
	amf0_long_string_marker  = 0x0C,
	amf0_unsupported_marker  = 0x0D,
	amf0_recordset_marker    = 0x0E,
	amf0_xml_document_marker = 0x0F,
	amf0_typed_object_marker = 0x10,
	amf0_avmplus_object_marker = 0x11
};

class TraitsRef
{
public:
	Class_base* type;
	std::vector<tiny_string> traitsNames;
	bool dynamic;
	TraitsRef(Class_base* t):type(t),dynamic(false){}
};

class Amf3Deserializer
{
private:
	ByteArray* input;
	tiny_string parseStringVR(std::vector<tiny_string>& stringMap) const;
	
	asAtom parseObject(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap) const;
	asAtom parseArray(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap) const;
	asAtom parseVector(uint8_t marker, std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap) const;
	asAtom parseDictionary(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap) const;
	asAtom parseByteArray(std::vector<asAtom>& objMap) const;
	asAtom parseValue(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap) const;
	asAtom parseInteger() const;
	asAtom parseDouble() const;
	asAtom parseDate() const;
	asAtom parseXML(std::vector<asAtom>& objMap, bool legacyXML) const;


	asAtom parseECMAArrayAMF0(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap) const;
	asAtom parseStrictArrayAMF0(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap) const;
	asAtom parseObjectAMF0(std::vector<tiny_string>& stringMap,
			std::vector<asAtom>& objMap,
			std::vector<TraitsRef>& traitsMap, const tiny_string& clsname="") const;
public:
	Amf3Deserializer(ByteArray* i):input(i) {}
	asAtom readObject() const;
	void readSharedObject(ASObject* ret);
	tiny_string parseStringAMF0() const;
};

}
#endif /* PARSING_AMF3_GENERATOR_H */
