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

#ifndef _FLASH_EVENTS_H
#define _FLASH_EVENTS_H

#include "compat.h"
#include "asobject.h"
#include "toplevel.h"
#include <string>
#include <semaphore.h>

#undef MOUSE_EVENT

namespace lightspark
{

enum EVENT_TYPE { EVENT=0,BIND_CLASS, SHUTDOWN, SYNC, MOUSE_EVENT, FUNCTION, CONTEXT_INIT, CONSTRUCT_OBJECT, CHANGE_FRAME };

class ABCContext;

class Event: public ASObject
{
public:
	Event():type("Event"),target(NULL),currentTarget(NULL),bubbles(false){}
	Event(const tiny_string& t, bool b=false);
	virtual ~Event();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getType);
	ASFUNCTION(_getTarget);
	virtual EVENT_TYPE getEventType() {return EVENT;}
	tiny_string type;
	//Altough events may be recycled and sent to more than a handler, the target property is set before sending
	//and the handling is serialized
	ASObject* target;
	ASObject* currentTarget;
	bool bubbles;
};

class KeyboardEvent: public Event
{
public:
	KeyboardEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
	ASFUNCTION(_constructor);
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

class FullScreenEvent: public Event
{
public:
	FullScreenEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
	ASFUNCTION(_constructor);
};

class NetStatusEvent: public Event
{
private:
	tiny_string level;
	tiny_string code;
public:
	NetStatusEvent():Event("netStatus"){}
	NetStatusEvent(const tiny_string& l, const tiny_string& c);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
	ASFUNCTION(_constructor);
};

class TextEvent: public Event
{
public:
	TextEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
	ASFUNCTION(_constructor);
};

class ErrorEvent: public TextEvent
{
public:
	ErrorEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
	ASFUNCTION(_constructor);
};

class IOErrorEvent: public ErrorEvent
{
public:
	IOErrorEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
};

class SecurityErrorEvent: public ErrorEvent
{
public:
	SecurityErrorEvent();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
	ASFUNCTION(_constructor);
};

class AsyncErrorEvent: public ErrorEvent
{
public:
	AsyncErrorEvent();
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
	MouseEvent(const tiny_string& t, bool b=true);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
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

class EventDispatcher: public ASObject
{
private:
	Mutex handlersMutex;
	std::map<tiny_string,std::list<listener> > handlers;
public:
	EventDispatcher();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	virtual ~EventDispatcher(){}
	void handleEvent(Event* e);
	void dumpHandlers();
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
	ASObject* base;
	tiny_string class_name;
public:
	BindClassEvent(ASObject* b, const tiny_string& c):
		Event("bindClass"),base(b),class_name(c){}
	static void sinit(Class_base*);
	EVENT_TYPE getEventType(){ return BIND_CLASS;}
};

class ShutdownEvent: public Event
{
public:
	ShutdownEvent() DLL_PUBLIC;
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() { return SHUTDOWN; }
};

class FunctionEvent: public Event
{
friend class ABCVm;
private:
	IFunction* f;
public:
	FunctionEvent(IFunction* _f):Event("functionEvent"),f(_f){}
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
	ABCContextInitEvent(ABCContext* c) DLL_PUBLIC;
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() { return CONTEXT_INIT; }
};

//This event is also synchronous
class ConstructObjectEvent: public SynchronizationEvent
{
friend class ABCVm;
private:
	Class_base* _class;
	ASObject* _obj;
	static void sinit(Class_base*);
public:
	ConstructObjectEvent(ASObject* o, Class_base* c):SynchronizationEvent("ConstructObjectEvent"),_class(c),_obj(o){}
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
	FrameChangeEvent(int f, MovieClip* m):Event("FrameChangeEvent"),frame(f),movieClip(m){}
	EVENT_TYPE getEventType() { return CHANGE_FRAME; }
};

};
#endif
