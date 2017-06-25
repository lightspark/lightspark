/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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
#include "backends/security.h"
#include "backends/streamcache.h"
#include "plugin/plugin.h"
#include "logger.h"
#include "compat.h"
#include <string>
#include <algorithm>
#ifdef _WIN32
#	include <gdk/gdkwin32.h>
#	include <wingdi.h>
#endif
#include "backends/urlutils.h"

#include "plugin/npscriptobject.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <gtk/gtk.h>
#ifndef _WIN32
#include <gdk/gdkx.h>
#endif
#if GTK_CHECK_VERSION (2,21,8)
#include <gdk/gdkkeysyms-compat.h> 
#else
#include <gdk/gdkkeysyms.h>
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
	LOG(LOG_INFO, _("NET: PLUGIN: DownloadManager::download '") << url.getParsedURL() << 
			"'" << (cached ? _(" - cached") : ""));
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

	LOG(LOG_INFO, _("NET: PLUGIN: DownloadManager::downloadWithData '") << url.getParsedURL());
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
	setTLSSys( NULL );
}

////////////////////////////////////////
//
// nsPluginInstance class implementation
//
nsPluginInstance::nsPluginInstance(NPP aInstance, int16_t argc, char** argn, char** argv) : 
	nsPluginInstanceBase(), mInstance(aInstance),mInitialized(FALSE),mWindow(0),
	mainDownloaderStreambuf(NULL),mainDownloaderStream(NULL),
	mainDownloader(NULL),scriptObject(NULL),m_pt(NULL)
{
	LOG(LOG_INFO, "Lightspark version " << VERSION << " Copyright 2009-2013 Alessandro Pignotti and others");
	setTLSSys( NULL );
	m_sys=new lightspark::SystemState(0, lightspark::SystemState::FLASH);
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
			//The SWF file url should be getted from NewStream
		}
		//basedir is a qualified URL or a path relative to the HTML page
		URLInfo page(getPageURL());
		m_sys->mainClip->setBaseURL(page.goToURL(baseURL).getURL());

		m_sys->downloadManager=new NPDownloadManager(mInstance);
	}
	else
		LOG(LOG_ERROR, "PLUGIN: Browser doesn't support NPRuntime");
	EngineData::mainthread_running = true;
	g_idle_add((GSourceFunc)EngineData::mainloop_from_plugin,m_sys);
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
	if (mainDownloaderStreambuf)
		delete mainDownloaderStreambuf;

	// Kill all stuff relating to NPScriptObject which is still running
	m_sys->extScriptObject->destroy();

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
			err = NPERR_INVALID_PARAM;
			break;
		default:
			err = NPERR_INVALID_PARAM;
			break;
	}
	return err;

}
SDL_Keycode getSDLKeyCode(unsigned gdkKeyval)
{
	switch (gdkKeyval)
	{
		case GDK_a: return SDLK_a;
		case GDK_Alt_L: return SDLK_LALT;
		case GDK_b: return SDLK_b;
		case GDK_Back: return SDLK_AC_BACK;
		case GDK_grave: return SDLK_BACKQUOTE;
		case GDK_backslash: return SDLK_BACKSLASH;
		case GDK_BackSpace: return SDLK_BACKSPACE;
		case GDK_Blue: return SDLK_UNKNOWN; // TODO
		case GDK_c: return SDLK_c;
		case GDK_Caps_Lock: return SDLK_CAPSLOCK;
		case GDK_comma: return SDLK_COMMA;
		case GDK_Control_L: return SDLK_LCTRL;
		case GDK_d: return SDLK_d;
		case GDK_Delete: return SDLK_DELETE;
		case GDK_Down: return SDLK_DOWN;
		case GDK_e: return SDLK_e;
		case GDK_End: return SDLK_END;
		case GDK_Return: return SDLK_RETURN;
		case GDK_equal: return SDLK_EQUALS;
		case GDK_Escape: return SDLK_ESCAPE;
		case GDK_f: return SDLK_f;
		case GDK_F1: return SDLK_F1;
		case GDK_F2: return SDLK_F2;
		case GDK_F3: return SDLK_F3;
		case GDK_F4: return SDLK_F4;
		case GDK_F5: return SDLK_F5;
		case GDK_F6: return SDLK_F6;
		case GDK_F7: return SDLK_F7;
		case GDK_F8: return SDLK_F8;
		case GDK_F9: return SDLK_F9;
		case GDK_F10: return SDLK_F10;
		case GDK_F11: return SDLK_F11;
		case GDK_F12: return SDLK_F12;
		case GDK_F13: return SDLK_F13;
		case GDK_F14: return SDLK_F14;
		case GDK_F15: return SDLK_F15;
		case GDK_g: return SDLK_g;
		case GDK_Green: return SDLK_UNKNOWN; // TODO
		case GDK_h: return SDLK_h;
		case GDK_Help: return SDLK_HELP;
		case GDK_Home: return SDLK_HOME;
		case GDK_i: return SDLK_i;
		case GDK_Insert: return SDLK_INSERT;
		case GDK_j: return SDLK_j;
		case GDK_k: return SDLK_k;
		case GDK_l: return SDLK_l;
		case GDK_Left: return SDLK_LEFT;
		case GDK_bracketleft: return SDLK_LEFTBRACKET;
		case GDK_m: return SDLK_m;
		case GDK_minus: return SDLK_MINUS;
		case GDK_n: return SDLK_n;
		case GDK_0: return SDLK_0;
		case GDK_1: return SDLK_1;
		case GDK_2: return SDLK_2;
		case GDK_3: return SDLK_3;
		case GDK_4: return SDLK_4;
		case GDK_5: return SDLK_5;
		case GDK_6: return SDLK_6;
		case GDK_7: return SDLK_7;
		case GDK_8: return SDLK_8;
		case GDK_9: return SDLK_9;
		case GDK_KP_0: return SDLK_KP_0;
		case GDK_KP_1: return SDLK_KP_1;
		case GDK_KP_2: return SDLK_KP_2;
		case GDK_KP_3: return SDLK_KP_3;
		case GDK_KP_4: return SDLK_KP_4;
		case GDK_KP_5: return SDLK_KP_5;
		case GDK_KP_6: return SDLK_KP_6;
		case GDK_KP_7: return SDLK_KP_7;
		case GDK_KP_8: return SDLK_KP_8;
		case GDK_KP_9: return SDLK_KP_9;
		case GDK_KP_Add: return SDLK_KP_MEMADD;
		case GDK_KP_Separator: return SDLK_KP_PERIOD; // TODO
		case GDK_KP_Divide: return SDLK_KP_DIVIDE;
		case GDK_KP_Enter: return SDLK_KP_ENTER;
		case GDK_KP_Multiply: return SDLK_KP_MULTIPLY;
		case GDK_KP_Subtract: return SDLK_KP_MINUS;
		case GDK_o: return SDLK_o;
		case GDK_p: return SDLK_p;
		case GDK_Page_Down: return SDLK_PAGEDOWN;
		case GDK_Page_Up: return SDLK_PAGEUP;
		case GDK_Pause: return SDLK_PAUSE;
		case GDK_period: return SDLK_PERIOD;
		case GDK_q: return SDLK_q;
		case GDK_quoteright: return SDLK_QUOTE;
		case GDK_r: return SDLK_r;
		case GDK_Red: return SDLK_UNKNOWN; // TODO
		case GDK_Right: return SDLK_RIGHT;
		case GDK_bracketright: return SDLK_RIGHTBRACKET;
		case GDK_s: return SDLK_s;
		case GDK_Search: return SDLK_AC_SEARCH;
		case GDK_semicolon: return SDLK_SEMICOLON;
		case GDK_Shift_L: return SDLK_LSHIFT;
		case GDK_slash: return SDLK_SLASH;
		case GDK_space: return SDLK_SPACE;
		case GDK_Subtitle: return SDLK_UNKNOWN; // TODO
		case GDK_t: return SDLK_t;
		case GDK_Tab: return SDLK_TAB;
		case GDK_u: return SDLK_u;
		case GDK_Up: return SDLK_UP;
		case GDK_v: return SDLK_v;
		case GDK_w: return SDLK_w;
		case GDK_x: return SDLK_x;
		case GDK_y: return SDLK_y;
		case GDK_Yellow: return SDLK_UNKNOWN; // TODO
		case GDK_z: return SDLK_z;
	}
	return SDLK_UNKNOWN;
};
static uint16_t getKeyModifier(GdkEvent *event)
{
	uint16_t res = KMOD_NONE;
	if (event->key.state & GDK_CONTROL_MASK)
		res |= KMOD_CTRL;
	if (event->key.state & GDK_MOD1_MASK)
		res |= KMOD_ALT;
	if (event->key.state & GDK_SHIFT_MASK)
		res |= KMOD_SHIFT;
	return res;
}

static gboolean inputDispatch(GtkWidget *widget, GdkEvent *event, PluginEngineData* e)
{
	SDL_Event ev;
	switch (event->type)
	{
		case GDK_EXPOSE:
		{
			ev.type = SDL_WINDOWEVENT;
			ev.window.event = SDL_WINDOWEVENT_EXPOSED;
			ev.window.windowID = SDL_GetWindowID(e->widget);
			break;
		}
		case GDK_KEY_PRESS:
		{
			ev.type = SDL_KEYDOWN;
			ev.key.keysym.sym = getSDLKeyCode(event->key.keyval);
			ev.key.keysym.mod = getKeyModifier(event);
			SDL_SetModState((SDL_Keymod)ev.key.keysym.mod);
			ev.key.windowID = SDL_GetWindowID(e->widget);
			break;
		}
		case GDK_KEY_RELEASE:
		{
			ev.type = SDL_KEYUP;
			ev.key.keysym.sym = getSDLKeyCode(event->key.keyval);
			ev.key.keysym.mod = getKeyModifier(event);
			SDL_SetModState((SDL_Keymod)ev.key.keysym.mod);
			ev.key.windowID = SDL_GetWindowID(e->widget);
			break;
		}
		case GDK_BUTTON_PRESS:
		{
			ev.type = SDL_MOUSEBUTTONDOWN;
			switch (event->button.button)
			{
				case 1:
					ev.button.button = SDL_BUTTON_LEFT;
					ev.button.state = event->button.state & GDK_BUTTON1_MASK ? SDL_PRESSED : SDL_RELEASED;
					break;
				case 2:
					ev.button.button = SDL_BUTTON_RIGHT;
					ev.button.state = event->button.state & GDK_BUTTON2_MASK ? SDL_PRESSED : SDL_RELEASED;
					break;
				default:
					ev.button.button = 0;
					ev.button.state = SDL_RELEASED;
					break;
			}
			ev.button.clicks = 1;
			ev.button.x = event->button.x;
			ev.button.y = event->button.y;
			ev.button.windowID = SDL_GetWindowID(e->widget);
			break;
		}
		case GDK_2BUTTON_PRESS:
		{
			ev.type = SDL_MOUSEBUTTONDOWN;
			switch (event->button.button)
			{
				case 1:
					ev.button.button = SDL_BUTTON_LEFT;
					ev.button.state = event->button.state & GDK_BUTTON1_MASK ? SDL_PRESSED : SDL_RELEASED;
					break;
				case 2:
					ev.button.button = SDL_BUTTON_RIGHT;
					ev.button.state = event->button.state & GDK_BUTTON2_MASK ? SDL_PRESSED : SDL_RELEASED;
					break;
				default:
					ev.button.button = 0;
					ev.button.state = SDL_RELEASED;
					break;
			}
			ev.button.clicks = 2;
			ev.button.x = event->button.x;
			ev.button.y = event->button.y;
			ev.button.windowID = SDL_GetWindowID(e->widget);
			break;
		}
		case GDK_BUTTON_RELEASE:
		{
			ev.type = SDL_MOUSEBUTTONUP;
			switch (event->button.button)
			{
				case 1:
					ev.button.button = SDL_BUTTON_LEFT;
					ev.button.state = event->button.state & GDK_BUTTON1_MASK ? SDL_PRESSED : SDL_RELEASED;
					break;
				case 2:
					ev.button.button = SDL_BUTTON_RIGHT;
					ev.button.state = event->button.state & GDK_BUTTON2_MASK ? SDL_PRESSED : SDL_RELEASED;
					break;
				default:
					ev.button.button = 0;
					ev.button.state = SDL_RELEASED;
					break;
			}
			ev.button.clicks = 0;
			ev.button.x = event->button.x;
			ev.button.y = event->button.y;
			ev.button.windowID = SDL_GetWindowID(e->widget);
			break;
		}
		case GDK_MOTION_NOTIFY:
		{
			ev.type = SDL_MOUSEMOTION;
			ev.motion.state = event->motion.state & GDK_BUTTON1_MASK ? SDL_PRESSED : SDL_RELEASED;
			ev.motion.x = event->motion.x;
			ev.motion.y = event->motion.y;
			ev.motion.windowID = SDL_GetWindowID(e->widget);
			break;
		}
		case GDK_SCROLL:
		{
			ev.type = SDL_MOUSEWHEEL;
#if SDL_VERSION_ATLEAST(2, 0, 4)
			ev.wheel.direction = event->scroll.state == GDK_SCROLL_UP ? SDL_MOUSEWHEEL_NORMAL : SDL_MOUSEWHEEL_FLIPPED ;
#endif
			ev.wheel.x = event->scroll.x;
			ev.wheel.y = event->scroll.y;
			ev.wheel.windowID = SDL_GetWindowID(e->widget);
			break;
		}
		case GDK_LEAVE_NOTIFY:
		{
			ev.type = SDL_WINDOWEVENT_LEAVE;
			ev.wheel.windowID = SDL_GetWindowID(e->widget);
			break;
		}
		default:
#ifdef EXPENSIVE_DEBUG
			LOG(LOG_INFO, "GDKTYPE " << event->type);
#endif
			return false;
	}
	EngineData::mainloop_handleevent(&ev,e->sys);
	return true;
}
static void sizeDispatch(GtkWidget* widget, GdkRectangle* allocation, PluginEngineData* e)
{
	SDL_Event ev;
	ev.type = SDL_WINDOWEVENT;
	ev.window.event = SDL_WINDOWEVENT_SIZE_CHANGED;
	ev.window.data1 = allocation->width;
	ev.window.data2 = allocation->height;
	ev.window.windowID = SDL_GetWindowID(e->widget);
	
	EngineData::mainloop_handleevent(&ev,e->sys);
}


SDL_Window* PluginEngineData::createWidget(uint32_t w,uint32_t h)
{
	widget_gtk = gtk_plug_new(instance->mWindow);
	gdk_threads_enter();
	gtk_widget_realize(widget_gtk);
#ifndef _WIN32
	windowID = GDK_WINDOW_XID(gtk_widget_get_window(widget_gtk));
#endif
	
	gtk_widget_set_size_request(widget_gtk, w, h);
	gtk_widget_show(widget_gtk);
	gtk_widget_map(widget_gtk);
	gtk_widget_set_can_focus(widget_gtk,TRUE);
	gtk_widget_add_events(widget_gtk,
		GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
		GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
		GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK |
		GDK_LEAVE_NOTIFY_MASK);
	inputHandlerId = g_signal_connect(widget_gtk, "event", G_CALLBACK(inputDispatch), this);
	sizeHandlerId = g_signal_connect(widget_gtk, "size-allocate", G_CALLBACK(sizeDispatch), this);
#ifdef _WIN32
	SDL_Window* sdlwin = SDL_CreateWindowFrom((const void*)gtk_widget_get_window(widget_gtk));
#else
	SDL_Window* sdlwin = SDL_CreateWindowFrom((const void*)gdk_x11_drawable_get_xid(gtk_widget_get_window(widget_gtk)));
#endif
	gdk_threads_leave();
	return sdlwin;
}

void PluginEngineData::grabFocus()
{
	if (!widget_gtk)
		return;

	gtk_widget_grab_focus(widget_gtk);
}

void PluginEngineData::setClipboardText(const std::string txt)
{
	GtkClipboard *clipboard;
	clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clipboard, txt.c_str(),txt.size());
	clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
	gtk_clipboard_set_text(clipboard, txt.c_str(), txt.size());
	LOG(LOG_INFO, "Copied error to clipboard");
}

bool PluginEngineData::getScreenData(SDL_DisplayMode *screen)
{
	GdkScreen*  s = gdk_screen_get_default();
	screen->w = gdk_screen_get_width (s);
	screen->h = gdk_screen_get_height (s);
	return true;
}

double PluginEngineData::getScreenDPI()
{
	GdkScreen*  screen = gdk_screen_get_default();
	return gdk_screen_get_resolution (screen);
}


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

	PluginEngineData* e = new PluginEngineData(this, width, height,m_sys);
	mWindow = win;

	LOG(LOG_INFO,"From Browser: Window " << mWindow << " Width: " << width << " Height: " << height);

#ifndef _WIN32
	NPSetWindowCallbackStruct *ws_info = (NPSetWindowCallbackStruct *)aWindow->ws_info;
	//mDisplay = ws_info->display;
	//mDepth = ws_info->depth;
	//mColormap = ws_info->colormap;
	if (ws_info->visual)
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
		mainDownloaderStreambuf = mainDownloader->getCache()->createReader();
		// loader is notified through parsethread
		mainDownloader->getCache()->setNotifyLoader(false);
		mainDownloaderStream.rdbuf(mainDownloaderStreambuf);
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

uint32_t PluginEngineData::getWindowForGnash()
{
	return (uint32_t)instance->mWindow;
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
		LOG(LOG_ERROR, _("Failed to open a page in the browser"));
	
	delete page;
}

void PluginEngineData::openPageInBrowser(const tiny_string& url, const tiny_string& window)
{
	instance->openLink(url, window);
}
void PluginEngineData::DoSwapBuffers()
{
#if defined(_WIN32)
	SwapBuffers(mDC);
#elif !defined(ENABLE_GLES2)
	glXSwapBuffers(mDisplay, windowID);
#else
	eglSwapBuffers(mEGLDisplay, mEGLSurface);
#endif
}
void PluginEngineData::InitOpenGL()
{
#if defined(_WIN32)
	PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
			PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
			32,                        //Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                        //Number of bits for the depthbuffer
			0,                        //Number of bits for the stencilbuffer
			0,                        //Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};
	if(!(mDC = GetDC((HWND)GDK_WINDOW_HWND(gtk_widget_get_window(widget_gtk)))))
		throw RunTimeException("GetDC failed");
	int PixelFormat;
	if (!(PixelFormat=ChoosePixelFormat(mDC,&pfd)))
		throw RunTimeException("ChoosePixelFormat failed");
	if(!SetPixelFormat(mDC,PixelFormat,&pfd))
		throw RunTimeException("SetPixelFormat failed");
	if (!(mRC=wglCreateContext(mDC)))
		throw RunTimeException("wglCreateContext failed");
	if(!wglMakeCurrent(mDC,mRC))
		throw RunTimeException("wglMakeCurrent failed");
#elif !defined(ENABLE_GLES2)
	mDisplay = XOpenDisplay(NULL);
	int a,b;
	Bool glx_present=glXQueryVersion(mDisplay, &a, &b);
	if(!glx_present)
	{
		XCloseDisplay(mDisplay);
		throw RunTimeException("glX not present");
	}

	int attrib[10]={GLX_DOUBLEBUFFER, True, 0L};
	GLXFBConfig* fb=glXChooseFBConfig(mDisplay, 0, attrib, &a);
	if(!fb)
	{
		attrib[6]=0L;
		LOG(LOG_ERROR,_("Falling back to no double buffering"));
		fb=glXChooseFBConfig(mDisplay, 0, attrib, &a);
	}
	if(!fb)
	{
		XCloseDisplay(mDisplay);
		throw RunTimeException(_("Could not find any GLX configuration"));
	}
	int i;
	for(i=0;i<a;i++)
	{
		int id;
		glXGetFBConfigAttrib(mDisplay, fb[i],GLX_VISUAL_ID,&id);
		if(visual == 0 || id==(int)visual)
			break;
	}
	if(i==a)
	{
		//No suitable id found
		XCloseDisplay(mDisplay);
		throw RunTimeException(_("No suitable graphics configuration available"));
	}
	mFBConfig=fb[i];
	LOG(LOG_INFO, "Chosen config " << hex << fb[i] << dec);
	XFree(fb);
		mContext = glXCreateNewContext(mDisplay, mFBConfig,GLX_RGBA_TYPE ,NULL,1);
	glXMakeCurrent(mDisplay, windowID, mContext);
	if(!glXIsDirect(mDisplay, mContext))
		LOG(LOG_INFO, "Indirect!!");
#else //egl
	mDisplay = XOpenDisplay(NULL);
	int a;
	eglBindAPI(EGL_OPENGL_ES_API);
	mEGLDisplay = eglGetDisplay(mDisplay);
	if (mEGLDisplay == EGL_NO_DISPLAY)
		throw RunTimeException(_("EGL not present"));
	EGLint major, minor;
	if (eglInitialize(mEGLDisplay, &major, &minor) == EGL_FALSE)
		throw RunTimeException(_("EGL initialization failed"));

	LOG(LOG_INFO, _("EGL version: ") << eglQueryString(mEGLDisplay, EGL_VERSION));
	EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
		};
	EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	if (!eglChooseConfig(mEGLDisplay, config_attribs, 0, 0, &a))
		throw RunTimeException(_("Could not get number of EGL configurations"));
	else
		LOG(LOG_INFO, "Number of EGL configurations: " << a);
	EGLConfig *conf = new EGLConfig[a];
	if (!eglChooseConfig(mEGLDisplay, config_attribs, conf, a, &a))
		throw RunTimeException(_("Could not find any EGL configuration"));

	int i;
	for(i=0;i<a;i++)
	{
		EGLint id;
		eglGetConfigAttrib(mEGLDisplay, conf[i], EGL_NATIVE_VISUAL_ID, &id);
		if(visual == 0 || id==(int)visual)
			break;
	}
	if(i==a)
	{
		//No suitable id found
		throw RunTimeException(_("No suitable graphics configuration available"));
	}
	mEGLConfig=conf[i];
	LOG(LOG_INFO, "Chosen config " << hex << conf[i] << dec);
	mEGLContext = eglCreateContext(mEGLDisplay, mEGLConfig, EGL_NO_CONTEXT, context_attribs);
	if (mEGLContext == EGL_NO_CONTEXT)
		throw RunTimeException(_("Could not create EGL context"));
	mEGLSurface = eglCreateWindowSurface(mEGLDisplay, mEGLConfig, windowID, NULL);
	if (mEGLSurface == EGL_NO_SURFACE)
		throw RunTimeException(_("Could not create EGL surface"));
	eglMakeCurrent(mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext);
#endif

	initGLEW();
}

void PluginEngineData::DeinitOpenGL()
{
#if defined(_WIN32)
	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(mRC);
	/* Do not ReleaseDC(e->window,hDC); as our window does not have CS_OWNDC */
#elif !defined(ENABLE_GLES2)
	glXMakeCurrent(mDisplay, 0L, NULL);
	glXDestroyContext(mDisplay, mContext);
	XCloseDisplay(mDisplay);
#else
	eglMakeCurrent(mEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(mEGLDisplay, mEGLContext);
	XCloseDisplay(mDisplay);
#endif
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
