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

#ifndef SCRIPTING_FLASH_EVENTS_STATUSEVENT_H
#define SCRIPTING_FLASH_EVENTS_STATUSEVENT_H 1

#include "scripting/flash/events/flashevents.h"

namespace lightspark
{

class StatusEvent: public Event
{
private:
	Event* cloneImpl() const override;
public:
	StatusEvent(ASWorker* wrk, Class_base* c, const tiny_string& _level="", asAtom _code = asAtomHandler::nullAtom);
	~StatusEvent();
	static void sinit(Class_base*);
	ASPROPERTY_GETTER_SETTER(asAtom, code);
	ASPROPERTY_GETTER_SETTER(tiny_string, level);
	ASFUNCTION_ATOM(_toString);
};

}
#endif /* SCRIPTING_FLASH_EVENTS_STATUSEVENT_H */
