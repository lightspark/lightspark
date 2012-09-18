/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/flash/external/ExternalInterface.h"
#include "scripting/class.h"
#include "backends/extscriptobject.h"
#include "scripting/toplevel/ASString.h"

using namespace lightspark;

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
	return abstract_b(getSys()->extScriptObject != NULL);
}

ASFUNCTIONBODY(ExternalInterface,_getObjectID)
{
	if(getSys()->extScriptObject == NULL)
		return Class<ASString>::getInstanceS("");

	ExtScriptObject* so=getSys()->extScriptObject;
	if(so->hasProperty("name")==false)
		return Class<ASString>::getInstanceS("");

	const ExtVariant& object = so->getProperty("name");
	std::string result = object.getString();
	return Class<ASString>::getInstanceS(result);
}

ASFUNCTIONBODY(ExternalInterface, _getMarshallExceptions)
{
	if(getSys()->extScriptObject == NULL)
		return abstract_b(false);
	else
		return abstract_b(getSys()->extScriptObject->getMarshallExceptions());
}

ASFUNCTIONBODY(ExternalInterface, _setMarshallExceptions)
{
	if(getSys()->extScriptObject != NULL)
		getSys()->extScriptObject->setMarshallExceptions(Boolean_concrete(args[0]));
	return NULL;
}


ASFUNCTIONBODY(ExternalInterface,addCallback)
{
	if(getSys()->extScriptObject == NULL)
		throw Class<ASError>::getInstanceS("Container doesn't support callbacks");

	assert_and_throw(argslen == 2);

	if(args[1]->getObjectType() == T_NULL)
		getSys()->extScriptObject->removeMethod(args[0]->toString().raw_buf());
	else
	{
		IFunction* f=static_cast<IFunction*>(args[1]);
		getSys()->extScriptObject->setMethod(args[0]->toString().raw_buf(), new ExtASCallback(f));
	}
	return abstract_b(true);
}

ASFUNCTIONBODY(ExternalInterface,call)
{
	if(getSys()->extScriptObject == NULL)
		throw Class<ASError>::getInstanceS("Container doesn't support callbacks");

	assert_and_throw(argslen >= 1);
	const tiny_string& arg0=args[0]->toString();

	// TODO: Check security constraints & throw SecurityException

	// Convert given arguments to ExtVariants
	const ExtVariant** callArgs = g_newa(const ExtVariant*,argslen-1);
	std::map<const ASObject*, std::unique_ptr<ExtObject>> objectsMap;
	for(uint32_t i = 0; i < argslen-1; i++)
	{
		args[i+1]->incRef();
		callArgs[i] = new ExtVariant(objectsMap,_MR(args[i+1]));
	}

	ASObject* asobjResult = NULL;
	// Let the external script object call the external method
	bool callSuccess = getSys()->extScriptObject->callExternal(arg0.raw_buf(), callArgs, argslen-1, &asobjResult);

	// Delete converted arguments
	for(uint32_t i = 0; i < argslen-1; i++)
		delete callArgs[i];

	if(!callSuccess)
	{
		assert(asobjResult==NULL);
		LOG(LOG_INFO, "External function failed, returning null: " << arg0);
		// If the call fails, return null
		asobjResult = getSys()->getNullRef();
	}

	return asobjResult;
}
