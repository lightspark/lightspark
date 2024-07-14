/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_EVENTS_FLASHEVENTS_H
#define SCRIPTING_FLASH_EVENTS_FLASHEVENTS_H 1

#include "asobject.h"
#include "compat.h"
#include "events.h"
#include "threading.h"
#include "tiny_string.h"
#include "scripting/flash/ui/keycodes.h"
#include <string>
#undef MOUSE_EVENT

namespace lightspark
{

enum EVENT_TYPE { EVENT=0, BIND_CLASS, SHUTDOWN, SYNC, MOUSE_EVENT,
	FUNCTION,FUNCTION_ASYNC, EXTERNAL_CALL, CONTEXT_INIT, INIT_FRAME,
	FLUSH_INVALIDATION_QUEUE, FLUSH_EVENT_BUFFER, ADVANCE_FRAME, PARSE_RPC_MESSAGE,EXECUTE_FRAMESCRIPT,TEXTINPUT_EVENT,IDLE_EVENT,
	AVM1INITACTION_EVENT,SET_LOADER_CONTENT_EVENT,ROOTCONSTRUCTEDEVENT, LOCALCONNECTIONEVENT,GETMOUSETARGET_EVENT };

class ABCContext;
class DictionaryTag;
class InteractiveObject;
class PlaceObject2Tag;
class DisplayObject;
class Responder;
class GameInputDevice;
class DefineSpriteTag;
class ByteArray;
class Rectangle;
class Loader;

class Event: public ASObject
{
public:
	Event(ASWorker* wrk, Class_base* cb, const tiny_string& t = "Event", bool b=false, bool c=false, CLASS_SUBTYPE st=SUBTYPE_EVENT);
	void finalize() override;
	static void sinit(Class_base*);
	virtual void setTarget(asAtom t) {target = t; }
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_preventDefault);
	ASFUNCTION_ATOM(_isDefaultPrevented);
	ASFUNCTION_ATOM(formatToString);
	ASFUNCTION_ATOM(clone);
	virtual EVENT_TYPE getEventType() const {return EVENT;}
	ASPROPERTY_GETTER(bool,bubbles);
	ASPROPERTY_GETTER(bool,cancelable);
	bool defaultPrevented;
	bool propagationStopped;
	bool immediatePropagationStopped;
	ACQUIRE_RELEASE_FLAG(queued); // indicates that this event was added to the event queue
	ASPROPERTY_GETTER(uint32_t,eventPhase);
	ASPROPERTY_GETTER(tiny_string,type);
	//Altough events may be recycled and sent to more than a handler, the target property is set before sending
	//and the handling is serialized
	ASPROPERTY_GETTER_ATOM(target);
	ASPROPERTY_GETTER(_NR<ASObject>,currentTarget);
	ASFUNCTION_ATOM(stopPropagation);
	ASFUNCTION_ATOM(stopImmediatePropagation);
private:
	/*
	 * To be implemented by each derived class to allow redispatching
	 */
	virtual Event* cloneImpl() const;
};

/* Base class for all events that the one can wait on */
class WaitableEvent : public Event
{
private:
	bool handled;
public:
	WaitableEvent(const tiny_string& t = "Event", bool b=false, bool c=false)
		: Event(nullptr,nullptr,t,b,c,SUBTYPE_WAITABLE_EVENT), handled(false) {}
	void wait();
	void signal();
	void finalize() override ;
};

class EventPhase: public ASObject
{
public:
	EventPhase(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	enum
	{
		CAPTURING_PHASE = 1,
		AT_TARGET = 2,
		BUBBLING_PHASE = 3
	};
	static void sinit(Class_base*);
};

class KeyboardEvent: public Event
{
private:
	Event* cloneImpl() const override ;

	LSModifier modifiers;
	ASPROPERTY_GETTER_SETTER(AS3KeyCode, charCode);
	ASPROPERTY_GETTER_SETTER(AS3KeyCode, keyCode);
	ASPROPERTY_GETTER_SETTER(uint32_t, keyLocation);
public:

	KeyboardEvent(ASWorker* wrk, Class_base* c, tiny_string _type="", const AS3KeyCode& _charCode = AS3KEYCODE_UNKNOWN, const AS3KeyCode& _keyCode = AS3KEYCODE_UNKNOWN, const LSModifier& _modifiers = LSModifier::None);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_GETTER_SETTER(altKey);
	ASFUNCTION_GETTER_SETTER(commandKey);
	ASFUNCTION_GETTER_SETTER(controlKey);
	ASFUNCTION_GETTER_SETTER(ctrlKey);
	ASFUNCTION_GETTER_SETTER(shiftKey);
	ASFUNCTION_ATOM(updateAfterEvent);
	AS3KeyCode getCharCode() const { return charCode; }
	AS3KeyCode getKeyCode() const { return keyCode; }
	LSModifier getModifiers() const { return modifiers; }
};

class FocusEvent: public Event
{
public:
	FocusEvent(ASWorker* wrk,Class_base* c, tiny_string _type="focusEvent");
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class FullScreenEvent: public Event
{
public:
	FullScreenEvent(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class NetStatusEvent: public Event
{
private:
	Event* cloneImpl() const override ;
public:
	NetStatusEvent(ASWorker* wrk,Class_base* cb, const tiny_string& l="", const tiny_string& c="");
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	tiny_string statuscode;
};

class HTTPStatusEvent: public Event
{
public:
	HTTPStatusEvent(ASWorker* wrk, Class_base* c):Event(wrk,c){}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class TextEvent: public Event
{
protected:
	ASPROPERTY_GETTER_SETTER(tiny_string,text);
public:
	TextEvent(ASWorker* wrk, Class_base* c, const tiny_string& t = "textEvent");
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class ErrorEvent: public TextEvent
{
protected:
	std::string errorMsg;
	Event* cloneImpl() const override;
	ASPROPERTY_GETTER(int32_t,errorID);
public:
	ErrorEvent(ASWorker* wrk, Class_base* c, const tiny_string& t = "error", const std::string& e = "", int id = 0);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class IOErrorEvent: public ErrorEvent
{
private:
	Event* cloneImpl() const override;
public:
	IOErrorEvent(ASWorker* wrk, Class_base* c, const tiny_string& t = "ioError", const std::string& e = "", int id = 0);
	static void sinit(Class_base*);
};

class SecurityErrorEvent: public ErrorEvent
{
public:
	SecurityErrorEvent(ASWorker* wrk, Class_base* c, const std::string& e = "");
	static void sinit(Class_base*);
};

class AsyncErrorEvent: public ErrorEvent
{
public:
	AsyncErrorEvent(ASWorker* wrk, Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class UncaughtErrorEvent: public ErrorEvent
{
public:
	UncaughtErrorEvent(ASWorker* wrk, Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};


class ProgressEvent: public Event
{
private:
	Event* cloneImpl() const override;
public:
	Mutex accesmutex;
	ASPROPERTY_GETTER_SETTER(int32_t,bytesLoaded);
	ASPROPERTY_GETTER_SETTER(int32_t,bytesTotal);
	ProgressEvent(ASWorker* wrk, Class_base* c);
	ProgressEvent(ASWorker* wrk, Class_base* c, uint32_t loaded, uint32_t total, const tiny_string& t="progress");
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class TimerEvent: public Event
{
public:
	TimerEvent(ASWorker* wrk, Class_base* c):Event(wrk, c, "DEPRECATED"){}
	TimerEvent(ASWorker* wrk, Class_base* c,const tiny_string& t):Event(wrk,c,t,true){}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(updateAfterEvent);
};

class MouseEvent: public Event
{
private:
	LSModifier modifiers;
	Event* cloneImpl() const override;
public:
	MouseEvent(ASWorker* wrk, Class_base* c);
	MouseEvent(ASWorker* wrk, Class_base* c, const tiny_string& t, number_t lx, number_t ly,
		   bool b=true, const LSModifier& _modifiers = LSModifier::None, bool _buttonDown = false,
		   _NR<InteractiveObject> relObj = NullRef, int32_t delta=1);
	static void sinit(Class_base*);
	void setTarget(asAtom t) override;
	EVENT_TYPE getEventType() const override { return MOUSE_EVENT;}
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(bool,buttonDown);
	ASFUNCTION_GETTER_SETTER(altKey);
	ASFUNCTION_GETTER_SETTER(controlKey);
	ASFUNCTION_GETTER_SETTER(ctrlKey);
	ASPROPERTY_GETTER_SETTER(int32_t,delta);
	ASPROPERTY_GETTER_SETTER(number_t,localX);
	ASPROPERTY_GETTER_SETTER(number_t,localY);
	ASFUNCTION_GETTER_SETTER(shiftKey);
	ASPROPERTY_GETTER(number_t,stageX);
	ASPROPERTY_GETTER(number_t,stageY);
	ASPROPERTY_GETTER_SETTER(_NR<InteractiveObject>,relatedObject);
	ASFUNCTION_ATOM(updateAfterEvent);
	MouseEvent* getclone() const;
};

class NativeDragEvent: public MouseEvent
{
public:
	NativeDragEvent(ASWorker* wrk, Class_base* c);
	
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class NativeWindowBoundsEvent: public Event
{
public:
	NativeWindowBoundsEvent(ASWorker* wrk, Class_base* c);
	NativeWindowBoundsEvent(ASWorker* wrk, Class_base* c, const tiny_string& t, NullableRef<Rectangle> _beforeBounds, NullableRef<Rectangle> _afterBounds);
	
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_toString);
	ASPROPERTY_GETTER(_NR<Rectangle>,afterBounds);
	ASPROPERTY_GETTER(_NR<Rectangle>,beforeBounds);
	Event* cloneImpl() const override;
};

class InvokeEvent: public Event
{
public:
	InvokeEvent(ASWorker* wrk, Class_base* c) : Event(wrk,c, "invoke"){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
};

class listener
{
friend class EventDispatcher;
private:
	asAtom f=asAtomHandler::invalidAtom;
	int32_t priority;
	/* true: get events in the capture phase
	 * false: get events in the bubble phase
	 */
	bool use_capture;
	ASWorker* worker;
public:
	explicit listener(asAtom _f, int32_t _p, bool _c,ASWorker* _w)
		:f(_f),priority(_p),use_capture(_c),worker(_w){}
	bool operator==(const listener& r);
	bool operator<(const listener& r) const
	{
		//The higher the priority the earlier this must be executed
		return priority>r.priority;
	}
	void resetClosure();
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
	/*
	 * This will be used when a target is passed to EventDispatcher constructor
	 */
	asAtom forcedTarget;
protected:
	virtual void eventListenerAdded(const tiny_string& eventName) {}
public:
	EventDispatcher(ASWorker* wrk, Class_base* c);
	void finalize() override;
	bool destruct() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	void prepareShutdown() override;
	void clearEventListeners();
	// is called when a new event is added to the event queue
	virtual void onNewEvent(Event* ev){}
	// is called after an event was handled by the event queue
	virtual void afterHandleEvent(Event* ev) {}
	static void sinit(Class_base*);
	void handleEvent(_R<Event> e);
	void dumpHandlers();
	bool hasEventListener(const tiny_string& eventName);
	virtual void defaultEventBehavior(_R<Event> e) {}
	virtual void afterExecution(_R<Event> e) {}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(addEventListener);
	ASFUNCTION_ATOM(removeEventListener);
	ASFUNCTION_ATOM(dispatchEvent);
	ASFUNCTION_ATOM(_hasEventListener);
};

class StatusEvent: public Event
{
private:
	Event* cloneImpl() const override;
public:
	StatusEvent(ASWorker* wrk, Class_base* c, const tiny_string& _code="", const tiny_string& _level=""):Event(wrk,c, "status"),
		code(_code),level(_level)
	{
	}
	static void sinit(Class_base*);
	ASPROPERTY_GETTER_SETTER(tiny_string, code);
	ASPROPERTY_GETTER_SETTER(tiny_string, level);
};

class DataEvent: public TextEvent
{
private:
	Event* cloneImpl() const override;
public:
	DataEvent(ASWorker* wrk, Class_base* c, const tiny_string& _data="") : TextEvent(wrk,c, "data"), data(_data) {}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(tiny_string, data);
};

class RootMovieClip;
//Internal events from now on, used to pass data to the execution thread
class BindClassEvent: public Event
{
friend class ABCVm;
private:
	_NR<RootMovieClip> base;
	DictionaryTag* tag;
	tiny_string class_name;
public:
	BindClassEvent(_R<RootMovieClip> b, const tiny_string& c);
	BindClassEvent(DictionaryTag* t, const tiny_string& c);
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const override { return BIND_CLASS;}
};

class ShutdownEvent: public Event
{
public:
	ShutdownEvent() DLL_PUBLIC;
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const override { return SHUTDOWN; }
};


class FunctionEvent: public WaitableEvent
{
friend class ABCVm;
private:
	asAtom f=asAtomHandler::invalidAtom;
	asAtom obj=asAtomHandler::invalidAtom;
	asAtom* args;
	unsigned int numArgs;
public:
	FunctionEvent(asAtom _f, asAtom _obj=asAtomHandler::invalidAtom, asAtom* _args=nullptr, uint32_t _numArgs=0);
	~FunctionEvent();
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const override { return FUNCTION; }
};

class FunctionAsyncEvent: public Event
{
friend class ABCVm;
private:
	asAtom f=asAtomHandler::invalidAtom;
	asAtom obj=asAtomHandler::invalidAtom;
	asAtom* args;
	unsigned int numArgs;
public:
	FunctionAsyncEvent(asAtom _f, asAtom _obj=asAtomHandler::invalidAtom, asAtom* _args=nullptr, uint32_t _numArgs=0);
	~FunctionAsyncEvent();
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const override { return FUNCTION_ASYNC; }
};

class ExternalCallEvent: public WaitableEvent
{
friend class ABCVm;
private:
	asAtom f=asAtomHandler::invalidAtom;
	ASObject* const* args;
	ASObject** result;
	bool* thrown;
	tiny_string* exception;
	unsigned int numArgs;
public:
	ExternalCallEvent(asAtom _f, ASObject* const* _args, uint32_t _numArgs,
			  ASObject** _result, bool* _thrown, tiny_string* _exception);
	~ExternalCallEvent();
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const override { return EXTERNAL_CALL; }
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
	EVENT_TYPE getEventType() const override { return CONTEXT_INIT; }
};
class AVM1InitActionEvent: public Event
{
friend class ABCVm;
private:
	RootMovieClip* root;
	_NR<MovieClip> clip;
public:
	AVM1InitActionEvent(RootMovieClip* r, _NR<MovieClip> c);
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const override { return AVM1INITACTION_EVENT; }
	void executeActions();
	void finalize() override;
};


class Frame;

//Event to construct a Frame in the VM context
class InitFrameEvent: public Event
{
friend class ABCVm;
private:
	_NR<DisplayObject> clip;
public:
	InitFrameEvent(_NR<DisplayObject> m);
	EVENT_TYPE getEventType() const override { return INIT_FRAME; }
};

class ExecuteFrameScriptEvent: public Event
{
friend class ABCVm;
private:
	_NR<DisplayObject> clip;
public:
	ExecuteFrameScriptEvent(_NR<DisplayObject> m);
	static void sinit(Class_base*);
	EVENT_TYPE getEventType() const override { return EXECUTE_FRAMESCRIPT; }
};

class AdvanceFrameEvent: public Event
{
friend class ABCVm;
private:
	_NR<DisplayObject> clip;
public:
	AdvanceFrameEvent(_NR<DisplayObject> m=NullRef);
	EVENT_TYPE getEventType() const override { return ADVANCE_FRAME; }
};
class SetLoaderContentEvent: public Event
{
friend class ABCVm;
private:
	_NR<DisplayObject> content;
	_NR<Loader> loader;
public:
	SetLoaderContentEvent(_NR<DisplayObject> m, _NR<Loader> _loader);
	//SetLoaderContentEvent(_NR<MovieClip> m, _NR<Loader> _loader);
	EVENT_TYPE getEventType() const override { return SET_LOADER_CONTENT_EVENT; }
};
class RootConstructedEvent: public Event
{
friend class ABCVm;
private:
	_NR<DisplayObject> clip;
	bool _explicit;
public:
	RootConstructedEvent(_NR<DisplayObject> m, bool explicit_ = false);
	EVENT_TYPE getEventType() const override { return ROOTCONSTRUCTEDEVENT; }
};
class LocalConnectionEvent: public Event
{
friend class SystemState;
private:
	uint32_t nameID;
	uint32_t methodID;
	asAtom* args;
	uint32_t numargs;
public:
	LocalConnectionEvent(uint32_t _nameID, uint32_t _methodID, asAtom* _args, uint32_t _numargs);
	~LocalConnectionEvent();
	EVENT_TYPE getEventType() const override { return LOCALCONNECTIONEVENT; }
};

class IdleEvent: public WaitableEvent
{
public:
	IdleEvent(): WaitableEvent("IdleEvent") {}
	EVENT_TYPE getEventType() const override { return IDLE_EVENT; }
};

class GetMouseTargetEvent: public WaitableEvent
{
public:
	uint32_t x;
	uint32_t y;
	HIT_TYPE type;
	_NR<DisplayObject> dispobj;
	GetMouseTargetEvent(uint32_t _x, uint32_t _y, HIT_TYPE _type);
	EVENT_TYPE getEventType() const override { return GETMOUSETARGET_EVENT; }
};

class FlushEventBufferEvent: public Event
{
friend class ABCVm;
private:
	bool append;
	bool reverse;
public:
	FlushEventBufferEvent(bool _append = true, bool _reverse = false): Event(nullptr,nullptr, "FlushEventBufferEvent"),append(_append),reverse(_reverse) {}
	EVENT_TYPE getEventType() const override { return FLUSH_EVENT_BUFFER; }
};

//Event to flush the invalidation queue
class FlushInvalidationQueueEvent: public Event
{
public:
	FlushInvalidationQueueEvent():Event(nullptr,nullptr, "FlushInvalidationQueueEvent"){}
	EVENT_TYPE getEventType() const override { return FLUSH_INVALIDATION_QUEUE; }
};

class ParseRPCMessageEvent: public Event
{
public:
	_NR<ByteArray> message;
	_NR<ASObject> client;
	_NR<Responder> responder;
	ParseRPCMessageEvent(_R<ByteArray> ba, _NR<ASObject> client, _NR<Responder> responder);
	EVENT_TYPE getEventType() const override { return PARSE_RPC_MESSAGE; }
	void finalize() override;
};
class TextInputEvent: public Event
{
friend class ABCVm;
private:
	_NR<InteractiveObject> target;
	tiny_string text;
public:
	TextInputEvent(_NR<InteractiveObject> m, const tiny_string& s);
	EVENT_TYPE getEventType() const override { return TEXTINPUT_EVENT; }
};

class DRMErrorEvent: public ErrorEvent
{
public:
	DRMErrorEvent(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class DRMStatusEvent: public Event
{
public:
	DRMStatusEvent(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class VideoEvent: public Event
{
private:
	Event* cloneImpl() const override;
public:
	VideoEvent(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER(tiny_string,status);
};

class StageVideoEvent: public Event
{
private:
	Event* cloneImpl() const override;
public:
	StageVideoEvent(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER(tiny_string,colorSpace);
	ASPROPERTY_GETTER(tiny_string,status);
};

class StageVideoAvailabilityEvent: public Event
{
private:
	Event* cloneImpl() const override;
public:
	StageVideoAvailabilityEvent(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER(tiny_string,availability);
};

class ContextMenuEvent: public Event
{
private:
	Event* cloneImpl() const override;
public:
	ContextMenuEvent(ASWorker* wrk, Class_base* c);
	ContextMenuEvent(ASWorker* wrk, Class_base* c, tiny_string t, _NR<InteractiveObject> target, _NR<InteractiveObject> owner);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(_NR<InteractiveObject>,mouseTarget);
	ASPROPERTY_GETTER_SETTER(_NR<InteractiveObject>,contextMenuOwner);
};

class TouchEvent: public Event
{
public:
	TouchEvent(ASWorker* wrk, Class_base* c) : Event(wrk,c, "TouchEvent") {}
	static void sinit(Class_base*);
};

class GestureEvent: public Event
{
public:
	GestureEvent(ASWorker* wrk, Class_base* c, const tiny_string& t = "GestureEvent") : Event(wrk,c, t) {}
	static void sinit(Class_base*);
};

class PressAndTapGestureEvent: public GestureEvent
{
public:
	PressAndTapGestureEvent(ASWorker* wrk, Class_base* c) : GestureEvent(wrk,c, "PressAndTapGestureEvent") {}
	static void sinit(Class_base*);
};
class TransformGestureEvent: public GestureEvent
{
public:
	TransformGestureEvent(ASWorker* wrk, Class_base* c) : GestureEvent(wrk,c, "TransformGestureEvent") {}
	static void sinit(Class_base*);
};

class UncaughtErrorEvents: public EventDispatcher
{
public:
	UncaughtErrorEvents(ASWorker* wrk, Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
};

class SampleDataEvent: public Event
{
private:
	Event* cloneImpl() const override;
public:
	SampleDataEvent(ASWorker* wrk, Class_base* c);
	SampleDataEvent(ASWorker* wrk, Class_base* c,_NR<ByteArray> _data,number_t _pos);
	bool destruct() override;
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_toString);
	ASPROPERTY_GETTER_SETTER(_NR<ByteArray>,data);
	ASPROPERTY_GETTER_SETTER(number_t,position);
};

class ThrottleEvent: public Event
{
private:
	Event* cloneImpl() const override;
public:
	ThrottleEvent(ASWorker* wrk, Class_base* c) : Event(wrk,c, "throttle",false,false,SUBTYPE_THROTTLE_EVENT),targetFrameRate(0) {}
	ThrottleEvent(ASWorker* wrk, Class_base* c,const tiny_string _state,number_t _targetFrameRate) : Event(wrk,c, "throttle",false,false,SUBTYPE_THROTTLE_EVENT),state(_state),targetFrameRate(_targetFrameRate) {}
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_toString);
	ASPROPERTY_GETTER(tiny_string,state);
	ASPROPERTY_GETTER(number_t,targetFrameRate);
};
class ThrottleType: public ASObject
{
public:
	ThrottleType(ASWorker* wrk, Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class GameInputEvent: public Event
{
private:
	Event* cloneImpl() const override;
public:
	GameInputEvent(ASWorker* wrk, Class_base* c);
	GameInputEvent(ASWorker* wrk, Class_base* c,_NR<GameInputDevice> _device);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER(_NR<GameInputDevice>,device);
};

}
#endif /* SCRIPTING_FLASH_EVENTS_FLASHEVENTS_H */
