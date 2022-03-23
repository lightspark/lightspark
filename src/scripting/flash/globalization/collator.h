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

#ifndef SCRIPTING_FLASH_GLOBALIZATION_COLLATOR_H
#define SCRIPTING_FLASH_GLOBALIZATION_COLLATOR_H 1

#include "asobject.h"

namespace lightspark
{

class Collator : public ASObject
{
private:
	std::locale currlocale;
	tiny_string initialMode;
	tiny_string sortingMode;
public:
	int compare(std::string string1,
		std::string string2,
		bool ignoreCase,
		bool ignoreCharacterWidth,
		bool ignoreDiacritics,
		bool ignoreKanaType,
		bool ignoreSymbols);
	bool equals(std::string string1,
		std::string string2,
		bool ignoreCase,
		bool ignoreCharacterWidth,
		bool ignoreDiacritics,
		bool ignoreKanaType,
		bool ignoreSymbols);
	bool isSymbol(uint32_t character);
	Collator(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(compare);
	ASFUNCTION_ATOM(equals);
	ASFUNCTION_ATOM(getAvailableLocaleIDNames);
	ASPROPERTY_GETTER(tiny_string, actualLocaleIDName);
	ASPROPERTY_GETTER_SETTER(bool, ignoreCase);
	ASPROPERTY_GETTER_SETTER(bool, ignoreCharacterWidth);
	ASPROPERTY_GETTER_SETTER(bool, ignoreDiacritics);
	ASPROPERTY_GETTER_SETTER(bool, ignoreKanaType);
	ASPROPERTY_GETTER_SETTER(bool, ignoreSymbols);
	ASPROPERTY_GETTER(tiny_string, lastOperationStatus);
	ASPROPERTY_GETTER_SETTER(bool, numericComparison);
	ASPROPERTY_GETTER(tiny_string, requestedLocaleIDName);
};

}
#endif /* SCRIPTING_FLASH_GLOBALIZATION_COLLATOR_H */
