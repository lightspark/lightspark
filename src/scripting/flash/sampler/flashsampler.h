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

#ifndef FLASHSAMPLER_H
#define FLASHSAMPLER_H

#include "asobject.h"
#include "scripting/toplevel/Array.h"

namespace lightspark
{
class Sample : public ASObject
{
public:
	Sample(Class_base* c);
	static void sinit(Class_base*);
};

class DeleteObjectSample : public Sample
{
public:
	DeleteObjectSample(Class_base* c);
	static void sinit(Class_base*);
};
class NewObjectSample : public Sample
{
public:
	NewObjectSample(Class_base* c);
	static void sinit(Class_base*);
	ASPROPERTY_GETTER(_NR<ASObject>,object);
	ASPROPERTY_GETTER(number_t,size);
};
class StackFrame : public ASObject
{
public:
	StackFrame(Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_toString);
};


asAtom clearSamples(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom getInvocationCount(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom getLexicalScopes(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom getMasterString(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom getMemberNames(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom getSampleCount(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom getSamples(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom getSize(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom isGetterSetter(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom pauseSampling(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom sampleInternalAllocs(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom startSampling(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom stopSampling(SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);





}
#endif // FLASHSAMPLER_H
