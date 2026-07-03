/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2024, 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include <algorithm>
#include <list>

#include "asobject.h"
#include "display_object/InteractiveObject.h"
#include "events.h"
#include "gc/context.h"
#include "gc/ptr.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/ui/ContextMenu.h"
#include "scripting/flash/ui/NativeMenuItem.h"
#include "tiny_string.h"

using namespace lightspark;

InteractiveObject::InteractiveObject
(
	const Type& type,
	SystemState* sys,
	Optional<const tiny_string&> name = {}
) : DisplayObject(type, sys, name),
mouseEnabled(true),
doubleClickEnabled(false),
hasFocus(false),
tabIndex(-1)
{
}

void InteractiveObject::defaultEventBehavior(_R<Event> e)
{
	if (mouseEnabled && e->type == "contextMenu")
		getSys()->pushEvent(LSOpenContextMenuEvent(this));
}

InteractiveObject& InteractiveObject::getCurrentContextMenuItems
(
	std::vector<_R<NativeMenuItem>>& items
)
{
	auto obj = toASObject();
	if (obj.isNull())
		return *this;

	if (!contextMenu.isNull())
	{
		contextMenu->getCurrentContextMenuItems(items);
		return *this;
	}

	auto parent =
	(
		getParent() != nullptr ?
		getParent()->as<InteractiveObject>() :
		nullptr
	);

	if (parent != nullptr)
		return parent->getCurrentContextMenuItems(items);

	ContextMenu::getVisibleBuiltinContextMenuItems
	(
		nullptr,
		items,
		obj->getInstanceWorker()
	);
	return *this;
}

bool InteractiveObject::isMouseFocusable() const
{
	return isAS3() && getTabEnabled();
}

bool InteractiveObject::isHighlightEnabled() const
{
	if (getSwfVersion() < 6)
		return getSys()->stage->getStageFocusRect();
	return focusRect.orElse([&]
	{
		return getSys()->stage->getStageFocusRect();
	})
}

bool InteractiveObject::getTabEnabled() const
{
	if (isAS3())
		return tabEnabled.orElse(getDefaultTabEnabled);
	return getAVM1BoolProp("tabEnabled", getDefaultTabEnabled);
}

void InteractiveObject::setTabEnabled(bool enabled)
{
	if (isAS3())
		tabEnabled = enabled;
	else
		setAVM1Prop("tabEnabled", enabled);
}
