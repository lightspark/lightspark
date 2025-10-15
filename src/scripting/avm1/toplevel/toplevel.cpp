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
