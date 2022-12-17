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
#include "scripting/flash/utils/flashutils.h"
#include "asobject.h"
#include "scripting/class.h"
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/IntervalRunner.h"
#include "scripting/flash/utils/IntervalManager.h"

using namespace std;
using namespace lightspark;


IntervalRunner::IntervalRunner(IntervalRunner::INTERVALTYPE _type, uint32_t _id, asAtom _callback, asAtom* _args,
		const unsigned int _argslen, asAtom _obj):
	EventDispatcher(nullptr,nullptr),type(_type), id(_id), callback(_callback),obj(_obj),argslen(_argslen)
{
	args = new asAtom[argslen];
	for(uint32_t i=0; i<argslen; i++)
		args[i] = _args[i];
}

IntervalRunner::~IntervalRunner()
{
	for(uint32_t i=0; i<argslen; i++)
		ASATOM_DECREF(args[i]);
	delete[] args;
}

void IntervalRunner::tick()
{
	if (getSys()->isShuttingDown())
		return;
	//incRef all arguments
	uint32_t i;
	for(i=0; i < argslen; i++)
	{
		ASATOM_INCREF(args[i]);
	}
	ASATOM_INCREF(obj);
	_R<FunctionEvent> event(new (getSys()->unaccountedMemory) FunctionEvent(callback, obj, args, argslen));
	getVm(getSys())->addEvent(NullRef,event);
	event->wait();
	if(type == TIMEOUT)
	{
		//TODO: IntervalRunner deletes itself. Is this allowed?
		//Delete ourselves from the active intervals list
		getSys()->intervalManager->clearInterval(id, TIMEOUT, false);
		//No actions may be performed after this point
	}
}

void IntervalRunner::tickFence()
{
	delete this;
}

