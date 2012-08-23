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

#ifndef PLUGIN_PLUGIN_H
#define PLUGIN_PLUGIN_H 1

#include "swf.h"
#include <iostream>
#include <sstream>

#include "compat.h"
#include "plugin/include/pluginbase.h"
#include "parsing/streams.h"
#include "backends/netutils.h"
#include "backends/urlutils.h"
#include "plugin/npscriptobject.h"

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
	lightspark::Downloader* download(const lightspark::URLInfo& url, bool cached, lightspark::ILoadable* owner);
	lightspark::Downloader* downloadWithData(const lightspark::URLInfo& url, const std::vector<uint8_t>& data, 
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
	NPDownloader(const lightspark::tiny_string& _url, bool _cached, NPP _instance, lightspark::ILoadable* owner);
	NPDownloader(const lightspark::tiny_string& _url, const std::vector<uint8_t>& _data,
			const std::list<tiny_string>& headers, NPP _instance, lightspark::ILoadable* owner);
};

class PluginEngineData:	public EngineData
{
private:
	nsPluginInstance* instance;
public:
	PluginEngineData(nsPluginInstance* i, uint32_t w, uint32_t h) : instance(i)
	{
		width = w;
		height = h;
	}
	/* The widget must not be gtk_widget_destroy'ed in the destructor. This is done
	 * by firefox.
	 */
	~PluginEngineData() {}

	void stopMainDownload();
	bool isSizable() const { return false; }
	GdkNativeWindow getWindowForGnash();
	/* must be called within the gtk_main() thread and within gdk_threads_enter/leave */
	GtkWidget* createGtkWidget();
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

	// locals
	const char * getVersion();
	void draw();

	lightspark::SystemState* m_sys;
private:
	std::string getPageURL() const;
	void asyncDownloaderDestruction(const char *url, NPDownloader* dl) const;
	void downloaderFinished(NPDownloader* dl, const char *url, NPReason reason) const;

	NPP mInstance;
	NPBool mInitialized;

	/* This is the window we got from firefox. It is not the
	 * window we draw into. We create a child of mWindow and
	 * draw into that.
	 */
	GdkNativeWindow mWindow;
	int mX, mY;

	std::istream mainDownloaderStream;
	NPDownloader* mainDownloader;
	NPScriptObjectGW* scriptObject;
	lightspark::ParseThread* m_pt;
};

}
#endif /* PLUGIN_PLUGIN_H */
