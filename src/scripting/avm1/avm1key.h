/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_AVM1_AVM1KEY_H
#define SCRIPTING_AVM1_AVM1KEY_H

#include "asobject.h"
namespace lightspark
{

class AVM1Key: public ASObject
{
public:
	AVM1Key(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);

	ASFUNCTION_ATOM(isDown);
	ASFUNCTION_ATOM(addListener);
	ASFUNCTION_ATOM(removeListener);
	ASFUNCTION_ATOM(getCode);
	ASFUNCTION_ATOM(getAscii);
};
class AVM1Mouse: public ASObject
{
public:
	AVM1Mouse(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);

	ASFUNCTION_ATOM(hide);
	ASFUNCTION_ATOM(show);
	ASFUNCTION_ATOM(addListener);
	ASFUNCTION_ATOM(removeListener);
};

}
#endif // SCRIPTING_AVM1_AVM1KEY_H
