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
#include "scripting/flash/display/DisplayObject.h"
#include "scripting/flash/system/ASWorker.h"
#include "scripting/flash/utils/IntervalRunner.h"
#include "scripting/flash/utils/IntervalManager.h"
#include "scripting/toplevel/AVM1Function.h"
#include "asobject.h"

using namespace std;
using namespace lightspark;

IntervalManager::IntervalManager() : currentID(1)
{
}

IntervalManager::~IntervalManager()
{
	//Run through all running intervals and remove their tickjob, delete their intervalRunner and erase their entry
	std::map<uint32_t,IntervalRunner*>::iterator it = runners.begin();
	while(it != runners.end())
	{
		IntervalRunner* r = (*it).second;
		it = runners.erase(it);
		getSys()->removeJob(r);
	}
}

void IntervalManager::setupTimer(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool isInterval)
{
	Locker l(mutex);
	if (argslen < 2)
		return;
	uint32_t paramstart = 2;
	asAtom func = args[0];
	uint32_t delayarg = 1;
	asAtom o = asAtomHandler::nullAtom;
	if (!asAtomHandler::isFunction(args[0])) // AVM1 also allows arguments object,functionname,interval,params...
	{
		if (argslen < 3)
			return;
		paramstart = 3;
		delayarg = 2;
		ASObject* oref = asAtomHandler::toObject(args[0],wrk);
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id= asAtomHandler::toStringId(args[1],wrk);
		func = asAtomHandler::invalidAtom;
		oref->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NO_INCREF,wrk);
		if (asAtomHandler::isInvalid(func))
		{
			ASObject* pr = oref->getprop_prototype();
			while (pr)
			{
				pr->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NO_INCREF,wrk);
				if (asAtomHandler::isValid(func))
					break;
				pr = pr->getprop_prototype();
			}
		}
		if (asAtomHandler::isInvalid(func) && oref->is<DisplayObject>())
		{
			uint32_t nameidlower = wrk->getSystemState()->getUniqueStringId(asAtomHandler::toString(args[1],wrk).lowercase());
			AVM1Function* f = oref->as<DisplayObject>()->AVM1GetFunction(nameidlower);
			if (f)
				func = asAtomHandler::fromObjectNoPrimitive(f);
		}
		if (!asAtomHandler::isFunction(func))
		{
			uint32_t id = getFreeID();
			asAtomHandler::setInt(ret,(int32_t)id);
			return;
		}
	}
	if (asAtomHandler::isUndefined(args[delayarg]))
		return;
	if (asAtomHandler::isNull(args[delayarg]))
	{
		uint32_t id = getFreeID();
		asAtomHandler::setInt(ret,(int32_t)id);
		return;
	}
	if (!asAtomHandler::isFunction(func))
	{
		uint32_t id = getFreeID();
		asAtomHandler::setInt(ret,(int32_t)id);
		return;
	}
	if (!asAtomHandler::is<AVM1Function>(func))
		o = asAtomHandler::getClosureAtom(func,asAtomHandler::nullAtom);
	else
	{
		// it seems that adobe uses the ObjectReference as "this" for the callback
		if (!asAtomHandler::isFunction(args[0]))
			o = args[0];
		else
			o = asAtomHandler::fromObjectNoPrimitive(asAtomHandler::as<AVM1Function>(func)->getClip());
	}

	//Build arguments array
	asAtom* callbackArgs = LS_STACKALLOC(asAtom,argslen-paramstart);
	uint32_t i;
	for(i=0; i<argslen-paramstart; i++)
		callbackArgs[i] = args[i+paramstart];

	uint32_t interval = asAtomHandler::toInt(args[delayarg]);
	if (interval == 0 && isInterval)
	{
		// interval is 0, so we make sure the function is executed for the first time ahead of the next event in the event loop
		ASATOM_INCREF(func);
		_R<FunctionAsyncEvent> event(new (asAtomHandler::getObject(func)->getSystemState()->unaccountedMemory) FunctionAsyncEvent(func, o, callbackArgs, argslen-paramstart));
		getVm(asAtomHandler::getObject(func)->getSystemState())->prependEvent(NullRef,event);
	}
	uint32_t id = getFreeID();
	IntervalRunner* runner = new (asAtomHandler::getObject(func)->getSystemState()->unaccountedMemory)
		IntervalRunner(isInterval ? IntervalRunner::INTERVAL : IntervalRunner::TIMEOUT, id, func, callbackArgs, argslen-paramstart, o);

	if (isInterval)
	{
		//Add runner as tickjob
		wrk->getSystemState()->addTick(interval, runner);
	}
	else
	{
		//Add runner as waitjob
		wrk->getSystemState()->addWait(interval, runner);
	}
	//Add runner to map
	runners[id] = runner;
	asAtomHandler::setInt(ret,(int32_t)id);
}

uint32_t IntervalManager::getFreeID()
{
	return currentID++;
}

void IntervalManager::clearInterval(uint32_t id, bool isTimeout)
{
	Locker l(mutex);

	std::map<uint32_t,IntervalRunner*>::iterator it = runners.find(id);
	//If the entry exists and the types match, remove its tickjob, delete its intervalRunner and erase their entry
	if(it != runners.end() && (*it).second->getType() == isTimeout? IntervalRunner::TIMEOUT : IntervalRunner::INTERVAL)
	{
		IntervalRunner* r = (*it).second;
		runners.erase(it);
		r->removeJob();
	}
}
