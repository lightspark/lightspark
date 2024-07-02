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

#include "scripting/abc.h"
#include "scripting/flash/geom/Matrix3D.h"
#include "scripting/flash/geom/Vector3D.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"

using namespace lightspark;
using namespace std;

void Matrix3D::append(number_t *otherdata)
{
	number_t m111 = data[0];
	number_t m121 = data[4];
	number_t m131 = data[8];
	number_t m141 = data[12];

	number_t m112 = data[1];
	number_t m122 = data[5];
	number_t m132 = data[9];
	number_t m142 = data[13];

	number_t m113 = data[2];
	number_t m123 = data[6];
	number_t m133 = data[10];
	number_t m143 = data[14];

	number_t m114 = data[3];
	number_t m124 = data[7];
	number_t m134 = data[11];
	number_t m144 = data[15];

	number_t m211 = otherdata[0];
	number_t m221 = otherdata[4];
	number_t m231 = otherdata[8];
	number_t m241 = otherdata[12];

	number_t m212 = otherdata[1];
	number_t m222 = otherdata[5];
	number_t m232 = otherdata[9];
	number_t m242 = otherdata[13];

	number_t m213 = otherdata[2];
	number_t m223 = otherdata[6];
	number_t m233 = otherdata[10];
	number_t m243 = otherdata[14];

	number_t m214 = otherdata[3];
	number_t m224 = otherdata[7];
	number_t m234 = otherdata[11];
	number_t m244 = otherdata[15];

	data[0] = m111 * m211 + m112 * m221 + m113 * m231 + m114 * m241;
	data[1] = m111 * m212 + m112 * m222 + m113 * m232 + m114 * m242;
	data[2] = m111 * m213 + m112 * m223 + m113 * m233 + m114 * m243;
	data[3] = m111 * m214 + m112 * m224 + m113 * m234 + m114 * m244;

	data[4] = m121 * m211 + m122 * m221 + m123 * m231 + m124 * m241;
	data[5] = m121 * m212 + m122 * m222 + m123 * m232 + m124 * m242;
	data[6] = m121 * m213 + m122 * m223 + m123 * m233 + m124 * m243;
	data[7] = m121 * m214 + m122 * m224 + m123 * m234 + m124 * m244;

	data[8] = m131 * m211 + m132 * m221 + m133 * m231 + m134 * m241;
	data[9] = m131 * m212 + m132 * m222 + m133 * m232 + m134 * m242;
	data[10] = m131 * m213 + m132 * m223 + m133 * m233 + m134 * m243;
	data[11] = m131 * m214 + m132 * m224 + m133 * m234 + m134 * m244;

	data[12] = m141 * m211 + m142 * m221 + m143 * m231 + m144 * m241;
	data[13] = m141 * m212 + m142 * m222 + m143 * m232 + m144 * m242;
	data[14] = m141 * m213 + m142 * m223 + m143 * m233 + m144 * m243;
	data[15] = m141 * m214 + m142 * m224 + m143 * m234 + m144 * m244;
}
void Matrix3D::prepend(number_t *otherdata)
{
	number_t m111 = otherdata[0];
	number_t m121 = otherdata[4];
	number_t m131 = otherdata[8];
	number_t m141 = otherdata[12];

	number_t m112 = otherdata[1];
	number_t m122 = otherdata[5];
	number_t m132 = otherdata[9];
	number_t m142 = otherdata[13];

	number_t m113 = otherdata[2];
	number_t m123 = otherdata[6];
	number_t m133 = otherdata[10];
	number_t m143 = otherdata[14];

	number_t m114 = otherdata[3];
	number_t m124 = otherdata[7];
	number_t m134 = otherdata[11];
	number_t m144 = otherdata[15];


	number_t m211 = data[0];
	number_t m221 = data[4];
	number_t m231 = data[8];
	number_t m241 = data[12];

	number_t m212 = data[1];
	number_t m222 = data[5];
	number_t m232 = data[9];
	number_t m242 = data[13];

	number_t m213 = data[2];
	number_t m223 = data[6];
	number_t m233 = data[10];
	number_t m243 = data[14];

	number_t m214 = data[3];
	number_t m224 = data[7];
	number_t m234 = data[11];
	number_t m244 = data[15];

	data[0] = m111 * m211 + m112 * m221 + m113 * m231 + m114 * m241;
	data[1] = m111 * m212 + m112 * m222 + m113 * m232 + m114 * m242;
	data[2] = m111 * m213 + m112 * m223 + m113 * m233 + m114 * m243;
	data[3] = m111 * m214 + m112 * m224 + m113 * m234 + m114 * m244;
	
	data[4] = m121 * m211 + m122 * m221 + m123 * m231 + m124 * m241;
	data[5] = m121 * m212 + m122 * m222 + m123 * m232 + m124 * m242;
	data[6] = m121 * m213 + m122 * m223 + m123 * m233 + m124 * m243;
	data[7] = m121 * m214 + m122 * m224 + m123 * m234 + m124 * m244;
	
	data[8] = m131 * m211 + m132 * m221 + m133 * m231 + m134 * m241;
	data[9] = m131 * m212 + m132 * m222 + m133 * m232 + m134 * m242;
	data[10] = m131 * m213 + m132 * m223 + m133 * m233 + m134 * m243;
	data[11] = m131 * m214 + m132 * m224 + m133 * m234 + m134 * m244;
	
	data[12] = m141 * m211 + m142 * m221 + m143 * m231 + m144 * m241;
	data[13] = m141 * m212 + m142 * m222 + m143 * m232 + m144 * m242;
	data[14] = m141 * m213 + m142 * m223 + m143 * m233 + m144 * m243;
	data[15] = m141 * m214 + m142 * m224 + m143 * m234 + m144 * m244;
}
number_t Matrix3D::getDeterminant()
{
	return 1 * ((data[0] * data[5] - data[4] * data[1]) * (data[10] * data[15] - data[14] * data[11])
			- (data[0] * data[9] - data[8] * data[1]) * (data[6] * data[15] - data[14] * data[7])
			+ (data[0] * data[13] - data[12] * data[1]) * (data[6] * data[11] - data[10] * data[7])
			+ (data[4] * data[9] - data[8] * data[5]) * (data[2] * data[15] - data[14] * data[3])
			- (data[4] * data[13] - data[12] * data[5]) * (data[2] * data[11] - data[10] * data[3])
			+ (data[8] * data[13] - data[12] * data[9]) * (data[2] * data[7] - data[6] * data[3]));
}

void Matrix3D::identity()
{
	// Identity Matrix
	uint32_t i = 0;
	data[i++] = 1.0;
	data[i++] = 0.0;
	data[i++] = 0.0;
	data[i++] = 0.0;

	data[i++] = 0.0;
	data[i++] = 1.0;
	data[i++] = 0.0;
	data[i++] = 0.0;

	data[i++] = 0.0;
	data[i++] = 0.0;
	data[i++] = 1.0;
	data[i++] = 0.0;

	data[i++] = 0.0;
	data[i++] = 0.0;
	data[i++] = 0.0;
	data[i++] = 1.0;
}

void Matrix3D::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<Matrix3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("recompose","",c->getSystemState()->getBuiltinFunction(recompose,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("decompose","",c->getSystemState()->getBuiltinFunction(decompose,0,Class<Vector>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("deltaTransformVector","",c->getSystemState()->getBuiltinFunction(deltaTransformVector,1,Class<Vector3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transformVectors","",c->getSystemState()->getBuiltinFunction(transformVectors),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prepend","",c->getSystemState()->getBuiltinFunction(prepend),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prependRotation","",c->getSystemState()->getBuiltinFunction(prependRotation),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prependScale","",c->getSystemState()->getBuiltinFunction(prependScale),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prependTranslation","",c->getSystemState()->getBuiltinFunction(prependTranslation),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("append","",c->getSystemState()->getBuiltinFunction(append),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendTranslation","",c->getSystemState()->getBuiltinFunction(appendTranslation),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendRotation","",c->getSystemState()->getBuiltinFunction(appendRotation),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendScale","",c->getSystemState()->getBuiltinFunction(appendScale),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyColumnFrom","",c->getSystemState()->getBuiltinFunction(copyColumnFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyColumnTo","",c->getSystemState()->getBuiltinFunction(copyColumnTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",c->getSystemState()->getBuiltinFunction(copyFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyRowFrom","",c->getSystemState()->getBuiltinFunction(copyRowFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyRowTo","",c->getSystemState()->getBuiltinFunction(copyRowTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyToMatrix3D","",c->getSystemState()->getBuiltinFunction(copyToMatrix3D),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyRawDataFrom","",c->getSystemState()->getBuiltinFunction(copyRawDataFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyRawDataTo","",c->getSystemState()->getBuiltinFunction(copyRawDataTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("identity","",c->getSystemState()->getBuiltinFunction(_identity),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("invert","",c->getSystemState()->getBuiltinFunction(invert,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("rawData","",c->getSystemState()->getBuiltinFunction(_get_rawData,0,Class<Vector>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("rawData","",c->getSystemState()->getBuiltinFunction(_set_rawData),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("determinant","",c->getSystemState()->getBuiltinFunction(_get_determinant,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",c->getSystemState()->getBuiltinFunction(_set_position),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",c->getSystemState()->getBuiltinFunction(_get_position,0,Class<Vector3D>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("transformVector","",c->getSystemState()->getBuiltinFunction(transformVector,2,Class<Vector3D>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
}

bool Matrix3D::destruct()
{
	return destructIntern();
}

void Matrix3D::getRowAsFloat(uint32_t rownum, float *rowdata)
{
	rowdata[0] = data[rownum];
	rowdata[1] = data[rownum+4];
	rowdata[2] = data[rownum+8];
	rowdata[3] = data[rownum+12];
}
void Matrix3D::getColumnAsFloat(uint32_t column, float *rowdata)
{
	rowdata[0] = data[column*4];
	rowdata[1] = data[column*4+1];
	rowdata[2] = data[column*4+2];
	rowdata[3] = data[column*4+3];
}

void Matrix3D::getRawDataAsFloat(float *rowdata)
{
	for (uint32_t i = 0; i < 16; i++)
		rowdata[i] = data[i];
}

ASFUNCTIONBODY_ATOM(Matrix3D,_constructor)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector> v;
	ARG_CHECK(ARG_UNPACK(v,NullRef));
	th->identity();
	if (!v.isNull() && v->size()==4*4)
	{
		for (uint32_t i = 0; i < 4*4; i++)
		{
			asAtom a = v->at(i);
			th->data[i] = asAtomHandler::toNumber(a);
		}
	}
}
ASFUNCTIONBODY_ATOM(Matrix3D,clone)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	Matrix3D * res = Class<Matrix3D>::getInstanceS(wrk);
	memcpy(res->data,th->data,4*4*sizeof(number_t));
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Matrix3D,recompose)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector> components;
	tiny_string orientationStyle;
	ARG_CHECK(ARG_UNPACK(components)(orientationStyle, "eulerAngles"));
	if (orientationStyle != "eulerAngles" && orientationStyle != "axisAngle" && orientationStyle != "quaternion")
	{
		tiny_string msg("Invalid orientation style ") ;
		msg +=orientationStyle;
		msg +=".  Value must be one of 'Orientation3D.EULER_ANGLES', 'Orientation3D.AXIS_ANGLE', or 'Orientation3D.QUATERNION'";
		createErrorWithMessage<ASError>(wrk,2187,msg);
		return;
	}

	asAtomHandler::setBool(ret,false);
	if (components.isNull() || !components->getType()->isSubClass(Class<Vector3D>::getRef(wrk->getSystemState()).getPtr()) || components->size() < 3)
		return;
	asAtom tmp;
	tmp = components->at(0);
	Vector3D* v0 = asAtomHandler::as<Vector3D>(tmp);
	tmp = components->at(1);
	Vector3D* v1 = asAtomHandler::as<Vector3D>(tmp);
	tmp = components->at(2);
	Vector3D* v2 = asAtomHandler::as<Vector3D>(tmp);
	th->identity();
	float scale[16];
	scale[0] = scale[1] = scale[2] = v2->x;
	scale[4] = scale[5] = scale[6] = v2->y;
	scale[8] = scale[9] = scale[10] = v2->z;

	
	if (orientationStyle == "eulerAngles")
	{
		number_t cx = cos(v1->x);
		number_t cy = cos(v1->y);
		number_t cz = cos(v1->z);
		number_t sx = sin(v1->x);
		number_t sy = sin(v1->y);
		number_t sz = sin(v1->z);
		th->data[0] = cy * cz * scale[0];
		th->data[1] = cy * sz * scale[1];
		th->data[2] = -sy * scale[2];
		th->data[3] = 0;
		th->data[4] = (sx * sy * cz - cx * sz) * scale[4];
		th->data[5] = (sx * sy * sz + cx * cz) * scale[5];
		th->data[6] = sx * cy * scale[6];
		th->data[7] = 0;
		th->data[8] = (cx * sy * cz + sx * sz) * scale[8];
		th->data[9] = (cx * sy * sz - sx * cz) * scale[9];
		th->data[10] = cx * cy * scale[10];
		th->data[11] = 0;
		th->data[12] = v0->x;
		th->data[13] = v0->y;
		th->data[14] = v0->z;
		th->data[15] = 1;
	}
	else if (orientationStyle == "axisAngle" || orientationStyle == "quaternion")
	{
		number_t x = v1->x;
		number_t y = v1->y;
		number_t z = v1->z;
		number_t w = v1->w;
		if (orientationStyle == "axisAngle")
		{
			x *= sin(w / 2);
			y *= sin(w / 2);
			z *= sin(w / 2);
			w = cos(w / 2);
		}
		th->data[0] = (1 - 2 * y * y - 2 * z * z) * scale[0];
		th->data[1] = (2 * x * y + 2 * w * z) * scale[1];
		th->data[2] = (2 * x * z - 2 * w * y) * scale[2];
		th->data[3] = 0;
		th->data[4] = (2 * x * y - 2 * w * z) * scale[4];
		th->data[5] = (1 - 2 * x * x - 2 * z * z) * scale[5];
		th->data[6] = (2 * y * z + 2 * w * x) * scale[6];
		th->data[7] = 0;
		th->data[8] = (2 * x * z + 2 * w * y) * scale[8];
		th->data[9] = (2 * y * z - 2 * w * x) * scale[9];
		th->data[10] = (1 - 2 * x * x - 2 * y * y) * scale[10];
		th->data[11] = 0;
		th->data[12] = v0->x;
		th->data[13] = v0->y;
		th->data[14] = v0->z;
		th->data[15] = 1;
	}
	asAtomHandler::setBool(ret,!(v2->x == 0 || v2->y == 0 || v2->z == 0));
}
ASFUNCTIONBODY_ATOM(Matrix3D,decompose)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	tiny_string orientationStyle;
	ARG_CHECK(ARG_UNPACK(orientationStyle, "eulerAngles"));
	if (orientationStyle != "eulerAngles" && orientationStyle != "axisAngle" && orientationStyle != "quaternion")
	{
		tiny_string msg("Invalid orientation style ") ;
		msg +=orientationStyle;
		msg +=".  Value must be one of 'Orientation3D.EULER_ANGLES', 'Orientation3D.AXIS_ANGLE', or 'Orientation3D.QUATERNION'";
		createErrorWithMessage<ASError>(wrk,2187,msg);
		return;
	}

	asAtom v=asAtomHandler::invalidAtom;
	RootMovieClip* root = wrk->rootClip.getPtr();
	Template<Vector>::getInstanceS(wrk,v,root,Class<Vector3D>::getClass(wrk->getSystemState()),NullRef);
	Vector *result = asAtomHandler::as<Vector>(v);

	// algorithm taken from openFL
	float mr[16];
	th->getRawDataAsFloat(mr);

	Vector3D* pos =  Class<Vector3D>::getInstanceSNoArgs(wrk);
	pos->x=mr[12];
	pos->y=mr[13];
	pos->z=mr[14];
	mr[12] = 0;
	mr[13] = 0;
	mr[14] = 0;

	Vector3D* scale =  Class<Vector3D>::getInstanceSNoArgs(wrk);

	scale->x = sqrt(mr[0] * mr[0] + mr[1] * mr[1] + mr[2] * mr[2]);
	scale->y = sqrt(mr[4] * mr[4] + mr[5] * mr[5] + mr[6] * mr[6]);
	scale->z = sqrt(mr[8] * mr[8] + mr[9] * mr[9] + mr[10] * mr[10]);

	if (mr[0] * (mr[5] * mr[10] - mr[6] * mr[9]) - mr[1] * (mr[4] * mr[10] - mr[6] * mr[8]) + mr[2] * (mr[4] * mr[9] - mr[5] * mr[8]) < 0)
		scale->z = -scale->z;

	mr[0] /= scale->x;
	mr[1] /= scale->x;
	mr[2] /= scale->x;
	mr[4] /= scale->y;
	mr[5] /= scale->y;
	mr[6] /= scale->y;
	mr[8] /= scale->z;
	mr[9] /= scale->z;
	mr[10] /= scale->z;

	Vector3D* rot =  Class<Vector3D>::getInstanceSNoArgs(wrk);

	if (orientationStyle == "axisAngle")
	{
		rot->w = acos((mr[0] + mr[5] + mr[10] - 1) / 2);
		float len = sqrt((mr[6] - mr[9]) * (mr[6] - mr[9]) + (mr[8] - mr[2]) * (mr[8] - mr[2]) + (mr[1] - mr[4]) * (mr[1] - mr[4]));
		if (len != 0)
		{
			rot->x = (mr[6] - mr[9]) / len;
			rot->y = (mr[8] - mr[2]) / len;
			rot->z = (mr[1] - mr[4]) / len;
		}
		else
		{
			rot->x = rot->y = rot->z = 0;
		}
	}
	else if (orientationStyle == "quaternion")
	{
		float tr = mr[0] + mr[5] + mr[10];
		if (tr > 0)
		{
			rot->w = sqrt(1 + tr) / 2;
			rot->x = (mr[6] - mr[9]) / (4 * rot->w);
			rot->y = (mr[8] - mr[2]) / (4 * rot->w);
			rot->z = (mr[1] - mr[4]) / (4 * rot->w);
		}
		else if ((mr[0] > mr[5]) && (mr[0] > mr[10]))
		{
			rot->x = sqrt(1 + mr[0] - mr[5] - mr[10]) / 2;
			rot->w = (mr[6] - mr[9]) / (4 * rot->x);
			rot->y = (mr[1] + mr[4]) / (4 * rot->x);
			rot->z = (mr[8] + mr[2]) / (4 * rot->x);
		}
		else if (mr[5] > mr[10])
		{
			rot->y = sqrt(1 + mr[5] - mr[0] - mr[10]) / 2;
			rot->x = (mr[1] + mr[4]) / (4 * rot->y);
			rot->w = (mr[8] - mr[2]) / (4 * rot->y);
			rot->z = (mr[6] + mr[9]) / (4 * rot->y);
		}
		else
		{
			rot->z = sqrt(1 + mr[10] - mr[0] - mr[5]) / 2;
			rot->x = (mr[8] + mr[2]) / (4 * rot->z);
			rot->y = (mr[6] + mr[9]) / (4 * rot->z);
			rot->w = (mr[1] - mr[4]) / (4 * rot->z);
		}
	}
	else if (orientationStyle == "eulerAngles")
	{
		rot->y = asin(-mr[2]);
		if (mr[2] != 1 && mr[2] != -1)
		{
			rot->x = atan2(mr[6], mr[10]);
			rot->z = atan2(mr[1], mr[0]);
		}
		else
		{
			rot->z = 0;
			rot->x = atan2(mr[4], mr[5]);
		}
	}
	asAtom a;
	a = asAtomHandler::fromObjectNoPrimitive(pos);
	result->append(a);
	a = asAtomHandler::fromObjectNoPrimitive(rot);
	result->append(a);
	a = asAtomHandler::fromObjectNoPrimitive(scale);
	result->append(a);
	ret = asAtomHandler::fromObjectNoPrimitive(result);
}
ASFUNCTIONBODY_ATOM(Matrix3D,deltaTransformVector)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector3D> v;
	ARG_CHECK(ARG_UNPACK(v));
	Vector3D* result = Class<Vector3D>::getInstanceSNoArgs(wrk);
	result->x = v->x * th->data[0] + v->y * th->data[4] + v->z * th->data[8];
	result->y = v->x * th->data[1] + v->y * th->data[5] + v->z * th->data[9];
	result->z = v->x * th->data[2] + v->y * th->data[6] + v->z * th->data[10];
	result->w = v->x * th->data[3] + v->y * th->data[7] + v->z * th->data[11];
	
	ret = asAtomHandler::fromObjectNoPrimitive(result);
}

ASFUNCTIONBODY_ATOM(Matrix3D,transformVectors)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector> vin;
	_NR<Vector> vout;
	ARG_CHECK(ARG_UNPACK(vin)(vout));
	if (vin.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError,"vin");
		return;
	}
	if (vout.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError,"vout");
		return;
	}
	if (!vout->ensureLength(vin->size()-vin->size()%3))
		return;
	uint32_t i = 0;
	while (i + 3 <= vin->size())
	{
		number_t x = asAtomHandler::toNumber(vin->at(i));
		number_t y = asAtomHandler::toNumber(vin->at(i+1));
		number_t z = asAtomHandler::toNumber(vin->at(i+2));
		asAtom xout = asAtomHandler::fromNumber(wrk, x * th->data[0] + y * th->data[4] + z * th->data[8] + th->data[12],false);
		asAtom yout = asAtomHandler::fromNumber(wrk, x * th->data[1] + y * th->data[5] + z * th->data[9] + th->data[13],false);
		asAtom zout = asAtomHandler::fromNumber(wrk, x * th->data[2] + y * th->data[6] + z * th->data[10] + th->data[14],false);
		vout->set(i,xout);
		vout->set(i+1,yout);
		vout->set(i+2,zout);
		i += 3;
	}
}
ASFUNCTIONBODY_ATOM(Matrix3D,prepend)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Matrix3D> rhs;
	ARG_CHECK(ARG_UNPACK(rhs));
	if (rhs.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"rhs");
		return;
	}
	th->prepend(rhs->data);
}
ASFUNCTIONBODY_ATOM(Matrix3D,prependRotation)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	number_t degrees;
	_NR<Vector3D> axis;
	_NR<Vector3D> pivotPoint;
	ARG_CHECK(ARG_UNPACK(degrees) (axis) (pivotPoint,NullRef));
	number_t tx=0;
	number_t ty=0;
	number_t tz=0;
	if (!pivotPoint.isNull())
	{
		tx = pivotPoint->x;
		ty = pivotPoint->y;
		tz = pivotPoint->z;
	}
	number_t radian = degrees * M_PI / 180.0;
	number_t c = cos(radian);
	number_t s = sin(radian);
	number_t x = axis->x;
	number_t y = axis->y;
	number_t z = axis->z;
	number_t x2 = x * x;
	number_t y2 = y * y;
	number_t z2 = z * z;
	number_t ls = x2 + y2 + z2;
	if (ls != 0.0)
	{
		number_t l = sqrt(ls);
		x /= l;
		y /= l;
		z /= l;
		x2 /= ls;
		y2 /= ls;
		z2 /= ls;
	}
	number_t cc = 1 - c;
	number_t m[16];
	uint32_t i = 0;
	m[i++] = x2 + (y2 + z2) * c;
	m[i++] = x * y * cc + z * s;
	m[i++] = x * z * cc - y * s;
	m[i++] = 0.0;

	m[i++] =  x * y * cc - z * s;
	m[i++] = y2 + (x2 + z2) * c;
	m[i++] = y * z * cc + x * s;
	m[i++] = 0.0;

	m[i++] = x * z * cc + y * s;
	m[i++] = y * z * cc - x * s;
	m[i++] = z2 + (x2 + y2) * c;
	m[i++] = 0.0;

	m[i++] = (tx * (y2 + z2) - x * (ty * y + tz * z)) * cc + (ty * z - tz * y) * s;
	m[i++] = (ty * (x2 + z2) - y * (tx * x + tz * z)) * cc + (tz * x - tx * z) * s;
	m[i++] = (tz * (x2 + y2) - z * (tx * x + ty * y)) * cc + (tx * y - ty * x) * s;
	m[i++] = 1.0;

	th->prepend(m);
}
ASFUNCTIONBODY_ATOM(Matrix3D,prependScale)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	number_t xScale, yScale, zScale;
	ARG_CHECK(ARG_UNPACK(xScale) (yScale) (zScale));
	
	number_t m[16];
	uint32_t i = 0;
	m[i++] = xScale;
	m[i++] = 0.0;
	m[i++] = 0.0;
	m[i++] = 0.0;

	m[i++] = 0.0;
	m[i++] = yScale;
	m[i++] = 0.0;
	m[i++] = 0.0;

	m[i++] = 0.0;
	m[i++] = 0.0;
	m[i++] = zScale;
	m[i++] = 0.0;

	m[i++] = 0.0;
	m[i++] = 0.0;
	m[i++] = 0.0;
	m[i++] = 1.0;
	
	th->prepend(m);
}
ASFUNCTIONBODY_ATOM(Matrix3D,prependTranslation)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	number_t x, y, z;
	ARG_CHECK(ARG_UNPACK(x) (y) (z));
	
	number_t m[16];
	uint32_t i = 0;
	m[i++] = 1.0;
	m[i++] = 0.0;
	m[i++] = 0.0;
	m[i++] = 0.0;

	m[i++] = 0.0;
	m[i++] = 1.0;
	m[i++] = 0.0;
	m[i++] = 0.0;

	m[i++] = 0.0;
	m[i++] = 0.0;
	m[i++] = 1.0;
	m[i++] = 0.0;

	m[i++] = x;
	m[i++] = y;
	m[i++] = z;
	m[i++] = 1.0;
	
	th->prepend(m);
}
ASFUNCTIONBODY_ATOM(Matrix3D,append)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Matrix3D> lhs;
	ARG_CHECK(ARG_UNPACK(lhs));
	if (lhs.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"lhs");
		return;
	}
	th->append(lhs->data);
}
ASFUNCTIONBODY_ATOM(Matrix3D,appendTranslation)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	number_t x, y, z;
	ARG_CHECK(ARG_UNPACK(x) (y) (z));
	th->data[12] += x;
	th->data[13] += y;
	th->data[14] += z;
}
ASFUNCTIONBODY_ATOM(Matrix3D,appendRotation)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	
	number_t degrees;
	_NR<Vector3D> axis;
	_NR<Vector3D> pivotPoint;
	
	ARG_CHECK(ARG_UNPACK(degrees) (axis) (pivotPoint,NullRef));

	// algorithm taken from https://github.com/openfl/openfl/blob/develop/openfl/geom/Matrix3D.hx
	number_t tx = 0;
	number_t ty = 0;
	number_t tz = 0;
	
	if (!pivotPoint.isNull()) {
		tx = pivotPoint->x;
		ty = pivotPoint->y;
		tz = pivotPoint->z;
	}
	number_t radian = degrees * 3.141592653589793/180.0;
	number_t cos = std::cos(radian);
	number_t sin = std::sin(radian);
	number_t x = axis->x;
	number_t y = axis->y;
	number_t z = axis->z;
	number_t x2 = x * x;
	number_t y2 = y * y;
	number_t z2 = z * z;
	number_t ls = x2 + y2 + z2;
	if (ls != 0) {
		number_t l = std::sqrt(ls);
		x /= l;
		y /= l;
		z /= l;
		x2 /= ls;
		y2 /= ls;
		z2 /= ls;
	}
	number_t ccos = 1 - cos;
	number_t d[4*4];
	d[0]  = x2 + (y2 + z2) * cos;
	d[1]  = x * y * ccos + z * sin;
	d[2]  = x * z * ccos - y * sin;
	d[3]  = 0.0;
	d[4]  = x * y * ccos - z * sin;
	d[5]  = y2 + (x2 + z2) * cos;
	d[6]  = y * z * ccos + x * sin;
	d[7]  = 0.0;
	d[8]  = x * z * ccos + y * sin;
	d[9]  = y * z * ccos - x * sin;
	d[10] = z2 + (x2 + y2) * cos;
	d[11] = 0.0;
	d[12] = (tx * (y2 + z2) - x * (ty * y + tz * z)) * ccos + (ty * z - tz * y) * sin;
	d[13] = (ty * (x2 + z2) - y * (tx * x + tz * z)) * ccos + (tz * x - tx * z) * sin;
	d[14] = (tz * (x2 + y2) - z * (tx * x + ty * y)) * ccos + (tx * y - ty * x) * sin;
	d[15] = 1.0;
	th->append(d);
}
ASFUNCTIONBODY_ATOM(Matrix3D,appendScale)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	
	number_t xScale;
	number_t yScale;
	number_t zScale;
	ARG_CHECK(ARG_UNPACK(xScale) (yScale) (zScale));

	number_t d[4*4];
	d[0]  = xScale;
	d[1]  = 0.0;
	d[2]  = 0.0;
	d[3]  = 0.0;
	
	d[4]  = 0.0;
	d[5]  = yScale;
	d[6]  = 0.0;
	d[7]  = 0.0;
	
	d[8]  = 0.0;
	d[9]  = 0.0;
	d[10] = zScale;
	d[11] = 0.0;
	
	d[12] = 0.0;
	d[13] = 0.0;
	d[14] = 0.0;
	d[15] = 1.0;
	th->append(d);
}
ASFUNCTIONBODY_ATOM(Matrix3D,copyColumnFrom)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	uint32_t column;
	_NR<Vector3D> vector3D;
	ARG_CHECK(ARG_UNPACK(column)(vector3D));
	if (vector3D.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"vector3D");
		return;
	}
	if (column >= 4)
	{
		createError<ArgumentError>(wrk,kInvalidParamError);
		return;
	}
	th->data[column*4  ] = vector3D->x;
	th->data[column*4+1] = vector3D->y;
	th->data[column*4+2] = vector3D->z;
	th->data[column*4+3] = vector3D->w;
}

ASFUNCTIONBODY_ATOM(Matrix3D,copyColumnTo)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	uint32_t column;
	_NR<Vector3D> vector3D;
	ARG_CHECK(ARG_UNPACK(column)(vector3D));
	if (vector3D.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"vector3D");
		return;
	}
	if (column >= 4)
	{
		createError<ArgumentError>(wrk,kInvalidParamError);
		return;
	}
	vector3D->x = th->data[column*4];
	vector3D->y = th->data[column*4+1];
	vector3D->z = th->data[column*4+2];
	vector3D->w = th->data[column*4+3];
}


ASFUNCTIONBODY_ATOM(Matrix3D,copyRawDataFrom)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector> vector;
	uint32_t index;
	bool transpose;
	ARG_CHECK(ARG_UNPACK(vector) (index,0) (transpose,false));
	if (transpose)
		LOG(LOG_NOT_IMPLEMENTED, "Matrix3D.copyRawDataFrom ignores parameter 'transpose'");
	for (uint32_t i = 0; i < vector->size()-index && i < 16; i++)
	{
		asAtom a = vector->at(index+i);
		th->data[i] = asAtomHandler::toNumber(a);
	}
}

ASFUNCTIONBODY_ATOM(Matrix3D,copyRawDataTo)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector> vector;
	uint32_t index;
	bool transpose;
	ARG_CHECK(ARG_UNPACK(vector) (index,0) (transpose,false));
	if (transpose)
	{
		number_t src[16];
		src[0] = th->data[0];
		src[1] = th->data[4];
		src[2] = th->data[8];
		src[3] = th->data[12];
		src[4] = th->data[1];
		src[5] = th->data[5];
		src[6] = th->data[9];
		src[7] = th->data[13];
		src[8] = th->data[2];
		src[9] = th->data[6];
		src[10] = th->data[10];
		src[11] = th->data[14];
		src[12] = th->data[3];
		src[13] = th->data[7];
		src[14] = th->data[11];
		src[15] = th->data[15];
		for (uint32_t i = 0; i < vector->size()-index && i < 16; i++)
		{
			vector->set(index+i,asAtomHandler::fromNumber(wrk,src[i],false));
		}
	}
	else
	{
		for (uint32_t i = 0; i < vector->size()-index && i < 16; i++)
		{
			vector->set(index+i,asAtomHandler::fromNumber(wrk,th->data[i],false));
		}
	}
}

ASFUNCTIONBODY_ATOM(Matrix3D,copyFrom)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Matrix3D> sourceMatrix3D;
	ARG_CHECK(ARG_UNPACK(sourceMatrix3D));
	if (sourceMatrix3D.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"sourceMatrix3D");
		return;
	}
	for (uint32_t i = 0; i < 4*4; i++)
	{
		th->data[i] = sourceMatrix3D->data[i];
	}
}

ASFUNCTIONBODY_ATOM(Matrix3D,copyRowFrom)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	uint32_t row;
	_NR<Vector3D> vector3D;
	ARG_CHECK(ARG_UNPACK(row)(vector3D));
	if (vector3D.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"vector3D");
		return;
	}
	if (row >= 4)
	{
		createError<ArgumentError>(wrk,kInvalidParamError);
		return;
	}
	switch (row)
	{
		case 0:
			th->data[0] = vector3D->x;
			th->data[4] = vector3D->y;
			th->data[8] = vector3D->z;
			th->data[12] = vector3D->w;
			break;
		case 1:
			th->data[1] = vector3D->x;
			th->data[5] = vector3D->y;
			th->data[9] = vector3D->z;
			th->data[13] = vector3D->w;
			break;
		case 2:
			th->data[2] = vector3D->x;
			th->data[6] = vector3D->y;
			th->data[10] = vector3D->z;
			th->data[14] = vector3D->w;
			break;
		case 3:
			th->data[3] = vector3D->x;
			th->data[7] = vector3D->y;
			th->data[11] = vector3D->z;
			th->data[15] = vector3D->w;
			break;
		default:
			break;
	}
}

ASFUNCTIONBODY_ATOM(Matrix3D,copyRowTo)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	uint32_t row;
	_NR<Vector3D> vector3D;
	ARG_CHECK(ARG_UNPACK(row)(vector3D));
	if (vector3D.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"vector3D");
		return;
	}
	if (row >= 4)
	{
		createError<ArgumentError>(wrk,kInvalidParamError);
		return;
	}
	switch (row)
	{
		case 0:
			vector3D->x = th->data[0];
			vector3D->y = th->data[4];
			vector3D->z = th->data[8];
			vector3D->w = th->data[12];
			break;
		case 1:
			vector3D->x = th->data[1];
			vector3D->y = th->data[5];
			vector3D->z = th->data[9];
			vector3D->w = th->data[13];
			break;
		case 2:
			vector3D->x = th->data[2];
			vector3D->y = th->data[6];
			vector3D->z = th->data[10];
			vector3D->w = th->data[14];
			break;
		case 3:
			vector3D->x = th->data[3];
			vector3D->y = th->data[7];
			vector3D->z = th->data[11];
			vector3D->w = th->data[15];
			break;
		default:
			break;
	}
}

ASFUNCTIONBODY_ATOM(Matrix3D,copyToMatrix3D)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Matrix3D> dest;
	ARG_CHECK(ARG_UNPACK(dest));
	if (dest.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"dest");
		return;
	}
	for (uint32_t i = 0; i < 4*4; i++)
	{
		dest->data[i] = th->data[i];
	}
}

ASFUNCTIONBODY_ATOM(Matrix3D,_identity)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	th->identity();
}
ASFUNCTIONBODY_ATOM(Matrix3D,invert)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);

	// algorithm taken from https://github.com/openfl/openfl/blob/develop/openfl/geom/Matrix3D.hx
	number_t det = th->getDeterminant();
	bool invertable = abs(det) > 0.00000000001;
	if (invertable)
	{
		number_t d = 1 / det;
		number_t m11 = th->data[0];	number_t m21 = th->data[4]; number_t m31 = th->data[8]; number_t m41 = th->data[12];
		number_t m12 = th->data[1]; number_t m22 = th->data[5]; number_t m32 = th->data[9]; number_t m42 = th->data[13];
		number_t m13 = th->data[2]; number_t m23 = th->data[6]; number_t m33 = th->data[10]; number_t m43 = th->data[14];
		number_t m14 = th->data[3]; number_t m24 = th->data[7]; number_t m34 = th->data[11]; number_t m44 = th->data[15];

		th->data[0] = d * (m22 * (m33 * m44 - m43 * m34) - m32 * (m23 * m44 - m43 * m24) + m42 * (m23 * m34 - m33 * m24));
		th->data[1] = -d * (m12 * (m33 * m44 - m43 * m34) - m32 * (m13 * m44 - m43 * m14) + m42 * (m13 * m34 - m33 * m14));
		th->data[2] = d * (m12 * (m23 * m44 - m43 * m24) - m22 * (m13 * m44 - m43 * m14) + m42 * (m13 * m24 - m23 * m14));
		th->data[3] = -d * (m12 * (m23 * m34 - m33 * m24) - m22 * (m13 * m34 - m33 * m14) + m32 * (m13 * m24 - m23 * m14));
		th->data[4] = -d * (m21 * (m33 * m44 - m43 * m34) - m31 * (m23 * m44 - m43 * m24) + m41 * (m23 * m34 - m33 * m24));
		th->data[5] = d * (m11 * (m33 * m44 - m43 * m34) - m31 * (m13 * m44 - m43 * m14) + m41 * (m13 * m34 - m33 * m14));
		th->data[6] = -d * (m11 * (m23 * m44 - m43 * m24) - m21 * (m13 * m44 - m43 * m14) + m41 * (m13 * m24 - m23 * m14));
		th->data[7] = d * (m11 * (m23 * m34 - m33 * m24) - m21 * (m13 * m34 - m33 * m14) + m31 * (m13 * m24 - m23 * m14));
		th->data[8] = d * (m21 * (m32 * m44 - m42 * m34) - m31 * (m22 * m44 - m42 * m24) + m41 * (m22 * m34 - m32 * m24));
		th->data[9] = -d * (m11 * (m32 * m44 - m42 * m34) - m31 * (m12 * m44 - m42 * m14) + m41 * (m12 * m34 - m32 * m14));
		th->data[10] = d * (m11 * (m22 * m44 - m42 * m24) - m21 * (m12 * m44 - m42 * m14) + m41 * (m12 * m24 - m22 * m14));
		th->data[11] = -d * (m11 * (m22 * m34 - m32 * m24) - m21 * (m12 * m34 - m32 * m14) + m31 * (m12 * m24 - m22 * m14));
		th->data[12] = -d * (m21 * (m32 * m43 - m42 * m33) - m31 * (m22 * m43 - m42 * m23) + m41 * (m22 * m33 - m32 * m23));
		th->data[13] = d * (m11 * (m32 * m43 - m42 * m33) - m31 * (m12 * m43 - m42 * m13) + m41 * (m12 * m33 - m32 * m13));
		th->data[14] = -d * (m11 * (m22 * m43 - m42 * m23) - m21 * (m12 * m43 - m42 * m13) + m41 * (m12 * m23 - m22 * m13));
		th->data[15] = d * (m11 * (m22 * m33 - m32 * m23) - m21 * (m12 * m33 - m32 * m13) + m31 * (m12 * m23 - m22 * m13));
	}
	asAtomHandler::setBool(ret,invertable);
}
ASFUNCTIONBODY_ATOM(Matrix3D,_get_determinant)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	asAtomHandler::setNumber(ret,wrk,th->getDeterminant());
}
ASFUNCTIONBODY_ATOM(Matrix3D,_get_rawData)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	asAtom v=asAtomHandler::invalidAtom;
	RootMovieClip* root = wrk->rootClip.getPtr();
	Template<Vector>::getInstanceS(wrk,v,root,Class<Number>::getClass(wrk->getSystemState()),NullRef);
	Vector *result = asAtomHandler::as<Vector>(v);
	for (uint32_t i = 0; i < 4*4; i++)
	{
		asAtom o = asAtomHandler::fromNumber(wrk,th->data[i],false);
		result->append(o);
	}
	ret =asAtomHandler::fromObject(result);
}
ASFUNCTIONBODY_ATOM(Matrix3D,_set_rawData)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector> data;
	ARG_CHECK(ARG_UNPACK(data));
	// TODO handle not invertible argument
	for (uint32_t i = 0; i < data->size(); i++)
	{
		asAtom a = data->at(i);
		th->data[i] = asAtomHandler::toNumber(a);
	}
}
ASFUNCTIONBODY_ATOM(Matrix3D,_get_position)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	Vector3D* res = Class<Vector3D>::getInstanceS(wrk);
	res->w = 0;
	res->x = th->data[12];
	res->y = th->data[13];
	res->z = th->data[14];
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(Matrix3D,_set_position)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector3D> value;
	ARG_CHECK(ARG_UNPACK(value));
	if (value.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"value");
		return;
	}
	th->data[12] = value->x;
	th->data[13] = value->y;
	th->data[14] = value->z;
}
ASFUNCTIONBODY_ATOM(Matrix3D,transformVector)
{
	Matrix3D * th=asAtomHandler::as<Matrix3D>(obj);
	_NR<Vector3D> v;
	ARG_CHECK(ARG_UNPACK(v));
	if (v.isNull())
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"v");
		return;
	}
	number_t x = v->x;
	number_t y = v->y;
	number_t z = v->z;
	Vector3D* res = Class<Vector3D>::getInstanceS(wrk);
	res->x = (x * th->data[0] + y * th->data[4] + z * th->data[8] + th->data[12]);
	res->y = (x * th->data[1] + y * th->data[5] + z * th->data[9] + th->data[13]);
	res->z = (x * th->data[2] + y * th->data[6] + z * th->data[10] + th->data[14]);
	res->w = (x * th->data[3] + y * th->data[7] + z * th->data[11] + th->data[15]);
	ret = asAtomHandler::fromObject(res);
}

