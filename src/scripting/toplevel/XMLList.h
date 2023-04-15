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
	typedef std::vector<_NR<XML>, reporter_allocator<_NR<XML>>> XMLListVector;
private:
	XMLListVector nodes;
	bool constructed;
	XMLList* targetobject;
	multiname targetproperty;

	tiny_string toString_priv();
	void buildFromString(ASWorker* wrk, const tiny_string& str);
	std::string extractXMLDeclaration(const std::string& xml, std::string& xmldecl_out);
	bool appendSingleNode(asAtom x);
	void replace(unsigned int i, asAtom x, const XML::XMLVector& retnodes, CONST_ALLOWED_FLAG allowConst, bool replacetext, bool* alreadyset, ASWorker* wrk);
	void getTargetVariables(const multiname& name, XML::XMLVector& retnodes);
public:
	XMLList(ASWorker* wrk,Class_base* c);
	/*
	   Special constructor to build empty XMLList out of AS code
	*/
	XMLList(ASWorker* wrk,Class_base* cb,bool c);
	XMLList(ASWorker* wrk,Class_base* c,const XML::XMLVector& r);
	XMLList(ASWorker* wrk,Class_base* c,const XML::XMLVector& r,XMLList* targetobject,const multiname& targetproperty);
	XMLList(ASWorker* wrk,Class_base* c,const std::string& str);
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	
	static void sinit(Class_base* c);
	static XMLList* create(ASWorker* wrk, const XML::XMLVector& r, XMLList *targetobject, const multiname &targetproperty);
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
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	GET_VARIABLE_RESULT getVariableByInteger(asAtom &ret, int index, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, lightspark::ASWorker* wrk) override;
	void setVariableByInteger(int index, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk) override;
	multiname *setVariableByMultinameIntern(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool replacetext, bool* alreadyset, ASWorker* wrk);
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk) override;
	bool deleteVariableByMultiname(const multiname& name, ASWorker* wrk) override;
	void getDescendantsByQName(const multiname& name, XML::XMLVector& ret);
	_NR<XML> convertToXML() const;
	bool hasSimpleContent() const;
	bool hasComplexContent() const;
	void append(_NR<XML> x);
	void append(_NR<XMLList> x);
	void prepend(_NR<XML> x);
	void prepend(_NR<XMLList> x);
	tiny_string toString();
	tiny_string toXMLString_internal(bool pretty=true);
	int32_t toInt() override;
	int64_t toInt64() override;
	number_t toNumber() override;
	number_t toNumberForComparison() override;
	bool isEqual(ASObject* r) override;
	uint32_t nextNameIndex(uint32_t cur_index) override;
	void nextName(asAtom &ret, uint32_t index) override;
	void nextValue(asAtom &ret, uint32_t index) override;
	_NR<XML> reduceToXML() const;
	void appendNodesTo(XML *dest) const;
	void prependNodesTo(XML *dest) const;
	void normalize();
	void clear();
	void removeNode(XML* node);
	XMLList* getTargetObject() { return targetobject; }
};
}
#endif /* SCRIPTING_TOPLEVEL_XMLLIST_H */
