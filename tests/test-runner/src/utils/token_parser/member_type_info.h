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

#ifndef INPUT_FORMATS_LIGHTSPARK_MEMBER_TYPE_INFO_H
#define INPUT_FORMATS_LIGHTSPARK_MEMBER_TYPE_INFO_H 1

#include <list>
#include <unordered_set>

#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>

#include "utils/token_parser/member.h"
#include "utils/token_parser/token.h"

using namespace lightspark;

namespace lightspark
{
	template<typename T>
	class Vector2Tmpl;
	class TimeSpec;
};

// TODO: Move this into the main codebase, at some point.

#define TYPE_INFO(type, name, ...) \
	LSMemberTypeInfo::create<type>(#name, { __VA_ARGS__ })

#define TYPE_INFO_MEMBER(type, member, ...) \
	TYPE_INFO(MEMBER_TYPE(type, member), member, __VA_ARGS__)

using Expr = LSToken::Expr;

template<typename T, typename U = void>
struct IsValid;

template<typename T>
struct IsValid<T, EnableIf<std::is_arithmetic<T>::value>>
{
	static bool isValid(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		return expr.isNumericExpr();
	}
};

template<>
struct IsValid<bool>
{
	static bool isValid(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		return expr.isBool();
	}
};

template<>
struct IsValid<tiny_string>
{
	static bool isValid(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		return expr.isString();
	}
};

template<typename T>
struct IsValid<Vector2Tmpl<T>>
{
	static bool isValid(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		try
		{
			(void)parsePoint<T>(expr);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
};

template<>
struct IsValid<TimeSpec>
{
	static bool isValid(const LSMemberInfo&, const tiny_string&, const Expr& expr)
	{
		try
		{
			(void)parseTime(expr);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
};

template<typename T>
struct IsValid<T, EnableIf<std::is_enum<T>::value>>
{
	static bool isValid(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		if (!expr.isIdent())
			return false;

		auto ch = expr.ident[0];
		auto _enum = memberInfo.tryGetEnum(name);
		auto shortEnum = memberInfo.tryGetShortEnum(name);

		if (!_enum.hasValue())
			return false;

		return
		(
			expr.ident.numChars() == 1 &&
			shortEnum.hasValue() &&
			shortEnum->tryGetValue<T>(ch).hasValue()
		) || _enum->tryGetValue<T>(expr.ident).hasValue();
	}
};

template<typename T>
struct IsValid<std::list<T>>
{
	static bool isValid(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		if (!expr.isList())
			return false;
		for (auto it : expr.value.list)
		{
			if (!IsValid<T>::isValid(memberInfo, name, it))
				return false;
		}
		return true;
	}
};

template<typename T>
struct IsValid<Optional<T>>
{
	static bool isValid(const LSMemberInfo& memberInfo, const tiny_string& name, const Expr& expr)
	{
		return IsValid<T>::isValid(memberInfo, name, expr);
	}
};

class LSMemberTypeInfo
{
using IsValidCallback = bool (*)
(
	const LSMemberInfo& memberInfo,
	const tiny_string& name,
	const Expr& expr
);
using MemberOfSet = std::unordered_set<tiny_string>;
private:
	tiny_string name;
	MemberOfSet memberOfSet;
	IsValidCallback isValidCallback;

	LSMemberTypeInfo
	(
		const tiny_string& _name,
		const MemberOfSet& _memberOfSet,
		IsValidCallback _isValidCallback
	) :
	name(_name),
	memberOfSet(_memberOfSet),
	isValidCallback(_isValidCallback) {}
public:
	template<typename T>
	static LSMemberTypeInfo create(const tiny_string& name, const MemberOfSet& memberOfSet)
	{
		return LSMemberTypeInfo(name, memberOfSet, &IsValid<T>::isValid);
	}

	bool isMemberOf(const tiny_string& typeName, const tiny_string& subTypeName) const
	{
		return isMemberOf(typeName) || isMemberOf(typeName + subTypeName);
	}

	bool isMemberOf(const tiny_string& typeName) const
	{
		return memberOfSet.find(typeName) != memberOfSet.end();
	}

	bool isValid(const LSMemberInfo& memberInfo, const Expr& expr) const
	{
		return isValidCallback(memberInfo, name, expr);
	}

	const tiny_string& getName() const { return name; }
};

#endif /* INPUT_FORMATS_LIGHTSPARK_MEMBER_TYPE_INFO_H */
