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

enum EVENT_TYPE { EVENT=0, BIND_CLASS, SHUTDOWN, SYNC, MOUSE_EVENT, FUNCTION, CONTEXT_INIT, INIT_FRAME,
			FLUSH_INVALIDATION_QUEUE, ADVANCE_FRAME };

class ABCContext;
class DictionaryTag;
class PlaceObject2Tag;
class MovieClip;

class Event: public ASObject
{
public:
	Event(const tiny_string& t = "Event", bool b=false, bool c=false)
		: type(t),target(NULL),currentTarget(NULL),bubbles(b),cancelable(c),
		  eventPhase(0),defaultPrevented(false) {}
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_preventDefault);
	ASFUNCTION(_isDefaultPrevented);
	ASFUNCTION(formatToString);
	virtual EVENT_TYPE getEventType() const {return EVENT;}
	//Altough events may be recycled and sent to more than a handler, the target property is set before sending
	//and the handling is serialized
	ASPROPERTY_GETTER(tiny_string,type);
	ASPROPERTY_GETTER(_NR<ASObject>,target);
	ASPROPERTY_GETTER(_NR<ASObject>,currentTarget);
	ASPROPERTY_GETTER(bool,bubbles);
	ASPROPERTY_GETTER(bool,cancelable);
	ASPROPERTY_GETTER(uint32_t,eventPhase);
	bool defaultPrevented;
};

class EventPhase: public ASObject
{
public:
	enum
	{
		CAPTURING_PHASE = 1,
		AT_TARGET = 2,
		BUBBLING_PHASE = 3
	};
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o) {}
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

class HTTPStatusEvent: public Event
{
public:
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o)
	{
	}
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
private:
	std::string errorMsg;
public:
	ErrorEvent() {};
	ErrorEvent(const std::string& e);
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
	SecurityErrorEvent() {};
	SecurityErrorEvent(const std::string& e);
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
	ProgressEvent(uint32_t loaded, uint32_t total);
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
	TimerEvent(const tiny_string& t):Event(t,true){};
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
	EVENT_TYPE getEventType() const { return MOUSE_EVENT;}
};

class listener
{
friend class EventDispatcher;
private:
	_R<IFunction> f;
	uint32_t priority;
	/* true: get events in the capture phase
	 * false: get events in the bubble phase
	 */
	bool use_capture;
public:
	explicit listener(_R<IFunction> _f, uint32_t _p, bool _c)
		:f(_f),priority(_p),use_capture(_c){};
	bool operator==(std::pair<IFunction*,bool> r)
	{
		/* One can register the same handle for the same event with
		 * different values of use_capture
		 */
		return (use_capture == r.second) && f->isEqual(r.first);
	}
	bool operator<(const listener& r) const
	{
		//The higher the priority the earlier this must be executed
		return priority>r.priority;
	}
};

class IEventDispatcher: public InterfaceClass
{
public:
	static void linkTraits(Class_base* c);
};

class EventDispatcher: public ASObject
{
private:
	Mutex handlersMutex;
	std::map<tiny_string,std::list<listener> > handlers;
public:
	EventDispatcher();
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	void handleEvent(_R<Event> e);
	void dumpHandlers();
	bool hasEventListener(const tiny_string& eventName);
	virtual void defaultEventBehavior(_R<Event> e) {};
	ASFUNCTION(_constructor);
	ASFUNCTION(addEventListener);
	ASFUNCTION(removeEventListener);
	ASFUNCTION(dispatchEvent);
	ASFUNCTION(_hasEventListener);
};

class RootMovieClip;
//Internal events from now on, used to pass data to the execution thread
class BindClassEvent: public Event
{
friend class ABCVm;
private:
	_NR<RootMovieClip> base;
	_NR<DictionaryTag> tag;
	tiny_string class_name;
public:
	BindClassEvent(_R<RootMovieClip> b, const tiny_string& c);
	BindClassEvent(_R<DictionaryTag> t, const tiny_string& c);
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const { return BIND_CLASS;}
};

class ShutdownEvent: public Event
{
public:
	ShutdownEvent() DLL_PUBLIC;
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const { return SHUTDOWN; }
};

class SynchronizationEvent;

class FunctionEvent: public Event
{
friend class ABCVm;
private:
	_R<IFunction> f;
	_NR<ASObject> obj;
	ASObject** args;
	unsigned int numArgs;
	ASObject** result;
	ASObject** exception;
	_NR<SynchronizationEvent> sync;
public:
	FunctionEvent(_R<IFunction> _f, _NR<ASObject> _obj=NullRef, ASObject** _args=NULL, uint32_t _numArgs=0, 
			ASObject** _result=NULL, ASObject** _exception=NULL, _NR<SynchronizationEvent> _sync=NullRef);
	~FunctionEvent();
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const { return FUNCTION; }
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
	EVENT_TYPE getEventType() const { return SYNC; }
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
	EVENT_TYPE getEventType() const { return CONTEXT_INIT; }
};

class Frame;

//Event to construct a Frame in the VM context
class InitFrameEvent: public Event
{
friend class ABCVm;
public:
	InitFrameEvent() : Event("InitFrameEvent") {}
	EVENT_TYPE getEventType() const { return INIT_FRAME; }
};

class AdvanceFrameEvent: public Event
{
public:
	AdvanceFrameEvent(): Event("AdvanceFrameEvent") {}
	EVENT_TYPE getEventType() const { return ADVANCE_FRAME; }
};

//Event to flush the invalidation queue
class FlushInvalidationQueueEvent: public Event
{
public:
	FlushInvalidationQueueEvent():Event("FlushInvalidationQueueEvent"){};
	EVENT_TYPE getEventType() const { return FLUSH_INVALIDATION_QUEUE; };
};

};
#endif
