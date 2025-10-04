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

#include "scripting/avm1/activation.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/scope.h"
#include "scripting/avm1/value.h"
#include "swf.h"
#include "swftypes.h"
#include "tiny_string.h"
#include "utils/optional.h"

using namespace lightspark;

// Based on Ruffle's `avm1::Value`.

#ifdef USE_STRING_ID
AVM1Value::AVM1Value(const tiny_string& _str) :
type(Type::String),
str(getSys()->getUniqueStringId(_str, true)) {}
#else

struct NullUndefVal
{
	NullUndefVal(const NullVal&) {}
	NullUndefVal(const UndefinedVal&) {}
};

bool AVM1Value::operator==(const AVM1Value& other) const
{
	if (type != other.type)
		return false;

	return visit(makeVisitor
	(
		[&](const NullUndefVal&) { return true; },
		[&](bool a) { return a == other._bool; },
		[&](number_t a)
		{
			auto b = other.num;
			// NOTE, PLAYER-SPECIFIC: In Flash Player 7, and later,
			// `NaN == NaN` returns true, while in Flash Player 6, and
			// earlier, `NaN == NaN` returns false. Let's do what Flash
			// Player 7, and later does, and return true.
			//
			// TODO: Allow for using either, once support for
			// mimicking different player versions is added.
			return a == b || (std::isnan(a) && std::isnan(b));
		},
		#ifdef USE_STRING_ID
		[&](uint32_t a) { return a == other.str; },
		#else
		[&](const tiny_string& a) { return a == other.str; },
		#endif
		[&](const _R<AVM1Object>& a) { return a == other.obj; },
		[&](const _R<AVM1MovieClipRef>& a)
		{
			return a->getPath() == other.clip->getPath();
		}
	));
}

number_t AVM1Value::toNumber(AVM1Activation& activation, bool primitiveHint) const
{
	constexpr auto NaN = std::numeric_limits<number_t>::quiet_NaN();
	auto swfVersion = activation.getSwfVersion();

	auto objToNumber = [&](const AVM1Value& value)
	{
		if (primitiveHint)
			return swfVersion > 4 ? NaN : 0;
		return value.toPrimitiveNum(activation).toNumber(activation);
	};

	return visit(makeVisitor
	(
		[&](const NullUndefVal&) { return swfVersion > 6 ? NaN : 0; },
		[&](bool _bool) { return _bool; },
		[&](number_t num) { return num; },
		#ifdef USE_STRING_ID
		[&](uint32_t str) { return strIdToNum(str, swfVersion); },
		#else
		[&](const tiny_string& str) { return strToNum(str, swfVersion); },
		#endif
		[&](const _R<AVM1Object>&) { return objToNumber(*this); },
		[&](const _R<AVM1MovieClipRef>&)
		{
			return objToNum(toObject(activation));
		}
	));
}

AVM1Value AVM1Value::toPrimitiveNum(AVM1Activation& activation) const
{
	return visit(makeVisitor
	(
		[&](const _R<AVM1Object>& obj)
		{
			if (obj->is<AVM1StageObject>())
				return *this;
			return obj->callMethod
			(
				#ifdef USE_STRING_ID
			 	STRING_VALUEOF,
				#else
				"valueOf",
				#endif
				{},
				activation,
				AVM1ExecutionReason::Special
			);
		},
		[&](const auto&) { return *this; }
	));
}

AVM1Value AVM1Value::toPrimitive(AVM1Activation& activation) const
{
	auto swfVersion = activation.getSwfVersion();

	if (isPrimitive())
		return *this;

	auto _obj = is<AVM1Object>() ? obj : toObject(activation);
	auto isDate = _obj->is<AVM1Date>();
	auto val = _obj->callMethod
	(
		// NOTE: In SWF 6, and later, `Date` objects call `toString()`.
		#ifdef USE_STRING_ID
		swfVersion > 5 && isDate ? STRING_TOSTRING : STRING_VALUEOF,
		#else
		swfVersion > 5 && isDate ? "toString" : "valueOf",
		#endif
		{},
		activation,
		AVM1ExecutionReason::Special
	);

	if (is<AVM1MovieClip>() && val.is<UndefinedVal>())
		return toString(activation);
	else if (is<AVM1Object>() && val.isPrimitive())
		return val;

	// If the above conversion returns an object, then the conversion
	// failed, so fall back to the original object.
	return *this;
}

AVM1Value AVM1Value::isLess(const AVM1Value& other, AVM1Activation& activation) const
{
	// If neither argument's `valueOf()` handler return a `MovieClip`,
	// then bail early, and return `false`.
	// This is the common case for `Object`s, since `Object`'s default
	// `valueOf()` handler returns itself.
	// For example, `{} < {}` == `false`.
	auto primA = toPrimitiveNum(activation);
	if (primA.is<AVM1Object>() && !primA.obj->is<AVM1StageObject>())
		return false;

	auto primB = other.toPrimitiveNum(activation);
	if (primB.is<AVM1Object>() && !primB.obj->is<AVM1StageObject>())
		return false;

	if (primA.is<AVM1String>() && primB.is<AVM1String>())
	{
		auto a = primA.str;
		auto b = primB.str;
		#ifdef USE_STRING_ID
		if (a < BUILTIN_STRINGS_CHAR_MAX && b < BUILTIN_STRINGS_CHAR_MAX)
			return a < b;

		auto sys = activation.getSys();
		return
		(
			sys->getStringFromUniqueId(a) <
			sys->getStringFromUniqueId(b)
		);
		#else
		return a < b;
		#endif
	}

	// Convert both values to `Number`s, and compare.
	// If either `Number` is `NaN`, return `undefined`.
	auto a = primA.toNumber(activation, true);
	auto b = primB.toNumber(activation, true);
	if (std::isnan(a) || std::isnan(b))
		return Type::Undefined;
	return a < b;
}

AVM1Value AVM1Value::isEqual(const AVM1Value& other, AVM1Activation& activation) const
{
	auto swfVersion = activation.getSwfVersion();

	auto a = other;
	auto b = *this;

	if (swfVersion < 6)
	{
		// NOTE: In SWF 5, `valueOf()` is always called, even in `Object`
		// to `Object` comparisons. Since `Object.prototype.valueOf()`
		// returns `this`, it'll do a pointer comparison. For `Object`
		// to `Value` comparisons, `valueOf` will be called a second
		// time.
		a = other.toPrimitiveNum(activation);
		b = toPrimitiveNum(activation);
	}

	if (a.type == b.type)
		return a == b;
	if (a.is<NullUndefVal>() || b.is<NullUndefVal>())
		return true;
	// `bool` to `Value` comparison. Convert `bool` 0/1, and compare.
	if (a.is<bool>() || b.is<bool>())
	{
		AVM1Value boolVal(number_t(a.is<bool> ? a._bool : b._bool));
		auto val = a.is<bool>() ? b : a;
		return val.isEqual(boolVal, activation);
	}
	// `Number` to `Value` comparison. Convert `Value` to `Number`, and
	// compare.
	if ((a.is<number_t>() || a.is<AVM1String>()) && (b.is<number_t>() || b.is<AVM1String>()))
	{
		auto numA = (a.is<number_t>() ? a : b).toNumber(activation, true);
		auto numB = (a.is<AVM1String>() ? a : b).toNumber(activation, true);
		return numA == numB;
	}
	// `Object` to `Value` comparison. Call `object.valueOf()`, and
	// compare.
	if (a.is<AVM1Object>() || b.is<AVM1Object>())
	{
		auto objVal = (a.is<AVM1Object>() ? a : b).toPrimitiveNum(activation);
		auto val = a.is<AVM1Object>() ? b : a;
		return objVal.isPrimitive() && val.isEqual(objVal, activation);

	}

	return false;
}
