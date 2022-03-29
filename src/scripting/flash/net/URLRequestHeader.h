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

#ifndef SCRIPTING_FLASH_NET_URLREQUESTHEADER_H
#define SCRIPTING_FLASH_NET_URLREQUESTHEADER_H 1

#include "asobject.h"
#include "tiny_string.h"

namespace lightspark
{

class URLRequestHeader: public ASObject
{
public:
	URLRequestHeader(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(tiny_string, name);
	ASPROPERTY_GETTER_SETTER(tiny_string, value);
};

}

#endif
