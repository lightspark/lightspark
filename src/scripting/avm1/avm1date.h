/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef SCRIPTING_AVM1_AVM1DATE_H
#define SCRIPTING_AVM1_AVM1DATE_H


#include "scripting/toplevel/Date.h"

namespace lightspark
{

class AVM1Date: public Date
{
public:
	AVM1Date(ASWorker* wrk, Class_base* c) : Date(wrk,c) {}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(AVM1_getYear);
	ASFUNCTION_ATOM(AVM1_getUTCYear);
	ASFUNCTION_ATOM(AVM1_setYear);
	ASFUNCTION_ATOM(AVM1_setUTCYear);
};

}
#endif // SCRIPTING_AVM1_AVM1DATE_H
