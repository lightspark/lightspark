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
	JSON(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_parse);
	ASFUNCTION_ATOM(_stringify);
	static ASObject* doParse(const tiny_string &jsonstring, asAtom reviver, ASWorker* wrk);
private:
	static void parseAll(const tiny_string &jsonstring, ASObject** parent , multiname &key, asAtom reviver, ASWorker* wrk);
	static void parse(const tiny_string &jsonstring, CharIterator& it, ASObject **parent, multiname &key, asAtom reviver, ASWorker* wrk);
	static void parseTrue(CharIterator& it, ASObject **parent, multiname &key, ASWorker* wrk);
	static void parseFalse(CharIterator& it, ASObject **parent, multiname &key, ASWorker* wrk);
	static void parseNull(CharIterator& it, ASObject **parent, multiname &key, ASWorker* wrk);
	static void parseString(const tiny_string &jsonstring, CharIterator& it, ASObject **parent, multiname &key, ASWorker* wrk, tiny_string *result = nullptr);
	static void parseNumber(const tiny_string &jsonstring, CharIterator& it, ASObject **parent, multiname &key, ASWorker* wrk);
	static void parseObject(const tiny_string &jsonstring, CharIterator& it, ASObject **parent, multiname &key, asAtom reviver, ASWorker* wrk);
	static void parseArray(const tiny_string &jsonstring, CharIterator& it, ASObject **parent, multiname &key, asAtom reviver, ASWorker* wrk);
};

}
#endif /* SCRIPTING_TOPLEVEL_JSON_H */
