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

#ifndef SCRIPTING_AVM1_TOPLEVEL_DATE_H
#define SCRIPTING_AVM1_TOPLEVEL_DATE_H 1

#include "gc/ptr.h"
#include "scripting/avm1/clamp.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "utils/array.h"
#include "utils/timespec.h"
#include "utils/type_traits.h"

// Based on Ruffle's `avm1::globals::date` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1SystemClass;
class AVM1DeclContext;
class tiny_string;

template<typename T, typename U, EnableIf<Conj
<
	std::is_floating_point<T>,
	std::is_integral<U>,
	std::is_signed<U>
>, bool> = false>
static constexpr remEuclidInt(const T& a, const U& b)
{
	auto ret = clampToInt<U>(std::fmod(a, b));
	return ret < U(0) ? ret + b : ret;
}

class AVM1DateImpl
{
	using Date = AVM1DateImpl;
private:
	#if 1
	static constexpr uint16_t monthOffsets[] =
	{
		31, 59, 90,
		120, 151, 181,
		212, 243, 273,
		304, 334, 365,
	};
	#else
	static constexpr auto monthOffsets = makeArray<uint16_t>
	({
		31, 59, 90,
		120, 151, 181,
		212, 243, 273,
		304, 334, 365,
	});
	#endif

	static constexpr TimeSpec invalidTime
	(
		UINT64_MAX,
		UINT64_MAX,
		TimeSpec::DontNormalizeTag {}
	);

	TimeSpec time;
public:
	static constexpr size_t secPerMin = 60;
	static constexpr size_t minPerHour = 60;
	static constexpr size_t hoursPerDay = 24;
	static constexpr size_t msPerSec = 1000;
	static constexpr size_t msPerMin = msPerSec * secPerMin;
	static constexpr size_t msPerHour = msPerMin * minPerHour;
	static constexpr size_t msPerDay = msPerHour * minPerDay;

	Date(const TimeSpec& _time = invalidTime) : time(_time) {}
	Date(number_t ms) : Date(TimeSpec::fromMs(ms)) {}

	Date
	(
		number_t year,
		number_t month,
		number_t date,
		number_t hours,
		number_t minutes,
		number_t seconds,
		number_t milliseconds
	) : Date(makeDate
	(
		makeDay(year < 100 ? year + 1900 : year, month, date),
		makeTime(hours, minutes, seconds, milliseconds)
	)) {}

	const TimeSpec& getTime() const { return time; }
	bool isValid() const { return time != invalidTime; }
	static Date now(AVM1Activation& act);
	static constexpr uint16_t getMonthOffset
	(
		ssize_t i,
		bool _inLeapYear
	);

	static constexpr uint16_t getMonthOffset(ssize_t i)
	{
		return getMonthOffset(i, inLeapYear());
	}

	bool operator==(const Date& other) const { return time == other.time; }
	bool operator!=(const Date& other) const { return time != other.time; }
	bool operator>(const Date& other) const { return time > other.time; }
	bool operator<(const Date& other) const { return time < other.time; }
	bool operator>=(const Date& other) const { return time >= other.time; }
	bool operator<=(const Date& other) const { return time <= other.time; }

	// ECMA-262 2nd edition sec. 15.9.1.2. `Day`.
	// Gets the number of days since the epoch.
	number_t getDay() const
	{
		return time.toFloatMs() / msPerDay;
	}

	// ECMA-262 2nd edition sec. 15.9.1.2. `TimeWithinDay`.
	// Gets the remainder of the day, in milliseconds.
	//
	// NOTE: This returns a `TimeSpec`, rather than a `number_t` to
	// prevent precision loss.
	TimeSpec getTimeWithinDay(uint8_t swfVersion) const;

	// ECMA-262 2nd edition sec. 15.9.1.3. `DayFromYear`.
	// Returns the number of days since the epoch, to January 1st of the
	// `Date`'s `year`.
	static number_t dayFromYear(number_t year);

	// ECMA-262 2nd edition sec. 15.9.1.3. `TimeFromYear`.
	// Returns the number of milliseconds since the epoch, to January 1st
	// of `year`.
	//
	// NOTE: Because this returns milliseconds, we return a `TimeSpec`,
	// rather than a `number_t`.
	static Date timeFromYear(ssize_t year)
	{
		return Date(dayFromYear(year) * msPerDay);
	}

	// ECMA-262 2nd edition sec. 15.9.1.3. `YearFromTime`.
	// Returns the `Date`'s `year`.
	size_t getYear() const;

	// Figures out whether `year` is a leap year, or not (i.e. a 366 day
	// year, instead of a 365 day year).
	static constexpr bool isLeapYear(ssize_t year)
	{
		return !(year % 4) &&
		(
			year % 100 ||
			!(year % 400)
		);
	}

	// ECMA-262 2nd edition sec. 15.9.1.3. `InLeapYear`.
	// Returns `true` if `year` is a leap year, and `false` otherwise.
	bool inLeapYear() const { return isLeapYear(getYear()); }

	// ECMA-262 2nd edition sec. 15.9.1.4. `MonthFromTime`.
	// Returns the `Date`'s `month` (zero-based).
	size_t getMonth() const;

	// ECMA-262 2nd edition sec. 15.9.1.4. `DayWithinYear`.
	// Get the number of days since the start of the year (zero-based).
	size_t getDayWithinYear() const
	{
		return clampToInt<int32_t>
		(
			getDay() -
			dayFromYear(getYear())
		);
	}

	// ECMA-262 2nd edition sec. 15.9.1.5. `DateFromTime`.
	// Get the number of days since the start of the month (one-based).
	size_t getDate() const;

	// ECMA-262 2nd edition sec. 15.9.1.6. `WeekDay`.
	// Get the number of days since the start of the week (zero-based).
	size_t getWeekDay() const;

	// ECMA-262 2nd edition sec. 15.9.1.7. `LocalTZA`.
	// Get the local timezone adjustment, in milliseconds.
	TimeSpec getLocalTZA(AVM1Activation& act, bool isUTC) const;

	// ECMA-262 2nd edition sec. 15.9.1.8. `DaylightSavingTA`.
	// Get the daylight savings time adjustment, in milliseconds.
	TimeSpec getDSTAdjustment(AVM1Activation& act) const;

	// ECMA-262 2nd edition sec. 15.9.1.9. `LocalTime`.
	// Converts from UTC to the local timezone.
	Date getLocalTime(AVM1Activation& act) const;

	// ECMA-262 2nd edition sec. 15.9.1.9. `UTC`.
	// Converts from the local timezone to UTC.
	Date getUTC(AVM1Activation& act) const;

	// Get the timezone offset, in minutes.
	number_t getTimezoneOffset(AVM1Activation& act) const;

	// ECMA-262 2nd edition sec. 15.9.1.10. `HourFromTime`.
	// Get the number of hours since the start of the day (zero-based).
	size_t getHours() const;

	// ECMA-262 2nd edition sec. 15.9.1.10. `MinFromTime`.
	// Get the number of minutes since the start of the hour (zero
	// based).
	size_t getMinutes() const;

	// ECMA-262 2nd edition sec. 15.9.1.10. `SecFromTime`.
	// Get the number of seconds since the start of the minute (zero
	// based).
	size_t getSeconds() const;

	// ECMA-262 2nd edition sec. 15.9.1.10. `msFromTime`.
	// Get the number of milliseconds since the start of the second
	// (zero-based).
	size_t getMilliseconds() const;

	// ECMA-262 2nd edition sec. 15.9.1.11. `MakeTime`.
	static TimeSpec makeTime
	(
		number_t hours,
		number_t minutes,
		number_t seconds,
		number_t milliseconds
	);

	static number_t dayFromMonth(number_t year, number_t month);

	// ECMA-262 2nd edition sec. 15.9.1.12. `MakeDay`.
	static number_t makeDay
	(
		number_t year,
		number_t month,
		number_t date
	);

	// ECMA-262 2nd edition sec. 15.9.1.13. `MakeDate`.
	// Creates a `Number` from the number of days, and milliseconds
	// since the epoch.
	static Date makeDate(number_t day, number_t time)
	{
		return Date(day * msPerDay + time);
	}

	// ECMA-262 2nd edition sec. 15.9.1.14. `TimeClip`.
	Date clip() const;

	// ECMA-262 2nd edition sec. 15.9.5.2. `toString`.
	tiny_string toString(AVM1Activation& act) const;
};

class AVM1Date : public AVM1Object
{
private:
	AVM1DateImpl date;
public:
	const AVM1DateImpl& getDate() const { return date; }
	AVM1DateImpl& getDate() { return date; }
	const TimeSpec& getTime() const { return date.getTime(); }
	bool isValid() const { return date.isValid(); }

	AVM1Date(AVM1Activation& act);
	AVM1Date
	(
		AVM1Activation& act,
		const TimeSpec& time
	) : AVM1Date(act), date(time) {}

	AVM1Date
	(
		AVM1Activation& act,
		number_t ms
	) : AVM1Date(act), date(ms) {}

	AVM1Date
	(
		AVM1Activation& act,
		number_t year,
		number_t month,
		number_t date,
		number_t hours,
		number_t minutes,
		number_t seconds,
		number_t milliseconds
	) : AVM1Date(act), date
	(
		year,
		month,
		date,
		hours,
		minutes,
		seconds,
		milliseconds
	) {}

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);
	AVM1_FUNCTION_DECL(func);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getFullYear);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getYear);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getMonth);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getDate);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getDay);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getHours);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getMinutes);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getSeconds);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getMilliseconds);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getTime);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getTimezoneOffset);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, toString);

	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setFullYear);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setYear);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setMonth);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setDate);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setHours);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setMinutes);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setSeconds);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setMilliseconds);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setTime);

	AVM1_FUNCTION_DECL(UTC);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getUTCFullYear);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getUTCYear);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getUTCMonth);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getUTCDate);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getUTCDay);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getUTCHours);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getUTCMinutes);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getUTCSeconds);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, getUTCMilliseconds);

	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setUTCFullYear);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setUTCMonth);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setUTCDate);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setUTCHours);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setUTCMinutes);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setUTCSeconds);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, setUTCMilliseconds);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_DATE_H */
