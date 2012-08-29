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

#include "version.h"
#include "plugin/plugin.h"
#include "logger.h"
#include "compat.h"
#include <string>
#include <algorithm>
#ifdef _WIN32
#	include <gdk/gdkwin32.h>
#endif
#include "backends/urlutils.h"
#include "backends/security.h"

#include "plugin/npscriptobject.h"

#define MIME_TYPES_HANDLED  "application/x-shockwave-flash"
#define FAKE_MIME_TYPE  "application/x-lightspark"
#define PLUGIN_NAME    "Shockwave Flash"
#define FAKE_PLUGIN_NAME    "Lightspark player"
#define MIME_TYPES_DESCRIPTION  MIME_TYPES_HANDLED ":swf:" PLUGIN_NAME ";" FAKE_MIME_TYPE ":swfls:" FAKE_PLUGIN_NAME
#define PLUGIN_DESCRIPTION "Shockwave Flash 12.1 r" SHORTVERSION

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
	if(url.isRTMP())
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
 * \param[in] headers Request headers in the full form, f.e. "Content-Type: ..."
 * \return A pointer to a newly created \c Downloader for the given URL.
 * \see DownloadManager::destroy()
 */
lightspark::Downloader* NPDownloadManager::downloadWithData(const lightspark::URLInfo& url, const std::vector<uint8_t>& data, 
		const std::list<tiny_string>& headers, lightspark::ILoadable* owner)
{
	// Handle RTMP requests internally, not through NPAPI
	if(url.isRTMP())
	{
		return StandaloneDownloadManager::downloadWithData(url, data, headers, owner);
	}

	LOG(LOG_INFO, _("NET: PLUGIN: DownloadManager::downloadWithData '") << url.getParsedURL());
	//Register this download
	NPDownloader* downloader=new NPDownloader(url.getParsedURL(), data, headers, instance, owner);
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
	Downloader(_url, false, owner),instance(NULL),cleanupInDestroyStream(true),state(INIT)
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
	Downloader(_url, _cached, owner),instance(_instance),cleanupInDestroyStream(false),state(INIT)
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
NPDownloader::NPDownloader(const lightspark::tiny_string& _url, const std::vector<uint8_t>& _data,
		const std::list<tiny_string>& headers, NPP _instance, lightspark::ILoadable* owner):
	Downloader(_url, _data, headers, owner),instance(_instance),cleanupInDestroyStream(false),state(INIT)
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
	{
		const char *linefeed="\r\n";
		const char *linefeedend=linefeed+strlen(linefeed);
		vector<uint8_t> tmpData;
		std::list<tiny_string>::const_iterator it;
		for(it=th->requestHeaders.begin(); it!=th->requestHeaders.end(); ++it)
		{
			tmpData.insert(tmpData.end(), it->raw_buf(), it->raw_buf()+it->numBytes());
			tmpData.insert(tmpData.end(), linefeed, linefeedend);
		}
		char buf[40];
		snprintf(buf, 40, "Content-Length: %lu\r\n\r\n", th->data.size());
		tmpData.insert(tmpData.end(), buf, buf+strlen(buf));
		tmpData.insert(tmpData.end(), th->data.begin(), th->data.end());
		e=NPN_PostURLNotify(th->instance, th->url.raw_buf(), NULL, tmpData.size(), (const char*)&tmpData[0], false, th);
	}
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
	LOG_LEVEL log_level = LOG_NOT_IMPLEMENTED;

	/* setup glib/gtk, this is already done on firefox/linux, but needs to be done
	 * on firefox/windows (because there we statically link to gtk) */
#ifdef HAVE_G_THREAD_INIT
	if(!g_thread_supported())
		g_thread_init(NULL);
#endif
#ifdef _WIN32
	//Calling gdk_threads_init multiple times (once by firefox, once by us)
	//will break in various different ways (hangs, segfaults, etc.)
	gdk_threads_init();
	gtk_init(NULL, NULL);
#endif

	char *envvar = getenv("LIGHTSPARK_PLUGIN_LOGLEVEL");
	if (envvar)
		log_level=(LOG_LEVEL) min(4, max(0, atoi(envvar)));

	envvar = getenv("LIGHTSPARK_PLUGIN_LOGFILE");
	if (envvar)
		Log::redirect(envvar);

	Log::setLogLevel(log_level);
	lightspark::SystemState::staticInit();

	/* On linux, firefox runs its own gtk main loop
	 * which we can utilise through g_add_idle
	 * but we must not create our own gtk_main.
	 */
#ifdef _WIN32
	EngineData::startGTKMain();
#endif
	return NPERR_NO_ERROR;
}

void NS_PluginShutdown()
{
	LOG(LOG_INFO,"Lightspark plugin shutdown");
	lightspark::SystemState::staticDeinit();
#ifdef _WIN32
	EngineData::quitGTKMain();
#endif
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
	LOG(LOG_INFO,"NS_DestroyPluginInstance " << aPlugin);
	if(aPlugin)
		delete (nsPluginInstance *)aPlugin;
	setTLSSys( NULL );
}

////////////////////////////////////////
//
// nsPluginInstance class implementation
//
nsPluginInstance::nsPluginInstance(NPP aInstance, int16_t argc, char** argn, char** argv) : 
	nsPluginInstanceBase(), mInstance(aInstance),mInitialized(FALSE),mWindow(0),
	mainDownloaderStream(NULL),mainDownloader(NULL),scriptObject(NULL),m_pt(NULL)
{
	LOG(LOG_INFO, "Lightspark version " << VERSION << " Copyright 2009-2012 Alessandro Pignotti and others");
	setTLSSys( NULL );
	m_sys=new lightspark::SystemState(0, lightspark::SystemState::FLASH);
	//Files running in the plugin have REMOTE sandbox
	m_sys->securityManager->setSandboxType(lightspark::SecurityManager::REMOTE);
	//Parse OBJECT/EMBED tag attributes
	string baseURL;
	for(int i=0;i<argc;i++)
	{
		if(argn[i]==NULL || argv[i]==NULL)
			continue;
		if(strcasecmp(argn[i],"flashvars")==0)
		{
			m_sys->parseParametersFromFlashvars(argv[i]);
		}
		else if(strcasecmp(argn[i],"base")==0)
		{
			baseURL = argv[i];
			//This is a directory, not a file
			baseURL += "/";
		}
		//The SWF file url should be getted from NewStream
	}
	//basedir is a qualified URL or a path relative to the HTML page
	URLInfo page(getPageURL());
	m_sys->mainClip->setBaseURL(page.goToURL(baseURL).getURL());

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
	LOG(LOG_INFO, "~nsPluginInstance " << this);
	//Shutdown the system
	setTLSSys(m_sys);
	if(mainDownloader)
		mainDownloader->stop();

	// Kill all stuff relating to NPScriptObject which is still running
	static_cast<NPScriptObject*>(m_sys->extScriptObject)->destroy();

	m_sys->setShutdownFlag();

	m_sys->destroy();
	delete m_sys;
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

#ifdef _WIN32
/*
 * Create a GtkWidget and embed it into that mWindow. This unifies the other code,
 * because on any platform and standalone/plugin, we always handle GtkWidgets.
 * Additionally, firefox always crashed on me when trying to directly draw into
 * mWindow.
 * This must be run in the gtk_main() thread for AttachThreadInput to make sense.
 */
GtkWidget* PluginEngineData::createGtkWidget()
{
	HWND parent_hwnd = (HWND)instance->mWindow;

	GtkWidget* widget=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	/* Remove window decorations */
	gtk_window_set_decorated((GtkWindow*)widget, FALSE);
	/* Realize to construct hwnd */
	gtk_widget_realize(widget);
	HWND window = (HWND)GDK_WINDOW_HWND(gtk_widget_get_window(widget));
	/* Set WS_CHILD, remove WS_POPUP - see MSDN on SetParent */
	DWORD dwStyle = GetWindowLong (window, GWL_STYLE);
	dwStyle |= WS_CHILD;
	dwStyle &= ~WS_POPUP;
	SetWindowLong(window, GWL_STYLE, dwStyle);
	SetForegroundWindow(window);
	/* Re-parent */
	SetParent(window, parent_hwnd);
	/* Attach our thread to that of the parent.
	 * This ensures that we get messages for input events*/
	DWORD parentThreadId;
	if(!(parentThreadId = GetWindowThreadProcessId(parent_hwnd, NULL)))
		LOG(LOG_ERROR,"GetWindowThreadProcessId failed");
	DWORD myThreadId;
	if(!(myThreadId = GetCurrentThreadId()))
		LOG(LOG_ERROR,"GetCurrentThreadId failed");
	if(!AttachThreadInput(myThreadId, parentThreadId, TRUE))
		LOG(LOG_ERROR,"AttachThreadInput failed");
	/* Set window size */
	gtk_widget_set_size_request(widget, width, height);

	return widget;
}
#else
GtkWidget* PluginEngineData::createGtkWidget()
{
	return gtk_plug_new(instance->mWindow);
}
#endif

NPError nsPluginInstance::SetWindow(NPWindow* aWindow)
{
	if(aWindow == NULL)
		return NPERR_GENERIC_ERROR;

	mX = aWindow->x;
	mY = aWindow->y;
	uint32_t width = aWindow->width;
	uint32_t height = aWindow->height;
	/* aWindow->window is always a void*,
	 * GdkNativeWindow is int on linux and void* on win32.
	 * Casting void* to int gvies an error (loss of precision), so we cast to intptr_t.
	 */
#ifdef _WIN32
	GdkNativeWindow win = reinterpret_cast<GdkNativeWindow>(aWindow->window);
#else
	GdkNativeWindow win = reinterpret_cast<intptr_t>(aWindow->window);
#endif
	if (mWindow == win )
	{
		// The page with the plugin is being resized.
		// Save any UI information because the next time
		// around expect a SetWindow with a new window id.
		LOG(LOG_ERROR,"Resize not supported");
		return NPERR_NO_ERROR;
	}
	assert(mWindow==0);

	PluginEngineData* e = new PluginEngineData(this, width, height);
	mWindow = win;

	LOG(LOG_INFO,"From Browser: Window " << mWindow << " Width: " << width << " Height: " << height);

#ifndef _WIN32
	NPSetWindowCallbackStruct *ws_info = (NPSetWindowCallbackStruct *)aWindow->ws_info;
	//mDisplay = ws_info->display;
	//mDepth = ws_info->depth;
	//mColormap = ws_info->colormap;
	e->visual = XVisualIDFromVisual(ws_info->visual);
#endif
	m_sys->setParamsAndEngine(e, false);
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

void nsPluginInstance::asyncDownloaderDestruction(const char *url, NPDownloader* dl) const
{
	LOG(LOG_INFO,_("Async destruction for ") << url);
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
			LOG(LOG_INFO, "NET: PLUGIN: redirect detected from " << dl->getURL() << " to " << stream->url);
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
		m_sys->mainClip->setOrigin(stream->url);
		m_sys->parseParametersFromURL(m_sys->mainClip->getOrigin());
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
		dl=new NPDownloader(stream->url,m_sys->mainClip->loaderInfo.getPtr());
		dl->setLength(stream->end);
		mainDownloader=dl;
		mainDownloaderStream.rdbuf(mainDownloader);
		m_pt=new lightspark::ParseThread(mainDownloaderStream,m_sys->mainClip);
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
			(void)e; // silence warning about unused variable
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

void nsPluginInstance::downloaderFinished(NPDownloader* dl, const char *url, NPReason reason) const
{
	setTLSSys(m_sys);
	//Check if async destruction of this downloader has been requested
	if(dl->state==NPDownloader::ASYNC_DESTROY)
	{
		dl->setFailed();
		asyncDownloaderDestruction(url, dl);
		setTLSSys(NULL);
		return;
	}
	dl->state=NPDownloader::STREAM_DESTROYED;

	//Notify our downloader of what happened
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
	setTLSSys(NULL);
	return;
}

NPError nsPluginInstance::DestroyStream(NPStream *stream, NPError reason)
{
	NPDownloader* dl=static_cast<NPDownloader*>(stream->pdata);
	if(!dl)
		return NPERR_NO_ERROR;

	/* Normally we cleanup in URLNotify, which is called after
	 * destroyStream and is called even if the stream fails before
	 * newStream/destroyStream. URLNotify won't be called for the
	 * main SWF stream and therefore we need to cleanup here.
	 */
	if(dl->cleanupInDestroyStream)
		downloaderFinished(dl, stream->url, reason);

	return NPERR_NO_ERROR;
}

void nsPluginInstance::URLNotify(const char* url, NPReason reason, void* notifyData)
{
	NPDownloader* dl=static_cast<NPDownloader*>(notifyData);
	if(!dl)
		return;

	downloaderFinished(dl, url, reason);
}

void PluginEngineData::stopMainDownload()
{
	if(instance->mainDownloader)
		instance->mainDownloader->stop();
}

GdkNativeWindow PluginEngineData::getWindowForGnash()
{
	return instance->mWindow;
}

#ifdef _WIN32
/* Setup for getExectuablePath() */
extern HINSTANCE g_hinstance;
extern "C"
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	g_hinstance = hinstDLL;
	return TRUE;
}
#endif
