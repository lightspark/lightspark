/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_TOPLEVEL_XML_H
#define SCRIPTING_TOPLEVEL_XML_H 1
#include "asobject.h"
#include "backends/xml_support.h"

namespace lightspark
{
class XMLList;
class XML: public ASObject, public XMLBase
{
friend class XMLList;
public:
	typedef std::vector<_R<XML>> XMLVector;
private:
	//Pointer to the root XML element, the one that owns the parser that created this node
	_NR<XML> root;
	//The node this object represent
	xmlpp::Node* node;
	static void recursiveGetDescendantsByQName(_R<XML> root, xmlpp::Node* node, const tiny_string& name, const tiny_string& ns, 
			XMLVector& ret);
	tiny_string toString_priv();
	bool constructed;
	bool nodesEqual(xmlpp::Node *a, xmlpp::Node *b) const;
	XMLVector getAttributes(const tiny_string& name="*",
				const tiny_string& namespace_uri="*");
	XMLList* getAllAttributes();
	void getText(XMLVector& ret);
	_NR<XML> getRootNode();
	bool ignoreComments;
	bool ignoreProcessingInstructions;
	bool ignoreWhitespace;
	uint32_t prettyIndent;
	bool prettyPrinting;
	/*
	 * @param name The name of the wanted children, "*" for all children
	 *
	 */
	void childrenImpl(XMLVector& ret, const tiny_string& name);
	void childrenImpl(XMLVector& ret, uint32_t index);
	tiny_string getNamespacePrefixByURI(const tiny_string& uri, bool create=false);
        void setLocalName(const tiny_string& localname);
        void setNamespace(const tiny_string& ns_uri, const tiny_string& ns_prefix="");
public:
	XML(Class_base* c);
	XML(Class_base* c,const std::string& str);
	XML(Class_base* c,_R<XML> _r, xmlpp::Node* _n);
	XML(Class_base* c,xmlpp::Node* _n);
	void finalize();
	ASFUNCTION(_constructor);
	ASFUNCTION(_toString);
	ASFUNCTION(toXMLString);
	ASFUNCTION(nodeKind);
	ASFUNCTION(child);
	ASFUNCTION(children);
	ASFUNCTION(childIndex);
	ASFUNCTION(contains);
	ASFUNCTION(_copy);
	ASFUNCTION(attributes);
	ASFUNCTION(attribute);
	ASFUNCTION(appendChild);
	ASFUNCTION(length);
	ASFUNCTION(localName);
	ASFUNCTION(name);
	ASFUNCTION(_namespace);
	ASFUNCTION(descendants);
	ASFUNCTION(generator);
	ASFUNCTION(_hasSimpleContent);
	ASFUNCTION(_hasComplexContent);
	ASFUNCTION(valueOf);
	ASFUNCTION(text);
	ASFUNCTION(elements);
	ASFUNCTION(parent);
	ASFUNCTION(inScopeNamespaces);
	ASFUNCTION(addNamespace);
	ASFUNCTION(_setLocalName);
	ASFUNCTION(_setName);
	ASFUNCTION(_setNamespace);
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	void getDescendantsByQName(const tiny_string& name, const tiny_string& ns, XMLVector& ret);
	void getElementNodes(const tiny_string& name, XMLVector& foundElements);
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst);
	tiny_string toString();
	void toXMLString_priv(xmlBufferPtr buf);
	bool hasSimpleContent() const;
	bool hasComplexContent() const;
        xmlElementType getNodeKind() const;
	ASObject *getParentNode();
	XML *copy() const;
	bool isEqual(ASObject* r);
	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};
}
#endif /* SCRIPTING_TOPLEVEL_XML_H */
