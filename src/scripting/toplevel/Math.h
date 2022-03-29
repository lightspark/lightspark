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

#ifndef SCRIPTING_TOPLEVEL_MATH_H
#define SCRIPTING_TOPLEVEL_MATH_H 1

#include "asobject.h"

namespace lightspark
{

class Math: public ASObject
{
public:
	Math(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);

	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);

	ASFUNCTION_ATOM(abs);
	ASFUNCTION_ATOM(acos);
	ASFUNCTION_ATOM(asin);
	ASFUNCTION_ATOM(atan);
	ASFUNCTION_ATOM(atan2);
	ASFUNCTION_ATOM(ceil);
	ASFUNCTION_ATOM(cos);
	ASFUNCTION_ATOM(exp);
	ASFUNCTION_ATOM(floor);
	ASFUNCTION_ATOM(log);
	ASFUNCTION_ATOM(_max);
	ASFUNCTION_ATOM(_min);
	ASFUNCTION_ATOM(pow);
	ASFUNCTION_ATOM(random);
	ASFUNCTION_ATOM(round);
	ASFUNCTION_ATOM(sin);
	ASFUNCTION_ATOM(sqrt);
	ASFUNCTION_ATOM(tan);
};

}
#endif /* SCRIPTING_TOPLEVEL_MATH_H */
