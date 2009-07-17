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

class ISWFObject;
class PlaceObject2Tag;

enum EVENT_TYPE { EVENT=0,BIND_CLASS, SHUTDOWN, SYNC, MOUSE_EVENT, FUNCTION };

class Event: public ASObject
{
public:
	Event(const std::string& t);
	virtual EVENT_TYPE getEventType() {return EVENT;} //DEPRECATED
	const std::string type;
	ISWFObject* clone()
	{
		return new Event(*this);
	}
};

class IOErrorEvent: public Event
{
public:
	IOErrorEvent();
};

class KeyboardEvent: public Event
{
public:
	KeyboardEvent();
};

class FocusEvent: public Event
{
public:
	FocusEvent();
};

class MouseEvent: public Event
{
public:
	MouseEvent();
	EVENT_TYPE getEventType(){ return MOUSE_EVENT;}
};

class EventDispatcher: public ASObject	
{
private:
	std::map<std::string,IFunction*> handlers;
public:
	float id;
	EventDispatcher();
	void handleEvent(Event* e);
	void setId(float i){id=i;}

	ASFUNCTION(_constructor);
	ASFUNCTION(addEventListener);
};

//Internal events from now on, used to pass data to the execution thread
class BindClassEvent: public Event
{
friend class ABCVm;
private:
	ASObject* base;
	std::string class_name;
public:
	BindClassEvent(ASObject* b, const std::string& c):
		Event("bindClass"),base(b),class_name(c){}
	EVENT_TYPE getEventType(){ return BIND_CLASS;}
};

class ShutdownEvent: public Event
{
public:
	ShutdownEvent():Event("shutdownEvent"){}
	EVENT_TYPE getEventType() { return SHUTDOWN; }
};

class FunctionEvent: public Event
{
friend class ABCVm;
private:
	IFunction* f;
public:
	FunctionEvent(IFunction* _f):f(_f),Event("functionEvent"){}
	EVENT_TYPE getEventType() { return FUNCTION; }
};

class SynchronizationEvent: public Event
{
private:
	sem_t* s;
public:
	SynchronizationEvent(sem_t* _s):s(_s),Event("SynchronizationEvent"){}
	EVENT_TYPE getEventType() { return SYNC; }
	void sync()
	{
		LOG(CALLS,"Posting sync");
		sem_post(s);
	}
};

#endif
