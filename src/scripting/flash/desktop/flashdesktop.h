/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_DESKTOP_FLASHDESKTOP_H
#define SCRIPTING_FLASH_DESKTOP_FLASHDESKTOP_H 1

#include "asobject.h"
#include "scripting/flash/events/flashevents.h"

namespace lightspark
{

class NativeApplication: public EventDispatcher
{
public:
	NativeApplication(Class_base* c):EventDispatcher(c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getNativeApplication);
	ASFUNCTION(addEventListener);
};

};

#endif /* SCRIPTING_FLASH_DESKTOP_FLASHDESKTOP_H */
