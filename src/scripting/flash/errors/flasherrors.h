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

#ifndef SCRIPTING_FLASH_ERRORS_FLASHERRORS_H
#define SCRIPTING_FLASH_ERRORS_FLASHERRORS_H 1

#include "scripting/toplevel/Error.h"
#include "swftypes.h"

namespace lightspark
{

class IOError: public ASError
{
public:
	IOError(Class_base* c,const tiny_string& error_message = "", int id = 0, tiny_string name = "IOError"):
		ASError(c, error_message, id, name){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class EOFError: public IOError
{
public:
	EOFError(Class_base* c, const tiny_string& error_message = "", int id = 0) : IOError(c, error_message, id, "EOFError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class IllegalOperationError: public ASError
{
public:
	IllegalOperationError(Class_base* c, const tiny_string& error_message = "", int id = 0):
		ASError(c, error_message, id, "IllegalOperationError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class InvalidSWFError: public ASError
{
public:
	InvalidSWFError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "InvalidSWFError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class MemoryError: public ASError
{
public:
	MemoryError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "MemoryError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class ScriptTimeoutError: public ASError
{
public:
	ScriptTimeoutError(Class_base* c, const tiny_string& error_message = "", int id = 0):
		ASError(c, error_message, id, "ScriptTimeoutError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class StackOverflowError: public ASError
{
public:
	StackOverflowError(Class_base* c, const tiny_string& error_message = "", int id = 0):
		ASError(c, error_message, id, "StackOverflowError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

};

#endif /* SCRIPTING_FLASH_ERRORS_FLASHERRORS_H */
