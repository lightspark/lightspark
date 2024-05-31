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

#ifndef SCRIPTING_FLASH_NET_LOCALCONNECTION_H
#define SCRIPTING_FLASH_NET_LOCALCONNECTION_H

#include "asobject.h"
#include "scripting/flash/events/flashevents.h"

namespace lightspark
{

class LocalConnection: public EventDispatcher
{
private:
	uint32_t connectionNameID;
public:
	LocalConnection(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	void finalize() override;
	bool destruct() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	void prepareShutdown() override;
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER(bool,isSupported);
	ASFUNCTION_ATOM(allowDomain);
	ASFUNCTION_ATOM(allowInsecureDomain);
	ASFUNCTION_ATOM(send);
	ASFUNCTION_ATOM(connect);
	ASFUNCTION_ATOM(close);
	ASFUNCTION_ATOM(domain);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,client);
};

}
#endif // SCRIPTING_FLASH_NET_LOCALCONNECTION_H
