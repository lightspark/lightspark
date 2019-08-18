/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_TOPLEVEL_ASSTRING_H
#define SCRIPTING_TOPLEVEL_ASSTRING_H 1

#include "scripting/class.h"

namespace Glib { class ustring; }

namespace lightspark
{
/*
 * The AS String class.
 * The 'data' is immutable -> it cannot be changed after creation of the object
 */
class ASString: public ASObject
{
	friend ASObject* abstract_s(SystemState* sys);
	friend ASObject* abstract_s(SystemState* sys, const char* s, uint32_t len);
	friend ASObject* abstract_s(SystemState* sys, const char* s);
	friend ASObject* abstract_s(SystemState* sys, const tiny_string& s);
	friend ASObject* abstract_s(SystemState* sys, uint32_t stringId);
private:
	number_t parseStringInfinite(const char *s, char **end) const;
	tiny_string data;
	
	// stores the position of the last call to charAt/charCodeAt
	// speeds up iterating over all chars in the string
	CharIterator currentpos;
	uint32_t currentindex;
public:
	ASString(Class_base* c);
	ASString(Class_base* c, const std::string& s);
	ASString(Class_base* c, const tiny_string& s);
	ASString(Class_base* c, const Glib::ustring& s);
	ASString(Class_base* c, const char* s);
	ASString(Class_base* c, const char* s, uint32_t len);
	bool hasId:1;
	bool datafilled:1;
	FORCE_INLINE tiny_string& getData()
	{
		if (!datafilled)
		{
			data = getSystemState()->getStringFromUniqueId(stringId);
			datafilled = true;
		}
		return data;
	}
	FORCE_INLINE bool isEmpty() const
	{
		if (hasId)
			return stringId == BUILTIN_STRINGS::EMPTY || stringId == UINT32_MAX;
		return data.empty();
	}

	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(charAt);
	ASFUNCTION_ATOM(charCodeAt);
	ASFUNCTION_ATOM(concat);
	ASFUNCTION_ATOM(fromCharCode);
	ASFUNCTION_ATOM(indexOf);
	ASFUNCTION_ATOM(lastIndexOf);
	ASFUNCTION_ATOM(match);
	ASFUNCTION_ATOM(replace);
	ASFUNCTION_ATOM(search);
	ASFUNCTION_ATOM(slice);
	ASFUNCTION_ATOM(split);
	ASFUNCTION_ATOM(substr);
	ASFUNCTION_ATOM(substring);
	ASFUNCTION_ATOM(toLowerCase);
	ASFUNCTION_ATOM(toUpperCase);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(_getLength);
	ASFUNCTION_ATOM(localeCompare);
	ASFUNCTION_ATOM(localeCompare_prototype);
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	TRISTATE isLessAtom(asAtom& r);
	number_t toNumber();
	int32_t toInt();
	int32_t toIntStrict();
	uint32_t toUInt();
	int64_t toInt64();
	
	ASFUNCTION_ATOM(generator);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
	std::string toDebugString() { return std::string("\"") + std::string(getData()) + "\""; }
	static bool isEcmaSpace(uint32_t c);
	static bool isEcmaLineTerminator(uint32_t c);
	inline bool destruct() 
	{ 
		data.clear(); 
		hasId = false;
		datafilled=false; 
		if (!destructIntern())
		{
			stringId = BUILTIN_STRINGS::EMPTY;
			hasId = true;
			datafilled = true;
			currentindex=0;
			return false;
		}
		return true;
	}
};

template<>
inline bool Class<ASString>::coerce(SystemState* sys,asAtom& o) const
{
	if (asAtomHandler::isString(o))
		return false;
	//Special handling for Null and Undefined follows avm2overview's description of 'coerce_s' opcode
	if(asAtomHandler::isNull(o))
		return false;
	if(asAtomHandler::isUndefined(o))
	{
		asAtomHandler::setNull(o);
		return true;
	}
	if(!asAtomHandler::isConstructed(o))
		return false;
	uint32_t stringID =asAtomHandler::toStringId(o,sys);
	o = asAtomHandler::fromStringID(stringID);
	return true;
}

}
#endif /* SCRIPTING_TOPLEVEL_ASSTRING_H */
