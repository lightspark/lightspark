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

#include "scripting/flash/events/LocalConnectionEvent.h"
#include "scripting/flash/net/LocalConnection.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

LocalConnectionEvent::LocalConnectionEvent(LocalConnection* _sender,
										   uint32_t _nameID,
										   uint32_t _methodID,
										   asAtom* _args,
										   uint32_t _numargs,
										   LOCALCONNECTION_EVENTTYPE _type): Event(nullptr,nullptr,"LocalConnectionEvent")
	,nameID(_nameID),methodID(_methodID),sender(_sender),eventtype(_type)
{
	numargs=_numargs;
	sender->incRef();
	if (eventtype == LOCALCONNECTION_SEND)
	{
		args = new asAtom[numargs];
		for (uint32_t i = 0; i < numargs; i++)
		{
			args[i]=_args[i];
			ASATOM_INCREF(args[i]);
		}
	}
}
LocalConnectionEvent::~LocalConnectionEvent()
{
	sender->decRef();
	if (eventtype == LOCALCONNECTION_SEND)
	{
		for (uint32_t i = 0; i < numargs; i++)
		{
			ASATOM_DECREF(args[i]);
		}
	}
}

