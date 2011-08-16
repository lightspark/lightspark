/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef _FLASH_SENSORS_H
#define _FLASH_SENSORS_H

#include "compat.h"
#include "asobject.h"
#include "flashevents.h"
#include "thread_pool.h"
#include "backends/netutils.h"
#include "timer.h"
#include "backends/decoder.h"
#include "backends/interfaces/audio/IAudioPlugin.h"

namespace lightspark
{
class Accelerometer: public ASObject {
	public:
		Accelerometer();
		static void sinit(Class_base* c);
		static void buildTraits(ASObject* o);
		ASFUNCTION(_isSupported);
};
}
#endif
