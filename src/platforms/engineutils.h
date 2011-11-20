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

#include <gtk/gtk.h>
#ifdef _WIN32
#	include <gdk/gdkwin32.h>
#else
#	include <sys/resource.h>
#	include <gdk/gdkx.h>
#endif

namespace lightspark
{

/*
 * On Linux, both standalone and plugin use GTKEngineData.
 * On Win32, the standalone uses GTKEngineData and the plugin uses HWNDEngineData.
 * Remember: For win32 portability, gtk/gdk functions may only be called from within the gtk main thread.
 * Use g_idle_add to enqueue a function in that thread.
 */
class EngineData
{
public:
	EngineData(int w, int h) : width(w),height(h) {}
	int width;
	int height;

	virtual ~EngineData() {}
	/* This calls 'func' from within the gtk main thread */
	virtual void setupMainThreadCallback(const sigc::slot<void>& func) = 0;
	virtual void stopMainDownload() = 0;
	virtual bool isSizable() const = 0;
	/* This function must be called from the gtk main thread */
	virtual void showWindow(uint32_t w, uint32_t h) = 0;
	/* hwnd, window and visual are only valid after a call to 'showWindow' */
#ifdef _WIN32
	HWND window;
#else
	Window window;
	VisualID visual;
#endif
	Mutex handlerMutex;
	sigc::slot<bool,GdkEvent*> inputHandler;
	virtual void setInputHandler(const sigc::slot<bool,GdkEvent*>& ic) = 0;
	virtual void removeInputHandler()
	{
		Mutex::Lock l(handlerMutex);
		inputHandler = sigc::slot<bool,GdkEvent*>();
	}
	sigc::slot<void,int32_t,int32_t> sizeHandler;
	virtual void setSizeChangeHandler(const sigc::slot<void,int32_t,int32_t>& sc) = 0;
	virtual void removeSizeChangeHandler()
	{
		Mutex::Lock l(handlerMutex);
		sizeHandler = sigc::slot<void,int32_t,int32_t>();
	}
};

class GtkEngineData: public EngineData
{
protected:
	GtkWidget* widget;
public:
	GtkEngineData(GtkWidget* wid, int w, int h)
		: EngineData(w,h), widget(wid)
	{
	}
	void showWindow(uint32_t w, uint32_t h)
	{
		gtk_widget_realize(widget);
#ifdef _WIN32
		window = (HWND)GDK_WINDOW_HWND(gtk_widget_get_window(widget));
#else
		window = GDK_WINDOW_XID(gtk_widget_get_window(widget));
#endif
		if(isSizable())
		{
			gtk_widget_set_size_request(widget, w, h);
			width = w;
			height = h;
		}
		gtk_widget_show(widget);
		gtk_widget_map(widget);
	}
	static gboolean inputDispatch(GtkWidget *widget, GdkEvent *event, EngineData* e)
	{
		Mutex::Lock l(e->handlerMutex);
		if(!e->inputHandler.empty())
			return e->inputHandler(event);
		return false;
	}
	void setInputHandler(const sigc::slot<bool,GdkEvent*>& ic)
	{
		Mutex::Lock l(handlerMutex);
		inputHandler = ic;
		gtk_widget_set_can_focus(widget,TRUE);
		gtk_widget_add_events(widget,GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
						GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK);
		g_signal_connect(widget, "event", G_CALLBACK(inputDispatch), this);
	}
	static void sizeDispatch(GtkWidget* widget, GdkRectangle* allocation, EngineData* e)
	{
		Mutex::Lock l(e->handlerMutex);
		if(!e->sizeHandler.empty())
			e->sizeHandler(allocation->width, allocation->height);
	}
	void setSizeChangeHandler(const sigc::slot<void,int32_t,int32_t>& sc)
	{
		Mutex::Lock l(handlerMutex);
		sizeHandler = sc;
		g_signal_connect(widget, "size-allocate", G_CALLBACK(sizeDispatch), this);
	}
};

#ifdef _WIN32
class HWNDEngineData: public EngineData
{
private:
	bool eventSetup;
	GdkWindow* gdkwin;
	void setupEventHandler()
	{
		if(eventSetup)
			return;
		eventSetup = true;
		gdk_event_handler_set((GdkEventFunc)&eventDispatch, this, NULL);
	}
public:
	HWNDEngineData(HWND hwnd, int w, int h): EngineData(w,h), eventSetup(false)
	{
		window = hwnd;
		gdkwin = gdk_window_foreign_new(hwnd);
	}
	void showWindow(uint32_t w, uint32_t h) {}
	/* guarded by gdk_threads_enter/leave */
	static void eventDispatch(GdkEvent *event, HWNDEngineData* e)
	{
		Mutex::Lock l(e->handlerMutex);
		if(!e->inputHandler.empty() && e->inputHandler(event))
			return; /* handled by inputHandler */

		if(!e->sizeHandler.empty() && event->type == GDK_CONFIGURE)
		{
			GdkEventConfigure* cfg = (GdkEventConfigure*)event;
			e->sizeHandler(cfg->width, cfg->height);
			return;
		}
		/*unhandled event*/
	}
	void setInputHandler(const sigc::slot<bool,GdkEvent*>& ic)
	{
		Mutex::Lock l(handlerMutex);
		inputHandler = ic;
		int mask = GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
					GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK | gdk_window_get_events(gdkwin);
		gdk_window_set_events(gdkwin, (GdkEventMask)mask);
		setupEventHandler();
	}
	void setSizeChangeHandler(const sigc::slot<void,int32_t,int32_t>& sc)
	{
		Mutex::Lock l(handlerMutex);
		sizeHandler = sc;
		int mask = GDK_STRUCTURE_MASK | gdk_window_get_events(gdkwin);
		gdk_window_set_events(gdkwin, (GdkEventMask)mask);
		setupEventHandler();
	}
};
#endif

};
#endif
