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
	fstream stream;
	uint32_t filesize;
	bool littleEndian;
	void afterPositionChange(number_t oldposition);
	FORCE_INLINE uint16_t endianIn(uint16_t value)
	{
		if(littleEndian)
			return GUINT16_TO_LE(value);
		else
			return GUINT16_TO_BE(value);
	}
	FORCE_INLINE uint32_t endianIn(uint32_t value)
	{
		if(littleEndian)
			return GUINT32_TO_LE(value);
		else
			return GUINT32_TO_BE(value);
	}
	FORCE_INLINE uint64_t endianIn(uint64_t value)
	{
		if(littleEndian)
			return GUINT64_TO_LE(value);
		else
			return GUINT64_TO_BE(value);
	}

	FORCE_INLINE uint16_t endianOut(uint16_t value)
	{
		if(littleEndian)
			return GUINT16_FROM_LE(value);
		else
			return GUINT16_FROM_BE(value);
	}
	FORCE_INLINE uint32_t endianOut(uint32_t value)
	{
		if(littleEndian)
			return GUINT32_FROM_LE(value);
		else
			return GUINT32_FROM_BE(value);
	}
	FORCE_INLINE uint64_t endianOut(uint64_t value)
	{
		if(littleEndian)
			return GUINT64_FROM_LE(value);
		else
			return GUINT64_FROM_BE(value);
	}
	
public:
	FileStream(ASWorker* wrk,Class_base* c);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getEndian);
	ASFUNCTION_ATOM(_setEndian);
	ASFUNCTION_ATOM(open);
	ASFUNCTION_ATOM(close);
	ASFUNCTION_ATOM(readBytes);
	ASFUNCTION_ATOM(readUTF);
	ASFUNCTION_ATOM(readByte);
	ASFUNCTION_ATOM(readUnsignedByte);
	ASFUNCTION_ATOM(readShort);
	ASFUNCTION_ATOM(readUnsignedShort);
	ASFUNCTION_ATOM(readInt);
	ASFUNCTION_ATOM(readUnsignedInt);
	ASPROPERTY_GETTER(uint32_t,bytesAvailable);
	ASPROPERTY_GETTER_SETTER(number_t,position);
	ASFUNCTION_ATOM(writeBytes);
	ASFUNCTION_ATOM(writeUTF);
};

class ASFile: public FileReference
{
private:
	void setupFile(const tiny_string& filename,ASWorker* wrk);
	tiny_string path;
public:
	ASFile(ASWorker* wrk,Class_base* c, const tiny_string _path="", bool _exists=false);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASPROPERTY_GETTER(bool,exists);
	ASPROPERTY_GETTER(_NR<ASFile>,applicationDirectory);
	ASPROPERTY_GETTER(_NR<ASFile>,applicationStorageDirectory);
	ASFUNCTION_ATOM(resolvePath);
	ASFUNCTION_ATOM(createDirectory);
	ASFUNCTION_ATOM(_getURL);
	ASFUNCTION_ATOM(_setURL);
	const tiny_string& getFullPath() const { return path; }
};
class FileMode: public ASObject
{
public:
	FileMode(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_FILEMODE){}
	static void sinit(Class_base* c);
};

}
#endif /* SCRIPTING_FLASH_FILESYSTEM_FLASHFILESYSTEM_H */
