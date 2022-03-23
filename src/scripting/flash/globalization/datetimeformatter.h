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

#ifndef SCRIPTING_FLASH_GLOBALIZATION_DATEFORMATTER_H
#define SCRIPTING_FLASH_GLOBALIZATION_DATEFORMATTER_H 1

#include "asobject.h"
#include <clocale>

namespace lightspark
{

class DateTimeFormatter : public ASObject
{
private:
	tiny_string dateStyle;
	tiny_string timeStyle;
	tiny_string pattern;
	std::locale currlocale;
public:
	DateTimeFormatter(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	static tiny_string buildDateTimePattern(tiny_string dateStyle, tiny_string timeStyle);
	static std::string buildInternalFormat(std::string pattern);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(setDateTimePattern);
	ASFUNCTION_ATOM(format);
	ASFUNCTION_ATOM(formatUTC);
	ASFUNCTION_ATOM(getAvailableLocaleIDNames);
	ASFUNCTION_ATOM(getDateStyle);
	ASFUNCTION_ATOM(getDateTimePattern);
	ASFUNCTION_ATOM(getFirstWeekday);
	ASFUNCTION_ATOM(getMonthNames);
	ASFUNCTION_ATOM(getTimeStyle);
	ASFUNCTION_ATOM(getWeekdayNames);
	ASFUNCTION_ATOM(setDateTimeStyles);
	ASPROPERTY_GETTER(tiny_string, actualLocaleIDName);
	ASPROPERTY_GETTER(tiny_string, lastOperationStatus);
	ASPROPERTY_GETTER(tiny_string, requestedLocaleIDName);
};

}
#endif /* SCRIPTING_FLASH_GLOBALIZATION_DATEFORMATTER_H */
