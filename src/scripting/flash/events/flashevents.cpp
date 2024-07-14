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
#include "scripting/toplevel/IFunction.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Undefined.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/display/RootMovieClip.h"

using namespace std;
using namespace lightspark;

bool listener::operator==(const listener &r)
{
	/* One can register the same handle for the same event with
		 * different values of use_capture
		 */
	if ((use_capture != r.use_capture) || (worker != r.worker))
		return false;
	if (asAtomHandler::getObjectNoCheck(f)->as<IFunction>()->closure_this
			&& asAtomHandler::getObjectNoCheck(r.f)->as<IFunction>()->closure_this
			&& asAtomHandler::getObjectNoCheck(f)->as<IFunction>()->closure_this != asAtomHandler::getObjectNoCheck(r.f)->as<IFunction>()->closure_this)
		return false;
	return asAtomHandler::getObjectNoCheck(f)->isEqual(asAtomHandler::getObjectNoCheck(r.f));
}

void listener::resetClosure()
{
	if (asAtomHandler::isFunction(f) && asAtomHandler::as<IFunction>(f)->closure_this)
	{
		ASObject* o = asAtomHandler::as<IFunction>(f)->closure_this;
		asAtomHandler::as<IFunction>(f)->closure_this=nullptr;
		o->removeStoredMember();
	}
}

void IEventDispatcher::linkTraits(Class_base* c)
{
	lookupAndLink(c,STRING_ADDEVENTLISTENER,STRING_FLASH_EVENTS_IEVENTDISPATCHER);
	lookupAndLink(c,STRING_REMOVEEVENTLISTENER,STRING_FLASH_EVENTS_IEVENTDISPATCHER);
	lookupAndLink(c,STRING_DISPATCHEVENT,STRING_FLASH_EVENTS_IEVENTDISPATCHER);
	lookupAndLink(c,STRING_HASEVENTLISTENER,STRING_FLASH_EVENTS_IEVENTDISPATCHER);
}

Event::Event(ASWorker* wrk, Class_base* cb, const tiny_string& t, bool b, bool c, CLASS_SUBTYPE st):
	ASObject(wrk,cb,T_OBJECT,st),bubbles(b),cancelable(c),defaultPrevented(false),propagationStopped(false),immediatePropagationStopped(false),queued(false),
	eventPhase(0),type(t),target(asAtomHandler::invalidAtom),currentTarget()
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

	c->setVariableAtomByQName("BROWSER_ZOOM_CHANGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"browserZoomChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CHANNEL_MESSAGE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"channelMessage"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CHANNEL_STATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"channelState"),DECLARED_TRAIT);
	c->setVariableAtomByQName("FRAME_LABEL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"frameLabel"),DECLARED_TRAIT);
	c->setVariableAtomByQName("VIDEO_FRAME",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"videoFrame"),DECLARED_TRAIT);
	c->setVariableAtomByQName("WORKER_STATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"workerState"),DECLARED_TRAIT);

	c->setDeclaredMethodByQName("formatToString","",c->getSystemState()->getBuiltinFunction(formatToString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isDefaultPrevented","",c->getSystemState()->getBuiltinFunction(_isDefaultPrevented),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("preventDefault","",c->getSystemState()->getBuiltinFunction(_preventDefault),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopPropagation","",c->getSystemState()->getBuiltinFunction(stopPropagation),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stopImmediatePropagation","",c->getSystemState()->getBuiltinFunction(stopImmediatePropagation),NORMAL_METHOD,true);
	REGISTER_GETTER_RESULTTYPE(c,currentTarget,ASObject);
	REGISTER_GETTER_RESULTTYPE(c,target,ASObject);
	REGISTER_GETTER_RESULTTYPE(c,type,ASString);
	REGISTER_GETTER_RESULTTYPE(c,eventPhase,UInteger);
	REGISTER_GETTER_RESULTTYPE(c,bubbles,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,cancelable,Boolean);
}

ASFUNCTIONBODY_ATOM(Event,_constructor)
{
	// Event constructor is called with zero arguments internally
	if(argslen==0)
		return;

	Event* th=asAtomHandler::as<Event>(obj);
	ARG_CHECK(ARG_UNPACK(th->type)(th->bubbles, false)(th->cancelable, false));
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
	msg += asAtomHandler::toString(args[0],wrk);

	for(unsigned int i=1; i<argslen; i++)
	{
		tiny_string prop(asAtomHandler::toString(args[i],wrk));
		msg += " ";
		msg += prop;
		msg += "=";

		multiname propName(nullptr);
		propName.name_type=multiname::NAME_STRING;
		propName.name_s_id=wrk->getSystemState()->getUniqueStringId(prop);
		propName.ns.push_back(nsNameAndKind(wrk->getSystemState(),"",NAMESPACE));
		asAtom value=asAtomHandler::invalidAtom;
		th->getVariableByMultiname(value,propName,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isValid(value))
			msg += asAtomHandler::toString(value,wrk);
	}
	msg += "]";

	ret = asAtomHandler::fromObject(abstract_s(wrk,msg));
}

Event* Event::cloneImpl() const
{
	return Class<Event>::getInstanceS(getInstanceWorker(),type, bubbles, cancelable);
}

ASFUNCTIONBODY_ATOM(Event,clone)
{
	Event* th=asAtomHandler::as<Event>(obj);
	ret = asAtomHandler::fromObject(th->cloneImpl());
}

ASFUNCTIONBODY_ATOM(Event,stopPropagation)
{
	Event* th=asAtomHandler::as<Event>(obj);
	th->propagationStopped=true;
}
ASFUNCTIONBODY_ATOM(Event,stopImmediatePropagation)
{
	Event* th=asAtomHandler::as<Event>(obj);
	th->immediatePropagationStopped=true;
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

FocusEvent::FocusEvent(ASWorker* wrk, Class_base* c, tiny_string _type):Event(wrk,c, _type)
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
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
}

MouseEvent::MouseEvent(ASWorker* wrk, Class_base* c)
 : Event(wrk,c, "mouseEvent",false,false,SUBTYPE_MOUSE_EVENT), modifiers(LSModifier::None),buttonDown(false), delta(1), localX(0), localY(0), stageX(0), stageY(0), relatedObject(NullRef)
{
}

MouseEvent::MouseEvent(ASWorker* wrk, Class_base* c, const tiny_string& t, number_t lx, number_t ly,
		       bool b, const LSModifier& _modifiers, bool _buttonDown, _NR<InteractiveObject> relObj, int32_t _delta)
  : Event(wrk,c,t,b,false,SUBTYPE_MOUSE_EVENT), modifiers(_modifiers), buttonDown(_buttonDown),delta(_delta), localX(lx), localY(ly), stageX(0), stageY(0), relatedObject(relObj)
{
}

Event* MouseEvent::cloneImpl() const
{
	return Class<MouseEvent>::getInstanceS(getInstanceWorker(),type,localX,localY,bubbles,modifiers,buttonDown,relatedObject,delta);
}

ProgressEvent::ProgressEvent(ASWorker* wrk, Class_base* c):Event(wrk,c, "progress",false,false,SUBTYPE_PROGRESSEVENT),bytesLoaded(0),bytesTotal(0)
{
}

ProgressEvent::ProgressEvent(ASWorker* wrk, Class_base* c, uint32_t loaded, uint32_t total, const tiny_string &t):Event(wrk,c, t,false,false,SUBTYPE_PROGRESSEVENT),bytesLoaded(loaded),bytesTotal(total)
{
}

Event* ProgressEvent::cloneImpl() const
{
	return Class<ProgressEvent>::getInstanceS(getInstanceWorker(),bytesLoaded, bytesTotal);
}

void ProgressEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("PROGRESS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"progress"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SOCKET_DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"socketData"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_ERROR_DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardErrorData"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_INPUT_PROGRESS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardInputProgress"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_OUTPUT_DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"standardOutputData"),DECLARED_TRAIT);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,bytesLoaded,Number);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,bytesTotal,Number);
}

ASFUNCTIONBODY_GETTER_SETTER(ProgressEvent,bytesLoaded)
ASFUNCTIONBODY_GETTER_SETTER(ProgressEvent,bytesTotal)


ASFUNCTIONBODY_ATOM(ProgressEvent,_constructor)
{
	ProgressEvent* th=asAtomHandler::as<ProgressEvent>(obj);
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
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
	c->setDeclaredMethodByQName("updateAfterEvent","",c->getSystemState()->getBuiltinFunction(updateAfterEvent),NORMAL_METHOD,true);
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
	c->setDeclaredMethodByQName("updateAfterEvent","",c->getSystemState()->getBuiltinFunction(updateAfterEvent),NORMAL_METHOD,true);

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
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
	if(argslen>=4)
		th->localX=asAtomHandler::toNumber(args[3]);
	if(argslen>=5)
		th->localY=asAtomHandler::toNumber(args[4]);
	if(argslen>=6)
		th->relatedObject=ArgumentConversionAtom< _NR<InteractiveObject> >::toConcrete(wrk,args[5],NullRef);
	if(argslen>=7)
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[6],false))
			th->modifiers |= LSModifier::Ctrl;
	if(argslen>=8)
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[7],false))
			th->modifiers |= LSModifier::Alt;
	if(argslen>=9)
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[8],false))
			th->modifiers |= LSModifier::Shift;
	if(argslen>=10)
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[9],false))
			th->buttonDown = true;
	if(argslen>=11)
		th->delta=asAtomHandler::toInt(args[10]);
	if(argslen>=12)
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[11],false))
			th->modifiers |= LSModifier::Super;
	if(argslen>=13)
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[12],false))
			th->modifiers |= LSModifier::Ctrl;
	// TODO: args[13] = clickCount
	if(argslen>=14)
		LOG(LOG_NOT_IMPLEMENTED,"MouseEvent: clickcount");
}

ASFUNCTIONBODY_GETTER_SETTER(MouseEvent,relatedObject)
ASFUNCTIONBODY_GETTER(MouseEvent,localX)
ASFUNCTIONBODY_GETTER(MouseEvent,localY)
ASFUNCTIONBODY_GETTER(MouseEvent,stageX)
ASFUNCTIONBODY_GETTER(MouseEvent,stageY)
ASFUNCTIONBODY_GETTER_SETTER(MouseEvent,delta)
ASFUNCTIONBODY_GETTER_SETTER(MouseEvent,buttonDown)

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_localX)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	if(argslen != 1) 
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Wrong number of arguments in setter"); 
		return;
	}
	number_t val=asAtomHandler::toNumber(args[0]);
	th->localX = val;
	//Change StageXY if target!=NULL else don't do anything
	//At this point, the target should be an InteractiveObject but check anyway
	if(asAtomHandler::isValid(th->target) &&(asAtomHandler::toObject(th->target,wrk)->getClass()->isSubClass(Class<InteractiveObject>::getClass(wrk->getSystemState()))))
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>(asAtomHandler::getObject(th->target));
		tar->localToGlobal(th->localX, th->localY, th->stageX, th->stageY);
	}
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_localY)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	if(argslen != 1) 
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"Wrong number of arguments in setter"); 
		return;
	}
	number_t val=asAtomHandler::toNumber(args[0]);
	th->localY = val;
	//Change StageXY if target!=NULL else don't do anything	
	//At this point, the target should be an InteractiveObject but check anyway
	if(asAtomHandler::isValid(th->target) &&(asAtomHandler::toObject(th->target,wrk)->getClass()->isSubClass(Class<InteractiveObject>::getClass(wrk->getSystemState()))))
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>(asAtomHandler::getObject(th->target));
		tar->localToGlobal(th->localX, th->localY, th->stageX, th->stageY);
	}
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_altKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	asAtomHandler::setBool(ret,(bool)(th->modifiers & LSModifier::Alt));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_altKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	th->modifiers |= LSModifier::Alt;
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_controlKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	asAtomHandler::setBool(ret,(bool)(th->modifiers & LSModifier::Ctrl));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_controlKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	th->modifiers |= LSModifier::Ctrl;
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_ctrlKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	asAtomHandler::setBool(ret,(bool)(th->modifiers & LSModifier::Ctrl));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_ctrlKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	th->modifiers |= LSModifier::Ctrl;
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_shiftKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	asAtomHandler::setBool(ret,(bool)(th->modifiers & LSModifier::Shift));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_shiftKey)
{
	MouseEvent* th=asAtomHandler::as<MouseEvent>(obj);
	th->modifiers |= LSModifier::Shift;
}
ASFUNCTIONBODY_ATOM(MouseEvent,updateAfterEvent)
{
	LOG(LOG_NOT_IMPLEMENTED,"MouseEvent::updateAfterEvent not implemented");
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
	else if(asAtomHandler::toObject(t,getInstanceWorker())->getClass()->isSubClass(Class<InteractiveObject>::getClass(getSystemState())))	
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>(asAtomHandler::getObject(t));
		tar->localToGlobal(localX, localY, stageX, stageY);
	}
}

MouseEvent *MouseEvent::getclone() const
{
	return (MouseEvent*)cloneImpl();
}

NativeDragEvent::NativeDragEvent(ASWorker* wrk, Class_base* c)
 : MouseEvent(wrk,c)
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
	MouseEvent::_constructor(ret,wrk,obj,args,baseClassArgs);
	LOG(LOG_NOT_IMPLEMENTED,"NativeDragEvent: constructor");
}

NativeWindowBoundsEvent::NativeWindowBoundsEvent(ASWorker* wrk, Class_base* c) : Event(wrk,c, "")
{
	subtype=SUBTYPE_NATIVEWINDOWBOUNDSEVENT;
}

NativeWindowBoundsEvent::NativeWindowBoundsEvent(ASWorker* wrk, Class_base* c, const tiny_string& t, _NR<Rectangle> _beforeBounds, _NR<Rectangle> _afterBounds)
 : Event(wrk,c,t),afterBounds(_afterBounds),beforeBounds(_beforeBounds)
{
	subtype=SUBTYPE_NATIVEWINDOWBOUNDSEVENT;
}

void NativeWindowBoundsEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("MOVE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"move"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MOVING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"moving"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RESIZE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"resize"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RESIZING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"resizing"),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	
	REGISTER_GETTER_RESULTTYPE(c,afterBounds,Rectangle);
	REGISTER_GETTER_RESULTTYPE(c,beforeBounds,Rectangle);
}

Event* NativeWindowBoundsEvent::cloneImpl() const
{
	return Class<NativeWindowBoundsEvent>::getInstanceS(getInstanceWorker(),type, beforeBounds,afterBounds);
}

ASFUNCTIONBODY_GETTER(NativeWindowBoundsEvent,afterBounds)
ASFUNCTIONBODY_GETTER(NativeWindowBoundsEvent,beforeBounds)

ASFUNCTIONBODY_ATOM(NativeWindowBoundsEvent,_constructor)
{
	NativeWindowBoundsEvent* th=asAtomHandler::as<NativeWindowBoundsEvent>(obj);
	ARG_CHECK(ARG_UNPACK(th->type)(th->bubbles, false)(th->cancelable, false)(th->beforeBounds,NullRef)(th->afterBounds,NullRef));

}
ASFUNCTIONBODY_ATOM(NativeWindowBoundsEvent,_toString)
{
	NativeWindowBoundsEvent* th=asAtomHandler::as<NativeWindowBoundsEvent>(obj);
	tiny_string res = "[NativeWindowBoundsEvent type=";
	res += th->type;
	res += " bubbles=";
	res += th->bubbles ? "true" : "false";
	res += " cancelable=";
	res += th->cancelable ? "true" : "false";
	res += " previousDisplayState=";
	if (th->beforeBounds)
		res += th->beforeBounds->toString();
	else
		res += "null";
	res += " currentDisplayState=";
	if (th->afterBounds)
		res += th->afterBounds->toString();
	else
		res += "null";
	res += "]";
	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
}
		
IOErrorEvent::IOErrorEvent(ASWorker* wrk, Class_base* c, const tiny_string& t, const std::string& e, int id) : ErrorEvent(wrk,c, t,e,id)
{
}

Event *IOErrorEvent::cloneImpl() const
{
	return Class<IOErrorEvent>::getInstanceS(getInstanceWorker(),text, errorMsg,errorID);
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

EventDispatcher::EventDispatcher(ASWorker* wrk, Class_base* c):ASObject(wrk,c),forcedTarget(asAtomHandler::invalidAtom)
{
}

void EventDispatcher::finalize()
{
	forcedTarget = asAtomHandler::invalidAtom;
	clearEventListeners();
	ASObject::finalize();
}
bool EventDispatcher::destruct()
{
	forcedTarget = asAtomHandler::invalidAtom;
	clearEventListeners();
	return ASObject::destruct();
}
bool EventDispatcher::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	for (auto it = handlers.begin(); it != handlers.end(); it++)
	{
		for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
			ret = asAtomHandler::getObjectNoCheck((*it2).f)->countAllCylicMemberReferences(gcstate) || ret;
	}
	return ret;
}

void EventDispatcher::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	ASObject* t = asAtomHandler::getObject(forcedTarget);
	if (t)
		t->prepareShutdown();
	auto it=handlers.begin();
	while(it!=handlers.end())
	{
		auto it2 = it->second.begin();
		while (it2 != it->second.end())
		{
			ASObject* f = asAtomHandler::getObject((*it2).f);
			if (f)
			{
				f->prepareShutdown();
				f->removeStoredMember();
			}
			it2 = it->second.erase(it2);
		}
		it++;
	}
}
void EventDispatcher::clearEventListeners()
{
	auto it=handlers.begin();
	while(it!=handlers.end())
	{
		auto it2 = it->second.begin();
		while (it2 != it->second.end())
		{
			IFunction* f = asAtomHandler::as<IFunction>((*it2).f);
			it2 = it->second.erase(it2);
			f->removeStoredMember();
		}
		it = handlers.erase(it);
	}
}


void EventDispatcher::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->addImplementedInterface(InterfaceClass<IEventDispatcher>::getClass(c->getSystemState()));

	c->setDeclaredMethodByQName("addEventListener","",c->getSystemState()->getBuiltinFunction(addEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasEventListener","",c->getSystemState()->getBuiltinFunction(_hasEventListener,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeEventListener","",c->getSystemState()->getBuiltinFunction(removeEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dispatchEvent","",c->getSystemState()->getBuiltinFunction(dispatchEvent,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);

	IEventDispatcher::linkTraits(c);
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

	bool useWeakReference=false;
	bool useCapture=false;
	int32_t priority=0;
	if(argslen>=3)
		useCapture=asAtomHandler::Boolean_concrete(args[2]);
	if(argslen>=4)
		priority=asAtomHandler::toInt(args[3]);
	if(argslen>=5)
		useWeakReference = asAtomHandler::Boolean_concrete(args[4]);

	const tiny_string& eventName=asAtomHandler::toString(args[0],wrk);
	if(wrk->isPrimordial // don't register frame listeners for background workers
			&& th->is<DisplayObject>() && (eventName=="enterFrame"
				|| eventName=="exitFrame"
				|| eventName=="frameConstructed"
				|| eventName=="render") )
	{
		th->getSystemState()->registerFrameListener(th->as<DisplayObject>());
	}

	{
		Locker l(th->handlersMutex);
		//Search if any listener is already registered for the event
		list<listener>& listeners=th->handlers[eventName];
		const listener newListener(args[1], priority, useCapture, wrk);
		//Ordered insertion
		list<listener>::iterator insertionPoint=lower_bound(listeners.begin(),listeners.end(),newListener);
		IFunction* newfunc = asAtomHandler::as<IFunction>(args[1]);
		if (useWeakReference && !newfunc->inClass)
			LOG(LOG_NOT_IMPLEMENTED,"EventDispatcher::addEventListener parameter useWeakReference is ignored");
		// check if a listener that matches type, use_capture and function is already registered
		if (insertionPoint != listeners.end() && (*insertionPoint).use_capture == newListener.use_capture)
		{
			IFunction* insertPointFunc = asAtomHandler::as<IFunction>((*insertionPoint).f);
			if (insertPointFunc == newfunc || (insertPointFunc->clonedFrom && insertPointFunc->clonedFrom == newfunc->clonedFrom && insertPointFunc->closure_this==newfunc->closure_this))
				return; // don't register the same listener twice
		}
		newfunc->incRef();
		newfunc->addStoredMember();
		listeners.insert(insertionPoint,newListener);
	}
	th->eventListenerAdded(eventName);
}

ASFUNCTIONBODY_ATOM(EventDispatcher,_hasEventListener)
{
	EventDispatcher* th=asAtomHandler::as<EventDispatcher>(obj);
	const tiny_string& eventName=asAtomHandler::toString(args[0],wrk);
	asAtomHandler::setBool(ret,th->hasEventListener(eventName));
}

ASFUNCTIONBODY_ATOM(EventDispatcher,removeEventListener)
{
	EventDispatcher* th=asAtomHandler::as<EventDispatcher>(obj);
	
	if (asAtomHandler::isNull(args[1])) // it seems that null is allowed as function
		return;
	if(!asAtomHandler::isString(args[0]) || !asAtomHandler::isFunction(args[1]))
		throw RunTimeException("Type mismatch in EventDispatcher::removeEventListener");

	const tiny_string& eventName=asAtomHandler::toString(args[0],wrk);

	bool useCapture=false;
	if(argslen>=3)
		useCapture=asAtomHandler::Boolean_concrete(args[2]);

	{
		Locker l(th->handlersMutex);
		map<tiny_string, list<listener> >::iterator h=th->handlers.find(eventName);
		if(h==th->handlers.end())
		{
			LOG(LOG_CALLS,"Event not found");
			return;
		}

		const listener ls(args[1],0,useCapture,wrk);
		std::list<listener>::iterator it=find(h->second.begin(),h->second.end(),ls);
		if(it!=h->second.end())
		{
			ASObject* listenerfunc = asAtomHandler::getObject(it->f);
			assert(listenerfunc);
			h->second.erase(it);
			listenerfunc->removeStoredMember();
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
		th->getSystemState()->unregisterFrameListener(th->as<DisplayObject>());
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
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = BUILTIN_STRINGS::STRING_TARGET;
	e->getVariableByMultiname(target,m,GET_VARIABLE_OPTION::NONE,wrk);
	if(asAtomHandler::isValid(target) && !asAtomHandler::isNull(target) && !asAtomHandler::isUndefined(target))
	{
		ASATOM_DECREF(target);
		//Object must be cloned, cloning is implemented with the clone AS method
		asAtom cloned=asAtomHandler::invalidAtom;
		e->executeASMethod(cloned,"clone", {""}, nullptr, 0);
		//Clone always exists since it's implemented in Event itself
		if(!asAtomHandler::getObject(cloned) || !asAtomHandler::getObject(cloned)->is<Event>())
		{
			asAtomHandler::setBool(ret,false);
			return;
		}

		e = _MR(asAtomHandler::getObject(cloned)->as<Event>());
	}
	if(asAtomHandler::isValid(th->forcedTarget))
		e->setTarget(th->forcedTarget);
	else
		e->setTarget(obj);
	ABCVm::publicHandleEvent(th, e);
	asAtomHandler::setBool(ret,true);
}

ASFUNCTIONBODY_ATOM(EventDispatcher,_constructor)
{
	EventDispatcher* th=asAtomHandler::as<EventDispatcher>(obj);
	asAtom forcedTarget=asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(forcedTarget, asAtomHandler::nullAtom));
	if(asAtomHandler::isValid(forcedTarget))
	{
		if(asAtomHandler::isNull(forcedTarget) || asAtomHandler::isUndefined(forcedTarget))
			forcedTarget=asAtomHandler::invalidAtom;
		else if(!asAtomHandler::toObject(forcedTarget,wrk)->getClass()->isSubClass(InterfaceClass<IEventDispatcher>::getClass(wrk->getSystemState())))
		{
			createError<ArgumentError>(wrk,kInvalidArgumentError,"Wrong argument for EventDispatcher");
			return;
		}
		else
		{
			asAtomHandler::getObject(forcedTarget)->addOwnedObject(th);
		}
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

	LOG(LOG_CALLS,"Handling event " << h->first<<" "<<e->getInstanceWorker());

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
		{
			ASATOM_DECREF(tmpListener[i].f);
			continue;
		}
		if (tmpListener[i].worker != e->getInstanceWorker()) // only handle listeners that are available in the current worker
		{
			ASATOM_DECREF(tmpListener[i].f);
			continue;
		}
		if (e->immediatePropagationStopped)
			break;
		asAtom arg0= asAtomHandler::fromObject(e.getPtr());
		IFunction* func = asAtomHandler::as<IFunction>(tmpListener[i].f);
		asAtom v = asAtomHandler::fromObject(func->closure_this ? func->closure_this : this);
		asAtom ret=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(tmpListener[i].f,tmpListener[i].worker,ret,v,&arg0,1,false);
		ASATOM_DECREF(ret);
		//And now no more, f can also be deleted
		ASATOM_DECREF(tmpListener[i].f);
		afterExecution(e);
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

NetStatusEvent::NetStatusEvent(ASWorker* wrk, Class_base* c, const tiny_string& level, const tiny_string& code):Event(wrk,c, "netStatus"),statuscode(code)
{
	ASObject* info=Class<ASObject>::getInstanceS(wrk);
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
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
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
	multiname infoName(nullptr);
	infoName.name_type=multiname::NAME_STRING;
	infoName.name_s_id=wrk->getSystemState()->getUniqueStringId("info");
	infoName.ns.push_back(nsNameAndKind());
	infoName.isAttribute = false;
	asAtomHandler::getObject(obj)->setVariableByMultiname(infoName, info, CONST_NOT_ALLOWED,nullptr,wrk);
}

Event* NetStatusEvent::cloneImpl() const
{
	NetStatusEvent *clone=Class<NetStatusEvent>::getInstanceS(getInstanceWorker());
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;

	multiname infoName(nullptr);
	infoName.name_type=multiname::NAME_STRING;
	infoName.name_s_id=getSystemState()->getUniqueStringId("info");
	infoName.ns.push_back(nsNameAndKind());
	infoName.isAttribute = false;

	asAtom info=asAtomHandler::invalidAtom;
	const_cast<NetStatusEvent*>(this)->getVariableByMultiname(info,infoName,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
	assert(asAtomHandler::isValid(info));
	ASATOM_INCREF(info);
	clone->setVariableByMultiname(infoName, info, CONST_NOT_ALLOWED,nullptr,getInstanceWorker());

	return clone;
}

FullScreenEvent::FullScreenEvent(ASWorker* wrk, Class_base* c):Event(wrk,c, "fullScreenEvent")
{
}

void FullScreenEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("FULL_SCREEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fullScreen"),DECLARED_TRAIT);
	c->setVariableAtomByQName("FULL_SCREEN_INTERACTIVE_ACCEPTED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"fullScreenInteractiveAccepted"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(FullScreenEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
}

KeyboardEvent::KeyboardEvent(ASWorker* wrk, Class_base* c, tiny_string _type, const AS3KeyCode& _charCode, const AS3KeyCode& _keyCode, const LSModifier& _modifiers)
  : Event(wrk,c, _type,false,false,SUBTYPE_KEYBOARD_EVENT), modifiers(_modifiers), charCode(_charCode), keyCode(_keyCode), keyLocation(0)
{
}

void KeyboardEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, altKey,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, charCode,UInteger);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, commandKey,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, controlKey,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, ctrlKey,Boolean);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, keyCode,UInteger);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, keyLocation,UInteger);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, shiftKey,Boolean);
	c->setVariableAtomByQName("KEY_DOWN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"keyDown"),DECLARED_TRAIT);
	c->setVariableAtomByQName("KEY_UP",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"keyUp"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("updateAfterEvent","",c->getSystemState()->getBuiltinFunction(updateAfterEvent),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(KeyboardEvent,_constructor)
{
	KeyboardEvent* th=asAtomHandler::as<KeyboardEvent>(obj);

	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);

	if(argslen > 3) {
		th->charCode = (AS3KeyCode)asAtomHandler::toUInt(args[3]);
	}
	if(argslen > 4) {
		th->keyCode = (AS3KeyCode)asAtomHandler::toUInt(args[4]);
	}
	if(argslen > 5) {
		th->keyLocation = asAtomHandler::toUInt(args[5]);
	}
	if(argslen > 6) {
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[6],false))
			th->modifiers |= LSModifier::Ctrl;
	}
	if(argslen > 7) {
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[7],false))
			th->modifiers |= LSModifier::Alt;
	}
	if(argslen > 8) {
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[8],false))
			th->modifiers |= LSModifier::Shift;
	}
	if(argslen > 9) {
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[9],false))
			th->modifiers |= LSModifier::Ctrl;
	}
	// args[10] (commandKeyValue) is only supported on Max OSX
	if(argslen > 10) {
		if (ArgumentConversionAtom<bool>::toConcrete(wrk,args[10],false))
			LOG(LOG_NOT_IMPLEMENTED,"KeyboardEvent:commandKeyValue");
	}
}

ASFUNCTIONBODY_GETTER_SETTER(KeyboardEvent, charCode)
ASFUNCTIONBODY_GETTER_SETTER(KeyboardEvent, keyCode)
ASFUNCTIONBODY_GETTER_SETTER(KeyboardEvent, keyLocation)

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_altKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	asAtomHandler::setBool(ret,(bool)(th->modifiers & LSModifier::Alt));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_altKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	th->modifiers |= LSModifier::Alt;
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
	asAtomHandler::setBool(ret,(bool)(th->modifiers & LSModifier::Ctrl));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_controlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	th->modifiers |= LSModifier::Ctrl;
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_ctrlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	asAtomHandler::setBool(ret,(bool)(th->modifiers & LSModifier::Ctrl));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_ctrlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	th->modifiers |= LSModifier::Ctrl;
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_shiftKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	asAtomHandler::setBool(ret,(bool)(th->modifiers & LSModifier::Shift));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_shiftKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(asAtomHandler::getObject(obj));
	th->modifiers |= LSModifier::Shift;
}

ASFUNCTIONBODY_ATOM(KeyboardEvent,updateAfterEvent)
{
	LOG(LOG_NOT_IMPLEMENTED,"KeyboardEvent::updateAfterEvent not implemented");
}

Event* KeyboardEvent::cloneImpl() const
{
	KeyboardEvent *cloned = Class<KeyboardEvent>::getInstanceS(getInstanceWorker());
	cloned->type = type;
	cloned->bubbles = bubbles;
	cloned->cancelable = cancelable;
	cloned->modifiers = modifiers;
	cloned->charCode = charCode;
	cloned->keyCode = keyCode;
	cloned->keyLocation = keyLocation;
	return cloned;
}

TextEvent::TextEvent(ASWorker* wrk, Class_base* c, const tiny_string& t):Event(wrk,c,t)
{
}

void TextEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("TEXT_INPUT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"textInput"),DECLARED_TRAIT);
	c->setVariableAtomByQName("LINK",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"link"),DECLARED_TRAIT);
	REGISTER_GETTER_SETTER(c,text);
}

ASFUNCTIONBODY_GETTER_SETTER(TextEvent,text)

ASFUNCTIONBODY_ATOM(TextEvent,_constructor)
{
	TextEvent* th=asAtomHandler::as<TextEvent>(obj);
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
	if(argslen>=4)
		th->text=asAtomHandler::toString(args[3],wrk);
}

ErrorEvent::ErrorEvent(ASWorker* wrk, Class_base* c, const tiny_string& t, const std::string& e, int id): TextEvent(wrk,c,t), errorMsg(e),errorID(id)
{
}

void ErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"error"),DECLARED_TRAIT);
	REGISTER_GETTER(c,errorID);
}
ASFUNCTIONBODY_GETTER(ErrorEvent,errorID)

Event* ErrorEvent::cloneImpl() const
{
	return Class<ErrorEvent>::getInstanceS(getInstanceWorker(),text, errorMsg,errorID);
}

ASFUNCTIONBODY_ATOM(ErrorEvent,_constructor)
{
	ErrorEvent* th=asAtomHandler::as<ErrorEvent>(obj);
	uint32_t baseClassArgs=imin(argslen,4);
	TextEvent::_constructor(ret,wrk,obj,args,baseClassArgs);
	if(argslen>=5)
		th->errorID=asAtomHandler::toInt(args[4]);
}

SecurityErrorEvent::SecurityErrorEvent(ASWorker* wrk, Class_base* c, const std::string& e):ErrorEvent(wrk,c, "securityError",e)
{
}

void SecurityErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("SECURITY_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"securityError"),DECLARED_TRAIT);
}

AsyncErrorEvent::AsyncErrorEvent(ASWorker* wrk, Class_base* c):ErrorEvent(wrk,c, "asyncError")
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
	ErrorEvent::_constructor(ret,wrk,obj,args,baseClassArgs);
}


UncaughtErrorEvent::UncaughtErrorEvent(ASWorker* wrk, Class_base* c):ErrorEvent(wrk,c, "uncaughtError")
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
	ErrorEvent::_constructor(ret,wrk,obj,args,baseClassArgs);
}

ABCContextInitEvent::ABCContextInitEvent(ABCContext* c, bool l):Event(nullptr,nullptr, "ABCContextInitEvent"),context(c),lazy(l)
{
}

AVM1InitActionEvent::AVM1InitActionEvent(RootMovieClip* r,  _NR<MovieClip> c):Event(nullptr,nullptr, "AVM1InitActionEvent"),root(r),clip(c)
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

ShutdownEvent::ShutdownEvent():Event(nullptr,nullptr, "shutdownEvent")
{
}

void HTTPStatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("HTTP_STATUS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"httpStatus"),DECLARED_TRAIT);

	// Value is undefined and not "httpResponseStatus" like stated in documentation
	c->setVariableAtomByQName("HTTP_RESPONSE_STATUS",nsNameAndKind(),asAtomHandler::fromObject(c->getSystemState()->getUndefinedRef()),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(HTTPStatusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
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

FunctionAsyncEvent::FunctionAsyncEvent(asAtom _f, asAtom _obj, asAtom* _args, uint32_t _numArgs):
		Event(nullptr,nullptr,"FunctionAsyncEvent"),f(_f),obj(_obj),numArgs(_numArgs)
{
	args = new asAtom[numArgs];
	uint32_t i;
	for(i=0; i<numArgs; i++)
	{
		args[i] = _args[i];
	}
}

FunctionAsyncEvent::~FunctionAsyncEvent()
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
	: Event(nullptr,nullptr, "bindClass"),base(b),tag(NULL),class_name(c)
{
}

BindClassEvent::BindClassEvent(DictionaryTag* t, const tiny_string& c)
	: Event(nullptr,nullptr, "bindClass"),tag(t),class_name(c)
{
}

ParseRPCMessageEvent::ParseRPCMessageEvent(_R<ByteArray> ba, _NR<ASObject> c, _NR<Responder> r):
	Event(nullptr,nullptr, "ParseRPCMessageEvent"),message(ba),client(c),responder(r)
{
}

void ParseRPCMessageEvent::finalize()
{
	Event::finalize();
	message.reset();
	client.reset();
	responder.reset();
}

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

void StatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	/* TODO: dispatch this event */
	c->setVariableAtomByQName("STATUS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"status"),DECLARED_TRAIT);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, code, ASString);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, level, ASString);
}

ASFUNCTIONBODY_GETTER_SETTER(StatusEvent, code)
ASFUNCTIONBODY_GETTER_SETTER(StatusEvent, level)

void DataEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextEvent, _constructor, CLASS_SEALED);
	/* TODO: dispatch this event */
	c->setVariableAtomByQName("DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"data"),DECLARED_TRAIT);
	/* TODO: dispatch this event */
	c->setVariableAtomByQName("UPLOAD_COMPLETE_DATA",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"uploadCompleteData"),DECLARED_TRAIT);

	REGISTER_GETTER_SETTER(c, data);
}

ASFUNCTIONBODY_GETTER_SETTER(DataEvent, data)

ASFUNCTIONBODY_ATOM(DataEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	TextEvent::_constructor(ret,wrk,obj,args,baseClassArgs);

	DataEvent* th=asAtomHandler::as<DataEvent>(obj);
	if (argslen >= 4)
	{
		th->data = asAtomHandler::toString(args[3],wrk);
	}
}

Event* DataEvent::cloneImpl() const
{
	DataEvent *clone = Class<DataEvent>::getInstanceS(getInstanceWorker());
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
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
}

DRMErrorEvent::DRMErrorEvent(ASWorker* wrk, Class_base* c) : ErrorEvent(wrk,c, "drmAuthenticate")
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
	ErrorEvent::_constructor(ret,wrk,obj,args,baseClassArgs);
	if(argslen > 3)
		LOG(LOG_NOT_IMPLEMENTED, "DRMErrorEvent constructor doesn't support all parameters");
}

DRMStatusEvent::DRMStatusEvent(ASWorker* wrk, Class_base* c) : Event(wrk,c, "drmAuthenticate")
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
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);
	if(argslen > 3)
		LOG(LOG_NOT_IMPLEMENTED, "DRMStatusEvent constructor doesn't support all parameters");
}

VideoEvent::VideoEvent(ASWorker* wrk, Class_base* c)
  : Event(wrk,c, "renderState"),status("unavailable")
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
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);

	VideoEvent* th=asAtomHandler::as<VideoEvent>(obj);
	if(argslen>=4)
	{
		th->status=asAtomHandler::toString(args[3],wrk);
	}
}

Event* VideoEvent::cloneImpl() const
{
	VideoEvent *clone;
	clone = Class<VideoEvent>::getInstanceS(getInstanceWorker());
	clone->status = status;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

ASFUNCTIONBODY_GETTER(VideoEvent,status)


StageVideoEvent::StageVideoEvent(ASWorker* wrk, Class_base* c)
  : Event(wrk,c, "renderState"),status("unavailable")
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
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);

	StageVideoEvent* th=asAtomHandler::as<StageVideoEvent>(obj);
	if(argslen>=4)
	{
		th->status=asAtomHandler::toString(args[3],wrk);
	}
	if(argslen>=5)
	{
		th->colorSpace=asAtomHandler::toString(args[4],wrk);
	}
}

Event* StageVideoEvent::cloneImpl() const
{
	StageVideoEvent *clone;
	clone = Class<StageVideoEvent>::getInstanceS(getInstanceWorker());
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

StageVideoAvailabilityEvent::StageVideoAvailabilityEvent(ASWorker* wrk, Class_base* c)
  : Event(wrk,c, "stageVideoAvailability"), availability("unavailable")
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
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);

	StageVideoAvailabilityEvent* th=asAtomHandler::as<StageVideoAvailabilityEvent>(obj);
	if(argslen>=4)
	{
		th->availability = asAtomHandler::toString(args[3],wrk);
	}
}

Event* StageVideoAvailabilityEvent::cloneImpl() const
{
	StageVideoAvailabilityEvent *clone;
	clone = Class<StageVideoAvailabilityEvent>::getInstanceS(getInstanceWorker());
	clone->availability = availability;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

ASFUNCTIONBODY_GETTER(StageVideoAvailabilityEvent,availability)

void ContextMenuEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("MENU_ITEM_SELECT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"menuItemSelect"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MENU_SELECT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"menuSelect"),DECLARED_TRAIT);
	REGISTER_GETTER_SETTER(c, mouseTarget);
	REGISTER_GETTER_SETTER(c, contextMenuOwner);
}
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuEvent,mouseTarget)
ASFUNCTIONBODY_GETTER_SETTER(ContextMenuEvent,contextMenuOwner)

ASFUNCTIONBODY_ATOM(ContextMenuEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);

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
	clone = Class<ContextMenuEvent>::getInstanceS(getInstanceWorker());
	clone->mouseTarget = mouseTarget;
	clone->contextMenuOwner = contextMenuOwner;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

ContextMenuEvent::ContextMenuEvent(ASWorker* wrk, Class_base* c) : Event(wrk,c, "ContextMenuEvent")
{
}

ContextMenuEvent::ContextMenuEvent(ASWorker* wrk, Class_base* c, tiny_string t, _NR<InteractiveObject> target, _NR<InteractiveObject> owner)
	: Event(wrk,c, t,false,false,SUBTYPE_CONTEXTMENUEVENT),mouseTarget(target),contextMenuOwner(owner)
{
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

UncaughtErrorEvents::UncaughtErrorEvents(ASWorker* wrk, Class_base* c):
	EventDispatcher(wrk,c)
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
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	
	REGISTER_GETTER_SETTER_RESULTTYPE(c, data,ByteArray);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, position,Number);
}
ASFUNCTIONBODY_GETTER_SETTER(SampleDataEvent,data)
ASFUNCTIONBODY_GETTER_SETTER(SampleDataEvent,position)

ASFUNCTIONBODY_ATOM(SampleDataEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);

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
	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
}

Event* SampleDataEvent::cloneImpl() const
{
	SampleDataEvent *clone;
	clone = Class<SampleDataEvent>::getInstanceS(getInstanceWorker());
	clone->position = position;
	clone->data = data;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

SampleDataEvent::SampleDataEvent(ASWorker* wrk, Class_base* c) : Event(wrk,c, "sampleData",false,false,SUBTYPE_SAMPLEDATA_EVENT),position(0)
{
}

SampleDataEvent::SampleDataEvent(ASWorker* wrk, Class_base* c, _NR<ByteArray> _data, number_t _pos) : Event(wrk,c, "sampleData",false,false,SUBTYPE_SAMPLEDATA_EVENT),data(_data),position(_pos) 
{
}

bool SampleDataEvent::destruct()
{
	data.reset();
	return Event::destruct();
}

void ThrottleEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("THROTTLE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"Throttle"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString),NORMAL_METHOD,true);
	
	REGISTER_GETTER(c, state);
	REGISTER_GETTER(c, targetFrameRate);
}
ASFUNCTIONBODY_GETTER(ThrottleEvent,state)
ASFUNCTIONBODY_GETTER(ThrottleEvent,targetFrameRate)

ASFUNCTIONBODY_ATOM(ThrottleEvent,_constructor)
{
	LOG(LOG_NOT_IMPLEMENTED,"ThrottleEvent is not dispatched anywhere");
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);

	ThrottleEvent* th=asAtomHandler::as<ThrottleEvent>(obj);
	if(argslen>=4)
	{
		th->state = asAtomHandler::toString(args[3],wrk);
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
	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
}

Event* ThrottleEvent::cloneImpl() const
{
	ThrottleEvent *clone;
	clone = Class<ThrottleEvent>::getInstanceS(getInstanceWorker());
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

GameInputEvent::GameInputEvent(ASWorker* wrk, Class_base *c) : Event(wrk,c, "gameinput",false,false,SUBTYPE_GAMEINPUTEVENT)
{
}

GameInputEvent::GameInputEvent(ASWorker* wrk, Class_base *c, NullableRef<GameInputDevice> _device) : Event(wrk,c, "gameinput",false,false,SUBTYPE_GAMEINPUTEVENT),device(_device)
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
ASFUNCTIONBODY_GETTER(GameInputEvent,device)

ASFUNCTIONBODY_ATOM(GameInputEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,wrk,obj,args,baseClassArgs);

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
	clone = Class<GameInputEvent>::getInstanceS(getInstanceWorker());
	clone->device = device;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}


LocalConnectionEvent::LocalConnectionEvent(uint32_t _nameID, uint32_t _methodID, asAtom* _args, uint32_t _numargs): Event(nullptr,nullptr,"LocalConnectionEvent")
  ,nameID(_nameID),methodID(_methodID)
{
	numargs=_numargs;
	args = new asAtom[numargs];
	for (uint32_t i = 0; i < numargs; i++)
	{
		args[i]=_args[i];
		ASATOM_INCREF(args[i]);
	}
}

LocalConnectionEvent::~LocalConnectionEvent()
{
	for (uint32_t i = 0; i < numargs; i++)
	{
		ASATOM_DECREF(args[i]);
	}
	
}

GetMouseTargetEvent::GetMouseTargetEvent(uint32_t _x, uint32_t _y, HIT_TYPE _type): WaitableEvent("GetMouseTargetEvent"),x(_x),y(_y),type(_type)
{
}

SetLoaderContentEvent::SetLoaderContentEvent(_NR<DisplayObject> m, _NR<Loader> _loader): Event(nullptr,nullptr,"SetLoaderContentEvent"),content(m),loader(_loader)
{
}

InitFrameEvent::InitFrameEvent(_NR<DisplayObject> m) : Event(nullptr,nullptr, "InitFrameEvent"),clip(m)
{
}

ExecuteFrameScriptEvent::ExecuteFrameScriptEvent(_NR<DisplayObject> m):Event(nullptr,nullptr, "ExecuteFrameScriptEvent"),clip(m)
{
}

AdvanceFrameEvent::AdvanceFrameEvent(_NR<DisplayObject> m): Event(nullptr,nullptr,"AdvanceFrameEvent"),clip(m)
{
}

RootConstructedEvent::RootConstructedEvent(_NR<DisplayObject> m, bool explicit_): Event(nullptr,nullptr,"RootConstructedEvent"),clip(m),_explicit(explicit_)
{
}

TextInputEvent::TextInputEvent(_NR<InteractiveObject> m, const tiny_string& s) : Event(nullptr,nullptr, "TextInputEvent"),target(m),text(s)
{
}
