/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef ENGINEUTILS_H
#define ENGINEUTILS_H

#include "compat.h"
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

namespace lightspark
{

enum RENDER_MODE { RENDER_NONE=0, RENDER_AUDIO=1, RENDER_VIDEO=2, RENDER_AUDIOVIDEO=3 };

typedef void (*ls_callback_t)(void*);

// Use this class for headless instances.
class DLL_PUBLIC EngineData
{
private:
	RENDER_MODE renderMode;
public:
	EngineData(RENDER_MODE r): renderMode(r) {}
	virtual ~EngineData() {}
	virtual void mainThreadCallback(ls_callback_t func, void* arg)
	{ func(arg); }
	virtual void stopMainDownload() {}
	virtual bool isVisual() const { return false; }
	virtual bool isStandalone() const { return true; }
	virtual void onDelayedStart(int32_t requestedWidth, int32_t requestedHeight) {}
	virtual void onSetShutdownFlag() {}
	virtual void doMain() {};
	RENDER_MODE getRenderMode() const { return renderMode; }
};

// Derive this class for instances that use GTK to output video (and possibly audio)
class DLL_PUBLIC GtkEngineData : public EngineData
{
protected:
	GtkEngineData(RENDER_MODE r): EngineData(r),container(NULL) {}
public:
	Display* display;
	VisualID visual;
	Window window;
	GtkWidget* container;
	int width;
	int height;
	virtual ~GtkEngineData() {}
	virtual bool isSizable() const = 0;
	bool isVisual() const { return true; }

	void onDelayedStart(int32_t requestedWidth, int32_t requestedHeight);
};

// EngineData for the standalone GTK lightspark instances
class DLL_PUBLIC StandaloneEngineData: public GtkEngineData
{
public:
	StandaloneEngineData(RENDER_MODE r, int argc, char** argv);
	static void init(int argc, char** argv);
	static void gtkDestroy(GtkWidget *widget, gpointer data);
	void mainThreadCallback(ls_callback_t func, void* arg);
	bool isSizable() const { return true; }
	bool isStandalone() const { return true; }

	void onSetShutdownFlag();
	virtual void doMain();
};

};
#endif
