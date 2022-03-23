/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_EXTERNAL_EXTERNALINTERFACE_H
#define SCRIPTING_FLASH_EXTERNAL_EXTERNALINTERFACE_H 1

#include "compat.h"
#include "asobject.h"

namespace lightspark
{

class ExternalInterface: public ASObject
{
public:
	ExternalInterface(ASWorker* wrk,Class_base* c):ASObject(wrk,c){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_getAvailable);
	ASFUNCTION_ATOM(_getObjectID);
	ASFUNCTION_ATOM(_getMarshallExceptions);
	ASFUNCTION_ATOM(_setMarshallExceptions);
	ASFUNCTION_ATOM(addCallback);
	ASFUNCTION_ATOM(call);
};

};
#endif /* SCRIPTING_FLASH_EXTERNAL_EXTERNALINTERFACE_H */
