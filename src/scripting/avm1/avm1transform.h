/**************************************************************************
    Lightspark, a free flash player implementation

	Copyright (C) 2026  Ludger Krämer <dbluelle@onlinehome.de>

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

#ifndef SCRIPTING_AVM1_AVM1TRANSFORM_H
#define SCRIPTING_AVM1_AVM1TRANSFORM_H


#include "scripting/flash/geom/flashgeom.h"

namespace lightspark
{

class AVM1Transform: public Transform
{
public:
	AVM1Transform(ASWorker* wrk,Class_base* c);
	AVM1Transform(ASWorker* wrk, Class_base* c, DisplayObject* o);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(AVM1_getColorTransform);
};

}
#endif // SCRIPTING_AVM1_AVM1TRANSFORM_H
