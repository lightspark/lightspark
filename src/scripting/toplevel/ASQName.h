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

#ifndef SCRIPTING_TOPLEVEL_ASQNAME_H
#define SCRIPTING_TOPLEVEL_ASQNAME_H 1

#include "asobject.h"

namespace lightspark
{
class ASWorker;

class ASQName: public ASObject
{
friend struct multiname;
friend class Namespace;
private:
	bool uri_is_null;
	uint32_t uri;
	uint32_t local_name;
public:
	ASQName(ASWorker* wrk,Class_base* c);
	void setByXML(XML* node);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_getURI);
	ASFUNCTION_ATOM(_getLocalName);
	ASFUNCTION_ATOM(_toString);
	uint32_t getURI() const { return uri; }
	uint32_t getLocalName() const { return local_name; }
	bool isEqual(ASObject* o) override;

	tiny_string toString();

	uint32_t nextNameIndex(uint32_t cur_index) override;
	void nextName(asAtom &ret, uint32_t index) override;
	void nextValue(asAtom &ret, uint32_t index) override;
};

}
#endif /* SCRIPTING_TOPLEVEL_ASQNAME_H */
