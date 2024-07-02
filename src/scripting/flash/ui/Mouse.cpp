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
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "platforms/engineutils.h"
#include "swf.h"

using namespace std;
using namespace lightspark;

void Mouse::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("hide","",c->getSystemState()->getBuiltinFunction(hide),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("show","",c->getSystemState()->getBuiltinFunction(show),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("cursor","",c->getSystemState()->getBuiltinFunction(getCursor,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("cursor","",c->getSystemState()->getBuiltinFunction(setCursor),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportsCursor","",c->getSystemState()->getBuiltinFunction(getSupportsCursor,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportsNativeCursor","",c->getSystemState()->getBuiltinFunction(getSupportsNativeCursor,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("registerCursor","",c->getSystemState()->getBuiltinFunction(registerCursor),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("unregisterCursor","",c->getSystemState()->getBuiltinFunction(unregisterCursor),NORMAL_METHOD,false);
}

ASFUNCTIONBODY_ATOM(Mouse, hide)
{
	wrk->getSystemState()->showMouseCursor(false);
}

ASFUNCTIONBODY_ATOM(Mouse, show)
{
	wrk->getSystemState()->showMouseCursor(true);
}

ASFUNCTIONBODY_ATOM(Mouse, getCursor)
{
	ret = asAtomHandler::fromString(wrk->getSystemState()
									,wrk->getSystemState()->getEngineData()->getMouseCursor(wrk->getSystemState()));
}

ASFUNCTIONBODY_ATOM(Mouse, setCursor)
{
	tiny_string cursorName;
	ARG_CHECK(ARG_UNPACK(cursorName));
	wrk->getSystemState()->getEngineData()->setMouseCursor(wrk->getSystemState(),cursorName);
}

ASFUNCTIONBODY_ATOM(Mouse, getSupportsCursor)
{
	asAtomHandler::setBool(ret,true);
}

ASFUNCTIONBODY_ATOM(Mouse, getSupportsNativeCursor)
{
	asAtomHandler::setBool(ret,false); // until registerCursor() is implemented
}

ASFUNCTIONBODY_ATOM(Mouse, registerCursor)
{
	tiny_string cursorName;
	_NR<MouseCursorData> mousecursordata;
	ARG_CHECK(ARG_UNPACK(cursorName) (mousecursordata));
	LOG(LOG_NOT_IMPLEMENTED,"Mouse.registerCursor is not implemented");
}
ASFUNCTIONBODY_ATOM(Mouse, unregisterCursor)
{
	tiny_string cursorName;
	ARG_CHECK(ARG_UNPACK(cursorName));
	LOG(LOG_NOT_IMPLEMENTED,"Mouse.unregisterCursor is not implemented");
}
void MouseCursor::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setVariableAtomByQName("ARROW",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"arrow"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("AUTO",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"auto"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BUTTON",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"button"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("HAND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"hand"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("IBEAM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ibeam"),CONSTANT_TRAIT);
}
MouseCursorData::MouseCursorData(ASWorker* wrk, Class_base *c):ASObject(wrk,c),frameRate(0)
{
}

void MouseCursorData::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL | CLASS_SEALED);
	REGISTER_GETTER_SETTER(c, data);
	REGISTER_GETTER_SETTER(c, frameRate);
	REGISTER_GETTER_SETTER(c, hotSpot);

}
ASFUNCTIONBODY_GETTER_SETTER(MouseCursorData, data)
ASFUNCTIONBODY_GETTER_SETTER(MouseCursorData, frameRate)
ASFUNCTIONBODY_GETTER_SETTER(MouseCursorData, hotSpot)

ASFUNCTIONBODY_ATOM(MouseCursorData,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"MouseCursorData is not implemented");
}
