/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2025 Ludger Krämer <dbluelle@onlinehome.de>

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


#include "scripting/flash/net/FileReferenceList.h"
#include "scripting/toplevel/Array.h"
#include "scripting/class.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;


FileReferenceList::FileReferenceList(ASWorker* wrk, Class_base* c):
	EventDispatcher(wrk,c),fileList(asAtomHandler::nullAtom)
{
	subtype = SUBTYPE_FILEREFERENCELIST;
}
void FileReferenceList::finalize()
{
	EventDispatcher::finalize();
	ASATOM_REMOVESTOREDMEMBER(fileList);
	fileList = asAtomHandler::nullAtom;
}
bool FileReferenceList::destruct()
{
	ASATOM_REMOVESTOREDMEMBER(fileList);
	fileList = asAtomHandler::nullAtom;
	return EventDispatcher::destruct();
}

void FileReferenceList::sinit(Class_base* c)
{
	CLASS_SETUP(c, EventDispatcher, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("browse","",c->getSystemState()->getBuiltinFunction(browse),NORMAL_METHOD,true);
	REGISTER_GETTER_RESULTTYPE(c,fileList,Array);
}
ASFUNCTIONBODY_GETTER(FileReferenceList, fileList)

ASFUNCTIONBODY_ATOM(FileReferenceList,_constructor)
{
	EventDispatcher::_constructor(ret,wrk,obj, NULL, 0);
	LOG(LOG_NOT_IMPLEMENTED,"FileReferenceList is not implemented");
}

ASFUNCTIONBODY_ATOM(FileReferenceList,browse)
{
	LOG(LOG_NOT_IMPLEMENTED,"FileReferenceList.browse does nothing");
}
