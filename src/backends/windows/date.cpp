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

#include <datetimeapi.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <ntdef.h>
#include <windef.h>
#include <winbase.h>
#include <winnt.h>

#include "backends/windows/date.h"
#include "utils/timespec.h"

using namespace lightspark;

// `fromFILETIME()` taken from `FileSystem`'s Windows backend.

// The number of seconds between 01-01-1601, and 01-01-1970.
constexpr uint64_t unixEpochOffset = 11644473600;
constexpr uint64_t ivalPerSec = TimeSpec::nsPerSec / 100;
constexpr uint64_t ivalToUnixEpoch = unixEpochOffset * ivalPerSec;

static TimeSpec fromFILETIME(const FILETIME& fileTime)
{
	ULARGE_INTEGER bigInt;
	bigInt.LowPart = fileTime.dwLowDateTime;
	bigInt.HighPart = fileTime.dwHighDateTime;

	return TimeSpec::fromNs((bigInt.QuadPart - ivalToUnixEpoch) * 100);
}

static SYSTEMTIME getSysTime()
{
	SYSTEMTIME sysTime;
	GetSystemTime(&sysTime);
	return sysTime;
}

static TIME_ZONE_INFORMATION getTimeZoneInfo()
{
	TIME_ZONE_INFORMATION tzInfo;
	GetTimeZoneInformation(&tzInfo);
	return tzInfo;
}

TimeSpec WindowsDate::now() const override
{
	FILETIME fileTime;
	auto sysTime = getSysTime();

	SystemTimeToFileTime(&sysTime, &fileTime);
	return fromFILETIME(fileTime);
}

int32_t WindowsDate::getLocalTZA(bool isUTC) const
{
	return -getTimeZoneInfo().Bias * 60;
}

// Based on avmplus' `ConvertWin32DST()`.
static TimeSpec fromWinDST(size_t year, const SYSTEMTIME& sysTime)
{
	constexpr auto nsPerWeek = 7 * Date::nsPerDay;

	if (sysTime.wYear)
	{
		return Date
		(
			year,
			sysTime.wMonth - 1,
			sysTime.wDay,
			sysTime.wHour,
			sysTime.wMinute,
			sysTime.wSecond,
			sysTime.wMilliseconds * TimeSpec::nsPerMs
		).toTimeSpec();
	}

	Date sysDate
	(
		year,
		sysTime.wMonth - 1,
		1,
		sysTime.wHour,
		sysTime.wMinute,
		sysTime.wSecond,
		sysTime.wMilliseconds * TimeSpec::nsPerMs
	);

	auto time = sysDate.toTimeSpec();
	auto nextMonth = Date
	(
		year + sysTime.wMonth >= 12,
		sysTime.wMonth < 12 ? sysTime.wMonth : 0,
		1
	).toTimeSpec();

	auto weekDay = sysDate.getWeekDay();
	if (weekDay != sysTime.wDayOfWeek)
	{
		auto dayDiff = sysTime.wDayOfWeek - weekDay;
		time += TimeSpec::fromNs
		((
			dayDiff < 0 ?
			dayDiff + 7 :
			dayDiff
		) * Date::nsPerDay);
	}

	// Advance by the appropriate number of weeks.
	time += TimeSpec::fromNs((sysTime.wDay - 1) * nsPerWeek);

	// Clamp it to the end of the month.
	for (; time >= nextMonth; time -= TimeSpec::fromNs(nsPerWeek));
	return time;
}

// Based on avmplus' Windows version of `VMPI_getDaylightSavingsTA()`.
int32_t WindowsDate::getDSTAdjustment(const TimeSpec& time) const
{
	auto tzInfo = getTimeZoneInfo();
	if (tzInfo.DaylightBias != -60 || !tzInfo.DaylightDate.wMonth)
		return 0;

	auto year = Date::getYear(time);

	auto timeDst = fromWinDST(year, tzInfo.DaylightDate);
	auto timeStd = fromWinDST(year, tzInfo.StandardDate);

	timeStd -= TimeSpec::fromNs(Date::nsPerHour);
	auto _time = time + TimeSpec::fromSec(getLocalTZA(true));

	bool isDst =
	(
		timeDst <= timeStd &&
		_time >= timeDst &&
		_time < timeStd
	) ||
	(
		_time >= timeStd &&
		_time < timeDst
	);

	return isDst ? Date::secPerHour : 0;
}
