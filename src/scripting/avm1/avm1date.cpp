/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include "avm1date.h"
#include "compat.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Number.h"

using namespace lightspark;

void AVM1Date::sinit(Class_base* c)
{
	CLASS_SETUP_CONSTRUCTOR_LENGTH(c, ASObject, _constructor, 7, CLASS_FINAL);
	c->isReusable = true;

	c->prototype->setVariableByQName("getDate","",c->getSystemState()->getBuiltinFunction(getDate,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getDay","",c->getSystemState()->getBuiltinFunction(getDay,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getFullYear","",c->getSystemState()->getBuiltinFunction(getFullYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getHours","",c->getSystemState()->getBuiltinFunction(getHours,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getMilliseconds","",c->getSystemState()->getBuiltinFunction(getMilliseconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getMinutes","",c->getSystemState()->getBuiltinFunction(getMinutes,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getMonth","",c->getSystemState()->getBuiltinFunction(getMonth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getSeconds","",c->getSystemState()->getBuiltinFunction(getSeconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getTime","",c->getSystemState()->getBuiltinFunction(getTime,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getTimezoneOffset","",c->getSystemState()->getBuiltinFunction(getTimezoneOffset,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCDate","",c->getSystemState()->getBuiltinFunction(getUTCDate,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCDay","",c->getSystemState()->getBuiltinFunction(getUTCDay,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCFullYear","",c->getSystemState()->getBuiltinFunction(getUTCFullYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCHours","",c->getSystemState()->getBuiltinFunction(getUTCHours,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCMilliseconds","",c->getSystemState()->getBuiltinFunction(getUTCMilliseconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCMinutes","",c->getSystemState()->getBuiltinFunction(getUTCMinutes,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCMonth","",c->getSystemState()->getBuiltinFunction(getUTCMonth,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCSeconds","",c->getSystemState()->getBuiltinFunction(getUTCSeconds,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCYear","",c->getSystemState()->getBuiltinFunction(AVM1_getUTCYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getYear","",c->getSystemState()->getBuiltinFunction(AVM1_getYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setDate","",c->getSystemState()->getBuiltinFunction(setDate),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setFullYear","",c->getSystemState()->getBuiltinFunction(setFullYear,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setHours","",c->getSystemState()->getBuiltinFunction(setHours,4),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setMilliseconds","",c->getSystemState()->getBuiltinFunction(setMilliseconds),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setMinutes","",c->getSystemState()->getBuiltinFunction(setMinutes,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setMonth","",c->getSystemState()->getBuiltinFunction(setMonth,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setSeconds","",c->getSystemState()->getBuiltinFunction(setSeconds,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setTime","",c->getSystemState()->getBuiltinFunction(setTime,1),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCDate","",c->getSystemState()->getBuiltinFunction(setUTCDate),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCFullYear","",c->getSystemState()->getBuiltinFunction(setUTCFullYear,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCHours","",c->getSystemState()->getBuiltinFunction(setUTCHours,4),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCMilliseconds","",c->getSystemState()->getBuiltinFunction(setUTCMilliseconds),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCMinutes","",c->getSystemState()->getBuiltinFunction(setUTCMinutes,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCMonth","",c->getSystemState()->getBuiltinFunction(setUTCMonth,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCSeconds","",c->getSystemState()->getBuiltinFunction(setUTCSeconds,2),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCYear","",c->getSystemState()->getBuiltinFunction(AVM1_setUTCYear,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setYear","",c->getSystemState()->getBuiltinFunction(AVM1_setYear,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("UTC","",c->getSystemState()->getBuiltinFunction(UTC,7,Class<Number>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",c->getSystemState()->getBuiltinFunction(valueOf,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
}

ASFUNCTIONBODY_ATOM(AVM1Date,AVM1_getYear)
{
	AVM1Date* th = asAtomHandler::as<AVM1Date>(obj);

	if (th->isValid())
		asAtomHandler::setInt(ret, wrk, th->getYear() - 1900);
	else
		asAtomHandler::setNumber(ret, wrk, Number::NaN);
}

ASFUNCTIONBODY_ATOM(AVM1Date,AVM1_getUTCYear)
{
	AVM1Date* th = asAtomHandler::as<AVM1Date>(obj);

	if (th->isValid())
		asAtomHandler::setInt(ret, wrk, th->getUTCYear() - 1900);
	else
		asAtomHandler::setNumber(ret, wrk,Number::NaN);
}

ASFUNCTIONBODY_ATOM(AVM1Date,AVM1_setYear)
{
	AVM1Date* th = asAtomHandler::as<AVM1Date>(obj);
	number_t y, m, d;
	ARG_CHECK(ARG_UNPACK (y, 0) (m, 0) (d, 0));

	if (argslen > 0)
	{
		if (y >= 0 && y <= 99)
			y += 1900;
	}
	else if (th->isValid())
		y = th->getYear();

	if (argslen > 1)
		m++;
	else if (th->isValid())
		m = g_date_time_get_month(th->getDateTime()) ;

	if (argslen < 3 && th->isValid())
		d = th->getDateTime() ? g_date_time_get_day_of_month(th->getDateTime()) : 0;

	th->MakeDate
	(
		y,
		m,
		d,
		th->isValid() ? g_date_time_get_hour(th->getDateTime()) : 0,
		th->isValid() ? g_date_time_get_minute(th->getDateTime()) : 0,
		th->isValid() ? g_date_time_get_second(th->getDateTime()) : 0,
		th->getMs() % 1000,
		true
	);

	ret = th->msSinceEpoch();
}

ASFUNCTIONBODY_ATOM(AVM1Date,AVM1_setUTCYear)
{
	AVM1Date* th = asAtomHandler::as<AVM1Date>(obj);
	number_t y, m, d;
	ARG_CHECK(ARG_UNPACK (y, 0) (m, 0) (d, 0));

	if (argslen > 0)
	{
		if (y >= 0 && y <= 99)
			y += 1900;
	}
	else
		y = th->getUTCYear();

	if (argslen > 1)
		m++;
	else if (th->isValid())
		m = g_date_time_get_month(th->getDateTimeUTC());

	if (argslen < 3)
		d = g_date_time_get_day_of_month(th->getDateTimeUTC());

	th->MakeDate
	(
		y,
		m,
		d,
		g_date_time_get_hour(th->getDateTimeUTC()),
		g_date_time_get_minute(th->getDateTimeUTC()),
		g_date_time_get_second(th->getDateTimeUTC()),
		th->getMs() % 1000,
		false
	);

	ret = th->msSinceEpoch();
}
