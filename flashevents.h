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

#ifndef _FLASH_EVENTS_H
#define _FLASH_EVENTS_H

//#include "asobjects.h"
#include <string>
#include <semaphore.h>

class ASObject;
class PlaceObject2Tag;
class ABCContext;

enum EVENT_TYPE { EVENT=0,BIND_CLASS, SHUTDOWN, SYNC, MOUSE_EVENT, FUNCTION, CONTEXT };

class Event: public IInterface
{
public:
	Event(){ Event("Event"); }
	Event(const tiny_string& t);
	virtual ~Event(){}
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getType);
	virtual EVENT_TYPE getEventType() {return EVENT;} //DEPRECATED
	tiny_string type;
	Event* clone()
	{
		return new Event(*this);
	}
};

class FakeEvent: public Event
{
public:
	FakeEvent():Event("fakeEvent"){}
	static void sinit(Class_base*);
};

class IOErrorEvent: public Event
{
public:
	IOErrorEvent();
	static void sinit(Class_base*);
};

class KeyboardEvent: public Event
{
public:
	KeyboardEvent();
	static void sinit(Class_base*);
};

class FocusEvent: public Event
{
public:
	FocusEvent();
	static void sinit(Class_base*);
};

class ProgressEvent: public Event
{
public:
	ProgressEvent();
	static void sinit(Class_base*);
};

class MouseEvent: public Event
{
public:
	MouseEvent();
	static void sinit(Class_base*);
	EVENT_TYPE getEventType(){ return MOUSE_EVENT;}
};

class EventDispatcher: public IInterface
{
private:
	std::map<tiny_string,IFunction*> handlers;
	void dumpHandlers();
public:
	float id;
	EventDispatcher();
	static void sinit(Class_base*);
	virtual ~EventDispatcher(){}
	void handleEvent(Event* e);
	void setId(float i){id=i;}

	ASFUNCTION(_constructor);
	ASFUNCTION(addEventListener);
	ASFUNCTION(dispatchEvent);
};

//Internal events from now on, used to pass data to the execution thread
class BindClassEvent: public Event
{
friend class ABCVm;
private:
	IInterface* base;
	std::string class_name;
public:
	BindClassEvent(IInterface* b, const std::string& c):
		Event("bindClass"),base(b),class_name(c){}
	static void sinit(Class_base*);
	EVENT_TYPE getEventType(){ return BIND_CLASS;}
};

class ShutdownEvent: public Event
{
public:
	ShutdownEvent():Event("shutdownEvent"){}
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() { return SHUTDOWN; }
};

class FunctionEvent: public Event
{
friend class ABCVm;
private:
	IFunction* f;
public:
	FunctionEvent(IFunction* _f):f(_f),Event("functionEvent"){}
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() { return FUNCTION; }
};

class SynchronizationEvent: public Event
{
private:
	sem_t s;
public:
	SynchronizationEvent():Event("SynchronizationEvent"){sem_init(&s,0,0);}
	static void sinit(Class_base*);
	~SynchronizationEvent(){sem_destroy(&s);}
	EVENT_TYPE getEventType() { return SYNC; }
	void sync()
	{
		sem_post(&s);
	}
	void wait()
	{
		sem_wait(&s);
	}
};

class ABCContextInitEvent: public Event
{
friend class ABCVm;
private:
	ABCContext* context;
public:
	ABCContextInitEvent(ABCContext* c):Event("ABCContextInitEvent"),context(c){}
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() { return CONTEXT; }
};

#endif
