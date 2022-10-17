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
#include "scripting/flash/display/NativeMenuItem.h"

namespace lightspark
{
class Array;

class ContextMenu : public EventDispatcher
{
public:
	ContextMenu(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(hideBuiltInItems);
	ASFUNCTION_ATOM(clone);
	ASPROPERTY_GETTER(bool,isSupported);
	ASPROPERTY_GETTER_SETTER(_NR<Array>,customItems);
	ASPROPERTY_GETTER_SETTER(_NR<ContextMenuBuiltInItems>,builtInItems);
	void getCurrentContextMenuItems(std::vector<_R<NativeMenuItem>>& items);
	InteractiveObject* owner;
	static void getVisibleBuiltinContextMenuItems(ContextMenu* m, std::vector<Ref<NativeMenuItem> > &items, ASWorker* worker);
};

}
#endif // SCRIPTING_FLASH_UI_CONTEXTMENU_H
