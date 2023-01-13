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

#ifndef SCRIPTING_FLASH_FLASHGEOM_H
#define SCRIPTING_FLASH_FLASHGEOM_H 1

#include "compat.h"
#include "asobject.h"

namespace lightspark
{
class BitmapContainer;

class Rectangle: public ASObject
{
public:
	Rectangle(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_RECTANGLE),x(0),y(0),width(0),height(0){}
	number_t x,y,width,height;
	static void sinit(Class_base* c);
	const RECT getRect() const;
	bool destruct() override;
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

class ColorTransform: public ASObject
{
friend class Bitmap;
friend class BitmapData;
friend class DisplayObject;
friend class AVM1Color;
friend class TokenContainer;
friend class TextField;
protected:
	number_t redMultiplier,greenMultiplier,blueMultiplier,alphaMultiplier;
	number_t redOffset,greenOffset,blueOffset,alphaOffset;
public:
	ColorTransform(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_COLORTRANSFORM)
	  ,redMultiplier(1.0),greenMultiplier(1.0),blueMultiplier(1.0),alphaMultiplier(1.0)
	  ,redOffset(0.0),greenOffset(0.0),blueOffset(0.0),alphaOffset(0.0)
	{}
	ColorTransform(ASWorker* wrk,Class_base* c, const CXFORMWITHALPHA& cx);
	// returning r,g,b,a values are between 0.0 and 1.0
	void applyTransformation(const RGBA &color, float& r, float& g, float& b, float &a);
	uint8_t* applyTransformation(BitmapContainer* bm);
	void applyTransformation(uint8_t* bm, uint32_t size);
	void setProperties(const CXFORMWITHALPHA& cx);
	static void sinit(Class_base* c);
	bool destruct() override;
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(setColor);
	ASFUNCTION_ATOM(getColor);

	ASFUNCTION_ATOM(getRedMultiplier);
	ASFUNCTION_ATOM(setRedMultiplier);
	ASFUNCTION_ATOM(getGreenMultiplier);
	ASFUNCTION_ATOM(setGreenMultiplier);
	ASFUNCTION_ATOM(getBlueMultiplier);
	ASFUNCTION_ATOM(setBlueMultiplier);
	ASFUNCTION_ATOM(getAlphaMultiplier);
	ASFUNCTION_ATOM(setAlphaMultiplier);

	ASFUNCTION_ATOM(getRedOffset);
	ASFUNCTION_ATOM(setRedOffset);
	ASFUNCTION_ATOM(getGreenOffset);
	ASFUNCTION_ATOM(setGreenOffset);
	ASFUNCTION_ATOM(getBlueOffset);
	ASFUNCTION_ATOM(setBlueOffset);
	ASFUNCTION_ATOM(getAlphaOffset);
	ASFUNCTION_ATOM(setAlphaOffset);

	ASFUNCTION_ATOM(concat);
	ASFUNCTION_ATOM(_toString);
};

class Matrix: public ASObject
{
friend class DisplayObject;
private:
	MATRIX matrix;
public:
	Matrix(ASWorker* wrk,Class_base* c);
	Matrix(ASWorker* wrk,Class_base* c,const MATRIX& m);
	static void sinit(Class_base* c);
	void _createBox(number_t scaleX, number_t scaleY, number_t angle, number_t x, number_t y);
	MATRIX getMATRIX() const;
	bool destruct() override;
	ASFUNCTION_ATOM(_constructor);
	
	//Methods
	ASFUNCTION_ATOM(clone);
	ASFUNCTION_ATOM(concat);
	ASFUNCTION_ATOM(copyFrom);
	ASFUNCTION_ATOM(createBox);
	ASFUNCTION_ATOM(createGradientBox);
	ASFUNCTION_ATOM(deltaTransformPoint);
	ASFUNCTION_ATOM(identity);
	ASFUNCTION_ATOM(invert);
	ASFUNCTION_ATOM(rotate);
	ASFUNCTION_ATOM(scale);
	ASFUNCTION_ATOM(setTo);
	ASFUNCTION_ATOM(transformPoint);
	ASFUNCTION_ATOM(translate);
	ASFUNCTION_ATOM(_toString);
	
	//Properties
	ASFUNCTION_ATOM(_get_a);
	ASFUNCTION_ATOM(_get_b);
	ASFUNCTION_ATOM(_get_c);
	ASFUNCTION_ATOM(_get_d);
	ASFUNCTION_ATOM(_get_tx);
	ASFUNCTION_ATOM(_get_ty);
	ASFUNCTION_ATOM(_set_a);
	ASFUNCTION_ATOM(_set_b);
	ASFUNCTION_ATOM(_set_c);
	ASFUNCTION_ATOM(_set_d);
	ASFUNCTION_ATOM(_set_tx);
	ASFUNCTION_ATOM(_set_ty);
};

class DisplayObject;
class PerspectiveProjection;
class Matrix3D;
class Transform: public ASObject
{
friend class DisplayObject;
private:
	_NR<DisplayObject> owner;
	void onSetMatrix3D(_NR<Matrix3D> oldValue);
public:
	Transform(ASWorker* wrk,Class_base* c);
	Transform(ASWorker* wrk, Class_base* c, _R<DisplayObject> o);
	ASFUNCTION_ATOM(_constructor);
	static void sinit(Class_base* c);
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	ASFUNCTION_ATOM(_getColorTransform);
	ASFUNCTION_ATOM(_setColorTransform);
	ASFUNCTION_ATOM(_getMatrix);
	ASFUNCTION_ATOM(_setMatrix);
	ASFUNCTION_ATOM(_getConcatenatedMatrix);
	ASPROPERTY_GETTER_SETTER(_NR<PerspectiveProjection>, perspectiveProjection);
	ASPROPERTY_GETTER_SETTER(_NR<Matrix3D>, matrix3D);

};

class Vector3D: public ASObject
{
public:
	Vector3D(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_VECTOR3D),w(0),x(0),y(0),z(0){}
	number_t w, x, y, z;
	static void sinit(Class_base* c);
	bool destruct() override;
	ASFUNCTION_ATOM(_constructor);
	
	//Methods
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

	//Properties
	ASFUNCTION_ATOM(_get_w);
	ASFUNCTION_ATOM(_get_x);
	ASFUNCTION_ATOM(_get_y);
	ASFUNCTION_ATOM(_get_z);
	ASFUNCTION_ATOM(_get_length);
	ASFUNCTION_ATOM(_get_lengthSquared);

	ASFUNCTION_ATOM(_set_w);
	ASFUNCTION_ATOM(_set_x);
	ASFUNCTION_ATOM(_set_y);
	ASFUNCTION_ATOM(_set_z);
	ASFUNCTION_ATOM(_toString);
};

class Matrix3D: public ASObject
{
private:
	number_t data[4*4];
	void append(number_t* otherdata);
	void prepend(number_t *otherdata);
	number_t getDeterminant();
	void identity();
public:
	Matrix3D(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_MATRIX3D){}
	static void sinit(Class_base* c);
	bool destruct() override;
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(clone);
	ASFUNCTION_ATOM(decompose);
	ASFUNCTION_ATOM(recompose);
	ASFUNCTION_ATOM(deltaTransformVector);
	ASFUNCTION_ATOM(prepend);
	ASFUNCTION_ATOM(prependScale);
	ASFUNCTION_ATOM(prependTranslation);
	ASFUNCTION_ATOM(append);
	ASFUNCTION_ATOM(appendTranslation);
	ASFUNCTION_ATOM(appendRotation);
	ASFUNCTION_ATOM(appendScale);
	ASFUNCTION_ATOM(copyColumnTo);
	ASFUNCTION_ATOM(copyRawDataFrom);
	ASFUNCTION_ATOM(copyRawDataTo);
	ASFUNCTION_ATOM(copyFrom);
	ASFUNCTION_ATOM(copyToMatrix3D);
	ASFUNCTION_ATOM(_identity);
	ASFUNCTION_ATOM(invert);
	ASFUNCTION_ATOM(_get_rawData);
	ASFUNCTION_ATOM(_set_rawData);
	ASFUNCTION_ATOM(_get_determinant);
	ASFUNCTION_ATOM(_get_position);
	ASFUNCTION_ATOM(_set_position);
	ASFUNCTION_ATOM(transformVector);
	void getRowAsFloat(uint32_t rownum,float* rowdata);
	void getColumnAsFloat(uint32_t rownum,float* rowdata);
	void getRawDataAsFloat(float* rowdata);
};
class PerspectiveProjection: public ASObject
{
public:
	PerspectiveProjection(ASWorker* wrk,Class_base* c):ASObject(wrk,c),fieldOfView(0),focalLength(0) {}
	static void sinit(Class_base* c);
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(number_t, fieldOfView);
	ASPROPERTY_GETTER_SETTER(number_t, focalLength);
	ASPROPERTY_GETTER_SETTER(_NR<Point>, projectionCenter);
};

}
#endif /* SCRIPTING_FLASH_FLASHGEOM_H */
