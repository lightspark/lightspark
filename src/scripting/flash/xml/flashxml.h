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
	XMLDocument* root;
	XMLNode* parent;
	Array* children;
	pugi::xml_node node;
	pugi::xml_document tmpdoc; // used for temproary copying/moving of nodes
	tiny_string toString_priv(pugi::xml_node outputNode);
	pugi::xml_node getParentNode();
	virtual XMLDocument* getRootDoc() { return root; }
	tiny_string getPrefix();
	static bool getNamespaceURI(const pugi::xml_node& n, const tiny_string& prefix, tiny_string& uri);
	static bool getPrefixFromNamespaceURI(const pugi::xml_node& n, const tiny_string& uri, tiny_string& prefix);
	void removeChild(const pugi::xml_node& child);
	void fillChildren();
	void reloadChildren();
	void refreshChildren();
	void fillIDMap(ASObject* o);
public:
	XMLNode(ASWorker* wrk,Class_base* c);
	XMLNode(ASWorker* wrk,Class_base* c, XMLDocument* _r, pugi::xml_node _n, XMLNode* _p);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	static void sinit(Class_base*);
	tiny_string toString();
	bool isEqual(ASObject* r) override;
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
	ASFUNCTION_ATOM(cloneNode);
	ASFUNCTION_ATOM(removeNode);
	ASFUNCTION_ATOM(hasChildNodes);
	ASFUNCTION_ATOM(insertBefore);
	ASFUNCTION_ATOM(prefix);
	ASFUNCTION_ATOM(namespaceURI);
	ASFUNCTION_ATOM(getNamespaceForPrefix);
	ASFUNCTION_ATOM(getPrefixForNamespace);
};

class XMLDocument: public XMLNode, public XMLBase
{
friend class XMLNode;
private:
	pugi::xml_node rootNode;
protected:
	int32_t status; // only needed for AVM1
	ASObject* idmap;
	tiny_string doctypedecl;
	tiny_string xmldecl;
	bool needsActionScript3;
	XMLDocument* getRootDoc() override
	{
		return this;
	}
	void setDecl();
public:
	XMLDocument(ASWorker* wrk,Class_base* c, tiny_string s="");
	int parseXMLImpl(const std::string& str);
	static void sinit(Class_base*);
	void finalize() override;
	bool destruct() override;
	tiny_string toString();
	ASPROPERTY_GETTER_SETTER(bool, ignoreWhite);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(parseXML);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(createElement);
	ASFUNCTION_ATOM(createTextNode);
	ASFUNCTION_ATOM(_idmap);
	ASFUNCTION_ATOM(_docTypeDecl);
	ASFUNCTION_ATOM(_xmlDecl);

	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
};

}
#endif /* SCRIPTING_FLASH_XML_FLASHXML */
