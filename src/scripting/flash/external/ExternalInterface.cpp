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

#include "ExternalInterface.h"
#include "class.h"
#include "backends/extscriptobject.h"

using namespace lightspark;

SET_NAMESPACE("flash.external");

REGISTER_CLASS_NAME(ExternalInterface);

void ExternalInterface::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setDeclaredMethodByQName("available","",Class<IFunction>::getFunction(_getAvailable),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("objectID","",Class<IFunction>::getFunction(_getObjectID),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("marshallExceptions","",Class<IFunction>::getFunction(_getMarshallExceptions),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("marshallExceptions","",Class<IFunction>::getFunction(_setMarshallExceptions),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("addCallback","",Class<IFunction>::getFunction(addCallback),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("call","",Class<IFunction>::getFunction(call),NORMAL_METHOD,false);
}

ASFUNCTIONBODY(ExternalInterface,_getAvailable)
{
	return abstract_b(sys->extScriptObject != NULL);
}

ASFUNCTIONBODY(ExternalInterface,_getObjectID)
{
	if(sys->extScriptObject == NULL)
		return Class<ASString>::getInstanceS("");

	ExtVariant* object = sys->extScriptObject->getProperty("name");
	if(object == NULL)
		return Class<ASString>::getInstanceS("");

	std::string result = object->getString();
	delete object;
	return Class<ASString>::getInstanceS(result);
}

ASFUNCTIONBODY(ExternalInterface, _getMarshallExceptions)
{
	if(sys->extScriptObject == NULL)
		return abstract_b(false);
	else
		return abstract_b(sys->extScriptObject->getMarshallExceptions());
}

ASFUNCTIONBODY(ExternalInterface, _setMarshallExceptions)
{
	if(sys->extScriptObject != NULL)
		sys->extScriptObject->setMarshallExceptions(Boolean_concrete(args[0]));
	return NULL;
}


ASFUNCTIONBODY(ExternalInterface,addCallback)
{
	if(sys->extScriptObject == NULL)
		throw Class<ASError>::getInstanceS("Container doesn't support callbacks");

	assert_and_throw(argslen == 2);

	if(args[1]->getObjectType() == T_NULL)
		sys->extScriptObject->removeMethod(args[0]->toString().raw_buf());
	else
	{
		IFunction* f=static_cast<IFunction*>(args[1]);
		sys->extScriptObject->setMethod(args[0]->toString().raw_buf(), new ExtASCallback(f));
	}
	return abstract_b(true);
}

ASFUNCTIONBODY(ExternalInterface,call)
{
	if(sys->extScriptObject == NULL)
		throw Class<ASError>::getInstanceS("Container doesn't support callbacks");

	assert_and_throw(argslen >= 1 && args[0]->getObjectType() == T_STRING);

	// TODO: Check security constraints & throw SecurityException

	// Convert given arguments to ExtVariants
	const ExtVariant** callArgs = g_newa(const ExtVariant*,argslen-1);
	for(uint32_t i = 0; i < argslen-1; i++)
		callArgs[i] = new ExtVariant(args[i+1]);

	ASObject* asobjResult = NULL;
	// Let the external script object call the external method
	bool callSuccess = sys->extScriptObject->callExternal(args[0]->toString().raw_buf(), callArgs, argslen-1, &asobjResult);

	// Delete converted arguments
	for(uint32_t i = 0; i < argslen-1; i++)
		delete callArgs[i];

	if(!callSuccess)
	{
		assert(asobjResult==NULL);
		LOG(LOG_INFO, "External function failed, returning null: " << args[0]->toString().raw_buf());
		// If the call fails, return null
		asobjResult = new Null;
	}

	return asobjResult;
}
