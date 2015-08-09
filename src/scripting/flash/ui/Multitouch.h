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
	Multitouch(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
	ASFUNCTION(getInputMode);
	ASFUNCTION(getMaxTouchPoints);
	ASFUNCTION(getSupportedGestures);
	ASFUNCTION(getSupportsGestureEvents);
	ASFUNCTION(getSupportsTouchEvents);
};

}

#endif /* SCRIPTING_FLASH_UI_MULTITOUCH_H */
