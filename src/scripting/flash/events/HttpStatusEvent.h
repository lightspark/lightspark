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

#ifndef SCRIPTING_FLASH_EVENTS_HTTPSTATUSEVENT_H
#define SCRIPTING_FLASH_EVENTS_HTTPSTATUSFOCUSEVENT_H 1

#include "scripting/flash/events/flashevents.h"

namespace lightspark
{
class Array;

class HTTPStatusEvent: public Event
{
public:
	HTTPStatusEvent(ASWorker* wrk, Class_base* c, const tiny_string& _type="httpStatus");
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
    ASFUNCTION_ATOM(_toString);
    ASPROPERTY_GETTER_SETTER(bool, redirected);
    ASPROPERTY_GETTER_SETTER(_NR<Array>, responseHeaders);
	ASPROPERTY_GETTER_SETTER(asAtom, responseURL);
    ASPROPERTY_GETTER(int, status);
};

}
#endif /* SCRIPTING_FLASH_EVENTS_HTTPSTATUSEVENT_H */
