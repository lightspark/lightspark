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

#ifndef UTILS_TOKEN_PARSER_MEMBER_H
#define UTILS_TOKEN_PARSER_MEMBER_H 1

#include <exception>
#include <list>
#include <map>

#include <lightspark/backends/geometry.h>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>

#include "utils/token_parser/enum.h"
#include "utils/token_parser/token.h"

using namespace lightspark;

namespace lightspark
{
	class TimeSpec;
};
class LSMember;

// TODO: Move this into the main codebase, at some point.

#define MEMBER_TYPE(type, member) decltype(std::declval<type>().member)

// Based on a version from SerenityOS. `https://github.com/SerenityOS/serenity/commit/af5fd99ff434529f5c8cc085bbd42483a3884781`.
// A version of `offsetof` that works with non-standard layout classes.
#define NSL_OFFSETOF(class, member) (reinterpret_cast<ptrdiff_t>(&reinterpret_cast<class*>(0)->member))

#define MEMBER(type, member) \
{ \
	#member, \
	LSMember::create<MEMBER_TYPE(type, member)> \
	( \
		NSL_OFFSETOF(type, member) \
	) \
}

#define MEMBER_LONG(type, member, ...) \
{ \
	#member, \
	LSMember::create<MEMBER_TYPE(type, member)> \
	( \
		NSL_OFFSETOF(type, member), \
		__VA_ARGS__ \
	) \
}

#define MEMBER_NAME(type, member, name) \
{ \
	#name, \
	LSMember::create<MEMBER_TYPE(type, member)>(NSL_OFFSETOF(type, member)) \
}

#define MEMBER_NAME_LONG(type, member, name, ...) \
{ \
	#name, \
	LSMember::create<MEMBER_TYPE(type, member)> \
	( \
		NSL_OFFSETOF(type, member), \
		__VA_ARGS__ \
	) \
}

using Block = std::list<LSToken>;
using BlockIter = Block::const_iterator;
using Expr = LSToken::Expr;

template<typename T>
Vector2Tmpl<T> parsePoint(const LSToken::Expr& expr);
TimeSpec parseTime(const LSToken::Expr& expr);

class LSMemberException : public std::exception
{
private:
	tiny_string reason;
public:
	LSMemberException(const tiny_string& _reason) : reason(_reason) {}

	const char* what() const noexcept override { return reason.raw_buf(); }
	const tiny_string& getReason() const noexcept { return reason; }
};

struct LSMemberInfo
{
	using MemberMap = std::unordered_map<tiny_string, LSMember>;
	using MemberAliasMap = std::unordered_map<tiny_string, tiny_string>;
	using EnumMap = std::unordered_map<tiny_string, LSEnumPair>;

	MemberMap memberMap;
	MemberAliasMap memberAliases;
	EnumMap enumMap;

	Optional<const LSMember&> tryGetMember(const tiny_string& name) const;
	Optional<const LSEnum&> tryGetEnum(const tiny_string& name) const;
	Optional<const LSShortEnum&> tryGetShortEnum(const tiny_string& name) const;
	const LSMember& getMember(const tiny_string& name) const;
	const LSEnum& getEnum(const tiny_string& name) const;
	const LSShortEnum& getShortEnum(const tiny_string& name) const;
	bool isAlias(const tiny_string& name) const;
	tiny_string getRealName(const tiny_string& name) const;
};

template<typename F>
static void parseAssignment(const Expr& expr, F&& callback, bool throwOnError = false)
{
	using OpType = LSToken::Expr::OpType;
	Optional<tiny_string> errorReason;
	auto trySetError = [&](const tiny_string& reason)
	{
		if (!errorReason.hasValue())
			errorReason = tiny_string("Assignment: ") + reason;
	};

	if (!expr.isOp())
		trySetError("Must only contain operators.");

	auto op = expr.op;
	if (op.type != OpType::Assign)
		trySetError("Must only contain assignments.");
	if (op.lhs == nullptr)
		trySetError("Left hand side of assignment is null.");
	if (op.rhs == nullptr)
		trySetError("Right hand side of assignment is null.");
	if (!op.lhs->isIdent())
		trySetError("Left hand side of assignment must be an identifier.");

	(void)errorReason.andThen([&](const tiny_string& reason) -> Optional<tiny_string>
	{
		if (throwOnError)
			throw LSMemberException(reason);
		return reason;
	}).orElse([&]
	{
		callback(op.lhs->ident, *op.rhs);
		return nullOpt;
	});
}

template<typename F>
static void parseBlock(const Block& block, F&& callback)
{
	for (auto it = block.begin(); it != block.end(); ++it)
		callback(it);
}


template<typename F>
static void parseAssignBlock(const Block& block, F&& callback, bool throwOnError = false)
{
	parseBlock(block, [&](const BlockIter& token)
	{
		if (token->isExpr())
			parseAssignment(token->expr, callback, throwOnError);
		else if (throwOnError)
			throw LSMemberException("Assignment block: Must only contain expressions.");
	});
}

template<typename T, typename U = void>
struct GetValue;

template<typename T>
struct GetValue<T, EnableIf<std::is_arithmetic<T>::value>>
{
	static T getValue(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		if (expr.isGroup())
			throw LSMemberException("Arithmetic members can't have expression groups.");
		return expr.eval<T>();
	}
};

template<>
struct GetValue<bool>
{
	static bool getValue(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		if (!expr.isBool() || !expr.isNumericExpr())
		{
			throw LSMemberException
			(
				"Bool member requires either a bool, or numeric expression."
			);
		}
		return expr.eval<bool>();
	}
};

template<>
struct GetValue<tiny_string>
{
	static tiny_string getValue(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		if (!expr.isString())
			throw LSMemberException("String member requires a string value.");
		return expr.value.asString();
	}
};

template<typename T>
struct GetValue<Vector2Tmpl<T>, EnableIf<std::is_floating_point<T>::value>>
{
	static Vector2Tmpl<T> getValue(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		// Float based points use pixels, so convert from twips, to pixels.
		return parsePoint<int64_t>(expr) / 20;
	}
};

template<typename T>
struct GetValue<Vector2Tmpl<T>, EnableIf<!std::is_floating_point<T>::value>>
{
	static Vector2Tmpl<T> getValue(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		return parsePoint<T>(expr);
	}
};

template<>
struct GetValue<TimeSpec>
{
	static TimeSpec getValue(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		return parseTime(expr);
	}
};

template<typename T, EnableIf<std::is_enum<T>::value, bool> = false>
static T getEnumValue(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
{
	if (!expr.isIdent())
		throw LSMemberException("getEnumValue: LSMemberInfos require an identifier.");
	auto _enum = memberInfo.getEnum(name);
	auto shortEnum = memberInfo.getShortEnum(name);
	return _enum.tryGetValue<T>(expr.ident).orElse([&]() -> Optional<T>
	{
		if (expr.ident.numChars() == 1)
		{
			auto ret = shortEnum.tryGetValue<T>(expr.ident[0]);
			if (ret.hasValue())
				return *ret;
		}
		std::stringstream s;
		s << "getEnumValue: `" << expr.ident <<
		"` isn't a valid enum value of member `" <<
		name << "`.";
		throw LSMemberException(s.str());
		return nullOpt;
	}).getValue();
}

template<typename T>
struct GetValue<T, EnableIf<std::is_enum<T>::value>>
{
	static T getValue(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		return getEnumValue<T>(memberInfo, name, expr);
	}
};

template<typename T>
struct GetValue<T, EnableIf<IsIterable<T>::value>>
{
	static T getValue(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		if (!expr.isList())
			throw LSMemberException("getValue: Lists require an array/table.");
		T list;
		for (auto it : expr.value.list)
			list.push_back(GetValue<typename T::value_type>::getValue(memberInfo, name, expr));
		return list;
	}
};

template<typename T>
struct GetValue<Optional<T>>
{
	static Optional<T> getValue(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		return GetValue<T>::getValue(memberInfo, name, expr);
	}
};

class LSMember
{
using Setter = void (*)
(
	void* data,
	size_t offset,
	const LSMemberInfo& memberInfo,
	const tiny_string& name,
	const Expr& expr
);
private:
	size_t memberOffset;
	Setter setter;

	template<typename T>
	static void setMember(void* data, size_t offset, const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		uint8_t* _data = static_cast<uint8_t*>(data);
		*reinterpret_cast<T*>(_data + offset) = GetValue<T>::getValue(memberInfo, name, expr);
	}

	template<typename T>
	static void setListMember(void* data, size_t offset, const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		uint8_t* _data = static_cast<uint8_t*>(data);
		reinterpret_cast<T*>(_data + offset)->push_back
		(
			GetValue<typename T::value_type>::getValue(memberInfo, name, expr)
		);
	}


	LSMember(size_t _memberOffset, Setter _setter) :
	memberOffset(_memberOffset),
	setter(_setter) {}
public:
	template<typename T, EnableIf<!IsIterable<T>::value, bool> = false>
	static LSMember create(size_t memberOffset)
	{
		return LSMember(memberOffset, &setMember<T>);
	}

	template<typename T, EnableIf<IsIterable<T>::value, bool> = false>
	static LSMember create(size_t memberOffset, bool singleEntry = false)
	{
		auto setter = singleEntry ? setListMember<T> : setMember<T>;
		return LSMember(memberOffset, setter);
	}

	void set(void* data, const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr) const
	{
		setter(data, memberOffset, memberInfo, name, expr);
	}
};

static bool isValidPointUnitStr(const tiny_string& str)
{
	if (str.empty())
		return false;
	switch (tolower(str[0]))
	{
		// Pixels.
		case 'p': return tolower(str[1]) == 'x'; break;
		// Twips.
		case 't': return tolower(str[1]) == 'w'; break;
		default: return false; break;
	}
}

using GroupIterPair = LSToken::Expr::GroupIterPair;
static tiny_string getUnit(Optional<GroupIterPair> groupIters)
{
	return groupIters.filter([&](const GroupIterPair& pair)
	{
		// In a group, check if the next element is an identifier.
		auto it = std::next(pair.first);
		auto end = pair.second;
		return it != end && it->isIdent();
	}).transformOr(tiny_string(), [&](const GroupIterPair& pair)
	{
		// It is, so consume it.
		return (++pair.first)->ident;
	});
}

template<typename T>
Vector2Tmpl<T> parsePoint(const LSToken::Expr& expr)
{
	using Expr = LSToken::Expr;
	using ExprList = std::list<Expr>;
	using EvalResult = Expr::EvalResult<T>;
	using EvalResultType = typename EvalResult::Type;
	using GroupIterPair = Expr::GroupIterPair;
	using OpType = Expr::OpType;

	bool parsingDimension = false;
	auto pointEvalVisitor = makeVisitor
	(
		[&]
		(
			const Expr::Value::Number& num,
			const Expr& parent,
			Optional<GroupIterPair> groupIters,
			Optional<size_t> listIndex,
			size_t
		) -> EvalResult
		{
			tiny_string unit = getUnit(groupIters);

			return listIndex.transform([&](size_t i) -> Optional<EvalResult>
			{
				// Ignore units, if we're in a list.
				// First element: Pixels.
				// Second element: Twips.
				if (!i)
					return EvalResult(num.as<T>() * 20);
				return EvalResult(typename EvalResult::Op(OpType::Plus, num.as<T>()));
			}).orElseIf(isValidPointUnitStr(unit), [&]() -> EvalResult
			{
				// Multiply the value by 20, if unit is pixels, since
				// points are always measured in twips.
				bool isPixels = tolower(unit[0]) == 'p';
				return isPixels ? num.as<T>() * 20 : num.as<T>();
			}).orElse([&]() -> EvalResult
			{
				if (unit.empty())
				{
					// Unitless values are usually treated as twips.
					return EvalResultType::NoOverride;
				}
				std::stringstream s;
				s << "Point: `" << unit << "` isn't a valid unit.";
				return typename EvalResult::Error { tiny_string(s.str()) };
			}).getValue();
		},
		[&]
		(
			const ExprList& list,
			const Expr&,
			Optional<GroupIterPair>,
			Optional<size_t>,
			size_t depth
		) -> EvalResult
		{
			if (depth > 1)
			{
				std::stringstream s;
				s << "Point: Only a single inner list is allowed, got " << depth << " instead.";
				return typename EvalResult::Error { tiny_string(s.str()) };
			}
			if (list.size() > 2)
			{
				std::stringstream s;
				s << "Point: Lists can't have more than 2 elements, got " << list.size() << " instead.";
				return typename EvalResult::Error { tiny_string(s.str()) };
			}
			return EvalResultType::NoOverride;
		},
		[&]
		(
			const tiny_string& ident,
			bool isString,
			const Expr&,
			Optional<GroupIterPair>,
			Optional<size_t>,
			size_t
		)
		{
			bool isDimensionDelim =
			(
				ident.numChars() == 1 &&
				(ident[0] == 'x' || ident[0] == 'X')
			);
			if (parsingDimension && !isString && isDimensionDelim)
				return EvalResultType::Done;
			return EvalResultType::Skip;
		},
		[&](const Expr& expr, const Expr&, Optional<GroupIterPair>, Optional<size_t>, size_t)
		{
			if (!expr.isNumericExpr() && !expr.isGroup() && !expr.isList())
				return EvalResultType::Invalid;
			return EvalResultType::NoOverride;
		}
	);

	Vector2Tmpl<T> ret;
	if (expr.isList())
	{
		// Got a list.
		auto list = expr.value.list;
		if (list.size() > 2)
		{
			std::stringstream s;
			s << "Point: Lists can't have more than 2 elements, got " << list.size() << " instead.";
			throw LSMemberException(s.str());
		}

		auto it = list.begin();
		if (it != list.end())
			ret.x = (it++)->eval<T>(pointEvalVisitor);
		if (it != list.end())
			ret.y = it->eval<T>(pointEvalVisitor);
	}
	else if (expr.isBlock())
	{
		// Got an assignment block.
		parseAssignBlock(expr.block, [&]
		(
			const tiny_string& name,
			const Expr& expr
		)
		{
			if (name == "width" || name == "x")
				ret.x = expr.eval<T>(pointEvalVisitor);
			else if (name == "height" || name == "y")
				ret.y = expr.eval<T>(pointEvalVisitor);
		}, true);
	}
	else if (expr.isGroup())
	{
		// Got a group.
		auto group = expr.group;

		// Check if it's a dimension.
		if (group.list.size() < 3)
		{
			std::stringstream s;
			s << "Point: Dimensions require at least 3 group elements, got " << group.list.size() << " instead.";
			throw LSMemberException(s.str());
		}

		// NOTE: We allow upto 5 group elements in a dimension, because
		// units can be used, e.g. `640px x 480px`. Which as a list,
		// would be `[640, px, x, 480, px]`, which has 5 group elements.
		if (group.list.size() > 5)
		{
			std::stringstream s;
			s << "Point: Dimensions can't have more than 5 group elements, got " << group.list.size() << " instead.";
			throw LSMemberException(s.str());
		}

		// It is, so parse it.
		parsingDimension = true;
		auto result = expr.resultEval<T>(pointEvalVisitor);

		ret.x = result.getValue().filter(result.isDone()).orElse([&]
		{
			throw LSMemberException("Dimension: Left hand side isn't valid.");
			return nullOpt;
		}).getValue();
		parsingDimension = false;
		ret.y = expr.eval<T>(pointEvalVisitor);
	}
	else
	{
		throw LSMemberException
		(
			"Point: Expression must be either a list, assignment block, or group"
		);
	}
	return ret;
}

#endif /* UTILS_TOKEN_PARSER_MEMBER_H */
