/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)
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

#include "logger.h"
#include "plugin/plugin.h"

#include "plugin/npscriptobject.h"
#include "scripting/flash/system/flashsystem.h"

using namespace std;
using namespace lightspark;

/* -- NPIdentifierObject -- */
// Constructors
NPIdentifierObject::NPIdentifierObject(const std::string& value)
{
	stringToInt(value);
}
NPIdentifierObject::NPIdentifierObject(const char* value)
{
	stringToInt(std::string(value));
}
NPIdentifierObject::NPIdentifierObject(int32_t value)
{
	identifier = NPN_GetIntIdentifier(value);
}
NPIdentifierObject::NPIdentifierObject(const ExtIdentifier& value)
{
	// It is possible we got a down-casted ExtIdentifier, so lets check for that
	const NPIdentifierObject* npio = dynamic_cast<const NPIdentifierObject*>(&value);
	if(npio)
	{
		npio->copy(identifier);
	}
	else
	{
		if(value.getType() == EI_STRING)
			identifier = NPN_GetStringIdentifier(value.getString().c_str());
		else
			identifier = NPN_GetIntIdentifier(value.getInt());
	}
}
NPIdentifierObject::NPIdentifierObject(const NPIdentifierObject& id)
	: ExtIdentifier()
{
	id.copy(identifier);
}
NPIdentifierObject::NPIdentifierObject(const NPIdentifier& id, bool convertToInt)
	: ExtIdentifier()
{
	if (convertToInt && NPN_IdentifierIsString(id))
	{
		NPUTF8* str = NPN_UTF8FromIdentifier(id);
		stringToInt(std::string((const char*)str));
		NPN_MemFree(str);
	}
	else
		copy(id, identifier);
}

// Convert integer string identifiers to integer identifiers
void NPIdentifierObject::stringToInt(const std::string& value)
{
	char* endptr;
	int intValue = strtol(value.c_str(), &endptr, 10);
	
	// Convert integer string identifiers to integer identifiers
	if(*endptr == '\0')
		identifier = NPN_GetIntIdentifier(intValue);
	else
		identifier = NPN_GetStringIdentifier(value.c_str());
}

// Copy a given NPIdentifier into another one
void NPIdentifierObject::copy(const NPIdentifier& from, NPIdentifier& dest)
{
	if(NPN_IdentifierIsString(from))
	{
		NPUTF8* str = NPN_UTF8FromIdentifier(from);
		dest = NPN_GetStringIdentifier(str);
		NPN_MemFree(str);
	}
	else
		dest = NPN_GetIntIdentifier(NPN_IntFromIdentifier(from));
}

// Copy this object's value to a NPIdentifier
void NPIdentifierObject::copy(NPIdentifier& dest) const
{
	copy(identifier, dest);
}

// Comparator
bool NPIdentifierObject::operator<(const ExtIdentifier& other) const
{
	const NPIdentifierObject* npi = dynamic_cast<const NPIdentifierObject*>(&other);
	if(npi)
		return identifier < npi->getNPIdentifier();
	else
		return ExtIdentifier::operator<(other);
}

// Type determination
NPIdentifierObject::EI_TYPE NPIdentifierObject::getType(const NPIdentifier& identifier)
{
	if(NPN_IdentifierIsString(identifier))
		return EI_STRING;
	else
		return EI_INT32;
}

// Conversion
std::string NPIdentifierObject::getString(const NPIdentifier& identifier)
{
	if(getType(identifier) == EI_STRING)
	{
		NPUTF8* str = NPN_UTF8FromIdentifier(identifier);
		std::string result(str);
		NPN_MemFree(str);
		return result;
	}
	else
		return "";
}
int32_t NPIdentifierObject::getInt(const NPIdentifier& identifier)
{
	if(getType(identifier) == EI_INT32)
		return NPN_IntFromIdentifier(identifier);
	else
		return 0;
}
NPIdentifier NPIdentifierObject::getNPIdentifier() const
{
	if(getType() == EI_STRING)
		return NPN_GetStringIdentifier(getString().c_str());
	else
		return NPN_GetIntIdentifier(getInt());
}

bool NPIdentifierObject::isNumeric() const
{
	if(getType() == EI_STRING)
	{
		std::string s = getString();
		for(unsigned i=0; i<s.size(); ++i)
		{
			if (!isdigit(s[i]))
				return false;
		}
	}

	return true;
}

/* -- NPObjectObject -- */
// Constructors
NPObjectObject::NPObjectObject(std::map<const NPObject*, std::unique_ptr<ExtObject>>& objectsMap, NPP _instance, NPObject* obj) :
	instance(_instance)
{
	//First of all add this object to the map, so that recursive cycles may be broken
	if(objectsMap.count(obj)==0)
		objectsMap[obj] = move(unique_ptr<ExtObject>(this));

	NPIdentifier* ids = NULL;
	NPVariant property;
	uint32_t count;

	if(instance == NULL || obj == NULL)
		return;

	bool is_array = isArray(obj);

	// Get all identifiers this NPObject has
	if(NPN_Enumerate(instance, obj, &ids, &count))
	{
		for(uint32_t i = 0; i < count; i++)
		{
			if(NPN_GetProperty(instance, obj, ids[i], &property))
			{
				// For arrays, force identifiers into
				// integers because Lightspark's Array
				// class requires integer indexes.
				setProperty(NPIdentifierObject(ids[i], is_array),
						NPVariantObject(objectsMap, instance, property));
				NPN_ReleaseVariantValue(&property);
			}
		}

		NPN_MemFree(ids);
	}

	if(is_array)
		setType(EO_ARRAY);
}

bool NPObjectObject::isArray(NPObject* obj) const
{
	// This object is an array if
	// 1) it has an un-enumerated length propoerty and
	// 2) the enumerated propoerties are numeric

	if(instance == NULL || obj == NULL)
		return false;

	// Check the existence of length property
	NPVariant property;
	bool hasLength = false;
	if(NPN_GetProperty(instance, obj, NPN_GetStringIdentifier("length"), &property))
	{
		// I've seen EV_DOUBLE lengths being sent by Chromium
		NPVariantObject::EV_TYPE type = NPVariantObject::getTypeS(property);
		if(type == NPVariantObject::EV_INT32 || type == NPVariantObject::EV_DOUBLE)
			hasLength = true;
		
		NPN_ReleaseVariantValue(&property);
	}

	if(!hasLength)
		return false;

	// Check that all properties are integers or strings of digits
	NPIdentifier* ids = NULL;
	uint32_t count;
	bool allIntIds = true;
	if(NPN_Enumerate(instance, obj, &ids, &count))
	{
		for(uint32_t i = 0; i < count; i++)
		{
			if(!NPIdentifierObject(ids[i]).isNumeric())
			{
				allIntIds = false;
				break;
			}
		}

		NPN_MemFree(ids);
	}

	return allIntIds;
}

// Conversion to NPObject
NPObject* NPObjectObject::getNPObject(std::map<const ExtObject*, NPObject*>& objectsMap, NPP instance, const lightspark::ExtObject* obj)
{
	auto it=objectsMap.find(obj);
	if(it!=objectsMap.end())
	{
		NPObject* ret=it->second;
		NPN_RetainObject(ret);
		return ret;
	}
	uint32_t count = obj->getLength();

	// This will hold the enumerated properties
	// This will hold the converted properties
	NPVariant varProperty;

	NPObject* windowObject;
	NPN_GetValue(instance, NPNVWindowNPObject, &windowObject);

	// This will hold the result from NPN_Invoke
	NPVariant resultVariant;
	// This will hold the converted result from NPN_Invoke
	NPObject* result;

	// We are converting to a javascript Array
	if(obj->getType() == lightspark::ExtObject::EO_ARRAY)
	{
		// Create a new Array NPObject
		NPN_Invoke(instance, windowObject, NPN_GetStringIdentifier("Array"), NULL, 0, &resultVariant);
		// Convert our NPVariant result to an NPObject
		result = NPVARIANT_TO_OBJECT(resultVariant);
		objectsMap[obj] = result;

		// Push all values onto the array
		for(uint32_t i = 0; i < count; i++)
		{
			const lightspark::ExtVariant& property = obj->getProperty(i);

			NPVariantObject::ExtVariantToNPVariant(objectsMap, instance, property, varProperty);

			// Push the value onto the newly created Array
			NPN_Invoke(instance, result, NPN_GetStringIdentifier("push"), &varProperty, 1, &resultVariant);

			NPN_ReleaseVariantValue(&resultVariant);
			NPN_ReleaseVariantValue(&varProperty);
		}
	}
	else
	{
		// Create a new Object NPObject
		NPN_Invoke(instance, windowObject, NPN_GetStringIdentifier("Object"), NULL, 0, &resultVariant);
		// Convert our NPVariant result to an NPObject
		result = NPVARIANT_TO_OBJECT(resultVariant);
		objectsMap[obj] = result;

		lightspark::ExtIdentifier** ids = NULL;
		// Set all values of the object
		if(obj->enumerate(&ids, &count))
		{
			for(uint32_t i = 0; i < count; i++)
			{
				const lightspark::ExtVariant& property = obj->getProperty(*ids[i]);
				NPVariantObject::ExtVariantToNPVariant(objectsMap, instance, property, varProperty);

				// Set the properties
				NPN_SetProperty(instance, result, NPIdentifierObject(*ids[i]).getNPIdentifier(), &varProperty);

				NPN_ReleaseVariantValue(&varProperty);
				delete ids[i];
			}
		}
		if(ids != NULL)
			delete[] ids;
	}
	return result;
}

/* -- NPVariantObject -- */
// Constructors
NPVariantObject::NPVariantObject(std::map<const NPObject*, std::unique_ptr<ExtObject>>& objectsMap, NPP _instance, const NPVariant& other)
{
	//copy(other, variant);
	switch(other.type)
	{
	case NPVariantType_Void:
		type = EV_VOID;
		break;
	case NPVariantType_Null:
		type = EV_NULL;
		break;
	case NPVariantType_Bool:
		type = EV_BOOLEAN;
		booleanValue = NPVARIANT_TO_BOOLEAN(other);
		break;
	case NPVariantType_Int32:
		type = EV_INT32;
		intValue = NPVARIANT_TO_INT32(other);
		break;
	case NPVariantType_Double:
		type = EV_DOUBLE;
		doubleValue = NPVARIANT_TO_DOUBLE(other);
		break;
	case NPVariantType_String:
		{
			type = EV_STRING;
			const NPString& str=NPVARIANT_TO_STRING(other);
			std::string tmp(str.UTF8Characters,str.UTF8Length);
			strValue.swap(tmp);
			break;
		}
	case NPVariantType_Object:
		{
			type = EV_OBJECT;
			NPObject* const npObj = NPVARIANT_TO_OBJECT(other);
			auto it=objectsMap.find(npObj);
			if(it!=objectsMap.end())
				objectValue = it->second.get();
			else
				objectValue = new NPObjectObject(objectsMap, _instance, npObj);
			break;
		}
	}
}

void NPVariantObject::ExtVariantToNPVariant(std::map<const ExtObject*, NPObject*>& objectsMap, NPP _instance,
		const ExtVariant& value, NPVariant& variant)
{
	switch(value.getType())
	{
	case EV_STRING:
		{
			const std::string& strValue = value.getString();
			NPUTF8* newValue = static_cast<NPUTF8*>(NPN_MemAlloc(strValue.size()));
			memcpy(newValue, strValue.c_str(), strValue.size());

			STRINGN_TO_NPVARIANT(newValue, strValue.size(), variant);
			break;
		}
	case EV_INT32:
		INT32_TO_NPVARIANT(value.getInt(), variant);
		break;
	case EV_DOUBLE:
		DOUBLE_TO_NPVARIANT(value.getDouble(), variant);
		break;
	case EV_BOOLEAN:
		BOOLEAN_TO_NPVARIANT(value.getBoolean(), variant);
		break;
	case EV_OBJECT:
		{
			lightspark::ExtObject* obj = value.getObject();
			OBJECT_TO_NPVARIANT(NPObjectObject::getNPObject(objectsMap, _instance, obj), variant);
			break;
		}
	case EV_NULL:
		NULL_TO_NPVARIANT(variant);
		break;
	case EV_VOID:
	default:
		VOID_TO_NPVARIANT(variant);
		break;
	}
}

// Type determination
NPVariantObject::EV_TYPE NPVariantObject::getTypeS(const NPVariant& variant)
{
	if(NPVARIANT_IS_STRING(variant))
		return EV_STRING;
	else if(NPVARIANT_IS_INT32(variant))
		return EV_INT32;
	else if(NPVARIANT_IS_DOUBLE(variant))
		return EV_DOUBLE;
	else if(NPVARIANT_IS_BOOLEAN(variant))
		return EV_BOOLEAN;
	else if(NPVARIANT_IS_OBJECT(variant))
		return EV_OBJECT;
	else if(NPVARIANT_IS_NULL(variant))
		return EV_NULL;
	else
		return EV_VOID;
}

/* -- NPScriptObject -- */
// Constructor
NPScriptObject::NPScriptObject(NPScriptObjectGW* _gw) :
	gw(_gw), instance(gw->getInstance()),
	currentCallback(NULL), hostCallData(NULL),
	shuttingDown(false), marshallExceptions(false)
{
	// This object is always created in the main plugin thread, so lets save
	// so that we can check if we are in the main plugin thread later on.
	mainThread = Thread::self();

	setProperty("$version", Capabilities::EMULATED_VERSION);

	// Standard methods
	setMethod("SetVariable", new lightspark::ExtBuiltinCallback(stdSetVariable));
	setMethod("GetVariable", new lightspark::ExtBuiltinCallback(stdGetVariable));
	setMethod("GotoFrame", new lightspark::ExtBuiltinCallback(stdGotoFrame));
	setMethod("IsPlaying", new lightspark::ExtBuiltinCallback(stdIsPlaying));
	setMethod("LoadMovie", new lightspark::ExtBuiltinCallback(stdLoadMovie));
	setMethod("Pan", new lightspark::ExtBuiltinCallback(stdPan));
	setMethod("PercentLoaded", new lightspark::ExtBuiltinCallback(stdPercentLoaded));
	setMethod("Play", new lightspark::ExtBuiltinCallback(stdPlay));
	setMethod("Rewind", new lightspark::ExtBuiltinCallback(stdRewind));
	setMethod("SetZoomRect", new lightspark::ExtBuiltinCallback(stdSetZoomRect));
	setMethod("StopPlay", new lightspark::ExtBuiltinCallback(stdStopPlay));
	setMethod("Zoom", new lightspark::ExtBuiltinCallback(stdZoom));
	setMethod("TotalFrames", new lightspark::ExtBuiltinCallback(stdTotalFrames));
}

// Destructor preparator
void NPScriptObject::destroy()
{
	mutex.lock();
	// Prevents new external calls from continuing
	shuttingDown = true;

	// If an external call is running, wake it up
	if(callStatusses.size() > 0)
		callStatusses.top()->signal();
	mutex.unlock();
	// Wait for all external calls to finish
	Mutex::Lock l(externalCall);
}

// Destructor
NPScriptObject::~NPScriptObject()
{
	std::map<ExtIdentifier, lightspark::ExtCallback*>::iterator meth_it = methods.begin();
	while(meth_it != methods.end())
	{
		delete (*meth_it).second;
		methods.erase(meth_it++);
	}
}

// NPRuntime interface: invoking methods
bool NPScriptObject::invoke(NPIdentifier id, const NPVariant* args, uint32_t argc, NPVariant* result)
{
	// If the NPScriptObject is shutting down, don't even continue
	if(shuttingDown)
		return false;

	NPIdentifierObject objId(id);
	// Check if the method exists
	std::map<ExtIdentifier, lightspark::ExtCallback*>::iterator it;
	it = methods.find(objId);
	if(it == methods.end())
		return false;

	LOG(LOG_CALLS,"Plugin callback from the browser: " << objId.getString());

	// Convert raw arguments to objects
	const lightspark::ExtVariant** objArgs = g_newa(const lightspark::ExtVariant*,argc);
	std::map<const NPObject*, std::unique_ptr<ExtObject>> npObjectsMap;
	for(uint32_t i = 0; i < argc; i++)
		objArgs[i] = new NPVariantObject(npObjectsMap, instance, args[i]);

	// This will hold our eventual callback result
	const lightspark::ExtVariant* objResult = NULL;
	// Make sure we use a copy of the callback
	lightspark::ExtCallback* callback = it->second->copy();

	// Set the current root callback only if there isn't one already
	bool rootCallback = false;

	// We must avoid colliding with Flash code attemping an external call now
	mutex.lock();

	if(currentCallback == NULL)
	{
		// Remember to reset the current callback
		rootCallback = true;
		currentCallback = callback;
	}

	// Call the callback synchronously if:
	// - We are not the root callback
	//     (case: BROWSER -> invoke -> VM -> external call -> BROWSER -> invoke)
	// - We are the root callback AND we are being called from within an external call
	//     (case: VM -> external call -> BROWSER -> invoke)
	bool synchronous = !rootCallback || (rootCallback && callStatusses.size() == 1);

	//Now currentCallback have been set, the VM thread will notice this and invoke the code using
	//a forced wake up
	mutex.unlock();

	// Call our callback.
	// We can call it synchronously in the cases specified above.
	// In both cases, the VM is suspended, waiting for the result from another external call.
	// Thus we don't have to worry about synchronizing with the VM.
	callback->call(*this, objId, objArgs, argc, synchronous);
	// Wait for its result or a forced wake-up
	callback->wait();

	//Again, hostCallData is shared between the VM thread and the main thread and it should be protected
	mutex.lock();

	// As long as we get forced wake-ups, execute the requested external calls and keep waiting.
	// Note that only the root callback can be forcibly woken up.
	while(hostCallData != NULL)
	{
		// Copy the external call data pointer
		HOST_CALL_DATA* data = hostCallData;
		// Clear the external call data pointer BEFORE executing the call.
		// This will make sure another nested external call will
		// be handled properly.
		hostCallData = NULL;
		mutex.unlock();
		// Execute the external call
		hostCallHandler(data);
		// Keep waiting
		callback->wait();
		mutex.lock();
	}
	// Get the result of our callback
	std::map<const ASObject*, std::unique_ptr<ExtObject>> asObjectsMap;
	bool res = callback->getResult(asObjectsMap, *this, &objResult);

	// Reset the root current callback to NULL, if necessary
	if(rootCallback)
		currentCallback = NULL;

	// No more shared data should be used hereafter
	mutex.unlock();

	// Delete our callback after use
	delete callback;

	// Delete converted arguments
	for(uint32_t i = 0; i < argc; i++)
		delete objArgs[i];

	if(objResult != NULL)
	{
		// Copy the result into the raw NPVariant result and delete intermediate result
		std::map<const ExtObject*, NPObject*> objectsMap;
		NPVariantObject::ExtVariantToNPVariant(objectsMap, instance, *objResult, *result);
		delete objResult;
	}
	return res;
}

bool NPScriptObject::invokeDefault(const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObjectGW::invokeDefault");
	return false;
}

// ExtScriptObject interface: methods
bool NPScriptObject::removeMethod(const lightspark::ExtIdentifier& id)
{
	std::map<ExtIdentifier, lightspark::ExtCallback*>::iterator it = methods.find(id);
	if(it == methods.end())
		return false;

	delete (*it).second;
	methods.erase(it);
	return true;
}

// ExtScriptObject interface: properties
const ExtVariant& NPScriptObject::getProperty(const lightspark::ExtIdentifier& id) const
{
	std::map<ExtIdentifier, ExtVariant>::const_iterator it = properties.find(id);
	assert(it != properties.end());
	return it->second;
}
bool NPScriptObject::removeProperty(const lightspark::ExtIdentifier& id)
{
	std::map<ExtIdentifier, ExtVariant>::iterator it = properties.find(id);
	if(it == properties.end())
		return false;

	properties.erase(it);
	return true;
}

// ExtScriptObject interface: enumeration
bool NPScriptObject::enumerate(lightspark::ExtIdentifier*** ids, uint32_t* count) const
{
	*count = properties.size()+methods.size();
	*ids = new lightspark::ExtIdentifier*[properties.size()+methods.size()];
	std::map<ExtIdentifier, ExtVariant>::const_iterator prop_it;
	int i = 0;
	for(prop_it = properties.begin(); prop_it != properties.end(); ++prop_it)
	{
		(*ids)[i] = new NPIdentifierObject(prop_it->first);
		i++;
	}
	std::map<ExtIdentifier, lightspark::ExtCallback*>::const_iterator meth_it;
	for(meth_it = methods.begin(); meth_it != methods.end(); ++meth_it)
	{
		(*ids)[i] = new NPIdentifierObject(meth_it->first);
		i++;
	}

	return true;
}

// This method allows calling a function on the main thread in a generic way.
void NPScriptObject::doHostCall(NPScriptObject::HOST_CALL_TYPE type,
	void* returnValue, void* arg1, void* arg2, void* arg3, void* arg4)
{
	// Used to signal completion of asynchronous external call
	Semaphore callStatus(0);
	HOST_CALL_DATA callData = {
		this,
		&callStatus,
		type,
		arg1,
		arg2,
		arg3,
		arg4,
		returnValue
	};
	
	// We are in the main thread,
	// so we can call the method ourselves synchronously straight away
	if(Thread::self() == mainThread)
	{
		hostCallHandler(&callData);
		return;
	}

	// Make sure we are the only external call being executed
	mutex.lock();
	// If we are shutting down, then don't even continue
	if(shuttingDown)
	{
		mutex.unlock();
		return;
	}

	// If we are the first external call, then indicate that an external call is running
	if(callStatusses.size() == 0)
		hostCall.lock();

	// Add this callStatus semaphore to the list of running call statuses to be cleaned up on shutdown
	callStatusses.push(&callStatus);

	// Main thread is not occupied by an invoked callback,
	// so ask the browser to asynchronously call our external function.
	if(currentCallback == NULL)
		NPN_PluginThreadAsyncCall(instance, &NPScriptObject::hostCallHandler, &callData);
	// Main thread is occupied by an invoked callback.
	// Wake it up and ask it run our external call
	else
	{
		hostCallData = &callData;
		currentCallback->wakeUp();
	}

	// Called JS may invoke a callback, which in turn may invoke another external method, which needs this mutex
	mutex.unlock();

	// Wait for the (possibly asynchronously) called function to finish
	callStatus.wait();

	mutex.lock();

	// This call status doesn't need to be cleaned up anymore on shutdown
	callStatusses.pop();

	// If we are the last external call, then indicate that all external calls are now finished
	if(callStatusses.size() == 0)
		hostCall.unlock();

	mutex.unlock();
}

void NPScriptObject::hostCallHandler(void* d)
{
	HOST_CALL_DATA* callData = static_cast<HOST_CALL_DATA*>(d);
	
	nsPluginInstance* plugin = static_cast<nsPluginInstance*>(callData->so->instance->pdata);
	lightspark::SystemState* prevSys = getSys();
	bool tlsSysSet = false;
	if(plugin && plugin->m_sys)
	{
		tlsSysSet = true;
		setTLSSys(plugin->m_sys);
	}

	// Assert we are in the main plugin thread
	callData->so->assertThread();

	switch(callData->type)
	{
	case EXTERNAL_CALL:
		*static_cast<bool*>(callData->returnValue) = callExternalHandler(callData->so->instance,
			static_cast<const char*>(callData->arg1), static_cast<const ExtVariant**>(callData->arg2),
			*static_cast<uint32_t*>(callData->arg3), static_cast<ASObject**>(callData->arg4));
		break;
	default:
		LOG(LOG_ERROR, "Unimplemented host call requested");
	}

	callData->callStatus->signal();
	if(tlsSysSet)
		setTLSSys(prevSys);
}

// ExtScriptObject interface: calling external methods
bool NPScriptObject::callExternal(const lightspark::ExtIdentifier& id,
		const lightspark::ExtVariant** args, uint32_t argc, lightspark::ASObject** result)
{
	// Signals when the async has been completed
	// True if NPN_Invoke succeeded
	bool success = false;

	// We forge an anonymous function with the right number of arguments
	std::string argsString;
	for(uint32_t i=0;i<argc;i++)
	{
		char buf[20];
		if((i+1)==argc)
			snprintf(buf,20,"a%u",i);
		else
			snprintf(buf,20,"a%u,",i);
		argsString += buf;
	}

	std::string scriptString = "(function(";
	scriptString += argsString;
	scriptString += ") { return (" + id.getString();
	scriptString += ")(" + argsString + "); })";

	LOG(LOG_CALLS,"Invoking " << scriptString << " in the browser ");

	doHostCall(EXTERNAL_CALL, &success, const_cast<char*>(scriptString.c_str()), const_cast<ExtVariant**>(args), &argc, result);
	return success;
}

bool NPScriptObject::callExternalHandler(NPP instance, const char* scriptString,
		const lightspark::ExtVariant** args, uint32_t argc, lightspark::ASObject** result)
{
	// !!! We assume this method is only called on the main thread !!!

	// This will hold the result from our NPN_Invoke(Default) call
	NPVariant resultVariant;

	NPObject* windowObject;
	NPN_GetValue(instance, NPNVWindowNPObject, &windowObject);

	NPString script;
	script.UTF8Characters = scriptString;
	script.UTF8Length = strlen(scriptString);
	bool success;
	success = NPN_Evaluate(instance, windowObject, &script, &resultVariant);

	//Evaluate should have returned a function object, if not bail out
	if(success)
	{
		//SUCCESS
		if(!NPVARIANT_IS_OBJECT(resultVariant))
		{
			//Something very wrong happended, our forged function is not a function!
			LOG(LOG_ERROR,"Could not evaluate wrapper function in external interface");
		}
		else
		{
			// These will get passed as arguments to NPN_Invoke(Default)
			NPVariant* variantArgs = g_newa(NPVariant,argc);
			for(uint32_t i = 0; i < argc; i++)
			{
				std::map<const ExtObject*, NPObject*> objectsMap;
				NPVariantObject::ExtVariantToNPVariant(objectsMap, instance, *(args[i]), variantArgs[i]);
			}

			NPVariant evalResult = resultVariant;
			NPObject* evalObj = NPVARIANT_TO_OBJECT(resultVariant);
			// Lets try to invoke the default function on the object returned by the evaluation
			success = NPN_InvokeDefault(instance, evalObj,
						variantArgs, argc, &resultVariant);

			//Release the result fo the evaluation
			NPN_ReleaseVariantValue(&evalResult);

			// Release the converted arguments
			for(uint32_t i = 0; i < argc; i++)
				NPN_ReleaseVariantValue(&(variantArgs[i]));

			if(success)
			{
				std::map<const NPObject*, std::unique_ptr<ExtObject>> npObjectsMap;
				NPVariantObject tmp(npObjectsMap, instance, resultVariant);
				std::map<const ExtObject*, ASObject*> asObjectsMap;
				*(result) = tmp.getASObject(asObjectsMap);
				NPN_ReleaseVariantValue(&resultVariant);
			}

		}
	}

	return success;
}

void NPScriptObject::setException(const std::string& message) const
{
	if(marshallExceptions)
		NPN_SetException(gw, message.c_str());
	else
		NPN_SetException(gw, "Error in Actionscript. Use a try/catch block to find error.");
}

// Standard Flash methods
bool NPScriptObject::stdGetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	if(argc!=1 || args[0]->getType()!=lightspark::ExtVariant::EV_STRING)
		return false;
	//Only support properties currently
	ExtIdentifier argId(args[0]->getString());
	if(so.hasProperty(argId))
	{
		*result=new ExtVariant(so.getProperty(argId));
		return true;
	}

	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdGetVariable");
	*result = new ExtVariant();
	return false;
}

bool NPScriptObject::stdSetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdSetVariable");
	*result = new ExtVariant(false);
	return false;
}

bool NPScriptObject::stdGotoFrame(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdGotoFrame");
	*result = new ExtVariant(false);
	return false;
}

bool NPScriptObject::stdIsPlaying(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdIsPlaying");
	*result = new ExtVariant(true);
	return true;
}

bool NPScriptObject::stdLoadMovie(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdLoadMovie");
	*result = new ExtVariant(false);
	return false;
}

bool NPScriptObject::stdPan(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPan");
	*result = new ExtVariant(false);
	return false;
}

bool NPScriptObject::stdPercentLoaded(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPercentLoaded");
	*result = new ExtVariant(100);
	return true;
}

bool NPScriptObject::stdPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPlay");
	*result = new ExtVariant(false);
	return false;
}

bool NPScriptObject::stdRewind(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdRewind");
	*result = new ExtVariant(false);
	return false;
}

bool NPScriptObject::stdStopPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdStopPlay");
	*result = new ExtVariant(false);
	return false;
}

bool NPScriptObject::stdSetZoomRect(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdSetZoomRect");
	*result = new ExtVariant(false);
	return false;
}

bool NPScriptObject::stdZoom(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdZoom");
	*result = new ExtVariant(false);
	return false;
}

bool NPScriptObject::stdTotalFrames(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdTotalFrames");
	*result = new ExtVariant(false);
	return false;
}

/* -- NPScriptObjectGW -- */
// NPScriptObjectGW NPClass for use with NPRuntime
NPClass NPScriptObjectGW::npClass = 
{
	NP_CLASS_STRUCT_VERSION,
	allocate,
	deallocate,
	invalidate,
	hasMethod,
	invoke,
	invokeDefault,
	hasProperty,
	getProperty,
	setProperty,
	removeProperty,
	enumerate,
	construct
};

// Constructor
NPScriptObjectGW::NPScriptObjectGW(NPP inst) : instance(inst)
{
	assert(instance != NULL);
	so = new NPScriptObject(this);

	NPN_GetValue(instance, NPNVWindowNPObject, &windowObject);
	NPN_GetValue(instance, NPNVPluginElementNPObject, &pluginElementObject);
}

// Destructor
NPScriptObjectGW::~NPScriptObjectGW()
{
	// The NPScriptObject should already be deleted _before_ we get to this point.
	// This is the only way we can guarantee that it is deleted _before_ SystemState is deleted.
};

// Properties
bool NPScriptObjectGW::getProperty(NPObject* obj, NPIdentifier id, NPVariant* result)
{
	lightspark::SystemState* prevSys = getSys();
	setTLSSys( static_cast<NPScriptObjectGW*>(obj)->m_sys );

	NPScriptObject* so = static_cast<NPScriptObjectGW*>(obj)->so;
	NPIdentifierObject idObj(id);
	if(so->hasProperty(idObj)==false)
	{
		setTLSSys( prevSys );
		return false;
	}
	const ExtVariant& resultObj = so->getProperty(idObj);

	std::map<const ExtObject*, NPObject*> objectsMap;
	NPVariantObject::ExtVariantToNPVariant(objectsMap, static_cast<NPScriptObjectGW*>(obj)->instance, resultObj, *result);

	setTLSSys( prevSys );
	return true;
}

// Enumeration
bool NPScriptObjectGW::enumerate(NPObject* obj, NPIdentifier** value, uint32_t* count)
{
	lightspark::SystemState* prevSys = getSys();
	setTLSSys( static_cast<NPScriptObjectGW*>(obj)->m_sys );

	NPScriptObject* o = static_cast<NPScriptObjectGW*>(obj)->so;
	lightspark::ExtIdentifier** ids = NULL;
	bool success = o->enumerate(&ids, count);
	if(success)
	{
		*value = (NPIdentifier*) NPN_MemAlloc(sizeof(NPIdentifier)*(*count));
		for(uint32_t i = 0; i < *count; i++)
		{
			(*value)[i] = dynamic_cast<NPIdentifierObject&>(*ids[i]).getNPIdentifier();
			delete ids[i];
		}
	}

	if(ids != NULL)
		delete ids;

	setTLSSys( prevSys );
	return success;
}
