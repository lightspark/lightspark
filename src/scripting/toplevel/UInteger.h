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
	UInteger(Class_base* c,uint32_t v=0):ASObject(c),val(v){type=T_UINTEGER;}

	static void sinit(Class_base* c);
	tiny_string toString();
	static tiny_string toString(uint32_t val);
	int32_t toInt()
	{
		return val;
	}
	uint32_t toUInt()
	{
		return val;
	}
	TRISTATE isLess(ASObject* r);
	bool isEqual(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	ASFUNCTION(_toString);
	std::string toDebugString() { return toString()+"ui"; }
	//CHECK: should this have a special serialization?
};

}
#endif /* SCRIPTING_TOPLEVEL_UINTEGER_H */
