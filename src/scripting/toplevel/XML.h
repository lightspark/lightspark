/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef XML_H
#define XML_H
#include "asobject.h"
#include <libxml/tree.h>
#include <libxml++/parsers/domparser.h>

namespace lightspark
{
class XMLList;
class XML: public ASObject
{
friend class XMLList;
private:
	//The parser will destroy the document and all the childs on destruction
	xmlpp::DomParser parser;
	//Pointer to the root XML element, the one that owns the parser that created this node
	_NR<XML> root;
	//The node this object represent
	xmlpp::Node* node;
	static void recursiveGetDescendantsByQName(_R<XML> root, xmlpp::Node* node, const tiny_string& name, const tiny_string& ns, 
			std::vector<_R<XML>>& ret);
	tiny_string toString_priv();
	void buildFromString(const std::string& str);
	bool constructed;
	bool nodesEqual(xmlpp::Node *a, xmlpp::Node *b) const;
	XMLList* getAllAttributes();
	void getText(std::vector<_R<XML>> &ret);
	std::string parserQuirks(const std::string& str);
	std::string quirkCData(const std::string& str);
	std::string quirkXMLDeclarationInMiddle(const std::string& str);
	bool ignoreComments;
	bool ignoreProcessingInstructions;
	bool ignoreWhitespace;
	uint32_t prettyIndent;
	bool prettyPrinting;
public:
	XML();
	XML(const std::string& str);
	XML(_R<XML> _r, xmlpp::Node* _n);
	XML(xmlpp::Node* _n);
	void finalize();
	ASFUNCTION(_constructor);
	ASFUNCTION(_toString);
	ASFUNCTION(toXMLString);
	ASFUNCTION(nodeKind);
	ASFUNCTION(children);
	ASFUNCTION(attributes);
	ASFUNCTION(attribute);
	ASFUNCTION(appendChild);
	ASFUNCTION(localName);
	ASFUNCTION(name);
	ASFUNCTION(descendants);
	ASFUNCTION(generator);
	ASFUNCTION(_hasSimpleContent);
	ASFUNCTION(_hasComplexContent);
	ASFUNCTION(valueOf);
	ASFUNCTION(text);
	ASFUNCTION(elements);
	static void buildTraits(ASObject* o){};
	static void sinit(Class_base* c);
	void getDescendantsByQName(const tiny_string& name, const tiny_string& ns, std::vector<_R<XML> >& ret);
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic);
	tiny_string toString();
	void toXMLString_priv(xmlBufferPtr buf);
	bool hasSimpleContent() const;
	bool hasComplexContent() const;
        xmlElementType getNodeKind() const;
	bool isEqual(ASObject* r);
	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
};
}
#endif