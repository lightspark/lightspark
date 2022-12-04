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
#include "scripting/toplevel/Number.h"
#include "scripting/flash/globalization/numberparseresult.h"

using namespace lightspark;

NumberFormatter::NumberFormatter(ASWorker* wrk, Class_base* c):
	ASObject(wrk,c)
{
}

void NumberFormatter::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);
	REGISTER_GETTER(c, actualLocaleIDName);
	REGISTER_GETTER(c, lastOperationStatus);
	REGISTER_GETTER(c, requestedLocaleIDName);
	REGISTER_GETTER_SETTER(c, fractionalDigits);
	REGISTER_GETTER_SETTER(c, decimalSeparator);
	REGISTER_GETTER_SETTER(c, digitsType);
	REGISTER_GETTER_SETTER(c, groupingPattern);
	REGISTER_GETTER_SETTER(c, groupingSeparator);
	REGISTER_GETTER_SETTER(c, leadingZero);
	REGISTER_GETTER_SETTER(c, negativeNumberFormat);
	REGISTER_GETTER_SETTER(c, negativeSymbol);
	REGISTER_GETTER_SETTER(c, trailingZeros);
	REGISTER_GETTER_SETTER(c, useGrouping);
	c->setDeclaredMethodByQName("formatInt","",Class<IFunction>::getFunction(c->getSystemState(),formatInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("formatUint","",Class<IFunction>::getFunction(c->getSystemState(),formatUint),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("formatNumber","",Class<IFunction>::getFunction(c->getSystemState(),formatNumber),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parse","",Class<IFunction>::getFunction(c->getSystemState(),parse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parseNumber","",Class<IFunction>::getFunction(c->getSystemState(),parseNumber),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getAvailableLocaleIDNames","",Class<IFunction>::getFunction(c->getSystemState(),formatNumber),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_GETTER(NumberFormatter, actualLocaleIDName)
ASFUNCTIONBODY_GETTER(NumberFormatter, lastOperationStatus)
ASFUNCTIONBODY_GETTER(NumberFormatter, requestedLocaleIDName)
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, fractionalDigits)
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, decimalSeparator)
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, digitsType)
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, groupingPattern)
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, groupingSeparator)
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, leadingZero)
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, negativeNumberFormat)
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, negativeSymbol)
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, trailingZeros)
ASFUNCTIONBODY_GETTER_SETTER(NumberFormatter, useGrouping)

ASFUNCTIONBODY_ATOM(NumberFormatter,_constructor)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
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

	std::locale l =  std::locale::global(th->currlocale);

	char localSeparatorChar = std::use_facet< std::numpunct<char> >(std::cout.getloc()).thousands_sep();
	char localDecimalChar = std::use_facet< std::numpunct<char> >(std::cout.getloc()).decimal_point();

	th->fractionalDigits = 2;
	th->decimalSeparator = std::to_string(localDecimalChar);
	th->digitsType = false;
	th->groupingSeparator = std::to_string(localSeparatorChar);
	th->negativeNumberFormat = 1;
	th->negativeSymbol = "-";
	th->useGrouping = false;
	std::locale::global(l);
}

ASFUNCTIONBODY_ATOM(NumberFormatter,formatInt)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
	if (th->digitsType)
	{
		LOG(LOG_NOT_IMPLEMENTED,"NumberFormatter.digitsType is not implemented");
	}
	LOG(LOG_NOT_IMPLEMENTED,"NumberFormatter.formatInt is not really tested for all formats");
	
	int32_t value;
	ARG_CHECK(ARG_UNPACK(value));
	tiny_string res;
	if (value > 1.72e308)
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"0");
		return;
	}
	std::stringstream ss;
	ss.imbue(th->currlocale);
	res = ss.str();
	if (res == "nan")
	{
		res = "NaN";
	}
	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
	th->lastOperationStatus="noError";
}

ASFUNCTIONBODY_ATOM(NumberFormatter,formatUint)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
	if (th->digitsType)
	{
		LOG(LOG_NOT_IMPLEMENTED,"NumberFormatter.digitsType is not implemented");
	}

	LOG(LOG_NOT_IMPLEMENTED,"NumberFormatter.formatUint is not really tested for all formats");
	
	uint32_t value;
	ARG_CHECK(ARG_UNPACK(value));
	tiny_string res;
	if (value > 1.72e308)
	{
		ret = asAtomHandler::fromString(wrk->getSystemState(),"0");
		return;
	}
	std::stringstream ss;
	//ss.imbue(th->currlocale);
	res = ss.str();
	if (res == "nan")
	{
		res = "NaN";
	}
	th->lastOperationStatus="noError";
	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
}

ASFUNCTIONBODY_ATOM(NumberFormatter,parse)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
	LOG(LOG_NOT_IMPLEMENTED,"NumberFormatter.parse is fully tested and implemented");
	NumberParseResult* npr = Class<NumberParseResult>::getInstanceS(wrk);

	tiny_string parseString;
	ARG_CHECK(ARG_UNPACK(parseString));

	std::locale l =  std::locale::global(th->currlocale);

	if (parseString.raw_buf() == nullptr)
	{
		createError<TypeError>(wrk,kNullArgumentError);
		return;
	}

	std::string stringToParse = parseString;

	bool negativeSymbolFound = false;
	std::string decimalSeparatorFormat = th->decimalSeparator;
	char decimalSeparator = ' ';
	if (decimalSeparatorFormat.size() == 1)
	{
		decimalSeparator = decimalSeparatorFormat.at(0);
	}
	
	if (stringToParse.size() == 0)
	{
		npr->endIndex = 0;
		npr->startIndex = 0;
		npr->value = 0;
		ret = asAtomHandler::fromObject(npr);
		return;
	}

	int32_t currIndex = 0;

	npr->startIndex = currIndex;
	npr->endIndex = currIndex;

	std::string output = "";

	std::string::iterator it = stringToParse.begin();

	bool found = false;

    if ((*it) == '(' && th->negativeNumberFormat == 0)
    {
        // Parse 0 negative number format
        npr->startIndex = currIndex;
        it++;
        currIndex++;

        output += "-";

        // Accept space
        if ((*it) == ' ')
        {
            it++;
            currIndex++;

            while (it != stringToParse.end())
            {
                if (std::isdigit((*it)) || (*it) == '-' || (*it) == decimalSeparator || (*it) == ',')
                {
                    output += (*it);
                    npr->endIndex = currIndex;
                    if (!found)
                    {
                        found = true;
                    }
                }
                it++;
                currIndex++;
            }
        }

        while (it != stringToParse.end())
        {
            if ((*it) == ' ')
            {
                npr->endIndex = currIndex;
            }
            else if ((*it) == '-')
            {
                output += (*it);
            }
            else if (std::isdigit((*it)) || (*it) == decimalSeparator || (*it) == ',')
            {
                output += (*it);
                npr->endIndex = currIndex;
                if (!found)
                {
                    found = true;
                }
            }
            else if (found)
            {
                break;
            }

            npr->endIndex = currIndex;
            it++;
            currIndex++;
        }

        // Accept space
        if ((*it) == ' ')
        {
            it++;
            currIndex++;

            if ((*it) == ')')
            {
                it++;
                currIndex++;
            }
        }
        else if ((*it) == ')')
        {
            it++;
            currIndex++;
        }
        npr->endIndex = currIndex;
    }
    else
    {
        while (it != stringToParse.end())
        {
            npr->endIndex = currIndex;

            char current = (char)(*it);

            string prev = "";
            string next = "";
            if (it != stringToParse.end())
            {
                it++;
                next += (*it);
                it--;
            }
            if (it != stringToParse.begin())
            {
                it--;
                prev += (*it);
                it++;
            }

            if (current == ' ')
            {
                npr->endIndex = currIndex;
            }
            else if (current == '-')
            {
                npr->endIndex = currIndex;
                if (th->negativeNumberFormat == 1)
                {
                    if (!found)
                    {
                        output = "-" + output;
                    }
                    if (!negativeSymbolFound && !negativeSymbolFound)
                    {
                        npr->startIndex = currIndex;
                    }
                    negativeSymbolFound = true;
                }
                if (th->negativeNumberFormat == 2 && !negativeSymbolFound && next == " ")
                {
                    output = "-" + output;
                    it++;
                    currIndex++;
                    npr->endIndex = currIndex;
                    negativeSymbolFound = true;
                }
                else if (th->negativeNumberFormat == 3 && !negativeSymbolFound)
                {
                    if (found)
                    {
                        output = "-" + output;
                    }
                    negativeSymbolFound = true;
                }
                else if (th->negativeNumberFormat == 4 && !negativeSymbolFound)
                {
                    if (found)
                    {
                        output = "-" + output;
                    }
                    negativeSymbolFound = true;
                }

            }
            else if (std::isdigit(current) || (*it) == decimalSeparator || (current == ','))
            {
                found = true;
                output += (*it);

                npr->endIndex = currIndex;

                if (!found)
                {
                    npr->startIndex = currIndex;
                    found = true;
                }
            }
            it++;
            currIndex++;
        }
    }

    // Trim text
    output.erase(0, output.find_first_not_of(' '));
    output.erase(output.find_last_not_of(' ') + 1);

    if (output == "")
    {
        npr->endIndex = stringToParse.size();
    }

	stringstream ss(output);
	ss.imbue(th->currlocale);
	number_t num;
	if((ss >> num).fail())
	{
        npr->value = Number::NaN;
        npr->startIndex = 0x7fffffff;
        npr->endIndex = 0x7fffffff;
	}
	else
	{
		th->lastOperationStatus="noError";
        ret = asAtomHandler::fromNumber(wrk,num, true);
	}
    npr->value = num;
    ret = asAtomHandler::fromObject(npr);
    std::locale::global(l);
}

ASFUNCTIONBODY_ATOM(NumberFormatter,parseNumber)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
	if (th->digitsType)
	{
        LOG(LOG_NOT_IMPLEMENTED,"NumberFormatter.digitsType is not implemented");
	}

    LOG(LOG_NOT_IMPLEMENTED,"NumberFormatter.parseNumber is fully not tested and implemented");

	tiny_string parseString;
	ARG_CHECK(ARG_UNPACK(parseString));

    if (parseString.raw_buf() == nullptr)
    {
        createError<TypeError>(wrk,kNullArgumentError);
		return;
    }

	std::locale l =  std::locale::global(th->currlocale);
	char localSeparatorChar = std::use_facet< std::numpunct<char> >(std::cout.getloc()).thousands_sep();

	std::string stringToParse = parseString;

	// Replace a few characters before parsing
	size_t pos;
	while ((pos = stringToParse.find(localSeparatorChar)) != std::string::npos)
	{
        stringToParse.replace(pos, 1, "");
    }

	stringstream ss(stringToParse);
	ss.imbue(th->currlocale);
	number_t num;
	if((ss >> num).fail())
	{
	}
	else
	{
		th->lastOperationStatus="noError";
		ret = asAtomHandler::fromNumber(wrk,num, true);
	}
    std::locale::global(l);
}

ASFUNCTIONBODY_ATOM(NumberFormatter,formatNumber)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
	if (th->digitsType)
	{
        LOG(LOG_NOT_IMPLEMENTED,"NumberFormatter.digitsType is not implemented");
	}
	
    std::locale l =  std::locale::global(th->currlocale);
	number_t value;
	ARG_CHECK(ARG_UNPACK(value));
	std::string number;

	std::stringstream ss;
	ss.imbue(th->currlocale);

	ss << std::setprecision(th->fractionalDigits) << std::fixed << value;
	number = ss.str();

	char localSeparatorChar = std::use_facet< std::numpunct<char> >(std::cout.getloc()).thousands_sep();
	char localDecimalChar = std::use_facet< std::numpunct<char> >(std::cout.getloc()).decimal_point();

	// Apply group pattern if enabled, replaces locale
	if (th->useGrouping)
	{
		// std::numpunct::grouping, do_grouping does not do exaclty what we want
		// so we implement a custom group pattern formatter
		if (th->groupingPattern == "")
		{
			th->lastOperationStatus = "fail"; // Replace this!!!
			return;
		}

		// Check grouping pattern format
		for (std::string::iterator it = number.begin(); it != number.end(); it++)
		{
			if (!std::isdigit((*it)) && (*it) != ';' && (*it) != '*')
			{
				th->lastOperationStatus = "fail"; // Replace this!!!
				return;
			}
		}

		// Remove "*" as it's optional
		size_t commaPos;
		while ((commaPos = number.find("*")) != std::string::npos)
		{
	        number.replace(commaPos, 1, "");
	    }

		// Find start position to where we will start grouping
		size_t decimalPos = 0;
		decimalPos = number.find(localDecimalChar);
		if (decimalPos != std::string::npos)
		{
			decimalPos = number.size();
		}

		uint32_t currentGrouping = 0;
		const char delim = ';';
		std::stringstream ss2(number);
		std::string s;

		std::string groupNumberResult;

		// Get current grouping value from groupingPattern
		if (std::getline(ss2, s, delim))
		{
			if (s != "")
			{
				currentGrouping = std::stoi(s);
			}
		}

		// Insert decimal places and dot
		for (int32_t i = 0; number.size() - decimalPos; i++)
		{
			groupNumberResult.insert(0, 1, number[i]);
		}

		int32_t pos = decimalPos;
		while (pos >= 0)
		{
			// Insert number group onto groupNumberResult
			uint32_t i = 0;
			while (pos >= 0 && i < currentGrouping)
			{
				groupNumberResult.insert(0, 1, number[i]);
				i++;
				pos--;
			}
			groupNumberResult.insert(0, 1, localSeparatorChar);

			// Get next grouping value from groupingPattern
			if (std::getline(ss2, s, delim))
			{
				if (s != "")
				{
					currentGrouping = std::stoi(s);
				}
			}
		}

		// Remove any commas at start of number
		if (groupNumberResult[0] == localSeparatorChar)
		{
			groupNumberResult.replace(0, 1, "");
		}

		// Replace number value
		number = groupNumberResult;
	}

	// Replace decimalSeparator in output
	if (th->decimalSeparator != std::to_string(localDecimalChar))
	{
		size_t pos;
		while ((pos = number.find(localDecimalChar)) != std::string::npos)
		{
	        number.replace(pos, 1, th->decimalSeparator);
	    }
	}

	// Replace groupingSeparator in output
	if (th->groupingSeparator != std::to_string(localSeparatorChar))
	{
		size_t pos;
		while ((pos = number.find(localSeparatorChar)) != std::string::npos)
		{
	        number.replace(pos, 1, th->decimalSeparator);
	    }
	}

	// Pad number with zeros if trailingZeros is true
	if (th->trailingZeros)
	{
		// Find dot position
		size_t decimalPos = 0;
		decimalPos = number.find(localDecimalChar);
		if (decimalPos != std::string::npos)
		{
			decimalPos = number.size();
		}

		// Pad number with zeros if possible
		int index = number.size() - decimalPos;
		if (th->fractionalDigits > index)
		{
			for (int i = 0; i < index; i++)
			{
				number.push_back('0');
			}
		}
	}

	if (th->leadingZero)
	{
		// TODO: To implement
	}
	else
	{
		if (number[0] == localDecimalChar)
		{
			number.insert(0, 1, '0');
		}
	}

	// Replace negative symbol and negative symbol format
	if (value < 0)
	{
		// The documentation says that the negativeSymbolFormat can be differnet
		// ... this is not the case as seen by in testings
		std::string minusSymbol = "-";

		// Remove negativeSymbol so we can replace it with negativeNumberFormat
		size_t pos;
		while ((pos = number.find("-")) != std::string::npos)
		{
			number.replace(pos, 1, "");
		}

		// Add negative symbol back in with negativeNumberFormat
		switch (th->negativeNumberFormat)
		{
			case 0:
				number = "(" + number + ")";
				break;
			case 1:
				number = minusSymbol + number;
				break;
			case 2:
				number = minusSymbol + " " + number;
				break;
			case 3:
				number = number + minusSymbol;
				break;
			case 4:
				number = number + " " + minusSymbol;
				break;
		}
	}
	if (number == "nan")
	{
		number = "NaN";
	}
    ret = asAtomHandler::fromString(wrk->getSystemState(),number);
	th->lastOperationStatus="noError";
    std::locale::global(l);
}

ASFUNCTIONBODY_ATOM(NumberFormatter,getAvailableLocaleIDNames)
{
	NumberFormatter* th =asAtomHandler::as<NumberFormatter>(obj);
	if (th->digitsType)
	{
        LOG(LOG_NOT_IMPLEMENTED,"NumberFormatter.digitsType is not implemented");
	}
	
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
