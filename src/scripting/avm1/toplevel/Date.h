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
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "utils/timespec.h"

// Based on Ruffle's `avm1::globals::date` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1SystemClass;
class AVM1DeclContext;

class AVM1Date : public AVM1Object
{
private:
	static constexpr TimeSpec invalidTime
	(
		UINT64_MAX,
		UINT64_MAX,
		TimeSpec::DontNormalizeTag {}
	);

	TimeSpec time;

	// ECMA-262 2nd edition sec. 15.9.1.2. `Day`.
	// Gets the number of days since the epoch.
	size_t getDay() const;

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
	static TimeSpec timeFromYear(ssize_t year);

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
	size_t getDayWithinYear() const;

	// ECMA-262 2nd edition sec. 15.9.1.5. `DateFromTime`.
	// Get the number of days since the start of the month (one-based).
	size_t getDate() const;

	// ECMA-262 2nd edition sec. 15.9.1.6. `WeekDay`.
	// Get the number of days since the start of the week (zero-based).
	size_t getWeekDay() const;

	// ECMA-262 2nd edition sec. 15.9.1.7. `LocalTZA`.
	// Get the local timezone adjustment, in milliseconds.
	TimeSpec getLocalTZA(bool isUTC) const;

	// ECMA-262 2nd edition sec. 15.9.1.9. `LocalTime`.
	// Converts from UTC to the local timezone.
	TimeSpec getLocalTime() const;

	// ECMA-262 2nd edition sec. 15.9.1.9. `UTC`.
	// Converts from the local timezone to UTC.
	TimeSpec getUTC() const;

	// Get the timezone offset, in minutes.
	number_t getTimezoneOffset() const;

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
	static TimeSpec makeDay
	(
		number_t year,
		number_t month,
		number_t date
	);

	// ECMA-262 2nd edition sec. 15.9.1.13. `MakeDate`.
	// Creates a `Number` from the number of days, and milliseconds
	// since the epoch.
	//
	// NOTE: Because this returns milliseconds, we return a `TimeSpec`,
	// rather than a `number_t`.
	static TimeSpec makeDate(number_t day, number_t time);

	// ECMA-262 2nd edition sec. 15.9.1.14. `TimeClip`.
	TimeSpec clip() const;
public:
	const TimeSpec& getTime() const { return time; }
	bool isValid() const { return time != invalidTime; }

	AVM1Date(AVM1Activation& act);
	AVM1Date
	(
		AVM1Activation& act,
		const TimeSpec& _time
	) : AVM1Date(act), time(_time) {}

	AVM1Date
	(
		AVM1Activation& act,
		number_t ms
	) : AVM1Date(act), time(TimeSpec::fromMs(ms)) {}

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
	) : AVM1Date(act makeDate
	(
		makeDay(year < 100 ? year + 1900 : year, month, date),
		makeTime(hours, minutes, seconds, milliseconds)
	)) {}

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);
	AVM1_FUNCTION_DECL(func);
	// NOTE: `PROPERTY_FUNCTION_TYPE_DECL` is used for `{g,s}etFullYear()`
	// because `setFullYear()` can take more than one argument.
	AVM1_PROPERTY_FUNCTION_TYPE_DECL(AVM1Date, FullYear);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, Year);
	// NOTE: `PROPERTY_FUNCTION_TYPE_DECL` is used for `{g,s}etMonth()`
	// because `setMonth()` can take more than one argument.
	AVM1_PROPERTY_FUNCTION_TYPE_DECL(AVM1Date, Month);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, Date);
	AVM1_GETTER_TYPE_DECL(AVM1Date, Day);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, Hours);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, Minutes);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, Seconds);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, Milliseconds);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, Time);
	AVM1_FUNCTION_TYPE_DECL(AVM1Date, toString);

	AVM1_FUNCTION_TYPE_DECL(AVM1Date, UTC);
	// NOTE: `PROPERTY_FUNCTION_TYPE_DECL` is used for
	// `{g,s}etUTCFullYear()` because `setUTCFullYear()` can take more
	// than one argument.
	AVM1_PROPERTY_FUNCTION_TYPE_DECL(AVM1Date, UTCFullYear);
	AVM1_GETTER_TYPE_DECL(AVM1Date, UTCYear);
	// NOTE: `PROPERTY_FUNCTION_TYPE_DECL` is used for `{g,s}etUTCMonth()`
	// because `setUTCMonth()` can take more than one argument.
	AVM1_PROPERTY_FUNCTION_TYPE_DECL(AVM1Date, UTCMonth);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, UTCDate);
	AVM1_GETTER_TYPE_DECL(AVM1Date, UTCDay);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, UTCHours);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, UTCMinutes);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, UTCSeconds);
	AVM1_PROPERTY_TYPE_DECL(AVM1Date, UTCMilliseconds);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_DATE_H */
