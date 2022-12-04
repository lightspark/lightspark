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

#include "scripting/flash/globalization/currencyparseresult.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void CurrencyParseResult::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);
    REGISTER_GETTER(c, currencyString);
    REGISTER_GETTER(c, value);
}

ASFUNCTIONBODY_GETTER(CurrencyParseResult, currencyString)
ASFUNCTIONBODY_GETTER(CurrencyParseResult, value)

ASFUNCTIONBODY_ATOM(CurrencyParseResult,_constructor)
{
    CurrencyParseResult* th =asAtomHandler::as<CurrencyParseResult>(obj);
	ARG_CHECK(ARG_UNPACK(th->value)(th->currencyString,""));
}
