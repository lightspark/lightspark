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

#ifndef UTILS_TOKEN_PARSER_TOKEN_PARSER_H
#define UTILS_TOKEN_PARSER_TOKEN_PARSER_H 1

#include <exception>
#include <list>

#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/visitor.h>

#include "utils/token_parser/token.h"

using Expr = LSToken::Expr;

using namespace lightspark;

// TODO: Move this into the main codebase, at some point.

class LSTokenParserException : public std::exception
{
private:
	tiny_string reason;
public:
	LSTokenParserException(const tiny_string& _reason) : reason(_reason) {}

	const char* what() const noexcept override { return reason.raw_buf(); }
	const tiny_string& getReason() const noexcept { return reason; }
};

class InvalidExprException : public LSTokenParserException
{
public:
	InvalidExprException(const tiny_string& reason) : LSTokenParserException(reason) {}
};

class LSTokenParser
{
using Expr = LSToken::Expr;
private:
	mutable bool inDirective { false };
	mutable bool isExprDone { false };
	mutable size_t listDepth { 0 };
	mutable size_t blockDepth { 0 };
	mutable size_t opDepth { 0 };
	tiny_string& inplaceRemoveWhitespace(tiny_string& str) const;

	// Value parsing.
	tiny_string parseString(tiny_string& data) const;
	uint32_t parseChar(tiny_string& data) const;
	Optional<bool> parseBool(tiny_string& data) const;
	Expr::Value::Number parseNumber(tiny_string& data) const;
	std::list<Expr> parseList(tiny_string& data) const;

	// Expression parsing.
	Expr::Op parseOp(tiny_string& data, const Optional<Expr>& lhs) const;
	Expr::Value parseValue(tiny_string& data) const;
	tiny_string parseIdent(tiny_string& data) const;
	Expr::Block parseBlock(tiny_string& data, bool implicit = false) const;

	// Token parsing.
	LSToken::Comment parseComment(tiny_string& data) const;
	LSToken::Directive parseDirective(tiny_string& data) const;
	LSToken::Expr parseExpr(tiny_string& data) const;
	LSToken parseToken(tiny_string& data) const;
public:
	std::list<LSToken> parseString(const tiny_string& str);
	std::list<LSToken> parseFile(const tiny_string& path);
};

#endif /* UTILS_TOKEN_PARSER_TOKEN_PARSER_H */
