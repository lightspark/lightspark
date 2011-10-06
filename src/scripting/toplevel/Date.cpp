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
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
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
	c->setDeclaredMethodByQName("getUTCFullYear",AS3,Class<IFunction>::getFunction(getUTCFullYear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMonth",AS3,Class<IFunction>::getFunction(getUTCMonth),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCDate",AS3,Class<IFunction>::getFunction(getUTCDate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCDay",AS3,Class<IFunction>::getFunction(getUTCDay),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCHours",AS3,Class<IFunction>::getFunction(getUTCHours),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMinutes",AS3,Class<IFunction>::getFunction(getUTCMinutes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCSeconds",AS3,Class<IFunction>::getFunction(getUTCSeconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getUTCMilliseconds",AS3,Class<IFunction>::getFunction(getUTCMilliseconds),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fullYear","",Class<IFunction>::getFunction(getFullYear),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("timezoneOffset","",Class<IFunction>::getFunction(getFullYear),GETTER_METHOD,true);
}

void Date::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Date,_constructor)
{
	Date* th=static_cast<Date*>(obj);
	if (argslen == 1)
	{
	//GLib's GDateTime sensibly does not support and store very large year numbers
	//so keep the extra years in units of 400 years separately. A 400 years period has
	//a fixed number of milliseconds, unaffected by leap year calculations.
		const gint64 MS_IN_400_YEARS = 1.26227808e+13;
		gint64 ms = gint64(args[0]->toNumber());
		th->extrayears = 400*(ms/MS_IN_400_YEARS);
		ms %= MS_IN_400_YEARS;
		th->millisecond = ms%1000;
		gint64 seconds = ms/1000;
		th->datetime = g_date_time_new_from_unix_utc(seconds);
	} else
	{
		number_t year, month, day, hour, minute, second, millisecond;
		ARG_UNPACK (year, 1970) (month, 0) (day, 1) (hour, 0) (minute, 0) (second, 0) (millisecond, 0);
		th->datetime = g_date_time_new_utc(year, month+1, day, hour, minute, second);
		th->millisecond = millisecond;
	}

	return NULL;
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

ASFUNCTIONBODY(Date,valueOf)
{
	Date* th=static_cast<Date*>(obj);
	return abstract_d(th->toNumber());
}

double Date::toNumber()
{
	return double(1000*g_date_time_to_unix(datetime) + millisecond);
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

