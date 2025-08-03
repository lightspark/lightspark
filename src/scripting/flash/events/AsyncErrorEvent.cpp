/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2025  Ludger Krämer <dbluelle@onlinehome.de>

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

#include "scripting/flash/events/AsyncErrorEvent.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Error.h"


using namespace std;
using namespace lightspark;

AsyncErrorEvent::AsyncErrorEvent(ASWorker* wrk, Class_base* c):ErrorEvent(wrk,c, "asyncError"),error(asAtomHandler::nullAtom)
{
}

void AsyncErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("ASYNC_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"asyncError"),DECLARED_TRAIT);
	REGISTER_GETTER_RESULTTYPE(c,error,ASError);
}

ASFUNCTIONBODY_ATOM(AsyncErrorEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,4);
	ErrorEvent::_constructor(ret,wrk,obj,args,baseClassArgs);
}

ASFUNCTIONBODY_GETTER(AsyncErrorEvent,error);

void AsyncErrorEvent::finalize()
{
	ASATOM_DECREF(error);
}
