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

using namespace std;

NPObjectObject::NPObjectObject(NPP _instance, NPObject* obj) :
	instance(_instance)
{
	NPIdentifier* ids = NULL;
	NPVariant property;
	NPIdentifierObject* id;
	bool allIntIds = true;
	uint32_t count;
	if(instance == NULL || obj == NULL)
		return;

	if(NPN_Enumerate(instance, obj, &ids, &count))
	{
		for(uint32_t i = 0; i < count; i++)
		{
			if(NPN_GetProperty(instance, obj, ids[i], &property))
			{
				id = new NPIdentifierObject(ids[i]);
				if(id->getType() != NPIdentifierObject::EI_INT32)
					allIntIds = false;

				setProperty(*id, NPVariantObject(instance, property));
				delete id;
				NPN_ReleaseVariantValue(&property);
			}
		}
		// Arrays don't enumerate the length property. So we can check that, if we got all integer ids,
		// and there exists an un-enumerated length property of type integer, we actually got passed an array.
		if(allIntIds)
		{
			if(NPN_GetProperty(instance, obj, NPIdentifierObject("length").getNPIdentifier(), &property))
			{
				if(NPVARIANT_IS_INT32(property))
				{
					setType(EO_ARRAY);
				}
				NPN_ReleaseVariantValue(&property);
			}
		}
		NPN_MemFree(ids);
	}
}

void NPObjectObject::copy(const lightspark::ExtObject& from, lightspark::ExtObject& to)
{
	lightspark::ExtIdentifierObject** ids;
	lightspark::ExtVariantObject* property;
	uint32_t count;
	if(from.enumerate(&ids, &count))
	{
		for(uint32_t i = 0; i < count; i++)
		{
			property = from.getProperty(*ids[i]);
			to.setProperty(*ids[i], *property);
			delete property;
			delete ids[i];
		}
	}
	delete ids;
}

bool NPObjectObject::hasProperty(const lightspark::ExtIdentifierObject& id) const
{
	return properties.find(id) != properties.end();
}
lightspark::ExtVariantObject* NPObjectObject::getProperty(const lightspark::ExtIdentifierObject& id) const
{
	std::map<NPIdentifierObject, NPVariantObject>::const_iterator it = properties.find(id);
	if(it == properties.end())
		return NULL;

	const NPVariantObject& temp = it->second;
	NPVariantObject* result = new NPVariantObject(instance, temp);
	return result;
}
void NPObjectObject::setProperty(const lightspark::ExtIdentifierObject& id, const lightspark::ExtVariantObject& value)
{
	properties[id] =  NPVariantObject(instance, value);
}

bool NPObjectObject::removeProperty(const lightspark::ExtIdentifierObject& id)
{
	std::map<NPIdentifierObject, NPVariantObject>::iterator it = properties.find(id);
	if(it == properties.end())
		return false;

	properties.erase(it);
	return true;
}
bool NPObjectObject::enumerate(lightspark::ExtIdentifierObject*** ids, uint32_t* count) const
{
	*count = properties.size();
	*ids = new lightspark::ExtIdentifierObject*[properties.size()];
	std::map<NPIdentifierObject, NPVariantObject>::const_iterator it;
	int i = 0;
	for(it = properties.begin(); it != properties.end(); ++it)
	{
		(*ids)[i] = new NPIdentifierObject(it->first);
		i++;
	}

	return true;
}



NPScriptObject::NPScriptObject(NPP _instance) : instance(_instance)
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
	std::map<NPIdentifierObject, lightspark::ExtCallbackFunction>::iterator it;
	it = methods.find(objId);
	if(it == methods.end())
		return false;

	// Convert raw arguments to objects
	const lightspark::ExtVariantObject* objArgs[argc];
	for(uint32_t i = 0; i < argc; i++)
		objArgs[i] = new NPVariantObject(instance, args[i]);

	// Run the function
	lightspark::ExtVariantObject* objResult = NULL;
	bool res = it->second(*this, objId, objArgs, argc, &objResult);

	// Delete converted arguments
	for(uint32_t i = 0; i < argc; i++)
		delete objArgs[i];

	if(objResult != NULL)
	{
		// Copy the result into the raw NPVariant result and delete intermediate result
		NPVariantObject(instance, *objResult).copy(*result);
		delete objResult;
	}

	return res;
}

bool NPScriptObject::invokeDefault(const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObjectGW::invokeDefault");
	return false;
}
bool NPScriptObject::removeMethod(const lightspark::ExtIdentifierObject& id)
{
	std::map<NPIdentifierObject, lightspark::ExtCallbackFunction>::iterator it = methods.find(id);
	if(it == methods.end())
		return false;

	methods.erase(it);
	return true;
}

NPVariantObject* NPScriptObject::getProperty(const lightspark::ExtIdentifierObject& id) const
{
	std::map<NPIdentifierObject, NPVariantObject>::const_iterator it = properties.find(id);
	if(it == properties.end())
		return NULL;

	NPVariantObject* result = new NPVariantObject(instance, it->second);
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

bool NPScriptObject::enumerate(lightspark::ExtIdentifierObject*** ids, uint32_t* count) const
{
	*count = properties.size()+methods.size();
	*ids = new lightspark::ExtIdentifierObject*[properties.size()+methods.size()];
	std::map<NPIdentifierObject, NPVariantObject>::const_iterator prop_it;
	int i = 0;
	for(prop_it = properties.begin(); prop_it != properties.end(); ++prop_it)
	{
		(*ids)[i] = new NPIdentifierObject(prop_it->first);
		i++;
	}
	std::map<NPIdentifierObject, lightspark::ExtCallbackFunction>::const_iterator meth_it;
	for(meth_it = methods.begin(); meth_it != methods.end(); ++meth_it)
	{
		(*ids)[i] = new NPIdentifierObject(meth_it->first);
		i++;
	}

	return true;
}

/* Standard methods */
bool NPScriptObject::stdGetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdGetVariable");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance);
	return false;
}

bool NPScriptObject::stdSetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdSetVariable");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdGotoFrame(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdGotoFrame");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdIsPlaying(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdIsPlaying");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, true);
	return true;
}

bool NPScriptObject::stdLoadMovie(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdLoadMovie");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdPan(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPan");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdPercentLoaded(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPercentLoaded");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, 100);
	return true;
}

bool NPScriptObject::stdPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPlay");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdRewind(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdRewind");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdStopPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdStopPlay");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdSetZoomRect(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdSetZoomRect");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdZoom(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdZoom");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdTotalFrames(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifierObject& id,
			const lightspark::ExtVariantObject** args, uint32_t argc, lightspark::ExtVariantObject** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdTotalFrames");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
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
