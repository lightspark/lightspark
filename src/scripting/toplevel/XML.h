/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
class Namespace;
class XMLList;
class XML: public ASObject, public XMLBase
{
friend class XMLList;
public:
	typedef std::vector<_R<XML>> XMLVector;
	typedef std::vector<_R<Namespace>> NSVector;
private:
	_NR<XMLList> childrenlist;
	_NR<XML> parentNode;
	pugi::xml_node_type nodetype;
	bool isAttribute;
	tiny_string nodename;
	tiny_string nodevalue;
	uint32_t nodenamespace_uri;
	uint32_t nodenamespace_prefix;
	_NR<XMLList> attributelist;
	_NR<XMLList> procinstlist;
	NSVector namespacedefs;

	void createTree(const pugi::xml_node &rootnode, bool fromXMLList);
	static void fillNode(XML* node, const pugi::xml_node &srcnode);
	tiny_string toString_priv();
	const char* nodekindString();
	
	bool constructed;
	bool nodesEqual(XML *a, XML *b) const;
	XMLVector getAttributes();
	XMLVector getAttributesByMultiname(const multiname& name, const tiny_string &normalizedName) const;
	XMLVector getValuesByMultiname(_NR<XMLList> nodelist, const multiname& name);
	XMLList* getAllAttributes();
	void getText(XMLVector& ret);
	/*
	 * @param name The name of the wanted children, "*" for all children
	 *
	 */
	void childrenImpl(XMLVector& ret, const tiny_string& name);
	void childrenImpl(XMLVector& ret, uint32_t index);
	uint32_t getNamespacePrefixByURI(uint32_t uri, bool create=false);
	void setLocalName(const tiny_string& localname);
	void setNamespace(uint32_t ns_uri, uint32_t ns_prefix=BUILTIN_STRINGS::EMPTY);
	// Append node or attribute to this. Concatenates adjacent
	// text nodes.
	void appendChild(_R<XML> child);
	void prependChild(_R<XML> child);
	static void normalizeRecursive(XML *node);
	void addTextContent(const tiny_string& str);
	void RemoveNamespace(Namespace *ns);
	void getComments(XMLVector& ret);
	void getprocessingInstructions(XMLVector& ret, tiny_string name);
	void CheckCyclicReference(XML* node);
public:
	XML(Class_base* c);
	XML(Class_base* c,const std::string& str);
	XML(Class_base* c,const pugi::xml_node& _n, XML* parent=NULL, bool fromXMLList=false);
	bool destruct();
	
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(toXMLString);
	ASFUNCTION_ATOM(nodeKind);
	ASFUNCTION_ATOM(child);
	ASFUNCTION_ATOM(children);
	ASFUNCTION_ATOM(childIndex);
	ASFUNCTION_ATOM(contains);
	ASFUNCTION_ATOM(_copy);
	ASFUNCTION_ATOM(attributes);
	ASFUNCTION_ATOM(attribute);
	ASFUNCTION_ATOM(_appendChild);
	ASFUNCTION_ATOM(length);
	ASFUNCTION_ATOM(localName);
	ASFUNCTION_ATOM(name);
	ASFUNCTION_ATOM(_namespace);
	ASFUNCTION_ATOM(_normalize);
	ASFUNCTION_ATOM(descendants);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_hasSimpleContent);
	ASFUNCTION_ATOM(_hasComplexContent);
	ASFUNCTION_ATOM(valueOf);
	ASFUNCTION_ATOM(text);
	ASFUNCTION_ATOM(elements);
	ASFUNCTION_ATOM(parent);
	ASFUNCTION_ATOM(inScopeNamespaces);
	ASFUNCTION_ATOM(addNamespace);
	ASFUNCTION_ATOM(_setLocalName);
	ASFUNCTION_ATOM(_setName);
	ASFUNCTION_ATOM(_setNamespace);
	ASFUNCTION_ATOM(_setChildren);

	ASFUNCTION_ATOM(_getIgnoreComments);
	ASFUNCTION_ATOM(_setIgnoreComments);
	ASFUNCTION_ATOM(_getIgnoreProcessingInstructions);
	ASFUNCTION_ATOM(_setIgnoreProcessingInstructions);
	ASFUNCTION_ATOM(_getIgnoreWhitespace);
	ASFUNCTION_ATOM(_setIgnoreWhitespace);
	ASFUNCTION_ATOM(_getPrettyIndent);
	ASFUNCTION_ATOM(_setPrettyIndent);
	ASFUNCTION_ATOM(_getPrettyPrinting);
	ASFUNCTION_ATOM(_setPrettyPrinting);
	ASFUNCTION_ATOM(_getSettings);
	ASFUNCTION_ATOM(_setSettings);
	ASFUNCTION_ATOM(_getDefaultSettings);
	ASFUNCTION_ATOM(_toJSON);
	ASFUNCTION_ATOM(insertChildAfter);
	ASFUNCTION_ATOM(insertChildBefore);
	ASFUNCTION_ATOM(namespaceDeclarations);
	ASFUNCTION_ATOM(removeNamespace);
	ASFUNCTION_ATOM(comments);
	ASFUNCTION_ATOM(processingInstructions);
	ASFUNCTION_ATOM(_propertyIsEnumerable);
	ASFUNCTION_ATOM(_hasOwnProperty);
	ASFUNCTION_ATOM(_prependChild);
	ASFUNCTION_ATOM(_replace);
	ASFUNCTION_ATOM(setNotification);

	static void buildTraits(ASObject* o){}
	static void sinit(Class_base* c);
	
	static bool getPrettyPrinting();
	static unsigned int getParseMode();
	static XML* createFromString(SystemState *sys, const tiny_string& s, bool usefirstchild=false);
	static XML* createFromNode(const pugi::xml_node& _n, XML* parent=NULL, bool fromXMLList=false);

	const tiny_string getName() const { return nodename;}
	uint32_t getNamespaceURI() const { return nodenamespace_uri;}
	XMLList* getChildrenlist() { return childrenlist ? childrenlist.getPtr() : NULL; }
	
	
	void getDescendantsByQName(const tiny_string& name, uint32_t ns, bool bIsAttribute, XMLVector& ret) const;
	void getElementNodes(const tiny_string& name, XMLVector& foundElements);
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt=NONE);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	bool hasProperty(const multiname& name,bool checkXMLPropsOnly, bool considerDynamic, bool considerPrototype);
	void setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst);
	void setVariableByMultinameIntern(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool replacetext);
	bool deleteVariableByMultiname(const multiname& name);
	static bool isValidMultiname(SystemState *sys, const multiname& name, uint32_t& index);

	void setTextContent(const tiny_string& content);
	tiny_string toString();
	const tiny_string toXMLString_internal(bool pretty=true, uint32_t defaultnsprefix = BUILTIN_STRINGS::EMPTY, const char* indent = "", bool bfirst = true);
	int32_t toInt();
	int64_t toInt64();
	number_t toNumber();
	bool hasSimpleContent() const;
	bool hasComplexContent() const;
	pugi::xml_node_type getNodeKind() const;
	ASObject *getParentNode();
	XML *copy();
	void normalize();
	bool isEqual(ASObject* r);
	uint32_t nextNameIndex(uint32_t cur_index);
	void nextName(asAtom &ret, uint32_t index);
	void nextValue(asAtom &ret, uint32_t index);
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};
}
#endif /* SCRIPTING_TOPLEVEL_XML_H */
