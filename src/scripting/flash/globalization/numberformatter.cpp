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

#include "scripting/flash/globalization/numberformatter.h"
#include "backends/locale.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

NumberFormatter::NumberFormatter(Class_base* c):
	ASObject(c),fractionalDigits(0)
{
}

void NumberFormatter::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);
	REGISTER_GETTER(c, actualLocaleIDName);
	REGISTER_GETTER(c, lastOperationStatus);
	REGISTER_GETTER(c, requestedLocaleIDName);
	REGISTER_GETTER_SETTER(c, fractionalDigits);
	c->setDeclaredMethodByQName("formatNumber","",Class<IFunction>::getFunction(c->getSystemState(),formatNumber),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getAvailableLocaleIDNames","",Class<IFunction>::getFunction(c->getSystemState(),formatNumber),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_GETTER(NumberFormatter, actualLocaleIDName);
ASFUNCTIONBODY_GETTER(NumberFormatter, lastOperationStatus);
ASFUNCTIONBODY_GETTER(NumberFormatter, requestedLocaleIDName);
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, fractionalDigits);

ASFUNCTIONBODY_ATOM(NumberFormatter,_constructor)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
	ARG_UNPACK_ATOM(th->requestedLocaleIDName);
	if (sys->localeManager->isLocaleAvailableOnSystem(th->requestedLocaleIDName))
	{
		std::string localeName = sys->localeManager->getSystemLocaleName(th->requestedLocaleIDName);
		th->currlocale = std::locale(localeName);
		th->actualLocaleIDName = th->requestedLocaleIDName;
		th->lastOperationStatus="noError";
	}
	else
	{
		LOG(LOG_INFO,"unknown locale:"<<th->requestedLocaleIDName);
		th->lastOperationStatus="usingDefaultWarning";
	}
}

ASFUNCTIONBODY_ATOM(NumberFormatter,formatNumber)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
	number_t value;
	ARG_UNPACK_ATOM(value);
	tiny_string res;

	std::stringstream ss;
	ss.imbue(th->currlocale);
	ss << std::setprecision(th->fractionalDigits) << std::fixed << value;
	res = ss.str();
	ret = asAtomHandler::fromString(sys,res);
}


ASFUNCTIONBODY_ATOM(NumberFormatter,getAvailableLocaleIDNames)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
	Array* res=Class<Array>::getInstanceSNoArgs(sys);
	std::vector<std::string> localeIds = sys->localeManager->getAvailableLocaleIDNames();
	for (std::vector<std::string>::iterator it = localeIds.begin(); it != localeIds.end(); ++it)
	{
		tiny_string value = (*it);
		res->push(asAtomHandler::fromObject(abstract_s(sys, value)));
	}
	th->lastOperationStatus="noError";
	ret = asAtomHandler::fromObject(res);
}
