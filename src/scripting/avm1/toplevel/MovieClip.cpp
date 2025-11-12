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
#include "scripting/avm1/toplevel/Matrix.h"
#include "scripting/avm1/toplevel/MovieClip.h"
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
	act.getPrototypes().movieClip->proto
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

#define MOVIECLIP_FUNC_PROTO(name, ...) \
	AVM1_FUNCTION_TYPE_PROTO(MovieClip, name, propFlags<__VA_ARGS__>)

#define MOVIECLIP_PROP_PROTO(name, funcName, ...) AVM1_PROPERTY_TYPE_PROTO \
( \
	MovieClip, \
	name, \
	funcName, \
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
	AVM1_FUNCTION_PROTO(removeMovieClip, protoFlags<>),
	MOVIECLIP_FUNC_PROTO(setMask, 6),
	MOVIECLIP_FUNC_PROTO(startDrag),
	MOVIECLIP_FUNC_PROTO(stop),
	MOVIECLIP_FUNC_PROTO(stopDrag),
	MOVIECLIP_FUNC_PROTO(swapDepths),
	MOVIECLIP_FUNC_PROTO(unloadMovie),

	MOVIECLIP_PROP_PROTO(blendMode, BlendMode, 8),
	// NOTE: `filters` is defined in `AVM1DisplayObject`.
	MOVIECLIP_PROP_PROTO(cacheAsBitmap, CacheAsBitmap, 8),
	MOVIECLIP_PROP_PROTO(opaqueBackground, OpaqueBackground, 8),
	// NOTE: `enabled` is defined in `AVM1DisplayObject`.
	MOVIECLIP_PROP_PROTO(_lockroot, LockRoot),
	MOVIECLIP_PROP_PROTO(scrollRect, ScrollRect, 8),
	MOVIECLIP_PROP_PROTO(scale9Grid, Scale9Grid, 8),
	MOVIECLIP_PROP_PROTO(transform, Transform, 8),
	AVM1Decl("useHandCursor", true, protoFlags<false>),
	// NOTE: `focusEnabled` isn't a builtin property of `MovieClip`.
	// NOTE: `tabEnabled` isn't a builtin property of `MovieClip`.
	// NOTE: `tabIndex` is defined in `AVM1DisplayObject`.
	// NOTE: `tabChildren` isn't a builtin property of `MovieClip`.
};

#undef MOVIECLIP_FUNC_PROTO
#undef MOVIECLIP_PROP_PROTO

GcPtr<AVM1SystemClass> AVM1MovieClip::createClass
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

AVM1Value AVM1MovieClip::makeRect(AVM1Activation& act, const RectF& rect)
{
	auto proto = act.getPrototypes().rectangle->ctor;
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
	data->afterCreation(NullGc, ObjectCreator::AVM1, true);
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
		graphics->setLineStyle({});
		return AVM1Value::undefinedVal;
	}

	AVM1_ARG_UNPACK_NAMED(unpacker);

	// TODO: Use twips instead of float.
	number_t width;
	RGBA color;
	bool pixelHinting;
	auto scaleMode = unpacker(width)(color)
	(
		pixelHinting,
		false
	).tryUnpack
	<
		tiny_string
	>().transformOr(std::make_pair(true, true), [&](const auto& str)
	{
		if (str == "none")
			return std::make_pair(false, false);
		else if (str == "horizontal")
			return std::make_pair(false, true);
		else if (str == "vertical")
			return std::make_pair(true, false);

		return std::make_pair(true, true);
	}(unpacker(width)(color)(pixelHinting, false));

	LineCapStyle capStyle;
	FIXED8 miterLimit(0x300);
	auto joinStyle = unpacker(capStyle, LineCapStyle::Round).tryUnpack
	<
		tiny_string
	>().transformOr(LineJoinStyle::Round, [&](const auto& str)
	{
		if (str == "bevel")
			return LineJoinStyle::Bevel;
		else if (str == "miter")
		{
			number_t _miterLimit;
			unpacker(_miterLimit, 3);
			miterLimit = _miterLimit * 256.0;
			return LineJoinStyle::Miter;
		}

		return LineJoinStyle::Round;
	});

	LINESTYLE2 style(0xff);
	style.Width = dclamp(width, 0, 255);
	style.Color = color;
	style.PixelHintingFlag = pixelHinting;
	std::tie(style.NoHScaleFlag, style.NoVScaleFlag) = allowScale;
	style.StartCapStyle = scaleMode;
	style.JointStyle = joinStyle;

	graphics->setLineStyle(style);
	return AVM1Value::undefinedVal;
}

static Optional<std::vector<GRADRECORD>> buildGradRecords
(
	AVM1Activation& act,
	const tiny_string& funcName,
	const GcPtr<AVM1Object>& colors,
	const GcPtr<AVM1Object>& alphas,
	const GcPtr<AVM1Object>& ratios
)
{
	auto numColors = colors->getLength(act);
	auto numAlphas = alphas->getLength(act);
	auto numRatios = ratios->getLength(act);
	if (numColors != numAlphas || numColors != numRatios)
	{
		LOG
		(
			LOG_ERROR,
			funcName << ": Received different size arrays for `colors`, "
			"`alphas`, and `ratios`."
		);
		return nullOpt;
	}

	std::vector<GRADRECORD> records;
	records.reserve(numColors);

	for (size_t i = 0; i < numColors; ++i)
	{
		GRADRECORD record(0xff);
		auto color = colors->getElement(act, i).toUInt32(act);
		auto ratio = ratios->getElement(act, i).toNumber(act);
		auto alpha = dclamp
		(
			alphas->getElement(act, i).toNumber(act),
			0,
			100
		);

		if (ratio <= 0 || ratio >= 256)
		{
			LOG
			(
				LOG_ERROR,
				funcName << ": Received an invalid ratio value " << ratio
			);
			return nullOpt;
		}

		record.Color = RGBA(color, uint8_t(alpha / 100.0 * 255.0));
		record.Ratio = ratio;
		records.push_back(record);
	}
	return records;
}

static Optional<FILL_STYLE_TYPE> parseGradType(const tiny_string& type)
{
	if (type == "linear")
		return LINEAR_GRADIENT;
	else if (type == "radial")
		return RADIAL_GRADIENT;
	return {};
}

static GradientSpreadMode parseSpreadMethod(const tiny_string& method)
{
	if (type == "reflect")
		return GradientSpreadMode::Reflect;
	else if (type == "repeat")
		return GradientSpreadMode::Repeat;
	return GradientSpreadMode::Pad;
}

static GradientInterpMethod parseInterpMethod(const tiny_string& method)
{
	if (type == "linearRGB")
		return GradientInterpMethod::LinearRGB;
	return GradientInterpMethod::RGB;
}

AVM1_MOVIECLIP_FUNC_BODY(lineGradientStyle)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	tiny_string type;
	AVM1Value colorsVal;
	AVM1Value alphasVal;
	AVM1Value ratiosVal;
	AVM1Value matrixVal;
	AVM1_ARG_CHECK(unpacker(type)(colorsVal)(alphasVal)(ratiosVal)(matrixVal));

	// Silently fail, if too many arguments are passed.
	if (args.size() > 8)
		return AVM1Value::undefinedVal;

	auto gradType = parseGradType(type);
	if (!gradType.hasValue())
	{
		LOG(LOG_ERROR, "lineGradientStyle(): Received invalid fill type " << type);
		return AVM1Value::undefinedVal;
	}

	auto records = buildGradRecords
	(
		act,
		"lineGradientStyle()",
		colorsVal.toObject(act),
		alphasVal.toObject(act),
		ratiosVal.toObject(act),
	);

	if (!records.hasValue())
		return AVM1Value::undefinedVal;

	auto matrix = AVM1Matrix::gradObjToMatrix
	(
		matrixVal.toObject(act)
	);

	tiny_string spreadMethod;
	tiny_string interpMethod;
	number_t focalPoint;
	unpack(spreadMethod, "")(interpMethod, "")(focalPoint, 0);

	auto spread = parseSpreadMethod(spreadMethod);
	auto interp = parseInterpMethod(interpMethod);

	#if 1
	_this->getGraphics()->lineGradientStyle
	(
		*gradType,
		records,
		matrix,
		spread,
		interp,
		focalPoint
	);
	#else
	auto graphics = _this->getGraphics();

	LINESTYLE2 style(0xff);
	style.Width = graphics->getCurrentLineWidth();
	style.HasFillFlag = true;

	auto& fillStyle = style.FillType;
	fillStyle.FillStyleType = *gradType;
	fillStyle.Matrix = matrix;

	auto& grad = fillStyle.Gradient;
	grad.FocalPoint = FIXED8(focalPoint * 16384.0);
	grad.GradientRecords = *records;
	grad.SpreadMethod = spread;
	grad.InterpolationModeMethod = interp;

	graphics->AddStrokeToken(SET_STROKE);
	graphics->AddLineStyleToken(graphics->addLineStyle(style));
	#endif
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(beginFill)
{
	if (args.empty())
		return endFill(act, _this, args);

	auto graphics = _this->getGraphics();
	graphics->dorender(true);

	uint32_t color;
	number_t alpha;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(color)(alpha, 100.0));

	#if 1
	_this->getGraphics()->beginFill(color, alpha / 100.0);
	#else
	graphics->setInFilling(true);

	auto style = Graphics::createSolidFill(color, uint8_t
	(
		fclamp(alpha, 0, 100) /
		100.0 *
		255.0
	));
	graphics->AddFillToken(SET_FILL);
	graphics->AddFillStyleToken(graphics->AddFillStyle(style));
	#endif
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(beginBitmapFill)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	AVM1_ARG_UNPACK_NAMED(unpacker);

	NullableGcPtr<AVM1BitmapData> data;
	AVM1_ARG_CHECK_RET(unpacker(data), endFill(act, _this, args));

	if (data.isNull())
		return endFill(act, _this, args);

	AVM1Value matrixVal;
	bool repeat;
	bool smoothing;
	unpacker(matrixVal, undefVal)(repeat, true)(smoothing, false);

	auto matrix = AVM1Matrix::objToMatrixOrIdent(act, matrixVal);

	#if 1
	_this->getGraphics()->beginBitmapFill
	(
		data,
		matrix,
		repeat,
		smoothing
	);
	#else
	auto graphics = _this->getGraphics();
	graphics->dorender(true);
	graphics->setInFilling(true);

	auto style = Graphics::createBitmapFill
	(
		data,
		matrix,
		repeat,
		smoothing
	);
	graphics->AddFillToken(SET_FILL);
	graphics->AddFillStyleToken(graphics->AddFillStyle(style));
	#endif
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(beginGradientFill)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	AVM1_ARG_UNPACK_NAMED(unpacker);

	if (unpacker[0].valueOr(undefVal).is<UndefinedVal>())
	{
		// The path has no fill style, if the first argument is either
		// `undefined`, or no arguments were passed.
		//
		// NOTE: `endFill()` is called here to remove the fill style from
		// the path.
		return endFill(act, _this, args);
	}

	tiny_string type;
	AVM1Value colorsVal;
	AVM1Value alphasVal;
	AVM1Value ratiosVal;
	AVM1Value matrixVal;


	AVM1_ARG_CHECK(unpacker(type)(colorsVal)(alphasVal)(ratiosVal)(matrixVal));

	// Silently fail, if too many arguments are passed.
	if (args.size() > 8 || (args.size() > 5 && act.getSwfVersion() < 8))
		return AVM1Value::undefinedVal;

	auto gradType = parseGradType(type);
	if (!gradType.hasValue())
	{
		LOG(LOG_ERROR, "beginGradientFill(): Received invalid fill type " << type);
		return AVM1Value::undefinedVal;
	}

	auto records = buildGradRecords
	(
		act,
		"beginGradientFill()",
		colorsVal.toObject(act),
		alphasVal.toObject(act),
		ratiosVal.toObject(act),
	);

	if (!records.hasValue())
		return AVM1Value::undefinedVal;

	auto matrix = AVM1Matrix::gradObjToMatrix
	(
		matrixVal.toObject(act)
	);

	tiny_string spreadMethod;
	tiny_string interpMethod;
	number_t focalPoint;
	unpack(spreadMethod, "")(interpMethod, "")(focalPoint, 0);

	auto spread = parseSpreadMethod(spreadMethod);
	auto interp = parseInterpMethod(interpMethod);

	_this->getGraphics()->begiGradientFill
	(
		*gradType,
		records,
		matrix,
		spread,
		interp,
		focalPoint
	);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(moveTo)
{
	if (args.size() < 2)
		return AVM1Value::undefinedVal;

	Vector2f pos;
	AVM1_ARG_UNPACK(pos);

	_this->getGraphics()->moveTo(pos);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(lineTo)
{
	if (args.size() < 2)
		return AVM1Value::undefinedVal;

	Vector2f pos;
	AVM1_ARG_UNPACK(pos);

	_this->getGraphics()->lineTo(pos);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(curveTo)
{
	if (args.size() < 4)
		return AVM1Value::undefinedVal;

	Vector2f control;
	Vector2f anchor;
	AVM1_ARG_UNPACK(control)(anchor);

	#if 1
	_this->getGraphics()->curveTo(control, anchor);
	#else
	auto graphics = _this->getGraphics();
	graphics->updateTokenBounds(control);
	graphics->updateTokenBounds(anchor);

	if (graphics->isFilling())
	{
		graphic->AddFillToken(CURVE_QUADRATIC);
		graphic->AddFillToken(control);
		graphic->AddFillToken(anchor);
	}

	if (graphics->hasLineStyle())
	{
		graphic->AddStrokeToken(CURVE_QUADRATIC);
		graphic->AddStrokeToken(control);
		graphic->AddStrokeToken(anchor);
	}

	graphics->setHasChanged(true);
	if (graphics->isFilling())
		graphics->dorender(false);
	#endif
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(curveTo)
{
	if (args.size() < 4)
		return AVM1Value::undefinedVal;

	Vector2f control;
	Vector2f anchor;
	AVM1_ARG_UNPACK(control)(anchor);

	_this->getGraphics()->curveTo(control, anchor);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(endFill)
{
	_this->getGraphics()->endFill();
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(clear)
{
	_this->getGraphics()->clear();
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(attachMovie)
{
	if (args.size() < 3)
	{
		LOG(LOG_ERROR, "MovieClip.attachMovie(): Too few arguments.");
		return AVM1Value::undefinedVal;
	}

	tiny_string exportName;
	tiny_string newInstanceName;
	int32_t depth;
	AVM1Value initObjVal;
	AVM1_ARG_UNPACK(exportName)(newInstanceName)(depth)(initObjVal);

	depth += AVM1depthOffset;

	// TODO: What's the derivation of this value? It shows up a few times
	// in the AVM... `2^31 - 16777220`.
	if (depth < 0 || depth > AVM1maxDepth)
		return AVM1Value::undefinedVal;

	auto newClip = _this->getMovie()->createClipByExportName
	(
		act.getGcCtx(),
		exportName
	);

	if (newClip.isNull())
	{
		LOG
		(
			LOG_ERROR,
			"MovieClip.attachMovie(): Failed to attach " << exportName
		);
		return AVM1Value::undefinedVal;
	}

	newClip->setPlacedByAVM1(true);

	// Set the name, and attach the new clip to the parent.
	newClip->setName(newInstanceName);
	_this->replaceLegacyChildAt(depth, newClip);
	newClip->afterCreation(initObjVal, ObjectCreator::AVM1, true);
	return newClip->toAVM1ValueOrUndef(act);
}

AVM1_MOVIECLIP_FUNC_BODY(createEmptyMovieClip)
{
	if (args.size() < 2)
	{
		LOG(LOG_ERROR, "MovieClip.createEmptyMovieClip(): Too few arguments.");
		return AVM1Value::undefinedVal;
	}

	tiny_string newInstanceName;
	int32_t depth;
	AVM1_ARG_UNPACK(newInstanceName)(depth);

	depth += AVM1depthOffset;


	// Create the empty `MovieClip`.
	auto newClip = NEW_GC_PTR(act.getGcCtx(), MovieClip
	(
		act.getGcCtx(),
		_this->getMovie()
	));

	newClip->setPlacedByAVM1(true);

	// Set the name, and attach the new clip to the parent.
	newClip->setName(newInstanceName);
	_this->replaceLegacyChildAt(depth, newClip);
	newClip->afterCreation(NullGc, ObjectCreator::AVM1, true);
	return newClip->toAVM1ValueOrUndef();
}

AVM1_MOVIECLIP_FUNC_BODY(createTextField)
{
	AVM1Value instanceNameVal;
	int32_t depth;
	RECT rect;
	AVM1_ARG_UNPACK(instanceNameVal)(depth)(rect);

	depth += AVM1depthOffset;

	auto textField = NEW_GC_PTR(act.getGcCtx(), TextField
	(
		act.getGcCtx(),
		act.getBaseClip()->getMovie(),
		rect
	));

	textField->setName(instanceNameVal.toString(act));
	_this->replaceLegacyChildAt(depth, textField);
	textField->afterCreation(NullGc, ObjectCreator::AVM1, true);

	return
	(
		act.getSwfVersion() >= 8 ?
		// NOTE: In SWF 8, and later, `createTextField()` returns the
		// newly created `TextField`.
		textField->toAVM1ValueOrUndef() :
		AVM1Value::undefinedVal
	);

}

AVM1_MOVIECLIP_FUNC_BODY(duplicateMovieClip)
{
	if (args.empty())
	{
		LOG(LOG_ERROR, "MovieClip.duplicateMovieClip(): Too few arguments.");
		return AVM1Value::undefinedVal;
	}

	tiny_string name;
	int32_t depth;
	NullableGcPtr<AVM1Object> initObj;
	// NOTE: Despite the AS1/2 docs stating that `initObject` is only
	// supported in Flash Player 6, and later, it isn't version gated.
	AVM1_ARG_UNPACK(name)(depth, 0)(initObj);


	auto newClip = cloneSprite
	(
		act.getGcCtx(),
		_this,
		name,
		// NOTE: `duplicateMovieClip()` uses an offset/biased depth,
		// compared to `ActionCloneSprite`.
		depth + AVM1depthOffset,
		initObj
	);

	return
	(
		act.getSwfVersion() <= 5 ?
		// NOTE: In SWF 5, and earlier, `undefined` is returned.
		AVM1Value::undefinedVal :
		newClip->toAVM1ValueOrUndef()
	);
}

NullableGcPtr<MovieClip> AVM1MovieClip::cloneSprite
(
	const GcContext& ctx,
	const GcPtr<MovieClip>& clip,
	const tiny_string& target,
	uint16_t depth,
	const NullableGcPtr<AVM1Object>& initObj
)
{
	auto parent = clip->getAVM1Parent()->as<MovieClip>();
	if (parent.isNull())
	{
		// Can't clone the root clip!
		return NullGc;
	}

	// TODO: What's the derivation of this value? It shows up a few times
	// in the AVM... `2^31 - 16777220`.
	if (depth < 0 || depth > AVM1maxDepth)
		return NullGc;

	auto movie = _this->getMovie();
	auto newClip =
	(
		clip->getTagID() != UINT32_MAX ?
		// A `MovieClip` from an SWF; create a new copy of it.
		movie->createClipById(ctx, clip->getTagID()) :
		// A dynamically created `MovieClip`; create a new, empty
		// `MovieClip`.
		NEW_GC_PTR(ctx, MovieClip(ctx, movie)
	);

	newClip->setPlacedByAVM1(true);

	// Set the name, and attach the new clip to the parent.
	newClip->setName(target);
	parent->replaceLegacyChildAt(depth, newClip);

	// Copy all display properties from the previous clip, into the new
	// clip.
	newClip->setMatrix(clip->getMatrix());
	newClip->setColorTransform(clip->getColorTransform());
	newClip->setupActions(clip->getClipActions())

	if (clip->hasGraphics())
		newClip->setGraphics(clip->getGraphics());

	// TODO: Are there any other properties that should be copied?
	// Certainly not `Object` properties.

	newClip->afterCreation(initObj, ObjectCreator::AVM1, true);
	return newClip;
}

AVM1_MOVIECLIP_FUNC_BODY(getBytesLoaded)
{
	return number_t(_this->getBytesLoaded());
}

AVM1_MOVIECLIP_FUNC_BODY(getBytesTotal)
{
	return number_t(_this->getBytesTotal());
}

AVM1_MOVIECLIP_FUNC_BODY(getInstanceAtDepth)
{
	if (act.getSwfVersion() < 7)
		return AVM1Value::undefinedVal;

	if (args.empty())
	{
		LOG(LOG_ERROR, "MovieClip.getInstanceAtDepth(): Too few arguments.");
		return AVM1Value::undefinedVal;
	}

	auto depth = args[0].toInt32(act) + AVM1depthOffset;

	auto child = _this->tryGetLegacyChildAt(depth);
	if (child.isNull())
		return AVM1Value::undefinedVal;

	// If the child doesn't have an AVM1 object, then return `_this`.
	// NOTE: This behaviour was guessed by `adrian17` of the Ruffle team
	// from observing the behaviour of `TextField`, and `Shape`'; He
	// didn't test other `DisplayObject`s like `BitmapData`, `MorphShape`,
	// or `Video`, nor `DisplayObject`s that weren't fully initialized
	// yet.
	auto obj = child->toAVM1Object();
	return !obj.isNull() ? obj : _this->toAVM1ValueOrUndef(act);
}

AVM1_MOVIECLIP_FUNC_BODY(getNextHighestDepth)
{
	if (act.getSwfVersion() < 7)
		return AVM1Value::undefinedVal;

	auto depth = _this->getHighestDepth() - AVM1depthOffset - 1;
	return number_t(imax(depth, 0));
}

AVM1_MOVIECLIP_FUNC_BODY(gotoAndPlay)
{
	return gotoFrame(act, _this, args, false, 0);
}

AVM1_MOVIECLIP_FUNC_BODY(gotoAndStop)
{
	return gotoFrame(act, _this, args, true, 0);
}

AVM1_MOVIECLIP_FUNC_BODY(gotoFrame, bool stop, uint16_t sceneOffset)
{
	using CallFrameType = Optional<std::pair
	<
		GcPtr<MovieClip>,
		int32_t
	>>>;

	AVM1_ARG_UNPACK_NAMED(unpacker);

	CallFrameType callFrame = [&](const AVM1Value& arg)
	{
		// NOTE: A direct goto only runs, if `n` is an integer.
		auto n = arg.tryAs<number_t>();
		if (n.hasValue() && *n != int32_t(*n))
		{
			// Frame number.
			// Going to frame 0, or lower has no effect.
			// Going to frame `_totalframes + 1`, or higher will go to
			// the last frame.
			// NOTE: This wraps around as an `int32_t`.
			// NOTE: The scene offset is only used by `ActionGotoFrame2`.
			return std::make_pair(_this, arg.toInt32(act));
		}

		// Convert the value into a `String`, and search for a frame
		// label.
		// NOTE: This can direct other clips than then one that this
		// method was called with!
		auto pair = act.resolveVariablePath(_this, arg.toString(act));
		if (!pair.hasValue() || !pair->first->is<MovieClip>())
			return {};

		auto clip = pair->first->as<MovieClip>();
		const auto& frame = pair->second;
		// TODO: Use `AVM1Value`'s `number_t` to `int32_t` conversion,
		// once thats moved out of `AVM1Value`.
		return frame.tryToNumber<number_t>().andThen([&](int32_t frame)
		{
			// First, try to parse it as a frame number.
			return makeOptional(std::make_pair(clip, frame));
		}).orElse([&]
		{
			auto frameNum = clip->frameLabelToNum(frame);
			if (!frameNum.hasValue())
				return {};
			// Otherwise, it's a frame label.
			return std::make_pair(clip, *frameNum);
		});
	}(unpacker[0].valueOr(AVM1Value::undefinedVal));

	if (!callFrame.hasValue())
		return AVM1Value::undefinedVal;

	auto clip = callFrame->first;
	auto frame = callFrame->second - 1 + sceneOffset;

	if (frame > 0)
		clip->gotoFrame(frame, stop);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(nextFrame)
{
	_this->nextFrame();
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(play)
{
	_this-setPlaying();
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(prevFrame)
{
	_this->prevFrame();
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_BODY(AVM1MovieClip, removeMovieClip)
{
	// NOTE: `removeMvovieClip()` can remove any type of `DisplayObject`,
	// e.g. `MovieClip.prototype.removeMovieClip.apply(textField);`.
	auto dispObj = _this->as<DisplayObject>();
	if (!dispObj.isNull())
		AVM1DisplayObject::removeDisplayObject(act, dispObj);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(setMask)
{
	if (args.empty())
		return AVM1Value::undefinedVal;

	if (args[0].isNullOrUndefined())
	{
		_this->setMask(NullGc);
		return true;
	}

	auto mask = act.resolveTargetClip
	(
		act.getTargetOrRootClip(),
		args[0],
		false
	);

	if (mask.isNull())
		return false;

	_this->setMask(mask);
	return true;
}

AVM1_MOVIECLIP_FUNC_BODY(startDrag)
{
	bool lockCenter;
	Optional<RectF> bounds;
	AVM1_ARG_UNPACK(lockCenter, false)(bounds);

	startDragImpl(act, _this, lockCenter, bounds);
	return AVM1Value::undefinedVal;
}

void AVM1MovieClip::startDragImpl
(
	AVM1Activation& act,
	const GcPtr<DisplayObject>& clip,
	bool lockCenter,
	const Optional<RectF>& _bounds
)
{
	auto bounds = _bounds.andThen([&](const auto& bounds)
	{
		// NOTE: Invalid values return `0`.
		Vector2Twips min;
		if (std::isfinite(bounds.min.x))
			min.x = bounds.min.x;
		if (std::isfinite(bounds.min.y))
			min.y = bounds.min.y;

		Vector2Twips max;
		if (std::isfinite(bounds.max.x))
			max.x = bounds.max.x;
		if (std::isfinite(bounds.max.y))
			max.y = bounds.max.y;

		// Normalize the bounds.
		if (max.x < min.x)
			std::swap(min.x, max.x);
		if (max.y < min.y)
			std::swap(min.y, max.y);
		return makeOptional(ValidRectTwips { min, max });
	});

	auto sys = act.getSys();
	sys->setDragObject(DragObject
	(
		clip,
		sys->getInputThread()->getMousePos(),
		lockCenter,
		bounds
	));
}

AVM1_MOVIECLIP_FUNC_BODY(stop)
{
	_this->setStopped();
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(stopDrag)
{
	// NOTE: It doesn't matter which clip we call this on; it stops any
	// active drag.
	act.getSys()->setDragObject({});
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(swapDepths)
{
	if (_this->isAVM1Removed())
		return AVM1Value::undefinedVal;

	auto parent = _this->getAVM1Parent()->as<MovieClip>();
	if (parent.isNull())
		return AVM1Value::undefinedVal;

	AVM1Value value;
	AVM1_ARG_UNPACK(value);

	auto depth = value.tryAs<number_t>().andThen([&](const auto&)
	{
		return makeOptional(value.toInt32(act) + AVM1depthOffset);
	}).orElse([&]
	{
		auto target = act.resolveTargetClip(_this, value, false);
		if (target.isNull())
		{
			LOG(LOG_ERROR, "MovieClip.swapDepths(): Invalid target.");
			return {};
		}

		auto targetParent = target->getAVM1Parent();
		if (targetParent.isNull())
			return {};
		if (targetParent == parent && !target->isAVM1Removed())
			return target->getDepth();

		LOG(LOG_ERROR, "MovieClip.swapDepths(): Objects don't have the same parent.");
		return {};
	});

	if (!depth.hasValue())
		return AVM1Value::undefinedVal;

	if (*depth < 0 || *depth > AVM1maxDepth)
	{
		// The depth is out of range, do nothing.
		return AVM1Value::undefinedVal;
	}

	if (depth == _this->getDepth())
		return AVM1Value::undefinedVal;

	parent->swapLegacyChildAt(depth, _this);
	_this->setTransformedByActionScript(true);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(localToGlobal)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	if (args.empty() || !args[0].is<AVM1Object>())
	{
		LOG(LOG_ERROR, "MovieClip.localToGlobal(): Missing `point` argument.");
		return undefVal;
	}

	auto point = args[0].toObject(act);
	// NOTE: `localToGlobal()` does no conversions; it fails if the
	// properties aren't numbers. It doesn't search the prototype chain,
	// and ignores virtual properties.
	auto x = point->getLocalProp
	(
		act,
		"x",
		false
	).andThen([&](const auto& val)
	{
		return val.tryAs<number_t>();
	});
	auto y = point->getLocalProp
	(
		act,
		"y",
		false
	).andThen([&](const auto& val)
	{
		return val.tryAs<number_t>();
	});

	if (!x.hasValue() && !y.hasValue())
	{
		LOG(LOG_ERROR, "MovieClip.localToGlobal(): Invalid `x`, and `y` properties.");
		return undefVal;
	}

	Vector2Twips localPoint(*x, *y);
	auto globalPoint = _this->localToGlobal(localPoint, false);

	point->setProp(act, "x", globalPoint.x.toPx());
	point->setProp(act, "y", globalPoint.y.toPx());
	return undefVal;
}

AVM1_MOVIECLIP_FUNC_BODY(getBounds)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);
	auto target = unpacker[0].andThen([&](const auto& val)
	{
		return act.resolveTargetClip(_this, val, false);
	}).valueOr(_this);

	if (target.isNull())
		return AVM1Value::undefinedVal;

	auto& ctx = act.getCtx();
	bool needsNewInvalidBounds = !ctx.getUseNewInvalidBounds() &&
	(
		// NOTE: `useNewInvalidBounds` is set to `true`, if either the
		// activation, or root movie's SWF version is 8, or higher.
		act.getSwfVersion() >= 8 ||
		act.getSys()->getRootMovie()->getSwfVersion() >= 8
	);

	if (needsNewInvalidBounds)
	{
		// NOTE, PLAYER-SPECIFIC: If just the activation's (not the root
		// movie's) SWF version is 8, or higher, and the `MovieClip` is
		// the same as the target, then `useNewInvalidBounds` sometimes
		// isn't set in Flash Player 10.
		ctx.activateUseNewInvalidBounds();
	}

	auto bounds = [&]
	{
		auto bounds = _this->getBounds();
		// Getting the clip's bounds in it's own coordinate space; no
		// AABB transform needed.
		if (_this == target)
			return bounds;

		// Transform the AABB to the target's coordinate space.
		// Compute the matrix to transform into the target's coordinate
		// space, and transform the above AABB.
		// NOTE: This doesn't produce as tight of an AABB as if we called
		// `getBounds()` with the final transform matrix, but this
		// matches Flash Player's behaviour.
		auto globalMatrix = _this->localToGlobalMatrix();
		auto targetMatrix = target->globalToLocalMatrix().valueOr(MATRIX());
		auto targetBounds = bounds * targetMatrix.multiplyMatrix(globalMatrix);

		bool isValid = bounds.isValid() || targetBounds.isValid();

		if (!ctx.getUseNewInvalidBounds() || isValid)
			return targetBounds;

		// If both bounds are invalid, and `useNewInvalidBounds` is
		// `true`, then the returned bounds use a specific invalid value.
		return RectTwips
		{
			Vector2Twips(0x8000000, 0x8000000),
			Vector2Twips(0x8000000, 0x8000000)
		};
	}();

	auto ret = NEW_GC_PTR(act.getGcCtx(), AVM1Object
	(
		act.getGcCtx(),
		ctx.getPrototypes().object->proto
	));

	ret.setProp(act, "xMin", bounds.min.x.toPx());
	ret.setProp(act, "xMax", bounds.max.x.toPx());
	ret.setProp(act, "yMin", bounds.min.y.toPx());
	ret.setProp(act, "yMax", bounds.max.y.toPx());
	return ret;
}

AVM1_MOVIECLIP_FUNC_BODY(getRect)
{
	// TODO: `getRect()` returns the bounds, ignoring strokes, which is
	// always less than, or equal to what `getBounds()` returns.
	// For now, just call `getBounds()`.
	return getBounds(act, _this, args);
}

AVM1_MOVIECLIP_FUNC_BODY(getSWFVersion)
{
	auto swfVersion = _this->getMovie()->getSwfVersion();
	return number_t(swfVersion != 0 ? swfVersion : -1);
}

AVM1_MOVIECLIP_FUNC_BODY(getURL)
{
	// TODO: Figure out the error behaviour, if no arguments were supplied.
	if (args.empty())
		return AVM1Value::undefinedVal;

	auto sys = act.getSys();
	AVM1_ARG_UNPACK_NAMED(unpacker);

	tiny_string url;
	unpacker(url);
	auto command = parseFSCommand(url);

	if (command.hasValue())
	{
		auto cmdArgs = unpacker.peek().toString(act);
		sys->extIfaceManager->handleFSCommand(*command, cmdArgs);
		return AVM1Value::undefinedVal;
	}

	tiny_string window;
	auto vars = unpacker(window, "").tryUnpack
	<
		RequestMethod
	>().andThen([&](const auto& method)
	{
		return makeOptional(std::make_pair
		(
			method,
			act.localsToFormVals()
		));
	});

	sys->navigateToURL(url, window, vars);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(globalToLocal)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	if (args.empty() || !args[0].is<AVM1Object>())
	{
		LOG(LOG_ERROR, "MovieClip.globalToLocal(): Missing `point` argument.");
		return undefVal;
	}

	auto point = args[0].toObject(act);
	// NOTE: `globalToLocal()` does no conversions; it fails if the
	// properties aren't numbers. It doesn't search the prototype chain,
	// and ignores virtual properties.
	auto x = point->getLocalProp
	(
		act,
		"x",
		false
	).andThen([&](const auto& val)
	{
		return val.tryAs<number_t>();
	});
	auto y = point->getLocalProp
	(
		act,
		"y",
		false
	).andThen([&](const auto& val)
	{
		return val.tryAs<number_t>();
	});

	if (!x.hasValue() && !y.hasValue())
	{
		LOG(LOG_ERROR, "MovieClip.globalToLocal(): Invalid `x`, and `y` properties.");
		return undefVal;
	}

	Vector2Twips globalPoint(*x, *y);
	auto localPoint = _this->globalToLocal(globalPoint, false);

	point->setProp(act, "x", localPoint.x.toPx());
	point->setProp(act, "y", localPoint.y.toPx());
	return undefVal;
}

AVM1_MOVIECLIP_FUNC_BODY(loadMovie)
{
	tiny_string url;
	Optional<RequestMethod> method;
	AVM1_ARG_UNPACK(url)(method);

	auto targetObj = _this->toAVM1ObjectOrUndef();
		act.objectToRequest(targetObj, url, method),

	act.getSys()->loaderManager->loadMovieIntoClip
	(
		act.objectToRequest(targetObj, url, method),
		_this,
		{},
		AVM1MovieLoaderData()
	);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(loadVariables)
{
	tiny_string url;
	Optional<RequestMethod> method;
	AVM1_ARG_UNPACK(url)(method);

	auto targetObj = _this->toAVM1ObjectOrUndef();
	act.getSys()->loaderManager->loadFormIntoAVM1Object
	(
		targetObj,
		act.objectToRequest(targetObj, url, method)
	);
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_FUNC_BODY(unloadMovie)
{
	_this->unloadAVM1Movie();
	return AVM1Value::undefinedVal;
}

AVM1_MOVIECLIP_GETTER_BODY(Transform)
{
	auto ctor = act.getPrototypes().transform->ctor;
	return ctor->construct(act, { _this->toAVM1ValueOrUndef() });
}

AVM1_MOVIECLIP_SETTER_BODY(Transform)
{
	auto transform = value.as<AVM1Transform>();
	if (transform.isNull())
		return;

	auto clip = transform->getClip(act);
	if (clip.isNull())
		return;

	_this->setMatrix(clip->getMatrix());
	_this->setColorTransform(clip->getColorTransform());
	_this->hasChanged = true;
	_this->setTransformedByActionScript(true);
}

AVM1_MOVIECLIP_GETTER_BODY(LockRoot)
{
	return _this->getLockRoot();
}

AVM1_MOVIECLIP_SETTER_BODY(LockRoot)
{
	_this->setLockRoot(value.toBool(act.getSwfVersion()));
}

AVM1_MOVIECLIP_GETTER_BODY(BlendMode)
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

AVM1_MOVIECLIP_SETTER_BODY(BlendMode)
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

AVM1_MOVIECLIP_GETTER_BODY(CacheAsBitmap)
{
	return _this->getCacheAsBitmap();
}

AVM1_MOVIECLIP_SETTER_BODY(CacheAsBitmap)
{
	_this->setCacheAsBitmap(value.toBool(act.getSwfVersion()));
}

AVM1_MOVIECLIP_GETTER_BODY(OpaqueBackground)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	auto opaqueBkg = _this->getOpaqueBackground();
	return opaqueBkg.transformOr(undefVal, [&](const auto& color)
	{
		return number_t(color.toUInt());
	});
}

AVM1_MOVIECLIP_SETTER_BODY(OpaqueBackground)
{
	if (value.isNullOrUndefined())
	{
		_this->setOpaqueBackground(nullOpt);
		return;
	}

	_this->setOpaqueBackground(RGB(value.toUInt32(act)));
}
