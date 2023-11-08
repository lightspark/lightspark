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

#ifndef SCRIPTING_FLASH_GEOM_VECTOR3D_H
#define SCRIPTING_FLASH_GEOM_VECTOR3D_H 1

#include "asobject.h"

namespace lightspark
{

class Vector3D: public ASObject
{
public:
	Vector3D(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_VECTOR3D),x(0),y(0),z(0),w(0){}
	static void sinit(Class_base* c);
	bool destruct() override;
	ASFUNCTION_ATOM(_constructor);
	
	ASPROPERTY_GETTER_SETTER(number_t,x);
	ASPROPERTY_GETTER_SETTER(number_t,y);
	ASPROPERTY_GETTER_SETTER(number_t,z);
	ASPROPERTY_GETTER_SETTER(number_t,w);
	ASFUNCTION_ATOM(add);
	ASFUNCTION_ATOM(angleBetween);
	ASFUNCTION_ATOM(clone);
	ASFUNCTION_ATOM(crossProduct);
	ASFUNCTION_ATOM(decrementBy);
	ASFUNCTION_ATOM(distance);
	ASFUNCTION_ATOM(dotProduct);
	ASFUNCTION_ATOM(equals);
	ASFUNCTION_ATOM(incrementBy);
	ASFUNCTION_ATOM(nearEquals);
	ASFUNCTION_ATOM(negate);
	ASFUNCTION_ATOM(normalize);
	ASFUNCTION_ATOM(project);
	ASFUNCTION_ATOM(scaleBy);
	ASFUNCTION_ATOM(subtract);
	ASFUNCTION_ATOM(setTo);
	ASFUNCTION_ATOM(copyFrom);
	ASFUNCTION_ATOM(_get_length);
	ASFUNCTION_ATOM(_get_lengthSquared);

	ASFUNCTION_ATOM(_toString);
};

}
#endif /* SCRIPTING_FLASH_GEOM_VECTOR3D_H */
