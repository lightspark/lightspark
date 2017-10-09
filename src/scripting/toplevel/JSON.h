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

#ifndef SCRIPTING_TOPLEVEL_JSON_H
#define SCRIPTING_TOPLEVEL_JSON_H 1
#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class JSON : public ASObject
{
public:
	JSON(Class_base* c);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_parse);
	ASFUNCTION_ATOM(_stringify);
	static ASObject* doParse(const tiny_string &jsonstring, asAtom reviver);
private:
	static void parseAll(const tiny_string &jsonstring, ASObject** parent , const multiname& key, asAtom reviver);
	static int parse(const tiny_string &jsonstring, int pos, ASObject **parent, const multiname &key,asAtom reviver);
	static int parseTrue(const tiny_string &jsonstring, int pos, ASObject **parent, const multiname &key);
	static int parseFalse(const tiny_string &jsonstring, int pos, ASObject **parent, const multiname &key);
	static int parseNull(const tiny_string &jsonstring, int pos, ASObject **parent, const multiname &key);
	static int parseString(const tiny_string &jsonstring, int pos, ASObject **parent, const multiname &key, tiny_string *result = NULL);
	static int parseNumber(const tiny_string &jsonstring, int pos, ASObject **parent, const multiname &key);
	static int parseObject(const tiny_string &jsonstring, int pos, ASObject **parent, const multiname &key, asAtom reviver);
	static int parseArray(const tiny_string &jsonstring, int pos, ASObject **parent, const multiname &key, asAtom reviver);
};

}
#endif /* SCRIPTING_TOPLEVEL_JSON_H */
