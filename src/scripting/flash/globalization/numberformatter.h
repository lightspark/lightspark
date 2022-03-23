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

#ifndef SCRIPTING_FLASH_GLOBALIZATION_NUMBERFORMATTER_H
#define SCRIPTING_FLASH_GLOBALIZATION_NUMBERFORMATTER_H 1

#include "asobject.h"
#include <clocale>

namespace lightspark
{

class NumberFormatter : public ASObject
{
private:
	std::locale currlocale;
public:
	NumberFormatter(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(formatNumber);
	ASFUNCTION_ATOM(formatInt);
	ASFUNCTION_ATOM(formatUint);
	ASFUNCTION_ATOM(parse);
	ASFUNCTION_ATOM(parseNumber);
	ASFUNCTION_ATOM(getAvailableLocaleIDNames);
	ASPROPERTY_GETTER(tiny_string, actualLocaleIDName);
	ASPROPERTY_GETTER(tiny_string, lastOperationStatus);
	ASPROPERTY_GETTER(tiny_string, requestedLocaleIDName);
	ASPROPERTY_GETTER_SETTER(int, fractionalDigits);
	ASPROPERTY_GETTER_SETTER(tiny_string, decimalSeparator);
	ASPROPERTY_GETTER_SETTER(uint32_t, digitsType);
	ASPROPERTY_GETTER_SETTER(tiny_string, groupingPattern);
	ASPROPERTY_GETTER_SETTER(tiny_string, groupingSeparator);
	ASPROPERTY_GETTER_SETTER(bool, leadingZero);
	ASPROPERTY_GETTER_SETTER(uint32_t, negativeNumberFormat);
	ASPROPERTY_GETTER_SETTER(tiny_string, negativeSymbol);
	ASPROPERTY_GETTER_SETTER(bool, trailingZeros);
	ASPROPERTY_GETTER_SETTER(bool, useGrouping);
};

}
#endif /* SCRIPTING_FLASH_GLOBALIZATION_NUMBERFORMATTER_H */
