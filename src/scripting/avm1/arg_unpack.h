/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef SCRIPTING_AVM1_ARG_UNPACK_H
#define SCRIPTING_AVM1_ARG_UNPACK_H 1

#include "gc/ptr.h"
#include "scripting/avm1/object/object.h"
#include "scripting/avm1/toplevel/Number.h"
#include "scripting/avm1/value.h"
#include "tiny_string.h"
#include "utils/optional.h"
#include "utils/span.h"

// Inspired by `ARG_UNPACK`.

namespace lightspark
{

class AVM1Activation;

#define IDENTITY(...) __VA_ARGS__

#define TO_VALUE_ARGS IDENTITY \
( \
	AVM1Activation& act, \
	const AVM1Value& value, \
	bool isStrict \
)

#define GET_DEFAULT_VALUE_ARGS IDENTITY \
( \
	AVM1Activation& act \
)

#define TO_VALUE_DECL(type) \
	static constexpr type toValue(TO_VALUE_ARGS)
#define TRY_TO_VALUE_DECL(type) \
	static constexpr Optional<type> tryToValue(TO_VALUE_ARGS)
#define GET_DEFAULT_VALUE_DECL(type) \
	static constexpr type getDefaultValue(GET_DEFAULT_VALUE_ARGS)

#define TO_VALUE_BODY(type) \
	template<> \
	static constexpr type AVM1ArgConverter<type>::toValue(TO_VALUE_ARGS)
#define TRY_TO_VALUE_BODY(type) \
	template<> \
	static constexpr Optional<type> AVM1ArgConverter<type>::tryToValue(TO_VALUE_ARGS)
#define GET_DEFAULT_VALUE_BODY(type) \
	template<> \
	static constexpr type AVM1ArgConverter<type>::getDefaultValue(GET_DEFAULT_VALUE_ARGS)

#define TO_VALUE_BODY_T_COND(cond) \
	template<typename T> \
	static constexpr type AVM1ArgConverter \
	< \
		T, \
		EnableIf<(cond)> \
	>::toValue(TO_VALUE_ARGS)

#define TRY_TO_VALUE_BODY_T_COND(cond) \
	template<typename T> \
	static constexpr Optional<type> AVM1ArgConverter \
	< \
		T, \
		EnableIf<(cond)> \
	>::tryToValue(TO_VALUE_ARGS)

#define GET_DEFAULT_VALUE_T_COND(cond) \
	template<typename T> \
	static constexpr type AVM1ArgConverter \
	< \
		T, \
		EnableIf<(cond)> \
	>::getDefaultValue(GET_DEFAULT_VALUE_ARGS)

template<typename T, typename = void>
class AVM1ArgConverter
{
public:
	TO_VALUE_DECL(T)
	{
		return tryToValue
		(
			act,
			value,
			isStrict
		).valueOr(getDefaultValue());
	}

	TRY_TO_VALUE_DECL(T);
	GET_DEFAULT_VALUE_DECL(T);
};

TRY_TO_VALUE_BODY(AVM1Value)
{
	return value;
}

GET_DEFAULT_VALUE_BODY(AVM1Value)
{
	return AVM1Value::undefinedVal;
}

template<typename T>
class AVM1ArgConverter<_NGC<T>>
{
public:
	TO_VALUE_DECL(_NGC<T>)
	{
		return !value.isNullOrUndefined() &&
		(
			!isStrict ||
			!value.isPrimitive()
		) ? value.toObject(act)->as<T>() : NullGc;
	}

	GET_DEFAULT_VALUE_DECL(_NGC<T>)
	{
		return NullGc;
	}
};

template<>
class AVM1ArgConverter<_NGC<AVM1Object>>
{
public:
	TO_VALUE_DECL(_NGC<AVM1Object>)
	{
		return !value.isNullOrUndefined() &&
		(
			!isStrict ||
			!value.isPrimitive()
		) ? value.toObject(act) : NullGc;
	}

	GET_DEFAULT_VALUE_DECL(_NGC<AVM1Object>)
	{
		return NullGc;
	}
};

TRY_TO_VALUE_BODY(number_t)
{
	if (isStrict && !value.is<number_t>())
		return {};
	return value.toNumber(act);
}

GET_DEFAULT_VALUE_BODY(number_t)
{
	return AVM1Value::undefinedVal.toNumber(act);
}

TRY_TO_VALUE_BODY_T_COND(Conj
<
	std::is_integral<T>,
	std::is_unsigned<T>
>)
{
	if (isStrict && !value.is<number_t>())
		return {};
	return value.toUInt32(act);
}

GET_DEFAULT_VALUE_BODY_T_COND(Conj
<
	std::is_integral<T>,
	std::is_unsigned<T>
>)
{
	return AVM1Value::undefinedVal.toUInt32(act);
}

TRY_TO_VALUE_BODY_T_COND(Conj
<
	std::is_integral<T>,
	std::is_signed<T>
>)
{
	if (isStrict && !value.is<number_t>())
		return {};
	return value.toInt32(act);
}

GET_DEFAULT_VALUE_BODY_T_COND(Conj
<
	std::is_integral<T>,
	std::is_signed<T>
>)
{
	return AVM1Value::undefinedVal.toInt32(act);
}

TRY_TO_VALUE_BODY(uint16_t)
{
	if (isStrict && !value.is<number_t>())
		return {};
	return value.toUInt16(act);
}

GET_DEFAULT_VALUE_BODY(uint16_t)
{
	return AVM1Value::undefinedVal.toUInt16(act);
}

TRY_TO_VALUE_BODY(int16_t)
{
	if (isStrict && !value.is<number_t>())
		return {};
	return value.toInt16(act);
}

GET_DEFAULT_VALUE_BODY(int16_t)
{
	return AVM1Value::undefinedVal.toInt16(act);
}

TRY_TO_VALUE_BODY(uint8_t)
{
	if (isStrict && !value.is<number_t>())
		return {};
	return value.toUInt8(act);
}

GET_DEFAULT_VALUE_BODY(uint8_t)
{
	return AVM1Value::undefinedVal.toUInt8(act);
}

TRY_TO_VALUE_BODY(int8_t)
{
	if (isStrict && !value.is<number_t>())
		return {};
	return value.toInt8(act);
}

GET_DEFAULT_VALUE_BODY(int8_t)
{
	return AVM1Value::undefinedVal.toInt8(act);
}

TRY_TO_VALUE_BODY(bool)
{
	if (isStrict && !value.is<bool>())
		return {};
	return value.toBool(act.getSwfVersion());
}

GET_DEFAULT_VALUE_BODY(bool)
{
	const auto& undefVal = AVM1Value::undefinedVal;
	return undefVal.toBool(act.getSwfVersion());
}

TRY_TO_VALUE_BODY(tiny_string)
{
	if (isStrict && !value.is<tiny_string>())
		return {};
	return value.toString(act);
}

GET_DEFAULT_VALUE_BODY(tiny_string)
{
	return AVM1Value::undefinedVal.toString(act);
}

#undef IDENTITY
#undef TO_VALUE_ARGS
#undef GET_DEFAULT_VALUE_ARGS
#undef TO_VALUE_DECL
#undef TRY_TO_VALUE_DECL
#undef GET_DEFAULT_VALUE_DECL
#undef TO_VALUE_BODY
#undef TRY_TO_VALUE_BODY
#undef GET_DEFAULT_VALUE_BODY
#undef TO_VALUE_BODY_T_COND
#undef TRY_TO_VALUE_BODY_T_COND
#undef GET_DEFAULT_VALUE_BODY_T_COND

#define AVM1_ARG_CHECK_RET(unpacker, ret) \
	if (!unpacker.isValid()) \
		return ret;

#define AVM1_ARG_CHECK(unpacker) \
	AVM1_ARG_CHECK_RET(unpacker, AVM1Value::undefinedVal)

#define AVM1_ARG_UNPACK AVM1ArgUnpacker(act, args)
#define AVM1_ARG_UNPACK_NAMED(unpacker) \
	AVM1ArgUnpacker unpacker(act, args)


class AVM1ArgUnpacker
{
private:
	template<typename T>
	using ArgConv = AVM1ArgConverter<T>;

	AVM1Activation& act;
	Span<const AVM1Value> args;
	size_t pos;
	bool valid;
	bool isStrict;
public:
	AVM1ArgUnpacker
	(
		AVM1Activation& _act,
		const Span<const AVM1Value>& _args
	) :
	act(_act),
	args(_args),
	pos(0),
	valid(true),
	isStrict(false) {}

	bool isValid() const { return valid; }

	template<typename T = AVM1Value>
	T unpack()
	{
		return unpack<T>(ArgConv<T>::getDefaultValue(act));
	}

	template<typename T = AVM1Value>
	T unpack(const T& defaultVal)
	{
		if (!valid || pos >= args.size())
		{
			valid = false;
			return defaultVal;
		}

		return ArgConv<T>::toValue(act, args[pos++], isStrict);
	}

	template<typename T = AVM1Value>
	Optional<T> tryUnpack()
	{
		if (!valid || pos >= args.size())
			return {};
		return ArgConv<T>::tryToValue(act, args[pos++], isStrict);
	}

	template<typename T = AVM1Value>
	AVM1ArgUnpacker& operator()(T& value)
	{
		return *this(value, ArgConv<T>::getDefaultValue(act));
	}

	template<typename T = AVM1Value>
	AVM1ArgUnpacker& operator()(T& value, const T& defaultVal)
	{
		value = unpack<T>(defaultVal);
		return *this;
	}
};

}
#endif /* SCRIPTING_AVM1_ARG_UNPACK_H */
