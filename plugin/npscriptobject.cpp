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

NPScriptObject::NPScriptObject()
{
	setProperty("$version", "10,0,r"SHORTVERSION);

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

bool NPScriptObject::invoke(NPIdentifier id, const NPVariant* args, uint32_t argc, NPVariant* result)
{
	std::map<NPIdentifierObject, NPInvokeFunctionPtr>::iterator it;
	it = methods.find(NPIdentifierObject(id));
	if(it != methods.end())
		return ((NPInvokeFunctionPtr) it->second)(NULL, id, args, argc, result);

	return false;
}

bool NPScriptObject::invokeDefault(const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObjectGW::invokeDefault");
	return false;
}

NPVariantObject* NPScriptObject::getProperty(const lightspark::ExtIdentifierObject& id)
{
	std::map<NPIdentifierObject, NPVariantObject>::const_iterator it = properties.find(id);
	LOG(LOG_NO_INFO, "properties stored: " << properties.size());
	//__asm__("int $3");
	if(it == properties.end())
		return NULL;

	NPVariantObject* result = new NPVariantObject(it->second);
	return result;
}
bool NPScriptObject::removeProperty(const lightspark::ExtIdentifierObject& id)
{
	std::map<NPIdentifierObject, NPVariantObject>::iterator it = properties.find(id);
	if(it == properties.end())
		return false;

	properties.erase(it);
	return true;
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
