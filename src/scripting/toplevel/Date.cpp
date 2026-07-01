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

#include "parsing/amf3_generator.h"
#include "scripting/toplevel/Date.h"
#include "scripting/toplevel/Number.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/utils/ByteArray.h"
#include "compat.h"
#include <sys/time.h>

using namespace std;
using namespace lightspark;

#define MS_PER_DAY number_t(24*60*60*1000)
#define MS_PER_HOUR number_t(60*60*1000)
#define MS_PER_MINUTE number_t(60*1000)
#define MS_PER_SECOND number_t(1000)

Date::Date(ASWorker* wrk, Class_base* c)
	:ASObject(wrk,c,T_OBJECT,SUBTYPE_DATE)
	,mstimestamp(numeric_limits<double>::quiet_NaN())
	,mstimezoneoffset(0)
	,nan(true)
	,isinfinite(false)
	,isinfiniteYear(false)
{
}

Date::~Date()
{
}

void Date::sinit(Class_base* c)
{
	CLASS_SETUP_CONSTRUCTOR_7_PARAMETER(c, ASObject, _constructor, 7, CLASS_GETREF(c,Number), CLASS_GETREF(c,Number), CLASS_GETREF(c,Number), CLASS_GETREF(c,Number), CLASS_GETREF(c,Number), CLASS_GETREF(c,Number), CLASS_GETREF(c,Number), CLASS_FINAL);
	c->length=7;
	c->isReusable = true;
	c->setDeclaredMethodByQName("getTimezoneOffset",AS3,c->getSystemState()->getBuiltinFunction(getTimezoneOffset,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,c->getSystemState()->getBuiltinFunction(valueOf,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getTime",AS3,c->getSystemState()->getBuiltinFunction(getTime,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getFullYear",AS3,c->getSystemState()->getBuiltinFunction(getFullYear,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getMonth",AS3,c->getSystemState()->getBuiltinFunction(getMonth,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDate",AS3,c->getSystemState()->getBuiltinFunction(getDate,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDay",AS3,c->getSystemState()->getBuiltinFunction(getDay,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getHours",AS3,c->getSystemState()->getBuiltinFunction(getHours,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getMinutes",AS3,c->getSystemState()->getBuiltinFunction(getMinutes,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getSeconds",AS3,c->getSystemState()->getBuiltinFunction(getSeconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getMilliseconds",AS3,c->getSystemState()->getBuiltinFunction(getMilliseconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setFullYear",AS3,c->getSystemState()->getBuiltinFunction(setFullYear,3),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMonth",AS3,c->getSystemState()->getBuiltinFunction(setMonth,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setDate",AS3,c->getSystemState()->getBuiltinFunction(setDate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setHours",AS3,c->getSystemState()->getBuiltinFunction(setHours,4),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMinutes",AS3,c->getSystemState()->getBuiltinFunction(setMinutes,3),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setSeconds",AS3,c->getSystemState()->getBuiltinFunction(setSeconds,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMilliseconds",AS3,c->getSystemState()->getBuiltinFunction(setMilliseconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCFullYear",AS3,c->getSystemState()->getBuiltinFunction(getUTCFullYear,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMonth",AS3,c->getSystemState()->getBuiltinFunction(getUTCMonth,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCDate",AS3,c->getSystemState()->getBuiltinFunction(getUTCDate,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCDay",AS3,c->getSystemState()->getBuiltinFunction(getUTCDay,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCHours",AS3,c->getSystemState()->getBuiltinFunction(getUTCHours,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMinutes",AS3,c->getSystemState()->getBuiltinFunction(getUTCMinutes,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCSeconds",AS3,c->getSystemState()->getBuiltinFunction(getUTCSeconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMilliseconds",AS3,c->getSystemState()->getBuiltinFunction(getUTCMilliseconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCFullYear",AS3,c->getSystemState()->getBuiltinFunction(setUTCFullYear,3),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCMonth",AS3,c->getSystemState()->getBuiltinFunction(setUTCMonth,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCDate",AS3,c->getSystemState()->getBuiltinFunction(setUTCDate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCHours",AS3,c->getSystemState()->getBuiltinFunction(setUTCHours,4),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCMinutes",AS3,c->getSystemState()->getBuiltinFunction(setUTCMinutes,3),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCSeconds",AS3,c->getSystemState()->getBuiltinFunction(setUTCSeconds,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCMilliseconds",AS3,c->getSystemState()->getBuiltinFunction(setUTCMilliseconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTime",AS3,c->getSystemState()->getBuiltinFunction(setTime,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("UTC","",c->getSystemState()->getBuiltinFunction(UTC,7,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("toString",AS3,c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toUTCString",AS3,c->getSystemState()->getBuiltinFunction(toUTCString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toDateString",AS3,c->getSystemState()->getBuiltinFunction(toDateString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toTimeString",AS3,c->getSystemState()->getBuiltinFunction(toTimeString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString",AS3,c->getSystemState()->getBuiltinFunction(toLocaleString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleDateString",AS3,c->getSystemState()->getBuiltinFunction(toLocaleDateString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleTimeString",AS3,c->getSystemState()->getBuiltinFunction(toLocaleTimeString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parse","",c->getSystemState()->getBuiltinFunction(_parse,0,Class<Number>::getClassUninitialized(c->getSystemState())),NORMAL_METHOD,false);

	c->setDeclaredMethodByQName("date","",c->getSystemState()->getBuiltinFunction(getDate,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("date","",c->getSystemState()->getBuiltinFunction(dateSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("dateUTC","",c->getSystemState()->getBuiltinFunction(getUTCDate,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("dateUTC","",c->getSystemState()->getBuiltinFunction(UTCDateSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("day","",c->getSystemState()->getBuiltinFunction(getDay,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("dayUTC","",c->getSystemState()->getBuiltinFunction(getUTCDay,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullYear","",c->getSystemState()->getBuiltinFunction(getFullYear,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullYear","",c->getSystemState()->getBuiltinFunction(fullYearSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullYearUTC","",c->getSystemState()->getBuiltinFunction(getUTCFullYear,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullYearUTC","",c->getSystemState()->getBuiltinFunction(UTCFullYearSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("hours","",c->getSystemState()->getBuiltinFunction(getHours,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hours","",c->getSystemState()->getBuiltinFunction(hoursSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("hoursUTC","",c->getSystemState()->getBuiltinFunction(getUTCHours,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hoursUTC","",c->getSystemState()->getBuiltinFunction(UTCHoursSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("milliseconds","",c->getSystemState()->getBuiltinFunction(getMilliseconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("milliseconds","",c->getSystemState()->getBuiltinFunction(millisecondsSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("millisecondsUTC","",c->getSystemState()->getBuiltinFunction(getUTCMilliseconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("millisecondsUTC","",c->getSystemState()->getBuiltinFunction(UTCMillisecondsSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("minutes","",c->getSystemState()->getBuiltinFunction(getMinutes,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("minutes","",c->getSystemState()->getBuiltinFunction(minutesSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("minutesUTC","",c->getSystemState()->getBuiltinFunction(getUTCMinutes,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("minutesUTC","",c->getSystemState()->getBuiltinFunction(UTCMinutesSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("month","",c->getSystemState()->getBuiltinFunction(getMonth,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("month","",c->getSystemState()->getBuiltinFunction(monthSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("monthUTC","",c->getSystemState()->getBuiltinFunction(getUTCMonth,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("monthUTC","",c->getSystemState()->getBuiltinFunction(UTCMonthSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("seconds","",c->getSystemState()->getBuiltinFunction(getSeconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("seconds","",c->getSystemState()->getBuiltinFunction(secondsSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("secondsUTC","",c->getSystemState()->getBuiltinFunction(getUTCSeconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("secondsUTC","",c->getSystemState()->getBuiltinFunction(UTCSecondsSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("time","",c->getSystemState()->getBuiltinFunction(getTime,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("time","",c->getSystemState()->getBuiltinFunction(timeSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("timezoneOffset","",c->getSystemState()->getBuiltinFunction(getTimezoneOffset,0,Class<Number>::getClassUninitialized(c->getSystemState())),GETTER_METHOD,true);

	c->prototype->setVariableByQName("getTimezoneOffset","",c->getSystemState()->getBuiltinFunction(getTimezoneOffset,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("valueOf","",c->getSystemState()->getBuiltinFunction(valueOf,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getTime","",c->getSystemState()->getBuiltinFunction(getTime,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getFullYear","",c->getSystemState()->getBuiltinFunction(getFullYear,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getMonth","",c->getSystemState()->getBuiltinFunction(getMonth,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getDate","",c->getSystemState()->getBuiltinFunction(getDate,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getDay","",c->getSystemState()->getBuiltinFunction(getDay,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getHours","",c->getSystemState()->getBuiltinFunction(getHours,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getMinutes","",c->getSystemState()->getBuiltinFunction(getMinutes,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getSeconds","",c->getSystemState()->getBuiltinFunction(getSeconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getMilliseconds","",c->getSystemState()->getBuiltinFunction(getMilliseconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setFullYear","",c->getSystemState()->getBuiltinFunction(setFullYear,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setMonth","",c->getSystemState()->getBuiltinFunction(setMonth,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setDate","",c->getSystemState()->getBuiltinFunction(setDate),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setHours","",c->getSystemState()->getBuiltinFunction(setHours,4),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setMinutes","",c->getSystemState()->getBuiltinFunction(setMinutes,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setSeconds","",c->getSystemState()->getBuiltinFunction(setSeconds,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setMilliseconds","",c->getSystemState()->getBuiltinFunction(setMilliseconds),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCFullYear","",c->getSystemState()->getBuiltinFunction(getUTCFullYear,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCMonth","",c->getSystemState()->getBuiltinFunction(getUTCMonth,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCDate","",c->getSystemState()->getBuiltinFunction(getUTCDate,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCDay","",c->getSystemState()->getBuiltinFunction(getUTCDay,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCHours","",c->getSystemState()->getBuiltinFunction(getUTCHours,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCMinutes","",c->getSystemState()->getBuiltinFunction(getUTCMinutes,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCSeconds","",c->getSystemState()->getBuiltinFunction(getUTCSeconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCMilliseconds","",c->getSystemState()->getBuiltinFunction(getUTCMilliseconds,0,Class<Number>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCFullYear","",c->getSystemState()->getBuiltinFunction(setUTCFullYear,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCMonth","",c->getSystemState()->getBuiltinFunction(setUTCMonth,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCDate","",c->getSystemState()->getBuiltinFunction(setUTCDate),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCHours","",c->getSystemState()->getBuiltinFunction(setUTCHours,4),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCMinutes","",c->getSystemState()->getBuiltinFunction(setUTCMinutes,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCSeconds","",c->getSystemState()->getBuiltinFunction(setUTCSeconds,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCMilliseconds","",c->getSystemState()->getBuiltinFunction(setUTCMilliseconds),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setTime","",c->getSystemState()->getBuiltinFunction(setTime,1),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toUTCString","",c->getSystemState()->getBuiltinFunction(toUTCString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toDateString","",c->getSystemState()->getBuiltinFunction(toDateString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toTimeString","",c->getSystemState()->getBuiltinFunction(toTimeString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleTimeString","",c->getSystemState()->getBuiltinFunction(toLocaleTimeString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleDateString","",c->getSystemState()->getBuiltinFunction(toLocaleDateString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",c->getSystemState()->getBuiltinFunction(toLocaleString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toJSON",AS3,c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getClassUninitialized(c->getSystemState())),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Date,_constructor)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if (wrk->needsActionScript3())
	{
		for (uint32_t i = 0; i < argslen; i++) {
			if(asAtomHandler::isNumeric(args[i]) && !std::isfinite(asAtomHandler::toNumber(args[i]))) {
				th->nan = true;
				return;
			}
		}
	}
	if (argslen == 0) {
		int64_t ms = th->getcurrentms();
		th->MakeDateFromMilliseconds(ms);
	} else
	if (argslen == 1)
	{
		number_t nm = Number::NaN;
		if (asAtomHandler::isString(args[0]))
			nm = parse(asAtomHandler::toString(args[0],wrk));
		else
			nm = asAtomHandler::toNumber(args[0]);
		th->MakeDateFromMilliseconds(nm);
	} else
	{
		number_t year, month, day, hour, minute, second, millisecond;
		ARG_CHECK(ARG_UNPACK (year, 1970) (month, 0) (day, 1) (hour, 0) (minute, 0) (second, 0) (millisecond, 0));
		if (!std::isnan(year) && !std::isinf(year) && year < 100)
			year = 1900 + year;

		th->MakeDate(year, month, day, hour, minute,second,millisecond,!wrk->needsActionScript3() || argslen > 3,false);
	}
}
void Date::MakeDateFromMilliseconds(number_t ms, bool clipped)
{
	if (std::isnan(ms))
	{
		nan=true;
		return;
	}
	isinfinite = std::isinf(ms);
	nan = false;
	mstimestamp = ms;
	mstimezoneoffset = computeMsTimezoneOffset();
	if (clipped)
	{
		if (fabs(mstimestamp) > 100000000.0*MS_PER_DAY)
			this->nan=true;
		else
			mstimestamp = floor(mstimestamp);
	}
}

// helper methods as described in ECMA-262 chapter 15.9
bool isLeapYear(int32_t year)
{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

number_t dayFromYear(number_t year)
{
	return (365.0 * (year - 1970.0))
		   + floor((year - 1969.0) / 4.0)
		   - floor((year - 1901.0) / 100.0)
		   + floor((year - 1601.0) / 400.0);
}

int daysinyear[] ={ 0,31,59,90,120,151,181,212,243,273,304,334 };
int daysinleapyear[] ={ 0,31,60,91,121,152,182,213,244,274,305,335 };

void Date::MakeDate(number_t year, number_t month, number_t day, number_t time, bool bIsLocalTime, bool clipped, int timezone)
{
	this->nan = std::isnan(month) || std::isinf(month)
				|| std::isnan(day)
				|| std::isnan(time);
	if (this->nan)
		return;
	if (std::isnan(year) || std::isinf(year))
	{
		year = INT32_MIN;
		isinfiniteYear=true;
	}
	year = floor(year);
	month = floor(month);
	day = floor(day);

	year += floor(month / 12.0);
	month = fmod(month,12.0);


	number_t days = Number::NaN;
	if (month < 0)
		month += 12.0;
	if (year < INT32_MIN || year > INT32_MAX)
		year = INT32_MIN+1970;
	if (!std::isnan(month) && month >= 0 && month <= 11)
		days = dayFromYear(year)
			   + (isLeapYear(year) ? daysinleapyear[int(month)] : daysinyear[int(month)])
			   + day
			   - 1.0;
	mstimestamp = days * MS_PER_DAY + time;
	mstimezoneoffset = timezone ? -(timezone*60/100+timezone%100)*MS_PER_MINUTE : computeMsTimezoneOffset();
	if (bIsLocalTime)
		mstimestamp += mstimezoneoffset;
	if (clipped && fabs(mstimestamp) > 100000000.0*MS_PER_DAY)
		this->nan=true;
	this->isinfinite = std::isinf(mstimestamp);
}
void Date::MakeDate(number_t year, number_t month, number_t day, number_t hour, number_t minute, number_t second, number_t millisecond, bool bIsLocalTime, bool clipped, int timezoneoffset)
{
	this->nan = std::isnan(month) || std::isinf(month)
				|| std::isnan(day)
				|| std::isnan(hour)
				|| std::isnan(minute)
				|| std::isnan(second)
				|| std::isnan(millisecond)
				;
	if (this->nan)
		return;
	number_t time = MakeTime(hour,minute,second,millisecond);
	MakeDate(year,month,day,time,bIsLocalTime,clipped,timezoneoffset);
}

number_t Date::MakeTime(number_t hour, number_t minute, number_t second, number_t millisecond)
{
	hour = floor(hour);
	minute = floor(minute);
	second = floor(second);
	millisecond = floor(millisecond);

	return hour * MS_PER_HOUR
		   + minute * MS_PER_MINUTE
		   + second * MS_PER_SECOND
		   + millisecond;
}

ASFUNCTIONBODY_ATOM(Date,generator)
{
	Date* th=Class<Date>::getInstanceS(wrk);
	int64_t ms = th->getcurrentms();

	th->MakeDateFromMilliseconds(ms);

	ret = asAtomHandler::fromObjectNoPrimitive(th);
}

ASFUNCTIONBODY_ATOM(Date,UTC)
{
	number_t year, month, day, hour, minute, second, millisecond;
	ARG_CHECK(ARG_UNPACK (year) (month) (day, 1) (hour, 0) (minute, 0) (second, 0) (millisecond, 0));
	_R<Date> dt=_MR(Class<Date>::getInstanceS(wrk));
	if (!isnan(year) && !isinf(year) && year < 100)
		year += 1900;

	dt->MakeDate(year, month, day, hour, minute,second, millisecond,false,false);
	if(dt->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	ret = dt->msSinceEpoch(true);
}

ASFUNCTIONBODY_ATOM(Date,getTimezoneOffset)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	if(th->isinfinite) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	number_t res = (th->getMsSinceEpoch(true)-th->getMsSinceEpoch(false))/MS_PER_MINUTE;
	asAtomHandler::setNumber(ret,res);
}

ASFUNCTIONBODY_ATOM(Date,getUTCFullYear)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getYear(true));
}

ASFUNCTIONBODY_ATOM(Date,getUTCMonth)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getMonth(true));
}

ASFUNCTIONBODY_ATOM(Date,getUTCDate)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getDayInMonth(true));
}

ASFUNCTIONBODY_ATOM(Date,getUTCDay)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getDayInWeek(true));
}

ASFUNCTIONBODY_ATOM(Date,getUTCHours)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getHour(true));
}

ASFUNCTIONBODY_ATOM(Date,getUTCMinutes)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getMinute(true));
}

ASFUNCTIONBODY_ATOM(Date,getUTCSeconds)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getSecond(true));
}

ASFUNCTIONBODY_ATOM(Date,getUTCMilliseconds)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getMillisecond(true));
}

ASFUNCTIONBODY_ATOM(Date,getFullYear)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getYear(false));
}

ASFUNCTIONBODY_ATOM(Date,getMonth)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getMonth(false));
}

ASFUNCTIONBODY_ATOM(Date,getDate)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getDayInMonth(false));
}

ASFUNCTIONBODY_ATOM(Date,getDay)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getDayInWeek(false));
}

ASFUNCTIONBODY_ATOM(Date,getHours)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getHour(false));
}

ASFUNCTIONBODY_ATOM(Date,getMinutes)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getMinute(false));
}

ASFUNCTIONBODY_ATOM(Date,getSeconds)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getSecond(false));
}

ASFUNCTIONBODY_ATOM(Date,getMilliseconds)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,th->getMillisecond(false));
}

ASFUNCTIONBODY_ATOM(Date,getTime)
{
	if (!asAtomHandler::is<Date>(obj)) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	ret = th->msSinceEpoch(true);
}
ASFUNCTIONBODY_ATOM(Date,setFullYear)
{
	_setFullYear(ret,wrk,obj,args,argslen,false,false);
}
void Date::_setFullYear(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc, bool fromsetter)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if (argslen == 0)
	{
		asAtomHandler::setNumber(ret, Number::NaN);
		th->nan=true;
		return;
	}
	number_t y, m, d;
	ARG_CHECK(ARG_UNPACK (y) (m, 0) (d, 0));
	if (th->isinfinite || isnan(y) || isinf(y)) {
		asAtomHandler::setNumber(ret,Number::NaN);
		th->nan=true;
		return;
	}
	if (th->nan && fromsetter)
	{
		th->nan=false;
		th->mstimestamp= utc ? 0 : th->mstimezoneoffset;
	}

	if (argslen < 2)
		m = th->getMonth(utc);
	if (argslen < 3)
		d = th->getDayInMonth(utc);
	th->MakeDate(
		y
		,m
		,d
		,th->getTimeInMS(utc)
		,!utc
		,true);
	ret = th->msSinceEpoch(true);
}

ASFUNCTIONBODY_ATOM(Date,fullYearSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	_setFullYear(o,wrk,obj, args, min(argslen, (unsigned)1),false,true);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setMonth)
{
	_setMonth(ret, wrk, obj, args, argslen, false);
}
void Date::_setMonth(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t m, d;
	ARG_CHECK(ARG_UNPACK (m,0) (d, 0));

	if (isinf(m) || th->isinfinite || th->nan) {
		asAtomHandler::setNumber(ret,Number::NaN);
		th->nan=true;
		return;
	}
	if (isnan(m)) {
		m=0;
	}
	if (argslen < 2)
		d = th->getDayInMonth(utc);
	th->MakeDate(
		th->getYear(utc)
		,m
		,d
		,th->getTimeInMS(utc)
		,!utc
		,true);
	ret = th->msSinceEpoch(true);
}

ASFUNCTIONBODY_ATOM(Date,monthSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setMonth(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}
ASFUNCTIONBODY_ATOM(Date,setDate)
{
	_setDate(ret, wrk, obj, args, argslen, false);
}
void Date::_setDate(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t d;
	ARG_CHECK(ARG_UNPACK (d,Number::NaN));

	if (th->nan || th->isinfinite) {
		asAtomHandler::setNumber(ret, Number::NaN);
		th->nan=true;
		return;
	}
	th->MakeDate(
		th->getYear(utc)
		,th->getMonth(utc)
		,d
		,th->getTimeInMS(utc)
		,!utc
		,true);
	ret = th->msSinceEpoch(true);
}

ASFUNCTIONBODY_ATOM(Date,dateSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setDate(o,wrk,obj, args, argslen);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setHours)
{
	_setHours(ret, wrk, obj, args, argslen, false);
}
void Date::_setHours(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t h, min, sec, ms;
	ARG_CHECK(ARG_UNPACK (h,Number::NaN) (min, 0) (sec, 0) (ms, th->getMillisecond(utc)));

	if (th->nan || th->isinfinite) {
		asAtomHandler::setNumber(ret, Number::NaN);
		th->nan=true;
		return;
	}
	if (argslen < 2)
		min = th->getMinute(utc);
	if (argslen < 3)
		sec = th->getSecond(utc);
	th->MakeDate(
				th->getYear(utc)
				,th->getMonth(utc)
				,th->getDayInMonth(utc)
				,h
				,min
				,sec
				,ms
				,!utc
				,true);
	ret = th->msSinceEpoch(true);
}

ASFUNCTIONBODY_ATOM(Date,hoursSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setHours(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setMinutes)
{
	_setMinutes(ret, wrk, obj, args, argslen, false);
}
void Date::_setMinutes(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t min, sec, ms;
	if (wrk->needsActionScript3())
	{
		ARG_CHECK(ARG_UNPACK (min) (sec, 0) (ms, th->getMillisecond(utc)));
	}
	else
	{
		ARG_CHECK(ARG_UNPACK (min,Number::NaN) (sec, 0) (ms, th->getMillisecond(utc)));
	}
	if (th->nan || th->isinfinite) {
		asAtomHandler::setNumber(ret, Number::NaN);
		th->nan=true;
		return;
	}
	if (argslen < 2)
		sec = th->getSecond(false);
	if (isnan(min) || isinf(min) || min < INT32_MIN)
		min = INT32_MIN;
	if (min > INT32_MAX)
		min = INT32_MAX;
	if (isnan(sec) || isinf(sec) || sec < INT32_MIN)
		sec = INT32_MIN;
	if (sec > INT32_MAX)
		sec = INT32_MAX;
	if (isnan(ms) || isinf(ms) || ms < INT32_MIN)
		ms = INT32_MIN;
	if (ms > INT32_MAX)
		ms = INT32_MAX;
	th->MakeDate(
		th->getYear(utc)
		,th->getMonth(utc)
		,th->getDayInMonth(utc)
		,th->getHour(utc)
		,min
		,sec
		,ms
		,!utc
		,true);
	ret = th->msSinceEpoch(true);
}

ASFUNCTIONBODY_ATOM(Date,minutesSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setMinutes(o,wrk,obj, args, argslen);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setSeconds)
{
	_setSeconds(ret, wrk, obj, args, argslen, false);
}
void Date::_setSeconds(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc)
{
	Date* th=asAtomHandler::as<Date>(obj);

	number_t sec, ms;
	if (wrk->needsActionScript3())
	{
		ARG_CHECK(ARG_UNPACK (sec) (ms, th->getMillisecond(utc)));
	}
	else
	{
		ARG_CHECK(ARG_UNPACK (sec,Number::NaN) (ms, th->getMillisecond(utc)));
	}

	if (th->nan || th->isinfinite)
	{
		asAtomHandler::setNumber(ret, Number::NaN);
		th->nan=true;
		return;
	}
	th->MakeDate(
				th->getYear(utc)
				,th->getMonth(utc)
				,th->getDayInMonth(utc)
				,th->getHour(utc)
				,th->getMinute(utc)
				,sec
				,ms
				,!utc
				,true);
	ret = th->msSinceEpoch(true);
}

ASFUNCTIONBODY_ATOM(Date,secondsSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setSeconds(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setMilliseconds)
{
	_setMilliseconds(ret, wrk, obj, args, argslen, false);
}
void Date::_setMilliseconds(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen, bool utc)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t ms;
	if (wrk->needsActionScript3())
	{
		ARG_CHECK(ARG_UNPACK (ms));
	}
	else
	{
		ARG_CHECK(ARG_UNPACK (ms,Number::NaN));
	}

	if (th->nan || th->isinfinite) {
		asAtomHandler::setNumber(ret, Number::NaN);
		th->nan=true;
		return;
	}
	th->MakeDate(
				th->getYear(utc)
				,th->getMonth(utc)
				,th->getDayInMonth(utc)
				,th->getHour(utc)
				,th->getMinute(utc)
				,th->getSecond(utc)
				,ms
				,!utc
				,true);
	ret = th->msSinceEpoch(true);
}

ASFUNCTIONBODY_ATOM(Date,millisecondsSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setMilliseconds(o,wrk,obj, args, argslen);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCFullYear)
{
	_setFullYear(ret,wrk,obj,args,argslen,true,false);
}

ASFUNCTIONBODY_ATOM(Date,UTCFullYearSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	_setFullYear(o,wrk,obj, args, min(argslen, (unsigned)1),true,true);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCMonth)
{
	_setMonth(ret, wrk, obj, args, argslen, true);
}

ASFUNCTIONBODY_ATOM(Date,UTCMonthSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCMonth(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCDate)
{
	_setDate(ret, wrk, obj, args, argslen, true);
}

ASFUNCTIONBODY_ATOM(Date,UTCDateSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCDate(o,wrk,obj, args, argslen);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCHours)
{
	_setHours(ret, wrk, obj, args, argslen, true);
}

ASFUNCTIONBODY_ATOM(Date,UTCHoursSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCHours(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCMinutes)
{
	_setMinutes(ret, wrk, obj, args, argslen, true);
}

ASFUNCTIONBODY_ATOM(Date,UTCMinutesSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCMinutes(o,wrk,obj, args, argslen==0?0:1);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCSeconds)
{
	_setSeconds(ret, wrk, obj, args, argslen, true);
}

ASFUNCTIONBODY_ATOM(Date,UTCSecondsSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCSeconds(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCMilliseconds)
{
	_setMilliseconds(ret, wrk, obj, args, argslen, false);
}

ASFUNCTIONBODY_ATOM(Date,UTCMillisecondsSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCMilliseconds(o,wrk,obj, args, argslen);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setTime)
{
	number_t ms;
	ARG_CHECK(ARG_UNPACK (ms, Number::NaN));
	if (!asAtomHandler::is<Date>(obj))
	{
		multiname name(nullptr);
		name.name_type=multiname::NAME_STRING;
		name.name_s_id=wrk->getSystemState()->getUniqueStringId("value");
		name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
		name.isAttribute = true;
		asAtom v = asAtomHandler::fromNumber(ms);
		asAtomHandler::toObject(obj,wrk)->setVariableByMultiname(name,v,CONST_NOT_ALLOWED,nullptr,wrk);
		asAtomHandler::setNumber(ret, ms);
		return;
	}
	assert_and_throw(asAtomHandler::is<Date>(obj));
	Date* th=asAtomHandler::as<Date>(obj);

	if (std::isnan(ms))
	{
		th->nan = true;
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	th->MakeDateFromMilliseconds(ms,true);

	ret = th->msSinceEpoch(true);
}

ASFUNCTIONBODY_ATOM(Date,timeSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setTime(o,wrk,obj, args, argslen);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,valueOf)
{
	if (!asAtomHandler::is<Date>(obj))
	{
		asAtomHandler::setNumber(ret, Number::NaN);
		return;
	}
	Date* th=asAtomHandler::as<Date>(obj);
	if (th->nan || th->isinfinite)
		ret = asAtomHandler::fromNumber(Number::NaN);
	else
		ret = asAtomHandler::fromNumber(int64_t(th->getMsSinceEpoch(true)));
}

asAtom Date::msSinceEpoch(bool utc) const
{
	return this->nan ? getSystemState()->nanAtom : asAtomHandler::fromNumber(getMsSinceEpoch(utc));
}
number_t Date::getMsSinceEpoch(bool utc) const
{
	return utc ? mstimestamp : mstimestamp - mstimezoneoffset;
}
number_t Date::computeMsTimezoneOffset() const
{
	timeval tv;
	gettimeofday(&tv,nullptr);
	struct tm* tloc = localtime(&tv.tv_sec);
	tloc->tm_isdst=-1;
	auto tl = mktime(tloc);
	struct tm* tutc = gmtime(&tv.tv_sec);
	auto tu = mktime(tutc);
	return difftime(tl,tu)*1000.0;
}
number_t Date::getTimeInMS(bool utc) const
{
	number_t time = fmod(getMsSinceEpoch(utc),MS_PER_DAY);
	if (time < 0)
		time += MS_PER_DAY;
	return time;
}


tiny_string Date::toString()
{
	assert_and_throw(implEnable);
	return toString_priv(false, "%a %b %e %H:%M:%S GMT%z %Y");
}

tiny_string Date::toFormat(bool utc, tiny_string format)
{
	return toString_priv(utc,format.raw_buf());
}
const char* strweekdays[] ={ "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
const char* strmonths[] ={ "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };

tiny_string Date::toString_priv(bool utc, const char* formatstr) const
{
	if(nan || isinfinite || std::isnan(mstimestamp))
		return "Invalid Date";

	char buf[20];
	tiny_string res;
	tiny_string f(formatstr);
	for (auto it = f.begin(); it != f.end(); it++)
	{
		if (*it != '%')
		{
			res += *it;
			continue;
		}
		it++; // skip %
		switch (*it)
		{
			case 'a':
				res += strweekdays[getDayInWeek(utc)];
				break;
			case 'b':
				res += strmonths[getMonth(utc)];
				break;
			case 'e':
				snprintf(buf,19,"%i",getDayInMonth(utc));
				res += buf;
				break;
			case 'H':
				snprintf(buf,19,"%02i",getHour(utc));
				res += buf;
				break;
			case 'M':
				snprintf(buf,19,"%02i",getMinute(utc));
				res += buf;
				break;
			case 'S':
				snprintf(buf,19,"%02i",getSecond(utc));
				res += buf;
				break;
			case 'z':
			{
				int32_t tzminutes = -mstimezoneoffset/1000/60;
				snprintf(buf,19,"%c%02i%02i",tzminutes < 0 ? '-' :'+', abs(tzminutes)/60,abs(tzminutes)%60);
				res += buf;
				break;
			}
			case 'Y':
				snprintf(buf,19,"%i",getYear(utc));
				res += buf;
				break;
			default:
				LOG(LOG_NOT_IMPLEMENTED,"Date format %"<<(char)(*it));
				break;
		}
	}
	return res;
}

ASFUNCTIONBODY_ATOM(Date,_toString)
{
	if (!asAtomHandler::is<Date>(obj))
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"Invalid Date");
		return;
	}
	Date* th=asAtomHandler::as<Date>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString()));
}

ASFUNCTIONBODY_ATOM(Date,toUTCString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv(true,"%a %b %e %H:%M:%S UTC %Y")));
}

ASFUNCTIONBODY_ATOM(Date,toDateString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv(false,"%a %b %e %Y")));
}

ASFUNCTIONBODY_ATOM(Date,toTimeString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toFormat(false, "%H:%M:%S GMT%z")));
}


ASFUNCTIONBODY_ATOM(Date,toLocaleString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if (!th->isValid())
	{
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	tiny_string res = th->toString_priv(false,"%a %b %e");
	tiny_string fs = th->toFormat(false," %I:%M:%S");
	res += fs;
	auto t = th->getDateTimeLocal();
	if (t->tm_hour > 12)
		res += " PM";
	else
		res += " AM";
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}
ASFUNCTIONBODY_ATOM(Date,toLocaleDateString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv(false,"%a %b %e")));
}
ASFUNCTIONBODY_ATOM(Date,toLocaleTimeString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toFormat(false,"%H:%M:%S %Z%z")));
}

ASFUNCTIONBODY_ATOM(Date,_parse)
{
	asAtomHandler::setNumber(ret, parse(asAtomHandler::toString(args[0],wrk)));
}

number_t Date::parse(tiny_string str)
{
	int d=0,h=0,m=0,s=0,y=0,mon=-1,tz=0;
	char buf[10];
	bool isPM = false;
	bool hasYear = false;
	bool hasTimezone = false;
	bool bvalid = false;
	number_t res = Number::NaN;

	std::list<tiny_string> tokens = str.split(' ');
	for (auto it = tokens.begin(); it != tokens.end(); it++)
	{
		if (it->empty())
			continue;
		bool tokenvalid=false;
		int tmp1,tmp2,tmp3;
		// MM/DD/YYYY
		// YYYY/MM/DD
		if (sscanf(it->raw_buf(), "%4d/%4d/%4d",&tmp1,&tmp2, &tmp3) == 3)
		{
			tokenvalid = true;
			hasYear=true;
			if (tmp1 < 32)
			{
				y=tmp3;
				mon=tmp1;
				d=tmp2;
			}
			else
			{
				y=tmp1;
				mon=tmp2;
				d=tmp3;
			}
		}
		if (!tokenvalid)
		{
			// MMM/DD/YYYY
			if (!hasYear && sscanf(it->raw_buf(), "%3s/%2d/%4d", buf,&tmp2,&tmp3)==3)
			{
				for (int i = 0; i < 12; i++)
				{
					if (strcmp(buf,strmonths[i])==0)
					{
						tokenvalid=true;
						d=tmp2;
						y=tmp3;
						mon = i+1;
						hasYear=true;
						break;
					}
				}
			}
		}
		if (!tokenvalid)
		{
			// HH:MM:SS
			if (sscanf(it->raw_buf(), "%2d:%2d:%2d", &tmp1,&tmp2,&tmp3)==3)
			{
				h=tmp1;
				m=tmp2;
				s=tmp3;
				tokenvalid = true;
			}
		}
		if (!tokenvalid)
		{
			// Day
			for (int i = 0; i < 7; i++)
			{
				if (*it == strweekdays[i])
				{
					tokenvalid=true;
					break;
				}
			}
		}
		if (!tokenvalid)
		{
			// Mon
			for (int i = 0; i < 12; i++)
			{
				if (*it == strmonths[i])
				{
					tokenvalid=true;
					mon = i+1;
					break;
				}
			}
		}
		if (!tokenvalid)
		{
			// TZD
			if (!hasTimezone && (it->startsWith("GMT") || it->startsWith("UTC")))
			{
				if (it->numBytes()>3)
					tokenvalid = sscanf(it->raw_buf()+3, "%d",&tz);
				else
					tokenvalid=true;
				hasTimezone=true;
			}
		}
		if (!tokenvalid)
		{
			// DD or YYYY
			int tmp=0;
			if (sscanf(it->raw_buf(), "%d",&tmp)==1 && tmp >=0)
			{
				if (tmp >= 70)
				{
					if (!hasYear)
					{
						y = tmp;
						hasYear=true;
						tokenvalid = true;
					}
				}
				else
				{
					d=tmp;
					tokenvalid = true;
				}
			}
		}
		if (!tokenvalid)
		{
			// AM
			if (it->uppercase() == "AM")
				tokenvalid=true;
		}
		if (!tokenvalid)
		{
			// PM
			if (it->uppercase() == "PM")
			{
				tokenvalid=true;
				isPM=true;
			}
		}
		bvalid = tokenvalid;
		if (!tokenvalid)
			break;
	}

	if (bvalid && hasYear && mon >0)
	{
		if (isPM)
			h += 12;
		if (y >=0 && y <= 99)
			y += 1900;
		Date* dt=Class<Date>::getInstanceS(getWorker());
		dt->MakeDate(y, mon-1, d, h,m, s, 0,true,false,tz);
		res =dt->nan ? Number::NaN : dt->mstimestamp;
		dt->decRef();
	}
	return res;
}
bool Date::isEqual(ASObject* r)
{
	//if we are comparing the same object the answer is true
	if(this==r)
		return true;
	if (r->is<Date>())
		return getMsSinceEpoch(true) == r->as<Date>()->getMsSinceEpoch(true);
	return ASObject::isEqual(r);
}

TRISTATE Date::isLess(ASObject* r)
{
	if (r->is<Date>())
		return (getMsSinceEpoch(true) < r->as<Date>()->getMsSinceEpoch(true))?TTRUE:TFALSE;
	return ASObject::isLess(r);
}
TRISTATE Date::isLessAtom(asAtom& r)
{
	if (asAtomHandler::is<Date>(r))
		return (getMsSinceEpoch(true) < asAtomHandler::as<Date>(r)->getMsSinceEpoch(true))?TTRUE:TFALSE;
	return ASObject::isLessAtom(r);
}

void Date::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing Date in AMF0 not implemented");
		return;
	}
	number_t val = getMsSinceEpoch(true);
	out->writeByte(date_marker);
	// write milliseconds since 1970 as double
	out->serializeDouble(val);
}

int64_t Date::getcurrentms()
{
	int64_t ms=0;
	if (getSystemState()->use_testrunner_date)
	{
		// ruffle tests expect a specific date in specific timezone as "current date"
		// 2001-02-03 04:05:06 GMT+05:45
#ifdef _WIN32
		_putenv_s("TZ", "GMT+5:45");
#else
		setenv("TZ", "GMT+5:45", 1);
#endif
		tzset();
		MakeDate(2001,2-1,3,4,5,6,0,true,false);
		ms = mstimestamp;
	}
	else
	{
		struct timeval tv;
		gettimeofday(&tv,nullptr);
		ms = (tv.tv_sec*1000)+tv.tv_usec/1000;
	}
	return ms;
}

struct tm* Date::getDateTimeUTC() const
{
	time_t t = floor(this->mstimestamp/1000.0);
	return gmtime(&t);
}

struct tm* Date::getDateTimeLocal() const
{
	time_t t = floor(this->mstimestamp/1000.0);
	return localtime(&t);
}

int32_t Date::getYear(bool utc) const
{
	if (isinfinite || isinfiniteYear)
		return INT32_MIN+1970;
	number_t ms = getMsSinceEpoch(utc);
	number_t daysin400years = (400 * 365) + 100 + 1 - 4; // 100 leap years + 1 leap year at 400 - 4 non-leap years every 100 years
	number_t day = floor(ms / MS_PER_DAY) - dayFromYear(0);
	number_t year = day * 400.0 / daysin400years;
	return int32_t(floor(year));
}

// getters based on algorithms described in ECMA-262 chapter 15.9
int32_t Date::getMonth(bool utc) const
{
	number_t y = floor(getYear(utc));
	number_t d = floor(getMsSinceEpoch(utc)/MS_PER_DAY) - dayFromYear(y);
	if (isinf(d) || d<0)
		return 0;
	bool isleapyear = isLeapYear(y);
	int i=0;
	while (i < 12 && d >= (isleapyear ? daysinleapyear[i] : daysinyear[i]))
		i++;
	return i-1;
}
int32_t Date::getDayInMonth(bool utc) const
{
	number_t y = floor(getYear(utc));
	number_t d = floor(getMsSinceEpoch(utc)/MS_PER_DAY) - dayFromYear(y);
	bool isleapyear = isLeapYear(y);
	int i=1;
	while (i < 12 && d >= (isleapyear ? daysinleapyear[i] : daysinyear[i]))
		i++;
	return int32_t(d- (isleapyear ? daysinleapyear[i-1] : daysinyear[i-1])) +1;
}
int32_t Date::getDayInWeek(bool utc) const
{
	number_t d =floor(getMsSinceEpoch(utc)/MS_PER_DAY);
	number_t res = fmod(d+4.0,7);
	if (res < INT32_MIN || res > INT32_MAX || std::isnan(res))
		res = INT32_MIN;
	return res < 0 ? res + 7 : res;
}
int32_t Date::getHour(bool utc) const
{
	number_t h =floor((getMsSinceEpoch(utc)+.5)/MS_PER_HOUR);
	number_t res = fmod(h,24);
	if (res < INT32_MIN || res > INT32_MAX || std::isnan(res))
		res = INT32_MIN;
	return res < 0 ? res + 24 : res;
}
int32_t Date::getMinute(bool utc) const
{
	number_t m =floor(getMsSinceEpoch(utc)/MS_PER_MINUTE);
	number_t res = fmod(m,60);
	if (res < INT32_MIN || res > INT32_MAX || std::isnan(res))
		res = INT32_MIN;
	return res < 0 ? res + 60 : res;
}
int32_t Date::getSecond(bool utc) const
{
	number_t s =floor(getMsSinceEpoch(utc)/MS_PER_SECOND);
	number_t res = fmod(s,60);
	if (res < INT32_MIN || res > INT32_MAX || std::isnan(res))
		res = INT32_MIN;
	return res < 0 ? res + 60 : res;
}
int32_t Date::getMillisecond(bool utc) const
{
	number_t ms = getMsSinceEpoch(utc);
	number_t res = fmod(ms,1000);
	if (res < INT32_MIN || res > INT32_MAX || std::isnan(res))
		res = INT32_MIN;
	return res < 0 ? res + 1000 : res;
}
