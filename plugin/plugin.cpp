/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "plugin.h"
#define MIME_TYPES_HANDLED  "application/x-lightspark"
#define PLUGIN_NAME         "Lightspark Netscape plugin"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED":sw2:"PLUGIN_NAME
#define PLUGIN_DESCRIPTION  PLUGIN_NAME " Lightspark Netscape plugin"

using namespace std;

char* NPP_GetMIMEDescription(void)
{
  return(MIME_TYPES_DESCRIPTION);
}

/////////////////////////////////////
// general initialization and shutdown
//
NPError NS_PluginInitialize()
{
  return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}

// get values per plugin
NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue)
{
  NPError err = NPERR_NO_ERROR;
  switch (aVariable) {
    case NPPVpluginNameString:
      *((char **)aValue) = PLUGIN_NAME;
      break;
    case NPPVpluginDescriptionString:
      *((char **)aValue) = PLUGIN_DESCRIPTION;
      break;
    default:
      err = NPERR_INVALID_PARAM;
      break;
  }
  return err;
}

/////////////////////////////////////////////////////////////
//
// construction and destruction of our plugin instance object
//
nsPluginInstanceBase * NS_NewPluginInstance(nsPluginCreateData * aCreateDataStruct)
{
  if(!aCreateDataStruct)
    return NULL;

  nsPluginInstance * plugin = new nsPluginInstance(aCreateDataStruct->instance);
  return plugin;
}

void NS_DestroyPluginInstance(nsPluginInstanceBase * aPlugin)
{
  if(aPlugin)
    delete (nsPluginInstance *)aPlugin;
}

////////////////////////////////////////
//
// nsPluginInstance class implementation
//
nsPluginInstance::nsPluginInstance(NPP aInstance) : nsPluginInstanceBase(),
  mInstance(aInstance),
  mInitialized(FALSE),
  mWindow(0),
  mXtwidget(0),
  mFontInfo(0),
  mFBConfig(0),
  mContext(0),
  swf_stream(&swf_buf),
  pt(swf_stream)
{
}

nsPluginInstance::~nsPluginInstance()
{
}

static void
xt_event_handler(Widget xtwidget, nsPluginInstance *plugin, XEvent *xevent, Boolean *b)
{
  switch (xevent->type) {
    case Expose:
      // get rid of all other exposure events
      if (plugin) {
        //while(XCheckTypedWindowEvent(plugin->Display(), plugin->Window(), Expose, xevent));
        plugin->draw();
      }
      default:
        break;
  }
}

extern SystemState sys;

void nsPluginInstance::draw()
{
	glViewport(0,0,640,480);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,640,480,0,-100,0);
	glMatrixMode(GL_MODELVIEW);
/*	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0,1,0);
	glBegin(GL_QUADS);
		glVertex2i(0,50);
		glVertex2i(0,250);
		glVertex2i(250,250);
		glVertex2i(250,50);
	glEnd();
	glFlush();*/
			cout << "FP: " << sys.clip.state.FP << endl;
			if(sys.clip.state.FP>=sys.clip.frames.size())
			{
				cout << "not yet" << endl;
				return;
			}
			else
				cout << "render" << endl;
			//Aquired lock on frames list

			sys.update_request=false;
			sys.clip.state.next_FP=sys.clip.state.FP+1;
			glClearColor(sys.Background.Red/255.0F,sys.Background.Green/255.0F,sys.Background.Blue/255.0F,0);
			glClearDepth(0xffff);
			glClearStencil(5);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
			glLoadIdentity();

			glScalef(0.1,0.1,1);

			//if(sys.clip.state.FP>=43)
			//	sys.clip.frames[sys.clip.state.FP].hack=1;
			sys.clip.frames[sys.clip.state.FP].Render(0);

			sys.clip.state.FP=sys.clip.state.next_FP;

			//thread_debug( "RENDER: frames mutex unlock");
			sem_post(&sys.clip.sem_frames);
			cout << "Frame " << sys.clip.state.FP << endl;
}

NPBool nsPluginInstance::init(NPWindow* aWindow)
{
  if(aWindow == NULL)
    return FALSE;
  
  if (SetWindow(aWindow))
  mInitialized = TRUE;
	
  return mInitialized;
}

void nsPluginInstance::shut()
{
  mInitialized = FALSE;
  if(mContext)
	  glXDestroyContext(mDisplay,mContext);
}

const char * nsPluginInstance::getVersion()
{
  return NPN_UserAgent(mInstance);
}

NPError nsPluginInstance::GetValue(NPPVariable aVariable, void *aValue)
{
  NPError err = NPERR_NO_ERROR;
  switch (aVariable) {
    case NPPVpluginNameString:
    case NPPVpluginDescriptionString:
      return NS_PluginGetValue(aVariable, aValue) ;
      break;
    default:
      err = NPERR_INVALID_PARAM;
      break;
  }
  return err;

}

NPError nsPluginInstance::SetWindow(NPWindow* aWindow)
{
  if(aWindow == NULL)
    return FALSE;

  mX = aWindow->x;
  mY = aWindow->y;
  mWidth = aWindow->width;
  mHeight = aWindow->height;
  if (mWindow == (Window) aWindow->window) {
    // The page with the plugin is being resized.
    // Save any UI information because the next time
    // around expect a SetWindow with a new window id.
  }
  else
  {
    if(mContext)
 	  glXDestroyContext(mDisplay,mContext);

    mWindow = (Window) aWindow->window;
    NPSetWindowCallbackStruct *ws_info = (NPSetWindowCallbackStruct *)aWindow->ws_info;
    mDisplay = ws_info->display;
    mVisual = ws_info->visual;
    mDepth = ws_info->depth;
    mColormap = ws_info->colormap;

    printf("Window visual 0x%x\n",XVisualIDFromVisual(mVisual));

    if (!mFontInfo) {
      if (!(mFontInfo = XLoadQueryFont(mDisplay, "9x15")))
        printf("Cannot open 9X15 font\n");
    }
    	int a,b;
    	Bool glx_present=glXQueryVersion(mDisplay,&a,&b);
	if(!glx_present)
	{
		printf("glX not present\n");
		return FALSE;
	}
	int attrib[10];
	attrib[0]=GLX_BUFFER_SIZE;
	attrib[1]=24;
	attrib[2]=GLX_VISUAL_ID;
	attrib[3]=XVisualIDFromVisual(mVisual);
	attrib[4]=GLX_DEPTH_SIZE;
	attrib[5]=24;
	attrib[6]=GLX_STENCIL_SIZE;
	attrib[7]=8;

	attrib[8]=None;
	GLXFBConfig* fb=glXChooseFBConfig(mDisplay, 0, attrib, &a);
	printf("returned %x pointer and %u elements\n",fb, a);
	int i;
	for(i=0;i<a;i++)
	{
		int id,v;
		glXGetFBConfigAttrib(mDisplay,fb[i],GLX_BUFFER_SIZE,&v);
		glXGetFBConfigAttrib(mDisplay,fb[i],GLX_VISUAL_ID,&id);
		printf("ID 0x%x size %u\n",id,v);
		if(id==XVisualIDFromVisual(mVisual))
			break;
	}
	mFBConfig=fb[i];
	XFree(fb);

	mContext = glXCreateNewContext(mDisplay,mFBConfig,GLX_RGBA_TYPE ,NULL,1);
	glXMakeContextCurrent(mDisplay, mWindow, mWindow, mContext);
	if(!glXIsDirect(mDisplay,mContext))
		printf("Indirect!!\n");
	/*GLXWindow glx_window=glXCreateWindow(mDisplay,fb[i],mWindow,NULL);
	printf("glx window ID %x\n",glx_window);
	char buffer[100];
	XGetErrorText(mDisplay,glx_window,buffer,100);
	glXDestroyWindow(mDisplay,glx_window);
	printf("error %s\n",buffer);*/

    // add xt event handler
    Widget xtwidget = XtWindowToWidget(mDisplay, mWindow);
    if (xtwidget && mXtwidget != xtwidget) {
      mXtwidget = xtwidget;
      long event_mask = ExposureMask;
      XSelectInput(mDisplay, mWindow, event_mask);
      XtAddEventHandler(xtwidget, event_mask, False, (XtEventHandler)xt_event_handler, this);
    }
  }
  draw();
  return TRUE;
}

NPError nsPluginInstance::NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype)
{
	*stype=NP_NORMAL;
	return NPERR_NO_ERROR; 
}

SystemState sys;

int32 nsPluginInstance::Write(NPStream *stream, int32 offset, int32 len, void *buffer)
{
	return swf_buf.sputn((char*)buffer,len);
}
