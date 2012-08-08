/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef PLATFORMS_ENGINEUTILS_H
#define PLATFORMS_ENGINEUTILS_H 1

#include <gtk/gtk.h>
#ifdef _WIN32
#	define VOID void
#	include <gdk/gdkwin32.h>
#	undef VOID
#else
#	include <sys/resource.h>
#	include <gdk/gdkx.h>
#endif

#include "compat.h"
#include "threading.h"

namespace lightspark
{

/* There is GdkNativeWindow, but that is not HWND on win32!? */
#ifdef _WIN32
typedef HWND NativeWindow;
#else
typedef Window NativeWindow;
#endif

class DLL_PUBLIC EngineData
{
protected:
	GtkWidget* widget;
	/* use a recursive mutex, because g_signal_connect may directly call
	 * the specific handler */
	RecMutex mutex;
	sigc::slot<bool,GdkEvent*> inputHandler;
	gulong inputHandlerId;
	sigc::slot<void,int32_t,int32_t,bool> sizeHandler;
	gulong sizeHandlerId;
	/* This function must be called from the gtk main thread
	 * and within gdk_threads_enter/leave */
	virtual GtkWidget* createGtkWidget()=0;
	/* This functions runs in the thread of gtk_main() */
	static int callHelper(sigc::slot<void>* slot)
	{
		(*slot)();
		delete slot;
		/* we must return 'false' or gtk will call this periodically */
		return FALSE;
	}
	static Thread* gtkThread;
public:
	int width;
	int height;
	NativeWindow window;
#ifndef _WIN32
	VisualID visual;
#endif
	EngineData();
	virtual ~EngineData();
	virtual bool isSizable() const = 0;
	virtual void stopMainDownload() = 0;
	/* you may not call getWindowForGnash and showWindow on the same EngineData! */
	virtual NativeWindow getWindowForGnash()=0;
	/* Runs 'func' in the thread of gtk_main() */
	static void runInGtkThread(const sigc::slot<void>& func)
	{
		g_idle_add((GSourceFunc)callHelper,new sigc::slot<void>(func));
	}
	/* This function must be called from the gtk main thread
	 * and within gdk_threads_enter/leave */
	void showWindow(uint32_t w, uint32_t h)
	{
		RecMutex::Lock l(mutex);

		assert(!widget);
		widget = createGtkWidget();
		/* create a window handle */
		gtk_widget_realize(widget);
#if _WIN32
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
		RecMutex::Lock l(e->mutex);
		if(!e->inputHandler.empty())
			return e->inputHandler(event);
		return false;
	}
	/* This function must be called from the gtk_main() thread */
	void setInputHandler(const sigc::slot<bool,GdkEvent*>& ic)
	{
		RecMutex::Lock l(mutex);
		if(!widget)
			return;

		assert(!inputHandlerId);
		inputHandler = ic;
		gtk_widget_set_can_focus(widget,TRUE);
		gtk_widget_add_events(widget,GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK |
						GDK_POINTER_MOTION_MASK | GDK_EXPOSURE_MASK);
		inputHandlerId = g_signal_connect(widget, "event", G_CALLBACK(inputDispatch), this);
	}
	void removeInputHandler()
	{
		RecMutex::Lock l(mutex);
		if(!inputHandler.empty() && widget)
		{
			g_signal_handler_disconnect(widget, inputHandlerId);
			inputHandler = sigc::slot<bool,GdkEvent*>();
		}
	}
	static void sizeDispatch(GtkWidget* widget, GdkRectangle* allocation, EngineData* e)
	{
		RecMutex::Lock l(e->mutex);
		if(!e->sizeHandler.empty() && widget)
			e->sizeHandler(allocation->width, allocation->height, false);
	}
	/* This function must be called from the gtk_main() thread */
	void setSizeChangeHandler(const sigc::slot<void,int32_t,int32_t,bool>& sc)
	{
		RecMutex::Lock l(mutex);
		if(!widget)
			return;

		assert(!sizeHandlerId);
		sizeHandler = sc;
		sizeHandlerId = g_signal_connect(widget, "size-allocate", G_CALLBACK(sizeDispatch), this);
	}
	void removeSizeChangeHandler()
	{
		RecMutex::Lock l(mutex);
		if(!sizeHandler.empty() && widget)
		{
			g_signal_handler_disconnect(widget, sizeHandlerId);
			sizeHandler = sigc::slot<void,int32_t,int32_t,bool>();
		}
	}

	static void startGTKMain();
	static void quitGTKMain();
};

};
#endif /* PLATFORMS_ENGINEUTILS_H */
