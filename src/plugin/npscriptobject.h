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

#ifndef _NP_SCRIPT_OBJECT_H
#define _NP_SCRIPT_OBJECT_H

#include <typeinfo>
#include <string.h>
#include <string>
#include <map>
#include <stack>

#include "npapi.h"
#include "npruntime.h"

#include "swf.h"
#include "asobject.h"
#include "platforms/pluginutils.h"

#include "backends/extscriptobject.h"

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
	NPIdentifierObject(const NPIdentifier& id);

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
	NPObjectObject();
	NPObjectObject(NPP _instance);
	NPObjectObject(NPP _instance, lightspark::ExtObject& other);
	NPObjectObject(NPP _instance, NPObject* obj);
	
	~NPObjectObject() {}

	NPObjectObject& operator=(const lightspark::ExtObject& other);

	// Properties
	bool hasProperty(const lightspark::ExtIdentifier& id) const;
	// The returned value should be "delete"d by the caller after use
	lightspark::ExtVariant* getProperty(const lightspark::ExtIdentifier& id) const;
	void setProperty(const lightspark::ExtIdentifier& id, const lightspark::ExtVariant& value);
	bool removeProperty(const lightspark::ExtIdentifier& id);

	bool enumerate(lightspark::ExtIdentifier*** ids, uint32_t* count) const;
	uint32_t getLength() const { return properties.size(); }

	NPObject* getNPObject() const { return getNPObject(instance, *this); }
	static NPObject* getNPObject(NPP instance, const lightspark::ExtObject& obj);
private:
	NPP instance;
	std::map<NPIdentifierObject, NPVariantObject> properties;

	static void copy(const lightspark::ExtObject& from, lightspark::ExtObject& to);
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
	NPVariantObject();
	NPVariantObject(NPP _instance);
	NPVariantObject(NPP _instance, const std::string& value);
	NPVariantObject(NPP _instance, const char* value);
	NPVariantObject(NPP _instance, int32_t value);
	NPVariantObject(NPP _instance, double value);
	NPVariantObject(NPP _instance, bool value);
	// ExtVariant copy-constructor
	NPVariantObject(NPP _instance, const ExtVariant& value);
	// NPVariantObject copy-constructor
	NPVariantObject(NPP _instance, const NPVariantObject& other);
	NPVariantObject(NPP _instance, const NPVariant& other);

	~NPVariantObject();

	// Copy this NPVariantObject's NPVariant value to another one.
	void copy(NPVariant& dest) const { copy(variant, dest); }

	// Straight copying of this object isn't correct.
	// We need to properly copy the stored data.
	NPVariantObject& operator=(const NPVariantObject& other);
	NPVariantObject& operator=(const NPVariant& other);

	NPP getInstance() const { return instance; }

	EV_TYPE getType() const { return getType(variant); }
	static EV_TYPE getType(const NPVariant& variant);

	std::string getString() const { return getString(variant); }
	static std::string getString(const NPVariant& variant);
	int32_t getInt() const { return getInt(variant); }
	static int32_t getInt(const NPVariant& variant);
	double getDouble() const { return getDouble(variant); }
	static double getDouble(const NPVariant& variant);
	bool getBoolean() const { return getBoolean(variant); }
	static bool getBoolean(const NPVariant& variant);
	lightspark::ExtObject* getObject() const;
private:
	NPP instance;
	NPVariant variant;

	// This forms the base for the NPVariantObject & NPVariant copy-constructors.
	static void copy(const NPVariant& from, NPVariant& dest);
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
	// The returned value should be "delete"d by the caller after use
	NPVariantObject* getProperty(const lightspark::ExtIdentifier& id) const;
	void setProperty(const lightspark::ExtIdentifier& id, const lightspark::ExtVariant& value)
	{
		properties[id] = NPVariantObject(instance, value);
	}
	bool removeProperty(const lightspark::ExtIdentifier& id);

	// Enumeration
	bool enumerate(lightspark::ExtIdentifier*** ids, uint32_t* count) const;
	
	// Calling methods in the external container
	bool callExternal(const lightspark::ExtIdentifier& id, const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);

	typedef struct {
		pthread_t* mainThread;
		NPP instance;
		NPIdentifier id;
		const char* scriptString;
		const lightspark::ExtVariant** args;
		uint32_t argc;
		lightspark::ExtVariant** result;
		sem_t* callStatus;
		bool* success;
	} EXT_CALL_DATA;
	// This must be called from the plugin thread
	static void callExternal(void* data);

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
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdSetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdGotoFrame(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdIsPlaying(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdLoadMovie(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdPan(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdPercentLoaded(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdRewind(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdStopPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdSetZoomRect(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdZoom(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
	static bool stdTotalFrames(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result);
private:
	NPScriptObjectGW* gw;
	NPP instance;
	// Used to determine if a method is called in the main plugin thread
	pthread_t mainThread;

	// Provides mutual exclusion for external calls
	sem_t mutex;
	std::stack<sem_t*> callStatusses;
	sem_t externalCallsFinished;

	// The root callback currently being invoked. If this is not NULL
	// when invoke() gets called, we can assume the invoke()
	// is nested inside another one.
	lightspark::ExtCallback* currentCallback;
	// The data for the external call that needs to be made.
	// If a callback is woken up and this is not NULL,
	// it was a forced wake-up and we should call an external method.
	EXT_CALL_DATA* externalCallData;

	// True if this object is being shut down
	bool shuttingDown;
	// True if exceptions should be marshalled to the container
	bool marshallExceptions;

	// This map stores this object's methods & properties
	// If an entry is set with a ExtIdentifier or ExtVariant,
	// they get converted to NPIdentifierObject or NPVariantObject by copy-constructors.
	std::map<NPIdentifierObject, NPVariantObject> properties;
	std::map<NPIdentifierObject, lightspark::ExtCallback*> methods;
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
		lightspark::SystemState* prevSys = sys;
		sys = ((NPScriptObjectGW*) obj)->m_sys;
		bool success = ((NPScriptObjectGW*) obj)->so->hasMethod(NPIdentifierObject(id));
		sys = prevSys;
		return success;
	}
	static bool invoke(NPObject* obj, NPIdentifier id,
			const NPVariant* args, uint32_t argc, NPVariant* result)
	{
		lightspark::SystemState* prevSys = sys;
		sys = ((NPScriptObjectGW*) obj)->m_sys;
		bool success = ((NPScriptObjectGW*) obj)->so->invoke(id, args, argc, result);
		sys = prevSys;
		return success;
	}
	static bool invokeDefault(NPObject* obj,
			const NPVariant* args, uint32_t argc, NPVariant* result)
	{
		lightspark::SystemState* prevSys = sys;
		sys = ((NPScriptObjectGW*) obj)->m_sys;
		bool success = ((NPScriptObjectGW*) obj)->so->invokeDefault(args, argc, result);
		sys = prevSys;
		return success;
	}

	static bool hasProperty(NPObject* obj, NPIdentifier id)
	{
		lightspark::SystemState* prevSys = sys;
		sys = ((NPScriptObjectGW*) obj)->m_sys;
		bool success = ((NPScriptObjectGW*) obj)->so->hasProperty(NPIdentifierObject(id));
		sys = prevSys;
		return success;
	}
	static bool getProperty(NPObject* obj, NPIdentifier id, NPVariant* result);
	static bool setProperty(NPObject* obj, NPIdentifier id, const NPVariant* value)
	{
		lightspark::SystemState* prevSys = sys;
		sys = ((NPScriptObjectGW*) obj)->m_sys;
		((NPScriptObjectGW*) obj)->so->setProperty(NPIdentifierObject(id), NPVariantObject(((NPScriptObjectGW*) obj)->instance, *value));
		bool success = true;
		sys = prevSys;
		return success;
	}
	static bool removeProperty(NPObject* obj, NPIdentifier id)
	{
		lightspark::SystemState* prevSys = sys;
		sys = ((NPScriptObjectGW*) obj)->m_sys;
		bool success = ((NPScriptObjectGW*) obj)->so->removeProperty(NPIdentifierObject(id));
		sys = prevSys;
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

#endif
