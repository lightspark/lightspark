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
	target.reset();
	currentTarget.reset();
}

void Event::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->setVariableByQName("ACTIVATE","",abstract_s(c->getSystemState(),"activate"),DECLARED_TRAIT);
	c->setVariableByQName("ADDED","",abstract_s(c->getSystemState(),"added"),DECLARED_TRAIT);
	c->setVariableByQName("ADDED_TO_STAGE","",abstract_s(c->getSystemState(),"addedToStage"),DECLARED_TRAIT);
	c->setVariableByQName("CANCEL","",abstract_s(c->getSystemState(),"cancel"),DECLARED_TRAIT);
	c->setVariableByQName("CHANGE","",abstract_s(c->getSystemState(),"change"),DECLARED_TRAIT);
	c->setVariableByQName("CLEAR","",abstract_s(c->getSystemState(),"clear"),DECLARED_TRAIT);
	c->setVariableByQName("CLOSE","",abstract_s(c->getSystemState(),"close"),DECLARED_TRAIT);
	c->setVariableByQName("CLOSING","",abstract_s(c->getSystemState(),"closing"),DECLARED_TRAIT);
	c->setVariableByQName("COMPLETE","",abstract_s(c->getSystemState(),"complete"),DECLARED_TRAIT);
	c->setVariableByQName("CONNECT","",abstract_s(c->getSystemState(),"connect"),DECLARED_TRAIT);
	c->setVariableByQName("CONTEXT3D_CREATE","",abstract_s(c->getSystemState(),"context3DCreate"),DECLARED_TRAIT);
	c->setVariableByQName("COPY","",abstract_s(c->getSystemState(),"copy"),DECLARED_TRAIT);
	c->setVariableByQName("CUT","",abstract_s(c->getSystemState(),"cut"),DECLARED_TRAIT);
	c->setVariableByQName("DEACTIVATE","",abstract_s(c->getSystemState(),"deactivate"),DECLARED_TRAIT);
	c->setVariableByQName("DISPLAYING","",abstract_s(c->getSystemState(),"displaying"),DECLARED_TRAIT);
	c->setVariableByQName("ENTER_FRAME","",abstract_s(c->getSystemState(),"enterFrame"),DECLARED_TRAIT);
	c->setVariableByQName("EXIT_FRAME","",abstract_s(c->getSystemState(),"exitFrame"),DECLARED_TRAIT);
	c->setVariableByQName("EXITING","",abstract_s(c->getSystemState(),"exiting"),DECLARED_TRAIT);
	c->setVariableByQName("FRAME_CONSTRUCTED","",abstract_s(c->getSystemState(),"frameConstructed"),DECLARED_TRAIT);
	c->setVariableByQName("FULLSCREEN","",abstract_s(c->getSystemState(),"fullScreen"),DECLARED_TRAIT);
	c->setVariableByQName("HTML_BOUNDS_CHANGE","",abstract_s(c->getSystemState(),"htmlBoundsChange"),DECLARED_TRAIT);
	c->setVariableByQName("HTML_DOM_INITIALIZE","",abstract_s(c->getSystemState(),"htmlDOMInitialize"),DECLARED_TRAIT);
	c->setVariableByQName("HTML_RENDER","",abstract_s(c->getSystemState(),"htmlRender"),DECLARED_TRAIT);
	c->setVariableByQName("ID3","",abstract_s(c->getSystemState(),"id3"),DECLARED_TRAIT);
	c->setVariableByQName("INIT","",abstract_s(c->getSystemState(),"init"),DECLARED_TRAIT);
	c->setVariableByQName("LOCATION_CHANGE","",abstract_s(c->getSystemState(),"locationChange"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_LEAVE","",abstract_s(c->getSystemState(),"mouseLeave"),DECLARED_TRAIT);
	c->setVariableByQName("NETWORK_CHANGE","",abstract_s(c->getSystemState(),"networkChange"),DECLARED_TRAIT);
	c->setVariableByQName("OPEN","",abstract_s(c->getSystemState(),"open"),DECLARED_TRAIT);
	c->setVariableByQName("PASTE","",abstract_s(c->getSystemState(),"paste"),DECLARED_TRAIT);
	c->setVariableByQName("REMOVED","",abstract_s(c->getSystemState(),"removed"),DECLARED_TRAIT);
	c->setVariableByQName("REMOVED_FROM_STAGE","",abstract_s(c->getSystemState(),"removedFromStage"),DECLARED_TRAIT);
	c->setVariableByQName("RENDER","",abstract_s(c->getSystemState(),"render"),DECLARED_TRAIT);
	c->setVariableByQName("RESIZE","",abstract_s(c->getSystemState(),"resize"),DECLARED_TRAIT);
	c->setVariableByQName("SCROLL","",abstract_s(c->getSystemState(),"scroll"),DECLARED_TRAIT);
	c->setVariableByQName("SELECT","",abstract_s(c->getSystemState(),"select"),DECLARED_TRAIT);
	c->setVariableByQName("SELECT_ALL","",abstract_s(c->getSystemState(),"selectAll"),DECLARED_TRAIT);
	c->setVariableByQName("SOUND_COMPLETE","",abstract_s(c->getSystemState(),"soundComplete"),DECLARED_TRAIT);
	c->setVariableByQName("STANDARD_ERROR_CLOSE","",abstract_s(c->getSystemState(),"standardErrorClose"),DECLARED_TRAIT);
	c->setVariableByQName("STANDARD_INPUT_CLOSE","",abstract_s(c->getSystemState(),"standardInputClose"),DECLARED_TRAIT);
	c->setVariableByQName("STANDARD_OUTPUT_CLOSE","",abstract_s(c->getSystemState(),"standardOutputClose"),DECLARED_TRAIT);
	c->setVariableByQName("TAB_CHILDREN_CHANGE","",abstract_s(c->getSystemState(),"tabChildrenChange"),DECLARED_TRAIT);
	c->setVariableByQName("TAB_ENABLED_CHANGE","",abstract_s(c->getSystemState(),"tabEnabledChange"),DECLARED_TRAIT);
	c->setVariableByQName("TAB_INDEX_CHANGE","",abstract_s(c->getSystemState(),"tabIndexChange"),DECLARED_TRAIT);
	c->setVariableByQName("TEXT_INTERACTION_MODE_CHANGE","",abstract_s(c->getSystemState(),"textInteractionModeChange"),DECLARED_TRAIT);
	c->setVariableByQName("TEXTURE_READY","",abstract_s(c->getSystemState(),"textureReady"),DECLARED_TRAIT);
	c->setVariableByQName("UNLOAD","",abstract_s(c->getSystemState(),"unload"),DECLARED_TRAIT);
	c->setVariableByQName("USER_IDLE","",abstract_s(c->getSystemState(),"userIdle"),DECLARED_TRAIT);
	c->setVariableByQName("USER_PRESENT","",abstract_s(c->getSystemState(),"userPresent"),DECLARED_TRAIT);

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

ASFUNCTIONBODY(Event,_constructor)
{
	// Event constructor is called with zero arguments internally
	if(argslen==0)
		return NULL;

	Event* th=static_cast<Event*>(obj);
	ARG_UNPACK(th->type)(th->bubbles, false)(th->cancelable, false);
	return NULL;
}

ASFUNCTIONBODY_GETTER(Event,currentTarget);
ASFUNCTIONBODY_GETTER(Event,target);
ASFUNCTIONBODY_GETTER(Event,type);
ASFUNCTIONBODY_GETTER(Event,eventPhase);
ASFUNCTIONBODY_GETTER(Event,bubbles);
ASFUNCTIONBODY_GETTER(Event,cancelable);

ASFUNCTIONBODY(Event,_isDefaultPrevented)
{
	Event* th=static_cast<Event*>(obj);
	return abstract_b(obj->getSystemState(),th->defaultPrevented);
}

ASFUNCTIONBODY(Event,_preventDefault)
{
	Event* th=static_cast<Event*>(obj);
	th->defaultPrevented = true;
	return NULL;
}

ASFUNCTIONBODY(Event,formatToString)
{
	assert_and_throw(argslen>=1);
	Event* th=static_cast<Event*>(obj);
	tiny_string msg = "[";
	msg += args[0]->toString();

	for(unsigned int i=1; i<argslen; i++)
	{
		tiny_string prop(args[i]->toString());
		msg += " ";
		msg += prop;
		msg += "=";

		multiname propName(NULL);
		propName.name_type=multiname::NAME_STRING;
		propName.name_s_id=obj->getSystemState()->getUniqueStringId(prop);
		propName.ns.push_back(nsNameAndKind(obj->getSystemState(),"",NAMESPACE));
		asAtom value=th->getVariableByMultiname(propName);
		if (value.type != T_INVALID)
			msg += value.toString();
	}
	msg += "]";

	return abstract_s(obj->getSystemState(),msg);
}

Event* Event::cloneImpl() const
{
	return Class<Event>::getInstanceS(getSystemState(),type, bubbles, cancelable);
}

ASFUNCTIONBODY(Event,clone)
{
	Event* th=static_cast<Event*>(obj);
	return th->cloneImpl();
}

ASFUNCTIONBODY(Event,stopPropagation)
{
	//Event* th=static_cast<Event*>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Event.stopPropagation not implemented");
	return NULL;
}
ASFUNCTIONBODY(Event,stopImmediatePropagation)
{
	//Event* th=static_cast<Event*>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"Event.stopImmediatePropagation not implemented");
	return NULL;
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
	c->setVariableByQName("CAPTURING_PHASE","",abstract_i(c->getSystemState(),CAPTURING_PHASE),DECLARED_TRAIT);
	c->setVariableByQName("BUBBLING_PHASE","",abstract_i(c->getSystemState(),BUBBLING_PHASE),DECLARED_TRAIT);
	c->setVariableByQName("AT_TARGET","",abstract_i(c->getSystemState(),AT_TARGET),DECLARED_TRAIT);
}

FocusEvent::FocusEvent(Class_base* c):Event(c, "focusEvent")
{
}

void FocusEvent::sinit(Class_base* c)
{	
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableByQName("FOCUS_IN","",abstract_s(c->getSystemState(),"focusIn"),DECLARED_TRAIT);
	c->setVariableByQName("FOCUS_OUT","",abstract_s(c->getSystemState(),"focusOut"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_FOCUS_CHANGE","",abstract_s(c->getSystemState(),"mouseFocusChange"),DECLARED_TRAIT);
	c->setVariableByQName("KEY_FOCUS_CHANGE","",abstract_s(c->getSystemState(),"keyFocusChange"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(FocusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	return NULL;
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

ProgressEvent::ProgressEvent(Class_base* c, uint32_t loaded, uint32_t total):Event(c, "progress",false,false,SUBTYPE_PROGRESSEVENT),bytesLoaded(loaded),bytesTotal(total)
{
}

Event* ProgressEvent::cloneImpl() const
{
	return Class<ProgressEvent>::getInstanceS(getSystemState(),bytesLoaded, bytesTotal);
}

void ProgressEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableByQName("PROGRESS","",abstract_s(c->getSystemState(),"progress"),DECLARED_TRAIT);
	REGISTER_GETTER_SETTER(c,bytesLoaded);
	REGISTER_GETTER_SETTER(c,bytesTotal);
}

ASFUNCTIONBODY_GETTER_SETTER(ProgressEvent,bytesLoaded);
ASFUNCTIONBODY_GETTER_SETTER(ProgressEvent,bytesTotal);

void ProgressEvent::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(ProgressEvent,_constructor)
{
	ProgressEvent* th=static_cast<ProgressEvent*>(obj);
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	if(argslen>=4)
		th->bytesLoaded=args[3]->toInt();
	if(argslen>=5)
		th->bytesTotal=args[4]->toInt();

	return NULL;
}

void TimerEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableByQName("TIMER","",abstract_s(c->getSystemState(),"timer"),DECLARED_TRAIT);
	c->setVariableByQName("TIMER_COMPLETE","",abstract_s(c->getSystemState(),"timerComplete"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("updateAfterEvent","",Class<IFunction>::getFunction(c->getSystemState(),updateAfterEvent),NORMAL_METHOD,true);
}
ASFUNCTIONBODY(TimerEvent,updateAfterEvent)
{
	LOG(LOG_NOT_IMPLEMENTED,"TimerEvent::updateAfterEvent not implemented");
	return NULL;
}

void MouseEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableByQName("CLICK","",abstract_s(c->getSystemState(),"click"),DECLARED_TRAIT);
	c->setVariableByQName("DOUBLE_CLICK","",abstract_s(c->getSystemState(),"doubleClick"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_DOWN","",abstract_s(c->getSystemState(),"mouseDown"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_OUT","",abstract_s(c->getSystemState(),"mouseOut"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_OVER","",abstract_s(c->getSystemState(),"mouseOver"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_UP","",abstract_s(c->getSystemState(),"mouseUp"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_WHEEL","",abstract_s(c->getSystemState(),"mouseWheel"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_MOVE","",abstract_s(c->getSystemState(),"mouseMove"),DECLARED_TRAIT);
	c->setVariableByQName("RIGHT_CLICK","",abstract_s(c->getSystemState(),"rightClick"),DECLARED_TRAIT);
	c->setVariableByQName("ROLL_OVER","",abstract_s(c->getSystemState(),"rollOver"),DECLARED_TRAIT);
	c->setVariableByQName("ROLL_OUT","",abstract_s(c->getSystemState(),"rollOut"),DECLARED_TRAIT);
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

ASFUNCTIONBODY(MouseEvent,_constructor)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj);
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	if(argslen>=4)
		th->localX=args[3]->toNumber();
	if(argslen>=5)
		th->localY=args[4]->toNumber();
	if(argslen>=6)
		th->relatedObject=ArgumentConversion< _NR<InteractiveObject> >::toConcrete(args[5]);
	if(argslen>=7)
		if (ArgumentConversion<bool>::toConcrete(args[6]))
			th->modifiers |= KMOD_CTRL;
	if(argslen>=8)
		if (ArgumentConversion<bool>::toConcrete(args[7]))
			th->modifiers |= KMOD_ALT;
	if(argslen>=9)
		if (ArgumentConversion<bool>::toConcrete(args[8]))
			th->modifiers |= KMOD_SHIFT;
	if(argslen>=10)
		if (ArgumentConversion<bool>::toConcrete(args[9]))
			th->buttonDown = true;
	if(argslen>=11)
		th->delta=args[10]->toInt();
	if(argslen>=12)
		if (ArgumentConversion<bool>::toConcrete(args[11]))
			th->modifiers |= KMOD_GUI;
	if(argslen>=13)
		if (ArgumentConversion<bool>::toConcrete(args[12]))
			th->modifiers |= KMOD_CTRL;
	// TODO: args[13] = clickCount
	if(argslen>=14)
		LOG(LOG_NOT_IMPLEMENTED,"MouseEvent: clickcount");

	return NULL;
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
	MouseEvent* th=static_cast<MouseEvent*>(obj.getObject());
	if(argslen != 1) 
		throw Class<ArgumentError>::getInstanceS(obj.getObject()->getSystemState(),"Wrong number of arguments in setter"); 
	number_t val=args[0].toNumber();
	th->localX = val;
	//Change StageXY if target!=NULL else don't do anything
	//At this point, the target should be an InteractiveObject but check anyway
	if(th->target &&(th->target->getClass()->isSubClass(Class<InteractiveObject>::getClass(obj.getObject()->getSystemState()))))
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>((th->target).getPtr());
		tar->localToGlobal(th->localX, th->localY, th->stageX, th->stageY);
	}
	return asAtom(); 
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_localY)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj.getObject());
	if(argslen != 1) 
		throw Class<ArgumentError>::getInstanceS(obj.getObject()->getSystemState(),"Wrong number of arguments in setter"); 
	number_t val=args[0].toNumber();
	th->localY = val;
	//Change StageXY if target!=NULL else don't do anything	
	//At this point, the target should be an InteractiveObject but check anyway
	if(th->target &&(th->target->getClass()->isSubClass(Class<InteractiveObject>::getClass(obj.getObject()->getSystemState()))))
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>((th->target).getPtr());
		tar->localToGlobal(th->localX, th->localY, th->stageX, th->stageY);
	}
	return asAtom(); 
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_altKey)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj.getObject());
	return asAtom((bool)(th->modifiers & KMOD_ALT));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_altKey)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj.getObject());
	th->modifiers |= KMOD_ALT;
	return asAtom();
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_controlKey)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj.getObject());
	return asAtom((bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_controlKey)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj.getObject());
	th->modifiers |= KMOD_CTRL;
	return asAtom();
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_ctrlKey)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj.getObject());
	return asAtom((bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_ctrlKey)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj.getObject());
	th->modifiers |= KMOD_CTRL;
	return asAtom();
}

ASFUNCTIONBODY_ATOM(MouseEvent,_getter_shiftKey)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj.getObject());
	return asAtom((bool)(th->modifiers & KMOD_SHIFT));
}

ASFUNCTIONBODY_ATOM(MouseEvent,_setter_shiftKey)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj.getObject());
	th->modifiers |= KMOD_SHIFT;
	return asAtom();
}
ASFUNCTIONBODY(MouseEvent,updateAfterEvent)
{
	LOG(LOG_NOT_IMPLEMENTED,"MouseEvent::updateAfterEvent not implemented");
	return NULL;
}

void MouseEvent::buildTraits(ASObject* o)
{
	//TODO: really handle local[XY]
	//o->setVariableByQName("localX","",abstract_d(0),DECLARED_TRAIT);
	//o->setVariableByQName("localY","",abstract_d(0),DECLARED_TRAIT);
}

void MouseEvent::setTarget(_NR<ASObject> t)
{
	target = t;
	//If t is NULL, it means MouseEvent is being reset
	if(!t)
	{
		localX = 0;
		localY = 0;
		stageX = 0;
		stageY = 0;
		relatedObject = NullRef;
	}
	//If t is non null, it should be an InteractiveObject
	else if(t->getClass()->isSubClass(Class<InteractiveObject>::getClass(getSystemState())))	
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>(t.getPtr());
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

ASFUNCTIONBODY(NativeDragEvent,_constructor)
{
	//NativeDragEvent* th=static_cast<NativeDragEvent*>(obj);
	uint32_t baseClassArgs=imin(argslen,6);
	MouseEvent::_constructor(obj,args,baseClassArgs);
	LOG(LOG_NOT_IMPLEMENTED,"NativeDragEvent: constructor");
	return NULL;
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
	c->setVariableByQName("IO_ERROR","",abstract_s(c->getSystemState(),"ioError"),CONSTANT_TRAIT);
	c->setVariableByQName("DISK_ERROR","",abstract_s(c->getSystemState(),"diskError"),CONSTANT_TRAIT);
	c->setVariableByQName("NETWORK_ERROR","",abstract_s(c->getSystemState(),"networkError"),CONSTANT_TRAIT);
	c->setVariableByQName("VERIFY_ERROR","",abstract_s(c->getSystemState(),"verifyError"),CONSTANT_TRAIT);
	c->setVariableByQName("STANDARD_ERROR_IO_ERROR","",abstract_s(c->getSystemState(),"standardErrorIoError"),CONSTANT_TRAIT);
	c->setVariableByQName("STANDARD_INPUT_IO_ERROR","",abstract_s(c->getSystemState(),"standardInputIoError"),CONSTANT_TRAIT);
	c->setVariableByQName("STANDARD_OUTPUT_IO_ERROR","",abstract_s(c->getSystemState(),"standardOutputIoError"),CONSTANT_TRAIT);
}

EventDispatcher::EventDispatcher(Class_base* c):ASObject(c)
{
}

void EventDispatcher::finalize()
{
	ASObject::finalize();
	handlers.clear();
	forcedTarget.reset();
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

ASFUNCTIONBODY(EventDispatcher,addEventListener)
{
	EventDispatcher* th=Class<EventDispatcher>::cast(obj);
	if(args[0]->getObjectType()!=T_STRING || args[1]->getObjectType()!=T_FUNCTION)
		//throw RunTimeException("Type mismatch in EventDispatcher::addEventListener");
		return NULL;

	bool useCapture=false;
	int32_t priority=0;

	if(argslen>=3)
		useCapture=Boolean_concrete(args[2]);
	if(argslen>=4)
		priority=args[3]->toInt();

	const tiny_string& eventName=args[0]->toString();
	IFunction* f=static_cast<IFunction*>(args[1]);

	if(th->is<DisplayObject>() && (eventName=="enterFrame"
				|| eventName=="exitFrame"
				|| eventName=="frameConstructed") )
	{
		th->incRef();
		obj->getSystemState()->registerFrameListener(_MR(th->as<DisplayObject>()));
	}

	{
		Locker l(th->handlersMutex);
		//Search if any listener is already registered for the event
		list<listener>& listeners=th->handlers[eventName];
		f->incRef();
		const listener newListener(_MR(f), priority, useCapture);
		//Ordered insertion
		list<listener>::iterator insertionPoint=upper_bound(listeners.begin(),listeners.end(),newListener);
		listeners.insert(insertionPoint,newListener);
	}
	th->eventListenerAdded(eventName);
	return NULL;
}

ASFUNCTIONBODY(EventDispatcher,_hasEventListener)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj);
	assert_and_throw(argslen==1 && args[0]->getObjectType()==T_STRING);
	const tiny_string& eventName=args[0]->toString();
	bool ret=th->hasEventListener(eventName);
	return abstract_b(obj->getSystemState(),ret);
}

ASFUNCTIONBODY(EventDispatcher,removeEventListener)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj);
	
	if (args[1]->getObjectType() == T_NULL) // it seems that null is allowed as function
		return NULL;
	if(args[0]->getObjectType()!=T_STRING || args[1]->getObjectType()!=T_FUNCTION)
		throw RunTimeException("Type mismatch in EventDispatcher::removeEventListener");

	const tiny_string& eventName=args[0]->toString();

	bool useCapture=false;
	if(argslen>=3)
		useCapture=Boolean_concrete(args[2]);

	{
		Locker l(th->handlersMutex);
		map<tiny_string, list<listener> >::iterator h=th->handlers.find(eventName);
		if(h==th->handlers.end())
		{
			LOG(LOG_CALLS,_("Event not found"));
			return NULL;
		}

		IFunction* f=static_cast<IFunction*>(args[1]);
		std::list<listener>::iterator it=find(h->second.begin(),h->second.end(),
											make_pair(f,useCapture));
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
		obj->getSystemState()->unregisterFrameListener(_MR(th->as<DisplayObject>()));
	}

	return NULL;
}

ASFUNCTIONBODY(EventDispatcher,dispatchEvent)
{
	EventDispatcher* th=Class<EventDispatcher>::cast(obj);
	if(args[0]->getClass()==NULL || !(args[0]->getClass()->isSubClass(Class<Event>::getClass(obj->getSystemState()))))
		return abstract_b(obj->getSystemState(),false);

	args[0]->incRef();
	_R<Event> e=_MR(Class<Event>::cast(args[0]));

	// Must call the AS getter, because the getter may have been
	// overridden
	asAtom target = e->getVariableByMultiname("target", {""});
	if(target.type != T_INVALID && target.type != T_NULL && target.type != T_UNDEFINED)
	{
		//Object must be cloned, cloning is implemented with the clone AS method
		asAtom cloned = e->executeASMethod("clone", {""}, NULL, 0);
		//Clone always exists since it's implemented in Event itself
		if(!cloned.getObject() || !cloned.getObject()->is<Event>())
			return abstract_b(obj->getSystemState(),false);

		ASATOM_INCREF(cloned);
		e = _MR(cloned.getObject()->as<Event>());
	}
	if(!th->forcedTarget.isNull())
		e->setTarget(th->forcedTarget);
	th->incRef();
	ABCVm::publicHandleEvent(_MR(th), e);
	return abstract_b(obj->getSystemState(),true);
}

ASFUNCTIONBODY(EventDispatcher,_constructor)
{
	EventDispatcher* th=Class<EventDispatcher>::cast(obj);
	_NR<ASObject> forcedTarget;
	ARG_UNPACK(forcedTarget, NullRef);
	if(!forcedTarget.isNull())
	{
		if(forcedTarget->getObjectType()==T_NULL || forcedTarget->getObjectType()==T_UNDEFINED)
			forcedTarget=NullRef;
		else if(!forcedTarget->getClass()->isSubClass(InterfaceClass<IEventDispatcher>::getClass(obj->getSystemState())))
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Wrong argument for EventDispatcher");
	}
	th->forcedTarget=forcedTarget;
	return NULL;
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
		incRef();
		//The object needs to be used multiple times
		e->incRef();
		//tmpListener is now also owned by the vector
		tmpListener[i].f->incRef();
		//If the f is a class method, the 'this' is ignored
		asAtom arg0= asAtom::fromObject(e.getPtr());
		asAtom ret=tmpListener[i].f->call(asAtom::fromObject(this),&arg0,1);
		ASATOM_DECREF(ret);
		//And now no more, f can also be deleted
		tmpListener[i].f->decRef();
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
	info->setVariableByQName("level","",abstract_s(c->getSystemState(),level),DECLARED_TRAIT);
	info->setVariableByQName("code","",abstract_s(c->getSystemState(),code),DECLARED_TRAIT);
	setVariableByQName("info","",info, DECLARED_TRAIT);
}

void NetStatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableByQName("NET_STATUS","",abstract_s(c->getSystemState(),"netStatus"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(NetStatusEvent,_constructor)
{
	//Also call the base class constructor, using only the first arguments
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	ASObject* info;
	if(argslen==0)
	{
		//Called from C++ code, info was set in the C++
		//constructor
		return NULL;
	}
	else if(argslen==4)
	{
		//Building from AS code, use the data
		args[3]->incRef();
		info=args[3];
	}
	else
	{
		//Uninitialized info
		info=obj->getSystemState()->getNullRef();
	}
	multiname infoName(NULL);
	infoName.name_type=multiname::NAME_STRING;
	infoName.name_s_id=obj->getSystemState()->getUniqueStringId("info");
	infoName.ns.push_back(nsNameAndKind(obj->getSystemState(),"",NAMESPACE));
	infoName.isAttribute = false;
	obj->setVariableByMultiname(infoName, asAtom::fromObject(info), CONST_NOT_ALLOWED);
	return NULL;
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
	infoName.ns.push_back(nsNameAndKind(getSystemState(),"",NAMESPACE));
	infoName.isAttribute = false;

	asAtom info = const_cast<NetStatusEvent*>(this)->getVariableByMultiname(infoName);
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
	c->setVariableByQName("FULL_SCREEN","",abstract_s(c->getSystemState(),"fullScreen"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(FullScreenEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	return NULL;
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
	c->setVariableByQName("KEY_DOWN","",abstract_s(c->getSystemState(),"keyDown"),DECLARED_TRAIT);
	c->setVariableByQName("KEY_UP","",abstract_s(c->getSystemState(),"keyUp"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(KeyboardEvent,_constructor)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj);

	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);

	if(argslen > 3) {
		th->charCode = args[3]->toUInt();
	}
	if(argslen > 4) {
		th->keyCode = args[4]->toUInt();
	}
	if(argslen > 5) {
		th->keyLocation = args[5]->toUInt();
	}
	if(argslen > 6) {
		if (ArgumentConversion<bool>::toConcrete(args[6]))
			th->modifiers |= KMOD_CTRL;
	}
	if(argslen > 7) {
		if (ArgumentConversion<bool>::toConcrete(args[7]))
			th->modifiers |= KMOD_ALT;
	}
	if(argslen > 8) {
		if (ArgumentConversion<bool>::toConcrete(args[8]))
			th->modifiers |= KMOD_SHIFT;
	}
	if(argslen > 9) {
		if (ArgumentConversion<bool>::toConcrete(args[9]))
			th->modifiers |= KMOD_CTRL;
	}
	// args[10] (commandKeyValue) is only supported on Max OSX
	if(argslen > 10) {
		if (ArgumentConversion<bool>::toConcrete(args[10]))
			LOG(LOG_NOT_IMPLEMENTED,"KeyboardEvent:commandKeyValue");
	}

	return NULL;
}

ASFUNCTIONBODY_GETTER_SETTER(KeyboardEvent, charCode);
ASFUNCTIONBODY_GETTER_SETTER(KeyboardEvent, keyCode);
ASFUNCTIONBODY_GETTER_SETTER(KeyboardEvent, keyLocation);

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_altKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	return asAtom((bool)(th->modifiers & KMOD_ALT));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_altKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	th->modifiers |= KMOD_ALT;
	return asAtom();
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_commandKey)
{
	// Supported only on OSX
	return asAtom(false);
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_commandKey)
{
	// Supported only on OSX
	return asAtom();
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_controlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	return asAtom((bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_controlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	th->modifiers |= KMOD_CTRL;
	return asAtom();
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_ctrlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	return asAtom((bool)(th->modifiers & KMOD_CTRL));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_ctrlKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	th->modifiers |= KMOD_CTRL;
	return asAtom();
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _getter_shiftKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	return asAtom((bool)(th->modifiers & KMOD_SHIFT));
}

ASFUNCTIONBODY_ATOM(KeyboardEvent, _setter_shiftKey)
{
	KeyboardEvent* th=static_cast<KeyboardEvent*>(obj.getObject());
	th->modifiers |= KMOD_SHIFT;
	return asAtom();
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
	c->setVariableByQName("TEXT_INPUT","",abstract_s(c->getSystemState(),"textInput"),DECLARED_TRAIT);
	REGISTER_GETTER_SETTER(c,text);
}

ASFUNCTIONBODY_GETTER_SETTER(TextEvent,text);

ASFUNCTIONBODY(TextEvent,_constructor)
{
	TextEvent* th=static_cast<TextEvent*>(obj);
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	if(argslen>=4)
		th->text=args[3]->toString();
	return NULL;
}

ErrorEvent::ErrorEvent(Class_base* c, const tiny_string& t, const std::string& e, int id): TextEvent(c,t), errorMsg(e),errorID(id)
{
}

void ErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextEvent, _constructor, CLASS_SEALED);
	c->setVariableByQName("ERROR","",abstract_s(c->getSystemState(),"error"),DECLARED_TRAIT);
	REGISTER_GETTER(c,errorID);
}
ASFUNCTIONBODY_GETTER(ErrorEvent,errorID);

Event* ErrorEvent::cloneImpl() const
{
	return Class<ErrorEvent>::getInstanceS(getSystemState(),text, errorMsg,errorID);
}

ASFUNCTIONBODY(ErrorEvent,_constructor)
{
	ErrorEvent* th=static_cast<ErrorEvent*>(obj);
	uint32_t baseClassArgs=imin(argslen,4);
	TextEvent::_constructor(obj,args,baseClassArgs);
	if(argslen>=5)
		th->errorID=args[4]->toInt();
	return NULL;
}

SecurityErrorEvent::SecurityErrorEvent(Class_base* c, const std::string& e):ErrorEvent(c, "securityError",e)
{
}

void SecurityErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableByQName("SECURITY_ERROR","",abstract_s(c->getSystemState(),"securityError"),DECLARED_TRAIT);
}

AsyncErrorEvent::AsyncErrorEvent(Class_base* c):ErrorEvent(c, "asyncError")
{
}

void AsyncErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableByQName("ASYNC_ERROR","",abstract_s(c->getSystemState(),"asyncError"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(AsyncErrorEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,4);
	ErrorEvent::_constructor(obj,args,baseClassArgs);
	return NULL;
}


UncaughtErrorEvent::UncaughtErrorEvent(Class_base* c):ErrorEvent(c, "uncaughtError")
{
}

void UncaughtErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableByQName("UNCAUGHT_ERROR","",abstract_s(c->getSystemState(),"uncaughtError"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(UncaughtErrorEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,4);
	ErrorEvent::_constructor(obj,args,baseClassArgs);
	return NULL;
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
	c->setVariableByQName("HTTP_STATUS","",abstract_s(c->getSystemState(),"httpStatus"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(HTTPStatusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	return NULL;
}

FunctionEvent::FunctionEvent(_R<IFunction> _f, _NR<ASObject> _obj, ASObject** _args, uint32_t _numArgs):
		WaitableEvent("FunctionEvent"),f(_f),obj(_obj),numArgs(_numArgs)
{
	args = new ASObject*[numArgs];
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

ExternalCallEvent::ExternalCallEvent(_R<IFunction> _f, ASObject* const* _args,
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
	c->setVariableByQName("STATUS","",abstract_s(c->getSystemState(),"status"),DECLARED_TRAIT);
}

void DataEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, TextEvent, _constructor, CLASS_SEALED);
	/* TODO: dispatch this event */
	c->setVariableByQName("DATA","",abstract_s(c->getSystemState(),"data"),DECLARED_TRAIT);
	/* TODO: dispatch this event */
	c->setVariableByQName("UPLOAD_COMPLETE_DATA","",abstract_s(c->getSystemState(),"uploadCompleteData"),DECLARED_TRAIT);

	REGISTER_GETTER_SETTER(c, data);
}

ASFUNCTIONBODY_GETTER_SETTER(DataEvent, data);

ASFUNCTIONBODY(DataEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	TextEvent::_constructor(obj,args,baseClassArgs);

	DataEvent* th=static_cast<DataEvent*>(obj);
	if (argslen >= 4)
	{
		th->data = args[3]->toString();
	}

	return NULL;
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
	c->setVariableByQName("INVOKE","",abstract_s(c->getSystemState(),"invoke"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(InvokeEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	return NULL;
}

DRMErrorEvent::DRMErrorEvent(Class_base* c) : ErrorEvent(c, "drmAuthenticate")
{
}

void DRMErrorEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, ErrorEvent, _constructor, CLASS_SEALED);
	c->setVariableByQName("DRM_ERROR","",abstract_s(c->getSystemState(),"drmError"),DECLARED_TRAIT);
	c->setVariableByQName("DRM_LOAD_DEVICEID_ERROR","",abstract_s(c->getSystemState(),"drmLoadDeviceIdError"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(DRMErrorEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	ErrorEvent::_constructor(obj,args,baseClassArgs);
	if(argslen > 3)
		LOG(LOG_NOT_IMPLEMENTED, "DRMErrorEvent constructor doesn't support all parameters");
	return NULL;
}

DRMStatusEvent::DRMStatusEvent(Class_base* c) : Event(c, "drmAuthenticate")
{
}

void DRMStatusEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableByQName("DRM_STATUS","",abstract_s(c->getSystemState(),"drmStatus"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(DRMStatusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	if(argslen > 3)
		LOG(LOG_NOT_IMPLEMENTED, "DRMStatusEvent constructor doesn't support all parameters");
	return NULL;
}

VideoEvent::VideoEvent(Class_base* c)
  : Event(c, "renderState"),status("unavailable")
{
}

void VideoEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableByQName("RENDER_STATE","",abstract_s(c->getSystemState(),"renderState"),CONSTANT_TRAIT);
	c->setVariableByQName("RENDER_STATUS_ACCELERATED","",abstract_s(c->getSystemState(),"accelerated"),CONSTANT_TRAIT);
	c->setVariableByQName("RENDER_STATUS_SOFTWARE","",abstract_s(c->getSystemState(),"software"),CONSTANT_TRAIT);
	c->setVariableByQName("RENDER_STATUS_UNAVAILABLE","",abstract_s(c->getSystemState(),"unavailable"),CONSTANT_TRAIT);
	REGISTER_GETTER(c,status);
}

ASFUNCTIONBODY(VideoEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);

	VideoEvent* th=static_cast<VideoEvent*>(obj);
	if(argslen>=4)
	{
		th->status=args[3]->toString();
	}

	return NULL;
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
	c->setVariableByQName("RENDER_STATE","",abstract_s(c->getSystemState(),"renderState"),CONSTANT_TRAIT);
	REGISTER_GETTER(c,colorSpace);
	REGISTER_GETTER(c,status);
}

ASFUNCTIONBODY(StageVideoEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);

	StageVideoEvent* th=static_cast<StageVideoEvent*>(obj);
	if(argslen>=4)
	{
		th->status=args[3]->toString();
	}
	if(argslen>=5)
	{
		th->colorSpace=args[4]->toString();
	}

	return NULL;
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
	c->setVariableByQName("STAGE_VIDEO_AVAILABILITY","",abstract_s(c->getSystemState(),"stageVideoAvailability"),DECLARED_TRAIT);
	REGISTER_GETTER(c, availability);
}

ASFUNCTIONBODY(StageVideoAvailabilityEvent, _constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);

	StageVideoAvailabilityEvent* th=static_cast<StageVideoAvailabilityEvent*>(obj);
	if(argslen>=4)
	{
		th->availability = args[3]->toString();
	}
	
	return NULL;
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
	c->setVariableByQName("MENU_ITEM_SELECT","",abstract_s(c->getSystemState(),"menuItemSelect"),DECLARED_TRAIT);
	c->setVariableByQName("MENU_SELECT","",abstract_s(c->getSystemState(),"menuSelect"),DECLARED_TRAIT);
}


void TouchEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableByQName("TOUCH_BEGIN","",abstract_s(c->getSystemState(),"touchBegin"),DECLARED_TRAIT);
	c->setVariableByQName("TOUCH_END","",abstract_s(c->getSystemState(),"touchEnd"),DECLARED_TRAIT);
	c->setVariableByQName("TOUCH_MOVE","",abstract_s(c->getSystemState(),"touchMove"),DECLARED_TRAIT);
	c->setVariableByQName("TOUCH_OUT","",abstract_s(c->getSystemState(),"touchOut"),DECLARED_TRAIT);
	c->setVariableByQName("TOUCH_OVER","",abstract_s(c->getSystemState(),"touchOver"),DECLARED_TRAIT);
	c->setVariableByQName("TOUCH_ROLL_OUT","",abstract_s(c->getSystemState(),"touchRollOut"),DECLARED_TRAIT);
	c->setVariableByQName("TOUCH_ROLL_OVER","",abstract_s(c->getSystemState(),"touchRollOver"),DECLARED_TRAIT);
	c->setVariableByQName("TOUCH_TAP","",abstract_s(c->getSystemState(),"touchTap"),DECLARED_TRAIT);
}

void GestureEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, Event, _constructor, CLASS_SEALED);
	c->setVariableByQName("GESTURE_TWO_FINGER_TAP","",abstract_s(c->getSystemState(),"gestureTwoFingerTap"),DECLARED_TRAIT);
}

void PressAndTapGestureEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, GestureEvent, _constructor, CLASS_SEALED);
	c->setVariableByQName("GESTURE_PRESS_AND_TAP","",abstract_s(c->getSystemState(),"gesturePressAndTap"),DECLARED_TRAIT);
}

void TransformGestureEvent::sinit(Class_base* c)
{
	CLASS_SETUP(c, GestureEvent, _constructor, CLASS_SEALED);
	c->setVariableByQName("GESTURE_PAN","",abstract_s(c->getSystemState(),"gesturePan"),DECLARED_TRAIT);
	c->setVariableByQName("GESTURE_ROTATE","",abstract_s(c->getSystemState(),"gestureRotate"),DECLARED_TRAIT);
	c->setVariableByQName("GESTURE_SWIPE","",abstract_s(c->getSystemState(),"gestureSwipe"),DECLARED_TRAIT);
	c->setVariableByQName("GESTURE_ZOOM","",abstract_s(c->getSystemState(),"gestureZoom"),DECLARED_TRAIT);
}

UncaughtErrorEvents::UncaughtErrorEvents(Class_base* c):
	EventDispatcher(c)
{
}

void UncaughtErrorEvents::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
}

ASFUNCTIONBODY(UncaughtErrorEvents, _constructor)
{
	//EventDispatcher::_constructor(obj, NULL, 0);
	//UncaughtErrorEvents* th=Class<UncaughtErrorEvents>::cast(obj);
	LOG(LOG_NOT_IMPLEMENTED,"UncaughtErrorEvents is not implemented");
	return NULL;
}
