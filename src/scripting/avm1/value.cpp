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

tiny_string numToStr(number_t num);
number_t strToNum(const tiny_string& str, uint8_t swfVersion);
#ifdef USE_STRING_ID
uint32_t numToStrID(number_t num);
number_t strIDToNum(uint32_t strId, uint8_t swfVersion);

AVM1Value::AVM1Value(const tiny_string& _str) :
type(Type::String),
str(getSys()->getUniqueStringId(_str, true)) {}
#endif

AVM1Value::~AVM1Value()
{
	switch (type)
	{
		case Type::Object: obj.~_R<AVM1Object>(); break;
		case Type::MovieClip: clip.~_R<AVM1MovieClipRef>(); break;
		#ifndef USE_STRING_ID
		case Type::String: str.~tiny_string(); break;
		#endif
		default: break;
	}
}

AVM1Value& AVM1Value::operator=(const AVM1Value& other)
{
	type = other.type;
	switch (type)
	{
		case Type::Bool: _bool = other._bool; break;
		case Type::Number: num = other.num; break;
		case Type::Object: new(&obj) _R<AVM1Object>(other.obj); break;
		case Type::MovieClip: new(&clip) _R<AVM1MovieClipRef>(other.clip); break;
		#ifdef USE_STRING_ID
		case Type::String: str = other.str; break;
		#else
		case Type::String: new(&str) tiny_string(other.str); break;
		#endif
		default: break;
	}
	return *this;
}

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
		[&](uint32_t str) { return strIDToNum(str, swfVersion); },
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

#define USE_REM_EUCLID
#ifdef USE_REM_EUCLID
template<typename T, EnableIf<std::is_floating_point<T>::value, bool> = false>
constexpr T remEuclid(const T& a, const T& b)
{
	auto ret = std::fmod(a, b);
	return ret < T(0) ? ret + std::fabs(b) : ret;
}

template<typename T, EnableIf<Conj
<
	std::is_integral<T>,
	std::is_signed<T>
>, bool> = false>
constexpr T remEuclid(const T& a, const T& b)
{
	auto ret = a % b;
	return ret < T(0) ? ret + abs(b) : ret;
}

template<typename T, EnableIf<Conj
<
	std::is_integral<T>,
	Neg<std::is_signed<T>>
>, bool> = false>
constexpr T remEuclid(const T& a, const T& b)
{
	return a % b;
}

template<typename T, EnableIf<std::is_integral<T>::value, bool> = false>
T toInt(const AVM1Value& val, AVM1Activation& activation)
{
	constexpr auto maxVal = std::numeric_limits<std::make_unsigned_t<T>>::max;
	auto ret = val.toNumber(activation);
	if (!std::isfinite(ret))
		return 0;
	return remEuclid(std::trunc(ret), number_t(maxVal + 1));
}
#else
template<typename T, EnableIf<Conj
<
	std::is_integral<T>,
	std::is_signed<T>
>, bool> = false>
T toInt(const AVM1Value& val, AVM1Activation& activation)
{
	constexpr auto maxUVal = std::numeric_limits<std::make_unsigned_t<T>>::max;
	constexpr auto maxVal = std::numeric_limits<T>::max;
	constexpr auto minVal = std::numeric_limits<T>::min;

	auto intVal = val.toNumber(activation);

	if (!std::isfinite(intVal) || intVal == 0.0)
		return 0;

	auto posInt = std::floor(std::fabs(intVal));
	if (posInt > maxUVal)
		posInt = std::fmod(posInt, number_t(maxUVal + 1));

	if (posInt < number_t(maxVal + 1))
		return ret < 0.0 ? -posInt : posInt;

	T ret(posInt - number_t(maxVal + 1));
	return intVal < 0.0 ? minVal - ret : minVal + ret;
}

template<typename T, EnableIf<Conj
<
	std::is_integral<T>,
	Neg<std::is_signed<T>>
>, bool> = false>
T toInt(const AVM1Value& val, AVM1Activation& activation)
{
	constexpr auto maxVal = std::numeric_limits<T>::max;
	auto intVal = val.toNumber(activation);

	if (!std::isfinite(intVal) || intVal == 0.0)
		return 0;

	auto posInt = std::floor(std::fabs(intVal));
	if (posInt > maxVal)
		posInt = std::fmod(posInt, number_t(maxVal + 1));
	return posInt;
}
#endif

uint8_t AVM1Value::toUInt8(AVM1Activation& activation) const
{
	return toInt<uint8_t>(*this, activation);
}

uint16_t AVM1Value::toUInt16(AVM1Activation& activation) const
{
	return toInt<uint16_t>(*this, activation);
}

uint32_t AVM1Value::toUInt32(AVM1Activation& activation) const
{
	return toInt<uint32_t>(*this, activation);
}

int16_t AVM1Value::toInt16(AVM1Activation& activation) const
{
	return toInt<int16_t>(*this, activation);
}

int32_t AVM1Value::toInt32(AVM1Activation& activation) const
{
	return toInt<int32_t>(*this, activation);
}

tiny_string AVM1Value::toString(AVM1Activation& activation) const
{
	auto swfVersion = activation.getSwfVersion();
	return visit(makeVisitor
	(
		[&](const UndefinedVal&) { return swfVersion > 6 ? "" : "undefined"; },
		[&](const NullVal&) { return "null"; },
		[&](bool boolVal)
		{
			if (swfVersion > 4)
				return boolVal ? "true" : "false";
			// NOTE: In SWF 4, bool to string conversions return 1, or 0,
			// rather than `true`, or `false`.
			return boolVal ? "1" : "0";
		},
		[&](number_t num) { return numToStr(num); },
		#ifdef USE_STRING_ID
		[&](uint32_t str)
		{
			auto sys = activation.getSys();
			return sys->getStringFromUniqueId(str);
		},
		#else
		[&](const tiny_string& str) { return str; },
		#endif
		[&](const _R<AVM1Object>& obj)
		{
			auto dispObj = obj->as<DisplayObject>();
			if (dispObj != nullptr)
			{
				// Special case for `DisplayObject`s.
				return dispObj->AVM1GetPath(true);
			}

			auto strVal = obj->callMethod
			(
				#ifdef USE_STRING_ID
				STRING_TOSTRING,
				#else
				"toString",
				#endif
				{},
				activation,
				AVM1ExecutionReason::Special
			);

			if (strVal.is<AVM1String>())
			{
				#ifdef USE_STRING_ID
				auto sys = activation.getSys();
				return sys->getStringFromUniqueId(strVal.str);
				#else
				return strVal.str;
				#endif
			}

			if (obj->is<AVM1Function>())
				return "[type Function]";
			return "[type Object]";
		},
		[&](const _R<AVM1MovieClipRef>& clip)
		{
			return clip->toString(activation);
		}
	));
}

#ifdef USE_STRING_ID
uint32_t AVM1Value::toStringID(AVM1Activation& activation) const
{
	if (is<AVM1String>())
		return str;

	auto sys = activation.getSys();
	return sys->getUniqueStringId(toString(activation), true);
}
#endif

bool AVM1Value::toBool(uint8_t swfVersion) const
{
	return visit(makeVisitor
	(
		[&](const NullUndefVal&) { return false; },
		[&](bool boolVal) { return boolVal; },
		[&](number_t num) { return !std::isnan(num) && num != 0.0; },
		#ifdef USE_STRING_ID
		[&](uint32_t str)
		{
			if (swfVersion > 6)
				return str != BUILTIN_STRINGS::EMPTY;
		#else
		[&](const tiny_string& str)
		{
			if (swfVersion > 6)
				return !str.empty();
		#endif
			auto num = strToNum(str, swfVersion);
			return !std::isnan(num) && num != 0.0;
		},
		[&](const _R<AVM1Object>& obj) { return true; },
		[&](const _R<AVM1MovieClipRef>& clip) { return true; }
	));
}

tiny_string AVM1Value::typeOf() const
{
	return visit(makeVisitor
	(
		[&](const UndefinedVal&) { return "undefined"; },
		[&](const NullVal&) { return "null"; },
		[&](bool) { return "boolean"; },
		[&](number_t) { return "number"; },
		#ifdef USE_STRING_ID
		[&](uint32_t) { return "string"; },
		#else
		[&](const tiny_string&) { return "string"; },
		#endif
		[&](const _R<AVM1Object>& obj)
		{
			return obj->is<AVM1Function>() ? "function" : "object";
		},
		[&](const _R<AVM1MovieClipRef>& clip) { return "movieclip"; }
	));
}

_R<AVM1Object> AVM1Value::toObject(AVM1Activation& activation) const
{
	if (!is<AVM1MovieClip>())
		return AVM1ValueObject::boxValue(activation, *this);
	auto obj = clip->toObject(activation);

	if (!obj.isNull())
		return obj;
	return AVM1ValueObject::boxValue(activation, Type::Undefined);
}

Optional<AS_BLENDMODE> AVM1Value::toBlendMode() const
{
	return visit(makeVisitor
	(
		[&](const NullUndefVal&) { return BLENDMODE_NORMAL; },
		[&](number_t num) { return AS_BLENDMODE(num); },
		// NOTE: Strings such as `"5"` *aren't* converted.
		#ifdef USE_STRING_ID
		[&](uint32_t str)
		{
			auto sys = activation.getSys();
			if (sys->getStringFromUniqueId(str) == "normal")
				return BLENDMODE_NORMAL;
			auto _str = str;
		#else
		[&](const tiny_string& str)
		{
			if (str == "normal")
				return BLENDMODE_NORMAL;
			auto sys = activation.getSys();
			auto _str = sys->getUniqueStringId(str);
		#endif
			switch (_str)
			{
				case STRING_ADD: return BLENDMODE_ADD; break;
				case STRING_ALPHA: return BLENDMODE_ALPHA; break;
				case STRING_DARKEN: return BLENDMODE_DARKEN; break;
				case STRING_DIFFERENCE: return BLENDMODE_DIFFERENCE; break;
				case STRING_ERASE: return BLENDMODE_ERASE; break;
				case STRING_HARDLIGHT: return BLENDMODE_HARDLIGHT; break;
				case STRING_INVERT: return BLENDMODE_INVERT; break;
				case STRING_LAYER: return BLENDMODE_LAYER; break;
				case STRING_LIGHTEN: return BLENDMODE_LIGHTEN; break;
				case STRING_MULTIPLY: return BLENDMODE_MULTIPLY; break;
				case STRING_OVERLAY: return BLENDMODE_OVERLAY; break;
				case STRING_SCREEN: return BLENDMODE_SCREEN; break;
				case STRING_SUBTRACT: return BLENDMODE_SUBTRACT; break;
				default: return {}; break;
			}
		},
		// All other types aren't converted either.
		[&](const auto&) { return {}; }
	));
}

template<typename T, typename U, EnableIf<sizeof(T) == sizeof(U), bool> = false>
T bitCast(const U& val)
{
	#if defined(__has_builtin) && __has_builtin(__builtin_bit_cast)
	return __builtin_bit_cast(T, val);
	#else
	T ret;
	std::memcpy(&ret, &val, std::min(sizeof(T)));
	return ret;
	#endif
}

tiny_string numToStr(number_t num)
{
	if (std::isnan(num))
		return "NaN";
	else if (std::isinf(num))
		return std::signbit(num) ? "-Infinity" : "Infinity";
	else if (num == 0.0)
		return "0";
	else if (num >= INT32_MIN && num <= INT32_MAX && (num - std::floor(num)) == 0.0)
	{
		// Fast path for ints.
		std::stringstream s;
		return (s << int(num)).str();
	}

	// AVM1: `Number` -> `String` (also tries to reproduce bugs).
	// AVM1 (Flash Player's) does this in a straightforward way, by
	// shifting the float into the range 0.0-10.0, multiplying by 10 to
	// extract each digit, then finally rounding the result. However, the
	// rounding step is buggy, when carrying 9.999 -> 10.
	// For example, `-9999999999999999.0` returns `"-e+16"`.
	std::string str(25, '\0');

	bool isNeg = num < 0.0;
	if (isNeg)
	{
		num = -num;
		str.push_back('-');
	}

	// Extract the binary exponent from a `double` (11 bits, with a bias
	// of 1023).
	constexpr uint64_t mantBits = 52;
	constexpr uint64_t expMask = 0x7ff;
	constexpr uint64_t expBias = 1023;

	auto doubleBits = bitCast<uint64_t>(num);
	auto binExp = int32_t((doubleBits >> mantBits) & expMask) - expBias;

	if (binExp == -expBias)
	{
		// Subnormal; scale back to the normal range, and try getting the
		// exponent again.

		// This value represents 2^54.
		constexpr number_t normalScale = 1.801439850948198e16;
		auto _num = num * normalScale;
		doubleBits = bitCast<uint64_t>(_num);
		binExp = int32_t((doubleBits >> mantBits) & expMask) - expBias;
	}

	// Convert exponent to decimal.

	// This value represents `log10(2)`.
	constexpr number_t log10_2 = 0.301029995663981;
	int32_t exp = std::round(number_t(binExp) * log10_2);

	// Shift the decimal, such that it's in the range 0.0-10.0.
	auto mant = decShift(num, -exp);

	// The exponent might be off by 1; try the next exponent if that's
	// the case.
	if (!int32_t(mant))
		mant = decShift(num, -(--exp));
	if (int32_t(mant) >= 10)
		mant = decShift(num, -(++exp));

	// This lambda generates the next digit character.
	auto getDigit = [&] -> char
	{
		int32_t digit = mant;
		assert(digit >= 0 && digit < 10);
		mant -= digit;
		mant *= 10;
		return '0' + digit;
	};

	constexpr auto maxFracDigits = 15;

	if (exp >= maxFracDigits)
	{
		// `1.2345e+15`.
		// NOTE: This case fails to push an extra `0` to handle rounding
		// from 9.9999 to 10, which eventually leads to the
		// `-9999999999999999.0` == `"-e+16"` bug.
		str.append({ getDigit(), '.' });
		for (size_t i = 0; i < maxFracDigits - 1; ++i)
			str.push_back(getDigit());
		exp = 0;
	}
	else if (exp >= 0 && exp <= 14)
	{
		// `12345.678901234`.
		str.push_back('0');
		for (size_t i = 0; i <= exp; ++i)
			str.push_back(getDigit());

		str.push_back('.');
		for (size_t i = 0; i < maxFracDigits - exp - 1; ++i)
			str.push_back(getDigit());
		exp = 0;
	}
	else if (exp >= -5 && exp <= -1)
	{
		// `0.0012345678901234`.
		str += "00.";
		str.resize(str.size() - exp - 1, '0');
		for (size_t i = 0; i < maxFracDigits; ++i)
			str.push_back(getDigit());
		exp = 0;
	}
	else
	{
		// `1.345e-15`.
		str.push_back('0');
		auto digit = getDigit();
		if (digit != '\0')
			str.push_back(digit);

		str.push_back('.');
		for (size_t i = 0; i < maxFracDigits - 1; ++i)
			str.push_back(getDigit());
	}

	// Rounding: Look at the next generated digit, and round accordingly.
	// Ties round away from 0.
	if (getDigit() >= '5')
	{
		// Add 1 to the right most digit, carrying if a 9 is encountered.
		for (auto it = str.rbegin(); it != str.rend(); ++it)
		{
			if (*it == '9')
				*it = '0';
			else if (*it >= '0')
			{
				(*it)++;
				break;
			}
		}
	}

	// Remove any trailing 0s, and the radix point.
	for (; !str.empty() && str.back() == '0'; str.pop_back());
	if (!str.empty() && str.back() == '.')
		str.pop_back();

	size_t start = 0;
	if (exp != 0)
	{
		// Write exponent `e+###`.

		// NOTE: Flash Player does a lot of hacks here, in an attempt to clean
		// up the above rounding.
		// Negative values aren't correctly handled in Flash Player,
		// causing several bugs.

		// NOTE, PLAYER-SPECIFIC: The Ruffle devs suspect that these
		// checks were added in Flash Player 6.

		// Remove any leading 0s.
		auto pos = str.find('0');

		if (pos != 0)
			str = str.substr(pos);

		if (str.empty())
		{
			// Fix up `9.99999` being rounded to `0.00000` when there's
			// no space for the carried 1.
			// If there's no more digits, the value was all 0s that got
			// trimmed, round it to 1.
			str.push_back('1');
			exp++;
		}
		else
		{
			// Fix up 100e15-1e17.
			auto pos = str.rfind('0');
			if (!pos)
			{
				exp += str.size() - 1;
				str.resize(1);
			}
		}
		std::stringstream s;
		str += (s << 'e' << std::showpos << exp).str();
	}

	// One last hack to remove any leading 0s.
	if (!str.empty() && str[isNeg] == '0' && str[isNeg + 1] != '.')
	{
		if (isNeg)
			str[isNeg] = str[isNeg - 1];
		start = 1;
	}

	return str.substr(start);
}

number_t strToNum(const tiny_string& str, uint8_t swfVersion)
{
}

#ifdef USE_STRING_ID
uint32_t numToStrID(number_t num)
{
	return getSys()->getUniqueStringId(numToStr(num));
}

number_t strIDToNum(uint32_t strId, uint8_t swfVersion)
{
	return strToNum(getSys()->getStringFromUniqueId(strId), swfVersion);
}
#endif
