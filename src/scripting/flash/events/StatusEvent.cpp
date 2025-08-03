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

#include "scripting/flash/events/StatusEvent.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

Event* StatusEvent::cloneImpl() const
{
	StatusEvent *clone = Class<StatusEvent>::getInstanceS(getInstanceWorker());
	clone->code = code;
	clone->level = level;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

StatusEvent::StatusEvent(ASWorker* wrk, Class_base* c, const tiny_string& _level, asAtom _code):Event(wrk,c, "status"),
	code(_code),level(_level)
{
	subtype=SUBTYPE_STATUSEVENT;
}

StatusEvent::~StatusEvent()
{
	ASATOM_DECREF(code);
}

void StatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("STATUS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"status"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, code, ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, level, ASString);
}

ASFUNCTIONBODY_GETTER_SETTER(StatusEvent, code)
ASFUNCTIONBODY_GETTER_SETTER(StatusEvent, level)

ASFUNCTIONBODY_ATOM(StatusEvent,_toString)
{
	StatusEvent* th=asAtomHandler::as<StatusEvent>(obj);
	tiny_string res = "[StatusEvent type=\"";
	res += th->type;
	res += "\" bubbles=";
	res += th->bubbles ? "true" : "false";
	res += " cancelable=";
	res += th->cancelable ? "true" : "false";
	res += " eventPhase=";
	res += Number::toString(th->eventPhase);
	res += " code=";
	if (asAtomHandler::isNull(th->code))
		res += "null";
	else
	{
		res += "\"";
		res += asAtomHandler::toString(th->code,wrk);
		res += "\"";
	}
	res += " level=\"";
	res += th->level;
	res += "\"";
	res += "]";
	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
}
