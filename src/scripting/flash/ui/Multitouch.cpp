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

#include "scripting/flash/ui/Multitouch.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Error.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "swf.h"

using namespace std;
using namespace lightspark;

void Multitouch::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("maxTouchPoints","",Class<IFunction>::getFunction(c->getSystemState(),getMaxTouchPoints,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportedGestures","",Class<IFunction>::getFunction(c->getSystemState(),getSupportedGestures,0,Class<Vector>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportsGestureEvents","",Class<IFunction>::getFunction(c->getSystemState(),getSupportsGestureEvents,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportsTouchEvents","",Class<IFunction>::getFunction(c->getSystemState(),getSupportsTouchEvents,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	REGISTER_GETTER_SETTER_STATIC_RESULTTYPE(c, inputMode,ASString);
}

ASFUNCTIONBODY_GETTER_SETTER_STATIC(Multitouch, inputMode);

ASFUNCTIONBODY_ATOM(Multitouch, getMaxTouchPoints)
{
	LOG(LOG_NOT_IMPLEMENTED,"Multitouch not supported");
	asAtomHandler::setInt(ret,wrk,1);
}

ASFUNCTIONBODY_ATOM(Multitouch, getSupportedGestures)
{
	LOG(LOG_NOT_IMPLEMENTED,"Multitouch not supported");
	ret = asAtomHandler::fromObject(Class<Vector>::getInstanceS(wrk)); 
}
ASFUNCTIONBODY_ATOM(Multitouch, getSupportsGestureEvents)
{
	LOG(LOG_NOT_IMPLEMENTED,"Multitouch not supported");
	asAtomHandler::setBool(ret,false);
}
ASFUNCTIONBODY_ATOM(Multitouch, getSupportsTouchEvents)
{
	LOG(LOG_NOT_IMPLEMENTED,"Multitouch not supported");
	asAtomHandler::setBool(ret,false);
}

void MultitouchInputMode::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setVariableByQName("GESTURE","",abstract_s(c->getInstanceWorker(),"gesture"),CONSTANT_TRAIT);
	c->setVariableByQName("NONE","",abstract_s(c->getInstanceWorker(),"none"),CONSTANT_TRAIT);
	c->setVariableByQName("TOUCH_POINT","",abstract_s(c->getInstanceWorker(),"touchPoint"),CONSTANT_TRAIT);
}
