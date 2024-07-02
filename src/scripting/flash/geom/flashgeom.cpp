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
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Matrix3D.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/flash/display/BitmapContainer.h"

using namespace lightspark;
using namespace std;

ColorTransform::ColorTransform(ASWorker* wrk, Class_base* c, const ColorTransformBase& r):ASObject(wrk,c,T_OBJECT,SUBTYPE_COLORTRANSFORM),ColorTransformBase(r)
{
}
ColorTransform::ColorTransform(ASWorker* wrk, Class_base* c, const CXFORMWITHALPHA& cx)
  : ASObject(wrk,c,T_OBJECT,SUBTYPE_COLORTRANSFORM)
{
	cx.getParameters(redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
					 redOffset, greenOffset, blueOffset, alphaOffset);
}

void ColorTransform::setProperties(const CXFORMWITHALPHA &cx)
{
	cx.getParameters(redMultiplier, greenMultiplier, blueMultiplier, alphaMultiplier,
					 redOffset, greenOffset, blueOffset, alphaOffset);
}

void ColorTransform::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;

	// properties
	c->setDeclaredMethodByQName("color","",c->getSystemState()->getBuiltinFunction(getColor,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("color","",c->getSystemState()->getBuiltinFunction(setColor),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("redMultiplier","",c->getSystemState()->getBuiltinFunction(getRedMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("redMultiplier","",c->getSystemState()->getBuiltinFunction(setRedMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenMultiplier","",c->getSystemState()->getBuiltinFunction(getGreenMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenMultiplier","",c->getSystemState()->getBuiltinFunction(setGreenMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueMultiplier","",c->getSystemState()->getBuiltinFunction(getBlueMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueMultiplier","",c->getSystemState()->getBuiltinFunction(setBlueMultiplier),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaMultiplier","",c->getSystemState()->getBuiltinFunction(getAlphaMultiplier,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaMultiplier","",c->getSystemState()->getBuiltinFunction(setAlphaMultiplier),SETTER_METHOD,true);

	c->setDeclaredMethodByQName("redOffset","",c->getSystemState()->getBuiltinFunction(getRedOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("redOffset","",c->getSystemState()->getBuiltinFunction(setRedOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenOffset","",c->getSystemState()->getBuiltinFunction(getGreenOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("greenOffset","",c->getSystemState()->getBuiltinFunction(setGreenOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueOffset","",c->getSystemState()->getBuiltinFunction(getBlueOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("blueOffset","",c->getSystemState()->getBuiltinFunction(setBlueOffset),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaOffset","",c->getSystemState()->getBuiltinFunction(getAlphaOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("alphaOffset","",c->getSystemState()->getBuiltinFunction(setAlphaOffset),SETTER_METHOD,true);

	// methods
	c->setDeclaredMethodByQName("concat","",c->getSystemState()->getBuiltinFunction(concat),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

bool ColorTransform::destruct()
{
	redMultiplier=1.0;
	greenMultiplier=1.0;
	blueMultiplier=1.0;
	alphaMultiplier=1.0;
	redOffset=0.0;
	greenOffset=0.0;
	blueOffset=0.0;
	alphaOffset=0.0;
	return ASObject::destruct();
}

ASFUNCTIONBODY_ATOM(ColorTransform,_constructor)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	assert_and_throw(argslen<=8);
	if(0 < argslen)
		th->redMultiplier=asAtomHandler::toNumber(args[0]);
	else
		th->redMultiplier=1.0;
	if(1 < argslen)
		th->greenMultiplier=asAtomHandler::toNumber(args[1]);
	else
		th->greenMultiplier=1.0;
	if(2 < argslen)
		th->blueMultiplier=asAtomHandler::toNumber(args[2]);
	else
		th->blueMultiplier=1.0;
	if(3 < argslen)
		th->alphaMultiplier=asAtomHandler::toNumber(args[3]);
	else
		th->alphaMultiplier=1.0;
	if(4 < argslen)
		th->redOffset=asAtomHandler::toNumber(args[4]);
	else
		th->redOffset=0.0;
	if(5 < argslen)
		th->greenOffset=asAtomHandler::toNumber(args[5]);
	else
		th->greenOffset=0.0;
	if(6 < argslen)
		th->blueOffset=asAtomHandler::toNumber(args[6]);
	else
		th->blueOffset=0.0;
	if(7 < argslen)
		th->alphaOffset=asAtomHandler::toNumber(args[7]);
	else
		th->alphaOffset=0.0;
}

ASFUNCTIONBODY_ATOM(ColorTransform,setColor)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	assert_and_throw(argslen==1);
	uint32_t tmp=asAtomHandler::toInt(args[0]);
	//Setting multiplier to 0
	th->redMultiplier=0;
	th->greenMultiplier=0;
	th->blueMultiplier=0;
	//Setting offset to the input value
	th->redOffset=(tmp>>16)&0xff;
	th->greenOffset=(tmp>>8)&0xff;
	th->blueOffset=tmp&0xff;
}

ASFUNCTIONBODY_ATOM(ColorTransform,getColor)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);

	int ro, go, bo;
	ro = static_cast<int>(th->redOffset) & 0xff;
	go = static_cast<int>(th->greenOffset) & 0xff;
	bo = static_cast<int>(th->blueOffset) & 0xff;

	uint32_t color = (ro<<16) | (go<<8) | bo;

	asAtomHandler::setUInt(ret,wrk,color);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getRedMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->redMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setRedMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->redMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getGreenMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->greenMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setGreenMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->greenMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getBlueMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->blueMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setBlueMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->blueMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getAlphaMultiplier)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->alphaMultiplier);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setAlphaMultiplier)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->alphaMultiplier = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getRedOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->redOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setRedOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->redOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getGreenOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->greenOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setGreenOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->greenOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getBlueOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->blueOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setBlueOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->blueOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,getAlphaOffset)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	asAtomHandler::setNumber(ret,wrk,th->alphaOffset);
}

ASFUNCTIONBODY_ATOM(ColorTransform,setAlphaOffset)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	th->alphaOffset = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(ColorTransform,concat)
{
	assert_and_throw(argslen==1);
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	ColorTransform* ct=asAtomHandler::as<ColorTransform>(args[0]);
	th->redMultiplier *= ct->redMultiplier;
	th->redOffset = th->redOffset * ct->redMultiplier + ct->redOffset;
	th->greenMultiplier *= ct->greenMultiplier;
	th->greenOffset = th->greenOffset * ct->greenMultiplier + ct->greenOffset;
	th->blueMultiplier *= ct->blueMultiplier;
	th->blueOffset = th->blueOffset * ct->blueMultiplier + ct->blueOffset;
	th->alphaMultiplier *= ct->alphaMultiplier;
	th->alphaOffset = th->alphaOffset * ct->alphaMultiplier + ct->alphaOffset;
}

ASFUNCTIONBODY_ATOM(ColorTransform,_toString)
{
	ColorTransform* th=asAtomHandler::as<ColorTransform>(obj);
	char buf[1024];
	snprintf(buf,1024,"(redOffset=%f, redMultiplier=%f, greenOffset=%f, greenMultiplier=%f blueOffset=%f blueMultiplier=%f alphaOffset=%f, alphaMultiplier=%f)",
			th->redOffset, th->redMultiplier, th->greenOffset, th->greenMultiplier, th->blueOffset, th->blueMultiplier, th->alphaOffset, th->alphaMultiplier);
	
	ret = asAtomHandler::fromObject(abstract_s(wrk,buf));
}

Transform::Transform(ASWorker* wrk, Class_base* c):ASObject(wrk,c),perspectiveProjection(Class<PerspectiveProjection>::getInstanceSNoArgs(wrk))
{
}
Transform::Transform(ASWorker* wrk,Class_base* c, _R<DisplayObject> o):ASObject(wrk,c),owner(o),perspectiveProjection(Class<PerspectiveProjection>::getInstanceSNoArgs(wrk))
{
}

bool Transform::destruct()
{
	owner.reset();
	perspectiveProjection.reset();
	matrix3D.reset();
	return destructIntern();
}

void Transform::finalize()
{
	owner.reset();
	perspectiveProjection.reset();
	matrix3D.reset();
}

void Transform::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (owner)
		owner->prepareShutdown();
	if (perspectiveProjection)
		perspectiveProjection->prepareShutdown();
	if (matrix3D)
		matrix3D->prepareShutdown();
}

bool Transform::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	if (owner)
		ret = owner->countAllCylicMemberReferences(gcstate) || ret;
	if (perspectiveProjection)
		ret = perspectiveProjection->countAllCylicMemberReferences(gcstate) || ret;
	if (matrix3D)
		ret = matrix3D->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

void Transform::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("colorTransform","",c->getSystemState()->getBuiltinFunction(_getColorTransform,0,Class<Transform>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("colorTransform","",c->getSystemState()->getBuiltinFunction(_setColorTransform),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("matrix","",c->getSystemState()->getBuiltinFunction(_setMatrix),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("matrix","",c->getSystemState()->getBuiltinFunction(_getMatrix,0,Class<Matrix>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("concatenatedMatrix","",c->getSystemState()->getBuiltinFunction(_getConcatenatedMatrix,0,Class<Matrix>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, perspectiveProjection, PerspectiveProjection);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, matrix3D, Matrix3D);
}
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(Transform, perspectiveProjection)
ASFUNCTIONBODY_GETTER_SETTER_CB(Transform, matrix3D,onSetMatrix3D)

void Transform::onSetMatrix3D(_NR<Matrix3D> oldValue)
{
	owner->setMatrix3D(this->matrix3D);
}

ASFUNCTIONBODY_ATOM(Transform,_constructor)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	// it's not in the specs but it seems to be possible to construct a Transform object with an owner as argment
	ARG_CHECK(ARG_UNPACK(th->owner,NullRef));
}

ASFUNCTIONBODY_ATOM(Transform,_getMatrix)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	assert_and_throw(argslen==0);
	if (th->owner->matrix.isNull())
	{
		asAtomHandler::setNull(ret);
		return;
	}
	const MATRIX& res=th->owner->getMatrix();
	ret = asAtomHandler::fromObject(Class<Matrix>::getInstanceS(wrk,res));
}

ASFUNCTIONBODY_ATOM(Transform,_setMatrix)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	_NR<Matrix> m;
	ARG_CHECK(ARG_UNPACK(m));
	th->owner->setMatrix(m);
}

ASFUNCTIONBODY_ATOM(Transform,_getColorTransform)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	if (th->owner->colorTransform.isNull())
		th->owner->colorTransform = _NR<ColorTransform>(Class<ColorTransform>::getInstanceS(wrk));
	th->owner->colorTransform->incRef();
	ret = asAtomHandler::fromObject(th->owner->colorTransform.getPtr());
}

ASFUNCTIONBODY_ATOM(Transform,_setColorTransform)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	_NR<ColorTransform> ct;
	ARG_CHECK(ARG_UNPACK(ct));
	if (ct.isNull())
	{
		createError<TypeError>(wrk,kNullPointerError, "colorTransform");
		return;
	}

	if (th->owner->colorTransform.isNull())
		th->owner->colorTransform = _NR<ColorTransform>(Class<ColorTransform>::getInstanceS(wrk, *ct.getPtr()));
	else
		*th->owner->colorTransform.getPtr() = *ct.getPtr();
	th->owner->hasChanged=true;
	if (th->owner->isOnStage())
		th->owner->requestInvalidation(wrk->getSystemState());
	else
		th->owner->requestInvalidationFilterParent();
}

ASFUNCTIONBODY_ATOM(Transform,_getConcatenatedMatrix)
{
	Transform* th=asAtomHandler::as<Transform>(obj);
	ret = asAtomHandler::fromObject(Class<Matrix>::getInstanceS(wrk,th->owner->getConcatenatedMatrix()));
}

Matrix::Matrix(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_MATRIX)
{
}

Matrix::Matrix(ASWorker* wrk,Class_base* c, const MATRIX& m):ASObject(wrk,c,T_OBJECT,SUBTYPE_MATRIX),matrix(m)
{
}

void Matrix::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable=true;
	//Properties
	c->setDeclaredMethodByQName("a","",c->getSystemState()->getBuiltinFunction(_get_a,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("b","",c->getSystemState()->getBuiltinFunction(_get_b,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("c","",c->getSystemState()->getBuiltinFunction(_get_c,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("d","",c->getSystemState()->getBuiltinFunction(_get_d,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("tx","",c->getSystemState()->getBuiltinFunction(_get_tx,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("ty","",c->getSystemState()->getBuiltinFunction(_get_ty,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	
	c->setDeclaredMethodByQName("a","",c->getSystemState()->getBuiltinFunction(_set_a),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("b","",c->getSystemState()->getBuiltinFunction(_set_b),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("c","",c->getSystemState()->getBuiltinFunction(_set_c),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("d","",c->getSystemState()->getBuiltinFunction(_set_d),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("tx","",c->getSystemState()->getBuiltinFunction(_set_tx),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("ty","",c->getSystemState()->getBuiltinFunction(_set_ty),SETTER_METHOD,true);
	
	//Methods 
	c->setDeclaredMethodByQName("clone","",c->getSystemState()->getBuiltinFunction(clone,0,Class<Matrix>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("concat","",c->getSystemState()->getBuiltinFunction(concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copyFrom","",c->getSystemState()->getBuiltinFunction(copyFrom),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createBox","",c->getSystemState()->getBuiltinFunction(createBox),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createGradientBox","",c->getSystemState()->getBuiltinFunction(createGradientBox),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("deltaTransformPoint","",c->getSystemState()->getBuiltinFunction(deltaTransformPoint,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("identity","",c->getSystemState()->getBuiltinFunction(identity),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("invert","",c->getSystemState()->getBuiltinFunction(invert),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("rotate","",c->getSystemState()->getBuiltinFunction(rotate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("scale","",c->getSystemState()->getBuiltinFunction(scale),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTo","",c->getSystemState()->getBuiltinFunction(setTo),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("transformPoint","",c->getSystemState()->getBuiltinFunction(transformPoint,1,Class<Point>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("translate","",c->getSystemState()->getBuiltinFunction(translate),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Matrix,_constructor)
{
	assert_and_throw(argslen <= 6);
	
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	
	//Mapping to cairo_matrix_t
	//a -> xx
	//b -> yx
	//c -> xy
	//d -> yy
	//tx -> x0
	//ty -> y0
	
	if (argslen >= 1)
		th->matrix.xx = asAtomHandler::toNumber(args[0]);
	if (argslen >= 2)
		th->matrix.yx = asAtomHandler::toNumber(args[1]);
	if (argslen >= 3)
		th->matrix.xy = asAtomHandler::toNumber(args[2]);
	if (argslen >= 4)
		th->matrix.yy = asAtomHandler::toNumber(args[3]);
	if (argslen >= 5)
		th->matrix.x0 = asAtomHandler::toNumber(args[4]);
	if (argslen == 6)
		th->matrix.y0 = asAtomHandler::toNumber(args[5]);
}

ASFUNCTIONBODY_ATOM(Matrix,_toString)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	char buf[512];
	snprintf(buf,512,"(a=%f, b=%f, c=%f, d=%f, tx=%f, ty=%f)",
			th->matrix.xx, th->matrix.yx, th->matrix.xy, th->matrix.yy, th->matrix.x0, th->matrix.y0);
	ret = asAtomHandler::fromObject(abstract_s(wrk,buf));
}

MATRIX Matrix::getMATRIX() const
{
	return matrix;
}

bool Matrix::destruct()
{
	matrix = MATRIX();
	return destructIntern();
}

ASFUNCTIONBODY_ATOM(Matrix,_get_a)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.xx);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_a)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.xx = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_b)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.yx);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_b)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.yx = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_c)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.xy);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_c)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.xy = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_d)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.yy);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_d)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.yy = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_tx)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.x0);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_tx)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.x0 = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,_get_ty)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	asAtomHandler::setNumber(ret,wrk,th->matrix.y0);
}

ASFUNCTIONBODY_ATOM(Matrix,_set_ty)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==1);
	th->matrix.y0 = asAtomHandler::toNumber(args[0]);
}

ASFUNCTIONBODY_ATOM(Matrix,clone)
{
	assert_and_throw(argslen==0);

	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Matrix* res=Class<Matrix>::getInstanceS(wrk,th->matrix);
	ret =asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(Matrix,concat)
{
	assert_and_throw(argslen==1);

	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Matrix* m=asAtomHandler::as<Matrix>(args[0]);

	//Premultiply, which is flash convention
	cairo_matrix_multiply(&th->matrix,&th->matrix,&m->matrix);
}

ASFUNCTIONBODY_ATOM(Matrix,identity)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	assert_and_throw(argslen==0);
	
	cairo_matrix_init_identity(&th->matrix);
}

ASFUNCTIONBODY_ATOM(Matrix,invert)
{
	assert_and_throw(argslen==0);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	th->matrix=th->matrix.getInverted();
}

ASFUNCTIONBODY_ATOM(Matrix,translate)
{
	assert_and_throw(argslen==2);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t dx = asAtomHandler::toNumber(args[0]);
	number_t dy = asAtomHandler::toNumber(args[1]);

	th->matrix.translate(dx,dy);
}

ASFUNCTIONBODY_ATOM(Matrix,rotate)
{
	assert_and_throw(argslen==1);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t angle = asAtomHandler::toNumber(args[0]);

	th->matrix.rotate(angle);
}

ASFUNCTIONBODY_ATOM(Matrix,scale)
{
	assert_and_throw(argslen==2);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t sx = asAtomHandler::toNumber(args[0]);
	number_t sy = asAtomHandler::toNumber(args[1]);

	th->matrix.scale(sx, sy);
}

void Matrix::_createBox (number_t scaleX, number_t scaleY, number_t angle, number_t x, number_t y) {
	/*
	 * sequence written in the spec:
	 *      identity();
	 *      rotate(angle);
	 *      scale(scaleX, scaleY);
	 *      translate(x, y);
	 */

	//Initialize using rotation
	cairo_matrix_init_rotate(&matrix,angle);

	matrix.scale(scaleX,scaleY);
	matrix.translate(x,y);
}

ASFUNCTIONBODY_ATOM(Matrix,createBox)
{
	assert_and_throw(argslen>=2 && argslen <= 5);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t scaleX = asAtomHandler::toNumber(args[0]);
	number_t scaleY = asAtomHandler::toNumber(args[1]);

	number_t angle = 0;
	if ( argslen > 2 ) angle = asAtomHandler::toNumber(args[2]);

	number_t translateX = 0;
	if ( argslen > 3 ) translateX = asAtomHandler::toNumber(args[3]);

	number_t translateY = 0;
	if ( argslen > 4 ) translateY = asAtomHandler::toNumber(args[4]);

	th->_createBox(scaleX, scaleY, angle, translateX, translateY);
}

ASFUNCTIONBODY_ATOM(Matrix,createGradientBox)
{
	assert_and_throw(argslen>=2 && argslen <= 5);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	number_t width  = asAtomHandler::toNumber(args[0]);
	number_t height = asAtomHandler::toNumber(args[1]);

	number_t angle = 0;
	if ( argslen > 2 ) angle = asAtomHandler::toNumber(args[2]);

	number_t translateX = width/2.0;
	if ( argslen > 3 ) translateX += asAtomHandler::toNumber(args[3]);

	number_t translateY = height/2.0;
	if ( argslen > 4 ) translateY += asAtomHandler::toNumber(args[4]);

	th->_createBox(width / 1638.4, height / 1638.4, angle, translateX, translateY);
}

ASFUNCTIONBODY_ATOM(Matrix,transformPoint)
{
	assert_and_throw(argslen==1);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t ttx = pt->getX();
	number_t tty = pt->getY();
	cairo_matrix_transform_point(&th->matrix,&ttx,&tty);
	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(wrk,ttx, tty));
}

ASFUNCTIONBODY_ATOM(Matrix,deltaTransformPoint)
{
	assert_and_throw(argslen==1);
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	Point* pt=asAtomHandler::as<Point>(args[0]);

	number_t ttx = pt->getX();
	number_t tty = pt->getY();
	cairo_matrix_transform_distance(&th->matrix,&ttx,&tty);
	ret = asAtomHandler::fromObject(Class<Point>::getInstanceS(wrk,ttx, tty));
}

ASFUNCTIONBODY_ATOM(Matrix,setTo)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	
	//Mapping to cairo_matrix_t
	//a -> xx
	//b -> yx
	//c -> xy
	//d -> yy
	//tx -> x0
	//ty -> y0
	
	ARG_CHECK(ARG_UNPACK(th->matrix.xx)(th->matrix.yx)(th->matrix.xy)(th->matrix.yy)(th->matrix.x0)(th->matrix.y0));
}
ASFUNCTIONBODY_ATOM(Matrix,copyFrom)
{
	Matrix* th=asAtomHandler::as<Matrix>(obj);
	
	_NR<Matrix> sourceMatrix;
	ARG_CHECK(ARG_UNPACK(sourceMatrix));
	th->matrix.xx = sourceMatrix->matrix.xx;
	th->matrix.yx = sourceMatrix->matrix.yx;
	th->matrix.xy = sourceMatrix->matrix.xy;
	th->matrix.yy = sourceMatrix->matrix.yy;
	th->matrix.x0 = sourceMatrix->matrix.x0;
	th->matrix.y0 = sourceMatrix->matrix.y0;
}



PerspectiveProjection::PerspectiveProjection(ASWorker* wrk, Class_base* c):ASObject(wrk,c),fieldOfView(0),focalLength(0)
{
}

void PerspectiveProjection::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	REGISTER_GETTER_SETTER(c, fieldOfView);
	REGISTER_GETTER_SETTER(c, focalLength);
	REGISTER_GETTER_SETTER(c, projectionCenter);
	
}

bool PerspectiveProjection::destruct()
{
	fieldOfView = 0;
	focalLength= 0;
	projectionCenter.reset();
	return destructIntern();
}

void PerspectiveProjection::finalize()
{
	projectionCenter.reset();
}

void PerspectiveProjection::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (projectionCenter)
		projectionCenter->prepareShutdown();
	
}

ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(PerspectiveProjection, fieldOfView)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(PerspectiveProjection, focalLength)
ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(PerspectiveProjection, projectionCenter)

ASFUNCTIONBODY_ATOM(PerspectiveProjection,_constructor)
{
	PerspectiveProjection* th=asAtomHandler::as<PerspectiveProjection>(obj);
	th->projectionCenter = _MR(Class<Point>::getInstanceSNoArgs(wrk));
	LOG(LOG_NOT_IMPLEMENTED,"PerspectiveProjection is not implemented");
}
