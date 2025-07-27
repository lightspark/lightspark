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

#include "scripting/flash/events/FocusEvent.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/flash/display/flashdisplay.h"

using namespace std;
using namespace lightspark;

FocusEvent::FocusEvent(ASWorker* wrk, Class_base* c)
	:Event(wrk,c, "focusEvent",true),
	relatedObject(NullRef),
	keyCode(0),
	shiftKey(false),
	direction("none"),
	isRelatedObjectInaccessible(false)
{
	subtype=SUBTYPE_FOCUSEVENT;
}

FocusEvent::FocusEvent(ASWorker* wrk, Class_base* c, tiny_string _type, bool _cancelable, _NR<InteractiveObject> _relatedObject, uint32_t _keyCode, bool _shiftKey)
	:Event(wrk,c, _type,true,_cancelable),
	relatedObject(_relatedObject),
	keyCode(_keyCode),
	shiftKey(_shiftKey),
	direction("none"),
	isRelatedObjectInaccessible(false)
{
	subtype=SUBTYPE_FOCUSEVENT;
}

void FocusEvent::sinit(Class_base* c)
{	
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("FOCUS_IN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"focusIn"),DECLARED_TRAIT);
	c->setVariableAtomByQName("FOCUS_OUT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"focusOut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_FOCUS_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mouseFocusChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("KEY_FOCUS_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"keyFocusChange"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,relatedObject,InteractiveObject);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,keyCode,UInteger);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,shiftKey,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,isRelatedObjectInaccessible,Boolean);
}
ASFUNCTIONBODY_GETTER_SETTER(FocusEvent,relatedObject)
ASFUNCTIONBODY_GETTER_SETTER(FocusEvent,keyCode)
ASFUNCTIONBODY_GETTER_SETTER(FocusEvent,shiftKey)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(FocusEvent,isRelatedObjectInaccessible)

ASFUNCTIONBODY_ATOM(FocusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
	FocusEvent* th=asAtomHandler::as<FocusEvent>(obj);
	ARG_CHECK(ARG_UNPACK(th->relatedObject,NullRef)(th->shiftKey, false)(th->keyCode, 0)(th->direction,"none"));
}

ASFUNCTIONBODY_ATOM(FocusEvent,_toString)
{
	FocusEvent* th=asAtomHandler::as<FocusEvent>(obj);
	tiny_string msg = "[";
	msg += th->getSystemState()->getStringFromUniqueId(th->getClass()->class_name.nameId);
	msg += " type=\"";
	msg += th->type;
	msg += "\" bubbles=";
	msg += th->bubbles ? "true" : "false";
	msg += " cancelable=";
	msg += th->cancelable ? "true" : "false";
	msg += " eventPhase=";
	msg += Number::toString(th->eventPhase);
	msg += " relatedObject=";
	msg += th->relatedObject.isNull() ? "null" : th->relatedObject->toString();
	msg += " shiftKey=";
	msg += th->shiftKey ? "true" : "false";
	msg += " keyCode=";
	msg += Number::toString(th->keyCode);
	msg += "]";

	ret = asAtomHandler::fromObject(abstract_s(wrk,msg));
}
