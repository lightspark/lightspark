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
#include "scripting/avm1/toplevel/GlowFilter.h"
#include "swftypes.h"
#include "utils/span.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::glow_filter` crate.

GcPtr<AVM1BitmapFilter> AVM1GlowFilter::cloneImpl(AVM1Activation& act) const
{
	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1GlowFilter(act));
	ret->color = color;
	ret->blur = blur;
	ret->strength = strength;
	ret->inner = inner;
	ret->knockout = knockout;
	ret->quality = quality;
	return ret;
}

FILTER AVM1GlowFilter::toFilterImpl() const
{
	GLOWFILTER filter;
	filter.GlowColor = color;
	filter.BlurX = blur.x * 65536;
	filter.BlurY = blur.y * 65536;
	filter.Strength = strength * 256;
	filter.InnerGlow = inner;
	filter.Knockout = knockout;
	filter.CompositeSource = true;
	filter.passes = iclamp(quality, 0, 15);

	FILTER ret;
	ret.FilterID = FILTER_GLOW;
	ret.GlowFilter = filter;
	return ret;
}

AVM1GlowFilter::AVM1GlowFilter(AVM1Activationt& act) : AVM1BitmapFilter
(
	act.getGcCtx(),
	act.getPrototypes().glowFilter->proto
),
color(0xff0000, 255),
blur(6, 6),
strength(2),
inner(false)
knockout(false),
quality(1)
{
}

AVM1GlowFilter::AVM1GlowFilter
(
	AVM1Activationt& act,
	const Span<AVM1Value>& args
) : AVM1GlowFilter(act)
{
	AVM1_ARG_UNPACK(color, RGBA(0xff0000, 255)(blur.x, 6)(blur.y, 6)
	(
		strength,
		2
	)(quality, 1)(inner, false)(knockout, false);

	blur.x = dclamp(blur.x, 0, 255);
	blur.y = dclamp(blur.y, 0, 255);
	quality = iclamp(quality, 0, 15);
}

using AVM1GlowFilter;

constexpr auto protoFlags = AVM1PropFlags::Version8;

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_TYPE_PROTO(AVM1GlowFilter, color, Color, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GlowFilter, alpha, Alpha, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GlowFilter, quality, Quality, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GlowFilter, inner, Inner, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GlowFilter, knockout, Knockout, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GlowFilter, blurX, BlurX, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GlowFilter, blurY, BlurY, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GlowFilter, strength, Strength, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1GlowFilter::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1GlowFilter, ctor)
{
	new (_this.getPtr()) AVM1GlowFilter(act, args);
	return _this;
}

AVM1_GETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Color)
{
	return number_t(_this->color.toRGB().toInt());
}

AVM1_SETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Color)
{
	_this->color = RGBA
	(
		value.toUInt32(act),
		_this->color.Alpha
	);
}

AVM1_GETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Alpha)
{
	return number_t(_this->color.Alpha) / 255;
}

AVM1_SETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Alpha)
{
	_this->color.Alpha = value.toNumber(act) * 255;
}

AVM1_GETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Quality)
{
	return number_t(_this->quality);
}

AVM1_SETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Quality)
{
	_this->quality = iclamp(value.toInt32(act), 0, 15);
}

AVM1_GETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Strength)
{
	return _this->strength;
}

AVM1_SETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Strength)
{
	_this->strength = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Inner)
{
	return _this->inner;
}

AVM1_SETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Inner)
{
	_this->inner = value.toBool(act);
}

AVM1_GETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Knockout)
{
	return _this->knockout;
}

AVM1_SETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, Knockout)
{
	_this->knockout = value.toBool(act);
}

AVM1_GETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, BlurX)
{
	return _this->blur.x;
}

AVM1_SETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, BlurX)
{
	_this->blur.x = dclamp(value.toNumber(act), 0, 255);
}

AVM1_GETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, BlurY)
{
	return _this->blur.y;
}

AVM1_SETTER_TYPE_BODY(AVM1GlowFilter, AVM1GlowFilter, BlurY)
{
	_this->blur.y = dclamp(value.toNumber(act), 0, 255);
}
