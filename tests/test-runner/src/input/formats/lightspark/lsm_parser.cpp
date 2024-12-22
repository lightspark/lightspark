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

#include <fstream>
#include <sstream>
#include <vector>

#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>

#include "input/formats/lightspark/lsm_parser.h"

// TODO: Move this into the main codebase, once TASing support is added.

using KeyType = LSKeyEvent::KeyType;
using TextType = LSTextEvent::TextType;

using LSButton = LSMouseButtonEvent::Button;
using LSButtonType = LSMouseButtonEvent::ButtonType;
using LSFocusType = LSWindowFocusEvent::FocusType;

// NOTE: The reason these enums must be in their own namespace, is due
// to them both having a value name in common, and also have to be able
// to implicitly convert into an `ssize_t` for `Enum`.
namespace MouseEnum
{

enum MouseType
{
	Move,
	Button,
	Wheel,
	Up,
	Down,
	Click,
	DoubleClick,
};

}

namespace WindowEnum
{

enum WindowType
{
	Resize,
	Move,
	Focus,
};

}

using namespace MouseEnum;
using namespace WindowEnum;

static std::list<AS3KeyCode> parseKeys(const LSMemberInfo& memberInfo, const tiny_string& name, const LSToken::Expr& expr);
static AS3KeyCode parseKey(const LSMemberInfo& memberInfo, const tiny_string& name, const LSToken::Expr& expr);
static LSModifier parseModifier(const LSMemberInfo& memberInfo, const tiny_string& name, const LSToken::Expr& expr);

template<>
struct GetValue<std::list<AS3KeyCode>>
{
	static std::list<AS3KeyCode> getValue(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		return parseKeys(memberInfo, name, expr);
	}
};

template<>
struct GetValue<AS3KeyCode>
{
	static AS3KeyCode getValue(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		return parseKey(memberInfo, name, expr);
	}
};

template<>
struct GetValue<LSModifier>
{
	static LSModifier getValue(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		return parseModifier(memberInfo, name, expr);
	}
};

template<>
struct IsValid<std::list<AS3KeyCode>>
{
	static bool isValid(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		try
		{
			(void)parseKeys(memberInfo, name, expr);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
};

template<>
struct IsValid<AS3KeyCode>
{
	static bool isValid(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		try
		{
			(void)parseKey(memberInfo, name, expr);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
};

template<>
struct IsValid<LSModifier>
{
	static bool isValid(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		try
		{
			(void)parseModifier(memberInfo, name, expr);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
};

static const LSEnumPair mouseTypes
{
	// shortTypes.
	LSShortEnum
	({
		{ 'm', MouseType::Move },
		{ 'b', MouseType::Button },
		{ 'w', MouseType::Wheel },
		{ 'u', MouseType::Up },
		{ 'd', MouseType::Down },
		{ 'c', MouseType::Click },
		{ 'D', MouseType::DoubleClick },
	}),
	// longTypes.
	LSEnum
	({
		{ "Move", MouseType::Move },
		{ "Button", MouseType::Button },
		{ "Wheel", MouseType::Wheel },
		{ "Up", MouseType::Up },
		{ "Down", MouseType::Down },
		{ "Click", MouseType::Click },
		{ "DoubleClick", MouseType::DoubleClick },
	})
};

static const LSEnumPair keyTypes
{
	// shortTypes.
	LSShortEnum
	({
		{ 'u', KeyType::Up },
		{ 'd', KeyType::Down },
		{ 'p', KeyType::Press },
		{ 'c', KeyType::Control },
	}),
	// longTypes.
	LSEnum
	({
		{ "Up", KeyType::Up },
		{ "Down", KeyType::Down },
		{ "Press", KeyType::Press },
		{ "Control", KeyType::Control },
	}),
};

static const LSEnumPair textTypes
{
	// shortTypes.
	LSShortEnum
	({
		{ 'i', TextType::Input },
		{ 'c', TextType::Clipboard },
	}),
	// longTypes.
	LSEnum
	({
		{ "Input", TextType::Input },
		{ "Clipboard", TextType::Clipboard },
	}),
};

static const LSEnumPair windowTypes
{
	// shortTypes.
	LSShortEnum
	({
		{ 'r', WindowType::Resize },
		{ 'm', WindowType::Move },
		{ 'f', WindowType::Focus },
	}),
	// longTypes.
	LSEnum
	({
		{ "Resize", WindowType::Resize },
		{ "Move", WindowType::Move },
		{ "Focus", WindowType::Focus },
	}),
};

const LSMemberInfo LSMovieParser::headerInfo
{
	// memberMap.
	{
		MEMBER(LSMovieHeader, player),
		MEMBER(LSMovieHeader, version),
		MEMBER(LSMovieHeader, name),
		MEMBER(LSMovieHeader, description),
		MEMBER(LSMovieHeader, authors),
		// Single entry version of `authors`.
		MEMBER_NAME_LONG(LSMovieHeader, authors, author, true),
		MEMBER_NAME(LSMovieHeader, path, file),
		MEMBER_NAME(LSMovieHeader, md5Hash, md5),
		MEMBER_NAME(LSMovieHeader, sha1Hash, sha1),
		MEMBER_NAME(LSMovieHeader, numFrames, frames),
		MEMBER_NAME(LSMovieHeader, numRerecords, rerecords),
		MEMBER(LSMovieHeader, allowNoScale),
		MEMBER(LSMovieHeader, endMode),
		MEMBER_NAME(LSMovieHeader, startingResolution, resolution),
		MEMBER_NAME(LSMovieHeader, startingSeed, seed),
		MEMBER_NAME(LSMovieHeader, eventTimeStep, timeStep),
		MEMBER_NAME(LSMovieHeader, scaleMode, scaling),
	},
	// memberAliases.
	{
			// Aliases.
			// `description`.
			{ "desc", "description" },
			{ "info", "description" },
			// `frames`.
			{ "numFrames", "frames" },
			{ "frameCount", "frames" },
			// `rerecords`.
			{ "numRerecords", "rerecords" },
			{ "rerecordCount", "rerecords" },
			{ "numSaveStates", "rerecords" },
			// `endMode`.
			{ "movieEndMode", "endMode" },
			{ "onMovieEnd", "endMode" },
			// `resolution`.
			{ "startingResolution", "resolution" },
			{ "res", "resolution" },
			{ "dimensions", "resolution" },
			// `seed`.
			{ "startingSeed", "seed" },
			{ "rngSeed", "seed" },
			// `scaling`.
			{ "scaleMode", "scaling" },
	},
	// enumMap.
	{
		{
			"endMode",
			{
				LSShortEnum(),
				LSEnum
				({
					{ "stop", LSMovieEndMode::Stop },
					{ "play", LSMovieEndMode::KeepPlaying },
					{ "loop", LSMovieEndMode::Loop },
					{ "repeatEvents", LSMovieEndMode::RepeatEvents },
					// Aliases.
					{ "keepPlaying", LSMovieEndMode::KeepPlaying },
					{ "repeat", LSMovieEndMode::Loop },
					{ "loopEvents", LSMovieEndMode::RepeatEvents },
				}, LSMovieEndMode::Stop)
			}
		},
		{
			"scaling",
			{
				LSShortEnum(),
				LSEnum
				({
					{ "exactFix", SystemState::SCALE_MODE::EXACT_FIT },
					{ "noBorder", SystemState::SCALE_MODE::NO_BORDER },
					{ "noScale", SystemState::SCALE_MODE::NO_SCALE },
					{ "showAll", SystemState::SCALE_MODE::SHOW_ALL },
				}, SystemState::SCALE_MODE::SHOW_ALL)
			}
		},
	}
};

const LSMemberInfo LSMovieParser::frameInfo
{
	// memberMap.
	{
		// `Mouse` event members.
		MEMBER(EventMemberData, pos),
		MEMBER(EventMemberData, button),
		MEMBER(EventMemberData, buttonType),
		MEMBER(EventMemberData, clicks),
		MEMBER(EventMemberData, delta),
		// `Key` event members.
		MEMBER(EventMemberData, keys),
		// Single entry version of `keys`.
		MEMBER_NAME_LONG(EventMemberData, keys, key, true),
		MEMBER(EventMemberData, modifiers),
		// `Text` event members.
		MEMBER(EventMemberData, text),
		// `Window` event members.
		MEMBER(EventMemberData, size),
		MEMBER(EventMemberData, focusType),
		MEMBER(EventMemberData, focused),
		// `ContextMenu` event members.
		MEMBER(EventMemberData, item),
	},
	// memberAliases.
	{
			// Aliases.
			// `clicks`.
			{ "numClicks", "clicks" },
			// `key`.
			{ "keyCode", "key" },
			{ "keys", "key" },
			// `keys`.
			{ "keyCodes", "keys" },
			// `text`.
			{ "textData", "text" },
			// `item`.
			{ "selectedItem", "item" },
	},
	// enumMap.
	{
		{
			"button",
			{
				// shortTypes.
				LSShortEnum
				({
					{ 'l', LSButton::Left },
					{ 'm', LSButton::Middle },
					{ 'r', LSButton::Right },
				}),
				// longTypes.
				LSEnum
				({
					{ "Left", LSButton::Left },
					{ "Middle", LSButton::Middle },
					{ "Right", LSButton::Right },
				})
			}
		},
		{
			"modifiers",
			{
				// shortTypes.
				LSShortEnum
				({
					{ 'n', LSModifier::None },
					{ 'c', LSModifier::Ctrl },
					{ 's', LSModifier::Shift },
					{ 'a', LSModifier::Alt },
					{ 'S', LSModifier::Super },
				}),
				// longTypes.
				LSEnum
				({
					{ "None", LSModifier::None },
					{ "Ctrl", LSModifier::Ctrl },
					{ "Shift", LSModifier::Shift },
					{ "Alt", LSModifier::Alt },
					{ "Super", LSModifier::Super },
				})
			}
		},
		{
			"buttonType",
			{
				// shortTypes.
				LSShortEnum
				({
					{ 'u', LSButtonType::Up },
					{ 'd', LSButtonType::Down },
					{ 'c', LSButtonType::Click },
					{ 'D', LSButtonType::DoubleClick },
				}),
				// longTypes.
				LSEnum
				({
					{ "Up", LSButtonType::Up },
					{ "Down", LSButtonType::Down },
					{ "Click", LSButtonType::Click },
					{ "DoubleClick", LSButtonType::DoubleClick },
				})
			}
		},
		{
			"focusType",
			{
				// shortTypes.
				LSShortEnum
				({
					{ 'k', LSFocusType::Keyboard },
					{ 'm', LSFocusType::Mouse },
				}),
				// longTypes.
				LSEnum
				({
					{ "Keyboard", LSFocusType::Keyboard },
					{ "Mouse", LSFocusType::Mouse },
					// Alises.
					{ "keyboard", LSFocusType::Keyboard },
					{ "mouse", LSFocusType::Mouse },
				})
			}
		},
		{
			"key",
			{
				LSShortEnum(),
				LSEnum
				({
				 	{ "MouseLeft", AS3KEYCODE_MOUSE_LEFT },
					{ "MouseRight", AS3KEYCODE_MOUSE_RIGHT },
					{ "MouseMiddle", AS3KEYCODE_MOUSE_MIDDLE },
					{ "Backspace", AS3KEYCODE_BACKSPACE },
					{ "Tab", AS3KEYCODE_TAB },
					{ "Return", AS3KEYCODE_ENTER },
					{ "Command", AS3KEYCODE_COMMAND },
					{ "Shift", AS3KEYCODE_SHIFT },
					{ "Ctrl", AS3KEYCODE_CONTROL },
					{ "Alt", AS3KEYCODE_ALTERNATE },
					{ "Pause", AS3KEYCODE_PAUSE },
					{ "CapsLock", AS3KEYCODE_CAPS_LOCK },
					{ "Numpad", AS3KEYCODE_NUMPAD },
					{ "Escape", AS3KEYCODE_ESCAPE },
					{ "Space", AS3KEYCODE_SPACE },
					{ "PgUp", AS3KEYCODE_PAGE_UP },
					{ "PgDown", AS3KEYCODE_PAGE_DOWN },
					{ "End", AS3KEYCODE_END },
					{ "Home", AS3KEYCODE_HOME },
					{ "Left", AS3KEYCODE_LEFT },
					{ "Up", AS3KEYCODE_UP },
					{ "Right", AS3KEYCODE_RIGHT },
					{ "Down", AS3KEYCODE_DOWN },
					{ "Insert", AS3KEYCODE_INSERT },
					{ "Delete", AS3KEYCODE_DELETE },
					{ "Numpad0", AS3KEYCODE_NUMPAD_0 },
					{ "Numpad1", AS3KEYCODE_NUMPAD_1 },
					{ "Numpad2", AS3KEYCODE_NUMPAD_2 },
					{ "Numpad3", AS3KEYCODE_NUMPAD_3 },
					{ "Numpad4", AS3KEYCODE_NUMPAD_4 },
					{ "Numpad5", AS3KEYCODE_NUMPAD_5 },
					{ "Numpad6", AS3KEYCODE_NUMPAD_6 },
					{ "Numpad7", AS3KEYCODE_NUMPAD_7 },
					{ "Numpad8", AS3KEYCODE_NUMPAD_8 },
					{ "Numpad9", AS3KEYCODE_NUMPAD_9 },
					{ "Multiply", AS3KEYCODE_NUMPAD_MULTIPLY },
					{ "Plus", AS3KEYCODE_NUMPAD_ADD },
					{ "NumpadEnter", AS3KEYCODE_NUMPAD_ENTER },
					{ "NumpadMinus", AS3KEYCODE_NUMPAD_SUBTRACT },
					{ "NumpadPeriod", AS3KEYCODE_NUMPAD_DECIMAL },
					{ "NumpadSlash", AS3KEYCODE_NUMPAD_DIVIDE },
					{ "F1", AS3KEYCODE_F1 },
					{ "F2", AS3KEYCODE_F2 },
					{ "F3", AS3KEYCODE_F3 },
					{ "F4", AS3KEYCODE_F4 },
					{ "F5", AS3KEYCODE_F5 },
					{ "F6", AS3KEYCODE_F6 },
					{ "F7", AS3KEYCODE_F7 },
					{ "F8", AS3KEYCODE_F8 },
					{ "F9", AS3KEYCODE_F9 },
					{ "F10", AS3KEYCODE_F10 },
					{ "F11", AS3KEYCODE_F11 },
					{ "F12", AS3KEYCODE_F12 },
					{ "F13", AS3KEYCODE_F13 },
					{ "F14", AS3KEYCODE_F14 },
					{ "F15", AS3KEYCODE_F15 },
					{ "F16", AS3KEYCODE_F16 },
					{ "F17", AS3KEYCODE_F17 },
					{ "F18", AS3KEYCODE_F18 },
					{ "F19", AS3KEYCODE_F19 },
					{ "F20", AS3KEYCODE_F20 },
					{ "F21", AS3KEYCODE_F21 },
					{ "F22", AS3KEYCODE_F22 },
					{ "F23", AS3KEYCODE_F23 },
					{ "F24", AS3KEYCODE_F24 },
					{ "NumLock", AS3KEYCODE_NUM_LOCK },
					{ "ScrollLock", AS3KEYCODE_SCROLL_LOCK },
					// Aliases.
					{ "LeftArrow", AS3KEYCODE_LEFT },
					{ "UpArrow", AS3KEYCODE_UP },
					{ "RightArrow", AS3KEYCODE_RIGHT },
					{ "DownArrow", AS3KEYCODE_DOWN },
					{ "Ins", AS3KEYCODE_INSERT },
					{ "Del", AS3KEYCODE_DELETE },
					{ "NumpadMultiply", AS3KEYCODE_NUMPAD_MULTIPLY },
					{ "NumpadPlus", AS3KEYCODE_NUMPAD_ADD },
				})
			}
		},
	}
};

const std::vector<LSMemberTypeInfo> LSMovieParser::frameTypeInfo
{
	// `Mouse` event members.
	TYPE_INFO_MEMBER(EventMemberData, pos, "Mouse", "WindowMove"),
	TYPE_INFO_MEMBER(EventMemberData, buttonType, "MouseButton"),
	TYPE_INFO_MEMBER(EventMemberData, button, "MouseButton", "MouseUp", "MouseDown", "MouseClick", "MouseDoubleClick"),
	TYPE_INFO_MEMBER(EventMemberData, clicks, "MouseButton", "MouseUp", "MouseDown", "MouseClick", "MouseDoubleClick"),
	TYPE_INFO_MEMBER(EventMemberData, delta, "MouseWheel"),
	// `Key` event members.
	TYPE_INFO_MEMBER(EventMemberData, keys, "Key"),
	// Single entry version of `keys`.
	TYPE_INFO(AS3KeyCode, key, "Key"),
	TYPE_INFO_MEMBER(EventMemberData, modifiers, "Mouse", "Key"),
	// `Text` event members.
	TYPE_INFO_MEMBER(EventMemberData, text, "Text"),
	// `Window` event members.
	TYPE_INFO_MEMBER(EventMemberData, size, "WindowResize"),
	TYPE_INFO_MEMBER(EventMemberData, focusType, "WindowFocus"),
	TYPE_INFO_MEMBER(EventMemberData, focused, "WindowFocus"),
	// `ContextMenu` event members.
	TYPE_INFO_MEMBER(EventMemberData, item, "ContextMenu"),
};

template<typename T>
static bool contains(const T& arr, const typename T::value_type& val)
{
	return std::find(arr.begin(), arr.end(), val) != arr.end();
}

static bool isLSMDirective(const LSToken& token)
{
	using TokenType = LSToken::Type;
	return token.type == TokenType::Directive && token.dir.name == "lsm";
}

static bool isLSMovieFile(const tiny_string& path, const LSToken& token)
{
	return path.find(".lsm") != tiny_string::npos || isLSMDirective(token);
}

template<typename T>
static void addType(std::list<T>& list, const Optional<T>& type)
{
	(void)type.andThen([&](const T& type) -> Optional<T>
	{
		if (contains(list, type))
		{
			throw LSMovieParserException
			(
				"addType: Can't have more than one instance of the same type."
			);
		}
		list.push_back(type);
		return type;
	}).orElse([&]
	{
		throw LSMovieParserException("addType: Got invalid type.");
		return nullOpt;
	});
}

template<typename T>
static Optional<T> parseTypeAssign(const Expr& expr, const LSEnum& _enum)
{
	Optional<T> ret;
	parseAssignment(expr, [&](const tiny_string& name, const Expr& expr)
	{
		if (name == "type" && expr.isIdent())
			ret = _enum.tryGetValue<T>(expr.ident);
	});
	return ret;
}

template<typename T>
static std::list<T> parseLongTypes(const LSEnum& _enum, tiny_string str)
{
	std::list<T> list;
	while (!str.empty())
		addType(list, _enum.tryGetValueStripPrefix<T>(str));
	return list;
}

template<typename T>
static std::list<T> parseShortTypes
(
	const LSShortEnum& shortTypes,
	const tiny_string& str,
	size_t start
)
{
	std::list<T> list;
	for (size_t i = start; i < str.numChars(); ++i)
		addType(list, shortTypes.tryGetValue<T>(str[i]));
	return list;
}

struct OpListElem
{
	Expr::OpType type;
	Optional<const Expr&> expr;
};

using OpList = std::list<OpListElem>;

static void makeOpListImpl(OpList& list, const Expr& expr, const Optional<Expr::OpType>& type = {})
{
	if (expr.isOp())
	{
		const auto& op = expr.op;
		if (op.lhs != nullptr)
			makeOpListImpl(list, *op.lhs, op.type);
		if (op.rhs != nullptr)
			makeOpListImpl(list, *op.rhs, op.type);
		return;
	}

	(void)type.andThen([&](const Expr::OpType& type) -> Optional<Expr::OpType>
	{
		list.push_back(OpListElem { type, expr });
		return nullOpt;
	});
}

static OpList makeOpList(const Expr& expr)
{
	assert(expr.isOp());

	OpList list;
	makeOpListImpl(list, expr);
	return list;
}

static std::list<AS3KeyCode> parseKeys(const LSMemberInfo& memberInfo, const tiny_string& name, const LSToken::Expr& expr)
{
	using OpType = LSToken::Expr::OpType;

	if (!expr.isList() && !expr.isOp() && !expr.isString())
		throw LSMovieParserException("Keys: Must be either a list, operator, or string.");

	// TODO: Use expression evaluation, once proper support for list
	// evaluation is implemented.
	std::list<AS3KeyCode> keys;
	if (expr.isList())
	{
		// Got a list.
		for (auto it : expr.value.list)
			keys.push_back(parseKey(memberInfo, name, it));
	}
	else if (expr.isOp())
	{
		// Got an operator.
		auto opList = makeOpList(expr);
		for (auto it : opList)
		{
			if (it.type != OpType::Plus)
				throw LSMovieParserException("Keys: Operator expressions must only contain `+`.");
			(void)it.expr.andThen([&](const Expr& expr) -> Optional<const Expr&>
			{
				keys.push_back(parseKey(memberInfo, name, expr));
				return nullOpt;
			});
		}
	}
	else
	{
		// Got a string.
		for (auto ch : expr.value.str)
			keys.push_back(parseKey(memberInfo, name, Expr::Value(ch)));
	}
	return keys;
}

static AS3KeyCode parseKey(const LSMemberInfo& memberInfo, const tiny_string& name, const LSToken::Expr& expr)
{
	using Expr = LSToken::Expr;
	using ExprList = std::list<Expr>;
	using EvalResult = Expr::EvalResult<AS3KeyCode>;
	using EvalResultType = EvalResult::Type;
	using GroupIterPair = Expr::GroupIterPair;

	auto _enum = memberInfo.getEnum(name);

	auto keyEvalVisitor = makeVisitor
	(
		[&]
		(
			const ExprList& list,
			const Expr&,
			Optional<GroupIterPair>,
			Optional<size_t>,
			size_t
		) -> EvalResult
		{
			return typename EvalResult::Error
			{
				tiny_string("Key: Lists aren't allowed in the single entry version.", true)
			};
		},
		[&]
		(
			uint32_t ch,
			const Expr&,
			Optional<GroupIterPair>,
			Optional<size_t>,
			size_t
		) -> EvalResult
		{
			if (isalnum(ch))
				return AS3KeyCode(ch);
			// TODO: Support shifted characters.
			switch (ch)
			{
				case ' ': return AS3KEYCODE_SPACE; break;
				case '=': return AS3KEYCODE_EQUAL; break;
				case '-': return AS3KEYCODE_MINUS; break;
				case '/': return AS3KEYCODE_SLASH; break;
				case '[': return AS3KEYCODE_LEFTBRACKET; break;
				case ']': return AS3KEYCODE_RIGHTBRACKET; break;
				case ',': return AS3KEYCODE_COMMA; break;
				case '.': return AS3KEYCODE_PERIOD; break;
				case '`': return AS3KEYCODE_BACKQUOTE; break;
				case ';': return AS3KEYCODE_SEMICOLON; break;
				case '\\': return AS3KEYCODE_BACKSLASH; break;
				case '\'': return AS3KEYCODE_QUOTE; break;
				default: break;
			}

			std::stringstream s;
			s << "Key: Character '" << tiny_string::fromChar(ch) <<
			"' isn't a valid keycode.";
			return typename EvalResult::Error
			{
				tiny_string(s.str())
			};
		},
		[&]
		(
			const tiny_string& ident,
			bool isString,
			const Expr&,
			Optional<GroupIterPair>,
			Optional<size_t>,
			size_t
		) -> EvalResult
		{
			if (isString)
			{
				return typename EvalResult::Error
				{
					tiny_string("Key: Strings aren't allowed in the single entry version.", true)
				};
			}

			return _enum.tryGetValue<AS3KeyCode>(ident).orElse([&]
			{
				if (ident.numChars() == 1 && isalpha(ident[0]))
					return AS3KeyCode(ident[0]);

				std::stringstream s;
				s << "Key: `" << ident << "` isn't a valid keycode.";
				throw LSMovieParserException(s.str());
				return AS3KEYCODE_UNKNOWN;
			}).getValue();
		},
		[&](const Expr::Value& value, const Expr&, Optional<GroupIterPair>, Optional<size_t>, size_t)
		{
			if (!value.isChar() && !value.isString() && !value.isList())
				return EvalResultType::Invalid;
			return EvalResultType::NoOverride;
		},
		[&](const Expr& expr, const Expr&, Optional<GroupIterPair>, Optional<size_t>, size_t)
		{
			if (!expr.isIdent() && !expr.isValue())
				return EvalResultType::Invalid;
			return EvalResultType::NoOverride;
		}
	);

	return expr.eval<AS3KeyCode>(keyEvalVisitor);
}

static LSModifier parseModifier(const LSMemberInfo& memberInfo, const tiny_string& name, const LSToken::Expr& expr)
{
	using Expr = LSToken::Expr;
	using ExprList = std::list<Expr>;
	using EvalResult = Expr::EvalResult<LSModifier>;
	using EvalResultType = EvalResult::Type;
	using GroupIterPair = Expr::GroupIterPair;
	using OpType = Expr::OpType;

	auto _enum = memberInfo.getEnum(name);
	auto shortEnum = memberInfo.getShortEnum(name);

	bool isFirst = true;
	auto modifierEvalVisitor = makeVisitor
	(
		[&]
		(
			const ExprList& list,
			const Expr&,
			Optional<GroupIterPair>,
			Optional<size_t>,
			size_t depth
		) -> EvalResult
		{
			if (depth > 0)
			{
				return typename EvalResult::Error
				{
					tiny_string("Modifier: Recursive lists aren't allowed.", true)
				};
			}
			return EvalResultType::NoOverride;
		},
		[&]
		(
			const tiny_string& ident,
			bool isString,
			const Expr& parent,
			Optional<GroupIterPair>,
			Optional<size_t> listIndex,
			size_t
		) -> EvalResult
		{
			if (isString)
			{
				return typename EvalResult::Error
				{
					tiny_string("Modifier: Strings aren't allowed.", true)
				};
			}

			auto modifier =
			(
				ident.numChars() == 1 ?
				shortEnum.tryGetValue<LSModifier>(ident[0]) :
				_enum.tryGetValue<LSModifier>(ident)
			).orElse([&]
			{
				std::stringstream s;
				s << "Modifier: `" << ident << "` isn't a valid modifier.";
				throw LSMovieParserException(s.str());
				return LSModifier::None;
			}).getValue();

			if (parent.isOp() && parent.op.type != Expr::OpType::Plus)
			{
				return typename EvalResult::Error
				{
					tiny_string("Modifier: Operator expressions must only contain `+`.", true)
				};
			}

			if (isFirst || !listIndex.valueOr(0))
			{
				isFirst = false;
				return modifier;
			}

			return typename EvalResult::Op(OpType::BitOr, modifier);
		},
		[&](const Expr::Value& value, const Expr&, Optional<GroupIterPair>, Optional<size_t>, size_t)
		{
			if (!value.isList() && !value.isString())
				return EvalResultType::Invalid;
			return EvalResultType::NoOverride;
		},
		[&](const Expr& expr, const Expr&, Optional<GroupIterPair>, Optional<size_t>, size_t)
		{
			if (!expr.isIdent() && !expr.isValue())
				return EvalResultType::Invalid;
			return EvalResultType::NoOverride;
		}
	);
	return expr.eval<LSModifier>(modifierEvalVisitor);
}

template<typename T, typename F, typename F2, typename F3>
void LSMovieParser::parseEvent
(
	const tiny_string& name,
	const Block& block,
	const tiny_string& eventName,
	Optional<const LSEnumPair&> eventTypes,
	F&& getSubTypeName,
	F2&& memberCount,
	F3&& createEvent
)
{
	using ListIter = typename std::list<T>::const_iterator;
	assert(name[0] == eventName[0]);
	auto throwError = [&](const tiny_string& reason)
	{
		std::stringstream s;
		s << eventName << " event: " << reason;
		throw LSMovieParserException(s.str());
	};

	auto getSubType = [&](const LSEnum& _enum, const tiny_string& name)
	{
		return *_enum.tryGetValue<T>(name).orElse([&]
		{
			std::stringstream s;
			s << '`' << name << "` isn't a valid sub-type.";
			throwError(s.str());
			return nullOpt;
		});
	};

	size_t i = 0;
	size_t numMembers = 0;

	MemberSet validMembers;
	MemberSet assignedMembers;
	auto checkAssignedMembers = [&](ListIter& it, const tiny_string& name)
	{
		auto _it = assignedMembers.find(name);
		if (_it == assignedMembers.end() && i < numMembers)
			return;

		for (auto _it = assignedMembers.begin(); _it != assignedMembers.end();)
		{
			if (validMembers.find(*_it) != validMembers.end())
				_it = assignedMembers.erase(_it);
			else
				++_it;
		}

		T type = eventTypes.hasValue() ? *it++ : T {};
		movie.events.push_back(createEvent(type));
	};

	auto setMember = [&](ListIter& it, const tiny_string& name, const Expr& expr)
	{
		checkAssignedMembers(it, name);
		frameInfo.getMember(name).set
		(
			&eventMembers,
			frameInfo,
			name,
			expr
		);

		assignedMembers.insert(name);
	};

	bool isLongName = name.startsWith(eventName);
	if (isLongName && inSimpleMode())
		throwError("Only short form names are allowed in simple mode.");

	std::list<T> types = eventTypes.transformOr(std::list<T> {}, [&](const LSEnumPair& eventTypes)
	{
		const auto& longTypes = eventTypes._enum;
		const auto& shortTypes = eventTypes.shortEnum;
		if (!isLongName)
			return parseShortTypes<T>(shortTypes, name, 1);
		return parseLongTypes<T>(longTypes, name.stripPrefix(eventName));
	});

	bool skipFirstToken = false;
	(void)eventTypes.filter
	(
		types.empty()
	).andThen([&](const LSEnumPair& eventTypes) -> Optional<LSEnumPair>
	{
		skipFirstToken = true;
		if (!block.front().isExpr())
			throwError("First member must be an expression.");

		const auto& expr = block.front().expr;
		const auto& _enum = eventTypes._enum;
		if (expr.isIdent())
		{
			types.push_back(getSubType(_enum, expr.ident));
			return {};
		}

		parseAssignment(expr, [&](const tiny_string& name, const Expr& expr)
		{
			if (name != "type")
			{
				std::stringstream s;
				s << "First member must be `type`, got `" <<
				name << "` instead.";
				throwError(s.str());
			}
			if (!expr.isIdent())
				throwError("Value of `type` must be an identifier.");

			types.push_back(getSubType(_enum, expr.ident));
		}, true);
		return {};
	});

	tiny_string subTypeName;
	TypeInfoTable typeInfoTable;

	auto resetMemberData = [&](const ListIter& it)
	{
		T type = eventTypes.hasValue() ? *it : T {};
		subTypeName = getSubTypeName(type);

		for (auto it : frameTypeInfo)
		{
			if (it.isMemberOf(eventName, subTypeName))
			{
				validMembers.insert(it.getName());
				typeInfoTable.push_back(it);
			}
		}

		numMembers = memberCount(type).valueOr(validMembers.size());
	};

	bool wasReset = false;
	auto it = types.cbegin();
	auto prevIt = types.cend();
	resetMemberData(it);

	parseBlock(block, [&](BlockIter& token)
	{
		if (skipFirstToken)
		{
			skipFirstToken = false;
			return;
		}
		if (!token->isExpr())
			throwError("Must only contain expressions.");

		auto expr = token->expr;

		if (prevIt == it)
			checkAssignedMembers(it, "");

		if (it == types.cend())
			it = types.cbegin();

		if (i == numMembers)
		{
			wasReset = true;
			i = 0;
		}

		if (!types.empty() && prevIt != it)
		{
			// Reset all member related stuff upon switching sub-types.
			validMembers.clear();
			typeInfoTable.clear();
			resetMemberData(it);
		}

		prevIt = it;

		if (!expr.isAssign())
		{
			// Filter for members that can be set by `expr`.
			TypeInfoTable infoTable;
			for (size_t j = 0; j < typeInfoTable.size(); ++j)
			{
				auto info = typeInfoTable[j];
				if (info.isValid(frameInfo, expr))
					infoTable.push_back(info);
			}

			if (infoTable.empty())
			{
				// Got no members, try the next sub-type.
				i = numMembers;
				checkAssignedMembers(it, "");
				if (it == types.cend())
					throwError("Couldn't find a valid type for the given expression.");
				token--;
				return;
			}
			setMember(it, infoTable[(i + 1) % infoTable.size()].getName(), expr);
			++i;
			return;
		}

		parseAssignment(expr, [&](const tiny_string& name, const Expr expr)
		{
			bool isAlias = frameInfo.isAlias(name);
			if (validMembers.find(name) == validMembers.end() && !isAlias)
			{
				std::stringstream s;
				s << '`' << name <<
				"` isn't a valid member of type `" <<
				eventName << subTypeName << "`.";
				throwError(s.str());
			}

			setMember(it, name, expr);
		});
		++i;
	});

	if (it == types.cend())
		it = types.cbegin();

	if (i >= numMembers - 1 || wasReset || block.empty())
	{
		for (; it != types.cend(); ++it)
			movie.events.push_back(createEvent(eventTypes.hasValue() ? *it : T {}));
	}
}

void LSMovieParser::parseMouseEvent(const tiny_string& name, const Block& block)
{
	auto getSubTypeName = [&](const MouseType& type)
	{
		switch (type)
		{
			case MouseType::Move: return "Move"; break;
			case MouseType::Button: return "Button"; break;
			case MouseType::Wheel: return "Wheel"; break;
			case MouseType::Up: return "Up"; break;
			case MouseType::Down: return "Down"; break;
			case MouseType::Click: return "Click"; break;
			case MouseType::DoubleClick: return "DoubleClick"; break;
			default: return ""; break;
		}
	};

	auto toLSButtonType = [&](const MouseType& type)
	{
		switch (type)
		{
			case MouseType::Up: return LSButtonType::Up; break;
			case MouseType::Down: return LSButtonType::Down; break;
			case MouseType::Click: return LSButtonType::Click; break;
			case MouseType::DoubleClick: return LSButtonType::DoubleClick; break;
			default: break;
		}

		std::stringstream s;
		s << "Mouse event: `Mouse" << getSubTypeName(type) <<
		"` can't be converted to `LSButtonType`.";
		throw LSMovieParserException(s.str());
	};

	auto createEvent = [&](const MouseType& type) -> LSEventStorage
	{
		switch (type)
		{
			case MouseType::Move:
				return LSMouseMoveEvent
				(
					0,
					Vector2f(NAN, NAN),
					// Convert position from twips, to pixels.
					Vector2f(eventMembers.pos)/20,
					eventMembers.modifiers,
					false
				);
				break;
			case MouseType::Button:
			case MouseType::Up:
			case MouseType::Down:
			case MouseType::Click:
			case MouseType::DoubleClick:
			{
				auto buttonType =
				(
					type == MouseType::Button ?
					eventMembers.buttonType.orElse([&]
					{
						throw LSMovieParserException
						(
							"Mouse event: `buttonType` is required for `MouseButton` events."
						);
						return nullOpt;
					}).getValue() : toLSButtonType(type)
				);

				if (eventMembers.button == LSButton::Invalid)
				{
					throw LSMovieParserException
					(
						"Mouse event: `button` must be defined at least once."
					);
				}

				return LSMouseButtonEvent
				(
					0,
					Vector2f(NAN, NAN),
					// Convert position from twips, to pixels.
					Vector2f(eventMembers.pos)/20,
					eventMembers.modifiers,
					buttonType == LSButtonType::Down,
					eventMembers.button,
					eventMembers.clicks,
					buttonType
				);
				break;
			}
			case MouseType::Wheel:
				if (!eventMembers.delta)
				{
					throw LSMovieParserException
					(
						"Mouse event: `delta` can't be 0."
					);
				}

				return LSMouseWheelEvent
				(
					0,
					Vector2f(NAN, NAN),
					// Convert position from twips, to pixels.
					Vector2f(eventMembers.pos)/20,
					eventMembers.modifiers,
					false,
					eventMembers.delta
				);
				break;
			default:
				throw LSMovieParserException("Mouse event: Somehow got an invalid `MouseType`.");
				break;
		}
	};

	parseEvent<MouseType>
	(
		name,
		block,
		"Mouse",
		mouseTypes,
		getSubTypeName,
		// `memberCount`.
		[&](const MouseType&) -> Optional<size_t> { return nullOpt; },
		createEvent
	);
}

void LSMovieParser::parseKeyEvent(const tiny_string& name, const Block& block)
{
	auto getSubTypeName = [&](const KeyType& type)
	{
		switch (type)
		{
			case KeyType::Up: return "Up"; break;
			case KeyType::Down: return "Down"; break;
			case KeyType::Press: return "Press"; break;
			case KeyType::Control: return "Control"; break;
			default: return ""; break;
		}
	};

	auto makeKeyEvent = [&](const AS3KeyCode& key, const KeyType& type)
	{
		return LSKeyEvent
		(
			Vector2f(NAN, NAN),
			// Convert position from twips, to pixels.
			Vector2f(eventMembers.pos)/20,
			key,
			key,
			eventMembers.modifiers,
			type
		);
	};

	auto createEvent = [&](const KeyType& type) -> LSEventStorage
	{
		const auto& keys = eventMembers.keys;
		auto end = std::prev(keys.end());
		for (auto it = keys.begin(); it != end; ++it)
			movie.events.push_back(makeKeyEvent(*it, type));
		return makeKeyEvent(keys.back(), type);
	};

	parseEvent<KeyType>
	(
		name,
		block,
		"Key",
		keyTypes,
		getSubTypeName,
		// `memberCount`.
		[&](const KeyType&) -> Optional<size_t> { return 2; },
		createEvent
	);
}

void LSMovieParser::parseTextEvent(const tiny_string& name, const Block& block)
{
	auto getSubTypeName = [&](const TextType& type)
	{
		switch (type)
		{
			case TextType::Input: return "Input"; break;
			case TextType::Clipboard: return "Clipboard"; break;
			default: return ""; break;
		}
	};

	auto createEvent = [&](const TextType& type)
	{
		if (eventMembers.text.empty())
			throw LSMovieParserException("Text event: `text` can't be empty.");
		return LSTextEvent(eventMembers.text, type);
	};

	parseEvent<TextType>
	(
		name,
		block,
		"Text",
		textTypes,
		getSubTypeName,
		// `memberCount`.
		[&](const TextType&) -> Optional<size_t> { return nullOpt; },
		createEvent
	);
}

void LSMovieParser::parseWindowEvent(const tiny_string& name, const Block& block)
{
	auto getSubTypeName = [&](const WindowType& type)
	{
		switch (type)
		{
			case WindowType::Resize: return "Resize"; break;
			case WindowType::Move: return "Move"; break;
			case WindowType::Focus: return "Focus"; break;
			default: return ""; break;
		}
	};

	auto createEvent = [&](const WindowType& type) -> LSEventStorage
	{
		switch (type)
		{
			case WindowType::Resize:
			{
				// Convert size from twips, to pixels.
				auto size = Vector2f(eventMembers.size)/20;
				if (size.x == 0 || size.y == 0)
				{
					std::stringstream s;
					s << "Window event: `size` can't have a width, or height of 0, got `" <<
					size.x << "px x " << size.y << "px` instead.";
					throw LSMovieParserException(s.str());
				}
				return LSWindowResizedEvent(size);
				break;
			}
			case WindowType::Move:
				return LSWindowMovedEvent
				(
					// Convert position from twips, to pixels.
					Vector2f(eventMembers.pos)/20
				);
				break;
			case WindowType::Focus:
				return LSWindowFocusEvent
				(
					eventMembers.focusType.orElse([&]
					{
						throw LSMovieParserException
						(
							"Window event: `focusType` is required for `WindowFocus` events"
						);
						return nullOpt;
					}).getValue(),
					eventMembers.focused
				);
				break;
		}
		return LSEvent();
	};

	parseEvent<WindowType>
	(
		name,
		block,
		"Window",
		windowTypes,
		getSubTypeName,
		// `memberCount`.
		[&](const WindowType&) -> Optional<size_t> { return {}; },
		createEvent
	);
}

void LSMovieParser::parseContextMenuEvent(const tiny_string& name, const Block& block)
{
	auto getSubTypeName = [&](const int&) { return ""; };

	auto createEvent = [&](const int&)
	{
		if (eventMembers.item < 0)
		{
			throw LSMovieParserException
			(
				"ContextMenu event: `item` can't be negative."
			);
		}
		// TODO: Uncomment this, once support for targetless context menu
		// selection has been added.
		//return LSContextMenuSelectionEvent(eventMembers.item);
		return LSEvent();
	};

	parseEvent<int>
	(
		name,
		block,
		"ContextMenu",
		{},
		getSubTypeName,
		// `memberCount`.
		[&](const int&) -> Optional<size_t> { return nullOpt; },
		createEvent
	);
}

void LSMovieParser::parseDirective(BlockIter& it)
{
	assert(it->isDirective());
	auto dir = it->dir;
	// The `#lsm` directive, is a magic number, so ignore it.
	if (dir.name == "lsm")
		return;
	if (dir.name == "version")
	{
		if (!(++it)->isNumericExpr())
		{
			std::stringstream s;
			s << "Directive: `#version` requires a numeric expression.";
			throw LSMovieParserException(s.str());
		}
		// Got a `#version` directive, so set the movie version.
		movie.version = it->expr.eval<size_t>();
	}
	else if (dir.name == "simple")
	{
		// Got a `#simple` directive, so go into simplified
		// parsing mode.
		parsingMode = Mode::Simple;
	}
	else if (dir.name == "normal")
	{
		// Got a `#normal` directive, so go back to normal
		// parsing.
		parsingMode = Mode::Normal;
	}
	else
	{
		std::stringstream s;
		s << "Directive: `#" << dir.name << "` isn't a valid directive.";
		throw LSMovieParserException(s.str());
	}
}

void LSMovieParser::parseExpr(const Expr& expr)
{
	if (!expr.isGroup() && !expr.isBlock())
		throw LSMovieParserException("Expr: Must only contain groups, or blocks.");

	if (expr.isGroup())
	{
		size_t repeatCount = 0;
		// Got a group.
		auto it = expr.group.list.begin();

		if (it->isNumericExpr())
		{
			auto count = (it++)->eval<ssize_t>();
			if (count <= 0)
			{
				std::stringstream s;
				s << "Expr: Repeat count must be bigger than 0, got " <<
				count << " instead.";
				throw LSMovieParserException(s.str());
			}

			// The first element is numeric, set the repeat count.
			repeatCount = count - 1;
		}

		if (!it->isIdent())
			throw LSMovieParserException("Expr: Group must contain a nameless block, or identifier-block pair.");

		auto ident = it->ident;
		if (ident != "header" && ident != "frame")
		{
			std::stringstream s;
			s << "Expr: `" << ident << "` isn't a valid statement.";
			throw LSMovieParserException(s.str());
		}

		if (!(++it)->isBlock())
		{
			std::stringstream s;
			s << "Expr: `" << ident << "` statement has no block after it.";
			throw LSMovieParserException(s.str());
		}

		if (ident == "header")
		{
			// Parse the header.
			parseHeader(it->block);
		}
		else
		{
			// Parse the events for this frame.
			parseEvents(it->block, repeatCount);
		}
	}
	else
	{
		// Got a nameless block, treat it as a `frame` statement.
		parseEvents(expr.block);
	}
}

void LSMovieParser::parseHeader(const Block& block)
{
	parseAssignBlock(block, [&](const tiny_string& name, const LSToken::Expr& expr)
	{
		headerInfo.getMember(name).set(&movie.header, headerInfo, name, expr);
	}, true);
}

void LSMovieParser::parseEvents(const Block& block, size_t repeatCount, size_t depth)
{
	bool hasInnerFrames = false;
	auto handleInnerFrame = [&](const Expr& expr, size_t innerRepeatCount = 0)
	{
		assert(expr.isBlock());
		hasInnerFrames = true;
		parseEvents(expr.block, innerRepeatCount, depth + 1);
	};

	if (depth > 1)
	{
		std::stringstream s;
		s << "Frame: Can't have more than a single layer of inner frames, got " << depth << " instead.";
		throw LSMovieParserException(s.str());
	}

	if (!movie.events.empty())
		movie.frameEventIndices.push_back(movie.events.size());

	parseBlock(block, [&](BlockIter& token)
	{
		if (!token->isDirective() && !token->isGroup() && !token->isBlock())
			throw LSMovieParserException("Frame: Must only contain directives, groups, or blocks.");

		if (token->isDirective())
		{
			auto name = token->dir.name;
			if (name == "lsm" || name == "version")
				throw LSMovieParserException("Frame: `#lsm`, and `#version` aren't allowed in `frame` statements.");
			parseDirective(token);
			return;
		}
		else if (token->isGroup())
		{
			size_t innerRepeatCount = 0;
			// Got a group.
			auto it = token->expr.group.list.begin();
			if (it->isNumericExpr())
			{
				auto count = (it++)->eval<ssize_t>();
				if (count <= 0)
				{
					std::stringstream s;
					s << "Expr: Repeat count must be bigger than 0, got " <<
					count << " instead.";
					throw LSMovieParserException(s.str());
				}

				// The first element is numeric, set the repeat count.
				innerRepeatCount = count - 1;
			}
			
			if (it->isBlock())
			{
				// Nameless blocks are treated as `frame` statements.
				handleInnerFrame(*it, innerRepeatCount);
				return;
			}

			if (!it->isIdent())
				throw LSMovieParserException("Frame: Group must contain a nameless block, or identifier-block pair.");
				
			if (!std::next(it)->isBlock())
			{
				std::stringstream s;
				s << "Frame: No block after identifier `" << it->ident << "`.";
				throw LSMovieParserException(s.str());
			}

			// Got an identifier, check which one.
			auto name = (it++)->ident;

			if (name == "frame")
			{
				// Got a `frame` statement.
				handleInnerFrame(*it, innerRepeatCount);
				return;
			}

			size_t repeatStart = movie.events.size();
			switch (name[0])
			{
				// `Mouse` event.
				case 'M': parseMouseEvent(name, it->block); break;
				// `Key` event.
				case 'K': parseKeyEvent(name, it->block); break;
				// `Text` event.
				case 'T': parseTextEvent(name, it->block); break;
				// `Window` event.
				case 'W': parseWindowEvent(name, it->block); break;
				// `ContextMenu` event.
				case 'C': parseContextMenuEvent(name, it->block); break;
				default:
				{
					std::stringstream s;
					s << "Frame: `" << name << "` isn't a valid identifier.";
					throw LSMovieParserException(s.str());
					break;
				}
			}

			LSMovie::EventList events
			(
				std::next(movie.events.begin(), repeatStart),
				movie.events.end()
			);
			for (size_t i = 0; i < innerRepeatCount; ++i)
			{
				movie.events.insert
				(
					movie.events.end(),
					events.begin(),
					events.end()
				);
			}
		}
		else
		{
			// Nameless blocks are treated as `frame` statements.
			handleInnerFrame(token->expr);
		}
	});

	eventMembers.clearNonPersistentMembers();

	if (!hasInnerFrames)
	{
		auto frame = movie.numEventFrames() - 1;
		auto frameEvents = *movie.getEventsForFrame(frame);
		for (size_t i = 0; i < repeatCount; ++i)
		{
			if (!movie.events.empty())
				movie.frameEventIndices.push_back(movie.events.size());
			movie.events.insert
			(
				movie.events.end(),
				frameEvents.begin(),
				frameEvents.end()
			);
		}
	}
}

static void removeComments(Block& tokens)
{
	auto searchExprList = [&](std::list<Expr>& list)
	{
		for (auto& elem : list)
		{
			if (elem.isBlock())
				removeComments(elem.block);
		}
	};

	for (auto it = tokens.begin(); it != tokens.end();)
	{
		if (it->isBlock())
			removeComments(it->expr.block);
		else if (it->isList())
			searchExprList(it->expr.value.list);
		else if (it->isGroup())
			searchExprList(it->expr.group.list);

		if (it->isComment())
		{
			// Found a comment, remove it.
			it = tokens.erase(it);
		}
		else
			it++;
	}
}

void LSMovieParser::parse(const tiny_string& path, Block tokens)
{
	if (!isLSMovieFile(path, tokens.front()))
	{
		std::stringstream s;
		if (!path.empty())
			s << '"' << path << "\" isn't a valid LSM file.";
		else
			s << "Supplied data is missing the starting `#lsm` directive.";
		throw LSMovieParserException(s.str());
	}

	// Remove the starting `#lsm` directive, if needed.
	if (isLSMDirective(tokens.front()))
		tokens.pop_front();

	// Also remove comments.
	removeComments(tokens);

	using TokenType = LSToken::Type;
	// Parse the tokens.
	parseBlock(tokens, [&](BlockIter& token)
	{
		switch (token->type)
		{
			case TokenType::Comment:
				// Skip over comments.
				break;
			case TokenType::Directive: parseDirective(token); break;
			case TokenType::Expr: parseExpr(token->expr); break;
		}
	});
}

LSMovieParser::LSMovieParser(const tiny_string& path) : parsingMode(Mode::Normal)
{
	parse(path, LSTokenParser().parseFile(path));
}
