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

#include "swf.h"
#include <iostream>
#include <sstream>

#include "pluginbase.h"
#include "parsing/streams.h"
#include "backends/netutils.h"
#include <GL/glx.h>

class NPDownloader;
typedef void(*helper_t)(void*);

class NPDownloadManager: public lightspark::DownloadManager
{
private:
	NPP instance;
public:
	NPDownloadManager(NPP i);
	~NPDownloadManager();
	lightspark::Downloader* download(const lightspark::tiny_string& u);
	void destroy(lightspark::Downloader* d);
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
	void    StreamAsFile(NPStream* stream, const char* fname);

	// locals
	const char * getVersion();
	void draw();

private:
	static void AsyncHelper(void* th, helper_t func, void* privArg);
	std::string getPageURL() const;

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

	std::istream swf_stream;
	sync_stream swf_buf;

	lightspark::SystemState* m_sys;
	lightspark::ParseThread* m_pt;
};

#endif // __PLUGIN_H__
