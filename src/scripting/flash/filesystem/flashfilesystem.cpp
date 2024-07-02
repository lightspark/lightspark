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

#include "scripting/flash/filesystem/flashfilesystem.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Number.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "compat.h"
#include "platforms/engineutils.h"

using namespace lightspark;

FileStream::FileStream(ASWorker* wrk,Class_base* c):
	EventDispatcher(wrk,c),filesize(0),littleEndian(false),bytesAvailable(0),position(0)
{
	subtype=SUBTYPE_FILESTREAM;
}

void FileStream::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("endian","",c->getSystemState()->getBuiltinFunction(_getEndian,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",c->getSystemState()->getBuiltinFunction(_setEndian),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("open", "", c->getSystemState()->getBuiltinFunction(open), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("close", "", c->getSystemState()->getBuiltinFunction(close), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("readBytes", "", c->getSystemState()->getBuiltinFunction(readBytes), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("readUTF", "", c->getSystemState()->getBuiltinFunction(readUTF,0,Class<ASString>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("readByte", "", c->getSystemState()->getBuiltinFunction(readByte,0,Class<Integer>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("readUnsignedByte", "", c->getSystemState()->getBuiltinFunction(readUnsignedByte,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("readShort", "", c->getSystemState()->getBuiltinFunction(readShort,0,Class<Integer>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("readUnsignedShort", "", c->getSystemState()->getBuiltinFunction(readUnsignedShort,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("readInt", "", c->getSystemState()->getBuiltinFunction(readInt,0,Class<Integer>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("readUnsignedInt", "", c->getSystemState()->getBuiltinFunction(readUnsignedInt,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("writeBytes", "", c->getSystemState()->getBuiltinFunction(writeBytes), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("writeUTF", "", c->getSystemState()->getBuiltinFunction(writeUTF), NORMAL_METHOD, true);
	REGISTER_GETTER_RESULTTYPE(c,bytesAvailable,UInteger);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,position,Number);
}
ASFUNCTIONBODY_GETTER(FileStream, bytesAvailable)
ASFUNCTIONBODY_GETTER_SETTER_CB(FileStream, position,afterPositionChange)

void FileStream::afterPositionChange(number_t oldposition)
{
	bytesAvailable = filesize-position;
	if (stream.is_open())
		stream.seekg(position);
}

ASFUNCTIONBODY_ATOM(FileStream,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, nullptr, 0);
}

ASFUNCTIONBODY_ATOM(FileStream,_getEndian)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if(th->littleEndian)
		ret = asAtomHandler::fromString(wrk->getSystemState(),Endian::littleEndian);
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),Endian::bigEndian);
}

ASFUNCTIONBODY_ATOM(FileStream,_setEndian)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if(asAtomHandler::toString(args[0],wrk) == Endian::littleEndian)
		th->littleEndian = true;
	else if(asAtomHandler::toString(args[0],wrk) == Endian::bigEndian)
		th->littleEndian = false;
	else
		createError<ArgumentError>(wrk,kInvalidEnumError, "endian");
}

ASFUNCTIONBODY_ATOM(FileStream,open)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	ARG_CHECK(ARG_UNPACK(th->file)(th->fileMode));
	tiny_string fullpath = th->file->getFullPath();
	th->bytesAvailable = th->filesize = wrk->getSystemState()->getEngineData()->FileSize(wrk->getSystemState(),fullpath,true);
	if (th->fileMode == "read")
		th->stream.open(th->file->getFullPath().raw_buf(),ios_base::in|ios_base::binary);
	else if (th->fileMode == "write")
	{
		wrk->getSystemState()->getEngineData()->FileCreateDirectory(wrk->getSystemState(),fullpath,true);
		th->stream.open(th->file->getFullPath().raw_buf(),ios_base::out|ios_base::binary|ios_base::trunc);
	}
	else if (th->fileMode == "update")
	{
		wrk->getSystemState()->getEngineData()->FileCreateDirectory(wrk->getSystemState(),fullpath,true);
		th->stream.open(th->file->getFullPath().raw_buf(),ios_base::in|ios_base::out|ios_base::binary);
	}
	else if (th->fileMode == "append")
	{
		wrk->getSystemState()->getEngineData()->FileCreateDirectory(wrk->getSystemState(),fullpath,true);
		th->stream.open(th->file->getFullPath().raw_buf(),ios_base::out|ios_base::binary|ios_base::ate);
	}
	else
		LOG(LOG_ERROR,"invalid filemode:"<<th->fileMode);
	if (!th->stream.is_open())
	{
		LOG(LOG_ERROR,"FileStream couldn't be opened:"<<th->file->getFullPath()<<" "<<th->fileMode);
		createError<IOError>(wrk,0,"FileStream couldn't be opened");
		return;
	}
	th->position=0;
}
ASFUNCTIONBODY_ATOM(FileStream,close)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if (th->stream.is_open())
		th->stream.close();
	th->bytesAvailable = 0;
	th->filesize=0;
	th->position=0;
}
ASFUNCTIONBODY_ATOM(FileStream,readBytes)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	_NR<ByteArray> bytes;
	uint32_t offset;
	uint32_t length;
	ARG_CHECK(ARG_UNPACK(bytes)(offset,0)(length,UINT32_MAX));
	if (!th->stream.is_open())
	{
		createError<IOError>(wrk,0,"FileStream is not open");
		return;
	}
	if (th->fileMode != "read" && th->fileMode != "update")
	{
		createError<IOError>(wrk,0,"FileStream is not readable");
		return;
	}
	if (length != UINT32_MAX && th->bytesAvailable < length)
	{
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	length = min(length,th->bytesAvailable);
	th->stream.read((char*)(bytes->getBuffer(max(bytes->getLength(),length+offset),true)+offset),length);
	th->position+= length;
	th->bytesAvailable = th->filesize-th->position;
}
ASFUNCTIONBODY_ATOM(FileStream,readUTF)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if (!th->stream.is_open())
	{
		createError<IOError>(wrk,0,"FileStream is not open");
		return;
	}
	if (th->fileMode != "read" && th->fileMode != "update")
	{
		createError<IOError>(wrk,0,"FileStream is not readable");
		return;
	}
	if (th->bytesAvailable < 2)
	{
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	uint16_t len;
	th->stream.read((char*)&len,2);
	len = th->endianOut(len);
	th->position+=2;
	if (th->bytesAvailable < (uint32_t)len)
	{
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	tiny_string s(th->stream,len);
	th->position+= len;
	th->bytesAvailable = th->filesize-th->position;
	ret = asAtomHandler::fromString(wrk->getSystemState(),s);
}
ASFUNCTIONBODY_ATOM(FileStream,readByte)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if (!th->stream.is_open())
	{
		createError<IOError>(wrk,0,"FileStream is not open");
		return;
	}
	if (th->fileMode != "read" && th->fileMode != "update")
	{
		createError<IOError>(wrk,0,"FileStream is not readable");
		return;
	}
	if (th->bytesAvailable==0)
	{
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	int8_t b;
	th->stream.read((char*)&b,1);
	th->position++;
	th->bytesAvailable = th->filesize-th->position;
	ret= asAtomHandler::fromInt(b);
}
ASFUNCTIONBODY_ATOM(FileStream,readUnsignedByte)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if (!th->stream.is_open())
	{
		createError<IOError>(wrk,0,"FileStream is not open");
		return;
	}
	if (th->fileMode != "read" && th->fileMode != "update")
	{
		createError<IOError>(wrk,0,"FileStream is not readable");
		return;
	}
	if (th->bytesAvailable==0)
	{
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	uint8_t b;
	th->stream.read((char*)&b,1);
	th->position++;
	th->bytesAvailable = th->filesize-th->position;
	ret= asAtomHandler::fromUInt(b);
}
ASFUNCTIONBODY_ATOM(FileStream,readShort)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if (!th->stream.is_open())
	{
		createError<IOError>(wrk,0,"FileStream is not open");
		return;
	}
	if (th->fileMode != "read" && th->fileMode != "update")
	{
		createError<IOError>(wrk,0,"FileStream is not readable");
		return;
	}
	if (th->bytesAvailable<2)
	{
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	uint16_t val;
	th->stream.read((char*)&val,2);
	th->position+=2;
	th->bytesAvailable = th->filesize-th->position;
	ret= asAtomHandler::fromInt((int32_t)th->endianOut(val));
}
ASFUNCTIONBODY_ATOM(FileStream,readUnsignedShort)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if (!th->stream.is_open())
	{
		createError<IOError>(wrk,0,"FileStream is not open");
		return;
	}
	if (th->fileMode != "read" && th->fileMode != "update")
	{
		createError<IOError>(wrk,0,"FileStream is not readable");
		return;
	}
	if (th->bytesAvailable<2)
	{
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	uint16_t val;
	th->stream.read((char*)&val,2);
	th->position+=2;
	th->bytesAvailable = th->filesize-th->position;
	ret= asAtomHandler::fromUInt(th->endianOut(val));
}
ASFUNCTIONBODY_ATOM(FileStream,readInt)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if (!th->stream.is_open())
	{
		createError<IOError>(wrk,0,"FileStream is not open");
		return;
	}
	if (th->fileMode != "read" && th->fileMode != "update")
	{
		createError<IOError>(wrk,0,"FileStream is not readable");
		return;
	}
	if (th->bytesAvailable<4)
	{
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	uint32_t val;
	th->stream.read((char*)&val,4);
	th->position+=4;
	th->bytesAvailable = th->filesize-th->position;
	ret= asAtomHandler::fromInt((int32_t)th->endianOut(val));
}
ASFUNCTIONBODY_ATOM(FileStream,readUnsignedInt)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if (!th->stream.is_open())
	{
		createError<IOError>(wrk,0,"FileStream is not open");
		return;
	}
	if (th->fileMode != "read" && th->fileMode != "update")
	{
		createError<IOError>(wrk,0,"FileStream is not readable");
		return;
	}
	if (th->bytesAvailable<4)
	{
		createError<EOFError>(wrk,kEOFError);
		return;
	}
	uint32_t val;
	th->stream.read((char*)&val,4);
	th->position+=4;
	th->bytesAvailable = th->filesize-th->position;
	ret= asAtomHandler::fromUInt(th->endianOut(val));
}
ASFUNCTIONBODY_ATOM(FileStream,writeBytes)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	_NR<ByteArray> bytes;
	uint32_t offset;
	uint32_t length;
	ARG_CHECK(ARG_UNPACK(bytes)(offset,0)(length,UINT32_MAX));
	if (!th->stream.is_open())
	{
		createError<IOError>(wrk,0,"FileStream is not open");
		return;
	}
	if (th->fileMode == "read")
	{
		createError<IOError>(wrk,0,"FileStream is not writable");
		return;
	}
	uint8_t* buf = bytes->getBuffer(bytes->getLength(),false);
	uint32_t len = length;
	if (length == UINT32_MAX || offset+length > bytes->getLength())
		len = bytes->getLength()-offset;
	th->stream.write((char*)buf+offset,len);
}
ASFUNCTIONBODY_ATOM(FileStream,writeUTF)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	tiny_string value;
	ARG_CHECK(ARG_UNPACK(value));
	if (!th->stream.is_open())
	{
		createError<IOError>(wrk,0,"FileStream is not open");
		return;
	}
	if (th->fileMode == "read")
	{
		createError<IOError>(wrk,0,"FileStream is not writable");
		return;
	}
	uint16_t len = value.numBytes();
	len = th->endianIn(len);
	th->stream.write((char*)&len,2);
	th->stream.write(value.raw_buf(),value.numBytes());
}

ASFile::ASFile(ASWorker* wrk,Class_base* c, const tiny_string _path, bool _exists, bool _isHidden, bool _isDirectory):
	FileReference(wrk,c),path(_path),exists(_exists),isHidden(_isHidden),isDirectory(_isDirectory)
{
	subtype=SUBTYPE_FILE;
	setupFile(_path,wrk);
}

void ASFile::sinit(Class_base* c)
{
	CLASS_SETUP(c, FileReference, _constructor, CLASS_SEALED);
	REGISTER_GETTER_RESULTTYPE(c,exists,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,isHidden,Boolean);
	REGISTER_GETTER_RESULTTYPE(c,isDirectory,Boolean);
	REGISTER_GETTER_STATIC_RESULTTYPE(c,applicationDirectory,ASFile);
	REGISTER_GETTER_STATIC_RESULTTYPE(c,applicationStorageDirectory,ASFile);
	c->setDeclaredMethodByQName("resolvePath", "", c->getSystemState()->getBuiltinFunction(resolvePath,1,Class<ASFile>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("createDirectory", "", c->getSystemState()->getBuiltinFunction(createDirectory), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("getDirectoryListing", "", c->getSystemState()->getBuiltinFunction(getDirectoryListing,0,Class<Array>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("url","",c->getSystemState()->getBuiltinFunction(_getURL,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("url","",c->getSystemState()->getBuiltinFunction(_setURL),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("nativePath","",c->getSystemState()->getBuiltinFunction(_getNativePath,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nativePath","",c->getSystemState()->getBuiltinFunction(_setNativePath),SETTER_METHOD,true);
}
ASFUNCTIONBODY_GETTER(ASFile, exists)
ASFUNCTIONBODY_GETTER(ASFile, isHidden)
ASFUNCTIONBODY_GETTER(ASFile, isDirectory)
ASFUNCTIONBODY_GETTER_STATIC(ASFile, applicationDirectory)
ASFUNCTIONBODY_GETTER_STATIC(ASFile, applicationStorageDirectory)

ASFUNCTIONBODY_ATOM(ASFile,_constructor)
{
	FileReference::_constructor(ret,wrk,obj, nullptr, 0);
	ASFile* th=asAtomHandler::as<ASFile>(obj);
	tiny_string filename;
	ARG_CHECK(ARG_UNPACK(filename,""));
	th->setupFile(filename,wrk);
}
void ASFile::setupFile(const tiny_string& filename, ASWorker* wrk)
{
	if (!filename.empty())
	{
		// TODO handle URLs (see actionscript3 documentation)
		// "app:/DesktopPathTest.xml"
		// "app-storage:/preferences.xml"
		// "file:///C:/Documents%20and%20Settings/bob/Desktop" (the desktop on Bob's Windows computer)
		// "file:///Users/bob/Desktop" (the desktop on Bob's Mac computer)
		path = filename;
		exists = wrk->getSystemState()->getEngineData()->FileExists(wrk->getSystemState(),path,true);
		isHidden = wrk->getSystemState()->getEngineData()->FileIsHidden(wrk->getSystemState(),path,true);
		isDirectory = wrk->getSystemState()->getEngineData()->FileIsDirectory(wrk->getSystemState(),path,true);
		size = wrk->getSystemState()->getEngineData()->FileSize(wrk->getSystemState(),path,true);
		name = wrk->getSystemState()->getEngineData()->FileBasename(wrk->getSystemState(),path,true);
	}
}


ASFUNCTIONBODY_ATOM(ASFile,_getURL)
{
	ASFile* th=asAtomHandler::as<ASFile>(obj);
	tiny_string url = URLInfo::encode(th->path,URLInfo::ENCODE_URI);
	url = tiny_string("file://")+url;
	ret = asAtomHandler::fromString(wrk->getSystemState(),url);
}
ASFUNCTIONBODY_ATOM(ASFile,_setURL)
{
	ASFile* th=asAtomHandler::as<ASFile>(obj);
	tiny_string url;
	ARG_CHECK(ARG_UNPACK(url));
	if (url.find("file://")==0)
	{
		tiny_string fullpath = URLInfo::decode(url.substr_bytes(7,UINT32_MAX),URLInfo::ENCODE_URI);
		th->setupFile(fullpath,wrk);
	}
	else
		createError<ArgumentError>(wrk,kInvalidParamError, "url");
}
ASFUNCTIONBODY_ATOM(ASFile,_getNativePath)
{
	ASFile* th=asAtomHandler::as<ASFile>(obj);
	ret = asAtomHandler::fromString(wrk->getSystemState(),th->path);
}
ASFUNCTIONBODY_ATOM(ASFile,_setNativePath)
{
	ASFile* th=asAtomHandler::as<ASFile>(obj);
	tiny_string newpath;
	ARG_CHECK(ARG_UNPACK(newpath));
	th->setupFile(newpath,wrk);
}

ASFUNCTIONBODY_ATOM(ASFile,resolvePath)
{
	ASFile* th=asAtomHandler::as<ASFile>(obj);
	tiny_string path;
	ARG_CHECK(ARG_UNPACK(path));
	tiny_string fullpath;
	if (wrk->getSystemState()->getEngineData()->FilePathIsAbsolute(path))
		fullpath = path;
	else
	{
		fullpath = th->path;
		fullpath += G_DIR_SEPARATOR_S;
		fullpath += path;
	}
	ASFile* res = Class<ASFile>::getInstanceS(wrk,fullpath);
	ret = asAtomHandler::fromObjectNoPrimitive(res);
}

ASFUNCTIONBODY_ATOM(ASFile,createDirectory)
{
	ASFile* th=asAtomHandler::as<ASFile>(obj);
	tiny_string p = th->path;
	// ensure that directory name ends with a directory separator
	if (!p.endsWith(G_DIR_SEPARATOR_S))
		p += G_DIR_SEPARATOR_S;
	
	if (!wrk->getSystemState()->getEngineData()->FileCreateDirectory(wrk->getSystemState(),p,true))
		createError<IOError>(wrk,kFileWriteError,th->path);
}

ASFUNCTIONBODY_ATOM(ASFile,getDirectoryListing)
{
	ASFile* th=asAtomHandler::as<ASFile>(obj);
	tiny_string p = th->path;
	// ensure that directory name ends with a directory separator
	if (!p.endsWith(G_DIR_SEPARATOR_S))
		p += G_DIR_SEPARATOR_S;
	std::vector<tiny_string> dirlist;
	if (!wrk->getSystemState()->getEngineData()->FilGetDirectoryListing(wrk->getSystemState(),p,true,dirlist))
	{
		createError<IOError>(wrk,kFileOpenError,th->path);
		return;
	}
	Array* res =Class<Array>::getInstanceSNoArgs(wrk);
	for (auto it = dirlist.begin(); it != dirlist.end(); it++)
	{
		ASFile* f=Class<ASFile>::getInstanceSNoArgs(wrk);
		tiny_string fullpath = p+(*it);
		f->setupFile(fullpath,wrk);
		res->push(asAtomHandler::fromObjectNoPrimitive(f));
	}
	ret=asAtomHandler::fromObjectNoPrimitive(res);
}

void FileMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("APPEND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"append"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("READ",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"read"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UPDATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"update"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("WRITE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"write"),CONSTANT_TRAIT);
}
