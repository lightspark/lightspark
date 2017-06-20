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

#include "compat.h"
#include "swftypes.h"
#include "scripting/flash/utils/IntervalRunner.h"


namespace lightspark
{

class IntervalManager
{
private:
	Mutex mutex;
	std::map<uint32_t,IntervalRunner*> runners;
	uint32_t currentID;
public:
	IntervalManager();
	~IntervalManager();
	uint32_t setInterval(asAtom callback, asAtom* args, const unsigned int argslen, asAtom obj, const uint32_t interval);
	uint32_t setTimeout(asAtom callback, asAtom* args, const unsigned int argslen, asAtom obj, const uint32_t interval);
	uint32_t getFreeID();
	void clearInterval(uint32_t id, IntervalRunner::INTERVALTYPE type, bool removeJob);
};

}

#endif /* SCRIPTING_FLASH_UTILS_INTERVALMANAGER_H */
