/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

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

////////////////////////////////////////////////////////////
//
// Implementation of plugin entry points (NPP_*)
//
#include "pluginbase.h"
#include <stdio.h>

// here the plugin creates a plugin instance object which 
// will be associated with this newly created NPP instance and 
// will do all the necessary job
NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData* saved)
{   
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;

  // create a new plugin instance object
  // initialization will be done when the associated window is ready
  nsPluginCreateData ds;
  
  ds.instance = instance;
  ds.type     = pluginType; 
  ds.mode     = mode; 
  ds.argc     = argc; 
  ds.argn     = argn; 
  ds.argv     = argv; 
  ds.saved    = saved;

  nsPluginInstanceBase * plugin = NS_NewPluginInstance(&ds);
  if(plugin == NULL)
    return NPERR_OUT_OF_MEMORY_ERROR;

  // associate the plugin instance object with NPP instance
  instance->pdata = (void *)plugin;
  return rv;
}

// here is the place to clean up and destroy the nsPluginInstance object
NPError NPP_Destroy (NPP instance, NPSavedData** save)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin != NULL) {
    plugin->shut();
    NS_DestroyPluginInstance(plugin);
  }
  return rv;
}

// during this call we know when the plugin window is ready or
// is about to be destroyed so we can do some gui specific
// initialization and shutdown
NPError NPP_SetWindow (NPP instance, NPWindow* pNPWindow)
{    
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  NPError rv = NPERR_NO_ERROR;

  if(pNPWindow == NULL)
    return NPERR_GENERIC_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;

  if(plugin == NULL) 
    return NPERR_GENERIC_ERROR;

  // window just created
  if(!plugin->isInitialized() && (pNPWindow->window != NULL)) { 
    if(!plugin->init(pNPWindow)) {
      NS_DestroyPluginInstance(plugin);
      return NPERR_MODULE_LOAD_FAILED_ERROR;
    }
  }

  // window goes away
  if((pNPWindow->window == NULL) && plugin->isInitialized())
    return plugin->SetWindow(pNPWindow);

  // window resized?
  if(plugin->isInitialized() && (pNPWindow->window != NULL))
    return plugin->SetWindow(pNPWindow);

  // this should not happen, nothing to do
  if((pNPWindow->window == NULL) && !plugin->isInitialized())
    return plugin->SetWindow(pNPWindow);

  return rv;
}

NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin == NULL) 
    return NPERR_GENERIC_ERROR;

  NPError rv = plugin->NewStream(type, stream, seekable, stype);
  return rv;
}

int32_t NPP_WriteReady (NPP instance, NPStream *stream)
{
  if(instance == NULL)
    return 0x0fffffff;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin == NULL) 
    return 0x0fffffff;

  int32_t rv = plugin->WriteReady(stream);
  return rv;
}

int32_t NPP_Write (NPP instance, NPStream *stream, int32_t offset, int32_t len, void *buffer)
{
  if(instance == NULL)
    return len;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin == NULL) 
    return len;

  int32_t rv = plugin->Write(stream, offset, len, buffer);
  return rv;
}

NPError NPP_DestroyStream (NPP instance, NPStream *stream, NPError reason)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin == NULL) 
    return NPERR_GENERIC_ERROR;

  NPError rv = plugin->DestroyStream(stream, reason);
  return rv;
}

void NPP_StreamAsFile (NPP instance, NPStream* stream, const char* fname)
{
  if(instance == NULL)
    return;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin == NULL) 
    return;

  plugin->StreamAsFile(stream, fname);
}

void NPP_Print (NPP instance, NPPrint* printInfo)
{
  if(instance == NULL)
    return;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin == NULL) 
    return;

  plugin->Print(printInfo);
}

void NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
  if(instance == NULL)
    return;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin == NULL) 
    return;

  plugin->URLNotify(url, reason, notifyData);
}

NPError	NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin == NULL) 
    return NPERR_GENERIC_ERROR;

  NPError rv = plugin->GetValue(variable, value);
  return rv;
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
  if(instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin == NULL) 
    return NPERR_GENERIC_ERROR;

  NPError rv = plugin->SetValue(variable, value);
  return rv;
}

int16_t NPP_HandleEvent(NPP instance, void* event)
{
  if(instance == NULL)
    return 0;

  nsPluginInstanceBase * plugin = (nsPluginInstanceBase *)instance->pdata;
  if(plugin == NULL) 
    return 0;

  uint16_t rv = plugin->HandleEvent(event);
  return rv;
}

#ifdef OJI
jref NPP_GetJavaClass (void)
{
  return NULL;
}
#endif

/**************************************************/
/*                                                */
/*                     Mac                        */
/*                                                */
/**************************************************/

// Mac needs these wrappers, see npplat.h for more info

#ifdef XP_MAC

NPError	Private_New(NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc, char* argn[], char* argv[], NPSavedData* saved)
{
  NPError rv = NPP_New(pluginType, instance, mode, argc, argn, argv, saved);
  return rv;	
}

NPError Private_Destroy(NPP instance, NPSavedData** save)
{
  NPError rv = NPP_Destroy(instance, save);
  return rv;
}

NPError Private_SetWindow(NPP instance, NPWindow* window)
{
  NPError rv = NPP_SetWindow(instance, window);
  return rv;
}

NPError Private_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype)
{
  NPError rv = NPP_NewStream(instance, type, stream, seekable, stype);
  return rv;
}

int32 Private_WriteReady(NPP instance, NPStream* stream)
{
  int32 rv = NPP_WriteReady(instance, stream);
  return rv;
}

int32 Private_Write(NPP instance, NPStream* stream, int32 offset, int32 len, void* buffer)
{
  int32 rv = NPP_Write(instance, stream, offset, len, buffer);
  return rv;
}

void Private_StreamAsFile(NPP instance, NPStream* stream, const char* fname)
{
  NPP_StreamAsFile(instance, stream, fname);
}


NPError Private_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
  NPError rv = NPP_DestroyStream(instance, stream, reason);
  return rv;
}

int16 Private_HandleEvent(NPP instance, void* event)
{
  int16 rv = NPP_HandleEvent(instance, event);
  return rv;
}

void Private_Print(NPP instance, NPPrint* platformPrint)
{
  NPP_Print(instance, platformPrint);
}

void Private_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
  NPP_URLNotify(instance, url, reason, notifyData);
}

jref Private_GetJavaClass(void)
{
  return NULL;
}

NPError Private_GetValue(NPP instance, NPPVariable variable, void *result)
{
  NPError rv = NPP_GetValue(instance, variable, result);
  return rv;
}

NPError Private_SetValue(NPP instance, NPNVariable variable, void *value)
{
  NPError rv = NPP_SetValue(instance, variable, value);
  return rv;
}

#endif //XP_MAC
