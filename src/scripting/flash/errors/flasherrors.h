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

#ifndef _FLASH_ERRORS_H
#define _FLASH_ERRORS_H

#include "toplevel/Error.h"
#include "swftypes.h"

namespace lightspark
{

class IOError: public ASError
{
CLASSBUILDABLE(IOError);
public:
	IOError(const tiny_string& error_message = "", int id = 0, tiny_string name = "IOError") : ASError(error_message, id, name){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class EOFError: public IOError
{
CLASSBUILDABLE(EOFError);
public:
	EOFError(const tiny_string& error_message = "", int id = 0) : IOError(error_message, id, "EOFError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class IllegalOperationError: public ASError
{
CLASSBUILDABLE(IllegalOperationError);
public:
	IllegalOperationError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "IllegalOperationError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class InvalidSWFError: public ASError
{
CLASSBUILDABLE(InvalidSWFError);
public:
	InvalidSWFError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "InvalidSWFError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class MemoryError: public ASError
{
CLASSBUILDABLE(MemoryError);
public:
	MemoryError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "MemoryError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class ScriptTimeoutError: public ASError
{
CLASSBUILDABLE(ScriptTimeoutError);
public:
	ScriptTimeoutError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "ScriptTimeoutError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class StackOverflowError: public ASError
{
CLASSBUILDABLE(StackOverflowError);
public:
	StackOverflowError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "StackOverflowError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

};

#endif
