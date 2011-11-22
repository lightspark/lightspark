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

class EngineData
{
private:
	GtkWidget* widget;
	Mutex handlerMutex;
        sigc::slot<bool,GdkEvent*> inputHandler;
	sigc::slot<void,int32_t,int32_t> sizeHandler;
public:
	int width;
	int height;
#ifdef _WIN32
	HWND window;
#else
	Window window;
	VisualID visual;
#endif
	EngineData(GtkWidget* wid, int w, int h)
		: widget(wid), width(w), height(h)
	{
		gtk_widget_realize(widget);
#ifdef _WIN32
		window = (HWND)GDK_WINDOW_HWND(gtk_widget_get_window(widget));
#else
		window = GDK_WINDOW_XID(gtk_widget_get_window(widget));
#endif
	}
	virtual ~EngineData() {}
	virtual bool isSizable() const = 0;
	virtual void stopMainDownload() = 0;
	/* This functions runs in the thread of gtk_main() */
	static int callHelper(sigc::slot<void>* slot)
	{
		(*slot)();
		delete slot;
		/* we must return 'false' or gtk will call this periodically */
		return FALSE;
	}
	/* Runs 'func' in the thread of gtk_main() */
	static void setupMainThreadCallback(const sigc::slot<void>& func)
	{
		g_idle_add((GSourceFunc)callHelper,new sigc::slot<void>(func));
	}
	/* This function must be called from the gtk main thread */
	void showWindow(uint32_t w, uint32_t h)
	{
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
	/* This function must be called from the gtk_main() thread */
	void setInputHandler(const sigc::slot<bool,GdkEvent*>& ic)
	{
		Mutex::Lock l(handlerMutex);
		inputHandler = ic;
		gtk_widget_set_can_focus(widget,TRUE);
		gtk_widget_add_events(widget,GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
						GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK);
		g_signal_connect(widget, "event", G_CALLBACK(inputDispatch), this);
	}
	void removeInputHandler()
	{
		Mutex::Lock l(handlerMutex);
		inputHandler = sigc::slot<bool,GdkEvent*>();
	}
	static void sizeDispatch(GtkWidget* widget, GdkRectangle* allocation, EngineData* e)
	{
		Mutex::Lock l(e->handlerMutex);
		if(!e->sizeHandler.empty())
			e->sizeHandler(allocation->width, allocation->height);
	}
	/* This function must be called from the gtk_main() thread */
	void setSizeChangeHandler(const sigc::slot<void,int32_t,int32_t>& sc)
	{
		Mutex::Lock l(handlerMutex);
		sizeHandler = sc;
		g_signal_connect(widget, "size-allocate", G_CALLBACK(sizeDispatch), this);
	}
	void removeSizeChangeHandler()
	{
		Mutex::Lock l(handlerMutex);
		sizeHandler = sigc::slot<void,int32_t,int32_t>();
	}
};

};
#endif
