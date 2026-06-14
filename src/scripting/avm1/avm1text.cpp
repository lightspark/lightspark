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
#include "scripting/toplevel/Integer.h"

using namespace std;
using namespace lightspark;

void AVM1TextField::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	InteractiveObject::AVM1SetupMethods(c);
	c->isSealed = false;
	c->prototype->setDeclaredMethodByQName("antiAliasType","",c->getSystemState()->getBuiltinFunction(TextField::_getAntiAliasType),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("antiAliasType","",c->getSystemState()->getBuiltinFunction(TextField::_setAntiAliasType),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("autoSize","",c->getSystemState()->getBuiltinFunction(TextField::_setAutoSize),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("autoSize","",c->getSystemState()->getBuiltinFunction(TextField::_getAutoSize),GETTER_METHOD,false,false);

	c->prototype->setDeclaredMethodByQName("background","",c->getSystemState()->getBuiltinFunction(TextField::_setter_background),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("background","",c->getSystemState()->getBuiltinFunction(TextField::_getter_background),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("backgroundColor","",c->getSystemState()->getBuiltinFunction(TextField::_setter_backgroundColor),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("backgroundColor","",c->getSystemState()->getBuiltinFunction(TextField::_getter_backgroundColor),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("border","",c->getSystemState()->getBuiltinFunction(TextField::_setter_border),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("border","",c->getSystemState()->getBuiltinFunction(TextField::_getter_border),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("borderColor","",c->getSystemState()->getBuiltinFunction(TextField::_setter_borderColor),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("borderColor","",c->getSystemState()->getBuiltinFunction(TextField::_getter_borderColor),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("bottomScroll","",c->getSystemState()->getBuiltinFunction(TextField::_getBottomScrollV,0,Class<Integer>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("condenseWhite","",c->getSystemState()->getBuiltinFunction(TextField::_setter_condenseWhite),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("condenseWhite","",c->getSystemState()->getBuiltinFunction(TextField::_getter_condenseWhite),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("embedFonts","",c->getSystemState()->getBuiltinFunction(TextField::_setEmbedFonts),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("embedFonts","",c->getSystemState()->getBuiltinFunction(TextField::_getEmbedFonts),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("gridFitType","",c->getSystemState()->getBuiltinFunction(TextField::_setGridFitType),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("gridFitType","",c->getSystemState()->getBuiltinFunction(TextField::_getGridFitType),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("hscroll","",c->getSystemState()->getBuiltinFunction(TextField::_setter_scrollH),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("hscroll","",c->getSystemState()->getBuiltinFunction(TextField::_getter_scrollH),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("htmlText","",c->getSystemState()->getBuiltinFunction(TextField::_getHtmlText,0,Class<ASString>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("htmlText","",c->getSystemState()->getBuiltinFunction(TextField::_setHtmlText),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(TextField::_getLength,0,Class<Integer>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("maxChars","",c->getSystemState()->getBuiltinFunction(TextField::_setter_maxChars),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("maxChars","",c->getSystemState()->getBuiltinFunction(TextField::_getter_maxChars),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("maxhscroll","",c->getSystemState()->getBuiltinFunction(TextField::_getMaxScrollH,0,Class<Integer>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("maxscroll","",c->getSystemState()->getBuiltinFunction(TextField::_getMaxScrollV,0,Class<Integer>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("mouseWheelEnabled","",c->getSystemState()->getBuiltinFunction(TextField::_setter_mouseWheelEnabled),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("mouseWheelEnabled","",c->getSystemState()->getBuiltinFunction(TextField::_getter_mouseWheelEnabled),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("multiline","",c->getSystemState()->getBuiltinFunction(TextField::_setter_multiline),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("multiline","",c->getSystemState()->getBuiltinFunction(TextField::_getter_multiline),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("password","",c->getSystemState()->getBuiltinFunction(TextField::_setdisplayAsPassword),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("password","",c->getSystemState()->getBuiltinFunction(TextField::_getdisplayAsPassword),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("restrict","",c->getSystemState()->getBuiltinFunction(TextField::_getRestrict),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("restrict","",c->getSystemState()->getBuiltinFunction(TextField::_setRestrict),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("scroll","",c->getSystemState()->getBuiltinFunction(TextField::_getter_scrollV),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("scroll","",c->getSystemState()->getBuiltinFunction(TextField::_setter_scrollV),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("selectable","",c->getSystemState()->getBuiltinFunction(TextField::_getter_selectable),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("selectable","",c->getSystemState()->getBuiltinFunction(TextField::_setter_selectable),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("sharpness","",c->getSystemState()->getBuiltinFunction(TextField::_setter_sharpness),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("sharpness","",c->getSystemState()->getBuiltinFunction(TextField::_getter_sharpness),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("styleSheet","",c->getSystemState()->getBuiltinFunction(TextField::_getter_styleSheet),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("styleSheet","",c->getSystemState()->getBuiltinFunction(TextField::_setter_styleSheet),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("text","",c->getSystemState()->getBuiltinFunction(TextField::_setText),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("text","",c->getSystemState()->getBuiltinFunction(TextField::_getText,0,Class<ASString>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("textHeight","",c->getSystemState()->getBuiltinFunction(TextField::_getTextHeight),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("textWidth","",c->getSystemState()->getBuiltinFunction(TextField::_getTextWidth),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("textColor","",c->getSystemState()->getBuiltinFunction(TextField::_setter_textColor),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("textColor","",c->getSystemState()->getBuiltinFunction(TextField::_getter_textColor),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("thickness","",c->getSystemState()->getBuiltinFunction(TextField::_setter_thickness),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("thickness","",c->getSystemState()->getBuiltinFunction(TextField::_getter_thickness),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("type","",c->getSystemState()->getBuiltinFunction(TextField::_setter_type),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("type","",c->getSystemState()->getBuiltinFunction(TextField::_getter_type),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("wordWrap","",c->getSystemState()->getBuiltinFunction(TextField::_setWordWrap),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("wordWrap","",c->getSystemState()->getBuiltinFunction(TextField::_getWordWrap),GETTER_METHOD,false,false);

	c->prototype->setVariableByQName("getTextFormat","",c->getSystemState()->getBuiltinFunction(TextField::_getTextFormat),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("setTextFormat","",c->getSystemState()->getBuiltinFunction(TextField::_setTextFormat),DYNAMIC_TRAIT);

	c->prototype->setVariableByQName("setNewTextFormat","",c->getSystemState()->getBuiltinFunction(TextField::_setDefaultTextFormat,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("replaceSel","",c->getSystemState()->getBuiltinFunction(TextField::_replaceSelectedText,1),DYNAMIC_TRAIT);
}


bool AVM1TextField::AVM1HandleMouseEvent(EventDispatcher* dispatcher, MouseEvent* e)
{
	// Textfield doesn't handle mouse events
	return false;
}
bool AVM1TextField::AVM1HandlePressedEvent(ASObject* dispobj, bool fromMouse)
{
	// Textfield doesn't handle pressed events
	return false;
}
bool AVM1TextField::AVM1HandleKeyboardEvent(KeyboardEvent *e)
{
	if (getSystemState()->stage->getAVM1FocusTarget() != this)
		return false;
	if (e->type =="keyDown"
		&& !e->defaultPrevented
		&& (e->getKeyCode() == AS3KEYCODE_TAB
			|| e->getKeyCode() == AS3KEYCODE_ESCAPE)
		)
		return false;
	return TextField::AVM1HandleKeyboardEvent(e);
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
	InteractiveObject* focus = wrk->getSystemState()->stage->getAVM1FocusTarget();
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
	if (wrk->getSystemState()->stage->setFocusTarget(args[0],false))
		ret = asAtomHandler::trueAtom;
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
	c->prototype->setDeclaredMethodByQName("getStyleNames","",c->getSystemState()->getBuiltinFunction(_getStyleNames),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("setStyle","",c->getSystemState()->getBuiltinFunction(setStyle),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("getStyle","",c->getSystemState()->getBuiltinFunction(getStyle),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("parseCSS","",c->getSystemState()->getBuiltinFunction(parseCSS),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("clear","",c->getSystemState()->getBuiltinFunction(clear),NORMAL_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("transform","",c->getSystemState()->getBuiltinFunction(transform),NORMAL_METHOD,false,false);
}
