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
#include "scripting/flash/utils/IntervalManager.h"
#include "asobject.h"
#include "scripting/class.h"
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/flash/errors/flasherrors.h"

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
		getSys()->removeJob((*it).second);
		runners.erase(it++);
	}
}

uint32_t IntervalManager::setInterval(asAtom callback, asAtom* args, const unsigned int argslen, asAtom obj, uint32_t interval)
{
	Locker l(mutex);

	if (interval == 0)
	{
		// interval is 0, so we make sure the function is executed for the first time ahead of the next event in the event loop
		_R<FunctionAsyncEvent> event(new (asAtomHandler::getObject(callback)->getSystemState()->unaccountedMemory) FunctionAsyncEvent(callback, obj, args, argslen));
		getVm(asAtomHandler::getObject(callback)->getSystemState())->prependEvent(NullRef,event);
	}
	uint32_t id = getFreeID();
	IntervalRunner* runner = new (asAtomHandler::getObject(callback)->getSystemState()->unaccountedMemory)
		IntervalRunner(IntervalRunner::INTERVAL, id, callback, args, argslen, obj);

	//Add runner as tickjob
	getSys()->addTick(interval, runner);
	//Add runner to map
	runners[id] = runner;
	//Increment currentID
	currentID++;

	return currentID-1;
}
uint32_t IntervalManager::setTimeout(asAtom callback, asAtom* args, const unsigned int argslen, asAtom obj, uint32_t interval)
{
	Locker l(mutex);

	uint32_t id = getFreeID();
	IntervalRunner* runner = new (asAtomHandler::getObject(callback)->getSystemState()->unaccountedMemory)
		IntervalRunner(IntervalRunner::TIMEOUT, id, callback, args, argslen, obj);

	//Add runner as waitjob
	asAtomHandler::getObject(callback)->getSystemState()->addWait(interval, runner);
	//Add runner to map
	runners[id] = runner;
	//increment currentID
	currentID++;

	return currentID-1;
}

uint32_t IntervalManager::getFreeID()
{
	//At the first run every currentID will be available. But eventually the currentID will wrap around.
	//Thats why we need to check if the currentID isn't used yet
	while(currentID == 0 || runners.count(currentID) != 0)
		currentID++;
	return currentID;
}

void IntervalManager::clearInterval(uint32_t id, IntervalRunner::INTERVALTYPE type, bool removeJob)
{
	Locker l(mutex);

	std::map<uint32_t,IntervalRunner*>::iterator it = runners.find(id);
	//If the entry exists and the types match, remove its tickjob, delete its intervalRunner and erase their entry
	if(it != runners.end() && (*it).second->getType() == type)
	{
		if(removeJob)
		{
			getSys()->removeJob((*it).second);
		}
		runners.erase(it);
	}
}
