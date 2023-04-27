/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
	pugi::xml_node node;
	tiny_string toString_priv(pugi::xml_node outputNode);
	pugi::xml_node getParentNode();
public:
	XMLNode(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_XMLNODE),root(NullRef),node(nullptr){}
	XMLNode(ASWorker* wrk,Class_base* c, _NR<XMLDocument> _r, pugi::xml_node _n);
	bool destruct()
	{
		root.reset();
		return destructIntern();
	}
	static void sinit(Class_base*);
	tiny_string toString();
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(firstChild);
	ASFUNCTION_ATOM(lastChild);
	ASFUNCTION_ATOM(childNodes);
	ASFUNCTION_ATOM(attributes);
	ASFUNCTION_ATOM(_getNodeType);
	ASFUNCTION_ATOM(_getNodeName);
	ASFUNCTION_ATOM(_setNodeName);
	ASFUNCTION_ATOM(_getNodeValue);
	ASFUNCTION_ATOM(_getLocalName);
	ASFUNCTION_ATOM(nextSibling);
	ASFUNCTION_ATOM(parentNode);
	ASFUNCTION_ATOM(previousSibling);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(appendChild);
};

class XMLDocument: public XMLNode, public XMLBase
{
friend class XMLNode;
private:
	pugi::xml_node rootNode;
protected:
	int32_t status; // only needed for AVM1
	bool needsActionScript3;
public:
	XMLDocument(ASWorker* wrk,Class_base* c, tiny_string s="");
	int parseXMLImpl(const std::string& str);
	static void sinit(Class_base*);
	tiny_string toString();
	ASPROPERTY_GETTER_SETTER(bool, ignoreWhite);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(parseXML);
	ASFUNCTION_ATOM(firstChild);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(createElement);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk);
};

}
#endif /* SCRIPTING_FLASH_XML_FLASHXML */
