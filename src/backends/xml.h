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

#ifndef BACKENDS_XML_H
#define BACKENDS_XML_H 1

#include <cstdint>
#include <list>
#include <utility>

#include "tiny_string.h"
#include "utils/optional.h"
#include "utils/span.h"
#include "utils/visitor.h"

namespace lightspark
{

class IXMLParser;

using XMLAttr = std::pair<tiny_string, tiny_string>;

class XMLStartTag
{
private:
	tiny_string name;
	std::list<XMLAttr> attrs;
public:
	XMLStartTag
	(
		const tiny_string& _name,
		const std::list<tiny_string>& _attrs
	) : name(_name), attrs(_attrs) {}

	template<typename Iter>
	XMLStartTag
	(
		const tiny_string& _name,
		Iter attrBegin,
		Iter attrEnd
	) : name(_name), attrs(attrBegin, attrEnd) {}

	XMLStartTag
	(
		const tiny_string& _name,
		Span<tiny_string> _attrs
	) : XMLStartTag(_name, _attrs.begin(), _attrs.end()) {}

	const tiny_string& getName() { return name; }
	const std::list<tiny_string>& getAttrs() { return attrs; }
};

class XMLEndTag : public tiny_string {};
class XMLEmptyTag : public XMLStartTag {};
class XMLText : public tiny_string {};
class XMLCData : public tiny_string {};
class XMLComment : public tiny_string {};
class XMLDecl : public tiny_string {};
class XMLPI : public tiny_string {};
class XMLDocType : public tiny_string {};
class XMLEntityRef : public tiny_string {};
struct XMLEndOfFile {};

class XMLEvent
{
public:
	enum class Type
	{
		StartTag,
		EndTag,
		EmptyTag,
		Text,
		CData,
		Comment,
		Decl,
		PI,
		DocType,
		EntityRef,
		EndOfFile,
	};
private:
	Type type;
	union
	{
		XMLStartTag start;
		tiny_string str;
	};
public:
	XMLEvent() : type(Type::EndOfFile) {}
	XMLEvent(const Type& _type, const tiny_string& _str);
	XMLEvent(const XMLStartTag& _start, bool isEmpty = false) : type
	(
		isEmpty ?
		Type::EmptyTag :
		Type::StartTag
	), start(_start) {}

	~XMLEvent();
	bool isStartEvent() const { return is<XMLStartTag>(); }
	bool isStrEvent() const { return is<tiny_string>(); }
	bool isEOF() const { return type == Type::EndOfFile; }
	const Type& getType() const { return type; }
	Optional<const XMLStartTag&> getStart() const;
	Optional<const tiny_string&> getStr() const;
	template<typename V>
	constexpr auto visit(V&& visitor) const;
	template<typename T>
	constexpr bool is() const
	{
		return visit(makeVisitor
		(
			[](const T&) { return true; }
			[](const auto&) { return false; }
		));
	}
};

class XMLParser
{
private:
	tiny_string data;
	CharIterator pos;
	IXMLParser* parser;
	bool ignoreWhite;
	bool ignoreCase;

	CharIterator trimWhitespace(const CharIterator& end) const;
	XMLEvent parseDocType();
	XMLEvent parseXMLDecl();
	XMLEvent parsePI();
	XMLEvent parseRef();
	XMLEvent parseComment(CharIterator it);
	XMLEvent parseCData(CharIterator it);
	tiny_string parseNodeWithTerminator(const tiny_string& str);
	XMLAttr parseAttr();
	void throwIf(bool flag);
	void throwIfEmptyAfterWhitespace();
public:
	XMLParser
	(
		const tiny_string& _data,
		IXMLParser* _parser = nullptr
	) : XMLParser(_data, false, false, _parser) {}

	XMLParser
	(
		const tiny_string& _data,
		bool _ignoreWhite,
		bool _ignoreCase,
		IXMLParser* _parser = nullptr
	) :
	data(_data),
	pos(data.begin()),
	parser(_parser),
	ignoreWhite(_ignoreWhite),
	ignoreCase(_ignoreCase) {}

	bool getIgnoreWhite() const { return ignoreWhite; }
	void setIgnoreWhite(bool flag) { ignoreWhite = flag; }
	bool getIgnoreCase() const { return ignoreCase; }
	void setIgnoreCase(bool flag) { ignoreCase = flag; }

	void read();
	template<typename V>
	void read(V&& visitor);
	XMLEvent readEvent();
	Optional<XMLEvent> tryReadEvent();
};

template<typename V>
void XMLParser::read(V&& visitor)
{
	for (auto ev = readEvent(); !ev.isEOF(); ev = readEvent())
		ev.visit(visitor);
}

template<typename V>
constexpr auto XMLEvent::visit(V&& visitor) const
{
	switch (type)
	{
		case Type::StartTag: return visitor(start); break;
		case Type::EndTag: return visitor(XMLEndTag(str)); break;
		case Type::EmptyTag: return visitor(XMLEmptyTag(start)); break;
		case Type::Text: return visitor(XMLText(str)); break;
		case Type::CData: return visitor(XMLCData(str)); break;
		case Type::Comment: return visitor(XMLComment(str)); break;
		case Type::Decl: return visitor(XMLDecl(str)); break;
		case Type::PI: return visitor(XMLPI(str)); break;
		case Type::DocType: return visitor(XMLDocType(str)); break;
		case Type::EntityRef: return visitor(XMLEntityRef(str)); break;
		case Type::EndOfFile: return visitor(XMLEndOfFile {}); break;
	}
}

}
#endif /* BACKENDS_XML_H */
