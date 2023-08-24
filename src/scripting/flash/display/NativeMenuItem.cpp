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

#include "scripting/flash/display/NativeMenuItem.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
using namespace lightspark;

void NativeMenuItem::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED | CLASS_FINAL);
	REGISTER_GETTER_SETTER(c,label);
	REGISTER_GETTER(c,isSeparator);
	REGISTER_GETTER_SETTER(c,enabled);
}

void NativeMenuItem::addToMenu(std::vector<_R<NativeMenuItem> > &items, ContextMenu* menu)
{
	this->incRef();
	items.push_back(_MR(this));
	this->menu=menu;
}

bool NativeMenuItem::isReservedLabel()
{
	static const auto reservedLabels = {
		"Save",
		"Zoom In",
		"Zoom Out",
		"100%",
		"Show All",
		"Quality",
		"Play",
		"Loop",
		"Rewind",
		"Forward",
		"Back",
		"Movie not loaded",
		"About",
		"Print",
		"Show Redraw Regions",
		"Debugger",
		"Undo",
		"Cut",
		"Copy",
		"Paste",
		"Delete",
		"Select All",
		"Open",
		"Open in new window",
		"Copy link",
	};
	bool found = false;
	for (auto reservedLabel : reservedLabels)
		found = label == reservedLabel;
	return found;
}
ASFUNCTIONBODY_GETTER_SETTER(NativeMenuItem,label)
ASFUNCTIONBODY_GETTER(NativeMenuItem,isSeparator)
ASFUNCTIONBODY_GETTER_SETTER(NativeMenuItem,enabled)

ASFUNCTIONBODY_ATOM(NativeMenuItem,_constructor)
{
	NativeMenuItem* th=asAtomHandler::as<NativeMenuItem>(obj);
	ARG_CHECK(ARG_UNPACK(th->label,"")(th->isSeparator,false));
	EventDispatcher::_constructor(ret,wrk,obj,nullptr,0);
}
