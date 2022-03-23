/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/flash/system/flashsystem.h"

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
			bool allNumericProperties = true;

			objectsMap[other.getPtr()] = unique_ptr<ExtObject>(objectValue);

			unsigned int index = 0;
			while((index=other->nextNameIndex(index))!=0)
			{
				asAtom nextName=asAtomHandler::invalidAtom;
				other->nextName(nextName,index);
				asAtom nextValue=asAtomHandler::invalidAtom;
				other->nextValue(nextValue,index);

				if(asAtomHandler::isInteger(nextName))
					objectValue->setProperty(asAtomHandler::toInt(nextName), ExtVariant(objectsMap, _MR(asAtomHandler::toObject(nextValue,other->getInstanceWorker()))));
				else
				{
					allNumericProperties = false;
					objectValue->setProperty(asAtomHandler::toString(nextName,other->getInstanceWorker()).raw_buf(), ExtVariant(objectsMap, _MR(asAtomHandler::toObject(nextValue,other->getInstanceWorker()))));
				}
			}

			if(other->getObjectType()==T_ARRAY && allNumericProperties)
			{
				objectValue->setType(ExtObject::EO_ARRAY);
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
ASObject* ExtVariant::getASObject(ASWorker *wrk, std::map<const lightspark::ExtObject*, lightspark::ASObject*>& objectsMap) const
{
	//don't create ASObjects from cache, as we are not in the vm thread
	ASObject* asobj;
	switch(getType())
	{
	case EV_STRING:
		asobj = Class<ASString>::getInstanceS(wrk,getString().c_str());
		break;
	case EV_INT32:
		asobj = Class<Integer>::getInstanceS(wrk,getInt());
		break;
	case EV_DOUBLE:
		asobj = Class<Number>::getInstanceS(wrk,getDouble());
		break;
	case EV_BOOLEAN:
		asobj = Class<Boolean>::getInstanceS(wrk,getBoolean());
		break;
	case EV_OBJECT:
		{
			ExtObject* objValue = getObject();
			/*
			auto it=objectsMap.find(objValue);
			if(it!=objectsMap.end())
			{
				it->second->incRef();
				return it->second;
			}
			*/
			uint32_t count;

			// We are converting an array, so lets set indexes
			if(objValue->getType() == ExtObject::EO_ARRAY)
			{
				asobj = Class<Array>::getInstanceSNoArgs(wrk);
				objectsMap[objValue] = asobj;

				count = objValue->getLength();
				static_cast<Array*>(asobj)->resize(count);
				for(uint32_t i = 0; i < count; i++)
				{
					const ExtVariant& property = objValue->getProperty(i);
					asAtom v = asAtomHandler::fromObject(property.getASObject(wrk,objectsMap));
					static_cast<Array*>(asobj)->set(i, v);
				}
			}
			// We are converting an object, so lets set variables
			else
			{
				asobj = Class<ASObject>::getInstanceS(wrk);
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
									property.getASObject(wrk,objectsMap), DYNAMIC_TRAIT);
						}
						else
						{
							conv.str("");
							conv << ids[i]->getInt();
							if(asobj->hasPropertyByMultiname(QName(wrk->getSystemState()->getUniqueStringId(conv.str()),BUILTIN_STRINGS::EMPTY),true,true,wrk))
							{
								LOG(LOG_NOT_IMPLEMENTED,"ExtVariant::getASObject: duplicate property " << conv.str());
								continue;
							}
							asobj->setVariableByQName(conv.str().c_str(), "",
									property.getASObject(wrk,objectsMap), DYNAMIC_TRAIT);
						}
						delete ids[i];
					}
				}
				delete[] ids;
			}
		}
		break;
	case EV_NULL:
		asobj = wrk->getSystemState()->getNullRef();
		break;
	case EV_VOID:
	default:
		asobj = wrk->getSystemState()->getUndefinedRef();
		break;
	}
	return asobj;
}

/* -- ExtASCallback -- */
ExtASCallback::ExtASCallback(asAtom _func):funcWasCalled(false), func(_func), result(nullptr), asArgs(nullptr)
{
	ASATOM_INCREF(func);
}

ExtASCallback::~ExtASCallback() {
	ASATOM_DECREF(func);
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
		asArgs[i] = args[i]->getASObject(asAtomHandler::getObject(func)->getInstanceWorker(),objectsMap);

	if(!synchronous)
	{
		ASATOM_INCREF(func);
		funcEvent = _MR(new (asAtomHandler::getObject(func)->getSystemState()->unaccountedMemory) ExternalCallEvent(func, asArgs, argc, &result, &exceptionThrown, &exception));
		// Add the callback function event to the top of the VM event queue
		funcWasCalled=getVm(asAtomHandler::getObject(func)->getSystemState())->prependEvent(NullRef,funcEvent);
		if(!funcWasCalled)
		{
			LOG(LOG_ERROR,"funcEvent not called");
			funcEvent = NullRef;
		}
		else
			asAtomHandler::getObject(func)->getSystemState()->sendMainSignal();
	}
	// The caller indicated the VM is currently suspended, so call synchronously.
	else
	{
		try
		{
			asAtom* newArgs=nullptr;
			if (argc > 0)
			{
				newArgs=g_newa(asAtom, argc);
				for (uint32_t i = 0; i < argc; i++)
				{
					newArgs[i] = asAtomHandler::fromObject(asArgs[i]);
				}
			}

			/* TODO: shouldn't we pass some global object instead of Null? */
			asAtom res=asAtomHandler::invalidAtom;
			asAtomHandler::callFunction(func,asAtomHandler::getObject(func)->getInstanceWorker(),res,asAtomHandler::nullAtom, newArgs, argc,false);
			result = asAtomHandler::toObject(res,asAtomHandler::getObject(func)->getInstanceWorker());
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
		funcEvent->wait();
}
void ExtASCallback::wakeUp()
{
	if(!funcEvent.isNull())
		funcEvent->signal();
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
		LOG(LOG_INFO,"ExtBuiltinCallback::call: exception:"<<_exception->toString()<<" "<<id.getString()<<" "<<argc);
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

// This method allows calling a function on the main thread in a generic way.
void ExtScriptObject::doHostCall(ExtScriptObject::HOST_CALL_TYPE type,
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
	if(SDL_GetThreadID(nullptr) == mainThreadID)
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
		callAsync(&callData);
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

void ExtScriptObject::hostCallHandler(void* d)
{
	HOST_CALL_DATA* callData = static_cast<HOST_CALL_DATA*>(d);
	
	lightspark::SystemState* prevSys = getSys();
	bool tlsSysSet = false;
	if(callData->so->getSystemState())
	{
		tlsSysSet = true;
		setTLSSys(callData->so->getSystemState());
	}

	// Assert we are in the main plugin thread
	callData->so->assertThread();

	switch(callData->type)
	{
	case EXTERNAL_CALL:
		*static_cast<bool*>(callData->returnValue) = callData->so->callExternalHandler(
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

bool ExtScriptObject::callExternal(const ExtIdentifier &id, const ExtVariant **args, uint32_t argc, ASObject **result)
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

// ExtScriptObject interface: methods
ExtScriptObject::ExtScriptObject(SystemState *sys):
	m_sys(sys),currentCallback(nullptr), hostCallData(nullptr),
	shuttingDown(false), marshallExceptions(false)
{
	// This object is always created in the main plugin thread, so lets save
	// so that we can check if we are in the main plugin thread later on.
	mainThreadID = SDL_GetThreadID(nullptr);

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

ExtScriptObject::~ExtScriptObject()
{
	std::map<ExtIdentifier, lightspark::ExtCallback*>::iterator meth_it = methods.begin();
	while(meth_it != methods.end())
	{
		delete (*meth_it).second;
		methods.erase(meth_it++);
	}
}
// Destructor preparator
void ExtScriptObject::destroy()
{
	mutex.lock();
	// Prevents new external calls from continuing
	shuttingDown = true;

	// If an external call is running, wake it up
	if(callStatusses.size() > 0)
		callStatusses.top()->signal();
	mutex.unlock();
	// Wait for all external calls to finish
	Locker l(externalCall);
}
bool ExtScriptObject::doinvoke(const ExtIdentifier& id, const ExtVariant **args, uint32_t argc, const lightspark::ExtVariant* result)
{
	// If the NPScriptObject is shutting down, don't even continue
	if(shuttingDown)
		return false;

	// Check if the method exists
	std::map<ExtIdentifier, lightspark::ExtCallback*>::iterator it;
	it = methods.find(id);
	if(it == methods.end())
		return false;

	LOG(LOG_CALLS,"Plugin callback from the browser: " << id.getString());

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
	callback->call(*this, id, args, argc, synchronous);
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
	bool res = callback->getResult(asObjectsMap, *this, &result);

	// Reset the root current callback to NULL, if necessary
	if(rootCallback)
		currentCallback = NULL;

	// No more shared data should be used hereafter
	mutex.unlock();

	// Delete our callback after use
	delete callback;

	return res;
}


bool ExtScriptObject::removeMethod(const lightspark::ExtIdentifier& id)
{
	std::map<ExtIdentifier, lightspark::ExtCallback*>::iterator it = methods.find(id);
	if(it == methods.end())
		return false;

	delete (*it).second;
	methods.erase(it);
	return true;
}

// ExtScriptObject interface: properties
const ExtVariant& ExtScriptObject::getProperty(const lightspark::ExtIdentifier& id) const
{
	std::map<ExtIdentifier, ExtVariant>::const_iterator it = properties.find(id);
	assert(it != properties.end());
	return it->second;
}
bool ExtScriptObject::removeProperty(const lightspark::ExtIdentifier& id)
{
	std::map<ExtIdentifier, ExtVariant>::iterator it = properties.find(id);
	if(it == properties.end())
		return false;

	properties.erase(it);
	return true;
}

bool ExtScriptObject::enumerate(ExtIdentifier ***ids, uint32_t *count) const
{
	*count = properties.size()+methods.size();
	*ids = new lightspark::ExtIdentifier*[properties.size()+methods.size()];
	std::map<ExtIdentifier, ExtVariant>::const_iterator prop_it;
	int i = 0;
	for(prop_it = properties.begin(); prop_it != properties.end(); ++prop_it)
	{
		(*ids)[i] = createEnumerationIdentifier(prop_it->first);
		i++;
	}
	std::map<ExtIdentifier, lightspark::ExtCallback*>::const_iterator meth_it;
	for(meth_it = methods.begin(); meth_it != methods.end(); ++meth_it)
	{
		(*ids)[i] = createEnumerationIdentifier(meth_it->first);
		i++;
	}

	return true;
}
// Standard Flash methods
bool ExtScriptObject::stdGetVariable(const lightspark::ExtScriptObject& so,
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

	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdGetVariable");
	*result = new ExtVariant();
	return false;
}

bool ExtScriptObject::stdSetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdSetVariable");
	*result = new ExtVariant(false);
	return false;
}

bool ExtScriptObject::stdGotoFrame(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdGotoFrame");
	*result = new ExtVariant(false);
	return false;
}

bool ExtScriptObject::stdIsPlaying(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdIsPlaying");
	*result = new ExtVariant(true);
	return true;
}

bool ExtScriptObject::stdLoadMovie(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdLoadMovie");
	*result = new ExtVariant(false);
	return false;
}

bool ExtScriptObject::stdPan(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdPan");
	*result = new ExtVariant(false);
	return false;
}

bool ExtScriptObject::stdPercentLoaded(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdPercentLoaded");
	*result = new ExtVariant(100);
	return true;
}

bool ExtScriptObject::stdPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdPlay");
	*result = new ExtVariant(false);
	return false;
}

bool ExtScriptObject::stdRewind(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdRewind");
	*result = new ExtVariant(false);
	return false;
}

bool ExtScriptObject::stdStopPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdStopPlay");
	*result = new ExtVariant(false);
	return false;
}

bool ExtScriptObject::stdSetZoomRect(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdSetZoomRect");
	*result = new ExtVariant(false);
	return false;
}

bool ExtScriptObject::stdZoom(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdZoom");
	*result = new ExtVariant(false);
	return false;
}

bool ExtScriptObject::stdTotalFrames(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "ExtScriptObject::stdTotalFrames");
	*result = new ExtVariant(false);
	return false;
}
