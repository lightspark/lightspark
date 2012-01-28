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
#include "toplevel/toplevel.h"
#include <string>

#undef MOUSE_EVENT

namespace lightspark
{

enum EVENT_TYPE { EVENT=0, BIND_CLASS, SHUTDOWN, SYNC, MOUSE_EVENT, FUNCTION, CONTEXT_INIT, INIT_FRAME,
			FLUSH_INVALIDATION_QUEUE, ADVANCE_FRAME };

class ABCContext;
class DictionaryTag;
class InteractiveObject;
class PlaceObject2Tag;
class MovieClip;

class Event: public ASObject
{
private:
	/*
	 * To be implemented by each derived class to allow redispatching
	 */
	virtual Event* cloneImpl() const;
public:
	Event(const tiny_string& t = "Event", bool b=false, bool c=false)
		: type(t),target(),currentTarget(),bubbles(b),cancelable(c),
		  eventPhase(0),defaultPrevented(false) {}
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	virtual void setTarget(_NR<ASObject> t) {target = t; }
	ASFUNCTION(_constructor);
	ASFUNCTION(_preventDefault);
	ASFUNCTION(_isDefaultPrevented);
	ASFUNCTION(formatToString);
	ASFUNCTION(clone);
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

/* Base class for all events that the one can wait on */
class WaitableEvent : public Event
{
public:
	WaitableEvent(const tiny_string& t = "Event", bool b=false, bool c=false)
		: Event(t,b,c), done(0) {}
	Semaphore done;
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
	TextEvent(const tiny_string& t = "textEvent");
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
	ErrorEvent(const tiny_string& t = "error", const std::string& e = "");
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
	SecurityErrorEvent(const std::string& e = "");
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
	ASPROPERTY_GETTER_SETTER(number_t,bytesLoaded);
	ASPROPERTY_GETTER_SETTER(number_t,bytesTotal);
	Event* cloneImpl() const;
public:
	ProgressEvent();
	ProgressEvent(uint32_t loaded, uint32_t total);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
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
	MouseEvent(const tiny_string& t, number_t lx, number_t ly, bool b=true, _NR<InteractiveObject> relObj = NullRef);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	void setTarget(_NR<ASObject> t);
	EVENT_TYPE getEventType() const { return MOUSE_EVENT;}
	ASPROPERTY_GETTER_SETTER(number_t,localX);
	ASPROPERTY_GETTER_SETTER(number_t,localY);
	ASPROPERTY_GETTER(number_t,stageX);
	ASPROPERTY_GETTER(number_t,stageY);
	ASPROPERTY_GETTER(_NR<InteractiveObject>,relatedObject);	
};

class listener
{
friend class EventDispatcher;
private:
	_R<IFunction> f;
	int32_t priority;
	/* true: get events in the capture phase
	 * false: get events in the bubble phase
	 */
	bool use_capture;
public:
	explicit listener(_R<IFunction> _f, int32_t _p, bool _c)
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

class IEventDispatcher
{
public:
	static void linkTraits(Class_base* c);
};

class EventDispatcher: public ASObject, public IEventDispatcher
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


class FunctionEvent: public WaitableEvent
{
friend class ABCVm;
private:
	_R<IFunction> f;
	_NR<ASObject> obj;
	ASObject** args;
	unsigned int numArgs;
	ASObject** result;
	ASObject** exception;
public:
	FunctionEvent(_R<IFunction> _f, _NR<ASObject> _obj=NullRef, ASObject** _args=NULL, uint32_t _numArgs=0, 
			ASObject** _result=NULL, ASObject** _exception=NULL);
	~FunctionEvent();
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const { return FUNCTION; }
};

class ABCContextInitEvent: public Event
{
friend class ABCVm;
private:
	ABCContext* context;
	bool lazy;
public:
	ABCContextInitEvent(ABCContext* c, bool lazy) DLL_PUBLIC;
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const { return CONTEXT_INIT; }
};

class Frame;

//Event to construct a Frame in the VM context
class InitFrameEvent: public Event
{
friend class ABCVm;
private:
	_NR<MovieClip> clip;
public:
	InitFrameEvent(_NR<MovieClip> m=NullRef) : Event("InitFrameEvent"),clip(m) {}
	EVENT_TYPE getEventType() const { return INIT_FRAME; }
};

class AdvanceFrameEvent: public WaitableEvent
{
public:
	AdvanceFrameEvent(): WaitableEvent("AdvanceFrameEvent") {}
	EVENT_TYPE getEventType() const { return ADVANCE_FRAME; }
};

//Event to flush the invalidation queue
class FlushInvalidationQueueEvent: public Event
{
public:
	FlushInvalidationQueueEvent():Event("FlushInvalidationQueueEvent"){};
	EVENT_TYPE getEventType() const { return FLUSH_INVALIDATION_QUEUE; };
};

class StatusEvent: public Event
{
public:
	StatusEvent() : Event("StatusEvent") {}
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o) {}
};

class DataEvent: public Event
{
public:
	DataEvent() : Event("DataEvent") {}
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o) {}
};

};
#endif
