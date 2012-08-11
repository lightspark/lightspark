/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

ASFUNCTIONBODY(IOError,_constructor)
{
	IOError* th=static_cast<IOError*>(obj);
	ARG_UNPACK(th->message, "");
	return NULL;
}

void IOError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
}

void IOError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(EOFError,_constructor)
{
	EOFError* th=static_cast<EOFError*>(obj);
	ARG_UNPACK(th->message, "");
	return NULL;
}

void EOFError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<IOError>::getRef());
}

void EOFError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(IllegalOperationError,_constructor)
{
	IllegalOperationError* th=static_cast<IllegalOperationError*>(obj);
	ARG_UNPACK(th->message, "");
	return NULL;
}

void IllegalOperationError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
}

void IllegalOperationError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(InvalidSWFError,_constructor)
{
	InvalidSWFError* th=static_cast<InvalidSWFError*>(obj);
	int32_t errorID;
	ARG_UNPACK(th->message, "") (errorID, 0);
	th->setErrorID(errorID);
	return NULL;
}

void InvalidSWFError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
}

void InvalidSWFError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(MemoryError,_constructor)
{
	MemoryError* th=static_cast<MemoryError*>(obj);
	ARG_UNPACK(th->message, "");
	return NULL;
}

void MemoryError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
}

void MemoryError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(ScriptTimeoutError,_constructor)
{
	ScriptTimeoutError* th=static_cast<ScriptTimeoutError*>(obj);
	ARG_UNPACK(th->message, "");
	return NULL;
}

void ScriptTimeoutError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
}

void ScriptTimeoutError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(StackOverflowError,_constructor)
{
	StackOverflowError* th=static_cast<StackOverflowError*>(obj);
	ARG_UNPACK(th->message, "");
	return NULL;
}

void StackOverflowError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
}

void StackOverflowError::buildTraits(ASObject* o)
{
}
