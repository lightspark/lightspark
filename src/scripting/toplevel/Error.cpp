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

#include "Error.h"
#include "asobject.h"
#include "toplevel.h"
#include "class.h"
#include "argconv.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("");

REGISTER_CLASS_NAME2(ASError, "Error", "");
REGISTER_CLASS_NAME(SecurityError);
REGISTER_CLASS_NAME(ArgumentError);
REGISTER_CLASS_NAME(DefinitionError);
REGISTER_CLASS_NAME(EvalError);
REGISTER_CLASS_NAME(RangeError);
REGISTER_CLASS_NAME(ReferenceError);
REGISTER_CLASS_NAME(SyntaxError);
REGISTER_CLASS_NAME(TypeError);
REGISTER_CLASS_NAME(URIError);
REGISTER_CLASS_NAME(VerifyError);

ASFUNCTIONBODY(ASError,getStackTrace)
{
	ASError* th=static_cast<ASError*>(obj);
	ASString* ret=Class<ASString>::getInstanceS(th->toString(true));
	LOG(LOG_NOT_IMPLEMENTED,_("Error.getStackTrace not yet implemented."));
	return ret;
}

tiny_string ASError::toString(bool debugMsg)
{
	if( !message.empty() )
		return name + ": " + message;
	else
		return name;
}

ASFUNCTIONBODY(ASError,_toString)
{
	ASError* th=static_cast<ASError*>(obj);
	return Class<ASString>::getInstanceS(th->ASError::toString(false));
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
		th->errorID = static_cast<Integer*>(args[0])->toInt();
	}
	return NULL;
}

void ASError::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("getStackTrace",AS3,Class<IFunction>::getFunction(getStackTrace),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
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

void SecurityError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
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

void ArgumentError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
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

void DefinitionError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
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

void EvalError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
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

void RangeError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
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

void ReferenceError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
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

void SyntaxError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
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

void TypeError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
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

void URIError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
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

void VerifyError::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASError>::getRef());
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
