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

#ifndef INPUT_FORMATS_LIGHTSPARK_LSM_PARSER_H
#define INPUT_FORMATS_LIGHTSPARK_LSM_PARSER_H 1

#include <exception>
#include <map>

#include <lightspark/events.h>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>

#include "input/formats/lightspark/lsm.h"

#include "utils/token_parser/enum.h"
#include "utils/token_parser/member.h"
#include "utils/token_parser/member_type_info.h"
#include "utils/token_parser/token_parser.h"

using namespace lightspark;

// TODO: Move this into the main codebase, once TASing support is added.

class LSMovieParserException : public std::exception
{
private:
	tiny_string reason;
public:
	LSMovieParserException(const tiny_string& _reason) : reason(_reason) {}

	const char* what() const noexcept override { return reason.raw_buf(); }
	const tiny_string& getReason() const noexcept { return reason; }
};


class LSMovieParser
{
using Block = std::list<LSToken>;
using BlockIter = Block::const_iterator;
using MemberSet = std::unordered_set<tiny_string>;
using TypeInfoTable = std::vector<LSMemberTypeInfo>;

using LSButton = LSMouseButtonEvent::Button;
using LSButtonType = LSMouseButtonEvent::ButtonType;
using LSFocusType = LSWindowFocusEvent::FocusType;
private:
	// Parsing mode.
	enum class Mode
	{
		Normal,
		Simple,
	};

	struct EventMemberData
	{
		// NOTE: A persistent member is a member that keeps it's last
		// set value across frames. This is mainly done to reduce the
		// file size, since only changes to these members can be saved,
		// rather than having to set them, every frame, even if the value
		// hasn't changed.

		// `Mouse` event member(s).
		// Persistent member(s): `pos`, `button`.
		//
		// NOTE: `pos` is also used by `WindowMove` events.
		Vector2 pos;
		// `Mouse{Button, Up, Down, Click}` event member(s).
		LSButton button { LSButton::Invalid };
		Optional<LSButtonType> buttonType;
		size_t clicks { 0 };
		// `MouseWheel` event member(s).
		number_t delta { 0 };

		// `Key` event members.
		// Persistent member(s): `keys`, `modifiers`.
		//
		// NOTE: `modifiers` is also used by `Mouse` events.
		std::list<AS3KeyCode> keys;
		LSModifier modifiers { LSModifier::None };

		// `Text` event members.
		// Has no persistent members.
		tiny_string text;

		// `Window` event members.
		// Has no persistent members.
		//
		// `WindowResize` event members.
		Vector2 size;
		// `WindowFocus` event members.
		Optional<LSFocusType> focusType;
		bool focused { false };

		// `ContextMenu` event members.
		// Has no persistent members.
		ssize_t item { -1 };

		void clearNonPersistentMembers()
		{
			buttonType = nullOpt;
			clicks = 0;
			delta = 0;
			text = tiny_string();
			size = Vector2();
			focusType = nullOpt;
			item = -1;
		}
	};

	// Member info for each statement.
	static const LSMemberInfo headerInfo;
	static const LSMemberInfo frameInfo;

	static const TypeInfoTable frameTypeInfo;

	Mode parsingMode { Mode::Normal };
	EventMemberData eventMembers;
	LSMovie movie;


	// Event parsing.
	template<typename T, typename F, typename F2, typename F3>
	void parseEvent
	(
		const tiny_string& name,
		const Block& block,
		const tiny_string& eventName,
		Optional<const LSEnumPair&> eventTypes,
		F&& getSubTypeName,
		F2&& memberCount,
		F3&& createEvent
	);
	void parseMouseEvent(const tiny_string& name, const Block& block);
	void parseKeyEvent(const tiny_string& name, const Block& block);
	void parseTextEvent(const tiny_string& name, const Block& block);
	void parseWindowEvent(const tiny_string& name, const Block& block);
	void parseContextMenuEvent(const tiny_string& name, const Block& block);

	void parseDirective(BlockIter& it);
	void parseExpr(const Expr& expr);
	void parseHeader(const Block& block);
	void parseEvents(const Block& block, size_t repeatCount = 0, size_t depth = 0);

	void parse(const tiny_string& path, Block tokens);

	bool inNormalMode() const { return parsingMode == Mode::Normal; }
	bool inSimpleMode() const { return parsingMode == Mode::Simple; }
public:
	LSMovieParser(const tiny_string& path);
	const LSMovie& getMovie() const { return movie; }
};

#endif /* INPUT_FORMATS_LIGHTSPARK_LSM_PARSER_H */
