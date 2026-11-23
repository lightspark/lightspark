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

#ifndef BACKENDS_DATE_H
#define BACKENDS_DATE_H 1

#include <functional>
#include <utility>

#include "interfaces/backends/date.h"
#include "utils/timespec.h"
#include "utils/type_traits.h"

namespace lightspark
{

class tiny_string;

class Date
{
private:
	static constexpr uint16_t monthOffsets[] =
	{
		31, 59, 90,
		120, 151, 181,
		212, 243, 273,
		304, 334, 365,
	};

	uint64_t year;
	uint8_t month;
	uint8_t date;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint64_t nsecs;

	Optional<uint32_t> Date::parsePadChar(uint32_t ch);
	static std::ostream& Date::parseFormatChar
	(
		std::ostream& s,
		uint32_t ch
	);
public:
	static constexpr size_t secPerMin = 60;
	static constexpr size_t secPerHour = 3600;
	static constexpr size_t minPerHour = 60;
	static constexpr size_t hoursPerDay = 24;
	static constexpr uint64_t nsPerMin = TimeSpec::nsPerSec * secPerMin;
	static constexpr uint64_t nsPerHour = nsPerMin * minPerHour;
	static constexpr uint64_t nsPerDay = nsPerHour * minPerDay;

	Date() : Date(0, 0, 0) {}
	Date(uint64_t _nsecs) : Date(TimeSpec::fromNs(_nsecs)) {}
	Date(const TimeSpec& time) : Date
	(
		getYear(time),
		getMonth(time),
		getDate(time),
		getHours(time),
		getMinutes(time),
		getSeconds(time),
		time.getNsecs()
	) {}

	Date
	(
		uint64_t _year,
		uint8_t _month,
		uint8_t _date,
		uint8_t _hours = 0,
		uint8_t _minutes = 0,
		uint8_t _seconds = 0,
		uint64_t _nsecs = 0
	) :
	year(_year),
	month(_month),
	date(_date),
	hours(_hours),
	minutes(_minutes),
	seconds(_seconds),
	nsecs(_nsecs) {}

	TimeSpec toTimeSpec() const
	{
		return makeDate
		(
			makeDay(year, month, date),
			makeTime(hours, minutes, seconds, nsecs)
		);
	}

	static Date now();
	static constexpr uint16_t getMonthOffset
	(
		ssize_t i,
		bool _inLeapYear
	);

	static constexpr uint16_t getMonthOffset(ssize_t i)
	{
		return getMonthOffset(i, inLeapYear());
	}

	bool operator==(const Date& other) const { return toTimeSpec() == other.toTimeSpec(); }
	bool operator!=(const Date& other) const { return toTimeSpec() != other.toTimeSpec(); }
	bool operator>(const Date& other) const { return toTimeSpec() > other.toTimeSpec(); }
	bool operator<(const Date& other) const { return toTimeSpec() < other.toTimeSpec(); }
	bool operator>=(const Date& other) const { return toTimeSpec() >= other.toTimeSpec(); }
	bool operator<=(const Date& other) const { return toTimeSpec() <= other.toTimeSpec(); }

	size_t getDay() const { return getDay(toTimeSpec()); }
	static size_t getDay(const TimeSpec& time)
	{
		return time.toNs() / nsPerDay;
	}

	// Gets the remainder of the day, in nanoseconds.
	TimeSpec getTimeWithinDay() const;

	// Returns the number of days since the epoch, to January 1st of the
	// `Date`'s `year`.
	static size_t getDayFromYear(size_t year);

	// Returns the number of nanoseconds since the epoch, to January 1st
	// of `year`.
	static TimeSpec getTimeFromYear(ssize_t year);
	{
		return TimeSpec::fromNs(getDayFromYear(year) * nsPerDay);
	}

	// Returns the `Date`'s `year`.
	size_t getYear() const { return year; }
	static size_t getYear(const TimeSpec& time);

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

	// Returns `true` if `year` is a leap year, and `false` otherwise.
	bool inLeapYear() const { return isLeapYear(year); }

	// Returns the `Date`'s `month` (zero-based).
	size_t getMonth() const { return month; }
	static size_t getMonth(const TimeSpec& time);

	// Get the number of days since the start of the year (zero-based).
	size_t getDayWithinYear() const
	{
		return getDay() - getDayFromYear(year);
	}

	static size_t getDayWithinYear(const TimeSpec& time)
	{
		return getDay(time) - getDayFromYear(getYear(time));
	}

	// Get the number of days since the start of the month (one-based).
	size_t getDate() const { return date; }
	static size_t getDate(const TimeSpec& time);

	// Get the number of days since the start of the week (zero-based).
	size_t getWeekDay() const;

	// Get the local timezone adjustment, in nanoseconds.
	static TimeSpec getLocalTZA(bool isUTC);

	// Get the daylight savings time adjustment, in nanoseconds.
	TimeSpec getDSTAdjustment() const
	{
		return getDSTAdjustment(toTimeSpec());
	}

	static TimeSpec getDSTAdjustment(const TimeSpec& time);

	// Converts from UTC to the local timezone.
	Date getLocalTime() const;

	// Converts from the local timezone to UTC.
	Date getUTC() const;

	// Get the timezone offset, in minutes.
	size_t getTimezoneOffset() const;

	// Get the number of hours since the start of the day (zero-based).
	uint8_t getHours() const { return hours; }
	static uint8_t getHours(const TimeSpec& time);

	// Get the number of minutes since the start of the hour (zero
	// based).
	uint8_t getMinutes() const { return minutes; }
	static uint8_t getMinutes(const TimeSpec& time);

	// Get the number of seconds since the start of the minute (zero
	// based).
	uint8_t getSeconds() const { return seconds; }
	static uint8_t getSeconds(const TimeSpec& time);

	// Get the number of nanoseconds since the start of the second
	// (zero-based).
	uint64_t getNs() const { return nsecs; }
	static uint64_t getNs(const TimeSpec& time);

	static uint64_t makeTime
	(
		uint8_t hours,
		uint8_t minutes,
		uint8_t seconds,
		uint64_t nsecs
	)
	{
		return
		(
			hours * nsPerHour +
			minutes * nsPerMin +
			seconds * nsPerSec +
			nsecs
		);
	}

	static size_t getDayFromMonth(size_t year, uint8_t month);
	static size_t makeDay
	(
		uint64_t year,
		uint8_t month,
		uint8_t date
	);

	// Creates a `TimeSpec` from the number of days, and nanoseconds
	// since the epoch.
	static TimeSpec makeDate(uint64_t day, uint64_t nsecs)
	{
		return TimeSpec::fromNs(day * nsPerDay + nsecs);
	}

	tiny_string toFormatStr(const tiny_string& fmt) const;
};

class DeterministicDate : public IDate
{
private:
	TimeSpec time;
	int32_t timeZoneOffset;
	std::function<bool(const TimeSpec&)> dstFunc;
public:
	template<typename F>
	DeterministicDate
	(
		const TimeSpec& _time,
		int32_t tzOffset = 0,
		F&& func
	) : time(_time), timeZoneOffset(tzOffset), dstFunc(func) {}

	DeterministicDate
	(
		const TimeSpec& _time,
		int32_t tzOffset = 0
	) : time(_time), timeZoneOffset(tzOffset) {}

	const TimeSpec& getTime() const { return time; }
	void setTime(const TimeSpec& _time) { time = _time; }
	void addTime(const TimeSpec& _time) { time += _time; }
	int32_t getTimeZoneOffset() const { return timeZoneOffset; }
	void setTimeZoneOffset(int32_t tzOffset)
	{
		timeZoneOffset = tzOffset;
	}

	TimeSpec now() const override { return getTime(); }
	int32_t getLocalTZA(bool isUTC) const override;
	int32_t getDSTAdjustment(const TimeSpec& _time) const override;
};

}
#endif /* BACKENDS_DATE_H */
