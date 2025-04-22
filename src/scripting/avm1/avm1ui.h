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

#ifndef SCRIPTING_AVM1_AVM1UI_H
#define SCRIPTING_AVM1_AVM1UI_H


#include "asobject.h"
#include "scripting/flash/ui/ContextMenu.h"
#include "scripting/flash/ui/ContextMenuItem.h"

namespace lightspark
{
class AVM1ContextMenu: public ContextMenu
{
    asAtom avm1builtInItems;
public:
	AVM1ContextMenu(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
    void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
    void AVM1enumerate(std::stack<asAtom>& stack) override;
    bool builtInItemEnabled(const tiny_string& name) override;
    ASFUNCTION_ATOM(AVM1_constructor);
    ASFUNCTION_ATOM(AVM1_get_builtInItems);
    ASFUNCTION_ATOM(AVM1_copy);
};

class AVM1ContextMenuItem: public ContextMenuItem
{
public:
	AVM1ContextMenuItem(ASWorker* wrk,Class_base* c):ContextMenuItem(wrk,c){}
	multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk) override;
	GET_VARIABLE_RESULT AVM1getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk, bool isSlashPath = true) override;
	static void sinit(Class_base* c);
    void AVM1enumerate(std::stack<asAtom>& stack) override;
};

}
#endif // SCRIPTING_AVM1_AVM1UI_H
