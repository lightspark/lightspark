/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
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

REGISTER_CLASS_NAME(IEventDispatcher);
REGISTER_CLASS_NAME(EventDispatcher);
REGISTER_CLASS_NAME(Event);
REGISTER_CLASS_NAME(MouseEvent);
REGISTER_CLASS_NAME(TimerEvent);
REGISTER_CLASS_NAME(ProgressEvent);
REGISTER_CLASS_NAME(IOErrorEvent);
REGISTER_CLASS_NAME(FocusEvent);

void IEventDispatcher::linkTraits(ASObject* o)
{
	lookupAndLink(o,"addEventListener","flash.events:IEventDispatcher");
}

Event::Event(const tiny_string& t, ASObject* _t):type(t),target(_t)
{
}

void Event::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->setVariableByQName("ENTER_FRAME","",Class<ASString>::getInstanceS(true,"enterFrame")->obj);
	c->setVariableByQName("RENDER","",Class<ASString>::getInstanceS(true,"render")->obj);
	c->setVariableByQName("ADDED_TO_STAGE","",Class<ASString>::getInstanceS(true,"addedToStage")->obj);
	c->setVariableByQName("INIT","",Class<ASString>::getInstanceS(true,"init")->obj);
	c->setVariableByQName("CLOSE","",Class<ASString>::getInstanceS(true,"close")->obj);
	c->setVariableByQName("ADDED","",Class<ASString>::getInstanceS(true,"added")->obj);
	c->setVariableByQName("COMPLETE","",Class<ASString>::getInstanceS(true,"complete")->obj);
	c->setVariableByQName("REMOVED","",Class<ASString>::getInstanceS(true,"removed")->obj);
	c->setVariableByQName("UNLOAD","",Class<ASString>::getInstanceS(true,"unload")->obj);
	c->setVariableByQName("ACTIVATE","",Class<ASString>::getInstanceS(true,"activate")->obj);
	c->setVariableByQName("DEACTIVATE","",Class<ASString>::getInstanceS(true,"deactivate")->obj);
	c->setVariableByQName("CHANGE","",Class<ASString>::getInstanceS(true,"change")->obj);
	c->setVariableByQName("RESIZE","",Class<ASString>::getInstanceS(true,"resize")->obj);
}

void Event::buildTraits(ASObject* o)
{
	o->setGetterByQName("target","",new Function(_getTarget));
}

ASFUNCTIONBODY(Event,_constructor)
{
	Event* th=static_cast<Event*>(obj->implementation);
	if(args)
	{
		assert(args->at(0)->getObjectType()==T_STRING);
		th->type=args->at(0)->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(Event,_getTarget)
{
	Event* th=static_cast<Event*>(obj->implementation);
	if(th->target)
	{
		th->target->incRef();
		return th->target;
	}
	else
		return new Undefined;
}

ASFUNCTIONBODY(Event,_getType)
{
	Event* th=static_cast<Event*>(obj->implementation);
	return Class<ASString>::getInstanceS(true,th->type)->obj;
}

FocusEvent::FocusEvent():Event("focusEvent")
{
}

void FocusEvent::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->setVariableByQName("FOCUS_IN","",Class<ASString>::getInstanceS(true,"focusIn")->obj);
	c->setVariableByQName("FOCUS_OUT","",Class<ASString>::getInstanceS(true,"focusOut")->obj);
	c->setVariableByQName("MOUSE_FOCUS_CHANGE","",Class<ASString>::getInstanceS(true,"mouseFocusChange")->obj);
	c->setVariableByQName("KEY_FOCUS_CHANGE","",Class<ASString>::getInstanceS(true,"keyFocusChange")->obj);
}

ASFUNCTIONBODY(FocusEvent,_constructor)
{
}

KeyboardEvent::KeyboardEvent():Event("keyboardEvent")
{
//	setVariableByQName("KEY_DOWN","",new ASString("keyDown"));
//	setVariableByQName("KEY_UP","",new ASString("keyUp"));
}

MouseEvent::MouseEvent():Event("mouseEvent")
{
}

ProgressEvent::ProgressEvent():Event("progress"),bytesLoaded(0),bytesTotal(0)
{
}

void ProgressEvent::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->setVariableByQName("PROGRESS","",Class<ASString>::getInstanceS(true,"progress")->obj);
}

void ProgressEvent::buildTraits(ASObject* o)
{
	o->setGetterByQName("bytesLoaded","",new Function(_getBytesLoaded));
	o->setGetterByQName("bytesTotal","",new Function(_getBytesTotal));
}

ASFUNCTIONBODY(ProgressEvent,_constructor)
{
	ProgressEvent* th=static_cast<ProgressEvent*>(obj->implementation);
	if(args->size()>=4)
		th->bytesLoaded=args->at(3)->toInt();
	if(args->size()>=5)
		th->bytesTotal=args->at(4)->toInt();
}

ASFUNCTIONBODY(ProgressEvent,_getBytesLoaded)
{
	ProgressEvent* th=static_cast<ProgressEvent*>(obj->implementation);
	return abstract_d(th->bytesLoaded);
}

ASFUNCTIONBODY(ProgressEvent,_getBytesTotal)
{
	ProgressEvent* th=static_cast<ProgressEvent*>(obj->implementation);
	return abstract_d(th->bytesTotal);
}

void TimerEvent::sinit(Class_base* c)
{
	c->setVariableByQName("TIMER","",Class<ASString>::getInstanceS(true,"timer")->obj);
	c->setVariableByQName("TIMER_COMPLETE","",Class<ASString>::getInstanceS(true,"timerComplete")->obj);
}

void MouseEvent::sinit(Class_base* c)
{
//	assert(c->constructor==NULL);
//	c->constructor=new Function(_constructor);
	c->setVariableByQName("CLICK","",Class<ASString>::getInstanceS(true,"click")->obj);
	c->setVariableByQName("DOUBLE_CLICK","",Class<ASString>::getInstanceS(true,"doubleClick")->obj);
	c->setVariableByQName("MOUSE_DOWN","",Class<ASString>::getInstanceS(true,"mouseDown")->obj);
	c->setVariableByQName("MOUSE_OUT","",Class<ASString>::getInstanceS(true,"mouseOut")->obj);
	c->setVariableByQName("MOUSE_OVER","",Class<ASString>::getInstanceS(true,"mouseOver")->obj);
	c->setVariableByQName("MOUSE_UP","",Class<ASString>::getInstanceS(true,"mouseUp")->obj);
	c->setVariableByQName("MOUSE_WHEEL","",Class<ASString>::getInstanceS(true,"mouseWheel")->obj);
	c->setVariableByQName("MOUSE_MOVE","",Class<ASString>::getInstanceS(true,"mouseMove")->obj);
	c->setVariableByQName("ROLL_OVER","",Class<ASString>::getInstanceS(true,"rollOver")->obj);
	c->setVariableByQName("ROLL_OUT","",Class<ASString>::getInstanceS(true,"rollOut")->obj);
}

IOErrorEvent::IOErrorEvent():Event("IOErrorEvent")
{
}

void IOErrorEvent::sinit(Class_base* c)
{
	c->setVariableByQName("IO_ERROR","",Class<ASString>::getInstanceS(true,"ioError")->obj);
}

void FakeEvent::sinit(Class_base* c)
{
	c->setVariableByQName("SECURITY_ERROR","",Class<ASString>::getInstanceS(true,"securityError")->obj);
	c->setVariableByQName("ERROR","",Class<ASString>::getInstanceS(true,"error")->obj);
	c->setVariableByQName("KEY_DOWN","",Class<ASString>::getInstanceS(true,"keyDown")->obj);
	c->setVariableByQName("KEY_UP","",Class<ASString>::getInstanceS(true,"keyUp")->obj);
}

EventDispatcher::EventDispatcher():id(0)
{
}

void EventDispatcher::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->addImplementedInterface(Class<IEventDispatcher>::getClass());
}

void EventDispatcher::buildTraits(ASObject* o)
{
	o->setVariableByQName("addEventListener","",new Function(addEventListener));
	o->setVariableByQName("hasEventListener","",new Function(_hasEventListener));
	o->setVariableByQName("removeEventListener","",new Function(removeEventListener));
	o->setVariableByQName("dispatchEvent","",new Function(dispatchEvent));

	IEventDispatcher::linkTraits(o);
}

void EventDispatcher::dumpHandlers()
{
	std::map<tiny_string,list<listener> >::iterator it=handlers.begin();
	for(it;it!=handlers.end();it++)
		std::cout << it->first << std::endl;
}

ASFUNCTIONBODY(EventDispatcher,addEventListener)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj->implementation);
	if(args->at(0)->getObjectType()!=T_STRING || args->at(1)->getObjectType()!=T_FUNCTION)
	{
		LOG(LOG_ERROR,"Type mismatch");
		abort();
	}
	const tiny_string& eventName=args->at(0)->toString();
	IFunction* f=static_cast<IFunction*>(args->at(1));

	//TODO: find a nice way to do this
	if(eventName=="enterFrame")
		sys->cur_input_thread->addListener(eventName,th);

	std::map<tiny_string,std::list<listener> >::iterator it=th->handlers.insert(make_pair(eventName,list<listener>())).first;

	if(find(it->second.begin(),it->second.end(),f)!=it->second.end())
	{
		cout << "Weird event reregistration" << endl;
		return NULL;
	}

	f->incRef();
	it->second.push_back(listener(f));

	sys->events_name.push_back(eventName);
}

ASFUNCTIONBODY(EventDispatcher,_hasEventListener)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj->implementation);
	assert(args->size()==1 && args->at(0)->getObjectType()==T_STRING);
	const tiny_string& eventName=args->at(0)->toString();
	bool ret=th->hasEventListener(eventName);
	return new Boolean(ret);
}

ASFUNCTIONBODY(EventDispatcher,removeEventListener)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj->implementation);
	if(args->at(0)->getObjectType()!=T_STRING || args->at(1)->getObjectType()!=T_FUNCTION)
	{
		LOG(LOG_ERROR,"Type mismatch");
		abort();
	}
//	sys->cur_input_thread->addListener(args->at(0)->toString(),th);

	map<tiny_string, list<listener> >::iterator h=th->handlers.find(args->at(0)->toString());
	if(h==th->handlers.end())
	{
		LOG(LOG_CALLS,"Event not found");
		return NULL;
	}

	IFunction* f=static_cast<IFunction*>(args->at(1));
	std::list<listener>::iterator it=find(h->second.begin(),h->second.end(),f);
	assert(it!=h->second.end());
	//The listener owns the function
	it->f->decRef();
	h->second.erase(it);
	return NULL;
}

ASFUNCTIONBODY(EventDispatcher,dispatchEvent)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj->implementation);
	Event* e=static_cast<Event*>(args->at(0)->implementation);
	if(e==NULL || th==NULL)
		return new Boolean(false);
	//CHECK: maybe is to be cloned
	args->at(0)->incRef();
	assert(e->type!="");
	sys->currentVm->addEvent(th,e);
	return new Boolean(true);
}

ASFUNCTIONBODY(EventDispatcher,_constructor)
{
	return NULL;
}

void EventDispatcher::handleEvent(Event* e)
{
	map<tiny_string, list<listener> >::iterator h=handlers.find(e->type);
	if(h==handlers.end())
	{
		LOG(LOG_NOT_IMPLEMENTED,"Not handled event " << e->type);
		return;
	}

	LOG(LOG_CALLS, "Handling event " << h->first);
	assert(e->obj);

	//Create a temporary copy of the listeners, as the list can be modified during the calls
	vector<listener> tmpListener(h->second.begin(),h->second.end());
	//TODO: check, ok we should also bind the level
	for(int i=0;i<tmpListener.size();i++)
	{
		arguments args(1);
		//The event is going to be decreffed as a function parameter
		e->obj->incRef();
		args.set(0,e->obj);
		obj->incRef();
		//tmpListener is now also owned by the vector
		tmpListener[i].f->incRef();
		tmpListener[i].f->call(obj,&args,obj->max_level);
		//And now no more, f can also be deleted
		tmpListener[i].f->decRef();
	}
	e->obj->decRef();
	
	//If the number of handlers now if 0, then purge the entry from the map
	//TODO
}

bool EventDispatcher::hasEventListener(const tiny_string& eventName)
{
	if(handlers.find(eventName)==handlers.end())
		return false;
	else
		return true;
}
