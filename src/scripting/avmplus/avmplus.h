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

namespace lightspark
{
class ApplicationDomain;
class avmplusFile : public ASObject
{
public:
	avmplusFile(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(exists);
	ASFUNCTION_ATOM(read);
	ASFUNCTION_ATOM(write);
	ASFUNCTION_ATOM(readByteArray);
	ASFUNCTION_ATOM(writeByteArray);
};

class avmplusSystem : public ASObject
{
public:
	avmplusSystem(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(getFeatures);
	ASFUNCTION_ATOM(queueCollection);
	ASFUNCTION_ATOM(forceFullCollection);
	ASFUNCTION_ATOM(getAvmplusVersion);
	ASFUNCTION_ATOM(pauseForGCIfCollectionImminent);
	ASFUNCTION_ATOM(getRunmode);
	ASFUNCTION_ATOM(isDebugger);
	ASFUNCTION_ATOM(isGlobal);
	ASFUNCTION_ATOM(_freeMemory);
	ASFUNCTION_ATOM(_totalMemory);
	ASFUNCTION_ATOM(_privateMemory);
	ASFUNCTION_ATOM(_swfVersion);
	ASFUNCTION_ATOM(argv);
	ASFUNCTION_ATOM(exec);
	ASFUNCTION_ATOM(write);
	ASFUNCTION_ATOM(sleep);
	ASFUNCTION_ATOM(exit);
	ASFUNCTION_ATOM(canonicalizeNumber);
};

class avmplusDomain : public ASObject
{
private:
	_NR<ApplicationDomain> appdomain;
public:
	avmplusDomain(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(load);
	ASFUNCTION_ATOM(loadBytes);
	ASFUNCTION_ATOM(getClass);
	ASFUNCTION_ATOM(_getCurrentDomain);
	ASFUNCTION_ATOM(_getMinDomainMemoryLength);
	ASFUNCTION_ATOM(_getDomainMemory);
	ASFUNCTION_ATOM(_setDomainMemory);
};

void casi32(asAtom& ret, ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
}
#endif /* SCRIPTING_AVMPLUS_AVMPLUS_H */
