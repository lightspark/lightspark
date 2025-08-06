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

#ifndef SCRIPTING_FLASH_UTILS_INTERVALMANAGER_H
#define SCRIPTING_FLASH_UTILS_INTERVALMANAGER_H 1

#include "swftypes.h"


namespace lightspark
{
class IntervalRunner;
class ASWorker;
class IntervalManager
{
private:
	Mutex mutex;
	std::map<uint32_t,IntervalRunner*> runners;
	uint32_t currentID;
public:
	IntervalManager();
	~IntervalManager();
	void setupTimer(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen,bool isInterval);
	uint32_t getFreeID();
	void clearInterval(uint32_t id, bool isTimeout);
};

}

#endif /* SCRIPTING_FLASH_UTILS_INTERVALMANAGER_H */
