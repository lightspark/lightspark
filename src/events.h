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

#include "backends/geometry.h"
#include "forwards/scripting/flash/display/flashdisplay.h"
#include "scripting/flash/ui/keycodes.h"
#include "utils/enum.h"
#include "swftypes.h"
#include "tiny_string.h"

namespace lightspark
{

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
		// Input events.
		Mouse,
		Key,
		Text,
		// Non-input events.
		Window,
		Quit,
		// Misc events.
		Init,
		Exec,
		ContextMenu,
		RemovedFromStage,
		NewTimer,
	};

	virtual ~LSEvent() {}
	virtual Type getType() const { return Type::Invalid; };
};

struct LSMouseEvent : public LSEvent
{
	enum MouseType
	{
		Move,
		Wheel,
		Button,
	};

	MouseType mouseType;
	Vector2f windowPos;
	// TODO: Use twips instead of float.
	Vector2f stagePos;
	LSModifier modifiers;

	LSMouseEvent
	(
		const MouseType& _mouseType,
		const Vector2f& _windowPos,
		const Vector2f& _stagePos,
		const LSModifier& _modifiers
	) :
	mouseType(_mouseType),
	windowPos(_windowPos),
	stagePos(_stagePos),
	modifiers(_modifiers) {}

	LSEvent::Type getType() const override { return LSEvent::Type::Mouse; }
};

struct LSMouseMoveEvent : public LSMouseEvent
{
	LSMouseMoveEvent
	(
		const Vector2f& windowPos,
		const Vector2f& stagePos,
		const LSModifier& modifiers
	) : LSMouseEvent
	(
		MouseType::Move,
		windowPos,
		stagePos,
		modifiers
	) {}
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
	) : LSMouseEvent
	(
		MouseType::Wheel,
		windowPos,
		stagePos,
		modifiers
	), delta(_delta) {}
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
	ButtonType buttonType;

	LSMouseButtonEvent
	(
		const Vector2f& windowPos,
		const Vector2f& stagePos,
		const LSModifier& modifiers,
		Button _button,
		int _clicks,
		ButtonType _buttonType
	) :
	LSMouseEvent
	(
		MouseType::Button,
		windowPos,
		stagePos,
		modifiers
	), button(_button), clicks(_clicks), buttonType(_buttonType) {}
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

// Non-input events.
struct LSWindowEvent : public LSEvent
{
	enum WindowType
	{
		Resized,
		Moved,
		Exposed,
		Focus,
	};

	WindowType type;

	LSWindowEvent(const WindowType& _type) : type(_type) {}
	LSEvent::Type getType() const override { return LSEvent::Type::Window; }
};

struct LSWindowResizedEvent : public LSWindowEvent
{
	// TODO: Use twips instead of float.
	Vector2f size;

	LSWindowResizedEvent(const Vector2f& _size) : LSWindowEvent(WindowType::Resized), size(_size) {}
};

struct LSWindowMovedEvent : public LSWindowEvent
{
	// TODO: Maybe use twips instead of float.
	Vector2f pos;

	LSWindowMovedEvent(const Vector2f& _pos) : LSWindowEvent(WindowType::Moved), pos(_pos) {}
};

struct LSWindowExposedEvent : public LSWindowEvent
{
	LSWindowExposedEvent() : LSWindowEvent(WindowType::Exposed) {}
};

struct LSWindowFocusEvent : public LSWindowEvent
{
	enum FocusType
	{
		Keyboard,
		Mouse,
	};

	FocusType focusType;
	bool focused;

	LSWindowFocusEvent
	(
		const FocusType& _focusType,
		bool _focused
	) : LSWindowEvent(WindowType::Focus), focusType(_focusType), focused(_focused) {}
};

struct LSQuitEvent : public LSEvent
{
	enum QuitType
	{
		System,
		User,
	};

	QuitType quitType;

	LSQuitEvent(const QuitType& _quitType) : quitType(_quitType) {}
	LSEvent::Type getType() const override { return LSEvent::Type::Quit; }
};

// Misc events.
struct LSInitEvent : public LSEvent
{
	LSEvent::Type getType() const override { return LSEvent::Type::Init; }
};

struct LSExecEvent : public LSEvent
{
	using Callback = void (*)(SystemState* sys);
	Callback callback;

	LSExecEvent(Callback _callback) : callback(_callback) {}
	LSEvent::Type getType() const override { return LSEvent::Type::Exec; }
};

struct LSContextMenuEvent : public LSEvent
{
	enum ContextMenuType
	{
		Open,
		Update,
		SelectItem,
	};

	ContextMenuType type;

	LSContextMenuEvent(const ContextMenuType& _type) : type(_type) {}
	LSEvent::Type getType() const override { return LSEvent::Type::Quit; }
};

struct LSOpenContextMenuEvent : public LSContextMenuEvent
{
	InteractiveObject* obj;

	LSOpenContextMenuEvent(InteractiveObject* _obj) : LSContextMenuEvent(ContextMenuType::Open), obj(_obj) {}
};

struct LSUpdateContextMenuEvent : public LSContextMenuEvent
{
	int selectedItem;

	LSUpdateContextMenuEvent(int _selectedItem) : LSContextMenuEvent(ContextMenuType::Update), selectedItem(_selectedItem) {}
};

struct LSSelectItemContextMenuEvent : public LSContextMenuEvent
{
	LSSelectItemContextMenuEvent() : LSContextMenuEvent(ContextMenuType::Update) {}
};

struct LSRemovedFromStageEvent : public LSEvent
{
	LSEvent::Type getType() const override { return LSEvent::Type::RemovedFromStage; }
};

struct LSNewTimerEvent : public LSEvent
{
	LSEvent::Type getType() const override { return LSEvent::Type::NewTimer; }
};

};
#endif /* EVENTS_H */
