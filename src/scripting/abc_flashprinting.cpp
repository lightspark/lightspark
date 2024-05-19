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

#include "scripting/flash/printing/flashprinting.h"

#include "scripting/toplevel/Global.h"
#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;


void ABCVm::registerClassesFlashPrinting(Global* builtin)
{
	builtin->registerBuiltin("PrintJob","flash.printing",Class<PrintJob>::getRef(m_sys));
	builtin->registerBuiltin("PrintJobOptions","flash.printing",Class<PrintJobOptions>::getRef(m_sys));
	builtin->registerBuiltin("PrintJobOrientation","flash.printing",Class<PrintJobOrientation>::getRef(m_sys));
}
