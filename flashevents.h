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

class ISWFObject;

enum EVENT_TYPE { EVENT=0,BIND_CLASS, SHUTDOWN, MOUSE_EVENT };

class Event: public ASObject
{
public:
	Event(const std::string& t);
	//DEPRECATED
	virtual EVENT_TYPE getEventType() {return EVENT;}
	const std::string type;
};

class MouseEvent: public Event
{
public:
	MouseEvent();
	EVENT_TYPE getEventType(){ return MOUSE_EVENT;}
};

class BindClassEvent: public Event
{
friend class ABCVm;
private:
	ISWFObject* base;
	std::string class_name;
public:
	BindClassEvent(ISWFObject* b, const std::string& c):Event("bindClass"),base(b),class_name(c){}
	EVENT_TYPE getEventType(){ return BIND_CLASS;}
};

class ShutdownEvent: public Event
{
public:
	ShutdownEvent():Event("shutdownEvent"){}
	EVENT_TYPE getEventType() { return SHUTDOWN; }
};

