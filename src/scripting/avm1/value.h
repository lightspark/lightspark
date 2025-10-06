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

#ifndef SCRIPTING_AVM1_VALUE_H
#define SCRIPTING_AVM1_VALUE_H 1

#include "forwards/scripting/flash/display/DisplayObject.h"
#include "forwards/swftypes.h"
#include "smartrefs.h"

// Based on Ruffle's `avm1::Value`.

namespace lightspark
{

class tiny_string;
class AVM1Activation;
class AVM1Function;
class AVM1MovieClip;
class AVM1MovieClipRef;
class AVM1Null;
class AVM1Object;
class AVM1String;
class AVM1Undefined;
template<typename T>
class Optional;

// Used by `visit()`.
struct UndefinedVal {};
struct NullVal {};

#define USE_STRING_ID
// TODO: Make this more like `asAtom`, but without `asAtomHandler`.
class AVM1Value
{
public:
	enum class Type
	{
		Undefined,
		Null,
		Bool,
		Number,
		String,
		Object,
		MovieClip,
	};
private:
	Type type;
	union
	{
		bool _bool;
		number_t num;
		#ifdef USE_STRING_ID
		uint32_t str;
		#else
		tiny_string str;
		#endif
		_R<AVM1Object> obj;
		_R<AVM1MovieClipRef> clip;
	};
public:
	AVM1Value(const Type& _type) : type(_type)
	{
		assert(type == Type::Undefined || type == Type::Null);
	}

	AVM1Value(bool boolVal) : type(Type::Bool), _bool(boolVal) {}
	AVM1Value(number_t _num) : type(Type::Number), num(_num) {}
	#ifdef USE_STRING_ID
	AVM1Value(uint32_t strID) : type(Type::String), str(strID) {}
	AVM1Value(const tiny_string& _str);
	#else
	AVM1Value(const tiny_string& _str) : type(Type::String), str(_str) {}
	#endif
	AVM1Value(const _R<AVM1Object>& _obj) : type(Type::Object), obj(_obj) {}
	AVM1Value(const _R<AVM1MovieClipRef>& _clip) :
	type(Type::MovieClip),
	clip(_clip) {}

	~AVM1Value();
	AVM1Value& operator=(const AVM1Value& other);

	bool operator==(const AVM1Value& other) const;

	// Returns `true`, if the given value is a primitive value.
	//
	// NOTE: Boxed primitives aren't considered primitives; it's expected
	// that their `{toString,valueOf}()` handlers already had a chance
	// to unbox the contained primitive.
	bool isPrimitive() const
	{
		return !is<AVM1Object>() && !is<MovieClip>();
	}

	// ECMA-262 2nd edition sec. 9.3. `ToNumber`.
	//
	// Flash Player diverges from the spec in sevaral ways. Which are
	// (as far as us, and other projects are aware) version gated:
	//
	// - In SWF 6, and earlier, `undefined` is converted to `0.0` (much
	// like `false`), instead of `NaN`, as required by the spec.
	// - In SWF 5, and earlier, hexadecimal isn't supported.
	// - In SWF 4, and earlier, `0.0` is returned, instead of `NaN`, if
	// a string can't be coverted to a number.
	number_t toNumber(AVM1Activation& activation, bool primitiveHint = false) const;

	// ECMA-262 2nd edition sec. 9.1. `ToPrimitive` (with `Number` hint).
	//
	// Flash Player diverges from the spec in sevaral ways. Which are
	// (as far as us, and other projects are aware) version gated:
	//
	// - `toString()` is never called when `ToPrimitive` is invoked with
	// a `Number` hint.
	// - `Object`s with uncallable `valueOf()` implementations are
	// converted to `undefined`. This isn't a special case: All values
	// are callable in AVM1.
	// - `Value`s that aren't callable `Object`s return `undefined`,
	// instead of throwing a runtime error.
	AVM1Value toPrimitiveNum(AVM1Activation& activation) const;

	// Attempts to convert a value to a primitive type.
	// All types are converted to a `Number`, except for `Date`, which
	// gets converted to a `String`.
	// If `{valueOf,toString}()` don't return a primitive, then the
	// original object is returned.
	// This is used by `ActionAdd2`, when concatenating `String`s, e.g.
	// `"a" + {}`.
	//
	// This is loosely based on the hintless version of ECMA-262 2nd
	// edition sec. 9.1. `ToPrimitive`.
	// Differences from ECMA-262:
	// - This is only used by `ActionAdd2` in AVM1, not `ActionEquals2`.
	// - `{valueOf,toString}()` are called, even if they're not functions,
	// returning `undefined`.
	// - `Date` objects won't fallback to `valueOf()`, if `toString()`
	// doesn't return primitive.
	// - `Date` objects are converted to `Number` in SWF 5.
	AVM1Value toPrimitive(AVM1Activation& activation) const;

	// ECMA-262 2nd edition sec. 11.8.5. The abstract relational
	// comparison algorithm.
	AVM1Value isLess(const AVM1Value& other, AVM1Activation& activation) const;

	// ECMA-262 2nd edition sec. 11.9.3. The abstract equality comparison
	// algorithm.
	AVM1Value isEqual(const AVM1Value& other, AVM1Activation& activation) const;

	uint8_t toUInt8(AVM1Activation& activation) const;

	// Convert a `Number` to a `uint16_t`, based on the ECMA-262 spec
	// for `ToUInt16`.
	// The value is truncated mod 2^16.
	// This calls `valueOf()`, and will do any necessary conversions.
	uint16_t toUInt16(AVM1Activation& activation) const;

	// Convert a `Number` to a `uint32_t`, based on the ECMA-262 spec
	// for `ToUInt32`.
	// The value is truncated mod 2^32.
	// This calls `valueOf()`, and will do any necessary conversions.
	uint32_t toUInt32(AVM1Activation& activation) const;

	// Convert a `Number` to an `int16_t`, based on the ECMA-262 spec
	// for `ToInt16`.
	// The value is truncated in the range -2^15 - 2^15.
	// This calls `valueOf()`, and will do any necessary conversions.
	int16_t toInt16(AVM1Activation& activation) const;

	// Convert a `Number` to an `int32_t`, based on the ECMA-262 spec
	// for `ToInt32`.
	// The value is truncated in the range -2^31 - 2^31.
	// This calls `valueOf()`, and will do any necessary conversions.
	int32_t toInt32(AVM1Activation& activation) const;

	// Convert a value to a string.
	tiny_string toString(AVM1Activation& activation) const;

	#ifdef USE_STRING_ID
	// Convert a value to a string ID.
	uint32_t toStringID(AVM1Activation& activation) const;
	#endif

	bool toBool(uint8_t swfVersion) const;
	tiny_string typeOf() const;
	#ifdef USE_STRING_ID
	uint32_t typeOfId() const;
	#endif
	_R<AVM1Object> toObject(AVM1Activation& activation) const;
	Optional<AS_BLENDMODE> toBlendMode() const;

	template<typename T>
	bool is() const
	{
		return visit(makeVisitor
		(
			[](const T&) { return true; },
			[](const auto&) { return false; }
		));
	}

	template<>
	bool is<tiny_string>() const { return type == Type::String; }
	template<>
	bool is<AVM1MovieClip>() const { return type == Type::MovieClip; }
	template<>
	bool is<AVM1Null>() const { return type == Type::Null; }
	template<>
	bool is<AVM1Object>() const { return type == Type::Object; }
	template<>
	bool is<AVM1String>() const { return type == Type::String; }
	template<>
	bool is<AVM1Undefined>() const { return type == Type::Undefined; }

	template<typename V>
	constexpr auto visit(V&& visitor)
	{
		switch (type)
		{
			case Type::Undefined: return visitor(UndefinedVal {}); break;
			case Type::Null: return visitor(NullVal {}); break;
			case Type::Bool: return visitor(_bool); break;
			case Type::Number: return visitor(num); break;
			case Type::String: return visitor(str); break;
			case Type::Object: return visitor(obj); break;
			case Type::MovieClip: return visitor(clip); break;
			default: assert(false); break;
		}
		return decltype(visitor(UndefinedVal {}))();
	}
};

// Converts a `tiny_string` to a `number_t`
//
// This function might fail on some invalid inputs, in which case, `NaN`
// is returned.
//
// `isStrict` usually indicates whether to act like `Number()`, or
// `parseFloat()`.
// - `isStrict == true` fails on trailing garbage (like `Number()`).
// - `isStrict == false` ignores trailing garbage (like `parseFloat()`).
number_t parseFloatImpl(const tiny_string& str, bool isStrict);

}
#endif /* SCRIPTING_AVM1_VALUE_H */
