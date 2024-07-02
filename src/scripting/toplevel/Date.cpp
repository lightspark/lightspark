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

using namespace std;
using namespace lightspark;

Date::Date(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_DATE),extrayears(0), nan(false), datetime(nullptr),datetimeUTC(nullptr)
{
}

Date::~Date()
{
}

void Date::sinit(Class_base* c)
{
	CLASS_SETUP_CONSTRUCTOR_LENGTH(c, ASObject, _constructor, 7, CLASS_FINAL);
	c->isReusable = true;
	c->setDeclaredMethodByQName("getTimezoneOffset",AS3,c->getSystemState()->getBuiltinFunction(getTimezoneOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,c->getSystemState()->getBuiltinFunction(valueOf,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getTime",AS3,c->getSystemState()->getBuiltinFunction(getTime,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getFullYear",AS3,c->getSystemState()->getBuiltinFunction(getFullYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getMonth",AS3,c->getSystemState()->getBuiltinFunction(getMonth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDate",AS3,c->getSystemState()->getBuiltinFunction(getDate,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDay",AS3,c->getSystemState()->getBuiltinFunction(getDay,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getHours",AS3,c->getSystemState()->getBuiltinFunction(getHours,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getMinutes",AS3,c->getSystemState()->getBuiltinFunction(getMinutes,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getSeconds",AS3,c->getSystemState()->getBuiltinFunction(getSeconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getMilliseconds",AS3,c->getSystemState()->getBuiltinFunction(getMilliseconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setFullYear",AS3,c->getSystemState()->getBuiltinFunction(setFullYear,3),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMonth",AS3,c->getSystemState()->getBuiltinFunction(setMonth,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setDate",AS3,c->getSystemState()->getBuiltinFunction(setDate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setHours",AS3,c->getSystemState()->getBuiltinFunction(setHours,4),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMinutes",AS3,c->getSystemState()->getBuiltinFunction(setMinutes,3),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setSeconds",AS3,c->getSystemState()->getBuiltinFunction(setSeconds,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMilliseconds",AS3,c->getSystemState()->getBuiltinFunction(setMilliseconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCFullYear",AS3,c->getSystemState()->getBuiltinFunction(getUTCFullYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMonth",AS3,c->getSystemState()->getBuiltinFunction(getUTCMonth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCDate",AS3,c->getSystemState()->getBuiltinFunction(getUTCDate,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCDay",AS3,c->getSystemState()->getBuiltinFunction(getUTCDay,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCHours",AS3,c->getSystemState()->getBuiltinFunction(getUTCHours,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMinutes",AS3,c->getSystemState()->getBuiltinFunction(getUTCMinutes,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCSeconds",AS3,c->getSystemState()->getBuiltinFunction(getUTCSeconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMilliseconds",AS3,c->getSystemState()->getBuiltinFunction(getUTCMilliseconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCFullYear",AS3,c->getSystemState()->getBuiltinFunction(setUTCFullYear,3),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCMonth",AS3,c->getSystemState()->getBuiltinFunction(setUTCMonth,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCDate",AS3,c->getSystemState()->getBuiltinFunction(setUTCDate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCHours",AS3,c->getSystemState()->getBuiltinFunction(setUTCHours,4),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCMinutes",AS3,c->getSystemState()->getBuiltinFunction(setUTCMinutes,3),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCSeconds",AS3,c->getSystemState()->getBuiltinFunction(setUTCSeconds,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCMilliseconds",AS3,c->getSystemState()->getBuiltinFunction(setUTCMilliseconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setTime",AS3,c->getSystemState()->getBuiltinFunction(setTime,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("UTC","",c->getSystemState()->getBuiltinFunction(UTC,7,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("toString",AS3,c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toUTCString",AS3,c->getSystemState()->getBuiltinFunction(toUTCString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toDateString",AS3,c->getSystemState()->getBuiltinFunction(toDateString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toTimeString",AS3,c->getSystemState()->getBuiltinFunction(toTimeString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString",AS3,c->getSystemState()->getBuiltinFunction(toLocaleString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleDateString",AS3,c->getSystemState()->getBuiltinFunction(toLocaleDateString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleTimeString",AS3,c->getSystemState()->getBuiltinFunction(toLocaleTimeString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parse","",c->getSystemState()->getBuiltinFunction(_parse,0,Class<Number>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,false);

	c->setDeclaredMethodByQName("date","",c->getSystemState()->getBuiltinFunction(getDate,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("date","",c->getSystemState()->getBuiltinFunction(dateSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("dateUTC","",c->getSystemState()->getBuiltinFunction(getUTCDate,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("dateUTC","",c->getSystemState()->getBuiltinFunction(UTCDateSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("day","",c->getSystemState()->getBuiltinFunction(getDay,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("dayUTC","",c->getSystemState()->getBuiltinFunction(getUTCDay,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullYear","",c->getSystemState()->getBuiltinFunction(getFullYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullYear","",c->getSystemState()->getBuiltinFunction(fullYearSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullYearUTC","",c->getSystemState()->getBuiltinFunction(getUTCFullYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("fullYearUTC","",c->getSystemState()->getBuiltinFunction(UTCFullYearSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("hours","",c->getSystemState()->getBuiltinFunction(getHours,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hours","",c->getSystemState()->getBuiltinFunction(hoursSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("hoursUTC","",c->getSystemState()->getBuiltinFunction(getUTCHours,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("hoursUTC","",c->getSystemState()->getBuiltinFunction(UTCHoursSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("milliseconds","",c->getSystemState()->getBuiltinFunction(getMilliseconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("milliseconds","",c->getSystemState()->getBuiltinFunction(millisecondsSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("millisecondsUTC","",c->getSystemState()->getBuiltinFunction(getUTCMilliseconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("millisecondsUTC","",c->getSystemState()->getBuiltinFunction(UTCMillisecondsSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("minutes","",c->getSystemState()->getBuiltinFunction(getMinutes,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("minutes","",c->getSystemState()->getBuiltinFunction(minutesSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("minutesUTC","",c->getSystemState()->getBuiltinFunction(getUTCMinutes,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("minutesUTC","",c->getSystemState()->getBuiltinFunction(UTCMinutesSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("month","",c->getSystemState()->getBuiltinFunction(getMonth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("month","",c->getSystemState()->getBuiltinFunction(monthSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("monthUTC","",c->getSystemState()->getBuiltinFunction(getUTCMonth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("monthUTC","",c->getSystemState()->getBuiltinFunction(UTCMonthSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("seconds","",c->getSystemState()->getBuiltinFunction(getSeconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("seconds","",c->getSystemState()->getBuiltinFunction(secondsSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("secondsUTC","",c->getSystemState()->getBuiltinFunction(getUTCSeconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("secondsUTC","",c->getSystemState()->getBuiltinFunction(UTCSecondsSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("time","",c->getSystemState()->getBuiltinFunction(getTime,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("time","",c->getSystemState()->getBuiltinFunction(timeSetter),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("timezoneOffset","",c->getSystemState()->getBuiltinFunction(getTimezoneOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);

	c->prototype->setVariableByQName("getTimezoneOffset","",c->getSystemState()->getBuiltinFunction(getTimezoneOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("valueOf","",c->getSystemState()->getBuiltinFunction(valueOf,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getTime","",c->getSystemState()->getBuiltinFunction(getTime,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getFullYear","",c->getSystemState()->getBuiltinFunction(getFullYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getMonth","",c->getSystemState()->getBuiltinFunction(getMonth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getDate","",c->getSystemState()->getBuiltinFunction(getDate,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getDay","",c->getSystemState()->getBuiltinFunction(getDay,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getHours","",c->getSystemState()->getBuiltinFunction(getHours,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getMinutes","",c->getSystemState()->getBuiltinFunction(getMinutes,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getSeconds","",c->getSystemState()->getBuiltinFunction(getSeconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getMilliseconds","",c->getSystemState()->getBuiltinFunction(getMilliseconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setFullYear","",c->getSystemState()->getBuiltinFunction(setFullYear,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setMonth","",c->getSystemState()->getBuiltinFunction(setMonth,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setDate","",c->getSystemState()->getBuiltinFunction(setDate),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setHours","",c->getSystemState()->getBuiltinFunction(setHours,4),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setMinutes","",c->getSystemState()->getBuiltinFunction(setMinutes,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setSeconds","",c->getSystemState()->getBuiltinFunction(setSeconds,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setMilliseconds","",c->getSystemState()->getBuiltinFunction(setMilliseconds),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCFullYear","",c->getSystemState()->getBuiltinFunction(getUTCFullYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCMonth","",c->getSystemState()->getBuiltinFunction(getUTCMonth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCDate","",c->getSystemState()->getBuiltinFunction(getUTCDate,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCDay","",c->getSystemState()->getBuiltinFunction(getUTCDay,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCHours","",c->getSystemState()->getBuiltinFunction(getUTCHours,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCMinutes","",c->getSystemState()->getBuiltinFunction(getUTCMinutes,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCSeconds","",c->getSystemState()->getBuiltinFunction(getUTCSeconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCMilliseconds","",c->getSystemState()->getBuiltinFunction(getUTCMilliseconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCFullYear","",c->getSystemState()->getBuiltinFunction(setUTCFullYear,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCMonth","",c->getSystemState()->getBuiltinFunction(setUTCMonth,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCDate","",c->getSystemState()->getBuiltinFunction(setUTCDate),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCHours","",c->getSystemState()->getBuiltinFunction(setUTCHours,4),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCMinutes","",c->getSystemState()->getBuiltinFunction(setUTCMinutes,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCSeconds","",c->getSystemState()->getBuiltinFunction(setUTCSeconds,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCMilliseconds","",c->getSystemState()->getBuiltinFunction(setUTCMilliseconds),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setTime","",c->getSystemState()->getBuiltinFunction(setTime,1),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toUTCString","",c->getSystemState()->getBuiltinFunction(toUTCString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toDateString","",c->getSystemState()->getBuiltinFunction(toDateString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toTimeString","",c->getSystemState()->getBuiltinFunction(toTimeString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleTimeString","",c->getSystemState()->getBuiltinFunction(toLocaleTimeString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleDateString","",c->getSystemState()->getBuiltinFunction(toLocaleDateString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",c->getSystemState()->getBuiltinFunction(toLocaleString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toJSON",AS3,c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}

const gint64 MS_IN_400_YEARS = 1.26227808e+13;

ASFUNCTIONBODY_ATOM(Date,_constructor)
{
	Date* th=asAtomHandler::as<Date>(obj);
	for (uint32_t i = 0; i < argslen; i++) {
		if(asAtomHandler::isNumeric(args[i]) && std::isnan(asAtomHandler::toNumber(args[i]))) {
			th->nan = true;
			return;
		}
	}
	if (argslen == 0) {
		GDateTime* tmp = g_date_time_new_now_utc();
		int64_t ms = g_date_time_to_unix(tmp)*1000 + g_date_time_get_microsecond (tmp)/1000;
		g_date_time_unref(tmp);
		th->MakeDateFromMilliseconds(ms);
	} else
	if (argslen == 1)
	{
		number_t nm = Number::NaN;
		if (asAtomHandler::isString(args[0]))
			nm = parse(asAtomHandler::toString(args[0],wrk));
		else
			nm = asAtomHandler::toNumber(args[0]);
		if (std::isnan(nm) || std::isinf(nm))
			th->nan = true;
		else
			th->MakeDateFromMilliseconds(int64_t(nm));
	} else
	{
		number_t year, month, day, hour, minute, second, millisecond;
		ARG_CHECK(ARG_UNPACK (year, 1970) (month, 0) (day, 1) (hour, 0) (minute, 0) (second, 0) (millisecond, 0));
		if (fabs(year) < 100 ) year = 1900 + year;
		th->MakeDate(year, month+1, day, hour, minute,second, millisecond,argslen > 3);
	}
}
void Date::MakeDateFromMilliseconds(int64_t ms)
{
	//GLib's GDateTime sensibly does not support and store very large year numbers
	//so keep the extra years in units of 400 years separately. A 400 years period has
	//a fixed number of milliseconds, unaffected by leap year calculations.	extrayears = year;
	if ( ms > 0) 
	{
		extrayears = 400*(ms/MS_IN_400_YEARS);
		ms %= MS_IN_400_YEARS;
	}
	else
	{
		extrayears = 0;
		while (ms < 0)
		{
			extrayears -=400;
			ms += MS_IN_400_YEARS;
		}
	}
	if (datetimeUTC)
		g_date_time_unref(datetimeUTC);
	if (datetime)
		g_date_time_unref(datetime);
	this->milliseconds = ms;
	datetimeUTC = g_date_time_new_from_unix_utc(ms/1000);
	datetime = g_date_time_to_local(datetimeUTC);
}

int daysinyear[] ={ 0,31,59,90,120,151,181,212,243,273,304,334 };
void Date::MakeDate(int64_t year, int64_t month, int64_t day, int64_t hour, int64_t minute, int64_t second, int64_t millisecond, bool bIsLocalTime)
{
	if (std::isnan(year) || std::isnan(month) || std::isnan(day) || std::isnan(hour) || std::isnan(minute) || std::isnan(second) || std::isnan(millisecond))
	{
		this->nan = true;
		return;
	}
	this->nan = false;

	//GLib's GDateTime seems to have problems if the parameters are big values (e.g. second = 1234567)
	//so we calculate the milliseconds by using the algorithm from ECMAScript documentation
	extrayears = year;
	year = 2000+year%400;
	extrayears = extrayears-year;
	int64_t y = year +(month-1)/12;
	int64_t m = (month-1)%12;

	int64_t d = (day-1) + (y-1970)*365 + (y-1969)/4 - (y-1901)/100 +(y-1601)/400+ daysinyear[m];
	if (m > 1 && (y % 4 == 0) && ((y%100 != 0) || (y%400 == 0))) // we are in a leap year after february
		d++;
	int64_t ms = d * 86400000 + hour * 3600000 + minute * 60000 + second * 1000 + millisecond;
	if (datetimeUTC)
		g_date_time_unref(datetimeUTC);
	if (datetime)
		g_date_time_unref(datetime);
	if (bIsLocalTime)
	{
		GDateTime* tmp = g_date_time_new_from_unix_local(ms/1000);
		GTimeSpan sp = g_date_time_get_utc_offset(tmp)/1000;
		ms = ms - sp;
		g_date_time_unref(tmp);
	}
	this->milliseconds = ms;
	datetimeUTC = g_date_time_new_from_unix_utc(ms/1000);
	datetime = g_date_time_to_local(datetimeUTC);
}

ASFUNCTIONBODY_ATOM(Date,generator)
{
	Date* th=Class<Date>::getInstanceS(wrk);
	GDateTime* tmp = g_date_time_new_now_utc();
	th->MakeDateFromMilliseconds(g_date_time_to_unix(tmp)*1000 + g_date_time_get_microsecond (tmp)/1000);
	g_date_time_unref(tmp);

	ret = asAtomHandler::fromObjectNoPrimitive(th);
}

ASFUNCTIONBODY_ATOM(Date,UTC)
{
	for (uint32_t i = 0; i < argslen; i++) {
		if(asAtomHandler::isNumeric(args[i]) && std::isnan(asAtomHandler::toNumber(args[i]))) {
			asAtomHandler::setNumber(ret,wrk,Number::NaN);
			return;
		}
	}
	number_t year, month, day, hour, minute, second, millisecond;
	ARG_CHECK(ARG_UNPACK (year) (month) (day, 1) (hour, 0) (minute, 0) (second, 0) (millisecond, 0));
	_R<Date> dt=_MR(Class<Date>::getInstanceS(wrk));
	dt->MakeDate(year, month+1, day, hour, minute,second, millisecond,false);
	if(dt->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	ret = dt->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,getTimezoneOffset)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	GTimeSpan diff = g_date_time_get_utc_offset(th->datetime);
	asAtomHandler::setInt(ret,wrk,(int32_t)(-diff/G_TIME_SPAN_MINUTE));
}

ASFUNCTIONBODY_ATOM(Date,getUTCFullYear)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,th->extrayears + g_date_time_get_year(th->datetimeUTC));
}

ASFUNCTIONBODY_ATOM(Date,getUTCMonth)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_month(th->datetimeUTC)-1);
}

ASFUNCTIONBODY_ATOM(Date,getUTCDate)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_day_of_month(th->datetimeUTC));
}

ASFUNCTIONBODY_ATOM(Date,getUTCDay)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_day_of_week(th->datetimeUTC)%7);
}

ASFUNCTIONBODY_ATOM(Date,getUTCHours)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_hour(th->datetimeUTC));
}

ASFUNCTIONBODY_ATOM(Date,getUTCMinutes)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_minute(th->datetimeUTC));
}

ASFUNCTIONBODY_ATOM(Date,getUTCSeconds)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_second(th->datetimeUTC));
}

ASFUNCTIONBODY_ATOM(Date,getUTCMilliseconds)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,(int32_t)(th->milliseconds % 1000));
}

ASFUNCTIONBODY_ATOM(Date,getFullYear)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,th->extrayears + g_date_time_get_year(th->datetime));
}

ASFUNCTIONBODY_ATOM(Date,getMonth)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_month(th->datetime)-1);
}

ASFUNCTIONBODY_ATOM(Date,getDate)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_day_of_month(th->datetime));
}

ASFUNCTIONBODY_ATOM(Date,getDay)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_day_of_week(th->datetime)%7);
}

ASFUNCTIONBODY_ATOM(Date,getHours)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_hour(th->datetime));
}

ASFUNCTIONBODY_ATOM(Date,getMinutes)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_minute(th->datetime));
}

ASFUNCTIONBODY_ATOM(Date,getSeconds)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,g_date_time_get_second(th->datetime));
}

ASFUNCTIONBODY_ATOM(Date,getMilliseconds)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	asAtomHandler::setInt(ret,wrk,(int32_t)(th->milliseconds % 1000));
}

ASFUNCTIONBODY_ATOM(Date,getTime)
{
	if (!asAtomHandler::is<Date>(obj)) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,setFullYear)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if (argslen == 0)
	{
		th->nan = true;
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	number_t y, m, d;
	ARG_CHECK(ARG_UNPACK (y) (m, 0) (d, 0));

	if (argslen > 1)
		m++;
	else
		m = g_date_time_get_month(th->datetime);
	if (argslen < 3)
		d = g_date_time_get_day_of_month(th->datetime);
	th->MakeDate(y, m, d, g_date_time_get_hour(th->datetime),g_date_time_get_minute(th->datetime),g_date_time_get_second(th->datetime),th->milliseconds % 1000,true);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,fullYearSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setFullYear(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setMonth)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t m, d;
	ARG_CHECK(ARG_UNPACK (m) (d, 0));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	if (argslen < 2)
		d = g_date_time_get_day_of_month(th->datetime);
	th->MakeDate(g_date_time_get_year(th->datetime)+th->extrayears, m+1, d, g_date_time_get_hour(th->datetime),g_date_time_get_minute(th->datetime),g_date_time_get_second(th->datetime),th->milliseconds % 1000,true);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,monthSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setMonth(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setDate)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t d;
	ARG_CHECK(ARG_UNPACK (d));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	th->MakeDate(g_date_time_get_year(th->datetime)+th->extrayears, g_date_time_get_month(th->datetime), d, g_date_time_get_hour(th->datetime),g_date_time_get_minute(th->datetime),g_date_time_get_second(th->datetime),th->milliseconds % 1000,true);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,dateSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setDate(o,wrk,obj, args, argslen);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setHours)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t h, min, sec, ms;
	ARG_CHECK(ARG_UNPACK (h) (min, 0) (sec, 0) (ms, 0));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	if (!min) min = g_date_time_get_minute(th->datetime);
	if (!sec) sec = g_date_time_get_second(th->datetime);
	if (!ms) ms = th->milliseconds % 1000;
	th->MakeDate(g_date_time_get_year(th->datetime)+th->extrayears, g_date_time_get_month(th->datetime), g_date_time_get_day_of_month(th->datetime), h, min, sec, ms,true);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,hoursSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setHours(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setMinutes)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t min, sec, ms;
	ARG_CHECK(ARG_UNPACK (min) (sec, 0) (ms, 0));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	if (!sec) sec = g_date_time_get_second(th->datetime);
	if (!ms) ms = th->milliseconds % 1000;
	th->MakeDate(g_date_time_get_year(th->datetime)+th->extrayears, g_date_time_get_month(th->datetime), g_date_time_get_day_of_month(th->datetime),  g_date_time_get_hour(th->datetime), min, sec, ms,true);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,minutesSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setMinutes(o,wrk,obj, args, argslen);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setSeconds)
{
	Date* th=asAtomHandler::as<Date>(obj);

	number_t sec, ms;
	ARG_CHECK(ARG_UNPACK (sec) (ms, 0));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	if (!ms) ms = th->milliseconds % 1000;
	th->MakeDate(g_date_time_get_year(th->datetime)+th->extrayears, g_date_time_get_month(th->datetime), g_date_time_get_day_of_month(th->datetime),  g_date_time_get_hour(th->datetime), g_date_time_get_minute(th->datetime), int64_t(sec), int64_t(ms),true);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,secondsSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setSeconds(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setMilliseconds)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t ms;
	ARG_CHECK(ARG_UNPACK (ms));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	th->MakeDate(g_date_time_get_year(th->datetime)+th->extrayears, g_date_time_get_month(th->datetime), g_date_time_get_day_of_month(th->datetime),  g_date_time_get_hour(th->datetime), g_date_time_get_minute(th->datetime), g_date_time_get_second(th->datetime), ms,true);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,millisecondsSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setMilliseconds(o,wrk,obj, args, argslen);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCFullYear)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t y, m, d;
	ARG_CHECK(ARG_UNPACK (y) (m, 0) (d, 0));

	if (argslen > 1)
		m++;
	else
		m = g_date_time_get_month(th->datetimeUTC);
	if (argslen < 3)
		d = g_date_time_get_day_of_month(th->datetimeUTC);
	th->MakeDate(y, m, d, g_date_time_get_hour(th->datetimeUTC),g_date_time_get_minute(th->datetimeUTC),g_date_time_get_second(th->datetimeUTC),th->milliseconds % 1000,false);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,UTCFullYearSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCFullYear(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCMonth)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t m, d;
	ARG_CHECK(ARG_UNPACK (m) (d, 0));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	if (argslen < 2)
		d = g_date_time_get_day_of_month(th->datetimeUTC);
	th->MakeDate(g_date_time_get_year(th->datetimeUTC)+th->extrayears, m+1, d, g_date_time_get_hour(th->datetimeUTC),g_date_time_get_minute(th->datetimeUTC),g_date_time_get_second(th->datetimeUTC),th->milliseconds % 1000,false);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,UTCMonthSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCMonth(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCDate)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t d;
	ARG_CHECK(ARG_UNPACK (d));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	th->MakeDate(g_date_time_get_year(th->datetimeUTC)+th->extrayears, g_date_time_get_month(th->datetimeUTC), d, g_date_time_get_hour(th->datetimeUTC),g_date_time_get_minute(th->datetimeUTC),g_date_time_get_second(th->datetimeUTC),th->milliseconds % 1000,false);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,UTCDateSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCDate(o,wrk,obj, args, argslen);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCHours)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t h, min, sec, ms;
	ARG_CHECK(ARG_UNPACK (h) (min, 0) (sec, 0) (ms, 0));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	if (!min) min = g_date_time_get_minute(th->datetimeUTC);
	if (!sec) sec = g_date_time_get_second(th->datetimeUTC);
	if (!ms) ms = th->milliseconds % 1000;
	th->MakeDate(g_date_time_get_year(th->datetimeUTC)+th->extrayears, g_date_time_get_month(th->datetimeUTC), g_date_time_get_day_of_month(th->datetimeUTC),  h, min, sec, ms,false);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,UTCHoursSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCHours(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCMinutes)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t min, sec, ms;
	ARG_CHECK(ARG_UNPACK (min) (sec, 0) (ms, 0));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	if (!sec) sec = g_date_time_get_second(th->datetimeUTC);
	if (!ms) ms = th->milliseconds % 1000;
	th->MakeDate(g_date_time_get_year(th->datetimeUTC)+th->extrayears, g_date_time_get_month(th->datetimeUTC), g_date_time_get_day_of_month(th->datetimeUTC),  g_date_time_get_hour(th->datetimeUTC), min, sec, ms,false);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,UTCMinutesSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCMinutes(o,wrk,obj, args, argslen==0?0:1);
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCSeconds)
{
	Date* th=asAtomHandler::as<Date>(obj);

	number_t sec, ms;
	ARG_CHECK(ARG_UNPACK (sec) (ms, 0));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	if (!ms) ms = th->milliseconds % 1000;
	th->MakeDate(g_date_time_get_year(th->datetimeUTC)+th->extrayears, g_date_time_get_month(th->datetimeUTC), g_date_time_get_day_of_month(th->datetimeUTC),  g_date_time_get_hour(th->datetimeUTC), g_date_time_get_minute(th->datetimeUTC), sec, ms,false);
	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(Date,UTCSecondsSetter)
{
	asAtom o=asAtomHandler::invalidAtom;
	Date::setUTCSeconds(o,wrk,obj, args, min(argslen, (unsigned)1));
	ASATOM_DECREF(o);
}

ASFUNCTIONBODY_ATOM(Date,setUTCMilliseconds)
{
	Date* th=asAtomHandler::as<Date>(obj);
	number_t ms;
	ARG_CHECK(ARG_UNPACK (ms));

	if (th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	th->MakeDate(g_date_time_get_year(th->datetimeUTC)+th->extrayears, g_date_time_get_month(th->datetimeUTC), g_date_time_get_day_of_month(th->datetimeUTC),  g_date_time_get_hour(th->datetimeUTC), g_date_time_get_minute(th->datetimeUTC), g_date_time_get_second(th->datetimeUTC), ms,false);
	ret = th->msSinceEpoch();
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
		asAtom v = asAtomHandler::fromNumber(wrk,ms,false);
		asAtomHandler::toObject(obj,wrk)->setVariableByMultiname(name,v,CONST_NOT_ALLOWED,nullptr,wrk);
		asAtomHandler::setNumber(ret,wrk,ms);
		return;
	}
	assert_and_throw(asAtomHandler::is<Date>(obj));
	Date* th=asAtomHandler::as<Date>(obj);
	
	if (std::isnan(ms))
	{
		th->nan = true;
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	th->MakeDateFromMilliseconds(int64_t(ms));

	ret = th->msSinceEpoch();
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
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	Date* th=asAtomHandler::as<Date>(obj);
	if(th->nan) {
		asAtomHandler::setNumber(ret,wrk,Number::NaN);
		return;
	}
	ret = th->msSinceEpoch();
}

asAtom Date::msSinceEpoch()
{
	return asAtomHandler::fromNumber(getInstanceWorker(),(number_t)getMsSinceEpoch(),false);
}
int64_t Date::getMsSinceEpoch()
{
	return milliseconds+extrayears/400*MS_IN_400_YEARS;
}


tiny_string Date::toString()
{
	assert_and_throw(implEnable);
	return toString_priv(false, "%a %b %_e %H:%M:%S GMT%z");
}

int Date::getYear()
{
	return this->extrayears + g_date_time_get_year(this->datetime);
}

tiny_string Date::toFormat(bool utc, tiny_string format)
{
	if(nan) 
		return "Invalid Date";
	gchar* fs = g_date_time_format(utc? this->datetimeUTC : this->datetime, format.raw_buf());
	tiny_string res(fs);
	//g_free(fs); // Should free here but it breaks system
	return res;
}

tiny_string Date::toString_priv(bool utc, const char* formatstr) const
{
	if(nan) 
		return "Invalid Date";
	gchar* fs = g_date_time_format(utc? this->datetimeUTC : this->datetime, formatstr);
	tiny_string res(fs);
	char buf[10];
	snprintf(buf,10," %d",(extrayears + g_date_time_get_year(utc? this->datetimeUTC : this->datetime)));
	res += buf;
	g_free(fs);
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
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv(true,"%a %b %_e %H:%M:%S UTC")));
}

ASFUNCTIONBODY_ATOM(Date,toDateString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv(false,"%a %b %_e")));
}

ASFUNCTIONBODY_ATOM(Date,toTimeString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,g_date_time_format(th->datetime, "%H:%M:%S GMT%z")));
}


ASFUNCTIONBODY_ATOM(Date,toLocaleString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	if (!th->datetime)
	{
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
		return;
	}
	tiny_string res = th->toString_priv(false,"%a %b %_e");
	gchar* fs = g_date_time_format(th->datetime, " %I:%M:%S");
	res += fs;
	if (g_date_time_get_hour(th->datetime) > 12)
		res += " PM";
	else
		res += " AM";
	g_free(fs);
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}
ASFUNCTIONBODY_ATOM(Date,toLocaleDateString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv(false,"%a %b %_e")));
}
ASFUNCTIONBODY_ATOM(Date,toLocaleTimeString)
{
	Date* th=asAtomHandler::as<Date>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,g_date_time_format(th->datetime, "%H:%M:%S %Z%z")));
}

ASFUNCTIONBODY_ATOM(Date,_parse)
{
	asAtomHandler::setNumber(ret,wrk,parse(asAtomHandler::toString(args[0],wrk)));
}

static const char* months[] = { "Jan", "Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
number_t Date::parse(tiny_string str)
{
	char mo[4],tzd[20];
	int c,d=0,h=0,m=0,s=0,y=0,mon=-1;
	bool bvalid = false;
	number_t res = Number::NaN;

	tzd[0] = 0;

	// Day Mon DD YYYY HH:MM:SS TZD
	c = sscanf(str.raw_buf(), "%*3s %3s %2d %4d %2d:%2d:%2d %19s",mo, &d,&y, &h,&m,&s,tzd);
	bvalid = (c == 7);
	if (!bvalid)
	{
		// Day Mon DD HH:MM:SS TZD YYYY
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%*3s %3s %2d %2d:%2d:%2d %19s %4d",mo, &d, &h,&m,&s,tzd,&y);
		bvalid = (c == 7);
	}
	if (!bvalid)
	{
		// DD YYYY Mon HH:MM:SS TZD
		// YYYY DD Mon HH:MM:SS TZD
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%4d %4d %3s %2d:%2d:%2d %19s", &d,&y, mo ,&h,&m,&s,tzd);
		bvalid = (c >= 3);
		if (bvalid && d > y)
		{
			int tmp = d;
			d = y;
			y = tmp;
		}
	}
	if (!bvalid)
	{
		// YYYY/MM/DD HH:MM TZD
		// MM/DD/YYYY HH:MM TZD
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%4d/%4d/%4d %2d:%2d %19s",&y,&mon, &d,&h,&m,tzd);
		bvalid = (c >= 3);
		if (bvalid)
		{
			if (y < 70)
			{
				int tmp = y;
				y = d;
				d = mon;
				mon = tmp;
			}
		}
	}
	if (!bvalid)
	{
		// MM/DD/YYYY HH:MM:SS TZD
		// YYYY/MM/DD HH:MM:SS TZD
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%4d/%4d/%4d %2d:%2d:%2d %19s",&mon, &d, &y,&h,&m,&s,tzd);
		bvalid = (c >= 3);
		if (bvalid)
		{
			if (y < 70)
			{
				int tmp = y;
				y = mon;
				mon = d;
				d = tmp;
			}
		}
	}
	if (!bvalid)
	{
		// HH:MM:SS TZD Day Mon/DD/YYYY 
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%2d:%2d:%2d %19s %*3s %3s/%2d/%4d",&h,&m,&s,tzd,mo,&d,&y);
		bvalid = (c == 7);
	}
	if (!bvalid)
	{
		// Mon YYYY DD HH:MM:SS TZD
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%3s %4d %2d %2d:%2d:%2d %19s",mo, &y,&d, &h,&m,&s,tzd);
		bvalid = (c >= 3);
	}
	if (!bvalid)
	{
		// Mon DD YYYY HH:MM:SS TZD
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%3s %2d %4d %2d:%2d:%2d %19s",mo, &d, &y,&h,&m,&s,tzd);
		bvalid = (c >= 3);
	}
	if (!bvalid)
	{
		// Day Mon DD HH:MM:SS TZD YYYY
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%*3s %3s %2d %2d:%2d:%2d %19s %4d",mo,&d,&h,&m,&s,tzd,&y);
		bvalid = (c == 7);
	}
	if (!bvalid)
	{
		// Day DD Mon HH:MM:SS TZD YYYY
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%*3s %2d %3s %2d:%2d:%2d %19s %4d",&d,mo,&h,&m,&s,tzd,&y);
		bvalid = (c == 7);
	}
	if (!bvalid)
	{
		// YYYY/MM/DD HH:MM:SS TZD
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%4d/%2d/%2d %2d:%2d:%2d %19s", &y, &mon,&d,&h,&m,&s,tzd);
		bvalid = (c >= 3);
	}
	if (!bvalid)
	{
		// Day Mon DD YYYY
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%*3s %3s %2d %4d",mo,&d,&y);
		bvalid = (c == 3);
	}
	if (!bvalid)
	{
		// DD Mon YYYY
		// YYYY Mon DD
		d=0;h=0;m=0;s=0;y=0;mon=-1;
		tzd[0] = 0;
		c = sscanf(str.raw_buf(), "%4d %3s %4d",&d,mo,&y);
		bvalid = (c == 3);
		if (bvalid && d > y)
		{
			int tmp = d;
			d = y;
			y = tmp;
		}
	}
	if (bvalid)
	{
		if (mon == -1)
		{
			for (int i = 0;i < 12; i++)
			{
				if (!strcmp(mo,months[i]))
				{
					mon = i+1;
					break;
				}
				
			}
		}
		bool bIsLocalTime = false;
		tiny_string ampm = tzd;
		ampm.uppercase();
		if (!strcmp(ampm.raw_buf(),"AM"))
		{
			bvalid = (h <= 12);
			h = h%12;
			bIsLocalTime = true;
		}
		if (!strcmp(ampm.raw_buf(),"PM"))
		{
			bvalid = (h <= 12);
			h = h%12;
			h += 12;
			bIsLocalTime = true;
		}
		if (bvalid && mon > 0)
		{
			// parse timezone string
			char* p = tzd;
			while (*p && isalpha(*p))
				p++;
			int tz = 0;
			if (*p)
				sscanf(p, "%19d",&tz);
				
			if (y >=70 && y<100)
				y += 1900;
			if (mon >= 1 && mon <= 12 && d >= 1 && d <= 31 && h >= 0 && h <= 23 && m >= 0 && m <= 59 && s >= 0 && s <= 59)
			{
				_R<Date> dt=_MR(Class<Date>::getInstanceS(getWorker()));
				if (tz == 0)
					dt->MakeDate(y, mon, d, h, m, s, 0,bIsLocalTime);
				else
					dt->MakeDate(y, mon, d, h-(tz/100),m-(tz%100), s, 0,false);
				res =dt->nan ? Number::NaN : dt->milliseconds+dt->extrayears/400*MS_IN_400_YEARS;
			}
		}
	}
	return res;
}
bool Date::isEqual(ASObject* r)
{
	check();
	//if we are comparing the same object the answer is true
	if(this==r)
		return true;
	if (r->is<Date>())
		return getMsSinceEpoch() == r->as<Date>()->getMsSinceEpoch();
	return ASObject::isEqual(r);
}

TRISTATE Date::isLess(ASObject* r)
{
	if (r->is<Date>())
		return (getMsSinceEpoch() < r->as<Date>()->getMsSinceEpoch())?TTRUE:TFALSE;
	return ASObject::isLess(r);
}
TRISTATE Date::isLessAtom(asAtom& r)
{
	if (asAtomHandler::is<Date>(r))
		return (getMsSinceEpoch() < asAtomHandler::as<Date>(r)->getMsSinceEpoch())?TTRUE:TFALSE;
	return ASObject::isLessAtom(r);
}

tiny_string Date::format(const char *fmt, bool utc)
{
	gchar* fs = g_date_time_format(utc? this->datetimeUTC : this->datetime, "%a %b %_e %H:%M:%S GMT%z");
	tiny_string res(fs);
	g_free(fs);
	return res;
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
	number_t val = getMsSinceEpoch();
	out->writeByte(date_marker);
	// write milliseconds since 1970 as double
	out->serializeDouble(val);
}

