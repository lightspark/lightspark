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

#include "avm1text.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/Stage.h"

using namespace std;
using namespace lightspark;

void AVM1TextField::sinit(Class_base* c)
{
	InteractiveObject::AVM1SetupMethods(c);
	TextField::sinit(c);
	c->isSealed = false;
	c->prototype->setVariableByQName("setNewTextFormat","",c->getSystemState()->getBuiltinFunction(TextField::_setDefaultTextFormat,1),DYNAMIC_TRAIT);
}
void AVM1TextFormat::sinit(Class_base* c)
{
	TextFormat::sinit(c);
	c->isSealed = false;
}

void AVM1Selection::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, 0);
	c->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(addListener),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("getFocus","",c->getSystemState()->getBuiltinFunction(getFocus),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("setFocus","",c->getSystemState()->getBuiltinFunction(setFocus),NORMAL_METHOD,false);
}
ASFUNCTIONBODY_ATOM(AVM1Selection,addListener)
{
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener));
	if (listener)
		wrk->getSystemState()->stage->AVM1AddMouseListener(listener.getPtr());
}
ASFUNCTIONBODY_ATOM(AVM1Selection,getFocus)
{
	InteractiveObject* focus = wrk->getSystemState()->stage->getFocusTarget();
	if (focus)
		ret = asAtomHandler::fromString(wrk->getSystemState(),focus->AVM1GetPath());
	else
		ret = asAtomHandler::nullAtom;
}
ASFUNCTIONBODY_ATOM(AVM1Selection,setFocus)
{
	if (argslen==0)
		return;
	if (asAtomHandler::isNull(args[0]) || asAtomHandler::isUndefined(args[0]))
		wrk->getSystemState()->stage->setFocusTarget(nullptr);
	else if (asAtomHandler::is<InteractiveObject>(args[0]))
		wrk->getSystemState()->stage->setFocusTarget(asAtomHandler::as<InteractiveObject>(args[0]));
	else
		LOG(LOG_ERROR,"invalid object for Selection.setFocus:"<<asAtomHandler::toDebugString(args[0]));
}

