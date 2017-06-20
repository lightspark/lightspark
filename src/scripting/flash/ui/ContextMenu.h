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

#ifndef SCRIPTING_FLASH_UI_CONTEXTMENU_H
#define SCRIPTING_FLASH_UI_CONTEXTMENU_H

#include "asobject.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/ui/ContextMenuBuiltInItems.h"
#include "scripting/toplevel/Array.h"

namespace lightspark
{
class Array;

class ContextMenu : public EventDispatcher
{
public:
	ContextMenu(Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION(hideBuiltInItems);
	ASPROPERTY_GETTER_SETTER(_NR<Array>,customItems);
	ASPROPERTY_GETTER_SETTER(_NR<ContextMenuBuiltInItems>,builtInItems);
};

}
#endif // SCRIPTING_FLASH_UI_CONTEXTMENU_H
