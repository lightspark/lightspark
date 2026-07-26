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

#ifndef BACKENDS_POSIX_DATE_H
#define BACKENDS_POSIX_DATE_H 1

#include <utility>

#include "interfaces/backends/date.h"
#include "utils/timespec.h"

namespace lightspark
{

class tiny_string;

class PosixDate : public IDate
{
public:
	TimeSpec now() const override;
	int32_t getLocalTZA(bool isUTC) const override;
	int32_t getDSTAdjustment(const TimeSpec& time) const override;
};

}
#endif /* BACKENDS_POSIX_DATE_H */
