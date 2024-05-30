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

#ifndef SCRIPTING_FLASH_GEOM_POINT_H
#define SCRIPTING_FLASH_GEOM_POINT_H 1

#include "compat.h"
#include "asobject.h"
#include "backends/graphics.h"

namespace lightspark
{

class Point: public ASObject
{
private:
	number_t x,y;
	static number_t lenImpl(number_t x, number_t y);
public:
	Point(ASWorker* wrk,Class_base* c,number_t _x = 0, number_t _y = 0):ASObject(wrk,c,T_OBJECT,SUBTYPE_POINT),x(_x),y(_y){}
	bool destruct() override;
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getX);
	ASFUNCTION_ATOM(_getY);
	ASFUNCTION_ATOM(_setX);
	ASFUNCTION_ATOM(_setY);
	ASFUNCTION_ATOM(_getlength);
	ASFUNCTION_ATOM(interpolate);
	ASFUNCTION_ATOM(distance);
	ASFUNCTION_ATOM(add);
	ASFUNCTION_ATOM(subtract);
	ASFUNCTION_ATOM(clone);
	ASFUNCTION_ATOM(equals);
	ASFUNCTION_ATOM(normalize);
	ASFUNCTION_ATOM(offset);
	ASFUNCTION_ATOM(polar);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(setTo);
	ASFUNCTION_ATOM(copyFrom);

	number_t len() const;
	number_t getX() const { return x; }
	number_t getY() const { return y; }
};

}
#endif /* SCRIPTING_FLASH_GEOM_POINT_H */
