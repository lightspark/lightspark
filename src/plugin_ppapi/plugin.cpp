/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2016 Ludger Krämer <dbluelle@onlinehome.de>

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
// - sound
// - font loading
// - register as separate plugin

#include "version.h"
#include "logger.h"
#include "compat.h"
#include "swf.h"
#include "backends/security.h"
#include "backends/rendering.h"
#include <string>
#include <algorithm>
#include <SDL2/SDL.h>
#include "threading.h"
#include "plugin_ppapi/plugin.h"
#include "plugin_ppapi/ppextscriptobject.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_module.h"
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
#include "ppapi/c/private/ppb_instance_private.h"
#include "ppapi/c/private/ppp_instance_private.h"
#include "ppapi/c/dev/ppb_var_deprecated.h"
#include "ppapi/c/dev/ppp_class_deprecated.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"
#include "ppapi/c/ppp_input_event.h"
#include "ppapi/c/ppb_opengles2.h"
#include "ppapi/c/ppb_graphics_3d.h"
#include "ppapi/c/ppb_input_event.h"
#include "ppapi/c/private/ppb_flash_clipboard.h"
#include "ppapi/c/ppb_file_io.h"
#include "ppapi/c/ppb_file_ref.h"
#include "ppapi/c/ppb_file_system.h"

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
static const PPB_Instance_Private* g_instance_private_interface = NULL;
static const PPB_Var_Deprecated* g_var_deprecated_interface = NULL;
static const PPB_InputEvent* g_inputevent_interface = NULL;
static const PPB_MouseInputEvent* g_mouseinputevent_interface = NULL;
static const PPB_KeyboardInputEvent* g_keyboardinputevent_interface = NULL;
static const PPB_WheelInputEvent* g_wheelinputevent_interface = NULL;
static const PPB_Flash_Clipboard* g_flashclipboard_interface = NULL;
static const PPB_FileIO* g_fileio_interface = NULL;
static const PPB_FileRef* g_fileref_interface = NULL;
static const PPB_FileSystem* g_filesystem_interface = NULL;



ppFileStreamCache::ppFileStreamCache(ppPluginInstance* instance):cache(0),cacheref(0),writeoffset(0),m_instance(instance)
{
}

ppFileStreamCache::~ppFileStreamCache()
{
	if (cache != 0)
	{
		g_fileio_interface->Close(cache);
		g_fileref_interface->Delete(cacheref,PP_MakeCompletionCallback(NULL,NULL));
	}
}

void ppFileStreamCache::handleAppend(const unsigned char* buffer, size_t length)
{
	if (cache == 0)
		openCache();

	int written = g_fileio_interface->Write(cache,writeoffset,(const char*)buffer,length,PP_MakeCompletionCallback(NULL,NULL));
	if (written < 0)
	{
		LOG(LOG_ERROR,"writing cache file failed, error code:"<<written);
		return;
	}
	writeoffset += written;
}

/**
 * \brief Creates & opens a temporary cache file
 *
 * Creates a temporary cache file in /tmp and calls \c openExistingCache() with that file.
 * Waits for mutex at start and releases mutex when finished.
 * \throw RunTimeException Temporary file could not be created
 * \throw RunTimeException Called when  the cache is already open
 * \see Downloader::openExistingCache()
 */
void ppFileStreamCache::openCache()
{
	if (cache != 0)
	{
		markFinished(true);
		throw RunTimeException("ppFileStreamCache::openCache called twice");
	}

	//Create a temporary file(name)
	
	cacheref = m_instance->createCacheFileRef();
	cache = g_fileio_interface->Create(m_instance->getFileSystem());
	int res = g_fileio_interface->Open(cache,cacheref,PP_FileOpenFlags::PP_FILEOPENFLAG_READ|PP_FileOpenFlags::PP_FILEOPENFLAG_WRITE,PP_MakeCompletionCallback(NULL,NULL));
	if (res != PP_OK)
	{
		LOG(LOG_ERROR,"opening cache file failed, error code:"<<res);
		cache = 0;
	}

}

void ppFileStreamCache::openForWriting()
{
	if (cache != 0)
		return;
	openCache();
}

bool ppFileStreamCache::waitForCache()
{
	if (cache != 0)
		return true;

	// Cache file will be opened when the first byte is received
	waitForData(0);

	return cache != 0;
}

std::streambuf *ppFileStreamCache::createReader()
{
	if (!waitForCache())
	{
		LOG(LOG_ERROR,"could not open cache file");
		return NULL;
	}

	incRef();
	ppFileStreamCache::ppFileStreamCacheReader *fbuf = new ppFileStreamCache::ppFileStreamCacheReader(_MR(this));
	return fbuf;
}

ppFileStreamCache::ppFileStreamCacheReader::ppFileStreamCacheReader(_R<ppFileStreamCache> b) : curpos(-1),buffer(b)
{
}

int ppFileStreamCache::ppFileStreamCacheReader::underflow()
{
	if (!buffer->hasTerminated())
		buffer->waitForData(seekoff(0, ios_base::cur, ios_base::in));

	return streambuf::underflow();
}

streamsize ppFileStreamCache::ppFileStreamCacheReader::xsgetn(char* s, streamsize n)
{
	streamsize read= g_fileio_interface->Read(buffer->cache,curpos,s,n,PP_MakeCompletionCallback(NULL,NULL));
	curpos += read;
	// If not enough data was available, wait for writer
	while (read < n)
	{
		buffer->waitForData(seekoff(0, ios_base::cur, ios_base::in));

		streamsize b = g_fileio_interface->Read(buffer->cache,curpos,s+read,n-read,PP_MakeCompletionCallback(NULL,NULL));
		curpos += b;

		// No more data after waiting, this must be EOF
		if (b == 0)
			return read;

		read += b;
	}

	return read;
}

streampos ppFileStreamCache::ppFileStreamCacheReader::seekoff(streamoff off, ios_base::seekdir way, ios_base::openmode which)
{
	switch (way)
	{
		case ios_base::beg:
			curpos = off;
			break;
		case ios_base::cur:
			curpos += off;
			break;
		case ios_base::end:
			curpos = buffer->getReceivedLength() + off;
			break;
		default:
			break;
	}
	return curpos;
}

streampos ppFileStreamCache::ppFileStreamCacheReader::seekpos(streampos sp, ios_base::openmode which)
{
	curpos = sp;
	return curpos;
}


ppVariantObject::ppVariantObject(std::map<int64_t, std::unique_ptr<ExtObject> > &objectsMap, PP_Var& other)
{
	switch(other.type)
	{
		case PP_VARTYPE_UNDEFINED:
			type = EV_VOID;
			break;
		case PP_VARTYPE_NULL:
			type = EV_NULL;
			break;
		case PP_VARTYPE_BOOL:
			type = EV_BOOLEAN;
			booleanValue = other.value.as_bool;
			break;
		case PP_VARTYPE_INT32:
			type = EV_INT32;
			intValue = other.value.as_int;
			break;
		case PP_VARTYPE_DOUBLE:
			type = EV_DOUBLE;
			doubleValue = other.value.as_double;
			break;
		case PP_VARTYPE_STRING:
		{
			uint32_t len;
			type = EV_STRING;
			strValue = g_var_interface->VarToUtf8(other,&len);
			break;
		}
		case PP_VARTYPE_OBJECT:
		{
			type = EV_OBJECT;
			/*
			auto it=objectsMap.find(npObj);
			if(it!=objectsMap.end())
				objectValue = it->second.get();
			else
			*/
				objectValue = new ppObjectObject(objectsMap, other);
			break;
		}
		default:
			LOG(LOG_NOT_IMPLEMENTED,"ppVariantObject for type:"<<(int)other.type);
			type = EV_VOID;
			break;
	}
}
void ppVariantObject::ExtVariantToppVariant(std::map<const ExtObject *, PP_Var> &objectsMap, PP_Instance instance, const ExtVariant& value, PP_Var& variant)
{
	switch(value.getType())
	{
		case EV_STRING:
		{
			const std::string& strValue = value.getString();
			variant = g_var_interface->VarFromUtf8(strValue.c_str(),strValue.length());
			break;
		}
		case EV_INT32:
			variant = PP_MakeInt32(value.getInt());
			break;
		case EV_DOUBLE:
			variant = PP_MakeDouble(value.getDouble());
			break;
		case EV_BOOLEAN:
			variant = PP_MakeBool(value.getBoolean() ? PP_TRUE:PP_FALSE);
			break;
		case EV_OBJECT:
		{
			ExtObject* obj = value.getObject();
			variant = ppObjectObject::getppObject(objectsMap,instance, obj);
			break;
		}
		case EV_NULL:
			variant = PP_MakeNull();
			break;
		case EV_VOID:
		default:
			variant = PP_MakeUndefined();
			break;
	}
}

// Type determination
ExtVariant::EV_TYPE ppVariantObject::getTypeS(const PP_Var& variant)
{
	switch(variant.type)
	{
		case PP_VARTYPE_UNDEFINED:
			return EV_VOID;
		case PP_VARTYPE_NULL:
			return EV_NULL;
		case PP_VARTYPE_BOOL:
			return EV_BOOLEAN;
		case PP_VARTYPE_INT32:
			return EV_INT32;
		case PP_VARTYPE_DOUBLE:
			return EV_DOUBLE;
		case PP_VARTYPE_STRING:
			return EV_STRING;
		case PP_VARTYPE_OBJECT:
			return EV_OBJECT;
		default:
			return EV_VOID;
	}
}

ppObjectObject::ppObjectObject(std::map<int64_t, std::unique_ptr<ExtObject>>& objectsMap, PP_Var& obj)
{
	//First of all add this object to the map, so that recursive cycles may be broken
	if(objectsMap.count(obj.value.as_id)==0)
		objectsMap[obj.value.as_id] = move(unique_ptr<ExtObject>(this));

	uint32_t property_count;
	PP_Var* properties;
	PP_Var exception = PP_MakeUndefined();
	g_var_deprecated_interface->GetAllPropertyNames(obj,&property_count,&properties,&exception);
	uint32_t len;
	if (exception.type == PP_VARTYPE_STRING)
	{
		LOG(LOG_ERROR,"exception during ppObjectObject::ppObjectObject GetAllPropertyNames:"<<g_var_interface->VarToUtf8(exception,&len));
		return;
	}
	ExtIdentifier id;
	for (uint32_t i=0; i < property_count; i++)
	{
		PP_Var prop = properties[i];
		PP_Var value = g_var_deprecated_interface->GetProperty(obj,prop,&exception);
		if (exception.type == PP_VARTYPE_STRING)
		{
			LOG(LOG_ERROR,"exception during ppObjectObject::ppObjectObject: GetProperty"<<g_var_interface->VarToUtf8(exception,&len));
			continue;
		}
		switch(prop.type)
		{
			case PP_VARTYPE_INT32:
				id = ExtIdentifier(prop.value.as_int);
				break;
			case PP_VARTYPE_STRING:
				id = ExtIdentifier(g_var_interface->VarToUtf8(prop,&len));
				break;
			default:
				LOG(LOG_NOT_IMPLEMENTED,"ppVariantObject::ppObjectObject for type:"<<(int)prop.type);
				continue;
		}
		setProperty(id,ppVariantObject(objectsMap,value));
	}
}


PP_Var ppObjectObject::getppObject(std::map<const ExtObject*, PP_Var>& objectsMap,PP_Instance instance, const ExtObject* obj)
{
	auto it=objectsMap.find(obj);
	if(it!=objectsMap.end())
	{
		return it->second;
	}
	uint32_t count = obj->getLength();

	tiny_string s("new Object()");
	PP_Var exception = PP_MakeUndefined();
	PP_Var scr = g_var_interface->VarFromUtf8(s.raw_buf(),s.numBytes());
	PP_Var result =g_instance_private_interface->ExecuteScript(instance,scr,&exception);
	uint32_t len;
	if (exception.type == PP_VARTYPE_STRING)
	{
		LOG(LOG_ERROR,"exception during ppObjectObject::getppObject Construct:"<<g_var_interface->VarToUtf8(exception,&len));
		return result;
	}
	
	objectsMap[obj] = result;

	PP_Var varProperty;
	ExtIdentifier** ids = NULL;
	// Set all values of the object
	if(obj->enumerate(&ids, &count))
	{
		for(uint32_t i = 0; i < count; i++)
		{
			const ExtVariant& property = obj->getProperty(*ids[i]);
			ppVariantObject::ExtVariantToppVariant(objectsMap,instance, property, varProperty);

			PP_Var propname;
			switch (ids[i]->getType())
			{
				case ExtVariant::EV_STRING:
				{
					std::string name = ids[i]->getString();
					propname = g_var_interface->VarFromUtf8(name.c_str(),name.length());
					break;
				}
				case ExtVariant::EV_INT32:
					propname = PP_MakeInt32(ids[i]->getInt());
					break;
				default:
					LOG(LOG_NOT_IMPLEMENTED,"ppObjectObject::getppObject for type "<<property.getType());
					continue;
			}
			g_var_deprecated_interface->SetProperty(result,propname,varProperty,&exception);
			if (exception.type == PP_VARTYPE_STRING)
			{
				LOG(LOG_ERROR,"exception during ppObjectObject::getppObject SetProperty:"<<g_var_interface->VarToUtf8(exception,&len));
			}
			delete ids[i];
		}
	}
	if(ids != NULL)
		delete[] ids;
	return result;
}

ppDownloadManager::ppDownloadManager(ppPluginInstance *_instance, SystemState *sys):m_instance(_instance),m_sys(sys)
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
	ppDownloader* downloader=new ppDownloader(url.getParsedURL(), cache, m_instance, owner);
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
	ppDownloader* downloader=new ppDownloader(url.getParsedURL(), cache, data, headers, m_instance, owner);
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
	setTLSSys(th->m_sys);
	
	if (result < 0)
	{
		LOG(LOG_ERROR,"download failed:"<<result<<" "<<th->getURL());
		th->setFailed();
		return;
	}
	PP_Resource response = g_urlloader_interface->GetResponseInfo(th->ppurlloader);
	PP_Var v;
	uint32_t len;
	v = g_urlresponseinfo_interface->GetProperty(response,PP_URLRESPONSEPROPERTY_HEADERS);
	tiny_string headers = g_var_interface->VarToUtf8(v,&len);
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
	if (th->hasEmptyAnswer())
	{
		th->setFinished();
		g_urlloader_interface->Close(th->ppurlloader);
		return;
	}
	g_urlloader_interface->ReadResponseBody(th->ppurlloader,th->buffer,4096,PP_MakeCompletionCallback(dlReadResponseCallback,th));
}
void ppDownloader::dlReadResponseCallback(void* userdata,int result)
{
	ppDownloader* th = (ppDownloader*)userdata;
	setTLSSys(th->m_sys);
	if (result < 0)
	{
		LOG(LOG_ERROR,"download failed:"<<result<<" "<<th->getURL()<<" "<<th->downloadedlength<<"/"<<th->getLength());
		th->setFailed();
		g_urlloader_interface->Close(th->ppurlloader);
		return;
	}
	th->append(th->buffer,result);
	if (th->downloadedlength == 0 && th->isMainClipDownloader)
		th->m_pluginInstance->startMainParser();
	th->downloadedlength += result;
	
	if (result == 0)
	{
		th->setFinished();
		g_urlloader_interface->Close(th->ppurlloader);
		LOG_CALL("download done:"<<th->getURL()<<" "<<th->downloadedlength<<" "<<th->getLength());
		return;
	}
	g_urlloader_interface->ReadResponseBody(th->ppurlloader,th->buffer,4096,PP_MakeCompletionCallback(dlReadResponseCallback,th));
}
void ppDownloader::dlStartDownloadCallback(void* userdata,int result)
{
	ppDownloader* th = (ppDownloader*)userdata;
	setTLSSys(th->m_sys);
	const tiny_string strurl = th->getURL();
	th->ppurlloader = g_urlloader_interface->Create(th->m_pluginInstance->getppInstance());
	g_urlloadedtrusted_interface->GrantUniversalAccess(th->ppurlloader);
	PP_Resource pprequest_info = g_urlrequestinfo_interface->Create(th->m_pluginInstance->getppInstance());
	PP_Var url = g_var_interface->VarFromUtf8(strurl.raw_buf(),strurl.numBytes());
	g_urlrequestinfo_interface->SetProperty(pprequest_info,PP_URLREQUESTPROPERTY_URL,url);
	g_urlrequestinfo_interface->SetProperty(pprequest_info,PP_URLREQUESTPROPERTY_ALLOWCROSSORIGINREQUESTS,PP_MakeBool(PP_TRUE));
	
	if (th->data.size() > 0)
	{
		PP_Var v = g_var_interface->VarFromUtf8("POST",4);
		g_urlrequestinfo_interface->SetProperty(pprequest_info,PP_URLREQUESTPROPERTY_METHOD,v);
		g_urlrequestinfo_interface->AppendDataToBody(pprequest_info,&th->data.front(),th->data.size());
	}
	LOG_CALL("starting download:"<<th->data.size()<<" "<<strurl);

	int res = g_urlloader_interface->Open(th->ppurlloader,pprequest_info,PP_MakeCompletionCallback(dlStartCallback,th));
	if (res != PP_OK_COMPLETIONPENDING)
		LOG(LOG_ERROR,"url opening failed:"<<res<<" "<<strurl);
}
void ppDownloader::startDownload()
{
	if (!g_core_interface->IsMainThread())
		g_core_interface->CallOnMainThread(0,PP_MakeCompletionCallback(dlStartDownloadCallback,this),0);
	else
		dlStartDownloadCallback(this,0);
	
}
ppDownloader::ppDownloader(const lightspark::tiny_string& _url, lightspark::ILoadable* owner, ppPluginInstance *ppinstance):
	Downloader(_url, _MR(new MemoryStreamCache), owner),isMainClipDownloader(true),m_sys(ppinstance->getSystemState()),m_pluginInstance(ppinstance),downloadedlength(0),state(INIT)
{
	startDownload();
}

ppDownloader::ppDownloader(const lightspark::tiny_string& _url, _R<StreamCache> _cache, ppPluginInstance *ppinstance, lightspark::ILoadable* owner):
	Downloader(_url, _cache, owner),isMainClipDownloader(false),m_sys(ppinstance->getSystemState()),m_pluginInstance(ppinstance),downloadedlength(0),state(INIT)
{
	startDownload();
}

ppDownloader::ppDownloader(const lightspark::tiny_string& _url, _R<StreamCache> _cache,
		const std::vector<uint8_t>& _data,
		const std::list<tiny_string>& headers, ppPluginInstance *ppinstance, lightspark::ILoadable* owner):
	Downloader(_url, _cache, _data, headers, owner),isMainClipDownloader(false),m_sys(ppinstance->getSystemState()),m_pluginInstance(ppinstance),downloadedlength(0),state(INIT)
{
	startDownload();
}
void openfilesystem_callback(void* userdata,int result)
{
	LOG(LOG_INFO,"filesystem opened");
}
ppPluginInstance::ppPluginInstance(PP_Instance instance, int16_t argc, const char *argn[], const char *argv[]) : 
	m_ppinstance(instance),
	mainDownloaderStreambuf(NULL),mainDownloaderStream(NULL),
	mainDownloader(NULL),
	m_pt(NULL)
{
	m_cachefilesystem = g_filesystem_interface->Create(instance,PP_FileSystemType::PP_FILESYSTEMTYPE_LOCALTEMPORARY);
	m_cachefilename = 0;
	g_filesystem_interface->Open(m_cachefilesystem, 1024*1024,PP_MakeCompletionCallback(openfilesystem_callback,NULL));
	m_last_size.width = 0;
	m_last_size.height = 0;
	m_graphics = 0;
	setTLSSys( NULL );
	m_sys=new lightspark::SystemState(0, lightspark::SystemState::FLASH);
	//Files running in the plugin have REMOTE sandbox
	m_sys->securityManager->setSandboxType(lightspark::SecurityManager::REMOTE);

	m_sys->extScriptObject = new ppExtScriptObject(this,m_sys);
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
			m_sys->extScriptObject->setProperty(argn[i],argv[i]);
		}
		else if(strcasecmp(argn[i],"src")==0)
		{
			swffile = argv[i];
		}
	}
	if (!swffile.empty())
	{
		m_sys->downloadManager=new ppDownloadManager(this,m_sys);
	
		//EngineData::startSDLMain();
		EngineData::mainthread_running = true;
		mainDownloader=new ppDownloader(swffile,m_sys->mainClip->loaderInfo.getPtr(),this);
		// loader is notified through parsethread
		mainDownloader->getCache()->setNotifyLoader(false);
	}
	//The sys var should be NULL in this thread
	setTLSSys( NULL );
}
void ppPluginInstance::startMainParser()
{
	mainDownloaderStreambuf = mainDownloader->getCache()->createReader();
	mainDownloaderStream.rdbuf(mainDownloaderStreambuf);
	m_pt=new lightspark::ParseThread(mainDownloaderStream,m_sys->mainClip);
	m_sys->addJob(m_pt);
	
}

PP_Resource ppPluginInstance::createCacheFileRef()
{
	char filenamebuf[100];
	sprintf(filenamebuf,"/tmp%d",++m_cachefilename);
	return g_fileref_interface->Create(getFileSystem(),filenamebuf);
}

ppPluginInstance::~ppPluginInstance()
{
	//Shutdown the system
	setTLSSys(m_sys);
	if(mainDownloader)
		mainDownloader->stop();
	if (mainDownloaderStreambuf)
		delete mainDownloaderStreambuf;

	if (m_sys->extScriptObject)
	{
		m_sys->extScriptObject->destroy();
		delete m_sys->extScriptObject;
		m_sys->extScriptObject = NULL;
	}

	m_sys->setShutdownFlag();

	m_sys->destroy();
	delete m_sys;
	delete m_pt;
	setTLSSys(NULL);
}
void swapbuffer_callback(void* userdata,int result)
{
	ppPluginEngineData* data = (ppPluginEngineData*)userdata;
	data->swapbuffer_rendering.signal();
}


void ppPluginInstance::handleResize(PP_Resource view)
{
	setTLSSys(m_sys);
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
typedef struct {
	const char* ppkey;
	SDL_Keycode sdlkeycode;
} ppKeyMap;

// the ppkey values are taken from https://www.w3.org/TR/uievents-key/
ppKeyMap ppkeymap[] = {
	{ "KeyA", SDLK_a },
	{ "AltLeft", SDLK_LALT },
	{ "KeyB", SDLK_b },
	{ "BrowserBack", SDLK_AC_BACK },
	{ "Backquote", SDLK_BACKQUOTE },
	{ "Backslash", SDLK_BACKSLASH },
	{ "Backspace", SDLK_BACKSPACE },
//	{ "Blue", SDLK_UNKNOWN }, // TODO
	{ "KeyC", SDLK_c },
	{ "CapsLock", SDLK_CAPSLOCK },
	{ "Comma", SDLK_COMMA },
	{ "ControlLeft", SDLK_LCTRL },
	{ "KeyD", SDLK_d },
	{ "Delete", SDLK_DELETE },
	{ "ArrowDown", SDLK_DOWN },
	{ "KeyE", SDLK_e },
	{ "End", SDLK_END },
	{ "Enter", SDLK_RETURN },
	{ "Equal", SDLK_EQUALS },
	{ "Escape", SDLK_ESCAPE },
	{ "KeyF", SDLK_f },
	{ "F1", SDLK_F1 },
	{ "F2", SDLK_F2 },
	{ "F3", SDLK_F3 },
	{ "F4", SDLK_F4 },
	{ "F5", SDLK_F5 },
	{ "F6", SDLK_F6 },
	{ "F7", SDLK_F7 },
	{ "F8", SDLK_F8 },
	{ "F9", SDLK_F9 },
	{ "F10", SDLK_F10 },
	{ "F11", SDLK_F11 },
	{ "F12", SDLK_F12 },
//	{ "F13", SDLK_F13 },// TODO
//	{ "F14", SDLK_F14 },// TODO
//	{ "F15", SDLK_F15 },// TODO
	{ "KeyG", SDLK_g },
//	{ "Green", SDLK_UNKNOWN }, // TODO
	{ "KeyH", SDLK_h },
	{ "Help", SDLK_HELP },
	{ "Home", SDLK_HOME },
	{ "KeyI", SDLK_i },
	{ "Insert", SDLK_INSERT },
	{ "KeyJ", SDLK_j },
	{ "KeyK", SDLK_k },
	{ "KeyL", SDLK_l },
	{ "ArrowLeft", SDLK_LEFT },
	{ "BracketLeft", SDLK_LEFTBRACKET },
	{ "KeyM", SDLK_m },
	{ "Minus", SDLK_MINUS },
	{ "KeyN", SDLK_n },
	{ "Digit0", SDLK_0 },
	{ "Digit1", SDLK_1 },
	{ "Digit2", SDLK_2 },
	{ "Digit3", SDLK_3 },
	{ "Digit4", SDLK_4 },
	{ "Digit5", SDLK_5 },
	{ "Digit6", SDLK_6 },
	{ "Digit7", SDLK_7 },
	{ "Digit8", SDLK_8 },
	{ "Digit9", SDLK_9 },
	{ "Numpad0", SDLK_KP_0 },
	{ "Numpad1", SDLK_KP_1 },
	{ "Numpad2", SDLK_KP_2 },
	{ "Numpad3", SDLK_KP_3 },
	{ "Numpad4", SDLK_KP_4 },
	{ "Numpad5", SDLK_KP_5 },
	{ "Numpad6", SDLK_KP_6 },
	{ "Numpad7", SDLK_KP_7 },
	{ "Numpad8", SDLK_KP_8 },
	{ "Numpad9", SDLK_KP_9 },
	{ "NumpadAdd", SDLK_KP_MEMADD },
	{ "NumpadDecimal", SDLK_KP_PERIOD },
	{ "NumpadDivide", SDLK_KP_DIVIDE },
	{ "NumpadEnter", SDLK_KP_ENTER },
	{ "NumpadMultiply", SDLK_KP_MULTIPLY },
	{ "NumpadSubtract", SDLK_KP_MINUS },
	{ "KeyO", SDLK_o },
	{ "KeyP", SDLK_p },
	{ "PageDown", SDLK_PAGEDOWN },
	{ "PageUp", SDLK_PAGEUP },
	{ "Pause", SDLK_PAUSE },
	{ "Period", SDLK_PERIOD },
	{ "KeyQ", SDLK_q },
	{ "Quote", SDLK_QUOTE },
	{ "KeyR", SDLK_r },
//	{ "Red", SDLK_UNKNOWN }, // TODO
	{ "ArrowRight", SDLK_RIGHT },
	{ "BracketRight", SDLK_RIGHTBRACKET },
	{ "KeyS", SDLK_s },
	{ "BrowserSearch", SDLK_AC_SEARCH },
	{ "Semicolon", SDLK_SEMICOLON },
	{ "ShiftLeft", SDLK_LSHIFT },
	{ "Slash", SDLK_SLASH },
	{ "Space", SDLK_SPACE },
//	{ "Subtitle", SDLK_UNKNOWN }, // TODO
	{ "KeyT", SDLK_t },
	{ "Tab", SDLK_TAB },
	{ "KeyU", SDLK_u },
	{ "ArrowUp", SDLK_UP },
	{ "KeyV", SDLK_v },
	{ "KeyW", SDLK_w },
	{ "KeyX", SDLK_x },
	{ "KeyY", SDLK_y },
//	{ "Yellow", SDLK_UNKNOWN }, // TODO
	{ "KeyZ", SDLK_z },
	{ "", SDLK_UNKNOWN } // indicator for last entry
};
SDL_Keycode getppSDLKeyCode(PP_Resource input_event)
{
	PP_Var v = g_keyboardinputevent_interface->GetCode(input_event);
	uint32_t len;
	const char* key = g_var_interface->VarToUtf8(v,&len);
	int i = 0;
	while (*ppkeymap[i].ppkey)
	{
		if (strcmp(ppkeymap[i].ppkey,key) == 0)
			return ppkeymap[i].sdlkeycode;
		++i;
	}
	LOG(LOG_NOT_IMPLEMENTED,"no matching keycode for input event found:"<<key);
	return SDLK_UNKNOWN;
};
static uint16_t getppKeyModifier(PP_Resource input_event)
{
	uint32_t mod = g_inputevent_interface->GetModifiers(input_event);
	uint16_t res = KMOD_NONE;
	if (mod & PP_INPUTEVENT_MODIFIER_CONTROLKEY)
		res |= KMOD_CTRL;
	if (mod & PP_INPUTEVENT_MODIFIER_ALTKEY)
		res |= KMOD_ALT;
	if (mod & PP_INPUTEVENT_MODIFIER_SHIFTKEY)
		res |= KMOD_SHIFT;
	return res;
}

PP_Bool ppPluginInstance::handleInputEvent(PP_Resource input_event)
{
	setTLSSys(m_sys);
	SDL_Event ev;
	switch (g_inputevent_interface->GetType(input_event))
	{
		case PP_INPUTEVENT_TYPE_KEYDOWN:
		{
			
			ev.type = SDL_KEYDOWN;
			ev.key.keysym.sym = getppSDLKeyCode(input_event);
			ev.key.keysym.mod = getppKeyModifier(input_event);
			SDL_SetModState((SDL_Keymod)ev.key.keysym.mod);
			break;
		}
		case PP_INPUTEVENT_TYPE_KEYUP:
		{
			ev.type = SDL_KEYUP;
			ev.key.keysym.sym = getppSDLKeyCode(input_event);
			ev.key.keysym.mod = getppKeyModifier(input_event);
			SDL_SetModState((SDL_Keymod)ev.key.keysym.mod);
			break;
		}
		case PP_INPUTEVENT_TYPE_MOUSEDOWN:
		{
			ev.type = SDL_MOUSEBUTTONDOWN;
			
			switch (g_mouseinputevent_interface->GetButton(input_event))
			{
				case PP_INPUTEVENT_MOUSEBUTTON_LEFT:
					ev.button.button = SDL_BUTTON_LEFT;
					ev.button.state = g_inputevent_interface->GetModifiers(input_event) & PP_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
					break;
				case PP_INPUTEVENT_MOUSEBUTTON_RIGHT:
					ev.button.button = SDL_BUTTON_RIGHT;
					ev.button.state = g_inputevent_interface->GetModifiers(input_event) & PP_INPUTEVENT_MODIFIER_RIGHTBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
					break;
				default:
					ev.button.button = 0;
					ev.button.state = SDL_RELEASED;
					break;
			}
			ev.button.clicks = g_mouseinputevent_interface->GetClickCount(input_event);
			PP_Point p = g_mouseinputevent_interface->GetPosition(input_event);
			ev.button.x = p.x;
			ev.button.y = p.y;
			break;
		}
		case PP_INPUTEVENT_TYPE_MOUSEUP:
		{
			ev.type = SDL_MOUSEBUTTONUP;
			switch (g_mouseinputevent_interface->GetButton(input_event))
			{
				case PP_INPUTEVENT_MOUSEBUTTON_LEFT:
					ev.button.button = SDL_BUTTON_LEFT;
					ev.button.state = g_inputevent_interface->GetModifiers(input_event) & PP_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
					break;
				case PP_INPUTEVENT_MOUSEBUTTON_RIGHT:
					ev.button.button = SDL_BUTTON_RIGHT;
					ev.button.state = g_inputevent_interface->GetModifiers(input_event) & PP_INPUTEVENT_MODIFIER_RIGHTBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
					break;
				default:
					ev.button.button = 0;
					ev.button.state = SDL_RELEASED;
					break;
			}
			ev.button.clicks = 0;
			PP_Point p = g_mouseinputevent_interface->GetPosition(input_event);
			ev.button.x = p.x;
			ev.button.y = p.y;
			break;
		}
		case PP_INPUTEVENT_TYPE_MOUSEMOVE:
		{
			ev.type = SDL_MOUSEMOTION;
			ev.motion.state = g_inputevent_interface->GetModifiers(input_event) & PP_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
			PP_Point p = g_mouseinputevent_interface->GetMovement(input_event);
			ev.motion.x = p.x;
			ev.motion.y = p.y;
			break;
		}
		case PP_INPUTEVENT_TYPE_WHEEL:
		{
			PP_FloatPoint p = g_wheelinputevent_interface->GetDelta(input_event);
			ev.type = SDL_MOUSEWHEEL;
#if SDL_VERSION_ATLEAST(2, 0, 4)
			ev.wheel.direction = p.y > 0 ? SDL_MOUSEWHEEL_NORMAL : SDL_MOUSEWHEEL_FLIPPED ;
#endif
			ev.wheel.x = p.x;
			ev.wheel.y = p.y;
			break;
		}
		case PP_INPUTEVENT_TYPE_MOUSELEAVE:
		{
			ev.type = SDL_WINDOWEVENT_LEAVE;
			break;
		}
		default:
			LOG(LOG_NOT_IMPLEMENTED,"ppp_inputevent:"<<(int)g_inputevent_interface->GetType(input_event));
			return PP_FALSE;
	}
	EngineData::mainloop_handleevent(&ev,this->m_sys);
	return PP_TRUE;
}

void executescript_callback(void* userdata,int result)
{
	ExtScriptObject::hostCallHandler(userdata);
}
void ppPluginInstance::executeScriptAsync(ExtScriptObject::HOST_CALL_DATA *data)
{
	g_core_interface->CallOnMainThread(0,PP_MakeCompletionCallback(executescript_callback,data),0);
}
bool ppPluginInstance::executeScript(const std::string script, const ExtVariant **args, uint32_t argc, ASObject **result)
{
	setTLSSys(m_sys);
	PP_Var scr = g_var_interface->VarFromUtf8(script.c_str(),script.length());
	PP_Var exception = PP_MakeUndefined();
	PP_Var func = g_instance_private_interface->ExecuteScript(m_ppinstance,scr,&exception);
	*result = NULL;
	uint32_t len;
	if (exception.type == PP_VARTYPE_STRING)
	{
		LOG(LOG_ERROR,"error preparing script:"<<script<<" "<<g_var_interface->VarToUtf8(exception,&len));
		return false;
	}
	PP_Var* variantArgs = g_newa(PP_Var,argc);
	for(uint32_t i = 0; i < argc; i++)
	{
		std::map<const ExtObject *, PP_Var> objectsMap;
		ppVariantObject::ExtVariantToppVariant(objectsMap,m_ppinstance, *(args[i]), variantArgs[i]);
	}

	PP_Var resultVariant = g_var_deprecated_interface->Call(func,PP_MakeUndefined(),argc,variantArgs,&exception);

	if (exception.type == PP_VARTYPE_STRING)
	{
		LOG(LOG_ERROR,"error calling script:"<<script<<" "<<g_var_interface->VarToUtf8(exception,&len));
		return false;
	}
	std::map<int64_t, std::unique_ptr<ExtObject>> ppObjectsMap;
	ppVariantObject tmp(ppObjectsMap, resultVariant);
	std::map<const ExtObject*, ASObject*> asObjectsMap;
	*(result) = tmp.getASObject(asObjectsMap);
	return true;
}


std::map<PP_Instance,ppPluginInstance*> all_instances;

static PP_Bool Instance_DidCreate(PP_Instance instance,uint32_t argc,const char* argn[],const char* argv[]) 
{
	LOG(LOG_INFO,"Instance_DidCreate:"<<instance);
	ppPluginInstance* newinstance = new ppPluginInstance(instance,argc,argn,argv);
	
	all_instances[instance] = newinstance;
	g_inputevent_interface->RequestInputEvents(instance, PP_INPUTEVENT_CLASS_MOUSE);
	g_inputevent_interface->RequestFilteringInputEvents(instance, PP_INPUTEVENT_CLASS_WHEEL | PP_INPUTEVENT_CLASS_KEYBOARD);
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

static bool PPP_Class_HasProperty(void* object,struct PP_Var name,struct PP_Var* exception)
{
	setTLSSys(((ppExtScriptObject*)object)->getSystemState());
	LOG_CALL("PPP_Class_HasProperty");
	uint32_t len;
	switch (name.type)
	{
		case PP_VARTYPE_INT32:
			return ((ppExtScriptObject*)object)->hasProperty(ExtIdentifier(name.value.as_int));
		case PP_VARTYPE_STRING:
			return ((ppExtScriptObject*)object)->hasProperty(g_var_interface->VarToUtf8(name,&len));
		default:
			LOG(LOG_NOT_IMPLEMENTED,"PPP_Class_HasProperty for type "<<(int)name.type);
			break;
	}
	return false;
	
}
static bool PPP_Class_HasMethod(void* object,struct PP_Var name,struct PP_Var* exception)
{
	setTLSSys(((ppExtScriptObject*)object)->getSystemState());
	LOG_CALL("PPP_Class_Method");
	uint32_t len;
	switch (name.type)
	{
		case PP_VARTYPE_INT32:
			return ((ppExtScriptObject*)object)->hasMethod(ExtIdentifier(name.value.as_int));
		case PP_VARTYPE_STRING:
			return ((ppExtScriptObject*)object)->hasMethod(g_var_interface->VarToUtf8(name,&len));
		default:
			LOG(LOG_NOT_IMPLEMENTED,"PPP_Class_HasMethod for type "<<(int)name.type);
			break;
	}
	return false;
}
static struct PP_Var PPP_Class_GetProperty(void* object,struct PP_Var name,struct PP_Var* exception)
{
	setTLSSys(((ppExtScriptObject*)object)->getSystemState());
	LOG_CALL("PPP_Class_GetProperty");
	ExtVariant v;
	uint32_t len;
	switch (name.type)
	{
		case PP_VARTYPE_INT32:
			v = ((ppExtScriptObject*)object)->getProperty(ExtIdentifier(name.value.as_int));
			break;
		case PP_VARTYPE_STRING:
			v = ((ppExtScriptObject*)object)->getProperty(g_var_interface->VarToUtf8(name,&len));
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"PPP_Class_HasMethod for type "<<(int)name.type);
			break;
	}
	PP_Var result;
	std::map<const ExtObject*, PP_Var> objectsMap;
	ppVariantObject::ExtVariantToppVariant(objectsMap,((ppExtScriptObject*)object)->getInstance()->getppInstance(),v, result);
	return result;
}
static void PPP_Class_GetAllPropertyNames(void* object,uint32_t* property_count,struct PP_Var** properties,struct PP_Var* exception)
{
	setTLSSys(((ppExtScriptObject*)object)->getSystemState());
	LOG_CALL("PPP_Class_GetAllPropertyNames");
	ExtIdentifier** ids = NULL;
	bool success = ((ppExtScriptObject*)object)->enumerate(&ids, property_count);
	if(success)
	{
		*properties = new PP_Var[*property_count];
		for(uint32_t i = 0; i < *property_count; i++)
		{
			switch (ids[i]->getType())
			{
				case ExtIdentifier::EI_STRING:
					*properties[i] = g_var_interface->VarFromUtf8(ids[i]->getString().c_str(),ids[i]->getString().length());
					break;
				case ExtIdentifier::EI_INT32:
					*properties[i] = PP_MakeInt32(ids[i]->getInt());
					break;
			}
			delete ids[i];
		}
	}
	else
	{
		properties = NULL;
		property_count = 0;
	}

	if(ids != NULL)
		delete ids;
}

static void PPP_Class_SetProperty(void* object,struct PP_Var name,struct PP_Var value,struct PP_Var* exception)
{
	setTLSSys(((ppExtScriptObject*)object)->getSystemState());
	LOG_CALL("PPP_Class_SetProperty");
	uint32_t len;
	std::map<int64_t, std::unique_ptr<ExtObject>> objectsMap;
	switch (name.type)
	{
		case PP_VARTYPE_INT32:
			((ppExtScriptObject*)object)->setProperty(ExtIdentifier(name.value.as_int),ppVariantObject(objectsMap,value));
			break;
		case PP_VARTYPE_STRING:
			((ppExtScriptObject*)object)->setProperty(ExtIdentifier(g_var_interface->VarToUtf8(name,&len)),ppVariantObject(objectsMap,value));
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"PPP_Class_setProperty for type "<<(int)name.type);
			break;
	}
}

static void PPP_Class_RemoveProperty(void* object,struct PP_Var name,struct PP_Var* exception)
{
	setTLSSys(((ppExtScriptObject*)object)->getSystemState());
	LOG_CALL("PPP_Class_RemoveProperty");
	uint32_t len;
	switch (name.type)
	{
		case PP_VARTYPE_INT32:
			((ppExtScriptObject*)object)->removeProperty(ExtIdentifier(name.value.as_int));
			break;
		case PP_VARTYPE_STRING:
			((ppExtScriptObject*)object)->removeProperty(ExtIdentifier(g_var_interface->VarToUtf8(name,&len)));
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"PPP_Class_removeProperty for type "<<(int)name.type);
			break;
	}
}

static struct PP_Var PPP_Class_Call(void* object,struct PP_Var name,uint32_t argc,struct PP_Var* argv,struct PP_Var* exception)
{
	setTLSSys(((ppExtScriptObject*)object)->getSystemState());
	PP_Var objResult = PP_MakeUndefined();
	uint32_t len;
	ExtIdentifier method_name;
	switch (name.type)
	{
		case PP_VARTYPE_INT32:
			method_name=ExtIdentifier(name.value.as_int);
			break;
		case PP_VARTYPE_STRING:
			method_name=ExtIdentifier(g_var_interface->VarToUtf8(name,&len));
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"PPP_Class_Call for method name type "<<(int)name.type);
			return PP_MakeUndefined();
	}
	LOG_CALL("PPP_Class_Call:"<<((ppExtScriptObject*)object)->getInstance()->getSystemState()<<" "<< method_name.getString());
	std::map<int64_t, std::unique_ptr<ExtObject>> objectsMap;
	const ExtVariant** objArgs = g_newa(const ExtVariant*,argc);
	for (uint32_t i = 0; i < argc; i++)
	{
		objArgs[i]=new ppVariantObject(objectsMap,argv[i]);
	}
	((ppExtScriptObject*)object)->invoke(method_name,argc,objArgs,&objResult);
		
	LOG_CALL("PPP_Class_Call done:"<<method_name.getString());
	return objResult;
}

static struct PP_Var PPP_Class_Construct(void* object,uint32_t argc,struct PP_Var* argv,struct PP_Var* exception)
{
	LOG(LOG_NOT_IMPLEMENTED,"PPP_Class_Construct:"<<object);
	return PP_MakeUndefined();
}

static void PPP_Class_Deallocate(void* object)
{
	LOG(LOG_NOT_IMPLEMENTED,"PPP_Class_Deallocate:"<<object);
}

static PPP_Class_Deprecated ppp_class_deprecated_scriptobject = {
	&PPP_Class_HasProperty,
	&PPP_Class_HasMethod,
	&PPP_Class_GetProperty,
	&PPP_Class_GetAllPropertyNames,
	&PPP_Class_SetProperty,
	&PPP_Class_RemoveProperty,
	&PPP_Class_Call,
	&PPP_Class_Construct,
	&PPP_Class_Deallocate 
}; 

static PP_Var Instance_Private_GetInstanceObject(PP_Instance instance)
{
	auto it = all_instances.find(instance);
	if (it == all_instances.end())
	{
		LOG(LOG_ERROR,"Instance_Private_GetInstanceObject: no matching PPPluginInstance found");
		return PP_MakeNull();
	}
	ppExtScriptObject* scr = (ppExtScriptObject*)it->second->getSystemState()->extScriptObject;
	if (scr == NULL)
		return PP_MakeNull();
	scr->ppScriptObject = g_var_deprecated_interface->CreateObject(instance,&ppp_class_deprecated_scriptobject,(void*)it->second->getSystemState()->extScriptObject);
	return scr->ppScriptObject;
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
static PPP_Instance_Private instance_private_interface = {
	&Instance_Private_GetInstanceObject,
};
static PP_Bool InputEvent_HandleInputEvent(PP_Instance instance, PP_Resource input_event)
{
	auto it = all_instances.find(instance);
	if (it == all_instances.end())
	{
		LOG(LOG_ERROR,"InputEvent_HandleInputEvent: no matching PPPluginInstance found");
		return PP_FALSE;
	}
	ppPluginInstance* info = it->second;
	return info->handleInputEvent(input_event);
}

static PPP_InputEvent input_event_interface = {
	&InputEvent_HandleInputEvent,
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
		EngineData::sdl_needinit = false;
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
		g_instance_private_interface = (const PPB_Instance_Private*)get_browser_interface(PPB_INSTANCE_PRIVATE_INTERFACE);
		g_var_deprecated_interface = (const PPB_Var_Deprecated*)get_browser_interface(PPB_VAR_DEPRECATED_INTERFACE);
		g_inputevent_interface = (const PPB_InputEvent*)get_browser_interface(PPB_INPUT_EVENT_INTERFACE);
		g_mouseinputevent_interface = (const PPB_MouseInputEvent*)get_browser_interface(PPB_MOUSE_INPUT_EVENT_INTERFACE);
		g_keyboardinputevent_interface = (const PPB_KeyboardInputEvent*)get_browser_interface(PPB_KEYBOARD_INPUT_EVENT_INTERFACE);
		g_wheelinputevent_interface = (const PPB_WheelInputEvent*)get_browser_interface(PPB_WHEEL_INPUT_EVENT_INTERFACE);
		g_flashclipboard_interface = (const PPB_Flash_Clipboard*)get_browser_interface(PPB_FLASH_CLIPBOARD_INTERFACE);
		g_fileio_interface = (const PPB_FileIO*)get_browser_interface(PPB_FILEIO_INTERFACE);
		g_fileref_interface = (const PPB_FileRef*)get_browser_interface(PPB_FILEREF_INTERFACE);
		g_filesystem_interface = (const PPB_FileSystem*)get_browser_interface(PPB_FILESYSTEM_INTERFACE);
		
		if (!g_core_interface ||
				!g_instance_interface || 
				!g_graphics_3d_interface || 
				!g_view_interface || 
				!g_var_interface ||
				!g_urlloader_interface ||
				!g_urlrequestinfo_interface ||
				!g_urlresponseinfo_interface ||
				!g_gles2_interface ||
				!g_urlloadedtrusted_interface ||
				!g_instance_private_interface ||
				!g_var_deprecated_interface ||
				!g_inputevent_interface ||
				!g_mouseinputevent_interface ||
				!g_keyboardinputevent_interface ||
			    !g_wheelinputevent_interface ||
				!g_flashclipboard_interface ||
				!g_fileio_interface ||
				!g_fileref_interface ||
				!g_filesystem_interface)
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
				<< g_urlloadedtrusted_interface<<" "
				<< g_instance_private_interface<<" "
				<< g_var_deprecated_interface<<" "
				<< g_inputevent_interface<<" "
				<< g_mouseinputevent_interface<<" "
				<< g_keyboardinputevent_interface<<" "
				<< g_wheelinputevent_interface<<" "
				<< g_flashclipboard_interface<<" "
				<< g_fileio_interface<<" "
				<< g_fileref_interface<<" "
				<< g_filesystem_interface);
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
		if (strcmp(interface_name, PPP_INSTANCE_PRIVATE_INTERFACE) == 0) 
		{
			return &instance_private_interface;
		}
		if (strcmp(interface_name, PPP_INPUT_EVENT_INTERFACE) == 0) 
		{
			return &input_event_interface;
		}
		return NULL;
	}

}

void ppPluginEngineData::stopMainDownload()
{
	LOG(LOG_NOT_IMPLEMENTED,"stopMainDownload");
//	if(instance->mainDownloader)
//		instance->mainDownloader->stop();
}

uint32_t ppPluginEngineData::getWindowForGnash()
{
//	return instance->mWindow;
	return 0;
}
struct userevent_callbackdata
{
	void (*func) (SystemState*);
	SystemState* sys;
};

void exec_ppPluginEngineData_callback(void* userdata,int result)
{
	userevent_callbackdata* data= (userevent_callbackdata*)userdata;
	data->func(data->sys);
	delete data;
}
void ppPluginEngineData::runInMainThread(SystemState* sys, void (*func) (SystemState*) )
{
	userevent_callbackdata* ue = new userevent_callbackdata;
	ue->func=func;
	ue->sys=sys;
	if (!g_core_interface->IsMainThread())
	{
		g_core_interface->CallOnMainThread(0,PP_MakeCompletionCallback(exec_ppPluginEngineData_callback,ue),0);
	}
	else
		exec_ppPluginEngineData_callback(ue,0);
}

void ppPluginEngineData::openPageInBrowser(const tiny_string& url, const tiny_string& window)
{
	LOG(LOG_NOT_IMPLEMENTED,"openPageInBrowser:"<<url<<" "<<window);
	//instance->openLink(url, window);
}

SDL_Window* ppPluginEngineData::createWidget(uint32_t w,uint32_t h)
{
	return 0;
}

void ppPluginEngineData::grabFocus()
{
	LOG(LOG_NOT_IMPLEMENTED,"grabFocus");
	/*
	if (!widget_gtk)
		return;

	gtk_widget_grab_focus(widget_gtk);
	*/
}

void ppPluginEngineData::setClipboardText(const std::string txt)
{
	uint32_t formats[1];
	formats[0] = PP_FLASH_CLIPBOARD_FORMAT_PLAINTEXT;
	PP_Var data[1];
	data[0] = g_var_interface->VarFromUtf8(txt.c_str(),txt.length());
	g_flashclipboard_interface->WriteData(this->instance->getppInstance(),PP_FLASH_CLIPBOARD_TYPE_STANDARD,1,formats,data);
}

bool ppPluginEngineData::getScreenData(SDL_DisplayMode *screen)
{
	LOG(LOG_NOT_IMPLEMENTED,"getScreenData");
	return true;
}

double ppPluginEngineData::getScreenDPI()
{
	LOG(LOG_NOT_IMPLEMENTED,"getScreenDPI");
	return 96.0;
}

StreamCache *ppPluginEngineData::createFileStreamCache()
{
	return new ppFileStreamCache(instance);
}

void swapbuffer_ppPluginEngineData_callback(void* userdata,int result)
{
	ppPluginEngineData* data = (ppPluginEngineData*)userdata;
	g_graphics_3d_interface->SwapBuffers(data->getGraphics(),PP_MakeCompletionCallback(swapbuffer_callback,userdata));
	
}
void ppPluginEngineData::SwapBuffers()
{
	if (!g_core_interface->IsMainThread())
	{
		g_core_interface->CallOnMainThread(0,PP_MakeCompletionCallback(swapbuffer_ppPluginEngineData_callback,this),0);
		swapbuffer_rendering.wait();
	}
	else
		swapbuffer_ppPluginEngineData_callback(this,0);
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

uint8_t *ppPluginEngineData::getCurrentPixBuf() const
{
	return currentPixelBufPtr;
}

uint8_t *ppPluginEngineData::switchCurrentPixBuf(uint32_t w, uint32_t h)
{
	//TODO See if a more elegant way of handling the non-PBO case can be found.
	//for now, each frame is uploaded one at a time synchronously to the server
	if(!currentPixelBufPtr)
		if(posix_memalign((void **)&currentPixelBufPtr, 16, w*h*4)) {
			LOG(LOG_ERROR, "posix_memalign could not allocate memory");
			return NULL;
		}
	return currentPixelBufPtr;
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
	// TODO calling this generates error in chromium:
	// [.PPAPIContext]GL ERROR :GL_INVALID_ENUM : glEnable: cap was GL_TEXTURE_2D
	//g_gles2_interface->Enable(instance->m_graphics,GL_TEXTURE_2D);
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

void ppPluginEngineData::exec_glDeleteBuffers()
{
	g_gles2_interface->DeleteBuffers(instance->m_graphics,2,pixelBuffers);
}

void ppPluginEngineData::exec_glBlendFunc_GL_ONE_GL_ONE_MINUS_SRC_ALPHA()
{
	g_gles2_interface->BlendFunc(instance->m_graphics,GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void ppPluginEngineData::exec_glActiveTexture_GL_TEXTURE0()
{
	g_gles2_interface->ActiveTexture(instance->m_graphics,GL_TEXTURE0);
}

void ppPluginEngineData::exec_glGenBuffers()
{
	g_gles2_interface->GenBuffers(instance->m_graphics,2,pixelBuffers);
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

void ppPluginEngineData::exec_glTexSubImage2D_GL_TEXTURE_2D(int32_t level,int32_t xoffset,int32_t yoffset,int32_t width,int32_t height,const void* pixels, uint32_t w, uint32_t curX, uint32_t curY)
{
	//We need to copy the texture area to a contiguous memory region first,
	//as GLES2 does not support UNPACK state (skip pixels, skip rows, row_lenght).
	uint8_t *gdata = new uint8_t[4*width*height];
	for(int j=0;j<height;j++) {
		memcpy(gdata+4*j*width, ((uint8_t *)pixels)+4*w*(j+curY)+4*curX, width*4);
	}
	g_gles2_interface->TexSubImage2D(instance->m_graphics,GL_TEXTURE_2D, level, xoffset, yoffset, width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_HOST, gdata);
	delete[] gdata;
}
void ppPluginEngineData::exec_glGetIntegerv_GL_MAX_TEXTURE_SIZE(int32_t* data)
{
	g_gles2_interface->GetIntegerv(instance->m_graphics,GL_MAX_TEXTURE_SIZE,data);
}
