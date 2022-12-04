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
	IOError* th=asAtomHandler::as<IOError>(obj);
	ARG_CHECK(ARG_UNPACK(th->message, ""));
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
	EOFError* th=asAtomHandler::as<EOFError>(obj);
	ARG_CHECK(ARG_UNPACK(th->message, ""));
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
	IllegalOperationError* th=asAtomHandler::as<IllegalOperationError>(obj);
	ARG_CHECK(ARG_UNPACK(th->message, ""));
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
	InvalidSWFError* th=asAtomHandler::as<InvalidSWFError>(obj);
	int32_t errorID;
	ARG_CHECK(ARG_UNPACK(th->message, "") (errorID, 0));
	th->setErrorID(errorID);
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
	MemoryError* th=asAtomHandler::as<MemoryError>(obj);
	ARG_CHECK(ARG_UNPACK(th->message, ""));
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
	ScriptTimeoutError* th=asAtomHandler::as<ScriptTimeoutError>(obj);
	ARG_CHECK(ARG_UNPACK(th->message, ""));
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
	StackOverflowError* th=asAtomHandler::as<StackOverflowError>(obj);
	ARG_CHECK(ARG_UNPACK(th->message, ""));
}

void StackOverflowError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void StackOverflowError::buildTraits(ASObject* o)
{
}
