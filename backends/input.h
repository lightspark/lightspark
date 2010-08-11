/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef INPUT_H
#define INPUT_H

#include "compat.h"
#include "threading.h"
#include "platforms/pluginutils.h"
#include "swftypes.h"
#include <vector>

namespace lightspark
{

class SystemState;
class InteractiveObject;
class Sprite;

class InputThread
{
private:
	SystemState* m_sys;
	pthread_t t;
	bool terminated;
	bool threaded;
	static void* sdl_worker(InputThread*);
#ifdef COMPILE_PLUGIN
	NPAPI_params* npapi_params;
	static gboolean gtkplug_worker(GtkWidget *widget, GdkEvent *event, InputThread* th);
	static void delayedCreation(InputThread* th);
#endif

	std::vector<InteractiveObject* > listeners;
	Mutex mutexListeners;
	Mutex mutexDragged;

	Sprite* curDragged;
	InteractiveObject* lastMouseDownTarget;
	RECT dragLimit;
public:
	InputThread(SystemState* s,ENGINE e, void* param=NULL);
	~InputThread();
	void wait();
	void addListener(InteractiveObject* ob);
	void removeListener(InteractiveObject* ob);
	void enableDrag(Sprite* s, const RECT& limit);
	void disableDrag();
};

};
#endif
