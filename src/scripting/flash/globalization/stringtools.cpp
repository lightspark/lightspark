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

#include "scripting/flash/globalization/stringtools.h"
#include "backends/locale.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Array.h"

#include <iostream>
#include <algorithm>

using namespace lightspark;

StringTools::StringTools(ASWorker* wrk, Class_base* c):
	ASObject(wrk,c)
{
}

void StringTools::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);

	REGISTER_GETTER(c, actualLocaleIDName);
	REGISTER_GETTER(c, lastOperationStatus);
	REGISTER_GETTER(c, requestedLocaleIDName);

	c->setDeclaredMethodByQName("getAvailableLocaleIDNames","",c->getSystemState()->getBuiltinFunction(getAvailableLocaleIDNames),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLowerCase","",c->getSystemState()->getBuiltinFunction(toLowerCase),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toUpperCase","",c->getSystemState()->getBuiltinFunction(toUpperCase),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(StringTools,_constructor)
{
	StringTools* th =asAtomHandler::as<StringTools>(obj);
	ARG_CHECK(ARG_UNPACK(th->requestedLocaleIDName));
	if (wrk->getSystemState()->localeManager->isLocaleAvailableOnSystem(th->requestedLocaleIDName))
	{
		std::string localeName = wrk->getSystemState()->localeManager->getSystemLocaleName(th->requestedLocaleIDName);
		th->currlocale = std::locale(localeName.c_str());
		th->actualLocaleIDName = th->requestedLocaleIDName;
		th->lastOperationStatus="noError";
	}
	else
	{
		LOG(LOG_INFO,"unknown locale:"<<th->requestedLocaleIDName);
		th->lastOperationStatus="usingDefaultWarning";
	}
}

ASFUNCTIONBODY_GETTER(StringTools, actualLocaleIDName)
ASFUNCTIONBODY_GETTER(StringTools, lastOperationStatus)
ASFUNCTIONBODY_GETTER(StringTools, requestedLocaleIDName)

ASFUNCTIONBODY_ATOM(StringTools,getAvailableLocaleIDNames)
{
	StringTools* th =asAtomHandler::as<StringTools>(obj);
	Array* res=Class<Array>::getInstanceSNoArgs(wrk);
	std::vector<std::string> localeIds = wrk->getSystemState()->localeManager->getAvailableLocaleIDNames();
	for (std::vector<std::string>::iterator it = localeIds.begin(); it != localeIds.end(); ++it)
	{
		tiny_string value = (*it);
		res->push(asAtomHandler::fromObject(abstract_s(wrk, value)));
	}
	th->lastOperationStatus="noError";
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(StringTools,toLowerCase)
{
  LOG(LOG_NOT_IMPLEMENTED,"StringTools.toLowerCase is not really tested for all formats");
  StringTools* th =asAtomHandler::as<StringTools>(obj);
  try
  {
    tiny_string s;
    ARG_CHECK(ARG_UNPACK(s));
    std::locale l =  std::locale::global(th->currlocale);
    std::string res = s.raw_buf();

    // TODO: tolower needs to be replaced with
    // something that matches Flash's tolower method better.
    // So "ÃŸ" here will not lower to "ãÿ" for example.
    transform(res.begin(), res.end(), res.begin(), ::tolower);
    std::locale::global(l);
    th->lastOperationStatus = "noError";
    ret = asAtomHandler::fromString(wrk->getSystemState(),res);
  }
  catch (std::runtime_error& e)
  {
    th->lastOperationStatus="usingDefaultWarning";
    LOG(LOG_ERROR,"unknown locale:"<<th->requestedLocaleIDName<<" "<<e.what());
  }
}

ASFUNCTIONBODY_ATOM(StringTools,toUpperCase)
{
  LOG(LOG_NOT_IMPLEMENTED,"StringTools.toUpperCase is not really tested for all formats");
  StringTools* th =asAtomHandler::as<StringTools>(obj);
  try
  {
    tiny_string s;
    ARG_CHECK(ARG_UNPACK(s));
    std::locale l =  std::locale::global(th->currlocale);
    std::string res = s.raw_buf();

    // TODO: toupper needs to be replaced with
    // something that matches Flash's toupper method better.
    transform(res.begin(), res.end(), res.begin(), ::toupper);
    std::locale::global(l);
    th->lastOperationStatus = "noError";
    ret = asAtomHandler::fromString(wrk->getSystemState(),res);
  }
  catch (std::runtime_error& e)
  {
    th->lastOperationStatus="usingDefaultWarning";
    LOG(LOG_ERROR,"unknown locale:"<<th->requestedLocaleIDName<<" "<<e.what());
  }
}
