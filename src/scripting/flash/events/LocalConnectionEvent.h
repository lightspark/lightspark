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

#ifndef SCRIPTING_FLASH_EVENTS_LOCALCONNECTIONEVENT_H
#define SCRIPTING_FLASH_EVENTS_LOCALCONNECTIONEVENT_H 1

#include "scripting/flash/events/flashevents.h"

namespace lightspark
{
enum LOCALCONNECTION_EVENTTYPE
{
	LOCALCONNECTION_CONNECT,
	LOCALCONNECTION_CLOSE,
	LOCALCONNECTION_SEND
};
class LocalConnection;
class LocalConnectionEvent: public Event
{
	friend class SystemState;
private:
	uint32_t nameID;
	uint32_t methodID;
	asAtom* args;
	uint32_t numargs;
	LocalConnection* sender;
	LOCALCONNECTION_EVENTTYPE eventtype;
public:
	LocalConnectionEvent(LocalConnection* _sender,
					 uint32_t _nameID,
					 uint32_t _methodID,
					 asAtom* _args,
					 uint32_t _numargs,
					 LOCALCONNECTION_EVENTTYPE _type);
	~LocalConnectionEvent();
	EVENT_TYPE getEventType() const override { return LOCALCONNECTIONEVENT; }
};

}
#endif /* SCRIPTING_FLASH_EVENTS_LOCALCONNECTIONEVENT_H */
