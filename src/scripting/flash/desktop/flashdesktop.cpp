/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/abc.h"
#include "scripting/class.h"

#include "scripting/flash/desktop/flashdesktop.h"

using namespace std;
using namespace lightspark;

void NativeApplication::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());

	c->setDeclaredMethodByQName("nativeApplication", "", Class<IFunction>::getFunction(_getNativeApplication), GETTER_METHOD, false);
	c->setDeclaredMethodByQName("addEventListener", "", Class<IFunction>::getFunction(addEventListener), NORMAL_METHOD, true);
}

void NativeApplication::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(NativeApplication,_constructor)
{
	return NULL;
}

//  Should actually be a Singleton
ASFUNCTIONBODY(NativeApplication, _getNativeApplication)
{
	return Class<NativeApplication>::getInstanceS();
}

ASFUNCTIONBODY(NativeApplication, addEventListener)
{
	EventDispatcher* th = Class<EventDispatcher>::cast(obj);
	EventDispatcher::addEventListener(obj, args, argslen);
	if (args[0]->toString() == "invoke")
	{
		th->incRef();
		getVm()->addEvent(_MR(th), _MR(Class<InvokeEvent>::getInstanceS()));
	}

	return NULL;
}
