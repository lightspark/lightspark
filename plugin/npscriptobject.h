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

#include <string.h>
#include <string>
#include <map>
#include <sstream>

#include "npapi.h"
#include "npruntime.h"

#include "backends/extscriptobject.h"

class NPVariantObject : public lightspark::ExtVariantObject
{
public:
	NPVariantObject() { NULL_TO_NPVARIANT(variant); }
	NPVariantObject(const NPVariantObject& other) { other.copy(variant); }
	NPVariantObject(const NPVariant& other) { copy(other, variant); }
	~NPVariantObject() { NPN_ReleaseVariantValue(&variant); }
	
	NPVariantObject& operator=(const NPVariantObject& other)
	{
		NPN_ReleaseVariantValue(&variant);
		other.copy(variant);
		return *this;
	}
	NPVariantObject& operator=(const NPVariant& other)
	{
		NPN_ReleaseVariantValue(&variant);
		copy(other, variant);
		return *this;
	}

	void copy(NPVariant& dest) const { copy(variant, dest); }

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
		else if(NPVARIANT_IS_VOID(variant))
			return EVO_VOID;
		else
			return EVO_UNKNOWN;
	}

	std::string toString() const { return toString(variant); }
	static std::string toString(const NPVariant& variant)
	{
		switch(getType(variant))
		{
		case EVO_STRING:
			{
				NPString val = NPVARIANT_TO_STRING(variant);
				return std::string(val.UTF8Characters, val.UTF8Length);
				break;
			}
		case EVO_INT32:
			{
				int32_t val = NPVARIANT_TO_INT32(variant);
				std::stringstream out;
				out << val;
				return out.str();
				break;
			}
		case EVO_DOUBLE:
			{
				double val = NPVARIANT_TO_DOUBLE(variant);
				std::stringstream out;
				out << val;
				return out.str();
				break;
			}
		case EVO_BOOLEAN:
			{
				bool val = NPVARIANT_TO_BOOLEAN(variant);
				return (val ? "true" : "false");
				break;
			}
		case EVO_OBJECT:
			return "object";
			break;
		case EVO_NULL:
			return "null";
			break;
		case EVO_VOID:
			return "void";
			break;
		default: {}
		}
		return "unknown type";
	}

private:
	NPVariant variant;

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

class NPScriptObject : public NPObject
{
	friend class NPScriptObjectGW;
public:
	NPScriptObject(NPP inst);
	~NPScriptObject() {};
	
	static NPClass npClass;

	/* Static methods used by NPRuntime */
	static NPObject* allocate(NPP instance, NPClass* _class)
	{ return new NPScriptObject(instance); }
	static void deallocate(NPObject* obj) { delete (NPScriptObject*) obj; }
	static void invalidate(NPObject* obj) { }

	static bool hasMethod(NPObject* obj, NPIdentifier name)
	{ return ((NPScriptObject*) obj)->hasMethod(name); }
	static bool invoke(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
	{ return ((NPScriptObject*) obj)->invoke(name, args, argc, result); }
	static bool invokeDefault(NPObject* obj,
			const NPVariant* args, uint32_t argc, NPVariant* result)
	{ return ((NPScriptObject*) obj)->invokeDefault(args, argc, result); }

	static bool hasProperty(NPObject* obj, NPIdentifier name)
	{ return ((NPScriptObject*) obj)->hasProperty(name); }
	static bool getProperty(NPObject* obj, NPIdentifier name, NPVariant* result)
	{ return ((NPScriptObject*) obj)->getProperty(name, result); }
	static bool setProperty(NPObject* obj, NPIdentifier name, const NPVariant* value)
	{ return ((NPScriptObject*) obj)->setProperty(name, *value); }
	static bool removeProperty(NPObject* obj, NPIdentifier name)
	{ return ((NPScriptObject*) obj)->removeProperty(name); }

	static bool enumerate(NPObject* obj, NPIdentifier** value, uint32_t* count)
	{ return ((NPScriptObject*) obj)->enumerate(value, count); }
	static bool construct(NPObject* obj,
			const NPVariant* args, uint32_t argc, NPVariant* result)
	{ return ((NPScriptObject*) obj)->construct(args, argc, result); }
private:
	NPP instance;
	NPObject* windowObject;
	NPObject* pluginElementObject;

	std::map<NPIdentifier, NPVariantObject> properties;
	std::map<NPIdentifier, NPInvokeFunctionPtr> methods;

	/* Utility methods */
	bool hasMethod(const std::string& name)
	{ return hasMethod(NPN_GetStringIdentifier(name.c_str())); }
	void setMethod(const std::string& name, NPInvokeFunctionPtr func)
	{ setMethod(NPN_GetStringIdentifier(name.c_str()), func); }

	bool hasProperty(const std::string& name)
	{ return hasProperty(NPN_GetStringIdentifier(name.c_str())); }
	NPVariantObject getProperty(const std::string& name);
	void setProperty(const std::string& name, const std::string& value);
	void setProperty(const std::string& name, double value);
	void setProperty(const std::string& name, int value);

	/* Methods indirectly used by NPRuntime */
	bool hasMethod(NPIdentifier name) { return methods.find(name) != methods.end(); }
	bool invoke(NPIdentifier name, const NPVariant* args, uint32_t argc, NPVariant* result);
	bool invokeDefault(const NPVariant* args, uint32_t argc, NPVariant* result);
	// Not really part of NPClass but used internally
	bool setMethod(NPIdentifier name, NPInvokeFunctionPtr func) { methods[name] = func; return true; }
	
	bool hasProperty(NPIdentifier name) { return properties.find(name) != properties.end(); }
	bool getProperty(NPIdentifier name, NPVariant* result);
	bool setProperty(NPIdentifier name, const NPVariant& value) { properties[name] = value; return true; }
	bool removeProperty(NPIdentifier name);

	bool enumerate(NPIdentifier** value, uint32_t* count);
	bool construct(const NPVariant* args, uint32_t argc, NPVariant* result);

	// Standard methods
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
 * Multiple inheritance doesn't seem to work will when used with the NPObject base class.
 * Thats why we use this gateway class which inherits only from ExtScriptObject.
 */
class DLL_PUBLIC NPScriptObjectGW : public lightspark::ExtScriptObject
{
private:
	NPScriptObject* so;
public:
	NPScriptObjectGW(NPScriptObject* scriptObject) : so(scriptObject)
	{ assert(so != NULL); };
	~NPScriptObjectGW() {};

	bool hasMethod(const std::string& name)
	{ return so->hasMethod(name); }
	void setMethod(const std::string& name, NPInvokeFunctionPtr func)
	{ so->setMethod(name, func); }

	bool hasProperty(const std::string& name)
	{ return so->hasProperty(name); }
	void setProperty(const std::string& name, const std::string& value)
	{ so->setProperty(name, value); }
	void setProperty(const std::string& name, double value)
	{ so->setProperty(name, value); }
	void setProperty(const std::string& name, int value)
	{ so->setProperty(name, value); }
	//The returned object should be deleted by the caller
	NPVariantObject* getProperty(const std::string& name)
	{ return new NPVariantObject(so->getProperty(name)); }
};

#endif
