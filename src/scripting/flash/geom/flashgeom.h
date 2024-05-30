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
#include "backends/graphics.h"

namespace lightspark
{
class BitmapContainer;
class Point;

class ColorTransform: public ASObject, public ColorTransformBase
{
friend class Bitmap;
friend class BitmapData;
friend class BitmapContainer;
friend class DisplayObject;
friend class AVM1Color;
friend class TokenContainer;
friend class TextField;
public:
	ColorTransform(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_COLORTRANSFORM)
	{}
	ColorTransform(ASWorker* wrk,Class_base* c, const ColorTransformBase& r);
	ColorTransform(ASWorker* wrk,Class_base* c, const CXFORMWITHALPHA& cx);
	ColorTransform& operator=(const ColorTransform& r)
	{
		ColorTransformBase::operator=(r);
		return *this;
	}
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

class PerspectiveProjection: public ASObject
{
public:
	PerspectiveProjection(ASWorker* wrk,Class_base* c);
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
