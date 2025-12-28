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

#include "backends/graphics.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/Number.h"
#include "scripting/avm1/toplevel/Point.h"
#include "scripting/avm1/toplevel/Rectangle.h"
#include "scripting/avm1/value.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/array.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::rectangle` crate.

GcPtr<AVM1Object> AVM1Rectangle::toObject
(
	AVM1Activation& act,
	const RectF& rect
)
{
	const auto& min = rect.min;
	const auto& max = rect.max;
	return makeRect(act, { min.x, min.y, max.x, max.y });
}

GcPtr<AVM1Object> AVM1Rectangle::makeRect
(
	AVM1Activation& act,
	const Span<AVM1Value>& args
)
{
	auto ctor = act.getPrototypes().rectangle->ctor;
	return ctor->construct(act, args);
}

RectF AVM1Rectangle::fromValue
(
	AVM1Activation& act,
	const AVM1Value& value
)
{
	return RectF { getMin(act, value), getMax(act, value) };
}

RectF AVM1Rectangle::fromObject
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& obj
)
{
	return RectF { getMin(act, obj), getMax(act, obj) };
}

Vector2f AVM1Rectangle::getMin
(
	AVM1Activation& act,
	const AVM1Value& value
)
{
	return AVM1Point::fromValue(act, value);
}

Vector2f AVM1Rectangle::getMin
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& obj
)
{
	return AVM1Point::fromObject(act, obj);
}

Vector2f AVM1Rectangle::getMax
(
	AVM1Activation& act,
	const AVM1Value& value
)
{
	return Vector2f
	(
		value.toObject(act)->getProp(act, "width").toNumber(act),
		value.toObject(act)->getProp(act, "height").toNumber(act)
	);
}

Vector2f AVM1Rectangle::getMax
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& obj
)
{
	return Vector2f
	(
		obj->getProp(act, "width").toNumber(act),
		obj->getProp(act, "height").toNumber(act)
	);
}

using AVM1Rectangle;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_PROTO(clone),
	AVM1_FUNCTION_PROTO(setEmpty),
	AVM1_FUNCTION_PROTO(isEmpty),
	AVM1_PROPERTY_PROTO(left, Left),
	AVM1_PROPERTY_PROTO(right, Right),
	AVM1_PROPERTY_PROTO(top, Top),
	AVM1_PROPERTY_PROTO(bottom, Bottom),
	AVM1_PROPERTY_PROTO(topLeft, TopLeft),
	AVM1_PROPERTY_PROTO(bottomRight, BottomRight),
	AVM1_PROPERTY_PROTO(size, Size),
	AVM1_FUNCTION_PROTO(inflate),
	AVM1_FUNCTION_PROTO(inflatePoint),
	AVM1_FUNCTION_PROTO(offset),
	AVM1_FUNCTION_PROTO(offsetPoint),
	AVM1_FUNCTION_PROTO(contains),
	AVM1_FUNCTION_PROTO(containsPoint),
	AVM1_FUNCTION_PROTO(containsRectangle),
	AVM1_FUNCTION_PROTO(intersection),
	AVM1_FUNCTION_PROTO(intersects),
	AVM1Decl("union", _union),
	AVM1_FUNCTION_PROTO(equals),
	AVM1_FUNCTION_PROTO(toString)
};

GcPtr<AVM1SystemClass> AVM1Rectangle::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Rectangle, ctor)
{
	if (args.empty())
		return setEmpty(act, _this, args);

	AVM1Value xVal;
	AVM1Value yVal;
	AVM1Value widthVal;
	AVM1Value heightVal;
	AVM1_ARG_UNPACK(xVal)(yVal)(widthVal)(heightVal);

	_this->setProp(act, "x", xVal);
	_this->setProp(act, "y", yVal);
	_this->setProp(act, "width", widthVal);
	_this->setProp(act, "height", heightVal);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Rectangle, clone)
{
	return makeRect(act,
	{
		_this->getProp(act, "x"),
		_this->getProp(act, "y"),
		_this->getProp(act, "width"),
		_this->getProp(act, "height")
	});
}

AVM1_FUNCTION_BODY(AVM1Rectangle, setEmpty)
{
	_this->setProp(act, "x", 0.0);
	_this->setProp(act, "y", 0.0);
	_this->setProp(act, "width", 0.0);
	_this->setProp(act, "height", 0.0);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Rectangle, isEmpty)
{
	auto max = getMax(act, _this);
	return max <= Vector2f() ||
	(
		std::isnan(max.x) ||
		std::isnan(max.y)
	);
}

AVM1_GETTER_BODY(AVM1Rectangle, Left)
{
	return _this->getProp(act, "x");
}

AVM1_SETTER_BODY(AVM1Rectangle, Left)
{
	auto x = _this->getProp(act, "x").toNumber(act);
	auto width = _this->getProp(act, "width").toNumber(act);
	_this->setProp(act, "x", value);
	_this->setProp(act, "width", width + (x - value.toNumber(act)));
}

AVM1_GETTER_BODY(AVM1Rectangle, Right)
{
	return
	(
		_this->getProp(act, "x").toNumber(act) +
		_this->getProp(act, "width").toNumber(act)
	);
}

AVM1_SETTER_BODY(AVM1Rectangle, Right)
{
	auto newWidth =
	(
		!value.is<UndefinedVal>() ?
		value.toNumber(act) :
		AVM1Number::NaN
	);

	auto x = _this->getProp(act, "x").toNumber(act);
	_this->setProp(act, "width", newWidth - x);
}

AVM1_GETTER_BODY(AVM1Rectangle, Top)
{
	return _this->getProp(act, "y");
}

AVM1_SETTER_BODY(AVM1Rectangle, Top)
{
	auto y = _this->getProp(act, "y").toNumber(act);
	auto height = _this->getProp(act, "height").toNumber(act);
	_this->setProp(act, "y", value);
	_this->setProp(act, "height", height + (y - value.toNumber(act)));
}

AVM1_GETTER_BODY(AVM1Rectangle, Bottom)
{
	return
	(
		_this->getProp(act, "y").toNumber(act) +
		_this->getProp(act, "height").toNumber(act)
	);
}

AVM1_SETTER_BODY(AVM1Rectangle, Bottom)
{
	auto newHeight =
	(
		!value.is<UndefinedVal>() ?
		value.toNumber(act) :
		AVM1Number::NaN
	);

	auto y = _this->getProp(act, "y").toNumber(act);
	_this->setProp(act, "height", newHeight - y);
}

AVM1_GETTER_BODY(AVM1Rectangle, TopLeft)
{
	return AVM1Point::makePoint(act,
	{
		_this->getProp(act, "x"),
		_this->getProp(act, "y")
	});
}

AVM1_SETTER_BODY(AVM1Rectangle, TopLeft)
{
	AVM1Value newX;
	AVM1Value newY;
	(void)value.tryAs<AVM1Object>().andThen([&](const auto& obj)
	{
		newX = obj->getProp(act, "x");
		newY = obj->getProp(act, "y");
		return obj.asOpt();
	});

	auto x = _this->getProp(act, "x").toNumber(act);
	auto width = _this->getProp(act, "width").toNumber(act);
	auto y = _this->getProp(act, "y").toNumber(act);
	auto height = _this->getProp(act, "height").toNumber(act);

	_this->setProp(act, "x", newX);
	_this->setProp(act, "y", newY);
	_this->setProp(act, "width", width + (x - newX.toNumber(act)));
	_this->setProp(act, "height", height + (y - newY.toNumber(act)));
}

AVM1_GETTER_BODY(AVM1Rectangle, BottomRight)
{
	auto point = getMin(act, _this) + getMax(act, _this);
	return AVM1Point::toObject(act, point);
}

AVM1_SETTER_BODY(AVM1Rectangle, BottomRight)
{
	auto newBR = AVM1Point::fromValue(act, value);
	auto topLeft = getMin(act, _this);
	_this->setProp(act, "width", newBR.x - topLeft.x);
	_this->setProp(act, "height", newBR.y - topLeft.y);
}

AVM1_GETTER_BODY(AVM1Rectangle, Size)
{
	return AVM1Point::makePoint(act,
	{
		_this->getProp(act, "width"),
		_this->getProp(act, "height")
	});
}

AVM1_SETTER_BODY(AVM1Rectangle, Size)
{
	AVM1Value newWidth;
	AVM1Value newHeight;
	(void)value.tryAs<AVM1Object>().andThen([&](const auto& obj)
	{
		newWidth = obj->getProp(act, "x");
		newHeight = obj->getProp(act, "y");
		return obj.asOpt();
	});

	_this->setProp(act, "width", newWidth);
	_this->setProp(act, "height", newHeight);
}

static void inflateRect
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& obj,
	const RectF& rect,
	const Vector2f& offset
)
{
	auto tl = rect.min - offset;
	obj->setProp(act, "x", tl.x);
	obj->setProp(act, "y", tl.y);

	auto br = rect.max + offset * 2;
	obj->setProp(act, "width", br.x);
	obj->setProp(act, "height", br.y);
}

AVM1_FUNCTION_BODY(AVM1Rectangle, inflate)
{
	auto rect = fromObject(act, _this);

	number_t horizontal;
	number_t vertical;
	AVM1_ARG_UNPACK(horizontal)(vertical)

	inflateRect(act, _this, rect, Vector2f(horizontal, vertical));
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Rectangle, inflatePoint)
{
	auto rect = fromObject(act, _this);
	auto offset = AVM1Point::fromValue(act, AVM1_ARG_UNPACK[0]);
	inflateRect(act, _this, rect, offset);
	return AVM1Value::undefinedVal;
}

static void offsetRect
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& obj,
	const Vector2f& point,
	const Vector2f& offset
)
{
	auto tl = point + offset;
	obj->setProp(act, "x", tl.x);
	obj->setProp(act, "y", tl.y);
}

AVM1_FUNCTION_BODY(AVM1Rectangle, offset)
{
	auto point = AVM1Point::fromObject(act, _this);

	number_t horizontal;
	number_t vertical;
	AVM1_ARG_UNPACK(horizontal)(vertical)

	offsetRect(act, _this, point, Vector2f(horizontal, vertical));
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1Rectangle, offsetPoint)
{
	auto point = AVM1Point::fromObject(act, _this);
	auto offset = AVM1Point::fromValue(act, AVM1_ARG_UNPACK[0]);

	offsetRect(act, _this, point, offset);
	return AVM1Value::undefinedVal;
}

using ValuePair = std::pair<AVM1Value, AVM1Value>;

// Based on Gnash's `Rectangle_contains()`.
template<typename F, typename F2>
static AVM1Value containsRect
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& obj,
	F&& getX,
	F2&& getY
)
{
	// NOTE: The reason this looks odd is because in Flash Player, the
	// `Rectangle.contains()` functions are all implemented in
	// ActionScript (i.e. it's defined in AVM1's `playerglobals`).
	auto getRectX = [&] { return obj->getProp(act, "x"); };
	auto getRectY = [&] { return obj->getProp(act, "y"); };
	auto getRectWidth = [&] { return obj->getProp(act, "width"); };
	auto getRectHeight = [&] { return obj->getProp(act, "height"); };

	// The point is contained within the rectangle if it's either on the
	// top edge, left edge, or completely inside of the rectangle.
	// The point isn't considered contained, if it's on the bottom, or
	// right edge.
	//
	// NOTE: Because Flash Player's versions of these functions are done
	// in ActionScript, the order of these checks matters.
	//
	// `x >= this.x`.
	auto ret = getX().isLess(getRectX(), act);
	if (ret.is<UndefinedVal>())
		return AVM1Value::undefinedVal;
	else if (ret.toBool(act, act.getSwfVersion()))
		return false;

	// `x < this.x + this.width`.
	ret = getX().isLess(getRectX() + getRectWidth(), act);
	if (ret.is<UndefinedVal>())
		return AVM1Value::undefinedVal;
	else if (!ret.toBool(act, act.getSwfVersion()))
		return false;

	// `y >= this.y`.
	auto ret = getY().isLess(getRectY(), act);
	if (ret.is<UndefinedVal>())
		return AVM1Value::undefinedVal;
	else if (ret.toBool(act, act.getSwfVersion()))
		return false;

	// `y < this.y + this.height`.
	ret = getY().isLess(getRectY() + getRectHeight(), act);
	if (ret.is<UndefinedVal>())
		return AVM1Value::undefinedVal;
	else if (!ret.toBool(act, act.getSwfVersion()))
		return false;

	return true;
}

AVM1_FUNCTION_BODY(AVM1Rectangle, contains)
{
	if (args.size() < 2)
		return AVM1Value::undefinedVal;

	auto getX = [&] { return args[0]; };
	auto getY = [&] { return args[1]; };
	return containsRect(act, _this, getX, getY);
}

AVM1_FUNCTION_BODY(AVM1Rectangle, containsPoint)
{
	if (args.empty())
		return AVM1Value::undefinedVal;

	auto val = args[0];
	auto getX = [&] { return val.toObject(act)->getProp(act, "x"); };
	auto getY = [&] { return val.toObject(act)->getProp(act, "y"); };
	return containsRect(act, _this, getX, getY);
}

AVM1_FUNCTION_BODY(AVM1Rectangle, containsRectangle)
{
}

AVM1_FUNCTION_BODY(AVM1Rectangle, intersection)
{
}

AVM1_FUNCTION_BODY(AVM1Rectangle, intersects)
{
}

AVM1_FUNCTION_BODY(AVM1Rectangle, _union)
{
}

AVM1_FUNCTION_BODY(AVM1Rectangle, equals)
{
}

AVM1_FUNCTION_BODY(AVM1Rectangle, toString)
{
}
