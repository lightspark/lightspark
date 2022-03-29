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

#ifndef SCRIPTING_TOPLEVEL_REGEXP_H
#define SCRIPTING_TOPLEVEL_REGEXP_H 1
#include "compat.h"
#include "asobject.h"
#include "3rdparty/avmplus/pcre/pcre.h"

namespace lightspark
{

class RegExp: public ASObject
{
public:
	RegExp(ASWorker* wrk,Class_base* c);
	RegExp(ASWorker* wrk, Class_base* c, const tiny_string& _re);
	pcre* compile(bool isutf8);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASObject *match(const tiny_string& str);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(exec);
	ASFUNCTION_ATOM(test);
	ASFUNCTION_ATOM(_toString);
	ASPROPERTY_GETTER(bool, dotall);
	ASPROPERTY_GETTER(bool, global);
	ASPROPERTY_GETTER(bool, ignoreCase);
	ASPROPERTY_GETTER(bool, extended);
	ASPROPERTY_GETTER(bool, multiline);
	ASPROPERTY_GETTER_SETTER(int, lastIndex);
	ASPROPERTY_GETTER(tiny_string, source);
};

}
#endif /* SCRIPTING_TOPLEVEL_REGEXP_H */
