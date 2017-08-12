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

#ifndef SCRIPTING_FLASH_UTILS_FLASHUTILS_H
#define SCRIPTING_FLASH_UTILS_FLASHUTILS_H 1

#include "compat.h"
#include "swftypes.h"
#include "asobject.h"


namespace lightspark
{
const tiny_string flash_proxy="http://www.adobe.com/2006/actionscript/flash/proxy";

class Endian : public ASObject
{
public:
	static const char* bigEndian;
	static const char* littleEndian;
	Endian(Class_base* c):ASObject(c){}
	static void sinit(Class_base* c);
};

class IExternalizable
{
public:
	static void linkTraits(Class_base* c);
};

class IDataInput
{
public:
	static void linkTraits(Class_base* c);
};

class IDataOutput
{
public:
	static void linkTraits(Class_base* c);
};



asAtom getQualifiedClassName(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom getQualifiedSuperclassName(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom getDefinitionByName(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom getTimer(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom setInterval(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom setTimeout(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom clearInterval(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom clearTimeout(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom describeType(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom escapeMultiByte(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
asAtom unescapeMultiByte(SystemState* sys,asAtom& obj,asAtom* args, const unsigned int argslen);
}

#endif /* SCRIPTING_FLASH_UTILS_FLASHUTILS_H */
