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
#include "scripting/avm1/clamp.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/toplevel/GradientBevelFilter.h"
#include "swftypes.h"
#include "utils/span.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::gradient_filter` crate.

GcPtr<AVM1BitmapFilter> AVM1GradientBevelFilter::cloneImpl(AVM1Activation& act) const
{
	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1GradientBevelFilter(act));
	ret->records = records;
	ret->blur = blur;
	ret->angle = angle;
	ret->distance = distance;
	ret->strength = strength;
	ret->knockout = knockout;
	ret->quality = quality;
	ret->type = type;
	return ret;
}

FILTER AVM1GradientBevelFilter::toFilterImpl() const
{
	GRADIENTBEVELFILTER filter;
	if (!records.empty())
	{
		filter.GradientColors = new RGBA[records.size()];
		filter.GradientRatio = new UI8[records.size()];
		filter.NumColors = records.size();
	}

	for (size_t i = 0; i < records.size(); ++i)
	{
		filter.GradientColors[i] = records[i].Color;
		filter.GradientRatio[i] = records[i].Ratio;
	}

	filter.BlurX = blur.x * 65536;
	filter.BlurY = blur.y * 65536;
	filter.Angle = angle * (M_PI / 180.0) * 65536;
	filter.Distance = distance * 65536;
	filter.Strength = strength * 256;
	filter.InnerShadow = type == Type::Inner;
	filter.Knockout = knockout;
	filter.CompositeSource = true;
	filter.OnTop = type == Type::Full;
	filter.passes = iclamp(quality, 0, 15);

	FILTER ret;
	ret.FilterID = FILTER_GRADIENTBEVEL;
	ret.GradientBevelFilter = filter;
	return ret;
}

void AVM1GradientBevelFilter::setColors
(
	AVM1Activation& act,
	Optional<const AVM1Value&> value
)
{
	if (!value.hasValue())
		return;

	// NOTE, PLAYER-SPECIFIC: In Flash Player 11, only "true" `Object`s
	// can resize `colors`, but in modern versions, `String`s will also
	// resize `colors` (which'll cause `colors` to be filled with `NaN`s,
	// since it has a `length`, but no elements).
	auto obj = value->toObject(act);
	size_t size = imax(obj->getLength(act), 0);

	records.resize(imin(size, 16), GRADRECORD(0xff));

	for (size_t i = 0; i < size; ++i)
	{
		records[i].Color = RGBA
		(
			obj->getElement(act, i).toInt32(act),
			records[i].Color.Alpha
		);
	}
}

void AVM1GradientBevelFilter::setAlphas
(
	AVM1Activation& act,
	Optional<const AVM1Value&> value
)
{
	if (!value.hasValue() || !value->is<AVM1Object>())
		return;

	auto obj = value->toObject(act);
	size_t size = imax(obj->getLength(act), 0);

	// NOTE: Unlike `colors`, and `ratios`, setting `alphas` doesn't
	// change the number of colours/records in the gradient.
	for (size_t i = 0; i < records.size(); ++i)
	{
		if (i >= size)
		{
			records[i].Color.Alpha = 0xff;
			continue;
		}

		auto alpha = obj->getElement(act, i).toNumber(act);
		records[i].Color.Alpha =
		(
			std::isfinite(alpha) ?
			uint8_t(255 * alpha + 0.5) :
			0xff
		);
	}
}

void AVM1GradientBevelFilter::setRatios
(
	AVM1Activation& act,
	Optional<const AVM1Value&> value
)
{
	if (!value.hasValue() || !value->is<AVM1Object>())
		return;

	auto obj = value->toObject(act);
	size_t size = imax(obj->getLength(act), 0);

	// NOTE: Modifying `ratios` can only reduce the colour/record count,
	// not increase it.
	records.resize(imin(size, records.size()), GRADRECORD(0xff));

	for (size_t i = 0; i < size; ++i)
	{
		records[i].Ratio = iclamp
		(
			obj->getElement(act, i).toInt32(act),
			0,
			0xff
		);
	}
}

AVM1GradientBevelFilter::AVM1GradientBevelFilter(AVM1Activationt& act) : AVM1BitmapFilter
(
	act.getGcCtx(),
	act.getPrototypes().gradientBevelFilter->proto
),
blur(4, 4),
angle(45),
distance(4),
strength(1),
knockout(false),
quality(1),
type(Type::Inner)
{
}

AVM1GradientBevelFilter::AVM1GradientBevelFilter
(
	AVM1Activationt& act,
	const Span<AVM1Value>& args
) : AVM1GradientBevelFilter(act)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);
	unpacker(distance, 4)(angle, 45);

	setColors(act, unpacker.tryUnpack());
	setAlphas(act, unpacker.tryUnpack());
	setRatios(act, unpacker.tryUnpack());

	unpacker(blur.x, 4)(blur.y, 4)(strength, 1)(quality, 1)
	(
		type,
		Type::Inner
	)(knockout, false);

	angle = std::fmod(angle, 360);
	blur.x = clampNaN(blur.x, 0, 255);
	blur.y = clampNaN(blur.y, 0, 255);
	strength = dclamp(strength, 0, 255);
	quality = iclamp(quality, 0, 15);
}

using AVM1GradientBevelFilter;

constexpr auto protoFlags = AVM1PropFlags::Version8;

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, distance, Distance, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, angle, Angle, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, colors, Colors, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, alphas, Alphas, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, ratios, Ratios, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, blurX, BlurX, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, blurY, BlurY, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, quality, Quality, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, strength, Strength, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, knockout, Knockout, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1GradientBevelFilter, type, Type, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1GradientBevelFilter::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1GradientBevelFilter, ctor)
{
	new (_this.getPtr()) AVM1GradientBevelFilter(act, args);
	return _this;
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Distance)
{
	return _this->distance;
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Distance)
{
	_this->distance = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Angle)
{
	return _this->angle;
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Angle)
{
	_this->angle = std::fmod(value.toNumber(act), 360);
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Colors)
{
	std::vector<AVM1Value> colors;
	ratios.reserve(_this->records.size());

	for (const auto& record : _this->records)
		colors.emplace_back(number_t(record.Color.toRGB().toUInt()));

	return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, colors));
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Colors)
{
	_this->setColors(act, value);
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Alphas)
{
	std::vector<AVM1Value> alphas;
	ratios.reserve(_this->records.size());

	for (const auto& record : _this->records)
		alphas.emplace_back(number_t(record.Color.Alpha) / 255);

	return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, alphas));
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Alphas)
{
	_this->setAlphas(act, value);
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Ratios)
{
	std::vector<AVM1Value> ratios;
	ratios.reserve(_this->records.size());

	for (const auto& record : _this->records)
		ratios.emplace_back(number_t(record.Ratio));

	return NEW_GC_PTR(act.getGcCtx(), AVM1Array(act, ratios));
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Ratios)
{
	_this->setRatios(act, value);
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, BlurX)
{
	return _this->blur.x;
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, BlurX)
{
	_this->blur.x = clampNaN(value.toNumber(act), 0, 255);
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, BlurY)
{
	return _this->blur.y;
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, BlurY)
{
	_this->blur.y = clampNaN(value.toNumber(act), 0, 255);
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Quality)
{
	return number_t(_this->quality);
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Quality)
{
	_this->quality = iclamp(value.toInt32(act), 0, 15);
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Strength)
{
	return _this->strength;
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Strength)
{
	_this->strength = value.toNumber(act);
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Knockout)
{
	return _this->knockout;
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Knockout)
{
	_this->knockout = value.toBool(act);
}

AVM1_GETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Type)
{
	switch (_this->type)
	{
		case Type::Inner: return "inner"; break;
		case Type::Outer: return "outer"; break;
		case Type::Full: return "full"; break;
		default: assert(false); break;
	}
}

AVM1_SETTER_TYPE_BODY(AVM1GradientBevelFilter, AVM1GradientBevelFilter, Type)
{
	auto str = value.toString(act);

	if (str == "inner")
		_this->type = Type::Inner;
	else if (str == "outer")
		_this->type = Type::Outer;
	else
		_this->type = Type::Full;
}
