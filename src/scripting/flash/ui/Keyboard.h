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

#ifndef SCRIPTING_FLASH_UI_KEYBOARD_H
#define SCRIPTING_FLASH_UI_KEYBOARD_H 1

#include "asobject.h"

namespace lightspark
{

class Keyboard : public ASObject
{
public:
	Keyboard(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(capsLock);
	ASFUNCTION_ATOM(hasVirtualKeyboard);
	ASFUNCTION_ATOM(numLock);
	ASFUNCTION_ATOM(physicalKeyboardType);
	ASFUNCTION_ATOM(isAccessible);
};

class KeyboardType : public ASObject
{
public:
	KeyboardType(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

class KeyLocation : public ASObject
{
public:
	KeyLocation(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
};

};

#endif /* SCRIPTING_FLASH_UI_KEYBOARD_H */
