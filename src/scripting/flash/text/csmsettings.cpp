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

#include "scripting/flash/text/csmsettings.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void CSMSettings::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);

	REGISTER_GETTER_SETTER(c,fontSize);
	REGISTER_GETTER_SETTER(c,insideCutoff);
	REGISTER_GETTER_SETTER(c,outsideCutoff);
}

ASFUNCTIONBODY_ATOM(CSMSettings,_constructor)
{
	CSMSettings* th =asAtomHandler::as<CSMSettings>(obj);
	ARG_CHECK(ARG_UNPACK(th->fontSize)(th->insideCutoff)(th->outsideCutoff));
}

ASFUNCTIONBODY_GETTER_SETTER(CSMSettings,fontSize)
ASFUNCTIONBODY_GETTER_SETTER(CSMSettings,insideCutoff)
ASFUNCTIONBODY_GETTER_SETTER(CSMSettings,outsideCutoff)
