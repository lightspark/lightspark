/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include <SDL2/SDL.h>
#include "compat.h"
#include "threading.h"
#include "tiny_string.h"

namespace lightspark
{

#ifndef _WIN32
//taken from X11/X.h
typedef unsigned long VisualID;
typedef unsigned long XID;
#endif
#define LS_USEREVENT_INIT EngineData::userevent
#define LS_USEREVENT_EXEC EngineData::userevent+1
#define LS_USEREVENT_QUIT EngineData::userevent+2
class SystemState;
class DLL_PUBLIC EngineData
{
	friend class RenderThread;
protected:
	/* use a recursive mutex, because g_signal_connect may directly call
	 * the specific handler */
	RecMutex mutex;
	virtual SDL_Window* createWidget(uint32_t w,uint32_t h)=0;
public:
	SDL_Window* widget;
	static uint32_t userevent;
	static Thread* mainLoopThread;
	int width;
	int height;
#ifndef _WIN32
	XID windowID;
	VisualID visual;
#endif
	EngineData();
	virtual ~EngineData();
	virtual bool isSizable() const = 0;
	virtual void stopMainDownload() = 0;
	/* you may not call getWindowForGnash and showWindow on the same EngineData! */
	virtual uint32_t getWindowForGnash()=0;
	/* Runs 'func' in the mainLoopThread */
	static void runInMainThread(void (*func) (SystemState*) )
	{
		SDL_Event event;
		SDL_zero(event);
		event.type = LS_USEREVENT_EXEC;
		event.user.data1 = (void*)func;
		SDL_PushEvent(&event);
	}
	static bool mainloop_handleevent(SDL_Event* event,SystemState* sys);
	static gboolean mainloop_from_plugin(SystemState* sys);
	
	/* This function must be called from mainLoopThread
	 * It fills this->widget and this->window.
	 */
	void showWindow(uint32_t w, uint32_t h);
	/* must be called within mainLoopThread */
	virtual void grabFocus()=0;
	virtual void openPageInBrowser(const tiny_string& url, const tiny_string& window)=0;

	static bool mainthread_running;
	static Semaphore mainthread_initialized;
	static bool startSDLMain();

	/* show/hide mouse cursor, must be called from mainLoopThread */
	static void showMouseCursor(SystemState *sys);
	static void hideMouseCursor(SystemState *sys);
	virtual void setClipboardText(const std::string txt);
	virtual bool getScreenData(SDL_DisplayMode* screen) = 0;
	virtual double getScreenDPI() = 0;
};

};
#endif /* PLATFORMS_ENGINEUTILS_H */
