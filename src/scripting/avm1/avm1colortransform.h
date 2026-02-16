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

#ifndef SCRIPTING_AVM1_AVM1COLORTRANSFORM_H
#define SCRIPTING_AVM1_AVM1COLORTRANSFORM_H


#include "scripting/flash/geom/flashgeom.h"

namespace lightspark
{

class AVM1ColorTransform: public ColorTransform
{
public:
	AVM1ColorTransform(ASWorker* wrk,Class_base* c):ColorTransform(wrk,c)
	{
		subtype=SUBTYPE_AVM1COLORTRANSFORM;
	}
	AVM1ColorTransform(ASWorker* wrk,Class_base* c, const ColorTransformBase& r):ColorTransform(wrk,c,r)
	{
		subtype=SUBTYPE_AVM1COLORTRANSFORM;
	}
	AVM1ColorTransform(ASWorker* wrk,Class_base* c, const CXFORMWITHALPHA& cx):ColorTransform(wrk,c,cx)
	{
		subtype=SUBTYPE_AVM1COLORTRANSFORM;
	}
	static void sinit(Class_base* c);

};

}
#endif // SCRIPTING_AVM1_AVM1COLORTRANSFORM_H
