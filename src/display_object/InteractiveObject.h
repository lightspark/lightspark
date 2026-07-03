/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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

#ifndef DISPLAY_OBJECT_INTERACTIVEOBJECT_H
#define DISPLAY_OBJECT_INTERACTIVEOBJECT_H 1

#include <cstddef>
#include <cstdint>
#include <vector>

#include "display_object/DisplayObject.h"
#include "smartrefs.h"
#include "swftypes.h"
#include "utils/optional.h"

// Based on Ruffle's `display_object::InteractiveObject`.

namespace lightspark
{

class tiny_string;
class ContextMenu;
class Event;
class NativeMenuItem;

class InteractiveObject : public DisplayObject
{
private:
	_NR<ContextMenu> contextMenu;

	bool mouseEnabled : 1;
	bool doubleClickEnabled : 1;
	bool hasFocus : 1;

	int32_t tabIndex;
	Optional<bool> tabEnabled;
	Optional<bool> focusRect;

	virtual bool getDefaultTabEnabled() const { return false; }
protected:
	~InteractiveObject() {}
public:
	bool isHittable(HIT_TYPE type)
	{
		if(type == MOUSE_CLICK_HIT)
			return mouseEnabled;
		else if(type == DOUBLE_CLICK_HIT)
			return doubleClickEnabled && mouseEnabled;
		else
			return true;
	}

	virtual bool isFocusable(bool fromMouse)
	{
		return fromMouse ? mouseEnabled : true;
	}

	InteractiveObject
	(
		const Type& type,
		SystemState* sys,
		Optional<const tiny_string&> name = {}
	);

	virtual void lostFocus() {}
	virtual void gotFocus() {}
	virtual void textInputChanged(const tiny_string& newtext) {}

	bool getMouseEnabled() const { return mouseEnabled; }
	void setMouseEnabled(bool enabled) { mouseEnabled = enabled; }
	bool getDoubleClickEnabled() const { return doubleClickEnabled; }
	void setDoubleClickEnabled(bool enabled) { doubleClickEnabled = enabled; }
	bool getHasFocus() const { return hasFocus; }
	void setHasFocus(bool _hasFocus) { hasFocus = _hasFocus; }

	_NR<ContextMenu> getContextMenu() const { return contextMenu; }
	void setContextMenu(_NR<ContextMenu> menu) { contextMenu = menu; }

	Optional<bool> getFocusRect() const { return focusRect; }
	void setFocusRect(const Optional<bool>& val) { focusRect = val; }

	virtual bool isMouseFocusable() const;
	virtual bool isHighlightable() const { return isHighlightEnabled(); }
	bool isHighlightEnabled() const;
	virtual bool isTabbable() const { return getTabEnabled(); }
	bool getTabEnabled() const;
	void setTabEnabled(bool enabled);

	int32_t getTabIndex() const { return tabIndex; }
	void setTabIndex(int32_t index) { tabIndex = index; }

	void defaultEventBehavior(_R<Event> e) override;
	// returns the owner of the contextmenu
	InteractiveObject& getCurrentContextMenuItems(std::vector<_R<NativeMenuItem>>& items);
};

}
#endif /* DISPLAY_OBJECT_INTERACTIVEOBJECT_H */
