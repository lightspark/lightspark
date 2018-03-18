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
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "compat.h"

using namespace lightspark;

FileStream::FileStream(Class_base* c):
	EventDispatcher(c),isSupported(false)
{
}

void FileStream::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	REGISTER_GETTER(c,isSupported);
}
ASFUNCTIONBODY_GETTER(FileStream, isSupported);

ASFUNCTIONBODY_ATOM(FileStream,_constructor)
{
	EventDispatcher::_constructor(ret,sys,obj, NULL, 0);
	//FileStream* th=Class<FileStream>::cast(obj);
	LOG(LOG_NOT_IMPLEMENTED,"FileStream is not implemented");
}

ASFile::ASFile(Class_base* c):
	FileReference(c),exists(false)
{
}

void ASFile::sinit(Class_base* c)
{
	CLASS_SETUP(c, FileReference, _constructor, CLASS_SEALED);
	REGISTER_GETTER(c,exists);
}
ASFUNCTIONBODY_GETTER(ASFile, exists);

ASFUNCTIONBODY_ATOM(ASFile,_constructor)
{
	FileReference::_constructor(ret,sys,obj, NULL, 0);
	//ASFile* th=Class<FileReference>::cast(obj);
	LOG(LOG_NOT_IMPLEMENTED,"File is not implemented");
}
