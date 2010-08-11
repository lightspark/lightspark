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
// Implementation of Netscape entry points (NPN_*)
//
#include "npplat.h"
#include <stdio.h>
#include <stdlib.h>

extern NPNetscapeFuncs NPNFuncs;

void NPN_Version(int* plugin_major, int* plugin_minor, int* netscape_major, int* netscape_minor)
{
  *plugin_major   = NP_VERSION_MAJOR;
  *plugin_minor   = NP_VERSION_MINOR;
  *netscape_major = HIBYTE(NPNFuncs.version);
  *netscape_minor = LOBYTE(NPNFuncs.version);
}

//TODO: understand the Call... wrappers and npupp.h
void NPN_PluginThreadAsyncCall(NPP instance, void (*func) (void *), void *userData)
{
  NPNFuncs.pluginthreadasynccall(instance, func, userData);
}

NPError NPN_GetValueForURL(NPP instance, NPNURLVariable variable, const char *url, char **value, uint32_t *len)
{
  return NPNFuncs.getvalueforurl(instance, variable, url, value, len);
}

NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name)
{
  return NPNFuncs.getstringidentifier(name);
}

void NPN_ReleaseVariantValue(NPVariant *variant)
{
  return NPNFuncs.releasevariantvalue(variant);
}

bool NPN_GetProperty(NPP npp, NPObject *npobj, NPIdentifier propertyName, NPVariant *result)
{
  return NPNFuncs.getproperty(npp, npobj, propertyName, result);
}

void NPN_ReleaseObject(NPObject *npobj)
{
  NPNFuncs.releaseobject(npobj);
}

NPError NPN_GetURLNotify(NPP instance, const char *url, const char *target, void* notifyData)
{
	int navMinorVers = NPNFuncs.version & 0xFF;
  NPError rv = NPERR_NO_ERROR;

  if( navMinorVers >= NPVERS_HAS_NOTIFICATION )
		rv = CallNPN_GetURLNotifyProc(NPNFuncs.geturlnotify, instance, url, target, notifyData);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

NPError NPN_GetURL(NPP instance, const char *url, const char *target)
{
  NPError rv = CallNPN_GetURLProc(NPNFuncs.geturl, instance, url, target);
  if(rv!=0)
	  abort();
  return rv;
}

NPError NPN_PostURLNotify(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file, void* notifyData)
{
	int navMinorVers = NPNFuncs.version & 0xFF;
  NPError rv = NPERR_NO_ERROR;

	if( navMinorVers >= NPVERS_HAS_NOTIFICATION )
		rv = CallNPN_PostURLNotifyProc(NPNFuncs.posturlnotify, instance, url, window, len, buf, file, notifyData);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

NPError NPN_PostURL(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file)
{
  NPError rv = CallNPN_PostURLProc(NPNFuncs.posturl, instance, url, window, len, buf, file);
  return rv;
} 

NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
  NPError rv = CallNPN_RequestReadProc(NPNFuncs.requestread, stream, rangeList);
  return rv;
}

NPError NPN_NewStream(NPP instance, NPMIMEType type, const char* target, NPStream** stream)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;

  NPError rv = NPERR_NO_ERROR;

	if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT )
		rv = CallNPN_NewStreamProc(NPNFuncs.newstream, instance, type, target, stream);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

int32_t NPN_Write(NPP instance, NPStream *stream, int32_t len, void *buffer)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;
  int32_t rv = 0;

  if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT )
		rv = CallNPN_WriteProc(NPNFuncs.write, instance, stream, len, buffer);
	else
		rv = -1;

  return rv;
}

NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
	int navMinorVersion = NPNFuncs.version & 0xFF;
  NPError rv = NPERR_NO_ERROR;

  if( navMinorVersion >= NPVERS_HAS_STREAMOUTPUT )
		rv = CallNPN_DestroyStreamProc(NPNFuncs.destroystream, instance, stream, reason);
	else
		rv = NPERR_INCOMPATIBLE_VERSION_ERROR;

  return rv;
}

void NPN_Status(NPP instance, const char *message)
{
  CallNPN_StatusProc(NPNFuncs.status, instance, message);
}

const char* NPN_UserAgent(NPP instance)
{
  const char * rv = NULL;
  rv = CallNPN_UserAgentProc(NPNFuncs.uagent, instance);
  return rv;
}

void* NPN_MemAlloc(uint32_t size)
{
  void * rv = NULL;
  rv = CallNPN_MemAllocProc(NPNFuncs.memalloc, size);
  return rv;
}

void NPN_MemFree(void* ptr)
{
  CallNPN_MemFreeProc(NPNFuncs.memfree, ptr);
}

uint32_t NPN_MemFlush(uint32_t size)
{
  uint32_t rv = CallNPN_MemFlushProc(NPNFuncs.memflush, size);
  return rv;
}

void NPN_ReloadPlugins(NPBool reloadPages)
{
  CallNPN_ReloadPluginsProc(NPNFuncs.reloadplugins, reloadPages);
}

#ifdef OJI
JRIEnv* NPN_GetJavaEnv(void)
{
  JRIEnv * rv = NULL;
  rv = (JRIEnv*)CallNPN_GetJavaEnvProc(NPNFuncs.getJavaEnv);
  return rv;
}

jref NPN_GetJavaPeer(NPP instance)
{
  jref rv;
  rv = (jref)CallNPN_GetJavaPeerProc(NPNFuncs.getJavaPeer, instance);
  return rv;
}
#endif

NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value)
{
  NPError rv = CallNPN_GetValueProc(NPNFuncs.getvalue, instance, variable, value);
  return rv;
}

NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value)
{
  NPError rv = CallNPN_SetValueProc(NPNFuncs.setvalue, instance, variable, value);
  return rv;
}

void NPN_InvalidateRect(NPP instance, NPRect *invalidRect)
{
  CallNPN_InvalidateRectProc(NPNFuncs.invalidaterect, instance, invalidRect);
}

void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
{
  CallNPN_InvalidateRegionProc(NPNFuncs.invalidateregion, instance, invalidRegion);
}

void NPN_ForceRedraw(NPP instance)
{
  CallNPN_ForceRedrawProc(NPNFuncs.forceredraw, instance);
}
