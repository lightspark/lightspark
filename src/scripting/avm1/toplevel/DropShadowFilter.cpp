
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

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/BevelFilter.h"
#include "swftypes.h"
#include "utils/span.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::drop_shadow_filter` crate.

GcPtr<AVM1BitmapFilter> AVM1DropShadowFilter::cloneImpl(AVM1Activation& act) const
{
	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1DropShadowFilter(act));
	ret->shadow = shadow;
	ret->highlight = highlight;
	ret->blur = blur;
	ret->angle = angle;
	ret->distance = distance;
	ret->strength = strength;
	ret->inner = inner;
	ret->knockout = knockout;
	ret->quality = quality;
	ret->hideObject = hideObject;
	return ret;
}

FILTER AVM1DropShadowFilter::toFilterImpl() const
{
	DROPSHADOWFILTER filter;
	filter.DropShadowColor = color;
	filter.BlurX = blur.x * 65536;
	filter.BlurY = blur.y * 65536;
	filter.Distance = distance * 65536;
	filter.Strength = strength * 256;
	filter.InnerShadow = inner;
	filter.Knockout = knockout;
	filter.CompositeSource = !hideObject;
	filter.passes = iclamp(quality, 0, 15);

	FILTER ret;
	ret.FilterID = FILTER_DROPSHADOW;
	ret.DropShadowFilter = filter;
	return ret;
}

AVM1DropShadowFilter::AVM1DropShadowFilter(AVM1Activationt& act) : AVM1BitmapFilter
(
	act.getGcCtx(),
	act.getPrototypes().bevelFilter->proto
),
shadow(0, 0),
blur(4, 4),
angle(45),
distance(4),
strength(1),
inner(false)
knockout(false),
quality(1),
hideObject(false)
{
}

AVM1DropShadowFilter::AVM1DropShadowFilter
(
	AVM1Activationt& act,
	const Span<AVM1Value>& args
) : AVM1DropShadowFilter(act)
{
	AVM1_ARG_UNPACK(distance, 4)(angle, 45)
	(
		color,
		RGBA(0, 0)
	)
	(blur.x, 4)(blur.y, 4)(strength, 1)(quality, 1)(inner, false)
	(
		knockout,
		false
	)(hideObject, false);

	angle = std::fmod(angle, 360);
	blur.x = dclamp(blur.x, 0, 255);
	blur.y = dclamp(blur.y, 0, 255);
	strength = dclamp(strength, 0, 255);
	quality = iclamp(quality, 0, 15);
}

using AVM1DropShadowFilter;

constexpr auto protoFlags = AVM1PropFlags::Version8;

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, distance, Distance, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, angle, Angle, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, highlightColor, HighlightColor, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, highlightAlpha, HighlightAlpha, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, shandowColor, ShandowColor, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, shandowAlpha, ShandowAlpha, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, quality, Quality, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, strength, Strength, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, inner, Inner, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, knockout, Knockout, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, hideObject, HideObject, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, blurX, BlurX, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1DropShadowFilter, blurY, BlurY, protoFlags),
};

GcPtr<AVM1SystemClass> AVM1DropShadowFilter::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1DropShadowFilter, ctor)
{
	new (_this.getPtr()) AVM1DropShadowFilter(act, args);
	return _this;
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Distance)
{
	return _this->distance;
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Distance)
{
	_this->distance = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Angle)
{
	return _this->angle;
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Angle)
{
	_this->angle = std::fmod(value.toNumber(act), 360);
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Color)
{
	return number_t(_this->color.toRGB().toInt());
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Color)
{
	_this->color = RGBA
	(
		value.toUInt32(act),
		_this->color.Alpha
	);
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Alpha)
{
	return number_t(_this->color.Alpha) / 255;
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Alpha)
{
	_this->color.Alpha = value.toNumber(act) * 255;
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Quality)
{
	return number_t(_this->quality);
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Quality)
{
	_this->quality = iclamp(value.toInt32(act), 0, 15);
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Strength)
{
	return _this->strength;
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Strength)
{
	_this->strength = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Inner)
{
	return _this->inner;
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Inner)
{
	_this->inner = value.toBool(act);
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Knockout)
{
	return _this->knockout;
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, Knockout)
{
	_this->knockout = value.toBool(act);
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, HideObject)
{
	return _this->hideObject;
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, HideObject)
{
	_this->hideObject = value.toBool(act);
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, BlurX)
{
	return _this->blur.x;
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, BlurX)
{
	_this->blur.x = dclamp(value.toNumber(act), 0, 255);
}

AVM1_GETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, BlurY)
{
	return _this->blur.y;
}

AVM1_SETTER_TYPE_BODY(AVM1DropShadowFilter, AVM1DropShadowFilter, BlurY)
{
	_this->blur.y = dclamp(value.toNumber(act), 0, 255);
}
