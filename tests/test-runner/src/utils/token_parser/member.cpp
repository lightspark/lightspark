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

#include <sstream>

#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/timespec.h>

#include "utils/token_parser/member.h"
#include "utils/token_parser/enum.h"

// TODO: Move this into the main codebase, at some point.

static bool isValidTimeUnitStr(const tiny_string& str)
{
	if (str.empty())
		return false;
	switch (tolower(str[0]))
	{
		// Nanoseconds.
		case 'n':
		// Microseconds.
		case 'u':
		// Milliseconds.
		case 'm': return tolower(str[1]) == 's'; break;
		// Seconds.
		case 's': return true; break;
		default: return false; break;
	}
}

TimeSpec parseTime(const LSToken::Expr& expr)
{
	using Expr = LSToken::Expr;
	using ExprList = std::list<Expr>;
	using EvalResult = Expr::EvalResult<uint64_t>;
	using EvalResultType = EvalResult::Type;
	using GroupIterPair = Expr::GroupIterPair;

	enum Unit
	{
		Nano,
		Micro,
		Milli,
		Second,
	};

	auto scaleToNs = [&](int64_t val, const Unit& unit) -> uint64_t
	{
		switch (unit)
		{
			default:
			case Unit::Nano: return val; break;
			case Unit::Micro: return val * TimeSpec::nsPerUs; break;
			case Unit::Milli: return val * TimeSpec::nsPerMs; break;
			case Unit::Second: return val * TimeSpec::nsPerSec; break;
		}
	};

	auto toUnit = [&](uint32_t ch)
	{
		switch (ch)
		{
			// Nanoseconds.
			case 'n': return Unit::Nano; break;
			// Microseconds.
			case 'u': return Unit::Micro; break;
			// Milliseconds.
			case 'm': return Unit::Milli; break;
			// Seconds.
			case 's': return Unit::Second; break;
			default:
				throw LSMemberException("Time: No valid unit found.");
				break;
		}
	};

	bool isSeconds = false;
	auto timeEvalVisitor = makeVisitor
	(
		[&]
		(
			const Expr::Value::Number& num,
			const Expr&,
			Optional<GroupIterPair> groupIters,
			Optional<size_t>,
			size_t
		) -> EvalResult
		{
			tiny_string unit = getUnit(groupIters);

			// Ignore the unit, if we're explicitly getting seconds.
			if (!isSeconds && isValidTimeUnitStr(unit))
			{
				// Convert the value to nanoseconds, since time is always
				// measured in the form [seconds, nanoseconds].
				return scaleToNs(num.as<uint64_t>(), toUnit(tolower(unit[0])));
			}
			else if (isSeconds || unit.empty())
			{
				// Unitless values are usually treated as nanoseconds.
				return EvalResultType::NoOverride;
			}
			std::stringstream s;
			s << "Time: `" << unit << "` isn't a valid unit.";
			return typename EvalResult::Error {tiny_string(s.str()) };
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
			return typename EvalResult::Error
			{
				tiny_string("Time: Recursive lists aren't allowed.", true)
			};
		},
		[&](const tiny_string&, bool, const Expr&, Optional<GroupIterPair>, Optional<size_t>, size_t)
		{
			return EvalResultType::Invalid;
		},
		[&](const Expr& expr, const Expr&, Optional<GroupIterPair>, Optional<size_t>, size_t)
		{
			if (!expr.isNumericExpr() && !expr.isGroup() && !expr.isList())
				return EvalResultType::Invalid;
			return EvalResultType::NoOverride;
		}
	);

	if (!expr.isList() && !expr.isBlock())
		return TimeSpec::fromNs(expr.eval<uint64_t>(timeEvalVisitor));

	uint64_t secs = 0;
	uint64_t nsecs = 0;
	if (expr.isList())
	{
		// Got a list.
		auto list = expr.value.list;
		if (list.size() > 2)
		{
			std::stringstream s;
			s << "Time: Lists can't have more than 2 elements, got " << list.size() << " instead.";
			throw LSMemberException(s.str());
		}

		auto it = list.begin();
		isSeconds = true;
		if (it != list.end())
			secs = (it++)->eval<uint64_t>(timeEvalVisitor);

		isSeconds = false;
		if (it != list.end())
			nsecs = it->eval<uint64_t>(timeEvalVisitor);
	}
	else
	{
		// Got an assignment block.
		parseAssignBlock(expr.block, [&]
		(
			const tiny_string& name,
			const Expr& expr
		)
		{
			if (!expr.isNumericExpr())
			{
				throw LSMemberException
				(
					"Time: Right hand side of assignment block must be numeric."
				);
			}
			if (name == "secs")
				secs = expr.eval<uint64_t>();
			else if (name.numChars() == 5 && name.endsWith("secs"))
				nsecs = scaleToNs(expr.eval<uint64_t>(), toUnit(name[0]));
		}, true);
	}
	return TimeSpec(secs, nsecs);
}

Optional<const LSMember&> LSMemberInfo::tryGetMember(const tiny_string& name) const
{
	auto it = memberMap.find(name);
	if (it == memberMap.end() && isAlias(name))
		it = memberMap.find(getRealName(name));
	if (it != memberMap.end())
		return it->second;
	return {};
}

Optional<const LSEnum&> LSMemberInfo::tryGetEnum(const tiny_string& name) const
{
	auto it = enumMap.find(name);
	if (it == enumMap.end() && isAlias(name))
		it = enumMap.find(getRealName(name));
	if (it != enumMap.end())
		return it->second._enum;
	return {};
}

Optional<const LSShortEnum&> LSMemberInfo::tryGetShortEnum(const tiny_string& name) const
{
	auto it = enumMap.find(name);
	if (it == enumMap.end() && isAlias(name))
		it = enumMap.find(getRealName(name));
	if (it != enumMap.end())
		return it->second.shortEnum;
	return {};
}

const LSMember& LSMemberInfo::getMember(const tiny_string& name) const
{
	return tryGetMember(name).orElse([&]
	{
		std::stringstream s;
		s << "getMember: `" << name << "` isn't a valid member";
		throw LSMemberException(s.str());
		return nullOpt;
	}).getValue();
}

const LSEnum& LSMemberInfo::getEnum(const tiny_string& name) const
{
	return tryGetEnum(name).orElse([&]
	{
		std::stringstream s;
		s << "getEnum: `" << name << "` isn't a valid enum.";
		throw LSMemberException(s.str());
		return nullOpt;
	}).getValue();
}

const LSShortEnum& LSMemberInfo::getShortEnum(const tiny_string& name) const
{
	return tryGetShortEnum(name).orElse([&]
	{
		std::stringstream s;
		s << "getShortEnum: `" << name << "` isn't a valid short enum.";
		throw LSMemberException(s.str());
		return nullOpt;
	}).getValue();
}

bool LSMemberInfo::isAlias(const tiny_string& name) const
{
	return memberAliases.find(name) != memberAliases.end();
}

tiny_string LSMemberInfo::getRealName(const tiny_string& name) const
{
	if (!isAlias(name))
	{
		std::stringstream s;
		s << "getRealName: `" << name << "` isn't an alias";
		throw LSMemberException(s.str());
	}
	return memberAliases.at(name);
}
