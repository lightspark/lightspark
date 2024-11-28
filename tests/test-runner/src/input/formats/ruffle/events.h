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
#include <lightspark/utils/optional.h>

#include "input/events.h"
#include "utils/cereal_overloads.h"
#include "utils/macros.h"
#include "utils/wide_char.h"

#define EVENTS(o) \
	o(Wait) \
	o(MouseMove, mouse_move) \
	o(MouseDown, mouse_down) \
	o(MouseWheel, mouse_wheel) \
	o(MouseUp, mouse_up) \
	o(KeyDown, key_down) \
	o(KeyUp, key_up) \
	o(TextInput, text_input) \
	o(TextControl, text_control) \
	o(SetClipboardText, set_clipboard_text)

using namespace lightspark;

namespace cereal
{
	template<class Archive>
	std::string save_minimal(Archive& archive, const tiny_string& str)
	{
		return str;
	}

	template<class Archive>
	void load_minimal(Archive& archive, tiny_string& value, const std::string& str)
	{
		value = str;
	}

	template<class Archive, typename T>
	void save(Archive& archive, const Optional<T>& optional)
	{
		if (optional.hasValue())
			cereal::save_inline(archive, *optional);
	}

	template<class Archive, typename T>
	void load(Archive& archive, Optional<T>& optional)
	{
		try
		{
			optional.emplace();
			cereal::load_inline(archive, *optional);
		}
		catch (std::exception& e)
		{
			optional.reset();
		}
	}

	template<class Archive, typename T>
	void save(Archive& archive, const Vector2Tmpl<T>& vec)
	{
		archive(cereal::make_size_tag(2));
		archive(vec.x, vec.y);
	}

	template<class Archive, typename T>
	void load(Archive& archive, Vector2Tmpl<T>& vec)
	{
		size_t size;
		archive(cereal::make_size_tag(size));
		archive(vec.x, vec.y);
	}
};

enum class EventType
{
	Wait,
	MouseMove,
	MouseDown,
	MouseUp,
	KeyDown,
	TextInput,
	TextControl,
	SetClipboardText,
};

enum class MouseButton
{
	Left,
	Middle,
	Right,
};

enum class TextControlCode
{
	MoveLeft,
	MoveLeftWord,
	MoveLeftLine,
	MoveLeftDocument,
	MoveRight,
	MoveRightWord,
	MoveRightLine,
	MoveRightDocument,
	SelectLeft,
	SelectLeftWord,
	SelectLeftLine,
	SelectLeftDocument,
	SelectRight,
	SelectRightWord,
	SelectRightLine,
	SelectRightDocument,
	SelectAll,
	Copy,
	Paste,
	Cut,
	Backspace,
	Enter,
	Delete,
};

static LSMouseButtonEvent::Button toButton(const MouseButton& button)
{
	using Button = LSMouseButtonEvent::Button;
	switch (button)
	{
		case MouseButton::Left: return Button::Left; break;
		case MouseButton::Middle: return Button::Middle; break;
		case MouseButton::Right: return Button::Right; break;
	}
	return Button::Invalid;
}

struct EventHandledAssertion
{
	bool value;
	tiny_string message;

	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(CEREAL_NVP(value));
		archive(CEREAL_NVP(message));
	}
};

struct Wait
{
	operator LSEventStorage() const { return TestRunnerNextFrameEvent {}; }
	template<class Archive>
	void serialize(Archive& archive) {}
};

struct MouseMove
{
	Vector2f pos;

	operator LSEventStorage() const
	{
		return LSMouseMoveEvent
		(
			0,
			pos,
			pos,
			LSModifier::None,
			false
		);
	}
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(CEREAL_NVP(pos));
	}
};

struct MouseDown
{
	using ButtonType = LSMouseButtonEvent::ButtonType;
	Vector2f pos;
	MouseButton btn;
	Optional<size_t> index;
	Optional<EventHandledAssertion> assert_handled;

	operator LSEventStorage() const
	{
		return LSMouseButtonEvent
		(
			0,
			pos,
			pos,
			LSModifier::None,
			true,
			toButton(btn),
			-1,
			ButtonType::Down
		);
	}
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(CEREAL_NVP(pos));
		archive(CEREAL_NVP(btn));
		try { archive(CEREAL_NVP(index)); } catch(std::exception&) {}
		try { archive(CEREAL_NVP(assert_handled)); } catch(std::exception&) {}
	}
};

struct MouseUp
{
	using ButtonType = LSMouseButtonEvent::ButtonType;
	Vector2f pos;
	MouseButton btn;

	operator LSEventStorage() const
	{
		return LSMouseButtonEvent
		(
			0,
			pos,
			pos,
			LSModifier::None,
			false,
			toButton(btn),
			-1,
			ButtonType::Up
		);
	}
	template<class Archive>
	void serialize(Archive &archive)
	{
		archive(CEREAL_NVP(pos));
		archive(CEREAL_NVP(btn));
	}
};

struct MouseWheel
{
	Optional<double> lines;
	Optional<double> pixels;

	operator LSEventStorage() const
	{
		return LSMouseWheelEvent
		(
			0,
			Vector2f(NAN, NAN),
			Vector2f(NAN, NAN),
			LSModifier::None,
			false,
			lines.valueOr(*pixels / 100.0)
		);
	}
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(CEREAL_NVP(lines));
		archive(CEREAL_NVP(pixels));
	}
};

struct KeyDown
{
	using KeyType = LSKeyEvent::KeyType;
	uint8_t key_code;

	operator LSEventStorage() const
	{
		return LSKeyEvent
		(
			Vector2f(NAN, NAN),
			Vector2f(NAN, NAN),
			AS3KeyCode(key_code),
			AS3KEYCODE_UNKNOWN,
			LSModifier::None,
			KeyType::Down
		);
	}
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(CEREAL_NVP(key_code));
	}
};

struct KeyUp
{
	using KeyType = LSKeyEvent::KeyType;
	uint8_t key_code;

	operator LSEventStorage() const
	{
		return LSKeyEvent
		(
			Vector2f(NAN, NAN),
			Vector2f(NAN, NAN),
			AS3KeyCode(key_code),
			AS3KEYCODE_UNKNOWN,
			LSModifier::None,
			KeyType::Up
		);
	}

	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(CEREAL_NVP(key_code));
	}
};

struct TextInput
{
	using TextType = LSTextEvent::TextType;
	WChar<char32_t> codepoint;

	operator LSEventStorage() const { return LSTextEvent(tiny_string(codepoint), TextType::Input); }
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(CEREAL_NVP(codepoint));
	}
};

struct TextControl
{
	using KeyType = LSKeyEvent::KeyType;
	TextControlCode code;

	operator LSEventStorage() const
	{
		return LSKeyEvent
		(
			Vector2f(NAN, NAN),
			Vector2f(NAN, NAN),
			toKeyCode(),
			AS3KEYCODE_UNKNOWN,
			toModifiers(),
			KeyType::Control
		);
	}
	AS3KeyCode toKeyCode() const
	{
		switch (code)
		{
			case TextControlCode::MoveLeft:
			case TextControlCode::MoveLeftWord:
			case TextControlCode::SelectLeft:
			case TextControlCode::SelectLeftWord: return AS3KEYCODE_LEFT; break;
			case TextControlCode::MoveLeftLine:
			case TextControlCode::MoveLeftDocument:
			case TextControlCode::SelectLeftLine:
			case TextControlCode::SelectLeftDocument: return AS3KEYCODE_HOME; break;
			case TextControlCode::MoveRight:
			case TextControlCode::MoveRightWord:
			case TextControlCode::SelectRight:
			case TextControlCode::SelectRightWord: return AS3KEYCODE_RIGHT; break;
			case TextControlCode::MoveRightLine:
			case TextControlCode::MoveRightDocument:
			case TextControlCode::SelectRightLine:
			case TextControlCode::SelectRightDocument: return AS3KEYCODE_END; break;
			case TextControlCode::Backspace: return AS3KEYCODE_BACKSPACE; break;
			case TextControlCode::Enter: return AS3KEYCODE_ENTER; break;
			case TextControlCode::Delete: return AS3KEYCODE_DELETE; break;
			case TextControlCode::SelectAll: return AS3KEYCODE_A; break;
			case TextControlCode::Copy: return AS3KEYCODE_C; break;
			case TextControlCode::Paste: return AS3KEYCODE_V; break;
			case TextControlCode::Cut: return AS3KEYCODE_X; break;
		}
		return AS3KEYCODE_UNKNOWN;
	}
	LSModifier toModifiers() const
	{
		switch (code)
		{
			case TextControlCode::SelectLeftWord:
			case TextControlCode::SelectRightWord:
			case TextControlCode::SelectLeftDocument:
			case TextControlCode::SelectRightDocument: return LSModifier(LSModifier::Ctrl | LSModifier::Shift); break;
			case TextControlCode::SelectLeftLine:
			case TextControlCode::SelectRightLine:
			case TextControlCode::MoveLeft:
			case TextControlCode::MoveRight:
			case TextControlCode::MoveLeftLine:
			case TextControlCode::MoveRightLine:
			case TextControlCode::Backspace:
			case TextControlCode::Enter:
			case TextControlCode::Delete: return LSModifier::None; break;
			case TextControlCode::SelectLeft:
			case TextControlCode::SelectRight: return LSModifier::Shift; break;
			case TextControlCode::MoveLeftWord:
			case TextControlCode::MoveRightWord:
			case TextControlCode::MoveLeftDocument:
			case TextControlCode::MoveRightDocument:
			case TextControlCode::SelectAll:
			case TextControlCode::Copy:
			case TextControlCode::Paste:
			case TextControlCode::Cut: return LSModifier::Ctrl; break;
		}
		return LSModifier::None;
	}
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(CEREAL_NVP(code));
	}
};

struct SetClipboardText
{
	using TextType = LSTextEvent::TextType;
	tiny_string text;

	SetClipboardText() {}
	SetClipboardText(const SetClipboardText &other) : text(other.text) {}

	operator LSEventStorage() const { return LSTextEvent(text, TextType::Clipboard); }
	template<class Archive>
	void serialize(Archive& archive)
	{
		archive(CEREAL_NVP(text));
	}
};

ENUM_STRUCT(AutomatedEvent, EVENTS);

#endif /* INPUT_FORMATS_RUFFLE_EVENTS_H */
