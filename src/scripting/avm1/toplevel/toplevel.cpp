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

#include <sstream>

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

AVM1_FUNCTION_DECL(trace)
{
	// NOTE: Unlike `ActionTrace`, `_global.trace()` always converts
	// `undefined` to `""` in SWF 6, and earlier. It also doesn't log
	// anything outside of Flash's (the authoring tool) trace window.
	const auto& undefVal = AVM1Value::undefinedVal;
	auto str = (args.empty() ? undefVal : args[0]).toString(act);
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
	// NOTE: ECMA-262 says `parseInt()` returns `NaN`, but AVM1 returns
	// `undefined` instead.
	if (args.empty())
		return AVM1Value::undefinedVal;

	AVM1_ARG_UNPACK_NAMED(unpacker);
	return parseIntImpl(act, unpacker[0], unpacker.tryGetArg(1))
}

AVM1_FUNCTION_DECL(parseFloat)
{
	if (args.empty())
		return AVM1Value::undefinedVal;
	return parseFloatImpl(args[0].toString(act), false);
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
	const auto& undefVal = AVM1Value::undefinedVal;
	act.getSys()->intervalManager->clearInterval
	(
		(args.empty() ? undefVal : args[0]).toInt32(act),
		false
	);
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_DECL(setInterval)
{
	return createTimer<false>(act, _this, args);
}

AVM1_FUNCTION_DECL(clearTimeout)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	act.getSys()->intervalManager->clearInterval
	(
		(args.empty() ? undefVal : args[0]).toInt32(act),
		true
	);
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
	if (args.empty())
		return AVM1Value::undefinedVal;

	return URLInfo::encode(args[0].toString(act), URLInfo::ENCODE_ESCAPE);
}

AVM1_FUNCTION_DECL(unescape)
{
	if (args.empty())
		return AVM1Value::undefinedVal;

	return URLInfo::decode(args[0].toString(act), URLInfo::ENCODE_ESCAPE);
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

static constexpr auto globalDecls = makeArray
(
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
);
