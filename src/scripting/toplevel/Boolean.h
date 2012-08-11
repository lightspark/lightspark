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

#ifndef SCRIPTING_TOPLEVEL_BOOLEAN_H
#define SCRIPTING_TOPLEVEL_BOOLEAN_H 1

#include "asobject.h"

namespace lightspark
{

/* returns a fully inialized Boolean object with value b
 * like Class<Boolean>::getInstanceS(b) but without the constructor problems */
Boolean* abstract_b(bool b);

/* implements ecma3's ToBoolean() operation, see section 9.2, but returns the value instead of an Boolean object */
bool Boolean_concrete(const ASObject* obj);

class Boolean: public ASObject
{
public:
	Boolean(Class_base* c, bool v=false):ASObject(c),val(v) {type=T_BOOLEAN;}
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o){};
	bool val;
	int32_t toInt()
	{
		return val ? 1 : 0;
	}
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	ASFUNCTION(_constructor);
	ASFUNCTION(_toString);
	ASFUNCTION(_valueOf);
	ASFUNCTION(generator);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};

}
#endif /* SCRIPTING_TOPLEVEL_BOOLEAN_H */
