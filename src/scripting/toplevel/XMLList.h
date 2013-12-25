/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013 Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SCRIPTING_TOPLEVEL_XMLLIST_H
#define SCRIPTING_TOPLEVEL_XMLLIST_H 1
#include "asobject.h"
#include "scripting/toplevel/XML.h"
#include <libxml/tree.h>
#include <libxml++/parsers/domparser.h>

namespace lightspark
{
class XMLList: public ASObject
{
friend class XML;
public:
	typedef std::vector<_R<XML>, reporter_allocator<_R<XML>>> XMLListVector;
private:
	XMLListVector nodes;
	bool constructed;
	XMLList* targetobject;
	multiname targetproperty;

	tiny_string toString_priv();
	void buildFromString(const std::string& str);
	std::string extractXMLDeclaration(const std::string& xml, std::string& xmldecl_out);
	void appendSingleNode(ASObject *x);
	void replace(unsigned int i, ASObject *x, const XML::XMLVector& retnodes, CONST_ALLOWED_FLAG allowConst);
	void getTargetVariables(const multiname& name, XML::XMLVector& retnodes);
public:
	XMLList(Class_base* c);
	/*
	   Special constructor to build empty XMLList out of AS code
	*/
	XMLList(Class_base* cb,bool c);
	XMLList(Class_base* c,const XML::XMLVector& r);
	XMLList(Class_base* c,const XML::XMLVector& r,XMLList* targetobject,const multiname& targetproperty);
	XMLList(Class_base* c,const std::string& str);
	void finalize();
	static void buildTraits(ASObject* o){}
	static void sinit(Class_base* c);
	ASFUNCTION(_constructor);
	ASFUNCTION(_getLength);
	ASFUNCTION(attribute);
	ASFUNCTION(attributes);
	ASFUNCTION(_appendChild);
	ASFUNCTION(child);
	ASFUNCTION(children);
	ASFUNCTION(childIndex);
	ASFUNCTION(contains);
	ASFUNCTION(copy);
	ASFUNCTION(_hasSimpleContent);
	ASFUNCTION(_hasComplexContent);
	ASFUNCTION(_toString);
	ASFUNCTION(toXMLString);
	ASFUNCTION(generator);
	ASFUNCTION(descendants);
	ASFUNCTION(elements);
	ASFUNCTION(parent);
	ASFUNCTION(valueOf);
	ASFUNCTION(text);
	ASFUNCTION(_namespace);
	ASFUNCTION(name);
	ASFUNCTION(nodeKind);
	ASFUNCTION(_normalize);
	ASFUNCTION(localName);
	ASFUNCTION(inScopeNamespaces);
	ASFUNCTION(addNamespace);
	ASFUNCTION(_setChildren);
	ASFUNCTION(_setLocalName);
	ASFUNCTION(_setName);
	ASFUNCTION(_setNamespace);
	ASFUNCTION(insertChildAfter);
	ASFUNCTION(insertChildBefore);
	ASFUNCTION(namespaceDeclarations);
	ASFUNCTION(removeNamespace);
	ASFUNCTION(comments);
	ASFUNCTION(processingInstructions);
	ASFUNCTION(_propertyIsEnumerable);
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	bool deleteVariableByMultiname(const multiname& name);
	void getDescendantsByQName(const tiny_string& name, const tiny_string& ns, bool bIsAttribute, XML::XMLVector& ret);
	_NR<XML> convertToXML() const;
	bool hasSimpleContent() const;
	bool hasComplexContent() const;
	void append(_R<XML> x);
	void append(_R<XMLList> x);
	tiny_string toString();
	tiny_string toXMLString_internal(bool pretty=true);
	int32_t toInt();
	bool isEqual(ASObject* r);
	uint32_t nextNameIndex(uint32_t cur_index);
	_R<ASObject> nextName(uint32_t index);
	_R<ASObject> nextValue(uint32_t index);
	_R<XML> reduceToXML() const;
	void appendNodesTo(XML *dest) const;
	void normalize();
	void clear();
	XMLList* getTargetObject() { return targetobject; }
};
}
#endif /* SCRIPTING_TOPLEVEL_XMLLIST_H */
