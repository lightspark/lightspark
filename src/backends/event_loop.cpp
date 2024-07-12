/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include <cstdlib>
#include <cassert>

#include "backends/event_loop.h"
#include "backends/input.h"
#include "interfaces/backends/event_loop.h"
#include "platforms/engineutils.h"
#include "compat.h"
#include "events.h"
#include "swf.h"
#include "timer.h"

using namespace lightspark;
using namespace std;

static LSModifier toLSModifier(const SDL_Keymod& mod)
{
	LSModifier ret = LSModifier::None;
	if (mod & KMOD_CTRL)
		ret |= LSModifier::Ctrl;
	if (mod & KMOD_SHIFT)
		ret |= LSModifier::Shift;
	if (mod & KMOD_ALT)
		ret |= LSModifier::Alt;
	if (mod & KMOD_GUI)
		ret |= LSModifier::Super;
	return ret;
}

static LSMouseButtonEvent::Button toButton(uint8_t button)
{
	using Button = LSMouseButtonEvent::Button;
	switch (button)
	{
		case SDL_BUTTON_LEFT: return Button::Left; break;
		case SDL_BUTTON_MIDDLE: return Button::Middle; break;
		case SDL_BUTTON_RIGHT: return Button::Right; break;
		default: break;
	}
	return Button::Invalid;
}

static SDL_Keymod toSDLKeymod(const LSModifier& mod)
{
	SDL_Keymod ret = KMOD_NONE;
	if (mod & LSModifier::Ctrl)
		ret |= KMOD_CTRL;
	if (mod & LSModifier::Shift)
		ret |= KMOD_SHIFT;
	if (mod & LSModifier::Alt)
		ret |= KMOD_ALT;
	if (mod & LSModifier::Super)
		ret |= KMOD_GUI;
	return ret;
}

static uint8_t toSDLMouseButton(const LSMouseButtonEvent::Button& button)
{
	using Button = LSMouseButtonEvent::Button;
	switch (button)
	{
		case Button::Left: return SDL_BUTTON_LEFT; break;
		case Button::Middle: return SDL_BUTTON_MIDDLE; break;
		case Button::Right: return SDL_BUTTON_RIGHT; break;
		default: break;
	}
	return 0;
}

static SDL_Keycode toSDLKeycode(const AS3KeyCode& keyCode)
{
	switch (keyCode)
	{
		case AS3KEYCODE_ENTER: return SDLK_RETURN;
		case AS3KEYCODE_ESCAPE: return SDLK_ESCAPE;
		case AS3KEYCODE_BACKSPACE: return SDLK_BACKSPACE;
		case AS3KEYCODE_TAB: return SDLK_TAB;
		case AS3KEYCODE_SPACE: return SDLK_SPACE;
		case AS3KEYCODE_QUOTE: return SDLK_QUOTE;
		case AS3KEYCODE_COMMA: return SDLK_COMMA;
		case AS3KEYCODE_MINUS: return SDLK_MINUS;
		case AS3KEYCODE_PERIOD: return SDLK_PERIOD;
		case AS3KEYCODE_SLASH: return SDLK_SLASH;
		case AS3KEYCODE_NUMBER_0: return SDLK_0;
		case AS3KEYCODE_NUMBER_1: return SDLK_1;
		case AS3KEYCODE_NUMBER_2: return SDLK_2;
		case AS3KEYCODE_NUMBER_3: return SDLK_3;
		case AS3KEYCODE_NUMBER_4: return SDLK_4;
		case AS3KEYCODE_NUMBER_5: return SDLK_5;
		case AS3KEYCODE_NUMBER_6: return SDLK_6;
		case AS3KEYCODE_NUMBER_7: return SDLK_7;
		case AS3KEYCODE_NUMBER_8: return SDLK_8;
		case AS3KEYCODE_NUMBER_9: return SDLK_9;
		case AS3KEYCODE_SEMICOLON: return SDLK_SEMICOLON;
		case AS3KEYCODE_EQUAL: return SDLK_EQUALS;
		case AS3KEYCODE_LEFTBRACKET: return SDLK_LEFTBRACKET;
		case AS3KEYCODE_BACKSLASH: return SDLK_BACKSLASH;
		case AS3KEYCODE_RIGHTBRACKET: return SDLK_RIGHTBRACKET;
		case AS3KEYCODE_BACKQUOTE: return SDLK_BACKQUOTE;
		case AS3KEYCODE_A: return SDLK_a;
		case AS3KEYCODE_B: return SDLK_b;
		case AS3KEYCODE_C: return SDLK_c;
		case AS3KEYCODE_D: return SDLK_d;
		case AS3KEYCODE_E: return SDLK_e;
		case AS3KEYCODE_F: return SDLK_f;
		case AS3KEYCODE_G: return SDLK_g;
		case AS3KEYCODE_H: return SDLK_h;
		case AS3KEYCODE_I: return SDLK_i;
		case AS3KEYCODE_J: return SDLK_j;
		case AS3KEYCODE_K: return SDLK_k;
		case AS3KEYCODE_L: return SDLK_l;
		case AS3KEYCODE_M: return SDLK_m;
		case AS3KEYCODE_N: return SDLK_n;
		case AS3KEYCODE_O: return SDLK_o;
		case AS3KEYCODE_P: return SDLK_p;
		case AS3KEYCODE_Q: return SDLK_q;
		case AS3KEYCODE_R: return SDLK_r;
		case AS3KEYCODE_S: return SDLK_s;
		case AS3KEYCODE_T: return SDLK_t;
		case AS3KEYCODE_U: return SDLK_u;
		case AS3KEYCODE_V: return SDLK_v;
		case AS3KEYCODE_W: return SDLK_w;
		case AS3KEYCODE_X: return SDLK_x;
		case AS3KEYCODE_Y: return SDLK_y;
		case AS3KEYCODE_Z: return SDLK_z;
		case AS3KEYCODE_CAPS_LOCK:return SDLK_CAPSLOCK;
		case AS3KEYCODE_F1: return SDLK_F1;
		case AS3KEYCODE_F2: return SDLK_F2;
		case AS3KEYCODE_F3: return SDLK_F3;
		case AS3KEYCODE_F4: return SDLK_F4;
		case AS3KEYCODE_F5: return SDLK_F5;
		case AS3KEYCODE_F6: return SDLK_F6;
		case AS3KEYCODE_F7: return SDLK_F7;
		case AS3KEYCODE_F8: return SDLK_F8;
		case AS3KEYCODE_F9: return SDLK_F9;
		case AS3KEYCODE_F10: return SDLK_F10;
		case AS3KEYCODE_F11: return SDLK_F11;
		case AS3KEYCODE_F12: return SDLK_F12;
		case AS3KEYCODE_PAUSE: return SDLK_PAUSE;
		case AS3KEYCODE_INSERT: return SDLK_INSERT;
		case AS3KEYCODE_HOME: return SDLK_HOME;
		case AS3KEYCODE_PAGE_UP: return SDLK_PAGEUP;
		case AS3KEYCODE_DELETE: return SDLK_DELETE;
		case AS3KEYCODE_END: return SDLK_END;
		case AS3KEYCODE_PAGE_DOWN: return SDLK_PAGEDOWN;
		case AS3KEYCODE_RIGHT: return SDLK_RIGHT;
		case AS3KEYCODE_LEFT: return SDLK_LEFT;
		case AS3KEYCODE_DOWN: return SDLK_DOWN;
		case AS3KEYCODE_UP: return SDLK_UP;
		case AS3KEYCODE_NUMPAD: return SDLK_NUMLOCKCLEAR;
		case AS3KEYCODE_NUMPAD_DIVIDE: return SDLK_KP_DIVIDE;
		case AS3KEYCODE_NUMPAD_MULTIPLY: return SDLK_KP_MULTIPLY;
		case AS3KEYCODE_NUMPAD_SUBTRACT:return SDLK_KP_MINUS;
		case AS3KEYCODE_NUMPAD_ADD: return SDLK_KP_PLUS;
		case AS3KEYCODE_NUMPAD_ENTER: return SDLK_KP_ENTER;
		case AS3KEYCODE_NUMPAD_1: return SDLK_KP_1;
		case AS3KEYCODE_NUMPAD_2: return SDLK_KP_2;
		case AS3KEYCODE_NUMPAD_3: return SDLK_KP_3;
		case AS3KEYCODE_NUMPAD_4: return SDLK_KP_4;
		case AS3KEYCODE_NUMPAD_5: return SDLK_KP_5;
		case AS3KEYCODE_NUMPAD_6: return SDLK_KP_6;
		case AS3KEYCODE_NUMPAD_7: return SDLK_KP_7;
		case AS3KEYCODE_NUMPAD_8: return SDLK_KP_8;
		case AS3KEYCODE_NUMPAD_9: return SDLK_KP_9;
		case AS3KEYCODE_NUMPAD_0: return SDLK_KP_0;
		case AS3KEYCODE_NUMPAD_DECIMAL: return SDLK_KP_PERIOD;
		case AS3KEYCODE_F13: return SDLK_F13;
		case AS3KEYCODE_F14: return SDLK_F14;
		case AS3KEYCODE_F15: return SDLK_F15;
		case AS3KEYCODE_HELP: return SDLK_HELP;
		case AS3KEYCODE_MENU: return SDLK_MENU;
		case AS3KEYCODE_CONTROL: return SDLK_LCTRL;
		case AS3KEYCODE_SHIFT: return SDLK_LSHIFT;
		case AS3KEYCODE_ALTERNATE: return SDLK_LALT;
		case AS3KEYCODE_SEARCH: return SDLK_AC_SEARCH;
		case AS3KEYCODE_BACK : return SDLK_AC_BACK;
		case AS3KEYCODE_STOP:return SDLK_AC_STOP;
		default: break;
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
	return SDLK_UNKNOWN;
}

LSEvent SDLEvent::toLSEvent(SystemState* sys) const
{
	using KeyType = LSKeyEvent::KeyType;
	using ButtonType = LSMouseButtonEvent::ButtonType;
	using TextType = LSTextEvent::TextType;
	using FocusType = LSWindowFocusEvent::FocusType;
	using QuitType = LSQuitEvent::QuitType;

	switch (event.type)
	{
		// Input events.
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			auto& key = event.key;
			return LSKeyEvent
			(
				getAS3KeyCode(key.keysym.sym),
				toLSModifier((SDL_Keymod)key.keysym.mod),
				event.type == SDL_KEYDOWN ? KeyType::Down : KeyType::Up
			);
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			auto& button = event.button;
			return LSMouseButtonEvent
			(
				Vector2f(button.x, button.y),
				sys->windowToStagePoint(Vector2f(button.x, button.y)),
				toLSModifier(SDL_GetModState()),
				toButton(button.button),
				button.clicks,
				event.type == SDL_MOUSEBUTTONDOWN ? button.clicks == 2 ? ButtonType::DoubleClick : ButtonType::Down : ButtonType::Up
			);
			break;
		}
		case SDL_MOUSEMOTION:
		{
			auto& motion = event.motion;
			return LSMouseMoveEvent
			(
				Vector2f(motion.x, motion.y),
				sys->windowToStagePoint(Vector2f(motion.x, motion.y)),
				toLSModifier(SDL_GetModState())
			);
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			auto& wheel = event.wheel;
			#if SDL_VERSION_ATLEAST(2, 26, 0)
			Vector2f mousePos(wheel.mouseX, wheel.mouseY);
			#else
			Vector2f mousePos = sys->getInputThread()->getMousePos();
			#endif
			return LSMouseWheelEvent
			(
				mousePos,
				sys->windowToStagePoint(mousePos),
				toLSModifier(SDL_GetModState()),
				#if SDL_VERSION_ATLEAST(2, 0, 18)
				wheel.preciseY
				#else
				wheel.y
				#endif
			);
			break;
		}
		case SDL_TEXTINPUT:
		{
			auto& text = event.text;
			return LSTextEvent
			(
				text.text,
				TextType::Input
			);
			break;
		}
		// Non-input events.
		case SDL_WINDOWEVENT:
		{
			auto& window = event.window;
			switch (window.event)
			{
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED: return LSWindowResizedEvent
				(
					Vector2f(window.data1, window.data2)
				);
				break;
				case SDL_WINDOWEVENT_MOVED: return LSWindowMovedEvent
				(
					Vector2f(window.data1, window.data2)
				);
				break;
				case SDL_WINDOWEVENT_EXPOSED: return LSWindowExposedEvent(); break;
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				case SDL_WINDOWEVENT_FOCUS_LOST:
				case SDL_WINDOWEVENT_ENTER:
				case SDL_WINDOWEVENT_LEAVE: return LSWindowFocusEvent
				(
					// focusType
					window.event == SDL_WINDOWEVENT_ENTER ||
					window.event == SDL_WINDOWEVENT_LEAVE
					? FocusType::Mouse : FocusType::Keyboard,
					// focused
					window.event == SDL_WINDOWEVENT_ENTER ||
					window.event == SDL_WINDOWEVENT_FOCUS_GAINED
				);
				break;
			}
			break;
		}
		case SDL_QUIT: return LSQuitEvent(QuitType::System); break;
		// Misc events.
		default:
		{
			switch (event.type-EngineData::userevent)
			{
				// LS_USEREVENT_INIT
				case 0: return LSInitEvent{}; break;
				// LS_USEREVENT_EXEC
				case 1: return LSExecEvent((LSExecEvent::Callback)event.user.data1); break;
				// LS_USEREVENT_QUIT
				case 2: return LSQuitEvent(QuitType::User); break;
				// LS_USEREVENT_OPEN_CONTEXTMENU
				case 3: return LSOpenContextMenuEvent((InteractiveObject*)event.user.data1); break;
				// LS_USEREVENT_UPDATE_CONTEXTMENU
				case 4: return LSUpdateContextMenuEvent(*(int*)event.user.data1); break;
				// LS_USEREVENT_SELECTITEM_CONTEXTMENU
				case 5: return LSSelectItemContextMenuEvent(); break;
				// LS_USEREVENT_INTERACTIVE_OBJECT_REMOVED_FROM_STAGE
				case 6: return LSRemovedFromStageEvent{}; break;
				// LS_USEREVENT_NEWTIMER
				case 7: return LSNewTimerEvent{}; break;
			}
			break;
		}
	}
	return LSEvent{};
}

IEvent& SDLEvent::fromLSEvent(const LSEvent& event)
{
	using KeyType = LSKeyEvent::KeyType;
	using MouseType = LSMouseEvent::MouseType;
	using ButtonType = LSMouseButtonEvent::ButtonType;
	using TextType = LSTextEvent::TextType;
	using WindowType = LSWindowEvent::WindowType;
	using FocusType = LSWindowFocusEvent::FocusType;
	using QuitType = LSQuitEvent::QuitType;
	using ContextMenuType = LSContextMenuEvent::ContextMenuType;

	switch (event.getType())
	{
		case LSEvent::Type::Key:
		{
			auto& key = static_cast<const LSKeyEvent&>(event);
			this->event.type = key.type == KeyType::Up ? SDL_KEYUP : SDL_KEYDOWN;
			this->event.key.keysym.sym = toSDLKeycode(key.keyCode);
			this->event.key.keysym.mod = toSDLKeymod(key.modifiers);
			break;
		}
		case LSEvent::Type::Mouse:
		{
			auto& mouse = static_cast<const LSMouseEvent&>(event);
			switch (mouse.mouseType)
			{
				case MouseType::Button:
				{
					auto& button = static_cast<const LSMouseButtonEvent&>(event);
					this->event.type = button.buttonType == ButtonType::Up ? SDL_MOUSEBUTTONUP : SDL_MOUSEBUTTONDOWN;
					this->event.button.x = button.mousePos.x;
					this->event.button.y = button.mousePos.y;
					this->event.button.button = toSDLMouseButton(button.button);
					this->event.button.clicks = button.clicks;
					break;
				}
				case MouseType::Move:
				{
					auto& move = static_cast<const LSMouseMoveEvent&>(event);
					this->event.type = SDL_MOUSEMOTION;
					this->event.motion.x = move.mousePos.x;
					this->event.motion.y = move.mousePos.y;
					break;
				}
				case MouseType::Wheel:
				{
					auto& wheel = static_cast<const LSMouseWheelEvent&>(event);
					this->event.type = SDL_MOUSEWHEEL;
					#if SDL_VERSION_ATLEAST(2, 0, 18)
					this->event.wheel.preciseY = wheel.delta;
					#endif
					this->event.wheel.y = wheel.delta;
					#if SDL_VERSION_ATLEAST(2, 26, 0)
					this->event.wheel.mouseX = wheel.mousePos.x;
					this->event.wheel.mouseY = wheel.mousePos.y;
					#endif
					break;
				}
			}
			break;
		}
		case LSEvent::Type::Text:
		{
			auto& text = static_cast<const LSTextEvent&>(event);
			if (text.type != TextType::Input)
				break;
			this->event.type = SDL_TEXTINPUT;
			memcpy(this->event.text.text, text.text.raw_buf(), text.text.numBytes());
			break;
		}
		// Non-input events.
		case LSEvent::Type::Window:
		{
			auto& window = static_cast<const LSWindowEvent&>(event);
			this->event.type = SDL_WINDOWEVENT;
			switch (window.type)
			{
				case WindowType::Resized:
				{
					auto& resize = static_cast<const LSWindowResizedEvent&>(event);
					this->event.window.event = SDL_WINDOWEVENT_RESIZED;
					this->event.window.data1 = resize.size.x;
					this->event.window.data2 = resize.size.y;
					break;
				}
				case WindowType::Moved:
				{
					auto& move = static_cast<const LSWindowMovedEvent&>(event);
					this->event.window.event = SDL_WINDOWEVENT_MOVED;
					this->event.window.data1 = move.pos.x;
					this->event.window.data2 = move.pos.y;
					break;
				}
				case WindowType::Exposed:
				{
					this->event.window.event = SDL_WINDOWEVENT_EXPOSED;
					break;
				}
				case WindowType::Focus:
				{
					auto& focus = static_cast<const LSWindowFocusEvent&>(event);
					if (focus.focusType == FocusType::Keyboard)
						this->event.window.event = focus.focused ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST;
					else
						this->event.window.event = focus.focused ? SDL_WINDOWEVENT_ENTER : SDL_WINDOWEVENT_LEAVE;
					break;
				}
			}
			break;
		}
		case LSEvent::Type::Quit:
		{
			auto& quit = static_cast<const LSQuitEvent&>(event);
			this->event.type = quit.quitType == QuitType::System ? SDL_QUIT : (SDL_EventType)LS_USEREVENT_QUIT;
			break;
		}
		// Misc events.
		case LSEvent::Type::Init:
		{
			this->event.type = LS_USEREVENT_INIT;
			break;
		}
		case LSEvent::Type::Exec:
		{
			auto& exec = static_cast<const LSExecEvent&>(event);
			this->event.type = LS_USEREVENT_EXEC;
			this->event.user.data1 = (void*)exec.callback;
			break;
		}
		case LSEvent::Type::ContextMenu:
		{
			auto& context = static_cast<const LSContextMenuEvent&>(event);
			switch (context.type)
			{
				case ContextMenuType::Open:
				{
					auto& open = static_cast<const LSOpenContextMenuEvent&>(event);
					this->event.type = LS_USEREVENT_OPEN_CONTEXTMENU;
					this->event.user.data1 = open.obj;
					break;
				}
				case ContextMenuType::Update:
				{
					auto& update = static_cast<const LSUpdateContextMenuEvent&>(event);
					this->event.type = LS_USEREVENT_UPDATE_CONTEXTMENU;
					this->event.user.data1 = new int(update.selectedItem);
					break;
				}
				case ContextMenuType::SelectItem:
				{
					this->event.type = LS_USEREVENT_SELECTITEM_CONTEXTMENU;
					break;
				}
			}
			break;
		}
		case LSEvent::Type::RemovedFromStage:
		{
			this->event.type = LS_USEREVENT_INTERACTIVEOBJECT_REMOVED_FOM_STAGE;
			break;
		}
		case LSEvent::Type::NewTimer:
		{
			this->event.type = LS_USEREVENT_NEW_TIMER;
			break;
		}
		default: break;
	}
	return *this;
}

bool SDLEventLoop::waitEvent(IEvent& event, SystemState* sys)
{
	SDLEvent& ev = static_cast<SDLEvent&>(event);
	Locker l(listMutex);
	for (;;)
	{
		int64_t delay = -1;
		if (!timers.empty())
		{
			auto now = TimeSpec::fromNs(time->getCurrentTime_ns());
			auto deadline = timers.front().deadline();
			delay = now < deadline ? (deadline - now).toMsRound() : 0;
		}

		l.release();
		int gotEvent = SDL_WaitEventTimeout(&ev.event, delay);
		l.acquire();
		if (gotEvent && ev.event.type != LS_USEREVENT_NEW_TIMER)
			return true;

		if (sys != nullptr && sys->isShuttingDown())
			return false;
		if (timers.empty())
			continue;

		TimeEvent timer = timers.front();
		auto deadline = timer.deadline();
		auto now = TimeSpec::fromNs(time->getCurrentTime_ns());

		if (now >= deadline)
		{
			timers.pop_front();
			if (timer.job->stopMe)
			{
				timer.job->tickFence();
				continue;
			}
			if (timer.isTick)
			{
				timer.startTime = now;
				insertEventNoLock(timer);
			}

			l.release();
			timer.job->tick();
			l.acquire();

			if (!timer.isTick)
				timer.job->tickFence();
		}
	}
}

void SDLEventLoop::insertEvent(const SDLEventLoop::TimeEvent& e)
{
	Locker l(listMutex);
	insertEventNoLock(e);
}

void SDLEventLoop::insertEventNoLock(const SDLEventLoop::TimeEvent& e)
{
	if (timers.empty() || timers.front().deadline() > e.deadline())
	{
		timers.push_front(e);
		SDL_Event event;
		SDL_zero(event);
		event.type = LS_USEREVENT_NEW_TIMER;
		SDL_PushEvent(&event);
		return;
	}

	auto it = std::find_if(std::next(timers.begin()), timers.end(), [&](const TimeEvent& it)
	{
		return it.deadline() > e.deadline();
	});
	timers.insert(it, e);
}
void SDLEventLoop::addJob(uint32_t ms, bool isTick, ITickJob* job)
{
	insertEvent(TimeEvent { isTick, TimeSpec::fromNs(time->getCurrentTime_ns()), TimeSpec::fromMs(ms), job });
}

void SDLEventLoop::addTick(uint32_t tickTime, ITickJob* job)
{
	addJob(tickTime, true, job);
}

void SDLEventLoop::addWait(uint32_t waitTime, ITickJob* job)
{
	addJob(waitTime, false, job);
}

void SDLEventLoop::removeJobNoLock(ITickJob* job)
{
	auto it = std::remove_if(timers.begin(), timers.end(), [&](const TimeEvent& it)
	{
		return it.job == job;
	});
	if (it == timers.end())
		return;
	bool first = it == timers.begin();
	timers.erase(it);
	if (first)
	{
		SDL_Event event;
		SDL_zero(event);
		event.type = LS_USEREVENT_NEW_TIMER;
		SDL_PushEvent(&event);
	}
}

void SDLEventLoop::removeJob(ITickJob* job)
{
	Locker l(listMutex);
	removeJobNoLock(job);
}
