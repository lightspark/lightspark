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

#include "scripting/flash/globalization/collator.h"
#include "backends/locale.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Array.h"
#include <iomanip>

using namespace lightspark;

void Collator::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);
	REGISTER_GETTER(c, actualLocaleIDName);
	REGISTER_GETTER_SETTER(c, ignoreCase);
	REGISTER_GETTER_SETTER(c, ignoreCharacterWidth);
	REGISTER_GETTER_SETTER(c, ignoreDiacritics);
	REGISTER_GETTER_SETTER(c, ignoreKanaType);
	REGISTER_GETTER_SETTER(c, ignoreSymbols);
	REGISTER_GETTER(c, lastOperationStatus);
	REGISTER_GETTER_SETTER(c, numericComparison);
	REGISTER_GETTER(c, requestedLocaleIDName);

	c->setDeclaredMethodByQName("compare","",c->getSystemState()->getBuiltinFunction(compare),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("equals","",c->getSystemState()->getBuiltinFunction(equals),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getAvailableLocaleIDNames","",c->getSystemState()->getBuiltinFunction(getAvailableLocaleIDNames),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(Collator,_constructor)
{
	Collator* th =asAtomHandler::as<Collator>(obj);
	ARG_CHECK(ARG_UNPACK(th->requestedLocaleIDName)(th->initialMode));
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

ASFUNCTIONBODY_GETTER(Collator, actualLocaleIDName)
ASFUNCTIONBODY_GETTER_SETTER(Collator, ignoreCase)
ASFUNCTIONBODY_GETTER_SETTER(Collator, ignoreCharacterWidth)
ASFUNCTIONBODY_GETTER_SETTER(Collator, ignoreDiacritics)
ASFUNCTIONBODY_GETTER_SETTER(Collator, ignoreKanaType)
ASFUNCTIONBODY_GETTER_SETTER(Collator, ignoreSymbols)
ASFUNCTIONBODY_GETTER(Collator, lastOperationStatus)
ASFUNCTIONBODY_GETTER_SETTER(Collator, numericComparison)
ASFUNCTIONBODY_GETTER(Collator, requestedLocaleIDName)

bool Collator::isSymbol(uint32_t character)
{
	// Space is a symbol in this API
	if (isspace(character) || character == 32) 
	{
		return true;
	}
	if (iswpunct(character))
	{
		return true;
	}
	return iswcntrl(character);
}

int Collator::compare(
	std::string string1,
	std::string string2,
	bool ignoreCase,
	bool ignoreCharacterWidth,
	bool ignoreDiacritics,
	bool ignoreKanaType,
	bool ignoreSymbols)
{
	string::iterator s1It= string1.begin();
    string::iterator s2It = string2.begin();
	
	// Build first string
	string s1 = "";
	while (s1It!= string1.end())
	{
		if (ignoreSymbols && isSymbol((*s1It)))
		{
			// Do nothing
		}
		else if (ignoreCase)
		{
			s1.push_back(std::tolower((*s1It)));
		}
		else
		{
			s1.push_back((*s1It));
		}
		s1It++;
	}

	// Build second string
	string s2 = "";
	while (s2It != string2.end())
	{
		if (ignoreSymbols && isSymbol((*s2It)))
		{
			// Do nothing
		}
		else if (ignoreCase)
		{
			s2.push_back(std::tolower((*s2It)));
		}
		else
		{
			s2.push_back((*s2It));
		}
		s2It++;
	}

	// Compare results
	int result = s2.compare(s1);
	if (result > 0)
	{
		return 1;
	}
	if (result < 0)
	{
		return -1;
	}
	return 0;
}

bool Collator::equals(
	std::string string1,
	std::string string2,
	bool ignoreCase,
	bool ignoreCharacterWidth,
	bool ignoreDiacritics,
	bool ignoreKanaType,
	bool ignoreSymbols)
{
	string::iterator s1It = string1.begin();
	string::iterator s2It = string2.begin();
	while (s1It!= string1.end() && s2It != string2.end())
	{
		uint32_t char1 = (*s1It);
		uint32_t char2 = (*s2It);
		if (ignoreCase)
		{
			char1 = std::tolower(char1);
			char2 = std::tolower(char2);
		}
		if (ignoreSymbols && (isSymbol(char1) || isSymbol(char2)))
		{
			if (isSymbol(char1))
			{
				++s1It;
			}
			if (isSymbol(char2))
			{
				++s2It;
			}
		}
		else
		{
			if (char1 != char2)
			{
				return false;
			}
			++s1It;
			++s2It;
		}
	}
	if (s1It!= string1.end())
	{
		return false;
	}
	if (s2It != string2.end())
	{
		return false;
	}
	return true;
}

ASFUNCTIONBODY_ATOM(Collator,compare)
{
    LOG(LOG_NOT_IMPLEMENTED,"Collator.compare is not really tested for all formats");
	Collator* th = asAtomHandler::as<Collator>(obj);
    if (th->ignoreKanaType)
	{
		LOG(LOG_NOT_IMPLEMENTED,"ignoreKanaType is not supported");
	}
    if (th->numericComparison)
	{
		LOG(LOG_NOT_IMPLEMENTED,"numericComparison is not supported");
	}
	if (th->ignoreDiacritics)
	{
		LOG(LOG_NOT_IMPLEMENTED,"diacritics is not supported");
	}
	if (th->ignoreCharacterWidth)
	{
		LOG(LOG_NOT_IMPLEMENTED,"ignoreCharacterWidth is not supported");
	}
	try
    {
		std::locale l =  std::locale::global(th->currlocale);
	    tiny_string string1;
	    tiny_string string2;
	    ARG_CHECK(ARG_UNPACK(string1)(string2));
		std::string s1 = string1.raw_buf();
		std::string s2 = string2.raw_buf();
	    std::locale::global(l);
	 	int value = 0;
		if (th->initialMode == "matching")
		{
			value = th->compare(s1, s2, true, true, true, true, true);
		}
		else
		{
			value = th->compare(s1, s2, th->ignoreCase,
				th->ignoreCharacterWidth,
				th->ignoreDiacritics,
				th->ignoreKanaType,
				th->ignoreSymbols);
		}
		ret = asAtomHandler::fromInt(value);
		th->lastOperationStatus = "noError";
    }
    catch (std::runtime_error& e)
    {
		th->lastOperationStatus="usingDefaultWarning";
		LOG(LOG_ERROR,e.what());
    }
}

ASFUNCTIONBODY_ATOM(Collator,equals)
{
	LOG(LOG_NOT_IMPLEMENTED,"Collator.equals is not really tested for all formats");
	Collator* th = asAtomHandler::as<Collator>(obj);
	if (th->ignoreKanaType)
	{
		LOG(LOG_NOT_IMPLEMENTED,"ignoreKanaType is not supported");
	}
    if (th->numericComparison)
	{
		LOG(LOG_NOT_IMPLEMENTED,"numericComparison is not supported");
	}
	if (th->ignoreDiacritics)
	{
		LOG(LOG_NOT_IMPLEMENTED,"diacritics is not supported");
	}
	if (th->ignoreCharacterWidth)
	{
		LOG(LOG_NOT_IMPLEMENTED,"ignoreCharacterWidth is not supported");
	}
	try
    {
		tiny_string string1;
	    tiny_string string2;
	    ARG_CHECK(ARG_UNPACK(string1)(string2));
		std::locale l =  std::locale::global(th->currlocale);
		std::string s1 = string1.raw_buf();
		std::string s2 = string2.raw_buf();
	    std::locale::global(l);
		bool value = false;
		if (th->initialMode == "matching")
		{
			value = th->equals(s1, s2, true, true, true, true, true);
		}
		else
		{
			value = th->equals(s1, s2, th->ignoreCase,
				th->ignoreCharacterWidth,
				th->ignoreDiacritics,
				th->ignoreKanaType,
				th->ignoreSymbols);
		}
		ret = asAtomHandler::fromBool(value);
		th->lastOperationStatus = "noError";
    }
    catch (std::runtime_error& e)
    {
		th->lastOperationStatus="usingDefaultWarning";
		LOG(LOG_ERROR,e.what());
    }
}

ASFUNCTIONBODY_ATOM(Collator,getAvailableLocaleIDNames)
{
	Collator* th = asAtomHandler::as<Collator>(obj);
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
