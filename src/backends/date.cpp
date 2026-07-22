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

static std::stringstream& parseFormatChar
(
	std::stringstream& s,
	uint32_t ch,
	const TimeSpec& time
)
{
	#define PAD(char, n) \
		std::setfill(char) << std::setw(n)
	
	static constexpr auto days = makeArray
	(
		"Sun", "Mon", "Tue",
		"Wed", "Thu", "Fri",
		"Sat"
	);
	
	static constexpr auto months = makeArray
	(
		"Jan", "Feb", "Mar", "Apr",
		"May", "Jun", "Jul", "Aug",
		"Sep", "Oct", "Nov", "Dec"
	);

	switch (ch)
	{
		case 'a': return s << days[getDayInWeek(time)];
		case 'b': return s << months[getMonth(time)];
		case 'e': return s << getDayInMonth(time);
		case 'T':
			return
			(
				s <<
				PAD('0', 2) << getHour(time) << ':' <<
				PAD('0', 2) << getMinute(time) << ':' <<
				PAD('0', 2) << getSecond(time)
			);
		case 'H': return s << PAD('0', 2) << getHour(time);
		case 'M': return s << PAD('0', 2) << getMinute(time);
		case 'S': return s << PAD('0', 2) << getSecond(time);
		case 'z':
		{
			auto tzOffset = getTimeZoneOffset();
			return
			(
				s << std::showpos <<
				PAD('0', 2) << tzOffset / 60 <<
				std::shownopos <<
				PAD('0', 2) << std::abs(tzOffset % 60)
			);
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
		
		auto ch = fmt[i++];

		parseFormatChar(s, ch, time);
		prevPos = i;
	}

	return s.str();
}
