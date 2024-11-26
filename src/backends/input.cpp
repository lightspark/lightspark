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
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/display/Stage.h"
#include "compat.h"
#include "scripting/class.h"
#include <algorithm>

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_clipboard.h>

using namespace lightspark;
using namespace std;

DEFINE_AND_INITIALIZE_TLS(inputThread);
InputThread* lightspark::getInputThread()
{
	InputThread* ret = (InputThread*)tls_get(inputThread);
	return ret;
}

InputThread::InputThread(SystemState* s):m_sys(s),engineData(nullptr),status(CREATED),
	curDragged(),currentMouseOver(),lastMouseDownTarget(),
	lastKeyUp(AS3KEYCODE_UNKNOWN), dragLimit(nullptr),button1pressed(false)
{
	LOG(LOG_INFO,"Creating input thread");
}

void InputThread::start(EngineData* e)
{
	status = STARTED;
	engineData = e;
	if (!m_sys->runSingleThreaded)
		t = SDL_CreateThread(InputThread::worker,"InputThread",this);
}

InputThread::~InputThread()
{
	wait();
	LOG(LOG_INFO, "~InputThread this=" << this);
}

void InputThread::wait()
{
	if(status==STARTED)
	{
		if (!m_sys->runSingleThreaded)
		{
			inputEventQueue.push_back(LSQuitEvent(LSQuitEvent::QuitType::System));
			eventCond.signal();
			SDL_WaitThread(t,nullptr);
		}
		else
			handleEvent(LSQuitEvent(LSQuitEvent::QuitType::System));
	}
}

int InputThread::worker(void* d)
{
	InputThread *th = (InputThread *)d;
	setTLSSys(th->m_sys);
	setTLSWorker(th->m_sys->worker);
	// Set TLS variable for `getInputThread()`.
	tls_set(inputThread, th);

	while (true)
	{
		const auto& event = [&]()
		{
			Locker l(th->mutexQueue);
			while (th->inputEventQueue.empty())
				th->eventCond.wait(th->mutexQueue);
			auto& ev = th->inputEventQueue.front();
			th->inputEventQueue.pop_front();
			return ev;
		}();
		if (th->handleEvent(event) < 0)
			break;
	}
	th->status = TERMINATED;
	return 0;
}

bool InputThread::queueEvent(const LSEvent& event)
{
	using FocusType = LSWindowFocusEvent::FocusType;
	using MouseType = LSMouseEvent::MouseType;

	bool queueable = event.visit(makeVisitor
	(
		[&](const LSWindowEvent&) { return false; },
		[&](const LSWindowFocusEvent& focus)
		{
			return focus.focusType != FocusType::Mouse || focus.focused;
		},
		[](const LSQuitEvent&) { return false; },
		[](const LSEvent&) { return true; }
	));
	if (!queueable)
		return false;
	else if (m_sys->runSingleThreaded)
	{
		handleEvent(event);
		return true;
	}
	else
	{
		Locker l(mutexQueue);
		if (!inputEventQueue.empty() && event.getType() == inputEventQueue.back().event().getType())
		{
			bool queue = event.visit(makeVisitor
			(
				[&](const LSMouseEvent& mouse)
				{
					if (mouse.mouseType != MouseType::Button)
					{
						inputEventQueue.back() = event;
						return false;
					}
					return true;
				},
				[&](const LSEvent&) { return true; }
			));
			if (!queue)
				return false;
		}
		inputEventQueue.push_back(event);
	}
	eventCond.signal();
	return true;
}

int InputThread::handleEvent(const LSEvent& event)
{
	using Button = LSMouseButtonEvent::Button;
	using ButtonType = LSMouseButtonEvent::ButtonType;
	using FocusType = LSWindowFocusEvent::FocusType;
	using KeyType = LSKeyEvent::KeyType;

	bool hasSys = m_sys != nullptr;
	bool hasEngineData = hasSys && m_sys->getEngineData() != nullptr;
	if (hasEngineData && m_sys->getEngineData()->inContextMenu() && handleContextMenuEvent(event))
		return true;
	if (event.getType() == LSEvent::Type::RemovedFromStage)
	{
		{
			Locker locker(inputDataSpinlock);
			if (!currentMouseOver.isNull() && !currentMouseOver->isOnStage())
				currentMouseOver.reset();
		}
		{
			Locker locker(mutexListeners);
			if (!lastMouseDownTarget.isNull() && !lastMouseDownTarget->isOnStage())
				lastMouseDownTarget.reset();
			if (!lastMouseUpTarget.isNull() && !lastMouseUpTarget->isOnStage())
				lastMouseUpTarget.reset();
		}
		{
			Locker locker(mutexDragged);
			if (!curDragged.isNull() && !curDragged->isOnStage())
				stopDrag(curDragged.getPtr());
		}
	}
	if (!hasSys || m_sys->isShuttingDown())
		return -1;

	EngineData* engineData = m_sys->getEngineData();
	return event.visit(makeVisitor
	(
		[&](const LSKeyEvent& key)
		{
			bool handled = false;
			if (key.keyType != KeyType::Up && (handled = handleKeyboardShortcuts(key), !handled))
				sendKeyEvent(key, true);
			if (key.keyType == KeyType::Up || (handled && key.keyType == KeyType::Press))
				sendKeyEvent(key, false);
			return true;
		},
		[&](const LSTextEvent& text)
		{
			if(m_sys->currentVm == nullptr)
				return false;
			_NR<InteractiveObject> target = m_sys->stage->getFocusTarget();
			if (target.isNull())
				return false;
			target->incRef();
			m_sys->currentVm->addIdleEvent(NullRef, _MR(new (m_sys->unaccountedMemory) TextInputEvent(target,text.text)));
			return false;
		},
		[&](const LSMouseButtonEvent& mouseButton)
		{
			if (mouseButton.buttonType != ButtonType::Up)
			{
				{
					Locker locker(mutexListeners);
					button1pressed=false;
				}
				mousePosStart = mouseButton.mousePos.round();
				if (mouseButton.button == Button::Left)
				{
					//Grab focus, to receive keypresses
					engineData->grabFocus();

					if (m_sys->mainClip->usesActionScript3)
					{
						handleMouseDown(mouseButton);
						if (mouseButton.buttonType == ButtonType::DoubleClick)
							handleMouseDoubleClick(mouseButton);
					}
					else
					{
						// AVM1 doesn't have double click events
						handleMouseDown(mouseButton);
					}
				}
				if (mouseButton.buttonType != ButtonType::Click)
					return true;
			}
			m_sys->setWindowMoveMode(false);
			handleMouseUp(mouseButton);
			if (mouseButton.button == Button::Left)
			{
				Locker locker(mutexListeners);
				button1pressed=true;
			}
			return true;
		},
		[&](const LSMouseMoveEvent& mouseMove)
		{
			if (m_sys->getInWindowMoveMode())
			{
				Vector2 size(engineData->width, engineData->height);
				Vector2 windowPos = engineData->getWindowPosition();
				Vector2 mousePos = mouseMove.mousePos;
				Vector2 newWindowPos = windowPos + mousePos - mousePosStart;
				engineData->setWindowPosition(newWindowPos, size);
			}
			else
			{
				handleMouseMove(mouseMove);
				Locker locker(mutexListeners);
				button1pressed=false;
			}
			return true;
		},
		[&](const LSMouseWheelEvent& mouseWheel)
		{
			handleScrollEvent(mouseWheel);
			return true;
		},
		[&](const LSWindowFocusEvent& focus)
		{
			if (focus.focusType == FocusType::Mouse && !focus.focused)
			{
				handleMouseLeave();
				return true;
			}
			return false;
		},
		[&](const LSEvent&) { return false; }
	));
}
bool InputThread::handleContextMenuEvent(const LSEvent& event)
{
	using Button = LSMouseButtonEvent::Button;
	using ButtonType = LSMouseButtonEvent::ButtonType;
	using FocusType = LSWindowFocusEvent::FocusType;

	return event.visit(makeVisitor
	(
		[&](const LSKeyEvent& key) { return true; },
		[&](const LSTextEvent& text) { return true; },
		[&](const LSMouseButtonEvent& mouseButton)
		{
			if (mouseButton.buttonType == ButtonType::Up && mouseButton.button == Button::Left)
			{
				m_sys->getEngineData()->updateContextMenuFromMouse(mouseButton.windowID, mouseButton.mousePos.y);
				m_sys->getEngineData()->selectContextMenuItem();
			}
			return true;
		},
		[&](const LSMouseMoveEvent& mouseMove)
		{
			m_sys->getEngineData()->updateContextMenuFromMouse(mouseMove.windowID, mouseMove.mousePos.y);
			return true;
		},
		[&](const LSMouseWheelEvent& mouseWheel) { return true; },
		[&](const LSWindowFocusEvent& focus)
		{
			if (focus.focusType == FocusType::Mouse && !focus.focused)
			{
				m_sys->getEngineData()->closeContextMenu();
				return true;
			}
			return false;
		},
		[&](const LSEvent&) { return false; }
	));
}

_NR<InteractiveObject> InputThread::getMouseTarget(const Vector2f& point, HIT_TYPE type)
{
	return getMouseTarget(point.x, point.y, type);
}

// TODO: Get rid of this version, and use the `Vector2` version only.
_NR<InteractiveObject> InputThread::getMouseTarget(uint32_t x, uint32_t y, HIT_TYPE type)
{
	_NR<InteractiveObject> selected = NullRef;
	if (m_sys->getRenderThread()->inSettings)
		return selected;
	try
	{
		// get mouse target in VM thread to avoid inconsistencies in hittesting on TokenContainers
		_R<GetMouseTargetEvent> ev  = _MR(new (m_sys->unaccountedMemory) GetMouseTargetEvent(x,y,type));
		if (m_sys->currentVm->prependEvent(NullRef, ev))
			ev->wait();
		if(!ev->dispobj.isNull() && ev->dispobj->is<InteractiveObject>())
		{
			ev->dispobj->incRef();
			selected=_MNR(ev->dispobj->as<InteractiveObject>());
		}
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Error in input handling " << e.cause);
		m_sys->setError(e.cause);
		return NullRef;
	}
	assert_and_throw(selected.isNull() || selected->is<InteractiveObject>());
	return selected;
}

void InputThread::handleMouseDown(const LSMouseButtonEvent& event)
{
	if(m_sys->currentVm == nullptr)
		return;
	_NR<InteractiveObject> selected = getMouseTarget(event.stagePos, MOUSE_CLICK_HIT);
	if (selected.isNull())
		return;
	Vector2f local = selected->globalToLocal(event.stagePos);
	m_sys->currentVm->addIdleEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"mouseDown",local.x,local.y,true,event.modifiers,event.pressed)));
	Locker locker(mutexListeners);
	lastMouseDownTarget=selected;
}

void InputThread::handleMouseDoubleClick(const LSMouseButtonEvent& event)
{
	if(m_sys->currentVm == nullptr)
		return;
	_NR<InteractiveObject> selected = getMouseTarget(event.stagePos, DOUBLE_CLICK_HIT);
	if (selected.isNull())
		return;
	if (!selected->isHittable(DOUBLE_CLICK_HIT))
	{
		// no double click hit found, add additional down-up-click sequence
		if (!lastMouseUpTarget.isNull())
		{
			// add mousedown event for last mouseUp target
			Vector2f local = lastMouseUpTarget->globalToLocal(event.stagePos);
			m_sys->currentVm->addIdleEvent(lastMouseUpTarget,
										   _MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"mouseDown",local.x,local.y,true,event.modifiers,event.pressed)));
			// reset lastMouseDownTarget to lastMouseUpTarget to ensure a normal "click" event is sended
			Locker locker(mutexListeners);
			lastMouseDownTarget = lastMouseUpTarget;
		}
		return;
	}
	Vector2f local = selected->globalToLocal(event.stagePos);
	m_sys->currentVm->addIdleEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"doubleClick",local.x,local.y,true,event.modifiers,event.pressed)));
}

void InputThread::handleMouseUp(const LSMouseButtonEvent& event)
{
	using Button = LSMouseButtonEvent::Button;

	if(m_sys->currentVm == nullptr)
		return;
	_NR<InteractiveObject> selected = getMouseTarget(event.stagePos, MOUSE_CLICK_HIT);
	if (selected.isNull())
		return;
	Vector2f local = selected->globalToLocal(event.stagePos);
	if (event.button == Button::Right)
	{
		m_sys->currentVm->addIdleEvent(selected,
			_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"contextMenu",local.x,local.y,true,event.modifiers,event.pressed)));
		return;
	}
	m_sys->currentVm->addIdleEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"mouseUp",local.x,local.y,true,event.modifiers,event.pressed)));
	mutexListeners.lock();
	lastMouseUpTarget=selected;
	if(lastMouseDownTarget==selected)
	{
		lastMouseDownTarget=NullRef;
		mutexListeners.unlock();
		//Also send the click event
		m_sys->currentVm->addIdleEvent(selected,
			_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"click",local.x,local.y,true,event.modifiers,event.pressed)));
	}
	else if (lastMouseDownTarget)
	{
		_NR<InteractiveObject> tmp = lastMouseDownTarget;
		lastMouseDownTarget=NullRef;
		mutexListeners.unlock();
		m_sys->currentVm->addIdleEvent(tmp,
			_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"releaseOutside",local.x,local.y,true,event.modifiers,event.pressed)));
	}
	else
	{
		lastMouseDownTarget=NullRef;
		mutexListeners.unlock();
	}
}
void InputThread::handleMouseMove(const LSMouseMoveEvent& event)
{
	if(m_sys->currentVm == nullptr)
		return;
	if (m_sys->getRenderThread()->inSettings)
	{
		mousePos = event.mousePos;
		return;
	}
	mousePos = event.stagePos;
	_NR<InteractiveObject> selected = getMouseTarget(event.stagePos, MOUSE_CLICK_HIT);
	mutexDragged.lock();
	if(curDragged)
	{
		Vector2f local;
		DisplayObjectContainer* parent = curDragged->getParent();
		if (parent == nullptr)
		{
			Sprite* s = curDragged.getPtr();
			mutexDragged.unlock();
			stopDrag(s);
			return;
		}
		local = parent->getConcatenatedMatrix().getInverted().multiply2D(event.stagePos);
		local += dragOffset;
		if(dragLimit)
			local = local.projectInto(*dragLimit);

		curDragged->setX(local.x);
		curDragged->setY(local.y);
	}
	mutexDragged.unlock();
	if(!currentMouseOver.isNull() && currentMouseOver != selected)
	{
		Vector2f local = currentMouseOver->globalToLocal(event.stagePos);
		m_sys->currentVm->addIdleEvent(currentMouseOver,
			_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"mouseOut",local.x,local.y,true,event.modifiers,event.pressed,selected)));
		if (selected.isNull())
			m_sys->currentVm->addIdleEvent(currentMouseOver,_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"rollOut",local.x,local.y,true,event.modifiers,event.pressed,selected)));
		currentMouseOver.reset();
	}
	if (selected.isNull())
		return;
	Vector2f local = selected->globalToLocal(event.stagePos);
	if(currentMouseOver == selected)
	{
		m_sys->currentVm->addIdleEvent(selected,
			_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"mouseMove",local.x,local.y,true,event.modifiers,event.pressed)),true);
	}
	else
	{
		m_sys->currentVm->addIdleEvent(selected,
			_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"mouseOver",local.x,local.y,true,event.modifiers,event.pressed,currentMouseOver)),true);
		currentMouseOver = selected;
	}
	if (selected != lastRolledOver)
	{
		if (lastRolledOver)
			m_sys->currentVm->addIdleEvent(lastRolledOver,_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"rollOut",local.x,local.y,true,event.modifiers,event.pressed,selected)));
		m_sys->currentVm->addIdleEvent(selected,
			_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"rollOver",local.x,local.y,true,event.modifiers,event.pressed,lastRolledOver)));
		lastRolledOver = selected;
	}
}

void InputThread::handleScrollEvent(const LSMouseWheelEvent& event)
{
	if(m_sys->currentVm == nullptr)
		return;

	_NR<InteractiveObject> selected = getMouseTarget(event.stagePos, MOUSE_CLICK_HIT);
	if (selected.isNull())
		return;
	Vector2f local = selected->globalToLocal(event.stagePos);
	m_sys->currentVm->addIdleEvent(selected,
		_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"mouseWheel",local.x,local.y,true,event.modifiers,event.pressed,NullRef,event.delta)));
}

void InputThread::handleMouseLeave()
{
	if(m_sys->currentVm == nullptr)
		return;
	if(!currentMouseOver.isNull())
	{
		m_sys->currentVm->addIdleEvent(currentMouseOver,
									   _MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"mouseOut",-1,-1)));
	}
	_NR<Stage> stage = _MR(m_sys->stage);
	m_sys->currentVm->addIdleEvent(stage,
		_MR(Class<Event>::getInstanceS(m_sys->worker,"mouseLeave")));
}

bool InputThread::handleKeyboardShortcuts(const LSKeyEvent& event)
{
	if (event.keyCode == AS3KEYCODE_MENU)
	{
		_NR<InteractiveObject> selected = getMouseTarget(event.stagePos, MOUSE_CLICK_HIT);
		if (!selected.isNull())
		{
			Vector2f local = selected->globalToLocal(event.stagePos);
			m_sys->currentVm->addIdleEvent(selected,
				_MR(Class<MouseEvent>::getInstanceS(m_sys->worker,"contextMenu",local.x,local.y,true,event.modifiers,false)));
			return true;
		}
	}
	bool handled = false;
	if (!(event.modifiers & LSModifier::Ctrl))
		return handled;

	switch(event.keyCode)
	{
		case AS3KEYCODE_Q:
			handled = true;
			if(m_sys->standalone)
				m_sys->setShutdownFlag();
			break;
		case AS3KEYCODE_F:
			handled = true;
			m_sys->getEngineData()->setDisplayState(m_sys->getEngineData()->inFullScreenMode() ? "normal" : "fullScreen",m_sys);
			break;
		case AS3KEYCODE_P:
			handled = true;
			m_sys->showProfilingData=!m_sys->showProfilingData;
			break;
		case AS3KEYCODE_M:
			handled = true;
			m_sys->audioManager->toggleMuteAll();
			if(m_sys->audioManager->allMuted())
				LOG(LOG_INFO, "All sounds muted");
			else
				LOG(LOG_INFO, "All sounds unmuted");
			break;
		case AS3KEYCODE_C:
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
		case AS3KEYCODE_D:
			m_sys->stage->dumpDisplayList();
			break;
		case AS3KEYCODE_S:
			if (m_sys->getRenderThread())
			{
				handled = true;
				m_sys->getRenderThread()->screenshotneeded=true;
			}
			break;
#ifndef NDEBUG
		case AS3KEYCODE_L:
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

AS3KeyCode lightspark::getAS3KeyCode(SDL_Keycode sdlkey)
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
AS3KeyCode lightspark::getAS3KeyCodeFromScanCode(SDL_Scancode sdlscancode)
{
	switch (sdlscancode)
	{
		case SDL_SCANCODE_RETURN: return AS3KEYCODE_ENTER;
		case SDL_SCANCODE_ESCAPE: return AS3KEYCODE_ESCAPE;
		case SDL_SCANCODE_BACKSPACE: return AS3KEYCODE_BACKSPACE;
		case SDL_SCANCODE_TAB: return AS3KEYCODE_TAB;
		case SDL_SCANCODE_SPACE: return AS3KEYCODE_SPACE;
		case SDL_SCANCODE_COMMA: return AS3KEYCODE_COMMA;
		case SDL_SCANCODE_MINUS: return AS3KEYCODE_MINUS;
		case SDL_SCANCODE_PERIOD: return AS3KEYCODE_PERIOD;
		case SDL_SCANCODE_SLASH: return AS3KEYCODE_SLASH;
		case SDL_SCANCODE_0: return AS3KEYCODE_NUMBER_0;
		case SDL_SCANCODE_1: return AS3KEYCODE_NUMBER_1;
		case SDL_SCANCODE_2: return AS3KEYCODE_NUMBER_2;
		case SDL_SCANCODE_3: return AS3KEYCODE_NUMBER_3;
		case SDL_SCANCODE_4: return AS3KEYCODE_NUMBER_4;
		case SDL_SCANCODE_5: return AS3KEYCODE_NUMBER_5;
		case SDL_SCANCODE_6: return AS3KEYCODE_NUMBER_6;
		case SDL_SCANCODE_7: return AS3KEYCODE_NUMBER_7;
		case SDL_SCANCODE_8: return AS3KEYCODE_NUMBER_8;
		case SDL_SCANCODE_9: return AS3KEYCODE_NUMBER_9;
		case SDL_SCANCODE_SEMICOLON: return AS3KEYCODE_SEMICOLON;
		case SDL_SCANCODE_EQUALS: return AS3KEYCODE_EQUAL;
		case SDL_SCANCODE_LEFTBRACKET: return AS3KEYCODE_LEFTBRACKET;
		case SDL_SCANCODE_BACKSLASH: return AS3KEYCODE_BACKSLASH;
		case SDL_SCANCODE_RIGHTBRACKET: return AS3KEYCODE_RIGHTBRACKET;
		case SDL_SCANCODE_A: return AS3KEYCODE_A;
		case SDL_SCANCODE_B: return AS3KEYCODE_B;
		case SDL_SCANCODE_C: return AS3KEYCODE_C;
		case SDL_SCANCODE_D: return AS3KEYCODE_D;
		case SDL_SCANCODE_E: return AS3KEYCODE_E;
		case SDL_SCANCODE_F: return AS3KEYCODE_F;
		case SDL_SCANCODE_G: return AS3KEYCODE_G;
		case SDL_SCANCODE_H: return AS3KEYCODE_H;
		case SDL_SCANCODE_I: return AS3KEYCODE_I;
		case SDL_SCANCODE_J: return AS3KEYCODE_J;
		case SDL_SCANCODE_K: return AS3KEYCODE_K;
		case SDL_SCANCODE_L: return AS3KEYCODE_L;
		case SDL_SCANCODE_M: return AS3KEYCODE_M;
		case SDL_SCANCODE_N: return AS3KEYCODE_N;
		case SDL_SCANCODE_O: return AS3KEYCODE_O;
		case SDL_SCANCODE_P: return AS3KEYCODE_P;
		case SDL_SCANCODE_Q: return AS3KEYCODE_Q;
		case SDL_SCANCODE_R: return AS3KEYCODE_R;
		case SDL_SCANCODE_S: return AS3KEYCODE_S;
		case SDL_SCANCODE_T: return AS3KEYCODE_T;
		case SDL_SCANCODE_U: return AS3KEYCODE_U;
		case SDL_SCANCODE_V: return AS3KEYCODE_V;
		case SDL_SCANCODE_W: return AS3KEYCODE_W;
		case SDL_SCANCODE_X: return AS3KEYCODE_X;
		case SDL_SCANCODE_Y: return AS3KEYCODE_Y;
		case SDL_SCANCODE_Z: return AS3KEYCODE_Z;
		case SDL_SCANCODE_CAPSLOCK:return AS3KEYCODE_CAPS_LOCK;
		case SDL_SCANCODE_F1: return AS3KEYCODE_F1;
		case SDL_SCANCODE_F2: return AS3KEYCODE_F2;
		case SDL_SCANCODE_F3: return AS3KEYCODE_F3;
		case SDL_SCANCODE_F4: return AS3KEYCODE_F4;
		case SDL_SCANCODE_F5: return AS3KEYCODE_F5;
		case SDL_SCANCODE_F6: return AS3KEYCODE_F6;
		case SDL_SCANCODE_F7: return AS3KEYCODE_F7;
		case SDL_SCANCODE_F8: return AS3KEYCODE_F8;
		case SDL_SCANCODE_F9: return AS3KEYCODE_F9;
		case SDL_SCANCODE_F10: return AS3KEYCODE_F10;
		case SDL_SCANCODE_F11: return AS3KEYCODE_F11;
		case SDL_SCANCODE_F12: return AS3KEYCODE_F12;
		case SDL_SCANCODE_PAUSE: return AS3KEYCODE_PAUSE;
		case SDL_SCANCODE_INSERT: return AS3KEYCODE_INSERT;
		case SDL_SCANCODE_HOME: return AS3KEYCODE_HOME;
		case SDL_SCANCODE_PAGEUP: return AS3KEYCODE_PAGE_UP;
		case SDL_SCANCODE_DELETE: return AS3KEYCODE_DELETE;
		case SDL_SCANCODE_END: return AS3KEYCODE_END;
		case SDL_SCANCODE_PAGEDOWN: return AS3KEYCODE_PAGE_DOWN;
		case SDL_SCANCODE_RIGHT: return AS3KEYCODE_RIGHT;
		case SDL_SCANCODE_LEFT: return AS3KEYCODE_LEFT;
		case SDL_SCANCODE_DOWN: return AS3KEYCODE_DOWN;
		case SDL_SCANCODE_UP: return AS3KEYCODE_UP;
		case SDL_SCANCODE_NUMLOCKCLEAR: return AS3KEYCODE_NUMPAD;
		case SDL_SCANCODE_KP_DIVIDE: return AS3KEYCODE_NUMPAD_DIVIDE;
		case SDL_SCANCODE_KP_MULTIPLY: return AS3KEYCODE_NUMPAD_MULTIPLY;
		case SDL_SCANCODE_KP_MINUS:return AS3KEYCODE_NUMPAD_SUBTRACT;
		case SDL_SCANCODE_KP_PLUS: return AS3KEYCODE_NUMPAD_ADD;
		case SDL_SCANCODE_KP_ENTER: return AS3KEYCODE_NUMPAD_ENTER;
		case SDL_SCANCODE_KP_1: return AS3KEYCODE_NUMPAD_1;
		case SDL_SCANCODE_KP_2: return AS3KEYCODE_NUMPAD_2;
		case SDL_SCANCODE_KP_3: return AS3KEYCODE_NUMPAD_3;
		case SDL_SCANCODE_KP_4: return AS3KEYCODE_NUMPAD_4;
		case SDL_SCANCODE_KP_5: return AS3KEYCODE_NUMPAD_5;
		case SDL_SCANCODE_KP_6: return AS3KEYCODE_NUMPAD_6;
		case SDL_SCANCODE_KP_7: return AS3KEYCODE_NUMPAD_7;
		case SDL_SCANCODE_KP_8: return AS3KEYCODE_NUMPAD_8;
		case SDL_SCANCODE_KP_9: return AS3KEYCODE_NUMPAD_9;
		case SDL_SCANCODE_KP_0: return AS3KEYCODE_NUMPAD_0;
		case SDL_SCANCODE_KP_PERIOD: return AS3KEYCODE_NUMPAD_DECIMAL;
		case SDL_SCANCODE_F13: return AS3KEYCODE_F13;
		case SDL_SCANCODE_F14: return AS3KEYCODE_F14;
		case SDL_SCANCODE_F15: return AS3KEYCODE_F15;
		case SDL_SCANCODE_HELP: return AS3KEYCODE_HELP;
		case SDL_SCANCODE_MENU: return AS3KEYCODE_MENU;
		case SDL_SCANCODE_LCTRL: return AS3KEYCODE_CONTROL;
		case SDL_SCANCODE_LSHIFT: return AS3KEYCODE_SHIFT;
		case SDL_SCANCODE_LALT: return AS3KEYCODE_ALTERNATE;
		case SDL_SCANCODE_RCTRL: return AS3KEYCODE_CONTROL;
		case SDL_SCANCODE_RSHIFT: return AS3KEYCODE_SHIFT;
		case SDL_SCANCODE_RALT: return AS3KEYCODE_ALTERNATE;
		case SDL_SCANCODE_AC_SEARCH: return AS3KEYCODE_SEARCH;
		case SDL_SCANCODE_AC_BACK: return AS3KEYCODE_BACK ;
		case SDL_SCANCODE_AC_STOP:return AS3KEYCODE_STOP;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"scancode mapping for "<<sdlscancode);
			return AS3KEYCODE_UNKNOWN;
	}
}


void InputThread::sendKeyEvent(const LSKeyEvent& event, bool pressed)
{
	if(m_sys->currentVm == nullptr)
		return;

	_NR<DisplayObject> target = m_sys->stage->getFocusTarget();
	if (target.isNull())
		return;

	tiny_string type = pressed ? "keyDown" : "keyUp";
	m_sys->currentVm->addIdleEvent(target,
	    _MR(Class<KeyboardEvent>::getInstanceS(m_sys->worker,type, event.charCode, event.keyCode, event.modifiers)));
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
	return lastKeyDown;
}

AS3KeyCode InputThread::getLastKeyUp()
{
	Locker locker(mutexListeners);
	return lastKeyUp;
}

AS3KeyCode InputThread::getLastKeyCode()
{
	Locker locker(mutexListeners);
	return lastKeyDown;
}

LSModifier InputThread::getLastKeyMod()
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
	lastKeymod = e->getModifiers();
	lastKeyDown = e->getKeyCode();
	keyDownSet.insert(e->getKeyCode());
	lastKeyUp = AS3KEYCODE_UNKNOWN;
}

void InputThread::setLastKeyUp(KeyboardEvent *e)
{
	Locker locker(mutexListeners);
	lastKeymod = e->getModifiers();
	lastKeyUp = e->getKeyCode();
	keyDownSet.erase(e->getKeyCode());
	lastKeyDown = AS3KEYCODE_UNKNOWN;
}
