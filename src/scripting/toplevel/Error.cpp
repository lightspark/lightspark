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
		for (auto it = getVm(sys)->stacktrace.crbegin(); it != getVm(sys)->stacktrace.crend(); it++)
		{
			stacktrace += "    at ";
			stacktrace += (*it).second->getClassName();
			stacktrace += "/";
			stacktrace += sys->getStringFromUniqueId((*it).first);
			stacktrace += "()\n";
		}
		LOG(LOG_INFO,"throwing exception:"<<errorID<<" "<<msg.str()<< "\n" << stacktrace);
	}
	return msg.str();
}

ASError::ASError(Class_base* c, const tiny_string& error_message, int id, const tiny_string& error_name):
	ASObject(c),errorID(id),name(error_name),message(error_message)
{
	stacktrace = "";
	for (auto it = getVm(c->getSystemState())->stacktrace.crbegin(); it != getVm(c->getSystemState())->stacktrace.crend(); it++)
	{
		stacktrace += "    at ";
		stacktrace += (*it).second->getClassName();
		stacktrace += "/";
		stacktrace += c->getSystemState()->getStringFromUniqueId((*it).first);
		stacktrace += "()\n";
	}
}

ASFUNCTIONBODY(ASError,_getStackTrace)
{
	ASError* th=static_cast<ASError*>(obj);

	ASString* ret=abstract_s(obj->getSystemState(),th->getStackTraceString());
	return ret;
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

ASFUNCTIONBODY(ASError,_toString)
{
	ASError* th=static_cast<ASError*>(obj);
	return abstract_s(obj->getSystemState(),th->ASError::toString());
}

ASFUNCTIONBODY(ASError,_constructor)
{
	ASError* th=static_cast<ASError*>(obj);
	assert_and_throw(argslen <= 2);
	if(argslen >= 1)
	{
		th->message = args[0]->toString();
	}
	if(argslen == 2)
	{
		th->errorID = static_cast<Integer*>(args[1])->toInt();
	}
	return NULL;
}

ASFUNCTIONBODY(ASError,generator)
{
	ASError* th=Class<ASError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void ASError::errorGenerator(ASError* obj, ASObject* const* args, const unsigned int argslen)
{
	assert_and_throw(argslen <= 2);
	if(argslen >= 1)
	{
		obj->message = args[0]->toString();
	}
	if(argslen == 2)
	{
		obj->errorID = static_cast<Integer*>(args[1])->toInt();
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

ASFUNCTIONBODY(SecurityError,_constructor)
{
	assert(argslen<=1);
	SecurityError* th=static_cast<SecurityError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(SecurityError,generator)
{
	ASError* th=Class<SecurityError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void SecurityError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void SecurityError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(ArgumentError,_constructor)
{
	assert(argslen<=1);
	ArgumentError* th=static_cast<ArgumentError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(ArgumentError,generator)
{
	ASError* th=Class<ArgumentError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void ArgumentError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void ArgumentError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(DefinitionError,_constructor)
{
	assert(argslen<=1);
	DefinitionError* th=static_cast<DefinitionError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(DefinitionError,generator)
{
	ASError* th=Class<DefinitionError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void DefinitionError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void DefinitionError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(EvalError,_constructor)
{
	assert(argslen<=1);
	EvalError* th=static_cast<EvalError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(EvalError,generator)
{
	ASError* th=Class<EvalError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void EvalError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void EvalError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(RangeError,_constructor)
{
	assert(argslen<=1);
	RangeError* th=static_cast<RangeError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(RangeError,generator)
{
	ASError* th=Class<RangeError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void RangeError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void RangeError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(ReferenceError,_constructor)
{
	assert(argslen<=1);
	ReferenceError* th=static_cast<ReferenceError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(ReferenceError,generator)
{
	ASError* th=Class<ReferenceError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void ReferenceError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void ReferenceError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(SyntaxError,_constructor)
{
	assert(argslen<=1);
	SyntaxError* th=static_cast<SyntaxError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(SyntaxError,generator)
{
	ASError* th=Class<SyntaxError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void SyntaxError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void SyntaxError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(TypeError,_constructor)
{
	assert(argslen<=1);
	TypeError* th=static_cast<TypeError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(TypeError,generator)
{
	ASError* th=Class<TypeError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void TypeError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void TypeError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(URIError,_constructor)
{
	assert(argslen<=1);
	URIError* th=static_cast<URIError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(URIError,generator)
{
	ASError* th=Class<URIError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void URIError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void URIError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(VerifyError,_constructor)
{
	assert(argslen<=1);
	VerifyError* th=static_cast<VerifyError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(VerifyError,generator)
{
	ASError* th=Class<VerifyError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void VerifyError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void VerifyError::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(UninitializedError,_constructor)
{
	assert(argslen<=1);
	UninitializedError* th=static_cast<UninitializedError*>(obj);
	if(argslen == 1)
	{
		th->message = args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(UninitializedError,generator)
{
	ASError* th=Class<UninitializedError>::getInstanceS(getSys());
	errorGenerator(th, args, argslen);
	return th;
}

void UninitializedError::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASError, _constructor, CLASS_DYNAMIC_NOT_FINAL);
}

void UninitializedError::buildTraits(ASObject* o)
{
}
