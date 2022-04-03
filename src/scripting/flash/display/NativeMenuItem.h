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

#ifndef SCRIPTING_FLASH_DISPLAY_NATIVEMENUITEM_H
#define SCRIPTING_FLASH_DISPLAY_NATIVEMENUITEM_H

#include "asobject.h"
#include "scripting/flash/events/flashevents.h"

namespace lightspark
{

class DLL_PUBLIC NativeMenuItem: public EventDispatcher
{
protected:
	ContextMenu* menu;
public:
	NativeMenuItem(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c),isSeparator(false),enabled(true){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(tiny_string, label);
	ASPROPERTY_GETTER(bool, isSeparator);
	ASPROPERTY_GETTER_SETTER(bool, enabled);
	virtual void addToMenu(std::vector<_R<NativeMenuItem>>& items,ContextMenu* menu);
};

}
#endif // SCRIPTING_FLASH_DISPLAY_NATIVEMENUITEM_H
