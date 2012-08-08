/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_SENSORS_FLASHSENSORS_H
#define SCRIPTING_FLASH_SENSORS_FLASHSENSORS_H 1

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/events/flashevents.h"
#include "thread_pool.h"
#include "backends/netutils.h"
#include "timer.h"
#include "backends/interfaces/audio/IAudioPlugin.h"

namespace lightspark
{
class Accelerometer: public ASObject {
	public:
		Accelerometer(Class_base* c);
		static void sinit(Class_base* c);
		static void buildTraits(ASObject* o);
		ASFUNCTION(_isSupported);
};
}
#endif /* SCRIPTING_FLASH_SENSORS_FLASHSENSORS_H */
