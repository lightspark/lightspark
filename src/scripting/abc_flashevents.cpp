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

#include "scripting/flash/events/flashevents.h"

#include "scripting/toplevel/Global.h"
#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;


void ABCVm::registerClassesFlashEvents(Global* builtin)
{
	builtin->registerBuiltin("EventDispatcher","flash.events",Class<EventDispatcher>::getRef(m_sys));
	builtin->registerBuiltin("Event","flash.events",Class<Event>::getRef(m_sys));
	builtin->registerBuiltin("EventPhase","flash.events",Class<EventPhase>::getRef(m_sys));
	builtin->registerBuiltin("MouseEvent","flash.events",Class<MouseEvent>::getRef(m_sys));
	builtin->registerBuiltin("ProgressEvent","flash.events",Class<ProgressEvent>::getRef(m_sys));
	builtin->registerBuiltin("TimerEvent","flash.events",Class<TimerEvent>::getRef(m_sys));
	builtin->registerBuiltin("IOErrorEvent","flash.events",Class<IOErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("ErrorEvent","flash.events",Class<ErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("SecurityErrorEvent","flash.events",Class<SecurityErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("AsyncErrorEvent","flash.events",Class<AsyncErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("FullScreenEvent","flash.events",Class<FullScreenEvent>::getRef(m_sys));
	builtin->registerBuiltin("TextEvent","flash.events",Class<TextEvent>::getRef(m_sys));
	builtin->registerBuiltin("IEventDispatcher","flash.events",InterfaceClass<IEventDispatcher>::getRef(m_sys));
	builtin->registerBuiltin("FocusEvent","flash.events",Class<FocusEvent>::getRef(m_sys));
	builtin->registerBuiltin("NetStatusEvent","flash.events",Class<NetStatusEvent>::getRef(m_sys));
	builtin->registerBuiltin("HTTPStatusEvent","flash.events",Class<HTTPStatusEvent>::getRef(m_sys));
	builtin->registerBuiltin("KeyboardEvent","flash.events",Class<KeyboardEvent>::getRef(m_sys));
	builtin->registerBuiltin("StatusEvent","flash.events",Class<StatusEvent>::getRef(m_sys));
	builtin->registerBuiltin("DataEvent","flash.events",Class<DataEvent>::getRef(m_sys));
	builtin->registerBuiltin("DRMErrorEvent","flash.events",Class<DRMErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("DRMStatusEvent","flash.events",Class<DRMStatusEvent>::getRef(m_sys));
	builtin->registerBuiltin("StageVideoEvent","flash.events",Class<StageVideoEvent>::getRef(m_sys));
	builtin->registerBuiltin("StageVideoAvailabilityEvent","flash.events",Class<StageVideoAvailabilityEvent>::getRef(m_sys));
	builtin->registerBuiltin("TouchEvent","flash.events",Class<TouchEvent>::getRef(m_sys));
	builtin->registerBuiltin("GameInputEvent","flash.events",Class<GameInputEvent>::getRef(m_sys));
	builtin->registerBuiltin("GestureEvent","flash.events",Class<GestureEvent>::getRef(m_sys));
	builtin->registerBuiltin("PressAndTapGestureEvent","flash.events",Class<PressAndTapGestureEvent>::getRef(m_sys));
	builtin->registerBuiltin("TransformGestureEvent","flash.events",Class<TransformGestureEvent>::getRef(m_sys));
	builtin->registerBuiltin("ContextMenuEvent","flash.events",Class<ContextMenuEvent>::getRef(m_sys));
	builtin->registerBuiltin("UncaughtErrorEvent","flash.events",Class<UncaughtErrorEvent>::getRef(m_sys));
	builtin->registerBuiltin("UncaughtErrorEvents","flash.events",Class<UncaughtErrorEvents>::getRef(m_sys));
	builtin->registerBuiltin("VideoEvent","flash.events",Class<VideoEvent>::getRef(m_sys));
	builtin->registerBuiltin("SampleDataEvent","flash.events",Class<SampleDataEvent>::getRef(m_sys));
	builtin->registerBuiltin("ThrottleEvent","flash.events",Class<ThrottleEvent>::getRef(m_sys));
	builtin->registerBuiltin("ThrottleType","flash.events",Class<ThrottleType>::getRef(m_sys));

	//If needed add AIR definitions
	if(m_sys->flashMode==SystemState::AIR)
	{
		builtin->registerBuiltin("InvokeEvent","flash.events",Class<InvokeEvent>::getRef(m_sys));
		builtin->registerBuiltin("NativeDragEvent","flash.events",Class<NativeDragEvent>::getRef(m_sys));
		builtin->registerBuiltin("NativeWindowBoundsEvent","flash.events",Class<NativeWindowBoundsEvent>::getRef(m_sys));
	}
}
