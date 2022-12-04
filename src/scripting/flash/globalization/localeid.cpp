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

#include "scripting/flash/globalization/localeid.h"
#include "backends/locale.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void LocaleID::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);
	c->setVariableAtomByQName("DEFAULT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"i-default"),CONSTANT_TRAIT);

	c->setDeclaredMethodByQName("determinePreferredLocales","",Class<IFunction>::getFunction(c->getSystemState(),determinePreferredLocales),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getKeysAndValues","",Class<IFunction>::getFunction(c->getSystemState(),getKeysAndValues),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getLanguage","",Class<IFunction>::getFunction(c->getSystemState(),getLanguage),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getRegion","",Class<IFunction>::getFunction(c->getSystemState(),getRegion),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getScript","",Class<IFunction>::getFunction(c->getSystemState(),getScript),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getVariant","",Class<IFunction>::getFunction(c->getSystemState(),getVariant),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isRightToLeft","",Class<IFunction>::getFunction(c->getSystemState(),isRightToLeft),NORMAL_METHOD,true);

	REGISTER_GETTER(c, name);
	REGISTER_GETTER(c, actualLocaleIDName);
	REGISTER_GETTER(c, lastOperationStatus);
	REGISTER_GETTER(c, requestedLocaleIDName);
}

ASFUNCTIONBODY_GETTER(LocaleID, name)
ASFUNCTIONBODY_GETTER(LocaleID, actualLocaleIDName)
ASFUNCTIONBODY_GETTER(LocaleID, lastOperationStatus)
ASFUNCTIONBODY_GETTER(LocaleID, requestedLocaleIDName)

ASFUNCTIONBODY_ATOM(LocaleID,_constructor)
{
	LocaleID* th =asAtomHandler::as<LocaleID>(obj);
	ARG_CHECK(ARG_UNPACK(th->requestedLocaleIDName));
	std::string locale = th->requestedLocaleIDName;
	th->name = th->requestedLocaleIDName;
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

ASFUNCTIONBODY_ATOM(LocaleID,determinePreferredLocales)
{
	// This method is optional, can return null
	LocaleID* th =asAtomHandler::as<LocaleID>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"LocaleID.determinePreferredLocales is not implemented.");
	th->lastOperationStatus="noError";
	ret = asAtomHandler::nullAtom;
}

ASFUNCTIONBODY_ATOM(LocaleID,getKeysAndValues)
{
	LocaleID* th =asAtomHandler::as<LocaleID>(obj);
	ASObject* object = Class<ASObject>::getInstanceS(wrk);
	std::string str(th->name.raw_buf());
	size_t pos = str.find("@");
	std::string keyValueDelim("=");
	std::string itemDelim(";");
	if (pos != string::npos)
	{
		std::size_t current, previous = pos+1;
		current = str.find(itemDelim);
		while (current != std::string::npos)
		{
			std::string item = str.substr(previous, current - previous);
			size_t keyValueDelimPos = item.find(keyValueDelim);
			if (keyValueDelimPos != std::string::npos)
			{
				std::string key = item.substr(0, keyValueDelimPos);
				std::string value = item.substr(keyValueDelimPos+1, item.size());
				object->setVariableAtomByQName(key,nsNameAndKind(),asAtomHandler::fromString(th->getSystemState(),value),DYNAMIC_TRAIT);
			}
			previous = current + 1;
			current = str.find(itemDelim, previous);
		}
		if (previous != std::string::npos)
		{
			std::string item = str.substr(previous, str.size());
			size_t keyValueDelimPos = item.find(keyValueDelim);
			if (keyValueDelimPos != std::string::npos)
			{
				std::string key = item.substr(0, keyValueDelimPos);
				std::string value = item.substr(keyValueDelimPos+1, item.size());
				object->setVariableAtomByQName(key,nsNameAndKind(),asAtomHandler::fromString(th->getSystemState(),value),DYNAMIC_TRAIT);
			}
		}
	}
	th->lastOperationStatus="noError";
	ret = asAtomHandler::fromObject(object);
}

ASFUNCTIONBODY_ATOM(LocaleID,getLanguage)
{
	LocaleID* th =asAtomHandler::as<LocaleID>(obj);
	auto locale = wrk->getSystemState()->localeManager->getLocaleId(th->name);
	if (locale != nullptr)
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),locale->language);
		th->lastOperationStatus="noError";
		return;
	}
	ret = asAtomHandler::fromString(wrk->getSystemState(),"");
}

ASFUNCTIONBODY_ATOM(LocaleID,getRegion)
{
	LocaleID* th =asAtomHandler::as<LocaleID>(obj);
	auto locale = wrk->getSystemState()->localeManager->getLocaleId(th->name);
	if (locale != nullptr)
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),locale->region);
		th->lastOperationStatus="noError";
		return;
	}
	ret = asAtomHandler::fromString(wrk->getSystemState(),"");
}

ASFUNCTIONBODY_ATOM(LocaleID,getScript)
{
	LocaleID* th =asAtomHandler::as<LocaleID>(obj);
	auto locale = wrk->getSystemState()->localeManager->getLocaleId(th->name);
	if (locale != nullptr)
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),locale->script);
		th->lastOperationStatus="noError";
		return;
	}
	ret = asAtomHandler::fromString(wrk->getSystemState(),"");
}

ASFUNCTIONBODY_ATOM(LocaleID,getVariant)
{
	LocaleID* th =asAtomHandler::as<LocaleID>(obj);
	auto locale = wrk->getSystemState()->localeManager->getLocaleId(th->name);
	if (locale != nullptr)
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),locale->variant);
		th->lastOperationStatus="noError";
		return;
	}
	ret = asAtomHandler::fromString(wrk->getSystemState(),"");
}

ASFUNCTIONBODY_ATOM(LocaleID,isRightToLeft)
{
	LocaleID* th =asAtomHandler::as<LocaleID>(obj);
	auto locale = wrk->getSystemState()->localeManager->getLocaleId(th->name);
	if (locale != nullptr)
	{
		ret = asAtomHandler::fromBool(locale->isRightToLeft);
		th->lastOperationStatus="noError";
		return;
	}
	ret = asAtomHandler::fromBool(false);
}
