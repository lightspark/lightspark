/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2013  Antti Ajanki (antti.ajanki@iki.fi)

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

#ifndef SCRIPTING_FLASH_UI_MULTITOUCH_H
#define SCRIPTING_FLASH_UI_MULTITOUCH_H 1

#include "asobject.h"
namespace lightspark
{

class Multitouch : public ASObject
{
public:
	Multitouch(ASWorker* wrk,Class_base* c):ASObject(wrk,c),inputMode("gesture") {}
	static void sinit(Class_base* c);
	ASPROPERTY_GETTER_SETTER(tiny_string,inputMode);
	ASFUNCTION_ATOM(getMaxTouchPoints);
	ASFUNCTION_ATOM(getSupportedGestures);
	ASFUNCTION_ATOM(getSupportsGestureEvents);
	ASFUNCTION_ATOM(getSupportsTouchEvents);
};
class MultitouchInputMode : public ASObject
{
public:
	MultitouchInputMode(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};
}

#endif /* SCRIPTING_FLASH_UI_MULTITOUCH_H */
