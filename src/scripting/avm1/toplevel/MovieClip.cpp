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
#include "display_object/Bitmap.h"
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

AVM1_MOVIECLIP_FUNC_BODY(hitTest)
{
	if (args.empty())
		return false;

	if (args.size() == 1)
	{
		auto other = act.resolveTargetClip(_this, args[0], false);
		return !other.isNull() && _this->hitTestObject(other);
	}

	// TODO: Use twips instead of float.
	Vector2f point;
	bool shapeFlag;
	AVM1_ARG_CHECK_THROW(AVM1_ARG_UNPACK(point)(shapeFlag, false));

	if (!std::isfinite(point.x) || !std::isfinite(point.y))
		return false;

	// NOTE: The AS1/2 docs say that the point is in "Stage coordinates",
	// but is actually in root coordinates. The root can be moved with
	// `_root._x`, etc. Meaning we have to transform from root space, to
	// world space.
	auto globalPoint = _this->getAVM1RootNoLock()->localToGlobal
	(
		point,
		false
	);

	if (!shapeFlag)
		return _this->hitTestBounds(point, globalPoint);

	return _this->hitTestShape
	(
		point,
		globalPoint,
		HitTestOptions::SkipMasks
	);
}

AVM1_MOVIECLIP_FUNC_BODY(attachBitmap)
{
	NullableGcPtr<AVM1BitmapData> data;
	int32_t depth;
	bool pixelSnapping;
	bool smoothing;

	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(data)(depth)
	(
		pixelSnapping,
		false
	)(smoothing, false));

	depth += AVM1depthOffset;
	auto bitmap = NEW_GC_PTR(act.getGcCtx(), Bitmap
	(
		act.getGcCtx(),
		data,
		pixelSnapping,
		smoothing,
		_this->getMovie()
	));

	_this->replaceLegacyChildAt(depth, data);
	data->constructionComplete();
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(attachAudio)
{
	if (args.empty())
		return AVM1Value::undefinedVal;

	if (!args[0].tryAs<bool>().valueOr(true))
	{
		_this->attachAudio(NullGc);
		return AVM1Value::undefinedVal;
	}

	auto stream = args[0].tryAs<AVM1NetStream>();
	if (!stream.isNull())
		_this->attachAudio(stream);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(lineStyle)
{
	auto graphics = _this->getGraphics();
	if (args.empty())
	{
		graphics->AddStrokeToken(CLEAR_STROKE);
		return AVM1Value::undefinedVal;
	}

	AVM1_ARG_UNPACK_NAMED(unpacker);
	LINESTYLE2 style(0xff);
	// TODO: Use twips instead of float.
	number_t width;
	style.Color = [&](AVM1ArgUnpacker& unpacker)
	{
		uint32_t color;
		number_t alpha;
		AVM1_ARG_CHECK_RET(unpacker(color)(alpha, 100.0), RGBA());
		return RGBA(color, uint8_t
		(
			fclamp(alpha, 0, 100) /
			100.0 *
			255.0
		));
	}(unpacker(width));
	style.Width = dclamp(width, 0, 255);

	std::tie
	(
		style.NoHScaleFlag,
		style.NoVScaleFlag
	) = [&](AVM1ArgUnpacker& unpacker)
	{
		tiny_string scaleMode;
		AVM1_ARG_CHECK_RET
		(
			unpacker(scaleMode),
			std::make_pair(true, true)
		);

		if (scaleMode == "none")
			return std::make_pair(false, false);
		else if (scaleMode == "horizontal")
			return std::make_pair(false, true);
		else if (scaleMode == "vertical")
			return std::make_pair(true, false);

		return std::make_pair(true, true);
	}(unpacker(style.PixelHintingFlag, false));

	style.JointStyle = [&](AVM1ArgUnpacker& unpacker)
	{
		tiny_string joinStyle;
		AVM1_ARG_CHECK_RET
		(
			unpacker(joinStyle),
			LineJoinStyle::Round
		);

		if (joinStyle == "bevel")
			return LineJoinStyle::Bevel;
		else if (joinStyle == "miter")
		{
			number_t miterLimit;
			unpacker(miterLimit, 3);
			style.MiterLimitFactor = miterLimit * 256.0;
			return LineJoinStyle::Miter;
		}

		return LineJoinStyle::Round;
	}(unpacker.template unpack<LineCapStyle>
	(
		style.StartCapStyle,
		LineCapStyle::Round
	));

	graphics->AddStrokeToken(SET_STROKE);
	graphics->AddLineStyleToken(graphics->addLineStyle(style));
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(lineGradientStyle)
{
}

AVM1_MOVIECLIP_FUNC_BODY(beginFill)
{
}

AVM1_MOVIECLIP_FUNC_BODY(beginBitmapFill)
{
}

AVM1_MOVIECLIP_FUNC_BODY(beginGradientFill)
{
}

AVM1_MOVIECLIP_FUNC_BODY(moveTo)
{
}

AVM1_MOVIECLIP_FUNC_BODY(lineTo)
{
}

AVM1_MOVIECLIP_FUNC_BODY(curveTo)
{
}

AVM1_MOVIECLIP_FUNC_BODY(endFill)
{
}

AVM1_MOVIECLIP_FUNC_BODY(clear)
{
}

AVM1_MOVIECLIP_FUNC_BODY(attachMovie)
{
}

AVM1_MOVIECLIP_FUNC_BODY(createEmptyMovieClip)
{
}

AVM1_MOVIECLIP_FUNC_BODY(createTextField)
{
}

AVM1_MOVIECLIP_FUNC_BODY(duplicateMovieClip)
{
}
