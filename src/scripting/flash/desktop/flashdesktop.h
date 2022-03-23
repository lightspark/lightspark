/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_DESKTOP_FLASHDESKTOP_H
#define SCRIPTING_FLASH_DESKTOP_FLASHDESKTOP_H 1

#include "scripting/flash/events/flashevents.h"

namespace lightspark
{
class Vector;
class ASFile;
class NativeApplication: public EventDispatcher
{
public:
	NativeApplication(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c){}
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getNativeApplication);
	ASFUNCTION_ATOM(addEventListener);
};

class NativeDragManager: public ASObject
{
public:
	NativeDragManager(ASWorker* wrk,Class_base* c):ASObject(wrk,c),isSupported(false){}
	static void sinit(Class_base* c);
	ASPROPERTY_GETTER(bool,isSupported);
};

class NativeProcess: public EventDispatcher
{
public:
	NativeProcess(ASWorker* wrk,Class_base* c):EventDispatcher(wrk,c),isSupported(false){}
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(start);
	ASPROPERTY_GETTER(bool,isSupported);
};
class NativeProcessStartupInfo: public ASObject
{
public:
	NativeProcessStartupInfo(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base* c);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER_SETTER(_NR<Vector>,arguments);
	ASPROPERTY_GETTER_SETTER(_NR<ASFile>,executable);
	ASPROPERTY_GETTER_SETTER(_NR<ASFile>,workingDirectory);
};


}

#endif /* SCRIPTING_FLASH_DESKTOP_FLASHDESKTOP_H */
