/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef BACKENDS_INPUT_H
#define BACKENDS_INPUT_H 1

#include "compat.h"
#include "backends/geometry.h"
#include "events.h"
#include "threading.h"
#include "platforms/engineutils.h"
#include "swftypes.h"
#include "smartrefs.h"
#include "forwards/scripting/flash/events/flashevents.h"
#include "scripting/flash/ui/keycodes.h"
#include "forwards/scripting/flash/events/flashevents.h"
#include <vector>
#include <deque>
#include <set>

#include <SDL2/SDL_keyboard.h>

namespace lightspark
{

class SystemState;
class InteractiveObject;
class Sprite;
class MouseEvent;

class InputThread
{
friend class Stage;
private:
	SystemState* m_sys;
	EngineData* engineData;
	SDL_Thread* t;
	enum STATUS { CREATED=0, STARTED, TERMINATED };
	volatile STATUS status;
	bool terminated;
	// this is called from mainloopthread
	//bool worker(SDL_Event *event);
	static int worker(void* d);

	std::deque<LSEventStorage> inputEventQueue;
	std::vector<InteractiveObject* > listeners;
	Mutex mutexListeners;
	Mutex mutexDragged;
	Mutex mutexQueue;
	Cond eventCond;

	_NR<Sprite> curDragged;
	_NR<InteractiveObject> currentMouseOver;
	_NR<InteractiveObject> lastMouseDownTarget;
	_NR<InteractiveObject> lastMouseUpTarget;
	_NR<InteractiveObject> lastRolledOver;
	LSModifier lastKeymod;
	std::set<AS3KeyCode> keyDownSet;
	AS3KeyCode lastKeyDown;
	AS3KeyCode lastKeyUp;
	const RECT* dragLimit;
	Vector2f dragOffset;
	class MaskData
	{
	public:
		DisplayObject* d;
		MATRIX m;
		MaskData(DisplayObject* _d, const MATRIX& _m):d(_d),m(_m){}
	};
	_NR<InteractiveObject> getMouseTarget(const Vector2f& point, HIT_TYPE type);
	_NR<InteractiveObject> getMouseTarget(uint32_t x, uint32_t y, HIT_TYPE type);
	void handleMouseDown(const LSMouseButtonEvent& event);
	void handleMouseDoubleClick(const LSMouseButtonEvent& event);
	void handleMouseUp(const LSMouseButtonEvent& event);
	void handleMouseMove(const LSMouseMoveEvent& event);
	void handleScrollEvent(const LSMouseWheelEvent& event);
	void handleMouseLeave();

	bool handleKeyboardShortcuts(const LSKeyEvent& event);
	void sendKeyEvent(const LSKeyEvent& event, bool pressed);

	int handleEvent(const LSEvent& event);
	bool handleContextMenuEvent(const LSEvent& event);
	Mutex inputDataSpinlock;
	Vector2 mousePos;
	Vector2 mousePosStart;
	bool button1pressed;
public:
	InputThread(SystemState* s);
	~InputThread();
	void wait();
	void start(EngineData* data);
	void startDrag(_R<Sprite> s, const RECT* limit, Vector2f dragOffset);
	void stopDrag(Sprite* s);

	Vector2 getMousePos()
	{
		Locker locker(inputDataSpinlock);
		return mousePos;
	}
	bool getLeftButtonPressed()
	{
		Locker locker(mutexListeners);
		if (button1pressed)
		{
			button1pressed=false;
			return true;
		}
		return false;
	}
	
	bool isCreated() const { return status == CREATED; }
	bool isStarted() const { return status == STARTED; }
	bool isTerminated() const { return status == TERMINATED; }
	bool queueEvent(const LSEvent& event);

	AS3KeyCode getLastKeyDown();
	AS3KeyCode getLastKeyUp();
	AS3KeyCode getLastKeyCode();
	LSModifier getLastKeyMod();
	bool isKeyDown(AS3KeyCode key);
	void setLastKeyDown(KeyboardEvent* e);
	void setLastKeyUp(KeyboardEvent* e);
};

InputThread* getInputThread();
AS3KeyCode getAS3KeyCode(SDL_Keycode sdlkey);
AS3KeyCode getAS3KeyCodeFromScanCode(SDL_Scancode sdlscancode);

}
#endif /* BACKENDS_INPUT_H */
