/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "abc.h"
#include "flashevents.h"
#include "swf.h"
#include "compat.h"
#include "class.h"
#include "argconv.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.events");

REGISTER_CLASS_NAME(IEventDispatcher);
REGISTER_CLASS_NAME(EventDispatcher);
REGISTER_CLASS_NAME(Event);
REGISTER_CLASS_NAME(EventPhase);
REGISTER_CLASS_NAME(MouseEvent);
REGISTER_CLASS_NAME(TimerEvent);
REGISTER_CLASS_NAME(ProgressEvent);
REGISTER_CLASS_NAME(IOErrorEvent);
REGISTER_CLASS_NAME(FocusEvent);
REGISTER_CLASS_NAME(NetStatusEvent);
REGISTER_CLASS_NAME(HTTPStatusEvent);
REGISTER_CLASS_NAME(FullScreenEvent);
REGISTER_CLASS_NAME(KeyboardEvent);
REGISTER_CLASS_NAME(TextEvent);
REGISTER_CLASS_NAME(ErrorEvent);
REGISTER_CLASS_NAME(SecurityErrorEvent);
REGISTER_CLASS_NAME(AsyncErrorEvent);

void IEventDispatcher::linkTraits(Class_base* c)
{
	lookupAndLink(c,"addEventListener","flash.events:IEventDispatcher");
	lookupAndLink(c,"removeEventListener","flash.events:IEventDispatcher");
	lookupAndLink(c,"dispatchEvent","flash.events:IEventDispatcher");
	lookupAndLink(c,"hasEventListener","flash.events:IEventDispatcher");
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

	c->setVariableByQName("ENTER_FRAME","",Class<ASString>::getInstanceS("enterFrame"),DECLARED_TRAIT);
	c->setVariableByQName("RENDER","",Class<ASString>::getInstanceS("render"),DECLARED_TRAIT);
	c->setVariableByQName("ADDED_TO_STAGE","",Class<ASString>::getInstanceS("addedToStage"),DECLARED_TRAIT);
	c->setVariableByQName("REMOVED_FROM_STAGE","",Class<ASString>::getInstanceS("removedFromStage"),DECLARED_TRAIT);
	c->setVariableByQName("INIT","",Class<ASString>::getInstanceS("init"),DECLARED_TRAIT);
	c->setVariableByQName("OPEN","",Class<ASString>::getInstanceS("open"),DECLARED_TRAIT);
	c->setVariableByQName("CLOSE","",Class<ASString>::getInstanceS("close"),DECLARED_TRAIT);
	c->setVariableByQName("ADDED","",Class<ASString>::getInstanceS("added"),DECLARED_TRAIT);
	c->setVariableByQName("COMPLETE","",Class<ASString>::getInstanceS("complete"),DECLARED_TRAIT);
	c->setVariableByQName("REMOVED","",Class<ASString>::getInstanceS("removed"),DECLARED_TRAIT);
	c->setVariableByQName("UNLOAD","",Class<ASString>::getInstanceS("unload"),DECLARED_TRAIT);
	c->setVariableByQName("ACTIVATE","",Class<ASString>::getInstanceS("activate"),DECLARED_TRAIT);
	c->setVariableByQName("DEACTIVATE","",Class<ASString>::getInstanceS("deactivate"),DECLARED_TRAIT);
	c->setVariableByQName("CHANGE","",Class<ASString>::getInstanceS("change"),DECLARED_TRAIT);
	c->setVariableByQName("RESIZE","",Class<ASString>::getInstanceS("resize"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_LEAVE","",Class<ASString>::getInstanceS("mouseLeave"),DECLARED_TRAIT);
	c->setVariableByQName("SELECT","",Class<ASString>::getInstanceS("select"),DECLARED_TRAIT);
	c->setVariableByQName("FULLSCREEN","",Class<ASString>::getInstanceS("fullScreen"),DECLARED_TRAIT);
	c->setVariableByQName("TAB_CHILDREN_CHANGE","",Class<ASString>::getInstanceS("tabChildrenChange"),DECLARED_TRAIT);
	c->setVariableByQName("TAB_ENABLED_CHANGE","",Class<ASString>::getInstanceS("tabEnabledChange"),DECLARED_TRAIT);
	c->setVariableByQName("TAB_INDEX_CHANGE","",Class<ASString>::getInstanceS("tabIndexChange"),DECLARED_TRAIT);

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
	Event* th=static_cast<Event*>(obj);
	if(argslen>=1)
	{
		assert_and_throw(args[0]->getObjectType()==T_STRING);
		th->type=args[0]->toString();
	}
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

		multiname propName;
		propName.name_type=multiname::NAME_STRING;
		propName.name_s=prop;
		propName.ns.push_back(nsNameAndKind("",PACKAGE_NAMESPACE));
		_NR<ASObject> value=th->getVariableByMultiname(propName);
		if (!value.isNull())
			msg += value->toString();
	}
	msg += "]";

	return Class<ASString>::getInstanceS(msg);
}

ASFUNCTIONBODY(Event,clone)
{
	Event* th=static_cast<Event*>(obj);
	return Class<Event>::getInstanceS(th->type, th->bubbles, th->cancelable);
}

void EventPhase::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("CAPTURING_PHASE","",abstract_i(CAPTURING_PHASE),DECLARED_TRAIT);
	c->setVariableByQName("BUBBLING_PHASE","",abstract_i(BUBBLING_PHASE),DECLARED_TRAIT);
	c->setVariableByQName("AT_TARGET","",abstract_i(AT_TARGET),DECLARED_TRAIT);
}

FocusEvent::FocusEvent():Event("focusEvent")
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
	return NULL;
}

MouseEvent::MouseEvent():Event("mouseEvent"), localX(0), localY(0), stageX(0), stageY(0), relatedObject(NullRef)
{
}

MouseEvent::MouseEvent(const tiny_string& t, number_t lx, number_t ly, bool b, _NR<InteractiveObject> relObj):Event(t,b),
	 localX(lx), localY(ly), stageX(0), stageY(0), relatedObject(relObj)
{
}

ProgressEvent::ProgressEvent():Event("progress",false),bytesLoaded(0),bytesTotal(0)
{
}

ProgressEvent::ProgressEvent(uint32_t loaded, uint32_t total):Event("progress",false),bytesLoaded(loaded),bytesTotal(total)
{
}

void ProgressEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("PROGRESS","",Class<ASString>::getInstanceS("progress"),DECLARED_TRAIT);
	c->setDeclaredMethodByQName("bytesLoaded","",Class<IFunction>::getFunction(_getBytesLoaded),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesTotal","",Class<IFunction>::getFunction(_getBytesTotal),GETTER_METHOD,true);
}

void ProgressEvent::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(ProgressEvent,_constructor)
{
	ProgressEvent* th=static_cast<ProgressEvent*>(obj);
	if(argslen>=4)
		th->bytesLoaded=args[3]->toInt();
	if(argslen>=5)
		th->bytesTotal=args[4]->toInt();

	return NULL;
}

ASFUNCTIONBODY(ProgressEvent,_getBytesLoaded)
{
	ProgressEvent* th=static_cast<ProgressEvent*>(obj);
	return abstract_d(th->bytesLoaded);
}

ASFUNCTIONBODY(ProgressEvent,_getBytesTotal)
{
	ProgressEvent* th=static_cast<ProgressEvent*>(obj);
	return abstract_d(th->bytesTotal);
}

void TimerEvent::sinit(Class_base* c)
{
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("TIMER","",Class<ASString>::getInstanceS("timer"),DECLARED_TRAIT);
	c->setVariableByQName("TIMER_COMPLETE","",Class<ASString>::getInstanceS("timerComplete"),DECLARED_TRAIT);
}

void MouseEvent::sinit(Class_base* c)
{
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("CLICK","",Class<ASString>::getInstanceS("click"),DECLARED_TRAIT);
	c->setVariableByQName("DOUBLE_CLICK","",Class<ASString>::getInstanceS("doubleClick"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_DOWN","",Class<ASString>::getInstanceS("mouseDown"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_OUT","",Class<ASString>::getInstanceS("mouseOut"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_OVER","",Class<ASString>::getInstanceS("mouseOver"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_UP","",Class<ASString>::getInstanceS("mouseUp"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_WHEEL","",Class<ASString>::getInstanceS("mouseWheel"),DECLARED_TRAIT);
	c->setVariableByQName("MOUSE_MOVE","",Class<ASString>::getInstanceS("mouseMove"),DECLARED_TRAIT);
	c->setVariableByQName("ROLL_OVER","",Class<ASString>::getInstanceS("rollOver"),DECLARED_TRAIT);
	c->setVariableByQName("ROLL_OUT","",Class<ASString>::getInstanceS("rollOut"),DECLARED_TRAIT);


	REGISTER_GETTER(c,relatedObject);
	REGISTER_GETTER(c,stageX);
	REGISTER_GETTER(c,stageY);
	REGISTER_GETTER_SETTER(c,localX);
	REGISTER_GETTER_SETTER(c,localY);
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
		throw ArgumentError("Wrong number of arguments in setter"); 
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
		throw ArgumentError("Wrong number of arguments in setter"); 
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

IOErrorEvent::IOErrorEvent() : ErrorEvent("ioError")
{
}

void IOErrorEvent::sinit(Class_base* c)
{
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("IO_ERROR","",Class<ASString>::getInstanceS("ioError"),DECLARED_TRAIT);
}

EventDispatcher::EventDispatcher():handlersMutex("handlersMutex")
{
}

void EventDispatcher::finalize()
{
	ASObject::finalize();
	handlers.clear();
}

void EventDispatcher::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->addImplementedInterface(Class<IEventDispatcher>::getClass());
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
		std::cout << it->first << std::endl;
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
		sys->registerFrameListener(_MR(dispobj));
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
		sys->unregisterFrameListener(_MR(dispobj));
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
		multiname cloneName;
		cloneName.name_type=multiname::NAME_STRING;
		cloneName.name_s="clone";
		cloneName.ns.push_back(nsNameAndKind("",PACKAGE_NAMESPACE));

		_NR<ASObject> clone=e->getVariableByMultiname(cloneName);
		if(!clone.isNull() && clone->getObjectType()==T_FUNCTION)
		{
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
		else
		{
			//TODO: support cloning of actual type
			LOG(LOG_NOT_IMPLEMENTED,"Event cloning not supported!");
			e=_MR(Class<Event>::getInstanceS(e->type,e->bubbles, e->cancelable));
		}
	}
	th->incRef();
	ABCVm::publicHandleEvent(_MR(th), e);
	return abstract_b(true);
}

ASFUNCTIONBODY(EventDispatcher,_constructor)
{
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
	l.unlock();
	//TODO: check, ok we should also bind the level
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

NetStatusEvent::NetStatusEvent(const tiny_string& l, const tiny_string& c):Event("netStatus"),level(l),code(c)
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
	if(th->level!="" && th->code!="")
	{
		//TODO: purge this away
		return NULL;
	}
	assert_and_throw(argslen>=1 && argslen<=4);
	//Also call the base class constructor, using only the first arguments
	uint32_t baseClassArgs=imin(argslen,3);
	Event::_constructor(obj,args,baseClassArgs);
	ASObject* info;
	if(argslen==4)
	{
		//Building from AS code, use the data
		args[3]->incRef();
		info=args[3];
	}
	else
	{
		//Uninitialized info
		info=new Null;
	}
	obj->setVariableByQName("info","",info,DECLARED_TRAIT);
	return NULL;
}

FullScreenEvent::FullScreenEvent():Event("fullScreenEvent")
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
	return NULL;
}

KeyboardEvent::KeyboardEvent():Event("keyboardEvent")
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
	return NULL;
}

TextEvent::TextEvent(const tiny_string& t):Event(t)
{
}

void TextEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("TEXT_INPUT","",Class<ASString>::getInstanceS("textInput"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(TextEvent,_constructor)
{
	Event::_constructor(obj,NULL,0);
	return NULL;
}

ErrorEvent::ErrorEvent(const tiny_string& t, const std::string& e): TextEvent(t), errorMsg(e)
{
}

void ErrorEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<TextEvent>::getRef());

	c->setVariableByQName("ERROR","",Class<ASString>::getInstanceS("error"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(ErrorEvent,_constructor)
{
	TextEvent::_constructor(obj,NULL,0);
	return NULL;
}

SecurityErrorEvent::SecurityErrorEvent(const std::string& e):ErrorEvent("securityError",e)
{
}

void SecurityErrorEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ErrorEvent>::getRef());

	c->setVariableByQName("SECURITY_ERROR","",Class<ASString>::getInstanceS("securityError"),DECLARED_TRAIT);
}

ASFUNCTIONBODY(SecurityErrorEvent,_constructor)
{
	ErrorEvent::_constructor(obj,NULL,0);
	return NULL;
}

AsyncErrorEvent::AsyncErrorEvent():ErrorEvent("asyncError")
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
	ErrorEvent::_constructor(obj,NULL,0);
	return NULL;
}

ABCContextInitEvent::ABCContextInitEvent(ABCContext* c, bool l):Event("ABCContextInitEvent"),context(c),lazy(l)
{
}

ShutdownEvent::ShutdownEvent():Event("shutdownEvent")
{
}

void HTTPStatusEvent::sinit(Class_base* c)
{
//	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<Event>::getRef());

	c->setVariableByQName("HTTP_STATUS","",Class<ASString>::getInstanceS("httpStatus"),DECLARED_TRAIT);
}

FunctionEvent::FunctionEvent(_R<IFunction> _f, _NR<ASObject> _obj, ASObject** _args, uint32_t _numArgs, 
		ASObject** _result, ASObject** _exception, _NR<SynchronizationEvent> _sync):
		Event("FunctionEvent"),f(_f),obj(_obj),numArgs(_numArgs),
		result(_result),exception(_exception),sync(_sync)
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
	delete[] args;
}

BindClassEvent::BindClassEvent(_R<RootMovieClip> b, const tiny_string& c)
	: Event("bindClass"),base(b),class_name(c)
{
}
BindClassEvent::BindClassEvent(_R<DictionaryTag> t, const tiny_string& c)
	: Event("bindClass"),tag(t),class_name(c)
{
}
