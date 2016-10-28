/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2013  Antti Ajanki (antti.ajanki@iki.fi)

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

#include "scripting/flash/ui/Mouse.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Error.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "swf.h"

using namespace std;
using namespace lightspark;

void Mouse::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("hide","",Class<IFunction>::getFunction(c->getSystemState(),hide),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("show","",Class<IFunction>::getFunction(c->getSystemState(),show),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("cursor","",Class<IFunction>::getFunction(c->getSystemState(),getCursor),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("cursor","",Class<IFunction>::getFunction(c->getSystemState(),setCursor),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportsCursor","",Class<IFunction>::getFunction(c->getSystemState(),getSupportsCursor),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportsNativeCursor","",Class<IFunction>::getFunction(c->getSystemState(),getSupportsNativeCursor),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("registerCursor","",Class<IFunction>::getFunction(c->getSystemState(),registerCursor),NORMAL_METHOD,false);
}

ASFUNCTIONBODY(Mouse, hide)
{
	getSys()->showMouseCursor(false);
	return NULL;
}

ASFUNCTIONBODY(Mouse, show)
{
	getSys()->showMouseCursor(true);
	return NULL;
}

ASFUNCTIONBODY(Mouse, getCursor)
{
	return abstract_s(getSys(),"auto");
}

ASFUNCTIONBODY(Mouse, setCursor)
{
	tiny_string cursorName;
	ARG_UNPACK(cursorName);
	if (cursorName != "auto")
		LOG(LOG_NOT_IMPLEMENTED,"setting mouse cursor is not implemented");
	return NULL;
}

ASFUNCTIONBODY(Mouse, getSupportsCursor)
{
	return abstract_b(getSys(),true);
}

ASFUNCTIONBODY(Mouse, getSupportsNativeCursor)
{
	return abstract_b(getSys(),false); // until registerCursor() is implemented
}

ASFUNCTIONBODY(Mouse, registerCursor)
{
	tiny_string cursorName;
	_NR<MouseCursorData> mousecursordata;
	ARG_UNPACK(cursorName) (mousecursordata);
	LOG(LOG_NOT_IMPLEMENTED,"Mouse.registerCursor is not implemented");
	return NULL;
}
void MouseCursor::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("ARROW","",abstract_s(c->getSystemState(),"arrow"),CONSTANT_TRAIT);
	c->setVariableByQName("AUTO","",abstract_s(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableByQName("BUTTON","",abstract_s(c->getSystemState(),"button"),CONSTANT_TRAIT);
	c->setVariableByQName("HAND","",abstract_s(c->getSystemState(),"hand"),CONSTANT_TRAIT);
	c->setVariableByQName("IBEAM","",abstract_s(c->getSystemState(),"ibeam"),CONSTANT_TRAIT);
}
void MouseCursorData::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	REGISTER_GETTER_SETTER(c, data);
	REGISTER_GETTER_SETTER(c, frameRate);
	REGISTER_GETTER_SETTER(c, hotSpot);

}
ASFUNCTIONBODY_GETTER_SETTER(MouseCursorData, data);
ASFUNCTIONBODY_GETTER_SETTER(MouseCursorData, frameRate);
ASFUNCTIONBODY_GETTER_SETTER(MouseCursorData, hotSpot);

ASFUNCTIONBODY(MouseCursorData,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"MouseCursorData is not implemented");
	return NULL;
}
