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

#include <initializer_list>
#include <sstream>

#include "backends/date.h"
#include "scripting/avm1/clamp.h"
#include "tiny_string.h"

using namespace lightspark;

template<typename T>
static constexpr remEuclid(const T& a, const T& b)
{
	auto ret = a % b;
	return ret < 0 ? ret + std::abs(b) : ret;
}

Date Date::now()
{
	return Date(getSys()->date->now());
}

uint16_t Date::getMonthOffset
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

TimeSpec Date::getTimeWithinDay() const
{
	return TimeSpec::fromNs(remEuclid(toTimeSpec.toSNs(), nsPerDay);
}

size_t Date::getDayFromYear(size_t year)
{
	return
	(
		(365 * (year - 1970)) +
		((year - 1969) / 4) -
		((year - 1901) / 100) +
		((year - 1601) / 400)
	);
}

size_t Date::getYear(const TimeSpec& time)
{
	auto day = getDay(time);

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

size_t Date::getMonth(const TimeSpec& time)
{
	auto day = getDayWithinYear(time);
	bool _inLeapYear = isLeapYear(getYear(time));

	size_t i;
	for (i = 0; i < 11 && day < getMonthOffset(i + 1, _inLeapYear); ++i);
	return i;
}

size_t Date::getDate(const TimeSpec& time)
{
	return getDayWithinYear(time) - getMonthOffset(getMonth(time)) + 1;
}

size_t Date::getWeekDay() const
{
	return remEuclid(getDay() + 4, 7);
}

TimeSpec Date::getLocalTZA(bool isUTC)
{
	return TimeSpec::fromSec(getSys()->date->getLocalTZA(isUTC));
}

TimeSpec Date::getDSTAdjustment(const TimeSpec& time)
{
	auto date = getSys()->date;
	return TimeSpec::fromSec(date->getDSTAdjustment(time));
}

TimeSpec AVM1DateImpl::getDSTAdjustment()
{
	auto date = getSys()->date;
	return TimeSpec::fromSec(date->getDSTAdjustment(toTimeSpec()));
}

Date Date::getLocalTime() const
{
	return Date
	(
		toTimeSpec() +
		getLocalTZA(true) +
		getDSTAdjustment()
	);
}

Date Date::getUTC() const
{
	auto time = toTimeSpec() - getLocalTZA(false);
	return Date(time - getDSTAdjustment(time));
}

size_t Date::getTimezoneOffset() const
{
	return (toTimeSpec() - getLocalTime()).getSecs() / secPerMin;
}

uint8_t Date::getHours(const TimeSpec& time)
{
	return remEuclid(time.getSecs() / secPerHour, hoursPerDay);
}

uint8_t Date::getMinutes(const TimeSpec& time)
{
	return remEuclid(time.getSecs() / secPerMin, minPerHour);
}

uint8_t Date::getSeconds(const TimeSpec& time)
{
	return remEuclid(time.getSecs(), secPerMin);
}

uint64_t Date::getNs(const TimeSpec& time)
{
	return time.getNsecs();
}

size_t Date::getDayFromMonth(size_t year, uint8_t month)
{
	if (month > 12)
		return -1;

	return getDayFromYear(year) + getMonthOffset
	(
		month,
		isLeapYear(year)
	);
}

size_t Date::makeDay
(
	uint64_t year,
	uint8_t month,
	uint8_t date
)
{
	return dayFromMonth
	(
		year + (month / 12),
		remEuclid(month, 12)
	) + date - 1;
}

TimeSpec Date::makeDate(uint64_t day, uint64_t nsecs)
{
	return TimeSpec::fromNs(day * nsPerDay + nsecs);
}

Optional<uint32_t> Date::parsePadChar(uint32_t ch)
{
	switch (ch)
	{
		case '0': return '0'; break;
		case '-': return '\0'; break;
		case '_': return ' '; break;
		default: return {}; break;
	}
}

std::ostream& Date::parseFormatChar
(
	std::ostream& s,
	uint32_t ch
)
{
	#define FORCE_PAD(char, n) std::setfill(char) << std::setw(n)
	#define PAD(char, n) \
		FORCE_PAD(padChar.transformOr(char, [](auto ch) \
		{ \
			return ch == '\0' ? ' ' : ch; \
		}), padChar.valueOr(char) == '\0' ? 0 : n)

	static constexpr auto days = makeArray
	(
		"Sunday", "Monday", "Tuesday",
		"Wednesday", "Thursday", "Friday",
		"Saturday"
	);

	static constexpr auto months = makeArray
	(
		"January", "February", "March", "April",
		"May", "June", "July", "August",
		"September", "October", "November", "December"
	);

	switch (ch)
	{
		case 'a': return s << days[getWeekDay()].substr(0, 3); break;
		case 'A': return s << days[getWeekDay()]; break;
		case 'b': return s << months[month].substr(0, 3); break;
		case 'B': return s << months[month]; break;
		case 'e': return s << date; break;
		case 'T':
			return
			(
				s <<
				PAD('0', 2) << hours << ':' <<
				PAD('0', 2) << minutes << ':' <<
				PAD('0', 2) << seconds
			);
			break;
		case 'H': return s << PAD('0', 2) << hours; break;
		case 'M': return s << PAD('0', 2) << minutes; break;
		case 'S': return s << PAD('0', 2) << seconds; break;
		case 'z':
		{
			auto tzOffset = -getTimeZoneOffset();
			return
			(
				s << std::showpos <<
				FORCE_PAD('0', 2) << tzOffset / 60 <<
				std::shownopos <<
				FORCE_PAD('0', 2) << std::abs(tzOffset % 60)
			);
			break;
		}
		case 'Y': return s << year;
		default:
			LOG
			(
				LOG_NOT_IMPLEMENTED,
				"Date: "
				"format char \"%" << char(ch) << '"'
			);
			break;
	}
	#undef PAD
	#undef FORCE_PAD
}

tiny_string Date::toFormatStr(const tiny_string& fmt) const
{
	constexpr auto npos = tiny_string::npos;
	std::stringstream s;

	size_t prevPos = 0;
	for (size_t i = fmt.find('%'); i != npos; i = fmt.find(i, '%'))
	{
		s << fmt.substr(prevPos, i - prevPos);
		
		auto padChar = parsePadChar(fmt[i]);
		if (padChar.hasValue())
			++i;

		parseFormatChar
		(
			s,
			fmt[i++],
			padChar
		);
		prevPos = i;
	}

	return s.str();
}

int32_t DeterministicDate::getLocalTZA(bool isUTC) const
{
	return isUTC ? timeZoneOffset : -timeZoneOffset;
}

int32_t DeterministicDate::getDSTAdjustment(const TimeSpec& _time) const
{
	return
	(
		dstFunc != nullptr &&
		dstFunc(_time)
	) ? Date::secPerHour : 0;
}
