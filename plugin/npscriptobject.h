/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010  Timon Van Overveldt (timonvo@gmail.com)

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
#include <sstream>

#include "npapi.h"
#include "npruntime.h"

#include "backends/extscriptobject.h"

/**
 * This class extends ExtVariantObject and as such can be used in the same way.
 * It can also convert NPVariants into NPVariantObjects and vice-versa.
 * It can also convert ExtVariantObjects into NPVariantObjects (object-type variants excluded)
 * and as such can convert ExtVariantObjects into NPVariants.
 */
class NPVariantObject : public lightspark::ExtVariantObject
{
public:
	NPVariantObject()
	{ VOID_TO_NPVARIANT(variant); }
	NPVariantObject(const std::string& value)
	{
		NPVariant temp;
		STRINGN_TO_NPVARIANT(value.c_str(), (int) value.size(), temp);
		copy(temp, variant);
	}
	NPVariantObject(const char* value)
	{
		NPVariant temp;
		STRINGN_TO_NPVARIANT(value, (int) strlen(value), temp);
		copy(temp, variant);
	}
	NPVariantObject(int32_t value)
	{ INT32_TO_NPVARIANT(value, variant); }
	NPVariantObject(double value)
	{ DOUBLE_TO_NPVARIANT(value, variant); }
	NPVariantObject(bool value)
	{ BOOLEAN_TO_NPVARIANT(value, variant); }

	// ExtVariantObject copy-constructor
	NPVariantObject(const ExtVariantObject& value)
	{
		// It's possible we got a down-casted NPVariantObject, so lets check for it
		try
		{
			dynamic_cast<const NPVariantObject&>(value).copy(variant);
		}
		// Seems we got a real ExtVariantObject, lets convert
		catch(std::bad_cast&)
		{
			switch(value.getType())
			{
			case EVO_STRING:
				{
					std::string strValue = value.getString();
					NPVariant temp;
					STRINGN_TO_NPVARIANT(strValue.c_str(), (int) strValue.size(), temp);
					copy(temp, variant);
					break;
				}
			case EVO_INT32:
				INT32_TO_NPVARIANT(value.getInt(), variant);
				break;
			case EVO_DOUBLE:
				DOUBLE_TO_NPVARIANT(value.getDouble(), variant);
				break;
			case EVO_BOOLEAN:
				BOOLEAN_TO_NPVARIANT(value.getBoolean(), variant);
				break;
			case EVO_OBJECT:
				{
					// We only do a very shallow copy here.
					// None of the object properties are actually copied.
					NPObject* obj = (NPObject*) NPN_MemAlloc(sizeof(NPObject));
					OBJECT_TO_NPVARIANT(obj, variant);
					break;
				}
			case EVO_NULL:
				NULL_TO_NPVARIANT(variant);
				break;
			case EVO_VOID:
			default:
				VOID_TO_NPVARIANT(variant);
				break;
			}
		}
	}
	// NPVariantObject copy-constructor
	NPVariantObject(const NPVariantObject& other) { other.copy(variant); }
	NPVariantObject(const NPVariant& other) {
		copy(other, variant);
	}

	~NPVariantObject() { NPN_ReleaseVariantValue(&variant); }

	// Copy this NPVariantObject's NPVariant value to another one.
	void copy(NPVariant& dest) const { copy(variant, dest); }

	// Straight copying of this object isn't correct.
	// We need to properly copy the stored data.
	NPVariantObject& operator=(const NPVariantObject& other)
	{
		if(this != &other)
		{
			NPN_ReleaseVariantValue(&variant);
			other.copy(variant);
		}
		return *this;
	}
	NPVariantObject& operator=(const NPVariant& other)
	{
		if(&variant != &other)
		{
			NPN_ReleaseVariantValue(&variant);
			copy(other, variant);
		}
		return *this;
	}

	EVO_TYPE getType() const { return getType(variant); }
	static EVO_TYPE getType(const NPVariant& variant)
	{
		if(NPVARIANT_IS_STRING(variant))
			return EVO_STRING;
		else if(NPVARIANT_IS_INT32(variant))
			return EVO_INT32;
		else if(NPVARIANT_IS_DOUBLE(variant))
			return EVO_DOUBLE;
		else if(NPVARIANT_IS_BOOLEAN(variant))
			return EVO_BOOLEAN;
		else if(NPVARIANT_IS_OBJECT(variant))
			return EVO_OBJECT;
		else if(NPVARIANT_IS_NULL(variant))
			return EVO_NULL;
		else
			return EVO_VOID;
	}

	std::string getString() const { return getString(variant); }
	static std::string getString(const NPVariant& variant)
	{
		if(getType(variant) == EVO_STRING)
		{
			NPString val = NPVARIANT_TO_STRING(variant);
			return std::string(val.UTF8Characters, val.UTF8Length);
		}
		return "";
	}
	int32_t getInt() const { return getInt(variant); }
	static int32_t getInt(const NPVariant& variant)
	{
		if(getType(variant) == EVO_INT32)
			return NPVARIANT_TO_INT32(variant);
		return 0;
	}
	double getDouble() const { return getDouble(variant); }
	static double getDouble(const NPVariant& variant)
	{
		if(getType(variant) == EVO_DOUBLE)
			return NPVARIANT_TO_DOUBLE(variant);
		return 0.0;
	}
	bool getBoolean() const { return getBoolean(variant); }
	static bool getBoolean(const NPVariant& variant)
	{
		if(getType(variant) == EVO_BOOLEAN)
			return NPVARIANT_TO_BOOLEAN(variant);
		return false;
	}

private:
	NPVariant variant;

	// This forms the base for the NPVariantObject & NPVariant copy-constructors.
	static void copy(const NPVariant& from, NPVariant& dest)
	{
		dest = from;

		switch(from.type)
		{
		case NPVariantType_String:
			{
				const NPString& value = NPVARIANT_TO_STRING(from);
				
				NPUTF8* newValue = static_cast<NPUTF8*>(NPN_MemAlloc(value.UTF8Length));
				memcpy(newValue, value.UTF8Characters, value.UTF8Length);

				STRINGN_TO_NPVARIANT(newValue, value.UTF8Length, dest);
				break;
			}
		case NPVariantType_Object:
			NPN_RetainObject(NPVARIANT_TO_OBJECT(dest));
			break;
		default: {}
		}
	}
};

/**
 * This class extends ExtIdentifierObject. As such it can be used the same way.
 * It can also convert NPIdentifiers into NPIdentifierObjects and vice versa.
 * It can also convert ExtIdentifierObjects into NPIdentifierObjects, and as such
 * can convert ExtIdentifierObjects into NPIdentifiers.
 */
class NPIdentifierObject : public lightspark::ExtIdentifierObject
{
public:
	NPIdentifierObject(const std::string& value)
	{ identifier = NPN_GetStringIdentifier(value.c_str()); }
	NPIdentifierObject(const char* value)
	{ identifier = NPN_GetStringIdentifier(value); }
	NPIdentifierObject(int32_t value)
	{ identifier = NPN_GetIntIdentifier(value); }

	// ExtIdentifierObject copy-constructor
	NPIdentifierObject(const ExtIdentifierObject& value)
	{
		// It is possible we got a down-casted ExtIdentifierObject, so lets check for that
		try
		{
			dynamic_cast<const NPIdentifierObject&>(value).copy(identifier);
		}
		// We got a real ExtIdentifierObject, lets convert it
		catch(std::bad_cast&)
		{
			if(value.getType() == EI_STRING)
				identifier = NPN_GetStringIdentifier(value.getString().c_str());
			else
				identifier = NPN_GetIntIdentifier(value.getInt());
		}
	}
	// NPIdentifierObject copy-constructor
	NPIdentifierObject(const NPIdentifierObject& id) { id.copy(identifier); }
	// NPIdentifier copy-constructor, converts and NPIdentifier into an NPIdentifierObject
	NPIdentifierObject(const NPIdentifier& id) { copy(id, identifier); }

	~NPIdentifierObject() {}

	void copy(NPIdentifier& dest) const { copy(identifier, dest); }

	// NPIdentifierObjects get used as keys in an std::map, so they need to comparable
	bool operator<(const ExtIdentifierObject& other) const
	{
		// It is possible we got a down-casted NPIdentifierObject, so lets check for that
		try
		{
			return identifier < dynamic_cast<const NPIdentifierObject&>(other).getNPIdentifier();
		}
		// We got a real ExtIdentifierObject, let ExtIdentifierObject::operator< handle this
		catch(std::bad_cast&)
		{
			return ExtIdentifierObject::operator<(other);
		}
	}


	EI_TYPE getType() const { return getType(identifier); }
	static EI_TYPE getType(const NPIdentifier& identifier)
	{
		if(NPN_IdentifierIsString(identifier))
			return EI_STRING;
		else
			return EI_INT32;
	}

	std::string getString() const { return getString(identifier); }
	static std::string getString(const NPIdentifier& identifier)
	{
		if(getType(identifier) == EI_STRING)
			return std::string(NPN_UTF8FromIdentifier(identifier));
		else
			return "";
	}
	int32_t getInt() const { return getInt(identifier); }
	static int32_t getInt(const NPIdentifier& identifier)
	{
		if(getType(identifier) == EI_STRING)
			return NPN_IntFromIdentifier(identifier);
		else
			return 0;
	}

	NPIdentifier getNPIdentifier() const
	{
		if(getType() == EI_STRING) return NPN_GetStringIdentifier(getString().c_str());
		else return NPN_GetIntIdentifier(getInt());
	}
private:
	NPIdentifier identifier;

	// Copy one identifier to the other.
	// This forms the base for the NPIdentifier & NPIdentifierObject copy-constructors.
	static void copy(const NPIdentifier& from, NPIdentifier& dest)
	{
		if(NPN_IdentifierIsString(from))
			dest = NPN_GetStringIdentifier(NPN_UTF8FromIdentifier(from));
		else
			dest = NPN_GetIntIdentifier(NPN_IntFromIdentifier(from));
	}
};


/**
 * Multiple inheritance doesn't seem to work will when used with the NPObject base class.
 * Thats why we use this gateway class which inherits only from ExtScriptObject.
 */
class DLL_PUBLIC NPScriptObject : public lightspark::ExtScriptObject
{
private:
	// This map stores this object's methods & properties
	// If an entry is set with a ExtIdentifierObject or ExtVariantObject,
	// they get converted to NPIdentifierObject or NPVariantObject by copy-constructors.
	std::map<NPIdentifierObject, NPVariantObject> properties;
	std::map<NPIdentifierObject, NPInvokeFunctionPtr> methods;
public:
	NPScriptObject();
	~NPScriptObject() {};

	//bool invoke(const NPIdentifierObject& id, const NPVariantObject* args, uint32_t argc, NPVariantObject* result);
	bool invoke(NPIdentifier name, const NPVariant* args, uint32_t argc, NPVariant* result);

	//bool invokeDefault(const NPVariantObject* args, uint32_t argc, NPVariantObject* result);
	bool invokeDefault(const NPVariant* args, uint32_t argc, NPVariant* result);

	/* ExtScriptObject interface */
	bool hasMethod(const lightspark::ExtIdentifierObject& id)
	{
		return methods.find(id) != methods.end();
	}
	void setMethod(const NPIdentifierObject& id, NPInvokeFunctionPtr func)
	{
		methods[id] = func;
	}
	bool hasProperty(const lightspark::ExtIdentifierObject& id)
	{
		return properties.find(id) != properties.end();
	}

	// The returned value should be "delete"d by the caller after use
	NPVariantObject* getProperty(const lightspark::ExtIdentifierObject& id);
	void setProperty(const lightspark::ExtIdentifierObject& id, const lightspark::ExtVariantObject& value)
	{
		properties[id] = value;
	}
	bool removeProperty(const lightspark::ExtIdentifierObject& id);

	// Standard methods
	// These methods are standard to every flash instance.
	// They provide features such as getting/setting internal variables,
	// going to a frame, pausing etc... to the external container.
	static bool stdGetVariable(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdSetVariable(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdGotoFrame(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdIsPlaying(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdLoadMovie(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdPan(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdPercentLoaded(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdPlay(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdRewind(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdStopPlay(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdSetZoomRect(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdZoom(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
	static bool stdTotalFrames(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result);
};

/**
 * This class acts as a gateway between NPRuntime & NPScriptObject.
 * Multiple inheritance doesn't seem to work will in conjunction with NPObject.
 * That's the main reason this gateway is used.
 */
class NPScriptObjectGW : public NPObject
{
public:
	NPScriptObjectGW(NPP inst) : instance(inst)
	{
		assert(instance != NULL);
		so = new NPScriptObject();

		NPN_GetValue(instance, NPNVWindowNPObject, &windowObject);
		NPN_GetValue(instance, NPNVPluginElementNPObject, &pluginElementObject);

		// Lets set these basic properties, fetched from the NPRuntime
		if(pluginElementObject != NULL)
		{
			NPVariant result;
			NPN_GetProperty(instance, pluginElementObject, NPN_GetStringIdentifier("id"), &result);
			NPVariantObject objResult(result);
			so->setProperty("id", objResult);
			NPN_ReleaseVariantValue(&result);
			NPN_GetProperty(instance, pluginElementObject, NPN_GetStringIdentifier("name"), &result);
			so->setProperty("name", NPVariantObject(result));
			NPN_ReleaseVariantValue(&result);
		}
	}
	~NPScriptObjectGW()
	{
		delete so;
	};

	NPScriptObject* getScriptObject() { return so; }
	
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
		return ((NPScriptObjectGW*) obj)->so->hasMethod(NPIdentifierObject(id));
	}
	static bool invoke(NPObject* obj, NPIdentifier id,
			const NPVariant* args, uint32_t argc, NPVariant* result)
	{
		return ((NPScriptObjectGW*) obj)->so->invoke(id, args, argc, result);
	}
	static bool invokeDefault(NPObject* obj,
			const NPVariant* args, uint32_t argc, NPVariant* result)
	{
		return ((NPScriptObjectGW*) obj)->so->invokeDefault(args, argc, result);
	}

	static bool hasProperty(NPObject* obj, NPIdentifier id)
	{
		return ((NPScriptObjectGW*) obj)->so->hasProperty(NPIdentifierObject(id));
	}
	static bool getProperty(NPObject* obj, NPIdentifier id, NPVariant* result)
	{
		NPVariantObject* resultObj = ((NPScriptObjectGW*) obj)->so->getProperty(NPIdentifierObject(id));
		if(resultObj == NULL)
			return false;

		resultObj->copy(*result);
		delete resultObj;
		return true;
	}
	static bool setProperty(NPObject* obj, NPIdentifier id, const NPVariant* value)
	{
		((NPScriptObjectGW*) obj)->so->setProperty(NPIdentifierObject(id), NPVariantObject(*value));
		return true;
	}
	static bool removeProperty(NPObject* obj, NPIdentifier id)
	{
		return ((NPScriptObjectGW*) obj)->so->removeProperty(NPIdentifierObject(id));
	}

	/* Non-implemented */
	static bool enumerate(NPObject* obj, NPIdentifier** value, uint32_t* count) { return false; }
	static bool construct(NPObject* obj,
			const NPVariant* args, uint32_t argc, NPVariant* result) { return false; }
private:
	NPScriptObject* so;
	NPP instance;
	NPObject* windowObject;
	NPObject* pluginElementObject;
};

#endif
