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

#include "asobject.h"

#include "backends/extscriptobject.h"

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
	{
		int intValue = atoi(value.c_str());
		std::stringstream out;
		out << intValue;
		// Convert integer string identifiers to integer identifiers
		if(out.str() == value)
			identifier = NPN_GetIntIdentifier(intValue);
		else
			identifier = NPN_GetStringIdentifier(value.c_str());
	}
	NPIdentifierObject(const char* value)
	{
		int intValue = atoi(value);
		std::stringstream out;
		out << intValue;
		// Convert integer string identifiers to integer identifiers
		if(out.str() == std::string(value))
			identifier = NPN_GetIntIdentifier(intValue);
		else
			identifier = NPN_GetStringIdentifier(value);
	}
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
		if(getType(identifier) == EI_INT32)
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

class NPVariantObject;
/**
 */
class DLL_PUBLIC NPObjectObject : public lightspark::ExtObject
{
public:
	NPObjectObject() : instance(NULL) {}
	NPObjectObject(NPP _instance) : instance(_instance) {}
	NPObjectObject(NPP _instance, lightspark::ExtObject& other) : instance(_instance)
	{
		setType(other.getType());
		copy(other, *this);
	}
	NPObjectObject(NPP _instance, NPObject* obj);
	
	~NPObjectObject() {}
	NPObjectObject& operator=(const lightspark::ExtObject& other)
	{
		setType(other.getType());
		copy(other, *this);
		return *this;
	}

	void copy(std::map<lightspark::ExtIdentifierObject, lightspark::ExtVariantObject>& dest) const;
	void copy(std::map<NPIdentifierObject, NPVariantObject>& dest) const;

	bool hasProperty(const lightspark::ExtIdentifierObject& id) const;
	// The returned value should be "delete"d by the caller after use
	lightspark::ExtVariantObject* getProperty(const lightspark::ExtIdentifierObject& id) const;
	void setProperty(const lightspark::ExtIdentifierObject& id, const lightspark::ExtVariantObject& value);
	bool removeProperty(const lightspark::ExtIdentifierObject& id);

	bool enumerate(lightspark::ExtIdentifierObject*** ids, uint32_t* count) const;

	uint32_t getLength() const { return properties.size(); }
private:
	NPP instance;
	std::map<NPIdentifierObject, NPVariantObject> properties;

	static void copy(const lightspark::ExtObject& from, lightspark::ExtObject& to);
};

/**
 * This class extends ExtVariantObject and as such can be used in the same way.
 * It can also convert NPVariants into NPVariantObjects and vice-versa.
 * It can also convert ExtVariantObjects into NPVariantObjects (object-type variants excluded)
 * and as such can convert ExtVariantObjects into NPVariants.
 */
class NPVariantObject : public lightspark::ExtVariantObject
{
public:
	NPVariantObject() : instance(NULL)
	{ VOID_TO_NPVARIANT(variant); }
	NPVariantObject(NPP _instance) : instance(_instance)
	{ VOID_TO_NPVARIANT(variant); }
	NPVariantObject(NPP _instance, const std::string& value) : instance(_instance)
	{
		NPVariant temp;
		STRINGN_TO_NPVARIANT(value.c_str(), (int) value.size(), temp);
		copy(temp, variant);
	}
	NPVariantObject(NPP _instance, const char* value) : instance(_instance)
	{
		NPVariant temp;
		STRINGN_TO_NPVARIANT(value, (int) strlen(value), temp);
		copy(temp, variant);
	}
	NPVariantObject(NPP _instance, int32_t value) : instance(_instance)
	{ INT32_TO_NPVARIANT(value, variant); }
	NPVariantObject(NPP _instance, double value) : instance(_instance)
	{ DOUBLE_TO_NPVARIANT(value, variant); }
	NPVariantObject(NPP _instance, bool value) : instance(_instance)
	{ BOOLEAN_TO_NPVARIANT(value, variant); }

	// ExtVariantObject copy-constructor
	NPVariantObject(NPP _instance, const ExtVariantObject& value) : instance(_instance)
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
					lightspark::ExtObject* objValue = value.getObject();
					uint32_t count = objValue->getLength();
					ExtVariantObject* property;
					NPVariant varProperty;
					NPObject* windowObject;
					NPN_GetValue(instance, NPNVWindowNPObject, &windowObject);
					if(objValue->getType() == lightspark::ExtObject::EO_ARRAY)
					{
						NPN_Invoke(instance, windowObject, NPN_GetStringIdentifier("Array"), NULL, 0, &variant);
						NPObject* arrObj = NPVARIANT_TO_OBJECT(variant);
						NPVariant varResult;

						for(uint32_t i = 0; i < count; i++)
						{
							property = objValue->getProperty(i);
							NPVariantObject(instance, *property).copy(varProperty);
							NPN_Invoke(instance, arrObj, NPN_GetStringIdentifier("push"), &varProperty, 1, &varResult);
							NPN_ReleaseVariantValue(&varResult);
							NPN_ReleaseVariantValue(&varProperty);
							delete property;
						}
					}
					else
					{
						NPN_Invoke(instance, windowObject, NPN_GetStringIdentifier("Object"), NULL, 0, &variant);
						NPObject* objObj = NPVARIANT_TO_OBJECT(variant);
						lightspark::ExtIdentifierObject** ids = NULL;

						if(objValue->enumerate(&ids, &count))
						{
							for(uint32_t i = 0; i < count; i++)
							{
								property = objValue->getProperty(*ids[i]);
								NPVariantObject(instance, *property).copy(varProperty);
								NPN_SetProperty(instance, objObj, NPIdentifierObject(*ids[i]).getNPIdentifier(), &varProperty);
								NPN_ReleaseVariantValue(&varProperty);
								delete property;
								delete ids[i];
							}
						}
						if(ids != NULL)
							delete ids;
					}
					delete objValue;
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
	NPVariantObject(NPP _instance, const NPVariantObject& other) : instance(_instance)
	{
		other.copy(variant);
	}
	NPVariantObject(NPP _instance, const NPVariant& other) : instance(_instance)
	{
		copy(other, variant);
	}

	~NPVariantObject() { NPN_ReleaseVariantValue(&variant); }

	// Copy this NPVariantObject's NPVariant value to another one.
	void copy(NPVariant& dest) const { copy(variant, dest); }

	// Straight copying of this object isn't correct.
	// We need to properly copy the stored data.
	NPVariantObject& operator=(const NPVariantObject& other)
	{
		instance = other.getInstance();
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

	NPP getInstance() const { return instance; }

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
	lightspark::ExtObject* getObject() const
	{
		return new NPObjectObject(instance, NPVARIANT_TO_OBJECT(variant));
	}
private:
	NPP instance;
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
 * Multiple inheritance doesn't seem to work will when used with the NPObject base class.
 * Thats why we use this gateway class which inherits only from ExtScriptObject.
 */
class DLL_PUBLIC NPScriptObject : public lightspark::ExtScriptObject
{
public:
	NPScriptObject(NPP _instance);
	~NPScriptObject() {};

	// These methods are not part of the ExtScriptObject interface.
	// ExtScriptObject does not provide a way to invoke the set methods.
	bool invoke(NPIdentifier name, const NPVariant* args, uint32_t argc, NPVariant* result);
	bool invokeDefault(const NPVariant* args, uint32_t argc, NPVariant* result);

	/* ExtScriptObject interface */
	bool hasMethod(const lightspark::ExtIdentifierObject& id) const
	{
		return methods.find(id) != methods.end();
	}
	void setMethod(const lightspark::ExtIdentifierObject& id, const lightspark::ExtCallbackFunction& func)
	{
		methods[id] = func;
	}
	bool removeMethod(const lightspark::ExtIdentifierObject& id);

	bool hasProperty(const lightspark::ExtIdentifierObject& id) const
	{
		return properties.find(id) != properties.end();
	}
	// The returned value should be "delete"d by the caller after use
	NPVariantObject* getProperty(const lightspark::ExtIdentifierObject& id) const;
	void setProperty(const lightspark::ExtIdentifierObject& id, const lightspark::ExtVariantObject& value)
	{
		properties[id] = NPVariantObject(instance, value);
	}
	bool removeProperty(const lightspark::ExtIdentifierObject& id);

	bool enumerate(lightspark::ExtIdentifierObject*** ids, uint32_t* count) const;

	// Standard methods
	// These methods are standard to every flash instance.
	// They provide features such as getting/setting internal variables,
	// going to a frame, pausing etc... to the external container.
	static bool stdGetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdSetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdGotoFrame(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdIsPlaying(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdLoadMovie(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdPan(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdPercentLoaded(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdRewind(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdStopPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdSetZoomRect(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdZoom(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
	static bool stdTotalFrames(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result);
private:
	NPP instance;
	// This map stores this object's methods & properties
	// If an entry is set with a ExtIdentifierObject or ExtVariantObject,
	// they get converted to NPIdentifierObject or NPVariantObject by copy-constructors.
	std::map<NPIdentifierObject, NPVariantObject> properties;
	std::map<NPIdentifierObject, lightspark::ExtCallbackFunction> methods;
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
		so = new NPScriptObject(instance);

		NPN_GetValue(instance, NPNVWindowNPObject, &windowObject);
		NPN_GetValue(instance, NPNVPluginElementNPObject, &pluginElementObject);

		// Lets set these basic properties, fetched from the NPRuntime
		if(pluginElementObject != NULL)
		{
			NPVariant result;
			NPN_GetProperty(instance, pluginElementObject, NPN_GetStringIdentifier("id"), &result);
			NPVariantObject objResult(instance, result);
			so->setProperty("id", objResult);
			NPN_ReleaseVariantValue(&result);
			NPN_GetProperty(instance, pluginElementObject, NPN_GetStringIdentifier("name"), &result);
			so->setProperty("name", NPVariantObject(instance, result));
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
		((NPScriptObjectGW*) obj)->so->setProperty(NPIdentifierObject(id), NPVariantObject(((NPScriptObjectGW*) obj)->instance, *value));
		return true;
	}
	static bool removeProperty(NPObject* obj, NPIdentifier id)
	{
		return ((NPScriptObjectGW*) obj)->so->removeProperty(NPIdentifierObject(id));
	}

	static bool enumerate(NPObject* obj, NPIdentifier** value, uint32_t* count)
	{
		NPScriptObject* o = ((NPScriptObjectGW*) obj)->so;
		lightspark::ExtIdentifierObject** ids = NULL;
		if(o->enumerate(&ids, count))
		{
			*value = (NPIdentifier*) NPN_MemAlloc(sizeof(NPIdentifier)*(*count));
			for(uint32_t i = 0; i < *count; i++)
			{
				(*value)[i] = NPIdentifierObject(*ids[i]).getNPIdentifier();
				delete ids[i];
			}
			if(ids != NULL)
				delete ids;

			return true;
		}
		if(ids != NULL)
			delete ids;

		return false;
	}
	
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
