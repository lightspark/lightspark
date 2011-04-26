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
	const RECT getRect() const;

	// properties
	ASFUNCTION(_getBottom);
	ASFUNCTION(_setBottom);
	ASFUNCTION(_getBottomRight);
	ASFUNCTION(_setBottomRight);
	ASFUNCTION(_getHeight);
	ASFUNCTION(_setHeight);
	ASFUNCTION(_getLeft);
	ASFUNCTION(_setLeft);
	ASFUNCTION(_getRight);
	ASFUNCTION(_setRight);
	ASFUNCTION(_getSize);
	ASFUNCTION(_setSize);
	ASFUNCTION(_getTop);
	ASFUNCTION(_setTop);
	ASFUNCTION(_getTopLeft);
	ASFUNCTION(_setTopLeft);
	ASFUNCTION(_getWidth);
	ASFUNCTION(_setWidth);

	// methods
	ASFUNCTION(_constructor);
	ASFUNCTION(clone);
	ASFUNCTION(contains);
	ASFUNCTION(containsPoint);
	ASFUNCTION(containsRect);
	ASFUNCTION(equals);
	ASFUNCTION(inflate);
	ASFUNCTION(inflatePoint);
	ASFUNCTION(intersection);
	ASFUNCTION(intersects);
	ASFUNCTION(isEmpty);
	ASFUNCTION(offset);
	ASFUNCTION(offsetPoint);
	ASFUNCTION(setEmpty);
	ASFUNCTION(_toString);
	tiny_string toString(bool debugMsg=false);
	ASFUNCTION(_union);
};

class Point: public ASObject
{
private:
	number_t x,y;
public:
	Point(number_t _x = 0, number_t _y = 0):x(_x),y(_y){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	tiny_string toString(bool debugMsg=false);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getX);
	ASFUNCTION(_getY);
	ASFUNCTION(_setX);
	ASFUNCTION(_setY);
	ASFUNCTION(_getlength);
	ASFUNCTION(interpolate);
	ASFUNCTION(distance);
	ASFUNCTION(add);
	ASFUNCTION(subtract);
	ASFUNCTION(clone);
	ASFUNCTION(equals);
	ASFUNCTION(normalize);
	ASFUNCTION(offset);
	ASFUNCTION(polar);
	
	number_t len() const;
	number_t getX() { return x; };
	number_t getY() { return y; };
};

class ColorTransform: public ASObject
{
private:
	number_t redMultiplier,greenMultiplier,blueMultiplier,alphaMultiplier;
	number_t redOffset,greenOffset,blueOffset,alphaOffset;
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(setColor);
	ASFUNCTION(getColor);

	ASFUNCTION(getRedMultiplier);
	ASFUNCTION(setRedMultiplier);
	ASFUNCTION(getGreenMultiplier);
	ASFUNCTION(setGreenMultiplier);
	ASFUNCTION(getBlueMultiplier);
	ASFUNCTION(setBlueMultiplier);
	ASFUNCTION(getAlphaMultiplier);
	ASFUNCTION(setAlphaMultiplier);

	ASFUNCTION(getRedOffset);
	ASFUNCTION(setRedOffset);
	ASFUNCTION(getGreenOffset);
	ASFUNCTION(setGreenOffset);
	ASFUNCTION(getBlueOffset);
	ASFUNCTION(setBlueOffset);
	ASFUNCTION(getAlphaOffset);
	ASFUNCTION(setAlphaOffset);

	ASFUNCTION(concat);

	tiny_string toString(bool debugMsg=false);
};

class Transform: public ASObject
{
public:
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class Matrix: public ASObject
{
public:
	number_t a, b, c, d, tx, ty;
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	void _createBox(number_t scaleX, number_t scaleY, number_t angle, number_t x, number_t y);
	MATRIX getMATRIX() const;

	//Overloads
	tiny_string toString(bool debugMsg=false);
	
	ASFUNCTION(_constructor);
	
	//Methods
	ASFUNCTION(clone);
	ASFUNCTION(concat);
	ASFUNCTION(createBox);
	ASFUNCTION(createGradientBox);
	ASFUNCTION(deltaTransformPoint);
	ASFUNCTION(identity);
	ASFUNCTION(invert);
	ASFUNCTION(rotate);
	ASFUNCTION(scale);
	ASFUNCTION(transformPoint);
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

class Vector3D: public ASObject
{
public:
	number_t w, x, y, z;
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	
	//Overloads
	tiny_string toString(bool debugMsg=false);
	
	ASFUNCTION(_constructor);
	
	//Methods
	ASFUNCTION(add);
	ASFUNCTION(angleBetween);
	ASFUNCTION(clone);
	ASFUNCTION(crossProduct);
	ASFUNCTION(decrementBy);
	ASFUNCTION(distance);
	ASFUNCTION(dotProduct);
	ASFUNCTION(equals);
	ASFUNCTION(incrementBy);
	ASFUNCTION(nearEquals);
	ASFUNCTION(negate);
	ASFUNCTION(normalize);
	ASFUNCTION(project);
	ASFUNCTION(scaleBy);
	ASFUNCTION(subtract);
	
	//Properties
	ASFUNCTION(_get_w);
	ASFUNCTION(_get_x);
	ASFUNCTION(_get_y);
	ASFUNCTION(_get_z);
	ASFUNCTION(_get_length);
	ASFUNCTION(_get_lengthSquared);

	ASFUNCTION(_set_w);
	ASFUNCTION(_set_x);
	ASFUNCTION(_set_y);
	ASFUNCTION(_set_z);
};

};
#endif
