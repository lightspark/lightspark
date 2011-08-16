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

#include "platforms/engineutils.h"
#include "swf.h"

using namespace lightspark;

TLSDATA DLL_PUBLIC SystemState* sys;

void GtkEngineData::onDelayedStart(int32_t requestedWidth, int32_t requestedHeight)
{
	//Create a plug in the XEmbed window
	GtkWidget* plug=gtk_plug_new(window);
	if(isSizable())
	{
		if(isStandalone())
			gtk_widget_set_size_request(plug,
				requestedWidth, requestedHeight);
		width=requestedWidth;
		height=requestedHeight;
	}
	container = plug;
	//Realize the widget now, as we need the X window
	gtk_widget_realize(plug);
	//Show it now
	gtk_widget_show(plug);
	gtk_widget_map(plug);
	if (isStandalone())
	{
		gtk_widget_set_can_focus(plug, true);
		gtk_widget_grab_focus(plug);
	}
	window=GDK_WINDOW_XID(gtk_widget_get_window(plug));
	XSync(display, False);
}

StandaloneEngineData::StandaloneEngineData(RENDER_MODE r, int argc, char** argv):
	GtkEngineData(r)
{
	//Make GTK thread enabled
	g_thread_init(NULL);
	gdk_threads_init();
	//Give GTK a chance to parse its own options.
	//We don't really care what remaining options it returns.
	gtk_init (&argc, &argv);

	gdk_threads_enter();
	//Create the main window
	GtkWidget* gtkWindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title((GtkWindow*)gtkWindow,"Lightspark");
	g_signal_connect(gtkWindow,"destroy",G_CALLBACK(gtkDestroy),NULL);
	GtkWidget* socket=gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(gtkWindow), socket);
	gtk_widget_show(socket);
	gtk_widget_show(gtkWindow);

	visual=XVisualIDFromVisual(gdk_x11_visual_get_xvisual(gdk_visual_get_system()));
	display=gdk_x11_display_get_xdisplay(gdk_display_get_default());
	window=gtk_socket_get_id((GtkSocket*)socket);
	width = 0;
	height = 0;
	gdk_threads_leave();
}

void StandaloneEngineData::gtkDestroy(GtkWidget *widget, gpointer data)
{
	sys->setShutdownFlag();
	gtk_main_quit();
}

void StandaloneEngineData::mainThreadCallback(ls_callback_t func, void* arg)
{
	//Synchronizing with the main gtk thread is what we actually need
	gdk_threads_enter();
	func(arg);
	gdk_threads_leave();
}

void StandaloneEngineData::onSetShutdownFlag()
{
	gtk_main_quit();
}

void StandaloneEngineData::doMain()
{
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
}
