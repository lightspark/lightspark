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

#include "scripting/flash/ui/Keyboard.h"
#include "scripting/class.h"
#include "scripting/toplevel/ASString.h"
#include "backends/input.h"

using namespace std;
using namespace lightspark;

void Keyboard::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("capsLock","",Class<IFunction>::getFunction(capsLock),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hasVirtualKeyboard","",Class<IFunction>::getFunction(hasVirtualKeyboard),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("numLock","",Class<IFunction>::getFunction(numLock),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("physicalKeyboardType","",Class<IFunction>::getFunction(physicalKeyboardType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("isAccessible","",Class<IFunction>::getFunction(isAccessible),NORMAL_METHOD,true);

	// key code constants
	const std::vector<KeyNameCodePair>& keys = getSys()->getInputThread()->getKeyNamesAndCodes();
	std::vector<KeyNameCodePair>::const_iterator it;
	for (it=keys.begin(); it!=keys.end(); ++it)
	{
		c->setVariableByQName(it->keyname,"",Class<UInteger>::getInstanceS(it->keycode),DECLARED_TRAIT);
	}
}

ASFUNCTIONBODY(Keyboard, capsLock)
{
	// gdk functions can be called only from the input thread!
	//return abstract_b(gdk_keymap_get_caps_lock_state(gdk_keymap_get_default()));

	LOG(LOG_NOT_IMPLEMENTED, "Keyboard::capsLock");
	return abstract_b(false);
}

ASFUNCTIONBODY(Keyboard, hasVirtualKeyboard)
{
	return abstract_b(false);
}

ASFUNCTIONBODY(Keyboard, numLock)
{
	LOG(LOG_NOT_IMPLEMENTED, "Keyboard::numLock");
	return abstract_b(false);
}

ASFUNCTIONBODY(Keyboard, physicalKeyboardType)
{
	return Class<ASString>::getInstanceS("alphanumeric");
}

ASFUNCTIONBODY(Keyboard, isAccessible)
{
	return abstract_b(false);
}

void KeyboardType::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("ALPHANUMERIC","",Class<ASString>::getInstanceS("alphanumeric"),DECLARED_TRAIT);
	c->setVariableByQName("KEYPAD","",Class<ASString>::getInstanceS("keypad"),DECLARED_TRAIT);
	c->setVariableByQName("NONE","",Class<ASString>::getInstanceS("none"),DECLARED_TRAIT);
}

void KeyLocation::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("LEFT","",Class<UInteger>::getInstanceS(1),DECLARED_TRAIT);
	c->setVariableByQName("NUM_PAD","",Class<UInteger>::getInstanceS(3),DECLARED_TRAIT);
	c->setVariableByQName("RIGHT","",Class<UInteger>::getInstanceS(2),DECLARED_TRAIT);
	c->setVariableByQName("STANDARD","",Class<UInteger>::getInstanceS(0),DECLARED_TRAIT);
}
