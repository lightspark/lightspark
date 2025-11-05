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
#include "display_object/DisplayObject.h"
#include "display_object/MovieClip.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/array.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/value.h"
#include "tiny_string.h"
#include "utils/array.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::movie_clip` crate.

AVM1MovieClip::AVM1MovieClip
(
	AVM1Activation& act,
	const GcPtr<MovieClip>& clip
) : AVM1DisplayObject
(
	act,
	clip,
	act.getSystemPrototypes().movieClip->proto
) {}

template<size_t swfVersion, EnableIf
<(
	swfVersion >= 5 &&
	swfVersion <= 10
), bool> = false>
static constexpr AVM1PropFlags allowSWFVersion()
{
	switch (swfVersion)
	{
		case 5: return AVM1PropFlags::Version5; break;
		case 6: return AVM1PropFlags::Version6; break;
		case 7: return AVM1PropFlags::Version7; break;
		case 8: return AVM1PropFlags::Version8; break;
		case 9: return AVM1PropFlags::Version9; break;
		case 10: return AVM1PropFlags::Version10; break;
		default: return AVM1PropFlags(0); break;
	}
}

template<bool dontDelete = true>
constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

template<>
constexpr auto protoFlags<false> = AVM1PropFlags::DontEnum;

template<size_t swfVersion, bool dontDelete = true>
constexpr auto protoFlags =
(
	protoFlags<dontDelete> |
	allowSWFVersion<swfVersion>()
);

#define MOVIECLIP_FUNC(func) [](AVM1_FUNCTION_ARGS) \
{ \
	if (!_this->is<MovieClip>()) \
		return AVM1Value::undefinedVal; \
	return func(act, _this->as<MovieClip>(), args); \
}

#define MOVIECLIP_GETTER(func) [](AVM1_FUNCTION_ARGS) \
{ \
	if (!_this->is<MovieClip>()) \
		return AVM1Value::undefinedVal; \
	return get##func(act, _this->as<MovieClip>()); \
}

#define MOVIECLIP_SETTER(func) [](AVM1_FUNCTION_ARGS) \
{ \
	if (!_this->is<MovieClip>()) \
		return AVM1Value::undefinedVal; \
	auto value = !args.empty() ? args[0] : AVM1Value::undefinedVal; \
	set##func(act, _this->as<MovieClip>(), value); \
	return AVM1Value::undefinedVal; \
}

#define MOVIECLIP_FUNC_PROTO(name, ...) \
	AVM1Decl(#name, MOVIECLIP_FUNC(name), propFlags<__VA_ARGS__>)

#define MOVIECLIP_PROP_PROTO(name, funcName, ...) AVM1Decl \
( \
	#name, \
	MOVIECLIP_GETTER(funcName), \
	MOVIECLIP_SETTER(funcName), \
	propFlags<__VA_ARGS__> \
)

static constexpr auto protoDecls =
{
	MOVIECLIP_FUNC_PROTO(attachAudio, 6),
	MOVIECLIP_FUNC_PROTO(attachBitmap, 8),
	MOVIECLIP_FUNC_PROTO(attachMovie),
	MOVIECLIP_FUNC_PROTO(beginFill, 6),
	MOVIECLIP_FUNC_PROTO(beginBitmapFill, 8),
	MOVIECLIP_FUNC_PROTO(beginGradientFill, 6),
	MOVIECLIP_FUNC_PROTO(clear, 6),
	MOVIECLIP_FUNC_PROTO(createEmptyMovieClip, 6),
	MOVIECLIP_FUNC_PROTO(createTextField),
	MOVIECLIP_FUNC_PROTO(curveTo, 6),
	MOVIECLIP_FUNC_PROTO(duplicateMovieClip),
	MOVIECLIP_FUNC_PROTO(endFill, 6),
	MOVIECLIP_FUNC_PROTO(getBounds),
	MOVIECLIP_FUNC_PROTO(getBytesLoaded),
	MOVIECLIP_FUNC_PROTO(getBytesTotal),
	// NOTE: `getDepth()` is defined in `AVM1DisplayObject`.
	MOVIECLIP_FUNC_PROTO(getInstanceAtDepth, 7),
	MOVIECLIP_FUNC_PROTO(getNextHighestDepth, 7),
	MOVIECLIP_FUNC_PROTO(getRect, 8),
	MOVIECLIP_FUNC_PROTO(getSWFVersion),
	MOVIECLIP_FUNC_PROTO(getURL),
	MOVIECLIP_FUNC_PROTO(globalToLocal),
	MOVIECLIP_FUNC_PROTO(gotoAndPlay),
	MOVIECLIP_FUNC_PROTO(gotoAndStop),
	MOVIECLIP_FUNC_PROTO(hitTest),
	MOVIECLIP_FUNC_PROTO(lineGradientStyle, 8),
	MOVIECLIP_FUNC_PROTO(lineStyle, 6),
	MOVIECLIP_FUNC_PROTO(lineTo, 6),
	MOVIECLIP_FUNC_PROTO(loadMovie),
	MOVIECLIP_FUNC_PROTO(loadVariables),
	MOVIECLIP_FUNC_PROTO(localToGlobal),
	MOVIECLIP_FUNC_PROTO(moveTo, 6),
	MOVIECLIP_FUNC_PROTO(nextFrame),
	MOVIECLIP_FUNC_PROTO(play),
	MOVIECLIP_FUNC_PROTO(prevFrame),
	MOVIECLIP_FUNC_PROTO(removeMovieClip),
	MOVIECLIP_FUNC_PROTO(setMask, 6),
	MOVIECLIP_FUNC_PROTO(startDrag),
	MOVIECLIP_FUNC_PROTO(stop),
	MOVIECLIP_FUNC_PROTO(stopDrag),
	MOVIECLIP_FUNC_PROTO(swapDepths),
	MOVIECLIP_FUNC_PROTO(unloadMovie),

	MOVIECLIP_PROP_PROTO(blendMode, BlendMode, 8),
	MOVIECLIP_PROP_PROTO(cacheAsBitmap, CacheAsBitmap, 8),
	MOVIECLIP_PROP_PROTO(filters, Filters, 8),
	MOVIECLIP_PROP_PROTO(opaqueBackground, OpaqueBackground, 8),
	AVM1Decl("enabled", true, protoFlags<false>),
	MOVIECLIP_PROP_PROTO(_lockroot, LockRoot),
	MOVIECLIP_PROP_PROTO(scrollRect, ScrollRect, 8),
	MOVIECLIP_PROP_PROTO(scale9Grid, Scale9Grid, 8),
	MOVIECLIP_PROP_PROTO(transform, Transform, 8),
	AVM1Decl("useHandCursor", true, protoFlags<false>),
	// NOTE: `focusEnabled` isn't a builtin property of `MovieClip`.
	// NOTE: `tabEnabled` isn't a builtin property of `MovieClip`.
	// NOTE: `tabIndex` isn't enumerable in `MovieClip`, unlike `Button`,
	// and `TextField`.
	MOVIECLIP_PROP_PROTO(tabIndex, TabIndex, 6, false)
	// NOTE: `tabChildren` isn't a builtin property of `MovieClip`.
};

#undef MOVIECLIP_FUNC
#undef MOVIECLIP_GETTER
#undef MOVIECLIP_SETTER
#undef MOVIECLIP_FUNC_PROTO
#undef MOVIECLIP_PROP_PROTO

GcPtr<AVM1SystemClass> AVM1MovieClip::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeEmptyClass(superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1Value AVM1MovieClip::makeRect(AVM1Activation& act, const RectF& rect)
{
	auto proto = act.getSystemPrototypes().rectangle->ctor;
	return proto->construct(act,
	{
		// `x`.
		rect.min.x,
		// `y`.
		rect.min.y,
		// `width`.
		rect.max.x,
		// `height`.
		rect.max.y
	});
}

Optional<RectF> AVM1MovieClip::objectToRect
(
	AVM1Activation& act,
	const GcPtr<AVM1Object>& obj
)
{
	static constexpr auto names = makeArray
	(
		"x",
		"y",
		"width",
		"height"
	);

	number_t values[4] = { 0.0 };
	for (size_t i = 0; i < names.size(); ++i)
	{
		auto val = obj->getLocalProp(act, names[i], false);
		if (!val.hasValue())
			return {};
		values[i] = val->toInt32(act);
	}

	return RectF
	{
		// `x`, `y`.
		Vector2f(values[0], values[1]),
		// `width`, `height`.
		Vector2f(values[2], values[3])
	};
}

AVM1_MOVIECLIP_GETTER_BODY(ScrollRect)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	auto scrollRect = _this->getNextScrollRect();

	return scrollRect.transformOr(undefVal, [&](const auto& rect)
	{
		return makeRect(act, rect);
	});
}

AVM1_MOVIECLIP_SETTER_BODY(ScrollRect)
{
	if (!value.is<AVM1Object>())
	{
		_this->setNextScrollRect({});
		return;
	}

	_this->setNextScrollRect(objToRect(act, value.toObject(*this)));
}

AVM1_MOVIECLIP_GETTER_BODY(Scale9Grid)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	auto scalingGrid = _this->getScalingGrid();

	return scalingGrid.transformOr(undefVal, [&](const auto& rect)
	{
		return makeRect(act, rect);
	});
}

AVM1_MOVIECLIP_SETTER_BODY(Scale9Grid)
{
	if (!value.is<AVM1Object>())
	{
		_this->setScalingGrid({});
		return;
	}

	_this->setScalingGrid(objToRect(act, value.toObject(*this)));
}
