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

#include "scripting/flash/globalization/flashglobalization.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Date.h"
#include <iomanip>

using namespace lightspark;

DateTimeFormatter::DateTimeFormatter(Class_base* c):
	ASObject(c)
{
}

void DateTimeFormatter::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);
	REGISTER_GETTER(c, actualLocaleIDName);
	REGISTER_GETTER(c, lastOperationStatus);
	REGISTER_GETTER(c, requestedLocaleIDName);
	c->setDeclaredMethodByQName("setDateTimePattern","",Class<IFunction>::getFunction(c->getSystemState(),setDateTimePattern),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("format","",Class<IFunction>::getFunction(c->getSystemState(),format),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("formatUTC","",Class<IFunction>::getFunction(c->getSystemState(),formatUTC),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,_constructor)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	ARG_UNPACK_ATOM(th->requestedLocaleIDName)(th->dateStyle,"long")(th->timeStyle,"long");
	try
	{
		th->currlocale = std::locale(th->requestedLocaleIDName.raw_buf());
		th->actualLocaleIDName = th->requestedLocaleIDName;
		th->lastOperationStatus="NO_ERROR";
	}
	catch (std::runtime_error& e)
	{
		uint32_t pos = th->requestedLocaleIDName.find("-");
		if(pos != tiny_string::npos)
		{
			tiny_string r("_");
			tiny_string l = th->requestedLocaleIDName.replace(pos,1,r);
			try
			{
				// try with "_" instead of "-"
				th->currlocale = std::locale(l.raw_buf());
				th->actualLocaleIDName = th->requestedLocaleIDName;
				th->lastOperationStatus="NO_ERROR";
			}
			catch (std::runtime_error& e)
			{
				try
				{
					// try appending ".UTF-8"
					l += ".UTF-8";
					th->currlocale = std::locale(l.raw_buf());
					th->actualLocaleIDName = th->requestedLocaleIDName;
					th->lastOperationStatus="NO_ERROR";
				}
				catch (std::runtime_error& e)
				{
					th->lastOperationStatus="USING_DEFAULT_WARNING";
					LOG(LOG_ERROR,"unknown locale:"<<th->requestedLocaleIDName<<" "<<e.what());
				}
			}
		}
		else
		{
			th->lastOperationStatus="USING_DEFAULT_WARNING";
			LOG(LOG_ERROR,"unknown locale:"<<th->requestedLocaleIDName<<" "<<e.what());
		}
	}
}
ASFUNCTIONBODY_GETTER(DateTimeFormatter, actualLocaleIDName);
ASFUNCTIONBODY_GETTER(DateTimeFormatter, lastOperationStatus);
ASFUNCTIONBODY_GETTER(DateTimeFormatter, requestedLocaleIDName);

ASFUNCTIONBODY_ATOM(DateTimeFormatter,setDateTimePattern)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	ARG_UNPACK_ATOM(th->pattern);
	th->lastOperationStatus="NO_ERROR";
}


ASFUNCTIONBODY_ATOM(DateTimeFormatter,format)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	_NR<Date> dt;
	ARG_UNPACK_ATOM(dt);
	tiny_string res;
	if (!dt.isNull())
	{
		LOG(LOG_NOT_IMPLEMENTED,"DateTimeFormatter.format is not really tested for all formats");
		std::locale l =  std::locale::global(th->currlocale);
		res = dt->format(th->pattern.raw_buf(),false);
		std::locale::global(l);
	}
	ret = asAtomHandler::fromString(sys,res);
}

ASFUNCTIONBODY_ATOM(DateTimeFormatter,formatUTC)
{
	DateTimeFormatter* th =asAtomHandler::as<DateTimeFormatter>(obj);
	_NR<Date> dt;
	ARG_UNPACK_ATOM(dt);
	tiny_string res;
	if (!dt.isNull())
	{
		LOG(LOG_NOT_IMPLEMENTED,"DateTimeFormatter.formatUTC is not really tested for all formats");
		std::locale l =  std::locale::global(th->currlocale);
		res = dt->format(th->pattern.raw_buf(),true);
		std::locale::global(l);
	}
	ret = asAtomHandler::fromString(sys,res);
}

void LastOperationStatus::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED|CLASS_FINAL);
	c->setVariableAtomByQName("BUFFER_OVERFLOW_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bufferOverflowError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ERROR_CODE_UNKNOWN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"errorCodeUnknown"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("ILLEGAL_ARGUMENT_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"illegalArgumentError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INDEX_OUT_OF_BOUNDS_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"indexOutOfBoundsError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVALID_ATTR_VALUE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invalidAttrValue"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVALID_CHAR_FOUND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invalidCharFound"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MEMORY_ALLOCATION_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"memoryAllocationError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NO_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"noError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NUMBER_OVERFLOW_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"numberOverflowError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PARSE_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"parseError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PATTERN_SYNTAX_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"patternSyntaxError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PLATFORM_API_FAILED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"platformAPIFailed"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TRUNCATED_CHAR_FOUND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"truncatedCharFound"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNEXPECTED_TOKEN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"unexpectedToken"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNSUPPORTED_ERROR",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"unsupportedError"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("USING_DEFAULT_WARNING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"usingDefaultWarning"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("USING_FALLBACK_WARNING",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"usingFallbackWarning"),CONSTANT_TRAIT);
}

void DateTimeStyle::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED|CLASS_FINAL);
	
	c->setVariableAtomByQName("CUSTOM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"custom"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("LONG",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"long"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MEDIUM",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"medium"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NONE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"none"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("SHORT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"short"),CONSTANT_TRAIT);
}


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
}
ASFUNCTIONBODY_GETTER(NumberFormatter, actualLocaleIDName);
ASFUNCTIONBODY_GETTER(NumberFormatter, lastOperationStatus);
ASFUNCTIONBODY_GETTER(NumberFormatter, requestedLocaleIDName);
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, fractionalDigits);

ASFUNCTIONBODY_ATOM(NumberFormatter,_constructor)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
	ARG_UNPACK_ATOM(th->requestedLocaleIDName);
	try
	{
		th->currlocale = std::locale(th->requestedLocaleIDName.raw_buf());
		th->actualLocaleIDName = th->requestedLocaleIDName;
		th->lastOperationStatus="NO_ERROR";
	}
	catch (std::runtime_error& e)
	{
		uint32_t pos = th->requestedLocaleIDName.find("-");
		if(pos != tiny_string::npos)
		{
			tiny_string r("_");
			tiny_string l = th->requestedLocaleIDName.replace(pos,1,r);
			try
			{
				// try with "_" instead of "-"
				th->currlocale = std::locale(l.raw_buf());
				th->actualLocaleIDName = th->requestedLocaleIDName;
				th->lastOperationStatus="NO_ERROR";
			}
			catch (std::runtime_error& e)
			{
				try
				{
					// try appending ".UTF-8"
					l += ".UTF-8";
					th->currlocale = std::locale(l.raw_buf());
					th->actualLocaleIDName = th->requestedLocaleIDName;
					th->lastOperationStatus="NO_ERROR";
				}
				catch (std::runtime_error& e)
				{
					th->lastOperationStatus="USING_DEFAULT_WARNING";
					LOG(LOG_ERROR,"unknown locale:"<<th->requestedLocaleIDName<<" "<<e.what());
				}
			}
		}
		else
		{
			th->lastOperationStatus="USING_DEFAULT_WARNING";
			LOG(LOG_ERROR,"unknown locale:"<<th->requestedLocaleIDName<<" "<<e.what());
		}
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
