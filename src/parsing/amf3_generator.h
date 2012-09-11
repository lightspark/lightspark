/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)
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
	object_marker = 0xa,
	xml_marker = 0xb
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
	_R<ASObject> parseObject(std::vector<tiny_string>& stringMap,
			std::vector<ASObject*>& objMap,
			std::vector<TraitsRef>& traitsMap) const;
	_R<ASObject> parseArray(std::vector<tiny_string>& stringMap,
			std::vector<ASObject*>& objMap,
			std::vector<TraitsRef>& traitsMap) const;
	_R<ASObject> parseValue(std::vector<tiny_string>& stringMap,
			std::vector<ASObject*>& objMap,
			std::vector<TraitsRef>& traitsMap) const;
	_R<ASObject> parseInteger() const;
	_R<ASObject> parseDouble() const;
	_R<ASObject> parseXML(std::vector<ASObject*>& objMap, bool legacyXML) const;
public:
	Amf3Deserializer(ByteArray* i):input(i) {}
	_R<ASObject> readObject() const;
};

};
#endif /* PARSING_AMF3_GENERATOR_H */
