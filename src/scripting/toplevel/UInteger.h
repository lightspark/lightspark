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

#ifndef SCRIPTING_TOPLEVEL_UINTEGER_H
#define SCRIPTING_TOPLEVEL_UINTEGER_H 1
#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class UInteger: public ASObject
{
friend ASObject* abstract_ui(uint32_t i);
public:
	uint32_t val;
	UInteger(Class_base* c,uint32_t v=0):ASObject(c,T_UINTEGER),val(v){}

	static void sinit(Class_base* c);
	tiny_string toString();
	static tiny_string toString(uint32_t val);
	inline number_t toNumber() { return val; }
	inline bool destruct() { val=0; return ASObject::destruct();}
	inline int32_t toInt() { return val; }
	inline int64_t toInt64() { return val; }
	inline uint32_t toUInt() { return val; }
	TRISTATE isLess(ASObject* r);
	bool isEqual(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION(_toString);
	ASFUNCTION(_valueOf);
	ASFUNCTION(_toExponential);
	ASFUNCTION(_toFixed);
	ASFUNCTION(_toPrecision);
	std::string toDebugString() { return toString()+"ui"; }
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};

}
#endif /* SCRIPTING_TOPLEVEL_UINTEGER_H */
