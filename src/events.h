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

#ifndef EVENTS_H
#define EVENTS_H 1

#include "swftypes.h"
#include "tiny_string.h"
#include "backends/geometry.h"
#include "scripting/flash/ui/keycodes.h"
#include <type_traits>

namespace lightspark
{

template<typename T, typename std::enable_if<std::is_enum<T>::value, bool>::type = false>
static T& operator&=(T& a, T b)
{
	return a = static_cast<T>(a & b);
}

template<typename T, typename std::enable_if<std::is_enum<T>::value, bool>::type = false>
static T& operator|=(T& a, T b)
{
	return a = static_cast<T>(a | b);
}

template<typename T, typename std::enable_if<std::is_enum<T>::value, bool>::type = false>
static T& operator^=(T& a, T b)
{
	return a = static_cast<T>(a ^ b);
}

enum LSModifier
{
	None = 0,
	Ctrl = 1 << 0,
	Shift = 1 << 1,
	Alt = 1 << 2,
	Super = 1 << 3,
};

struct LSEvent
{
	enum Type
	{
		Invalid,
		MouseMove,
		MouseWheel,
		MouseButton,
		Key,
		Text,
	};

	virtual ~LSEvent() {}
	virtual Type getType() const { return Type::Invalid; };
};

struct LSMouseEvent : public LSEvent
{
	Vector2f windowPos;
	// TODO: Use twips instead of float.
	Vector2f stagePos;
	LSModifier modifiers;

	LSMouseEvent
	(
		const Vector2f& _windowPos,
		const Vector2f& _stagePos,
		const LSModifier& _modifiers
	) : windowPos(_windowPos), stagePos(_stagePos), modifiers(_modifiers) {}
};

struct LSMouseMoveEvent : public LSMouseEvent
{
	LSMouseMoveEvent
	(
		const Vector2f& windowPos,
		const Vector2f& stagePos,
		const LSModifier& modifiers
	) : LSMouseEvent(windowPos, stagePos, modifiers) {}

	LSEvent::Type getType() const override { return LSEvent::Type::MouseMove; }
};

struct LSMouseWheelEvent : public LSMouseEvent
{
	number_t delta;

	LSMouseWheelEvent
	(
		const Vector2f& windowPos,
		const Vector2f& stagePos,
		const LSModifier& modifiers,
		number_t _delta
	) : LSMouseEvent(windowPos, stagePos, modifiers), delta(_delta) {}

	LSEvent::Type getType() const override { return LSEvent::Type::MouseWheel; }
};

struct LSMouseButtonEvent : public LSMouseEvent
{
	enum Button
	{
		Invalid,
		Left,
		Middle,
		Right,
	};

	enum ButtonType
	{
		Up,
		Down,
		Click,
		DoubleClick,
	};

	Button button;
	int clicks;
	ButtonType type;

	LSMouseButtonEvent
	(
		const Vector2f& windowPos,
		const Vector2f& stagePos,
		const LSModifier& modifiers,
		Button _button,
		int _clicks,
		ButtonType _type
	) : LSMouseEvent(windowPos, stagePos, modifiers), button(_button), clicks(_clicks), type(_type) {}

	LSEvent::Type getType() const override { return LSEvent::Type::MouseButton; }
};

struct LSKeyEvent : public LSEvent
{
	enum KeyType
	{
		Down,
		Up,
		Press,
		Control,
	};

	AS3KeyCode keyCode;
	LSModifier modifiers;
	KeyType type;

	LSKeyEvent
	(
		const AS3KeyCode& _keyCode,
		const LSModifier& _modifiers,
		const KeyType& _type
	) : keyCode(_keyCode), modifiers(_modifiers), type(_type) {}

	LSEvent::Type getType() const override { return LSEvent::Type::Key; }
};

struct LSTextEvent : public LSEvent
{
	enum TextType
	{
		Input,
		Clipboard,
	};

	tiny_string text;
	TextType type;

	LSTextEvent
	(
		const tiny_string& _text,
		const TextType& _type
	) : text(_text), type(_type) {}

	LSEvent::Type getType() const override { return LSEvent::Type::Text; }
};

};
#endif /* EVENTS_H */
