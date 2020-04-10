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

#include "scripting/flash/globalization/currencyformatter.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace lightspark;

CurrencyFormatter::CurrencyFormatter(Class_base* c):
	ASObject(c)
{
}

void CurrencyFormatter::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);

	REGISTER_GETTER(c, actualLocaleIDName);
	REGISTER_GETTER(c, currencyISOCode);
	REGISTER_GETTER(c, currencySymbol);
	REGISTER_GETTER_SETTER(c, decimalSeparator);
	REGISTER_GETTER_SETTER(c, digitsType);
	REGISTER_GETTER_SETTER(c, fractionalDigits);
	REGISTER_GETTER_SETTER(c, groupingPattern);
	REGISTER_GETTER_SETTER(c, groupingSeparator);
	REGISTER_GETTER(c, lastOperationStatus);
	REGISTER_GETTER_SETTER(c, leadingZero);
	REGISTER_GETTER_SETTER(c, negativeCurrencyFormat);
	REGISTER_GETTER_SETTER(c, negativeSymbol);
	REGISTER_GETTER_SETTER(c, positiveCurrencyFormat);
	REGISTER_GETTER(c, requestedLocaleIDName);
	REGISTER_GETTER_SETTER(c, trailingZeros);
	REGISTER_GETTER_SETTER(c, useGrouping);

	c->setDeclaredMethodByQName("format","",Class<IFunction>::getFunction(c->getSystemState(),format),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("formattingWithCurrencySymbolIsSafe","",Class<IFunction>::getFunction(c->getSystemState(),formattingWithCurrencySymbolIsSafe),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getAvailableLocaleIDNames","",Class<IFunction>::getFunction(c->getSystemState(),getAvailableLocaleIDNames),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parse","",Class<IFunction>::getFunction(c->getSystemState(),parse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setCurrency","",Class<IFunction>::getFunction(c->getSystemState(),setCurrency),NORMAL_METHOD,true);

	c->setVariableAtomByQName("LocaleID.DEFAULT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"LocaleID.DEFAULT"),CONSTANT_TRAIT);
}

ASFUNCTIONBODY_ATOM(CurrencyFormatter,_constructor)
{
	CurrencyFormatter* th =asAtomHandler::as<CurrencyFormatter>(obj);

	th->fractionalDigits = 0;

	ARG_UNPACK_ATOM(th->requestedLocaleIDName);
	try
	{
		th->currlocale = std::locale(th->requestedLocaleIDName.raw_buf());
		th->actualLocaleIDName = th->requestedLocaleIDName;
		th->lastOperationStatus="noError";
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
				th->lastOperationStatus="noError";
			}
			catch (std::runtime_error& e)
			{
				try
				{
					// try appending ".UTF-8"
					l += ".UTF-8";
					th->currlocale = std::locale(l.raw_buf());
					th->actualLocaleIDName = th->requestedLocaleIDName;
					th->lastOperationStatus="noError";
				}
				catch (std::runtime_error& e)
				{
					th->lastOperationStatus="usingDefaultWarning";
					LOG(LOG_ERROR,"unknown locale:"<<th->requestedLocaleIDName<<" "<<e.what());
				}
			}
		}
		else
		{
			try
			{
				// try appending ".UTF-8"
				th->requestedLocaleIDName += ".UTF-8";
				th->currlocale = std::locale(th->requestedLocaleIDName.raw_buf());
				th->actualLocaleIDName = th->requestedLocaleIDName;
				th->lastOperationStatus="noError";
			}
			catch (std::runtime_error& e)
			{
				th->lastOperationStatus="usingDefaultWarning";
				LOG(LOG_ERROR,"unknown locale:"<<th->requestedLocaleIDName<<" "<<e.what());
			}
		}
	}
}

ASFUNCTIONBODY_GETTER(CurrencyFormatter, actualLocaleIDName);
ASFUNCTIONBODY_GETTER(CurrencyFormatter, currencyISOCode);
ASFUNCTIONBODY_GETTER(CurrencyFormatter, currencySymbol);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, decimalSeparator);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, digitsType);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, fractionalDigits);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, groupingPattern);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, groupingSeparator);
ASFUNCTIONBODY_GETTER(CurrencyFormatter, lastOperationStatus);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, leadingZero);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, negativeCurrencyFormat);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, negativeSymbol);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, positiveCurrencyFormat);
ASFUNCTIONBODY_GETTER(CurrencyFormatter, requestedLocaleIDName);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, trailingZeros);
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, useGrouping);

ASFUNCTIONBODY_ATOM(CurrencyFormatter,format)
{
		LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.format is not really tested for all formats");
		CurrencyFormatter* th =asAtomHandler::as<CurrencyFormatter>(obj);
		double value;
		bool withCurrencySymbol = false; // withCurrencySymbol argument not implemented
		std::stringstream res;
		ARG_UNPACK_ATOM(value);
		value *= 100;
		res.imbue(th->currlocale);
		res << std::showbase << std::setprecision(th->fractionalDigits) << std::put_money(value,!withCurrencySymbol) << std::fixed;
		ret = asAtomHandler::fromString(sys,res.str());
}

ASFUNCTIONBODY_ATOM(CurrencyFormatter,formattingWithCurrencySymbolIsSafe)
{
	LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.formattingWithCurrencySymbolIsSafe is not implemented.");
}

ASFUNCTIONBODY_ATOM(CurrencyFormatter,getAvailableLocaleIDNames)
{
	LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.getAvailableLocaleIDNames is not implemented.");
	// Returns a vector of supported LocateIDNames.
}

ASFUNCTIONBODY_ATOM(CurrencyFormatter,parse)
{
	LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.parse is not implemented.");
}

ASFUNCTIONBODY_ATOM(CurrencyFormatter,setCurrency)
{
	LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.setCurrency is not tested.");
}
