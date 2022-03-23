/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "scripting/argconv.h"
#include "scripting/flash/filesystem/flashfilesystem.h"
#include "scripting/toplevel/Vector.h"

using namespace std;
using namespace lightspark;

void NativeApplication::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_FINAL | CLASS_SEALED);
	c->setDeclaredMethodByQName("nativeApplication", "", Class<IFunction>::getFunction(c->getSystemState(),_getNativeApplication), GETTER_METHOD, false);
	c->setDeclaredMethodByQName("addEventListener", "", Class<IFunction>::getFunction(c->getSystemState(),addEventListener), NORMAL_METHOD, true);
}

void NativeApplication::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(NativeApplication,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
}

//  Should actually be a Singleton
ASFUNCTIONBODY_ATOM(NativeApplication, _getNativeApplication)
{
	ret = asAtomHandler::fromObject(Class<NativeApplication>::getInstanceS(wrk));
}

ASFUNCTIONBODY_ATOM(NativeApplication, addEventListener)
{
	EventDispatcher* th = asAtomHandler::as<EventDispatcher>(obj);
	EventDispatcher::addEventListener(ret,wrk,obj, args, argslen);
	if (asAtomHandler::toString(args[0],wrk) == "invoke")
	{
		th->incRef();
		getVm(th->getSystemState())->addEvent(_MR(th), _MR(Class<InvokeEvent>::getInstanceS(wrk)));
	}
}

void NativeDragManager::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_FINAL | CLASS_SEALED);
	REGISTER_GETTER(c,isSupported);
}
ASFUNCTIONBODY_GETTER(NativeDragManager,isSupported)


void NativeProcess::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("start", "", Class<IFunction>::getFunction(c->getSystemState(),start), NORMAL_METHOD, true);
	REGISTER_GETTER_RESULTTYPE(c,isSupported,Boolean);
}
ASFUNCTIONBODY_GETTER(NativeProcess,isSupported)

ASFUNCTIONBODY_ATOM(NativeProcess,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
}
ASFUNCTIONBODY_ATOM(NativeProcess, start)
{
	LOG(LOG_NOT_IMPLEMENTED,"NativeProcess.start does nothing");
}

NativeProcessStartupInfo::NativeProcessStartupInfo(ASWorker* wrk, Class_base* c):ASObject(wrk,c)
{
}

void NativeProcessStartupInfo::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,arguments,Vector);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,executable,ASFile);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,workingDirectory,ASFile);
	
}
ASFUNCTIONBODY_ATOM(NativeProcessStartupInfo,_constructor)
{
}
ASFUNCTIONBODY_GETTER_SETTER(NativeProcessStartupInfo,arguments)
ASFUNCTIONBODY_GETTER_SETTER(NativeProcessStartupInfo,executable)
ASFUNCTIONBODY_GETTER_SETTER(NativeProcessStartupInfo,workingDirectory)
