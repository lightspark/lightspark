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

#ifndef SCRIPTING_TOPLEVEL_INTEGER_H
#define SCRIPTING_TOPLEVEL_INTEGER_H 1
#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class Integer : public ASObject
{
friend class Number;
friend class Array;
friend class ABCVm;
friend class ABCContext;
friend ASObject* abstract_i(int32_t i);
public:
	Integer(Class_base* c,int32_t v=0):ASObject(c,T_INTEGER),val(v){}
	int32_t val;
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	inline number_t toNumber() { return val; }
	inline bool destruct() { val=0; return ASObject::destruct(); }
	ASFUNCTION_ATOM(_toString);
	tiny_string toString();
	static tiny_string toString(int32_t val);
	int32_t toInt()
	{
		return val;
	}
	int64_t toInt64()
	{
		return val;
	}
	TRISTATE isLess(ASObject* r);
	bool isEqual(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_valueOf);
	ASFUNCTION_ATOM(_toExponential);
	ASFUNCTION_ATOM(_toFixed);
	ASFUNCTION_ATOM(_toPrecision);
	std::string toDebugString() { return toString()+"i"; }
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
	/*
	 * This method skips trailing spaces and zeroes
	 */
	static bool fromStringFlashCompatible(const char* str, number_t &ret, int radix, bool strict=false);
	/* Converts a string into AS (32 bit) integer value using ECMA
	 * script conversion algorithm (i.e. accepts hex, skips
	 * whitespace, zeroes). Returns 0 if conversion fails.
	 */
	static int32_t stringToASInteger(const char* cur, int radix, bool strict=false);
	
};

}
#endif /* SCRIPTING_TOPLEVEL_INTEGER_H */
