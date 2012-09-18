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

#ifndef PLUGIN_NPSCRIPTOBJECT_H
#define PLUGIN_NPSCRIPTOBJECT_H 1

#include <typeinfo>
#include <string.h>
#include <string>
#include <map>
#include <stack>

#include "plugin/include/npapi/npapi.h"
#include "plugin/include/npapi/npruntime.h"

#include "swf.h"
#include "asobject.h"

#include "backends/extscriptobject.h"

namespace lightspark
{
/**
 * This class extends ExtIdentifier. As such it can be used the same way.
 * It can also convert NPIdentifiers into NPIdentifierObjects and vice versa.
 * It can also convert ExtIdentifiers into NPIdentifierObjects, and as such
 * can convert ExtIdentifiers into NPIdentifiers.
 */
class NPIdentifierObject : public lightspark::ExtIdentifier
{
public:
	NPIdentifierObject(const std::string& value);
	NPIdentifierObject(const char* value);
	NPIdentifierObject(int32_t value);
	NPIdentifierObject(const ExtIdentifier& value);
	NPIdentifierObject(const NPIdentifierObject& id);
	NPIdentifierObject(const NPIdentifier& id, bool convertToInt=false);

	~NPIdentifierObject() {}

	void copy(NPIdentifier& dest) const;

	// NPIdentifierObjects get used as keys in an std::map, so they need to comparable
	bool operator<(const ExtIdentifier& other) const;

	EI_TYPE getType() const { return getType(identifier); }
	static EI_TYPE getType(const NPIdentifier& identifier);

	std::string getString() const { return getString(identifier); }
	static std::string getString(const NPIdentifier& identifier);
	int32_t getInt() const { return getInt(identifier); }
	static int32_t getInt(const NPIdentifier& identifier);
	NPIdentifier getNPIdentifier() const;

	// Returns true, if identifier is an interger or an all-digit
	// string.
	bool isNumeric() const;
private:
	NPIdentifier identifier;

	void stringToInt(const std::string& value);

	// Copy one identifier to the other.
	// This forms the base for the NPIdentifier & NPIdentifierObject copy-constructors.
	static void copy(const NPIdentifier& from, NPIdentifier& dest);
};

class NPVariantObject;
/**
 * This class extends ExtObject. As such it can be used in the same way.
 * It can also convert NPObjects into NPObjectObjects and vice versa.
 * It can also convert ExtObjects into NPObjectObjects, and as such
 * can convert ExtObjects into NPObjects.
 */
class NPObjectObject : public lightspark::ExtObject
{
public:
	NPObjectObject(std::map<const NPObject*, std::unique_ptr<ExtObject>>& objectsMap, NPP _instance, NPObject* obj);
	
	NPObject* getNPObject(std::map<const ExtObject*, NPObject*>& objectsMap) const { return getNPObject(objectsMap, instance, this); }
	static NPObject* getNPObject(std::map<const ExtObject*, NPObject*>& objectsMap, NPP instance, const lightspark::ExtObject* obj);
private:
	NPP instance;

	bool isArray(NPObject* obj) const;
};

/**
 * This class extends ExtVariant and as such can be used in the same way.
 * It can also convert NPVariants into NPVariantObjects and vice-versa.
 * It can also convert ExtVariants into NPVariantObjects (object-type variants excluded)
 * and as such can convert ExtVariants into NPVariants.
 */
class NPVariantObject : public lightspark::ExtVariant
{
public:
	NPVariantObject(std::map<const NPObject*, std::unique_ptr<ExtObject>>& objectsMap, NPP _instance, const NPVariant& other);
	static void ExtVariantToNPVariant(std::map<const ExtObject*, NPObject*>& objectsMap, NPP _instance,
			const ExtVariant& value, NPVariant& variant);

	static EV_TYPE getTypeS(const NPVariant& variant);
};

class NPScriptObjectGW;
/**
 * Multiple inheritance doesn't seem to work will when used with the NPObject base class.
 * Thats why we use this gateway class which inherits only from ExtScriptObject.
 */
class DLL_PUBLIC NPScriptObject : public lightspark::ExtScriptObject
{
public:
	NPScriptObject(NPScriptObjectGW* gw);
	~NPScriptObject();
	// Stops all waiting external calls, should be called before destruction.
	// Actual destruction should be initiated by the browser, as a last step of destruction.
	void destroy();

	void assertThread() { assert(Thread::self() == mainThread); }

	// These methods are not part of the ExtScriptObject interface.
	// ExtScriptObject does not provide a way to invoke the set methods.
	bool invoke(NPIdentifier name, const NPVariant* args, uint32_t argc, NPVariant* result);
	bool invokeDefault(const NPVariant* args, uint32_t argc, NPVariant* result);

	/* ExtScriptObject interface */
	// Methods
	bool hasMethod(const lightspark::ExtIdentifier& id) const
	{
		return methods.find(id) != methods.end();
	}
	void setMethod(const lightspark::ExtIdentifier& id, lightspark::ExtCallback* func)
	{
		methods[id] = func;
	}
	bool removeMethod(const lightspark::ExtIdentifier& id);

	// Properties
	bool hasProperty(const lightspark::ExtIdentifier& id) const
	{
		return properties.find(id) != properties.end();
	}
	const ExtVariant& getProperty(const lightspark::ExtIdentifier& id) const;
	void setProperty(const lightspark::ExtIdentifier& id, const lightspark::ExtVariant& value)
	{
		properties[id] = value;
	}
	bool removeProperty(const lightspark::ExtIdentifier& id);

	// Enumeration
	bool enumerate(lightspark::ExtIdentifier*** ids, uint32_t* count) const;
	

	enum HOST_CALL_TYPE {EXTERNAL_CALL};
	typedef struct {
		NPScriptObject* so;
		Semaphore* callStatus;
		HOST_CALL_TYPE type;
		void* arg1;
		void* arg2;
		void* arg3;
		void* arg4;
		void* returnValue;
	} HOST_CALL_DATA;
	// This method allows calling some method, while making sure
	// no unintended blocking occurs.
	void doHostCall(HOST_CALL_TYPE type, void* returnValue,
		void* arg1, void* arg2=NULL, void* arg3=NULL, void* arg4=NULL);
	static void hostCallHandler(void* d);

	// Calling methods in the external container
	bool callExternal(const lightspark::ExtIdentifier& id, const lightspark::ExtVariant** args, uint32_t argc, lightspark::ASObject** result);

	// This is called from hostCallHandler() via doHostCall(EXTERNAL_CALL, ...)
	static bool callExternalHandler(NPP instance, const char* scriptString,
		const lightspark::ExtVariant** args, uint32_t argc, lightspark::ASObject** result);

	// Throwing exceptions to the container
	void setException(const std::string& message) const;
	void setMarshallExceptions(bool marshall) { marshallExceptions = marshall; }
	bool getMarshallExceptions() const { return marshallExceptions; }

	// Standard methods
	// These methods are standard to every flash instance.
	// They provide features such as getting/setting internal variables,
	// going to a frame, pausing etc... to the external container.
	static bool stdGetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdSetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdGotoFrame(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdIsPlaying(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdLoadMovie(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdPan(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdPercentLoaded(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdRewind(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdStopPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdSetZoomRect(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdZoom(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
	static bool stdTotalFrames(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, const lightspark::ExtVariant** result);
private:
	NPScriptObjectGW* gw;
	NPP instance;
	// Used to determine if a method is called in the main plugin thread
	Thread* mainThread;

	// Provides mutual exclusion for external calls
	Mutex mutex;
	std::stack<Semaphore*> callStatusses;
	Mutex externalCall;
	Mutex hostCall;

	// The root callback currently being invoked. If this is not NULL
	// when invoke() gets called, we can assume the invoke()
	// is nested inside another one.
	lightspark::ExtCallback* currentCallback;
	// The data for the external call that needs to be made.
	// If a callback is woken up and this is not NULL,
	// it was a forced wake-up and we should call an external method.
	HOST_CALL_DATA* hostCallData;

	// True if this object is being shut down
	bool shuttingDown;
	// True if exceptions should be marshalled to the container
	bool marshallExceptions;

	// This map stores this object's methods & properties
	// If an entry is set with a ExtIdentifier or ExtVariant,
	// they get converted to NPIdentifierObject or NPVariantObject by copy-constructors.
	std::map<ExtIdentifier, ExtVariant> properties;
	std::map<ExtIdentifier, lightspark::ExtCallback*> methods;
};

/**
 * This class acts as a gateway between NPRuntime & NPScriptObject.
 * Multiple inheritance doesn't seem to work will in conjunction with NPObject.
 * That's the main reason this gateway is used.
 */
class NPScriptObjectGW : public NPObject
{
public:
	NPScriptObjectGW(NPP inst);
	~NPScriptObjectGW();

	NPScriptObject* getScriptObject() { return so; }
	NPP getInstance() { return instance; }
	
	lightspark::SystemState* m_sys;

	static NPClass npClass;

	/* Static gateway methods used by NPRuntime */
	static NPObject* allocate(NPP instance, NPClass* _class)
	{
		return new NPScriptObjectGW(instance);
	}
	static void deallocate(NPObject* obj)
	{
		delete (NPScriptObjectGW*) obj;
	}
	static void invalidate(NPObject* obj) { }

	/* NPScriptObject forwarders */
	static bool hasMethod(NPObject* obj, NPIdentifier id)
	{
		lightspark::SystemState* prevSys = getSys();
		setTLSSys( static_cast<NPScriptObjectGW*>(obj)->m_sys );
		bool success = static_cast<NPScriptObjectGW*>(obj)->so->hasMethod(NPIdentifierObject(id));
		setTLSSys(prevSys);
		return success;
	}
	static bool invoke(NPObject* obj, NPIdentifier id,
			const NPVariant* args, uint32_t argc, NPVariant* result)
	{
		lightspark::SystemState* prevSys = getSys();
		setTLSSys( static_cast<NPScriptObjectGW*>(obj)->m_sys );
		bool success = static_cast<NPScriptObjectGW*>(obj)->so->invoke(id, args, argc, result);
		setTLSSys(prevSys);
		return success;
	}
	static bool invokeDefault(NPObject* obj,
			const NPVariant* args, uint32_t argc, NPVariant* result)
	{
		lightspark::SystemState* prevSys = getSys();
		setTLSSys( static_cast<NPScriptObjectGW*>(obj)->m_sys );
		bool success = static_cast<NPScriptObjectGW*>(obj)->so->invokeDefault(args, argc, result);
		setTLSSys( prevSys );
		return success;
	}

	static bool hasProperty(NPObject* obj, NPIdentifier id)
	{
		lightspark::SystemState* prevSys = getSys();
		setTLSSys( static_cast<NPScriptObjectGW*>(obj)->m_sys );
		bool success = static_cast<NPScriptObjectGW*>(obj)->so->hasProperty(NPIdentifierObject(id));
		setTLSSys( prevSys );
		return success;
	}
	static bool getProperty(NPObject* obj, NPIdentifier id, NPVariant* result);
	static bool setProperty(NPObject* obj, NPIdentifier id, const NPVariant* value)
	{
		lightspark::SystemState* prevSys = getSys();
		setTLSSys( static_cast<NPScriptObjectGW*>(obj)->m_sys );
		std::map<const NPObject*, std::unique_ptr<ExtObject>> objectsMap;
		static_cast<NPScriptObjectGW*>(obj)->so->setProperty(
			NPIdentifierObject(id), NPVariantObject(objectsMap,static_cast<NPScriptObjectGW*>(obj)->instance, *value));
		bool success = true;
		setTLSSys( prevSys );
		return success;
	}
	static bool removeProperty(NPObject* obj, NPIdentifier id)
	{
		lightspark::SystemState* prevSys = getSys();
		setTLSSys( static_cast<NPScriptObjectGW*>(obj)->m_sys );
		bool success = static_cast<NPScriptObjectGW*>(obj)->so->removeProperty(NPIdentifierObject(id));
		setTLSSys( prevSys );
		return success;
	}

	static bool enumerate(NPObject* obj, NPIdentifier** value, uint32_t* count);
	
	/* Not implemented */
	static bool construct(NPObject* obj,
			const NPVariant* args, uint32_t argc, NPVariant* result) { return false; }
private:
	NPScriptObject* so;
	NPP instance;
	NPObject* windowObject;
	NPObject* pluginElementObject;
};

}
#endif /* PLUGIN_NPSCRIPTOBJECT_H */
