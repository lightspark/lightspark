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

#include "scripting/flash/errors/flasherrors.h"
#include "scripting/argconv.h"
#include "scripting/class.h"

using namespace lightspark;

ASFUNCTIONBODY_ATOM(IOError,_constructor)
{
	IOError* th=obj.as<IOError>();
	ARG_UNPACK_ATOM(th->message, "");
	return asAtom::invalidAtom;
}

void IOError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void IOError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(EOFError,_constructor)
{
	EOFError* th=obj.as<EOFError>();
	ARG_UNPACK_ATOM(th->message, "");
	return asAtom::invalidAtom;
}

void EOFError::sinit(Class_base* c)
{
	CLASS_SETUP(c, IOError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void EOFError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(IllegalOperationError,_constructor)
{
	IllegalOperationError* th=obj.as<IllegalOperationError>();
	ARG_UNPACK_ATOM(th->message, "");
	return asAtom::invalidAtom;
}

void IllegalOperationError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void IllegalOperationError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(InvalidSWFError,_constructor)
{
	InvalidSWFError* th=obj.as<InvalidSWFError>();
	int32_t errorID;
	ARG_UNPACK_ATOM(th->message, "") (errorID, 0);
	th->setErrorID(errorID);
	return asAtom::invalidAtom;
}

void InvalidSWFError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void InvalidSWFError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(MemoryError,_constructor)
{
	MemoryError* th=obj.as<MemoryError>();
	ARG_UNPACK_ATOM(th->message, "");
	return asAtom::invalidAtom;
}

void MemoryError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void MemoryError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(ScriptTimeoutError,_constructor)
{
	ScriptTimeoutError* th=obj.as<ScriptTimeoutError>();
	ARG_UNPACK_ATOM(th->message, "");
	return asAtom::invalidAtom;
}

void ScriptTimeoutError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void ScriptTimeoutError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(StackOverflowError,_constructor)
{
	StackOverflowError* th=obj.as<StackOverflowError>();
	ARG_UNPACK_ATOM(th->message, "");
	return asAtom::invalidAtom;
}

void StackOverflowError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void StackOverflowError::buildTraits(ASObject* o)
{
}
