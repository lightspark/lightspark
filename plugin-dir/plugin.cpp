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
#define MIME_TYPES_HANDLED  "application/x-shockwave-flash"
//#define MIME_TYPES_HANDLED  "application/x-lightspark"
#define PLUGIN_NAME    "Shockwave Flash"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED":swf:"PLUGIN_NAME
#define PLUGIN_DESCRIPTION "Shockwave Flash 10.0 r04_Lightspark"
#include "class.h"
#include <gtk/gtk.h>

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
	if(!pendingLoads.empty())
		abort();
	sem_destroy(&mutex);
}

lightspark::Downloader* NPDownloadManager::download(const lightspark::tiny_string& u)
{
	sem_wait(&mutex);
	//TODO: Url encode the string
	std::string tmp2;
	tmp2.reserve(u.len()*2);
	for(int i=0;i<u.len();i++)
	{
		if(u[i]==' ')
		{
			char buf[4];
			sprintf(buf,"%%%x",(unsigned char)u[i]);
			tmp2+=buf;
		}
		else
			tmp2.push_back(u[i]);
	}
	lightspark::tiny_string url=tmp2.c_str();
	//Register this download
	NPDownloader* ret=new NPDownloader(instance, url);
	pendingLoads.push_back(make_pair(url,ret));
	sem_post(&mutex);
	return ret;
}

void NPDownloadManager::destroy(lightspark::Downloader* d)
{
	//First of all, wait for termination
	if(!sys->isShuttingDown())
		d->wait();
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
		cout << it->first << endl;
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

NPDownloader::NPDownloader(NPP i, const lightspark::tiny_string& u):instance(i),url(u),started(false)
{
	NPN_PluginThreadAsyncCall(instance, dlStartCallback, this);
}

void NPDownloader::terminate()
{
	sem_post(&terminated);
}

void NPDownloader::dlStartCallback(void* t)
{
	NPDownloader* th=static_cast<NPDownloader*>(t);
	cerr << "Start download for " << th->url << endl;
	th->started=true;
	NPError e=NPN_GetURLNotify(th->instance, th->url.raw_buf(), NULL, th);
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
	Log::initLogging(LOG_ERROR);
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
/*		case NPPVpluginNeedsXEmbed:
			*((bool *)aValue) = true;
			break;*/
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
  sys=NULL;
}

////////////////////////////////////////
//
// nsPluginInstance class implementation
//
nsPluginInstance::nsPluginInstance(NPP aInstance, int16_t argc, char** argn, char** argv) : nsPluginInstanceBase(),
	mInstance(aInstance),mInitialized(FALSE),mWindow(0),swf_stream(&swf_buf),m_it(NULL),m_rt(NULL)
{
	m_sys=new lightspark::SystemState;
	m_pt=new lightspark::ParseThread(m_sys,swf_stream);
	//Find flashvars argument
	for(int i=0;i<argc;i++)
	{
		if(argn[i]==NULL || argv[i]==NULL)
			continue;
		if(strcasecmp(argn[i],"flashvars")==0)
		{
			lightspark::ASObject* params=lightspark::Class<lightspark::ASObject>::getInstanceS();
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
					//cout << varName << ' ' << varValue << endl;
					params->setVariableByQName(varName.c_str(),"",
							lightspark::Class<lightspark::ASString>::getInstanceS(varValue));
				}
				cur=n2+1;
			}
			m_sys->setParameters(params);
		}
		else if(strcasecmp(argn[i],"src")==0)
		{
			m_sys->setOrigin(argv[i]);
		}
	}
	m_sys->downloadManager=new NPDownloadManager(mInstance);
	m_sys->addJob(m_pt);
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
	sys=m_sys;
	//cerr << "instance dying" << endl;
	swf_buf.destroy();
	m_pt->stop();
	m_sys->setShutdownFlag();
	m_sys->wait();
	if(m_rt)
		m_rt->wait();
	if(m_it)
		m_it->wait();
	delete m_sys;
	delete m_pt;
	delete m_rt;
	delete m_it;
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
    case NPPVpluginNeedsXEmbed:
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
	{
		LOG(LOG_ERROR,"No size in SetWindow");
		return FALSE;
	}
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
		//p->container=gtk_plug_new((GdkNativeWindow)p->window);
		lightspark::NPAPI_params* p2=new lightspark::NPAPI_params(*p);
		if(m_rt!=NULL)
		{
			cout << "destroy old context" << endl;
			abort();
		}
		if(p->width==0 || p->height==0)
			abort();

		m_rt=new lightspark::RenderThread(m_sys,lightspark::NPAPI,p);

		if(m_it!=NULL)
		{
			cout << "destroy old input" << endl;
			abort();
		}
		m_it=new lightspark::InputThread(m_sys,lightspark::NPAPI,p2);

		sys=NULL;
		m_sys->inputThread=m_it;
		m_sys->renderThread=m_rt;
	}
	//draw();
	return TRUE;
}

NPError nsPluginInstance::NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
{
	//We have to cast the downloadanager to a NPDownloadManager
	NPDownloadManager* manager=static_cast<NPDownloadManager*>(m_sys->downloadManager);
	lightspark::Downloader* dl=manager->getDownloaderForUrl(stream->url);
	LOG(LOG_NO_INFO,"Newstream for " << stream->url);
	//cerr << stream->headers << endl;
	if(dl)
	{
		cerr << "via NPDownloader" << endl;
		dl->setLen(stream->end);
	}
	//The downloader is set as the private data for this stream
	stream->pdata=dl;
	*stype=NP_NORMAL;
	return NPERR_NO_ERROR; 
}

int32_t nsPluginInstance::WriteReady(NPStream *stream)
{
	if(stream->pdata) //This is a Downloader based download
		return 1024*1024;
	else
		return swf_buf.getFree();
}

int32_t nsPluginInstance::Write(NPStream *stream, int32_t offset, int32_t len, void *buffer)
{
	if(stream->pdata)
	{
		lightspark::Downloader* dl=static_cast<lightspark::Downloader*>(stream->pdata);
		if(dl->hasFailed())
			return -1;
		dl->append((uint8_t*)buffer,len);
		return len;
	}
	else
		return swf_buf.write((char*)buffer,len);
}

NPError nsPluginInstance::DestroyStream(NPStream *stream, NPError reason)
{
	if(stream->pdata)
	{
		cerr << "Destroy " << stream->pdata << endl;
		NPDownloader* dl=static_cast<NPDownloader*>(stream->pdata);
		dl->terminate();
	}
	else
		LOG(LOG_NO_INFO, "DestroyStream on main stream?");
	return NPERR_NO_ERROR;
}

void nsPluginInstance::URLNotify(const char* url, NPReason reason, void* notifyData)
{
	cerr << "URLnotify" << notifyData << endl;
	cerr << url << endl;
	cerr << reason << endl;
	cerr << notifyData << endl;
/*	if(reason!=NPRES_USER_BREAK)
	{
		cerr << url << endl;
		cerr << reason << endl;
		cerr << notifyData << endl;
		abort();
	}*/
}
