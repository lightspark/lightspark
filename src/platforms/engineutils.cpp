/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)
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
#include "swf.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>
#include "backends/input.h"
#include "backends/rendering.h"

using namespace std;
using namespace lightspark;

uint32_t EngineData::userevent = (uint32_t)-1;
Thread* EngineData::mainLoopThread = NULL;
bool EngineData::mainthread_running = false;
Semaphore EngineData::mainthread_initialized(0);
EngineData::EngineData() : widget(0), width(0), height(0),windowID(0),visual(0)
{
}

EngineData::~EngineData()
{
}
bool EngineData::mainloop_handleevent(SDL_Event* event,SystemState* sys)
{
	if (event->type == LS_USEREVENT_INIT)
	{
		sys = (SystemState*)event->user.data1;
		setTLSSys(sys);
	}
	else if (event->type == LS_USEREVENT_EXEC)
	{
		if (event->user.data1)
			((void (*)(SystemState*))event->user.data1)(sys);
	}
	else if (event->type == LS_USEREVENT_QUIT)
	{
		setTLSSys(NULL);
		SDL_Quit();
		return true;
	}
	else
	{
		if (sys && sys->getInputThread() && sys->getInputThread()->handleEvent(event))
			return false;
		switch (event->type)
		{
			case SDL_WINDOWEVENT:
			{
				switch (event->window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
					case SDL_WINDOWEVENT_SIZE_CHANGED:
						if (sys && sys->getRenderThread())
							sys->getRenderThread()->requestResize(event->window.data1,event->window.data2,false);
						break;
					case SDL_WINDOWEVENT_EXPOSED:
					{
						//Signal the renderThread
						if (sys && sys->getRenderThread())
							sys->getRenderThread()->draw(false);
						break;
					}
						
					default:
						break;
				}
				break;
			}
			case SDL_QUIT:
				sys->setShutdownFlag();
				break;
		}
	}
	return false;
}

/* main loop handling */
static void mainloop_runner()
{
	bool sdl_available = false;
	if (SDL_WasInit(0)) // some part of SDL already was initialized
		sdl_available = SDL_InitSubSystem ( SDL_INIT_VIDEO );
	else
		sdl_available = SDL_Init ( SDL_INIT_VIDEO );
	if (sdl_available)
	{
		LOG(LOG_ERROR,"Unable to initialize SDL:"<<SDL_GetError());
		EngineData::mainthread_initialized.signal();
		return;
	}
	else
	{
		EngineData::mainthread_running = true;
		EngineData::mainthread_initialized.signal();
		SDL_Event event;
		while (SDL_WaitEvent(&event))
		{
			SystemState* sys = getSys();
			
			if (EngineData::mainloop_handleevent(&event,sys))
			{
				EngineData::mainthread_running = false;
				return;
			}
		}
	}
}
gboolean EngineData::mainloop_from_plugin(SystemState* sys)
{
	SDL_Event event;
	setTLSSys(sys);
	while (SDL_PollEvent(&event))
	{
		mainloop_handleevent(&event,sys);
	}
	setTLSSys(NULL);
	return G_SOURCE_CONTINUE;
}

/* This is not run in the linux plugin, as firefox
 * runs its own gtk_main, which we must not interfere with.
 */
bool EngineData::startSDLMain()
{
	assert(!mainLoopThread);
#ifdef HAVE_NEW_GLIBMM_THREAD_API
	mainLoopThread = Thread::create(sigc::ptr_fun(&mainloop_runner));
#else
	mainLoopThread = Thread::create(sigc::ptr_fun(&mainloop_runner),true);
#endif
	mainthread_initialized.wait();
	return mainthread_running;
}

void EngineData::showWindow(uint32_t w, uint32_t h)
{
	RecMutex::Lock l(mutex);

	assert(!widget);
	widget = createWidget(w,h);
	this->width = w;
	this->height = h;
	SDL_ShowWindow(widget);
	grabFocus();
	
}

void EngineData::showMouseCursor(SystemState* /*sys*/)
{
	SDL_ShowCursor(SDL_ENABLE);
}

void EngineData::hideMouseCursor(SystemState* /*sys*/)
{
	SDL_ShowCursor(SDL_DISABLE);
}

void EngineData::setClipboardText(const std::string txt)
{
	int ret = SDL_SetClipboardText(txt.c_str());
	if (ret == 0)
		LOG(LOG_INFO, "Copied error to clipboard");
	else
		LOG(LOG_ERROR, "copying text to clipboard failed:"<<SDL_GetError());
}

