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

// Based on Ruffle's `avm1::globals::bevel_filter` crate.

GcPtr<AVM1BitmapFilter> AVM1BevelFilter::cloneImpl(AVM1Activation& act) const
{
	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1BevelFilter(act));
	ret->shadow = shadow;
	ret->highlight = highlight;
	ret->blur = blur;
	ret->angle = angle;
	ret->distance = distance;
	ret->strength = strength;
	ret->knockout = knockout;
	ret->quality = quality;
	ret->type = type;
	return ret;
}

FILTER AVM1BevelFilter::toFilterImpl() const
{
	BEVELFILTER filter;
	filter.ShadowColor = shadow;
	filter.HighlightColor = highlight;
	filter.BlurX = blur.x * 65536;
	filter.BlurY = blur.y * 65536;
	filter.Angle = angle * (M_PI / 180.0) * 65536;
	filter.Distance = distance * 65536;
	filter.Strength = strength * 256;
	filter.InnerShadow = type == Type::Inner;
	filter.Knockout = knockout;
	filter.CompositeSource = false;
	filter.OnTop = type == Type::Full;
	filter.passes = iclamp(quality, 0, 15);

	FILTER ret;
	ret.FilterID = FILTER_BEVEL;
	ret.BevelFilter = filter;
	return ret;
}

AVM1BevelFilter::AVM1BevelFilter(AVM1Activationt& act) : AVM1BitmapFilter
(
	act.getGcCtx(),
	act.getPrototypes().bevelFilter->proto
),
shadow(0, 255),
highlight(0xffffff, 255),
blur(4, 4),
angle(45),
distance(4),
strength(1),
knockout(false),
quality(1),
type(Type::Inner)
{
}

AVM1BevelFilter::AVM1BevelFilter
(
	AVM1Activationt& act,
	const Span<AVM1Value>& args
) : AVM1BevelFilter(act)
{
	constexpr auto white = RGBA(0xffffff, 255);
	constexpr auto black = RGBA(0, 255);
	AVM1_ARG_UNPACK(distance, 4)(angle, 45)(highlight, white)
	(
		shadow,
		black
	)(blur.x, 4)(blur.y, 4)(strength, 1)(quality, 1)
	(
		type,
		Type::Inner
	)(knockout, false);

	angle = std::fmod(angle, 360);
	blur.x = dclamp(blur.x, 0, 255);
	blur.y = dclamp(blur.y, 0, 255);
	strength = dclamp(strength, 0, 255);
	quality = iclamp(quality, 0, 15);
}

using AVM1BevelFilter;

constexpr auto protoFlags = AVM1PropFlags::Version8;

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, distance, Distance, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, angle, Angle, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, highlightColor, HighlightColor, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, highlightAlpha, HighlightAlpha, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, shandowColor, ShandowColor, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, shandowAlpha, ShandowAlpha, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, quality, Quality, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, strength, Strength, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, knockout, Knockout, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, blurX, BlurX, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, blurY, BlurY, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BevelFilter, type, Type, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1BevelFilter::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1BevelFilter, ctor)
{
	new (_this.getPtr()) AVM1BevelFilter(act, args);
	return _this;
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Distance)
{
	return _this->distance;
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Distance)
{
	_this->distance = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Angle)
{
	return _this->angle;
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Angle)
{
	_this->angle = std::fmod(value.toNumber(act), 360);
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, HighlightColor)
{
	return number_t(_this->highlight.toRGB().toInt());
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, HighlightColor)
{
	_this->highlight = RGBA
	(
		value.toUInt32(act),
		_this->highlight.Alpha
	);
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, HighlightAlpha)
{
	return number_t(_this->highlight.Alpha) / 255;
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, HighlightAlpha)
{
	_this->highlight.Alpha = value.toNumber(act) * 255;
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, ShandowColor)
{
	return number_t(_this->shadow.toRGB().toInt());
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, ShandowColor)
{
	_this->shadow = RGBA
	(
		value.toUInt32(act),
		_this->shadow.Alpha
	);
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, ShandowAlpha)
{
	return number_t(_this->shadow.Alpha) / 255;
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, ShandowAlpha)
{
	_this->shadow.Alpha = value.toNumber(act) * 255;
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Quality)
{
	return number_t(_this->quality);
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Quality)
{
	_this->quality = iclamp(value.toInt32(act), 0, 15);
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Strength)
{
	return _this->strength;
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Strength)
{
	_this->strength = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Knockout)
{
	return _this->knockout;
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Knockout)
{
	_this->knockout = value.toBool(act);
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, BlurX)
{
	return _this->blur.x;
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, BlurX)
{
	_this->blur.x = dclamp(value.toNumber(act), 0, 255);
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, BlurY)
{
	return _this->blur.y;
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, BlurY)
{
	_this->blur.y = dclamp(value.toNumber(act), 0, 255);
}

AVM1_GETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Type)
{
	switch (_this->type)
	{
		case Type::Inner: return "inner"; break;
		case Type::Outer: return "outer"; break;
		case Type::Full: return "full"; break;
		default: assert(false); break;
	}
}

AVM1_SETTER_TYPE_BODY(AVM1BevelFilter, AVM1BevelFilter, Type)
{
	auto str = value.toString(act);

	if (str == "inner")
		_this->type = Type::Inner;
	else if (str == "outer")
		_this->type = Type::Outer;
	else
		_this->type = Type::Full;
}
