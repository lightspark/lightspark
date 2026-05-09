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
#include "scripting/avm1/toplevel/BlurFilter.h"
#include "swftypes.h"
#include "utils/span.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::blur_filter` crate.

GcPtr<AVM1BitmapFilter> AVM1BlurFilter::cloneImpl(AVM1Activation& act) const
{
	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1BlurFilter(act));
	ret->blur = blur;
	ret->quality = quality;
	return ret;
}

FILTER AVM1BlurFilter::toFilterImpl() const
{
	BLURFILTER filter;
	filter.BlurX = blur.x * 65536;
	filter.BlurY = blur.y * 65536;
	filter.Passes = iclamp(quality, 0, 15);

	FILTER ret;
	ret.FilterID = FILTER_BLUR;
	ret.BlurFilter = filter;
	return ret;
}

AVM1BlurFilter::AVM1BlurFilter(AVM1Activationt& act) : AVM1BitmapFilter
(
	act.getGcCtx(),
	act.getPrototypes().blurFilter->proto
), blur(4, 4), quality(1)
{
}

AVM1BlurFilter::AVM1BlurFilter
(
	AVM1Activationt& act,
	const Span<AVM1Value>& args
) : AVM1BlurFilter(act)
{
	AVM1_ARG_UNPACK(blur.x, 4)(blur.y, 4)(quality, 1);
	blur.x = dclamp(blur.x, 0, 255);
	blur.y = dclamp(blur.y, 0, 255);
	quality = iclamp(quality, 0, 15);
}

using AVM1BlurFilter;

constexpr auto protoFlags = AVM1PropFlags::Version8;

static constexpr auto protoDecls =
{
	AVM1_PROPERTY_TYPE_PROTO(AVM1BlurFilter, blurX, BlurX, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BlurFilter, blurY, BlurY, protoFlags),
	AVM1_PROPERTY_TYPE_PROTO(AVM1BlurFilter, quality, Quality, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1BlurFilter::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1BlurFilter, ctor)
{
	new (_this.getPtr()) AVM1BlurFilter(act, args);
	return _this;
}

AVM1_GETTER_TYPE_BODY(AVM1BlurFilter, AVM1BlurFilter, BlurX)
{
	return _this->blur.x;
}

AVM1_SETTER_TYPE_BODY(AVM1BlurFilter, AVM1BlurFilter, BlurX)
{
	_this->blur.x = dclamp(value.toNumber(act), 0, 255);
}

AVM1_GETTER_TYPE_BODY(AVM1BlurFilter, AVM1BlurFilter, BlurY)
{
	return _this->blur.y;
}

AVM1_SETTER_TYPE_BODY(AVM1BlurFilter, AVM1BlurFilter, BlurY)
{
	_this->blur.y = dclamp(value.toNumber(act), 0, 255);
}

AVM1_GETTER_TYPE_BODY(AVM1BlurFilter, AVM1BlurFilter, Quality)
{
	return number_t(_this->quality);
}

AVM1_SETTER_TYPE_BODY(AVM1BlurFilter, AVM1BlurFilter, Quality)
{
	_this->quality = iclamp(value.toInt32(act), 0, 15);
}
