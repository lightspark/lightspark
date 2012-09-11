/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_FLASH_XML_FLASHXML_H
#define SCRIPTING_FLASH_XML_FLASHXML_H 1

#include "compat.h"
#include "asobject.h"
#include "smartrefs.h"
#include "backends/xml_support.h"

namespace lightspark
{

class XMLDocument;

class XMLNode: public ASObject
{
friend class XML;
protected:
	_NR<XMLDocument> root;
	xmlpp::Node* node;
	tiny_string toString_priv(xmlpp::Node *outputNode);
public:
	XMLNode(Class_base* c):ASObject(c),root(NullRef),node(NULL){}
	XMLNode(Class_base* c, _R<XMLDocument> _r, xmlpp::Node* _n);
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	tiny_string toString();
	ASFUNCTION(_constructor);
	ASFUNCTION(firstChild);
	ASFUNCTION(childNodes);
	ASFUNCTION(attributes);
	ASFUNCTION(_getNodeType);
	ASFUNCTION(_getNodeName);
	ASFUNCTION(_getNodeValue);
	ASFUNCTION(_toString);
};

class XMLDocument: public XMLNode, public XMLBase
{
friend class XMLNode;
private:
	xmlpp::Node* rootNode;
public:
	XMLDocument(Class_base* c, tiny_string s="");
	void parseXMLImpl(const std::string& str);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	tiny_string toString();
	ASFUNCTION(_constructor);
	ASFUNCTION(parseXML);
	ASFUNCTION(firstChild);
	ASFUNCTION(_toString);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};

};
#endif /* SCRIPTING_FLASH_XML_FLASHXML */
