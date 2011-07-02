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

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "swf.h"
#include <iostream>
#include <sstream>

#include "pluginbase.h"
#include "parsing/streams.h"
#include "backends/netutils.h"
#include "backends/urlutils.h"
#include "npscriptobject.h"

class NPDownloader;
typedef void(*helper_t)(void*);

class NPDownloadManager: public lightspark::DownloadManager
{
private:
	NPP instance;
public:
	NPDownloadManager(NPP i);
	~NPDownloadManager();
	lightspark::Downloader* download(const lightspark::URLInfo& url, bool cached, lightspark::ILoadable* owner);
	lightspark::Downloader* downloadWithData(const lightspark::URLInfo& url, const std::vector<uint8_t>& data, 
			lightspark::ILoadable* owner);
	void destroy(lightspark::Downloader* downloader);
};

class nsPluginInstance;
class NPDownloader: public lightspark::Downloader
{
	friend class nsPluginInstance;
private:
	NPP instance;
	static void dlStartCallback(void* th);
public:
	enum STATE { INIT=0, STREAM_DESTROYED, ASYNC_DESTROY };
	STATE state;
	//Constructor used for the main file
	NPDownloader(const lightspark::tiny_string& _url, lightspark::ILoadable* owner);
	NPDownloader(const lightspark::tiny_string& _url, bool _cached, NPP _instance, lightspark::ILoadable* owner);
	NPDownloader(const lightspark::tiny_string& _url, const std::vector<uint8_t>& _data, NPP _instance, lightspark::ILoadable* owner);
};

class PluginEngineData: public lightspark::EngineData
{
private:
	nsPluginInstance* instance;
public:
	PluginEngineData(nsPluginInstance* i, Display* d, VisualID v, Window win, int w, int h);
	void setupMainThreadCallback(lightspark::ls_callback_t func, void* arg);
	void stopMainDownload();
	bool isSizable() const;
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
	void asyncDownloaderDestruction(NPStream* stream, NPDownloader* dl) const;

	NPP mInstance;
	NPBool mInitialized;

	GtkWidget* mContainer;
	Window mWindow;
	Display *mDisplay;
	int mX, mY;
	int mWidth, mHeight;
	Visual* mVisual;
	Colormap mColormap;
	unsigned int mDepth;

	std::istream mainDownloaderStream;
	NPDownloader* mainDownloader;
	NPScriptObjectGW* scriptObject;
	lightspark::ParseThread* m_pt;
};

#endif // __PLUGIN_H__
