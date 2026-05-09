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

#ifndef SCRIPTING_AVM1_CLAMP_H
#define SCRIPTING_AVM1_CLAMP_H 1

#include <cmath>
#include <limits>

#include "utils/type_traits.h"

// Based on Ruffle's `avm1::clamp::Clamp` trait.

namespace lightspark
{

// Clamp/Restrict a value to a certain range (including `NaN`).
//
// Normally, with `dclamp(a, b, c)`, if `a` is `NaN`, it propagates the
// `NaN`, rather than returning either `b`, or `c`.
// This implementation always returns the smallest value, from the provided
// values (i.e. returns `b`, if `a` is `NaN`).
template<typename T, EnableIf<std::is_floating_point<T>::value, bool> = false>
inline T clampNaN(const T& a, const T& b, const T& c)
{
	return std::fmin(std::fmax(a, b), c);
}

// Like `clampNaN()`, but returns the largest value, from the provided
// values (i.e. returns `c`, if `a` is `NaN`).
template<typename T, EnableIf<std::is_floating_point<T>::value, bool> = false>
inline T clampMaxNaN(const T& a, const T& b, const T& c)
{
	return std::fmax(b, std::fmin(c, a));
}

// Clamp/Restrict a value to a certain (signed) integer range.
template<typename T, typename U, EnableIf<Conj
<
	std::is_integral<T>,
	std::is_signed<T>,
	std::is_floating_point<U>
>, bool> = false>
inline T clampToInt(const U& val)
{
	using IntLimits = std::numeric_limits<T>;
	// Clamp `NaN`, and out of range values (including `Infinity`) to
	// `IntLimits::min`.
	return
	(
		val >= IntLimits::min &&
		val <= IntLimits::max
	) ? T(val) : IntLimits::min;
}

// Clamp/Restrict a value to a certain (unsigned) integer range.
template<typename T, typename U, EnableIf<Conj
<
	std::is_integral<T>,
	std::is_unsigned<T>,
	std::is_floating_point<U>
>, bool> = false>
inline T clampToUInt(const U& val)
{
	using UIntLimits = std::numeric_limits<T>;
	// Clamp `NaN`, and out of range values (including `Infinity`) to
	// `UIntLimits::min`.
	return
	(
		val >= UIntLimits::min &&
		val <= UIntLimits::max
	) ? T(val) : UIntLimits::min;
}

}
#endif /* SCRIPTING_AVM1_CLAMP_H */
