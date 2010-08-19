/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _FLASH_GEOM_H
#define _FLASH_GEOM_H

#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class Rectangle: public ASObject
{
public:
	Rectangle():x(0),y(0),width(0),height(0){}
	number_t x,y,width,height;
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getLeft);
	ASFUNCTION(_setLeft);
	ASFUNCTION(_getRight);
	ASFUNCTION(_setRight);
	ASFUNCTION(_getWidth);
	ASFUNCTION(_setWidth);
	ASFUNCTION(_getTop);
	ASFUNCTION(_setTop);
	ASFUNCTION(_getBottom);
	ASFUNCTION(_setBottom);
	ASFUNCTION(_getHeight);
	ASFUNCTION(_setHeight);
	ASFUNCTION(clone);
	const RECT getRect() const;
};

class Point: public ASObject
{
private:
	number_t x,y;
public:
	Point():x(0),y(0){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getX);
	ASFUNCTION(_getY);
};

class ColorTransform: public ASObject
{
private:
	number_t redMultiplier,greenMultiplier,blueMultiplier,alphaMultiplier;
	number_t redOffset,greenOffset,blueOffset,alphaOffset;
public:
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(setColor);
	ASFUNCTION(getColor);
};

class Transform: public ASObject
{
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};


class Matrix: public ASObject
{
	double a, b, c, d, tx, ty;
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	
	//Overloads
	tiny_string toString(bool debugMsg=false);
	
	ASFUNCTION(_constructor);
	
	//Methods
	ASFUNCTION(identity);
	ASFUNCTION(rotate);
	ASFUNCTION(scale);
	ASFUNCTION(translate);
	
	//Properties
	ASFUNCTION(_get_a);
	ASFUNCTION(_get_b);
	ASFUNCTION(_get_c);
	ASFUNCTION(_get_d);
	ASFUNCTION(_get_tx);
	ASFUNCTION(_get_ty);
	ASFUNCTION(_set_a);
	ASFUNCTION(_set_b);
	ASFUNCTION(_set_c);
	ASFUNCTION(_set_d);
	ASFUNCTION(_set_tx);
	ASFUNCTION(_set_ty);
};

};
#endif
