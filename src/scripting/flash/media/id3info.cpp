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

#include "scripting/flash/media/id3info.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void ID3Info::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);

    REGISTER_GETTER_SETTER(c, album);
    REGISTER_GETTER_SETTER(c, artist);
    REGISTER_GETTER_SETTER(c, comment);
    REGISTER_GETTER_SETTER(c, genre);
    REGISTER_GETTER_SETTER(c, songName);
    REGISTER_GETTER_SETTER(c, track);
    REGISTER_GETTER_SETTER(c, year);
}

ASFUNCTIONBODY_GETTER_SETTER(ID3Info, album);
ASFUNCTIONBODY_GETTER_SETTER(ID3Info, artist);
ASFUNCTIONBODY_GETTER_SETTER(ID3Info, comment);
ASFUNCTIONBODY_GETTER_SETTER(ID3Info, genre);
ASFUNCTIONBODY_GETTER_SETTER(ID3Info, songName);
ASFUNCTIONBODY_GETTER_SETTER(ID3Info, track);
ASFUNCTIONBODY_GETTER_SETTER(ID3Info, year);