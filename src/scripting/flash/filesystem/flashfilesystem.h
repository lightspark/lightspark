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

#ifndef SCRIPTING_FLASH_FILESYSTEM_FLASHFILESYSTEM_H
#define SCRIPTING_FLASH_FILESYSTEM_FLASHFILESYSTEM_H 1

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/net/flashnet.h"

namespace lightspark
{

class ASFile;
class FileStream: public EventDispatcher
{
	_NR<ASFile> file;
	tiny_string fileMode;
	_NR<ByteArray> bytes;
public:
	FileStream(Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(open);
	ASFUNCTION_ATOM(close);
	ASFUNCTION_ATOM(readBytes);
	ASPROPERTY_GETTER(uint32_t,bytesAvailable);
};

class ASFile: public FileReference
{
	tiny_string path;
public:
	ASFile(Class_base* c, const tiny_string _path="", bool _exists=false);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER(bool,exists);
	ASPROPERTY_GETTER(_NR<ASFile>,applicationDirectory);
	ASFUNCTION_ATOM(resolvePath);
	const tiny_string& getFullPath() const { return path; }
};
class FileMode: public ASObject
{
public:
	FileMode(Class_base* c):ASObject(c,T_OBJECT,SUBTYPE_FILEMODE){}
	static void sinit(Class_base* c);
};

}
#endif /* SCRIPTING_FLASH_FILESYSTEM_FLASHFILESYSTEM_H */
