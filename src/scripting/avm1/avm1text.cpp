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
	c->prototype->setVariableByQName("replaceSel","",c->getSystemState()->getBuiltinFunction(TextField::_replaceSelectedText,1),DYNAMIC_TRAIT);
}
void AVM1TextFormat::sinit(Class_base* c)
{
	TextFormat::sinit(c);
	c->isSealed = false;
}

void AVM1Selection::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, 0);
	c->prototype->setVariableByQName("addListener","",c->getSystemState()->getBuiltinFunction(addListener),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("removeListener","",c->getSystemState()->getBuiltinFunction(removeListener),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getFocus","",c->getSystemState()->getBuiltinFunction(getFocus),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("setFocus","",c->getSystemState()->getBuiltinFunction(setFocus),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getBeginIndex","",c->getSystemState()->getBuiltinFunction(getBeginIndex),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getCaretIndex","",c->getSystemState()->getBuiltinFunction(getCaretIndex),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getEndIndex","",c->getSystemState()->getBuiltinFunction(getEndIndex),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("setSelection","",c->getSystemState()->getBuiltinFunction(setSelection),DYNAMIC_TRAIT);
}
ASFUNCTIONBODY_ATOM(AVM1Selection,addListener)
{
	asAtom listener = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(listener));
	wrk->getSystemState()->stage->AVM1AddFocusListener(listener);
}
ASFUNCTIONBODY_ATOM(AVM1Selection,removeListener)
{
	asAtom listener = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(listener));
	ret = asAtomHandler::fromBool(wrk->getSystemState()->stage->AVM1RemoveFocusListener(listener));
}
ASFUNCTIONBODY_ATOM(AVM1Selection,getFocus)
{
	InteractiveObject* focus = wrk->getSystemState()->stage->getFocusTarget();
	if (focus && focus != wrk->getSystemState()->stage)
		ret = asAtomHandler::fromString(wrk->getSystemState(),focus->AVM1GetPath());
	else
		ret = asAtomHandler::nullAtom;
}
ASFUNCTIONBODY_ATOM(AVM1Selection,setFocus)
{
	if (argslen==0)
		return;
	ret = asAtomHandler::falseAtom;
	if (asAtomHandler::isNull(args[0]) || asAtomHandler::isUndefined(args[0]))
	{
		if (wrk->getSystemState()->stage->setFocusTarget(nullptr))
			ret = asAtomHandler::trueAtom;
	}
	else if (asAtomHandler::is<InteractiveObject>(args[0]))
	{
		if (wrk->getSystemState()->stage->setFocusTarget(asAtomHandler::as<InteractiveObject>(args[0])))
			ret = asAtomHandler::trueAtom;
	}
	else
		LOG(LOG_ERROR,"invalid object for Selection.setFocus:"<<asAtomHandler::toDebugString(args[0]));
}
ASFUNCTIONBODY_ATOM(AVM1Selection,getBeginIndex)
{
	InteractiveObject* focus = wrk->getSystemState()->stage->getFocusTarget();
	if (focus && focus->is<TextField>())
		ret = asAtomHandler::fromInt(focus->as<TextField>()->selectionBeginIndex);
	else
		ret = asAtomHandler::fromInt(-1);
}
ASFUNCTIONBODY_ATOM(AVM1Selection,getCaretIndex)
{
	InteractiveObject* focus = wrk->getSystemState()->stage->getFocusTarget();
	if (focus && focus->is<TextField>())
		ret = asAtomHandler::fromInt(focus->as<TextField>()->caretIndex);
	else
		ret = asAtomHandler::fromInt(-1);
}
ASFUNCTIONBODY_ATOM(AVM1Selection,getEndIndex)
{
	InteractiveObject* focus = wrk->getSystemState()->stage->getFocusTarget();
	if (focus && focus->is<TextField>())
		ret = asAtomHandler::fromInt(focus->as<TextField>()->selectionEndIndex);
	else
		ret = asAtomHandler::fromInt(-1);
}
ASFUNCTIONBODY_ATOM(AVM1Selection,setSelection)
{
	InteractiveObject* focus = wrk->getSystemState()->stage->getFocusTarget();
	if (focus && focus->is<TextField>())
	{
		switch (argslen) {
			case 0:
				break;
			case 1:
			{
				TextField* tf =focus->as<TextField>();
				tf->selectionBeginIndex=asAtomHandler::toInt(args[0]);
				tf->selectionEndIndex=tf->getTextCharCount();
				tf->caretIndex=tf->selectionEndIndex;
				break;
			}
			default:
			{
				asAtom tf = asAtomHandler::fromObjectNoPrimitive(focus);
				TextField::_setSelection(ret,wrk,tf,args,argslen);
				break;
			}
		}
	}
}

void AVM1StyleSheet::sinit(Class_base* c)
{
	StyleSheet::sinit(c);
	c->prototype->setDeclaredMethodByQName("getStyleNames","",c->getSystemState()->getBuiltinFunction(_getStyleNames),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("setStyle","",c->getSystemState()->getBuiltinFunction(setStyle),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("getStyle","",c->getSystemState()->getBuiltinFunction(getStyle),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("parseCSS","",c->getSystemState()->getBuiltinFunction(parseCSS),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("clear","",c->getSystemState()->getBuiltinFunction(clear),NORMAL_METHOD,false);
	c->prototype->setDeclaredMethodByQName("transform","",c->getSystemState()->getBuiltinFunction(transform),NORMAL_METHOD,false);
}
