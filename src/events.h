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
#include "forwards/events.h"
#include "forwards/scripting/flash/display/flashdisplay.h"
#include "scripting/flash/ui/keycodes.h"
#include "utils/enum.h"
#include "utils/impl.h"
#include "utils/visitor.h"
#include "utils/union.h"
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

struct LSUserEvent;
struct LSUserEventImpl;

using EventTypes = TypeList
<
	// Input events.
	LSKeyEvent,
	LSMouseButtonEvent,
	LSMouseMoveEvent,
	LSMouseWheelEvent,
	LSTextEvent,
	// Non-input events.
	LSWindowResizedEvent,
	LSWindowMovedEvent,
	LSWindowExposedEvent,
	LSWindowFocusEvent,
	LSQuitEvent,
	// Misc events.
	LSInitEvent,
	LSExecEvent,
	LSOpenContextMenuEvent,
	LSSelectItemContextMenuEvent,
	LSRemovedFromStageEvent,
	LSUserEventImpl
>;

// TODO: Write our own variant implementation, and turn this into a variant.
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
		User,
	};
	using EventType = Type;

	constexpr LSEvent(const Type& _type = EventType::Invalid) : type(_type) {}
	constexpr Type getType() const { return type; }

	template<typename U = LSUserEvent, typename V>
	constexpr auto visit(V&& visitor) const;
	template<typename T, EnableIf<std::is_base_of<LSEvent, T>::value, bool> = false>
	constexpr bool has() const
	{
		return visit(makeVisitor
		(
			[](const T&) { return true; },
			[](const LSEvent&) { return false; }
		));
	}
	template<typename T, EnableIf<std::is_base_of<LSUserEvent, T>::value, bool> = false>
	constexpr bool has() const
	{
		return visit<T>(makeVisitor
		(
			[](const T&) { return true; },
			[](const LSEvent&) { return false; }
		));
	}
	constexpr bool isInvalid() const { return type == Type::Invalid; }
private:
	Type type;
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
	uint32_t windowID;
	Vector2f mousePos;
	// TODO: Use twips instead of float.
	Vector2f stagePos;
	LSModifier modifiers;
	bool pressed;

	constexpr LSMouseEvent
	(
		const MouseType& _mouseType,
		uint32_t _windowID,
		const Vector2f& _mousePos,
		const Vector2f& _stagePos,
		const LSModifier& _modifiers,
		bool _pressed
	) :
	LSEvent(EventType::Mouse),
	mouseType(_mouseType),
	windowID(_windowID),
	mousePos(_mousePos),
	stagePos(_stagePos),
	modifiers(_modifiers),
	pressed(_pressed) {}
};

struct LSMouseMoveEvent : public LSMouseEvent
{
	constexpr LSMouseMoveEvent
	(
		uint32_t windowID,
		const Vector2f& mousePos,
		const Vector2f& stagePos,
		const LSModifier& modifiers,
		bool pressed
	) : LSMouseEvent
	(
		MouseType::Move,
		windowID,
		mousePos,
		stagePos,
		modifiers,
		pressed
	) {}
};

struct LSMouseWheelEvent : public LSMouseEvent
{
	number_t delta;

	LSMouseWheelEvent
	(
		uint32_t windowID,
		const Vector2f& mousePos,
		const Vector2f& stagePos,
		const LSModifier& modifiers,
		bool pressed,
		number_t _delta
	) : LSMouseEvent
	(
		MouseType::Wheel,
		windowID,
		mousePos,
		stagePos,
		modifiers,
		pressed
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

	constexpr LSMouseButtonEvent
	(
		uint32_t windowID,
		const Vector2f& mousePos,
		const Vector2f& stagePos,
		const LSModifier& modifiers,
		bool pressed,
		Button _button,
		int _clicks,
		ButtonType _buttonType
	) :
	LSMouseEvent
	(
		MouseType::Button,
		windowID,
		mousePos,
		stagePos,
		modifiers,
		pressed
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

	Vector2f mousePos;
	// TODO: Use twips instead of float.
	Vector2f stagePos;
	AS3KeyCode charCode;
	AS3KeyCode keyCode;
	LSModifier modifiers;
	KeyType keyType;

	constexpr LSKeyEvent
	(
		const Vector2f& _mousePos,
		const Vector2f& _stagePos,
		const AS3KeyCode& _charCode,
		const AS3KeyCode& _keyCode,
		const LSModifier& _modifiers,
		const KeyType& _keyType
	) :
	LSEvent(EventType::Key),
	mousePos(_mousePos),
	stagePos(_stagePos),
	charCode(_charCode),
	keyCode(_keyCode),
	modifiers(_modifiers),
	keyType(_keyType) {}
};

struct LSTextEvent : public LSEvent
{
	enum TextType
	{
		Input,
		Clipboard,
	};

	tiny_string text;
	TextType textType;

	LSTextEvent
	(
		const tiny_string& _text,
		const TextType& _textType
	) : LSEvent(EventType::Text), text(_text), textType(_textType) {}
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

	WindowType windowType;

	constexpr LSWindowEvent(const WindowType& _windowType) : LSEvent(EventType::Window), windowType(_windowType) {}
};

struct LSWindowResizedEvent : public LSWindowEvent
{
	// TODO: Use twips instead of float.
	Vector2f size;

	constexpr LSWindowResizedEvent(const Vector2f& _size) : LSWindowEvent(WindowType::Resized), size(_size) {}
};

struct LSWindowMovedEvent : public LSWindowEvent
{
	// TODO: Maybe use twips instead of float.
	Vector2f pos;

	constexpr LSWindowMovedEvent(const Vector2f& _pos) : LSWindowEvent(WindowType::Moved), pos(_pos) {}
};

struct LSWindowExposedEvent : public LSWindowEvent
{
	constexpr LSWindowExposedEvent() : LSWindowEvent(WindowType::Exposed) {}
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

	constexpr LSWindowFocusEvent
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

	constexpr LSQuitEvent(const QuitType& _quitType) : LSEvent(EventType::Quit), quitType(_quitType) {}
};

// Misc events.
struct LSInitEvent : public LSEvent
{
	SystemState* sys;

	constexpr LSInitEvent(SystemState* _sys) : LSEvent(EventType::Init), sys(_sys) {}
};

struct LSExecEvent : public LSEvent
{
	using Callback = std::function<void(SystemState*)>;
	Callback callback;

	LSExecEvent(Callback _callback) : LSEvent(EventType::Exec), callback(_callback) {}
};

struct LSContextMenuEvent : public LSEvent
{
	enum ContextMenuType
	{
		Open,
		SelectItem,
	};

	ContextMenuType menuType;

	constexpr LSContextMenuEvent(const ContextMenuType& _menuType) :
	LSEvent(EventType::ContextMenu),
	menuType(_menuType) {}
};

struct LSOpenContextMenuEvent : public LSContextMenuEvent
{
	InteractiveObject* obj;

	constexpr LSOpenContextMenuEvent(InteractiveObject* _obj) : LSContextMenuEvent(ContextMenuType::Open), obj(_obj) {}
};

struct LSSelectItemContextMenuEvent : public LSContextMenuEvent
{
	constexpr LSSelectItemContextMenuEvent() : LSContextMenuEvent(ContextMenuType::SelectItem) {}
};

struct LSRemovedFromStageEvent : public LSEvent
{
	constexpr LSRemovedFromStageEvent() : LSEvent(EventType::RemovedFromStage) {}
};

struct LSUserEvent
{
	constexpr operator LSEvent() const { return LSEvent(); }
};

struct LSUserEventImpl : public LSEvent
{
	Impl<LSUserEvent> data;

	constexpr LSUserEventImpl(const LSUserEvent& event) :
	LSEvent(EventType::User),
	data(std::move(event)) {}

	template<typename T, typename V, EnableIf<std::is_base_of<LSUserEvent, T>::value, bool> = false>
	constexpr auto visit(V&& visitor) const { return visitor(static_cast<const T&>(*data)); }

	template<typename T, EnableIf<std::is_base_of<LSUserEvent, T>::value, bool> = false>
	constexpr operator const T&() const { return static_cast<const T&>(*data); }
	template<typename T, EnableIf<std::is_base_of<LSUserEvent, T>::value, bool> = false>
	operator T&() { return static_cast<T&>(*data); }
};

template<typename U, typename V>
constexpr auto LSEvent::visit(V&& visitor) const
{
	using ContextMenuType = LSContextMenuEvent::ContextMenuType;
	using MouseType = LSMouseEvent::MouseType;
	using WindowType = LSWindowEvent::WindowType;

	switch (getType())
	{
		// Input events.
		case LSEvent::Type::Key: return visitor(static_cast<const LSKeyEvent&>(*this)); break;
		case LSEvent::Type::Mouse:
		{
			auto& mouse = static_cast<const LSMouseEvent&>(*this);
			switch (mouse.mouseType)
			{
				case MouseType::Button: return visitor(static_cast<const LSMouseButtonEvent&>(*this)); break;
				case MouseType::Move: return visitor(static_cast<const LSMouseMoveEvent&>(*this)); break;
				case MouseType::Wheel: return visitor(static_cast<const LSMouseWheelEvent&>(*this)); break;
			}
			break;
		}
		case LSEvent::Type::Text: return visitor(static_cast<const LSTextEvent&>(*this)); break;
		// Non-input events.
		case LSEvent::Type::Window:
		{
			auto& window = static_cast<const LSWindowEvent&>(*this);
			switch (window.windowType)
			{
				case WindowType::Resized: return visitor(static_cast<const LSWindowResizedEvent&>(*this)); break;
				case WindowType::Moved: return visitor(static_cast<const LSWindowMovedEvent&>(*this)); break;
				case WindowType::Exposed: return visitor(static_cast<const LSWindowExposedEvent&>(*this)); break;
				case WindowType::Focus: return visitor(static_cast<const LSWindowFocusEvent&>(*this)); break;
			}
			break;
		}
		case LSEvent::Type::Quit: return visitor(static_cast<const LSQuitEvent&>(*this)); break;
		// Misc events.
		case LSEvent::Type::Init: return visitor(static_cast<const LSInitEvent&>(*this)); break;
		case LSEvent::Type::Exec: return visitor(static_cast<const LSExecEvent&>(*this)); break;
		case LSEvent::Type::ContextMenu:
		{
			auto& contextMenu = static_cast<const LSContextMenuEvent&>(*this);
			switch (contextMenu.menuType)
			{
				case ContextMenuType::Open: return visitor(static_cast<const LSOpenContextMenuEvent&>(*this)); break;
				case ContextMenuType::SelectItem: return visitor(static_cast<const LSSelectItemContextMenuEvent&>(*this)); break;
			}
			break;
		}
		case LSEvent::Type::RemovedFromStage: return visitor(static_cast<const LSRemovedFromStageEvent&>(*this)); break;
		case LSEvent::Type::User: return static_cast<const LSUserEventImpl&>(*this).visit<U>(visitor); break;
		// Invalid event, should be unreachable.
		// TODO: Add `compat_unreachable()`, and use it here.
		default: assert(false); break;
	}
	return decltype(visitor(*this))();
}

// Wrapper/Container for returning, and storing `LSEvent`s.
struct LSEventStorage
{
	LSEventStorage() : data() {}
	LSEventStorage(const LSUserEvent& event)
	{
		new(&data) LSUserEventImpl(event);
	}

	LSEventStorage(const LSEvent& event)
	{
		if (event.isInvalid())
			new(&data) LSEvent();
		else if (event.getType() == LSEvent::Type::User)
			new(&data) LSUserEventImpl(static_cast<const LSUserEventImpl&>(event));
		else
			event.visit([&](const auto& event)
			{
				new(&data) RemoveCVRef<decltype(event)>(event);
			});
	}

	LSEventStorage(const LSEventStorage& other) : LSEventStorage((const LSEvent&)other) {}

	LSEventStorage& operator=(const LSEventStorage& other)
	{
		const LSEvent& event = other;
		if (event.isInvalid())
			new(&data) LSEvent();
		else if (event.getType() == LSEvent::Type::User)
			new(&data) LSUserEventImpl(static_cast<const LSUserEventImpl&>(event));
		else
			event.visit([&](const auto& event)
			{
				new(&data) RemoveCVRef<decltype(event)>(event);
			});
		return *this;
	}

	const LSEvent& event() const { return reinterpret_cast<const LSEvent&>(data); }
	LSEvent& event() { return reinterpret_cast<LSEvent&>(data); }
	operator const LSEvent&() const { return event(); }
	operator LSEvent&() { return event(); }
private:
	static constexpr size_t dataSize = UnionSize<EventTypes>::value;
	static constexpr size_t dataAlign = UnionAlign<EventTypes>::value;

	alignas(dataAlign) uint8_t data[dataSize];
};

};
#endif /* EVENTS_H */
