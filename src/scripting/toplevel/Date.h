/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_TOPLEVEL_DATE_H
#define SCRIPTING_TOPLEVEL_DATE_H 1

#include "asobject.h"

namespace lightspark
{

class Date: public ASObject
{
private:
	int64_t milliseconds;
	int extrayears;
	bool nan;
	~Date();
	GDateTime *datetime;
	GDateTime *datetimeUTC;
	ASObject *msSinceEpoch();
	tiny_string toString_priv(bool utc, const char* formatstr) const;
	void MakeDate(int64_t year, int64_t month, int64_t day, int64_t hour, int64_t minute, int64_t second, int64_t millisecond, bool bIsLocalTime);
	void MakeDateFromMilliseconds(int64_t ms);
	static number_t parse(tiny_string str);
public:
	Date(Class_base* c);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	ASFUNCTION(UTC);
	ASFUNCTION(getTimezoneOffset);
	ASFUNCTION(getTime);
	ASFUNCTION(getFullYear);
	ASFUNCTION(getMonth);
	ASFUNCTION(getDate);
	ASFUNCTION(getDay);
	ASFUNCTION(getHours);
	ASFUNCTION(getMinutes);
	ASFUNCTION(getSeconds);
	ASFUNCTION(getMilliseconds);
	ASFUNCTION(getUTCFullYear);
	ASFUNCTION(getUTCMonth);
	ASFUNCTION(getUTCDate);
	ASFUNCTION(getUTCDay);
	ASFUNCTION(getUTCHours);
	ASFUNCTION(getUTCMinutes);
	ASFUNCTION(getUTCSeconds);
	ASFUNCTION(getUTCMilliseconds);
	ASFUNCTION(valueOf);
	ASFUNCTION(setFullYear);
	ASFUNCTION(setMonth);
	ASFUNCTION(setDate);
	ASFUNCTION(setHours);
	ASFUNCTION(setMinutes);
	ASFUNCTION(setSeconds);
	ASFUNCTION(setMilliseconds);
	ASFUNCTION(setUTCFullYear);
	ASFUNCTION(setUTCMonth);
	ASFUNCTION(setUTCDate);
	ASFUNCTION(setUTCHours);
	ASFUNCTION(setUTCMinutes);
	ASFUNCTION(setUTCSeconds);
	ASFUNCTION(setUTCMilliseconds);
	ASFUNCTION(setTime);
	ASFUNCTION(fullYearSetter);
	ASFUNCTION(monthSetter);
	ASFUNCTION(dateSetter);
	ASFUNCTION(hoursSetter);
	ASFUNCTION(minutesSetter);
	ASFUNCTION(secondsSetter);
	ASFUNCTION(millisecondsSetter);
	ASFUNCTION(UTCFullYearSetter);
	ASFUNCTION(UTCMonthSetter);
	ASFUNCTION(UTCDateSetter);
	ASFUNCTION(UTCHoursSetter);
	ASFUNCTION(UTCMinutesSetter);
	ASFUNCTION(UTCSecondsSetter);
	ASFUNCTION(UTCMillisecondsSetter);
	ASFUNCTION(timeSetter);
	ASFUNCTION(_toString);
	ASFUNCTION(_parse);
	ASFUNCTION(toUTCString);
	ASFUNCTION(toDateString);
	ASFUNCTION(toTimeString);
	ASFUNCTION(toLocaleString);
	ASFUNCTION(toLocaleDateString);
	ASFUNCTION(toLocaleTimeString);
	tiny_string toString();
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};
}
#endif /* SCRIPTING_TOPLEVEL_DATE_H */
