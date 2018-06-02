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
	void buildFromString(const tiny_string& str);
	std::string extractXMLDeclaration(const std::string& xml, std::string& xmldecl_out);
	void appendSingleNode(ASObject *x);
	void replace(unsigned int i, ASObject *x, const XML::XMLVector& retnodes, CONST_ALLOWED_FLAG allowConst, bool replacetext);
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
	bool destruct();
	
	static void buildTraits(ASObject* o){}
	static void sinit(Class_base* c);
	static XMLList* create(SystemState *sys, const XML::XMLVector& r, XMLList *targetobject, const multiname &targetproperty);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(_getLength);
	ASFUNCTION_ATOM(attribute);
	ASFUNCTION_ATOM(attributes);
	ASFUNCTION_ATOM(_appendChild);
	ASFUNCTION_ATOM(child);
	ASFUNCTION_ATOM(children);
	ASFUNCTION_ATOM(childIndex);
	ASFUNCTION_ATOM(contains);
	ASFUNCTION_ATOM(copy);
	ASFUNCTION_ATOM(_hasSimpleContent);
	ASFUNCTION_ATOM(_hasComplexContent);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(toXMLString);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(descendants);
	ASFUNCTION_ATOM(elements);
	ASFUNCTION_ATOM(parent);
	ASFUNCTION_ATOM(valueOf);
	ASFUNCTION_ATOM(text);
	ASFUNCTION_ATOM(_namespace);
	ASFUNCTION_ATOM(name);
	ASFUNCTION_ATOM(nodeKind);
	ASFUNCTION_ATOM(_normalize);
	ASFUNCTION_ATOM(localName);
	ASFUNCTION_ATOM(inScopeNamespaces);
	ASFUNCTION_ATOM(addNamespace);
	ASFUNCTION_ATOM(_setChildren);
	ASFUNCTION_ATOM(_setLocalName);
	ASFUNCTION_ATOM(_setName);
	ASFUNCTION_ATOM(_setNamespace);
	ASFUNCTION_ATOM(insertChildAfter);
	ASFUNCTION_ATOM(insertChildBefore);
	ASFUNCTION_ATOM(namespaceDeclarations);
	ASFUNCTION_ATOM(removeNamespace);
	ASFUNCTION_ATOM(comments);
	ASFUNCTION_ATOM(processingInstructions);
	ASFUNCTION_ATOM(_propertyIsEnumerable);
	ASFUNCTION_ATOM(_prependChild);
	ASFUNCTION_ATOM(_hasOwnProperty);
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt);
	void setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst);
	void setVariableByMultinameIntern(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool replacetext);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	bool deleteVariableByMultiname(const multiname& name);
	void getDescendantsByQName(const tiny_string& name, uint32_t ns, bool bIsAttribute, XML::XMLVector& ret);
	_NR<XML> convertToXML() const;
	bool hasSimpleContent() const;
	bool hasComplexContent() const;
	void append(_R<XML> x);
	void append(_R<XMLList> x);
	void prepend(_R<XML> x);
	void prepend(_R<XMLList> x);
	tiny_string toString();
	tiny_string toXMLString_internal(bool pretty=true);
	int32_t toInt();
	int64_t toInt64();
	number_t toNumber();
	bool isEqual(ASObject* r);
	uint32_t nextNameIndex(uint32_t cur_index);
	void nextName(asAtom &ret, uint32_t index);
	void nextValue(asAtom &ret, uint32_t index);
	_R<XML> reduceToXML() const;
	void appendNodesTo(XML *dest) const;
	void prependNodesTo(XML *dest) const;
	void normalize();
	void clear();
	void removeNode(XML* node);
	XMLList* getTargetObject() { return targetobject; }
};
}
#endif /* SCRIPTING_TOPLEVEL_XMLLIST_H */
