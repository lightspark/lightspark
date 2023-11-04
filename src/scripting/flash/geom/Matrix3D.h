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

#ifndef SCRIPTING_FLASH_GEOM_MATRIX3D_H
#define SCRIPTING_FLASH_GEOM_MATRIX3D_H 1

#include "asobject.h"

namespace lightspark
{

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
	ASFUNCTION_ATOM(transformVectors);
	ASFUNCTION_ATOM(prepend);
	ASFUNCTION_ATOM(prependRotation);
	ASFUNCTION_ATOM(prependScale);
	ASFUNCTION_ATOM(prependTranslation);
	ASFUNCTION_ATOM(append);
	ASFUNCTION_ATOM(appendTranslation);
	ASFUNCTION_ATOM(appendRotation);
	ASFUNCTION_ATOM(appendScale);
	ASFUNCTION_ATOM(copyColumnTo);
	ASFUNCTION_ATOM(copyColumnFrom);
	ASFUNCTION_ATOM(copyRawDataFrom);
	ASFUNCTION_ATOM(copyRawDataTo);
	ASFUNCTION_ATOM(copyFrom);
	ASFUNCTION_ATOM(copyRowFrom);
	ASFUNCTION_ATOM(copyRowTo);
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

}
#endif /* SCRIPTING_FLASH_GEOM_MATRIX3D_H */
