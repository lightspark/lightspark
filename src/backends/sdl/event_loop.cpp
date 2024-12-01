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

#include <SDL.h>

#include "backends/sdl/event_loop.h"
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

LSEventStorage SDLEventLoop::toLSEvent(SystemState* sys, const SDL_Event& event)
{
	using ButtonType = LSMouseButtonEvent::ButtonType;
	using FocusType = LSWindowFocusEvent::FocusType;
	using KeyType = LSKeyEvent::KeyType;
	using QuitType = LSQuitEvent::QuitType;
	using TextType = LSTextEvent::TextType;

	switch (event.type)
	{
		// Input events.
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			auto& key = event.key;
			Vector2 mousePos;
			SDL_GetMouseState(&mousePos.x, &mousePos.y);
			return LSKeyEvent
			(
				mousePos,
				sys->windowToStagePoint(mousePos),
				getAS3KeyCodeFromScanCode(key.keysym.scancode),
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
				button.windowID,
				Vector2f(button.x, button.y),
				sys->windowToStagePoint(Vector2f(button.x, button.y)),
				toLSModifier(SDL_GetModState()),
				button.state == SDL_PRESSED,
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
				motion.windowID,
				Vector2f(motion.x, motion.y),
				sys->windowToStagePoint(Vector2f(motion.x, motion.y)),
				toLSModifier(SDL_GetModState()),
				motion.state == SDL_PRESSED
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
				wheel.windowID,
				mousePos,
				sys->windowToStagePoint(mousePos),
				toLSModifier(SDL_GetModState()),
				false,
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
			// SDL_TEXINPUT sometimes seems to send an empty text, we ignore those events
			if (text.text[0] != '\0')
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
		default: break;
	}
	return LSEvent();
}

void SDLEventLoop::notify()
{
	SDL_Event event;
	SDL_zero(event);
	event.type = LS_USEREVENT_NOTIFY;
	SDL_PushEvent(&event);
}

bool SDLEventLoop::notified(const SDL_Event& event) const
{
	return event.type == LS_USEREVENT_NOTIFY && peekEvent().hasValue();
}

Optional<LSEventStorage> SDLEventLoop::waitEventImpl(SystemState* sys)
{
	bool hasSys = sys != nullptr;

	auto getNewDeadline = [&]
	{
		return time->now() + sys->timeUntilNextTick().valueOr(TimeSpec());
	};

	deadline = deadline.orElseIf(hasSys, [&]
	{
		startTime = time->now();
		return getNewDeadline();
	});

	for (;;)
	{
		int64_t delay = hasSys ? deadline->saturatingSub(time->now()).toMsRound() : -1;
		SDL_Event ev;
		if (SDL_WaitEventTimeout(&ev, delay) && ev.type != LS_USEREVENT_NEW_TIMER)
			return notified(ev) ? popEvent() : toLSEvent(sys, ev);
		if (hasSys)
		{
			if (sys->isShuttingDown())
				return {};
			TimeSpec endTime = time->now();
			if (endTime >= *deadline)
			{
				auto delta = endTime.absDiff(startTime);
				startTime = time->now();
				sys->updateTimers(delta, true);
				deadline = getNewDeadline();
			}
		}
	}
}
