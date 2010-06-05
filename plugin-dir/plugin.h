/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

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

/* Xlib/Xt stuff */
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#include <iostream>
#include <sstream>

#include "pluginbase.h"
#include "swf.h"
#include "streams.h"
#include "netutils.h"
#include <GL/glx.h>

class NPDownloader;

class NPDownloadManager: public lightspark::DownloadManager
{
private:
	std::list<std::pair<lightspark::tiny_string,NPDownloader*> > pendingLoads;
	NPP instance;
	sem_t mutex;
public:
	NPDownloadManager(NPP i);
	~NPDownloadManager();
	lightspark::Downloader* download(const lightspark::tiny_string& u);
	void destroy(lightspark::Downloader* d);
	NPDownloader* getDownloaderForUrl(const char* u);
};

class NPDownloader: public lightspark::Downloader
{
private:
	NPP instance;
	lightspark::tiny_string url;
	static void dlStartCallback(void* th);
public:
	bool started;
	NPDownloader(NPP i, const lightspark::tiny_string& u);
	void terminate();
};

class nsPluginInstance : public nsPluginInstanceBase
{
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
	void    URLNotify(const char* url, NPReason reason, void* notifyData);

	// locals
	const char * getVersion();
	void draw();

private:
	int hexToInt(char c);

	NPP mInstance;
	NPBool mInitialized;

	Window mWindow;
	Display *mDisplay;
	int mX, mY;
	int mWidth, mHeight;
	Visual* mVisual;
	Colormap mColormap;
	unsigned int mDepth;

	std::istream swf_stream;
	sync_stream swf_buf;

	lightspark::SystemState* m_sys;
	lightspark::ParseThread* m_pt;
	lightspark::InputThread* m_it;
	lightspark::RenderThread* m_rt;
};

#endif // __PLUGIN_H__
