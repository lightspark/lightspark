/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "plugin.h"
#include "logger.h"
//#define MIME_TYPES_HANDLED  "application/x-shockwave-flash"
#define MIME_TYPES_HANDLED  "application/x-lightspark"
#define PLUGIN_NAME    "Shockwave Flash"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED":swf:"PLUGIN_NAME
#define PLUGIN_DESCRIPTION "Shockwave Flash 9.0 r31"

using namespace std;

__thread SystemState* sys;

MovieTimer::MovieTimer(SystemState* s,RenderThread* r):rt(r)
{
	m_sys=s;
	sem_init(&mutex,0,1);
	pthread_create(&t,0,(thread_worker)timer_worker,this);
}

MovieTimer::~MovieTimer()
{
	void* ret;
	pthread_cancel(t);
	pthread_join(t,&ret);
}

void MovieTimer::setRenderThread(RenderThread* r)
{
	sem_wait(&mutex);

	rt=r;

	sem_post(&mutex);
}

void* MovieTimer::timer_worker(MovieTimer* th)
{
	sys=th->m_sys;
	while(1)
	{
		sys->waitToRun();
		sem_wait(&th->mutex);
		if(th->rt!=NULL)
			th->rt->draw(&sys->getFrameAtFP());
		else
			throw "rt null";
		sem_post(&th->mutex);
		//sys->advanceFP();
	}
}

char* NPP_GetMIMEDescription(void)
{
	return (char*)(MIME_TYPES_DESCRIPTION);
}

/////////////////////////////////////
// general initialization and shutdown
//
NPError NS_PluginInitialize()
{
	Log::initLogging(NOT_IMPLEMENTED);
	return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
}

// get values per plugin
NPError NS_PluginGetValue(NPPVariable aVariable, void *aValue)
{
	NPError err = NPERR_NO_ERROR;
	switch (aVariable)
	{
		case NPPVpluginNameString:
			*((char **)aValue) = (char*)PLUGIN_NAME;
			break;
		case NPPVpluginDescriptionString:
			*((char **)aValue) = (char*)PLUGIN_DESCRIPTION;
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
	mInstance(aInstance),mInitialized(FALSE),mWindow(0),mXtwidget(0),swf_stream(&swf_buf),
	pt(&m_sys,swf_stream),rt(NULL),mt(&m_sys,NULL),it(NULL)
{
}

nsPluginInstance::~nsPluginInstance()
{
	delete rt;
	delete it;
}

void xt_event_handler(Widget xtwidget, nsPluginInstance *plugin, XEvent *xevent, Boolean *b)
{
	switch (xevent->type)
	{
		case Expose:
			// get rid of all other exposure events
			if (plugin)
			{
				//while(XCheckTypedWindowEvent(plugin->Display(), plugin->Window(), Expose, xevent));
				plugin->draw();
			}
		case MotionNotify:
			cout << "Motion" << endl;
			break;
		default:
			break;
	}
}

void nsPluginInstance::draw()
{
	if(rt==NULL)
		return;
	rt->draw(NULL);
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
	if (mWindow == (Window) aWindow->window)
	{
		// The page with the plugin is being resized.
		// Save any UI information because the next time
		// around expect a SetWindow with a new window id.
	}
	else
	{
		mWindow = (Window) aWindow->window;
		NPSetWindowCallbackStruct *ws_info = (NPSetWindowCallbackStruct *)aWindow->ws_info;
		mDisplay = ws_info->display;
		mVisual = ws_info->visual;
		mDepth = ws_info->depth;
		mColormap = ws_info->colormap;

		printf("Window visual 0x%x\n",XVisualIDFromVisual(mVisual));

		NPAPI_params* p=new NPAPI_params;

		p->display=mDisplay;
		p->visual=XVisualIDFromVisual(mVisual);
		p->window=mWindow;
		p->width=mWidth;
		p->height=mHeight;
		NPAPI_params* p2=new NPAPI_params(*p);
		if(rt!=NULL)
		{
			cout << "destroy old context" << endl;
			abort();
		}
		rt=new RenderThread(&m_sys,NPAPI,p);
		mt.setRenderThread(rt);

		if(it!=NULL)
		{
			cout << "destroy old input" << endl;
			abort();
		}
		it=new InputThread(&m_sys,NPAPI,p2);
		

		// add xt event handler
		Widget xtwidget = XtWindowToWidget(mDisplay, mWindow);
		if (xtwidget && mXtwidget != xtwidget)
		{
			mXtwidget = xtwidget;
			long event_mask = ExposureMask|PointerMotionMask;
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

int32 nsPluginInstance::Write(NPStream *stream, int32 offset, int32 len, void *buffer)
{
	return swf_buf.sputn((char*)buffer,len);
}
