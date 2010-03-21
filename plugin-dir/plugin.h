/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

/* Xlib/Xt stuff */
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>

#include <iostream>
#include <sstream>

#include "pluginbase.h"
#include "swf.h"
#include "streams.h"
#include <GL/glx.h>

class nsPluginInstance : public nsPluginInstanceBase
{
public:
	nsPluginInstance(NPP aInstance, int16 argc, char** argn, char** argv);
	virtual ~nsPluginInstance();

	NPBool init(NPWindow* aWindow);
	void shut();
	NPBool isInitialized() {return mInitialized;}
	NPError GetValue(NPPVariable variable, void *value);
	NPError SetWindow(NPWindow* aWindow);
	NPError NewStream(NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype); 
	int32   Write(NPStream *stream, int32 offset, int32 len, void *buffer);
	int32   WriteReady(NPStream *stream);

	// locals
	const char * getVersion();
	void draw();

private:
	int hexToInt(char c);

	NPP mInstance;
	NPBool mInitialized;

	Window mWindow;
	Display *mDisplay;
	Widget mXtwidget;
	int mX, mY;
	int mWidth, mHeight;
	Visual* mVisual;
	Colormap mColormap;
	unsigned int mDepth;

	std::istream swf_stream;
	sync_stream swf_buf;

	lightspark::SystemState m_sys;
	lightspark::ParseThread m_pt;
	lightspark::InputThread* m_it;
	lightspark::RenderThread* m_rt;
};

#endif // __PLUGIN_H__
