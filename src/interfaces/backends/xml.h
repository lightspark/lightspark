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

#ifndef INTERFACES_BACKENDS_XML_H
#define INTERFACES_BACKENDS_XML_H 1

#include <list>
#include <utility>

namespace lightspark
{

class tiny_string;

class IXMLParser
{
public:
	using XMLAttr = std::pair<tiny_string, tiny_string>;
	using XMLAttrs = std::list<XMLAttr>;

	// Handler for start tags (`<foo>`).
	virtual void handleStart
	(
		const tiny_string& name,
		const XMLAttrs& attrs
	) {}

	// Handler for end tags (`</foo>`).
	virtual void handleEnd(const tiny_string& name) {}

	// Handler for an empty element tag (`<foo/>`).
	virtual void handleEmpty
	(
		const tiny_string& name,
		const XMLAttrs& attrs
	) {}

	// Handler for comments (`<!--...-->`).
	virtual void handleComment(const tiny_string& str) {}

	// Handler for document type definitions (DTDs) (`<!DOCTYPE ...>`).
	virtual void handleDocType(const tiny_string& str) {}

	// Handler for escaped text data.
	virtual void handleText(const tiny_string& str) {}

	// Handler for unescaped text data (`<![CDATA[...]]>`).
	virtual void handleCData(const tiny_string& str) {}

	// Handler for XML declarations (`<?xml ...?>`).
	virtual void handleDecl(const tiny_string& str) {}

	// Handler for processing instructions (`<?...?>`).
	virtual void handlePI(const tiny_string& str) {}

	// Handler for entity references (`&foo;`).
	virtual void handleRef(const tiny_string& str) {}
};

}
#endif /* INTERFACES_BACKENDS_XML_H */
