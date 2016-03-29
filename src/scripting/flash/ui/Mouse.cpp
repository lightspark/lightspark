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
