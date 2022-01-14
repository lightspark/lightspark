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

#include "scripting/flash/desktop/flashdesktop.h"
#include "scripting/flash/desktop/clipboardformats.h"
#include "scripting/flash/desktop/clipboardtransfermode.h"

#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesFlashDesktop(Global* builtin)
{
	builtin->registerBuiltin("ClipboardFormats","flash.desktop",Class<ClipboardFormats>::getRef(m_sys));
	builtin->registerBuiltin("ClipboardTransferMode","flash.desktop",Class<ClipboardTransferMode>::getRef(m_sys));
	//If needed add AIR definitions
	if(m_sys->flashMode==SystemState::AIR)
	{
		builtin->registerBuiltin("NativeApplication","flash.desktop",Class<NativeApplication>::getRef(m_sys));
		builtin->registerBuiltin("NativeDragManager","flash.desktop",Class<NativeDragManager>::getRef(m_sys));
		builtin->registerBuiltin("NativeProcess","flash.desktop",Class<NativeProcess>::getRef(m_sys));
		builtin->registerBuiltin("NativeProcessStartupInfo","flash.desktop",Class<NativeProcessStartupInfo>::getRef(m_sys));
	}
}
