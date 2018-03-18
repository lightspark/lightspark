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
	if (Log::getLevel() >= LOG_INFO)
	{
		SystemState* sys = getSys();
		tiny_string stacktrace;
		for (auto it = getVm(sys)->stacktrace.rbegin(); it != getVm(sys)->stacktrace.rend(); it++)
		{
			stacktrace += "    at ";
			stacktrace += (*it).second.toObject(sys)->getClassName();
			stacktrace += "/";
			stacktrace += sys->getStringFromUniqueId((*it).first);
			stacktrace += "()\n";
		}
		LOG(LOG_INFO,"throwing exception:"<<errorID<<" "<<msg.str()<< "\n" << stacktrace);
	}
//	Log::setLogLevel(LOG_CALLS);
	return msg.str();
}

ASError::ASError(Class_base* c, const tiny_string& error_message, int id, const tiny_string& error_name):
	ASObject(c),errorID(id),name(error_name),message(error_message)
{
	stacktrace = "";
	for (auto it = getVm(c->getSystemState())->stacktrace.rbegin(); it != getVm(c->getSystemState())->stacktrace.rend(); it++)
	{
		stacktrace += "    at ";
		stacktrace += (*it).second.toObject(c->getSystemState())->getClassName();
		stacktrace += "/";
		stacktrace += c->getSystemState()->getStringFromUniqueId((*it).first);
		stacktrace += "()\n";
	}
}

ASFUNCTIONBODY_ATOM(ASError,_getStackTrace)
{
	ASError* th=obj.as<ASError>();

	ret = asAtom::fromObject(abstract_s(sys,th->getStackTraceString()));
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
	ASError* th=obj.as<ASError>();
	ret = asAtom::fromObject(abstract_s(sys,th->ASError::toString()));
}

ASFUNCTIONBODY_ATOM(ASError,_constructor)
{
	ASError* th=obj.as<ASError>();
	assert_and_throw(argslen <= 2);
	if(argslen >= 1)
	{
		th->message = args[0].toString(sys);
	}
	if(argslen == 2)
	{
		th->errorID = args[1].toInt();
	}
}

ASFUNCTIONBODY_ATOM(ASError,generator)
{
	ASError* th=Class<ASError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
}

void ASError::errorGenerator(ASError* obj, asAtom* args, const unsigned int argslen)
{
	assert_and_throw(argslen <= 2);
	if(argslen >= 1)
	{
		obj->message = args[0].toString(obj->getSystemState());
	}
	if(argslen == 2)
	{
		obj->errorID = args[1].toInt();
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
	SecurityError* th=obj.as<SecurityError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(SecurityError,generator)
{
	ASError* th=Class<SecurityError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
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
	ArgumentError* th=obj.as<ArgumentError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(ArgumentError,generator)
{
	ASError* th=Class<ArgumentError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
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
	DefinitionError* th=obj.as<DefinitionError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(DefinitionError,generator)
{
	ASError* th=Class<DefinitionError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
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
	EvalError* th=obj.as<EvalError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(EvalError,generator)
{
	ASError* th=Class<EvalError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
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
	RangeError* th=obj.as<RangeError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(RangeError,generator)
{
	ASError* th=Class<RangeError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
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
	ReferenceError* th=obj.as<ReferenceError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(ReferenceError,generator)
{
	ASError* th=Class<ReferenceError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
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
	SyntaxError* th=obj.as<SyntaxError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(SyntaxError,generator)
{
	ASError* th=Class<SyntaxError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
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
	TypeError* th=obj.as<TypeError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(TypeError,generator)
{
	ASError* th=Class<TypeError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
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
	URIError* th=obj.as<URIError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(URIError,generator)
{
	ASError* th=Class<URIError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
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
	VerifyError* th=obj.as<VerifyError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(VerifyError,generator)
{
	ASError* th=Class<VerifyError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
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
	UninitializedError* th=obj.as<UninitializedError>();
	if(argslen == 1)
	{
		th->message = args[0].toString(sys);
	}
}

ASFUNCTIONBODY_ATOM(UninitializedError,generator)
{
	ASError* th=Class<UninitializedError>::getInstanceS(sys);
	errorGenerator(th, args, argslen);
	ret = asAtom::fromObject(th);
}

void UninitializedError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void UninitializedError::buildTraits(ASObject* o)
{
}
