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
#include "backends/locale.h"
#include "backends/currency.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "currencyparseresult.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Number.h"
#include <string>

using namespace lightspark;

CurrencyFormatter::CurrencyFormatter(ASWorker* wrk, Class_base* c):
	ASObject(wrk,c)
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

	c->setDeclaredMethodByQName("format","",c->getSystemState()->getBuiltinFunction(format),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("formattingWithCurrencySymbolIsSafe","",c->getSystemState()->getBuiltinFunction(formattingWithCurrencySymbolIsSafe),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getAvailableLocaleIDNames","",c->getSystemState()->getBuiltinFunction(getAvailableLocaleIDNames),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parse","",c->getSystemState()->getBuiltinFunction(parse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setCurrency","",c->getSystemState()->getBuiltinFunction(setCurrency),NORMAL_METHOD,true);

	c->setVariableAtomByQName("LocaleID.DEFAULT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"LocaleID.DEFAULT"),CONSTANT_TRAIT);


}

ASFUNCTIONBODY_ATOM(CurrencyFormatter,_constructor)
{
	CurrencyFormatter* th =asAtomHandler::as<CurrencyFormatter>(obj);
	th->fractionalDigits = 0;
	ARG_CHECK(ARG_UNPACK(th->requestedLocaleIDName));

	std::string localeId = th->requestedLocaleIDName;
	std::string isoCurrencyCode = "";
	if (localeId.size() > 4)
	{
		isoCurrencyCode = localeId.substr(3, localeId.size());
	}

	if (wrk->getSystemState()->localeManager->isLocaleAvailableOnSystem(th->requestedLocaleIDName))
	{
		LOG(LOG_INFO,"unknown locale:"<<th->requestedLocaleIDName);
		th->lastOperationStatus="usingDefaultWarning";
	}

	std::string localeName = wrk->getSystemState()->localeManager->getSystemLocaleName(th->requestedLocaleIDName);
	th->currlocale = std::locale(localeName.c_str());
	th->actualLocaleIDName = th->requestedLocaleIDName;
	th->lastOperationStatus="noError";

	std::string currencyIsoCode = wrk->getSystemState()->currencyManager->getCountryISOSymbol(isoCurrencyCode);
	std::string currencySymbol = wrk->getSystemState()->currencyManager->getCurrencySymbol(currencyIsoCode);

	th->currencyISOCode = currencyIsoCode;
	th->currencySymbol = currencySymbol;
	std::locale l =  std::locale::global(th->currlocale);

	char localSeparatorChar = std::use_facet< std::numpunct<char> >(std::cout.getloc()).thousands_sep();
	char localDecimalChar = std::use_facet< std::numpunct<char> >(std::cout.getloc()).decimal_point();

	std::string localDecimalCharString(1, localDecimalChar);
	std::string localSeparatorCharString(1, localSeparatorChar);

	th->fractionalDigits = 2;
	th->decimalSeparator = localDecimalCharString;
	th->digitsType = false;
	th->groupingSeparator = localSeparatorCharString;
	th->negativeCurrencyFormat = 1;
	th->negativeSymbol = "-";
	th->useGrouping = false;
	std::locale::global(l);
}

ASFUNCTIONBODY_GETTER(CurrencyFormatter, actualLocaleIDName)
ASFUNCTIONBODY_GETTER(CurrencyFormatter, currencyISOCode)
ASFUNCTIONBODY_GETTER(CurrencyFormatter, currencySymbol)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, decimalSeparator)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, digitsType)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, fractionalDigits)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, groupingPattern)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, groupingSeparator)
ASFUNCTIONBODY_GETTER(CurrencyFormatter, lastOperationStatus)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, leadingZero)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, negativeCurrencyFormat)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, negativeSymbol)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, positiveCurrencyFormat)
ASFUNCTIONBODY_GETTER(CurrencyFormatter, requestedLocaleIDName)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, trailingZeros)
ASFUNCTIONBODY_GETTER_SETTER(CurrencyFormatter, useGrouping)

ASFUNCTIONBODY_ATOM(CurrencyFormatter,format)
{
	LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.format is not really tested for all formats");
	CurrencyFormatter* th =asAtomHandler::as<CurrencyFormatter>(obj);

	if (th->negativeCurrencyFormat != 0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.negativeCurrencyFormat is not supported");
	}
	if (th->positiveCurrencyFormat != 0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.negativeCurrencyFormat is not supported");
	}
	if (th->useGrouping || th->groupingPattern != "" || th->groupingSeparator != "")
	{
		LOG(LOG_NOT_IMPLEMENTED,"Grouping in CurrencyFormatter is not supported");
	}
	if (th->positiveCurrencyFormat != 0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.positiveCurrencyFormat is not supported");
	}
	if (th->leadingZero != 0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.leadingZero is not supported");
	}
	LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.digitsType is not supported");

	char localDecimalChar = std::use_facet< std::numpunct<char> >(std::cout.getloc()).decimal_point();

	double value;
	bool withCurrencySymbol = false; // withCurrencySymbol argument not implemented
	std::stringstream res;
	ARG_CHECK(ARG_UNPACK(value));
	value *= 100;
	res.imbue(th->currlocale);
	res << std::showbase << std::setprecision(th->fractionalDigits) << std::put_money(value,!withCurrencySymbol) << std::fixed;

	std::string output = res.str();

	// Now we format the output

	// Replace spaces in input, get rid of the spaces generated by put_money
	std::unordered_map<string, string> currencies = wrk->getSystemState()->currencyManager->getCurrencySymbols();
	for (std::unordered_map<string, string>::iterator it = currencies.begin(); it != currencies.end(); ++it)
	{
		std::string key = it->first;
		std::string value = it->second;

		// Find 2 spaces to remove
		std::size_t found = output.find(key + "  ");
		if (found != std::string::npos)
		{
			output.replace(found, key.length()+2, key);
		}

		// Find 1 space to remove
		found = output.find(key + " ");
		if (found != std::string::npos)
		{
			output.replace(found, key.length()+1, key);
		}
	}

	for (std::unordered_map<string, string>::iterator it = currencies.begin(); it != currencies.end(); ++it)
	{
		std::string key = it->first;
		std::string value = it->second;

		// Find 2 spaces to remove
		std::size_t found = output.find(key + "  ");
		if (found != std::string::npos)
		{
			output.replace(found, key.length()+2, key);
		}

		// Find 1 space to remove
		found = output.find(key + " ");
		if (found != std::string::npos)
		{
			output.replace(found, key.length()+1, key);
		}
	}

	// Replace decimal separator
	std::string seperatorCharString = th->decimalSeparator;
	char seperatorChar = seperatorCharString.at(0);
	if (localDecimalChar != seperatorChar)
	{
		std::string a;
		a.push_back(seperatorChar);

		size_t pos;
		while ((pos = output.find(localDecimalChar)) != std::string::npos)
		{
			output.replace(pos, 1, a);
		}
	}

	// Add trailing zeros
	// Find dot position
	size_t decimalPos = 0;
	decimalPos = output.find(localDecimalChar);
	if (decimalPos == std::string::npos)
	{
		decimalPos = output.size();
	}

	// Pad number with zeros if possible
	int index = output.size()-1 - decimalPos;
	if (th->fractionalDigits > index)
	{
		int extraPaddingCount = th->fractionalDigits-index;
		for (int i = 0; i < extraPaddingCount; i++)
		{
			output.push_back('0');
		}
	}

	ret = asAtomHandler::fromString(wrk->getSystemState(),output);
}

ASFUNCTIONBODY_ATOM(CurrencyFormatter,formattingWithCurrencySymbolIsSafe)
{
	CurrencyFormatter* th =asAtomHandler::as<CurrencyFormatter>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.formattingWithCurrencySymbolIsSafe is not really tested for all formats");
	tiny_string requestedISOCode;
	std::string currencyISOCode = th->currencyISOCode;
	ARG_CHECK(ARG_UNPACK(requestedISOCode));

	if (requestedISOCode.raw_buf() == nullptr)
	{
		createError<TypeError>(wrk,kNullArgumentError);
		return;
	}

	bool result = (requestedISOCode == currencyISOCode);
	th->lastOperationStatus = "noError";
	ret = asAtomHandler::fromBool(result);
}

ASFUNCTIONBODY_ATOM(CurrencyFormatter,getAvailableLocaleIDNames)
{
	CurrencyFormatter* th =asAtomHandler::as<CurrencyFormatter>(obj);
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

ASFUNCTIONBODY_ATOM(CurrencyFormatter,parse)
{
	CurrencyFormatter* th =asAtomHandler::as<CurrencyFormatter>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.parse is not really tested for all formats");
	std::string inputString;
	tiny_string input;
	ARG_CHECK(ARG_UNPACK(input));

	if (input.raw_buf() == nullptr)
	{
		createError<TypeError>(wrk,kNullArgumentError);
		return;
	}

	inputString = input;
	
	if (th->negativeCurrencyFormat != 0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.negativeCurrencyFormat is not supported");
	}
	if (th->positiveCurrencyFormat != 0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.negativeCurrencyFormat is not supported");
	}
	if (th->useGrouping || th->groupingPattern != "" || th->groupingSeparator != "")
	{
		LOG(LOG_NOT_IMPLEMENTED,"Grouping in CurrencyFormatter is not supported");
	}
	if (th->positiveCurrencyFormat != 0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.positiveCurrencyFormat is not supported");
	}
	if (th->leadingZero != 0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.leadingZero is not supported");
	}
	LOG(LOG_NOT_IMPLEMENTED,"CurrencyFormatter.digitsType is not supported");

	// Trim input text
	inputString.erase(0, inputString.find_first_not_of(' '));
	inputString.erase(inputString.find_last_not_of(' ') + 1);

	CurrencyParseResult* cpr = Class<CurrencyParseResult>::getInstanceS(wrk);

	std::unordered_map<string, string> currencies = wrk->getSystemState()->currencyManager->getCurrencySymbols();
	for (std::unordered_map<string, string>::iterator it = currencies.begin(); it != currencies.end(); ++it)
	{
		std::string key = it->first;
		std::string value = it->second;

		// Check to see if currency is at start of string
		if (input.find(key) == 0 || input.find(value) == 0)
		{
			cpr->currencyString = key;
		}

		// Check to see if currency is at end of string
		if (input.numChars() > 0 && key.size() > 0)
		{
			if (input.find(key) == input.numChars()-key.size()-2)
			{
				cpr->currencyString = key;
			}
		}
		if (input.numChars() > 0 && value.size() > 0)
		{
			if (input.find(value) == input.numChars()-key.size()-2)
			{
				cpr->currencyString = key;
			}
		}
	}

	std::istringstream in(inputString);
	long double value;
	in.imbue(th->currlocale);
	in >> std::get_money(value);

	std::string output = "";

	if (in)
	{
		std::string value = in.str();

		// Convert text to number
		stringstream ss(value);
		ss.imbue(th->currlocale);
		number_t num;
		if((ss >> num).fail())
		{
			cpr->value = Number::NaN;
		}
		else
		{
			// Set end result
			cpr->value = num;
			th->lastOperationStatus="noError";
		}
	}
	else
	{
		th->lastOperationStatus="parseError";
		ret = asAtomHandler::fromObject(cpr);
		return;
	}

	/*
	 * TODO:
		decimalSeparator
		fractionalDigits
		trailingZeros
	*/

	th->lastOperationStatus="noError";
	ret = asAtomHandler::fromObject(cpr);
}

ASFUNCTIONBODY_ATOM(CurrencyFormatter,setCurrency)
{
	CurrencyFormatter* th =asAtomHandler::as<CurrencyFormatter>(obj);
	ARG_CHECK(ARG_UNPACK(th->currencyISOCode)(th->currencySymbol));
	th->lastOperationStatus="noError";
}
