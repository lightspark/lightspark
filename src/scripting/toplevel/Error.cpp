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

#include <sstream>
#include "scripting/toplevel/Error.h"
#include "asobject.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/abc.h"

using namespace std;
using namespace lightspark;

tiny_string lightspark::createErrorMessage(int errorID, const tiny_string& arg1, const tiny_string& arg2, const tiny_string& arg3)
{
	auto found = errorMessages.find(errorID);
	if (found == errorMessages.end())
		return "";

	//Replace "%1", "%2" and "%3" with the optional arguments
	stringstream msg;
	msg << "Error #"<<errorID;
	msg << ": ";
	const char *msgtemplate = found->second;
	assert(msgtemplate);
	while (*msgtemplate)
	{
		if (*msgtemplate == '%')
		{
			msgtemplate++;
			switch (*msgtemplate)
			{
				case '1':
					msg << arg1;
					break;
				case '2':
					msg << arg2;
					break;
				case '3':
					msg << arg3;
					break;
				default:
					msg << '%';
					msg << *msgtemplate;
					break;
			}

			if (*msgtemplate == '\0')
				break;
		}
		else
		{
			msg << *msgtemplate;
		}

		msgtemplate++;
	}
//	if (Log::getLevel() >= LOG_INFO)
//	{
//		SystemState* sys = getSys();
//		tiny_string stacktrace;
//		ASWorker* w = getWorker();
//		int cur_rec = w ? w->cur_recursion : getVm(sys)->cur_recursion;
//		stacktrace_entry* strace = w ? w->stacktrace : getVm(sys)->stacktrace;
//		for (uint32_t i = cur_rec; i > 0; i--)
//		{
//			stacktrace += "    at ";
//			stacktrace += asAtomHandler::toObject(strace[i-1].object,sys)->getClassName();
//			stacktrace += "/";
//			stacktrace += sys->getStringFromUniqueId(strace[i-1].name);
//			stacktrace += "()\n";
//		}
//		LOG(LOG_INFO,"throwing exception:"<<w<<" id:"<<errorID<<" "<<msg.str()<< "\n" << stacktrace);
//	}
//	Log::setLogLevel(LOG_CALLS);
	LOG(LOG_INFO,"throwing exception:"<<getWorker()<<" id:"<<errorID<<" "<<msg.str());
	return msg.str();
}
void lightspark::setError(ASObject* error)
{
	if (error->getInstanceWorker()->currentCallContext)
	{
		if (error->getInstanceWorker()->currentCallContext->exceptionthrown)
			error->decRef();
		else
			error->getInstanceWorker()->currentCallContext->exceptionthrown = error;
	}
	else
		throw error;
}

ASError::ASError(ASWorker* wrk,Class_base* c, const tiny_string& error_message, int id, const tiny_string& error_name):
	ASObject(wrk,c),errorID(id),name(error_name),message(error_message)
{
	stacktrace = "";
	for (uint32_t i = wrk->cur_recursion; i > 0; i--)
	{
		stacktrace += "    at ";
		stacktrace += asAtomHandler::toObject(wrk->stacktrace[i-1].object,wrk)->getClassName();
		stacktrace += "/";
		stacktrace += c->getSystemState()->getStringFromUniqueId(wrk->stacktrace[i-1].name);
		stacktrace += "()\n";
	}
}

ASFUNCTIONBODY_ATOM(ASError,_getStackTrace)
{
	ASError* th=asAtomHandler::as<ASError>(obj);

	ret = asAtomHandler::fromObject(abstract_s(wrk,th->getStackTraceString()));
}
tiny_string ASError::getStackTraceString()
{
	tiny_string ret = toString();
	ret += "\n";
	ret += stacktrace;
	return ret;
}

tiny_string ASError::toString()
{
	tiny_string ret;
	ret = name;
	if(errorID != 0)
		ret += tiny_string(": Error #") + Integer::toString(errorID);
	if (!message.empty())
		ret += tiny_string(": ") + message;
	return ret;
}

ASFUNCTIONBODY_ATOM(ASError,_toString)
{
	ASError* th=asAtomHandler::as<ASError>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->ASError::toString()));
}

ASFUNCTIONBODY_ATOM(ASError,_constructor)
{
	ASError* th=asAtomHandler::as<ASError>(obj);
	assert_and_throw(argslen <= 2);
	if(argslen >= 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
	if(argslen == 2)
	{
		th->errorID = asAtomHandler::toInt(args[1]);
	}
}

ASFUNCTIONBODY_ATOM(ASError,generator)
{
	ASError* th=Class<ASError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void ASError::errorGenerator(ASError* obj, asAtom* args, const unsigned int argslen)
{
	assert_and_throw(argslen <= 2);
	if(argslen >= 1)
	{
		obj->message = asAtomHandler::toString(args[0],obj->getInstanceWorker());
	}
	if(argslen == 2)
	{
		obj->errorID = asAtomHandler::toInt(args[1]);
	}
}

void ASError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("getStackTrace","",Class<IFunction>::getFunction(c->getSystemState(),_getStackTrace),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	REGISTER_GETTER(c, errorID);
	REGISTER_GETTER_SETTER(c, message);
	REGISTER_GETTER_SETTER(c, name);
}

ASFUNCTIONBODY_GETTER(ASError, errorID)
ASFUNCTIONBODY_GETTER_SETTER(ASError, message)
ASFUNCTIONBODY_GETTER_SETTER(ASError, name)

void ASError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(SecurityError,_constructor)
{
	assert(argslen<=1);
	SecurityError* th=asAtomHandler::as<SecurityError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(SecurityError,generator)
{
	ASError* th=Class<SecurityError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void SecurityError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void SecurityError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(ArgumentError,_constructor)
{
	assert(argslen<=1);
	ArgumentError* th=asAtomHandler::as<ArgumentError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(ArgumentError,generator)
{
	ASError* th=Class<ArgumentError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void ArgumentError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void ArgumentError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(DefinitionError,_constructor)
{
	assert(argslen<=1);
	DefinitionError* th=asAtomHandler::as<DefinitionError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(DefinitionError,generator)
{
	ASError* th=Class<DefinitionError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void DefinitionError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void DefinitionError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(EvalError,_constructor)
{
	assert(argslen<=1);
	EvalError* th=asAtomHandler::as<EvalError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(EvalError,generator)
{
	ASError* th=Class<EvalError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void EvalError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void EvalError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(RangeError,_constructor)
{
	assert(argslen<=1);
	RangeError* th=asAtomHandler::as<RangeError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(RangeError,generator)
{
	ASError* th=Class<RangeError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void RangeError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void RangeError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(ReferenceError,_constructor)
{
	assert(argslen<=1);
	ReferenceError* th=asAtomHandler::as<ReferenceError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(ReferenceError,generator)
{
	ASError* th=Class<ReferenceError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void ReferenceError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void ReferenceError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(SyntaxError,_constructor)
{
	assert(argslen<=1);
	SyntaxError* th=asAtomHandler::as<SyntaxError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(SyntaxError,generator)
{
	ASError* th=Class<SyntaxError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void SyntaxError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void SyntaxError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(TypeError,_constructor)
{
	assert(argslen<=1);
	TypeError* th=asAtomHandler::as<TypeError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(TypeError,generator)
{
	ASError* th=Class<TypeError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void TypeError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void TypeError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(URIError,_constructor)
{
	assert(argslen<=1);
	URIError* th=asAtomHandler::as<URIError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(URIError,generator)
{
	ASError* th=Class<URIError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void URIError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void URIError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(VerifyError,_constructor)
{
	assert(argslen<=1);
	VerifyError* th=asAtomHandler::as<VerifyError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(VerifyError,generator)
{
	ASError* th=Class<VerifyError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void VerifyError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void VerifyError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(UninitializedError,_constructor)
{
	assert(argslen<=1);
	UninitializedError* th=asAtomHandler::as<UninitializedError>(obj);
	if(argslen == 1)
	{
		th->message = asAtomHandler::toString(args[0],wrk);
	}
}

ASFUNCTIONBODY_ATOM(UninitializedError,generator)
{
	ASError* th=Class<UninitializedError>::getInstanceS(wrk);
	errorGenerator(th, args, argslen);
	ret = asAtomHandler::fromObject(th);
}

void UninitializedError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void UninitializedError::buildTraits(ASObject* o)
{
}
