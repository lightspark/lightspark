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

static constexpr auto monthOffsets = makeArray<uint16_t>
(
	31, 59, 90,
	120, 151, 181,
	212, 243, 273,
	304, 334, 365
);

size_t DeterministicDate::getDayWithinYear(const TimeSpec& time)
{
	return clampToInt<int32_t>
	(
		getDay(time) -
		dayFromYear(getYear(time))
	);
}

uint16_t DeterministicDate::getMonthOffset
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

uint64_t DeterministicDate::dayFromYear(uint64_t year)
{
	return
	(
		(365 * (year - 1970)) +
		((year - 1969) / 4) -
		((year - 1901) / 100) +
		((year - 1601) / 400)
	);
}

static TimeSpec timeFromYear(ssize_t year)
{
	constexpr uint64_t secPerMin = 60;
	constexpr uint64_t minPerHour = 60;
	constexpr uint64_t hoursPerDay = 24;
	constexpr uint64_t nsPerMin = TimeSpec::nsPerSec * secPerMin;
	constexpr uint64_t nsPerHour = nsPerMin * minPerHour;
	constexpr uint64_t nsPerDay = nsPerHour * hoursPerDay;
	return TimeSpec(dayFromYear(year) * nsPerDay);
}

size_t DeterministicDate::getYear(const TimeSpec& time)
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

size_t DeterministicDate::getMonth(const TimeSpec& time)
{
	auto day = getDayWithinYear(time);
	bool _inLeapYear = inLeapYear(time);

	#if 1
	size_t i;
	for (i = 0; i < 11 && day < getMonthOffset(i + 1, _inLeapYear); ++i);
	return i;
	#else
	return std::distance(monthOffsets.begin() + 1, std::find_if
	(
		monthOffsets.begin() + 1,
		monthOffsets.end(),
		[&](uint16_t offset)
		{
			return day < offset + _inLeapYear;
		}
	)) - 1;
	#endif
}

size_t DeterministicDate::getDate(const TimeSpec& time)
{
	return dayWithinYear(time) - getMonthOffset(getMonth(time)) + 1;
}

size_t DeterministicDate::getWeekDay(const TimeSpec& time)
{
	return remEuclidInt(getDay(time) + 4, 7);
}

int32_t DeterministicDate::getLocalTZA(bool isUTC) const
{
	return isUTC ? timeZoneOffset : -timeZoneOffset;
}

int32_t DeterministicDate::getDSTAdjustment(const TimeSpec& _time) const
{
	constexpr int32_t secPerHour = 3600;
	return
	(
		dstFunc != nullptr &&
		dstFunc(_time)
	) ? secPerHour : 0;
}

static Optional<uint32_t> parsePadChar(uint32_t ch)
{
	switch (ch)
	{
		case '0': return '0'; break;
		case '-': return '\0'; break;
		case '_': return ' '; break;
		default: return {}; break;
	}
}

static std::stringstream& parseFormatChar
(
	std::stringstream& s,
	uint32_t ch,
	const TimeSpec& time
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
		case 'a': return s << days[getDayInWeek(time)].substr(0, 3); break;
		case 'A': return s << days[getDayInWeek(time)]; break;
		case 'b': return s << months[getMonth(time)].substr(0, 3); break;
		case 'B': return s << months[getMonth(time)]; break;
		case 'e': return s << getDayInMonth(time); break;
		case 'T':
			return
			(
				s <<
				PAD('0', 2) << getHour(time) << ':' <<
				PAD('0', 2) << getMinute(time) << ':' <<
				PAD('0', 2) << getSecond(time)
			);
			break;
		case 'H': return s << PAD('0', 2) << getHour(time); break;
		case 'M': return s << PAD('0', 2) << getMinute(time); break;
		case 'S': return s << PAD('0', 2) << getSecond(time); break;
		case 'z':
		{
			auto tzOffset = getTimeZoneOffset();
			return
			(
				s << std::showpos <<
				FORCE_PAD('0', 2) << tzOffset / 60 <<
				std::shownopos <<
				FORCE_PAD('0', 2) << std::abs(tzOffset % 60)
			);
			break;
		}
		case 'Y': return s << getYear(time);
		default:
			LOG
			(
				LOG_NOT_IMPLEMENTED,
				"DeterministicDate: "
				"format char \"%" << char(ch) << '"'
			);
			break;
	}
	#undef PAD
	#undef FORCE_PAD
}

tiny_string DeterministicDate::toFormatStr
(
	const TimeSpec& _time,
	const tiny_string& fmt
) const
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
			time,
			padChar
		);
		prevPos = i;
	}

	return s.str();
}
