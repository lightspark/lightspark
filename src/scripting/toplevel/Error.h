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

#ifndef SCRIPTING_TOPLEVEL_ERROR_H
#define SCRIPTING_TOPLEVEL_ERROR_H 1

#include "asobject.h"

namespace lightspark
{

class ASError: public ASObject
{
private:
	ASPROPERTY_GETTER(int32_t, errorID);
	ASPROPERTY_GETTER_SETTER(tiny_string, name);
protected:
	ASPROPERTY_GETTER_SETTER(tiny_string, message);
	void setErrorID(int32_t id) { errorID=id; }
	static void errorGenerator(ASError *obj, ASObject* const* args, const unsigned int argslen);
public:
	ASError(Class_base* c, const tiny_string& error_message = "", int id = 0, const tiny_string& error_name="Error");
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	ASFUNCTION(getStackTrace);
	ASFUNCTION(_toString);
	tiny_string toString(bool debugMsg=false);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class SecurityError: public ASError
{
public:
	SecurityError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "SecurityError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class ArgumentError: public ASError
{
public:
	ArgumentError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "ArgumentError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class DefinitionError: public ASError
{
public:
	DefinitionError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "DefinitionError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class EvalError: public ASError
{
public:
	EvalError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "EvalError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class RangeError: public ASError
{
public:
	RangeError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "RangeError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class ReferenceError: public ASError
{
public:
	ReferenceError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "ReferenceError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class SyntaxError: public ASError
{
public:
	SyntaxError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "SyntaxError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class TypeError: public ASError
{
public:
	TypeError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "TypeError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class URIError: public ASError
{
public:
	URIError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "URIError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class VerifyError: public ASError
{
public:
	VerifyError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "VerifyError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

/* Not found in the spec but used by testcases */
class UninitializedError: public ASError
{
public:
	UninitializedError(Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(c, error_message, id, "UninitializedError"){}
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

}

#endif /* SCRIPTING_TOPLEVEL_ERROR_H */
