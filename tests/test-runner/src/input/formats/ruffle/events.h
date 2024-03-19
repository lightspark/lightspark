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

#ifndef INPUT_FORMATS_RUFFLE_EVENTS_H
#define INPUT_FORMATS_RUFFLE_EVENTS_H 1

#include <cstdint>
#include <string>
#include <variant>

#include <cereal/cereal.hpp>
#include <lightspark/swftypes.h>
#include <lightspark/tiny_string.h>

#include "input/events.h"
#include "utils/macros.h"
#include "utils/wide_char.h"

#define EVENTS(o) \
	o(Wait) \
	o(MouseMove, mouse_move) \
	o(MouseDown, mouse_down) \
	o(MouseUp, mouse_up) \
	o(KeyDown, key_down) \
	o(TextInput, text_input) \
	o(TextControl, text_control) \
	o(SetClipboardText, set_clipboard_text)

using namespace lightspark;

enum class EventType {
	Wait,
	MouseMove,
	MouseDown,
	MouseUp,
	KeyDown,
	TextInput,
	TextControl,
	SetClipboardText,
};

enum class MouseButton {
	Left,
	Middle,
	Right,
};

enum class TextControlCode {
	MoveLeft,
	MoveRight,
	SelectLeft,
	SelectRight,
	SelectAll,
	Copy,
	Paste,
	Cut,
	Backspace,
	Enter,
	Delete,
};

static MouseButtonEvent::Button toButton(const MouseButton& button)
{
	switch (button)
	{
		MouseButton::Left: return MouseButtonEvent::Left; break;
		MouseButton::Middle: return MouseButtonEvent::Middle; break;
		MouseButton::Right: return MouseButtonEvent::Right; break;
	}
}

struct Wait {
	operator InputEvent() const { return InputEvent { NextFrameEvent {} }; }
	template<class Archive>
	void serialize(Archive &archive) {}
};

struct MouseMove {
	Vector2f pos;

	operator InputEvent() const { return InputEvent { MouseMoveEvent { pos } }; }
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(pos));
	}
};

struct MouseDown {
	Vector2f pos;
	MouseButton btn;

	operator InputEvent() const { return InputEvent { MouseButtonEvent { pos, toButton(btn), MouseButtonType::Down } }; }
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(pos));
		archive(CEREAL_NVP(btn));
	}
};

struct MouseUp {
	Vector2f pos;
	MouseButton btn;

	operator InputEvent() const { return InputEvent { MouseButtonEvent { pos, toButton(btn), MouseButtonType::Up } }; }
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(pos));
		archive(CEREAL_NVP(btn));
	}
};

struct KeyDown {
	uint8_t key_code;

	operator InputEvent() const { return InputEvent { KeyEvent { KeyCode(key_code), Modifiers::None, KeyType::Down } }; }
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(key_code));
	}
};

struct TextInput {
	WChar<char32_t> codepoint;

	operator InputEvent() const { return InputEvent { TextEvent { codepoint, TextType::Input } }; }
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(codepoint));
	}
};

struct TextControl {
	TextControlCode code;

	operator InputEvent() const { return InputEvent { KeyEvent { toKeyCode(), toModifiers(), KeyType::Control } }; }
	const KeyCode& toKeyCode() const
	{
		switch (code)
		{
			case TextControlCode::MoveLeft:
			case TextControlCode::MoveRight
			case TextControlCode::Backspace:
			case TextControlCode::Enter:
			case TextControlCode::Delete: return Modifier::None; break;
			case TextControlCode::SelectLeft:
			case TextControlCode::SelectRight: return Modifier::Shift; break;
			case TextControlCode::SelectAll:
			case TextControlCode::Copy:
			case TextControlCode::Paste:
			case TextControlCode::Cut: return Modifier::Ctrl; break;
		}
	}
	const Modifier& toModifiers() const
	{
		switch (code)
		{
			case TextControlCode::MoveLeft:
			case TextControlCode::MoveRight
			case TextControlCode::Backspace:
			case TextControlCode::Enter:
			case TextControlCode::Delete: return Modifier::None; break;
			case TextControlCode::SelectLeft:
			case TextControlCode::SelectRight: return Modifier::Shift; break;
			case TextControlCode::SelectAll:
			case TextControlCode::Copy:
			case TextControlCode::Paste:
			case TextControlCode::Cut: return Modifier::Ctrl; break;
		}
	}
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(code));
	}
};

struct SetClipboardText {
	tiny_string text;

	SetClipboardText(const SetClipboardText &other) : text(other.text) {}

	operator InputEvent() const { return InputEvent { TextEvent { text, TextType::Clipboard } }; }
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(text));
	}
};

ENUM_STRUCT(AutomatedEvent, EVENTS);

#endif /* INPUT_FORMATS_RUFFLE_EVENTS_H */
