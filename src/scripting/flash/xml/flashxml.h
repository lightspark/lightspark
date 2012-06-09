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

#ifndef _FLASH_XML_H
#define _FLASH_XML_H

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
public:
	XMLNode(Class_base* c):ASObject(c),root(NullRef),node(NULL){}
	XMLNode(Class_base* c, _R<XMLDocument> _r, xmlpp::Node* _n);
	void finalize();
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(firstChild);
	ASFUNCTION(childNodes);
	ASFUNCTION(attributes);
	ASFUNCTION(_getNodeType);
	ASFUNCTION(_getNodeName);
	ASFUNCTION(_getNodeValue);
};

class XMLDocument: public XMLNode, public XMLBase
{
friend class XMLNode;
private:
	xmlpp::Node* rootNode;
public:
	XMLDocument(Class_base* c):XMLNode(c),rootNode(NULL){}
	void parseXMLImpl(const std::string& str);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(parseXML);
	ASFUNCTION(firstChild);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};

};
#endif
