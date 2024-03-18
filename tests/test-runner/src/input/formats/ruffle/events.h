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

struct InputEvent;

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

struct Wait {
	const InputEvent& toInputEvent() const;
	template<class Archive>
	void serialize(Archive &archive) {}
};

struct MouseMove {
	Vector2f pos;

	const InputEvent& toInputEvent() const;
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(pos));
	}
};

struct MouseDown {
	Vector2f pos;
	MouseButton btn;

	const InputEvent& toInputEvent() const;
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(pos));
		archive(CEREAL_NVP(btn));
	}
};

struct MouseUp {
	Vector2f pos;
	MouseButton btn;

	const InputEvent& toInputEvent() const;
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(pos));
		archive(CEREAL_NVP(btn));
	}
};

struct KeyDown {
	uint8_t key_code;

	const InputEvent& toInputEvent() const;
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(key_code));
	}
};

struct TextInput {
	WChar<char32_t> codepoint;

	const InputEvent& toInputEvent() const;
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(codepoint));
	}
};

struct TextControl {
	TextControlCode code;

	const InputEvent& toInputEvent() const;
	const KeyCode& toKeyCode() const;
	const Modifier& toModifiers() const;
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(code));
	}
};

struct SetClipboardText {
	tiny_string text;

	SetClipboardText(const SetClipboardText &other) : text(other.text) {}

	const InputEvent& toInputEvent() const;
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(text));
	}
};

ENUM_STRUCT(AutomatedEvent, EVENTS);

#endif /* INPUT_FORMATS_RUFFLE_EVENTS_H */
