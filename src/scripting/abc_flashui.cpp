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

#include "scripting/flash/ui/Keyboard.h"
#include "scripting/flash/ui/Mouse.h"
#include "scripting/flash/ui/Multitouch.h"
#include "scripting/flash/ui/ContextMenu.h"
#include "scripting/flash/ui/ContextMenuItem.h"
#include "scripting/flash/ui/ContextMenuBuiltInItems.h"
#include "scripting/flash/ui/gameinput.h"
#include "scripting/flash/display/RootMovieClip.h"

#include "scripting/toplevel/Global.h"
#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesFlashUI(Global* builtin)
{
	builtin->registerBuiltin("Keyboard","flash.ui",Class<Keyboard>::getRef(m_sys));
	builtin->registerBuiltin("KeyboardType","flash.ui",Class<KeyboardType>::getRef(m_sys));
	builtin->registerBuiltin("KeyLocation","flash.ui",Class<KeyLocation>::getRef(m_sys));
	builtin->registerBuiltin("ContextMenu","flash.ui",Class<ContextMenu>::getRef(m_sys));
	builtin->registerBuiltin("ContextMenuItem","flash.ui",Class<ContextMenuItem>::getRef(m_sys));
	builtin->registerBuiltin("ContextMenuBuiltInItems","flash.ui",Class<ContextMenuBuiltInItems>::getRef(m_sys));
	builtin->registerBuiltin("Mouse","flash.ui",Class<Mouse>::getRef(m_sys));
	builtin->registerBuiltin("MouseCursor","flash.ui",Class<MouseCursor>::getRef(m_sys));
	builtin->registerBuiltin("MouseCursorData","flash.ui",Class<MouseCursorData>::getRef(m_sys));
	builtin->registerBuiltin("Multitouch","flash.ui",Class<Multitouch>::getRef(m_sys));
	builtin->registerBuiltin("MultitouchInputMode","flash.ui",Class<MultitouchInputMode>::getRef(m_sys));

	// contrary to specs the GameInput classes are _not_ AIR only but available for every swf file with version >= 20
	if(m_sys->flashMode==SystemState::AIR || m_sys->mainClip->version>=20)
	{
		builtin->registerBuiltin("GameInput","flash.ui",Class<GameInput>::getRef(m_sys));
		builtin->registerBuiltin("GameInputDevice","flash.ui",Class<GameInputDevice>::getRef(m_sys));
	}
}
