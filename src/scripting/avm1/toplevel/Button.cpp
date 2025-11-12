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

#include "display_object/Button.h"
#include "display_object/DisplayObject.h"
#include "display_object/InteractiveObject.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/display_object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::button` crate.

AVM1Button::AVM1Button
(
	AVM1Activation& act,
	const GcPtr<Button>& clip
) : AVM1DisplayObject
(
	act,
	clip,
	act.getPrototypes().button->proto
) {}


constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::Version8
);

#define BUTTON_PROP_PROTO(name, funcName) \
	AVM1_PROPERTY_TYPE_PROTO(Button, name, funcName, protoFlags)

static constexpr auto protoDecls =
{
	// NOTE: `enabled` is defined in `AVM1DisplayObject`.
	AVM1Decl("useHandCursor", true),
	// NOTE: `getDepth()` is defined in `AVM1DisplayObject`.
	BUTTON_PROP_PROTO(blendMode, BlendMode),
	// NOTE: `filters` is defined in `AVM1DisplayObject`.
	BUTTON_PROP_PROTO(cacheAsBitmap, CacheAsBitmap),
	BUTTON_PROP_PROTO(scrollRect, ScrollRect),
	BUTTON_PROP_PROTO(scale9Grid, Scale9Grid),
	// NOTE: `tabEnabled` isn't a builtin property of `Button`.
	// NOTE: `tabIndex` is defined in `AVM1DisplayObject`.
};

#undef BUTTON_PROP_PROTO

GcPtr<AVM1SystemClass> AVM1Button::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeEmptyClass(superProto);
	AVM1DisplayObject::defineInteractiveProto(ctx, _class, true);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_GETTER_TYPE_BODY(Button, AVM1Button, BlendMode)
{
	switch (_this->getBlendMode())
	{
		case BLENDMODE_LAYER: return "layer"; break;
		case BLENDMODE_MULTIPLY: return "multiply"; break;
		case BLENDMODE_SCREEN: return "screen"; break;
		case BLENDMODE_LIGHTEN: return "lighten"; break;
		case BLENDMODE_DARKEN: return "darken"; break;
		case BLENDMODE_DIFFERENCE: return "difference"; break;
		case BLENDMODE_ADD: return "add"; break;
		case BLENDMODE_SUBTRACT: return "subtract"; break;
		case BLENDMODE_INVERT: return "invert"; break;
		case BLENDMODE_ALPHA: return "alpha"; break;
		case BLENDMODE_ERASE: return "erase"; break;
		case BLENDMODE_OVERLAY: return "overlay"; break;
		case BLENDMODE_HARDLIGHT: return "hardlight"; break;
		default: return "normal"; break;
	}
}

AVM1_SETTER_TYPE_BODY(Button, AVM1Button, BlendMode)
{
	(void)value.toBlendMode().andThen([&](const auto& blendMode)
	{
		_this->setBlendMode(blendMode);
		return makeOptional(blendMode);
	}).orElse([&]
	{
		// Don't do anything, if `value` isn't a valid blend mode.
		LOG(LOG_ERROR, "setBlendMode(): Unknown blend mode " << value);
		return {};
	});
}

AVM1_GETTER_TYPE_BODY(Button, AVM1Button, CacheAsBitmap)
{
	return _this->getCacheAsBitmap();
}

AVM1_SETTER_TYPE_BODY(Button, AVM1Button, CacheAsBitmap)
{
	_this->setCacheAsBitmap(value.toBool(act.getSwfVersion()));
}

AVM1_GETTER_TYPE_BODY(Button, AVM1Button, ScrollRect)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	auto scrollRect = _this->getNextScrollRect();

	return scrollRect.transformOr(undefVal, [&](const auto& rect)
	{
		return makeRect(act, rect);
	});
}

AVM1_SETTER_TYPE_BODY(Button, AVM1Button, ScrollRect)
{
	if (!value.is<AVM1Object>())
	{
		_this->setNextScrollRect({});
		return;
	}

	_this->setNextScrollRect(objToRect(act, value.toObject(*this)));
}

AVM1_GETTER_TYPE_BODY(Button, AVM1Button, Scale9Grid)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	auto scalingGrid = _this->getScalingGrid();

	return scalingGrid.transformOr(undefVal, [&](const auto& rect)
	{
		return makeRect(act, rect);
	});
}

AVM1_SETTER_TYPE_BODY(Button, AVM1Button, Scale9Grid)
{
	if (!value.is<AVM1Object>())
	{
		_this->setScalingGrid({});
		return;
	}

	_this->setScalingGrid(objToRect(act, value.toObject(*this)));
}
