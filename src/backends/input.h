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

#ifndef INPUT_H
#define INPUT_H

#include "compat.h"
#include "geometry.h"
#include "threading.h"
#include "platforms/pluginutils.h"
#include "swftypes.h"
#include "smartrefs.h"
#include <vector>

namespace lightspark
{

class SystemState;
class DisplayObject;
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

	_NR<Sprite> curDragged;
	_NR<InteractiveObject> lastMouseDownTarget;
	const RECT* dragLimit;
	Vector2f dragOffset;
	class MaskData
	{
	public:
		DisplayObject* d;
		MATRIX m;
		MaskData(DisplayObject* _d, const MATRIX& _m):d(_d),m(_m){}
	};
	std::vector<MaskData> maskStack;
	void handleMouseDown(uint32_t x, uint32_t y);
	void handleMouseUp(uint32_t x, uint32_t y);
	void handleMouseMove(uint32_t x, uint32_t y);

	Spinlock inputDataSpinlock;
	Vector2 mousePos;
public:
	InputThread(SystemState* s);
	~InputThread();
	void wait();
	void start(ENGINE e, void* param);
	void addListener(InteractiveObject* ob);
	void removeListener(InteractiveObject* ob);
	void startDrag(_R<Sprite> s, const RECT* limit, Vector2f dragOffset);
	void stopDrag(Sprite* s);
	/**
	  	Add a mask to the stack mask
		@param d The DisplayObject used as a mask
		@param m The total matrix from object to stage
		\pre A reference is not acquired, we assume the object life is protected until the corresponding pop
	*/
	void pushMask(DisplayObject* d, const MATRIX& m)
	{
		maskStack.emplace_back(d,m);
	}
	/**
	  	Remove the last pushed mask
	*/
	void popMask()
	{
		maskStack.pop_back();
	}
	bool isMaskPresent()
	{
		return !maskStack.empty();
	}
	/*
	   	Checks if the given point in Stage coordinates is currently masked or not
	*/
	bool isMasked(number_t x, number_t y) const;

	Vector2 getMousePos()
	{
		SpinlockLocker locker(inputDataSpinlock);
		return mousePos;
	}
};

};
#endif
