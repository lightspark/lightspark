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

using namespace std;

extern __thread SystemState* sys;

REGISTER_CLASS_NAME(EventDispatcher);
REGISTER_CLASS_NAME(Event);
REGISTER_CLASS_NAME(MouseEvent);

Event::Event(const string& t):type(t)
{
}

void Event::sinit(Class_base* c)
{
	//assert(c->constructor==NULL);
	//c->constructor=new Function(_constructor);
	c->setVariableByQName("ENTER_FRAME","",new ASString("enterFrame"));
	c->setVariableByQName("ADDED_TO_STAGE","",new ASString("addedToStage"));
	c->setVariableByQName("INIT","",new ASString("init"));
	c->setVariableByQName("ADDED","",new ASString("added"));
	c->setVariableByQName("COMPLETE","",new ASString("complete"));
	c->setVariableByQName("REMOVED","",new ASString("removed"));
	c->setVariableByQName("UNLOAD","",new ASString("unload"));
	c->setVariableByQName("ACTIVATE","",new ASString("activate"));
	c->setVariableByQName("DEACTIVATE","",new ASString("deactivate"));
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
/*	setVariableByQName("MOUSE_DOWN","",new ASString("mouseDown"));
	setVariableByQName("MOUSE_UP","",new ASString("mouseUp"));
	setVariableByQName("CLICK","",new ASString("click"));*/
}

void MouseEvent::sinit(Class_base* c)
{
//	assert(c->constructor==NULL);
//	c->constructor=new Function(_constructor);
}

IOErrorEvent::IOErrorEvent():Event("IOErrorEvent")
{
//	setVariableByQName("IO_ERROR","",new ASString("ioError"));
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
	std::map<std::string,IFunction*>::iterator it=handlers.begin();
	for(it;it!=handlers.end();it++)
		std::cout << it->first << std::endl;
}

ASFUNCTIONBODY(Event,_getType)
{
	Event* th=static_cast<Event*>(obj->interface);
	return new ASString(th->type);
}

ASFUNCTIONBODY(EventDispatcher,addEventListener)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj->interface);
	if(args->at(0)->getObjectType()!=T_STRING || args->at(1)->getObjectType()!=T_FUNCTION)
	{
		LOG(ERROR,"Type mismatch");
		abort();
	}
	if(th==NULL)
		return NULL;
	sys->cur_input_thread->addListener(args->at(0)->toString().raw_buf(),th);

	th->handlers.insert(make_pair(args->at(0)->toString().raw_buf(),args->at(1)->toFunction()));
	sys->events_name.push_back(args->at(0)->toString().raw_buf());
}

ASFUNCTIONBODY(EventDispatcher,dispatchEvent)
{
	EventDispatcher* th=static_cast<EventDispatcher*>(obj->interface);
	Event* e=static_cast<Event*>(args->at(0)->interface);
	assert(th->magic==0xdeadbeaf);
	if(e==NULL || th==NULL)
		return new Boolean(false);
/*	//CHECK: maybe is to be cloned
	args->at(0)->incRef();
	sys->currentVm->addEvent(th,e);*/
	return new Boolean(true);
}

ASFUNCTIONBODY(EventDispatcher,_constructor)
{
	cout << "EventDispatcher constructor" << endl;
	obj->setVariableByQName("addEventListener","",new Function(addEventListener));
	obj->setVariableByQName("dispatchEvent","",new Function(dispatchEvent));
}

void EventDispatcher::handleEvent(Event* e)
{
	map<string, IFunction*>::iterator h=handlers.find(e->type);
	if(h==handlers.end())
	{
		LOG(NOT_IMPLEMENTED,"Not handled event " << e->type);
		return;
	}

	LOG(CALLS, "Handling event " << h->first);
	arguments args(1);
	assert(e->obj);
	//The event is going to be decreffed as a function parameter
	args.set(0,e->obj);
	obj->incRef();
	h->second->call(obj,&args);
}

