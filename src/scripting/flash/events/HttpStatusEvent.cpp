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

#include "scripting/flash/events/HttpStatusEvent.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Integer.h"

using namespace std;
using namespace lightspark;


HTTPStatusEvent::HTTPStatusEvent(ASWorker* wrk, Class_base* c, const tiny_string& _type):
	Event(wrk,c,_type),
	redirected(false),
	responseURL(asAtomHandler::nullAtom),
	status(0)
{
	subtype=SUBTYPE_HTTPSTATUSEVENT;
}

void HTTPStatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("HTTP_STATUS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"httpStatus"),DECLARED_TRAIT);

	// Value is undefined and not "httpResponseStatus" like stated in documentation
	c->setVariableAtomByQName("HTTP_RESPONSE_STATUS",nsNameAndKind(),asAtomHandler::undefinedAtom,DECLARED_TRAIT);

	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,redirected,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,responseHeaders,Array);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,responseURL,ASString);
	REGISTER_GETTER_RESULTTYPE(c,status,Integer);
}

ASFUNCTIONBODY_ATOM(HTTPStatusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
	HTTPStatusEvent* th=asAtomHandler::as<HTTPStatusEvent>(obj);
	ARG_CHECK(ARG_UNPACK(th->status,0)(th->redirected, false));
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(HTTPStatusEvent,redirected)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(HTTPStatusEvent,responseHeaders)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(HTTPStatusEvent,responseURL)
ASFUNCTIONBODY_GETTER(HTTPStatusEvent,status)

ASFUNCTIONBODY_ATOM(HTTPStatusEvent,_toString)
{
	HTTPStatusEvent* th=asAtomHandler::as<HTTPStatusEvent>(obj);
	tiny_string msg = "[";
	msg += th->getSystemState()->getStringFromUniqueId(th->getClass()->class_name.nameId);
	msg += " type=\"";
	msg += th->type;
	msg += "\" bubbles=";
	msg += th->bubbles ? "true" : "false";
	msg += " cancelable=";
	msg += th->cancelable ? "true" : "false";
	msg += " eventPhase=";
	msg += Integer::toString(th->eventPhase);
	msg += " status=";
	msg += Integer::toString(th->status);
	msg += " redirected=";
	msg += th->redirected ? "true" : "false";
	msg += " responseURL=";
	msg += asAtomHandler::toString(th->responseURL,wrk);
	msg += "]";

	ret = asAtomHandler::fromObject(abstract_s(wrk,msg));
}
