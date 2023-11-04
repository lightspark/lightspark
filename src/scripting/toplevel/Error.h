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

template<class T>
void createErrorWithMessage(ASWorker* wrk, int errorID, const tiny_string& message)
{
	setError(Class<T>::getInstanceS(wrk,message, errorID));
}

class ASError: public ASObject
{
private:
	std::vector<std::pair<uint32_t,uint32_t>> stacktrace;
	ASPROPERTY_GETTER(int32_t, errorID);
	ASPROPERTY_GETTER_SETTER(tiny_string, name);
protected:
	ASPROPERTY_GETTER_SETTER(tiny_string, message);
	void setErrorID(int32_t id) { errorID=id; }
	static void errorGenerator(ASError *obj, asAtom *args, const unsigned int argslen);
public:
	ASError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0, const tiny_string& error_name="Error",CLASS_SUBTYPE subtype=SUBTYPE_ERROR);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_getStackTrace);
	ASFUNCTION_ATOM(_toString);
	tiny_string toString();
	static void sinit(Class_base* c);
	tiny_string getStackTraceString();
};

class SecurityError: public ASError
{
public:
	SecurityError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "SecurityError",SUBTYPE_SECURITYERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
};

class ArgumentError: public ASError
{
public:
	ArgumentError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "ArgumentError",SUBTYPE_ARGUMENTERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
};

class DefinitionError: public ASError
{
public:
	DefinitionError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "DefinitionError",SUBTYPE_DEFINITIONERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
};

class EvalError: public ASError
{
public:
	EvalError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "EvalError",SUBTYPE_EVALERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
};

class RangeError: public ASError
{
public:
	RangeError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "RangeError",SUBTYPE_RANGEERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
};

class ReferenceError: public ASError
{
public:
	ReferenceError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "ReferenceError",SUBTYPE_REFERENCEERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
};

class SyntaxError: public ASError
{
public:
	SyntaxError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "SyntaxError",SUBTYPE_SYNTAXERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
};

class TypeError: public ASError
{
public:
	TypeError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "TypeError",SUBTYPE_TYPEERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
};

class URIError: public ASError
{
public:
	URIError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "URIError",SUBTYPE_URIERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);

};

class VerifyError: public ASError
{
public:
	VerifyError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "VerifyError",SUBTYPE_VERIFYERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
};

/* Not found in the spec but used by testcases */
class UninitializedError: public ASError
{
public:
	UninitializedError(ASWorker* wrk,Class_base* c, const tiny_string& error_message = "", int id = 0) : ASError(wrk,c, error_message, id, "UninitializedError",SUBTYPE_UNINITIALIZEDERROR){}
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	static void sinit(Class_base* c);
};

}

#endif /* SCRIPTING_TOPLEVEL_ERROR_H */
