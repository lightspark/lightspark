/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2012  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2011       Matthias Gehre (M.Gehre@gmx.de)

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

#include "platforms/engineutils.h"

#ifdef _WIN32
#	include <gdk/gdkwin32.h>
#else
#	include <sys/resource.h>
#	include <gdk/gdkx.h>
#endif


using namespace std;
using namespace lightspark;

EngineData::EngineData() : widget(0), inputHandlerId(0), sizeHandlerId(0), width(0), height(0), window(0)
{
}

EngineData::~EngineData()
{
	RecMutex::Lock l(mutex);
	removeSizeChangeHandler();
	removeInputHandler();
}

/* gtk main loop handling */
static void gtk_main_runner()
{
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
}

Thread* EngineData::gtkThread = NULL;

/* This is not run in the linux plugin, as firefox
 * runs its own gtk_main, which we must not interfere with.
 */
void EngineData::startGTKMain()
{
	assert(!gtkThread);
#ifdef HAVE_NEW_GLIBMM_THREAD_API
		gtkThread = Thread::create(sigc::ptr_fun(&gtk_main_runner));
#else
		gtkThread = Thread::create(sigc::ptr_fun(&gtk_main_runner),true);
#endif
}

void EngineData::quitGTKMain()
{
	assert(gtkThread);
	gdk_threads_enter();
	gtk_main_quit();
	gdk_threads_leave();
	gtkThread->join();
	gtkThread = NULL;
}

/* This function must be called from the gtk main thread
	 * and within gdk_threads_enter/leave */
void EngineData::showWindow(uint32_t w, uint32_t h)
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
