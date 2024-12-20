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
#include <string>

#include <lightspark/tiny_string.h>
#include <lightspark/swftypes.h>
#include <lightspark/utils/optional.h>

#include "utils/token_parser/token.h"
#include "utils/token_parser/token_parser.h"

using namespace lightspark;

// TODO: Move this into the main codebase, at some point.

static tiny_string openTextFile(const tiny_string& path)
{
	auto openMode =
	(
		std::ifstream::in |
		std::ifstream::binary |
		std::ifstream::ate
	);

	// Open the file.
	std::ifstream file(path, openMode);

	if (!file.is_open())
	{
		std::cerr << "Couldn't open \"" << path << '"' << std::endl;
		std::exit(1);
	}

	// Get the file size.
	const auto size = file.tellg();
	if (size < 0)
	{
		std::cerr << "Couldn't get the size of file \"" << path << '"' << std::endl;
		std::exit(1);
	}

	file.seekg(0, std::ifstream::beg);

	// Read, and return the file contents.
	return tiny_string(file, size);
}

static bool isInitialIdentChar(uint32_t ch)
{
	return isalpha(ch) || ch == '_';
}

static bool isIdentChar(uint32_t ch)
{
	return isalnum(ch) || ch == '_' || ch == '.';
}

static tiny_string shiftSubstr(tiny_string& str, size_t start, size_t len = tiny_string::npos, size_t offset = 0)
{
	auto ret = str.substr(start, len-start);
	str = str.substr(std::min(str.numChars() - offset, len) + offset, UINT32_MAX);
	return ret;
}

static tiny_string& inplaceSubstr(tiny_string& str, size_t start, size_t len = tiny_string::npos)
{
	return str = str.substr(start, len);
}

tiny_string& LSTokenParser::inplaceRemoveWhitespace(tiny_string& str) const
{
	auto _str = str.removeWhitespace();
	auto whitespaceStr = str.substr(0, str.numChars() - _str.numChars());
	if (inDirective && whitespaceStr.find("\n") != tiny_string::npos)
		inDirective = false;
	return str = _str;
}

template<typename F>
static size_t matches(const tiny_string& data, F&& pred, size_t i = 0)
{
	for (; i < data.numChars() && pred(data[i]); ++i);
	return i;
}

template<typename F>
static size_t invMatches(const tiny_string& data, F&& pred, size_t i = 0)
{
	for (; i < data.numChars() && !pred(data[i]); ++i);
	return i;
}

template<typename F, typename F2>
static size_t onMatch(const tiny_string& data, F&& pred, F2&& callback, size_t i = 0)
{
	for (; i < data.numChars() && pred(data[i]); ++i)
		callback(data[i], i);
	return i;
}

template<typename F, typename F2>
static size_t onInvMatch(const tiny_string& data, F&& pred, F2&& callback, size_t i = 0)
{
	for (; i < data.numChars() && !pred(data[i]); ++i)
		callback(data[i], i);
	return i;
}

static uint32_t unescapeChar(const tiny_string& _str, size_t& offset)
{
	auto str = _str.substr(++offset, UINT32_MAX);

	uint32_t ch;
	switch (str[0])
	{
		case '"': ch = '"'; break;
		case '\'': ch = '\''; break;
		case '\\': ch = '\\'; break;
		case 'b': ch = '\b'; break;
		case 'f': ch = '\f'; break;
		case 'n': ch = '\n'; break;
		case 'r': ch = '\r'; break;
		case 't': ch = '\t'; break;
		case 'u':
		case 'U':
		{
			size_t i = 0;
			const size_t codePointSize = str[0] == 'u' ? 4 : 6;
			ch = std::stoul(str.substr(1, UINT32_MAX), &i, 16);
			if (i != codePointSize)
				throw LSTokenParserException("Bad unicode escape sequence.");
			offset += i;
			break;
		}
		default:
			throw LSTokenParserException("Bad escape sequence.");
			break;
	}
	offset++;
	return ch;
}

template<typename F>
static void dumpType(const tiny_string& name, F&& callback)
{
	std::cerr << name << '(' << callback() << ')';
}

template<typename F>
static void dumpTypePretty(const tiny_string& name, size_t depth, F&& callback)
{
	std::string indent(depth, '\t');
	std::cerr << name << '(' << std::endl;
	callback(depth + 1);
	std::cerr << std::endl << indent << ')';
}

template<typename T>
static void dumpList(const std::list<T>& list, const tiny_string name, size_t depth = 0)
{
	dumpTypePretty(name, depth, [&](size_t depth)
	{
		size_t i = 0;
		for (auto elem : list)
		{
			elem.dump(depth);
			if (i++ < list.size() - 1)
				std::cerr << ',' << std::endl;
		}
	});
}

void LSToken::Expr::Value::dump(size_t depth) const
{
	std::string indent(depth, '\t');
	std::cerr << indent;

	switch (type)
	{
		case Type::String:
			dumpType("String", [&] { return str; });
			break;
		case Type::Char:
			dumpType("Char", [&] { return tiny_string::fromChar(ch); });
			break;
		case Type::Bool:
			dumpType("Char", [&] { return std::to_string(flag); });
			break;
		case Type::Number:
			dumpType("Number", [&]
			{
				if (num.isInt())
					return std::to_string(num.intVal);
				return std::to_string(num.floatVal);
			});
			break;
		case Type::List: dumpList(list, "List", depth); break;
	}
}

void LSToken::Expr::Op::dump(size_t depth) const
{
	std::string indent(depth, '\t');
	std::cerr << indent;

	tiny_string opName;
	switch (type)
	{
		case OpType::Assign: opName = "Assign"; break;
		case OpType::Plus: opName = "Plus"; break;
		case OpType::Minus: opName = "Minus"; break;
		case OpType::Multiply: opName = "Multiply"; break;
		case OpType::Divide: opName = "Divide"; break;
		case OpType::LeftShift: opName = "LeftShift"; break;
		case OpType::RightShift: opName = "RightShift"; break;
		case OpType::BitNot: opName = "BitNot"; break;
		case OpType::BitAnd: opName = "BitAnd"; break;
		case OpType::BitOr: opName = "BitOr"; break;
		case OpType::BitXor: opName = "BitXor"; break;
		case OpType::Not: opName = "Not"; break;
		case OpType::And: opName = "And"; break;
		case OpType::Or: opName = "Or"; break;
		case OpType::Equal: opName = "Equal"; break;
		case OpType::NotEqual: opName = "NotEqual"; break;
		case OpType::LessThan: opName = "LessThan"; break;
		case OpType::LessEqual: opName = "LessEqual"; break;
		case OpType::GreaterThan: opName = "GreaterThan"; break;
		case OpType::GreaterEqual: opName = "GreaterEqual"; break;
	}

	std::cerr << opName;
	if (lhs != nullptr)
	{
		std::cerr << ',' << std::endl << indent;
		dumpTypePretty("Expr", depth, [&](size_t depth)
		{
			lhs->dump(depth);
		});
	}
	if (rhs != nullptr)
	{
		std::cerr << ',' << std::endl << indent;
		dumpTypePretty("Expr", depth, [&](size_t depth)
		{
			rhs->dump(depth);
		});
	}
}

void LSToken::Expr::dump(size_t depth) const
{
	std::string indent(depth, '\t');
	std::cerr << indent;
	switch (type)
	{
		case Type::Op:
			dumpTypePretty("Op", depth, [&](size_t depth)
			{
				op.dump(depth);
			});
			break;
		case Type::Value:
			dumpTypePretty("Value", depth, [&](size_t depth)
			{
				value.dump(depth);
			});
			break;
		case Type::Identifier:
			dumpType("Identifier", [&] { return ident; });
			break;
		case Type::Block: dumpList(block, "Block", depth); break;
		case Type::Group: dumpList(group.list, "Group", depth); break;
	}
}

void LSToken::dump(size_t depth) const
{
	std::string indent(depth, '\t');
	std::cerr << indent;
	switch (type)
	{
		case Type::Comment:
			dumpType("Comment", [&] { return comment.str; });
			break;
		case Type::Directive:
			dumpType("Directive", [&] { return dir.name; });
			break;
		case Type::Expr:
			dumpTypePretty("Expr", depth, [&](size_t depth)
			{
				expr.dump(depth);
			});
			break;
	}
}

tiny_string LSTokenParser::parseString(tiny_string& data) const
{
	auto isQuote = [](uint32_t ch) { return ch == '"'; };
	assert(isQuote(data[0]));
	if (isQuote(data[1]) && !isQuote(data[2]))
	{
		std::stringstream s;
		s << R"(Invalid multi-line string. Expected `"""`, got )" << data.substr(0, 3) << '.';
		throw LSTokenParserException(s.str());
	}

	bool isMultiLine = data[1] == '"';
	tiny_string delim = isMultiLine ? R"(""")" : "\"";
	tiny_string str;

	data = data.stripPrefix(delim);

	auto i = onInvMatch(data, isQuote, [&](uint32_t ch, size_t& i)
	{
		if (isMultiLine && ch == '\n')
			throw LSTokenParserException("Invalid single-line string. Got end-of-line before ending `\"`.");
		str += ch == '\\' ? unescapeChar(data, i) : ch;
	});

	if (!inplaceSubstr(data, i).startsWith(delim))
	{
		std::stringstream s;
		s << "Invalid string. missing ending `" << delim << "`.";
		throw LSTokenParserException(s.str());
	}
	data = data.stripPrefix(delim);
	return str;
}

uint32_t LSTokenParser::parseChar(tiny_string& data) const
{
	assert(data[0] == '\'');
	size_t i = 1;
	uint32_t ret = data[i] == '\\' ? unescapeChar(data, i) : data[i++];

	if (!shiftSubstr(data, 1, i + 1).endsWith("'"))
		throw LSTokenParserException("Invalid character. Missing ending `'`.");

	return ret;

}

Optional<bool> LSTokenParser::parseBool(tiny_string& data) const
{
	auto ch = tolower(data[0]);
	assert(ch == 't' || ch == 'f');

	auto i = matches(data, isIdentChar);
	auto boolStr = data.substr(0, i).lowercase();
	Optional<bool> ret;

	if (boolStr == "true")
		ret = true;
	if (boolStr == "false")
		ret = false;

	return ret.andThen([&](bool flag) -> Optional<bool>
	{
		inplaceSubstr(data, i);
		return flag;
	});
}

LSToken::Expr::Value::Number LSTokenParser::parseNumber(tiny_string& data) const
{
	using Value = LSToken::Expr::Value;
	assert(isdigit(data[0]) || data[0] == '.');
	size_t i = 0;

	if (data[i] == '0')
	{
		switch (tolower(data[++i]))
		{
			// Binary.
			case 'b':
				++i;
				if (data[i] != '0' && data[i] != '1')
					throw LSTokenParserException("Invalid binary number");
				i = matches(data, [](uint32_t ch)
				{
					return ch == '0' || ch == '1';
				}, i);
				break;
			// Hexadecimal.
			case 'x':
				if (!isxdigit(data[++i]))
					throw LSTokenParserException("Invalid hex number");
				i = matches(data, isxdigit, i);
				break;
			// Floating point.
			case '.':
				if (!isdigit(data[++i]))
					throw LSTokenParserException("Invalid float number");
				i = matches(data, isdigit, i);
				break;
			// Octal, or just a single `0`.
			default:
				// End early, if this character isn't a digit.
				if (!isdigit(data[i]))
					break;

				// Is this characther a valid octal digit?
				if (data[i] < '0' || data[i] > '7')
					throw LSTokenParserException("Invalid octal number");
				i = matches(data, [](uint32_t ch)
				{
					return ch >= '0' && ch <= '7';
				}, i);
				break;
		}
	}
	else
	{
		i = matches(data, isdigit, i);
		if (data[i] == '.')
		{
			if (!isdigit(data[++i]))
				throw LSTokenParserException("Invalid float number");
			i = matches(data, isdigit, i);
		}
	}

	auto numStr = shiftSubstr(data, 0, i);

	// Binary
	if (numStr.numChars() > 2 && numStr[0] == '0' && tolower(numStr[1]) == 'b')
	{
		return Value::Number
		(
			std::stoll(numStr.substr(2, UINT32_MAX), nullptr, 2)
		);
	}
	// Floating point.
	else if (numStr.find(".") != tiny_string::npos)
		return Value::Number(std::stod(numStr));
	// Integer.
	else
		return Value::Number(std::stoll(numStr));
}

std::list<LSToken::Expr> LSTokenParser::parseList(tiny_string& data) const
{
	assert(data[0] == '[');
	inplaceSubstr(data, 1);
	listDepth++;

	std::list<LSToken::Expr> list;
	while (!data.empty() && data[0] != ']')
	{
		list.push_back(parseExpr(data));
		if (data[0] != ',' && data[0] != ']')
			throw LSTokenParserException("Invalid list. Missing `,`.");
		if (data[0] == ',')
			inplaceSubstr(data, 1);
	}

	if (data[0] != ']')
		throw LSTokenParserException("Invalid list. Missing ending `]`.");

	listDepth--;
	inplaceSubstr(data, 1);
	isExprDone = false;
	return list;
}

LSToken::Expr::Op LSTokenParser::parseOp
(
	tiny_string& data,
	const Optional<LSToken::Expr>& lhs
) const
{
	using OpType = Expr::OpType;
	assert(Expr::Op::isOpChar(data[0]));

	bool isUnary = !lhs.hasValue();
	auto ch = data[0];

	Optional<OpType> type;
	switch (ch)
	{
		// Assignment, or Equality.
		case '=':
		{
			auto ch2 = data[1];
			if (Expr::Op::isOpChar(ch2) && ch2 != '=')
				break;
			type = ch2 == '=' ? OpType::Equal : OpType::Assign;
			break;
		}
		// Plus.
		case '+': type = OpType::Plus; break;
		// Minus.
		case '-': type = OpType::Minus; break;
		// Multiply.
		case '*': type = OpType::Multiply; break;
		// Divide.
		case '/': type = OpType::Divide; break;
		// Less/Greater than (or equal), or Left/Right shift.
		case '<':
		case '>':
		{
			auto ch2 = data[1];
			if (Expr::Op::isOpChar(ch2) && ch2 != ch && ch2 != '=')
				break;
			bool isLess = ch == '<';
			// Less/Greater than.
			if (!Expr::Op::isOpChar(ch2))
			{
				type =
				(
					isLess ?
					OpType::LessThan :
					OpType::GreaterThan
				);
				break;
			}
			// Less/Greater than, or equal.
			if (ch2 == ch)
			{
				type =
				(
					isLess ?
					OpType::LessEqual :
					OpType::GreaterEqual
				);
				break;
			}
			// Left/Right shift.
			type =
			(
				isLess ?
				OpType::LeftShift :
				OpType::RightShift
			);
			break;
		}
		// Not (equal).
		case '!':
		{
			auto ch2 = data[1];
			if (Expr::Op::isOpChar(ch2) && ch2 != '=')
				break;
			type = ch2 == '=' ? OpType::NotEqual : OpType::Not;
			break;
		}
		// (Bitwise) And/Or.
		case '&':
		case '|':
		{
			auto ch2 = data[1];
			if (Expr::Op::isOpChar(ch2) && ch2 != ch)
				break;
			bool isAnd = ch == '&';
			// And/Or.
			if (ch2 == ch)
				type = isAnd ? OpType::And : OpType::Or;
			// Bitwise And/Or.
			else
				type = isAnd ? OpType::BitAnd : OpType::BitOr;
			break;
		}
		// Bitwise Xor.
		case '^': type = OpType::BitXor; break;
		// Bitwise Not.
		case '~': type = OpType::BitNot; break;
		default: break;
	}

	return *type.transform([&](const OpType& type) -> Optional<Expr::Op>
	{
		bool isSingleCharOp = Expr::Op::isSingleCharOp(type);
		inplaceSubstr(data, isSingleCharOp ? 1 : 2);

		if (isUnary)
		{
			if (!Expr::Op::isUnaryOp(type))
				throw LSTokenParserException("Invalid unary operator.");
			return Expr::Op(type, parseExpr(data));
		}
		else
		{
			if (!Expr::Op::isBinaryOp(type))
				throw LSTokenParserException("Invalid binary operator.");
			return Expr::Op(type, *lhs, parseExpr(data));
		}
	}).orElse([&]
	{
		throw LSTokenParserException("Invalid operator.");
		return nullOpt;
	});
}

LSToken::Expr::Value LSTokenParser::parseValue(tiny_string& data) const
{
	switch (data[0])
	{
		// Strings.
		case '"': return parseString(data); break;
		// Characters.
		case '\'': return parseChar(data); break;
		// Lists.
		case '[': return parseList(data); break;
		// Numbers.
		default:
		{
			if (!isdigit(data[0]) && data[0] != '.')
				throw LSTokenParserException("Invalid value.");
			return parseNumber(data);
			break;
		}
	}
}

tiny_string LSTokenParser::parseIdent(tiny_string& data) const
{
	assert(isInitialIdentChar(data[0]));
	return shiftSubstr(data, 0, matches(data, isIdentChar, 1));
}

LSToken::Expr::Block LSTokenParser::parseBlock(tiny_string& data, bool implicit) const
{
	assert(implicit || data[0] == '{');
	bool hasStartingBrace = data[0] == '{';
	if (hasStartingBrace)
		inplaceSubstr(data, 1);

	Expr::Block block;
	while (!data.empty() && (!hasStartingBrace || data[0] != '}'))
	{
		try
		{
			block.push_back(parseToken(data));
		} catch (InvalidExprException& e)
		{
			if (hasStartingBrace && data[0] != '}')
				throw LSTokenParserException(e.getReason());
		}
	}

	if (!implicit && data.empty())
		throw LSTokenParserException("Invalid explicit block. String is empty.");
	if (hasStartingBrace && data[0] != '}')
		throw LSTokenParserException("Invalid block. Has a starting `{`, but is missing an ending `}`.");

	if (hasStartingBrace)
	{
		inplaceSubstr(data, 1);
		isExprDone = false;
	}
	return block;
}

LSToken::Comment LSTokenParser::parseComment(tiny_string& data) const
{
	assert(data[0] == '/');
	if (data[1] != '/' && data[1] != '*')
	{
		std::stringstream s;
		s << "Invalid comment. Expected `//`, or `/*`, got `" << data.substr(0, 2) << "`.";
		throw LSTokenParserException(s.str());
	}

	tiny_string endDelim = data[1] == '/' ? "\n" : "*/";

	auto commentStr = shiftSubstr
	(
		data,
		2,
		data.find(endDelim, 2)
	);

	auto end = shiftSubstr(data, 0, endDelim.numChars());
	if (!end.startsWith(endDelim))
	{
		std::stringstream s;
		s << "Invalid comment. Expected end-of-line, or `*/`, got `" << end << "`.";
		throw LSTokenParserException(s.str());
	}
	return LSToken::Comment { commentStr };
}

LSToken::Directive LSTokenParser::parseDirective(tiny_string& data) const
{
	assert(data[0] == '#');
	inDirective = true;
	return LSToken::Directive
	{
		parseIdent(inplaceRemoveWhitespace(inplaceSubstr(data, 1)))
	};
}

LSToken::Expr LSTokenParser::parseExpr(tiny_string& data) const
{
	std::list<LSToken::Expr> exprs;
	bool prevInDirective = inDirective;

	auto isEndDelim = [&](uint32_t ch) { return ch == ';' || ch == ','; };
	auto isExprDone = [&](uint32_t ch)
	{
		return
		(
			isEndDelim(ch) ||
			(blockDepth > 0 && ch == '}') ||
			(listDepth > 0 && ch == ']') ||
			inDirective != prevInDirective
		);
	};

	auto popExpr = [&]() -> Optional<LSToken::Expr>
	{
		if (!exprs.empty())
		{
			auto expr = exprs.front();
			exprs.pop_front();
			return expr;
		}
		return {};
	};

	this->isExprDone = isExprDone(inplaceRemoveWhitespace(data)[0]);
	while (!this->isExprDone)
	{
		auto ch = data[0];
		switch (ch)
		{
			// Blocks.
			case '{':
				blockDepth++;
				exprs.push_back(parseBlock(data));
				blockDepth--;
				break;
			// Bools.
			// NOTE: This is a special case, since `true`, and `false`
			// would've normally been treated as identifiers.
			case 't':
			case 'f':
			case 'T':
			case 'F':
			{
				auto boolVal = parseBool(data);
				if (boolVal.hasValue())
				{
					// Got `true`, or `false`, create a bool value.
					exprs.push_back(Expr::Value(boolVal.getValue()));
					break;
				}
				// Didn't get `true`, or `false`, fall through, and
				// parse an identifier.
			}
			default:
				// Identifiers.
				if (isInitialIdentChar(ch))
					exprs.push_back(parseIdent(data));
				// Operators.
				else if (Expr::Op::isOpChar(ch))
				{
					opDepth++;
					exprs.push_back(parseOp(data, popExpr()));
					opDepth--;
				}
				// Values.
				else
					exprs.push_back(parseValue(data));
				break;
		}

		this->isExprDone |= isExprDone(inplaceRemoveWhitespace(data)[0]);
	}

	// Skip over expression end delimiter, if needed.
	if (isEndDelim(data[0]) && listDepth == 0)
		inplaceSubstr(data, 1);

	// Make a group, if we have multiple expressions left in the list.
	if (exprs.size() > 1)
		return Expr::Group { exprs };

	// Have a single expression left in the list, return it.
	return popExpr().orElse([&]
	{
		// The list was empty, throw an exception.
		throw InvalidExprException("Invalid expression.");
		return nullOpt;
	}).getValue();
}

LSToken LSTokenParser::parseToken(tiny_string& data) const
{
	switch (inplaceRemoveWhitespace(data)[0])
	{
		// Comments.
		case '/': return parseComment(data); break;
		// Directives.
		case '#': return parseDirective(data); break;
		// Expressions.
		default: return parseExpr(data); break;
	}
}

std::list<LSToken> LSTokenParser::parseString(const tiny_string& str)
{
	// Parse the string into a global, implicit block (token list).
	tiny_string data = str;
	return parseBlock(data, true);
}

std::list<LSToken> LSTokenParser::parseFile(const tiny_string& path)
{
	// Parse the file into a global, implicit block (token list).
	auto data = openTextFile(path);
	return parseBlock(data, true);
}
