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

#ifndef SCRIPTING_AVM1_TOPLEVEL_DROPSHADOWFILTER_H
#define SCRIPTING_AVM1_TOPLEVEL_DROPSHADOWFILTER_H 1

#include "backends/geometry.h"
#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/toplevel/BitmapFilter.h"
#include "swftypes.h"

// Based on Ruffle's `avm1::globals::drop_shadow_filter` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class FILTER;
class GcContext;
template<typename T>
class Span;

class AVM1DropShadowFilter : public AVM1BitmapFilter
{
private:
	GcPtr<AVM1BitmapFilter> cloneImpl(AVM1Activation& act) const override;
	FILTER toFilterImpl() const override;

	RGBA color;
	Vector2f blur;
	number_t angle;
	number_t distance;
	number_t strength;
	bool inner;
	bool knockout;
	int32_t quality;
	bool hideObject;
public:
	AVM1DropShadowFilter(AVM1Activationt& act);
	AVM1DropShadowFilter
	(
		AVM1Activationt& act,
		const Span<AVM1Value>& args
	);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);

	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, Distance);
	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, Angle);
	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, Color);
	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, Alpha);
	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, Quality);
	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, Strength);
	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, Inner);
	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, Knockout);
	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, HideObject);
	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, BlurX);
	AVM1_PROPERTY_TYPE_DECL(AVM1DropShadowFilter, BlurY);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_DROPSHADOWFILTER_H */
