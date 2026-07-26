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

#ifndef INTERFACES_BACKENDS_DATE_H
#define INTERFACES_BACKENDS_DATE_H 1

#include <cstdint>

namespace lightspark
{

class TimeSpec;

class IDate
{
public:
	// Gets the current system time, in `TimeSpec` form.
	//
	// NOTE: This differs from `ITime::now()` in that it returns real
	// time, rather than monotonic time.
	virtual TimeSpec now() const = 0;

	// Gets the current local timezone offset, in seconds.
	virtual int32_t getLocalTZA(bool isUTC) const = 0;

	// Gets the current daylight savings time offset, in seconds.
	virtual int32_t getDSTAdjustment(const TimeSpec& time) const = 0;
};

}
#endif /* INTERFACES_BACKENDS_DATE_H */
