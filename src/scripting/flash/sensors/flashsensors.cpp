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

#include <map>
#include "abc.h"
#include "flashsensors.h"
#include "class.h"
#include "flash/system/flashsystem.h"
#include "compat.h"
#include "backends/audio.h"
#include "backends/builtindecoder.h"
#include "backends/rendering.h"
#include "backends/security.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.sensors");

REGISTER_CLASS_NAME(Accelerometer);

Accelerometer::Accelerometer() {}

void Accelerometer::sinit(Class_base* c)
{
	// properties
	c->setDeclaredMethodByQName("isSupported", "", Class<IFunction>::getFunction(_isSupported),GETTER_METHOD,false);
}

void Accelerometer::buildTraits(ASObject *o)
{
}

ASFUNCTIONBODY(Accelerometer,_isSupported)
{
	return abstract_b(false);
}
