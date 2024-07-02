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

#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/system/messagechannel.h"
#include "scripting/flash/system/messagechannelstate.h"
#include "scripting/flash/system/securitypanel.h"
#include "scripting/flash/system/systemupdater.h"
#include "scripting/flash/system/systemupdatertype.h"
#include "scripting/flash/system/touchscreentype.h"
#include "scripting/flash/system/ime.h"
#include "scripting/flash/system/jpegloadercontext.h"

#include "scripting/toplevel/Global.h"
#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;


void ABCVm::registerClassesFlashSystem(Global* builtin)
{
	builtin->registerBuiltin("fscommand","flash.system",_MR(m_sys->getBuiltinFunction(fscommand)));
	builtin->registerBuiltin("Capabilities","flash.system",Class<Capabilities>::getRef(m_sys));
	builtin->registerBuiltin("Security","flash.system",Class<Security>::getRef(m_sys));
	builtin->registerBuiltin("ApplicationDomain","flash.system",Class<ApplicationDomain>::getRef(m_sys));
	builtin->registerBuiltin("SecurityDomain","flash.system",Class<SecurityDomain>::getRef(m_sys));
	builtin->registerBuiltin("LoaderContext","flash.system",Class<LoaderContext>::getRef(m_sys));
	builtin->registerBuiltin("System","flash.system",Class<System>::getRef(m_sys));
	builtin->registerBuiltin("Worker","flash.system",Class<ASWorker>::getRef(m_sys));
	builtin->registerBuiltin("WorkerDomain","flash.system",Class<WorkerDomain>::getRef(m_sys));
	builtin->registerBuiltin("WorkerState","flash.system",Class<WorkerState>::getRef(m_sys));
	builtin->registerBuiltin("ImageDecodingPolicy","flash.system",Class<ImageDecodingPolicy>::getRef(m_sys));
	builtin->registerBuiltin("IMEConversionMode","flash.system",Class<IMEConversionMode>::getRef(m_sys));
	builtin->registerBuiltin("MessageChannel","flash.system",Class<MessageChannel>::getRef(m_sys));
	builtin->registerBuiltin("MessageChannelState","flash.system",Class<MessageChannelState>::getRef(m_sys));
	builtin->registerBuiltin("SecurityPanel","flash.system",Class<SecurityPanel>::getRef(m_sys));
	builtin->registerBuiltin("SystemUpdater","flash.system",Class<SystemUpdater>::getRef(m_sys));
	builtin->registerBuiltin("SystemUpdaterType","flash.system",Class<SystemUpdaterType>::getRef(m_sys));
	builtin->registerBuiltin("TouchscreenType","flash.system",Class<TouchscreenType>::getRef(m_sys));
	builtin->registerBuiltin("IME","flash.system",Class<IME>::getRef(m_sys));
	builtin->registerBuiltin("JPEGLoaderContext","flash.system",Class<JPEGLoaderContext>::getRef(m_sys));
}
