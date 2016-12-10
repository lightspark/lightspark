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


// TODO
// - download
// - rendering
// - javascript communication with browser
// - sound
// - keyboard/mouse handling
// - run within sandbox
// - register as separate plugin

#include "version.h"
#include "logger.h"
#include "compat.h"
#include "swf.h"
#include "backends/security.h"
#include "backends/rendering.h"
#include <string>
#include <algorithm>
#include "threading.h"
#include "plugin_ppapi/plugin.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppb_view.h"
#include "ppapi/c/ppb_url_loader.h"
#include "ppapi/c/ppb_url_request_info.h"
#include "ppapi/c/ppb_url_response_info.h"
#include "ppapi/c/trusted/ppb_url_loader_trusted.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"
#include "ppapi/c/ppb_opengles2.h"
#include "ppapi/c/ppb_graphics_3d.h"
#include "GLES2/gl2.h"

//The interpretation of texture data change with the endianness
#if __BYTE_ORDER == __BIG_ENDIAN
#define GL_UNSIGNED_INT_8_8_8_8_HOST GL_UNSIGNED_INT_8_8_8_8_REV
#else
#define GL_UNSIGNED_INT_8_8_8_8_HOST GL_UNSIGNED_BYTE
#endif

using namespace lightspark;
using namespace std;

static PPB_GetInterface g_get_browser_interface = NULL;
static const PPB_Core* g_core_interface = NULL;
static const PPB_Graphics3D* g_graphics_3d_interface = NULL;
static const PPB_Instance* g_instance_interface = NULL;
static const PPB_View* g_view_interface = NULL;
static const PPB_Var* g_var_interface = NULL;
static const PPB_URLLoader* g_urlloader_interface = NULL;
static const PPB_URLRequestInfo* g_urlrequestinfo_interface = NULL;
static const PPB_URLResponseInfo* g_urlresponseinfo_interface = NULL;
static const PPB_OpenGLES2* g_gles2_interface = NULL;
static const PPB_URLLoaderTrusted* g_urlloadedtrusted_interface = NULL;

ppDownloadManager::ppDownloadManager(PP_Instance _instance, SystemState *sys):instance(_instance),m_sys(sys)
{
	type = NPAPI;
}

lightspark::Downloader* ppDownloadManager::download(const lightspark::URLInfo& url, _R<StreamCache> cache, lightspark::ILoadable* owner)
{
	// empty URL means data is generated from calls to NetStream::appendBytes
	if(!url.isValid() && url.getInvalidReason() == URLInfo::IS_EMPTY)
	{
		return StandaloneDownloadManager::download(url, cache, owner);
	}
	// Handle RTMP requests internally, not through PPAPI
	if(url.isRTMP())
	{
		return StandaloneDownloadManager::download(url, cache, owner);
	}

	bool cached = false;
	LOG(LOG_INFO, _("NET: PLUGIN: DownloadManager::download '") << url.getParsedURL() << 
			"'" << (cached ? _(" - cached") : ""));
	//Register this download
	ppDownloader* downloader=new ppDownloader(url.getParsedURL(), cache, instance, owner);
	addDownloader(downloader);
	return downloader;
}
lightspark::Downloader* ppDownloadManager::downloadWithData(const lightspark::URLInfo& url,
		_R<StreamCache> cache, const std::vector<uint8_t>& data,
		const std::list<tiny_string>& headers, lightspark::ILoadable* owner)
{
	// Handle RTMP requests internally, not through PPAPI
	if(url.isRTMP())
	{
		return StandaloneDownloadManager::downloadWithData(url, cache, data, headers, owner);
	}

	LOG(LOG_INFO, _("NET: PLUGIN: DownloadManager::downloadWithData '") << url.getParsedURL());
	//Register this download
	ppDownloader* downloader=new ppDownloader(url.getParsedURL(), cache, data, headers, instance, owner);
	addDownloader(downloader);
	return downloader;
}

void ppDownloadManager::destroy(lightspark::Downloader* downloader)
{
	ppDownloader* d=dynamic_cast<ppDownloader*>(downloader);
	if(!d)
	{
		StandaloneDownloadManager::destroy(downloader);
		return;
	}
	if(d->state!=ppDownloader::STREAM_DESTROYED && d->state!=ppDownloader::ASYNC_DESTROY)
	{
		//The NP stream is still alive. Flag this downloader for aync destruction
		d->state=ppDownloader::ASYNC_DESTROY;
		return;
	}
	//If the downloader was still in the active-downloader list, delete it
	if(removeDownloader(downloader))
	{
		downloader->waitForTermination();
		delete downloader;
	}
}


void ppDownloader::dlStartCallback(void* userdata,int result)
{
	ppDownloader* th = (ppDownloader*)userdata;
	if (result < 0)
	{
		LOG(LOG_ERROR,"download failed:"<<result<<" "<<th->getURL());
		th->setFailed();
		return;
	}
	PP_Resource response = g_urlloader_interface->GetResponseInfo(th->ppurlloader);
	PP_Var v;
	uint32_t len;
	v = g_urlresponseinfo_interface->GetProperty(response,PP_URLRESPONSEPROPERTY_STATUSCODE);
	LOG(LOG_INFO,"statuscode:"<<v.value.as_int);
	v = g_urlresponseinfo_interface->GetProperty(response,PP_URLRESPONSEPROPERTY_STATUSLINE);
	tiny_string statusline = g_var_interface->VarToUtf8(v,&len);
	LOG(LOG_INFO,"statusline:"<<statusline);
	v = g_urlresponseinfo_interface->GetProperty(response,PP_URLRESPONSEPROPERTY_HEADERS);
	tiny_string headers = g_var_interface->VarToUtf8(v,&len);
	LOG(LOG_INFO,"headers:"<<len<<" "<<headers);
	th->parseHeaders(headers.raw_buf(),true);
	
	if (th->isMainClipDownloader)
	{
		v = g_urlresponseinfo_interface->GetProperty(response,PP_URLRESPONSEPROPERTY_URL);
		tiny_string url = g_var_interface->VarToUtf8(v,&len);
		LOG(LOG_INFO,"mainclip url:"<<url);
		
		th->m_sys->mainClip->setOrigin(url);
		th->m_sys->parseParametersFromURL(th->m_sys->mainClip->getOrigin());
		
		th->m_sys->mainClip->setBaseURL(url);
	}
	 
	struct PP_CompletionCallback cb;
	cb.func = dlReadResponseCallback;
	cb.flags = 0;
	cb.user_data = th;
	g_urlloader_interface->ReadResponseBody(th->ppurlloader,th->buffer,4096,cb);
}
void ppDownloader::dlReadResponseCallback(void* userdata,int result)
{
	ppDownloader* th = (ppDownloader*)userdata;
	if (result < 0)
	{
		LOG(LOG_ERROR,"download failed:"<<result<<" "<<th->getURL()<<" "<<th->getReceivedLength()<<"/"<<th->getLength());
		th->setFailed();
		return;
	}
	bool haslength = th->getReceivedLength() < th->getLength();
	th->append(th->buffer,result);
	if ((haslength && th->getReceivedLength() == th->getLength())||
		(!haslength && result == 0)) // no content-length header set and no bytes read => finish download
			
	{
		th->setFinished();
		LOG(LOG_INFO,"download done:"<<th->getURL()<<" "<<th->getReceivedLength()<<" "<<th->getLength());
		return;
	}
	struct PP_CompletionCallback cb;
	cb.func = dlReadResponseCallback;
	cb.flags = 0;
	cb.user_data = th;
	g_urlloader_interface->ReadResponseBody(th->ppurlloader,th->buffer,4096,cb);
}
ppDownloader::ppDownloader(const lightspark::tiny_string& _url, PP_Instance _instance, lightspark::ILoadable* owner,SystemState* sys):
	Downloader(_url, _MR(new MemoryStreamCache), owner),isMainClipDownloader(true),m_sys(sys),state(INIT)
{
	PP_Var btrue;
	btrue.type = PP_VARTYPE_BOOL;
	btrue.value.as_bool = PP_TRUE;
	
	ppurlloader = g_urlloader_interface->Create(_instance);
	g_urlloadedtrusted_interface->GrantUniversalAccess(ppurlloader);
	PP_Resource pprequest_info = g_urlrequestinfo_interface->Create(_instance);
	PP_Var url = g_var_interface->VarFromUtf8(_url.raw_buf(),_url.numBytes());
	g_urlrequestinfo_interface->SetProperty(pprequest_info,PP_URLREQUESTPROPERTY_URL,url);
	g_urlrequestinfo_interface->SetProperty(pprequest_info,PP_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS,btrue);
	LOG(LOG_INFO,"constructing downloader:"<<_url);

	struct PP_CompletionCallback cb;
	cb.func = dlStartCallback;
	cb.flags = 0;
	cb.user_data = this;
	
	int res = g_urlloader_interface->Open(ppurlloader,pprequest_info,cb);
	if (res != PP_OK_COMPLETIONPENDING)
		LOG(LOG_ERROR,"url opening failed:"<<res<<" "<<_url);
}

ppDownloader::ppDownloader(const lightspark::tiny_string& _url, _R<StreamCache> _cache, PP_Instance _instance, lightspark::ILoadable* owner):
	Downloader(_url, _cache, owner),isMainClipDownloader(false),m_sys(NULL),state(INIT)
{
	ppurlloader = g_urlloader_interface->Create(_instance);
}

ppDownloader::ppDownloader(const lightspark::tiny_string& _url, _R<StreamCache> _cache,
		const std::vector<uint8_t>& _data,
		const std::list<tiny_string>& headers, PP_Instance _instance, lightspark::ILoadable* owner):
	Downloader(_url, _cache, _data, headers, owner),isMainClipDownloader(false),m_sys(NULL),state(INIT)
{
	ppurlloader = g_urlloader_interface->Create(_instance);
}

ppPluginInstance::ppPluginInstance(PP_Instance instance, int16_t argc, const char *argn[], const char *argv[]) : 
	m_ppinstance(instance),
	mainDownloaderStreambuf(NULL),mainDownloaderStream(NULL),
	mainDownloader(NULL),
	//scriptObject(NULL),
	m_pt(NULL)
{
	m_last_size.width = 0;
	m_last_size.height = 0;
	m_graphics = 0;
	LOG(LOG_INFO, "Lightspark version " << VERSION << " Copyright 2009-2013 Alessandro Pignotti and others");
	setTLSSys( NULL );
	m_sys=new lightspark::SystemState(0, lightspark::SystemState::FLASH);
	//Files running in the plugin have REMOTE sandbox
	m_sys->securityManager->setSandboxType(lightspark::SecurityManager::REMOTE);

	m_sys->extScriptObject = NULL;
//		scriptObject =
//			(NPScriptObjectGW *) NPN_CreateObject(mInstance, &NPScriptObjectGW::npClass);
//		m_sys->extScriptObject = scriptObject->getScriptObject();
//		scriptObject->m_sys = m_sys;
	//Parse OBJECT/EMBED tag attributes
	tiny_string swffile;
	for(int i=0;i<argc;i++)
	{
		if(argn[i]==NULL || argv[i]==NULL)
			continue;
		LOG(LOG_INFO,"param:"<<argn[i]<<" "<<argv[i]);
		if(strcasecmp(argn[i],"flashvars")==0)
		{
			m_sys->parseParametersFromFlashvars(argv[i]);
		}
		else if(strcasecmp(argn[i],"name")==0)
		{
			//m_sys->extScriptObject->setProperty(argn[i],argv[i]);
		}
		else if(strcasecmp(argn[i],"src")==0)
		{
			swffile = argv[i];
		}
	}
	if (!swffile.empty())
	{
		m_sys->downloadManager=new ppDownloadManager(m_ppinstance,m_sys);
	
		//EngineData::startSDLMain();
		mainDownloader=new ppDownloader(swffile,m_ppinstance,m_sys->mainClip->loaderInfo.getPtr(),m_sys);
		mainDownloaderStreambuf = mainDownloader->getCache()->createReader();
		mainDownloaderStream.rdbuf(mainDownloaderStreambuf);
		m_pt=new lightspark::ParseThread(mainDownloaderStream,m_sys->mainClip);
		m_sys->addJob(m_pt);
	
		//EngineData::mainthread_running = true;
	}
	//The sys var should be NULL in this thread
	setTLSSys( NULL );
}

ppPluginInstance::~ppPluginInstance()
{
	//Shutdown the system
	setTLSSys(m_sys);
	if(mainDownloader)
		mainDownloader->stop();
	if (mainDownloaderStreambuf)
		delete mainDownloaderStreambuf;

	// Kill all stuff relating to NPScriptObject which is still running
//	static_cast<NPScriptObject*>(m_sys->extScriptObject)->destroy();

	m_sys->setShutdownFlag();

	m_sys->destroy();
	delete m_sys;
	delete m_pt;
	setTLSSys(NULL);
}
void swapbuffer_callback(void* userdata,int result)
{
	SystemState* sys = (SystemState*)userdata;
	setTLSSys(sys);
	
	sys->getRenderThread()->doRender();
	
	struct PP_CompletionCallback cb;
	cb.func = swapbuffer_callback;
	cb.flags = 0;
	cb.user_data = sys;
	g_graphics_3d_interface->SwapBuffers(((ppPluginEngineData*)sys->getEngineData())->getGraphics(),cb);
}
void ppPluginInstance::handleResize(PP_Resource view)
{
	struct PP_Rect position;
	if (g_view_interface->GetRect(view, &position) == PP_FALSE)
	{
		LOG(LOG_ERROR,"Instance_DidChangeView: couldn't get rect");
		return;
	}
	if (m_last_size.width != position.size.width ||
		m_last_size.height != position.size.height) {

		if (!m_graphics) {
			int32_t attribs[] = {
				PP_GRAPHICS3DATTRIB_WIDTH, position.size.width,
				PP_GRAPHICS3DATTRIB_HEIGHT, position.size.height,
				PP_GRAPHICS3DATTRIB_NONE,
			};
			m_graphics = g_graphics_3d_interface->Create(m_ppinstance, 0, attribs);
			g_instance_interface->BindGraphics(m_ppinstance, m_graphics);
			if (!m_graphics)
			{
				LOG(LOG_ERROR,"Instance_DidChangeView: couldn't create graphics");
				 return;
			}
			LOG(LOG_INFO,"Instance_DidChangeView: create:"<<position.size.width<<" "<<position.size.height);
			ppPluginEngineData* e = new ppPluginEngineData(this, position.size.width, position.size.height,m_sys);
			m_sys->setParamsAndEngine(e, false);
			g_graphics_3d_interface->ResizeBuffers(m_graphics,position.size.width, position.size.height);
			m_sys->getRenderThread()->SetEngineData(m_sys->getEngineData());
			m_sys->getRenderThread()->init();
			struct PP_CompletionCallback cb;
			cb.func = swapbuffer_callback;
			cb.flags = 0;
			cb.user_data = m_sys;
			g_graphics_3d_interface->SwapBuffers(m_graphics,cb);
		}
		else
		{
			LOG(LOG_INFO,"Instance_DidChangeView: resize after creation:"<<position.size.width<<" "<<position.size.height);
			g_graphics_3d_interface->ResizeBuffers(m_graphics,position.size.width, position.size.height);
			m_sys->getEngineData()->width =position.size.width;
			m_sys->getEngineData()->height =position.size.height;
			m_sys->getRenderThread()->requestResize(position.size.width,position.size.height,true);
		}
		m_last_size.width = position.size.width;
		m_last_size.height = position.size.height;
	}
}


std::map<PP_Instance,ppPluginInstance*> all_instances;

static PP_Bool Instance_DidCreate(PP_Instance instance,uint32_t argc,const char* argn[],const char* argv[]) 
{
	LOG(LOG_INFO,"Instance_DidCreate:"<<instance);
	ppPluginInstance* newinstance = new ppPluginInstance(instance,argc,argn,argv);
	
	all_instances[instance] = newinstance;
	return PP_TRUE;
}
static void Instance_DidDestroy(PP_Instance instance)
{
	LOG(LOG_INFO,"Instance_DidDestroy:"<<instance);
	ppPluginInstance* it = all_instances[instance];
	all_instances.erase(instance);
	if (it)
		delete it;
}
static void Instance_DidChangeView(PP_Instance instance,PP_Resource view) 
{
	auto it = all_instances.find(instance);
	if (it == all_instances.end())
	{
		LOG(LOG_ERROR,"Instance_DidChangeView: no matching PPPluginInstance found");
		return;
	}
	ppPluginInstance* info = it->second;
	info->handleResize(view);
}

static void Instance_DidChangeFocus(PP_Instance instance, PP_Bool has_focus) {}

static PP_Bool Instance_HandleDocumentLoad(PP_Instance instance,PP_Resource url_loader) 
{
	LOG(LOG_INFO,"HandleDocumentLoad");
	return PP_FALSE;
}
static void Messaging_HandleMessage(PP_Instance instance, struct PP_Var message) 
{
	LOG(LOG_INFO,"handleMessage:"<<(int)message.type);
}
static PPP_Instance instance_interface = {
  &Instance_DidCreate,
  &Instance_DidDestroy,
  &Instance_DidChangeView,
  &Instance_DidChangeFocus,
  &Instance_HandleDocumentLoad
};
static PPP_Messaging messaging_interface = {
	&Messaging_HandleMessage,
};

extern "C"
{
	PP_EXPORT int32_t PPP_InitializeModule(PP_Module module_id,PPB_GetInterface get_browser_interface) 
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
		
		LOG(LOG_INFO, "Lightspark version " << VERSION << " Copyright 2009-2013 Alessandro Pignotti and others");
		g_get_browser_interface = get_browser_interface;
		g_core_interface = (const PPB_Core*)get_browser_interface(PPB_CORE_INTERFACE);
		g_instance_interface = (const PPB_Instance*)get_browser_interface(PPB_INSTANCE_INTERFACE);
		g_graphics_3d_interface = (const PPB_Graphics3D*)get_browser_interface(PPB_GRAPHICS_3D_INTERFACE);
		g_view_interface = (const PPB_View*)get_browser_interface(PPB_VIEW_INTERFACE);
		g_var_interface = (const PPB_Var*)get_browser_interface(PPB_VAR_INTERFACE);
		g_urlloader_interface = (const PPB_URLLoader*)get_browser_interface(PPB_URLLOADER_INTERFACE);
		g_urlrequestinfo_interface = (const PPB_URLRequestInfo*)get_browser_interface(PPB_URLREQUESTINFO_INTERFACE);
		g_urlresponseinfo_interface = (const PPB_URLResponseInfo*)get_browser_interface(PPB_URLRESPONSEINFO_INTERFACE);
		g_gles2_interface = (const PPB_OpenGLES2*)get_browser_interface(PPB_OPENGLES2_INTERFACE);
		g_urlloadedtrusted_interface = (const PPB_URLLoaderTrusted*)get_browser_interface(PPB_URLLOADERTRUSTED_INTERFACE);
		
		if (!g_core_interface ||
				!g_instance_interface || 
				!g_graphics_3d_interface || 
				!g_view_interface || 
				!g_var_interface ||
				!g_urlloader_interface ||
				!g_urlrequestinfo_interface ||
				!g_urlresponseinfo_interface ||
				!g_gles2_interface ||
				!g_urlloadedtrusted_interface)
		{
			LOG(LOG_ERROR,"get_browser_interface failed:"
				<< g_core_interface <<" "
				<< g_instance_interface <<" "
				<< g_graphics_3d_interface <<" "
				<< g_view_interface<<" "
				<< g_var_interface<<" "
				<< g_urlloader_interface<<" "
				<< g_urlrequestinfo_interface<<" "
				<< g_urlresponseinfo_interface<<" "
				<< g_gles2_interface<<" "
				<< g_urlloadedtrusted_interface);
			return PP_ERROR_NOINTERFACE;
		}
		return PP_OK;
	}
	PP_EXPORT void PPP_ShutdownModule() 
	{
		LOG(LOG_INFO,"PPP_ShutdownModule");
		SystemState::staticDeinit();
	}
	PP_EXPORT const void* PPP_GetInterface(const char* interface_name) 
	{
		LOG(LOG_INFO,"PPP_getInterface:"<<interface_name);
		if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0) 
		{
			return &instance_interface;
		}
		if (strcmp(interface_name, PPP_MESSAGING_INTERFACE) == 0) 
		{
			return &messaging_interface;
		}
		return NULL;
	}

}

void ppPluginEngineData::stopMainDownload()
{
//	if(instance->mainDownloader)
//		instance->mainDownloader->stop();
}

uint32_t ppPluginEngineData::getWindowForGnash()
{
//	return instance->mWindow;
	return 0;
}

void ppPluginEngineData::openPageInBrowser(const tiny_string& url, const tiny_string& window)
{
	//instance->openLink(url, window);
}

SDL_Window* ppPluginEngineData::createWidget(uint32_t w,uint32_t h)
{
	return 0;
}

void ppPluginEngineData::grabFocus()
{
	/*
	if (!widget_gtk)
		return;

	gtk_widget_grab_focus(widget_gtk);
	*/
}

void ppPluginEngineData::setClipboardText(const std::string txt)
{
	LOG(LOG_NOT_IMPLEMENTED,"setCLipboardText");
}

bool ppPluginEngineData::getScreenData(SDL_DisplayMode *screen)
{
	LOG(LOG_NOT_IMPLEMENTED,"getScreenData");
	return true;
}

double ppPluginEngineData::getScreenDPI()
{
	LOG(LOG_NOT_IMPLEMENTED,"getScreenDPI");
	return 0;
}

void ppPluginEngineData::SwapBuffers()
{
	//SwapBuffers is handled in callback
}

void ppPluginEngineData::InitOpenGL()
{
	
}

void ppPluginEngineData::DeinitOpenGL()
{
	
}

bool ppPluginEngineData::getGLError(uint32_t &errorCode) const
{
	errorCode = g_gles2_interface->GetError(instance->m_graphics);
	return errorCode!=GL_NO_ERROR;
}

void ppPluginEngineData::exec_glUniform1f(int location, float v0)
{
	g_gles2_interface->Uniform1f(instance->m_graphics,location,v0);
}

void ppPluginEngineData::exec_glBindTexture_GL_TEXTURE_2D(uint32_t id)
{
	g_gles2_interface->BindTexture(instance->m_graphics,GL_TEXTURE_2D,id);
}

void ppPluginEngineData::exec_glVertexAttribPointer(uint32_t index, int32_t size, int32_t stride, const void *coords)
{
	g_gles2_interface->VertexAttribPointer(instance->m_graphics,index, size, GL_FLOAT, GL_FALSE, stride, coords);
}

void ppPluginEngineData::exec_glEnableVertexAttribArray(uint32_t index)
{
	g_gles2_interface->EnableVertexAttribArray(instance->m_graphics,index);
}

void ppPluginEngineData::exec_glDrawArrays_GL_TRIANGLES(int32_t first, int32_t count)
{
	g_gles2_interface->DrawArrays(instance->m_graphics,GL_TRIANGLES,first,count);
}
void ppPluginEngineData::exec_glDrawArrays_GL_LINE_STRIP(int32_t first, int32_t count)
{
	g_gles2_interface->DrawArrays(instance->m_graphics,GL_LINE_STRIP,first,count);
}

void ppPluginEngineData::exec_glDrawArrays_GL_TRIANGLE_STRIP(int32_t first, int32_t count)
{
	g_gles2_interface->DrawArrays(instance->m_graphics,GL_TRIANGLE_STRIP,first,count);
}

void ppPluginEngineData::exec_glDrawArrays_GL_LINES(int32_t first, int32_t count)
{
	g_gles2_interface->DrawArrays(instance->m_graphics,GL_LINES,first,count);
}

void ppPluginEngineData::exec_glDisableVertexAttribArray(uint32_t index)
{
	g_gles2_interface->DisableVertexAttribArray(instance->m_graphics,index);
}

void ppPluginEngineData::exec_glUniformMatrix4fv(int32_t location, int32_t count, bool transpose, const float *value)
{
	g_gles2_interface->UniformMatrix4fv(instance->m_graphics,location, count, transpose, value);
}

void ppPluginEngineData::exec_glBindBuffer_GL_PIXEL_UNPACK_BUFFER(uint32_t buffer)
{
	// PPAPI has no BindBuffer
	//g_gles2_interface->BindBuffer(instance->m_graphics,GL_PIXEL_UNPACK_BUFFER, buffer);
}
uint8_t* ppPluginEngineData::exec_glMapBuffer_GL_PIXEL_UNPACK_BUFFER_GL_WRITE_ONLY()
{
	// PPAPI has no GLEW
	return NULL;
}
void ppPluginEngineData::exec_glUnmapBuffer_GL_PIXEL_UNPACK_BUFFER()
{
	// PPAPI has no GLEW
}

void ppPluginEngineData::exec_glEnable_GL_TEXTURE_2D()
{
	g_gles2_interface->Enable(instance->m_graphics,GL_TEXTURE_2D);
}

void ppPluginEngineData::exec_glEnable_GL_BLEND()
{
	g_gles2_interface->Enable(instance->m_graphics,GL_BLEND);
}

void ppPluginEngineData::exec_glDisable_GL_TEXTURE_2D()
{
	g_gles2_interface->Disable(instance->m_graphics,GL_TEXTURE_2D);
}
void ppPluginEngineData::exec_glFlush()
{
	g_gles2_interface->Flush(instance->m_graphics);
}

uint32_t ppPluginEngineData::exec_glCreateShader_GL_FRAGMENT_SHADER()
{
	return g_gles2_interface->CreateShader(instance->m_graphics,GL_FRAGMENT_SHADER);
}

uint32_t ppPluginEngineData::exec_glCreateShader_GL_VERTEX_SHADER()
{
	return g_gles2_interface->CreateShader(instance->m_graphics,GL_VERTEX_SHADER);
}

void ppPluginEngineData::exec_glShaderSource(uint32_t shader, int32_t count, const char **name, int32_t* length)
{
	g_gles2_interface->ShaderSource(instance->m_graphics,shader,count,name,length);
}

void ppPluginEngineData::exec_glCompileShader(uint32_t shader)
{
	g_gles2_interface->CompileShader(instance->m_graphics,shader);
}

void ppPluginEngineData::exec_glGetShaderInfoLog(uint32_t shader,int32_t bufSize,int32_t* length,char* infoLog)
{
	g_gles2_interface->GetShaderInfoLog(instance->m_graphics,shader,bufSize,length,infoLog);
}

void ppPluginEngineData::exec_glGetShaderiv_GL_COMPILE_STATUS(uint32_t shader,int32_t* params)
{
	g_gles2_interface->GetShaderiv(instance->m_graphics,shader,GL_COMPILE_STATUS,params);
}

uint32_t ppPluginEngineData::exec_glCreateProgram()
{
	return g_gles2_interface->CreateProgram(instance->m_graphics);
}

void ppPluginEngineData::exec_glBindAttribLocation(uint32_t program,uint32_t index, const char* name)
{
	g_gles2_interface->BindAttribLocation(instance->m_graphics,program,index,name);
}

void ppPluginEngineData::exec_glAttachShader(uint32_t program, uint32_t shader)
{
	g_gles2_interface->AttachShader(instance->m_graphics,program,shader);
}

void ppPluginEngineData::exec_glLinkProgram(uint32_t program)
{
	g_gles2_interface->LinkProgram(instance->m_graphics,program);
}

void ppPluginEngineData::exec_glGetProgramiv_GL_LINK_STATUS(uint32_t program,int32_t* params)
{
	g_gles2_interface->GetProgramiv(instance->m_graphics,program,GL_LINK_STATUS,params);
}

void ppPluginEngineData::exec_glBindFramebuffer_GL_FRAMEBUFFER(uint32_t framebuffer)
{
	g_gles2_interface->BindFramebuffer(instance->m_graphics,GL_FRAMEBUFFER,framebuffer);
}

void ppPluginEngineData::exec_glDeleteTextures(int32_t n,uint32_t* textures)
{
	g_gles2_interface->DeleteTextures(instance->m_graphics,n,textures);
}

void ppPluginEngineData::exec_glDeleteBuffers(int32_t n,uint32_t* buffers)
{
	g_gles2_interface->DeleteBuffers(instance->m_graphics,n,buffers);
}

void ppPluginEngineData::exec_glBlendFunc_GL_ONE_GL_ONE_MINUS_SRC_ALPHA()
{
	g_gles2_interface->BlendFunc(instance->m_graphics,GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void ppPluginEngineData::exec_glActiveTexture_GL_TEXTURE0()
{
	g_gles2_interface->ActiveTexture(instance->m_graphics,GL_TEXTURE0);
}

void ppPluginEngineData::exec_glGenBuffers(int32_t n,uint32_t* buffers)
{
	g_gles2_interface->GenBuffers(instance->m_graphics,n,buffers);
}

void ppPluginEngineData::exec_glUseProgram(uint32_t program)
{
	g_gles2_interface->UseProgram(instance->m_graphics,program);
}

int32_t ppPluginEngineData::exec_glGetUniformLocation(uint32_t program,const char* name)
{
	return g_gles2_interface->GetUniformLocation(instance->m_graphics,program,name);
}

void ppPluginEngineData::exec_glUniform1i(int32_t location,int32_t v0)
{
	g_gles2_interface->Uniform1i(instance->m_graphics,location,v0);
}

void ppPluginEngineData::exec_glGenTextures(int32_t n,uint32_t* textures)
{
	g_gles2_interface->GenTextures(instance->m_graphics,n,textures);
}

void ppPluginEngineData::exec_glViewport(int32_t x,int32_t y,int32_t width,int32_t height)
{
	g_gles2_interface->Viewport(instance->m_graphics,x,y,width,height);
}

void ppPluginEngineData::exec_glBufferData_GL_PIXEL_UNPACK_BUFFER_GL_STREAM_DRAW(int32_t size,const void* data)
{
	// PPAPI has no BufferData
	//g_gles2_interface->BufferData(instance->m_graphics,GL_PIXEL_UNPACK_BUFFER,size, data,GL_STREAM_DRAW);
}

void ppPluginEngineData::exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MIN_FILTER_GL_LINEAR()
{
	g_gles2_interface->TexParameteri(instance->m_graphics,GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void ppPluginEngineData::exec_glTexParameteri_GL_TEXTURE_2D_GL_TEXTURE_MAG_FILTER_GL_LINEAR()
{
	g_gles2_interface->TexParameteri(instance->m_graphics,GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void ppPluginEngineData::exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_BYTE(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels)
{
	g_gles2_interface->TexImage2D(instance->m_graphics,GL_TEXTURE_2D, level, GL_RGBA, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}
void ppPluginEngineData::exec_glTexImage2D_GL_TEXTURE_2D_GL_UNSIGNED_INT_8_8_8_8_HOST(int32_t level,int32_t width, int32_t height,int32_t border, const void* pixels)
{
	g_gles2_interface->TexImage2D(instance->m_graphics,GL_TEXTURE_2D, level, GL_RGBA, width, height, border, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_HOST, pixels);
}

void ppPluginEngineData::exec_glDrawBuffer_GL_BACK()
{
	// PPAPI has no DrawBuffer
	//g_gles2_interface->DrawBuffer(instance->m_graphics,GL_BACK);
}

void ppPluginEngineData::exec_glClearColor(float red,float green,float blue,float alpha)
{
	g_gles2_interface->ClearColor(instance->m_graphics,red,green,blue,alpha);
}

void ppPluginEngineData::exec_glClear_GL_COLOR_BUFFER_BIT()
{
	g_gles2_interface->Clear(instance->m_graphics,GL_COLOR_BUFFER_BIT);
}

void ppPluginEngineData::exec_glPixelStorei_GL_UNPACK_ROW_LENGTH(int32_t param)
{
	// PPAPI has no PixelStorei
	//g_gles2_interface->PixelStorei(instance->m_graphics,GL_UNPACK_ROW_LENGTH,param);
}

void ppPluginEngineData::exec_glPixelStorei_GL_UNPACK_SKIP_PIXELS(int32_t param)
{
	// PPAPI has no PixelStorei
	//g_gles2_interface->PixelStorei(instance->m_graphics,GL_UNPACK_SKIP_PIXELS,param);
}

void ppPluginEngineData::exec_glPixelStorei_GL_UNPACK_SKIP_ROWS(int32_t param)
{
	// PPAPI has no PixelStorei
	//g_gles2_interface->PixelStorei(instance->m_graphics,GL_UNPACK_SKIP_ROWS,param);
}

void ppPluginEngineData::exec_glTexSubImage2D_GL_TEXTURE_2D(int32_t level,int32_t xoffset,int32_t yoffset,int32_t width,int32_t height,const void* pixels)
{
	g_gles2_interface->TexSubImage2D(instance->m_graphics,GL_TEXTURE_2D, level, xoffset, yoffset, width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_HOST, pixels);
}
void ppPluginEngineData::exec_glGetIntegerv_GL_MAX_TEXTURE_SIZE(int32_t* data)
{
	g_gles2_interface->GetIntegerv(instance->m_graphics,GL_MAX_TEXTURE_SIZE,data);
}
