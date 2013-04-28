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
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("hide","",Class<IFunction>::getFunction(hide),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("show","",Class<IFunction>::getFunction(show),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("cursor","",Class<IFunction>::getFunction(getCursor),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("cursor","",Class<IFunction>::getFunction(setCursor),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportsCursor","",Class<IFunction>::getFunction(getSupportsCursor),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportsNativeCursor","",Class<IFunction>::getFunction(getSupportsNativeCursor),GETTER_METHOD,false);
}

ASFUNCTIONBODY(Mouse, _constructor)
{
	throwError<ArgumentError>(kCantInstantiateError, "Mouse");
	return NULL;
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
	return Class<ASString>::getInstanceS("auto");
}

ASFUNCTIONBODY(Mouse, setCursor)
{
	tiny_string cursorName;
	ARG_UNPACK(cursorName);
	if (cursorName != "auto")
		throwError<ArgumentError>(kInvalidEnumError, "cursor");
	return NULL;
}

ASFUNCTIONBODY(Mouse, getSupportsCursor)
{
	return abstract_b(true);
}

ASFUNCTIONBODY(Mouse, getSupportsNativeCursor)
{
	return abstract_b(false); // until registerCursor() is implemented
}
