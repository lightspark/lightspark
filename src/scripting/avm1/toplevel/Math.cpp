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

#include <cmath>
#include <initializer_list>

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/Math.h"
#include "scripting/avm1/toplevel/Number.h"
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::math` crate.

using AVM1Math;

constexpr auto objFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

static constexpr auto objDecls =
{
	AVM1Decl("SQRT2", M_SQRT2, objFlags),
	AVM1Decl("SQRT1_2", M_SQRT1_2, objFlags),
	AVM1Decl("PI", M_PI, objFlags),
	AVM1Decl("LOG2E", M_LOG2E, objFlags),
	AVM1Decl("LOG10E", M_LOG10E, objFlags),
	AVM1Decl("LN2", M_LN2, objFlags),
	AVM1Decl("LN10", M_LN10, objFlags),
	AVM1Decl("E", M_E, objFlags),
	AVM1_FUNCTION_PROTO(abs, objFlags),
	AVM1_FUNCTION_PROTO(min, objFlags),
	AVM1_FUNCTION_PROTO(max, objFlags),
	AVM1_FUNCTION_PROTO(sin, objFlags),
	AVM1_FUNCTION_PROTO(cos, objFlags),
	AVM1_FUNCTION_PROTO(atan2, objFlags),
	AVM1_FUNCTION_PROTO(tan, objFlags),
	AVM1_FUNCTION_PROTO(exp, objFlags),
	AVM1_FUNCTION_PROTO(log, objFlags),
	AVM1_FUNCTION_PROTO(sqrt, objFlags),
	AVM1_FUNCTION_PROTO(round, objFlags),
	AVM1_FUNCTION_PROTO(random, objFlags),
	AVM1_FUNCTION_PROTO(floor, objFlags),
	AVM1_FUNCTION_PROTO(ceil, objFlags),
	AVM1_FUNCTION_PROTO(atan, objFlags),
	AVM1_FUNCTION_PROTO(asin, objFlags),
	AVM1_FUNCTION_PROTO(acos, objFlags),
	AVM1_FUNCTION_PROTO(pow, objFlags)
};

AVM1Math::AVM1Math(AVM1DeclContext& ctx) : AVM1Object
(
	ctx.getGcCtx(),
	ctx.getObjectProto()
)
{
	ctx.definePropsOn(this, objDecls);
}

using AVM1Number;

#define MATH_STD_FUNC(name) AVM1_FUNCTION_BODY(AVM1Math, name) \
{ \
	number_t val; \
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(val), NaN); \
	return std::name(val); \
}

MATH_STD_FUNC(abs);

AVM1_FUNCTION_BODY(AVM1Math, min)
{
	if (args.empty())
		return Infinity;
	number_t a;
	number_t b;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(a)(b), NaN);

	if (std::isnan(a) || std::isnan(b))
		return NaN;
	return std::min(a, b);
}

AVM1_FUNCTION_BODY(AVM1Math, max)
{
	if (args.empty())
		return -Infinity;
	number_t a;
	number_t b;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(a)(b), NaN);

	if (std::isnan(a) || std::isnan(b))
		return NaN;
	return std::max(a, b);
}

MATH_STD_FUNC(sin);
MATH_STD_FUNC(cos);

AVM1_FUNCTION_BODY(AVM1Math, atan2)
{
	number_t y;
	number_t x;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(y)(x, 0), NaN);
	return std::atan2(y, x);
}

MATH_STD_FUNC(tan);
MATH_STD_FUNC(exp);
MATH_STD_FUNC(log);
MATH_STD_FUNC(sqrt);

AVM1_FUNCTION_BODY(AVM1Math, round)
{
	number_t x;
	AVM1_ARG_CHECK_RET(AVM1_ARG_UNPACK(x), NaN);

	// NOTE: `Math.round()` always rounds towards infinity, while
	// `std::round()` rounds away from 0.
	return std::floor(x + 0.5);
}

AVM1_FUNCTION_BODY(AVM1Math, random)
{
	// See `https://github.com/adobe/avmplus/blob/858d034/core/MathUtils.cpp#L1731`.
	// NOTE: This generates a restricted set of `Number`s, which some
	// SWFs rely on implicitly.
	constexpr uint32_t maxVal = 0x7fffffff;
	number_t rng = act.getSys()->randomFlashCompatible();
	return rng / number_t(maxVal + 1);
}

MATH_STD_FUNC(floor);
MATH_STD_FUNC(ceil);
MATH_STD_FUNC(atan);
MATH_STD_FUNC(asin);
MATH_STD_FUNC(acos);

AVM1_FUNCTION_BODY(AVM1Math, pow)
{
	if (args.empty())
		return NaN;

	number_t x;
	number_t y;
	AVM1_ARG_UNPACK(x)(y);
	return std::pow(x, y);
}
