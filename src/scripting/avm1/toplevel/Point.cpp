/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/Number.h"
#include "scripting/avm1/toplevel/Point.h"
#include "scripting/avm1/value.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::point` crate.

GcPtr<AVM1Object> AVM1Point::toObject
(
	AVM1Activation& act,
	const Vector2f& point
)
{
	return makePoint(act, { point.x, point.y });
}

GcPtr<AVM1Object> AVM1Point::makePoint
(
	AVM1Activation& act,
	const Span<AVM1Value>& args
)
{
	auto ctor = act.getPrototypes().point->ctor;
	return ctor->construct(act, args);
}

Vector2f AVM1Point::fromValue
(
	AVM1Activation& act,
	const AVM1Value& value
)
{
	return Vector2f
	(
		value.toObject(act)->getProp(act, "x").toNumber(act),
		value.toObject(act)->getProp(act, "y").toNumber(act)
	);
}

Vector2f AVM1Point::fromObject
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& obj
)
{
	return Vector2f
	(
		obj->getProp(act, "x").toNumber(act),
		obj->getProp(act, "y").toNumber(act)
	);
}

using AVM1Point;

static constexpr auto protoDecls =
{
	AVM1_GETTER_PROTO(length, Length, AVM1PropFlags::ReaodOnly),
	AVM1_FUNCTION_PROTO(clone),
	AVM1_FUNCTION_PROTO(offset),
	AVM1_FUNCTION_PROTO(equals),
	AVM1_FUNCTION_PROTO(subtract),
	AVM1_FUNCTION_PROTO(add),
	AVM1_FUNCTION_PROTO(normalize),
	AVM1_FUNCTION_PROTO(toString)
};

static constexpr auto objDecls =
{
	AVM1_FUNCTION_PROTO(distance),
	AVM1_FUNCTION_PROTO(polar),
	AVM1_FUNCTION_PROTO(interpolate)
}

GcPtr<AVM1SystemClass> AVM1Point::createClass
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

AVM1_FUNCTION_BODY(AVM1Point, ctor)
{
	if (args.empty())
	{
		_this->setProp(act, "y", 0.0);
		_this->setProp(act, "x", 0.0);
		return AVM1Value::undefinedVal;
	}

	AVM1Value xVal;
	AVM1Value yVal;
	AVM1_ARG_UNPACK(xVal)(yVal);

	_this->setProp(act, "y", yVal);
	_this->setProp(act, "x", xVal);
	return AVM1Value::undefinedVal;
}

AVM1_GETTER_BODY(AVM1Point, Length)
{
	// TODO: Use `Vector2Tmpl::dot()` here, after changing it's return
	// type from `int` -> `T`.
	auto point = fromValue(act, _this);
	return std::sqrt(point.x * point.x + point.y * point.y);
}

AVM1_FUNCTION_BODY(AVM1Point, clone)
{
	return makePoint(act,
	{
		_this->getProp(act, "x"),
		_this->getProp(act, "y")
	});
}

AVM1_FUNCTION_BODY(AVM1Point, offset)
{
	auto point = fromValue(act, _this);
	Vector2f delta;
	AVM1_ARG_UNPACK(delta.x)(delta.y);
	point += delta;

	_this->setProp(act, "x", point.x);
	_this->setProp(act, "y", point.y);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Point, equals)
{
	if (args.empty())
		return false;

	auto thisPoint = std::make_pair
	(
		_this->getProp(act, "x"),
		_this->getProp(act, "y")
	);

	auto other = args[0].toObject(act);
	auto otherPoint = std::make_pair
	(
		other->getProp(act, "x"),
		other->getProp(act, "y")
	);
	return thisPoint == otherPoint;
}

AVM1_FUNCTION_BODY(AVM1Point, subtract)
{
	auto point = fromObject(act, _this);
	auto other = fromValue(act, AVM1_ARG_UNPACK[0]);
	return toObject(act, point - other);
}

AVM1_FUNCTION_BODY(AVM1Point, add)
{
	auto point = fromObject(act, _this);
	auto other = fromValue(act, AVM1_ARG_UNPACK[0]);
	return toObject(act, point + other);
}

AVM1_FUNCTION_BODY(AVM1Point, normalize)
{
	auto curLen = _this->getProp(act, "length").toNumber(act);
	if (!std::isfinite(curLen))
		return AVM1Value::undefinedVal;

	auto point = fromObject(act, _this);
	number_t len;
	AVM1_ARG_UNPACK(len);
	auto newPoint = (curLen ? point / curLen : point) * len;

	_this->setProp(act, "x", newPoint.x);
	_this->setProp(act, "y", newPoint.y);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Point, toString)
{
	return
	(
		std::stringstream("(x=") <<
		_this->getProp(act, "x").toString(act) << ", y=" <<
		_this->getProp(act, "y").toString(act) << ')'
	).str();
}

AVM1_FUNCTION_BODY(AVM1Point, distance)
{
	if (args.size() < 2)
		return AVM1Number::NaN;

	auto a = args[0].toObject(act);
	auto b = args[1].toObject(act);
	return a->callMethod
	(
		act,
		"subtract"
		{ b },
		AVM1ExecutionReason::FunctionCall
	).toObject(act)->getProp(act, "length");
}

AVM1_FUNCTION_BODY(AVM1Point, polar)
{
	number_t len;
	number_t angle;
	AVM1_ARG_UNPACK(len)(angle);

	return toObject(act, Vector2f
	(
		len * std::cos(angle),
		len * std::sin(angle)
	));
}

AVM1_FUNCTION_BODY(AVM1Point, interpolate)
{
	constexpr auto NaN = AVM1Number::NaN;
	if (args.size() < 3)
		return toObject(act, Vector2f(NaN, NaN));

	auto a = fromValue(act, args[0]);
	auto b = fromValue(act, args[1]);
	auto f = args[2].toNumber(act);
	return toObject(act, b - (b - a) * f);
}
