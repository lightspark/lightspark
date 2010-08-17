/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;

SET_NAMESPACE("flash.events");

REGISTER_CLASS_NAME(IEventDispatcher);
REGISTER_CLASS_NAME(EventDispatcher);
REGISTER_CLASS_NAME(Event);
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

void IEventDispatcher::linkTraits(ASObject* o)
{
	lookupAndLink(o,"addEventListener","flash.events:IEventDispatcher");
	lookupAndLink(o,"removeEventListener","flash.events:IEventDispatcher");
	lookupAndLink(o,"dispatchEvent","flash.events:IEventDispatcher");
	lookupAndLink(o,"hasEventListener","flash.events:IEventDispatcher");
}

Event::Event(const tiny_string& t, bool b):type(t),target(NULL),currentTarget(NULL),bubbles(b)
{
}

Event::~Event()
{
//	cout << "Destroying event type " << type << " this " << this << endl;
}

void Event::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("ENTER_FRAME","",Class<ASString>::getInstanceS("enterFrame"));
	c->setVariableByQName("RENDER","",Class<ASString>::getInstanceS("render"));
	c->setVariableByQName("ADDED_TO_STAGE","",Class<ASString>::getInstanceS("addedToStage"));
	c->setVariableByQName("REMOVED_FROM_STAGE","",Class<ASString>::getInstanceS("removedFromStage"));
	c->setVariableByQName("INIT","",Class<ASString>::getInstanceS("init"));
	c->setVariableByQName("CLOSE","",Class<ASString>::getInstanceS("close"));
	c->setVariableByQName("ADDED","",Class<ASString>::getInstanceS("added"));
	c->setVariableByQName("COMPLETE","",Class<ASString>::getInstanceS("complete"));
	c->setVariableByQName("REMOVED","",Class<ASString>::getInstanceS("removed"));
	c->setVariableByQName("UNLOAD","",Class<ASString>::getInstanceS("unload"));
	c->setVariableByQName("ACTIVATE","",Class<ASString>::getInstanceS("activate"));
	c->setVariableByQName("DEACTIVATE","",Class<ASString>::getInstanceS("deactivate"));
	c->setVariableByQName("CHANGE","",Class<ASString>::getInstanceS("change"));
	c->setVariableByQName("RESIZE","",Class<ASString>::getInstanceS("resize"));
	c->setVariableByQName("MOUSE_LEAVE","",Class<ASString>::getInstanceS("mouseLeave"));
	c->setVariableByQName("SELECT","",Class<ASString>::getInstanceS("select"));
	c->setVariableByQName("TAB_CHILDREN_CHANGE","",Class<ASString>::getInstanceS("tabChildrenChange"));
	c->setVariableByQName("TAB_ENABLED_CHANGE","",Class<ASString>::getInstanceS("tabEnabledChange"));
	c->setVariableByQName("TAB_INDEX_CHANGE","",Class<ASString>::getInstanceS("tabIndexChange"));

	c->setGetterByQName("target","",Class<IFunction>::getFunction(_getTarget));
	c->setGetterByQName("type","",Class<IFunction>::getFunction(_getType));
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

ASFUNCTIONBODY(Event,_getTarget)
{
	Event* th=static_cast<Event*>(obj);
	if(th->target)
	{
		th->target->incRef();
		return th->target;
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"Target for event " << th->type);
		return new Undefined;
	}
}

ASFUNCTIONBODY(Event,_getType)
{
	Event* th=static_cast<Event*>(obj);
	return Class<ASString>::getInstanceS(th->type);
}

FocusEvent::FocusEvent():Event("focusEvent")
{
}

void FocusEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("FOCUS_IN","",Class<ASString>::getInstanceS("focusIn"));
	c->setVariableByQName("FOCUS_OUT","",Class<ASString>::getInstanceS("focusOut"));
	c->setVariableByQName("MOUSE_FOCUS_CHANGE","",Class<ASString>::getInstanceS("mouseFocusChange"));
	c->setVariableByQName("KEY_FOCUS_CHANGE","",Class<ASString>::getInstanceS("keyFocusChange"));
}

ASFUNCTIONBODY(FocusEvent,_constructor)
{
	return NULL;
}

MouseEvent::MouseEvent():Event("mouseEvent")
{
}

MouseEvent::MouseEvent(const tiny_string& t, bool b):Event(t,b)
{
}

ProgressEvent::ProgressEvent():Event("progress"),bytesLoaded(0),bytesTotal(0)
{
}

void ProgressEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("PROGRESS","",Class<ASString>::getInstanceS("progress"));

	c->setGetterByQName("bytesLoaded","",Class<IFunction>::getFunction(_getBytesLoaded));
	c->setGetterByQName("bytesTotal","",Class<IFunction>::getFunction(_getBytesTotal));
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
	c->setVariableByQName("TIMER","",Class<ASString>::getInstanceS("timer"));
	c->setVariableByQName("TIMER_COMPLETE","",Class<ASString>::getInstanceS("timerComplete"));
}

void MouseEvent::sinit(Class_base* c)
{
//	assert(c->constructor==NULL);
//	c->constructor=Class<IFunction>::getFunction(_constructor);
	c->super=Class<Event>::getClass();
	c->max_level=c->super->max_level+1;
	c->setVariableByQName("CLICK","",Class<ASString>::getInstanceS("click"));
	c->setVariableByQName("DOUBLE_CLICK","",Class<ASString>::getInstanceS("doubleClick"));
	c->setVariableByQName("MOUSE_DOWN","",Class<ASString>::getInstanceS("mouseDown"));
	c->setVariableByQName("MOUSE_OUT","",Class<ASString>::getInstanceS("mouseOut"));
	c->setVariableByQName("MOUSE_OVER","",Class<ASString>::getInstanceS("mouseOver"));
	c->setVariableByQName("MOUSE_UP","",Class<ASString>::getInstanceS("mouseUp"));
	c->setVariableByQName("MOUSE_WHEEL","",Class<ASString>::getInstanceS("mouseWheel"));
	c->setVariableByQName("MOUSE_MOVE","",Class<ASString>::getInstanceS("mouseMove"));
	c->setVariableByQName("ROLL_OVER","",Class<ASString>::getInstanceS("rollOver"));
	c->setVariableByQName("ROLL_OUT","",Class<ASString>::getInstanceS("rollOut"));
}

void MouseEvent::buildTraits(ASObject* o)
{
	//TODO: really handle local[XY]
	o->setVariableByQName("localX","",abstract_d(0));
	o->setVariableByQName("localY","",abstract_d(0));
}

IOErrorEvent::IOErrorEvent()
{
}

void IOErrorEvent::sinit(Class_base* c)
{
	c->setVariableByQName("IO_ERROR","",Class<ASString>::getInstanceS("ioError"));
}

EventDispatcher::EventDispatcher():handlersMutex("handlersMutex")
{
}

void EventDispatcher::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->addImplementedInterface(Class<IEventDispatcher>::getClass());
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;

	c->setVariableByQName("addEventListener","",Class<IFunction>::getFunction(addEventListener));
	c->setVariableByQName("hasEventListener","",Class<IFunction>::getFunction(_hasEventListener));
	c->setVariableByQName("removeEventListener","",Class<IFunction>::getFunction(removeEventListener));
	c->setVariableByQName("dispatchEvent","",Class<IFunction>::getFunction(dispatchEvent));

	IEventDispatcher::linkTraits(c);
}

void EventDispatcher::buildTraits(ASObject* o)
{
}

void EventDispatcher::dumpHandlers()
{
	std::map<tiny_string,list<listener> >::iterator it=handlers.begin();
	for(;it!=handlers.end();it++)
		std::cout << it->first << std::endl;
}

ASFUNCTIONBODY(EventDispatcher,addEventListener)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj);
	if(args[0]->getObjectType()!=T_STRING || args[1]->getObjectType()!=T_FUNCTION)
		throw RunTimeException("Type mismatch in EventDispatcher::addEventListener");

	bool useCapture=false;
	int priority=0;

	if(argslen>=3)
		useCapture=Boolean_concrete(args[2]);
	if(argslen>=4)
		priority=args[3]->toInt();

	if(useCapture || priority!=0)
		LOG(LOG_NOT_IMPLEMENTED,"Not implemented mode for addEventListener");

	const tiny_string& eventName=args[0]->toString();
	IFunction* f=static_cast<IFunction*>(args[1]);

	{
		Locker l(th->handlersMutex);
		std::map<tiny_string,std::list<listener> >::iterator it=th->handlers.insert(make_pair(eventName,list<listener>())).first;

		if(find(it->second.begin(),it->second.end(),f)!=it->second.end())
		{
			LOG(LOG_CALLS,"Weird event reregistration");
			return NULL;
		}

		f->incRef();
		it->second.push_back(listener(f));
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

	{
		Locker l(th->handlersMutex);
		map<tiny_string, list<listener> >::iterator h=th->handlers.find(args[0]->toString());
		if(h==th->handlers.end())
		{
			LOG(LOG_CALLS,"Event not found");
			return NULL;
		}

		IFunction* f=static_cast<IFunction*>(args[1]);
		std::list<listener>::iterator it=find(h->second.begin(),h->second.end(),f);
		if(it!=h->second.end())
		{
			//The listener owns the function
			it->f->decRef();
			h->second.erase(it);
		}
	}
	return NULL;
}

ASFUNCTIONBODY(EventDispatcher,dispatchEvent)
{
	EventDispatcher* th=Class<EventDispatcher>::cast(obj);
	if(args[0]->getPrototype()==NULL || !(args[0]->getPrototype()->isSubClass(Class<Event>::getClass())))
		return abstract_b(false);

	Event* e=Class<Event>::cast(args[0]);
	if(e==NULL || th==NULL)
		return abstract_b(false);
	//CHECK: maybe is to be cloned
	e->incRef();
	assert_and_throw(e->type!="");
	th->handleEvent(e);
	return abstract_b(true);
}

ASFUNCTIONBODY(EventDispatcher,_constructor)
{
	return NULL;
}

void EventDispatcher::handleEvent(Event* e)
{
	check();
	e->check();
	Locker l(handlersMutex);
	map<tiny_string, list<listener> >::iterator h=handlers.find(e->type);
	if(h==handlers.end())
	{
		LOG(LOG_CALLS,"Not handled event " << e->type);
		return;
	}

	LOG(LOG_CALLS, "Handling event " << h->first);

	//Create a temporary copy of the listeners, as the list can be modified during the calls
	vector<listener> tmpListener(h->second.begin(),h->second.end());
	l.unlock();
	//TODO: check, ok we should also bind the level
	for(unsigned int i=0;i<tmpListener.size();i++)
	{
		incRef();
		//The object needs to be used multiple times
		e->incRef();
		//tmpListener is now also owned by the vector
		tmpListener[i].f->incRef();
		//If the f is a class method, the 'this' is ignored
		ASObject* const arg0=e;
		ASObject* ret=tmpListener[i].f->call(this,&arg0,1);
		if(ret)
			ret->decRef();
		//And now no more, f can also be deleted
		tmpListener[i].f->decRef();
	}
	
	e->check();
	//If the number of handlers now if 0, then purge the entry from the map
	//TODO
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
}

void NetStatusEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("NET_STATUS","",Class<ASString>::getInstanceS("netStatus"));
}

ASFUNCTIONBODY(NetStatusEvent,_constructor)
{
	NetStatusEvent* th=Class<NetStatusEvent>::cast(obj);
	ASObject* info=Class<ASObject>::getInstanceS();
	info->setVariableByQName("level","",Class<ASString>::getInstanceS(th->level));
	info->setVariableByQName("code","",Class<ASString>::getInstanceS(th->code));
	obj->setVariableByQName("info","",info);
	return NULL;
}

FullScreenEvent::FullScreenEvent():Event("fullScreenEvent")
{
}

void FullScreenEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("FULL_SCREEN","",Class<ASString>::getInstanceS("fullScreen"));
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
	c->setVariableByQName("KEY_DOWN","",Class<ASString>::getInstanceS("keyDown"));
	c->setVariableByQName("KEY_UP","",Class<ASString>::getInstanceS("keyUp"));
}

ASFUNCTIONBODY(KeyboardEvent,_constructor)
{
	return NULL;
}

TextEvent::TextEvent():Event("textEvent")
{
}

void TextEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("TEXT_INPUT","",Class<ASString>::getInstanceS("textInput"));
	c->super=Class<Event>::getClass();
	c->max_level=c->super->max_level+1;
}

ASFUNCTIONBODY(TextEvent,_constructor)
{
	Event::_constructor(obj,NULL,0);
	return NULL;
}

ErrorEvent::ErrorEvent()
{
}

void ErrorEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<TextEvent>::getClass();
	c->max_level=c->super->max_level+1;

	c->setVariableByQName("ERROR","",Class<ASString>::getInstanceS("error"));
}

ASFUNCTIONBODY(ErrorEvent,_constructor)
{
	TextEvent::_constructor(obj,NULL,0);
	return NULL;
}

SecurityErrorEvent::SecurityErrorEvent()
{
}

void SecurityErrorEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ErrorEvent>::getClass();
	c->max_level=c->super->max_level+1;

	c->setVariableByQName("SECURITY_ERROR","",Class<ASString>::getInstanceS("securityError"));
}

ASFUNCTIONBODY(SecurityErrorEvent,_constructor)
{
	ErrorEvent::_constructor(obj,NULL,0);
	return NULL;
}

AsyncErrorEvent::AsyncErrorEvent()
{
}

void AsyncErrorEvent::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ErrorEvent>::getClass();
	c->max_level=c->super->max_level+1;

	c->setVariableByQName("ASYNC_ERROR","",Class<ASString>::getInstanceS("asyncError"));
}

ASFUNCTIONBODY(AsyncErrorEvent,_constructor)
{
	ErrorEvent::_constructor(obj,NULL,0);
	return NULL;
}

ABCContextInitEvent::ABCContextInitEvent(ABCContext* c):Event("ABCContextInitEvent"),context(c)
{
}

ShutdownEvent::ShutdownEvent():Event("shutdownEvent")
{
}

void HTTPStatusEvent::sinit(Class_base* c)
{
//	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setVariableByQName("HTTP_STATUS","",Class<ASString>::getInstanceS("httpStatus"));
}

