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
#include "display_object/DisplayObject.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/ColorTransform.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::color_transform` crate.

AVM1ColorTransform::AVM1ColorTransform
(
	AVM1Activation& act,
	const ColorTransformBase& ct
) : AVM1Object
(
	act,
	act.getPrototypes().colorTransform->proto
), ColorTransformBase(ct) {}

using AVM1ColorTransform;

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_TYPE_PROTO(AVM1ColorTransform, redMultiplier, RedMultiplier),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ColorTransform, greenMultiplier, GreenMultiplier),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ColorTransform, blueMultiplier, BlueMultiplier),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ColorTransform, alphaMultiplier, AlphaMultiplier),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ColorTransform, redOffset, RedOffset),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ColorTransform, greenOffset, GreenOffset),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ColorTransform, blueOffset, BlueOffset),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ColorTransform, alphaOffset, AlphaOffset),
	AVM1_PROPERTY_TYPE_PROTO(AVM1ColorTransform, rgb, RGB),
	AVM1_FUNCTION_TYPE_PROTO(AVM1ColorTransform, concat),
	AVM1_FUNCTION_PROTO(toString),
};

GcPtr<AVM1SystemClass> AVM1ColorTransform::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1ColorTransform, ctor)
{
	ColorTransformBase ct;
	AVM1_ARG_UNPACK(ct);

	return _this.constCast() = NEW_GC_PTR(act.getGcCtx(), AVM1ColorTransform
	(
		act,
		ct
	));
}

AVM1_GETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, RedMultiplier)
{
	return _this->redMultiplier;
}

AVM1_SETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, RedMultiplier)
{
	_this->redMultiplier = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, GreenMultiplier)
{
	return _this->greenMultiplier;
}

AVM1_SETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, GreenMultiplier)
{
	_this->greenMultiplier = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, BlueMultiplier)
{
	return _this->blueMultiplier;
}

AVM1_SETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, BlueMultiplier)
{
	_this->blueMultiplier = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, AlphaMultiplier)
{
	return _this->alphaMultiplier;
}

AVM1_SETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, AlphaMultiplier)
{
	_this->alphaMultiplier = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, RedOffset)
{
	return _this->redOffset;
}

AVM1_SETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, RedOffset)
{
	_this->redOffset = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, GreenOffset)
{
	return _this->greenOffset;
}

AVM1_SETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, GreenOffset)
{
	_this->greenOffset = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, BlueOffset)
{
	return _this->blueOffset;
}

AVM1_SETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, BlueOffset)
{
	_this->blueOffset = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, AlphaOffset)
{
	return _this->alphaOffset;
}

AVM1_SETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, AlphaOffset)
{
	_this->alphaOffset = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, RGB)
{
	return number_t(_this->getRGB());
}

AVM1_SETTER_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, RGB)
{
	_this->setfromRGB(value.toUInt32(act));
}

AVM1_FUNCTION_TYPE_BODY(AVM1ColorTransform, AVM1ColorTransform, concat)
{
	NullableGcPtr<AVM1ColorTransform> other;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(other));

	// NOTE: The offsets are done first because the calculation of
	// of the offsets depend on the original multiplier values.
	_this->redOffset += other->redOffset * _this->redMultiplier;
	_this->greenOffset += other->greenOffset * _this->greenMultiplier;
	_this->blueOffset += other->blueOffset * _this->blueMultiplier;
	_this->alphaOffset += other->alphaOffset * _this->alphaMultiplier;

	// Now we can do the multipliers.
	_this->redMultiplier *= other->redMultiplier;
	_this->greenMultiplier *= other->greenMultiplier;
	_this->blueMultiplier *= other->blueMultiplier;
	_this->alphaMultiplier *= other->alphaMultiplier;
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1ColorTransform, toString)
{
	auto getPropStr = [&](const tiny_string& str)
	{
		return _this->getProp(act, str).toString(act);
	};

	std::stringstring s;
	return
	(
		s << "(redMultiplier=" << getPropStr("redMultiplier") <<
		", greenMultiplier=" << getPropStr("greenMultiplier") <<
		", blueMultiplier=" << getPropStr("blueMultiplier") <<
		", alphaMultiplier=" << getPropStr("alphaMultiplier") <<
		", redOffset=" << getPropStr("redOffset") <<
		", greenOffset=" << getPropStr("greenOffset") <<
		", blueOffset=" << getPropStr("blueOffset") <<
		", alphaOffset=" << getPropStr("alphaOffset") << ')'
	).str();
}
