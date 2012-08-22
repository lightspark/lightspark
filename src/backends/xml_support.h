/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef BACKENDS_XML_SUPPORT_H
#define BACKENDS_XML_SUPPORT_H 1

#include <libxml/tree.h>
#include <libxml++/parsers/domparser.h>
#include <libxml++/exceptions/internal_error.h>
//For xmlCreateFileParserCtxt().
#include <libxml/parserInternals.h>

namespace lightspark
{

#ifdef XMLPP_2_35_1
//Create a utility derived class from xmlpp::DomParser since we want to use the recovery mode
class RecoveryDomParser:public xmlpp::DomParser
{
public:
	void parse_memory_raw(const unsigned char* contents, size_type bytes_count);
};
//Also create a utility derived class from xmlpp::Document to access the protected constructor
class RecoveryDocument: public xmlpp::Document
{
public:
	RecoveryDocument(_xmlDoc* d);
};
typedef RecoveryDomParser LSDomParser;
#else
typedef xmlpp::DomParser LSDomParser;
#endif

/*
 * Base class for both XML and XMLNode
 */
class XMLBase
{
protected:
	//The parser will destroy the document and all the childs on destruction
	LSDomParser parser;
	xmlpp::Node* buildFromString(const std::string& str,
				     const std::string& default_ns=std::string());
	void addDefaultNamespace(xmlpp::Element *root, const std::string& default_ns);
	void addDefaultNamespaceRecursive(xmlNodePtr node, xmlNsPtr ns);
	// Set the root to be a copy of src. If src is a text node,
	// create a new element node with the same content.
	xmlpp::Node* buildCopy(const xmlpp::Node* node);
	static std::string parserQuirks(const std::string& str);
	static std::string quirkCData(const std::string& str);
	static std::string quirkXMLDeclarationInMiddle(const std::string& str);
};

};

#endif /* BACKENDS_XML_SUPPORT_H */
