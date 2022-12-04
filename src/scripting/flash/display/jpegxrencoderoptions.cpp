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

#include "scripting/flash/display/jpegxrencoderoptions.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void JPEGXREncoderOptions::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);
	REGISTER_GETTER_SETTER(c,colorSpace);
	REGISTER_GETTER_SETTER(c,quantization);
	REGISTER_GETTER_SETTER(c,trimFlexBits);
}

ASFUNCTIONBODY_ATOM(JPEGXREncoderOptions,_constructor)
{
	JPEGXREncoderOptions* th =asAtomHandler::as<JPEGXREncoderOptions>(obj);
	ARG_CHECK(ARG_UNPACK(th->quantization,20)(th->colorSpace,"auto")(th->trimFlexBits,0));
}

ASFUNCTIONBODY_GETTER_SETTER(JPEGXREncoderOptions, colorSpace)
ASFUNCTIONBODY_GETTER_SETTER(JPEGXREncoderOptions, quantization)
ASFUNCTIONBODY_GETTER_SETTER(JPEGXREncoderOptions, trimFlexBits)
