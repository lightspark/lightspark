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

#ifndef SCRIPTING_TOPLEVEL_ERROR_H
#define SCRIPTING_TOPLEVEL_ERROR_H 1

#include "errorconstants.h"
#include "asobject.h"

namespace lightspark
{

tiny_string createErrorMessage(int errorID, const tiny_string& arg1, const tiny_string& arg2, const tiny_string& arg3);
void setError(ASObject* error);
/*
 * Retrieves the error message for the errorID, constructs an Error
 * object and sets it in the current call_contexts to be handled.
 * if no call_context is set, the error object is thrown as an exception
 */
template<class T>
void createError(ASWorker* wrk, int errorID, const tiny_string& arg1="", const tiny_string& arg2="", const tiny_string& arg3="")
{
	tiny_string message = createErrorMessage(errorID, arg1, arg2, arg3);
	setError(Class<T>::getInstanceS(wrk,message, errorID));
}

class ASError: public ASObject
{
private:
	tiny_string stacktrace;
	ASPROPERTY_GETTER(int32_t, errorID);
	ASPROPERTY_GETTER_SETTER(tiny_string, name);
protected:
	ASPROPERTY_GETTER_SETTER(tiny_string, message);
	void setErrorID(int32_t id) { errorID=id; }
	static void errorGenerator(ASError *obj, asAtom *args, const unsigned int argslen);
public:
	ASError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0, const tiny_string& error_name="Error");
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_getStackTrace);
	ASFUNCTION_ATOM(_toString);
	tiny_string toString();
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	tiny_string getStackTraceString();
};

class SecurityError: public ASError
{
public:
	SecurityError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "SecurityError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class ArgumentError: public ASError
{
public:
	ArgumentError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "ArgumentError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class DefinitionError: public ASError
{
public:
	DefinitionError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "DefinitionError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class EvalError: public ASError
{
public:
	EvalError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "EvalError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class RangeError: public ASError
{
public:
	RangeError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "RangeError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class ReferenceError: public ASError
{
public:
	ReferenceError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "ReferenceError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class SyntaxError: public ASError
{
public:
	SyntaxError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "SyntaxError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class TypeError: public ASError
{
public:
	TypeError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "TypeError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class URIError: public ASError
{
public:
	URIError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "URIError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

class VerifyError: public ASError
{
public:
	VerifyError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "VerifyError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

/* Not found in the spec but used by testcases */
class UninitializedError: public ASError
{
public:
	UninitializedError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "UninitializedError"){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
};

}

#endif /* SCRIPTING_TOPLEVEL_ERROR_H */
