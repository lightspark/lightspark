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
#include "scripting/toplevel/Error.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "swf.h"

using namespace std;
using namespace lightspark;

void Multitouch::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructorNotInstantiatable, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("inputMode","",Class<IFunction>::getFunction(getInputMode),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("maxTouchPoints","",Class<IFunction>::getFunction(getMaxTouchPoints),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportedGestures","",Class<IFunction>::getFunction(getSupportedGestures),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportsGestureEvents","",Class<IFunction>::getFunction(getSupportsGestureEvents),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("supportsTouchEvents","",Class<IFunction>::getFunction(getSupportsTouchEvents),GETTER_METHOD,false);
}

ASFUNCTIONBODY(Multitouch, getInputMode)
{
	LOG(LOG_NOT_IMPLEMENTED,"Multitouch not supported");
	return Class<ASString>::getInstanceS("gesture");
}

ASFUNCTIONBODY(Multitouch, getMaxTouchPoints)
{
	LOG(LOG_NOT_IMPLEMENTED,"Multitouch not supported");
	return abstract_i(1);
}

ASFUNCTIONBODY(Multitouch, getSupportedGestures)
{
	LOG(LOG_NOT_IMPLEMENTED,"Multitouch not supported");
	return Class<Vector>::getInstanceS(); 
}
ASFUNCTIONBODY(Multitouch, getSupportsGestureEvents)
{
	LOG(LOG_NOT_IMPLEMENTED,"Multitouch not supported");
	return abstract_b(false); 
}
ASFUNCTIONBODY(Multitouch, getSupportsTouchEvents)
{
	LOG(LOG_NOT_IMPLEMENTED,"Multitouch not supported");
	return abstract_b(false); 
}
