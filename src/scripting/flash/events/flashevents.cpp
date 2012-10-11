/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

Event::Event(Class_base* cb, const tiny_string& t, bool b, bool c):
	ASObject(cb),bubbles(b),cancelable(c),defaultPrevented(false),eventPhase(0),type(t),target(),currentTarget()
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());

	c->setVariableByQName("ACTIVATE","",Class<ASString>::getInstanceS("activate"),DECLARED_TRAIT);
	c->setVariableByQName("ADDED","",Class<ASString>::getInstanceS("added"),DECLARED_TRAIT);
	c->setVariableByQName("ADDED_TO_STAGE","",Class<ASString>::getInstanceS("addedToStage"),DECLARED_TRAIT);
	c->setVariableByQName("CANCEL","",Class<ASString>::getInstanceS("cancel"),DECLARED_TRAIT);
	c->setVariableByQName("CHANGE","",Class<ASString>::getInstanceS("change"),DECLARED_TRAIT);
	c->setVariableByQName("CLEAR","",Class<ASString>::getInstanceS("clear"),DECLARED_TRAIT);
	c->setVariableByQName("CLOSE","",Class<ASString>::getInstanceS("close"),DECLARED_TRAIT);
	c->setVariableByQName("CLOSING","",Class<ASString>::getInstanceS("closing"),DECLARED_TRAIT);
	c->setVariableByQName("COMPLETE","",Class<ASString>::getInstanceS("complete"),DECLARED_TRAIT);
	c->setVariableByQName("CONNECT","",Class<ASString>::getInstanceS("connect"),DECLARED_TRAIT);
	c->setVariableByQName("CONTEXT3D_CREATE","",Class<ASString>::getInstanceS("context3DCreate"),DECLARED_TRAIT);
	c->setVariableByQName("COPY","",Class<ASString>::getInstanceS("copy"),DECLARED_TRAIT);
	c->setVariableByQName("CUT","",Class<ASString>::getInstanceS("cut"),DECLARED_TRAIT);
	c->setVariableByQName("DEACTIVATE","",Class<ASString>::getInstanceS("deactivate"),DECLARED_TRAIT);
	c->setVariableByQName("DISPLAYING","",Class<ASString>::getInstanceS("displaying"),DECLARED_TRAIT);
	c->setVariableByQName("ENTER_FRAME","",Class<ASString>::getInstanceS("enterFrame"),DECLARED_TRAIT);
	c->setVariableByQName("EXIT_FRAME","",Class<ASString>::getInstanceS("exitFrame"),DECLARED_TRAIT);
	c->setVariableByQName("EXITING","",Class<ASString>::getInstanceS("exiting"),DECLARED_TRAIT);
	c->setVariableByQName("FRAME_CONSTRUCTED","",Class<ASString>::getInstanceS("frameConstructed"),DECLARED_TRAIT);
	c->setVariableByQName("FULLSCREEN","",Class<ASString>::getInstanceS("fullScreen"),DECLARED_TRAIT);
	c->setVariableByQName("HTML_BOUNDS_CHANGE","",Class<ASString>::getInstanceS("htmlBoundsChange"),DECLARED_TRAIT);
	c->setVariableByQName("HTML_DOM_INITIALIZE","",Class<ASString>::getInstanceS("htmlDOMInitialize"),DECLARED_TRAIT);
	c->setVariableByQName("HTML_RENDER","",Class<ASString>::getInstanceS("htmlRender"),DECLARED_TRAIT);
	c->setVariableByQName("ID3","",Class<ASString>::getInstanceS("id3"),DECLARED_TRAIT);
	c->setVariableByQName("INIT","",Class<ASString>::getInstanceS("init"),DECLARED_TRAIT);
	c->setVariableByQName("LOCATION_CHANGE","",Class<ASString>::getInstanceS("locationChange"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_LEAVE","",Class<ASString>::getInstanceS("mouseLeave"),DECLARED_TRAIT);
	c->setVariableByQName("NETWORK_CHANGE","",Class<ASString>::getInstanceS("networkChange"),DECLARED_TRAIT);
	c->setVariableByQName("OPEN","",Class<ASString>::getInstanceS("open"),DECLARED_TRAIT);
	c->setVariableByQName("PASTE","",Class<ASString>::getInstanceS("paste"),DECLARED_TRAIT);
	c->setVariableByQName("REMOVED","",Class<ASString>::getInstanceS("removed"),DECLARED_TRAIT);
	c->setVariableByQName("REMOVED_FROM_STAGE","",Class<ASString>::getInstanceS("removedFromStage"),DECLARED_TRAIT);
	c->setVariableByQName("RENDER","",Class<ASString>::getInstanceS("render"),DECLARED_TRAIT);
	c->setVariableByQName("RESIZE","",Class<ASString>::getInstanceS("resize"),DECLARED_TRAIT);
	c->setVariableByQName("SCROLL","",Class<ASString>::getInstanceS("scroll"),DECLARED_TRAIT);
	c->setVariableByQName("SELECT","",Class<ASString>::getInstanceS("select"),DECLARED_TRAIT);
	c->setVariableByQName("SELECT_ALL","",Class<ASString>::getInstanceS("selectAll"),DECLARED_TRAIT);
	c->setVariableByQName("SOUND_COMPLETE","",Class<ASString>::getInstanceS("soundComplete"),DECLARED_TRAIT);
	c->setVariableByQName("STANDARD_ERROR_CLOSE","",Class<ASString>::getInstanceS("standardErrorClose"),DECLARED_TRAIT);
	c->setVariableByQName("STANDARD_INPUT_CLOSE","",Class<ASString>::getInstanceS("standardInputClose"),DECLARED_TRAIT);
	c->setVariableByQName("STANDARD_OUTPUT_CLOSE","",Class<ASString>::getInstanceS("standardOutputClose"),DECLARED_TRAIT);
	c->setVariableByQName("TAB_CHILDREN_CHANGE","",Class<ASString>::getInstanceS("tabChildrenChange"),DECLARED_TRAIT);
	c->setVariableByQName("TAB_ENABLED_CHANGE","",Class<ASString>::getInstanceS("tabEnabledChange"),DECLARED_TRAIT);
	c->setVariableByQName("TAB_INDEX_CHANGE","",Class<ASString>::getInstanceS("tabIndexChange"),DECLARED_TRAIT);
	c->setVariableByQName("TEXT_INTERACTION_MODE_CHANGE","",Class<ASString>::getInstanceS("textInteractionModeChange"),DECLARED_TRAIT);
	c->setVariableByQName("TEXTURE_READY","",Class<ASString>::getInstanceS("textureReady"),DECLARED_TRAIT);
	c->setVariableByQName("UNLOAD","",Class<ASString>::getInstanceS("unload"),DECLARED_TRAIT);
	c->setVariableByQName("USER_IDLE","",Class<ASString>::getInstanceS("userIdle"),DECLARED_TRAIT);
	c->setVariableByQName("USER_PRESENT","",Class<ASString>::getInstanceS("userPresent"),DECLARED_TRAIT);

	c->setDeclaredMethodByQName("formatToString","",Class<IFunction>::getFunction(formatToString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isDefaultPrevented","",Class<IFunction>::getFunction(_isDefaultPrevented),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("preventDefault","",Class<IFunction>::getFunction(_preventDefault),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("clone","",Class<IFunction>::getFunction(clone),NORMAL_METHOD,true);
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
	return abstract_b(th->defaultPrevented);
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
		propName.name_s_id=getSys()->getUniqueStringId(prop);
		propName.ns.push_back(nsNameAndKind("",NAMESPACE));
		_NR<ASObject> value=th->getVariableByMultiname(propName);
		if (!value.isNull())
			msg += value->toString();
	}
	msg += "]";

	return Class<ASString>::getInstanceS(msg);
}

Event* Event::cloneImpl() const
{
	return Class<Event>::getInstanceS(type, bubbles, cancelable);
}

ASFUNCTIONBODY(Event,clone)
{
	Event* th=static_cast<Event*>(obj);
	return th->cloneImpl();
}

void EventPhase::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("CAPTURING_PHASE","",abstract_i(CAPTURING_PHASE),DECLARED_TRAIT);
	c->setVariableByQName("BUBBLING_PHASE","",abstract_i(BUBBLING_PHASE),DECLARED_TRAIT);
	c->setVariableByQName("AT_TARGET","",abstract_i(AT_TARGET),DECLARED_TRAIT);
}

FocusEvent::FocusEvent(Class_base* c):Event(c, "focusEvent")
{
}

void FocusEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("FOCUS_IN","",Class<ASString>::getInstanceS("focusIn"),DECLARED_TRAIT);
	c->setVariableByQName("FOCUS_OUT","",Class<ASString>::getInstanceS("focusOut"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_FOCUS_CHANGE","",Class<ASString>::getInstanceS("mouseFocusChange"),DECLARED_TRAIT);
	c->setVariableByQName("KEY_FOCUS_CHANGE","",Class<ASString>::getInstanceS("keyFocusChange"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(FocusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	return NULL;
}

MouseEvent::MouseEvent(Class_base* c):Event(c, "mouseEvent"), localX(0), localY(0), stageX(0), stageY(0), relatedObject(NullRef)
{
}

MouseEvent::MouseEvent(Class_base* c, const tiny_string& t, number_t lx, number_t ly, bool b, _NR<InteractiveObject> relObj):Event(c,t,b),
	 localX(lx), localY(ly), stageX(0), stageY(0), relatedObject(relObj)
{
}

Event* MouseEvent::cloneImpl() const
{
	return Class<MouseEvent>::getInstanceS(type,localX,localY,bubbles,relatedObject);
}

ProgressEvent::ProgressEvent(Class_base* c):Event(c, "progress",false),bytesLoaded(0),bytesTotal(0)
{
}

ProgressEvent::ProgressEvent(Class_base* c, uint32_t loaded, uint32_t total):Event(c, "progress",false),bytesLoaded(loaded),bytesTotal(total)
{
}

Event* ProgressEvent::cloneImpl() const
{
	return Class<ProgressEvent>::getInstanceS(bytesLoaded, bytesTotal);
}

void ProgressEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("PROGRESS","",Class<ASString>::getInstanceS("progress"),DECLARED_TRAIT);
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("TIMER","",Class<ASString>::getInstanceS("timer"),DECLARED_TRAIT);
	c->setVariableByQName("TIMER_COMPLETE","",Class<ASString>::getInstanceS("timerComplete"),DECLARED_TRAIT);
}

void MouseEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("CLICK","",Class<ASString>::getInstanceS("click"),DECLARED_TRAIT);
	c->setVariableByQName("DOUBLE_CLICK","",Class<ASString>::getInstanceS("doubleClick"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_DOWN","",Class<ASString>::getInstanceS("mouseDown"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_OUT","",Class<ASString>::getInstanceS("mouseOut"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_OVER","",Class<ASString>::getInstanceS("mouseOver"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_UP","",Class<ASString>::getInstanceS("mouseUp"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_WHEEL","",Class<ASString>::getInstanceS("mouseWheel"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_MOVE","",Class<ASString>::getInstanceS("mouseMove"),DECLARED_TRAIT);
	c->setVariableByQName("RIGHT_CLICK","",Class<ASString>::getInstanceS("rightClick"),DECLARED_TRAIT);
	c->setVariableByQName("ROLL_OVER","",Class<ASString>::getInstanceS("rollOver"),DECLARED_TRAIT);
	c->setVariableByQName("ROLL_OUT","",Class<ASString>::getInstanceS("rollOut"),DECLARED_TRAIT);


	REGISTER_GETTER(c,relatedObject);
	REGISTER_GETTER(c,stageX);
	REGISTER_GETTER(c,stageY);
	REGISTER_GETTER_SETTER(c,localX);
	REGISTER_GETTER_SETTER(c,localY);
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
	return NULL;
}

ASFUNCTIONBODY_GETTER(MouseEvent,relatedObject);
ASFUNCTIONBODY_GETTER(MouseEvent,localX);
ASFUNCTIONBODY_GETTER(MouseEvent,localY);
ASFUNCTIONBODY_GETTER(MouseEvent,stageX);
ASFUNCTIONBODY_GETTER(MouseEvent,stageY);

ASFUNCTIONBODY(MouseEvent,_setter_localX)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj);
	if(argslen != 1) 
		throw Class<ArgumentError>::getInstanceS("Wrong number of arguments in setter"); 
	number_t val=args[0]->toNumber();
	th->localX = val;
	//Change StageXY if target!=NULL else don't do anything
	//At this point, the target should be an InteractiveObject but check anyway
	if(th->target &&(th->target->getClass()->isSubClass(Class<InteractiveObject>::getClass())))
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>((th->target).getPtr());
		tar->localToGlobal(th->localX, th->localY, th->stageX, th->stageY);
	}
	return NULL; 
}

ASFUNCTIONBODY(MouseEvent,_setter_localY)
{
	MouseEvent* th=static_cast<MouseEvent*>(obj);
	if(argslen != 1) 
		throw Class<ArgumentError>::getInstanceS("Wrong number of arguments in setter"); 
	number_t val=args[0]->toNumber();
	th->localY = val;
	//Change StageXY if target!=NULL else don't do anything	
	//At this point, the target should be an InteractiveObject but check anyway
	if(th->target &&(th->target->getClass()->isSubClass(Class<InteractiveObject>::getClass())))
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>((th->target).getPtr());
		tar->localToGlobal(th->localX, th->localY, th->stageX, th->stageY);
	}
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
	else if(t->getClass()->isSubClass(Class<InteractiveObject>::getClass()))	
	{		
		InteractiveObject* tar = static_cast<InteractiveObject*>(t.getPtr());
		tar->localToGlobal(localX, localY, stageX, stageY);
	}
}

IOErrorEvent::IOErrorEvent(Class_base* c) : ErrorEvent(c, "ioError")
{
}

void IOErrorEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ErrorEvent>::getRef());

	c->setVariableByQName("IO_ERROR","",Class<ASString>::getInstanceS("ioError"),DECLARED_TRAIT);
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->addImplementedInterface(InterfaceClass<IEventDispatcher>::getClass());
	c->setSuper(Class<ASObject>::getRef());

	c->setDeclaredMethodByQName("addEventListener","",Class<IFunction>::getFunction(addEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasEventListener","",Class<IFunction>::getFunction(_hasEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeEventListener","",Class<IFunction>::getFunction(removeEventListener),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("dispatchEvent","",Class<IFunction>::getFunction(dispatchEvent),NORMAL_METHOD,true);

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

	DisplayObject* dispobj=dynamic_cast<DisplayObject*>(th);
	if(dispobj && (eventName=="enterFrame"
				|| eventName=="exitFrame"
				|| eventName=="frameConstructed") )
	{
		dispobj->incRef();
		getSys()->registerFrameListener(_MR(dispobj));
	}

	{
		Locker l(th->handlersMutex);
		//Search if any listener is already registered for the event
		list<listener>& listeners=th->handlers[eventName];
		if(find(listeners.begin(),listeners.end(),make_pair(f,useCapture))!=listeners.end())
		{
			LOG(LOG_CALLS,_("Weird event reregistration"));
			return NULL;
		}
		f->incRef();
		const listener newListener(_MR(f), priority, useCapture);
		//Ordered insertion
		list<listener>::iterator insertionPoint=upper_bound(listeners.begin(),listeners.end(),newListener);
		listeners.insert(insertionPoint,newListener);
	}

	return NULL;
}

ASFUNCTIONBODY(EventDispatcher,_hasEventListener)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj);
	assert_and_throw(argslen==1 && args[0]->getObjectType()==T_STRING);
	const tiny_string& eventName=args[0]->toString();
	bool ret=th->hasEventListener(eventName);
	return abstract_b(ret);
}

ASFUNCTIONBODY(EventDispatcher,removeEventListener)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj);
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
	DisplayObject* dispobj=dynamic_cast<DisplayObject*>(th);
	if(dispobj && (eventName=="enterFrame"
					|| eventName=="exitFrame"
					|| eventName=="frameConstructed")
				&& (!th->hasEventListener("enterFrame")
					&& !th->hasEventListener("exitFrame")
					&& !th->hasEventListener("frameConstructed")) )
	{
		dispobj->incRef();
		getSys()->unregisterFrameListener(_MR(dispobj));
	}

	return NULL;
}

ASFUNCTIONBODY(EventDispatcher,dispatchEvent)
{
	EventDispatcher* th=Class<EventDispatcher>::cast(obj);
	if(args[0]->getClass()==NULL || !(args[0]->getClass()->isSubClass(Class<Event>::getClass())))
		return abstract_b(false);

	args[0]->incRef();
	_R<Event> e=_MR(Class<Event>::cast(args[0]));
	assert_and_throw(e->type!="");
	if(!e->target.isNull())
	{
		//Object must be cloned, closing is implemented with the clone AS method
		multiname cloneName(NULL);
		cloneName.name_type=multiname::NAME_STRING;
		cloneName.name_s_id=getSys()->getUniqueStringId("clone");
		cloneName.ns.push_back(nsNameAndKind("",NAMESPACE));

		_NR<ASObject> clone=e->getVariableByMultiname(cloneName);
		//Clone always exists since it's implemented in Event itself
		assert(!clone.isNull());
		IFunction* f = static_cast<IFunction*>(clone.getPtr());
		e->incRef();
		ASObject* funcRet=f->call(e.getPtr(),NULL,0);
		//Verify that the returned object is actually an event
		Event* newEvent=dynamic_cast<Event*>(funcRet);
		if(newEvent==NULL)
		{
			if(funcRet)
				funcRet->decRef();
			return abstract_b(false);
		}
		e=_MR(newEvent);
	}
	if(!th->forcedTarget.isNull())
		e->setTarget(th->forcedTarget);
	th->incRef();
	ABCVm::publicHandleEvent(_MR(th), e);
	return abstract_b(true);
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
		else if(!forcedTarget->getClass()->isSubClass(InterfaceClass<IEventDispatcher>::getClass()))
			throw Class<ArgumentError>::getInstanceS("Wrong argument for EventDispatcher");
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
		ASObject* const arg0=e.getPtr();
		ASObject* ret=tmpListener[i].f->call(this,&arg0,1);
		if(ret)
			ret->decRef();
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

NetStatusEvent::NetStatusEvent(Class_base* cb, const tiny_string& level, const tiny_string& code):Event(cb, "netStatus")
{
	//The object has been initialized internally
	ASObject* info=Class<ASObject>::getInstanceS();
	info->setVariableByQName("level","",Class<ASString>::getInstanceS(level),DECLARED_TRAIT);
	info->setVariableByQName("code","",Class<ASString>::getInstanceS(code),DECLARED_TRAIT);
	setVariableByQName("info","",info, DECLARED_TRAIT);
}

void NetStatusEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("NET_STATUS","",Class<ASString>::getInstanceS("netStatus"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(NetStatusEvent,_constructor)
{
	NetStatusEvent* th=Class<NetStatusEvent>::cast(obj);
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
		info=getSys()->getNullRef();
	}
	obj->setVariableByQName("info","",info,DECLARED_TRAIT);
	return NULL;
}

Event* NetStatusEvent::cloneImpl() const
{
	NetStatusEvent *clone=Class<NetStatusEvent>::getInstanceS();
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;

	multiname infoName(NULL);
	infoName.name_type=multiname::NAME_STRING;
	infoName.name_s_id=getSys()->getUniqueStringId("info");
	infoName.ns.push_back(nsNameAndKind("",NAMESPACE));
	infoName.isAttribute = false;

	_NR<ASObject> info = const_cast<NetStatusEvent*>(this)->getVariableByMultiname(infoName);
	assert(!info.isNull());
	info->incRef();
	clone->setVariableByMultiname(infoName, info.getPtr(), CONST_NOT_ALLOWED);

	return clone;
}

FullScreenEvent::FullScreenEvent(Class_base* c):Event(c, "fullScreenEvent")
{
}

void FullScreenEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("FULL_SCREEN","",Class<ASString>::getInstanceS("fullScreen"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(FullScreenEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	return NULL;
}

KeyboardEvent::KeyboardEvent(Class_base* c):Event(c, "keyboardEvent")
{
}

void KeyboardEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("KEY_DOWN","",Class<ASString>::getInstanceS("keyDown"),DECLARED_TRAIT);
	c->setVariableByQName("KEY_UP","",Class<ASString>::getInstanceS("keyUp"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(KeyboardEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	return NULL;
}

TextEvent::TextEvent(Class_base* c,const tiny_string& t):Event(c,t)
{
}

void TextEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("TEXT_INPUT","",Class<ASString>::getInstanceS("textInput"),DECLARED_TRAIT);

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

ErrorEvent::ErrorEvent(Class_base* c, const tiny_string& t, const std::string& e): TextEvent(c,t), errorMsg(e)
{
}

void ErrorEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<TextEvent>::getRef());

	c->setVariableByQName("ERROR","",Class<ASString>::getInstanceS("error"),DECLARED_TRAIT);
}

Event* ErrorEvent::cloneImpl() const
{
	return Class<ErrorEvent>::getInstanceS(text, errorMsg);
}

ASFUNCTIONBODY(ErrorEvent,_constructor)
{
	TextEvent::_constructor(obj,args,argslen);
	return NULL;
}

SecurityErrorEvent::SecurityErrorEvent(Class_base* c, const std::string& e):ErrorEvent(c, "securityError",e)
{
}

void SecurityErrorEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ErrorEvent>::getRef());

	c->setVariableByQName("SECURITY_ERROR","",Class<ASString>::getInstanceS("securityError"),DECLARED_TRAIT);
}

AsyncErrorEvent::AsyncErrorEvent(Class_base* c):ErrorEvent(c, "asyncError")
{
}

void AsyncErrorEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ErrorEvent>::getRef());

	c->setVariableByQName("ASYNC_ERROR","",Class<ASString>::getInstanceS("asyncError"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(AsyncErrorEvent,_constructor)
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("HTTP_STATUS","",Class<ASString>::getInstanceS("httpStatus"),DECLARED_TRAIT);
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	/* TODO: dispatch this event */
	c->setVariableByQName("STATUS","",Class<ASString>::getInstanceS("status"),DECLARED_TRAIT);
}

void DataEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<TextEvent>::getRef());

	/* TODO: dispatch this event */
	c->setVariableByQName("DATA","",Class<ASString>::getInstanceS("data"),DECLARED_TRAIT);
	/* TODO: dispatch this event */
	c->setVariableByQName("UPLOAD_COMPLETE_DATA","",Class<ASString>::getInstanceS("uploadCompleteData"),DECLARED_TRAIT);

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
	DataEvent *clone = Class<DataEvent>::getInstanceS();
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("INVOKE","",Class<ASString>::getInstanceS("invoke"),DECLARED_TRAIT);
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ErrorEvent>::getRef());

	c->setVariableByQName("DRM_ERROR","",Class<ASString>::getInstanceS("drmError"),DECLARED_TRAIT);
	c->setVariableByQName("DRM_LOAD_DEVICEID_ERROR","",Class<ASString>::getInstanceS("drmLoadDeviceIdError"),DECLARED_TRAIT);
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
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("DRM_STATUS","",Class<ASString>::getInstanceS("drmStatus"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(DRMStatusEvent,_constructor)
{
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	if(argslen > 3)
		LOG(LOG_NOT_IMPLEMENTED, "DRMStatusEvent constructor doesn't support all parameters");
	return NULL;
}

StageVideoEvent::StageVideoEvent(Class_base* c)
  : Event(c, "renderState")
{
}

void StageVideoEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());
	
	c->setVariableByQName("RENDER_STATE","",Class<ASString>::getInstanceS("renderState"),DECLARED_TRAIT);

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
	clone = Class<StageVideoEvent>::getInstanceS();
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
	c->setVariableByQName("STAGE_VIDEO_AVAILABILITY","",Class<ASString>::getInstanceS("stageVideoAvailability"),DECLARED_TRAIT);
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
	clone = Class<StageVideoAvailabilityEvent>::getInstanceS();
	clone->availability = availability;
	// Event
	clone->type = type;
	clone->bubbles = bubbles;
	clone->cancelable = cancelable;
	return clone;
}

ASFUNCTIONBODY_GETTER(StageVideoAvailabilityEvent,availability);
