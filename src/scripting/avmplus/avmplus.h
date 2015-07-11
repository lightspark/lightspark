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

#ifndef SCRIPTING_AVMPLUS_AVMPLUS_H
#define SCRIPTING_AVMPLUS_AVMPLUS_H 1

#include "asobject.h"
#include "scripting/flash/system/flashsystem.h"

namespace lightspark
{

class avmplusFile : public ASObject
{
public:
	avmplusFile(Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION(exists);
	ASFUNCTION(read);
	ASFUNCTION(write);
	ASFUNCTION(readByteArray);
	ASFUNCTION(writeByteArray);
};

class avmplusSystem : public ASObject
{
public:
	avmplusSystem(Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION(getFeatures);
	ASFUNCTION(queueCollection);
	ASFUNCTION(forceFullCollection);
	ASFUNCTION(getAvmplusVersion);
	ASFUNCTION(pauseForGCIfCollectionImminent);
	ASFUNCTION(getRunmode);
	ASFUNCTION(isDebugger);
	ASFUNCTION(isGlobal);
	ASFUNCTION(_freeMemory);
	ASFUNCTION(_totalMemory);
	ASFUNCTION(_privateMemory);
	ASFUNCTION(argv);
	ASFUNCTION(exec);
	ASFUNCTION(write);
	ASFUNCTION(exit);
	ASFUNCTION(canonicalizeNumber);
};

class avmplusDomain : public ApplicationDomain
{
public:
	avmplusDomain(Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
	ASFUNCTION(load);
	ASFUNCTION(loadBytes);
	ASFUNCTION(getClass);
};

}
#endif /* SCRIPTING_AVMPLUS_AVMPLUS_H */
