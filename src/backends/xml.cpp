/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#include <cassert>

#include "backends/xml.h"
#include "interfaces/backends/xml.h"

using namespace lightspark;

XMLEvent::XMLEvent
(
	const Type& _type,
	const tiny_string& _str
) : type(_type), str(_str)
{
	assert(isStrEvent());
}

XMLEvent::~XMLEvent()
{
	switch (type)
	{
		case Type::StartTag:
		case Type::EmptyTag:
			delete start;
			break;
		case Type::EndTag:
		case Type::Text:
		case Type::CData:
		case Type::Comment:
		case Type::Decl:
		case Type::PI:
		case Type::DocType:
		case Type::EntityRef:
			delete str;
			break;
		default:
			break;
	}
}

Optional<const XMLStartTag&> XMLEvent::getStart() const
{
	if (isStartEvent())
		return makeOptionalRef(start);
	return {};
}

Optional<const tiny_string&> XMLEvent::getStr() const
{
	if (isStrEvent())
		return makeOptionalRef(str);
	return {};
}

CharIterator XMLParser::trimWhitespace(const CharIterator& end) const
{
	return std::find_if_not(pos, end, [](uint32_t ch)
	{
		return
		(
			ch == '\t' ||
			ch == '\r' ||
			ch == '\n' ||
			ch == ' '
		);
	});
}

XMLEvent XMLParser::parseDocType()
{
	CharIterator end;
	auto it = pos;
	for (size_t i = 1; i; i += std::count
	(
		it,
		end,
		'>'
	) - 1, it = std::next(end))
	{
		end = data.findIter('>', it);
		throwIf(end == data.end());
	}

	auto str = data.substr(pos, end);
	pos = std::next(end);
	return XMLEvent(XMLEvent::Type::DocType, str);
}

XMLEvent XMLParser::parseXMLDecl()
{
	auto str = parseNodeWithTerminator("?>");
	throwIf(str.empty());

	return XMLEvent(XMLEvent::Type::Decl, str);
}

XMLEvent XMLParser::parsePI()
{
	auto str = parseNodeWithTerminator("?>");
	throwIf(str.empty());

	return XMLEvent(XMLEvent::Type::PI, str);
}

XMLEvent XMLParser::parseRef()
{
	auto str = parseNodeWithTerminator(";");
	throwIf(str.empty());

	return XMLEvent(XMLEvent::Type::EntityRef, str);
}

XMLEvent XMLParser::parseComment(CharIterator it)
{
	pos = it;
	auto str = parseNodeWithTerminator("-->");
	throwIf(str.empty());

	return XMLEvent(XMLEvent::Type::Comment, str);
}

XMLEvent XMLParser::parseCData(CharIterator it)
{
	pos = it;
	auto str = parseNodeWithTerminator("]]>");
	throwIf(str.empty());

	return XMLEvent(XMLEvent::Type::CData, str);
}

tiny_string XMLParser::parseNodeWithTerminator(const tiny_string& str)
{
	auto it = pos;
	auto end = data.findIter("]]>", it);

	if (end == data.end())
		return "";

	pos = std::next(end, str.numChars());
	return data.substr(it, end);
}

XMLAttr XMLParser::parseAttr()
{
	auto end = data.findFirstIter("\t\r\n >=", pos);
	throwIf(end == data.end() || pos == end);

	auto attrName = data.substr(pos, end);
	pos = end;

	throwIfEmptyAfterWhitespace();
	throwIf(*pos++ != '=');
	throwIfEmptyAfterWhitespace();
	throwIf(*pos != '"' && *pos != '\'');

	for
	(
		end = pos;
		end != data.end() && *std::prev(end) != '\\';
		end = str.findIter(*pos, ++end)
	);

	throwIf(end == data.end());
	auto attrValue = data.substr(++pos, end);
	pos = ++end;
	return std::make_pair(attrName, attrValue);

}

void XMLParser::throwIf(bool flag)
{
	if (!flag)
		return;
	throw XMLException(XMLError::UnterminatedElem);
}

void XMLParser::throwIfEmptyAfterWhitespace()
{
	throwIf(trimWhitespace(data.end()) != data.end());
}

void XMLParser::read()
{
	if (parser == nullptr)
		throw ParseException("No parser interface provided.");

	read(makeVisitor
	(
		[&](const XMLStartTag& start)
		{
			parser->handleStart(start.getName(), start.getAttrs());
		},
		[&](const XMLEmptyTag& empty)
		{
			parser->handleEmpty(empty.getName(), empty.getAttrs());
		},
		[&](const XMLComment& comment)
		{
			parser->handleComment(comment);
		},
		[&](const XMLDocType& docType)
		{
			parser->handleDocType(docType);
		},
		[&](const XMLEndTag& end) { parser->handleEnd(end); },
		[&](const XMLText& text) { parser->handleText(text); },
		[&](const XMLCData& cdata) { parser->handleCData(cdata); },
		[&](const XMLDecl& decl) { parser->handleDecl(decl); },
		[&](const XMLPI& pi) { parser->handlePI(pi); },
		[&](const XMLEntityRef& ref) { parser->handleRef(ref); },
	));

	parser->handleEOF();
}

XMLEvent XMLParser::readEvent()
{
	using EventType = XMLEvent::Type;
	constexpr auto npos = tiny_string::npos;

	if (pos == data.end())
		return XMLEvent();

	if (*pos == '&')
		return parseRef();
	if (*pos != '<')
	{
		findFirstInv
		auto textEnd = data.findIter(pos, '<');
		if (ignoreWhite)
			pos = trimWhitespace(textEnd);
		auto text = data.substr(pos, textEnd);
		pos = textEnd;
		return XMLEvent(EventType::Text, text);
	}

	if (data.containsWithCase("!DOCTYPE", ++pos, ignoreCase))
		return parseDocType();
	else if (data.containsWithCase("?xml", pos, ignoreCase))
		return parseXMLDecl();
	else if (*pos == '?')
		return parsePI();

	auto it = data.findIterWithCase("!--", pos, ignoreCase);
	if (it != data.end())
		return parseComment(it);

	it = data.findIterWithCase("![CDATA[", pos, ignoreCase);
	if (it != data.end())
		return parseCData(it);

	// Parse a normal tag.
	bool isClosing = *pos == '/';
	pos = std::next(pos, isClosing);

	// NOTE: These are terminiators for the tag name, not the tag.
	auto endName = data.findFirstIter("\t\r\n >", pos);
	throwIf(endName == data.end());

	endName = std::prev(endName, data.substr
	(
		std::prev(endName),
		2
	) == "/>");

	throwIf(std::distance(pos, endName) >= 0);

	auto tagName = data.substr(pos, endName);
	if (isClosing)
	{
		throwIf(!data.contains('>', endName))
		return XMLEvent(EventType::EndTag, tagName);
	}

	pos = endName;
	throwIfEmptyAfterWhitespace();

	bool isEmpty = false;
	std::list<XMLAttr> attrs;
	for (; pos != data.end() && *pos != '>';)
	{
		isEmpty = std::distance
		(
			pos,
			data.end()
		) > 1 && data.substr(pos, 2) == "/>";
		if (isEmpty)
			break;

		// NOTE: `parseAttr()` advances `pos`.
		attrs.push_back(parseAttr());
		throwIfEmptyAfterWhitespace();
	}

	return XMLEvent(XMLStartTag(tagName, attrs), isEmpty);
}

Optional<XMLEvent> XMLParser::tryReadEvent()
{
	try
	{
		return readEvent();
	}
	catch (...)
	{
		return {};
	}
}
