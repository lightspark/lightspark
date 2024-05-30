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

#ifndef SCRIPTING_FLASH_GEOM_RECTANGLE_H
#define SCRIPTING_FLASH_GEOM_RECTANGLE_H 1

#include "compat.h"
#include "asobject.h"

namespace lightspark
{
class DisplayObject;

class Rectangle: public ASObject
{
private:
	std::set<DisplayObject*> users;
	void notifyUsers();
public:
	Rectangle(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_RECTANGLE),x(0),y(0),width(0),height(0) {}
	number_t x,y,width,height;
	static void sinit(Class_base* c);
	const RECT getRect() const;
	bool destruct() override;
	void addUser(DisplayObject* u);
	void removeUser(DisplayObject* u);
	
	// properties
	ASFUNCTION_ATOM(_getBottom);
	ASFUNCTION_ATOM(_setBottom);
	ASFUNCTION_ATOM(_getBottomRight);
	ASFUNCTION_ATOM(_setBottomRight);
	ASFUNCTION_ATOM(_getHeight);
	ASFUNCTION_ATOM(_setHeight);
	ASFUNCTION_ATOM(_getLeft);
	ASFUNCTION_ATOM(_setLeft);
	ASFUNCTION_ATOM(_getRight);
	ASFUNCTION_ATOM(_setRight);
	ASFUNCTION_ATOM(_getSize);
	ASFUNCTION_ATOM(_setSize);
	ASFUNCTION_ATOM(_getTop);
	ASFUNCTION_ATOM(_setTop);
	ASFUNCTION_ATOM(_getTopLeft);
	ASFUNCTION_ATOM(_setTopLeft);
	ASFUNCTION_ATOM(_getWidth);
	ASFUNCTION_ATOM(_setWidth);

	// methods
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(clone);
	ASFUNCTION_ATOM(contains);
	ASFUNCTION_ATOM(containsPoint);
	ASFUNCTION_ATOM(containsRect);
	ASFUNCTION_ATOM(equals);
	ASFUNCTION_ATOM(inflate);
	ASFUNCTION_ATOM(inflatePoint);
	ASFUNCTION_ATOM(intersection);
	ASFUNCTION_ATOM(intersects);
	ASFUNCTION_ATOM(isEmpty);
	ASFUNCTION_ATOM(offset);
	ASFUNCTION_ATOM(offsetPoint);
	ASFUNCTION_ATOM(setEmpty);
	ASFUNCTION_ATOM(setTo);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(_union);
	ASFUNCTION_ATOM(copyFrom);
};

}
#endif /* SCRIPTING_FLASH_GEOM_RECTANGLE_H */
