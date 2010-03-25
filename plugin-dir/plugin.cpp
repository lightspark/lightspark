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
#define PLUGIN_DESCRIPTION "Shockwave Flash 10.0 - Lightspark implementation"
#include "class.h"

using namespace std;

__thread lightspark::SystemState* sys;
TLSDATA lightspark::RenderThread* rt=NULL;
TLSDATA lightspark::ParseThread* pt=NULL;

NPDownloadManager::NPDownloadManager(NPP i):instance(i)
{
	sem_init(&mutex,0,1);
}

NPDownloadManager::~NPDownloadManager()
{
	sem_destroy(&mutex);
}

lightspark::Downloader* NPDownloadManager::download(const lightspark::tiny_string& u)
{
	sem_wait(&mutex);
	NPDownloader* ret=new NPDownloader(instance, u);
	//Register this download
	pendingLoads.push_back(make_pair(u,ret));
	sem_post(&mutex);
	return ret;
}

void NPDownloadManager::destroy(lightspark::Downloader* d)
{
	sem_wait(&mutex);
	list<pair<lightspark::tiny_string,NPDownloader*> >::iterator it=pendingLoads.begin();
	for(;it!=pendingLoads.end();it++)
	{
		if(it->second==d)
		{
			pendingLoads.erase(it);
			break;
		}
	}
	sem_post(&mutex);

	delete d;
}

NPDownloader* NPDownloadManager::getDownloaderForUrl(const char* u)
{
	sem_wait(&mutex);
	list<pair<lightspark::tiny_string,NPDownloader*> >::iterator it=pendingLoads.begin();
	NPDownloader* ret=NULL;
	for(;it!=pendingLoads.end();it++)
	{
		if(it->first==u)
		{
			ret=it->second;
			pendingLoads.erase(it);
			break;
		}
	}
	sem_post(&mutex);
	return ret;
}

NPDownloader::NPDownloader(NPP i, const lightspark::tiny_string& u):instance(i)
{
	NPN_PluginThreadAsyncCall(instance, dlStartCallback, this);

}

void NPDownloader::dlStartCallback(void* t)
{
	NPDownloader* th=static_cast<NPDownloader*>(t);
	//NPError e=NPN_GetURLNotify(instance, u.raw_buf(), "_blank", this);
	NPError e=NPN_GetURLNotify(th->instance, "http://www.google.com", "_blank", th);
	//NPError e=NPN_GetURL(instance, "http://www.google.com", "_blank");
	if(e!=NPERR_NO_ERROR)
		abort();
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
	Log::initLogging(LOG_NOT_IMPLEMENTED);
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

  nsPluginInstance * plugin = new nsPluginInstance(aCreateDataStruct->instance,
		  					aCreateDataStruct->argc,
		  					aCreateDataStruct->argn,
		  					aCreateDataStruct->argv);
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
nsPluginInstance::nsPluginInstance(NPP aInstance, int16 argc, char** argn, char** argv) : nsPluginInstanceBase(),
	mInstance(aInstance),mInitialized(FALSE),mWindow(0),mXtwidget(0),swf_stream(&swf_buf),
	m_pt(&m_sys,swf_stream),m_it(NULL),m_rt(NULL)
{
	//Find flashvars argument
	for(int i=0;i<argc;i++)
	{
		if(argn[i]==NULL || argv[i]==NULL)
			continue;
		if(strcmp(argn[i],"flashvars")!=0)
			continue;

		lightspark::ASObject* params=new lightspark::ASObject;
		//Add arguments to SystemState
		std::string vars(argv[i]);
		uint32_t cur=0;
		while(cur<vars.size())
		{
			int n1=vars.find('=',cur);
			if(n1==-1) //Incomplete parameters string, ignore the last
				break;

			int n2=vars.find('&',n1+1);
			if(n2==-1)
				n2=vars.size();

			std::string varName=vars.substr(cur,(n1-cur));

			//The variable value has to be urldecoded
			bool ok=true;
			std::string varValue;
			varValue.reserve(n2-n1); //The maximum lenght
			for(int j=n1+1;j<n2;j++)
			{
				if(vars[j]!='%')
					varValue.push_back(vars[j]);
				else
				{
					if((n2-j)<3) //Not enough characters
					{
						ok=false;
						break;
					}

					int t1=hexToInt(vars[j+1]);
					int t2=hexToInt(vars[j+2]);
					if(t1==-1 || t2==-1)
					{
						ok=false;
						break;
					}

					int c=(t1*16)+t2;
					varValue.push_back(c);
					j+=2;
				}
			}

			if(ok)
			{
				cout << varName << ' ' << varValue << endl;
				params->setVariableByQName(varName.c_str(),"",
						lightspark::Class<lightspark::ASString>::getInstanceS(true,varValue)->obj);
			}
			cur=n2+1;
		}
		sys->setParameters(params);
		break;
	}
}

int nsPluginInstance::hexToInt(char c)
{
	if(c>='0' && c<='9')
		return c-'0';
	else if(c>='a' && c<='f')
		return c-'a'+10;
	else if(c>='A' && c<='F')
		return c-'A'+10;
	else
		return -1;
}

nsPluginInstance::~nsPluginInstance()
{
	//Shutdown the system
	m_sys.setShutdownFlag();
	delete m_rt;
	delete m_it;
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
			//cout << "Motion" << endl;
			break;
		default:
			break;
	}
}

void nsPluginInstance::draw()
{
/*	if(m_rt==NULL || m_it==NULL)
		return;
	m_rt->draw();*/
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
	if(mHeight==0 || mHeight==0)
		assert(false);
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

		lightspark::NPAPI_params* p=new lightspark::NPAPI_params;

		p->display=mDisplay;
		p->visual=XVisualIDFromVisual(mVisual);
		p->window=mWindow;
		p->width=mWidth;
		p->height=mHeight;
		lightspark::NPAPI_params* p2=new lightspark::NPAPI_params(*p);
		if(m_rt!=NULL)
		{
			cout << "destroy old context" << endl;
			abort();
		}
		if(p->width==0 || p->height==0)
			abort();
		m_rt=new lightspark::RenderThread(&m_sys,lightspark::NPAPI,p);

		if(m_it!=NULL)
		{
			cout << "destroy old input" << endl;
			abort();
		}
		m_it=new lightspark::InputThread(&m_sys,lightspark::NPAPI,p2);

		sys=NULL;
		m_sys.cur_input_thread=m_it;
		m_sys.cur_render_thread=m_rt;
		m_sys.downloadManager=new NPDownloadManager(mInstance);
		m_sys.fps_prof=new lightspark::fps_profiling();
		m_sys.addJob(&m_pt);

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
	//draw();
	return TRUE;
}

NPError nsPluginInstance::NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype)
{
	//We have to cast the downloadanager to a NPDownloadManager
	NPDownloadManager* manager=static_cast<NPDownloadManager*>(m_sys.downloadManager);
	lightspark::Downloader* dl=manager->getDownloaderForUrl(stream->url);
	if(dl)
		abort();
	//The downloaded is set as the private data for this stream
	stream->pdata=dl;
	*stype=NP_NORMAL;
	return NPERR_NO_ERROR; 
}

int32 nsPluginInstance::WriteReady(NPStream *stream)
{
	if(stream->pdata)
		abort();
	int32 ret=swf_buf.getFree();
	return ret;
}

int32 nsPluginInstance::Write(NPStream *stream, int32 offset, int32 len, void *buffer)
{
	if(stream->pdata)
		abort();
	return swf_buf.write((char*)buffer,len);
}

NPError nsPluginInstance::DestroyStream(NPStream *stream, NPError reason)
{
	if(stream->pdata)
		abort();
	return NPERR_NO_ERROR;
}

void nsPluginInstance::URLNotify(const char* url, NPReason reason, void* notifyData)
{
	cerr << url << endl;
	cerr << reason << endl;
	cerr << notifyData << endl;
}
