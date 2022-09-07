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

#ifndef SCRIPTING_FLASH_SYSTEM_MESSAGECHANNEL_H
#define SCRIPTING_FLASH_SYSTEM_MESSAGECHANNEL_H 1

#include "scripting/flash/events/flashevents.h"
#include <queue>

namespace lightspark
{

class MessageChannel: public EventDispatcher
{
private:
	Mutex messagequeuemutex;
	std::list<ASObject*> messagequeue;
public:
	MessageChannel(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c),sender(nullptr),receiver(nullptr),state("open")
	{
		subtype=SUBTYPE_MESSAGECHANNEL;
	}
	static void sinit(Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	
	ASWorker* sender;
	ASWorker* receiver;
	ASFUNCTION_ATOM(messageAvailable);
	ASPROPERTY_GETTER(tiny_string,state);
	ASFUNCTION_ATOM(_addEventListener);
	ASFUNCTION_ATOM(_removeEventListener);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(close);
	ASFUNCTION_ATOM(receive);
	ASFUNCTION_ATOM(send);
};

}
#endif /* SCRIPTING_FLASH_SYSTEM_MESSAGECHANNELSTATE_H */
