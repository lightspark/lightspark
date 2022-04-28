/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef FLASH_NET_DATAGRAMSOCKET_H_
#define FLASH_NET_DATAGRAMSOCKET_H_

#include "scripting/flash/events/flashevents.h"

namespace lightspark
{

class DatagramSocket : public EventDispatcher
{
public:
	DatagramSocket(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);

	ASPROPERTY_GETTER(bool,bound);
	ASPROPERTY_GETTER(bool,connected);
	ASPROPERTY_GETTER(bool,isSupported);
	ASPROPERTY_GETTER(tiny_string,localAddress);
	ASPROPERTY_GETTER(int,localPort);
	ASPROPERTY_GETTER(tiny_string,remoteAddress);
	ASPROPERTY_GETTER(int,remotePort);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_bind);
	ASFUNCTION_ATOM(_close);
	ASFUNCTION_ATOM(_connect);
	ASFUNCTION_ATOM(_receive);
	ASFUNCTION_ATOM(_send);
};

}

#endif
