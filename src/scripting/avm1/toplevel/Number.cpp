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

#include <sstream>

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/runtime.h"
#include "scripting/avm1/value.h"
#include "scripting/avm1/toplevel/Number.h"
#include "tiny_string.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::number` crate.

AVM1Number::AVM1Number
(
	const GcContext& ctx,
	GcPtr<AVM1Object> proto,
	number_t _num
) : AVM1Object(ctx, proto), num(_num) {}

using AVM1Number;
using NumLimits = std::numeric_limits<double>;

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

constexpr auto objFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1Number, toString, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Number, valueOf, protoFlags)
};

static constexpr auto objDecls =
{
	AVM1Decl("MAX_VALUE", NumLimits::max(), objFlags),
	// NOTE: `Number.MIN_VALUE` returns the smallest positive subnormal
	// `double`.
	AVM1Decl("MIN_VALUE", NumLimits::denorm_min(), objFlags),
	AVM1Decl("NaN", NaN, objFlags),
	AVM1Decl("NEGATIVE_INFINITY", -Infinity, objFlags),
	AVM1Decl("POSITIVE_INFINITY", Infinity, objFlags)
};

GcPtr<AVM1SystemClass> AVM1Number::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, number, superProto);
	ctx.definePropsOn(_class->proto, protoDecls);
	ctx.definePropsOn(_class->ctor, objDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Number, ctor)
{
	number_t num;
	AVM1_ARG_UNPACK(num, 0);

	INPLACE_NEW_GC_PTR(act.getGcCtx(), _this, AVM1Number
	(
		act.getGcCtx(),
		_this->getProto(act).toObject(act),
		num
	));

	return _this;
}

AVM1_FUNCTION_BODY(AVM1Number, number)
{
	// When `Number()` is called as a function, it returns the value
	// itself, converted into a double.
	number_t num;
	AVM1_ARG_UNPACK(num, 0);
	return num;
}

AVM1_FUNCTION_TYPE_BODY(AVM1Number, AVM1Number, toString)
{
	// NOTE: The object must be a `Number`. This is because
	// `Number.prototype.toString.call()` returns `undefined` for non
	// `Number` objects.
	number_t _radix;
	AVM1_ARG_UNPACK(_radix, 10);
	size_t radix = _radix >= 2 && _radix <= 36 ? _radix : 10;

	if (radix == 10)
	{
		// Output the number as a floating-point decimal.
		return AVM1Value(_this->num).toString(act);
	}

	// NOTE, PLAYER-SPECIFIC: In Flash Player 7, and later,
	// `NaN.toString()` returns garbage/junk values:
	// for example, `NaN.toString(3)` returns "-/.//./..././/0.0./0.".
	// Flash Player 6, and earlier return a more sane value of "0".
	// TODO: Implement that behaviour, once support for mimicking
	// different player versions is added.

	if (_this->num == 0)
	{
		// Bail out early, if it's 0.
		return "0";
	}

	bool isNeg = _this->num < 0;

	// Max of 32 digits in binary, plus the negative sign.
	char digits[34] = { '\0' };

	size_t i = sizeof(digits)-1;

	for (auto num = std::fabs(_this->num); num; num /= radix)
	{
		uint8_t digit = size_t(number) % radix;
		digit[--i] =
		(
			digit < 10 ?
			'0' + digit :
			'a' + digit - 10
		);
	}

	if (isNeg)
		digits[--i] = '-';
	return tiny_string(&digits[i], true);
}

AVM1_FUNCTION_TYPE_BODY(AVM1Number, AVM1Number, valueOf)
{
	// NOTE: The object must be a `Number`. This is because
	// `Number.prototype.valueOf.call()` returns `undefined` for non
	// `Number` objects.
	return _this->number;
}
