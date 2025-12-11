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
#include "utils/array.h"
#include "utils/timespec.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::date` crate.

constexpr size_t secPerMin = 60;
constexpr size_t minPerHour = 60;
constexpr size_t hoursPerDay = 24;
constexpr size_t msPerSec = 1000;
constexpr size_t msPerMin = msPerSec * secPerMin;
constexpr size_t msPerHour = msPerMin * minPerHour;
constexpr size_t msPerDay = msPerHour * minPerDay;

constexpr uint16_t monthOffsets[] =
{
	0, 31, 59,
	90, 120, 151,
	181, 212, 243,
	273, 304, 334,
	365,
};

AVM1Date::AVM1Date(AVM1Activation& act) : AVM1Object
(
	act,
	act.getPrototypes().date->proto
)
{
}

size_t AVM1Date::getDay() const
{
	return time.toMs() / msPerDay;
}

TimeSpec AVM1Date::getTimeWithinDay(uint8_t swfVersion) const
{
	auto _time = time.toFloat() * msPerSec;

	return TimeSpec::fromFloat
	(
		swfVersion > 7 ?
		remEuclid(_time, msPerDay) :
		std::fmod(_time, msPerDay)
	);
}

size_t AVM1Date::dayFromYear(number_t year)
{
	return
	(
		(365 * (year - 1970)) +
		std::floor((year - 1969) / 4) -
		std::floor((year - 1901) / 100) +
		std::floor((year - 1601) / 400)
	);
}

TimeSpec AVM1Date::timeFromYear(ssize_t year)
{
	return TimeSpec::fromMs(dayFromYear(year) * msPerDay);
}

size_t AVM1Date::getYear() const
{
	number_t day = getDay();

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

		if (timeFromYear(pivot) > time)
		{
			high = pivot - 1;
			continue;
		}

		assert(timeFromYear(pivot) <= time);
		if (timeFromYear(pivot + 1) > time)
			return pivot;
		low = pivot + 1;
	}

	return low;
}

size_t AVM1Date::getMonth() const
{
}

size_t AVM1Date::getDayWithinYear() const
{
}

size_t AVM1Date::getDate() const
{
}

size_t AVM1Date::getWeekDay() const
{
}

TimeSpec AVM1Date::getLocalTZA(bool isUTC) const
{
}

TimeSpec AVM1Date::getLocalTime() const
{
}

TimeSpec AVM1Date::getUTC() const
{
}

number_t AVM1Date::getTimezoneOffset() const
{
}

size_t AVM1Date::getHours() const
{
}

size_t AVM1Date::getMinutes() const
{
}

size_t AVM1Date::getSeconds() const
{
}

size_t AVM1Date::getMilliseconds() const
{
}

TimeSpec AVM1Date::makeTime
(
	number_t hours,
	number_t minutes,
	number_t seconds,
	number_t milliseconds
)
{
}

number_t AVM1Date::dayFromMonth(number_t year, number_t month)
{
}

TimeSpec AVM1Date::makeDay
(
	number_t year,
	number_t month,
	number_t date
)
{
}

TimeSpec AVM1Date::makeDate(number_t day, number_t time)
{
}

TimeSpec AVM1Date::clip() const
{
}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontEnum |
	AVM1PropFlags::DontDelete
);

using AVM1Date;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_PROTO(domain, protoFlags),
	AVM1_FUNCTION_PROTO(connect, protoFlags),
	AVM1_FUNCTION_TYPE_PROTO(AVM1Date, close, protoFlags),
	AVM1_FUNCTION_PROTO(send, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1Date::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeNativeClass(ctor, nullptr, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
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

// NOTE: `Date()`, when invoked without `new` returns the current date,
// and time as a `String`, as defined in ECMA-262.
AVM1_FUNCTION_BODY(AVM1Date, func)
{
	// TODO: Implement.
}

AVM1_PROPERTY_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, FullYear)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, Year)
{
}

AVM1_PROPERTY_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, Month)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, Date)
{
}

AVM1_GETTER_TYPE_BODY(AVM1Date, AVM1Date, Day)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, Hours)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, Minutes)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, Seconds)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, Milliseconds)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, Time)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, toString)
{
}

AVM1_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, UTC)
{
}

AVM1_PROPERTY_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, UTCFullYear)
{
}

AVM1_GETTER_TYPE_BODY(AVM1Date, AVM1Date, UTCYear)
{
}

AVM1_PROPERTY_FUNCTION_TYPE_BODY(AVM1Date, AVM1Date, UTCMonth)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, UTCDate)
{
}

AVM1_GETTER_TYPE_BODY(AVM1Date, AVM1Date, UTCDay)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, UTCHours)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, UTCMinutes)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, UTCSeconds)
{
}

AVM1_PROPERTY_TYPE_BODY(AVM1Date, AVM1Date, UTCMilliseconds)
{
}
