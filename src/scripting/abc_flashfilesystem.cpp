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

#include "scripting/flash/filesystem/flashfilesystem.h"

#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesFlashFilesystem(Global* builtin)
{
	//If needed add AIR definitions
	if(m_sys->flashMode==SystemState::AIR)
	{
		builtin->registerBuiltin("File","flash.filesystem",Class<ASFile>::getRef(m_sys));
		builtin->registerBuiltin("FileMode","flash.filesystem",Class<FileMode>::getRef(m_sys));
		builtin->registerBuiltin("FileStream","flash.filesystem",Class<FileStream>::getRef(m_sys));
	}
}
