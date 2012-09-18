/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010-2012  Timon Van Overveldt (timonvo@gmail.com)

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

#include <string>
#include <sstream>

#include "asobject.h"
#include "compat.h"
#include "scripting/abc.h"
#include "scripting/class.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/ASString.h"

#include "backends/extscriptobject.h"

using namespace lightspark;
using namespace std;

/* -- ExtIdentifier -- */
// Constructors
ExtIdentifier::ExtIdentifier() :
	strValue(""), intValue(0), type(EI_STRING)
{
}
ExtIdentifier::ExtIdentifier(const std::string& value) :
	strValue(value), intValue(0), type(EI_STRING)
{
	stringToInt();
}
ExtIdentifier::ExtIdentifier(const char* value) :
	strValue(value), intValue(0), type(EI_STRING)
{
	stringToInt();
}

ExtIdentifier::ExtIdentifier(int32_t value) :
	strValue(""), intValue(value), type(EI_INT32)
{
}
ExtIdentifier::ExtIdentifier(const ExtIdentifier& other)
{
	*this=other;
}

ExtIdentifier& ExtIdentifier::operator=(const ExtIdentifier& other)
{
	type = other.getType();
	strValue = other.getString();
	intValue = other.getInt();
	return *this;
}

// Convert integer string identifiers to integer identifiers
void ExtIdentifier::stringToInt()
{
	char* endptr;
	intValue = strtol(strValue.c_str(), &endptr, 10);

	if(*endptr == '\0')
		type = EI_INT32;
}

// Comparator
bool ExtIdentifier::operator<(const ExtIdentifier& other) const
{
	if(getType() == EI_STRING && other.getType() == EI_STRING)
		return getString() < other.getString();
	else if(getType() == EI_INT32 && other.getType() == EI_INT32)
		return getInt() < other.getInt();
	else if(getType() == EI_INT32 && other.getType() == EI_STRING)
		return true;
	return false;
}

/* -- ExtObject -- */
// Constructors
ExtObject::ExtObject() : type(EO_OBJECT)
{
}
ExtObject::ExtObject(const ExtObject& other)
{
	type = other.getType();
	other.copy(properties);
}

// Copying
ExtObject& ExtObject::operator=(const ExtObject& other)
{
	setType(other.getType());
	other.copy(properties);
	return *this;
} 
void ExtObject::copy(std::map<ExtIdentifier, ExtVariant>& dest) const
{
	dest = properties;
}

// Properties
bool ExtObject::hasProperty(const ExtIdentifier& id) const
{
	return properties.find(id) != properties.end();
}
const ExtVariant& ExtObject::getProperty(const ExtIdentifier& id) const
{
	std::map<ExtIdentifier, ExtVariant>::const_iterator it = properties.find(id);
	assert(it != properties.end());

	return it->second;
}
void ExtObject::setProperty(const ExtIdentifier& id, const ExtVariant& value)
{
	properties[id] = value;
}
bool ExtObject::removeProperty(const ExtIdentifier& id)
{
	std::map<ExtIdentifier, ExtVariant>::iterator it = properties.find(id);
	if(it == properties.end())
		return false;

	properties.erase(it);
	return true;
}
bool ExtObject::enumerate(ExtIdentifier*** ids, uint32_t* count) const
{
	*count = properties.size();
	*ids = new ExtIdentifier*[properties.size()];
	std::map<ExtIdentifier, ExtVariant>::const_iterator it;
	int i = 0;
	for(it = properties.begin(); it != properties.end(); ++it)
	{
		(*ids)[i] = new ExtIdentifier(it->first);
		i++;
	}

	return true;
}

/* -- ExtVariant -- */
// Constructors
ExtVariant::ExtVariant() :
	strValue(""), doubleValue(0), intValue(0), type(EV_VOID), booleanValue(false)
{
}
ExtVariant::ExtVariant(const std::string& value) :
	strValue(value), doubleValue(0), intValue(0), type(EV_STRING), booleanValue(false)
{
}
ExtVariant::ExtVariant(const char* value) :
	strValue(value), doubleValue(0), intValue(0), type(EV_STRING), booleanValue(false)
{
}
ExtVariant::ExtVariant(int32_t value) :
	strValue(""), doubleValue(0), intValue(value), type(EV_INT32), booleanValue(false)
{
}
ExtVariant::ExtVariant(double value) :
	strValue(""), doubleValue(value), intValue(0), type(EV_DOUBLE), booleanValue(false)
{
}
ExtVariant::ExtVariant(bool value) :
	strValue(""), doubleValue(0), intValue(0), type(EV_BOOLEAN), booleanValue(value)
{
}
ExtVariant::ExtVariant(std::map<const ASObject*, std::unique_ptr<ExtObject>>& objectsMap, _R<ASObject> other) :
	strValue(""), doubleValue(0), intValue(0), booleanValue(false)
{
	switch(other->getObjectType())
	{
	case T_STRING:
		strValue = other->toString().raw_buf();
		type = EV_STRING;
		break;
	case T_INTEGER:
		intValue = other->toInt();
		type = EV_INT32;
		break;
	case T_NUMBER:
		doubleValue = other->toNumber();
		type = EV_DOUBLE;
		break;
	case T_BOOLEAN:
		booleanValue = Boolean_concrete(other.getPtr());
		type = EV_BOOLEAN;
		break;
	case T_ARRAY:
	case T_OBJECT:
		{
			type = EV_OBJECT;
			auto it=objectsMap.find(other.getPtr());
			if(it!=objectsMap.end())
			{
				objectValue = it->second.get();
				break;
			}
			objectValue = new ExtObject();
			if(other->getObjectType()==T_ARRAY)
				objectValue->setType(ExtObject::EO_ARRAY);

			objectsMap[other.getPtr()] = move(unique_ptr<ExtObject>(objectValue));

			unsigned int index = 0;
			while((index=other->nextNameIndex(index))!=0)
			{
				_R<ASObject> nextName=other->nextName(index);
				_R<ASObject> nextValue=other->nextValue(index);

				if(nextName->getObjectType() == T_INTEGER)
					objectValue->setProperty(nextName->toInt(), ExtVariant(objectsMap, nextValue));
				else
					objectValue->setProperty(nextName->toString().raw_buf(), ExtVariant(objectsMap, nextValue));
			}
		}
		break;
	case T_NULL:
		type = EV_NULL;
		break;
	case T_UNDEFINED:
	default:
		type = EV_VOID;
		break;
	}
}

// Conversion to ASObject
ASObject* ExtVariant::getASObject(std::map<const lightspark::ExtObject*, lightspark::ASObject*>& objectsMap) const
{
	ASObject* asobj;
	switch(getType())
	{
	case EV_STRING:
		asobj = Class<ASString>::getInstanceS(getString().c_str());
		break;
	case EV_INT32:
		asobj = abstract_i(getInt());
		break;
	case EV_DOUBLE:
		asobj = abstract_d(getDouble());
		break;
	case EV_BOOLEAN:
		asobj = abstract_b(getBoolean());
		break;
	case EV_OBJECT:
		{
			ExtObject* objValue = getObject();
			auto it=objectsMap.find(objValue);
			if(it!=objectsMap.end())
			{
				it->second->incRef();
				return it->second;
			}

			uint32_t count;

			// We are converting an array, so lets set indexes
			if(objValue->getType() == ExtObject::EO_ARRAY)
			{
				asobj = Class<Array>::getInstanceS();
				objectsMap[objValue] = asobj;

				count = objValue->getLength();
				static_cast<Array*>(asobj)->resize(count);
				for(uint32_t i = 0; i < count; i++)
				{
					const ExtVariant& property = objValue->getProperty(i);
					static_cast<Array*>(asobj)->set(i, _MR(property.getASObject(objectsMap)));
				}
			}
			// We are converting an object, so lets set variables
			else
			{
				asobj = Class<ASObject>::getInstanceS();
				objectsMap[objValue] = asobj;
			
				ExtIdentifier** ids;
				uint32_t count;
				std::stringstream conv;
				if(objValue->enumerate(&ids, &count))
				{
					for(uint32_t i = 0; i < count; i++)
					{
						const ExtVariant& property = objValue->getProperty(*ids[i]);

						if(ids[i]->getType() == ExtIdentifier::EI_STRING)
						{
							asobj->setVariableByQName(ids[i]->getString(), "",
									property.getASObject(objectsMap), DYNAMIC_TRAIT);
						}
						else
						{
							conv.str("");
							conv << ids[i]->getInt();
							if(asobj->hasPropertyByMultiname(QName(conv.str(),""),true,true))
							{
								LOG(LOG_NOT_IMPLEMENTED,"ExtVariant::getASObject: duplicate property " << conv.str());
								continue;
							}
							asobj->setVariableByQName(conv.str().c_str(), "",
									property.getASObject(objectsMap), DYNAMIC_TRAIT);
						}
						delete ids[i];
					}
				}
				delete[] ids;
			}
		}
		break;
	case EV_NULL:
		asobj = getSys()->getNullRef();
		break;
	case EV_VOID:
	default:
		asobj = getSys()->getUndefinedRef();
		break;
	}
	return asobj;
}

/* -- ExtASCallback -- */
ExtASCallback::~ExtASCallback() {
	func->decRef();
	if(asArgs)
		delete[] asArgs;
}

void ExtASCallback::call(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, bool synchronous)
{
	assert(funcEvent == NullRef);
	// Explanation for argument "synchronous":
	// Take a callback which (indirectly) calls another callback, while running in the VM thread.
	// The parent waits for the result of the second. (hence, the VM is suspended)
	// If the second callback would also try to execute in the VM thread, it would only
	// get executed when the parent had completed, since the event/function queue is FIFO.
	// This would result in a deadlock. Therefore, we run the function straight away.

	// The caller indicated the VM isn't currently suspended,
	// so add a FunctionEvent to the VM event queue.

	// Convert ExtVariant arguments to ASObjects
	assert(!asArgs);
	asArgs = new ASObject*[argc];
	std::map<const ExtObject*, ASObject*> objectsMap;
	for(uint32_t i = 0; i < argc; i++)
		asArgs[i] = args[i]->getASObject(objectsMap);

	if(!synchronous)
	{
		func->incRef();
		funcEvent = _MR(new (getSys()->unaccountedMemory) ExternalCallEvent(_MR(func), asArgs, argc, &result, &exceptionThrown, &exception));
		// Add the callback function event to the VM event queue
		funcWasCalled=getVm()->addEvent(NullRef,funcEvent);
		if(!funcWasCalled)
			funcEvent = NullRef;
	}
	// The caller indicated the VM is currently suspended, so call synchronously.
	else
	{
		try
		{
			/* TODO: shouldn't we pass some global object instead of Null? */
			result = func->call(getSys()->getNullRef(), asArgs, argc);
		}
		// Catch AS exceptions and pass them on
		catch(ASObject* _exception)
		{
			exception = _exception->toString();
		}
		// Catch LS exceptions and report them
		catch(LightsparkException& e)
		{
			LOG(LOG_ERROR, "LightsparkException caught in external callback, cause: " << e.what());
			getSys()->setError(e.cause);
		}
		funcWasCalled = true;
		delete[] asArgs;
		asArgs = NULL;
	}
}
void ExtASCallback::wait()
{
	if(!funcEvent.isNull())
		funcEvent->done.wait();
}
void ExtASCallback::wakeUp()
{
	if(!funcEvent.isNull())
		funcEvent->done.signal();
}
bool ExtASCallback::getResult(std::map<const ASObject*, std::unique_ptr<ExtObject>>& objectsMap,
		const ExtScriptObject& so, const ExtVariant** _result)
{
	funcEvent = NullRef;
	// Did the callback throw an AS exception?
	if(exceptionThrown)
	{
		if(result != NULL)
		{
			result->decRef();
			result = NULL;
		}

		// Pass on the exception to the container through the script object
		so.setException(exception.raw_buf());
		LOG(LOG_ERROR, "ASObject exception caught in external callback");
		success = false;
	}
	// There was an error executing the function
	else if(!funcWasCalled)
	{
		success = false;
	}
	// Did the callback return a non-NULL result?
	else if(result != NULL)
	{
		// Convert the result
		*_result = new ExtVariant(objectsMap, _MR(result));
		success = true;
	}
	// No exception but also no result, still a success
	else
		success = true;

	// Clean up pointers
	result = NULL;
	exceptionThrown = false;
	exception = "";
	if (asArgs)
	{
		// The references have been consumed by the called
		// function. We only delete the array.
		delete[] asArgs;
		asArgs = NULL;
	}

	return success;
}
/* -- ExtBuiltinCallback -- */
void ExtBuiltinCallback::call(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, bool synchronous)
{
	try
	{
		// We expect the builtin callback to handle main <--> VM thread synchronization by itself
		success = func(so, id, args, argc, &result);
	}
	// Catch AS exceptions and pass them on
	catch(ASObject* _exception)
	{
		exceptionThrown = true;
		exception = _exception->toString();
		_exception->decRef();
	}
	// Catch LS exceptions and report them
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR, "LightsparkException caught in external callback, cause: " << e.what());
		getSys()->setError(e.cause);
		success = false;
	}
}
void ExtBuiltinCallback::wait()
{
}
void ExtBuiltinCallback::wakeUp()
{
}
bool ExtBuiltinCallback::getResult(std::map<const ASObject*, std::unique_ptr<ExtObject>>& objectsMap,
		const ExtScriptObject& so, const ExtVariant** _result)
{
	// Set the result
	*_result = result;
	// Did the callback throw an AS exception?
	if(exceptionThrown)
	{
		so.setException(exception.raw_buf());
		LOG(LOG_ERROR, "ASObject exception caught in external callback");
		return false;
	}
	return success;
}
