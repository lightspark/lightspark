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

#ifndef SCRIPTING_TOPLEVEL_GLOBAL_H
#define SCRIPTING_TOPLEVEL_GLOBAL_H 1

#include "asobject.h"

namespace lightspark
{

class Global : public ASObject
{
private:
	int scriptId;
	ABCContext* context;
	bool isavm1;
public:
	Global(ASWorker* wrk,Class_base* cb, ABCContext* c, int s, bool avm1);
	static void sinit(Class_base* c);
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname &name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;
	void getVariableByMultinameOpportunistic(asAtom& ret, const multiname& name, ASWorker* wrk);
	/*
	 * Utility method to register builtin methods and classes
	 */
	void registerBuiltin(const char* name, const char* ns, _R<ASObject> o, NS_KIND nskind=NAMESPACE);
	// ensures that the init script has been run
	void checkScriptInit();
	bool isAVM1() const { return isavm1; }
	ABCContext* getContext() const { return context; }
};

}

#endif /* SCRIPTING_TOPLEVEL_GLOBAL_H */
