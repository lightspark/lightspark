/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef ERROR_H_
#define ERROR_H_

#include "asobject.h"

namespace lightspark
{

class ASError: public ASObject
{
CLASSBUILDABLE(ASError);
protected:
	ASPROPERTY_GETTER_SETTER(tiny_string, message);
private:
	ASPROPERTY_GETTER(int32_t, errorID);
	ASPROPERTY_GETTER_SETTER(tiny_string, name);
public:
	ASError(const tiny_string& error_message = "", int id = 0, const tiny_string& error_name="Error") : message(error_message), errorID(id), name(error_name){};
	ASFUNCTION(_constructor);
	ASFUNCTION(getStackTrace);
	ASFUNCTION(_toString);
	tiny_string toString(bool debugMsg=false);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class SecurityError: public ASError
{
CLASSBUILDABLE(SecurityError);
public:
	SecurityError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "SecurityError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class ArgumentError: public ASError
{
CLASSBUILDABLE(ArgumentError);
public:
	ArgumentError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "ArgumentError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class DefinitionError: public ASError
{
CLASSBUILDABLE(DefinitionError);
public:
	DefinitionError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "DefinitionError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class EvalError: public ASError
{
CLASSBUILDABLE(EvalError);
public:
	EvalError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "EvalError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class RangeError: public ASError
{
CLASSBUILDABLE(RangeError);
public:
	RangeError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "RangeError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class ReferenceError: public ASError
{
CLASSBUILDABLE(ReferenceError);
public:
	ReferenceError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "ReferenceError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class SyntaxError: public ASError
{
CLASSBUILDABLE(SyntaxError);
public:
	SyntaxError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "SyntaxError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class TypeError: public ASError
{
CLASSBUILDABLE(TypeError);
public:
	TypeError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "TypeError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class URIError: public ASError
{
CLASSBUILDABLE(URIError);
public:
	URIError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "URIError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class VerifyError: public ASError
{
CLASSBUILDABLE(VerifyError);
public:
	VerifyError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "VerifyError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

/* Not found in the spec but used by testcases */
class UninitializedError: public ASError
{
CLASSBUILDABLE(UninitializedError);
	UninitializedError(const tiny_string& error_message = "", int id = 0) : ASError(error_message, id, "UninitializedError"){}
	ASFUNCTION(_constructor);
	static void sinit(Class_base* c) {}
	static void buildTraits(ASObject* o) {}
};

}

#endif /* ERROR_H_ */
