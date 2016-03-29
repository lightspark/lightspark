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
	c->setDeclaredMethodByQName("capsLock","",Class<IFunction>::getFunction(c->getSystemState(),capsLock),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hasVirtualKeyboard","",Class<IFunction>::getFunction(c->getSystemState(),hasVirtualKeyboard),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("numLock","",Class<IFunction>::getFunction(c->getSystemState(),numLock),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("physicalKeyboardType","",Class<IFunction>::getFunction(c->getSystemState(),physicalKeyboardType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("isAccessible","",Class<IFunction>::getFunction(c->getSystemState(),isAccessible),NORMAL_METHOD,true);

	// key code constants
	const std::vector<KeyNameCodePair>& keys = getSys()->getInputThread()->getKeyNamesAndCodes();
	std::vector<KeyNameCodePair>::const_iterator it;
	for (it=keys.begin(); it!=keys.end(); ++it)
	{
		c->setVariableByQName(it->keyname,"",abstract_ui(c->getSystemState(),it->keycode),DECLARED_TRAIT);
	}
}

ASFUNCTIONBODY(Keyboard, capsLock)
{
	// gdk functions can be called only from the input thread!
	//return abstract_b(gdk_keymap_get_caps_lock_state(gdk_keymap_get_default()));

	LOG(LOG_NOT_IMPLEMENTED, "Keyboard::capsLock");
	return abstract_b(getSys(),false);
}

ASFUNCTIONBODY(Keyboard, hasVirtualKeyboard)
{
	return abstract_b(getSys(),false);
}

ASFUNCTIONBODY(Keyboard, numLock)
{
	LOG(LOG_NOT_IMPLEMENTED, "Keyboard::numLock");
	return abstract_b(getSys(),false);
}

ASFUNCTIONBODY(Keyboard, physicalKeyboardType)
{
	return abstract_s(getSys(),"alphanumeric");
}

ASFUNCTIONBODY(Keyboard, isAccessible)
{
	return abstract_b(getSys(),false);
}

void KeyboardType::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("ALPHANUMERIC","",abstract_s(c->getSystemState(),"alphanumeric"),DECLARED_TRAIT);
	c->setVariableByQName("KEYPAD","",abstract_s(c->getSystemState(),"keypad"),DECLARED_TRAIT);
	c->setVariableByQName("NONE","",abstract_s(c->getSystemState(),"none"),DECLARED_TRAIT);
}

void KeyLocation::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("LEFT","",abstract_ui(c->getSystemState(),1),DECLARED_TRAIT);
	c->setVariableByQName("NUM_PAD","",abstract_ui(c->getSystemState(),3),DECLARED_TRAIT);
	c->setVariableByQName("RIGHT","",abstract_ui(c->getSystemState(),2),DECLARED_TRAIT);
	c->setVariableByQName("STANDARD","",abstract_ui(c->getSystemState(),0),DECLARED_TRAIT);
}
