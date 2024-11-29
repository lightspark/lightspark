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
	Date::sinit(c);
	c->prototype->setVariableByQName("getYear","",c->getSystemState()->getBuiltinFunction(AVM1_getYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("getUTCYear","",c->getSystemState()->getBuiltinFunction(AVM1_getUTCYear,0,Class<Number>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setYear","",c->getSystemState()->getBuiltinFunction(AVM1_setYear,3),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("setUTCYear","",c->getSystemState()->getBuiltinFunction(AVM1_setUTCYear,3),CONSTANT_TRAIT);
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

	if (argslen > 0 && y >= 0 && y <= 99)
		y += 1900;
	else
		y = th->getYear();

	if (argslen > 1)
		m++;
	else
		m = g_date_time_get_month(th->getDateTime());

	if (argslen < 3)
		d = g_date_time_get_day_of_month(th->getDateTime());

	th->MakeDate
	(
		y,
		m,
		d,
		g_date_time_get_hour(th->getDateTime()),
		g_date_time_get_minute(th->getDateTime()),
		g_date_time_get_second(th->getDateTime()),
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

	if (argslen > 0 && y >= 0 && y <= 99)
		y += 1900;
	else
		y = th->getUTCYear();

	if (argslen > 1)
		m++;
	else
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
