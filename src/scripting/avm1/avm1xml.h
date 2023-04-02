/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_AVM1_AVM1XML_H
#define SCRIPTING_AVM1_AVM1XML_H


#include "scripting/flash/xml/flashxml.h"
#include "scripting/flash/net/flashnet.h"

namespace lightspark
{

class AVM1XMLDocument: public XMLDocument
{
	_NR<URLLoader> loader;
public:
	AVM1XMLDocument(ASWorker* wrk,Class_base* c):XMLDocument(wrk,c){ needsActionScript3=false; }
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(load);
	ASFUNCTION_ATOM(_getter_status);
	multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk) override;
	void AVM1HandleEvent(EventDispatcher* dispatcher, Event* e) override;
};

}
#endif // SCRIPTING_AVM1_AVM1XML_H
