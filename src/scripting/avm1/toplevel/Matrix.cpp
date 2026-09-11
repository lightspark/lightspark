/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include <initializer_list>
#include <sstream>

#include "backends/graphics.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/Matrix.h"
#include "scripting/avm1/toplevel/Number.h"
#include "scripting/avm1/toplevel/Point.h"
#include "scripting/avm1/value.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::matrix` crate.

MATRIX AVM1Matrix::fromValue
(
	AVM1Activation& act,
	const AVM1Value& value
)
{
	auto a = value.toObject(act)->getProp(act, "a").toNumber(act);
	auto b = value.toObject(act)->getProp(act, "b").toNumber(act);
	auto c = value.toObject(act)->getProp(act, "c").toNumber(act);
	auto d = value.toObject(act)->getProp(act, "d").toNumber(act);
	auto tx = value.toObject(act)->getProp(act, "tx").toNumber(act);
	auto ty = value.toObject(act)->getProp(act, "ty").toNumber(act);

	return MATRIX
	(
		a,
		d,
		b,
		c,
		tx * TWIPS_FACTOR,
		ty * TWIPS_FACTOR
	);
}

static MATRIX fromGradientObject
(
	AVM1Activation& act,
	GcPtr<AVM1Object> obj
)
{
	auto type = obj->getProp(act, "matrixType").toString(act);

	if (type != "box")
	{
		// TODO: You can also pass a 3x3 matrix here. How does it work?
		// e.g. `{ a: 200, b: 0, c: 0, d: 0, e: 200, f: 0, g: 200, h: 200, i: 1 };`
		return fromObject(act, obj);
	}

	auto width = obj->getProp(act, "w").toNumber(act);
	auto height = obj->getProp(act, "h").toNumber(act);
	auto rotation = obj->getProp(act, "r").toNumber(act);
	auto tx = obj->getProp(act, "x").toNumber(act);
	auto ty = obj->getProp(act, "y").toNumber(act);
	return createBox
	(
		Vector2f(width, height) / 1638.4,
		rotation,
		Vector2f(tx, ty) * TWIPS_FACTOR
	);
}

MATRIX AVM1Matrix::fromObject
(
	AVM1Activation& act,
	GcPtr<AVM1Object> obj
)
{
	auto a = obj->getProp(act, "a").toNumber(act);
	auto b = obj->getProp(act, "b").toNumber(act);
	auto c = obj->getProp(act, "c").toNumber(act);
	auto d = obj->getProp(act, "d").toNumber(act);
	auto tx = obj->getProp(act, "tx").toNumber(act);
	auto ty = obj->getProp(act, "ty").toNumber(act);

	return MATRIX
	(
		a,
		d,
		b,
		c,
		tx * TWIPS_FACTOR,
		ty * TWIPS_FACTOR
	);
}

MATRIX AVM1Matrix::fromObjectOrIdent
(
	AVM1Activation& act,
	GcPtr<AVM1Object> obj
)
{
	// NOTE: These lookups don't search the prototype chain, and ignore
	// virtual properties.
	auto _a = obj->getLocalProp(act, "a");
	auto _b = obj->getLocalProp(act, "b");
	auto _c = obj->getLocalProp(act, "c");
	auto _d = obj->getLocalProp(act, "d");
	auto _tx = obj->getLocalProp(act, "tx");
	auto _ty = obj->getLocalProp(act, "ty");

	bool hasProps =
	(
		_a.hasValue() &&
		_b.hasValue() &&
		_c.hasValue() &&
		_d.hasValue() &&
		_tx.hasValue() &&
		_ty.hasValue()
	);

	if (!hasProps)
		return MATRIX();

	auto a  = _a->toNumber(act);
	auto b  = _b->toNumber(act);
	auto c  = _c->toNumber(act);
	auto d  = _d->toNumber(act);
	auto tx = _tx->toNumber(act);
	auto ty = _ty->toNumber(act);

	return MATRIX
	(
		a,
		d,
		b,
		c,
		tx * TWIPS_FACTOR,
		ty * TWIPS_FACTOR
	);
}

AVM1Value AVM1Matrix::toValue
(
	AVM1Activation& act,
	const MATRIX& mtx
)
{
	auto ctor = act.getPrototypes().matrix->ctor;
	return ctor->construct(act,
	{
		mtx.xx,
		mtx.yx,
		mtx.xy,
		mtx.yy,
		mtx.x0 / TWIPS_FACTOR,
		mtx.y0 / TWIPS_FACTOR
	});
}

void AVM1Matrix::applyToObject
(
	AVM1Activation& act,
	GcPtr<AVM1Object> obj,
	const MATRIX& mtx
)
{
	obj->setProp(act, "a", mtx.xx);
	obj->setProp(act, "b", mtx.yx);
	obj->setProp(act, "c", mtx.xy);
	obj->setProp(act, "d", mtx.yy);
	obj->setProp(act, "tx", mtx.x0 / TWIPS_FACTOR);
	obj->setProp(act, "ty", mtx.y0 / TWIPS_FACTOR);
}

MATRIX AVM1Matrix::createBox
(
	const Vector2f& scale,
	number_t rotation,
	const Vector2f& offset
)
{
	MATRIX mtx;
	mtx.rotate(rotation);
	mtx.scale(scale.x, scale.y);
	mtx.translate(offset.x, offset.y);
	return mtx;
}

using AVM1Matrix;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_PROTO(concat),
	AVM1_FUNCTION_PROTO(invert),
	AVM1_FUNCTION_PROTO(createBox),
	AVM1_FUNCTION_PROTO(createGradientBox),
	AVM1_FUNCTION_PROTO(clone),
	AVM1_FUNCTION_PROTO(identity),
	AVM1_FUNCTION_PROTO(rotate),
	AVM1_FUNCTION_PROTO(translate),
	AVM1_FUNCTION_PROTO(scale),
	AVM1_FUNCTION_PROTO(deltaTransformPoint),
	AVM1_FUNCTION_PROTO(transformPoint),
	AVM1_FUNCTION_PROTO(toString)
};

GcPtr<AVM1SystemClass> AVM1Matrix::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	ctx.definePropsOn(_class->ctor, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Matrix, ctor)
{
	if (args.empty())
		return identity(act, _this, args);

	AVM1_ARG_UNPACK_NAMED(unpacker);
	_this->setProp(act, "a", unpacker.unpack<AVM1Value>());
	_this->setProp(act, "b", unpacker.unpack<AVM1Value>());
	_this->setProp(act, "c", unpacker.unpack<AVM1Value>());
	_this->setProp(act, "d", unpacker.unpack<AVM1Value>());
	_this->setProp(act, "tx", unpacker.unpack<AVM1Value>());
	_this->setProp(act, "ty", unpacker.unpack<AVM1Value>());
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Matrix, concat)
{
	AVM1Value other;
	AVM1_ARG_UNPACK(other);

	auto mtx = fromObject(act, _this);
	auto otherMtx = fromValue(act, other);
	applyToObject(act, _this, otherMtx.multiplyMatrix(mtx));
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Matrix, invert)
{
	auto mtx = fromObject(act, _this);
	// TODO: Make this more accurate to AVM1's `playerglobal`.
	applyToObject(act, _this, mtx.getInverted());
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Matrix, createBox)
{
	number_t scaleX;
	number_t scaleY;
	number_t rotation;
	number_t translateX;
	number_t translateY;
	AVM1_ARG_UNPACK(scaleX)(scaleY)
	(
		rotation,
		0
	)(translateX, 0)(translateY, 0);

	applyToObject(act, _this, createBox
	(
		Vector2f(scaleX, scaleY),
		rotation,
		Vector2f(translateX, translateY) * TWIPS_FACTOR
	));
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Matrix, createGradientBox)
{
	number_t scaleX;
	number_t scaleY;
	number_t rotation;
	number_t translateX;
	number_t translateY;
	AVM1_ARG_UNPACK(scaleX)(scaleY)
	(
		rotation,
		0
	)(translateX, 0)(translateY, 0);

	applyToObject(act, _this, createBox
	(
		Vector2f(scaleX, scaleY) / 1638.4,
		rotation,
		Vector2f(translateX, translateY) * TWIPS_FACTOR
	));
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Matrix, clone)
{
	auto ctor = act.getPrototypes().matrix->ctor;
	return ctor->construct(act,
	{
		_this->getProp(act, "a"),
		_this->getProp(act, "b"),
		_this->getProp(act, "c"),
		_this->getProp(act, "d"),
		_this->getProp(act, "tx"),
		_this->getProp(act, "ty")
	});
}

AVM1_FUNCTION_BODY(AVM1Matrix, identity)
{
	// NOTE: The assignment order looks weird here because the
	// `playerglobal` assigns them like this: `foo = bar = 0;`. Which
	// in this case, means `bar` gets assigned first, then `foo`.
	_this->setProp(act, "d", 1);
	_this->setProp(act, "a", 1);
	_this->setProp(act, "c", 0);
	_this->setProp(act, "b", 0);
	_this->setProp(act, "ty", 0);
	_this->setProp(act, "tx", 0);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Matrix, rotate)
{
	number_t angle;
	AVM1_ARG_UNPACK(angle);

	auto mtx = fromObject(act, _this);
	applyToObject(act, _this, mtx.rotate(angle));
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Matrix, translate)
{
	number_t dx;
	number_t dy;
	AVM1_ARG_UNPACK(dx)(dy);

	auto mtx = fromObject(act, _this);
	applyToObject(act, _this, mtx.translate
	(
		dx * TWIPS_FACTOR,
		dy * TWIPS_FACTOR
	));
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Matrix, scale)
{
	number_t sx;
	number_t sy;
	AVM1_ARG_UNPACK(sx)(sy);

	auto mtx = fromObject(act, _this);
	applyToObject(act, _this, mtx.scale(sx, sy));
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Matrix, deltaTransformPoint)
{
	AVM1Value pointVal;
	AVM1_ARG_UNPACK(pointVal);

	auto mtx = fromObject(act, _this);
	auto point = AVM1Point::fromValue(act, pointVal);
	return AVM1Point::toObject
	(
		act,
		mtx.multiply2D(point * TWIPS_FACTOR) -
		Vector2f(mtx.tx, mtx.ty)
	);
}

AVM1_FUNCTION_BODY(AVM1Matrix, transformPoint)
{
	AVM1Value pointVal;
	AVM1_ARG_UNPACK(pointVal);

	auto mtx = fromObject(act, _this);
	auto point = AVM1Point::fromValue(act, pointVal);
	return AVM1Point::toObject
	(
		act,
		mtx.multiply2D(point * TWIPS_FACTOR)
	);
}

AVM1_FUNCTION_BODY(AVM1Matrix, toString)
{
	return
	(
		std::ostringstream("(a=", std::ios::ate) <<
		_this->getProp(act, "a").toString(act) << ", b=" <<
		_this->getProp(act, "b").toString(act) << ", c=" <<
		_this->getProp(act, "c").toString(act) << ", d=" <<
		_this->getProp(act, "d").toString(act) << ", tx=" <<
		_this->getProp(act, "tx").toString(act) << ", ty=" <<
		_this->getProp(act, "ty").toString(act) << ')'
	).str();
}
