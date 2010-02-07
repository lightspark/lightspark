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

#include "asobjects.h"
#include <string>
#include <semaphore.h>
#include "compat.h"

#undef MOUSE_EVENT

namespace lightspark
{

enum EVENT_TYPE { EVENT=0,BIND_CLASS, SHUTDOWN, SYNC, MOUSE_EVENT, FUNCTION, CONTEXT_INIT, CONSTRUCT_OBJECT, CHANGE_FRAME };

class ASObject;
class PlaceObject2Tag;
class ABCContext;

class Event: public IInterface
{
public:
	Event():type("Event"),target(NULL){}
	Event(const tiny_string& t, ASObject* _t=NULL);
	virtual ~Event(){}
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getType);
	ASFUNCTION(_getTarget);
	virtual EVENT_TYPE getEventType() {return EVENT;} //DEPRECATED
	tiny_string type;
	ASObject* target;
};

class FakeEvent: public Event
{
public:
	FakeEvent():Event("fakeEvent"){}
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
};

class IOErrorEvent: public Event
{
public:
	IOErrorEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
};

class KeyboardEvent: public Event
{
public:
	KeyboardEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
};

class FocusEvent: public Event
{
public:
	FocusEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
	ASFUNCTION(_constructor);
};

class ProgressEvent: public Event
{
private:
	uint32_t bytesLoaded;
	uint32_t bytesTotal;
public:
	ProgressEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getBytesLoaded);
	ASFUNCTION(_getBytesTotal);
};

class TimerEvent: public Event
{
public:
	TimerEvent():Event("DEPRECATED"){}
	TimerEvent(const tiny_string& t):Event(t){};
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
};

class MouseEvent: public Event
{
public:
	MouseEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
	EVENT_TYPE getEventType(){ return MOUSE_EVENT;}
};

class listener
{
friend class EventDispatcher;
private:
	IFunction* f;
public:
	explicit listener(IFunction* _f):f(_f){};
	bool operator==(IFunction* r)
	{
		return f->isEqual(r);
	}
};

class IEventDispatcher: public InterfaceClass
{
public:
	static void linkTraits(ASObject* o);
};

class EventDispatcher: public IInterface
{
private:
	std::map<tiny_string,std::list<listener> > handlers;
	void dumpHandlers();
public:
	float id;
	EventDispatcher();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	virtual ~EventDispatcher(){}
	void handleEvent(Event* e);
	void setId(float i){id=i;}
	bool hasEventListener(const tiny_string& eventName);

	ASFUNCTION(_constructor);
	ASFUNCTION(addEventListener);
	ASFUNCTION(removeEventListener);
	ASFUNCTION(dispatchEvent);
	ASFUNCTION(_hasEventListener);
};

//Internal events from now on, used to pass data to the execution thread
class BindClassEvent: public Event
{
friend class ABCVm;
private:
	IInterface* base;
	tiny_string class_name;
public:
	BindClassEvent(IInterface* b, const tiny_string& c):
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
	SynchronizationEvent(const tiny_string& _s):Event(_s){sem_init(&s,0,0);}
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
	EVENT_TYPE getEventType() { return CONTEXT_INIT; }
};

//This event is also synchronous
class ConstructObjectEvent: public SynchronizationEvent
{
friend class ABCVm;
private:
	ASObject* obj;
	Class_base* _class;
	static void sinit(Class_base*);
public:
	ConstructObjectEvent(ASObject* o, Class_base* c):SynchronizationEvent("ConstructObjectEvent"),_class(c),obj(o){}
	EVENT_TYPE getEventType() { return CONSTRUCT_OBJECT; }
};

//Event to change the current frame
class FrameChangeEvent: public Event
{
friend class ABCVm;
private:
	int frame;
	MovieClip* movieClip;
	static void sinit(Class_base*);
public:
	FrameChangeEvent(int f, MovieClip* m):frame(f),movieClip(m),Event("FrameChangeEvent"){}
	EVENT_TYPE getEventType() { return CHANGE_FRAME; }
};

};
#endif
