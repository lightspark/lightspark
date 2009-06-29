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

#include "asobjects.h"
#include <string>
#include <semaphore.h>

class ISWFObject;
class PlaceObject2Tag;

enum EVENT_TYPE { EVENT=0,BIND_CLASS, SHUTDOWN, SYNC, MOUSE_EVENT };

class Event: public ASObject
{
public:
	Event(const std::string& t);
	virtual EVENT_TYPE getEventType() {return EVENT;} //DEPRECATED
	const std::string type;
};

class MouseEvent: public Event
{
public:
	MouseEvent();
	EVENT_TYPE getEventType(){ return MOUSE_EVENT;}
};


//Internal events from now on, used to pass data to the execution thread
class BindClassEvent: public Event
{
friend class ABCVm;
private:
	ASObject* base;
	std::string class_name;
	ISWFObject* parent;
	std::string obj_name;
	PlaceObject2Tag* placed_by;
public:
	BindClassEvent(ASObject* b, ISWFObject* p, const std::string& c, const std::string& n, PlaceObject2Tag* t):
		Event("bindClass"),base(b),class_name(c),parent(p),obj_name(n),placed_by(t){}
	EVENT_TYPE getEventType(){ return BIND_CLASS;}
};

class ShutdownEvent: public Event
{
public:
	ShutdownEvent():Event("shutdownEvent"){}
	EVENT_TYPE getEventType() { return SHUTDOWN; }
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
