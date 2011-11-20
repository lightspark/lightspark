/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)
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

#include "plugin.h"
#include "logger.h"
#include "compat.h"
#include <string>
#include <algorithm>
#include "backends/urlutils.h"
#include "backends/security.h"

#include "npscriptobject.h"

#define MIME_TYPES_HANDLED  "application/x-shockwave-flash"
#define FAKE_MIME_TYPE  "application/x-lightspark"
#define PLUGIN_NAME    "Shockwave Flash"
#define FAKE_PLUGIN_NAME    "Lightspark player"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED":swf:"PLUGIN_NAME";"FAKE_MIME_TYPE":swfls:"FAKE_PLUGIN_NAME
#define PLUGIN_DESCRIPTION "Shockwave Flash 10.2 r"SHORTVERSION

using namespace std;
using namespace lightspark;

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
 * \brief Create a Downloader for an URL.
 *
 * Returns a pointer to a newly created Downloader for the given URL.
 * \param[in] url The URL (as a \c URLInfo) the \c Downloader is requested for
 * \param[in] cached Whether or not to disk-cache the download (default=false)
 * \return A pointer to a newly created \c Downloader for the given URL.
 * \see DownloadManager::destroy()
 */
lightspark::Downloader* NPDownloadManager::download(const lightspark::URLInfo& url, bool cached, lightspark::ILoadable* owner)
{
	// Handle RTMP requests internally, not through NPAPI
	if(url.getProtocol()=="rtmp" ||
	   url.getProtocol()=="rtmpe" ||
	   url.getProtocol()=="rtmps")
	{
		return StandaloneDownloadManager::download(url, cached, owner);
	}

	LOG(LOG_INFO, _("NET: PLUGIN: DownloadManager::download '") << url.getParsedURL() << 
			"'" << (cached ? _(" - cached") : ""));
	//Register this download
	NPDownloader* downloader=new NPDownloader(url.getParsedURL(), cached, instance, owner);
	addDownloader(downloader);
	return downloader;
}
/*
   \brief Create a Downloader for an URL and sending additional data

   Returns a pointer to the newly create Downloader for the given URL
 * \param[in] url The URL (as a \c URLInfo) the \c Downloader is requested for
 * \param[in] data Additional data that will be sent with the request
 * \return A pointer to a newly created \c Downloader for the given URL.
 * \see DownloadManager::destroy()
 */
lightspark::Downloader* NPDownloadManager::downloadWithData(const lightspark::URLInfo& url, const std::vector<uint8_t>& data, 
		lightspark::ILoadable* owner)
{
	// Handle RTMP requests internally, not through NPAPI
	if(url.getProtocol()=="rtmp" ||
	   url.getProtocol()=="rtmpe" ||
	   url.getProtocol()=="rtmps")
	{
		return StandaloneDownloadManager::downloadWithData(url, data, owner);
	}

	LOG(LOG_INFO, _("NET: PLUGIN: DownloadManager::downloadWithData '") << url.getParsedURL());
	//Register this download
	NPDownloader* downloader=new NPDownloader(url.getParsedURL(), data, instance, owner);
	addDownloader(downloader);
	return downloader;
}

void NPDownloadManager::destroy(lightspark::Downloader* downloader)
{
	NPDownloader* d=dynamic_cast<NPDownloader*>(downloader);
	if(!d)
	{
		StandaloneDownloadManager::destroy(downloader);
		return;
	}

	/*If the NP stream is already destroyed we can surely destroy the Downloader.
	  Moreover, if ASYNC_DESTROY is already set, destroy is being called for the second time.
	  This may happen when:
	  1) destroy is called by any NPP callback the checks the ASYNC_DESTROY flag and destroys the downloader
	  2) At shutdown destroy is called on every active download
	  In both cases it's safe to destroy the downloader, no more NPP call for this downloader will be received */
	if(d->state!=NPDownloader::STREAM_DESTROYED && d->state!=NPDownloader::ASYNC_DESTROY)
	{
		//The NP stream is still alive. Flag this downloader for aync destruction
		d->state=NPDownloader::ASYNC_DESTROY;
		return;
	}
	//If the downloader was still in the active-downloader list, delete it
	if(removeDownloader(downloader))
	{
		downloader->waitForTermination();
		delete downloader;
	}
}

/**
 * \brief Constructor for the NPDownloader class (used for the main stream)
 *
 * \param[in] _url The URL for the Downloader.
 */
NPDownloader::NPDownloader(const lightspark::tiny_string& _url, lightspark::ILoadable* owner):
	Downloader(_url, false, owner),instance(NULL),state(INIT)
{
}

/**
 * \brief Constructor for the NPDownloader class
 *
 * \param[in] _url The URL for the Downloader.
 * \param[in] _cached Whether or not to cache this download.
 * \param[in] _instance The netscape plugin instance
 * \param[in] owner The \c LoaderInfo object that keeps track of this download
 */
NPDownloader::NPDownloader(const lightspark::tiny_string& _url, bool _cached, NPP _instance, lightspark::ILoadable* owner):
	Downloader(_url, _cached, owner),instance(_instance),state(INIT)
{
	NPN_PluginThreadAsyncCall(instance, dlStartCallback, this);
}

/**
 * \brief Constructor for the NPDownloader class when sending additional data (POST)
 *
 * \param[in] _url The URL for the Downloader.
 * \param[in] _data Additional data that is sent with the request
 * \param[in] _instance The netscape plugin instance
 * \param[in] owner The \c LoaderInfo object that keeps track of this download
 */
NPDownloader::NPDownloader(const lightspark::tiny_string& _url, const std::vector<uint8_t>& _data, NPP _instance, lightspark::ILoadable* owner):
	Downloader(_url, _data, owner),instance(_instance),state(INIT)
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
	LOG(LOG_INFO,_("Start download for ") << th->url);
	NPError e=NPERR_NO_ERROR;
	if(th->data.empty())
		e=NPN_GetURLNotify(th->instance, th->url.raw_buf(), NULL, th);
	else
		e=NPN_PostURLNotify(th->instance, th->url.raw_buf(), NULL, th->data.size(), (const char*)&th->data[0], false, th);
	if(e!=NPERR_NO_ERROR)
		th->setFailed(); //No need to crash, we can just mark the download failed
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
	LOG_LEVEL log_level = LOG_INFO;

	/* setup glib/gtk, this is already done on firefox/linux, but needs to be done
	 * on firefox/windows */
	g_thread_init(NULL);
	gdk_threads_init();
	gtk_init(NULL, NULL);

	char *envvar = getenv("LIGHTSPARK_PLUGIN_LOGLEVEL");
	if (envvar)
		log_level=(LOG_LEVEL) min(4, max(0, atoi(envvar)));
	Log::setLogLevel(log_level);
	lightspark::SystemState::staticInit();
	return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
	LOG(LOG_INFO,"Lightspark plugin shutdown");
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
	setTLSSys( NULL );
}

////////////////////////////////////////
//
// nsPluginInstance class implementation
//
nsPluginInstance::nsPluginInstance(NPP aInstance, int16_t argc, char** argn, char** argv) : 
	nsPluginInstanceBase(), mInstance(aInstance),mInitialized(FALSE),mContainer(NULL),mWindow(0),
	mainDownloaderStream(NULL),mainDownloader(NULL),scriptObject(NULL),m_pt(NULL)
{
	LOG(LOG_INFO, "Lightspark version " << VERSION << " Copyright 2009-2011 Alessandro Pignotti and others");
	setTLSSys( NULL );
	m_sys=new lightspark::SystemState(NULL,0);
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

	int p_major, p_minor, n_major, n_minor;
	NPN_Version(&p_major, &p_minor, &n_major, &n_minor);
	if (n_minor >= 14) { // since NPAPI start to support
		scriptObject =
			(NPScriptObjectGW *) NPN_CreateObject(mInstance, &NPScriptObjectGW::npClass);
		m_sys->extScriptObject = scriptObject->getScriptObject();
		scriptObject->m_sys = m_sys;
	}
	else
		LOG(LOG_ERROR, "PLUGIN: Browser doesn't support NPRuntime");

	//The sys var should be NULL in this thread
	setTLSSys( NULL );
}

nsPluginInstance::~nsPluginInstance()
{
	//Shutdown the system
	setTLSSys(m_sys);
	if(mainDownloader)
		mainDownloader->stop();

	// Kill all stuff relating to NPScriptObject which is still running
	static_cast<NPScriptObject*>(m_sys->extScriptObject)->destroy();

	if(m_pt)
		m_pt->stop();
	m_sys->setShutdownFlag();

	m_sys->wait();

	// Delete our external script object
	delete m_sys->extScriptObject;

	m_sys->destroy();
	delete m_pt;
	setTLSSys(NULL);
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
  
  if (SetWindow(aWindow) == NPERR_NO_ERROR)
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
		case NPPVpluginScriptableNPObject:
			if(scriptObject)
			{
				void **v = (void **)aValue;
				NPN_RetainObject(scriptObject);
				*v = scriptObject;
				LOG(LOG_INFO, "PLUGIN: NPScriptObjectGW returned to browser");
				break;
			}
			else
				LOG(LOG_INFO, "PLUGIN: NPScriptObjectGW requested but was NULL");
		default:
			err = NPERR_INVALID_PARAM;
			break;
	}
	return err;

}

NPError nsPluginInstance::SetWindow(NPWindow* aWindow)
{
	if(aWindow == NULL)
		return NPERR_GENERIC_ERROR;

	mX = aWindow->x;
	mY = aWindow->y;
	mWidth = aWindow->width;
	mHeight = aWindow->height;
	if(mHeight==0 || mHeight==0)
	{
		LOG(LOG_ERROR,_("No size in SetWindow"));
		return NPERR_GENERIC_ERROR;
	}
#ifdef _WIN32
	if (mWindow == (HWND) aWindow->window)
	{
		// The page with the plugin is being resized.
		// Save any UI information because the next time
		// around expect a SetWindow with a new window id.
		LOG(LOG_ERROR,"Resize not supported");
	}
	else
	{
		assert(mWindow==0);
		mWindow = (HWND) aWindow->window;
		PluginEngineData* e= new PluginEngineData(this, mWindow, mWidth, mHeight);
		LOG(LOG_INFO,"Plugin Window " << hex << mWindow << dec << " Width: " << mWidth << " Height: " << mHeight);
		m_sys->setParamsAndEngine(e, false);
	}
#else
	if (mWindow == (Window) aWindow->window)
	{
		// The page with the plugin is being resized.
		// Save any UI information because the next time
		// around expect a SetWindow with a new window id.
		LOG(LOG_ERROR,"Resize not supported");
	}
	else
	{
		assert(mWindow==0);
		mWindow = (Window) aWindow->window;
		NPSetWindowCallbackStruct *ws_info = (NPSetWindowCallbackStruct *)aWindow->ws_info;
		mDisplay = ws_info->display;
		mVisual = ws_info->visual;
		mDepth = ws_info->depth;
		mColormap = ws_info->colormap;

		VisualID visual=XVisualIDFromVisual(mVisual);
		PluginEngineData* e= new PluginEngineData(this, mDisplay, visual, mWindow, mWidth, mHeight);
		LOG(LOG_INFO,"X Window " << hex << mWindow << dec << " Width: " << mWidth << " Height: " << mHeight);
		m_sys->setParamsAndEngine(e, false);
	}
#endif
	//draw();
	return NPERR_NO_ERROR;
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

void nsPluginInstance::asyncDownloaderDestruction(NPStream* stream, NPDownloader* dl) const
{
	LOG(LOG_INFO,_("Async destructin for ") << stream->url);
	m_sys->downloadManager->destroy(dl);
}

NPError nsPluginInstance::NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
{
	NPDownloader* dl=static_cast<NPDownloader*>(stream->notifyData);
	LOG(LOG_INFO,_("Newstream for ") << stream->url);
	setTLSSys( m_sys );
	if(dl)
	{
		//Check if async destructin of this downloader has been requested
		if(dl->state==NPDownloader::ASYNC_DESTROY)
		{
			//NPN_DestroyStream will call NPP_DestroyStream
			NPError e=NPN_DestroyStream(mInstance, stream, NPRES_USER_BREAK);
			assert(e==NPERR_NO_ERROR);
			return e;
		}
		dl->setLength(stream->end);
		//TODO: confirm that growing buffers are normal. This does fix a bug I found though. (timonvo)
		*stype=NP_NORMAL;

		if(strcmp(stream->url, dl->getURL().raw_buf()) != 0)
		{
			LOG(LOG_INFO, _("NET: PLUGIN: redirect detected"));
			dl->setRedirected(lightspark::tiny_string(stream->url));
		}
		if(NP_VERSION_MINOR >= NPVERS_HAS_RESPONSE_HEADERS)
		{
			//We've already got the length of the download, no need to set it from the headers
			dl->parseHeaders(stream->headers, false);
		}
	}
	else if(m_pt==NULL)
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
		//Now create a Downloader for this
		dl=new NPDownloader(stream->url,m_sys->getLoaderInfo());
		dl->setLength(stream->end);
		mainDownloader=dl;
		mainDownloaderStream.rdbuf(mainDownloader);
		m_pt=new lightspark::ParseThread(mainDownloaderStream,m_sys);
		m_sys->addJob(m_pt);
	}
	//The downloader is set as the private data for this stream
	stream->pdata=dl;
	setTLSSys( NULL );
	return NPERR_NO_ERROR;
}

void nsPluginInstance::StreamAsFile(NPStream* stream, const char* fname)
{
	assert(stream->notifyData==NULL);
	m_sys->setDownloadedPath(lightspark::tiny_string(fname,true));
}

int32_t nsPluginInstance::WriteReady(NPStream *stream)
{
	if(stream->pdata) //This is a Downloader based download
		return 1024*1024;
	else
		return 0;
}

int32_t nsPluginInstance::Write(NPStream *stream, int32_t offset, int32_t len, void *buffer)
{
	if(stream->pdata)
	{
		setTLSSys( m_sys );
		NPDownloader* dl=static_cast<NPDownloader*>(stream->pdata);

		//Check if async destructin of this downloader has been requested
		if(dl->state==NPDownloader::ASYNC_DESTROY)
		{
			//NPN_DestroyStream will call NPP_DestroyStream
			NPError e=NPN_DestroyStream(mInstance, stream, NPRES_USER_BREAK);
			assert(e==NPERR_NO_ERROR);
			return -1;
		}

		if(dl->hasFailed())
			return -1;
		dl->append((uint8_t*)buffer,len);
		setTLSSys( NULL );
		return len;
	}
	else
		return len; //Drop everything (this is a second stream for the same SWF instance)
}

NPError nsPluginInstance::DestroyStream(NPStream *stream, NPError reason)
{
	setTLSSys(m_sys);
	NPDownloader* dl=static_cast<NPDownloader*>(stream->pdata);
	assert(dl);
	//Check if async destructin of this downloader has been requested
	if(dl->state==NPDownloader::ASYNC_DESTROY)
	{
		dl->setFailed();
		asyncDownloaderDestruction(stream, dl);
		return NPERR_NO_ERROR;
	}
	dl->state=NPDownloader::STREAM_DESTROYED;
	if(stream->notifyData)
	{
		//URLNotify will be called later
		return NPERR_NO_ERROR;
	}
	//Notify our downloader of what happened
	URLNotify(stream->url, reason, stream->pdata);
	setTLSSys(NULL);
	return NPERR_NO_ERROR;
}

void nsPluginInstance::URLNotify(const char* url, NPReason reason, void* notifyData)
{
	//If the download was successful, termination is handle in DestroyStream
	NPDownloader* dl=static_cast<NPDownloader*>(notifyData);
	assert(dl);
	switch(reason)
	{
		case NPRES_DONE:
			LOG(LOG_INFO,_("Download complete ") << url);
			dl->setFinished();
			break;
		case NPRES_USER_BREAK:
			LOG(LOG_ERROR,_("Download stopped ") << url);
			dl->setFailed();
			break;
		case NPRES_NETWORK_ERR:
			LOG(LOG_ERROR,_("Download error ") << url);
			dl->setFailed();
			break;
	}
}

void callHelper(sigc::slot<void>* slot)
{
	(*slot)();
	delete slot;
}

void PluginEngineData::setupMainThreadCallback(const sigc::slot<void>& slot)
{
	//create a slot on heap to survive the return of this function
	typedef void (*npn_callback)(void *);
	NPN_PluginThreadAsyncCall(instance->mInstance, (npn_callback)callHelper, new sigc::slot<void>(slot));
}

void PluginEngineData::stopMainDownload()
{
	if(instance->mainDownloader)
		instance->mainDownloader->stop();
}
