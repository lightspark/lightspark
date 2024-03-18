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

#ifndef INPUT_EVENTS_H
#define INPUT_EVENTS_H 1

#include <cstdint>

#include <lightspark/swftypes.h>
#include <lightspark/tiny_string.h>
#include <lightspark/scripting/flash/ui/keycodes.h>

using namespace lightspark;

// TODO: Turn this into a real type.
using Vector2Twips = Vector2f;

using KeyCode = AS3KeyCode;

// TODO: Put this in the main codebase.
enum class Modifier
{
	None = 0,
	Ctrl = 1 << 0,
	Shift = 1 << 1,
	Alt = 1 << 2,
	Super = 1 << 3,
};

enum class EventType
{
	NextFrame,
	MouseMove,
	MouseWheel,
	MouseButton,
	Key,
	Text,
};

enum class MouseButtonType
{
	Up,
	Down,
	Click,
	DoubleClick,
};

enum class KeyType
{
	Down,
	Up,
	Press,
	Control,
};


enum class TextType
{
	Input,
	Clipboard,
};

struct NextFrameEvent {};

struct MouseMoveEvent
{
	Vector2Twips pos;
};

struct MouseWheelEvent
{
	float delta;
};

struct MouseButtonEvent
{
	enum Button
	{
		Left,
		Middle,
		Right,
	};
	Vector2Twips pos;
	Button button;
	MouseButtonType type;
};

struct KeyEvent
{
	KeyCode keyCode;
	Modifier modifiers;
	KeyType type;
};

struct TextEvent
{
	tiny_string text;
	TextType type;
};

struct InputEvent
{
	EventType type;
	union
	{
		MouseMoveEvent mouseMove;
		MouseWheelEvent mouseWheel;
		MouseButtonEvent mouseButton;
		KeyEvent key;
		TextEvent text;
	};
};

#endif /* INPUT_EVENTS_H */
