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
	NPIdentifierObject objId(id);
	// Check if the method exists
	std::map<NPIdentifierObject, lightspark::ExtCallbackFunctionPtr>::iterator it;
	it = methods.find(objId);
	if(it == methods.end())
		return false;

	// Convert raw arguments to objects
	NPVariantObject objArgs[argc];
	for(uint32_t i = 0; i < argc; i++)
		objArgs[i] = NPVariantObject(args[i]);

	// Run the function
	lightspark::ExtVariantObject* objResult;
	bool res = ((lightspark::ExtCallbackFunctionPtr) it->second)(objId, objArgs, argc, &objResult);
	if(objResult == NULL)
		return false;

	// Copy the result into the raw NPVariant result and delete intermediate result
	NPVariantObject(*objResult).copy(*result);
	delete objResult;

	return res;
}

bool NPScriptObject::invokeDefault(const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObjectGW::invokeDefault");
	return false;
}

NPVariantObject* NPScriptObject::getProperty(const lightspark::ExtIdentifierObject& id)
{
	std::map<NPIdentifierObject, NPVariantObject>::const_iterator it = properties.find(id);
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
bool NPScriptObject::stdGetVariable(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdGetVariable");
	*result = new NPVariantObject();
	return false;
}

bool NPScriptObject::stdSetVariable(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdSetVariable");
	*result = new NPVariantObject(false);
	return false;
}

bool NPScriptObject::stdGotoFrame(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdGotoFrame");
	*result = new NPVariantObject(false);
	return false;
}

bool NPScriptObject::stdIsPlaying(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdIsPlaying");
	*result = new NPVariantObject(true);
	return true;
}

bool NPScriptObject::stdLoadMovie(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdLoadMovie");
	*result = new NPVariantObject(false);
	return false;
}

bool NPScriptObject::stdPan(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPan");
	*result = new NPVariantObject(false);
	return false;
}

bool NPScriptObject::stdPercentLoaded(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPercentLoaded");
	*result = new NPVariantObject(100);
	return true;
}

bool NPScriptObject::stdPlay(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPlay");
	*result = new NPVariantObject(false);
	return false;
}

bool NPScriptObject::stdRewind(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdRewind");
	*result = new NPVariantObject(false);
	return false;
}

bool NPScriptObject::stdStopPlay(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdStopPlay");
	*result = new NPVariantObject(false);
	return false;
}

bool NPScriptObject::stdSetZoomRect(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdSetZoomRect");
	*result = new NPVariantObject(false);
	return false;
}

bool NPScriptObject::stdZoom(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdZoom");
	*result = new NPVariantObject(false);
	return false;
}

bool NPScriptObject::stdTotalFrames(const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject* args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdTotalFrames");
	*result = new NPVariantObject(false);
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
