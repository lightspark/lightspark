/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010-2011  Timon Van Overveldt (timonvo@gmail.com)
    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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
#include "backends/input.h"
#include "backends/security.h"
#include "backends/streamcache.h"
#include "backends/config.h"
#include "backends/rendering.h"
#include "logger.h"
#include "compat.h"
#include <string>
#include <algorithm>
#include "backends/urlutils.h"
#include "abc.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "plugin/plugin.h"

#include "plugin/npscriptobject.h"
#include <SDL.h>
#include <SDL_syswm.h>
#ifdef MOZ_X11
#include <X11/keysym.h>
#endif

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
lightspark::Downloader* NPDownloadManager::download(const lightspark::URLInfo& url, _R<StreamCache> cache, lightspark::ILoadable* owner)
{
	// empty URL means data is generated from calls to NetStream::appendBytes
	if(!url.isValid() && url.getInvalidReason() == URLInfo::IS_EMPTY)
	{
		return StandaloneDownloadManager::download(url, cache, owner);
	}
	// Handle RTMP requests internally, not through NPAPI
	if(url.isRTMP())
	{
		return StandaloneDownloadManager::download(url, cache, owner);
	}

	// FIXME: dynamic_cast fails because the linker doesn't find
	// typeinfo for FileStreamCache
	//bool cached = dynamic_cast<FileStreamCache *>(cache.getPtr()) != NULL;
	bool cached = false;
	LOG(LOG_INFO, "NET: PLUGIN: DownloadManager::download '" << url.getParsedURL() << 
			"'" << (cached ? " - cached" : ""));
	//Register this download
	NPDownloader* downloader=new NPDownloader(url.getParsedURL(), cache, instance, owner);
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
lightspark::Downloader* NPDownloadManager::downloadWithData(const lightspark::URLInfo& url,
		_R<StreamCache> cache, const std::vector<uint8_t>& data,
		const std::list<tiny_string>& headers, lightspark::ILoadable* owner)
{
	// Handle RTMP requests internally, not through NPAPI
	if(url.isRTMP())
	{
		return StandaloneDownloadManager::downloadWithData(url, cache, data, headers, owner);
	}

	LOG(LOG_INFO, "NET: PLUGIN: DownloadManager::downloadWithData '" << url.getParsedURL());
	//Register this download
	NPDownloader* downloader=new NPDownloader(url.getParsedURL(), cache, data, headers, instance, owner);
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
	Downloader(_url, _MR(new MemoryStreamCache(getSys())), owner),instance(NULL),cleanupInDestroyStream(true),state(INIT)
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
NPDownloader::NPDownloader(const lightspark::tiny_string& _url, _R<StreamCache> _cache, NPP _instance, lightspark::ILoadable* owner):
	Downloader(_url, _cache, owner),instance(_instance),cleanupInDestroyStream(false),state(INIT)
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
NPDownloader::NPDownloader(const lightspark::tiny_string& _url, _R<StreamCache> _cache,
		const std::vector<uint8_t>& _data,
		const std::list<tiny_string>& headers, NPP _instance, lightspark::ILoadable* owner):
	Downloader(_url, _cache, _data, headers, owner),instance(_instance),cleanupInDestroyStream(false),state(INIT)
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
	LOG(LOG_INFO,"Start download for " << th->url);
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
		snprintf(buf, 40, "Content-Length: %lu\r\n\r\n", (uint64_t)th->data.size());
		tmpData.insert(tmpData.end(), buf, buf+strlen(buf));
		tmpData.insert(tmpData.end(), th->data.begin(), th->data.end());
		e=NPN_PostURLNotify(th->instance, th->url.raw_buf(), NULL, tmpData.size(), (const char*)&tmpData[0], false, th);
	}
	if(e!=NPERR_NO_ERROR)
	{
		LOG(LOG_ERROR,"download failed for " << th->url << " code:"<<e);
		th->setFailed(); //No need to crash, we can just mark the download failed
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
	LOG_LEVEL log_level = LOG_NOT_IMPLEMENTED;

	char *envvar = getenv("LIGHTSPARK_PLUGIN_LOGLEVEL");
	if (envvar)
		log_level=(LOG_LEVEL) min(4, max(0, atoi(envvar)));

	envvar = getenv("LIGHTSPARK_PLUGIN_LOGFILE");
	if (envvar)
		Log::redirect(envvar);

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
	LOG(LOG_INFO,"NS_DestroyPluginInstance " << aPlugin);
	if(aPlugin)
		delete (nsPluginInstance *)aPlugin;
	setTLSSys( nullptr );
	setTLSWorker(nullptr);
}

////////////////////////////////////////
//
// nsPluginInstance class implementation
//
nsPluginInstance::nsPluginInstance(NPP aInstance, int16_t argc, char** argn, char** argv) : 
	nsPluginInstanceBase(), mInstance(aInstance),mInitialized(FALSE),mWindow(0),
	mainDownloaderStreambuf(nullptr),mainDownloaderStream(nullptr),
	mainDownloader(nullptr),scriptObject(nullptr),m_pt(nullptr),
	eventLoop(new Time(), this)
{
	LOG(LOG_INFO, "Lightspark version " << VERSION << " Copyright 2009-2013 Alessandro Pignotti and others");
	setTLSSys( nullptr );
	setTLSWorker(nullptr);
	// TODO: Add an NPAPI logging implementation, that can print to the
	// JavaScript console.
	m_sys=new lightspark::SystemState
	(
		0,
		lightspark::SystemState::FLASH,
		&eventLoop
	);

	mInstance->pdata = this;

	//Files running in the plugin have REMOTE sandbox
	m_sys->securityManager->setSandboxType(lightspark::SecurityManager::REMOTE);

	int p_major, p_minor, n_major, n_minor;
	NPN_Version(&p_major, &p_minor, &n_major, &n_minor);
	if (n_minor >= 14) { // since NPAPI start to support
		scriptObject =
			(NPScriptObjectGW *) NPN_CreateObject(mInstance, &NPScriptObjectGW::npClass);
		scriptObject->createScriptObject(m_sys);
		m_sys->extScriptObject = scriptObject->getScriptObject();
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
			else if(strcasecmp(argn[i],"name")==0)
			{
				m_sys->extScriptObject->setProperty(argn[i],argv[i]);
			}
			else if(strcasecmp(argn[i],"allowfullscreen")==0)
			{
				m_sys->allowFullscreen= strcasecmp(argv[i],"true") == 0;
			}
			else if(strcasecmp(argn[i],"allowfullscreeninteractive")==0)
			{
				m_sys->allowFullscreen= strcasecmp(argv[i],"true") == 0;
			}
			//The SWF file url should be getted from NewStream
		}
		NPN_SetValue(aInstance,NPPVpluginWindowBool,(void*)false);
		//basedir is a qualified URL or a path relative to the HTML page
		URLInfo page(getPageURL());
		m_sys->mainClip->setBaseURL(page.goToURL(baseURL).getURL());

		m_sys->downloadManager=new NPDownloadManager(mInstance);
	}
	else
		LOG(LOG_ERROR, "PLUGIN: Browser doesn't support NPRuntime");
	// NOTE: `mainthread_running` has to be `false`, because `SystemState`
	// would otherwise try to delete the event loop upon destruction, but
	// in this case, the event loop is inside `nsPluginInstance`.
	EngineData::mainthread_running = false;
	//The sys var should be NULL in this thread
	setTLSSys( nullptr );
	setTLSWorker(nullptr);
}

nsPluginInstance::~nsPluginInstance()
{
	LOG(LOG_INFO, "~nsPluginInstance " << this);
	//Shutdown the system
	setTLSSys(m_sys);
	setTLSWorker(m_sys->worker);
	if(mainDownloader)
		mainDownloader->stop();
	if (mainDownloaderStreambuf)
		delete mainDownloaderStreambuf;

	// stop fullscreen ticker
	if (m_sys->getEngineData() && m_sys->getEngineData()->widget)
		SDL_HideWindow(m_sys->getEngineData()->widget);
	
	// Kill all stuff relating to NPScriptObject which is still running
	m_sys->extScriptObject->destroy();

	m_sys->setShutdownFlag();

	m_sys->destroy();
	delete m_sys;
	delete m_pt;
	setTLSSys(nullptr);
	setTLSWorker(nullptr);
}

NPBool nsPluginInstance::init(NPWindow* aWindow)
{
  if(aWindow == nullptr)
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
			err = NPERR_INVALID_PARAM;
			break;
		default:
			err = NPERR_INVALID_PARAM;
			break;
	}
	return err;

}
#ifdef MOZ_X11
AS3KeyCode x11GetAS3KeyCode(unsigned x11Keyval)
{
	switch (x11Keyval)
	{
		case XK_a: return AS3KEYCODE_A;
		case XK_Alt_L: return AS3KEYCODE_ALTERNATE;
		case XK_b: return AS3KEYCODE_B;
		case XK_grave: return AS3KEYCODE_BACKQUOTE;
		case XK_backslash: return AS3KEYCODE_BACKSLASH;
		case XK_BackSpace: return AS3KEYCODE_BACKSPACE;
		case XK_c: return AS3KEYCODE_C;
		case XK_Caps_Lock: return AS3KEYCODE_CAPS_LOCK;
		case XK_comma: return AS3KEYCODE_COMMA;
		case XK_Control_L: return AS3KEYCODE_CONTROL;
		case XK_d: return AS3KEYCODE_D;
		case XK_Delete: return AS3KEYCODE_DELETE;
		case XK_Down: return AS3KEYCODE_DOWN;
		case XK_e: return AS3KEYCODE_E;
		case XK_End: return AS3KEYCODE_END;
		case XK_Return: return AS3KEYCODE_ENTER;
		case XK_equal: return AS3KEYCODE_EQUAL;
		case XK_Escape: return AS3KEYCODE_ESCAPE;
		case XK_f: return AS3KEYCODE_F;
		case XK_F1: return AS3KEYCODE_F1;
		case XK_F2: return AS3KEYCODE_F2;
		case XK_F3: return AS3KEYCODE_F3;
		case XK_F4: return AS3KEYCODE_F4;
		case XK_F5: return AS3KEYCODE_F5;
		case XK_F6: return AS3KEYCODE_F6;
		case XK_F7: return AS3KEYCODE_F7;
		case XK_F8: return AS3KEYCODE_F8;
		case XK_F9: return AS3KEYCODE_F9;
		case XK_F10: return AS3KEYCODE_F10;
		case XK_F11: return AS3KEYCODE_F11;
		case XK_F12: return AS3KEYCODE_F12;
		case XK_F13: return AS3KEYCODE_F13;
		case XK_F14: return AS3KEYCODE_F14;
		case XK_F15: return AS3KEYCODE_F15;
		case XK_g: return AS3KEYCODE_G;
		case XK_h: return AS3KEYCODE_H;
		case XK_Help: return AS3KEYCODE_HELP;
		case XK_Home: return AS3KEYCODE_HOME;
		case XK_i: return AS3KEYCODE_I;
		case XK_Insert: return AS3KEYCODE_INSERT;
		case XK_j: return AS3KEYCODE_J;
		case XK_k: return AS3KEYCODE_K;
		case XK_l: return AS3KEYCODE_L;
		case XK_Left: return AS3KEYCODE_LEFT;
		case XK_bracketleft: return AS3KEYCODE_LEFTBRACKET;
		case XK_m: return AS3KEYCODE_M;
		case XK_minus: return AS3KEYCODE_MINUS;
		case XK_n: return AS3KEYCODE_N;
		case XK_0: return AS3KEYCODE_NUMBER_0;
		case XK_1: return AS3KEYCODE_NUMBER_1;
		case XK_2: return AS3KEYCODE_NUMBER_2;
		case XK_3: return AS3KEYCODE_NUMBER_3;
		case XK_4: return AS3KEYCODE_NUMBER_4;
		case XK_5: return AS3KEYCODE_NUMBER_5;
		case XK_6: return AS3KEYCODE_NUMBER_6;
		case XK_7: return AS3KEYCODE_NUMBER_7;
		case XK_8: return AS3KEYCODE_NUMBER_8;
		case XK_9: return AS3KEYCODE_NUMBER_9;
		case XK_KP_0: return AS3KEYCODE_NUMPAD_0;
		case XK_KP_1: return AS3KEYCODE_NUMPAD_1;
		case XK_KP_2: return AS3KEYCODE_NUMPAD_2;
		case XK_KP_3: return AS3KEYCODE_NUMPAD_3;
		case XK_KP_4: return AS3KEYCODE_NUMPAD_4;
		case XK_KP_5: return AS3KEYCODE_NUMPAD_5;
		case XK_KP_6: return AS3KEYCODE_NUMPAD_6;
		case XK_KP_7: return AS3KEYCODE_NUMPAD_7;
		case XK_KP_8: return AS3KEYCODE_NUMPAD_8;
		case XK_KP_9: return AS3KEYCODE_NUMPAD_9;
		case XK_KP_Add: return AS3KEYCODE_NUMPAD_ADD;
		case XK_KP_Separator: return AS3KEYCODE_NUMPAD_DECIMAL; // TODO
		case XK_KP_Divide: return AS3KEYCODE_NUMPAD_DIVIDE;
		case XK_KP_Enter: return AS3KEYCODE_NUMPAD_ENTER;
		case XK_KP_Multiply: return AS3KEYCODE_NUMPAD_MULTIPLY;
		case XK_KP_Subtract: return AS3KEYCODE_NUMPAD_SUBTRACT;
		case XK_o: return AS3KEYCODE_O;
		case XK_p: return AS3KEYCODE_P;
		case XK_Page_Down: return AS3KEYCODE_PAGE_DOWN;
		case XK_Page_Up: return AS3KEYCODE_PAGE_UP;
		case XK_Pause: return AS3KEYCODE_PAUSE;
		case XK_period: return AS3KEYCODE_PERIOD;
		case XK_q: return AS3KEYCODE_Q;
		case XK_quoteright: return AS3KEYCODE_QUOTE;
		case XK_r: return AS3KEYCODE_R;
		case XK_Right: return AS3KEYCODE_RIGHT;
		case XK_bracketright: return AS3KEYCODE_RIGHTBRACKET;
		case XK_s: return AS3KEYCODE_S;
		case XK_semicolon: return AS3KEYCODE_SEMICOLON;
		case XK_Shift_L: return AS3KEYCODE_SHIFT;
		case XK_slash: return AS3KEYCODE_SLASH;
		case XK_space: return AS3KEYCODE_SPACE;
		case XK_t: return AS3KEYCODE_T;
		case XK_Tab: return AS3KEYCODE_TAB;
		case XK_u: return AS3KEYCODE_U;
		case XK_Up: return AS3KEYCODE_UP;
		case XK_v: return AS3KEYCODE_V;
		case XK_w: return AS3KEYCODE_W;
		case XK_x: return AS3KEYCODE_X;
		case XK_y: return AS3KEYCODE_Y;
		case XK_z: return AS3KEYCODE_Z;
	}
	return AS3KEYCODE_UNKNOWN;
}
#endif
SDL_Window* PluginEngineData::createWidget(uint32_t w,uint32_t h)
{
	return SDL_CreateWindow("Lightspark",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,w,h,SDL_WINDOW_BORDERLESS|SDL_WINDOW_HIDDEN| SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
}

void PluginEngineData::setDisplayState(const tiny_string &displaystate, SystemState *sys)
{
	if (!this->widget)
	{
		LOG(LOG_ERROR,"no widget available for setting displayState");
		return;
	}
	SDL_SetWindowFullscreen(widget, displaystate.startsWith("fullScreen") ? SDL_WINDOW_FULLSCREEN_DESKTOP: 0);
	if (displaystate == "fullScreen")
	{
		SDL_ShowWindow(widget);
		startSDLEventTicker(sys);
	}
	else
	{
		SDL_HideWindow(widget);
		inRendering=false;
	}
	int w,h;
	SDL_GetWindowSize(widget,&w,&h);
	sys->getRenderThread()->requestResize(w,h,true);
}

void PluginEngineData::grabFocus()
{
}

bool PluginEngineData::getScreenData(SDL_DisplayMode *screen)
{
	if (SDL_GetDesktopDisplayMode(0, screen) != 0) {
		LOG(LOG_ERROR,"Capabilities: SDL_GetDesktopDisplayMode failed:"<<SDL_GetError());
		return false;
	}
	return true;
}

double PluginEngineData::getScreenDPI()
{
#if SDL_VERSION_ATLEAST(2, 0, 4)
		float ddpi;
		float hdpi;
		float vdpi;
		SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(widget),&ddpi,&hdpi,&vdpi);
		return ddpi;
#else
		LOG(LOG_NOT_IMPLEMENTED,"getScreenDPI needs SDL version >= 2.0.4");
		return 96.0;
#endif
}


NPError nsPluginInstance::SetWindow(NPWindow* aWindow)
{
	if(aWindow == NULL)
		return NPERR_GENERIC_ERROR;

	PluginEngineData* e = ((PluginEngineData*)m_sys->getEngineData());
	if (!e || !m_sys->mainClip)
		return NPERR_NO_ERROR;

	// only resize if the plugin is displayed larger than the size of the main clip
	if ((e->width != aWindow->width || e->height < aWindow->height) &&
			(e->origwidth<aWindow->width || e->origheight<aWindow->height))
	{
		Locker l(e->resizeMutex);
		if (e->mPixels)
		{
			delete[] e->mPixels;
			e->mPixels = NULL;
		}
		e->width=aWindow->width;
		e->height=aWindow->height;
		SDL_SetWindowSize(e->widget,e->width,e->height);
	}
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
			LOG(LOG_ERROR,"Cannot handle UTF8 URLs");
			return "";
		}
	}
	string ret(url.UTF8Characters, url.UTF8Length);
	NPN_ReleaseVariantValue(&variantValue);
	return ret;
}

void nsPluginInstance::asyncDownloaderDestruction(const char *url, NPDownloader* dl) const
{
	LOG(LOG_INFO,"Async destruction for " << url);
	m_sys->downloadManager->destroy(dl);
}

NPError nsPluginInstance::NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
{
	NPDownloader* dl=static_cast<NPDownloader*>(stream->notifyData);
	LOG(LOG_INFO,"Newstream for " << stream->url);
	setTLSSys( m_sys );
	setTLSWorker(m_sys->worker);
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
	else if(m_pt==nullptr)
	{
		//This is the main file
		m_sys->mainClip->setOrigin(stream->url);
		if (m_sys->getEngineData())
			((PluginEngineData*)m_sys->getEngineData())->setupLocalStorage();
		m_sys->parseParametersFromURL(m_sys->mainClip->getOrigin());
		*stype=NP_NORMAL;
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
		dl=new NPDownloader(stream->url,m_sys->mainClip->loaderInfo);
		dl->setLength(stream->end);
		mainDownloader=dl;
		mainDownloaderStreambuf = mainDownloader->getCache()->createReader();
		// loader is notified through parsethread
		mainDownloader->getCache()->setNotifyLoader(false);
		mainDownloaderStream.rdbuf(mainDownloaderStreambuf);
		m_pt=new lightspark::ParseThread(mainDownloaderStream,m_sys->mainClip);
		m_sys->addJob(m_pt);
	}
	//The downloader is set as the private data for this stream
	stream->pdata=dl;
	setTLSSys( nullptr );
	setTLSWorker(nullptr);
	return NPERR_NO_ERROR;
}

void nsPluginInstance::StreamAsFile(NPStream* stream, const char* fname)
{
	assert(stream->notifyData==nullptr);
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
		setTLSWorker(m_sys->worker);
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
		setTLSSys( nullptr );
		setTLSWorker(nullptr);
		return len;
	}
	else
		return len; //Drop everything (this is a second stream for the same SWF instance)
}

void nsPluginInstance::downloaderFinished(NPDownloader* dl, const char *url, NPReason reason) const
{
	setTLSSys(m_sys);
	setTLSWorker(m_sys->worker);
	//Check if async destruction of this downloader has been requested
	if(dl->state==NPDownloader::ASYNC_DESTROY)
	{
		dl->setFailed();
		asyncDownloaderDestruction(url, dl);
		setTLSSys(nullptr);
		setTLSWorker(nullptr);
		return;
	}
	dl->state=NPDownloader::STREAM_DESTROYED;

	//Notify our downloader of what happened
	switch(reason)
	{
		case NPRES_DONE:
			LOG(LOG_INFO,"Download complete " << url);
			dl->setFinished();
			break;
		case NPRES_USER_BREAK:
			LOG(LOG_ERROR,"Download stopped " << url);
			dl->setFailed();
			break;
		case NPRES_NETWORK_ERR:
			LOG(LOG_ERROR,"Download error " << url);
			dl->setFailed();
			break;
	}
	setTLSSys(nullptr);
	setTLSWorker(nullptr);
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

#ifdef None
#undef None
#endif

uint16_t nsPluginInstance::HandleEvent(void *event)
{
	using Button = LSMouseButtonEvent::Button;
	using ButtonType = LSMouseButtonEvent::ButtonType;
	using FocusType = LSWindowFocusEvent::FocusType;
	using KeyType = LSKeyEvent::KeyType;

	EngineData::mainloop_from_plugin(m_sys, &eventLoop);
#if defined(MOZ_X11)
	XEvent* nsEvent = (XEvent*)event;
	LSEventStorage ev;
	bool allowEvents =
	(
		m_sys->getEngineData() != nullptr &&
		m_sys->getEngineData()->widget != nullptr &&
		m_sys->currentVm != nullptr &&
		m_sys->currentVm->hasEverStarted()
	);
	switch (nsEvent->type)
	{
		case GraphicsExpose:
		{
			const XGraphicsExposeEvent& expose = nsEvent->xgraphicsexpose;
			this->mWindow = reinterpret_cast<XID>(expose.drawable);
			if (!m_sys->getEngineData())
			{
				PluginEngineData* e = new PluginEngineData(this, expose.width, expose.height,m_sys);
				m_sys->setParamsAndEngine(e, false);
			}
			((PluginEngineData*)m_sys->getEngineData())->draw(event, expose.x,expose.y,expose.width, expose.height);
			return true;
		}
		case KeyPress:
		case KeyRelease:
			if (allowEvents)
			{
				auto mousePos = m_sys->getInputThread()->getMousePos();
				LSModifier modifiers = LSModifier::None;
				AS3KeyCode key = x11GetAS3KeyCode(XLookupKeysym(&nsEvent->xkey,0));
				if (nsEvent->xkey.state & ControlMask)
					modifiers |= LSModifier::Ctrl;
				if (nsEvent->xkey.state & Mod1Mask)
					modifiers |= LSModifier::Alt;
				if (nsEvent->xkey.state & ShiftMask)
					modifiers |= LSModifier::Shift;
				ev = LSKeyEvent
				(
					mousePos,
					m_sys->windowToStagePoint(mousePos),
					key,
					key,
					modifiers,
					nsEvent->type == KeyPress ? KeyType::Down : KeyType::Up
				);
			}
			break;
		case MotionNotify: 
			if (m_sys->getEngineData() && m_sys->getEngineData()->widget && m_sys->currentVm && m_sys->currentVm->hasEverStarted())
			{
				XMotionEvent* event = &nsEvent->xmotion;
				Vector2f mousePos(event->x, event->y);
				ev = LSMouseMoveEvent
				(
					SDL_GetWindowID(m_sys->getEngineData()->widget),
					mousePos,
					m_sys->windowToStagePoint(mousePos),
					LSModifier::None,
					event->state & Button1Mask
				);
			}
			break;
		case ButtonPress:
		case ButtonRelease: 
			if (allowEvents)
			{
				XButtonEvent* event = &nsEvent->xbutton;
				Vector2f mousePos(event->x, event->y);

				bool isPressed = nsEvent->type == ButtonPress;
				auto buttonType = isPressed ? ButtonType::Down : ButtonType::Up;

				Button button;
				switch (event->button)
				{
					case 1: button = Button::Left; break;
					case 2: button = Button::Middle; break;
					case 3: button = Button::Right; break;
					default: button = Button::Invalid; break;
				}

				int clicks = 0;
				if (nsEvent->type == ButtonPress)
				{
					TimeSpec now = m_sys->now();
					auto delta = now - lastclicktime;
					if (delta < TimeSpec::fromMs(500)) // TODO get double click intervall from elsewehere instead of hard coded 500 milliseconds?
						clicks = 2;
					else
						clicks = 1;
					lastclicktime = now;
				}

				buttonType = clicks == 2 ? ButtonType::DoubleClick : buttonType;

				ev = LSMouseButtonEvent
				(
					SDL_GetWindowID(m_sys->getEngineData()->widget),
					mousePos,
					m_sys->windowToStagePoint(mousePos),
					LSModifier::None,
					isPressed,
					button,
					clicks,
					buttonType
				);
			}
			break;
		case FocusOut:
			if (allowEvents)
				ev = LSWindowFocusEvent(FocusType::Keyboard, false);
			break;
	}
	return EngineData::mainloop_handleevent(ev, m_sys);
#endif
#if defined XP_WIN
	
	NPEvent *npevent = reinterpret_cast<NPEvent *> (event);
	switch (npevent->event)
	{
		case WM_PAINT:
		{
			mDC = reinterpret_cast<HDC> (npevent->wParam);
			if (!mDC) {
				return false;
			}
			RECTL *clipRect = reinterpret_cast<RECTL *> (npevent->lParam);
			if (!m_sys->getEngineData())
			{
				PluginEngineData* e = new PluginEngineData(this, clipRect->right-clipRect->left, clipRect->bottom-clipRect->top,m_sys);
				m_sys->setParamsAndEngine(e, false);
			}
			int savedID = SaveDC(mDC);
			((PluginEngineData*)m_sys->getEngineData())->draw(event,clipRect->left,clipRect->top,clipRect->right-clipRect->left, clipRect->bottom-clipRect->top);
			RestoreDC(mDC, savedID);
			return true;
		}
	}
#endif
	return 0;
}

void PluginEventLoop::notify()
{
	NPN_PluginThreadAsyncCall(instance->getInstance(), [](void* data)
	{
		PluginEventLoop* eventLoop = static_cast<PluginEventLoop*>(data);
		EngineData::mainloop_from_plugin(eventLoop->instance->m_sys, eventLoop);
	}, this);
}

PluginEngineData::PluginEngineData(nsPluginInstance *i, uint32_t w, uint32_t h, SystemState *_sys) : instance(i),inputHandlerId(0),sizeHandlerId(0),sys(_sys)
{
	width = w;
	height = h;
	mPixels = nullptr;
	inRendering = false;
	winposx=0;
	winposy=0;
	if (sys->mainClip->getOrigin().isValid())
		setupLocalStorage();
}

void PluginEngineData::setupLocalStorage()
{
	std::string filename = sys->mainClip->getOrigin().getPathDirectory();
	std::replace(filename.begin(),filename.end(),'/',G_DIR_SEPARATOR);
	filename+= G_DIR_SEPARATOR;
	filename += sys->mainClip->getOrigin().getPathFile();
	std::string filedatapath = sys->mainClip->getOrigin().getHostname() + G_DIR_SEPARATOR_S;
	filedatapath += filename;
	std::replace(filedatapath.begin(),filedatapath.end(),':','_');
	std::replace(filedatapath.begin(),filedatapath.end(),'.','_');
	sharedObjectDatapath = Config::getConfig()->getCacheDirectory();
	sharedObjectDatapath += G_DIR_SEPARATOR;
	sharedObjectDatapath += "data";
	sharedObjectDatapath += G_DIR_SEPARATOR;
	sharedObjectDatapath += filedatapath;
}

void PluginEngineData::stopMainDownload()
{
	if(instance->mainDownloader)
		instance->mainDownloader->stop();
}

uint32_t PluginEngineData::getWindowForGnash()
{
	return instance->mWindow;
}

struct linkOpenData
{
	NPP instance;
	tiny_string url;
	tiny_string window;
};

void nsPluginInstance::openLink(const tiny_string& url, const tiny_string& window)
{
	assert(!window.empty());

	linkOpenData *data = new linkOpenData;
	data->instance = mInstance;
	data->url = url;
	data->window = window;
	NPN_PluginThreadAsyncCall(mInstance, asyncOpenPage, data);
}

void nsPluginInstance::asyncOpenPage(void *data)
{
	linkOpenData *page = (linkOpenData *)data;

	NPError e = NPN_GetURL(page->instance, page->url.raw_buf(), page->window.raw_buf());
	if (e != NPERR_NO_ERROR)
		LOG(LOG_ERROR, "Failed to open a page in the browser");
	
	delete page;
}

void PluginEngineData::openPageInBrowser(const tiny_string& url, const tiny_string& window)
{
	instance->openLink(url, window);
}

void PluginEngineData::forceRedraw(SystemState* sys)
{
   NPRect rect;
   rect.left = 0;
   rect.top = 0;
   rect.right = ((PluginEngineData*)sys->getEngineData())->width;
   rect.bottom = ((PluginEngineData*)sys->getEngineData())->height;
   NPN_InvalidateRect(((PluginEngineData*)sys->getEngineData())->instance->mInstance, &rect);
   NPN_ForceRedraw(((PluginEngineData*)sys->getEngineData())->instance->mInstance);
}

void PluginEngineData::runInTrueMainThread(SystemState* sys, MainThreadCallback func)
{
	struct tmp
	{
		SystemState* sys;
		MainThreadCallback func;
	};
	tmp* _tmp = new tmp { sys, func };
	NPN_PluginThreadAsyncCall(instance->mInstance, [](void* data)
	{
		tmp* _tmp = static_cast<tmp*>(data);
		_tmp->func(_tmp->sys);
		delete _tmp;
	}, _tmp);
}

void PluginEngineData::runInMainThread(SystemState* sys, MainThreadCallback func)
{
	EngineData::runInMainThread(sys,func);
}

void PluginEngineData::openContextMenu()
{
	EngineData::openContextMenu();
	if (!inFullScreenMode())
		startSDLEventTicker(sys);
}

void PluginEngineData::DoSwapBuffers()
{
	if (inRendering)
	{
		SDL_GL_SwapWindow(widget);
		return;
	}
	{
		Locker l(resizeMutex);
		inRendering = true;
		if (!mPixels)
			mPixels = new unsigned char[width*height*4]; // 4 bytes for BGRA
		char* buf = g_newa(char, width*height*4);

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0,0,width, height, GL_BGRA, GL_UNSIGNED_BYTE, buf);
		// received image is upside down, so flip it vertically
		for (uint32_t i= 0; i < height; i++)
		{
			memcpy(mPixels+i*width*4,buf+(height-i)*width*4,width*4);
		}
		SDL_GL_SwapWindow(widget);
		runInMainThread(instance->m_sys,forceRedraw);
	}
}
void PluginEngineData::InitOpenGL()
{
	mSDLContext = SDL_GL_CreateContext(widget);
	if (!mSDLContext)
		LOG(LOG_ERROR,"failed to create openGL context:"<<SDL_GetError());
	initGLEW();
	return;
}

void PluginEngineData::DeinitOpenGL()
{
	SDL_GL_DeleteContext(mSDLContext);
}
void PluginEngineData::draw(void *event,uint32_t evx, uint32_t evy, uint32_t evwidth, uint32_t evheight)
{
	if (!mPixels)
	{
		inRendering=false;
		return;
	}
#if defined MOZ_X11
	Display *dpy = ((XEvent*)event)->xexpose.display;
	int screen = DefaultScreen(dpy);
	Visual* vi = DefaultVisual(dpy,screen);
	XImage *xi = XCreateImage(dpy, vi, 24, ZPixmap, 0,(char*)mPixels, width, height, 32, 4*width);
	XPutImage(dpy,((XEvent*)event)->xexpose.window,DefaultGC(dpy, screen), xi, 0, 0, evx, evy, evwidth, evheight);
	XFree(xi);
#endif
#if defined XP_WIN
	// taken from https://code.videolan.org/videolan/npapi-vlc/blob/master/npapi/vlcwindowless_win.cpp
	BITMAPINFO BmpInfo; ZeroMemory(&BmpInfo, sizeof(BmpInfo));
	BITMAPINFOHEADER& BmpH = BmpInfo.bmiHeader;
	BmpH.biSize = sizeof(BITMAPINFOHEADER);
	BmpH.biWidth = width;
	BmpH.biHeight = -((int)height);
	BmpH.biPlanes = 1;
	BmpH.biBitCount = 4*8;
	BmpH.biCompression = BI_RGB;
	//following members are already zeroed
	//BmpH.biSizeImage = 0;
	//BmpH.biXPelsPerMeter = 0;
	//BmpH.biYPelsPerMeter = 0;
	//BmpH.biClrUsed = 0;
	//BmpH.biClrImportant = 0;
	SetDIBitsToDevice(instance->mDC,evx,evy,evwidth,evheight,0,0,0,evheight,mPixels,&BmpInfo, DIB_RGB_COLORS);
#endif
	inRendering=false;
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
