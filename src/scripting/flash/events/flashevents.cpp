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

#include "scripting/abc.h"
#include "scripting/flash/events/flashevents.h"
#include "swf.h"
#include "compat.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <algorithm>
#include "scripting/flash/ui/gameinput.h"

using namespace std;
using namespace lightspark;

void IEventDispatcher::linkTraits(Class_base* c)
{
	lookupAndLink(c,"addEventListener","flash.events:IEventDispatcher");
	lookupAndLink(c,"removeEventListener","flash.events:IEventDispatcher");
	lookupAndLink(c,"dispatchEvent","flash.events:IEventDispatcher");
	lookupAndLink(c,"hasEventListener","flash.events:IEventDispatcher");
}

Event::Event(Class_base* cb, const tiny_string& t, bool b, bool c, CLASS_SUBTYPE st):
	ASObject(cb,T_OBJECT,st),bubbles(b),cancelable(c),defaultPrevented(false),queued(false),eventPhase(0),type(t),target(asAtomHandler::invalidAtom),currentTarget()
{
}

void Event::finalize()
{
	ASObject::finalize();
	currentTarget.reset();
	target = asAtomHandler::invalidAtom;
}

void Event::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("ACTIVATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"activate"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ADDED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"added"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ADDED_TO_STAGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"addedToStage"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CANCEL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"cancel"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"change"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CLEAR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"clear"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CLOSE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"close"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CLOSING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"closing"),DECLARED_TRAIT);
	c->setVariableAtomByQName("COMPLETE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"complete"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CONNECT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"connect"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CONTEXT3D_CREATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"context3DCreate"),DECLARED_TRAIT);
	c->setVariableAtomByQName("COPY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"copy"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CUT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"cut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DEACTIVATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"deactivate"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DISPLAYING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"displaying"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ENTER_FRAME",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"enterFrame"),DECLARED_TRAIT);
	c->setVariableAtomByQName("EXIT_FRAME",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"exitFrame"),DECLARED_TRAIT);
	c->setVariableAtomByQName("EXITING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"exiting"),DECLARED_TRAIT);
	c->setVariableAtomByQName("FRAME_CONSTRUCTED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"frameConstructed"),DECLARED_TRAIT);
	c->setVariableAtomByQName("FULLSCREEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fullScreen"),DECLARED_TRAIT);
	c->setVariableAtomByQName("HTML_BOUNDS_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"htmlBoundsChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("HTML_DOM_INITIALIZE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"htmlDOMInitialize"),DECLARED_TRAIT);
	c->setVariableAtomByQName("HTML_RENDER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"htmlRender"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ID3",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"id3"),DECLARED_TRAIT);
	c->setVariableAtomByQName("INIT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"init"),DECLARED_TRAIT);
	c->setVariableAtomByQName("LOCATION_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"locationChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_LEAVE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mouseLeave"),DECLARED_TRAIT);
	c->setVariableAtomByQName("NETWORK_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"networkChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("OPEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"open"),DECLARED_TRAIT);
	c->setVariableAtomByQName("PASTE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"paste"),DECLARED_TRAIT);
	c->setVariableAtomByQName("REMOVED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"removed"),DECLARED_TRAIT);
	c->setVariableAtomByQName("REMOVED_FROM_STAGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"removedFromStage"),DECLARED_TRAIT);
	c->setVariableAtomByQName("RENDER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"render"),DECLARED_TRAIT);
	c->setVariableAtomByQName("RESIZE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"resize"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SCROLL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"scroll"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SELECT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"select"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SELECT_ALL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"selectAll"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SOUND_COMPLETE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"soundComplete"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_ERROR_CLOSE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardErrorClose"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_INPUT_CLOSE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardInputClose"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_OUTPUT_CLOSE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardOutputClose"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TAB_CHILDREN_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"tabChildrenChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TAB_ENABLED_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"tabEnabledChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TAB_INDEX_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"tabIndexChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TEXT_INTERACTION_MODE_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"textInteractionModeChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TEXTURE_READY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"textureReady"),DECLARED_TRAIT);
	c->setVariableAtomByQName("UNLOAD",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"unload"),DECLARED_TRAIT);
	c->setVariableAtomByQName("USER_IDLE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"userIdle"),DECLARED_TRAIT);
	c->setVariableAtomByQName("USER_PRESENT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"userPresent"),DECLARED_TRAIT);

	c->setDeclaredMethodByQName("formatToString","",Class<IFunction>::getFunction(c->getSystemState(),formatToString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isDefaultPrevented","",Class<IFunction>::getFunction(c->getSystemState(),_isDefaultPrevented),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("preventDefault","",Class<IFunction>::getFunction(c->getSystemState(),_preventDefault),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(c->getSystemState(),clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopPropagation","",Class<IFunction>::getFunction(c->getSystemState(),stopPropagation),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopImmediatePropagation","",Class<IFunction>::getFunction(c->getSystemState(),stopImmediatePropagation),NORMAL_METHOD,true);
	REGISTER_GETTER(c,currentTarget);
	REGISTER_GETTER(c,target);
	REGISTER_GETTER(c,type);
	REGISTER_GETTER(c,eventPhase);
	REGISTER_GETTER(c,bubbles);
	REGISTER_GETTER(c,cancelable);
}

void Event::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(Event,_constructor)
{
	// Event constructor is called with zero arguments internally
	if(argslen==0)
		return;

	Event* th=asAtomHandler::as<Event>(obj);
	ARG_UNPACK_ATOM(th->type)(th->bubbles, false)(th->cancelable, false);
}

ASFUNCTIONBODY_GETTER(Event,currentTarget)
ASFUNCTIONBODY_GETTER(Event,target)
ASFUNCTIONBODY_GETTER(Event,type)
ASFUNCTIONBODY_GETTER(Event,eventPhase)
ASFUNCTIONBODY_GETTER(Event,bubbles)
ASFUNCTIONBODY_GETTER(Event,cancelable)

ASFUNCTIONBODY_ATOM(Event,_isDefaultPrevented)
{
	Event* th=asAtomHandler::as<Event>(obj);
	asAtomHandler::setBool(ret,th->defaultPrevented);
}

ASFUNCTIONBODY_ATOM(Event,_preventDefault)
{
	Event* th=asAtomHandler::as<Event>(obj);
	th->defaultPrevented = true;
}

ASFUNCTIONBODY_ATOM(Event,formatToString)
{
	assert_and_throw(argslen>=1);
	Event* th=asAtomHandler::as<Event>(obj);
	tiny_string msg = "[";
	msg += asAtomHandler::toString(args[0],sys);

	for(unsigned int i=1; i<argslen; i++)
	{
		tiny_string prop(asAtomHandler::toString(args[i],sys));
		msg += " ";
		msg += prop;
		msg += "=";

		multiname propName(NULL);
		propName.name_type=multiname::NAME_STRING;
		propName.name_s_id=sys->getUniqueStringId(prop);
		propName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		asAtom value=asAtomHandler::invalidAtom;
		th->getVariableByMultiname(value,propName);
		if (asAtomHandler::isValid(value))
			msg += asAtomHandler::toString(value,sys);
	}
	msg += "]";

	ret = asAtomHandler::fromObject(abstract_s(sys,msg));
}

Event* Event::cloneImpl() const
{
	return Class<Event>::getInstanceS(getSystemState(),type, bubbles, cancelable);
}

ASFUNCTIONBODY_ATOM(Event,clone)
{
	Event* th=asAtomHandler::as<Event>(obj);
	ret = asAtomHandler::fromObject(th->cloneImpl());
}

ASFUNCTIONBODY_ATOM(Event,stopPropagation)
{
	Event* th=asAtomHandler::as<Event>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Event.stopPropagation not implemented:"<<th->toDebugString());
}
ASFUNCTIONBODY_ATOM(Event,stopImmediatePropagation)
{
	Event* th=asAtomHandler::as<Event>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Event.stopImmediatePropagation not implemented:"<<th->toDebugString());
}

void WaitableEvent::wait()
{
	while (!handled)
		getSys()->waitMainSignal();
}

void WaitableEvent::signal()
{
	handled = true;
	getSys()->sendMainSignal();
}

void WaitableEvent::finalize()
{
	handled = false;
	Event::finalize();
}

void EventPhase::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("CAPTURING_PHASE",nsNameAndKind(),asAtomHandler::fromUInt(CAPTURING_PHASE),DECLARED_TRAIT);
	c->setVariableAtomByQName("BUBBLING_PHASE",nsNameAndKind(),asAtomHandler::fromUInt(BUBBLING_PHASE),DECLARED_TRAIT);
	c->setVariableAtomByQName("AT_TARGET",nsNameAndKind(),asAtomHandler::fromUInt(AT_TARGET),DECLARED_TRAIT);
}

FocusEvent::FocusEvent(Class_base* c):Event(c, "focusEvent")
{
}

void FocusEvent::sinit(Class_base* c)
{	
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("FOCUS_IN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"focusIn"),DECLARED_TRAIT);
	c->setVariableAtomByQName("FOCUS_OUT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"focusOut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_FOCUS_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mouseFocusChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("KEY_FOCUS_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"keyFocusChange"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(FocusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
}

MouseEvent::MouseEvent(Class_base* c)
 : Event(c, "mouseEvent",false,false,SUBTYPE_MOUSE_EVENT), modifiers(KMOD_NONE),buttonDown(false), delta(1), localX(0), localY(0), stageX(0), stageY(0), relatedObject(NullRef)
{
}

MouseEvent::MouseEvent(Class_base* c, const tiny_string& t, number_t lx, number_t ly,
		       bool b, SDL_Keymod _modifiers, bool _buttonDown, _NR<InteractiveObject> relObj, int32_t _delta)
  : Event(c,t,b,false,SUBTYPE_MOUSE_EVENT), modifiers(_modifiers), buttonDown(_buttonDown),delta(_delta), localX(lx), localY(ly), stageX(0), stageY(0), relatedObject(relObj)
{
}

Event* MouseEvent::cloneImpl() const
{
	return Class<MouseEvent>::getInstanceS(getSystemState(),type,localX,localY,bubbles,(SDL_Keymod)modifiers,buttonDown,relatedObject,delta);
}

ProgressEvent::ProgressEvent(Class_base* c):Event(c, "progress",false,false,SUBTYPE_PROGRESSEVENT),bytesLoaded(0),bytesTotal(0)
{
}

ProgressEvent::ProgressEvent(Class_base* c, uint32_t loaded, uint32_t total, const tiny_string &t):Event(c, t,false,false,SUBTYPE_PROGRESSEVENT),bytesLoaded(loaded),bytesTotal(total)
{
}

Event* ProgressEvent::cloneImpl() const
{
	return Class<ProgressEvent>::getInstanceS(getSystemState(),bytesLoaded, bytesTotal);
}

void ProgressEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("PROGRESS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"progress"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SOCKET_DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"socketData"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_ERROR_DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardErrorData"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_INPUT_PROGRESS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardInputProgress"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_OUTPUT_DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardOutputData"),DECLARED_TRAIT);
	REGISTER_GETTER_SETTER(c,bytesLoaded);
	REGISTER_GETTER_SETTER(c,bytesTotal);
}

ASFUNCTIONBODY_GETTER_SETTER(ProgressEvent,bytesLoaded);
ASFUNCTIONBODY_GETTER_SETTER(ProgressEvent,bytesTotal);

void ProgressEvent::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(ProgressEvent,_constructor)
{
	ProgressEvent* th=asAtomHandler::as<ProgressEvent>(obj);
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
	if(argslen>=4)
		th->bytesLoaded=asAtomHandler::toInt(args[3]);
	if(argslen>=5)
		th->bytesTotal=asAtomHandler::toInt(args[4]);
}

void TimerEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("TIMER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"timer"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TIMER_COMPLETE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"timerComplete"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("updateAfterEvent","",Class<IFunction>::getFunction(c->getSystemState(),updateAfterEvent),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(TimerEvent,updateAfterEvent)
{
	LOG(LOG_NOT_IMPLEMENTED,"TimerEvent::updateAfterEvent not implemented");
}

void MouseEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("CLICK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"click"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DOUBLE_CLICK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"doubleClick"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_DOWN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mouseDown"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_OUT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mouseOut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_OVER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mouseOver"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_UP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mouseUp"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_WHEEL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mouseWheel"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_MOVE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"mouseMove"),DECLARED_TRAIT);
	c->setVariableAtomByQName("RIGHT_CLICK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rightClick"),DECLARED_TRAIT);
	c->setVariableAtomByQName("RIGHT_MOUSE_DOWN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rightMouseDown"),DECLARED_TRAIT);
	c->setVariableAtomByQName("RIGHT_MOUSE_UP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rightMouseUp"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ROLL_OVER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rollOver"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ROLL_OUT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"rollOut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MIDDLE_CLICK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"middleClick"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MIDDLE_MOUSE_DOWN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"middleMouseDown"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MIDDLE_MOUSE_UP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"middleMouseUp"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CONTEXT_MENU",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"contextMenu"),DECLARED_TRAIT);
	c->setVariableAtomByQName("RELEASE_OUTSIDE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"releaseOutside"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("updateAfterEvent","",Class<IFunction>::getFunction(c->getSystemState(),updateAfterEvent),NORMAL_METHOD,true);

	REGISTER_GETTER_SETTER(c,relatedObject);
	REGISTER_GETTER(c,stageX);
	REGISTER_GETTER(c,stageY);
	REGISTER_GETTER_SETTER(c,localX);
	REGISTER_GETTER_SETTER(c,localY);
	REGISTER_GETTER_SETTER(c,altKey);
	REGISTER_GETTER_SETTER(c,buttonDown);
	REGISTER_GETTER_SETTER(c,ctrlKey);
	REGISTER_GETTER_SETTER(c,shiftKey);
	REGISTER_GETTER_SETTER(c,delta);
}

ASFUNCTIONBODY_ATOM(MouseEvent,_constructor)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
	if(argslen>=4)
		th->localX=asAtomHandler::toNumber(args[3]);
	if(argslen>=5)
		th->localY=asAtomHandler::toNumber(args[4]);
	if(argslen>=6)
		th->relatedObject=ArgumentConversionAtom< _NR<InteractiveObject> >::toConcrete(sys,args[5],NullRef);
	if(argslen>=7)
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[6],false))
			th->modifiers |= KMOD_CTRL;
	if(argslen>=8)
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[7],false))
			th->modifiers |= KMOD_ALT;
	if(argslen>=9)
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[8],false))
			th->modifiers |= KMOD_SHIFT;
	if(argslen>=10)
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[9],false))
			th->buttonDown = true;
	if(argslen>=11)
		th->delta=asAtomHandler::toInt(args[10]);
	if(argslen>=12)
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[11],false))
			th->modifiers |= KMOD_GUI;
	if(argslen>=13)
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[12],false))
			th->modifiers |= KMOD_CTRL;
	// TODO: args[13] = clickCount
	if(argslen>=14)
		LOG(LOG_NOT_IMPLEMENTED,"MouseEvent: clickcount");
}

ASFUNCTIONBODY_GETTER_SETTER(MouseEvent,relatedObject);
ASFUNCTIONBODY_GETTER(MouseEvent,localX);
ASFUNCTIONBODY_GETTER(MouseEvent,localY);
ASFUNCTIONBODY_GETTER(MouseEvent,stageX);
ASFUNCTIONBODY_GETTER(MouseEvent,stageY);
ASFUNCTIONBODY_GETTER_SETTER(MouseEvent,delta);
ASFUNCTIONBODY_GETTER_SETTER(MouseEvent,buttonDown);

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_localX)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	if(argslen != 1) 
		throw Class<ArgumentError>::getInstanceS(sys,"Wrong number of arguments in setter"); 
	number_t val=asAtomHandler::toNumber(args[0]);
	th->localX = val;
	//Change StageXY if target!=NULL else don't do anything
	//At this point, the target should be an InteractiveObject but check anyway
	if(asAtomHandler::isValid(th->target) &&(asAtomHandler::toObject(th->target,sys)->getClass()->isSubClass(Class<InteractiveObject>::getClass(sys))))
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>(asAtomHandler::getObject(th->target));
		tar->localToGlobal(th->localX, th->localY, th->stageX, th->stageY);
	}
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_localY)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	if(argslen != 1) 
		throw Class<ArgumentError>::getInstanceS(sys,"Wrong number of arguments in setter"); 
	number_t val=asAtomHandler::toNumber(args[0]);
	th->localY = val;
	//Change StageXY if target!=NULL else don't do anything	
	//At this point, the target should be an InteractiveObject but check anyway
	if(asAtomHandler::isValid(th->target) &&(asAtomHandler::toObject(th->target,sys)->getClass()->isSubClass(Class<InteractiveObject>::getClass(sys))))
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>(asAtomHandler::getObject(th->target));
		tar->localToGlobal(th->localX, th->localY, th->stageX, th->stageY);
	}
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_altKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	asAtomHandler::setBool(ret,(bool)(th->modifiers & KMOD_ALT));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_altKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	th->modifiers |= KMOD_ALT;
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_controlKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	asAtomHandler::setBool(ret,(bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_controlKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	th->modifiers |= KMOD_CTRL;
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_ctrlKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	asAtomHandler::setBool(ret,(bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_ctrlKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	th->modifiers |= KMOD_CTRL;
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_shiftKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	asAtomHandler::setBool(ret,(bool)(th->modifiers & KMOD_SHIFT));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_shiftKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	th->modifiers |= KMOD_SHIFT;
}
ASFUNCTIONBODY_ATOM(MouseEvent,updateAfterEvent)
{
	LOG(LOG_NOT_IMPLEMENTED,"MouseEvent::updateAfterEvent not implemented");
}

void MouseEvent::buildTraits(ASObject* o)
{
	//TODO: really handle local[XY]
	//o->setVariableByQName("localX","",abstract_d(0),DECLARED_TRAIT);
	//o->setVariableByQName("localY","",abstract_d(0),DECLARED_TRAIT);
}

void MouseEvent::setTarget(asAtom t)
{
	target = t;
	//If t is NULL, it means MouseEvent is being reset
	if(asAtomHandler::isInvalid(t))
	{
		localX = 0;
		localY = 0;
		stageX = 0;
		stageY = 0;
		relatedObject = NullRef;
	}
	//If t is non null, it should be an InteractiveObject
	else if(asAtomHandler::toObject(t,getSystemState())->getClass()->isSubClass(Class<InteractiveObject>::getClass(getSystemState())))	
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>(asAtomHandler::getObject(t));
		tar->localToGlobal(localX, localY, stageX, stageY);
	}
}

MouseEvent *MouseEvent::getclone() const
{
	return (MouseEvent*)cloneImpl();
}

NativeDragEvent::NativeDragEvent(Class_base* c)
 : MouseEvent(c)
{
}

void NativeDragEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, MouseEvent, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(NativeDragEvent,_constructor)
{
	//NativeDragEvent* th=obj.as<NativeDragEvent>();
	uint32_t baseClassArgs=imin(argslen,6);
	MouseEvent::_constructor(ret,sys,obj,args,baseClassArgs);
	LOG(LOG_NOT_IMPLEMENTED,"NativeDragEvent: constructor");
}

IOErrorEvent::IOErrorEvent(Class_base* c,const tiny_string& t, const std::string& e, int id) : ErrorEvent(c, t,e,id)
{
}

Event *IOErrorEvent::cloneImpl() const
{
	return Class<IOErrorEvent>::getInstanceS(getSystemState(),text, errorMsg,errorID);
}


void IOErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("IO_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"ioError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DISK_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"diskError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NETWORK_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"networkError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VERIFY_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"verifyError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD_ERROR_IO_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardErrorIoError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD_INPUT_IO_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardInputIoError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD_OUTPUT_IO_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardOutputIoError"),CONSTANT_TRAIT);
}

EventDispatcher::EventDispatcher(Class_base* c):ASObject(c),forcedTarget(asAtomHandler::invalidAtom)
{
}

void EventDispatcher::finalize()
{
	ASObject::finalize();
	handlers.clear();
}
bool EventDispatcher::destruct()
{
	forcedTarget = asAtomHandler::invalidAtom;
	return ASObject::destruct();
}

void EventDispatcher::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->addImplementedInterface(InterfaceClass<IEventDispatcher>::getClass(c->getSystemState()));

	c->setDeclaredMethodByQName("addEventListener","",Class<IFunction>::getFunction(c->getSystemState(),addEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasEventListener","",Class<IFunction>::getFunction(c->getSystemState(),_hasEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeEventListener","",Class<IFunction>::getFunction(c->getSystemState(),removeEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dispatchEvent","",Class<IFunction>::getFunction(c->getSystemState(),dispatchEvent),NORMAL_METHOD,true);

	IEventDispatcher::linkTraits(c);
}

void EventDispatcher::buildTraits(ASObject* o)
{
}

void EventDispatcher::dumpHandlers()
{
	std::map<tiny_string,list<listener> >::iterator it=handlers.begin();
	for(;it!=handlers.end();++it)
	{
		for (auto it2 = it->second.begin();it2 != it->second.end(); it2++)
			LOG(LOG_INFO, it->first<<":"<<asAtomHandler::toDebugString(it2->f));
	}
}

ASFUNCTIONBODY_ATOM(EventDispatcher,addEventListener)
{
	EventDispatcher* th=asAtomHandler::as<EventDispatcher>(obj);
	if(!asAtomHandler::isString(args[0]) || !asAtomHandler::isFunction(args[1]))
		//throw RunTimeException("Type mismatch in EventDispatcher::addEventListener");
		return;

	bool useCapture=false;
	int32_t priority=0;

	if(argslen>=3)
		useCapture=asAtomHandler::Boolean_concrete(args[2]);
	if(argslen>=4)
		priority=asAtomHandler::toInt(args[3]);

	const tiny_string& eventName=asAtomHandler::toString(args[0],sys);

	if(th->is<DisplayObject>() && (eventName=="enterFrame"
				|| eventName=="exitFrame"
				|| eventName=="frameConstructed") )
	{
		th->incRef();
		th->getSystemState()->registerFrameListener(_MR(th->as<DisplayObject>()));
	}

	{
		Locker l(th->handlersMutex);
		//Search if any listener is already registered for the event
		list<listener>& listeners=th->handlers[eventName];
		ASATOM_INCREF(args[1]);
		const listener newListener(args[1], priority, useCapture);
		//Ordered insertion
		list<listener>::iterator insertionPoint=upper_bound(listeners.begin(),listeners.end(),newListener);
		listeners.insert(insertionPoint,newListener);
	}
	th->eventListenerAdded(eventName);
}

ASFUNCTIONBODY_ATOM(EventDispatcher,_hasEventListener)
{
	EventDispatcher* th=asAtomHandler::as<EventDispatcher>(obj);
	const tiny_string& eventName=asAtomHandler::toString(args[0],sys);
	asAtomHandler::setBool(ret,th->hasEventListener(eventName));
}

ASFUNCTIONBODY_ATOM(EventDispatcher,removeEventListener)
{
	EventDispatcher* th=asAtomHandler::as<EventDispatcher>(obj);
	
	if (asAtomHandler::isNull(args[1])) // it seems that null is allowed as function
		return;
	if(!asAtomHandler::isString(args[0]) || !asAtomHandler::isFunction(args[1]))
		throw RunTimeException("Type mismatch in EventDispatcher::removeEventListener");

	const tiny_string& eventName=asAtomHandler::toString(args[0],sys);

	bool useCapture=false;
	if(argslen>=3)
		useCapture=asAtomHandler::Boolean_concrete(args[2]);

	{
		Locker l(th->handlersMutex);
		map<tiny_string, list<listener> >::iterator h=th->handlers.find(eventName);
		if(h==th->handlers.end())
		{
			LOG(LOG_CALLS,_("Event not found"));
			return;
		}

		std::list<listener>::iterator it=find(h->second.begin(),h->second.end(),
											make_pair(args[1],useCapture));
		if(it!=h->second.end())
		{
			ASATOM_DECREF(it->f);
			h->second.erase(it);
		}
		if(h->second.empty()) //Remove the entry from the map
			th->handlers.erase(h);
	}

	// Only unregister the enterFrame listener _after_ the handlers have been erased.
	if(th->is<DisplayObject>() && (eventName=="enterFrame"
					|| eventName=="exitFrame"
					|| eventName=="frameConstructed")
				&& (!th->hasEventListener("enterFrame")
					&& !th->hasEventListener("exitFrame")
					&& !th->hasEventListener("frameConstructed")) )
	{
		th->incRef();
		th->getSystemState()->unregisterFrameListener(_MR(th->as<DisplayObject>()));
	}
}

ASFUNCTIONBODY_ATOM(EventDispatcher,dispatchEvent)
{
	EventDispatcher* th=asAtomHandler::as<EventDispatcher>(obj);
	if(!asAtomHandler::is<Event>(args[0]))
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	ASATOM_INCREF(args[0]);
	_R<Event> e=_MR(Class<Event>::cast(asAtomHandler::getObject(args[0])));

	// Must call the AS getter, because the getter may have been
	// overridden
	asAtom target=asAtomHandler::invalidAtom;
	e->getVariableByMultiname(target,"target", {""});
	if(asAtomHandler::isValid(target) && !asAtomHandler::isNull(target) && !asAtomHandler::isUndefined(target))
	{
		//Object must be cloned, cloning is implemented with the clone AS method
		asAtom cloned=asAtomHandler::invalidAtom;
		e->executeASMethod(cloned,"clone", {""}, NULL, 0);
		//Clone always exists since it's implemented in Event itself
		if(!asAtomHandler::getObject(cloned) || !asAtomHandler::getObject(cloned)->is<Event>())
		{
			asAtomHandler::setBool(ret,false);
			return;
		}

		ASATOM_INCREF(cloned);
		e = _MR(asAtomHandler::getObject(cloned)->as<Event>());
	}
	if(asAtomHandler::isValid(th->forcedTarget))
		e->setTarget(th->forcedTarget);
	ABCVm::publicHandleEvent(th, e);
	asAtomHandler::setBool(ret,true);
}

ASFUNCTIONBODY_ATOM(EventDispatcher,_constructor)
{
	EventDispatcher* th=asAtomHandler::as<EventDispatcher>(obj);
	asAtom forcedTarget=asAtomHandler::invalidAtom;
	ARG_UNPACK_ATOM(forcedTarget, asAtomHandler::nullAtom);
	if(asAtomHandler::isValid(forcedTarget))
	{
		if(asAtomHandler::isNull(forcedTarget) || asAtomHandler::isUndefined(forcedTarget))
			forcedTarget=asAtomHandler::invalidAtom;
		else if(!asAtomHandler::toObject(forcedTarget,sys)->getClass()->isSubClass(InterfaceClass<IEventDispatcher>::getClass(sys)))
			throw Class<ArgumentError>::getInstanceS(sys,"Wrong argument for EventDispatcher");
	}
	th->forcedTarget=forcedTarget;
}

void EventDispatcher::handleEvent(_R<Event> e)
{
	check();
	e->check();
	Locker l(handlersMutex);
	map<tiny_string, list<listener> >::iterator h=handlers.find(e->type);
	if(h==handlers.end())
		return;

	LOG(LOG_CALLS, _("Handling event ") << h->first);

	//Create a temporary copy of the listeners, as the list can be modified during the calls
	vector<listener> tmpListener(h->second.begin(),h->second.end());
	l.release();
	// listeners may be removed during the call to a listener, so we have to incref them before the call
	// TODO how to handle listeners that are removed during the call to a listener, should they really be executed anyway?
	for(unsigned int i=0;i<tmpListener.size();i++)
	{
		//tmpListener is now also owned by the vector
		ASATOM_INCREF(tmpListener[i].f);
	}
	for(unsigned int i=0;i<tmpListener.size();i++)
	{
		if( (e->eventPhase == EventPhase::BUBBLING_PHASE && tmpListener[i].use_capture)
		||  (e->eventPhase == EventPhase::CAPTURING_PHASE && !tmpListener[i].use_capture))
			continue;
		asAtom arg0= asAtomHandler::fromObject(e.getPtr());
		IFunction* func = asAtomHandler::as<IFunction>(tmpListener[i].f);
		asAtom v = asAtomHandler::fromObject(func->closure_this ? func->closure_this.getPtr() : this);
		asAtom ret=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(tmpListener[i].f,ret,v,&arg0,1,false);
		ASATOM_DECREF(ret);
		//And now no more, f can also be deleted
		ASATOM_DECREF(tmpListener[i].f);
	}
	e->check();
}

bool EventDispatcher::hasEventListener(const tiny_string& eventName)
{
	Locker l(handlersMutex);
	if(handlers.find(eventName)==handlers.end())
		return false;
	else
		return true;
}

NetStatusEvent::NetStatusEvent(Class_base* c, const tiny_string& level, const tiny_string& code):Event(c, "netStatus")
{
	ASObject* info=Class<ASObject>::getInstanceS(c->getSystemState());
	info->setVariableAtomByQName("level",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),level),DECLARED_TRAIT);
	info->setVariableAtomByQName("code",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),code),DECLARED_TRAIT);
	setVariableByQName("info","",info, DECLARED_TRAIT);
}

void NetStatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("NET_STATUS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"netStatus"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(NetStatusEvent,_constructor)
{
	//Also call the base class constructor, using only the first arguments
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
	asAtom info=asAtomHandler::invalidAtom;
	if(argslen==0)
	{
		//Called from C++ code, info was set in the C++
		//constructor
		return;
	}
	else if(argslen==4)
	{
		//Building from AS code, use the data
		ASATOM_INCREF(args[3]);
		info=args[3];
	}
	else
	{
		//Uninitialized info
		info=asAtomHandler::nullAtom;
	}
	multiname infoName(NULL);
	infoName.name_type=multiname::NAME_STRING;
	infoName.name_s_id=sys->getUniqueStringId("info");
	infoName.ns.push_back(nsNameAndKind());
	infoName.isAttribute = false;
	asAtomHandler::getObject(obj)->setVariableByMultiname(infoName, info, CONST_NOT_ALLOWED);
}

Event* NetStatusEvent::cloneImpl() const
{
	NetStatusEvent *clone=Class<NetStatusEvent>::getInstanceS(getSystemState());
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;

	multiname infoName(NULL);
	infoName.name_type=multiname::NAME_STRING;
	infoName.name_s_id=getSystemState()->getUniqueStringId("info");
	infoName.ns.push_back(nsNameAndKind());
	infoName.isAttribute = false;

	asAtom info=asAtomHandler::invalidAtom;
	const_cast<NetStatusEvent*>(this)->getVariableByMultiname(info,infoName);
	assert(asAtomHandler::isValid(info));
	ASATOM_INCREF(info);
	clone->setVariableByMultiname(infoName, info, CONST_NOT_ALLOWED);

	return clone;
}

FullScreenEvent::FullScreenEvent(Class_base* c):Event(c, "fullScreenEvent")
{
}

void FullScreenEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("FULL_SCREEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fullScreen"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(FullScreenEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
}

KeyboardEvent::KeyboardEvent(Class_base* c, tiny_string _type, uint32_t _charcode, uint32_t _keycode, SDL_Keymod _modifiers, SDL_Keycode _sdlkeycode)
  : Event(c, _type,false,false,SUBTYPE_KEYBOARD_EVENT), modifiers(_modifiers), charCode(_charcode), keyCode(_keycode), keyLocation(0),sdlkeycode(_sdlkeycode)
{
}

void KeyboardEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	REGISTER_GETTER_SETTER(c, altKey);
	REGISTER_GETTER_SETTER(c, charCode);
	REGISTER_GETTER_SETTER(c, commandKey);
	REGISTER_GETTER_SETTER(c, controlKey);
	REGISTER_GETTER_SETTER(c, ctrlKey);
	REGISTER_GETTER_SETTER(c, keyCode);
	REGISTER_GETTER_SETTER(c, keyLocation);
	REGISTER_GETTER_SETTER(c, shiftKey);
	c->setVariableAtomByQName("KEY_DOWN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"keyDown"),DECLARED_TRAIT);
	c->setVariableAtomByQName("KEY_UP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"keyUp"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(KeyboardEvent,_constructor)
{
	KeyboardEvent* th=asAtomHandler::as<KeyboardEvent>(obj);

	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	if(argslen > 3) {
		th->charCode = asAtomHandler::toUInt(args[3]);
	}
	if(argslen > 4) {
		th->keyCode = asAtomHandler::toUInt(args[4]);
	}
	if(argslen > 5) {
		th->keyLocation = asAtomHandler::toUInt(args[5]);
	}
	if(argslen > 6) {
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[6],false))
			th->modifiers |= KMOD_CTRL;
	}
	if(argslen > 7) {
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[7],false))
			th->modifiers |= KMOD_ALT;
	}
	if(argslen > 8) {
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[8],false))
			th->modifiers |= KMOD_SHIFT;
	}
	if(argslen > 9) {
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[9],false))
			th->modifiers |= KMOD_CTRL;
	}
	// args[10] (commandKeyValue) is only supported on Max OSX
	if(argslen > 10) {
		if (ArgumentConversionAtom<bool>::toConcrete(sys,args[10],false))
			LOG(LOG_NOT_IMPLEMENTED,"KeyboardEvent:commandKeyValue");
	}
}

ASFUNCTIONBODY_GETTER_SETTER(KeyboardEvent, charCode);
ASFUNCTIONBODY_GETTER_SETTER(KeyboardEvent, keyCode);
ASFUNCTIONBODY_GETTER_SETTER(KeyboardEvent, keyLocation);

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_altKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	asAtomHandler::setBool(ret,(bool)(th->modifiers & KMOD_ALT));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_altKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	th->modifiers |= KMOD_ALT;
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_commandKey)
{
	// Supported only on OSX
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_commandKey)
{
	// Supported only on OSX
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_controlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	asAtomHandler::setBool(ret,(bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_controlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	th->modifiers |= KMOD_CTRL;
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_ctrlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	asAtomHandler::setBool(ret,(bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_ctrlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	th->modifiers |= KMOD_CTRL;
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_shiftKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	asAtomHandler::setBool(ret,(bool)(th->modifiers & KMOD_SHIFT));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_shiftKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	th->modifiers |= KMOD_SHIFT;
}

Event* KeyboardEvent::cloneImpl() const
{
	KeyboardEvent *cloned = Class<KeyboardEvent>::getInstanceS(getSystemState());
	cloned->type = type;
	cloned->bubbles = bubbles;
	cloned->cancelable = cancelable;
	cloned->modifiers = modifiers;
	cloned->charCode = charCode;
	cloned->keyCode = keyCode;
	cloned->keyLocation = keyLocation;
	cloned->sdlkeycode = sdlkeycode;
	return cloned;
}

TextEvent::TextEvent(Class_base* c,const tiny_string& t):Event(c,t)
{
}

void TextEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("TEXT_INPUT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"textInput"),DECLARED_TRAIT);
	REGISTER_GETTER_SETTER(c,text);
}

ASFUNCTIONBODY_GETTER_SETTER(TextEvent,text);

ASFUNCTIONBODY_ATOM(TextEvent,_constructor)
{
	TextEvent* th=asAtomHandler::as<TextEvent>(obj);
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
	if(argslen>=4)
		th->text=asAtomHandler::toString(args[3],sys);
}

ErrorEvent::ErrorEvent(Class_base* c, const tiny_string& t, const std::string& e, int id): TextEvent(c,t), errorMsg(e),errorID(id)
{
}

void ErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"error"),DECLARED_TRAIT);
	REGISTER_GETTER(c,errorID);
}
ASFUNCTIONBODY_GETTER(ErrorEvent,errorID);

Event* ErrorEvent::cloneImpl() const
{
	return Class<ErrorEvent>::getInstanceS(getSystemState(),text, errorMsg,errorID);
}

ASFUNCTIONBODY_ATOM(ErrorEvent,_constructor)
{
	ErrorEvent* th=asAtomHandler::as<ErrorEvent>(obj);
	uint32_t baseClassArgs=imin(argslen,4);
	TextEvent::_constructor(ret,sys,obj,args,baseClassArgs);
	if(argslen>=5)
		th->errorID=asAtomHandler::toInt(args[4]);
}

SecurityErrorEvent::SecurityErrorEvent(Class_base* c, const std::string& e):ErrorEvent(c, "securityError",e)
{
}

void SecurityErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("SECURITY_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"securityError"),DECLARED_TRAIT);
}

AsyncErrorEvent::AsyncErrorEvent(Class_base* c):ErrorEvent(c, "asyncError")
{
}

void AsyncErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("ASYNC_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"asyncError"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(AsyncErrorEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,4);
	ErrorEvent::_constructor(ret,sys,obj,args,baseClassArgs);
}


UncaughtErrorEvent::UncaughtErrorEvent(Class_base* c):ErrorEvent(c, "uncaughtError")
{
}

void UncaughtErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("UNCAUGHT_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"uncaughtError"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(UncaughtErrorEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,4);
	ErrorEvent::_constructor(ret,sys,obj,args,baseClassArgs);
}

ABCContextInitEvent::ABCContextInitEvent(ABCContext* c, bool l):Event(nullptr, "ABCContextInitEvent"),context(c),lazy(l)
{
}

AVM1InitActionEvent::AVM1InitActionEvent(RootMovieClip* r,  _NR<MovieClip> c):Event(nullptr, "AVM1InitActionEvent"),root(r),clip(c)
{
}
void AVM1InitActionEvent::finalize()
{
	root = nullptr;
	clip.reset();
	Event::finalize();
}
void AVM1InitActionEvent::executeActions()
{
	root->AVM1checkInitActions(clip.getPtr());
}

ShutdownEvent::ShutdownEvent():Event(NULL, "shutdownEvent")
{
}

void HTTPStatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("HTTP_STATUS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"httpStatus"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(HTTPStatusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
}

FunctionEvent::FunctionEvent(asAtom _f, asAtom _obj, asAtom* _args, uint32_t _numArgs):
		WaitableEvent("FunctionEvent"),f(_f),obj(_obj),numArgs(_numArgs)
{
	args = new asAtom[numArgs];
	uint32_t i;
	for(i=0; i<numArgs; i++)
	{
		args[i] = _args[i];
	}
}

FunctionEvent::~FunctionEvent()
{
	//Since the array is used in Function::call the object inside are already been decReffed
	delete[] args;
}

ExternalCallEvent::ExternalCallEvent(asAtom _f, ASObject* const* _args,
	uint32_t _numArgs, ASObject** _result, bool* _thrown, tiny_string* _exception):
		WaitableEvent("ExternalCallEvent"),
		f(_f),args(_args),result(_result),thrown(_thrown),exception(_exception),numArgs(_numArgs)
{
}

ExternalCallEvent::~ExternalCallEvent()
{
}

BindClassEvent::BindClassEvent(_R<RootMovieClip> b, const tiny_string& c)
	: Event(NULL, "bindClass"),base(b),tag(NULL),class_name(c)
{
}

BindClassEvent::BindClassEvent(DictionaryTag* t, const tiny_string& c)
	: Event(NULL, "bindClass"),tag(t),class_name(c)
{
}

ParseRPCMessageEvent::ParseRPCMessageEvent(_R<ByteArray> ba, _NR<ASObject> c, _NR<Responder> r):
	Event(NULL, "ParseRPCMessageEvent"),message(ba),client(c),responder(r)
{
}

void ParseRPCMessageEvent::finalize()
{
	Event::finalize();
	message.reset();
	client.reset();
	responder.reset();
}

void StatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	/* TODO: dispatch this event */
	c->setVariableAtomByQName("STATUS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"status"),DECLARED_TRAIT);
}

void DataEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextEvent, _constructor, CLASS_SEALED);
	/* TODO: dispatch this event */
	c->setVariableAtomByQName("DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"data"),DECLARED_TRAIT);
	/* TODO: dispatch this event */
	c->setVariableAtomByQName("UPLOAD_COMPLETE_DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"uploadCompleteData"),DECLARED_TRAIT);

	REGISTER_GETTER_SETTER(c, data);
}

ASFUNCTIONBODY_GETTER_SETTER(DataEvent, data);

ASFUNCTIONBODY_ATOM(DataEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	TextEvent::_constructor(ret,sys,obj,args,baseClassArgs);

	DataEvent* th=asAtomHandler::as<DataEvent>(obj);
	if (argslen >= 4)
	{
		th->data = asAtomHandler::toString(args[3],sys);
	}
}

Event* DataEvent::cloneImpl() const
{
	DataEvent *clone = Class<DataEvent>::getInstanceS(getSystemState());
	clone->data = data;
	// TextEvent
	clone->text = text;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

void InvokeEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("INVOKE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invoke"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(InvokeEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
}

DRMErrorEvent::DRMErrorEvent(Class_base* c) : ErrorEvent(c, "drmAuthenticate")
{
}

void DRMErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("DRM_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"drmError"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DRM_LOAD_DEVICEID_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"drmLoadDeviceIdError"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(DRMErrorEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	ErrorEvent::_constructor(ret,sys,obj,args,baseClassArgs);
	if(argslen > 3)
		LOG(LOG_NOT_IMPLEMENTED, "DRMErrorEvent constructor doesn't support all parameters");
}

DRMStatusEvent::DRMStatusEvent(Class_base* c) : Event(c, "drmAuthenticate")
{
}

void DRMStatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("DRM_STATUS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"drmStatus"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(DRMStatusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
	if(argslen > 3)
		LOG(LOG_NOT_IMPLEMENTED, "DRMStatusEvent constructor doesn't support all parameters");
}

VideoEvent::VideoEvent(Class_base* c)
  : Event(c, "renderState"),status("unavailable")
{
}

void VideoEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("RENDER_STATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"renderState"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RENDER_STATUS_ACCELERATED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"accelerated"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RENDER_STATUS_SOFTWARE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"software"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RENDER_STATUS_UNAVAILABLE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"unavailable"),CONSTANT_TRAIT);
	REGISTER_GETTER(c,status);
}

ASFUNCTIONBODY_ATOM(VideoEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	VideoEvent* th=asAtomHandler::as<VideoEvent>(obj);
	if(argslen>=4)
	{
		th->status=asAtomHandler::toString(args[3],sys);
	}
}

Event* VideoEvent::cloneImpl() const
{
	VideoEvent *clone;
	clone = Class<VideoEvent>::getInstanceS(getSystemState());
	clone->status = status;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

ASFUNCTIONBODY_GETTER(VideoEvent,status)


StageVideoEvent::StageVideoEvent(Class_base* c)
  : Event(c, "renderState"),status("unavailable")
{
}

void StageVideoEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("RENDER_STATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"renderState"),CONSTANT_TRAIT);
	REGISTER_GETTER(c,colorSpace);
	REGISTER_GETTER(c,status);
}

ASFUNCTIONBODY_ATOM(StageVideoEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	StageVideoEvent* th=asAtomHandler::as<StageVideoEvent>(obj);
	if(argslen>=4)
	{
		th->status=asAtomHandler::toString(args[3],sys);
	}
	if(argslen>=5)
	{
		th->colorSpace=asAtomHandler::toString(args[4],sys);
	}
}

Event* StageVideoEvent::cloneImpl() const
{
	StageVideoEvent *clone;
	clone = Class<StageVideoEvent>::getInstanceS(getSystemState());
	clone->status = status;
	clone->colorSpace = colorSpace;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

ASFUNCTIONBODY_GETTER(StageVideoEvent,colorSpace)
ASFUNCTIONBODY_GETTER(StageVideoEvent,status)

StageVideoAvailabilityEvent::StageVideoAvailabilityEvent(Class_base* c)
  : Event(c, "stageVideoAvailability"), availability("unavailable")
{
}

void StageVideoAvailabilityEvent::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, Event, CLASS_SEALED);
	c->setVariableAtomByQName("STAGE_VIDEO_AVAILABILITY",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"stageVideoAvailability"),DECLARED_TRAIT);
	REGISTER_GETTER(c, availability);
}

ASFUNCTIONBODY_ATOM(StageVideoAvailabilityEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	StageVideoAvailabilityEvent* th=asAtomHandler::as<StageVideoAvailabilityEvent>(obj);
	if(argslen>=4)
	{
		th->availability = asAtomHandler::toString(args[3],sys);
	}
}

Event* StageVideoAvailabilityEvent::cloneImpl() const
{
	StageVideoAvailabilityEvent *clone;
	clone = Class<StageVideoAvailabilityEvent>::getInstanceS(getSystemState());
	clone->availability = availability;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

ASFUNCTIONBODY_GETTER(StageVideoAvailabilityEvent,availability);

void ContextMenuEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("MENU_ITEM_SELECT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"menuItemSelect"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MENU_SELECT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"menuSelect"),DECLARED_TRAIT);
	REGISTER_GETTER_SETTER(c, mouseTarget);
	REGISTER_GETTER_SETTER(c, contextMenuOwner);
}
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuEvent,mouseTarget);
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuEvent,contextMenuOwner);

ASFUNCTIONBODY_ATOM(ContextMenuEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	ContextMenuEvent* th=asAtomHandler::as<ContextMenuEvent>(obj);
	if(argslen>=4)
	{
		if (asAtomHandler::is<InteractiveObject>(args[3]))
			th->mouseTarget = _NR<InteractiveObject>(asAtomHandler::as<InteractiveObject>(args[3]));
	}
	if(argslen>=5)
	{
		if (asAtomHandler::is<InteractiveObject>(args[4]))
			th->contextMenuOwner = _NR<InteractiveObject>(asAtomHandler::as<InteractiveObject>(args[4]));
	}
}

Event* ContextMenuEvent::cloneImpl() const
{
	ContextMenuEvent *clone;
	clone = Class<ContextMenuEvent>::getInstanceS(getSystemState());
	clone->mouseTarget = mouseTarget;
	clone->contextMenuOwner = contextMenuOwner;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

void TouchEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("TOUCH_BEGIN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"touchBegin"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_END",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"touchEnd"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_MOVE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"touchMove"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_OUT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"touchOut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_OVER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"touchOver"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_ROLL_OUT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"touchRollOut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_ROLL_OVER",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"touchRollOver"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_TAP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"touchTap"),DECLARED_TRAIT);
}

void GestureEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("GESTURE_TWO_FINGER_TAP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"gestureTwoFingerTap"),DECLARED_TRAIT);
}

void PressAndTapGestureEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, GestureEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("GESTURE_PRESS_AND_TAP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"gesturePressAndTap"),DECLARED_TRAIT);
}

void TransformGestureEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, GestureEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("GESTURE_PAN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"gesturePan"),DECLARED_TRAIT);
	c->setVariableAtomByQName("GESTURE_ROTATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"gestureRotate"),DECLARED_TRAIT);
	c->setVariableAtomByQName("GESTURE_SWIPE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"gestureSwipe"),DECLARED_TRAIT);
	c->setVariableAtomByQName("GESTURE_ZOOM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"gestureZoom"),DECLARED_TRAIT);
}

UncaughtErrorEvents::UncaughtErrorEvents(Class_base* c):
	EventDispatcher(c)
{
}

void UncaughtErrorEvents::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY_ATOM(UncaughtErrorEvents,_constructor)
{
	//EventDispatcher::_constructor(obj, NULL, 0);
	//UncaughtErrorEvents* th=Class<UncaughtErrorEvents>::cast(obj);
	LOG(LOG_NOT_IMPLEMENTED,"UncaughtErrorEvents is not implemented");
}

void SampleDataEvent::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, Event, CLASS_SEALED);
	c->setVariableAtomByQName("SAMPLE_DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"sampleData"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	
	REGISTER_GETTER_SETTER(c, data);
	REGISTER_GETTER_SETTER(c, position);
}
ASFUNCTIONBODY_GETTER_SETTER(SampleDataEvent,data)
ASFUNCTIONBODY_GETTER_SETTER(SampleDataEvent,position)

ASFUNCTIONBODY_ATOM(SampleDataEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	SampleDataEvent* th=asAtomHandler::as<SampleDataEvent>(obj);
	if(argslen>=4)
	{
		th->position = asAtomHandler::toNumber(args[3]);
		if (argslen>4 && asAtomHandler::is<ByteArray>(args[4]))
			th->data = _MR(asAtomHandler::as<ByteArray>(args[4]));
		else
			th->data.reset();
	}
}
ASFUNCTIONBODY_ATOM(SampleDataEvent,_toString)
{
	SampleDataEvent* th=asAtomHandler::as<SampleDataEvent>(obj);
	tiny_string res = "[SampleDataEvent type=";
	res += th->type;
	res += " bubbles=";
	res += th->bubbles ? "true" : "false";
	res += " cancelable=";
	res += th->cancelable ? "true" : "false";
	res += " theposition=";
	res += Number::toString(th->position);
	res += " thedata=";
	res += th->data.isNull() ? "null" : th->data->toString();
	res += "]";
	ret = asAtomHandler::fromString(sys,res);
}

Event* SampleDataEvent::cloneImpl() const
{
	SampleDataEvent *clone;
	clone = Class<SampleDataEvent>::getInstanceS(getSystemState());
	clone->position = position;
	clone->data = data;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

void ThrottleEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("THROTTLE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"Throttle"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	
	REGISTER_GETTER(c, state);
	REGISTER_GETTER(c, targetFrameRate);
}
ASFUNCTIONBODY_GETTER(ThrottleEvent,state)
ASFUNCTIONBODY_GETTER(ThrottleEvent,targetFrameRate)

ASFUNCTIONBODY_ATOM(ThrottleEvent,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"ThrottleEvent is not dispatched anywhere");
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	ThrottleEvent* th=asAtomHandler::as<ThrottleEvent>(obj);
	if(argslen>=4)
	{
		th->state = asAtomHandler::toString(args[3],sys);
		if (argslen>4)
			th->targetFrameRate = asAtomHandler::toNumber(args[3]);
	}
}
ASFUNCTIONBODY_ATOM(ThrottleEvent,_toString)
{
	ThrottleEvent* th=asAtomHandler::as<ThrottleEvent>(obj);
	tiny_string res = "[ThrottleEvent type=";
	res += th->type;
	res += " bubbles=";
	res += th->bubbles ? "true" : "false";
	res += " cancelable=";
	res += th->cancelable ? "true" : "false";
	res += " state=";
	res += th->state;
	res += " targetFrameRate=";
	res += Number::toString(th->targetFrameRate);
	res += "]";
	ret = asAtomHandler::fromString(sys,res);
}

Event* ThrottleEvent::cloneImpl() const
{
	ThrottleEvent *clone;
	clone = Class<ThrottleEvent>::getInstanceS(getSystemState());
	clone->state = state;
	clone->targetFrameRate = targetFrameRate;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}
void ThrottleType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("PAUSE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"pause"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RESUME",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"resume"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("THROTTLE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"throttle"),CONSTANT_TRAIT);
}

GameInputEvent::GameInputEvent(Class_base *c) : Event(c, "gameinput",false,false,SUBTYPE_GAMEINPUTEVENT)
{
}

GameInputEvent::GameInputEvent(Class_base *c, NullableRef<GameInputDevice> _device) : Event(c, "gameinput",false,false,SUBTYPE_GAMEINPUTEVENT),device(_device)
{
}

void GameInputEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("DEVICE_ADDED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"deviceAdded"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DEVICE_REMOVED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"deviceRemoved"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DEVICE_UNUSABLE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"deviceUnusable"),DECLARED_TRAIT);
	REGISTER_GETTER(c, device);
}
ASFUNCTIONBODY_GETTER(GameInputEvent,device);

ASFUNCTIONBODY_ATOM(GameInputEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	GameInputEvent* th=asAtomHandler::as<GameInputEvent>(obj);
	if(argslen>=4)
	{
		if (asAtomHandler::is<GameInputDevice>(args[3]))
			th->device = _MR(asAtomHandler::as<GameInputDevice>(args[3]));
		else
			th->device.reset();
	}
}
Event* GameInputEvent::cloneImpl() const
{
	GameInputEvent *clone;
	clone = Class<GameInputEvent>::getInstanceS(getSystemState());
	clone->device = device;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

