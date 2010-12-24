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

#include "logger.h"

#include "npscriptobject.h"

NPClass NPScriptObject::npClass = 
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

NPScriptObject::NPScriptObject(NPP inst) : instance(inst)
{
	assert(instance != NULL);

	NPN_GetValue(instance, NPNVWindowNPObject, &windowObject);
	NPN_GetValue(instance, NPNVPluginElementNPObject, &pluginElementObject);

	setProperty("$version", "10,0,r"SHORTVERSION);

	if(pluginElementObject != NULL)
	{
		NPVariant result;
		NPN_GetProperty(instance, pluginElementObject, NPN_GetStringIdentifier("id"), &result);
		setProperty("id", NPVariantObject::getString(result));
		NPN_ReleaseVariantValue(&result);
		NPN_GetProperty(instance, pluginElementObject, NPN_GetStringIdentifier("name"), &result);
		setProperty("name", NPVariantObject::getString(result));
		NPN_ReleaseVariantValue(&result);
	}

	// Standard methods
	setMethod("SetVariable", stdSetVariable);
	setMethod("GetVariable", stdGetVariable);
	setMethod("GotoFrame", stdGotoFrame);
	setMethod("IsPlaying", stdIsPlaying);
	setMethod("LoadMovie", stdLoadMovie);
	setMethod("Pan", stdPan);
	setMethod("PercentLoaded", stdPercentLoaded);
	setMethod("Play", stdPlay);
	setMethod("Rewind", stdRewind);
	setMethod("SetZoomRect", stdSetZoomRect);
	setMethod("StopPlay", stdStopPlay);
	setMethod("Zoom", stdZoom);
	setMethod("TotalFrames", stdTotalFrames);
}

/* Utility: property related */
NPVariantObject NPScriptObject::getProperty(const std::string& name)
{
	NPVariant variant;
	getProperty(NPN_GetStringIdentifier(name.c_str()), &variant);
	NPVariantObject obj(variant);
	NPN_ReleaseVariantValue(&variant);
	return obj;
}
void NPScriptObject::setProperty(const std::string& name, const std::string& value)
{
	NPVariant strvar;
	STRINGN_TO_NPVARIANT(value.c_str(), static_cast<int>(value.size()), strvar);
	setProperty(NPN_GetStringIdentifier(name.c_str()), strvar);
}
void NPScriptObject::setProperty(const std::string& name, double value)
{
	NPVariant doublevar;
	DOUBLE_TO_NPVARIANT(value, doublevar);
	setProperty(NPN_GetStringIdentifier(name.c_str()), doublevar);
}
void NPScriptObject::setProperty(const std::string& name, int value)
{
	NPVariant intvar;
	INT32_TO_NPVARIANT(value, intvar);
	setProperty(NPN_GetStringIdentifier(name.c_str()), intvar);
}

/* NPRuntime: method related */
bool NPScriptObject::invoke(NPIdentifier name, const NPVariant* args, uint32_t argc, NPVariant* result)
{
	std::map<NPIdentifier, NPInvokeFunctionPtr>::iterator it = methods.find(name);
	if(it != methods.end())
		return ((NPInvokeFunctionPtr) it->second)(NULL, name, args, argc, result);

	return false;
}

bool NPScriptObject::invokeDefault(const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::invokeDefault");
	return false;
}

/* NPRuntime: property related */
bool NPScriptObject::getProperty(NPIdentifier name, NPVariant* result)
{
	std::map<NPIdentifier, NPVariantObject>::const_iterator it = properties.find(name);
	if(it == properties.end())
		return false;

	it->second.copy(*result);
	return true;
}

bool NPScriptObject::removeProperty(NPIdentifier name)
{
	std::map<NPIdentifier, NPVariantObject>::iterator it = properties.find(name);
	if(it != properties.end())
	{
		properties.erase(it);
		return true;
	}

	return false;
}

/* NPRuntime: misc */
bool NPScriptObject::enumerate(NPIdentifier** value, uint32_t* count)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::enumerate");
	return false;
}

bool NPScriptObject::construct(const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::construct");
	return false;
}

/* Standard methods */
bool NPScriptObject::stdGetVariable(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdGetVariable");
	NULL_TO_NPVARIANT(*result);
	return false;
}

bool NPScriptObject::stdSetVariable(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdSetVariable");
	BOOLEAN_TO_NPVARIANT(false, *result);
	return false;
}

bool NPScriptObject::stdGotoFrame(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdGotoFrame");
	BOOLEAN_TO_NPVARIANT(false, *result);
	return false;
}

bool NPScriptObject::stdIsPlaying(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdIsPlaying");
	BOOLEAN_TO_NPVARIANT(true, *result);
	return true;
}

bool NPScriptObject::stdLoadMovie(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdLoadMovie");
	BOOLEAN_TO_NPVARIANT(false, *result);
	return false;
}

bool NPScriptObject::stdPan(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPan");
	BOOLEAN_TO_NPVARIANT(false, *result);
	return false;
}

bool NPScriptObject::stdPercentLoaded(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPercentLoaded");
	INT32_TO_NPVARIANT(100, *result);
	return true;
}

bool NPScriptObject::stdPlay(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPlay");
	BOOLEAN_TO_NPVARIANT(false, *result);
	return false;
}

bool NPScriptObject::stdRewind(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdRewind");
	BOOLEAN_TO_NPVARIANT(false, *result);
	return false;
}

bool NPScriptObject::stdStopPlay(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdStopPlay");
	BOOLEAN_TO_NPVARIANT(false, *result);
	return false;
}

bool NPScriptObject::stdSetZoomRect(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdSetZoomRect");
	BOOLEAN_TO_NPVARIANT(false, *result);
	return false;
}

bool NPScriptObject::stdZoom(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdZoom");
	BOOLEAN_TO_NPVARIANT(false, *result);
	return false;
}

bool NPScriptObject::stdTotalFrames(NPObject* obj, NPIdentifier name,
			const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdTotalFrames");
	BOOLEAN_TO_NPVARIANT(false, *result);
	return false;
}
