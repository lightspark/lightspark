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

#include <string>

#include "abc.h"
#include "flashevents.h"
#include "swf.h"
#include "compat.h"
#include "class.h"

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;

REGISTER_CLASS_NAME(EventDispatcher);
REGISTER_CLASS_NAME(Event);
REGISTER_CLASS_NAME(MouseEvent);
REGISTER_CLASS_NAME(TimerEvent);
REGISTER_CLASS_NAME(ProgressEvent);
REGISTER_CLASS_NAME(IOErrorEvent);

Event::Event(const tiny_string& t):type(t)
{
}

void Event::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
	c->setVariableByQName("ENTER_FRAME","",new ASString("enterFrame"));
	c->setVariableByQName("ADDED_TO_STAGE","",new ASString("addedToStage"));
	c->setVariableByQName("INIT","",new ASString("init"));
	c->setVariableByQName("CLOSE","",new ASString("close"));
	c->setVariableByQName("ADDED","",new ASString("added"));
	c->setVariableByQName("COMPLETE","",new ASString("complete"));
	c->setVariableByQName("REMOVED","",new ASString("removed"));
	c->setVariableByQName("UNLOAD","",new ASString("unload"));
	c->setVariableByQName("ACTIVATE","",new ASString("activate"));
	c->setVariableByQName("DEACTIVATE","",new ASString("deactivate"));
	c->setVariableByQName("CHANGE","",new ASString("change"));
}

ASFUNCTIONBODY(Event,_constructor)
{
	Event* th=static_cast<Event*>(obj->implementation);
	assert(args->at(0)->getObjectType()==T_STRING);
	th->type=args->at(0)->toString();
	return NULL;
}

FocusEvent::FocusEvent():Event("focusEvent")
{
	/*setVariableByQName("FOCUS_IN","",new ASString("focusIn"));
	setVariableByQName("FOCUS_OUT","",new ASString("focusOut"));
	setVariableByQName("MOUSE_FOCUS_CHANGE","",new ASString("mouseFocusChange"));
	setVariableByQName("KEY_FOCUS_CHANGE","", new ASString("keyFocusChange"));*/
}

KeyboardEvent::KeyboardEvent():Event("keyboardEvent")
{
//	setVariableByQName("KEY_DOWN","",new ASString("keyDown"));
//	setVariableByQName("KEY_UP","",new ASString("keyUp"));
}

MouseEvent::MouseEvent():Event("mouseEvent")
{
//	setVariableByQName("MOUSE_UP","",new ASString("mouseUp"));
}

ProgressEvent::ProgressEvent():Event("progress")
{
}

void ProgressEvent::sinit(Class_base* c)
{
	c->setVariableByQName("PROGRESS","",new ASString("progress"));
}

void TimerEvent::sinit(Class_base* c)
{
	c->setVariableByQName("TIMER","",new ASString("timer"));
	c->setVariableByQName("TIMER_COMPLETE","",new ASString("timerComplete"));
}

void MouseEvent::sinit(Class_base* c)
{
//	assert(c->constructor==NULL);
//	c->constructor=new Function(_constructor);
	c->setVariableByQName("CLICK","",new ASString("click"));
	c->setVariableByQName("DOUBLE_CLICK","",new ASString("doubleClick"));
	c->setVariableByQName("MOUSE_DOWN","",new ASString("mouseDown"));
	c->setVariableByQName("MOUSE_OUT","",new ASString("mouseOut"));
	c->setVariableByQName("MOUSE_OVER","",new ASString("mouseOver"));
	c->setVariableByQName("MOUSE_UP","",new ASString("mouseUp"));
	c->setVariableByQName("ROLL_OVER","",new ASString("rollOver"));
	c->setVariableByQName("ROLL_OUT","",new ASString("rollOut"));
}

IOErrorEvent::IOErrorEvent():Event("IOErrorEvent")
{
}

void IOErrorEvent::sinit(Class_base* c)
{
	c->setVariableByQName("IO_ERROR","",new ASString("ioError"));
}

void FakeEvent::sinit(Class_base* c)
{
	c->setVariableByQName("SECURITY_ERROR","",new ASString("securityError"));
	c->setVariableByQName("ERROR","",new ASString("error"));
	c->setVariableByQName("FOCUS_IN","",new ASString("focusIn"));
	c->setVariableByQName("FOCUS_OUT","",new ASString("focusOut"));
	c->setVariableByQName("KEY_DOWN","",new ASString("keyDown"));
	c->setVariableByQName("KEY_UP","",new ASString("keyUp"));
}

EventDispatcher::EventDispatcher():id(0)
{
	magic=0xdeadbeaf;
}

void EventDispatcher::sinit(Class_base* c)
{
	assert(c->constructor==NULL);
	c->constructor=new Function(_constructor);
}

void EventDispatcher::dumpHandlers()
{
	std::map<tiny_string,IFunction*>::iterator it=handlers.begin();
	for(it;it!=handlers.end();it++)
		std::cout << it->first << std::endl;
}

ASFUNCTIONBODY(Event,_getType)
{
	Event* th=static_cast<Event*>(obj->implementation);
	return new ASString(th->type);
}

ASFUNCTIONBODY(EventDispatcher,addEventListener)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj->implementation);
	if(args->at(0)->getObjectType()!=T_STRING || args->at(1)->getObjectType()!=T_FUNCTION)
	{
		LOG(LOG_ERROR,"Type mismatch");
		abort();
	}
//	sys->cur_input_thread->addListener(args->at(0)->toString(),th);

	th->handlers.insert(make_pair(args->at(0)->toString(),args->at(1)->toFunction()));
	sys->events_name.push_back(args->at(0)->toString());
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

	map<tiny_string, IFunction*>::iterator h=th->handlers.find(args->at(0)->toString());
	if(h==th->handlers.end())
	{
		LOG(LOG_CALLS,"Event not found");
		return NULL;
	}

	//SERIOUS_TODO: functions should be compared based on their method body
	//assert(h->second==args->at(1)->toFunction());

	th->handlers.erase(h);
	return NULL;
}

ASFUNCTIONBODY(EventDispatcher,dispatchEvent)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj->implementation);
	Event* e=static_cast<Event*>(args->at(0)->implementation);
	assert(th->magic==0xdeadbeaf);
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
	cout << "EventDispatcher constructor" << endl;
	obj->setVariableByQName("addEventListener","",new Function(addEventListener));
	obj->setVariableByQName("removeEventListener","",new Function(removeEventListener));
	//MEGAHACK!!! should implement interface support
	obj->setVariableByQName("addEventListener","flash.events:IEventDispatcher",new Function(addEventListener));
	obj->setVariableByQName("dispatchEvent","",new Function(dispatchEvent));
	return NULL;
}

void EventDispatcher::handleEvent(Event* e)
{
	map<tiny_string, IFunction*>::iterator h=handlers.find(e->type);
	if(h==handlers.end())
	{
		LOG(LOG_NOT_IMPLEMENTED,"Not handled event " << e->type);
		return;
	}

	LOG(LOG_CALLS, "Handling event " << h->first);
	arguments args(1);
	assert(e->obj);
	//The event is going to be decreffed as a function parameter
	args.set(0,e->obj);
	obj->incRef();
	//TODO: check, ok we should also bind the level
	h->second->call(obj,&args,obj->max_level);
}

