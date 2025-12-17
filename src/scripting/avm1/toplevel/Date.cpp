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

#include "backends/date.h"
#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/clamp.h"
#include "scripting/avm1/prop.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/Date.h"
#include "swf.h"
#include "tiny_string.h"
#include "utils/timespec.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::date` crate.

static constexpr auto NaN = std::numeric_limits<number_t>::quiet_NaN();

AVM1Date::AVM1Date(AVM1Activation& act) : AVM1Object
(
	act,
	act.getPrototypes().date->proto
)
{
}

AVM1DateImpl AVM1DateImpl::now(AVM1Activation& act)
{
	return AVM1DateImpl(act.getSys()->date->now());
}

constexpr uint16_t AVM1DateImpl::getMonthOffset
(
	ssize_t i,
	bool _inLeapYear
)
{
	if (i < 0)
		return 0;
	bool addLeapDay = _inLeapYear && i >= 1;
	return monthOffsets[std::min(i, 11)] + addLeapDay;
}

TimeSpec AVM1DateImpl::getTimeWithinDay(uint8_t swfVersion) const
{
	auto _time = time.toFloatMs();

	return TimeSpec::fromFloatMs
	(
		swfVersion > 7 ?
		remEuclid(_time, msPerDay) :
		std::fmod(_time, msPerDay)
	);
}

number_t AVM1DateImpl::dayFromYear(number_t year)
{
	return
	(
		(365 * (year - 1970)) +
		std::floor((year - 1969) / 4) -
		std::floor((year - 1901) / 100) +
		std::floor((year - 1601) / 400)
	);
}

size_t AVM1DateImpl::getYear() const
{
	auto day = getDay();

	bool isBeforeEpoch = time.toSFloat() < 0;
	// Use binary search to find the biggest `year`, such that
	// `timeFromYear(year) <= time`.
	auto low = clampToInt<int32_t>(day / (365 + !isBeforeEpoch)) + 1970;
	auto high = clampToInt<int32_t>(day / (365 + isBeforeEpoch)) + 1970;

	while (low < high)
	{
		auto pivot = clampToInt<int32_t>
		(
			number_t(low) +
			number_t(high) / 2
		);

		if (timeFromYear(pivot) > *this)
		{
			high = pivot - 1;
			continue;
		}

		assert(timeFromYear(pivot) <= *this);
		if (timeFromYear(pivot + 1) > *this)
			return pivot;
		low = pivot + 1;
	}

	return low;
}

size_t AVM1DateImpl::getMonth() const
{
	auto day = getDayWithinYear();
	bool _inLeapYear = inLeapYear();

	size_t i;
	for (i = 0; i < 11 && day < getMonthOffset(i + 1, _inLeapYear); ++i);
	return i;
}

size_t AVM1DateImpl::getDate() const
{
	return dayWithinYear() - getMonthOffset(getMonth()) + 1;
}

size_t AVM1DateImpl::getWeekDay() const
{
	return remEuclidInt(getDay() + 4, 7);
}

TimeSpec AVM1DateImpl::getLocalTZA(AVM1Activation& act, bool isUTC) const
{
	return TimeSpec::fromSec(act.getSys()->date->getLocalTZA(isUTC));
}

TimeSpec AVM1DateImpl::getDSTAdjustment(AVM1Activation& act) const
{
	auto date = act.getSys()->date;
	return TimeSpec::fromSec(date->getDSTAdjustment(time));
}

AVM1DateImpl AVM1DateImpl::getLocalTime(AVM1Activation& act) const
{
	return AVM1DateImpl
	(
		time +
		getLocalTZA(act, true) +
		getDSTAdjust(act)
	);
}

AVM1DateImpl AVM1DateImpl::getUTC(AVM1Activation& act) const
{
	return AVM1DateImpl
	(
		time -
		getLocalTZA(act, true) +
		getDSTAdjust(act)
	);
}

number_t AVM1DateImpl::getTimezoneOffset(AVM1Activation& act) const
{
	return (time - getLocalTime(act)).toFloatMs();
}

size_t AVM1DateImpl::getHours() const
{
	auto _time = time.toFloatMs() + 0.5;
	return remEuclidInt(std::floor(_time / msPerHour), hoursPerDay);
}

size_t AVM1DateImpl::getMinutes() const
{
	auto _time = time.toFloatMs();
	return remEuclidInt(std::floor(_time / msPerMin), minPerHour);
}

size_t AVM1DateImpl::getSeconds() const
{
	auto _time = time.toFloatMs();
	return remEuclidInt(std::floor(_time / msPerSec), secPerMin);
}

size_t AVM1DateImpl::getMilliseconds() const
{
	auto _time = time.toFloatMs();
	return remEuclidInt(time.toFloatMs(), msPerSec);
}

TimeSpec AVM1DateImpl::makeTime
(
	number_t hours,
	number_t minutes,
	number_t seconds,
	number_t milliseconds
)
{
	auto oldRoundMode = std::fegetround();
	// Round all time components towards zero.
	std::fesetround(FE_TOWARDZERO);

	hours = std::round(hours);
	minutes = std::round(minutes);
	seconds = std::round(seconds);
	milliseconds = std::round(milliseconds);

	std::fesetround(oldRoundMode);

	return TimeSpec::fromFloatMs
	(
		hours * msPerHour +
		minutes * msPerMin +
		seconds * msPerSec +
		milliseconds
	);
}

number_t AVM1DateImpl::dayFromMonth(number_t year, number_t month)
{
	auto _year = clampToInt<int32_t>(year);
	auto _month = clampToInt<int32_t>(std::floor(month));

	if (_month < 0 || _month > 12)
		return NaN;

	return dayFromYear(_year) + getMonthOffset
	(
		_month,
		isLeapYear(_year)
	);
}

number_t AVM1DateImpl::makeDay
(
	number_t year,
	number_t month,
	number_t date
)
{
	auto oldRoundMode = std::fegetround();
	// Round all time components towards zero.
	std::fesetround(FE_TOWARDZERO);

	year = std::round(year);
	month = std::round(month);
	date = std::round(date);

	std::fesetround(oldRoundMode);

	return dayFromMonth
	(
		year + std::floor(month / 12),
		remEuclid(month, 12)
	) + date - 1;
}

AVM1DateImpl AVM1DateImpl::clip() const
{
	constexpr number_t limit = 100000000.0 * msPerDay;
	return
	(
		isValid() &&
		std::fabs(time.toFloatMs()) <= limit
	) ? time : AVM1DateImpl();
}

tiny_string AVM1DateImpl::toString(AVM1Activation& act) const
{
	if (!isValid())
		return "Invalid Date";
	return act.getSys()->date->toFormatStr(time, "%a %b %-e %T GMT%z");
}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

constexpr auto objFlags = protoFlags | AVM1PropFlags::ReadOnly;

using AVM1Date;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getFullYear, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getYear, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getMonth, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getDate, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getDay, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getHours, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getMinutes, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getSeconds, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getMilliseconds, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getTime, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getTimezoneOffset, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, toString, protoFlags),

	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setFullYear, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setYear, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setMonth, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setDate, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setHours, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setMinutes, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setSeconds, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setMilliseconds, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setTime, protoFlags),

	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getUTCFullYear, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getUTCYear, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getUTCMonth, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getUTCDate, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getUTCDay, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getUTCHours, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getUTCMinutes, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getUTCSeconds, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, getUTCMilliseconds, protoFlags),

	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setUTCFullYear, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setUTCMonth, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setUTCDate, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setUTCHours, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setUTCMinutes, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setUTCSeconds, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, setUTCMilliseconds, protoFlags)
};

static constexpr auto objDecls =
{
	AVM1_FUNCTION_PROTO(UTC, objFlags)
};

GcPtr<AVM1SystemClass> AVM1Date::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, func, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);

	// NOTE: `Date.prototype.valueOf()` uses the same function object as
	// `Date.prototype.getTime()`.
	auto getTimeVal = _class->proto->getLocalProp(act, "getTime");
	_class->proto->defineValue
	(
		"valueOf",
		getTimeVal.valueOr(AVM1Value::undefinedVal),
		protoFlags
	);
	ctx.definePropsOn(_class->ctor, objDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1Date, ctor)
{
	if (args.empty())
	{
		auto date = act.getSys()->date->now();
		if (act.getSwfVersion() > 7)
			date = TimeSpec::fromMs(date.toMsRound());

		return _this.constCast() = NEW_GC_PTR
		(
			act.getGcCtx(),
			AVM1Date(act, date)
		);
	}

	if (args.size() == 1)
	{
		return _this.constCast() = NEW_GC_PTR(act.getGcCtx(), AVM1Date
		(
			act,
			args[0].toNumber(act)
		));
	}

	number_t year;
	number_t month;
	number_t date;
	number_t hours;
	number_t minutes;
	number_t seconds;
	number_t milliseconds;
	AVM1_ARG_UNPACK(year)(month)(date, 1)(hours, 0)(minutes, 0)
	(
		seconds,
		0
	)(milliseconds, 0);

	return _this.constCast() = NEW_GC_PTR(act.getGcCtx(), AVM1Date
	(
		act,
		year,
		month,
		date,
		hours,
		minutes,
		seconds,
		milliseconds
	));
}

static number_t setDate
(
	AVM1Activation& act,
	AVM1DateImpl& date,
	number_t day,
	number_t time,
	bool isUTC = false
)
{
	date = AVM1DateImpl::makeDate(day, time);
	if (!isUTC)
		date = date.getUTC(act);
	return date.getTime().toFloatMs();
}

// NOTE: `Date()`, when invoked without `new` returns the current date,
// and time as a `String`, as defined in ECMA-262.
AVM1_FUNCTION_BODY(AVM1Date, func)
{
	return AVM1DateImpl::now(act).getLocalTime(act).toString(act);
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getFullYear)
{
	if (!th->date.isValid())
		return NaN;
	return number_t(th->date.getLocalTime(act).getYear());
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getYear)
{
	if (!th->date.isValid())
		return NaN;
	return number_t(th->date.getLocalTime(act).getYear() - 1900);
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getMonth)
{
	if (!th->date.isValid())
		return NaN;
	return number_t(th->date.getLocalTime(act).getMonth());
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getDate)
{
	if (!th->date.isValid())
		return NaN;
	return number_t(th->date.getLocal(act).getDate());
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getDay)
{
	if (!th->date.isValid())
		return NaN;
	return number_t(th->date.getLocal(act).getDay());
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getHours)
{
	if (!th->date.isValid())
		return NaN;
	return number_t(th->date.getLocal(act).getHours());
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getMinutes)
{
	if (!th->date.isValid())
		return NaN;
	return number_t(th->date.getLocal(act).getMinutes());
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getSeconds)
{
	if (!th->date.isValid())
		return NaN;
	return number_t(th->date.getLocal(act).getSeconds());
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getMilliseconds)
{
	if (!th->date.isValid())
		return NaN;
	return number_t(th->date.getLocal(act).getMilliseconds());
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getTime)
{
	return th->date.getTime().toFloatMs();
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getTimezoneOffset)
{
	return th->date.getTimezoneOffset(act);
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, toString)
{
	return th->date.getLocal(act).toString(act);
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setFullYear)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	auto date = th->date.getLocalTime(act);
	auto year = unpacker.unpackIfDefined<number_t>([&]
	{
		return date.getYear();
	});

	auto month = unpacker.unpackIfDefined<number_t>([&]
	{
		return date.getMonth();
	});

	auto _date = unpacker.unpackIfDefined<number_t>([&]
	{
		return date.getDate();
	});

	return setDate
	(
		act,
		th->date,
		AVM1DateImpl::makeDay(year, month, _date),
		date.getTimeWithinDay(act.getSwfVersion())
	);
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setYear)
{
	auto date = th->date.getLocal(act);
	auto year = AVM1_ARG_UNPACK.tryUnpackIfDefined
	<
		number_t
	>().transformOrElse([&]
	{
		return date.getYear();
	}, [&](number_t year)
	{
		return year >= 0 && year <= 99 ? year + 1900 : year;
	});

	return setDate
	(
		act,
		th->date,
		AVM1DateImpl::makeDay(year, date.getMonth(), date.getDate()),
		date.getTimeWithinDay(act.getSwfVersion())
	);
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setMonth)
{
	AVM1_ARG_UNPACK_NAMED(unpacker);

	auto date = th->date.getLocal(act);
	auto month = unpacker.unpackIfDefined<number_t>([&]
	{
		return date.getMonth();
	});

	// NOTE: `setMonth()` has a special case where a non finite `month`
	// argument is treated as January.
	month = !std::isfinite(month) ? 0 : month;

	// NOTE: `setMonth()` has a special case where in SWF 7, and later,
	// if `date` is invalid, or empty, the `Date`'s time becomes `NaN`.
	if (unpacker.peek().is<UndefinedVal>() && act.getSwfVersion() > 6)
	{
		th->date = AVM1DateImpl();
		return NaN;
	}

	auto _date = unpacker.unpackOrElse<number_t>([&]
	{
		return date.getDate();
	});

	return setDate
	(
		act,
		th->date,
		AVM1DateImpl::makeDay(date.getYear(), month, _date),
		date.getTimeWithinDay(act.getSwfVersion())
	);
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setDate)
{
	auto date = th->date.getLocal(act);
	auto _date = AVM1_ARG_UNPACK.unpackIfDefined<number_t>([&]
	{
		return date.getDate();
	});

	return setDate
	(
		act,
		th->date,
		AVM1DateImpl::makeDay(date.getYear(), date.getMonth(), _date),
		date.getTimeWithinDay(act.getSwfVersion())
	);
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setHours)
{
	auto date = th->date.getLocal(act);
	auto hours = AVM1_ARG_UNPACK.unpackIfDefined<number_t>([&]
	{
		return date.getHours();
	});

	return setDate(act, th->date, date.getDay(), AVM1DateImpl::makeDate
	(
		hours,
		date.getMinutes(),
		date.getSeconds(),
		date.getMilliseconds()
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setMinutes)
{
	auto date = th->date.getLocal(act);
	auto mins = AVM1_ARG_UNPACK.unpackIfDefined<number_t>([&]
	{
		return date.getMinutes();
	});

	return setDate(act, th->date, date.getDay(), AVM1DateImpl::makeDate
	(
		date.getHours(),
		mins,
		date.getSeconds(),
		date.getMilliseconds()
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setSeconds)
{
	auto date = th->date.getLocal(act);
	auto secs = AVM1_ARG_UNPACK.unpackIfDefined<number_t>([&]
	{
		return date.getSeconds();
	});

	return setDate(act, th->date, date.getDay(), AVM1DateImpl::makeDate
	(
		date.getHours(),
		date.getMinutes(),
		secs,
		date.getMilliseconds()
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setMilliseconds)
{
	auto date = th->date.getLocal(act);
	auto ms = AVM1_ARG_UNPACK.unpackIfDefined<number_t>([&]
	{
		return date.getMillieconds();
	});

	return setDate(act, th->date, date.getDay(), AVM1DateImpl::makeDate
	(
		date.getHours(),
		date.getMinutes(),
		date.getSeconds(),
		ms
	));
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setTime)
{
	number_t timestamp;
	AVM1_ARG_UNPACK(timestamp, NaN);
	th->date = AVM1DateImpl(timestamp).clip();
	return th->date.getTime().toFloatMs();
}

AVM1_FUNCTION_BODY(AVM1Date, UTC)
{
	if (args.empty() || args.size() == 1)
		return AVM1Value::undefinedVal;

	number_t year;
	number_t month;
	number_t date;
	number_t hours;
	number_t minutes;
	number_t seconds;
	number_t milliseconds;
	AVM1_ARG_UNPACK(year)(month)(date, 1)(hours, 0)(minutes, 0)
	(
		seconds,
		0
	)(milliseconds, 0);

	return AVM1DateImpl
	(
		year,
		month,
		date,
		hours,
		minutes,
		seconds,
		milliseconds
	).getTime().toFloatMs();
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getUTCFullYear)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getUTCYear)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getUTCMonth)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getUTCDate)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getUTCDay)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getUTCHours)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getUTCMinutes)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getUTCSeconds)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, getUTCMilliseconds)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setUTCFullYear)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setUTCMonth)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setUTCDate)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setUTCHours)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setUTCMinutes)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setUTCSeconds)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, setUTCMilliseconds)
{
}
