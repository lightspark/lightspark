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
	ASObject(cb,T_OBJECT,st),bubbles(b),cancelable(c),defaultPrevented(false),eventPhase(0),type(t),target(),currentTarget()
{
}

void Event::finalize()
{
	ASObject::finalize();
	currentTarget.reset();
}

void Event::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("ACTIVATE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"activate"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ADDED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"added"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ADDED_TO_STAGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"addedToStage"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CANCEL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"cancel"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CHANGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"change"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CLEAR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"clear"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CLOSE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"close"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CLOSING",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"closing"),DECLARED_TRAIT);
	c->setVariableAtomByQName("COMPLETE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"complete"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CONNECT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"connect"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CONTEXT3D_CREATE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"context3DCreate"),DECLARED_TRAIT);
	c->setVariableAtomByQName("COPY",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"copy"),DECLARED_TRAIT);
	c->setVariableAtomByQName("CUT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"cut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DEACTIVATE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"deactivate"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DISPLAYING",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"displaying"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ENTER_FRAME",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"enterFrame"),DECLARED_TRAIT);
	c->setVariableAtomByQName("EXIT_FRAME",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"exitFrame"),DECLARED_TRAIT);
	c->setVariableAtomByQName("EXITING",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"exiting"),DECLARED_TRAIT);
	c->setVariableAtomByQName("FRAME_CONSTRUCTED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"frameConstructed"),DECLARED_TRAIT);
	c->setVariableAtomByQName("FULLSCREEN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"fullScreen"),DECLARED_TRAIT);
	c->setVariableAtomByQName("HTML_BOUNDS_CHANGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"htmlBoundsChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("HTML_DOM_INITIALIZE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"htmlDOMInitialize"),DECLARED_TRAIT);
	c->setVariableAtomByQName("HTML_RENDER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"htmlRender"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ID3",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"id3"),DECLARED_TRAIT);
	c->setVariableAtomByQName("INIT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"init"),DECLARED_TRAIT);
	c->setVariableAtomByQName("LOCATION_CHANGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"locationChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_LEAVE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"mouseLeave"),DECLARED_TRAIT);
	c->setVariableAtomByQName("NETWORK_CHANGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"networkChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("OPEN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"open"),DECLARED_TRAIT);
	c->setVariableAtomByQName("PASTE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"paste"),DECLARED_TRAIT);
	c->setVariableAtomByQName("REMOVED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"removed"),DECLARED_TRAIT);
	c->setVariableAtomByQName("REMOVED_FROM_STAGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"removedFromStage"),DECLARED_TRAIT);
	c->setVariableAtomByQName("RENDER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"render"),DECLARED_TRAIT);
	c->setVariableAtomByQName("RESIZE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"resize"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SCROLL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"scroll"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SELECT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"select"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SELECT_ALL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"selectAll"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SOUND_COMPLETE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"soundComplete"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_ERROR_CLOSE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standardErrorClose"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_INPUT_CLOSE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standardInputClose"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_OUTPUT_CLOSE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standardOutputClose"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TAB_CHILDREN_CHANGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"tabChildrenChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TAB_ENABLED_CHANGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"tabEnabledChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TAB_INDEX_CHANGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"tabIndexChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TEXT_INTERACTION_MODE_CHANGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"textInteractionModeChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TEXTURE_READY",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"textureReady"),DECLARED_TRAIT);
	c->setVariableAtomByQName("UNLOAD",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"unload"),DECLARED_TRAIT);
	c->setVariableAtomByQName("USER_IDLE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"userIdle"),DECLARED_TRAIT);
	c->setVariableAtomByQName("USER_PRESENT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"userPresent"),DECLARED_TRAIT);

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

	Event* th=obj.as<Event>();
	ARG_UNPACK_ATOM(th->type)(th->bubbles, false)(th->cancelable, false);
}

ASFUNCTIONBODY_GETTER(Event,currentTarget);
ASFUNCTIONBODY_GETTER(Event,target);
ASFUNCTIONBODY_GETTER(Event,type);
ASFUNCTIONBODY_GETTER(Event,eventPhase);
ASFUNCTIONBODY_GETTER(Event,bubbles);
ASFUNCTIONBODY_GETTER(Event,cancelable);

ASFUNCTIONBODY_ATOM(Event,_isDefaultPrevented)
{
	Event* th=obj.as<Event>();
	ret.setBool(th->defaultPrevented);
}

ASFUNCTIONBODY_ATOM(Event,_preventDefault)
{
	Event* th=obj.as<Event>();
	th->defaultPrevented = true;
}

ASFUNCTIONBODY_ATOM(Event,formatToString)
{
	assert_and_throw(argslen>=1);
	Event* th=obj.as<Event>();
	tiny_string msg = "[";
	msg += args[0].toString(sys);

	for(unsigned int i=1; i<argslen; i++)
	{
		tiny_string prop(args[i].toString(sys));
		msg += " ";
		msg += prop;
		msg += "=";

		multiname propName(NULL);
		propName.name_type=multiname::NAME_STRING;
		propName.name_s_id=sys->getUniqueStringId(prop);
		propName.ns.push_back(nsNameAndKind(sys,"",NAMESPACE));
		asAtom value;
		th->getVariableByMultiname(value,propName);
		if (value.type != T_INVALID)
			msg += value.toString(sys);
	}
	msg += "]";

	ret = asAtom::fromObject(abstract_s(sys,msg));
}

Event* Event::cloneImpl() const
{
	return Class<Event>::getInstanceS(getSystemState(),type, bubbles, cancelable);
}

ASFUNCTIONBODY_ATOM(Event,clone)
{
	Event* th=obj.as<Event>();
	ret = asAtom::fromObject(th->cloneImpl());
}

ASFUNCTIONBODY_ATOM(Event,stopPropagation)
{
	//Event* th=obj.as<Event>();
	LOG(LOG_NOT_IMPLEMENTED,"Event.stopPropagation not implemented");
}
ASFUNCTIONBODY_ATOM(Event,stopImmediatePropagation)
{
	//Event* th=obj.as<Event>();
	LOG(LOG_NOT_IMPLEMENTED,"Event.stopImmediatePropagation not implemented");
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
	c->setVariableAtomByQName("CAPTURING_PHASE",nsNameAndKind(),asAtom(CAPTURING_PHASE),DECLARED_TRAIT);
	c->setVariableAtomByQName("BUBBLING_PHASE",nsNameAndKind(),asAtom(BUBBLING_PHASE),DECLARED_TRAIT);
	c->setVariableAtomByQName("AT_TARGET",nsNameAndKind(),asAtom(AT_TARGET),DECLARED_TRAIT);
}

FocusEvent::FocusEvent(Class_base* c):Event(c, "focusEvent")
{
}

void FocusEvent::sinit(Class_base* c)
{	
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("FOCUS_IN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"focusIn"),DECLARED_TRAIT);
	c->setVariableAtomByQName("FOCUS_OUT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"focusOut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_FOCUS_CHANGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"mouseFocusChange"),DECLARED_TRAIT);
	c->setVariableAtomByQName("KEY_FOCUS_CHANGE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"keyFocusChange"),DECLARED_TRAIT);
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
	c->setVariableAtomByQName("PROGRESS",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"progress"),DECLARED_TRAIT);
	c->setVariableAtomByQName("SOCKET_DATA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"socketData"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_ERROR_DATA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standardErrorData"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_INPUT_PROGRESS",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standardInputProgress"),DECLARED_TRAIT);
	c->setVariableAtomByQName("STANDARD_OUTPUT_DATA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standardOutputData"),DECLARED_TRAIT);
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
	ProgressEvent* th=obj.as<ProgressEvent>();
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
	if(argslen>=4)
		th->bytesLoaded=args[3].toInt();
	if(argslen>=5)
		th->bytesTotal=args[4].toInt();
}

void TimerEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("TIMER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"timer"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TIMER_COMPLETE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"timerComplete"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("updateAfterEvent","",Class<IFunction>::getFunction(c->getSystemState(),updateAfterEvent),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(TimerEvent,updateAfterEvent)
{
	LOG(LOG_NOT_IMPLEMENTED,"TimerEvent::updateAfterEvent not implemented");
}

void MouseEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("CLICK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"click"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DOUBLE_CLICK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"doubleClick"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_DOWN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"mouseDown"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_OUT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"mouseOut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_OVER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"mouseOver"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_UP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"mouseUp"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_WHEEL",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"mouseWheel"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MOUSE_MOVE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"mouseMove"),DECLARED_TRAIT);
	c->setVariableAtomByQName("RIGHT_CLICK",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"rightClick"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ROLL_OVER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"rollOver"),DECLARED_TRAIT);
	c->setVariableAtomByQName("ROLL_OUT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"rollOut"),DECLARED_TRAIT);
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
	MouseEvent* th=obj.as<MouseEvent>();
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
	if(argslen>=4)
		th->localX=args[3].toNumber();
	if(argslen>=5)
		th->localY=args[4].toNumber();
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
		th->delta=args[10].toInt();
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
	MouseEvent* th=obj.as<MouseEvent>();
	if(argslen != 1) 
		throw Class<ArgumentError>::getInstanceS(sys,"Wrong number of arguments in setter"); 
	number_t val=args[0].toNumber();
	th->localX = val;
	//Change StageXY if target!=NULL else don't do anything
	//At this point, the target should be an InteractiveObject but check anyway
	if(th->target.type != T_INVALID &&(th->target.toObject(sys)->getClass()->isSubClass(Class<InteractiveObject>::getClass(sys))))
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>((th->target).getObject());
		tar->localToGlobal(th->localX, th->localY, th->stageX, th->stageY);
	}
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_localY)
{
	MouseEvent* th=obj.as<MouseEvent>();
	if(argslen != 1) 
		throw Class<ArgumentError>::getInstanceS(sys,"Wrong number of arguments in setter"); 
	number_t val=args[0].toNumber();
	th->localY = val;
	//Change StageXY if target!=NULL else don't do anything	
	//At this point, the target should be an InteractiveObject but check anyway
	if(th->target.type != T_INVALID &&(th->target.toObject(sys)->getClass()->isSubClass(Class<InteractiveObject>::getClass(sys))))
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>((th->target).getObject());
		tar->localToGlobal(th->localX, th->localY, th->stageX, th->stageY);
	}
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_altKey)
{
	MouseEvent* th=obj.as<MouseEvent>();
	ret.setBool((bool)(th->modifiers & KMOD_ALT));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_altKey)
{
	MouseEvent* th=obj.as<MouseEvent>();
	th->modifiers |= KMOD_ALT;
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_controlKey)
{
	MouseEvent* th=obj.as<MouseEvent>();
	ret.setBool((bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_controlKey)
{
	MouseEvent* th=obj.as<MouseEvent>();
	th->modifiers |= KMOD_CTRL;
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_ctrlKey)
{
	MouseEvent* th=obj.as<MouseEvent>();
	ret.setBool((bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_ctrlKey)
{
	MouseEvent* th=obj.as<MouseEvent>();
	th->modifiers |= KMOD_CTRL;
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_shiftKey)
{
	MouseEvent* th=obj.as<MouseEvent>();
	ret.setBool((bool)(th->modifiers & KMOD_SHIFT));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_shiftKey)
{
	MouseEvent* th=obj.as<MouseEvent>();
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
	if(t.type == T_INVALID)
	{
		localX = 0;
		localY = 0;
		stageX = 0;
		stageY = 0;
		relatedObject = NullRef;
	}
	//If t is non null, it should be an InteractiveObject
	else if(t.toObject(getSystemState())->getClass()->isSubClass(Class<InteractiveObject>::getClass(getSystemState())))	
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>(t.getObject());
		tar->localToGlobal(localX, localY, stageX, stageY);
	}
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
	c->setVariableAtomByQName("IO_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"ioError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("DISK_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"diskError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NETWORK_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"networkError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("VERIFY_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"verifyError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD_ERROR_IO_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standardErrorIoError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD_INPUT_IO_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standardInputIoError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("STANDARD_OUTPUT_IO_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"standardOutputIoError"),CONSTANT_TRAIT);
}

EventDispatcher::EventDispatcher(Class_base* c):ASObject(c)
{
}

void EventDispatcher::finalize()
{
	ASObject::finalize();
	handlers.clear();
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
		LOG(LOG_INFO, it->first);
}

ASFUNCTIONBODY_ATOM(EventDispatcher,addEventListener)
{
	EventDispatcher* th=obj.as<EventDispatcher>();
	if(args[0].type !=T_STRING || args[1].type !=T_FUNCTION)
		//throw RunTimeException("Type mismatch in EventDispatcher::addEventListener");
		return;

	bool useCapture=false;
	int32_t priority=0;

	if(argslen>=3)
		useCapture=args[2].Boolean_concrete();
	if(argslen>=4)
		priority=args[3].toInt();

	const tiny_string& eventName=args[0].toString(sys);

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
	EventDispatcher* th=obj.as<EventDispatcher>();
	const tiny_string& eventName=args[0].toString(sys);
	ret.setBool(th->hasEventListener(eventName));
}

ASFUNCTIONBODY_ATOM(EventDispatcher,removeEventListener)
{
	EventDispatcher* th=obj.as<EventDispatcher>();
	
	if (args[1].type == T_NULL) // it seems that null is allowed as function
		return;
	if(args[0].type !=T_STRING || args[1].type !=T_FUNCTION)
		throw RunTimeException("Type mismatch in EventDispatcher::removeEventListener");

	const tiny_string& eventName=args[0].toString(sys);

	bool useCapture=false;
	if(argslen>=3)
		useCapture=args[2].Boolean_concrete();

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
			h->second.erase(it);
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
	EventDispatcher* th=obj.as<EventDispatcher>();
	if(!args[0].is<Event>())
	{
		ret.setBool(false);
		return;
	}

	ASATOM_INCREF(args[0]);
	_R<Event> e=_MR(Class<Event>::cast(args[0].getObject()));

	// Must call the AS getter, because the getter may have been
	// overridden
	asAtom target;
	e->getVariableByMultiname(target,"target", {""});
	if(target.type != T_INVALID && target.type != T_NULL && target.type != T_UNDEFINED)
	{
		//Object must be cloned, cloning is implemented with the clone AS method
		asAtom cloned;
		e->executeASMethod(cloned,"clone", {""}, NULL, 0);
		//Clone always exists since it's implemented in Event itself
		if(!cloned.getObject() || !cloned.getObject()->is<Event>())
		{
			ret.setBool(false);
			return;
		}

		ASATOM_INCREF(cloned);
		e = _MR(cloned.getObject()->as<Event>());
	}
	if(th->forcedTarget.type != T_INVALID)
		e->setTarget(th->forcedTarget);
	th->incRef();
	ABCVm::publicHandleEvent(_MR(th), e);
	ret.setBool(true);
}

ASFUNCTIONBODY_ATOM(EventDispatcher,_constructor)
{
	EventDispatcher* th=obj.as<EventDispatcher>();
	asAtom forcedTarget;
	ARG_UNPACK_ATOM(forcedTarget, asAtom::nullAtom);
	if(forcedTarget.type != T_INVALID)
	{
		if(forcedTarget.type==T_NULL || forcedTarget.type==T_UNDEFINED)
			forcedTarget=asAtom::invalidAtom;
		else if(!forcedTarget.toObject(sys)->getClass()->isSubClass(InterfaceClass<IEventDispatcher>::getClass(sys)))
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
	{
		LOG(LOG_CALLS,_("Not handled event ") << e->type);
		return;
	}

	LOG(LOG_CALLS, _("Handling event ") << h->first);

	//Create a temporary copy of the listeners, as the list can be modified during the calls
	vector<listener> tmpListener(h->second.begin(),h->second.end());
	l.release();
	for(unsigned int i=0;i<tmpListener.size();i++)
	{
		if( (e->eventPhase == EventPhase::BUBBLING_PHASE && tmpListener[i].use_capture)
		||  (e->eventPhase == EventPhase::CAPTURING_PHASE && !tmpListener[i].use_capture))
			continue;
		//tmpListener is now also owned by the vector
		ASATOM_INCREF(tmpListener[i].f);
		//If the f is a class method, the 'this' is ignored
		asAtom arg0= asAtom::fromObject(e.getPtr());
		asAtom v = asAtom::fromObject(this);
		asAtom ret;
		tmpListener[i].f.callFunction(ret,v,&arg0,1,false);
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
	info->setVariableAtomByQName("level",nsNameAndKind(),asAtom::fromString(c->getSystemState(),level),DECLARED_TRAIT);
	info->setVariableAtomByQName("code",nsNameAndKind(),asAtom::fromString(c->getSystemState(),code),DECLARED_TRAIT);
	setVariableByQName("info","",info, DECLARED_TRAIT);
}

void NetStatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("NET_STATUS",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"netStatus"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(NetStatusEvent,_constructor)
{
	//Also call the base class constructor, using only the first arguments
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
	asAtom info;
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
		info=asAtom::nullAtom;
	}
	multiname infoName(NULL);
	infoName.name_type=multiname::NAME_STRING;
	infoName.name_s_id=sys->getUniqueStringId("info");
	infoName.ns.push_back(nsNameAndKind());
	infoName.isAttribute = false;
	obj.getObject()->setVariableByMultiname(infoName, info, CONST_NOT_ALLOWED);
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

	asAtom info;
	const_cast<NetStatusEvent*>(this)->getVariableByMultiname(info,infoName);
	assert(info.type != T_INVALID);
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
	c->setVariableAtomByQName("FULL_SCREEN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"fullScreen"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(FullScreenEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
}

KeyboardEvent::KeyboardEvent(Class_base* c, tiny_string _type, uint32_t _charcode, uint32_t _keycode, SDL_Keymod _modifiers)
  : Event(c, _type,false,false,SUBTYPE_KEYBOARD_EVENT), modifiers(_modifiers), charCode(_charcode), keyCode(_keycode), keyLocation(0)
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
	c->setVariableAtomByQName("KEY_DOWN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"keyDown"),DECLARED_TRAIT);
	c->setVariableAtomByQName("KEY_UP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"keyUp"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(KeyboardEvent,_constructor)
{
	KeyboardEvent* th=obj.as<KeyboardEvent>();

	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	if(argslen > 3) {
		th->charCode = args[3].toUInt();
	}
	if(argslen > 4) {
		th->keyCode = args[4].toUInt();
	}
	if(argslen > 5) {
		th->keyLocation = args[5].toUInt();
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
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	ret.setBool((bool)(th->modifiers & KMOD_ALT));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_altKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	th->modifiers |= KMOD_ALT;
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_commandKey)
{
	// Supported only on OSX
	ret.setBool(false);
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_commandKey)
{
	// Supported only on OSX
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_controlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	ret.setBool((bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_controlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	th->modifiers |= KMOD_CTRL;
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_ctrlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	ret.setBool((bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_ctrlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	th->modifiers |= KMOD_CTRL;
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_shiftKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	ret.setBool((bool)(th->modifiers & KMOD_SHIFT));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_shiftKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
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
	return cloned;
}

TextEvent::TextEvent(Class_base* c,const tiny_string& t):Event(c,t)
{
}

void TextEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("TEXT_INPUT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"textInput"),DECLARED_TRAIT);
	REGISTER_GETTER_SETTER(c,text);
}

ASFUNCTIONBODY_GETTER_SETTER(TextEvent,text);

ASFUNCTIONBODY_ATOM(TextEvent,_constructor)
{
	TextEvent* th=obj.as<TextEvent>();
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);
	if(argslen>=4)
		th->text=args[3].toString(sys);
}

ErrorEvent::ErrorEvent(Class_base* c, const tiny_string& t, const std::string& e, int id): TextEvent(c,t), errorMsg(e),errorID(id)
{
}

void ErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"error"),DECLARED_TRAIT);
	REGISTER_GETTER(c,errorID);
}
ASFUNCTIONBODY_GETTER(ErrorEvent,errorID);

Event* ErrorEvent::cloneImpl() const
{
	return Class<ErrorEvent>::getInstanceS(getSystemState(),text, errorMsg,errorID);
}

ASFUNCTIONBODY_ATOM(ErrorEvent,_constructor)
{
	ErrorEvent* th=obj.as<ErrorEvent>();
	uint32_t baseClassArgs=imin(argslen,4);
	TextEvent::_constructor(ret,sys,obj,args,baseClassArgs);
	if(argslen>=5)
		th->errorID=args[4].toInt();
}

SecurityErrorEvent::SecurityErrorEvent(Class_base* c, const std::string& e):ErrorEvent(c, "securityError",e)
{
}

void SecurityErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("SECURITY_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"securityError"),DECLARED_TRAIT);
}

AsyncErrorEvent::AsyncErrorEvent(Class_base* c):ErrorEvent(c, "asyncError")
{
}

void AsyncErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("ASYNC_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"asyncError"),DECLARED_TRAIT);
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
	c->setVariableAtomByQName("UNCAUGHT_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"uncaughtError"),DECLARED_TRAIT);
}

ASFUNCTIONBODY_ATOM(UncaughtErrorEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,4);
	ErrorEvent::_constructor(ret,sys,obj,args,baseClassArgs);
}

ABCContextInitEvent::ABCContextInitEvent(ABCContext* c, bool l):Event(NULL, "ABCContextInitEvent"),context(c),lazy(l)
{
}

ShutdownEvent::ShutdownEvent():Event(NULL, "shutdownEvent")
{
}

void HTTPStatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("HTTP_STATUS",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"httpStatus"),DECLARED_TRAIT);
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
	c->setVariableAtomByQName("STATUS",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"status"),DECLARED_TRAIT);
}

void DataEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextEvent, _constructor, CLASS_SEALED);
	/* TODO: dispatch this event */
	c->setVariableAtomByQName("DATA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"data"),DECLARED_TRAIT);
	/* TODO: dispatch this event */
	c->setVariableAtomByQName("UPLOAD_COMPLETE_DATA",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"uploadCompleteData"),DECLARED_TRAIT);

	REGISTER_GETTER_SETTER(c, data);
}

ASFUNCTIONBODY_GETTER_SETTER(DataEvent, data);

ASFUNCTIONBODY_ATOM(DataEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	TextEvent::_constructor(ret,sys,obj,args,baseClassArgs);

	DataEvent* th=obj.as<DataEvent>();
	if (argslen >= 4)
	{
		th->data = args[3].toString(sys);
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
	c->setVariableAtomByQName("INVOKE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"invoke"),DECLARED_TRAIT);
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
	c->setVariableAtomByQName("DRM_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"drmError"),DECLARED_TRAIT);
	c->setVariableAtomByQName("DRM_LOAD_DEVICEID_ERROR",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"drmLoadDeviceIdError"),DECLARED_TRAIT);
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
	c->setVariableAtomByQName("DRM_STATUS",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"drmStatus"),DECLARED_TRAIT);
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
	c->setVariableAtomByQName("RENDER_STATE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"renderState"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RENDER_STATUS_ACCELERATED",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"accelerated"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RENDER_STATUS_SOFTWARE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"software"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("RENDER_STATUS_UNAVAILABLE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"unavailable"),CONSTANT_TRAIT);
	REGISTER_GETTER(c,status);
}

ASFUNCTIONBODY_ATOM(VideoEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	VideoEvent* th=obj.as<VideoEvent>();
	if(argslen>=4)
	{
		th->status=args[3].toString(sys);
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

ASFUNCTIONBODY_GETTER(VideoEvent,status);


StageVideoEvent::StageVideoEvent(Class_base* c)
  : Event(c, "renderState"),status("unavailable")
{
}

void StageVideoEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("RENDER_STATE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"renderState"),CONSTANT_TRAIT);
	REGISTER_GETTER(c,colorSpace);
	REGISTER_GETTER(c,status);
}

ASFUNCTIONBODY_ATOM(StageVideoEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	StageVideoEvent* th=obj.as<StageVideoEvent>();
	if(argslen>=4)
	{
		th->status=args[3].toString(sys);
	}
	if(argslen>=5)
	{
		th->colorSpace=args[4].toString(sys);
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

ASFUNCTIONBODY_GETTER(StageVideoEvent,colorSpace);
ASFUNCTIONBODY_GETTER(StageVideoEvent,status);

StageVideoAvailabilityEvent::StageVideoAvailabilityEvent(Class_base* c)
  : Event(c, "stageVideoAvailability"), availability("unavailable")
{
}

void StageVideoAvailabilityEvent::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, Event, CLASS_SEALED);
	c->setVariableAtomByQName("STAGE_VIDEO_AVAILABILITY",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"stageVideoAvailability"),DECLARED_TRAIT);
	REGISTER_GETTER(c, availability);
}

ASFUNCTIONBODY_ATOM(StageVideoAvailabilityEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(ret,sys,obj,args,baseClassArgs);

	StageVideoAvailabilityEvent* th=obj.as<StageVideoAvailabilityEvent>();
	if(argslen>=4)
	{
		th->availability = args[3].toString(sys);
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
	c->setVariableAtomByQName("MENU_ITEM_SELECT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"menuItemSelect"),DECLARED_TRAIT);
	c->setVariableAtomByQName("MENU_SELECT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"menuSelect"),DECLARED_TRAIT);
}


void TouchEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("TOUCH_BEGIN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"touchBegin"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_END",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"touchEnd"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_MOVE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"touchMove"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_OUT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"touchOut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_OVER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"touchOver"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_ROLL_OUT",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"touchRollOut"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_ROLL_OVER",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"touchRollOver"),DECLARED_TRAIT);
	c->setVariableAtomByQName("TOUCH_TAP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"touchTap"),DECLARED_TRAIT);
}

void GestureEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("GESTURE_TWO_FINGER_TAP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"gestureTwoFingerTap"),DECLARED_TRAIT);
}

void PressAndTapGestureEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, GestureEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("GESTURE_PRESS_AND_TAP",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"gesturePressAndTap"),DECLARED_TRAIT);
}

void TransformGestureEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, GestureEvent, _constructor, CLASS_SEALED);
	c->setVariableAtomByQName("GESTURE_PAN",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"gesturePan"),DECLARED_TRAIT);
	c->setVariableAtomByQName("GESTURE_ROTATE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"gestureRotate"),DECLARED_TRAIT);
	c->setVariableAtomByQName("GESTURE_SWIPE",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"gestureSwipe"),DECLARED_TRAIT);
	c->setVariableAtomByQName("GESTURE_ZOOM",nsNameAndKind(),asAtom::fromString(c->getSystemState(),"gestureZoom"),DECLARED_TRAIT);
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
