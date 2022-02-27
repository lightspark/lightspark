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

#ifndef PLUGIN_PLUGIN_H
#define PLUGIN_PLUGIN_H 1

#include "platforms/engineutils.h"
#include "swf.h"
#include <iostream>
#include <sstream>

#include "compat.h"
#include "plugin/include/pluginbase.h"
#include "parsing/streams.h"
#include "backends/netutils.h"
#include "backends/urlutils.h"
#include "plugin/npscriptobject.h"
#include "backends/lsopengl.h"

namespace lightspark
{

class NPDownloader;
typedef void(*helper_t)(void*);

class NPDownloadManager: public lightspark::StandaloneDownloadManager
{
private:
	NPP instance;
public:
	NPDownloadManager(NPP i);
	lightspark::Downloader* download(const lightspark::URLInfo& url,
					 _R<StreamCache> cache,
					 lightspark::ILoadable* owner);
	lightspark::Downloader* downloadWithData(const lightspark::URLInfo& url,
			_R<StreamCache> cache, const std::vector<uint8_t>& data,
			const std::list<tiny_string>& headers, lightspark::ILoadable* owner);
	void destroy(lightspark::Downloader* downloader);
};

class nsPluginInstance;
class NPDownloader: public lightspark::Downloader
{
	friend class nsPluginInstance;
private:
	NPP instance;
	bool cleanupInDestroyStream;
	static void dlStartCallback(void* th);
public:
	enum STATE { INIT=0, STREAM_DESTROYED, ASYNC_DESTROY };
	STATE state;
	//Constructor used for the main file
	NPDownloader(const lightspark::tiny_string& _url, lightspark::ILoadable* owner);
	NPDownloader(const lightspark::tiny_string& _url, _R<StreamCache> cache, NPP _instance, lightspark::ILoadable* owner);
	NPDownloader(const lightspark::tiny_string& _url, _R<StreamCache> cache, const std::vector<uint8_t>& _data,
			const std::list<tiny_string>& headers, NPP _instance, lightspark::ILoadable* owner);
};

class PluginEngineData:	public EngineData
{
friend class nsPluginInstance;
private:
	nsPluginInstance* instance;
	gulong inputHandlerId;
	gulong sizeHandlerId;
	SDL_GLContext mSDLContext;
	unsigned char * mPixels;
	bool inRendering;
	Mutex resizeMutex;
	int winposx;
	int winposy;
public:
	SystemState* sys;
	PluginEngineData(nsPluginInstance* i, uint32_t w, uint32_t h,SystemState* _sys);
	~PluginEngineData()
	{
		if(inputHandlerId)
			g_signal_handler_disconnect(widget, inputHandlerId);
		if(sizeHandlerId)
			g_signal_handler_disconnect(widget, sizeHandlerId);
		if (mPixels)
			delete[] mPixels;
	}
	void setupLocalStorage();
	void stopMainDownload() override;
	bool isSizable() const override { return false; }
	uint32_t getWindowForGnash() override;
	/* must be called within mainLoopThread */
	SDL_Window* createWidget(uint32_t w,uint32_t h) override;
	void setDisplayState(const tiny_string& displaystate,SystemState* sys) override;
	
	/* must be called within mainLoopThread */
	void grabFocus() override;
	void openPageInBrowser(const tiny_string& url, const tiny_string& window) override;
	bool getScreenData(SDL_DisplayMode* screen) override;
	double getScreenDPI() override;
	static void forceRedraw(SystemState* sys);
	void DoSwapBuffers() override;
	void InitOpenGL() override;
	void DeinitOpenGL() override;
	void draw(void *event, uint32_t evx, uint32_t evy, uint32_t evwidth, uint32_t evheight);
	void runInMainThread(SystemState *sys, void (*func)(SystemState *)) override;
	static void pluginCallHandler(void* d)
	{
		mainloop_from_plugin((SystemState*)d);
	}
	void openContextMenu() override;
};

class nsPluginInstance : public nsPluginInstanceBase
{
friend class PluginEngineData;
public:
	nsPluginInstance(NPP aInstance, int16_t argc, char** argn, char** argv);
	virtual ~nsPluginInstance();

	NPBool init(NPWindow* aWindow);
	void shut();
	NPBool isInitialized() {return mInitialized;}
	NPError GetValue(NPPVariable variable, void *value);
	NPError SetWindow(NPWindow* aWindow);
	NPError NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype); 
	NPError DestroyStream(NPStream *stream, NPError reason);
	int32_t Write(NPStream *stream, int32_t offset, int32_t len, void *buffer);
	int32_t WriteReady(NPStream *stream);
	void    StreamAsFile(NPStream* stream, const char* fname);
	void    URLNotify(const char* url, NPReason reason,
			void* notifyData);
	uint16_t  HandleEvent(void* event);
	void openLink(const tiny_string& url, const tiny_string& window);

	// locals
	const char * getVersion();

	lightspark::SystemState* m_sys;
private:
	std::string getPageURL() const;
	void asyncDownloaderDestruction(const char *url, NPDownloader* dl) const;
	void downloaderFinished(NPDownloader* dl, const char *url, NPReason reason) const;
	static void asyncOpenPage(void* data);

	NPP mInstance;
	NPBool mInitialized;

	/* This is the window we got from firefox. It is not the
	 * window we draw into. We create a child of mWindow and
	 * draw into that.
	 */
	uint32_t mWindow;
#ifdef XP_WIN
	HDC mDC;
#endif

	std::streambuf *mainDownloaderStreambuf;
	std::istream mainDownloaderStream;
	NPDownloader* mainDownloader;
	NPScriptObjectGW* scriptObject;
	lightspark::ParseThread* m_pt;
	gint64 lastclicktime;
};

}
#endif /* PLUGIN_PLUGIN_H */
