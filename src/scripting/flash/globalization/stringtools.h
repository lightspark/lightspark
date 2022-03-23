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

#ifndef SCRIPTING_FLASH_GLOBALIZATION_STRINGTOOLS_H
#define SCRIPTING_FLASH_GLOBALIZATION_STRINGTOOLS_H 1

#include "asobject.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace lightspark
{

class StringTools : public ASObject
{
private:
	std::locale currlocale;

public:
	StringTools(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(getAvailableLocaleIDNames);
	ASFUNCTION_ATOM(toLowerCase);
	ASFUNCTION_ATOM(toUpperCase);

	ASPROPERTY_GETTER(tiny_string, actualLocaleIDName);
	ASPROPERTY_GETTER(tiny_string, lastOperationStatus);
	ASPROPERTY_GETTER(tiny_string, requestedLocaleIDName);
};

}
#endif /* SCRIPTING_FLASH_GLOBALIZATION_STRINGTOOLS_H */
