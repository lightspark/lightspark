/**************************************************************************
    Lightspark, a free flash player implementation

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

#ifndef SCRIPTING_FLASH_UI_GAMEINPUT_H
#define SCRIPTING_FLASH_UI_GAMEINPUT_H

#include "asobject.h"
#include "scripting/flash/events/flashevents.h"

namespace lightspark
{
class GameInput : public EventDispatcher
{
public:
	GameInput(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
};
class GameInputDevice : public ASObject
{
public:
	GameInputDevice(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_GAMEINPUTDEVICE) {}
	static void sinit(Class_base* c);
};


}
#endif // GAMEINPUT_H
