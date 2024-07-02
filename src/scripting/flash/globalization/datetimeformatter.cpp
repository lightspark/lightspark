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

#include "scripting/flash/globalization/datetimeformatter.h"
#include "backends/locale.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Date.h"
#include <iomanip>

using namespace lightspark;

DateTimeFormatter::DateTimeFormatter(ASWorker* wrk, Class_base* c):
	ASObject(wrk,c)
{
}

void DateTimeFormatter::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);
	REGISTER_GETTER(c, actualLocaleIDName);
	REGISTER_GETTER(c, lastOperationStatus);
	REGISTER_GETTER(c, requestedLocaleIDName);
	c->setDeclaredMethodByQName("setDateTimePattern","",c->getSystemState()->getBuiltinFunction(setDateTimePattern),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("format","",c->getSystemState()->getBuiltinFunction(format),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("formatUTC","",c->getSystemState()->getBuiltinFunction(formatUTC),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getAvailableLocaleIDNames","",c->getSystemState()->getBuiltinFunction(formatUTC),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDateStyle","",c->getSystemState()->getBuiltinFunction(getDateStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getDateTimePattern","",c->getSystemState()->getBuiltinFunction(getDateTimePattern),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getFirstWeekday","",c->getSystemState()->getBuiltinFunction(getFirstWeekday),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getMonthNames","",c->getSystemState()->getBuiltinFunction(getMonthNames),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getTimeStyle","",c->getSystemState()->getBuiltinFunction(getTimeStyle),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getWeekdayNames","",c->getSystemState()->getBuiltinFunction(getWeekdayNames),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setDateTimeStyles","",c->getSystemState()->getBuiltinFunction(setDateTimeStyles),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,_constructor)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	tiny_string dateStyle, timeStyle;
	ARG_CHECK(ARG_UNPACK(th->requestedLocaleIDName)(dateStyle,"long")(timeStyle,"long"));

	if (dateStyle == "long" || dateStyle == "medium" ||
		dateStyle == "short" || dateStyle == "none")
	{
		th->dateStyle = dateStyle;
	}
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"dateStyle value is not valid");
		return;
	}
	if (timeStyle == "long" || timeStyle == "medium" ||
		timeStyle == "short" || timeStyle == "none")
	{
		th->timeStyle = timeStyle;
	}
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"timeStyle value is not valid");
		return;
	}

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

ASFUNCTIONBODY_GETTER(DateTimeFormatter, actualLocaleIDName)
ASFUNCTIONBODY_GETTER(DateTimeFormatter, lastOperationStatus)
ASFUNCTIONBODY_GETTER(DateTimeFormatter, requestedLocaleIDName)

ASFUNCTIONBODY_ATOM(DateTimeFormatter,setDateTimePattern)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	ARG_CHECK(ARG_UNPACK(th->pattern));
	th->dateStyle = "custom";
	th->timeStyle = "custom";
	th->lastOperationStatus="noError";
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,format)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	_NR<Date> dt;
	ARG_CHECK(ARG_UNPACK(dt));
	tiny_string res;
	if (!dt.isNull())
	{
		LOG(LOG_NOT_IMPLEMENTED,"DateTimeFormatter.format is not really tested for all formats");
		std::locale l =  std::locale::global(th->currlocale);
		std::string pattern = th->pattern;
		if (th->dateStyle != "custom")
		{
			pattern = buildDateTimePattern(th->dateStyle, th->timeStyle);
		}
		tiny_string internalPattern = pattern = buildInternalFormat(pattern);
		tiny_string value = dt->toFormat(true, internalPattern);
		if (value.startsWith(" "))
		{
			value = value.substr(1,value.numBytes());
		}
		ret = asAtomHandler::fromString(wrk->getSystemState(),value);
		std::locale::global(l);
		th->lastOperationStatus = "noError";
	}
	else
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"");
	}
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,formatUTC)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	_NR<Date> dt;
	ARG_CHECK(ARG_UNPACK(dt));
	tiny_string res;
	if (!dt.isNull())
	{
		LOG(LOG_NOT_IMPLEMENTED,"DateTimeFormatter.format is not really tested for all formats");
		std::locale l =  std::locale::global(th->currlocale);
		std::string pattern = th->pattern;

		if (th->dateStyle != "custom")
		{
			pattern = buildDateTimePattern(th->dateStyle, th->timeStyle);
		}
		tiny_string internalPattern = pattern = buildInternalFormat(pattern);
		tiny_string value = dt->toFormat(true, internalPattern);	
		if (value.startsWith(" "))
		{
			value = value.substr(1,value.numBytes());
		}
		ret = asAtomHandler::fromString(wrk->getSystemState(),value);
		std::locale::global(l);
		th->lastOperationStatus = "noError";
	}
	else
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"");
	}
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,getAvailableLocaleIDNames)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
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

ASFUNCTIONBODY_ATOM(DateTimeFormatter,getDateStyle)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	ret = asAtomHandler::fromString(wrk->getSystemState(),th->dateStyle);
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,getDateTimePattern)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	ret = asAtomHandler::fromString(wrk->getSystemState(),th->pattern);
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,getFirstWeekday)
{
	LOG(LOG_NOT_IMPLEMENTED,"DateTimeFormatter.getFirstWeekday is not implemented and always returns 0");
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,getMonthNames)
{
	LOG(LOG_NOT_IMPLEMENTED,"DateTimeFormatter.getMonthNames is not implemented");
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,getTimeStyle)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	ret = asAtomHandler::fromString(wrk->getSystemState(),th->timeStyle);
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,getWeekdayNames)
{
	LOG(LOG_NOT_IMPLEMENTED,"DateTimeFormatter.getWeekdayNames is not implemented");
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,setDateTimeStyles)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	tiny_string dateStyle, timeStyle;
	ARG_CHECK(ARG_UNPACK(dateStyle)(timeStyle));

	if (dateStyle == "long" || dateStyle == "medium" ||
		dateStyle == "short" || dateStyle == "none")
	{
		th->dateStyle = dateStyle;
	}
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"dateStyle value is not valid");
		return;
	}
	if (timeStyle == "long" || timeStyle == "medium" ||
		timeStyle == "short" || timeStyle == "none")
	{
		th->timeStyle = timeStyle;
	}
	else
	{
		createError<ArgumentError>(wrk,kInvalidArgumentError,"timeStyle value is not valid");
	}
}

tiny_string DateTimeFormatter::buildDateTimePattern(tiny_string dateStyle, tiny_string timeStyle)
{
	tiny_string pattern;
	if (timeStyle == "long" || timeStyle == "medium")
	{
		pattern = pattern + "YYYY-mm-dd p";
	}
	else if (dateStyle == "short")
	{
		pattern = pattern + "YYYY-mm-dd p";
	}

	if (timeStyle == "long" || timeStyle == "medium")
	{
		pattern = pattern + " hh:MM:ss";
	}
	else if (timeStyle == "short")
	{
		pattern = pattern + " hh:MM";
	}
	return pattern;
}

std::string DateTimeFormatter::buildInternalFormat(std::string pattern)
{
	std::vector<std::string> list;
	std::string word = "";

	for (std::string::iterator it = pattern.begin(); it != pattern.end(); ++it)
	{
		if (word == "")
		{
			word.push_back((*it));
		}
		else
		{
			if (word[0] == (*it))
			{
				word.push_back((*it));
			}
			else
			{
				list.push_back(word);
				word = (*it);
			}
		}
	}
	list.push_back(word);

	bool quatation = false;

	for (std::vector<std::string>::iterator it = list.begin(); it != list.end(); ++it)
	{
		std::string item = (*it);

		if (item == "'")
		{
			(*it) = "";
			quatation = !quatation;
		}
		else if (quatation)
		{
			// Do nothing here as we don't convert literal strings
		}
		else if (item == "%")
		{
			(*it) = "%%";
		}
		else if (item == "yyyy" || item == "yyyyy" || item == "YYYY" || item == "YYYYY" || item == "y" || item == "yy")
		{
			(*it) = "%y";	// Not correct but will do, returns something like "98"
		}
		else if (item == "M")
		{
			(*it) = "%m";	// Not correct but will do, value is padded with 0
		}
		else if (item == "MM")
		{
			(*it) = "%m";
		}
		else if (item == "MMM")
		{
			(*it) = "%b";
		}
		else if (item == "MMMM" || item == "MMMMM")
		{
			(*it) = "%B";
		}
		else if (item == "d")
		{
			(*it) = "%e";
		}
		else if (item == "dd")
		{
			(*it) = "%d";
		}
		else if (item == "a" || item == "p")
		{
			(*it) = "%p";
		}
		else if (item == "H" || item == "HH"|| item == "HHH")
		{
			(*it) = "%H";	// Not correct but will do, value is padded with 0
		}
		else if (item == "m" || item == "mm")
		{
			(*it) = "%M";	// Not correct but will do, value is padded with 0
		}
		else if (item == "s")
		{
			(*it) = "%S";
		}
		else if (item == "ss")
		{
			(*it) = "%S";
		}
		else if (item == "E" || item == "EE" || item == "EEE")
		{
			(*it) = "%a";
		}
		else if (item == "EEEE" || item == "EEEEE")
		{
			(*it) = "%A";
		}
		else if (item == "h")
		{
			(*it) = "%l"; // Not correct but will do, value is padded with 0
		}
		else if (item == "hh" || item == "hhh")
		{
			(*it) = "%I";
		}
		else if (item == "K")
		{
			(*it) = "%l";
		}
		else if (item == "KK")
		{
			(*it) = "%I";
		}
		else if (item == "k")
		{
			(*it) = "%k";
		}
		else if (item == "kk")
		{
			(*it) = "%H";
		}
		else if (item == "G" ||item == "GG" || item == "GGG" || item == "GGGG" || item == "GGGGG")
		{
			// Do nothing. Not supported.
		}
		else
		{
			// Specifiers below are empty in Flash even though
			// AS3 documentation says the opposite
			std::vector<std::string> unusedTokens;
			unusedTokens.push_back("S");
			unusedTokens.push_back("SS");
			unusedTokens.push_back("SSS");
			unusedTokens.push_back("SSSS");
			unusedTokens.push_back("SSSSS");
			unusedTokens.push_back("Z");
			unusedTokens.push_back("ZZ");
			unusedTokens.push_back("ZZZ");
			unusedTokens.push_back("ZZZZ");
			unusedTokens.push_back("z");
			unusedTokens.push_back("zz");
			unusedTokens.push_back("zzz");
			unusedTokens.push_back("zzzz");
			unusedTokens.push_back("F");
			unusedTokens.push_back("Q");
			unusedTokens.push_back("QQ");
			unusedTokens.push_back("QQQ");
			unusedTokens.push_back("vvvv");
			unusedTokens.push_back("v");
			unusedTokens.push_back("Q");
			unusedTokens.push_back("QQ");
			unusedTokens.push_back("QQQ");
			unusedTokens.push_back("QQQQ");
			unusedTokens.push_back("w");
			unusedTokens.push_back("ww");
			unusedTokens.push_back("W");
			unusedTokens.push_back("WW");
			unusedTokens.push_back("D");
			unusedTokens.push_back("DD");
			unusedTokens.push_back("DDD");

			for (std::vector<std::string>::iterator t = unusedTokens.begin(); t != unusedTokens.end(); ++t)
			{
				if (item == (*t))
				{
					(*it) = "";
				}
			}
		}
	}

	std::string output = "";
	for (std::vector<std::string>::iterator it = list.begin(); it != list.end(); ++it)
	{
		output += (*it);
	}
	return output;
}
