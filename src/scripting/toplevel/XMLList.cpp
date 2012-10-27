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

#include "scripting/toplevel/XMLList.h"
#include "scripting/class.h"
#include "compat.h"
#include "scripting/argconv.h"
#include "abc.h"
#include <libxml/tree.h>
#include <libxml++/parsers/domparser.h>
#include <libxml++/nodes/textnode.h>

using namespace std;
using namespace lightspark;

/* XMLList of size 1 delegates function calls to the single XML
 * object, if no method with the same name has been defined for
 * XMLList. */
#define REGISTER_XML_DELEGATE(name) \
	c->setDeclaredMethodByQName(#name,AS3,Class<IFunction>::getFunction(name),NORMAL_METHOD,true)

#define REGISTER_XML_DELEGATE2(asname,cppname) \
	c->setDeclaredMethodByQName(#asname,AS3,Class<IFunction>::getFunction(cppname),NORMAL_METHOD,true)

#define ASFUNCTIONBODY_XML_DELEGATE(name) \
	ASObject* XMLList::name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		XMLList* th=obj->as<XMLList>(); \
		if(!th) \
			throw Class<ArgumentError>::getInstanceS("Function applied to wrong object"); \
		if(th->nodes.size()==1) \
			return XML::name(th->nodes[0].getPtr(), args, argslen); \
		else \
			throw Class<TypeError>::getInstanceS("Error #1086: The method only works on lists of one item."); \
	}

XMLList::XMLList(Class_base* c):ASObject(c),nodes(c->memoryAccount),constructed(false)
{
}

XMLList::XMLList(Class_base* cb,bool c):ASObject(cb),nodes(cb->memoryAccount),constructed(c)
{
	assert(c);
}

XMLList::XMLList(Class_base* c, const std::string& str):ASObject(c),nodes(c->memoryAccount),constructed(true)
{
	buildFromString(str);
}

XMLList::XMLList(Class_base* c,const XML::XMLVector& r):
	ASObject(c),nodes(r.begin(),r.end(),c->memoryAccount),constructed(true)
{
}

void XMLList::finalize()
{
	nodes.clear();
	ASObject::finalize();
}

void XMLList::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_getLength),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attribute",AS3,Class<IFunction>::getFunction(attribute),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes",AS3,Class<IFunction>::getFunction(attributes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("child",AS3,Class<IFunction>::getFunction(child),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("children",AS3,Class<IFunction>::getFunction(children),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains",AS3,Class<IFunction>::getFunction(contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copy",AS3,Class<IFunction>::getFunction(copy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descendants",AS3,Class<IFunction>::getFunction(descendants),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("elements",AS3,Class<IFunction>::getFunction(elements),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parent",AS3,Class<IFunction>::getFunction(parent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasSimpleContent",AS3,Class<IFunction>::getFunction(_hasSimpleContent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasComplexContent",AS3,Class<IFunction>::getFunction(_hasComplexContent),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(valueOf),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(valueOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toXMLString",AS3,Class<IFunction>::getFunction(toXMLString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("text",AS3,Class<IFunction>::getFunction(text),NORMAL_METHOD,true);
	REGISTER_XML_DELEGATE(addNamespace);
	REGISTER_XML_DELEGATE(appendChild);
	REGISTER_XML_DELEGATE(childIndex);
	REGISTER_XML_DELEGATE(inScopeNamespaces);
	//REGISTER_XML_DELEGATE(insertChildAfter);
	//REGISTER_XML_DELEGATE(insertChildBefore);
	REGISTER_XML_DELEGATE(localName);
	REGISTER_XML_DELEGATE(name);
	REGISTER_XML_DELEGATE2(namespace,_namespace);
	//REGISTER_XML_DELEGATE(namespaceDeclarations);
	REGISTER_XML_DELEGATE(nodeKind);
	//REGISTER_XML_DELEGATE(prependChild);
	//REGISTER_XML_DELEGATE(removeNamespace);
	//REGISTER_XML_DELEGATE(replace);
	//REGISTER_XML_DELEGATE(setChildren);
	REGISTER_XML_DELEGATE2(setLocalName,_setLocalName);
	REGISTER_XML_DELEGATE2(setName,_setName);
	REGISTER_XML_DELEGATE2(setNamespace,_setNamespace);
}

ASFUNCTIONBODY_XML_DELEGATE(addNamespace);
ASFUNCTIONBODY_XML_DELEGATE(appendChild);
ASFUNCTIONBODY_XML_DELEGATE(childIndex);
ASFUNCTIONBODY_XML_DELEGATE(inScopeNamespaces);
//ASFUNCTIONBODY_XML_DELEGATE(insertChildAfter);
//ASFUNCTIONBODY_XML_DELEGATE(insertChildBefore);
ASFUNCTIONBODY_XML_DELEGATE(localName);
ASFUNCTIONBODY_XML_DELEGATE(name);
ASFUNCTIONBODY_XML_DELEGATE(_namespace);
//ASFUNCTIONBODY_XML_DELEGATE(namespaceDeclarations);
ASFUNCTIONBODY_XML_DELEGATE(nodeKind);
//ASFUNCTIONBODY_XML_DELEGATE(prependChild);
//ASFUNCTIONBODY_XML_DELEGATE(removeNamespace);
//ASFUNCTIONBODY_XML_DELEGATE(replace);
//ASFUNCTIONBODY_XML_DELEGATE(setChildren);
ASFUNCTIONBODY_XML_DELEGATE(_setLocalName);
ASFUNCTIONBODY_XML_DELEGATE(_setName);
ASFUNCTIONBODY_XML_DELEGATE(_setNamespace);

ASFUNCTIONBODY(XMLList,_constructor)
{
	assert_and_throw(argslen<=1);
	XMLList* th=Class<XMLList>::cast(obj);
	if(argslen==0 && th->constructed)
	{
		//Called from internal code
		return NULL;
	}
	if(argslen==0 ||
	   args[0]->is<Null>() || 
	   args[0]->is<Undefined>())
	{
		return NULL;
	}
	else if(args[0]->is<XML>())
	{
		args[0]->incRef();
		th->append(_MR(args[0]->as<XML>()));
	}
	else if(args[0]->is<XMLList>())
	{
		args[0]->incRef();
		th->append(_MR(args[0]->as<XMLList>()));
	}
	else if(args[0]->is<ASString>() ||
		args[0]->is<Number>() ||
		args[0]->is<Integer>() ||
		args[0]->is<UInteger>() ||
		args[0]->is<Boolean>())
	{
		th->buildFromString(args[0]->toString());
	}
	else
	{
		throw RunTimeException("Type not supported in XMLList()");
	}

	return NULL;
}

void XMLList::buildFromString(const std::string& str)
{
	xmlpp::DomParser parser;
	std::string default_ns=getVm()->getDefaultXMLNamespace();
	std::string expanded="<parent xmlns=\"" + default_ns + "\">" + str + "</parent>";
	try
	{
		parser.parse_memory(expanded);
	}
	catch(const exception& e)
	{
		try
		{
			parser.parse_memory(str);
		}
		catch(const exception& e)
		{
			throw RunTimeException("Error while parsing XML");
		}
	}
	const xmlpp::Node::NodeList& children=\
	  parser.get_document()->get_root_node()->get_children();
	xmlpp::Node::NodeList::const_iterator it;
	for(it=children.begin(); it!=children.end(); ++it)
		nodes.push_back(_MR(Class<XML>::getInstanceS(*it)));
}

_R<XML> XMLList::reduceToXML() const
{
	//Needed to convert XMLList to XML
	//Check if there is a single non null XML object. If not throw an exception
	if(nodes.size()==1)
		return nodes[0];
	else
		throw Class<TypeError>::getInstanceS("#1080");
}

ASFUNCTIONBODY(XMLList,_getLength)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_i(th->nodes.size());
}

ASFUNCTIONBODY(XMLList,_hasSimpleContent)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_b(th->hasSimpleContent());
}

ASFUNCTIONBODY(XMLList,_hasComplexContent)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_b(th->hasComplexContent());
}

ASFUNCTIONBODY(XMLList,generator)
{
	assert(obj==NULL);
	if(argslen==0)
	{
		return Class<XMLList>::getInstanceS("");
	}
	else if(args[0]->is<ASString>() ||
		args[0]->is<Number>() ||
		args[0]->is<Integer>() ||
		args[0]->is<UInteger>() ||
		args[0]->is<Boolean>())
	{
		return Class<XMLList>::getInstanceS(args[0]->toString());
	}
	else if(args[0]->is<XMLList>())
	{
		args[0]->incRef();
		return args[0];
	}
	else if(args[0]->is<XML>())
	{
		XML::XMLVector nodes;
		args[0]->incRef();
		nodes.push_back(_MR(Class<XML>::cast(args[0])));
		return Class<XMLList>::getInstanceS(nodes);
	}
	else if(args[0]->getObjectType()==T_NULL ||
		args[0]->getObjectType()==T_UNDEFINED)
	{
		return Class<XMLList>::getInstanceS();
	}
	else
		throw RunTimeException("Type not supported in XMLList()");
}

ASFUNCTIONBODY(XMLList,descendants)
{
	XMLList* th=Class<XMLList>::cast(obj);
	tiny_string name;
	assert_and_throw(argslen==0 || args[0]->getObjectType()!=T_QNAME);
	ARG_UNPACK(name,"*");
	XML::XMLVector ret;
	th->getDescendantsByQName(name,"",ret);
	return Class<XMLList>::getInstanceS(ret);
}

ASFUNCTIONBODY(XMLList,elements)
{
	XMLList* th=Class<XMLList>::cast(obj);
	tiny_string name;
	ARG_UNPACK(name, "");

	XML::XMLVector elems;
	auto it=th->nodes.begin();
        for(; it!=th->nodes.end(); ++it)
        {
		(*it)->getElementNodes(name, elems);
	}
	return Class<XMLList>::getInstanceS(elems);
}

ASFUNCTIONBODY(XMLList,parent)
{
	XMLList* th=Class<XMLList>::cast(obj);

	if(th->nodes.size()==0)
		return getSys()->getUndefinedRef();

	auto it=th->nodes.begin();
	ASObject *parent=(*it)->getParentNode();
	++it;

        for(; it!=th->nodes.end(); ++it)
        {
		ASObject *otherParent=(*it)->getParentNode();
		if(!parent->isEqual(otherParent))
			return getSys()->getUndefinedRef();
	}

	return parent;
}

ASFUNCTIONBODY(XMLList,valueOf)
{
	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(XMLList,child)
{
	XMLList* th = obj->as<XMLList>();
	assert_and_throw(argslen==1);
	const tiny_string& arg0=args[0]->toString();
	XML::XMLVector ret;
	auto it=th->nodes.begin();
        for(; it!=th->nodes.end(); ++it)
        {
		(*it)->childrenImpl(ret, arg0);
	}
	XMLList* retObj=Class<XMLList>::getInstanceS(ret);
	return retObj;
}

ASFUNCTIONBODY(XMLList,children)
{
	XMLList* th = obj->as<XMLList>();
	assert_and_throw(argslen==0);
	XML::XMLVector ret;
	auto it=th->nodes.begin();
        for(; it!=th->nodes.end(); ++it)
        {
		(*it)->childrenImpl(ret, "*");
	}
	XMLList* retObj=Class<XMLList>::getInstanceS(ret);
	return retObj;
}

ASFUNCTIONBODY(XMLList,text)
{
	XMLList* th = obj->as<XMLList>();
	ARG_UNPACK;
	XML::XMLVector ret;
	auto it=th->nodes.begin();
        for(; it!=th->nodes.end(); ++it)
        {
		(*it)->getText(ret);
	}
	return Class<XMLList>::getInstanceS(ret);
}

ASFUNCTIONBODY(XMLList,contains)
{
	XMLList* th = obj->as<XMLList>();
	_NR<ASObject> value;
	ARG_UNPACK (value);
	if(!value->is<XML>())
		return abstract_b(false);

	auto it=th->nodes.begin();
        for(; it!=th->nodes.end(); ++it)
        {
		if((*it)->isEqual(value.getPtr()))
			return abstract_b(true);
	}

	return abstract_b(false);
}

ASFUNCTIONBODY(XMLList,copy)
{
	XMLList* th = obj->as<XMLList>();
	XMLList *dest = Class<XMLList>::getInstanceS();
	auto it=th->nodes.begin();
        for(; it!=th->nodes.end(); ++it)
        {
		dest->nodes.push_back(_MR((*it)->copy()));
	}
	return dest;
}

ASFUNCTIONBODY(XMLList,attribute)
{
	XMLList *th = obj->as<XMLList>();

	if(argslen > 0 && args[0]->is<QName>())
		LOG(LOG_NOT_IMPLEMENTED,"XMLList.attribute called with QName");

	tiny_string attrname;
	ARG_UNPACK (attrname);
	multiname mname(NULL);
	mname.name_type=multiname::NAME_STRING;
	mname.name_s_id=getSys()->getUniqueStringId(attrname);
	mname.ns.push_back(nsNameAndKind("",NAMESPACE));
	mname.isAttribute = true;

	_NR<ASObject> attr=th->getVariableByMultiname(mname, NONE);
	assert(!attr.isNull());
	attr->incRef();
	return attr.getPtr();
}

ASFUNCTIONBODY(XMLList,attributes)
{
	XMLList *th = obj->as<XMLList>();
	XMLList *res = Class<XMLList>::getInstanceS();
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		XML::XMLVector nodeAttributes = (*it)->getAttributes();
		res->nodes.insert(res->nodes.end(), nodeAttributes.begin(), nodeAttributes.end());
	}
	return res;
}

_NR<ASObject> XMLList::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
	{
		_NR<ASObject> res=ASObject::getVariableByMultiname(name,opt);

		//If a method is not found on XMLList object and this
		//is a single element list with simple content,
		//delegate to ASString
		if(res.isNull() && nodes.size()==1 && nodes[0]->hasSimpleContent())
		{
			ASString *contentstr=Class<ASString>::getInstanceS(nodes[0]->toString_priv());
			res=contentstr->getVariableByMultiname(name, opt);
			contentstr->decRef();
		}

		return res;
	}

	unsigned int index=0;
	if(Array::isValidMultiname(name,index))
	{
		if(index<nodes.size())
			return nodes[index];
		else
			return NullRef;
	}
	else
	{
		XML::XMLVector retnodes;
		auto it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			_NR<ASObject> o=(*it)->getVariableByMultiname(name,opt);
			XMLList *x=dynamic_cast<XMLList *>(o.getPtr());
			if(!x)
				continue;

			retnodes.insert(retnodes.end(), x->nodes.begin(), x->nodes.end());
		}

		if(retnodes.size()==0 && (opt & XML_STRICT)!=0)
			return NullRef;

		return _MNR(Class<XMLList>::getInstanceS(retnodes));
	}
}

bool XMLList::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	assert_and_throw(name.ns.size()>0);
	if(!name.ns[0].hasEmptyName())
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	unsigned int index=0;
	if(Array::isValidMultiname(name,index))
		return index<nodes.size();
	else
	{
		XML::XMLVector retnodes;
		auto it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			bool ret=(*it)->hasPropertyByMultiname(name, considerDynamic, considerPrototype);
			if(ret)
				return ret;
		}
	}

	return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
}

void XMLList::setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name,o,allowConst);

	XML* newNode=dynamic_cast<XML*>(o);
	if(newNode==NULL)
		return ASObject::setVariableByMultiname(name,o,allowConst);

	//Nodes are always added at the end. The requested index are ignored. This is a tested behaviour.
	nodes.push_back(_MR(newNode));
}

void XMLList::getDescendantsByQName(const tiny_string& name, const tiny_string& ns, XML::XMLVector& ret)
{
	auto it=nodes.begin();
	for(; it!=nodes.end(); ++it)
	{
		(*it)->getDescendantsByQName(name, ns, ret);
	}
}

_NR<XML> XMLList::convertToXML() const
{
	if(nodes.size()==1)
		return nodes[0];
	else
		return NullRef;
}

bool XMLList::hasSimpleContent() const
{
	if(nodes.size()==0)
		return true;
	else if(nodes.size()==1)
		return nodes[0]->hasSimpleContent();
	else
	{
		auto it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			if((*it)->getNodeKind()==XML_ELEMENT_NODE)
				return false;
		}
	}

	return true;
}

bool XMLList::hasComplexContent() const
{
	if(nodes.size()==0)
		return false;
	else if(nodes.size()==1)
		return nodes[0]->hasComplexContent();
	else
	{
		auto it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			if((*it)->getNodeKind()==XML_ELEMENT_NODE)
				return true;
		}
	}

	return false;
}

void XMLList::append(_R<XML> x)
{
	nodes.push_back(x);
}

void XMLList::append(_R<XMLList> x)
{
	nodes.insert(nodes.end(),x->nodes.begin(),x->nodes.end());
}

tiny_string XMLList::toString_priv() const
{
	if(hasSimpleContent())
	{
		tiny_string ret;
		for(uint32_t i=0;i<nodes.size();i++)
		{
			xmlElementType kind=nodes[i]->getNodeKind();
			if(kind!=XML_COMMENT_NODE && kind!=XML_PI_NODE)
				ret+=nodes[i]->toString();
		}
		return ret;
	}
	else
	{
		xmlBufferPtr xmlBuffer=xmlBufferCreateSize(4096);
		toXMLString_priv(xmlBuffer);
		tiny_string ret((char*)xmlBuffer->content, true);
		xmlBufferFree(xmlBuffer);
		return ret;
	}
}

tiny_string XMLList::toString()
{
	return toString_priv();
}

ASFUNCTIONBODY(XMLList,_toString)
{
	XMLList* th=Class<XMLList>::cast(obj);
	return Class<ASString>::getInstanceS(th->toString_priv());
}

void XMLList::toXMLString_priv(xmlBufferPtr buf) const
{
	for(size_t i=0; i<nodes.size(); i++)
	{
		if(i>0)
			xmlBufferWriteChar(buf, "\n");
		nodes[i].getPtr()->toXMLString_priv(buf);
	}
}

ASFUNCTIONBODY(XMLList,toXMLString)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	xmlBufferPtr xmlBuffer=xmlBufferCreateSize(4096);
	th->toXMLString_priv(xmlBuffer);
	ASString* ret=Class<ASString>::getInstanceS((char*)xmlBuffer->content);
	xmlBufferFree(xmlBuffer);
	return ret;
}

bool XMLList::isEqual(ASObject* r)
{
	if(nodes.size()==0 && r->getObjectType()==T_UNDEFINED)
		return true;

	XMLList *x=dynamic_cast<XMLList *>(r);
	if(x)
	{
		if(nodes.size()!=x->nodes.size())
			return false;

		for(unsigned int i=0; i<nodes.size(); i++)
			if(!nodes[i]->isEqual(x->nodes[i].getPtr()))
				return false;

		return true;
	}
	else if(nodes.size()==1)
	{
		return nodes[0]->isEqual(r);
	}

	return false;
}

uint32_t XMLList::nextNameIndex(uint32_t cur_index)
{
	if(cur_index < nodes.size())
		return cur_index+1;
	else
		return 0;
}

_R<ASObject> XMLList::nextName(uint32_t index)
{
	if(index<=nodes.size())
		return _MR(abstract_i(index-1));
	else
		throw RunTimeException("XMLList::nextName out of bounds");
}

_R<ASObject> XMLList::nextValue(uint32_t index)
{
	if(index<=nodes.size())
		return nodes[index-1];
	else
		throw RunTimeException("XMLList::nextValue out of bounds");
}

void XMLList::appendNodesTo(XML *dest) const
{
	std::vector<_R<XML>, reporter_allocator<_R<XML>>>::const_iterator it;
	for (it=nodes.begin(); it!=nodes.end(); ++it)
	{
		ASObject *arg0=it->getPtr();
		ASObject *ret=XML::appendChild(dest, &arg0, 1);
		if(ret)
			ret->decRef();
	}
}
