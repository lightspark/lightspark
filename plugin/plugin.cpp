/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "plugin.h"
#include "logger.h"
#include "compat.h"
#include <string>
#include <algorithm>
#include "backends/urlutils.h"
#define MIME_TYPES_HANDLED  "application/x-shockwave-flash"
#define FAKE_MIME_TYPE  "application/x-lightspark"
#define PLUGIN_NAME    "Shockwave Flash"
#define FAKE_PLUGIN_NAME    "Lightspark player"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED":swf:"PLUGIN_NAME";"FAKE_MIME_TYPE":swfls:"FAKE_PLUGIN_NAME
#define PLUGIN_DESCRIPTION "Shockwave Flash 10.0 r443"

using namespace std;

TLSDATA DLL_PUBLIC lightspark::SystemState* sys=NULL;
TLSDATA DLL_PUBLIC lightspark::RenderThread* rt=NULL;
TLSDATA DLL_PUBLIC lightspark::ParseThread* pt=NULL;

/**
 * \brief Constructor for NPDownloadManager
 *
 * The NPAPI download manager produces \c NPDownloader-type \c Downloaders.
 * It should only be used in the plugin version of LS.
 * \param[in] _instance An \c NPP object representing the plugin
 */
NPDownloadManager::NPDownloadManager(NPP _instance):instance(_instance)
{
	type = NPAPI;
}

/**
 * \brief Destructor for NPDownloaderManager
 */
NPDownloadManager::~NPDownloadManager()
{
}

/**
 * \brief Create a Downloader for an URL.
 *
 * Returns a pointer to a newly created \c Downloader for the given URL.
 * \param[in] url The URL (as a \c tiny_string) the \c Downloader is requested for
 * \param[in] cached Whether or not to disk-cache the download (default=false)
 * \return A pointer to a newly created \c Downloader for the given URL.
 * \see DownloadManager::destroy()
 */
lightspark::Downloader* NPDownloadManager::download(const lightspark::tiny_string& url, bool cached)
{
	return download(sys->getOrigin().goToURL(url), cached);
}

/**
 * \brief Create a Downloader for an URL.
 *
 * Returns a pointer to a newly created Downloader for the given URL.
 * \param[in] url The URL (as a \c URLInfo) the \c Downloader is requested for
 * \param[in] cached Whether or not to disk-cache the download (default=false)
 * \return A pointer to a newly created \c Downloader for the given URL.
 * \see DownloadManager::destroy()
 */
lightspark::Downloader* NPDownloadManager::download(const lightspark::URLInfo& url, bool cached)
{
	LOG(LOG_NO_INFO, _("NET: PLUGIN: DownloadManager::download '") << url.getParsedURL() << 
			"'" << (cached ? _(" - cached") : ""));
	//Register this download
	NPDownloader* downloader=new NPDownloader(url.getParsedURL(), cached, instance);
	addDownloader(downloader);
	return downloader;
}

/**
 * \brief Constructor for the NPDownloader class
 *
 * \param[in] _url The URL for the Downloader.
 * \param[in] _cached Whether or not to cache this download.
 */
NPDownloader::NPDownloader(const lightspark::tiny_string& _url, bool _cached, NPP _instance):
	Downloader(_url, _cached),instance(_instance),started(false)
{
	NPN_PluginThreadAsyncCall(instance, dlStartCallback, this);
}

/**
 * \brief Callback to start the download.
 *
 * Gets called by NPN_PluginThreadAsyncCall to start the download in a new thread.
 * \param t \c this pointer to the \c NPDownloader object
 */
void NPDownloader::dlStartCallback(void* t)
{
	NPDownloader* th=static_cast<NPDownloader*>(t);
	cerr << _("Start download for ") << th->url << endl;
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
	lightspark::SystemState::staticInit();
	return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
	lightspark::SystemState::staticDeinit();
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
		case NPPVpluginNeedsXEmbed:
			*((bool *)aValue) = true;
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
	sys=NULL;
}

////////////////////////////////////////
//
// nsPluginInstance class implementation
//
nsPluginInstance::nsPluginInstance(NPP aInstance, int16_t argc, char** argn, char** argv) : nsPluginInstanceBase(),
	mInstance(aInstance),mInitialized(FALSE),mContainer(NULL),mWindow(0),swf_stream(&swf_buf)
{
	sys=NULL;
	m_pt=new lightspark::ParseThread(NULL,swf_stream);
	m_sys=new lightspark::SystemState(m_pt);
	//As this is the plugin, enable fallback on Gnash for older clips
	m_sys->enableGnashFallback();
	//Files running in the plugin have REMOTE sandbox
	m_sys->securityManager->setSandboxType(lightspark::SecurityManager::REMOTE);
	//Find flashvars argument
	for(int i=0;i<argc;i++)
	{
		if(argn[i]==NULL || argv[i]==NULL)
			continue;
		if(strcasecmp(argn[i],"flashvars")==0)
		{
			m_sys->parseParametersFromFlashvars(argv[i]);
		}
		//The SWF file url should be getted from NewStream
	}
	m_sys->downloadManager=new NPDownloadManager(mInstance);
	m_sys->addJob(m_pt);
	//The sys var should be NULL in this thread
	sys=NULL;
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
	delete m_sys;
	delete m_pt;
	sys=NULL;
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
  switch (aVariable)
	{
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

void nsPluginInstance::AsyncHelper(void* th_void, helper_t func, void* privArg)
{
	nsPluginInstance* th=(nsPluginInstance*)th_void;
	NPN_PluginThreadAsyncCall(th->mInstance, func, privArg);
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
		LOG(LOG_ERROR,_("No size in SetWindow"));
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

		lightspark::NPAPI_params p;

		p.visual=XVisualIDFromVisual(mVisual);
		p.container=NULL;
		p.display=mDisplay;
		p.window=mWindow;
		p.width=mWidth;
		p.height=mHeight;
		p.helper=AsyncHelper;
		p.helperArg=this;
		cout << "X Window " << hex << p.window << dec << endl;
		if(m_sys->getRenderThread()!=NULL)
		{
			cout << "destroy old context" << endl;
			abort();
		}
		if(m_sys->getInputThread()!=NULL)
		{
			cout << "destroy old input" << endl;
			abort();
		}
		m_sys->setParamsAndEngine(lightspark::GTKPLUG,&p);
	}
	//draw();
	return TRUE;
}

string nsPluginInstance::getPageURL() const
{
	//Copied from https://developer.mozilla.org/en/Getting_the_page_URL_in_NPAPI_plugin 
	// Get the window object.
	NPObject* sWindowObj;
	NPN_GetValue(mInstance, NPNVWindowNPObject, &sWindowObj);
	// Declare a local variant value.
	NPVariant variantValue;
	// Create a "location" identifier.
	NPIdentifier identifier = NPN_GetStringIdentifier( "location" );
	// Get the location property from the window object (which is another object).
	bool b1 = NPN_GetProperty(mInstance, sWindowObj, identifier, &variantValue);
	NPN_ReleaseObject(sWindowObj);
	if(!b1)
		return "";
	if(!NPVARIANT_IS_OBJECT(variantValue))
	{
		NPN_ReleaseVariantValue(&variantValue);
		return "";
	}
	// Get a pointer to the "location" object.
	NPObject *locationObj = variantValue.value.objectValue;
	// Create a "href" identifier.
	identifier = NPN_GetStringIdentifier( "href" );
	// Get the href property from the location object.
	b1 = NPN_GetProperty(mInstance, locationObj, identifier, &variantValue);
	NPN_ReleaseObject(locationObj);
	if(!b1)
		return "";
	if(!NPVARIANT_IS_STRING(variantValue))
	{
		NPN_ReleaseVariantValue(&variantValue);
		return "";
	}
	NPString url=NPVARIANT_TO_STRING(variantValue);
	//TODO: really handle UTF8 url!!
	for(unsigned int i=0;i<url.UTF8Length;i++)
	{
		if(url.UTF8Characters[i]&0x80)
		{
			LOG(LOG_ERROR,_("Cannot handle UTF8 URLs"));
			return "";
		}
	}
	string ret(url.UTF8Characters, url.UTF8Length);
	NPN_ReleaseVariantValue(&variantValue);
	return ret;
}

NPError nsPluginInstance::NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
{
	NPDownloader* dl=static_cast<NPDownloader*>(stream->notifyData);
	LOG(LOG_NO_INFO,_("Newstream for ") << stream->url);

	if(dl)
	{
		cerr << "via NPDownloader" << endl;
		dl->setLength(stream->end);
		//TODO: confirm that growing buffers are normal. This does fix a bug I found though. (timonvo)
		*stype=NP_NORMAL;

		if(strcmp(stream->url, dl->getURL().raw_buf()) != 0)
		{
			LOG(LOG_NO_INFO, _("NET: PLUGIN: redirect detected"));
			dl->setRedirected(lightspark::tiny_string(stream->url));
		}
		if(NP_VERSION_MINOR >= NPVERS_HAS_RESPONSE_HEADERS)
		{
			//We've already got the length of the download, no need to set it from the headers
			dl->parseHeaders(stream->headers, false);
		}
	}
	else
	{
		//This is the main file
		m_sys->setOrigin(stream->url);
		*stype=NP_ASFILE;
		//Let's get the cookies now, they might be useful
		uint32_t len = 0;
		char *data = 0;
		const string& url(getPageURL());
		if(!url.empty())
		{
			//Skip the protocol slashes
			int protocolEnd=url.find("//");
			//Find the first slash after the protocol ones
			int urlEnd=url.find("/",protocolEnd+2);
			NPN_GetValueForURL(mInstance, NPNURLVCookie, url.substr(0,urlEnd+1).c_str(), &data, &len);
			string packedCookies(data, len);
			NPN_MemFree(data);
			m_sys->setCookies(packedCookies.c_str());
		}
	}
	//The downloader is set as the private data for this stream
	stream->pdata=dl;
	return NPERR_NO_ERROR; 
}

void nsPluginInstance::StreamAsFile(NPStream* stream, const char* fname)
{
	assert(stream->notifyData==NULL);
	m_sys->setDownloadedPath(fname);
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
		NPDownloader* dl=static_cast<NPDownloader*>(stream->pdata);
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
		cerr << "Destroy " << stream->pdata << endl;
	else
		LOG(LOG_NO_INFO, _("DestroyStream on main stream"));
	return NPERR_NO_ERROR;
}

void nsPluginInstance::URLNotify(const char* url, NPReason reason, void* notifyData)
{
	NPDownloader* dl=static_cast<NPDownloader*>(notifyData);
	cout << "URLnotify " << url << endl;
	//Notify our downloader of what happened
	switch(reason)
	{
		case NPRES_DONE:
			cout << "Done" <<endl;
			dl->setFinished();
			break;
		case NPRES_USER_BREAK:
			cout << "User Break" <<endl;
			dl->setFailed();
			break;
		case NPRES_NETWORK_ERR:
			cout << "Network Error" <<endl;
			dl->setFailed();
			break;
	}
}
