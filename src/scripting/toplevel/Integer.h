/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
	Integer(Class_base* c,int32_t v=0):ASObject(c),val(v){type=T_INTEGER;}
	int32_t val;
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	ASFUNCTION(_toString);
	tiny_string toString();
	static tiny_string toString(int32_t val);
	int32_t toInt()
	{
		return val;
	}
	TRISTATE isLess(ASObject* r);
	bool isEqual(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	std::string toDebugString() { return toString()+"i"; }
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
	/*
	 * This method skips trailing spaces and zeroes
	 */
	static bool fromStringFlashCompatible(const char* str, int64_t& ret,int radix);
};

}
#endif /* SCRIPTING_TOPLEVEL_INTEGER_H */
