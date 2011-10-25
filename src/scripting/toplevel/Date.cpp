/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "Date.h"
#include "class.h"
#include "argconv.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("");
REGISTER_CLASS_NAME(Date);

Date::Date():extrayears(0), millisecond(0),datetime(NULL)
{
}

void Date::sinit(Class_base* c)
{
	c->super=Class<ASObject>::getRef();
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("getTimezoneOffset",AS3,Class<IFunction>::getFunction(getTimezoneOffset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(valueOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getTime",AS3,Class<IFunction>::getFunction(getTime),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getFullYear",AS3,Class<IFunction>::getFunction(getFullYear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getMonth",AS3,Class<IFunction>::getFunction(getMonth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDate",AS3,Class<IFunction>::getFunction(getDate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDay",AS3,Class<IFunction>::getFunction(getDay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getHours",AS3,Class<IFunction>::getFunction(getHours),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getMinutes",AS3,Class<IFunction>::getFunction(getMinutes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getSeconds",AS3,Class<IFunction>::getFunction(getSeconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getMilliseconds",AS3,Class<IFunction>::getFunction(getMilliseconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setFullYear",AS3,Class<IFunction>::getFunction(setFullYear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMonth",AS3,Class<IFunction>::getFunction(setMonth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setDate",AS3,Class<IFunction>::getFunction(setDate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setHours",AS3,Class<IFunction>::getFunction(setHours),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMinutes",AS3,Class<IFunction>::getFunction(setMinutes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setSeconds",AS3,Class<IFunction>::getFunction(setSeconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setMilliseconds",AS3,Class<IFunction>::getFunction(setMilliseconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCFullYear",AS3,Class<IFunction>::getFunction(getUTCFullYear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMonth",AS3,Class<IFunction>::getFunction(getUTCMonth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCDate",AS3,Class<IFunction>::getFunction(getUTCDate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCDay",AS3,Class<IFunction>::getFunction(getUTCDay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCHours",AS3,Class<IFunction>::getFunction(getUTCHours),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMinutes",AS3,Class<IFunction>::getFunction(getUTCMinutes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCSeconds",AS3,Class<IFunction>::getFunction(getUTCSeconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMilliseconds",AS3,Class<IFunction>::getFunction(getUTCMilliseconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCFullYear",AS3,Class<IFunction>::getFunction(setUTCFullYear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCMonth",AS3,Class<IFunction>::getFunction(setUTCMonth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCDate",AS3,Class<IFunction>::getFunction(setUTCDate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCHours",AS3,Class<IFunction>::getFunction(setUTCHours),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCMinutes",AS3,Class<IFunction>::getFunction(setUTCMinutes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCSeconds",AS3,Class<IFunction>::getFunction(setUTCSeconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setUTCMilliseconds",AS3,Class<IFunction>::getFunction(setUTCMilliseconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fullYear","",Class<IFunction>::getFunction(getFullYear),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("timezoneOffset","",Class<IFunction>::getFunction(timezoneOffset),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("UTC","",Class<IFunction>::getFunction(UTC),NORMAL_METHOD,false);
}

void Date::buildTraits(ASObject* o)
{
}

const gint64 MS_IN_400_YEARS = 1.26227808e+13;

ASFUNCTIONBODY(Date,_constructor)
{
	Date* th=static_cast<Date*>(obj);
	if (argslen == 1)
	{
	//GLib's GDateTime sensibly does not support and store very large year numbers
	//so keep the extra years in units of 400 years separately. A 400 years period has
	//a fixed number of milliseconds, unaffected by leap year calculations.
		gint64 ms = gint64(args[0]->toNumber());
		th->extrayears = 400*(ms/MS_IN_400_YEARS);
		ms %= MS_IN_400_YEARS;
		th->millisecond = ms%1000;
		gint64 seconds = ms/1000;
		th->datetime = g_date_time_new_from_unix_utc(seconds);
		//If negative date, adjust with the milliseconds value
		if (th->millisecond < 0) {
			th->datetime = g_date_time_add(th->datetime, th->millisecond*1000);
			th->millisecond = 0;
		}
	} else
	{
		number_t year, month, day, hour, minute, second, millisecond;
		ARG_UNPACK (year, 1970) (month, 0) (day, 1) (hour, 0) (minute, 0) (second, 0) (millisecond, 0);
		th->extrayears = year;
		year = 1600+int(year)%400;
		th->extrayears = th->extrayears-year;
		th->datetime = g_date_time_new_utc(year, month+1, day, hour, minute, second);
		if (millisecond > 999)
			th->datetime = g_date_time_add(th->datetime, gint64(millisecond)/1000*G_TIME_SPAN_SECOND);
		th->millisecond = uint64_t(millisecond) % 1000;
	}

	th->year = g_date_time_get_year(th->datetime);
	th->month = g_date_time_get_month(th->datetime);
	th->day = g_date_time_get_day_of_month(th->datetime);
	th->day_of_week = g_date_time_get_day_of_week(th->datetime);
	th->hour = g_date_time_get_hour(th->datetime);
	th->minute = g_date_time_get_minute(th->datetime);
	th->second = g_date_time_get_second(th->datetime);

	return NULL;
}

ASFUNCTIONBODY(Date,UTC)
{
	number_t year, month, day, hour, minute, second, millisecond;
	ARG_UNPACK (year) (month) (day, 1) (hour, 0) (minute, 0) (second, 0) (millisecond, 0);
	GDateTime *tmp = g_date_time_new_utc(year, month+1, day, hour, minute, second);
	if (millisecond > 999)
		tmp = g_date_time_add(tmp, gint64(millisecond)/1000*G_TIME_SPAN_SECOND);
	millisecond = uint64_t(millisecond) % 1000;
	return abstract_d(1000*g_date_time_to_unix(tmp) + millisecond);
}

ASFUNCTIONBODY(Date,getTimezoneOffset)
{
	Date* th=static_cast<Date*>(obj);
	GTimeSpan diff = g_date_time_get_utc_offset(th->datetime);
	return abstract_d(diff/1000000);
}

ASFUNCTIONBODY(Date,timezoneOffset)
{
	return getTimezoneOffset(obj,args,argslen);
}

ASFUNCTIONBODY(Date,getUTCFullYear)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->extrayears + g_date_time_get_year(th->datetime));
}

ASFUNCTIONBODY(Date,getUTCMonth)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_month(th->datetime)-1);
}

ASFUNCTIONBODY(Date,getUTCDate)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_day_of_month(th->datetime));
}

ASFUNCTIONBODY(Date,getUTCDay)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_day_of_week(th->datetime)%7);
}

ASFUNCTIONBODY(Date,getUTCHours)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_hour(th->datetime));
}

ASFUNCTIONBODY(Date,getUTCMinutes)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_minute(th->datetime));
}

ASFUNCTIONBODY(Date,getUTCSeconds)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_second(th->datetime));
}

ASFUNCTIONBODY(Date,getUTCMilliseconds)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->millisecond);
}

ASFUNCTIONBODY(Date,getFullYear)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->extrayears + g_date_time_get_year(th->datetime));
}

ASFUNCTIONBODY(Date,getMonth)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_month(th->datetime)-1);
}

ASFUNCTIONBODY(Date,getDate)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_day_of_month(th->datetime));
}

ASFUNCTIONBODY(Date,getDay)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_day_of_week(th->datetime)%7);
}

ASFUNCTIONBODY(Date,getHours)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_hour(th->datetime));
}

ASFUNCTIONBODY(Date,getMinutes)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_minute(th->datetime));
}

ASFUNCTIONBODY(Date,getSeconds)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(g_date_time_get_second(th->datetime));
}

ASFUNCTIONBODY(Date,getMilliseconds)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->millisecond);
}

ASFUNCTIONBODY(Date,getTime)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->toNumber());
}

ASFUNCTIONBODY(Date,setFullYear)
{
	Date* th=static_cast<Date*>(obj);
	number_t y, m, d;
	ARG_UNPACK (y) (m, 0) (d, 0);

	th->year = y;

	if (m)
		th->month = m+1;

	if (d)
		th->day = d;

	th->datetime = g_date_time_new_local(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setMonth)
{
	Date* th=static_cast<Date*>(obj);
	number_t m, d;
	ARG_UNPACK (m) (d, 0);

	th->month = m+1;

	if (d)
		th->day = d;

	th->datetime = g_date_time_new_local(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setDate)
{
	Date* th=static_cast<Date*>(obj);
	number_t d;
	ARG_UNPACK (d);

	th->day = d;

	th->datetime = g_date_time_new_local(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setHours)
{
	Date* th=static_cast<Date*>(obj);
	number_t h, min, sec, ms;
	ARG_UNPACK (h) (min, 0) (sec, 0) (ms, 0);

	th->hour = h;
	if (min)
		th->minute = min;
	if (sec)
		th->second = sec;
	if (ms)
		th->millisecond = ms;

	th->datetime = g_date_time_new_local(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setMinutes)
{
	Date* th=static_cast<Date*>(obj);
	number_t min, sec, ms;
	ARG_UNPACK (min) (sec, 0) (ms, 0);

	th->minute = min;
	if (sec)
		th->second = sec;
	if (ms)
		th->millisecond = ms;

	th->datetime = g_date_time_new_local(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setSeconds)
{
	Date* th=static_cast<Date*>(obj);

	number_t sec, ms;
	ARG_UNPACK (sec) (ms, 0);

	th->second = sec;
	if (ms)
		th->millisecond = ms;
	th->datetime = g_date_time_new_local(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setMilliseconds)
{
	Date* th=static_cast<Date*>(obj);
	number_t ms;
	ARG_UNPACK (ms);

	th->datetime = g_date_time_add(th->datetime, gint64(ms)/1000*G_TIME_SPAN_SECOND);
	th->millisecond = uint64_t(ms) % 1000;

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setUTCFullYear)
{
	Date* th=static_cast<Date*>(obj);
	number_t y, m, d;
	ARG_UNPACK (y) (m, 0) (d, 0);

	th->year = y;

	if (m)
		th->month = m+1;

	if (d)
		th->day = d;

	th->datetime = g_date_time_new_utc(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setUTCMonth)
{
	Date* th=static_cast<Date*>(obj);
	number_t m, d;
	ARG_UNPACK (m) (d, 0);

	th->month = m+1;

	if (d)
		th->day = d;

	th->datetime = g_date_time_new_utc(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setUTCDate)
{
	Date* th=static_cast<Date*>(obj);
	number_t d;
	ARG_UNPACK (d);

	th->day = d;

	th->datetime = g_date_time_new_utc(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setUTCHours)
{
	Date* th=static_cast<Date*>(obj);
	number_t h, min, sec, ms;
	ARG_UNPACK (h) (min, 0) (sec, 0) (ms, 0);

	th->hour = h;
	if (min)
		th->minute = min;
	if (sec)
		th->second = sec;
	if (ms)
		th->millisecond = ms;

	th->datetime = g_date_time_new_utc(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setUTCMinutes)
{
	Date* th=static_cast<Date*>(obj);
	number_t min, sec, ms;
	ARG_UNPACK (min) (sec, 0) (ms, 0);

	th->minute = min;
	if (sec)
		th->second = sec;
	if (ms)
		th->millisecond = ms;

	th->datetime = g_date_time_new_utc(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setUTCSeconds)
{
	Date* th=static_cast<Date*>(obj);

	number_t sec, ms;
	ARG_UNPACK (sec) (ms, 0);

	th->second = sec;
	if (ms)
		th->millisecond = ms;
	th->datetime = g_date_time_new_utc(th->year, th->month, th->day, th->hour, th->minute, th->second);

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,setUTCMilliseconds)
{
	Date* th=static_cast<Date*>(obj);
	number_t ms;
	ARG_UNPACK (ms);

	th->datetime = g_date_time_add(th->datetime, gint64(ms)/1000*G_TIME_SPAN_SECOND);
	th->millisecond = uint64_t(ms) % 1000;

	return abstract_d(1000*g_date_time_to_unix(th->datetime) + th->millisecond);
}

ASFUNCTIONBODY(Date,valueOf)
{
	Date* th=static_cast<Date*>(obj);
	return th->msSinceEpoch();
}

ASObject* Date::msSinceEpoch()
{
	return abstract_d(1000*g_date_time_to_unix(datetime) + extrayears/400*MS_IN_400_YEARS + millisecond);
}

tiny_string Date::toString(bool debugMsg)
{
	assert_and_throw(implEnable);
	return toString_priv();
}

tiny_string Date::toString_priv() const
{
	return g_date_time_format(datetime, "%a %b %e %H:%M:%S %Z%z %Y");
}

void Date::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	throw UnsupportedException("Date::serialize not implemented");
}

