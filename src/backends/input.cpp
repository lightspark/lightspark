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

#include "scripting/abc.h"
#include "backends/audio.h"
#include "backends/input.h"
#include "backends/rendering.h"
#include "compat.h"
#include "scripting/class.h"
#include <algorithm>

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_clipboard.h>

using namespace lightspark;
using namespace std;

InputThread::InputThread(SystemState* s):m_sys(s),engineData(nullptr),terminated(false),
	curDragged(),currentMouseOver(),lastMouseDownTarget(),
	lastKeyUp(SDLK_UNKNOWN), dragLimit(nullptr),button1pressed(false)
{
	LOG(LOG_INFO,_("Creating input thread"));
}

void InputThread::start(EngineData* e)
{
	engineData = e;
}

InputThread::~InputThread()
{
	wait();
}

void InputThread::wait()
{
	if(terminated)
		return;
	terminated=true;
}

bool InputThread::worker(SDL_Event *event)
{
	bool ret=false;
	if (m_sys && m_sys->getEngineData() && m_sys->getEngineData()->inContextMenu())
	{
		ret = handleContextMenuEvent(event);
		if (ret)
			return ret;
	}
	switch(event->type)
	{
		case SDL_KEYDOWN:
		{
			bool handled = handleKeyboardShortcuts(&event->key);
			if (!handled)
				sendKeyEvent(&event->key);
			ret=true;
			break;
		}
		case SDL_KEYUP:
		{
			sendKeyEvent(&event->key);
			ret=true;
			break;
		}
		case SDL_TEXTINPUT:
		{
			if(m_sys->currentVm == nullptr)
				break;
			_NR<InteractiveObject> target = m_sys->stage->getFocusTarget();
			if (target.isNull())
				break;
			// SDL_TEXINPUT sometimes seems to send an empty text, we ignore those events
			tiny_string s = std::string(event->text.text);
			if (s.numChars()> 0)
			{
				target->incRef();
				m_sys->currentVm->addIdleEvent(NullRef, _MR(new (m_sys->unaccountedMemory) TextInputEvent(target,s)));
			}
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			{
				Locker locker(mutexListeners);
				button1pressed=false;
			}
			if(event->button.button == SDL_BUTTON_LEFT)
			{
				//Grab focus, to receive keypresses
				engineData->grabFocus();

				int stageX, stageY;
				m_sys->windowToStageCoordinates(event->button.x,event->button.y,stageX,stageY);
				if (m_sys->mainClip->usesActionScript3)
				{
					if (event->button.clicks == 1)
						handleMouseDown(stageX,stageY,SDL_GetModState(),event->button.state == SDL_PRESSED);
					if (event->button.clicks == 2)
						handleMouseDoubleClick(stageX,stageY,SDL_GetModState(),event->button.state == SDL_PRESSED);
				}
				else
				{
					// AVM1  doesn't have double click events
					handleMouseDown(stageX,stageY,SDL_GetModState(),event->button.state == SDL_PRESSED);
				}
			}
			ret=true;
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			int stageX, stageY;
			m_sys->windowToStageCoordinates(event->button.x,event->button.y,stageX,stageY);
			handleMouseUp(stageX,stageY,SDL_GetModState(),event->button.state == SDL_PRESSED,event->button.button);
			ret=true;
			if (event->button.button == SDL_BUTTON_LEFT)
			{
				Locker locker(mutexListeners);
				button1pressed=true;
			}
			break;
		}
		case SDL_MOUSEMOTION:
		{
			int stageX, stageY;
			if (m_sys->getRenderThread()->inSettings)
			{
				stageX=event->motion.x;
				stageY=event->motion.y;
			}
			else
				m_sys->windowToStageCoordinates(event->motion.x,event->motion.y,stageX,stageY);
			handleMouseMove(stageX,stageY,SDL_GetModState(),event->motion.state == SDL_PRESSED);
			{
				Locker locker(mutexListeners);
				button1pressed=false;
			}
			ret=true;
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			int stageX, stageY;
			m_sys->windowToStageCoordinates(event->wheel.x,event->wheel.y,stageX,stageY);
#if SDL_VERSION_ATLEAST(2, 0, 4)
			handleScrollEvent(stageX,stageY,event->wheel.direction,SDL_GetModState(),false);
#else
			handleScrollEvent(stageX,stageY,1,SDL_GetModState(),false);
#endif
			ret=true;
			break;
		}
		case SDL_WINDOWEVENT_LEAVE:
		{
			handleMouseLeave();
			ret=true;
			break;
		}
		default:
			break;
	}
	return ret;
}
bool InputThread::handleContextMenuEvent(SDL_Event *event)
{
	bool ret = false;
	switch(event->type)
	{
		case SDL_KEYDOWN:
		{
			ret=true;
			break;
		}
		case SDL_KEYUP:
		{
			ret=true;
			break;
		}
		case SDL_TEXTINPUT:
		{
			ret=true;
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			ret=true;
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			if(event->button.button == SDL_BUTTON_LEFT)
			{
				m_sys->getEngineData()->updateContextMenuFromMouse(event->button.windowID, event->button.y);
				m_sys->getEngineData()->selectContextMenuItem();
			}
			ret=true;
			break;
		}
		case SDL_MOUSEMOTION:
		{
			m_sys->getEngineData()->updateContextMenuFromMouse(event->motion.windowID,event->motion.y);
			ret=true;
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			ret=true;
			break;
		}
		case SDL_WINDOWEVENT_LEAVE:
		{
			m_sys->getEngineData()->closeContextMenu();
			ret=true;
			break;
		}
		default:
			break;
	}
	return ret;
}

_NR<InteractiveObject> InputThread::getMouseTarget(uint32_t x, uint32_t y, DisplayObject::HIT_TYPE type)
{
	_NR<InteractiveObject> selected = NullRef;
	if (m_sys->getRenderThread()->inSettings)
		return selected;
	try
	{
		_NR<DisplayObject> dispobj=m_sys->stage->hitTest(NullRef,x,y, type,true);
		if(!dispobj.isNull() && dispobj->is<InteractiveObject>())
		{
			dispobj->incRef();
			selected=_MNR(dispobj->as<InteractiveObject>());
		}
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,_("Error in input handling ") << e.cause);
		m_sys->setError(e.cause);
		return NullRef;
	}
	assert(selected); /* atleast we hit the stage */
	assert_and_throw(selected->is<InteractiveObject>());
	return selected;
}

void InputThread::handleMouseDown(uint32_t x, uint32_t y, SDL_Keymod buttonState, bool pressed)
{
	if(m_sys->currentVm == nullptr)
		return;
	_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::MOUSE_CLICK);
	if (selected.isNull())
		return;
	number_t localX, localY;
	selected->globalToLocal(x,y,localX,localY);
	selected->incRef();
	m_sys->currentVm->addIdleEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS(m_sys,"mouseDown",localX,localY,true,buttonState,pressed)));
	Locker locker(mutexListeners);
	lastMouseDownTarget=selected;
}

void InputThread::handleMouseDoubleClick(uint32_t x, uint32_t y, SDL_Keymod buttonState, bool pressed)
{
	if(m_sys->currentVm == nullptr)
		return;
	_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::DOUBLE_CLICK);
	if (selected.isNull())
		return;
	number_t localX, localY;
	selected->globalToLocal(x,y,localX,localY);
	selected->incRef();
	m_sys->currentVm->addIdleEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS(m_sys,"doubleClick",localX,localY,true,buttonState,pressed)));
}

void InputThread::handleMouseUp(uint32_t x, uint32_t y, SDL_Keymod buttonState, bool pressed, uint8_t button)
{
	if(m_sys->currentVm == nullptr)
		return;
	_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::MOUSE_CLICK);
	if (selected.isNull())
		return;
	number_t localX, localY;
	selected->globalToLocal(x,y,localX,localY);
	selected->incRef();
	if (button == SDL_BUTTON_RIGHT)
	{
		m_sys->currentVm->addIdleEvent(selected,
			_MR(Class<MouseEvent>::getInstanceS(m_sys,"contextMenu",localX,localY,true,buttonState,pressed)));
		return;
	}
	m_sys->currentVm->addIdleEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS(m_sys,"mouseUp",localX,localY,true,buttonState,pressed)));
	mutexListeners.lock();
	if(lastMouseDownTarget==selected)
	{
		lastMouseDownTarget=NullRef;
		mutexListeners.unlock();
		//Also send the click event
		selected->incRef();
		m_sys->currentVm->addIdleEvent(selected,
			_MR(Class<MouseEvent>::getInstanceS(m_sys,"click",localX,localY,true,buttonState,pressed)));
	}
	else if (lastMouseDownTarget)
	{
		_NR<InteractiveObject> tmp = lastMouseDownTarget;
		lastMouseDownTarget=NullRef;
		mutexListeners.unlock();
		tmp->incRef();
		m_sys->currentVm->addIdleEvent(tmp,
			_MR(Class<MouseEvent>::getInstanceS(m_sys,"releaseOutside",localX,localY,true,buttonState,pressed)));
	}
	else
	{
		lastMouseDownTarget=NullRef;
		mutexListeners.unlock();
	}
}

void InputThread::handleMouseMove(uint32_t x, uint32_t y, SDL_Keymod buttonState, bool pressed)
{
	Locker locker(inputDataSpinlock);
	if(m_sys->currentVm == nullptr)
		return;
	mousePos.x=x;
	mousePos.y=y;
	if (m_sys->getRenderThread()->inSettings)
		return;
	mutexDragged.lock();
	// Handle current drag operation
	if(curDragged)
	{
		Vector2f local;
		DisplayObjectContainer* parent = curDragged->getParent();
		if(!parent)
		{
			Sprite* s = curDragged.getPtr();
			mutexDragged.unlock();
			stopDrag(s);
			return;
		}
		local = parent->getConcatenatedMatrix().getInverted().multiply2D(mousePos);
		local += dragOffset;
		if(dragLimit)
			local = local.projectInto(*dragLimit);

		curDragged->setX(local.x);
		curDragged->setY(local.y);
		mutexDragged.unlock();
	}
	// Handle non-drag mouse movement
	else
	{
		mutexDragged.unlock();
		_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::MOUSE_CLICK);
		number_t localX, localY;
		if(!currentMouseOver.isNull() && currentMouseOver != selected)
		{
			number_t clocalX, clocalY;
			currentMouseOver->globalToLocal(x,y,clocalX,clocalY);
			currentMouseOver->incRef();
			m_sys->currentVm->addIdleEvent(currentMouseOver,
				_MR(Class<MouseEvent>::getInstanceS(m_sys,"mouseOut",clocalX,clocalY,true,buttonState,pressed,selected)));
			currentMouseOver->incRef();
			m_sys->currentVm->addIdleEvent(currentMouseOver,
				_MR(Class<MouseEvent>::getInstanceS(m_sys,"rollOut",clocalX,clocalY,true,buttonState,pressed,selected)));
			currentMouseOver.reset();
		}
		if (selected.isNull())
			return;
		selected->globalToLocal(x,y,localX,localY);
		if(currentMouseOver == selected)
		{
			selected->incRef();
			m_sys->currentVm->addIdleEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS(m_sys,"mouseMove",localX,localY,true,buttonState,pressed)));
		}
		else
		{
			selected->incRef();
			m_sys->currentVm->addIdleEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS(m_sys,"mouseOver",localX,localY,true,buttonState,pressed,currentMouseOver)));
			selected->incRef();
			m_sys->currentVm->addIdleEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS(m_sys,"rollOver",localX,localY,true,buttonState,pressed,currentMouseOver)));
			currentMouseOver = selected;
		}
	}
}

void InputThread::handleScrollEvent(uint32_t x, uint32_t y, uint32_t direction, SDL_Keymod buttonState,bool pressed)
{
	if(m_sys->currentVm == NULL)
		return;

	int delta = 1;
#if SDL_VERSION_ATLEAST(2, 0, 4)
 	if(direction==SDL_MOUSEWHEEL_NORMAL)
 		delta = 1;
 	else if(direction==SDL_MOUSEWHEEL_FLIPPED)
 		delta = -1;
 	else
 		return;
#endif

	_NR<InteractiveObject> selected = getMouseTarget(x, y, DisplayObject::MOUSE_CLICK);
	if (selected.isNull())
		return;
	number_t localX, localY;
	selected->globalToLocal(x,y,localX,localY);
	selected->incRef();
	m_sys->currentVm->addIdleEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS(m_sys,"mouseWheel",localX,localY,true,buttonState,pressed,NullRef,delta)));
}

void InputThread::handleMouseLeave()
{
	if(m_sys->currentVm == nullptr)
		return;

	_NR<Stage> stage = _MR(m_sys->stage);
	stage->incRef();
	m_sys->currentVm->addIdleEvent(stage,
		_MR(Class<Event>::getInstanceS(m_sys,"mouseLeave")));
}

bool InputThread::handleKeyboardShortcuts(const SDL_KeyboardEvent *keyevent)
{
	if (m_sys->getEngineData()->inFullScreenMode() && keyevent->keysym.sym == SDLK_ESCAPE)
	{
		m_sys->getEngineData()->setDisplayState("normal",m_sys);
		return true;
	}
	if (keyevent->keysym.sym == SDLK_MENU)
	{
		int stageX,stageY;
		int x, y;
		SDL_GetMouseState(&x,&y);
		m_sys->windowToStageCoordinates(x,y,stageX,stageY);
		_NR<InteractiveObject> selected = getMouseTarget(stageX,stageY, DisplayObject::MOUSE_CLICK);
		if (!selected.isNull())
		{
			number_t localX, localY;
			selected->globalToLocal(x,y,localX,localY);
			selected->incRef();
			m_sys->currentVm->addIdleEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS(m_sys,"contextMenu",localX,localY,true,(SDL_Keymod)keyevent->keysym.mod,false)));
			return true;
		}
	}
	bool handled = false;
	if (!(keyevent->keysym.mod & KMOD_CTRL))
		return handled;

	switch(keyevent->keysym.sym)
	{
		case SDLK_q:
			handled = true;
			if(m_sys->standalone)
				m_sys->setShutdownFlag();
			break;
		case SDLK_f:
			handled = true;
			m_sys->getEngineData()->setDisplayState(m_sys->getEngineData()->inFullScreenMode() ? "normal" : "fullScreen",m_sys);
			break;
		case SDLK_p:
			handled = true;
			m_sys->showProfilingData=!m_sys->showProfilingData;
			break;
		case SDLK_m:
			handled = true;
			m_sys->audioManager->toggleMuteAll();
			if(m_sys->audioManager->allMuted())
				LOG(LOG_INFO, "All sounds muted");
			else
				LOG(LOG_INFO, "All sounds unmuted");
			break;
		case SDLK_c:
			handled = true;
			if(m_sys->hasError())
			{
				std::string s = "SWF file: ";
				s.append(m_sys->mainClip->getOrigin().getParsedURL());
				s.append("\n");
				s.append(m_sys->getErrorCause());
				engineData->setClipboardText(s);
			}
			else
				LOG(LOG_INFO, "No error to be copied to clipboard");
			break;
		case SDLK_d:
			m_sys->stage->dumpDisplayList();
			break;
		case SDLK_s:
			if (m_sys->getRenderThread())
			{
				handled = true;
				m_sys->getRenderThread()->screenshotneeded=true;
			}
			break;
#ifndef NDEBUG
		case SDLK_l:
			// switch between log levels LOG_CALLS and LOG_INFO
			if (Log::getLevel() == LOG_CALLS)
				Log::setLogLevel(LOG_INFO);
			else
				Log::setLogLevel(LOG_CALLS);
			break;
#endif
		default:
			break;
	}

	return handled;
}

AS3KeyCode getAS3KeyCode(SDL_Keycode sdlkey)
{
	switch (sdlkey)
	{
		case SDLK_RETURN: return AS3KEYCODE_ENTER;
		case SDLK_ESCAPE: return AS3KEYCODE_ESCAPE;
		case SDLK_BACKSPACE: return AS3KEYCODE_BACKSPACE;
		case SDLK_TAB: return AS3KEYCODE_TAB;
		case SDLK_SPACE: return AS3KEYCODE_SPACE;
		case SDLK_QUOTE: return AS3KEYCODE_QUOTE;
		case SDLK_COMMA: return AS3KEYCODE_COMMA;
		case SDLK_MINUS: return AS3KEYCODE_MINUS;
		case SDLK_PERIOD: return AS3KEYCODE_PERIOD;
		case SDLK_SLASH: return AS3KEYCODE_SLASH;
		case SDLK_0: return AS3KEYCODE_NUMBER_0;
		case SDLK_1: return AS3KEYCODE_NUMBER_1;
		case SDLK_2: return AS3KEYCODE_NUMBER_2;
		case SDLK_3: return AS3KEYCODE_NUMBER_3;
		case SDLK_4: return AS3KEYCODE_NUMBER_4;
		case SDLK_5: return AS3KEYCODE_NUMBER_5;
		case SDLK_6: return AS3KEYCODE_NUMBER_6;
		case SDLK_7: return AS3KEYCODE_NUMBER_7;
		case SDLK_8: return AS3KEYCODE_NUMBER_8;
		case SDLK_9: return AS3KEYCODE_NUMBER_9;
		case SDLK_COLON: return AS3KEYCODE_PERIOD;
		case SDLK_SEMICOLON: return AS3KEYCODE_SEMICOLON;
		case SDLK_EQUALS: return AS3KEYCODE_EQUAL;
		case SDLK_LEFTBRACKET: return AS3KEYCODE_LEFTBRACKET;
		case SDLK_BACKSLASH: return AS3KEYCODE_BACKSLASH;
		case SDLK_RIGHTBRACKET: return AS3KEYCODE_RIGHTBRACKET;
		case SDLK_BACKQUOTE: return AS3KEYCODE_BACKQUOTE;
		case SDLK_a: return AS3KEYCODE_A;
		case SDLK_b: return AS3KEYCODE_B;
		case SDLK_c: return AS3KEYCODE_C;
		case SDLK_d: return AS3KEYCODE_D;
		case SDLK_e: return AS3KEYCODE_E;
		case SDLK_f: return AS3KEYCODE_F;
		case SDLK_g: return AS3KEYCODE_G;
		case SDLK_h: return AS3KEYCODE_H;
		case SDLK_i: return AS3KEYCODE_I;
		case SDLK_j: return AS3KEYCODE_J;
		case SDLK_k: return AS3KEYCODE_K;
		case SDLK_l: return AS3KEYCODE_L;
		case SDLK_m: return AS3KEYCODE_M;
		case SDLK_n: return AS3KEYCODE_N;
		case SDLK_o: return AS3KEYCODE_O;
		case SDLK_p: return AS3KEYCODE_P;
		case SDLK_q: return AS3KEYCODE_Q;
		case SDLK_r: return AS3KEYCODE_R;
		case SDLK_s: return AS3KEYCODE_S;
		case SDLK_t: return AS3KEYCODE_T;
		case SDLK_u: return AS3KEYCODE_U;
		case SDLK_v: return AS3KEYCODE_V;
		case SDLK_w: return AS3KEYCODE_W;
		case SDLK_x: return AS3KEYCODE_X;
		case SDLK_y: return AS3KEYCODE_Y;
		case SDLK_z: return AS3KEYCODE_Z;
		case SDLK_CAPSLOCK:return AS3KEYCODE_CAPS_LOCK;
		case SDLK_F1: return AS3KEYCODE_F1;
		case SDLK_F2: return AS3KEYCODE_F2;
		case SDLK_F3: return AS3KEYCODE_F3;
		case SDLK_F4: return AS3KEYCODE_F4;
		case SDLK_F5: return AS3KEYCODE_F5;
		case SDLK_F6: return AS3KEYCODE_F6;
		case SDLK_F7: return AS3KEYCODE_F7;
		case SDLK_F8: return AS3KEYCODE_F8;
		case SDLK_F9: return AS3KEYCODE_F9;
		case SDLK_F10: return AS3KEYCODE_F10;
		case SDLK_F11: return AS3KEYCODE_F11;
		case SDLK_F12: return AS3KEYCODE_F12;
		case SDLK_PAUSE: return AS3KEYCODE_PAUSE;
		case SDLK_INSERT: return AS3KEYCODE_INSERT;
		case SDLK_HOME: return AS3KEYCODE_HOME;
		case SDLK_PAGEUP: return AS3KEYCODE_PAGE_UP;
		case SDLK_DELETE: return AS3KEYCODE_DELETE;
		case SDLK_END: return AS3KEYCODE_END;
		case SDLK_PAGEDOWN: return AS3KEYCODE_PAGE_DOWN;
		case SDLK_RIGHT: return AS3KEYCODE_RIGHT;
		case SDLK_LEFT: return AS3KEYCODE_LEFT;
		case SDLK_DOWN: return AS3KEYCODE_DOWN;
		case SDLK_UP: return AS3KEYCODE_UP;
		case SDLK_NUMLOCKCLEAR: return AS3KEYCODE_NUMPAD;
		case SDLK_KP_DIVIDE: return AS3KEYCODE_NUMPAD_DIVIDE;
		case SDLK_KP_MULTIPLY: return AS3KEYCODE_NUMPAD_MULTIPLY;
		case SDLK_KP_MINUS:return AS3KEYCODE_NUMPAD_SUBTRACT;
		case SDLK_KP_PLUS: return AS3KEYCODE_NUMPAD_ADD;
		case SDLK_KP_ENTER: return AS3KEYCODE_NUMPAD_ENTER;
		case SDLK_KP_1: return AS3KEYCODE_NUMPAD_1;
		case SDLK_KP_2: return AS3KEYCODE_NUMPAD_2;
		case SDLK_KP_3: return AS3KEYCODE_NUMPAD_3;
		case SDLK_KP_4: return AS3KEYCODE_NUMPAD_4;
		case SDLK_KP_5: return AS3KEYCODE_NUMPAD_5;
		case SDLK_KP_6: return AS3KEYCODE_NUMPAD_6;
		case SDLK_KP_7: return AS3KEYCODE_NUMPAD_7;
		case SDLK_KP_8: return AS3KEYCODE_NUMPAD_8;
		case SDLK_KP_9: return AS3KEYCODE_NUMPAD_9;
		case SDLK_KP_0: return AS3KEYCODE_NUMPAD_0;
		case SDLK_KP_PERIOD: return AS3KEYCODE_NUMPAD_DECIMAL;
		case SDLK_F13: return AS3KEYCODE_F13;
		case SDLK_F14: return AS3KEYCODE_F14;
		case SDLK_F15: return AS3KEYCODE_F15;
		case SDLK_HELP: return AS3KEYCODE_HELP;
		case SDLK_MENU: return AS3KEYCODE_MENU;
		case SDLK_LCTRL: return AS3KEYCODE_CONTROL;
		case SDLK_LSHIFT: return AS3KEYCODE_SHIFT;
		case SDLK_LALT: return AS3KEYCODE_ALTERNATE;
		case SDLK_RCTRL: return AS3KEYCODE_CONTROL;
		case SDLK_RSHIFT: return AS3KEYCODE_SHIFT;
		case SDLK_RALT: return AS3KEYCODE_ALTERNATE;
		case SDLK_AC_SEARCH: return AS3KEYCODE_SEARCH;
		case SDLK_AC_BACK: return AS3KEYCODE_BACK ;
		case SDLK_AC_STOP:return AS3KEYCODE_STOP;
	}
	//AS3KEYCODE_AUDIO
	//AS3KEYCODE_BLUE
	//AS3KEYCODE_YELLOW
	//AS3KEYCODE_CHANNEL_DOWN
	//AS3KEYCODE_CHANNEL_UP
	//AS3KEYCODE_COMMAND
	//AS3KEYCODE_DVR
	//AS3KEYCODE_EXIT
	//AS3KEYCODE_FAST_FORWARD
	//AS3KEYCODE_GREEN
	//AS3KEYCODE_GUIDE
	//AS3KEYCODE_INFO
	//AS3KEYCODE_INPUT
	//AS3KEYCODE_LAST
	//AS3KEYCODE_LIVE
	//AS3KEYCODE_MASTER_SHELL
	//AS3KEYCODE_NEXT
	//AS3KEYCODE_PLAY
	//AS3KEYCODE_PREVIOUS
	//AS3KEYCODE_RECORD
	//AS3KEYCODE_RED
	//AS3KEYCODE_REWIND
	//AS3KEYCODE_SETUP
	//AS3KEYCODE_SKIP_BACKWARD
	//AS3KEYCODE_SKIP_FORWARD
	//AS3KEYCODE_SUBTITLE
	//AS3KEYCODE_VOD
	return AS3KEYCODE_UNKNOWN;
}


void InputThread::sendKeyEvent(const SDL_KeyboardEvent *keyevent)
{
	if(m_sys->currentVm == NULL)
		return;

	_NR<DisplayObject> target = m_sys->stage->getFocusTarget();
	if (target.isNull())
		return;

	tiny_string type;
	if (keyevent->type == SDL_KEYDOWN)
		type = "keyDown";
	else
		type = "keyUp";

	target->incRef();
	m_sys->currentVm->addIdleEvent(target,
	    _MR(Class<KeyboardEvent>::getInstanceS(m_sys,type,keyevent->keysym.scancode,getAS3KeyCode(keyevent->keysym.sym), (SDL_Keymod)keyevent->keysym.mod,keyevent->keysym.sym)));
}


void InputThread::startDrag(_R<Sprite> s, const lightspark::RECT* limit, Vector2f offset)
{
	Locker locker(mutexDragged);
	if(s==curDragged)
		return;

	curDragged=s;
	curDragged->dragged=true;
	dragLimit=limit;
	dragOffset=offset;
}

void InputThread::stopDrag(Sprite* s)
{
	Locker locker(mutexDragged);
	if(curDragged == s)
	{
		curDragged->dragged=false;
		curDragged = NullRef;
		delete dragLimit;
		dragLimit = 0;
	}
}

AS3KeyCode InputThread::getLastKeyDown()
{
	Locker locker(mutexListeners);
	return getAS3KeyCode(lastKeyDown);
}

AS3KeyCode InputThread::getLastKeyUp()
{
	Locker locker(mutexListeners);
	return getAS3KeyCode(lastKeyUp);
}

SDL_Keycode InputThread::getLastKeyCode()
{
	Locker locker(mutexListeners);
	return lastKeyDown;
}

SDL_Keymod InputThread::getLastKeyMod()
{
	Locker locker(mutexListeners);
	return lastKeymod;
}

bool InputThread::isKeyDown(AS3KeyCode key)
{
	Locker locker(mutexListeners);
	return keyDownSet.count(key);
}

void InputThread::setLastKeyDown(KeyboardEvent *e)
{
	Locker locker(mutexListeners);
	lastKeymod = SDL_Keymod(e->getModifiers());
	lastKeyDown = e->getSDLKeyCode();
	keyDownSet.insert(getAS3KeyCode(e->getSDLKeyCode()));
	lastKeyUp = 0;
}

void InputThread::setLastKeyUp(KeyboardEvent *e)
{
	Locker locker(mutexListeners);
	lastKeymod = SDL_Keymod(e->getModifiers());
	lastKeyUp = e->getSDLKeyCode();
	keyDownSet.erase(getAS3KeyCode(e->getSDLKeyCode()));
	lastKeyDown = 0;
	
}
