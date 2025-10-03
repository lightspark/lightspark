/**************************************************************************
    Lightspark, a free flash player implementation

	Copyright (C) 2025  Ludger Krämer <dbluelle@onlinehome.de>

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

#ifndef SCRIPTING_AVM1_AVM1POINT_H
#define SCRIPTING_AVM1_AVM1POINT_H


#include "scripting/flash/geom/Point.h"

namespace lightspark
{

class AVM1Point: public Point
{
private:
	asAtom atomX;
	asAtom atomY;
public:
	AVM1Point(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;

	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(AVM1_getX);
	ASFUNCTION_ATOM(AVM1_setX);
	ASFUNCTION_ATOM(AVM1_getY);
	ASFUNCTION_ATOM(AVM1_setY);
	ASFUNCTION_ATOM(AVM1_interpolate);
	ASFUNCTION_ATOM(AVM1_distance);
	ASFUNCTION_ATOM(AVM1_normalize);
	ASFUNCTION_ATOM(AVM1_offset);
	ASFUNCTION_ATOM(AVM1_polar);
	ASFUNCTION_ATOM(AVM1_toString);

	void setX(asAtom val, ASWorker* wrk, bool incref);
	void setY(asAtom val, ASWorker* wrk, bool incref);
	asAtom getAtomX() { return atomX; }
	asAtom getAtomY() { return atomY; }
};

}
#endif // SCRIPTING_AVM1_AVM1POINT_H
