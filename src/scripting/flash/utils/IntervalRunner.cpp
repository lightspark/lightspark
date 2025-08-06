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

#include "scripting/abc.h"
#include "asobject.h"
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/flash/utils/IntervalRunner.h"
#include "scripting/flash/utils/IntervalManager.h"

using namespace std;
using namespace lightspark;

IntervalRunner::IntervalRunner(IntervalRunner::INTERVALTYPE _type, uint32_t _id, asAtom _callback, asAtom* _args,
		const unsigned int _argslen, asAtom _obj):
	EventDispatcher(nullptr,nullptr),type(_type), id(_id), callback(_callback),obj(_obj),argslen(_argslen),running(false),finished(false)
{
	ASATOM_ADDSTOREDMEMBER(callback);
	ASATOM_ADDSTOREDMEMBER(obj);
	args = new asAtom[argslen];
	for(uint32_t i=0; i<argslen; i++)
	{
		args[i] = _args[i];
		ASATOM_ADDSTOREDMEMBER(args[i]);
	}
}

IntervalRunner::~IntervalRunner()
{
	ASATOM_REMOVESTOREDMEMBER(callback);
	ASATOM_REMOVESTOREDMEMBER(obj);
	for(uint32_t i=0; i<argslen; i++)
		ASATOM_REMOVESTOREDMEMBER(args[i]);
	delete[] args;
}

void IntervalRunner::tick()
{
	SystemState* sys = asAtomHandler::getObjectNoCheck(callback)->getSystemState();
	if (sys->isShuttingDown())
		return;
	RELEASE_WRITE(running,true);

	_R<FunctionEvent> event(new (sys->unaccountedMemory) FunctionEvent(callback, obj, args, argslen));
	getVm(sys)->addEvent(NullRef,event);
	event->wait();
	RELEASE_WRITE(running,false);
	if(type == TIMEOUT)
	{
		//remove ourselves from the active intervals list
		sys->intervalManager->clearInterval(id, true);
	}
	if (ACQUIRE_READ(finished))
	{
		// clearInterval was called from inside the callback, so we delete ourselves after the callback is completed
		sys->removeJob(this);
	}
}

void IntervalRunner::tickFence()
{
	delete this;
}

void IntervalRunner::removeJob()
{
	if (ACQUIRE_READ(running))
	{
		// clearInterval was called from inside the callback, so we don't delete ourselves (yet)
		RELEASE_WRITE(finished,true);
	}
	else
		asAtomHandler::getObjectNoCheck(callback)->getSystemState()->removeJob(this);
}

