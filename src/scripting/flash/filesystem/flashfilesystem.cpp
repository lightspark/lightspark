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
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/Number.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "compat.h"
#include "platforms/engineutils.h"

using namespace lightspark;

FileStream::FileStream(Class_base* c):
	EventDispatcher(c),filesize(0),bytesAvailable(0),position(0)
{
	subtype=SUBTYPE_FILESTREAM;
}

void FileStream::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("open", "", Class<IFunction>::getFunction(c->getSystemState(),open), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("close", "", Class<IFunction>::getFunction(c->getSystemState(),close), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("readBytes", "", Class<IFunction>::getFunction(c->getSystemState(),readBytes), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("readUnsignedByte", "", Class<IFunction>::getFunction(c->getSystemState(),readUnsignedByte,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("writeBytes", "", Class<IFunction>::getFunction(c->getSystemState(),writeBytes), NORMAL_METHOD, true);
	REGISTER_GETTER_RESULTTYPE(c,bytesAvailable,UInteger);
	REGISTER_GETTER_SETTER_RESULTTYPE(c,position,Number);
}
ASFUNCTIONBODY_GETTER(FileStream, bytesAvailable);
ASFUNCTIONBODY_GETTER_SETTER_CB(FileStream, position,afterPositionChange);
void FileStream::afterPositionChange(number_t oldposition)
{
	bytesAvailable = filesize-position;
	if (stream.is_open())
		stream.seekg(position);
}

ASFUNCTIONBODY_ATOM(FileStream,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, nullptr, 0);
}

ASFUNCTIONBODY_ATOM(FileStream,open)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	ARG_UNPACK_ATOM(th->file)(th->fileMode);
	th->bytesAvailable = th->filesize = sys->getEngineData()->FileSize(sys,th->file->getFullPath(),true);
	if (th->fileMode == "read")
		th->stream.open(th->file->getFullPath().raw_buf(),ios_base::in|ios_base::binary);
	else if (th->fileMode == "write")
		th->stream.open(th->file->getFullPath().raw_buf(),ios_base::out|ios_base::binary|ios_base::trunc);
	else if (th->fileMode == "update")
		th->stream.open(th->file->getFullPath().raw_buf(),ios_base::in|ios_base::out|ios_base::binary);
	else if (th->fileMode == "append")
		th->stream.open(th->file->getFullPath().raw_buf(),ios_base::out|ios_base::binary|ios_base::ate);
	else
		LOG(LOG_ERROR,"invalid filemode:"<<th->fileMode);
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
	ARG_UNPACK_ATOM(bytes)(offset,0)(length,UINT32_MAX);
	if (!th->stream.is_open())
		throw Class<IOError>::getInstanceS(sys,"FileStream is not open");
	if (th->fileMode != "read" && th->fileMode != "update")
		throw Class<IOError>::getInstanceS(sys,"FileStream is not readable");
	if (length != UINT32_MAX && th->bytesAvailable < length)
		throwError<EOFError>(kEOFError);
	length = min(length,th->bytesAvailable);
	th->stream.read((char*)(bytes->getBufferNoCheck()+offset),length);
	th->position+= length;
	th->bytesAvailable = th->filesize-th->position;
}
ASFUNCTIONBODY_ATOM(FileStream,readUnsignedByte)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	if (!th->stream.is_open())
		throw Class<IOError>::getInstanceS(sys,"FileStream is not open");
	if (th->fileMode != "read" && th->fileMode != "update")
		throw Class<IOError>::getInstanceS(sys,"FileStream is not readable");
	if (th->bytesAvailable==0)
		throwError<EOFError>(kEOFError);
	uint8_t b;
	th->stream>>b;
	th->position++;
	th->bytesAvailable = th->filesize-th->position;
	ret= asAtomHandler::fromUInt(b);
}
ASFUNCTIONBODY_ATOM(FileStream,writeBytes)
{
	FileStream* th=asAtomHandler::as<FileStream>(obj);
	_NR<ByteArray> bytes;
	uint32_t offset;
	uint32_t length;
	ARG_UNPACK_ATOM(bytes)(offset,0)(length,UINT32_MAX);
	if (!th->stream.is_open())
		throw Class<IOError>::getInstanceS(sys,"FileStream is not open");
	if (th->fileMode == "read")
		throw Class<IOError>::getInstanceS(sys,"FileStream is not writable");
	uint8_t* buf = bytes->getBuffer(bytes->getLength(),false);
	uint32_t len = length;
	if (length == UINT32_MAX || offset+length > bytes->getLength())
		len = bytes->getLength()-offset;
	th->stream.write((char*)buf+offset,len);
}

ASFile::ASFile(Class_base* c, const tiny_string _path, bool _exists):
	FileReference(c),path(_path),exists(_exists)
{
	subtype=SUBTYPE_FILE;
}

void ASFile::sinit(Class_base* c)
{
	CLASS_SETUP(c, FileReference, _constructor, CLASS_SEALED);
	REGISTER_GETTER_RESULTTYPE(c,exists,Boolean);
	REGISTER_GETTER_STATIC_RESULTTYPE(c,applicationDirectory,ASFile);
	c->setDeclaredMethodByQName("resolvePath", "", Class<IFunction>::getFunction(c->getSystemState(),resolvePath,1,Class<ASFile>::getRef(c->getSystemState()).getPtr()), NORMAL_METHOD, true);
	c->setDeclaredMethodByQName("createDirectory", "", Class<IFunction>::getFunction(c->getSystemState(),createDirectory), NORMAL_METHOD, true);
}
ASFUNCTIONBODY_GETTER(ASFile, exists);
ASFUNCTIONBODY_GETTER_STATIC(ASFile, applicationDirectory);

ASFUNCTIONBODY_ATOM(ASFile,_constructor)
{
	FileReference::_constructor(ret,sys,obj, nullptr, 0);
	ASFile* th=asAtomHandler::as<ASFile>(obj);
	tiny_string filename;
	ARG_UNPACK_ATOM(filename,"");
	if (filename!="")
	{
		// TODO handle URLs (see actionscript3 documentation)
		// "app:/DesktopPathTest.xml"
		// "app-storage:/preferences.xml"
		// "file:///C:/Documents%20and%20Settings/bob/Desktop" (the desktop on Bob's Windows computer)
		// "file:///Users/bob/Desktop" (the desktop on Bob's Mac computer)
		th->path = sys->getEngineData()->FileFullPath(sys,filename);
		th->exists = sys->getEngineData()->FileExists(sys,th->path,true);
	}
}
ASFUNCTIONBODY_ATOM(ASFile,resolvePath)
{
	//ASFile* th=Class<FileReference>::cast(obj);
	LOG(LOG_NOT_IMPLEMENTED,"File.resolvePath is not implemented");
	ASFile* res = Class<ASFile>::getInstanceS(sys);
	ret = asAtomHandler::fromObjectNoPrimitive(res);
}

ASFUNCTIONBODY_ATOM(ASFile,createDirectory)
{
	ASFile* th=asAtomHandler::as<ASFile>(obj);
	if (!sys->getEngineData()->FileCreateDirectory(sys,th->path,true))
		throwError<IOError>(kFileWriteError,th->path);
}

void FileMode::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("APPEND",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"append"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("READ",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"read"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UPDATE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"update"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("WRITE",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"write"),CONSTANT_TRAIT);
}
