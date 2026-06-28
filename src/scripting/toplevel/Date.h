/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
protected:
	number_t mstimestamp;
	number_t mstimezoneoffset;
	bool nan;
	bool isinfinite;
	bool isinfiniteYear;
	number_t getMsSinceEpoch(bool utc) const;
	number_t getTimeInMS(bool utc) const;
	number_t computeMsTimezoneOffset() const;
	tiny_string toString_priv(bool utc, const char* formatstr) const;
	static number_t parse(tiny_string str);
	int64_t getcurrentms();
	bool isValid() const { return !nan; }
	bool isInfinite() const { return isinfinite; }
	struct tm* getDateTimeLocal() const;
	struct tm* getDateTimeUTC() const;
	int32_t getYear(bool utc) const;
	int32_t getMonth(bool utc) const;
	int32_t getDayInMonth(bool utc) const;
	int32_t getDayInWeek(bool utc) const;
	int32_t getHour(bool utc) const;
	int32_t getMinute(bool utc) const;
	int32_t getSecond(bool utc) const;
	int32_t getMillisecond(bool utc) const;
	asAtom msSinceEpoch(bool utc) const;
	void MakeDate(number_t year, number_t month, number_t day, number_t time, bool bIsLocalTime, bool clipped, int timezone=0);
	void MakeDate(number_t year, number_t month, number_t day, number_t hour, number_t minute, number_t second, number_t millisecond, bool bIsLocalTime, bool clipped, int timezoneoffset=0);
	number_t MakeTime(number_t hour, number_t minute, number_t second, number_t millisecond);
	~Date();
	static void _setFullYear(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc, bool fromsetter);
	static void _setMonth(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc);
	static void _setDate(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc);
	static void _setHours(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc);
	static void _setMinutes(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc);
	static void _setSeconds(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc);
	static void _setMilliseconds(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc);
public:
	Date(ASWorker* wrk,Class_base* c);
	bool destruct()
	{
		mstimestamp=numeric_limits<double>::quiet_NaN();
		mstimezoneoffset=0;
		nan = true;
		isinfinite = false;
		isinfiniteYear = false;
		return destructIntern();
	}

	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(UTC);
	ASFUNCTION_ATOM(getTimezoneOffset);
	ASFUNCTION_ATOM(getTime);
	ASFUNCTION_ATOM(getFullYear);
	ASFUNCTION_ATOM(getMonth);
	ASFUNCTION_ATOM(getDate);
	ASFUNCTION_ATOM(getDay);
	ASFUNCTION_ATOM(getHours);
	ASFUNCTION_ATOM(getMinutes);
	ASFUNCTION_ATOM(getSeconds);
	ASFUNCTION_ATOM(getMilliseconds);
	ASFUNCTION_ATOM(getUTCFullYear);
	ASFUNCTION_ATOM(getUTCMonth);
	ASFUNCTION_ATOM(getUTCDate);
	ASFUNCTION_ATOM(getUTCDay);
	ASFUNCTION_ATOM(getUTCHours);
	ASFUNCTION_ATOM(getUTCMinutes);
	ASFUNCTION_ATOM(getUTCSeconds);
	ASFUNCTION_ATOM(getUTCMilliseconds);
	ASFUNCTION_ATOM(valueOf);
	ASFUNCTION_ATOM(setFullYear);
	ASFUNCTION_ATOM(setMonth);
	ASFUNCTION_ATOM(setDate);
	ASFUNCTION_ATOM(setHours);
	ASFUNCTION_ATOM(setMinutes);
	ASFUNCTION_ATOM(setSeconds);
	ASFUNCTION_ATOM(setMilliseconds);
	ASFUNCTION_ATOM(setUTCFullYear);
	ASFUNCTION_ATOM(setUTCMonth);
	ASFUNCTION_ATOM(setUTCDate);
	ASFUNCTION_ATOM(setUTCHours);
	ASFUNCTION_ATOM(setUTCMinutes);
	ASFUNCTION_ATOM(setUTCSeconds);
	ASFUNCTION_ATOM(setUTCMilliseconds);
	ASFUNCTION_ATOM(setTime);
	ASFUNCTION_ATOM(fullYearSetter);
	ASFUNCTION_ATOM(monthSetter);
	ASFUNCTION_ATOM(dateSetter);
	ASFUNCTION_ATOM(hoursSetter);
	ASFUNCTION_ATOM(minutesSetter);
	ASFUNCTION_ATOM(secondsSetter);
	ASFUNCTION_ATOM(millisecondsSetter);
	ASFUNCTION_ATOM(UTCFullYearSetter);
	ASFUNCTION_ATOM(UTCMonthSetter);
	ASFUNCTION_ATOM(UTCDateSetter);
	ASFUNCTION_ATOM(UTCHoursSetter);
	ASFUNCTION_ATOM(UTCMinutesSetter);
	ASFUNCTION_ATOM(UTCSecondsSetter);
	ASFUNCTION_ATOM(UTCMillisecondsSetter);
	ASFUNCTION_ATOM(timeSetter);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(_parse);
	ASFUNCTION_ATOM(toUTCString);
	ASFUNCTION_ATOM(toDateString);
	ASFUNCTION_ATOM(toTimeString);
	ASFUNCTION_ATOM(toLocaleString);
	ASFUNCTION_ATOM(toLocaleDateString);
	ASFUNCTION_ATOM(toLocaleTimeString);

	void MakeDateFromMilliseconds(number_t ms, bool clipped=false);
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	TRISTATE isLessAtom(asAtom& r);
	
	tiny_string toFormat(bool utc, tiny_string format);
	tiny_string toString();
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk);
};
}
#endif /* SCRIPTING_TOPLEVEL_DATE_H */
