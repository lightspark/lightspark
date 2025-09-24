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

#ifndef SCRIPTING_AVM1_AVM1RECTANGLE_H
#define SCRIPTING_AVM1_AVM1RECTANGLE_H


#include "scripting/flash/geom/Rectangle.h"

namespace lightspark
{

class AVM1Rectangle: public Rectangle
{
private:
	asAtom atomX;
	asAtom atomY;
	asAtom atomWidth;
	asAtom atomHeight;
	void setX(number_t val, ASWorker* wrk);
	void setY(number_t val, ASWorker* wrk);
	void setWidth(number_t val, ASWorker* wrk);
	void setHeight(number_t val, ASWorker* wrk);
public:
	AVM1Rectangle(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate);

	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(AVM1_getLeft);
	ASFUNCTION_ATOM(AVM1_setLeft);
	ASFUNCTION_ATOM(AVM1_setX);
	ASFUNCTION_ATOM(AVM1_getTop);
	ASFUNCTION_ATOM(AVM1_setTop);
	ASFUNCTION_ATOM(AVM1_setY);
	ASFUNCTION_ATOM(AVM1_getHeight);
	ASFUNCTION_ATOM(AVM1_setHeight);
	ASFUNCTION_ATOM(AVM1_getWidth);
	ASFUNCTION_ATOM(AVM1_setWidth);
	ASFUNCTION_ATOM(AVM1_getTopLeft);
	ASFUNCTION_ATOM(AVM1_setTopLeft);
	ASFUNCTION_ATOM(AVM1_getBottomRight);
	ASFUNCTION_ATOM(AVM1_setBottomRight);
	ASFUNCTION_ATOM(AVM1_getSize);
	ASFUNCTION_ATOM(AVM1_setSize);
	ASFUNCTION_ATOM(AVM1_getBottom);
	ASFUNCTION_ATOM(AVM1_setBottom);
	ASFUNCTION_ATOM(AVM1_getRight);
	ASFUNCTION_ATOM(AVM1_setRight);
	ASFUNCTION_ATOM(AVM1_setEmpty);
	ASFUNCTION_ATOM(AVM1_contains);
	ASFUNCTION_ATOM(AVM1_containsPoint);
	ASFUNCTION_ATOM(AVM1_containsRectangle);
	ASFUNCTION_ATOM(AVM1_inflate);
	ASFUNCTION_ATOM(AVM1_inflatePoint);
	ASFUNCTION_ATOM(AVM1_intersection);
	ASFUNCTION_ATOM(AVM1_intersects);
	ASFUNCTION_ATOM(AVM1_offset);
	ASFUNCTION_ATOM(AVM1_offsetPoint);
	ASFUNCTION_ATOM(AVM1_union);
	ASFUNCTION_ATOM(AVM1_isEmpty);
	ASFUNCTION_ATOM(AVM1_toString);

};

}
#endif // SCRIPTING_AVM1_AVM1RECTANGLE_H
