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
	Endian(Class_base* c):ASObject(c){};
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



ASObject* getQualifiedClassName(ASObject*, ASObject* const* args, const unsigned int len);
ASObject* getQualifiedSuperclassName(ASObject*, ASObject* const* args, const unsigned int len);
ASObject* getDefinitionByName(ASObject*, ASObject* const* args, const unsigned int len);
ASObject* getTimer(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* setInterval(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* setTimeout(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* clearInterval(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* clearTimeout(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* describeType(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* escapeMultiByte(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* unescapeMultiByte(ASObject* obj,ASObject* const* args, const unsigned int argslen);

}

#endif /* SCRIPTING_FLASH_UTILS_FLASHUTILS_H */
