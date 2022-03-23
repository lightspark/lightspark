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

#ifndef SCRIPTING_FLASH_MEDIA_ID3INFO_H
#define SCRIPTING_FLASH_MEDIA_ID3INFO_H 1

#include "asobject.h"

namespace lightspark
{

class ID3Info: public ASObject
{
public:
	ID3Info(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
    ASPROPERTY_GETTER_SETTER(tiny_string, album);
    ASPROPERTY_GETTER_SETTER(tiny_string, artist);
    ASPROPERTY_GETTER_SETTER(tiny_string, comment);
    ASPROPERTY_GETTER_SETTER(tiny_string, genre);
    ASPROPERTY_GETTER_SETTER(tiny_string, songName);
    ASPROPERTY_GETTER_SETTER(tiny_string, track);
    ASPROPERTY_GETTER_SETTER(tiny_string, year);
};

}
#endif /* SCRIPTING_FLASH_MEDIA_ID3INFO_H */
