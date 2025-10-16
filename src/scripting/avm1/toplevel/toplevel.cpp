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

#include "display_object/DisplayObject.h"
#include "gc/context.h"
#include "gc/ptr.h"
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

// Based on Ruffle's `avm1::globals` crate.

AVM1Value parseIntImpl
(
	AVM1Activation& act,
	const tiny_string& str,
	const Optional<int32_t>& radix
)
{
	constexpr auto NaN = std::numeric_limits<number_t>::quiet_NaN();

	if (radix.hasValue() && (*radix < 2 || *radix > 36))
		return NaN;

	auto parseSign = [&](CharIterator& it) -> Optional<number_t>
	{
		if (*it != '+' && *it != '-')
			return {};
		return *it++ == '+' ? 1 : -1;
	};

	auto isOctalDigit = [](uint32_t ch)
	{
		return ch >= '0' && ch <= '7';
	};

	int32_t _radix = radix.valueOr(10);
	bool ignoreSign = false;

	auto it = str.begin();
	bool hasSign = parseSign(it).hasValue();
	bool isZero = it != str.end() && *it++ == '0';
	bool isHex = isZero && (*it == 'x' || *it == 'X');

	// NOTE, FLASH-PLAYER-BUG: Unless the hex prefix (`0x`) is a valid
	// digit sequence in the given radix, the prefixes `[+-]0{x,X}`
	// return `NaN`, instead of 0.
	// Otherwise the sign should be ignored.
	if (isHex && hasSign && radix.valueOr(0) < 34)
		return NaN;
	else if (isHex && hasSign)
	{
		_radix = *radix;
		ignoreSign = true;
		it = str.begin();
	}
	else if (isHex)
	{
		// Auto detect a hexadecimal prefix (`0x`), and strip it.
		// NOTE, FLASH-PLAYER-BUG: The prefix is always stripped,
		// regardless of the radix.
		// `parseInt("0x100", 10) == 100`, instead of 0.
		// `parseInt("0x100", 36) == 1296`, instead of 1540944.

		// NOTE, FLASH-PLAYER-BUG: The prefix is expected before the sign,
		// or any whitespace.
		// `parseInt("0x -10") == -16`, instead of `NaN`.
		// `parseInt(" -0x10") == NaN`, instead of -16.
		_radix = radix.valueOr(16);
		++it;
	}
	else if (isZero && std::all_of(it, str.end(), isOctalDigit))
	{
		// NOTE, ECMA-262 violation: Octal prefixes (`[+-]0`).
		// NOTE: A number with an octal prefix can't contain leading
		// whitespace, or extra trailing characters.
		_radix = 8;
		it = str.begin();
	}

	// Strip any leading whitespace.
	it = std::find_if(it, str.end(), [](uint32_t ch)
	{
		return
		(
			ch != '\t' &&
			ch != '\n' &&
			ch != '\r' &&
			ch != ' '
		);
	});


	// Based on Rust's `char::to_digit()`.
	auto toDigit = [&](uint32_t ch, int32_t radix) -> Optional<int32_t>
	{
		if (radix < 2 || radix > 36)
			return {};

		int32_t value =
		(
			ch > '9' && radix > 10 ?
			// Convert an ASCII letter into it's corresponding value
			// (`[a-zA-Z]` -> `10-35`).
			// NOTE: Other characters will produce values >= 36.
			//
			// NOTE: The subtraction is done inside the `toupper()` call
			// to prevent the value from exceeding `UINT32_MAX - 0x20`.
			toupper(ch - 'A') + 10 :
			// Convert the digit to a value.
			// NOTE: Non digit characters will produce values > 36.
			ch - '0'
		);

		if (value >= radix)
			return {};
		return value;
	};

	auto sign = parseSign(it).filter(ignoreSign).valueOr(1);

	bool empty = true;
	number_t ret = 0;

	for (; it != str.end(); ++it)
	{
		auto digit = toDigit(*it, _radix);
		if (!digit.hasValue())
			break;
		ret = ret * _radix + *digit;
		empty = false;
	}

	return empty ? NaN : std::copysign(ret, sign);
}

template<bool isTimeout>
AVM1_FUNCTION_DECL(createTimer)
{
	// NOTE: `setInterval()` was added in Flash Player 6, but isn't
	// version gated.
	AVM1_ARG_UNPACK_NAMED(unpacker);

	GcPtr<AVM1Object> obj = _this;
	AVM1_ARG_CHECK(unpacker(obj));

	bool isFunc = obj->is<AVM1Executable>();

	int32_t ival;
	std::vector<AVM1Value> funcArgs;
	AVM1_ARG_CHECK(unpacker.next(!isFunc)(ival, [&](const auto& val)
	{
		return !val.is<UndefinedVal>();
	})(funcArgs).prev(!isFunc));

	Impl<IIntervalRunner> callback;
	if (isFunc)
		callback = AVM1IntervalRunner(obj, funcArgs);
	else
	{
		tiny_string methodName;
		unpacker(methodName);
		callback = AVM1IntervalRunner(obj, methodName, funcArgs);
	}

	return number_t(act.getSys()->intervalManager->addTimer
	(
		callback,
		ival,
		isTimeout
	));
}

AVM1_FUNCTION_DECL(trace)
{
	// NOTE: Unlike `ActionTrace`, `_global.trace()` always converts
	// `undefined` to `""` in SWF 6, and earlier. It also doesn't log
	// anything outside of Flash's (the authoring tool) trace window.
	tiny_string str;
	AVM1_ARG_UNPACK(str);
	act.getSys()->trace(str);
	return undefVal;
}

AVM1_FUNCTION_DECL(isFinite)
{
	return !args.empty() && std::isfinite(args[0].toNumber(act));
}

AVM1_FUNCTION_DECL(isNaN)
{
	return !args.empty() && std::isnan(args[0].toNumber(act));
}

AVM1_FUNCTION_DECL(parseInt)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	// NOTE, ECMA-262 violation: `parseInt()` returns `undefined`, instead
	// of `NaN`.
	tiny_string str;
	Optional<int32_t> radix
	AVM1_ARG_CHECK(unpacker(str)(radix));
	return parseIntImpl(act, str, radix)
}

AVM1_FUNCTION_DECL(parseFloat)
{
	tiny_string str;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(str));
	return parseFloatImpl(str, false);
}

// This is an undocumented function that allows AS 2 classes to set the
// flags of a given property. It's not part of `Object.prototype`, which
// both us, and other projects suspect to be a deliberate omission.
AVM1_FUNCTION_DECL(asSetPropFlags)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	GcPtr<AVM1Object> obj;

	if (!unpacker(obj).isValid())
	{
		LOG(LOG_ERROR, "`ASSetPropFlags()` called without an object.");
		return AVM1Value::undefinedVal;
	}

	Optional<tiny_string> propList;
	unpacker(propList, [&](const AVM1Value& val)
	{
		return !val.is<NullVal>();
	});

	if (!unpacker.isValid())
	{
		LOG(LOG_ERROR, "`ASSetPropFlags()` called without a property list.");
		return AVM1Value::undefinedVal;
	}


	AVM1PropFlags setFlags;
	AVM1PropFlags clearFlags;

	unpacker(setFlags, 0)(clearFlags, 0);

	obj->setPropFlags(propList, setFlags, clearFlags);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_DECL(clearInterval)
{
	int32_t id;
	AVM1_ARG_UNPACK(id);
	act.getSys()->intervalManager->clearInterval(id, false);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_DECL(setInterval)
{
	return createTimer<false>(act, _this, args);
}

AVM1_FUNCTION_DECL(clearTimeout)
{
	int32_t id;
	AVM1_ARG_UNPACK(id);
	act.getSys()->intervalManager->clearInterval(id, true);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_DECL(setTimeout)
{
	return createTimer<true>(act, _this, args);
}

AVM1_FUNCTION_DECL(updateAfterEvent)
{
	act.getStage()->forceInvalidation();
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_DECL(escape)
{
	tiny_string str;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(str));
	return URLInfo::encode(str, URLInfo::ENCODE_ESCAPE);
}

AVM1_FUNCTION_DECL(unescape)
{
	tiny_string str;
	AVM1_ARG_CHECK(AVM1_ARG_UNPACK(str));
	return URLInfo::decode(str, URLInfo::ENCODE_ESCAPE);
}

AVM1_FUNCTION_DECL(getNaN)
{
	// NOTE: `NaN` was added in SWF 5.
	if (act.getSwfVersion() <= 4)
		return AVM1Value::undefinedVal;

	return std::numeric_limits<number_t>::quiet_NaN();
}

AVM1_FUNCTION_DECL(getInfinity)
{
	// NOTE: `Infinity` was added in SWF 5.
	if (act.getSwfVersion() <= 4)
		return AVM1Value::undefinedVal;

	return std::numeric_limits<number_t>::infinity();
}

static constexpr auto globalDecls =
{
	AVM1Decl("trace", trace, AVM1PropFlags::DontEnum),
	AVM1Decl("isFinite", isFinite, AVM1PropFlags::DontEnum),
	AVM1Decl("isNaN", isNaN, AVM1PropFlags::DontEnum),
	AVM1Decl("parseInt", parseInt, AVM1PropFlags::DontEnum),
	AVM1Decl("parseFloat", parseFloat, AVM1PropFlags::DontEnum),
	AVM1Decl("ASSetPropFlags", asSetPropFlags, AVM1PropFlags::DontEnum),
	AVM1Decl("clearInterval", clearInterval, AVM1PropFlags::DontEnum),
	AVM1Decl("setInterval", setInterval, AVM1PropFlags::DontEnum),
	AVM1Decl("clearTimeout", clearTimeout, AVM1PropFlags::DontEnum),
	AVM1Decl("setTimeout", setTimeout, AVM1PropFlags::DontEnum),
	AVM1Decl("updateAfterEvent", updateAfterEvent, AVM1PropFlags::DontEnum),
	AVM1Decl("escape", escape, AVM1PropFlags::DontEnum),
	AVM1Decl("unescape", unescape, AVM1PropFlags::DontEnum),
	AVM1Decl("NaN", getNaN, nullptr, AVM1PropFlags::DontEnum),
	AVM1Decl("Infinity", getInfinity, nullptr, AVM1PropFlags::DontEnum)
};

CreateGlobalsType createGlobals(GcContext& ctx)
{
	AVM1DeclContext declCtx(ctx);

	auto objClass = AVM1Object::createClass(declCtx);
	auto funcClass = AVM1FunctionObject::createClass(declCtx);
	auto bcastPair = AsBroadcaster::createClass(declCtx, objClass->proto);
	auto bcastFuncs = bcastPair.funcs;
	auto bcastClass = bcastPair._class;

	auto flash = NEW_GC_PTR(ctx, AVM1Object(ctx, objClass->proto));
	auto external = NEW_GC_PTR(ctx, AVM1Object(ctx, objClass->proto));
	auto geom = NEW_GC_PTR(ctx, AVM1Object(ctx, objClass->proto));
	auto filters = NEW_GC_PTR(ctx, AVM1Object(ctx, objClass->proto));
	auto display = NEW_GC_PTR(ctx, AVM1Object(ctx, objClass->proto));
	auto net = NEW_GC_PTR(ctx, AVM1Object(ctx, objClass->proto));

	#define CREATE_CLASS(name, class, parent, ...) \
		auto name#Class = class::createClass(declCtx, parent##Class->proto, ##__VA_ARGS__)

	CREATE_CLASS(button, AVM1Button, obj);
	CREATE_CLASS(movieClip, AVM1MovieClip, obj);
	CREATE_CLASS(sound, AVM1Sound, obj);
	CREATE_CLASS(styleSheet, AVM1StyleSheet, obj);
	CREATE_CLASS(textField, AVM1TextField, obj);
	CREATE_CLASS(textFormat, AVM1TextFormat, obj);
	CREATE_CLASS(array, AVM1Array, obj);
	auto arrayProto = arrayClass->proto;
	CREATE_CLASS(color, AVM1Color, obj);
	CREATE_CLASS(error, AVM1Error, obj);
	CREATE_CLASS(xmlNode, AVM1XMLNode, obj);
	CREATE_CLASS(string, AVM1String, obj);
	CREATE_CLASS(number, AVM1Number, obj);
	CREATE_CLASS(boolean, AVM1Boolean, obj);
	CREATE_CLASS(loadVars, AVM1LoadVars, obj);
	CREATE_CLASS(localConnection, AVM1LocalConnection, obj);
	CREATE_CLASS(matrix, AVM1Matrix, obj);
	CREATE_CLASS(point, AVM1Point, obj);
	CREATE_CLASS(rectangle, AVM1Rectangle, obj);
	CREATE_CLASS(colorTransform, AVM1ColorTransform, obj);
	CREATE_CLASS(externalIface, AVM1ExternalInterface, obj);
	CREATE_CLASS(movieClipLoader, AVM1MovieClipLoader, obj, bcastFuncs, arrayProto);
	CREATE_CLASS(video, AVM1Video, obj);
	CREATE_CLASS(netStream, AVM1NetStream, obj);
	CREATE_CLASS(netConnection, AVM1NetConnection, obj);
	CREATE_CLASS(xmlSocket, AVM1XMLSocket, obj);
	CREATE_CLASS(contextMenu, AVM1ContextMenu, obj);
	CREATE_CLASS(contextMenuItem, AVM1ContextMenuItem, obj);
	CREATE_CLASS(xml, AVM1XML, xmlNode);
	CREATE_CLASS(date, AVM1Date, obj);
	CREATE_CLASS(transform, AVM1Transform, obj);
	CREATE_CLASS(bitmapFilter, AVM1BitmapFilter, obj);
	CREATE_CLASS(blurFilter, AVM1BlurFilter, bitmapFilter);
	CREATE_CLASS(bevelFilter, AVM1BevelFilter, bitmapFilter);
	CREATE_CLASS(glowFilter, AVM1GlowFilter, bitmapFilter);
	CREATE_CLASS(dropShadowFilter, AVM1DropShadowFilter, bitmapFilter);
	CREATE_CLASS(colorMatrixFilter, AVM1ColorMatrixFilter, bitmapFilter);
	CREATE_CLASS(displacementMapFilter, AVM1DisplacementMapFilter, bitmapFilter);
	CREATE_CLASS(convolutionFilter, AVM1ConvolutionFilter, bitmapFilter);
	CREATE_CLASS(gradientBevelFilter, AVM1GradientBevelFilter, bitmapFilter);
	CREATE_CLASS(gradientGlowFilter, AVM1GradientGlowFilter, bitmapFilter);
	CREATE_CLASS(bitmapData, AVM1BitmapData, obj);
	CREATE_CLASS(fileReference, AVM1FileReference, obj, bcastFuncs, arrayProto);
	CREATE_CLASS(sharedObject, AVM1SharedObject, obj);

	#undef CREATE_CLASS

	auto sel = NEW_GC_PTR(ctx, AVM1Selection(declCtx, bcastFuncs, arrayProto));

	auto system = NEW_GC_PTR(ctx, AVM1System(declCtx));
	auto sysSec = NEW_GC_PTR(ctx, AVM1SystemSecurity(declCtx));
	auto sysCaps = NEW_GC_PTR(ctx, AVM1SystemCapabilities(declCtx));
	auto sysIME = NEW_GC_PTR(ctx, AVM1SystemIME(declCtx, bcastFuncs, arrayProto));

	auto math = NEW_GC_PTR(ctx, AVM1Math(declCtx));
	auto mouse = NEW_GC_PTR(ctx, AVM1Mouse(declCtx, bcastFuncs, arrayProto));
	auto key = NEW_GC_PTR(ctx, AVM1Key(declCtx, bcastFuncs, arrayProto));
	auto stage = NEW_GC_PTR(ctx, AVM1Stage(declCtx, bcastFuncs, arrayProto));
	auto accessibility = NEW_GC_PTR(ctx, AVM1Accessibility
	(
		declCtx,
		bcastFuncs,
		arrayProto
	));

	auto globals = NEW_GC_PTR(ctx, AVM1Global(ctx));
	declCtx.definePropsOn(globals, globalDecls);

	struct GlobalDef
	{
		GcPtr<AVM1Object> obj;
		const char* name;
		GcPtr<AVM1Object> value;
		AVM1PropFlags flags;
	};

	using GlobalDefList = std::initializer_list<GlobalDef>;
	auto defineGlobals = [&](const GlobalDefList& defs)
	{
		for (const auto& def : defs)
			def.obj->defineValue(def.name, def.value, def.flags);
	};

	using PropFlags = AVM1PropFlags;
	constexpr auto dontEnum = PropFlags::DontEnum;

	#define GLOBAL_DEF_FLAGS(obj, name, value, flags) \
		{ obj, name, value, flags }

	#define GLOBAL_DEF(obj, name, value) \
		GLOBAL_DEF_FLAGS(obj, name, value, PropFlags(0))

	#define GLOBAL_CLASS_FLAGS(obj, name, class, flags) \
		GLOBAL_DEF_FLAGS(obj, name, class##Class->ctor, flags)

	#define GLOBAL_CLASS(obj, name, class) \
		GLOBAL_DEF(obj, name, class##Class->ctor)

	#define GLOBAL_CLASS_DONT_ENUM(obj, name, class) \
		GLOBAL_CLASS_FLAGS(obj, name, class, dontEnum)

	auto textFieldCtor = textFieldClass->ctor;
	defineGlobals(GlobalDefList
	{
		GLOBAL_CLASS_DONT_ENUM(globals, "Array", array),
		GLOBAL_CLASS_DONT_ENUM(globals, "AsBroadcaster", bcast),
		GLOBAL_CLASS_DONT_ENUM(globals, "Button", button),
		GLOBAL_CLASS_DONT_ENUM(globals, "Color", color),
		GLOBAL_CLASS_DONT_ENUM(globals, "Error", error),
		GLOBAL_CLASS_DONT_ENUM(globals, "Object", obj),
		GLOBAL_CLASS_DONT_ENUM(globals, "Function", func),
		GLOBAL_CLASS_DONT_ENUM(globals, "LoadVars", loadVars),
		GLOBAL_CLASS_DONT_ENUM(globals, "LocalConnection", localConnection),
		GLOBAL_CLASS_DONT_ENUM(globals, "MovieClip", movieClip),
		GLOBAL_CLASS_DONT_ENUM(globals, "MovieClipLoader", movieClipLoader),
		GLOBAL_CLASS_DONT_ENUM(globals, "Sound", sound),
		GLOBAL_CLASS_DONT_ENUM(globals, "TextField", textField),
		GLOBAL_CLASS_DONT_ENUM(globals, "TextFormat", textFormat),
		GLOBAL_CLASS_DONT_ENUM(globals, "XMLNode", xmlNode),
		GLOBAL_CLASS_DONT_ENUM(globals, "XML", xml),
		GLOBAL_CLASS_DONT_ENUM(globals, "String", string),
		GLOBAL_CLASS_DONT_ENUM(globals, "Number", number),
		GLOBAL_CLASS_DONT_ENUM(globals, "Boolean", boolean),
		GLOBAL_CLASS_DONT_ENUM(globals, "Date", date),
		GLOBAL_CLASS_DONT_ENUM(globals, "SharedObject", sharedObject),
		GLOBAL_CLASS_DONT_ENUM(globals, "ContextMenu", contextMenu),
		GLOBAL_DEF_FLAGS(globals, "Selection", sel, dontEnum),
		GLOBAL_CLASS_DONT_ENUM(globals, "ContextMenuItem", contextMenuItem),
		GLOBAL_DEF_FLAGS(globals, "System", system, dontEnum),
		GLOBAL_DEF_FLAGS(globals, "Math", math, dontEnum),
		GLOBAL_DEF_FLAGS(globals, "Mouse", mouse, dontEnum),
		GLOBAL_DEF_FLAGS(globals, "Key", key, dontEnum),
		GLOBAL_DEF_FLAGS(globals, "Stage", stage, dontEnum),
		GLOBAL_DEF_FLAGS(globals, "Accessibility", accessibility, dontEnum),
		GLOBAL_CLASS_DONT_ENUM(globals, "NetStream", netStream),
		GLOBAL_CLASS_DONT_ENUM(globals, "NetConnection", netConnection),
		GLOBAL_CLASS_DONT_ENUM(globals, "XMLSocket", xmlSocket),

		GLOBAL_DEF_FLAGS(globals, "flash", flash, dontEnum),
		GLOBAL_DEF(flash, "display", display),
		GLOBAL_DEF(flash, "external", external),
		GLOBAL_DEF(flash, "filters", filters),
		GLOBAL_DEF(flash, "geom", geom),
		GLOBAL_DEF(flash, "net", net),

		GLOBAL_CLASS(display, "BitmapData", bitmapData),

		GLOBAL_CLASS(external, "ExternalInterface", externalIface),

		GLOBAL_CLASS(geom, "ColorTransform", colorTransform),
		GLOBAL_CLASS(geom, "Matrix", matrix),
		GLOBAL_CLASS(geom, "Point", point),
		GLOBAL_CLASS(geom, "Rectangle", rectangle),
		GLOBAL_CLASS(geom, "Transform", transform),

		GLOBAL_CLASS(filters, "BevelFilter", bevelFilter),
		GLOBAL_CLASS(filters, "BitmapFilter", bitmapFilter),
		GLOBAL_CLASS(filters, "BlurFilter", blurFilter),
		GLOBAL_CLASS(filters, "ColorMatrixFilter", colorMatrixFilter),
		GLOBAL_CLASS(filters, "ConvolutionFilter", convolutionFilter),
		GLOBAL_CLASS(filters, "DisplacementMapFilter", displacementMapFilter),
		GLOBAL_CLASS(filters, "DropShadowFilter", dropShadowFilter),
		GLOBAL_CLASS(filters, "GradientBevelFilter", gradientBevelFilter),
		GLOBAL_CLASS(filters, "GradientGlowFilter", gradientGlowFilter),
		GLOBAL_CLASS(filters, "GlowFilter", glowFilter),

		GLOBAL_CLASS(net, "FileReference", fileReference),

		GLOBAL_DEF(system, "IME", sysIME),
		GLOBAL_DEF(system, "security", sysSec),
		GLOBAL_DEF(system, "capabilities", sysCaps),

		GLOBAL_CLASS_FLAGS
		(
			textFieldCtor,
			"StyleSheet",
			styleSheet,
			dontEnum | PropFlags::Version7
		),
	});

	#undef GLOBAL_DEF_FLAGS
	#undef GLOBAL_DEF
	#undef GLOBAL_CLASS_FLAGS
	#undef GLOBAL_CLASS
	#undef GLOBAL_CLASS_DONT_ENUM

	return std::make_tuple
	(
		AVM1SystemPrototypes
		{
			buttonClass,
			objClass,
			funcClass,
			movieClipClass,
			soundClass,
			textFieldClass,
			textFormatClass,
			arrayClass,
			xmlNodeClass,
			xmlClass,
			stringClass,
			numberClass,
			booleanClass,
			matrixClass,
			pointClass,
			rectangleClass,
			transformClass,
			sharedObjectClass,
			colorTransformClass,
			contextMenuClass,
			contextMenuItemClass,
			dateClass,
			bitmapDataClass,
			videoClass,
			blurFilterClass,
			bevelFilterClass,
			glowFilterClass,
			dropShadowFilterClass,
			colorMatrixFilterClass,
			displacementMapFilterClass,
			convolutionFilterClass,
			gradientBevelFilterClass,
			gradientGlowFilterClass,
		},
		globals,
		bcastFuncs
	);
}

AVM1_FUNCTION_BODY(lightspark, getDepth)
{
	auto dispObj = _this->as<DisplayObject>();
	if (dispObj == nullptr || act.getSwfVersion() < 6)
		return AVM1Value::undefinedVal;
	return number_t(dispObj->getSWFDepth() - AVM1depthOffset);
}

void removeDisplayObject
(
	AVM1Activation& activation,
	const GcPtr<DisplayObject>& _this
)
{
	auto depth = _this->getSWFDepth();

	// NOTE: This can only remove positive depths (when offset by the AVM
	// depth offset).
	// This usually prevents removing non dynamic clips, but it can be
	// bypassed with `swapDepths()`.
	if (depth < AVM1depthOffset || depth >= AVM1maxRemoveDepth)
		return;

	// Requires a parent to remove from.
	auto parent = _this->getAVM1Parent();
	if (parent == nullptr || !parent->is<MovieClip>())
		return;

	parent->as<MovieClip>()->removeChild(activation.getCtx(), _this);
}
