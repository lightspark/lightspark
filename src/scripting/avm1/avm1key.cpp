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

#include "scripting/avm1/avm1key.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/flashdisplay.h"
#include "backends/input.h"

using namespace std;
using namespace lightspark;

void AVM1Key::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("BACKSPACE",nsNameAndKind(),asAtomHandler::fromUInt(8),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CAPSLOCK",nsNameAndKind(),asAtomHandler::fromUInt(20),CONSTANT_TRAIT);
	c->setVariableAtomByQName("CONTROL",nsNameAndKind(),asAtomHandler::fromUInt(17),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DELETEKEY",nsNameAndKind(),asAtomHandler::fromUInt(46),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DOWN",nsNameAndKind(),asAtomHandler::fromUInt(40),CONSTANT_TRAIT);
	c->setVariableAtomByQName("END",nsNameAndKind(),asAtomHandler::fromUInt(35),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ENTER",nsNameAndKind(),asAtomHandler::fromUInt(13),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ESCAPE",nsNameAndKind(),asAtomHandler::fromUInt(27),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HOME",nsNameAndKind(),asAtomHandler::fromUInt(36),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INSERT",nsNameAndKind(),asAtomHandler::fromUInt(45),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LEFT",nsNameAndKind(),asAtomHandler::fromUInt(37),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PGDN",nsNameAndKind(),asAtomHandler::fromUInt(34),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PGUP",nsNameAndKind(),asAtomHandler::fromUInt(33),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RIGHT",nsNameAndKind(),asAtomHandler::fromUInt(39),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SHIFT",nsNameAndKind(),asAtomHandler::fromUInt(16),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SPACE",nsNameAndKind(),asAtomHandler::fromUInt(32),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TAB",nsNameAndKind(),asAtomHandler::fromUInt(9),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UP",nsNameAndKind(),asAtomHandler::fromUInt(38),CONSTANT_TRAIT);

	c->setDeclaredMethodByQName("isDown","",c->getSystemState()->getBuiltinFunction(isDown),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(addListener),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(removeListener),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("getCode","",c->getSystemState()->getBuiltinFunction(getCode),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("getAscii","",c->getSystemState()->getBuiltinFunction(getAscii),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(AVM1Key,isDown)
{
	int key;
	ARG_CHECK(ARG_UNPACK(key));
	asAtomHandler::setBool(ret,wrk->getSystemState()->getInputThread()->isKeyDown((AS3KeyCode)key));
}
ASFUNCTIONBODY_ATOM(AVM1Key,addListener)
{
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener));
	if (listener)
		wrk->getSystemState()->stage->AVM1AddKeyboardListener(listener.getPtr());
}
ASFUNCTIONBODY_ATOM(AVM1Key,removeListener)
{
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener));
	if (listener)
		wrk->getSystemState()->stage->AVM1RemoveKeyboardListener(listener.getPtr());
}
ASFUNCTIONBODY_ATOM(AVM1Key,getCode)
{
	AS3KeyCode c = wrk->getSystemState()->getInputThread()->getLastKeyDown();
	if (c==0)
		c = wrk->getSystemState()->getInputThread()->getLastKeyUp();
	asAtomHandler::setInt(ret,wrk,c);
}
ASFUNCTIONBODY_ATOM(AVM1Key,getAscii)
{
	SDL_Keycode c = wrk->getSystemState()->getInputThread()->getLastKeyCode();
	if ((c < 0x20) || (c > 0x80))
		c = 0;
	SDL_Keymod m = wrk->getSystemState()->getInputThread()->getLastKeyMod();
	if (m & KMOD_SHIFT)
		c = toupper(c);
	asAtomHandler::setInt(ret,wrk,c);
}



void AVM1Mouse::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);

	c->setDeclaredMethodByQName("hide","",c->getSystemState()->getBuiltinFunction(hide),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("show","",c->getSystemState()->getBuiltinFunction(show),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("addListener","",c->getSystemState()->getBuiltinFunction(addListener),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("removeListener","",c->getSystemState()->getBuiltinFunction(removeListener),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(AVM1Mouse,hide)
{
	wrk->getSystemState()->showMouseCursor(false);
}
ASFUNCTIONBODY_ATOM(AVM1Mouse,show)
{
	wrk->getSystemState()->showMouseCursor(true);
}
ASFUNCTIONBODY_ATOM(AVM1Mouse,addListener)
{
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener));
	if (listener)
		wrk->getSystemState()->stage->AVM1AddMouseListener(listener.getPtr());
}
ASFUNCTIONBODY_ATOM(AVM1Mouse,removeListener)
{
	_NR<ASObject> listener;
	ARG_CHECK(ARG_UNPACK(listener));
	if (listener)
		wrk->getSystemState()->stage->AVM1RemoveMouseListener(listener.getPtr());
}

