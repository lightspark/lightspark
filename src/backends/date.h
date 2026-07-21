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

#include <utility>

#include "interfaces/backends/date.h"
#include "utils/timespec.h"
#include "utils/type_traits.h"

namespace lightspark
{

class tiny_string;

class DeterministicDate : public IDate
{
private:
	TimeSpec time;
	int32_t timeZoneOffset;
public:
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
	tiny_string toFormatStr
	(
		const TimeSpec& _time,
		const tiny_string& fmt
	) const override;
};

}
#endif /* BACKENDS_DATE_H */
