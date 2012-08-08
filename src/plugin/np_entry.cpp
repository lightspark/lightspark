/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)
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

//////////////////////////////////////////////////////////////
//
// Main plugin entry point implementation -- exports from the 
// plugin library
//
#include "compat.h"
#include "plugin/include/npplat.h"
#include "plugin/include/pluginbase.h"

NPNetscapeFuncs NPNFuncs;

NPError OSCALL NP_Shutdown()
{
  NS_PluginShutdown();
  return NPERR_NO_ERROR;
}

static NPError fillPluginFunctionTable(NPPluginFuncs* aNPPFuncs)
{
  if(aNPPFuncs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  // Set up the plugin function table that Netscape will use to
  // call us. Netscape needs to know about our version and size   
  // and have a UniversalProcPointer for every function we implement.

  aNPPFuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
#ifdef XP_MAC
  aNPPFuncs->newp          = NewNPP_NewProc(Private_New);
  aNPPFuncs->destroy       = NewNPP_DestroyProc(Private_Destroy);
  aNPPFuncs->setwindow     = NewNPP_SetWindowProc(Private_SetWindow);
  aNPPFuncs->newstream     = NewNPP_NewStreamProc(Private_NewStream);
  aNPPFuncs->destroystream = NewNPP_DestroyStreamProc(Private_DestroyStream);
  aNPPFuncs->asfile        = NewNPP_StreamAsFileProc(Private_StreamAsFile);
  aNPPFuncs->writeready    = NewNPP_WriteReadyProc(Private_WriteReady);
  aNPPFuncs->write         = NewNPP_WriteProc(Private_Write);
  aNPPFuncs->print         = NewNPP_PrintProc(Private_Print);
  aNPPFuncs->event         = NewNPP_HandleEventProc(Private_HandleEvent);	
  aNPPFuncs->urlnotify     = NewNPP_URLNotifyProc(Private_URLNotify);			
  aNPPFuncs->getvalue      = NewNPP_GetValueProc(Private_GetValue);
  aNPPFuncs->setvalue      = NewNPP_SetValueProc(Private_SetValue);
#else
  aNPPFuncs->newp          = NPP_New;
  aNPPFuncs->destroy       = NPP_Destroy;
  aNPPFuncs->setwindow     = NPP_SetWindow;
  aNPPFuncs->newstream     = NPP_NewStream;
  aNPPFuncs->destroystream = NPP_DestroyStream;
  aNPPFuncs->asfile        = NPP_StreamAsFile;
  aNPPFuncs->writeready    = NPP_WriteReady;
  aNPPFuncs->write         = NPP_Write;
  aNPPFuncs->print         = NPP_Print;
  aNPPFuncs->event         = NPP_HandleEvent;
  aNPPFuncs->urlnotify     = NPP_URLNotify;
  aNPPFuncs->getvalue      = NPP_GetValue;
  aNPPFuncs->setvalue      = NPP_SetValue;
#endif
#ifdef OJI
  aNPPFuncs->javaClass     = NULL;
#endif

  return NPERR_NO_ERROR;
}

static NPError fillNetscapeFunctionTable(NPNetscapeFuncs* aNPNFuncs)
{
  if(aNPNFuncs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if(HIBYTE(aNPNFuncs->version) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  if(aNPNFuncs->size < sizeof(NPNetscapeFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  NPNFuncs.size             = aNPNFuncs->size;
  NPNFuncs.version          = aNPNFuncs->version;
  NPNFuncs.geturlnotify     = aNPNFuncs->geturlnotify;
  NPNFuncs.geturl           = aNPNFuncs->geturl;
  NPNFuncs.posturlnotify    = aNPNFuncs->posturlnotify;
  NPNFuncs.posturl          = aNPNFuncs->posturl;
  NPNFuncs.requestread      = aNPNFuncs->requestread;
  NPNFuncs.newstream        = aNPNFuncs->newstream;
  NPNFuncs.write            = aNPNFuncs->write;
  NPNFuncs.destroystream    = aNPNFuncs->destroystream;
  NPNFuncs.status           = aNPNFuncs->status;
  NPNFuncs.uagent           = aNPNFuncs->uagent;
  NPNFuncs.memalloc         = aNPNFuncs->memalloc;
  NPNFuncs.memfree          = aNPNFuncs->memfree;
  NPNFuncs.memflush         = aNPNFuncs->memflush;
  NPNFuncs.reloadplugins    = aNPNFuncs->reloadplugins;
#ifdef OJI
  NPNFuncs.getJavaEnv       = aNPNFuncs->getJavaEnv;
  NPNFuncs.getJavaPeer      = aNPNFuncs->getJavaPeer;
#endif
  NPNFuncs.getvalue         = aNPNFuncs->getvalue;
  NPNFuncs.setvalue         = aNPNFuncs->setvalue;
  NPNFuncs.invalidaterect   = aNPNFuncs->invalidaterect;
  NPNFuncs.invalidateregion = aNPNFuncs->invalidateregion;
  NPNFuncs.forceredraw      = aNPNFuncs->forceredraw;
  NPNFuncs.pluginthreadasynccall = aNPNFuncs->pluginthreadasynccall;
  NPNFuncs.getvalueforurl   = aNPNFuncs->getvalueforurl;

	// NPRuntime
  NPNFuncs.releasevariantvalue = aNPNFuncs->releasevariantvalue;
  NPNFuncs.identifierisstring = aNPNFuncs->identifierisstring;
  NPNFuncs.utf8fromidentifier = aNPNFuncs->utf8fromidentifier;
  NPNFuncs.intfromidentifier = aNPNFuncs->intfromidentifier;
  NPNFuncs.getstringidentifier = aNPNFuncs->getstringidentifier;
  NPNFuncs.getstringidentifiers = aNPNFuncs->getstringidentifiers;
  NPNFuncs.getintidentifier = aNPNFuncs->getintidentifier;
  NPNFuncs.createobject     = aNPNFuncs->createobject;
  NPNFuncs.retainobject     = aNPNFuncs->retainobject;
  NPNFuncs.releaseobject    = aNPNFuncs->releaseobject;
  NPNFuncs.invoke           = aNPNFuncs->invoke;
  NPNFuncs.invokeDefault    = aNPNFuncs->invokeDefault;
  NPNFuncs.enumerate        = aNPNFuncs->enumerate;
  NPNFuncs.evaluate         = aNPNFuncs->evaluate;
  NPNFuncs.getproperty      = aNPNFuncs->getproperty;
  NPNFuncs.setproperty      = aNPNFuncs->setproperty;
  NPNFuncs.removeproperty   = aNPNFuncs->removeproperty;
  NPNFuncs.hasproperty      = aNPNFuncs->hasproperty;
  NPNFuncs.hasmethod        = aNPNFuncs->hasmethod;
  NPNFuncs.setexception     = aNPNFuncs->setexception;

  return NPERR_NO_ERROR;
}

//
// Some exports are different on different platforms
//

/**************************************************/
/*                                                */
/*                   Windows                      */
/*                                                */
/**************************************************/
#ifdef XP_WIN

NPError OSCALL NP_Initialize(NPNetscapeFuncs* aNPNFuncs)
{
  NPError rv = fillNetscapeFunctionTable(aNPNFuncs);
  if(rv != NPERR_NO_ERROR)
    return rv;

  return NS_PluginInitialize();
}

NPError OSCALL NP_GetEntryPoints(NPPluginFuncs* aNPPFuncs)
{
  return fillPluginFunctionTable(aNPPFuncs);
}

#endif //XP_WIN

/**************************************************/
/*                                                */
/*                    Unix                        */
/*                                                */
/**************************************************/
#ifdef XP_UNIX

NPError NP_Initialize(NPNetscapeFuncs* aNPNFuncs, NPPluginFuncs* aNPPFuncs)
{
  NPError rv = fillNetscapeFunctionTable(aNPNFuncs);
  if(rv != NPERR_NO_ERROR)
    return rv;

  rv = fillPluginFunctionTable(aNPPFuncs);
  if(rv != NPERR_NO_ERROR)
    return rv;

  return NS_PluginInitialize();
}

char * NP_GetMIMEDescription(void)
{
  return NPP_GetMIMEDescription();
}

NPError NP_GetValue(void *future, NPPVariable aVariable, void *aValue)
{
  return NS_PluginGetValue(aVariable, aValue);
}

#endif //XP_UNIX

/**************************************************/
/*                                                */
/*                     Mac                        */
/*                                                */
/**************************************************/
#ifdef XP_MAC

#if !TARGET_API_MAC_CARBON
QDGlobals* gQDPtr; // Pointer to Netscape's QuickDraw globals
#endif

short gResFile; // Refnum of the plugin's resource file

NPError Private_Initialize(void)
{
  NPError rv = NS_PluginInitialize();
  return rv;
}

void Private_Shutdown(void)
{
  NS_PluginShutdown();
  __destroy_global_chain();
}

void SetUpQD(void);

void SetUpQD(void)
{
  ProcessSerialNumber PSN;
  FSSpec              myFSSpec;
  Str63               name;
  ProcessInfoRec      infoRec;
  OSErr               result = noErr;
  CFragConnectionID   connID;
  Str255              errName;

  // Memorize the plugin's resource file refnum for later use.
  gResFile = CurResFile();

#if !TARGET_API_MAC_CARBON
  // Ask the system if CFM is available.
  long response;
  OSErr err = Gestalt(gestaltCFMAttr, &response);
  Boolean hasCFM = BitTst(&response, 31-gestaltCFMPresent);

  if (hasCFM) {
    // GetProcessInformation takes a process serial number and 
    // will give us back the name and FSSpec of the application.
    // See the Process Manager in IM.
    infoRec.processInfoLength = sizeof(ProcessInfoRec);
    infoRec.processName = name;
    infoRec.processAppSpec = &myFSSpec;

    PSN.highLongOfPSN = 0;
    PSN.lowLongOfPSN = kCurrentProcess;

    result = GetProcessInformation(&PSN, &infoRec);
  }
	else
    // If no CFM installed, assume it must be a 68K app.
    result = -1;		

  if (result == noErr) {
    // Now that we know the app name and FSSpec, we can call GetDiskFragment
    // to get a connID to use in a subsequent call to FindSymbol (it will also
    // return the address of 'main' in app, which we ignore).  If GetDiskFragment 
    // returns an error, we assume the app must be 68K.
    Ptr mainAddr; 	
    result =  GetDiskFragment(infoRec.processAppSpec, 0L, 0L, infoRec.processName,
                              kReferenceCFrag, &connID, (Ptr*)&mainAddr, errName);
  }

  if (result == noErr) {
    // The app is a PPC code fragment, so call FindSymbol
    // to get the exported 'qd' symbol so we can access its
    // QuickDraw globals.
    CFragSymbolClass symClass;
    result = FindSymbol(connID, "\pqd", (Ptr*)&gQDPtr, &symClass);
  }
  else {
    // The app is 68K, so use its A5 to compute the address
    // of its QuickDraw globals.
    gQDPtr = (QDGlobals*)(*((long*)SetCurrentA5()) - (sizeof(QDGlobals) - sizeof(GrafPtr)));
  }
#endif /* !TARGET_API_MAC_CARBON */
}

NPError main(NPNetscapeFuncs* nsTable, NPPluginFuncs* pluginFuncs, NPP_ShutdownUPP* unloadUpp);

#if !TARGET_API_MAC_CARBON
#pragma export on
#if GENERATINGCFM
RoutineDescriptor mainRD = BUILD_ROUTINE_DESCRIPTOR(uppNPP_MainEntryProcInfo, main);
#endif
#pragma export off
#endif /* !TARGET_API_MAC_CARBON */


NPError main(NPNetscapeFuncs* aNPNFuncs, NPPluginFuncs* aNPPFuncs, NPP_ShutdownUPP* aUnloadUpp)
{
  NPError rv = NPERR_NO_ERROR;

  if (aUnloadUpp == NULL)
    rv = NPERR_INVALID_FUNCTABLE_ERROR;

  if (rv == NPERR_NO_ERROR)
    rv = fillNetscapeFunctionTable(aNPNFuncs);

  if (rv == NPERR_NO_ERROR) {
    // defer static constructors until the global functions are initialized.
    __InitCode__();
    rv = fillPluginFunctionTable(aNPPFuncs);
  }

  *aUnloadUpp = NewNPP_ShutdownProc(Private_Shutdown);
  SetUpQD();
  rv = Private_Initialize();
	
  return rv;
}
#endif //XP_MAC
