/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2011  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010-2011  Timon Van Overveldt (timonvo@gmail.com)

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

#include "extscriptobject.h"

using namespace lightspark;
using namespace std;

/* -- ExtIdentifier -- */
// Constructors
ExtIdentifier::ExtIdentifier() :
	type(EI_STRING), strValue(""), intValue(0)
{
}
ExtIdentifier::ExtIdentifier(const std::string& value) :
	type(EI_STRING), strValue(value), intValue(0)
{
	stringToInt();
}
ExtIdentifier::ExtIdentifier(const char* value) :
	type(EI_STRING), strValue(value), intValue(0)
{
	stringToInt();
}

ExtIdentifier::ExtIdentifier(int32_t value) :
	type(EI_INT32), strValue(""), intValue(value)
{
}
ExtIdentifier::ExtIdentifier(const ExtIdentifier& other)
{
	type = other.getType();
	strValue = other.getString();
	intValue = other.getInt();
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
	type = other.getType();
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
ExtVariant* ExtObject::getProperty(const ExtIdentifier& id) const
{
	std::map<ExtIdentifier, ExtVariant>::const_iterator it = properties.find(id);
	if(it == properties.end())
		return NULL;

	return new ExtVariant(it->second);
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
	type(EV_VOID), strValue(""), intValue(0), doubleValue(0), booleanValue(false)
{
}
ExtVariant::ExtVariant(const std::string& value) :
	type(EV_STRING), strValue(value), intValue(0), doubleValue(0), booleanValue(false)
{
}
ExtVariant::ExtVariant(const char* value) :
	type(EV_STRING), strValue(value), intValue(0), doubleValue(0), booleanValue(false)
{
}
ExtVariant::ExtVariant(int32_t value) :
	type(EV_INT32), strValue(""), intValue(value), doubleValue(0), booleanValue(false)
{
}
ExtVariant::ExtVariant(double value) :
	type(EV_DOUBLE), strValue(""), intValue(0), doubleValue(value), booleanValue(false)
{
}
ExtVariant::ExtVariant(bool value) :
	type(EV_BOOLEAN), strValue(""), intValue(0), doubleValue(0), booleanValue(value)
{
}
ExtVariant::ExtVariant(const ExtVariant& other)
{
	type = other.getType();
	strValue = other.getString();
	intValue = other.getInt();
	doubleValue = other.getDouble();
	booleanValue = other.getBoolean();
	objectValue = *other.getObject();
}
ExtVariant::ExtVariant(ASObject* other) :
	strValue(""), intValue(0), doubleValue(0), booleanValue(false)
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
		booleanValue = Boolean_concrete(other);
		type = EV_BOOLEAN;
		break;
	case T_ARRAY:
		objectValue.setType(ExtObject::EO_ARRAY);
	case T_OBJECT:
		type = EV_OBJECT;
		{
			unsigned int index = 0;
			while((index=other->nextNameIndex(index))!=0)
			{
				_R<ASObject> nextName=other->nextName(index);
				_R<ASObject> nextValue=other->nextValue(index);

				nextValue->incRef();
				if(nextName->getObjectType() == T_INTEGER)
					objectValue.setProperty(nextName->toInt(), nextValue.getPtr());
				else
					objectValue.setProperty(nextName->toString().raw_buf(), nextValue.getPtr());
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
ASObject* ExtVariant::getASObject() const
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

			ExtVariant* property;
			uint32_t count;

			// We are converting an array, so lets set indexes
			if(objValue->getType() == ExtObject::EO_ARRAY)
			{
				asobj = Class<Array>::getInstanceS();
				count = objValue->getLength();
				static_cast<Array*>(asobj)->resize(count);
				for(uint32_t i = 0; i < count; i++)
				{
					property = objValue->getProperty(i);
					static_cast<Array*>(asobj)->set(i, property->getASObject());
					delete property;
				}
			}
			// We are converting an object, so lets set variables
			else
			{
				asobj = Class<ASObject>::getInstanceS();
			
				ExtIdentifier** ids;
				uint32_t count;
				std::stringstream conv;
				if(objValue != NULL && objValue->enumerate(&ids, &count))
				{
					for(uint32_t i = 0; i < count; i++)
					{
						property = objValue->getProperty(*ids[i]);

						if(ids[i]->getType() == ExtIdentifier::EI_STRING)
						{
							asobj->setVariableByQName(ids[i]->getString(), "",
									property->getASObject(), DYNAMIC_TRAIT);
						}
						else
						{
							conv.str("");
							conv << ids[i]->getInt();
							asobj->setVariableByQName(conv.str().c_str(), "",
									property->getASObject(), DYNAMIC_TRAIT);
						}
						delete property;
						delete ids[i];
					}
				}
				delete ids;
			}
			if(objValue != NULL)
				delete objValue;
		}
		break;
	case EV_NULL:
		asobj = new Null;
		break;
	case EV_VOID:
	default:
		asobj = new Undefined;
		break;
	}
	return asobj;
}

/* -- ExtASCallback -- */
void ExtASCallback::call(const ExtScriptObject& so, const ExtIdentifier& id,
		const ExtVariant** args, uint32_t argc, bool synchronous)
{
	// Convert raw arguments to objects
	ASObject* objArgs[argc];
	for(uint32_t i = 0; i < argc; i++)
	{
		objArgs[i] = args[i]->getASObject();
	}

	// This event is not actually added to the VM event queue.
	// It is instead synched by the VM (or ourselves) after 
	// the callback has completed execution.
	syncEvent = new SynchronizationEvent;

	// Explanation for argument "synchronous":
	// Take a callback which (indirectly) calls another callback, while running in the VM thread.
	// The parent waits for the result of the second. (hence, the VM is suspended)
	// If the second callback would also try to execute in the VM thread, it would only
	// get executed when the parent had completed, since the event/function queue is FIFO.
	// This would result in a deadlock. Therefore, we run the function straight away.

	// The caller indicated the VM isn't currently suspended,
	// so add a FunctionEvent to the VM event queue.
	if(!synchronous)
	{
		func->incRef();
		syncEvent->incRef();
		funcEvent = new FunctionEvent(_MR(func), _MR(new Null), objArgs, argc, &result, &exception, _MR(syncEvent));
		// Add the callback function event to the VM event queue
		funcEvent->incRef();
		bool added=getVm()->addEvent(NullRef,_MR(funcEvent));
		if(added==false)
		{
			//Could not add the event, so the VM is shutting down
			syncEvent->decRef();
			syncEvent=NULL;
		}
		// We won't use this event any more
		funcEvent->decRef();
	}
	// The caller indicated the VM is currently suspended, so call synchronously.
	else
	{
		try
		{
			result = func->call(new Null, objArgs, argc);
		}
		// Catch AS exceptions and pass them on
		catch(ASObject* _exception)
		{
			exception = _exception;
		}
		// Catch LS exceptions and report them
		catch(LightsparkException& e)
		{
			LOG(LOG_ERROR, "LightsparkException caught in external callback, cause: " << e.what());
			sys->setError(e.cause);
		}

		// Sync the syncEvent since we called the function synchronously
		syncEvent->sync();
	}

}
void ExtASCallback::wait()
{
	if(syncEvent != NULL)
		syncEvent->wait();
}
void ExtASCallback::wakeUp()
{
	if(syncEvent != NULL)
		syncEvent->sync();
}
bool ExtASCallback::getResult(const ExtScriptObject& so, ExtVariant** _result)
{
	// syncEvent should be not-NULL since call() should have set it
	if(syncEvent == NULL)
		return false;

	// We've no use for syncEvent anymore
	syncEvent->decRef();
	// Clean up pointers
	syncEvent = NULL;
	funcEvent = NULL;
	// Did the callback throw an AS exception?
	if(exception != NULL)
	{
		if(result != NULL)
			result->decRef();

		// Pass on the exception to the container through the script object
		so.setException(exception->toString().raw_buf());
		exception->decRef();
		LOG(LOG_ERROR, "ASObject exception caught in external callback");
		success = false;
	}
	// Did the callback return a non-NULL result?
	else if(result != NULL)
	{
		// Convert the result
		*_result = new ExtVariant(result);
		result->decRef();
		success = true;
	}
	// No exception but also not result, still a success
	else
		success = true;

	// Clean up pointers
	result = NULL;
	exception = NULL;
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
		exception = _exception;
	}
	// Catch LS exceptions and report them
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR, "LightsparkException caught in external callback, cause: " << e.what());
		sys->setError(e.cause);
		success = false;
	}
}
void ExtBuiltinCallback::wait()
{
}
void ExtBuiltinCallback::wakeUp()
{
}
bool ExtBuiltinCallback::getResult(const ExtScriptObject& so, ExtVariant** _result)
{
	// Set the result
	*_result = result;
	// Did the callback throw an AS exception?
	if(exception != NULL)
	{
		so.setException(exception->toString().raw_buf());
		exception->decRef();
		LOG(LOG_ERROR, "ASObject exception caught in external callback");
		return false;
	}
	return success;
}
