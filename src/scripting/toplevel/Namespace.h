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

#ifndef SCRIPTING_TOPLEVEL_NAMESPACE_H
#define SCRIPTING_TOPLEVEL_NAMESPACE_H 1

#include "asobject.h"

namespace lightspark
{

class Namespace: public ASObject
{
friend class ASQName;
friend class ABCContext;
private:
	NS_KIND nskind;
	bool prefix_is_undefined;
	uint32_t uri;
	uint32_t prefix;
public:
	Namespace(ASWorker* wrk,Class_base* c);
	Namespace(ASWorker* wrk, Class_base* c, uint32_t _uri, uint32_t _prefix=BUILTIN_STRINGS::EMPTY, NS_KIND _nskind = NAMESPACE,bool _prefix_is_undefined=false);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_getURI);
	// according to ECMA-357 and tamarin tests uri/prefix properties are readonly
	//ASFUNCTION_ATOM(_setURI);
	ASFUNCTION_ATOM(_getPrefix);
	//ASFUNCTION_ATOM(_setPrefix);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(_valueOf);
	ASFUNCTION_ATOM(_ECMA_valueOf);
	bool isEqual(ASObject* o) override;
	NS_KIND getKind() const { return nskind; }
	uint32_t getURI() const { return uri; }
	uint32_t getPrefix(bool& is_undefined) { is_undefined=prefix_is_undefined; return prefix; }

	uint32_t nextNameIndex(uint32_t cur_index) override;
	void nextName(asAtom &ret, uint32_t index) override;
	void nextValue(asAtom &ret, uint32_t index) override;
};

}

#endif /* SCRIPTING_TOPLEVEL_NAMESPACE_H */
