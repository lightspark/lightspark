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

void XMLParser::read()
{
	if (parser == nullptr)
		throw ParseException("No parser interface provided.");

	read(makeVisitor
	(
		[&](const XMLStartTag& start)
		{
			parser->handleStart(start);
		},
		[&](const XMLEmptyTag& empty)
		{
			parser->handleEmpty(empty);
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
