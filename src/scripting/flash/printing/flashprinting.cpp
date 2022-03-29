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

#include "scripting/flash/printing/flashprinting.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "compat.h"

using namespace lightspark;

PrintJob::PrintJob(ASWorker* wrk, Class_base* c):
	EventDispatcher(wrk,c),isSupported(false)
{
}

void PrintJob::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	REGISTER_GETTER(c,isSupported);
}
ASFUNCTIONBODY_GETTER(PrintJob, isSupported);

ASFUNCTIONBODY_ATOM(PrintJob,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, NULL, 0);
	//PrintJob* th=Class<PrintJob>::cast(obj);
	LOG(LOG_NOT_IMPLEMENTED,"PrintJob is not implemented");
}

PrintJobOptions::PrintJobOptions(ASWorker* wrk, Class_base* c):
	ASObject(wrk,c)
{
}

void PrintJobOptions::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(PrintJobOptions,_constructor)
{
	//PrintJobOptions* th=Class<PrintJobOptions>::cast(obj);
	LOG(LOG_NOT_IMPLEMENTED,"PrintJobOptions is not implemented");
}

void PrintJobOrientation::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED| CLASS_FINAL);
	c->setVariableAtomByQName("LANDSCAPE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"landscape"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PORTRAIT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"portrait"),CONSTANT_TRAIT);
}
