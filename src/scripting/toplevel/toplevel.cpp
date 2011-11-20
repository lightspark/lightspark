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

#include <list>
#include <algorithm>
#include <pcre.h>
#include <string.h>
#include <sstream>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <iomanip>
#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <errno.h>

#include <glib.h>

#include "abc.h"
#include "toplevel.h"
#include "flash/events/flashevents.h"
#include "swf.h"
#include "compat.h"
#include "class.h"
#include "exceptions.h"
#include "backends/urlutils.h"
#include "parsing/amf3_generator.h"
#include <libxml++/nodes/textnode.h>
#include "argconv.h"

using namespace std;
using namespace lightspark;

SET_NAMESPACE("");

REGISTER_CLASS_NAME2(ASQName,"QName","");
REGISTER_CLASS_NAME2(IFunction,"Function","");
REGISTER_CLASS_NAME2(UInteger,"uint","");
REGISTER_CLASS_NAME2(Integer,"int","");
REGISTER_CLASS_NAME2(Global,"global","");
REGISTER_CLASS_NAME(Number);
REGISTER_CLASS_NAME(Namespace);
REGISTER_CLASS_NAME(RegExp);
REGISTER_CLASS_NAME2(ASString, "String", "");
REGISTER_CLASS_NAME(XML);
REGISTER_CLASS_NAME(XMLList);

Any* const Type::anyType = new Any();
Void* const Type::voidType = new Void();

XML::XML():node(NULL),constructed(false)
{
}

XML::XML(const string& str):node(NULL),constructed(true)
{
	buildFromString(str);
}

XML::XML(_R<XML> _r, xmlpp::Node* _n):root(_r),node(_n),constructed(true)
{
	assert(node);
}

XML::XML(xmlpp::Node* _n):constructed(true)
{
	assert(_n);
	node=parser.get_document()->create_root_node_by_import(_n);
}

void XML::finalize()
{
	ASObject::finalize();
	root.reset();
}

void XML::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(XML::_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toXMLString",AS3,Class<IFunction>::getFunction(toXMLString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nodeKind",AS3,Class<IFunction>::getFunction(nodeKind),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("children",AS3,Class<IFunction>::getFunction(children),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes",AS3,Class<IFunction>::getFunction(attributes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("localName",AS3,Class<IFunction>::getFunction(localName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("name",AS3,Class<IFunction>::getFunction(name),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descendants",AS3,Class<IFunction>::getFunction(descendants),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendChild",AS3,Class<IFunction>::getFunction(appendChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasSimpleContent",AS3,Class<IFunction>::getFunction(_hasSimpleContent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasComplexContent",AS3,Class<IFunction>::getFunction(_hasComplexContent),NORMAL_METHOD,true);
}

void XML::buildFromString(const string& str)
{
	if(str.empty())
	{
		xmlpp::Element *el=parser.get_document()->create_root_node("");
		node=el->add_child_text();
		// TODO: node's parent (root) should be inaccessible from AS code
	}
	else
	{
		string buf(str.c_str());
		//if this is a CDATA node replace CDATA tags to make it look like a text-node
		//for compatibility with the Adobe player
		if (str.compare(0, 9, "<![CDATA[") == 0) {
			buf = "<a>"+str.substr(9, str.size()-12)+"</a>";
		}

		try
		{
			parser.parse_memory_raw((const unsigned char*)buf.c_str(), buf.size());
		}
		catch(const exception& e)
		{
			throw RunTimeException("Error while parsing XML");
		}
		node=parser.get_document()->get_root_node();
	}
}

ASFUNCTIONBODY(XML,generator)
{
	assert(obj==NULL);
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType()==T_STRING)
	{
		ASString* str=Class<ASString>::cast(args[0]);
		return Class<XML>::getInstanceS(std::string(str->data));
	}
	else if(args[0]->getObjectType()==T_NULL ||
		args[0]->getObjectType()==T_UNDEFINED)
	{
		return Class<XML>::getInstanceS("");
	}
	else if(args[0]->getClass()==Class<XML>::getClass())
	{
		args[0]->incRef();
		return args[0];
	}
	else
		throw RunTimeException("Type not supported in XML()");
}

ASFUNCTIONBODY(XML,_constructor)
{
	assert_and_throw(argslen<=1);
	XML* th=Class<XML>::cast(obj);
	if(argslen==0 && th->constructed) //If root is already set we have been constructed outside AS code
		return NULL;

	if(argslen==0 ||
	   args[0]->getObjectType()==T_NULL || 
	   args[0]->getObjectType()==T_UNDEFINED)
	{
		th->buildFromString("");
	}
	else
	{
		assert_and_throw(args[0]->getObjectType()==T_STRING);
		ASString* str=Class<ASString>::cast(args[0]);
		th->buildFromString(std::string(str->data));
	}
	return NULL;
}

ASFUNCTIONBODY(XML,nodeKind)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	xmlNodePtr libXml2Node=th->node->cobj();
	switch(libXml2Node->type)
	{
		case XML_ATTRIBUTE_NODE:
			return Class<ASString>::getInstanceS("attribute");
		case XML_ELEMENT_NODE:
			return Class<ASString>::getInstanceS("element");
		case XML_TEXT_NODE:
			return Class<ASString>::getInstanceS("text");
		default:
		{
			LOG(LOG_ERROR,"Unsupported XML type " << libXml2Node->type);
			throw UnsupportedException("Unsupported XML node type");
		}
	}
}

ASFUNCTIONBODY(XML,localName)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	xmlElementType nodetype=th->node->cobj()->type;
	if(nodetype==XML_TEXT_NODE || nodetype==XML_COMMENT_NODE)
		return new Null;
	else
		return Class<ASString>::getInstanceS(th->node->get_name());
}

ASFUNCTIONBODY(XML,name)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	xmlElementType nodetype=th->node->cobj()->type;
	//TODO: add namespace
	if(nodetype==XML_TEXT_NODE || nodetype==XML_COMMENT_NODE)
		return new Null;
	else
		return Class<ASString>::getInstanceS(th->node->get_name());
}

ASFUNCTIONBODY(XML,descendants)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==1);
	assert_and_throw(args[0]->getObjectType()!=T_QNAME);
	vector<_R<XML>> ret;
	th->getDescendantsByQName(args[0]->toString(),"",ret);
	return Class<XMLList>::getInstanceS(ret);
}

ASFUNCTIONBODY(XML,appendChild)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==1);
	XML* arg=NULL;
	if(args[0]->getClass()==Class<XML>::getClass())
		arg=Class<XML>::cast(args[0]);
	else if(args[0]->getClass()==Class<XMLList>::getClass())
		arg=Class<XMLList>::cast(args[0])->convertToXML().getPtr();

	if(arg==NULL)
		throw RunTimeException("Invalid argument for XML::appendChild");
	//Change the root of the appended XML node
	_NR<XML> rootXML=NullRef;
	if(th->root.isNull())
	{
		th->incRef();
		rootXML=_MR(th);
	}
	else
		rootXML=th->root;

	arg->root=rootXML;
	xmlUnlinkNode(arg->node->cobj());
	xmlNodePtr ret=xmlAddChild(th->node->cobj(),arg->node->cobj());
	assert_and_throw(ret);
	arg->incRef();
	return arg;
}

ASFUNCTIONBODY(XML,attributes)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	//Needed dynamic cast, we want the type check
	xmlpp::Element* elem=dynamic_cast<xmlpp::Element*>(th->node);
	if(elem==NULL)
		return Class<XMLList>::getInstanceS();
	const xmlpp::Element::AttributeList& list=elem->get_attributes();
	xmlpp::Element::AttributeList::const_iterator it=list.begin();
	std::vector<_R<XML>> ret;
	_NR<XML> rootXML=NullRef;
	if(th->root.isNull())
	{
		th->incRef();
		rootXML=_MR(th);
	}
	else
		rootXML=th->root;

	for(;it!=list.end();it++)
		ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));

	XMLList* retObj=Class<XMLList>::getInstanceS(ret);
	return retObj;
}

void XML::toXMLString_priv(xmlBufferPtr buf)
{
	//NOTE: this function is not thread-safe, as it can modify the xmlNode
	_NR<XML> rootXML=NullRef;
	if(root.isNull())
	{
		this->incRef();
		rootXML=_MR(this);
	}
	else
		rootXML=root;

	xmlDocPtr xmlDoc=rootXML->parser.get_document()->cobj();
	assert(xmlDoc);
	xmlNodePtr cNode=node->cobj();
	//As libxml2 does not automatically add the needed namespaces to the dump
	//we have to workaround the issue

	//Get the needed namespaces
	xmlNsPtr* neededNamespaces=xmlGetNsList(xmlDoc,cNode);
	//Save a copy of the namespaces actually defined in the node
	xmlNsPtr oldNsDef=cNode->nsDef;

	//Copy the namespaces (we need to modify them to create a customized list)
	vector<xmlNs> localNamespaces;
	if(neededNamespaces)
	{
		xmlNsPtr* cur=neededNamespaces;
		while(*cur)
		{
			localNamespaces.emplace_back(**cur);
			cur++;
		}
		for(uint32_t i=0;i<localNamespaces.size()-1;i++)
			localNamespaces[i].next=&localNamespaces[i+1];
		localNamespaces.back().next=NULL;
		//Free the namespaces arrary
		xmlFree(neededNamespaces);
		//Override the node defined namespaces
		cNode->nsDef=&localNamespaces.front();
	}

	int retVal=xmlNodeDump(buf, xmlDoc, cNode, 0, 0);
	//Restore the previously defined namespaces
	cNode->nsDef=oldNsDef;
	if(retVal==-1)
		throw RunTimeException("Error om XML::toXMLString_priv");
}

ASFUNCTIONBODY(XML,toXMLString)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	//Allocate a page at the beginning
	xmlBufferPtr xmlBuffer=xmlBufferCreateSize(4096);
	th->toXMLString_priv(xmlBuffer);
	ASString* ret=Class<ASString>::getInstanceS((char*)xmlBuffer->content);
	xmlBufferFree(xmlBuffer);
	return ret;
}

ASFUNCTIONBODY(XML,children)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	const xmlpp::Node::NodeList& list=th->node->get_children();
	xmlpp::Node::NodeList::const_iterator it=list.begin();
	std::vector<_R<XML>> ret;

	_NR<XML> rootXML=NullRef;
	if(th->root.isNull())
	{
		th->incRef();
		rootXML=_MR(th);
	}
	else
		rootXML=th->root;

	for(;it!=list.end();it++)
	{
		rootXML->incRef();
		ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));
	}
	XMLList* retObj=Class<XMLList>::getInstanceS(ret);
	return retObj;
}

ASFUNCTIONBODY(XML,_hasSimpleContent)
{
	XML *th=static_cast<XML*>(obj);
	return abstract_b(th->hasSimpleContent());
}

ASFUNCTIONBODY(XML,_hasComplexContent)
{
	XML *th=static_cast<XML*>(obj);
	return abstract_b(th->hasComplexContent());
}

bool XML::hasSimpleContent() const
{
	xmlElementType nodetype=node->cobj()->type;
	if(nodetype==XML_COMMENT_NODE || nodetype==XML_PI_NODE)
		return false;

	const xmlpp::Node::NodeList& children=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=children.begin();
	for(;it!=children.end();++it)
	{
		if((*it)->cobj()->type==XML_ELEMENT_NODE)
			return false;
	}

	return true;
}

bool XML::hasComplexContent() const
{
	const xmlpp::Node::NodeList& children=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=children.begin();
	for(;it!=children.end();++it)
	{
		if((*it)->cobj()->type==XML_ELEMENT_NODE)
			return true;
	}

	return false;
}

xmlElementType XML::getNodeKind() const
{
	return node->cobj()->type;
}

void XML::recursiveGetDescendantsByQName(_R<XML> root, xmlpp::Node* node, const tiny_string& name, const tiny_string& ns,
		std::vector<_R<XML>>& ret)
{
	//Check if this node is being requested. The empty string means ALL
	if(name.empty() || name == node->get_name())
	{
		root->incRef();
		ret.push_back(_MR(Class<XML>::getInstanceS(root, node)));
	}
	//NOTE: Creating a temporary list is quite a large overhead, but there is no way in libxml++ to access the first child
	const xmlpp::Node::NodeList& list=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=list.begin();
	for(;it!=list.end();it++)
		recursiveGetDescendantsByQName(root, *it, name, ns, ret);
}

void XML::getDescendantsByQName(const tiny_string& name, const tiny_string& ns, std::vector<_R<XML> >& ret)
{
	assert(node);
	assert_and_throw(ns=="");
	_NR<XML> rootXML=NullRef;
	if(root.isNull())
	{
		this->incRef();
		rootXML=_MR(this);
	}
	else
		rootXML=root;

	recursiveGetDescendantsByQName(rootXML, node, name, ns, ret);
}

_NR<ASObject> XML::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0)
		return ASObject::getVariableByMultiname(name,opt);

	if(node==NULL)
	{
		//This is possible if the XML object was created from an empty string
		return NullRef;
	}

	bool isAttr=name.isAttribute;
	const tiny_string& normalizedName=name.normalizedName();
	const char *buf=normalizedName.raw_buf();
	if(normalizedName.charAt(0)=='@')
	{
		isAttr=true;
		buf+=1;
	}
	if(isAttr)
	{
		//Lookup attribute
		//TODO: support namespaces
		assert_and_throw(name.ns.size()>0 && name.ns[0].name=="");
		//Normalize the name to the string form
		assert(node);
		//To have attributes we must be an Element
		xmlpp::Element* element=dynamic_cast<xmlpp::Element*>(node);
		if(element==NULL)
			return NullRef;
		xmlpp::Attribute* attr=element->get_attribute(buf);
		if(attr==NULL)
			return NullRef;

		_NR<XML> rootXML=NullRef;
		if(root.isNull())
		{
			this->incRef();
			rootXML=_MR(this);
		}
		else
			rootXML=root;

		std::vector<_R<XML> > retnode;
		retnode.push_back(_MR(Class<XML>::getInstanceS(rootXML, attr)));
		return _MNR(Class<XMLList>::getInstanceS(retnode));
	}
	else
	{
		//Lookup children
		//TODO: support namespaces
		assert_and_throw(name.ns.size()>0 && name.ns[0].name=="");
		//Normalize the name to the string form
		assert(node);
		const xmlpp::Node::NodeList& children=node->get_children(buf);
		xmlpp::Node::NodeList::const_iterator it=children.begin();

		std::vector<_R<XML>> ret;

		_NR<XML> rootXML=NullRef;
		if(root.isNull())
		{
			this->incRef();
			rootXML=_MR(this);
		}
		else
			rootXML=root;

		for(;it!=children.end();it++)
			ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));

		if(ret.size()==0 && (opt & XML_STRICT)!=0)
			return NullRef;

		return _MNR(Class<XMLList>::getInstanceS(ret));
	}
}

bool XML::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	if(node==NULL || considerDynamic == false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	bool isAttr=name.isAttribute;
	const tiny_string& normalizedName=name.normalizedName();
	const char *buf=normalizedName.raw_buf();
	if(normalizedName.charAt(0)=='@')
	{
		isAttr=true;
		buf+=1;
	}
	if(isAttr)
	{
		//Lookup attribute
		//TODO: support namespaces
		assert_and_throw(name.ns.size()>0 && name.ns[0].name=="");
		//Normalize the name to the string form
		assert(node);
		//To have attributes we must be an Element
		xmlpp::Element* element=dynamic_cast<xmlpp::Element*>(node);
		if(element)
		{
			xmlpp::Attribute* attr=element->get_attribute(buf);
			if(attr!=NULL)
				return true;
		}
	}
	else
	{
		//Lookup children
		//TODO: support namespaces
		assert_and_throw(name.ns.size()>0 && name.ns[0].name=="");
		//Normalize the name to the string form
		assert(node);
		const xmlpp::Node::NodeList& children=node->get_children(buf);
		if(!children.empty())
			return true;
	}

	//Try the normal path as the last resource
	return ASObject::hasPropertyByMultiname(name, considerDynamic);
}

ASFUNCTIONBODY(XML,_toString)
{
	XML* th=Class<XML>::cast(obj);
	return Class<ASString>::getInstanceS(th->toString_priv());
}

tiny_string XML::toString_priv()
{
	//We have to use vanilla libxml2, libxml++ is not enough
	xmlNodePtr libXml2Node=node->cobj();
	tiny_string ret;
	if(hasSimpleContent())
	{
		xmlChar* content=xmlNodeGetContent(libXml2Node);
		ret=tiny_string((char*)content,true);
		xmlFree(content);
	}
	else
	{
		assert_and_throw(!node->get_children().empty());
		xmlBufferPtr xmlBuffer=xmlBufferCreateSize(4096);
		toXMLString_priv(xmlBuffer);
		ret=tiny_string((char*)xmlBuffer->content,true);
		xmlBufferFree(xmlBuffer);
	}
	return ret;
}

tiny_string XML::toString()
{
	return toString_priv();
}

bool XML::nodesEqual(xmlpp::Node *a, xmlpp::Node *b) const
{
	assert(a && b);

	// type
	if(a->cobj()->type!=b->cobj()->type)
		return false;

	// name
	if(a->get_name()!=b->get_name() || 
	   (!a->get_name().empty() && 
	    a->get_namespace_uri()!=b->get_namespace_uri()))
		return false;

	// attributes
	xmlpp::Element *el1=dynamic_cast<xmlpp::Element *>(a);
	xmlpp::Element *el2=dynamic_cast<xmlpp::Element *>(b);
	if(el1 && el2)
	{
		xmlpp::Element::AttributeList attrs1=el1->get_attributes();
		xmlpp::Element::AttributeList attrs2=el2->get_attributes();
		if(attrs1.size()!=attrs2.size())
			return false;

		xmlpp::Element::AttributeList::iterator it=attrs1.begin();
		while(it!=attrs1.end())
		{
			xmlpp::Attribute *attr=el2->get_attribute((*it)->get_name(),
								  (*it)->get_namespace_prefix());
			if(!attr || (*it)->get_value()!=attr->get_value())
				return false;

			++it;
		}
	}

	// content
	xmlpp::ContentNode *c1=dynamic_cast<xmlpp::ContentNode *>(a);
	xmlpp::ContentNode *c2=dynamic_cast<xmlpp::ContentNode *>(b);
	if(el1 && el2)
	{
		xmlpp::TextNode *text1=el1->get_child_text();
		xmlpp::TextNode *text2=el2->get_child_text();

		if(text1 && text2)
		{
			if(text1->get_content()!=text2->get_content())
				return false;
		}
		else if(text1 || text2)
			return false;

	}
	else if(c1 && c2)
	{
		if(c1->get_content()!=c2->get_content())
			return false;
	}
	
	// children
	xmlpp::Node::NodeList myChildren=a->get_children();
	xmlpp::Node::NodeList otherChildren=b->get_children();
	if(myChildren.size()!=otherChildren.size())
		return false;

	xmlpp::Node::NodeList::iterator it1=myChildren.begin();
	xmlpp::Node::NodeList::iterator it2=otherChildren.begin();
	while(it1!=myChildren.end())
	{
		if (!nodesEqual(*it1, *it2))
			return false;
		++it1;
		++it2;
	}

	return true;
}

uint32_t XML::nextNameIndex(uint32_t cur_index)
{
	if(cur_index < 1)
		return 1;
	else
		return 0;
}

_R<ASObject> XML::nextName(uint32_t index)
{
	if(index<=1)
		return _MR(abstract_i(index-1));
	else
		throw RunTimeException("XML::nextName out of bounds");
}

_R<ASObject> XML::nextValue(uint32_t index)
{
	if(index<=1)
	{
		incRef();
		return _MR(this);
	}
	else
		throw RunTimeException("XML::nextValue out of bounds");
}

bool XML::isEqual(ASObject* r)
{
	XML *x=dynamic_cast<XML *>(r);
	if(x)
		return nodesEqual(node, x->node);

	XMLList *xl=dynamic_cast<XMLList *>(r);
	if(xl)
		return xl->isEqual(this);

	if(hasSimpleContent())
		return toString()==r->toString();

	return false;
}

void XML::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	throw UnsupportedException("XML::serialize not implemented");
}

XMLList::XMLList(const std::string& str):constructed(true)
{
	buildFromString(str);
}

void XMLList::finalize()
{
	nodes.clear();
}

void XMLList::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_getLength),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendChild",AS3,Class<IFunction>::getFunction(appendChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descendants",AS3,Class<IFunction>::getFunction(descendants),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasSimpleContent",AS3,Class<IFunction>::getFunction(_hasSimpleContent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasComplexContent",AS3,Class<IFunction>::getFunction(_hasComplexContent),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toXMLString",AS3,Class<IFunction>::getFunction(toXMLString),NORMAL_METHOD,true);
}

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
	   args[0]->getObjectType()==T_NULL || 
	   args[0]->getObjectType()==T_UNDEFINED)
		return NULL;

	assert_and_throw(args[0]->getObjectType()==T_STRING);
	ASString* str=Class<ASString>::cast(args[0]);
	th->buildFromString(std::string(str->data));
	return NULL;
}

void XMLList::buildFromString(const std::string& str)
{
	xmlpp::DomParser parser;
	std::string expanded="<parent>" + str + "</parent>";
	try
	{
		parser.parse_memory(expanded);
	}
	catch(const exception& e)
	{
		throw RunTimeException("Error while parsing XML");
	}
	const xmlpp::Node::NodeList& children=\
	  parser.get_document()->get_root_node()->get_children();
	xmlpp::Node::NodeList::const_iterator it;
	for(it=children.begin(); it!=children.end(); ++it)
		nodes.push_back(_MR(Class<XML>::getInstanceS(*it)));
}

ASFUNCTIONBODY(XMLList,_getLength)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_i(th->nodes.size());
}

ASFUNCTIONBODY(XMLList,appendChild)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(th->nodes.size()==1);
	//Forward to the XML object
	return XML::appendChild(th->nodes[0].getPtr(),args,argslen);
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
	assert_and_throw(argslen==1);
	if(args[0]->getObjectType()==T_STRING)
	{
		ASString* str=Class<ASString>::cast(args[0]);
		return Class<XMLList>::getInstanceS(std::string(str->data));
	}
	else if(args[0]->getClass()==Class<XMLList>::getClass())
	{
		args[0]->incRef();
		return args[0];
	}
	else if(args[0]->getClass()==Class<XML>::getClass())
	{
		std::vector< _R<XML> > nodes;
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
	assert_and_throw(argslen==1);
	assert_and_throw(args[0]->getObjectType()!=T_QNAME);
	vector<_R<XML>> ret;
	th->getDescendantsByQName(args[0]->toString(),"",ret);
	return Class<XMLList>::getInstanceS(ret);
}

_NR<ASObject> XMLList::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
		return ASObject::getVariableByMultiname(name,opt);

	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="")
		return ASObject::getVariableByMultiname(name,opt);

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
		std::vector<_R<XML> > retnodes;
		std::vector<_R<XML> >::iterator it=nodes.begin();
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

bool XMLList::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="")
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	unsigned int index=0;
	if(Array::isValidMultiname(name,index))
		return index<nodes.size();
	else
	{
		std::vector<_R<XML> > retnodes;
		std::vector<_R<XML> >::iterator it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			bool ret=(*it)->hasPropertyByMultiname(name, considerDynamic);
			if(ret)
				return ret;
		}
	}

	return ASObject::hasPropertyByMultiname(name, considerDynamic);
}

void XMLList::setVariableByMultiname(const multiname& name, ASObject* o)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name,o);

	XML* newNode=dynamic_cast<XML*>(o);
	if(newNode==NULL)
		return ASObject::setVariableByMultiname(name,o);

	//Nodes are always added at the end. The requested index are ignored. This is a tested behaviour.
	nodes.push_back(_MR(newNode));
}

void XMLList::getDescendantsByQName(const tiny_string& name, const tiny_string& ns, std::vector<_R<XML> >& ret)
{
	std::vector<_R<XML> >::iterator it=nodes.begin();
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
		std::vector<_R<XML> >::const_iterator it=nodes.begin();
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
		std::vector<_R<XML>>::const_iterator it=nodes.begin();
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
	{
		nodes[index-1]->incRef();
		return nodes[index-1];
	}
	else
		throw RunTimeException("XMLList::nextValue out of bounds");
}

ASString::ASString()
{
	type=T_STRING;
}

ASString::ASString(const string& s): data(s)
{
	type=T_STRING;
}

ASString::ASString(const tiny_string& s) : data(s)
{
	type=T_STRING;
}

ASString::ASString(const char* s) : data(s, /*copy:*/true)
{
	type=T_STRING;
}

ASString::ASString(const char* s, uint32_t len)
{
	data = std::string(s,len);
	type=T_STRING;
}

ASFUNCTIONBODY(ASString,_constructor)
{
	ASString* th=static_cast<ASString*>(obj);
	if(args && argslen==1)
		th->data=args[0]->toString().raw_buf();
	return NULL;
}

ASFUNCTIONBODY(ASString,_getLength)
{
	ASString* th=static_cast<ASString*>(obj);
	return abstract_i(th->data.numChars());
}

void ASString::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("split",AS3,Class<IFunction>::getFunction(split),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("substr",AS3,Class<IFunction>::getFunction(substr),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("substring",AS3,Class<IFunction>::getFunction(substring),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("replace",AS3,Class<IFunction>::getFunction(replace),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("concat",AS3,Class<IFunction>::getFunction(concat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("match",AS3,Class<IFunction>::getFunction(match),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("search",AS3,Class<IFunction>::getFunction(search),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",AS3,Class<IFunction>::getFunction(indexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(lastIndexOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("charCodeAt",AS3,Class<IFunction>::getFunction(charCodeAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("charAt",AS3,Class<IFunction>::getFunction(charAt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",AS3,Class<IFunction>::getFunction(slice),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleLowerCase",AS3,Class<IFunction>::getFunction(toLowerCase),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleUpperCase",AS3,Class<IFunction>::getFunction(toUpperCase),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLowerCase",AS3,Class<IFunction>::getFunction(toLowerCase),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toUpperCase",AS3,Class<IFunction>::getFunction(toUpperCase),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("fromCharCode",AS3,Class<IFunction>::getFunction(fromCharCode),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_getLength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
}

void ASString::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(ASString,search)
{
	ASString* th=static_cast<ASString*>(obj);
	int ret = -1;
	if(argslen == 0 || args[0]->getObjectType() == T_UNDEFINED)
		return abstract_i(-1);

	int options=PCRE_UTF8;
	tiny_string restr;
	if(args[0]->getClass() && args[0]->getClass()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);
		restr = re->re;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		if(re->multiline)
			options|=PCRE_MULTILINE;
	}
	else
	{
		restr = args[0]->toString();
	}

	const char* error;
	int errorOffset;
	pcre* pcreRE=pcre_compile(restr.raw_buf(), options, &error, &errorOffset,NULL);
	if(error)
		return abstract_i(ret);
	//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
	int capturingGroups;
	int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return abstract_i(ret);
	}
	assert_and_throw(capturingGroups<10);
	int ovector[30];
	int offset=0;
	//Global is not used in search
	int rc=pcre_exec(pcreRE, NULL, th->data.raw_buf(), th->data.numBytes(), offset, 0, ovector, 30);
	if(rc<0)
	{
		//No matches or error
		pcre_free(pcreRE);
		return abstract_i(ret);
	}
	ret=ovector[0];
	return abstract_i(ret);
}

ASFUNCTIONBODY(ASString,match)
{
	ASString* th=static_cast<ASString*>(obj);
	if(argslen == 0 || args[0]->getObjectType()==T_NULL || args[0]->getObjectType()==T_UNDEFINED)
		return new Null;
	Array* ret=NULL;

	int options=PCRE_UTF8;
	tiny_string restr;
	bool isGlobal = false;
	if(args[0]->getClass() && args[0]->getClass()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);
		restr = re->re;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		if(re->multiline)
			options|=PCRE_MULTILINE;
		isGlobal = re->global;
	}
	else
		restr = args[0]->toString();

	const char* error;
	int errorOffset;
	pcre* pcreRE=pcre_compile(restr.raw_buf(), options, &error, &errorOffset,NULL);
	if(error)
		return new Null;
	//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
	int capturingGroups;
	int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return new Null;
	}
	assert_and_throw(capturingGroups<10);
	int ovector[30];
	int offset=0;
	ret=Class<Array>::getInstanceS();
	do
	{
		int rc=pcre_exec(pcreRE, NULL, th->data.raw_buf(), th->data.numBytes(), offset, 0, ovector, 30);
		if(rc<0)
		{
			//No matches or error
			pcre_free(pcreRE);
			return ret;
		}
		//we cannot use ustrings substr here, because pcre returns those indices in bytes
		//and ustring expects number of UTF8 characters. The same holds for ustring constructor
		ret->push(Class<ASString>::getInstanceS(th->data.substr_bytes(ovector[0],ovector[1]-ovector[0])));
		offset=ovector[1];
	}
	while(isGlobal);
	return ret;
}

ASFUNCTIONBODY(ASString,_toString)
{
	if(Class<ASString>::getClass()->prototype == obj)
		return Class<ASString>::getInstanceS("");
	if(!obj->is<ASString>())
		throw Class<TypeError>::getInstanceS("String.toString is not generic");
	assert_and_throw(argslen==0);

	//As ASStrings are immutable, we can just return ourself
	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(ASString,split)
{
	ASString* th=static_cast<ASString*>(obj);
	Array* ret=Class<Array>::getInstanceS();
	ASObject* delimiter=args[0];
	if(argslen == 0 || delimiter->getObjectType()==T_UNDEFINED)
	{
		ret->push(Class<ASString>::getInstanceS(th->data));
		return ret;
	}

	if(args[0]->getClass() && args[0]->getClass()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);

		if(re->re.empty())
		{
			//the RegExp is empty, so split every character
			for(auto i=th->data.begin();i!=th->data.end();++i)
				ret->push( Class<ASString>::getInstanceS( tiny_string::fromChar(*i) ) );
			return ret;
		}

		const char* error;
		int offset;
		int options=PCRE_UTF8;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		if(re->multiline)
			options|=PCRE_MULTILINE;
		pcre* pcreRE=pcre_compile(re->re.raw_buf(), options, &error, &offset,NULL);
		if(error)
			return ret;
		//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
		if(infoOk!=0)
		{
			pcre_free(pcreRE);
			return ret;
		}
		assert_and_throw(capturingGroups<10);
		int ovector[30];
		offset=0;
		unsigned int end;
		uint32_t lastMatch = 0;
		do
		{
			//offset is a byte offset that must point to the beginning of an utf8 character
			int rc=pcre_exec(pcreRE, NULL, th->data.raw_buf(), th->data.numBytes(), offset, 0, ovector, 30);
			end=ovector[0];
			if(rc<0)
				break;
			if(ovector[0] == ovector[1])
			{ //matched the empty string
				offset++;
				continue;
			}
			//Extract string from last match until the beginning of the current match
			ASString* s=Class<ASString>::getInstanceS(th->data.substr_bytes(lastMatch,end-lastMatch));
			ret->push(s);
			lastMatch=offset=ovector[1];

			//Insert capturing groups
			for(int i=1;i<rc;i++)
			{
				//use string interface through raw(), because we index on bytes, not on UTF-8 characters
				ASString* s=Class<ASString>::getInstanceS(th->data.substr_bytes(ovector[i*2],ovector[i*2+1]-ovector[i*2]));
				ret->push(s);
			}
		}
		while(end<th->data.numBytes());
		if(lastMatch != th->data.numBytes()+1)
		{
			ASString* s=Class<ASString>::getInstanceS(th->data.substr_bytes(lastMatch,th->data.numBytes()-lastMatch));
			ret->push(s);
		}
		pcre_free(pcreRE);
	}
	else
	{
		const tiny_string& del=args[0]->toString();
		if(del.empty())
		{
			//the string is empty, so split every character
			for(auto i=th->data.begin();i!=th->data.end();++i)
				ret->push( Class<ASString>::getInstanceS( tiny_string::fromChar(*i) ) );
			return ret;
		}
		unsigned int start=0;
		do
		{
			int match=th->data.find(del,start);
			if(del.empty())
				match++;
			if(match==-1)
				match=th->data.numChars();
			ASString* s=Class<ASString>::getInstanceS(th->data.substr(start,(match-start)));
			ret->push(s);
			start=match+del.numChars();
		}
		while(start<th->data.numChars());
	}

	return ret;
}

ASFUNCTIONBODY(ASString,substr)
{
	ASString* th=static_cast<ASString*>(obj);
	int start=0;
	if(argslen>=1)
		start=args[0]->toInt();
	if(start<0) {
		start=th->data.numChars()+start;
		if(start<0)
			start=0;
	}
	if(start>(int)th->data.numChars())
		start=th->data.numChars();

	int len=0x7fffffff;
	if(argslen==2)
		len=args[1]->toInt();

	return Class<ASString>::getInstanceS(th->data.substr(start,len));
}

ASFUNCTIONBODY(ASString,substring)
{
	ASString* th=static_cast<ASString*>(obj);
	int start=0;
	if (argslen>=1)
		start=args[0]->toInt();
	if(start<0)
		start=0;
	if(start>(int)th->data.numChars())
		start=th->data.numChars();

	int end=0x7fffffff;
	if(argslen>=2)
		end=args[1]->toInt();
	if(end<0)
		end=0;
	if(end>(int)th->data.numChars())
		end=th->data.numChars();

	if(start>end) {
		int tmp=start;
		start=end;
		end=tmp;
	}

	return Class<ASString>::getInstanceS(th->data.substr(start,end-start));
}

tiny_string ASString::toString_priv() const
{
	return data;
}

/* Note that toNumber() is not virtual.
 * ASString::toNumber is directly called by ASObject::toNumber
 */
double ASString::toNumber() const
{
	assert_and_throw(implEnable);

	/* TODO: data holds a utf8-character sequence, not ascii! */
	const char *s=data.raw_buf();
	char *end=NULL;
	while(g_ascii_isspace(*s))
		s++;
	double val=g_ascii_strtod(s, &end);

	// strtod converts case-insensitive "inf" and "infinity" to
	// inf, flash only accepts case-sensitive "Infinity".
	if(std::isinf(val)) {
		const char *tmp=s;
		while(g_ascii_isspace(*tmp))
			tmp++;
		if(*tmp=='+' || *tmp=='-')
			tmp++;
		if(strncasecmp(tmp, "inf", 3)==0 && strcmp(tmp, "Infinity")!=0)
			return numeric_limits<double>::quiet_NaN();
	}

	// Fail if there is any rubbish after the converted number
	while(*end) {
		if(!g_ascii_isspace(*end))
			return numeric_limits<double>::quiet_NaN();
		end++;
	}

	return val;
}

int32_t ASString::toInt()
{
	assert_and_throw(implEnable);
	//TODO: this assumes data to be ascii, but it is utf8!
	return atoi(data.raw_buf());
}

uint32_t ASString::toUInt()
{
	assert_and_throw(implEnable);
	//TODO: this assumes data to be ascii, but it is utf8!
	return atol(data.raw_buf());
}

bool ASString::isEqual(ASObject* r)
{
	assert_and_throw(implEnable);
	switch(r->getObjectType())
	{
		case T_STRING:
		{
			const ASString* s=static_cast<const ASString*>(r);
			return s->data==data;
		}
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_BOOLEAN:
			return toNumber()==r->toNumber();
		case T_NULL:
		case T_UNDEFINED:
			return false;
		default:
			return r->isEqual(this);
	}
}

TRISTATE ASString::isLess(ASObject* r)
{
	//ECMA-262 11.8.5 algorithm
	assert_and_throw(implEnable);
	_R<ASObject> rprim=r->toPrimitive();
	if(getObjectType()==T_STRING && rprim->getObjectType()==T_STRING)
	{
		ASString* rstr=static_cast<ASString*>(rprim.getPtr());
		return (data<rstr->data)?TTRUE:TFALSE;
	}
	number_t a=toNumber();
	number_t b=rprim->toNumber();
	if(std::isnan(a) || std::isnan(b))
		return TUNDEFINED;
	return (a<b)?TTRUE:TFALSE;
}

void ASString::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	out->writeByte(amf3::string_marker);
	out->writeStringVR(stringMap, data);
}

Undefined::Undefined()
{
	type=T_UNDEFINED;
}

ASFUNCTIONBODY(Undefined,call)
{
	LOG(LOG_CALLS,_("Undefined function"));
	return NULL;
}

TRISTATE Undefined::isLess(ASObject* r)
{
	//ECMA-262 complaiant
	//As undefined became NaN when converted to number the operation is undefined
	return TUNDEFINED;
}

bool Undefined::isEqual(ASObject* r)
{
	switch(r->getObjectType())
	{
		case T_UNDEFINED:
		case T_NULL:
			return true;
		case T_NUMBER:
		case T_INTEGER:
		case T_UINTEGER:
		case T_STRING:
		case T_BOOLEAN:
			return false;
		default:
			return r->isEqual(this);
	}
}

int Undefined::toInt()
{
	return 0;
}

ASObject *Undefined::describeType() const
{
	return new Undefined;
}

void Undefined::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	throw UnsupportedException("Undefined::serialize not implemented");
}

ASFUNCTIONBODY(Integer,_toString)
{
	Integer* th=static_cast<Integer*>(obj);
	int radix=10;
	char buf[20];
	if(argslen==1)
		radix=args[0]->toUInt();
	assert_and_throw(radix==10 || radix==16);
	if(radix==10)
		snprintf(buf,20,"%i",th->val);
	else if(radix==16)
		snprintf(buf,20,"%x",th->val);

	return Class<ASString>::getInstanceS(buf);
}

ASFUNCTIONBODY(Integer,generator)
{
	return abstract_i(args[0]->toInt());
}

TRISTATE Integer::isLess(ASObject* o)
{
	switch(o->getObjectType())
	{
		case T_INTEGER:
			{
				Integer* i=static_cast<Integer*>(o);
				return (val < i->toInt())?TTRUE:TFALSE;
			}
			break;

		case T_UINTEGER:
			{
				UInteger* i=static_cast<UInteger*>(o);
				return (val < i->toInt())?TTRUE:TFALSE;
			}
			break;
		
		case T_NUMBER:
			{
				Number* i=static_cast<Number*>(o);
				if(std::isnan(i->toNumber())) return TUNDEFINED;
				return (val < i->toNumber())?TTRUE:TFALSE;
			}
			break;
		
		case T_STRING:
			{
				double val2=o->toNumber();
				if(std::isnan(val2)) return TUNDEFINED;
				return (val<val2)?TTRUE:TFALSE;
			}
			break;
		
		case T_BOOLEAN:
			{
				Boolean* i=static_cast<Boolean*>(o);
				return (val < i->toInt())?TTRUE:TFALSE;
			}
			break;
		
		case T_UNDEFINED:
			{
				return TUNDEFINED;
			}
			break;
			
		case T_NULL:
			{
				return (val < 0)?TTRUE:TFALSE;
			}
			break;

		default:
			break;
	}
	
	double val2=o->toPrimitive()->toNumber();
	if(std::isnan(val2)) return TUNDEFINED;
	return (val<val2)?TTRUE:TFALSE;
}

bool Integer::isEqual(ASObject* o)
{
	switch(o->getObjectType())
	{
		case T_INTEGER:
			return val==o->toInt();
		case T_UINTEGER:
			//CHECK: somehow wrong
			return val==o->toInt();
		case T_NUMBER:
			return val==o->toNumber();
		case T_BOOLEAN:
			return val==o->toInt();
		case T_STRING:
			return val==o->toNumber();
		case T_NULL:
		case T_UNDEFINED:
			return false;
		default:
			return o->isEqual(this);
	}
}

tiny_string Integer::toString()
{
	return Integer::toString(val);
}

/* static helper function */
tiny_string Integer::toString(int32_t val)
{
	char buf[20];
	if(val<0)
	{
		//This can be a slow path, as it not used for array access
		snprintf(buf,20,"%i",val);
		return tiny_string(buf,true);
	}
	buf[19]=0;
	char* cur=buf+19;

	int v=val;
	do
	{
		cur--;
		*cur='0'+(v%10);
		v/=10;
	}
	while(v!=0);
	return tiny_string(cur,true); //Create a copy
}

void Integer::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("MAX_VALUE","",new Integer(numeric_limits<int32_t>::max()),DECLARED_TRAIT);
	c->setVariableByQName("MIN_VALUE","",new Integer(numeric_limits<int32_t>::min()),DECLARED_TRAIT);
	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(Integer::_toString),DYNAMIC_TRAIT);
}

void Integer::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	out->writeByte(amf3::integer_marker);
	out->writeU29(val);
}

tiny_string UInteger::toString()
{
	return UInteger::toString(val);
}

/* static helper function */
tiny_string UInteger::toString(uint32_t val)
{
	char buf[20];
	snprintf(buf,sizeof(buf),"%u",val);
	return tiny_string(buf,true);
}

TRISTATE UInteger::isLess(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER ||
	   o->getObjectType()==T_UINTEGER || 
	   o->getObjectType()==T_BOOLEAN)
	{
		uint32_t val1=val;
		int32_t val2=o->toInt();
		if(val2<0)
			return TFALSE;
		else
			return (val1<(uint32_t)val2)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		Number* i=static_cast<Number*>(o);
		if(std::isnan(i->toNumber())) return TUNDEFINED;
		return (val < i->toUInt())?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NULL)
	{
		// UInteger is never less than int(null) == 0
		return TFALSE;
	}
	else if(o->getObjectType()==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else if(o->getObjectType()==T_STRING)
	{
		double val2=o->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
	else
	{
		double val2=o->toPrimitive()->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
}

ASFUNCTIONBODY(UInteger,generator)
{
	return abstract_ui(args[0]->toUInt());
}

const number_t Number::NaN = numeric_limits<double>::quiet_NaN();

number_t ASObject::toNumber()
{
	switch(this->getObjectType())
	{
	case T_UNDEFINED:
		return Number::NaN;
	case T_NULL:
		return +0;
	case T_BOOLEAN:
		return as<Boolean>()->val ? 1 : 0;
	case T_NUMBER:
		return as<Number>()->val;
	case T_INTEGER:
		return as<Integer>()->val;
	case T_UINTEGER:
		return as<UInteger>()->val;
	case T_STRING:
		return as<ASString>()->toNumber();
	default:
		//everything else is an Object regarding to the spec
		return toPrimitive(NUMBER_HINT)->toNumber();
	}
}

bool Number::isEqual(ASObject* o)
{
	switch(o->getObjectType())
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_STRING:
		case T_BOOLEAN:
			return val==o->toNumber();
		case T_NULL:
		case T_UNDEFINED:
			return false;
		default:
			return o->isEqual(this);
	}
}

TRISTATE Number::isLess(ASObject* o)
{
	if(std::isnan(val))
		return TUNDEFINED;
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return (val<i->val)?TTRUE:TFALSE;
	}
	if(o->getObjectType()==T_UINTEGER)
	{
		const UInteger* i=static_cast<const UInteger*>(o);
		return (val<i->val)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		const Number* i=static_cast<const Number*>(o);
		if(std::isnan(i->val)) return TUNDEFINED;
		return (val<i->val)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_BOOLEAN)
	{
		return (val<o->toNumber())?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_UNDEFINED)
	{
		//Undefined is NaN, so the result is undefined
		return TUNDEFINED;
	}
	else if(o->getObjectType()==T_STRING)
	{
		double val2=o->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
	else if(o->getObjectType()==T_NULL)
	{
		return (val<0)?TTRUE:TFALSE;
	}
	else
	{
		double val2=o->toPrimitive()->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (val<val2)?TTRUE:TFALSE;
	}
}

/*
 * This purges trailing zeros from the right, i.e.
 * "144124.45600" -> "144124.456"
 * "144124.000" -> "144124"
 * "144124.45600e+12" -> "144124.456e+12"
 * "144124.45600e+07" -> 144124.456e+7"
 * and it transforms the ',' into a '.' if found.
 */
void Number::purgeTrailingZeroes(char* buf)
{
	int i=strlen(buf)-1;
	int Epos = 0;
	if(i>4 && buf[i-3] == 'e')
	{
		Epos = i-3;
		i=i-4;
	}
	for(;i>0;i--)
	{
		if(buf[i]!='0')
			break;
	}
	bool commaFound=false;
	if(buf[i]==',' || buf[i]=='.')
	{
		i--;
		commaFound=true;
	}
	if(Epos)
	{
		//copy e+12 to the current place
		strncpy(buf+i+1,buf+Epos,5);
		if(buf[i+3] == '0')
		{
			//this looks like e+07, so turn it into e+7
			buf[i+3] = buf[i+4];
			buf[i+4] = '\0';
		}
	}
	else
		buf[i+1]='\0';

	if(commaFound)
		return;

	//Also change the comma to the point if needed
	for(;i>0;i--)
	{
		if(buf[i]==',')
		{
			buf[i]='.';
			break;
		}
	}
}

ASFUNCTIONBODY(Number,_toString)
{
	if(Class<Number>::getClass()->prototype == obj)
		return Class<ASString>::getInstanceS("0");
	if(!obj->is<Number>())
		throw Class<TypeError>::getInstanceS("Number.toString is not generic");
	Number* th=static_cast<Number*>(obj);
	int radix=10;
	char buf[20];
	if(argslen==1)
		radix=args[0]->toUInt();
	assert_and_throw(radix==10 || radix==16);

	if(radix==10)
	{
		//see e 15.7.4.2
		return Class<ASString>::getInstanceS(th->toString());
	}
	else if(radix==16)
		snprintf(buf,20,"%"PRIx64,(int64_t)th->val);

	return Class<ASString>::getInstanceS(buf);
}

ASFUNCTIONBODY(Number,generator)
{
	if(argslen==0)
		return abstract_d(0.);
	else
		return abstract_d(args[0]->toNumber());
}

tiny_string Number::toString()
{
	return Number::toString(val);
}

/* static helper function */
tiny_string Number::toString(number_t val)
{
	if(std::isnan(val))
		return "NaN";
	if(std::isinf(val))
	{
		if(val > 0)
			return "Infinity";
		else
			return "-Infinity";
	}
	if(val == 0) //this also handles the case '-0'
		return "0";

	//See ecma3 8.9.1
	char buf[40];
	if(fabs(val) >= 1e+21 || fabs(val) <= 1e-6)
		snprintf(buf,40,"%.15e",val);
	else
		snprintf(buf,40,"%.15f",val);
	purgeTrailingZeroes(buf);
	return tiny_string(buf,true);
}

void Number::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	//Must create and link the number the hard way
	Number* ninf=new Number(-numeric_limits<double>::infinity());
	Number* pinf=new Number(numeric_limits<double>::infinity());
	Number* pmax=new Number(numeric_limits<double>::max());
	Number* pmin=new Number(numeric_limits<double>::min());
	Number* pnan=new Number(numeric_limits<double>::quiet_NaN());
	ninf->setClass(c);
	pinf->setClass(c);
	pmax->setClass(c);
	pmin->setClass(c);
	pnan->setClass(c);
	c->setVariableByQName("NEGATIVE_INFINITY","",ninf,DECLARED_TRAIT);
	c->setVariableByQName("POSITIVE_INFINITY","",pinf,DECLARED_TRAIT);
	c->setVariableByQName("MAX_VALUE","",pmax,DECLARED_TRAIT);
	c->setVariableByQName("MIN_VALUE","",pmin,DECLARED_TRAIT);
	c->setVariableByQName("NaN","",pnan,DECLARED_TRAIT);
	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(Number::_toString),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY(Number,_constructor)
{
	Number* th=static_cast<Number*>(obj);
	if(args && argslen==1)
		th->val=args[0]->toNumber();
	else
		th->val=0;
	return NULL;
}

void Number::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	out->writeByte(amf3::double_marker);
	//We have to write the double in network byte order (big endian)
	const uint64_t* tmpPtr=reinterpret_cast<const uint64_t*>(&val);
	uint64_t bigEndianVal=GINT64_FROM_BE(*tmpPtr);
	uint8_t* bigEndianPtr=reinterpret_cast<uint8_t*>(&bigEndianVal);

	for(uint32_t i=0;i<8;i++)
		out->writeByte(bigEndianPtr[i]);
}

IFunction::IFunction():inClass(NULL),length(0)
{
	type=T_FUNCTION;
	prototype = _MR(new_asobject());
	prototype->setprop_prototype(Class<ASObject>::getClass()->prototype);
}

void IFunction::finalize()
{
	ASObject::finalize();
	closure_this.reset();
}

ASFUNCTIONBODY_GETTER_SETTER(IFunction,prototype);
ASFUNCTIONBODY_GETTER(IFunction,length);

ASFUNCTIONBODY(IFunction,apply)
{
	/* This function never changes the 'this' pointer of a method closure */
	IFunction* th=static_cast<IFunction*>(obj);
	assert_and_throw(argslen<=2);

	ASObject* newObj=NULL;
	ASObject** newArgs=NULL;
	int newArgsLen=0;
	//Validate parameters
	if(argslen==0 || args[0]->is<Null>() || args[0]->is<Undefined>())
	{
		newObj=getVm()->curGlobalObj;
		newObj->incRef();
	}
	else
	{
		newObj=args[0];
		newObj->incRef();
	}
	if(argslen == 2 && args[1]->getObjectType()==T_ARRAY)
	{
		Array* array=Class<Array>::cast(args[1]);

		newArgsLen=array->size();
		newArgs=new ASObject*[newArgsLen];
		for(int i=0;i<newArgsLen;i++)
		{
			newArgs[i]=array->at(i);
			newArgs[i]->incRef();
		}
	}

	ASObject* ret=th->call(newObj,newArgs,newArgsLen);
	delete[] newArgs;
	return ret;
}

ASFUNCTIONBODY(IFunction,_call)
{
	/* This function never changes the 'this' pointer of a method closure */
	IFunction* th=static_cast<IFunction*>(obj);
	ASObject* newObj=NULL;
	ASObject** newArgs=NULL;
	uint32_t newArgsLen=0;
	if(argslen==0 || args[0]->is<Null>() || args[0]->is<Undefined>())
	{
		newObj=getVm()->curGlobalObj;
		newObj->incRef();
	}
	else
	{
		newObj=args[0];
		newObj->incRef();
	}
	if(argslen > 1)
	{
		newArgsLen=argslen-1;
		newArgs=new ASObject*[newArgsLen];
		for(unsigned int i=0;i<newArgsLen;i++)
		{
			newArgs[i]=args[i+1];
			newArgs[i]->incRef();
		}
	}
	ASObject* ret=th->call(newObj,newArgs,newArgsLen);
	delete[] newArgs;
	return ret;
}

ASFUNCTIONBODY(IFunction,_toString)
{
	return Class<ASString>::getInstanceS("function Function() {}");
}

ASObject *IFunction::describeType() const
{
	xmlpp::DomParser p;
	xmlpp::Element* root=p.get_document()->create_root_node("type");

	root->set_attribute("name", "Function");
	root->set_attribute("base", "Object");
	root->set_attribute("isDynamic", "true");
	root->set_attribute("isFinal", "false");
	root->set_attribute("isStatic", "false");

	xmlpp::Element* node=root->add_child("extendsClass");
	node->set_attribute("type", "Object");

	// TODO: accessor
	LOG(LOG_NOT_IMPLEMENTED, "describeType for Function not completely implemented");

	return Class<XML>::getInstanceS(root);
}

SyntheticFunction::SyntheticFunction(method_info* m):hit_count(0),mi(m),val(NULL)
{
	if(mi)
		length = mi->numArgs();
}

void SyntheticFunction::finalize()
{
	IFunction::finalize();
	func_scope.clear();
}

/**
 * This prepares a new call_context and then executes the ABC bytecode function
 * by ABCVm::executeFunction() or through JIT.
 * It consumes one reference of obj and one of each arg
 */
ASObject* SyntheticFunction::call(ASObject* obj, ASObject* const* args, uint32_t numArgs)
{
	const int hit_threshold=10;
	assert_and_throw(mi->body);

	if(ABCVm::cur_recursion == getVm()->limits.max_recursion)
	{
		for(uint32_t i=0;i<numArgs;i++)
			args[i]->decRef();
		obj->decRef();
		throw Class<ASError>::getInstanceS("Error #1023: Stack overflow occurred");
	}

	/* resolve argument and return types */
	if(!mi->returnType)
	{
		mi->hasExplicitTypes = false;
		mi->paramTypes.reserve(mi->numArgs());
		for(size_t i=0;i < mi->numArgs();++i)
		{
			const Type* t = Type::getTypeFromMultiname(mi->paramTypeName(i));
			mi->paramTypes.push_back(t);
			if(t != Type::anyType)
				mi->hasExplicitTypes = true;
		}

		const Type* t = Type::getTypeFromMultiname(mi->returnTypeName());
		mi->returnType = t;
	}

	if(numArgs < mi->numArgs()-mi->option_count)
	{
		/* Not enough arguments provided.
		 * We throw if this is a method.
		 * We won't throw if all arguments are of 'Any' type.
		 * This is in accordance with the proprietary player. */
		if(isMethod() || mi->hasExplicitTypes)
			throw Class<ArgumentError>::getInstanceS("Not enough arguments provided");
	}

	//Temporarily disable JITting
	if(!mi->body->exception_count && getSys()->useJit && (hit_count==hit_threshold || getSys()->useInterpreter==false))
	{
		//We passed the hot function threshold, synt the function
		val=mi->synt_method();
		assert_and_throw(val);
	}

	//Prepare arguments
	uint32_t args_len=mi->numArgs();
	int passedToLocals=imin(numArgs,args_len);
	uint32_t passedToRest=(numArgs > args_len)?(numArgs-mi->numArgs()):0;

	/* setup argumentsArray if needed */
	Array* argumentsArray = NULL;
	if(mi->needsArgs())
	{
		//The arguments does not contain default values of optional parameters,
		//i.e. f(a,b=3) called as f(7) gives arguments = { 7 }
		argumentsArray=Class<Array>::getInstanceS();
		argumentsArray->resize(numArgs);
		for(uint32_t j=0;j<numArgs;j++)
		{
			args[j]->incRef();
			argumentsArray->set(j,args[j]);
		}
		//Add ourself as the callee property
		incRef();
		argumentsArray->setVariableByQName("callee","",this,DECLARED_TRAIT);
	}

	/* setup call_context */
	call_context cc;
	cc.inClass = inClass;
	cc.mi=mi;
	cc.locals_size=mi->body->local_count+1;
	ASObject** locals = g_newa(ASObject*, cc.locals_size);
	cc.locals=locals;
	memset(cc.locals,0,sizeof(ASObject*)*cc.locals_size);
	cc.max_stack = mi->body->max_stack;
	ASObject** stack = g_newa(ASObject*, cc.max_stack);
	cc.stack=stack;
	cc.stack_index=0;
	cc.context=mi->context;
	//cc.code= new istringstream(mi->body->code);
	cc.scope_stack=func_scope;
	cc.initialScopeStack=func_scope.size();
	cc.exec_pos=0;
	getVm()->curGlobalObj = ABCVm::getGlobalScope(&cc);

	if(isBound())
	{ /* closure_this can never been overriden */
		LOG(LOG_CALLS,_("Calling with closure ") << this);
		if(obj)
			obj->decRef();
		obj=closure_this.getPtr();
		obj->incRef();
	}

	assert_and_throw(obj);
	obj->incRef(); //this is free'd in ~call_context
	cc.locals[0]=obj;

	/* coerce arguments to expected types */
	for(int i=0;i<passedToLocals;++i)
	{
		cc.locals[i+1] = mi->paramTypes[i]->coerce(args[i]);
	}

	//Fill missing parameters until optional parameters begin
	//like fun(a,b,c,d=3,e=5) called as fun(1,2) becomes
	//locals = {this, 1, 2, Undefined, 3, 5}
	for(uint32_t i=passedToLocals;i<args_len;++i)
	{
		int iOptional = mi->option_count-args_len+i;
		if(iOptional >= 0)
			cc.locals[i+1]=mi->paramTypes[i]->coerce(mi->getOptional(iOptional));
		else {
			assert(mi->paramTypes[i] == Type::anyType);
			cc.locals[i+1]=new Undefined();
		}
	}

	assert_and_throw(mi->needsArgs()==false || mi->needsRest()==false);
	if(mi->needsRest()) //TODO
	{
		Array* rest=Class<Array>::getInstanceS();
		rest->resize(passedToRest);
		for(uint32_t j=0;j<passedToRest;j++)
			rest->set(j,args[passedToLocals+j]);

		assert_and_throw(cc.locals_size>args_len+1);
		cc.locals[args_len+1]=rest;
	}
	else if(mi->needsArgs())
	{
		assert_and_throw(cc.locals_size>args_len+1);
		cc.locals[args_len+1]=argumentsArray;
	}
	//Parameters are ready

	ASObject* ret;
	//obtain a local reference to this function, as it may delete itself
	this->incRef();

	ABCVm::cur_recursion++; //increment current recursion depth
	Log::calls_indent++;
	while (true)
	{
		try
		{
			if(mi->body->exception_count || (val==NULL && getSys()->useInterpreter))
			{
				//This is not a hot function, execute it using the interpreter
				ret=ABCVm::executeFunction(this,&cc);
			}
			else
				ret=val(&cc);
		}
		catch (ASObject* excobj) // Doesn't have to be an ASError at all.
		{
			unsigned int pos = cc.exec_pos;
			bool no_handler = true;

			LOG(LOG_TRACE, "got an " << excobj->toString());
			LOG(LOG_TRACE, "pos=" << pos);
			for (unsigned int i=0;i<mi->body->exception_count;i++)
			{
				exception_info exc=mi->body->exceptions[i];
				multiname* name=mi->context->getMultiname(exc.exc_type, &cc);
				LOG(LOG_TRACE, "f=" << exc.from << " t=" << exc.to);
				if (pos > exc.from && pos <= exc.to && ABCContext::isinstance(excobj, name))
				{
					no_handler = false;
					cc.exec_pos = (uint32_t)exc.target;
					cc.runtime_stack_clear();
					cc.runtime_stack_push(excobj);
					cc.scope_stack.clear();
					cc.scope_stack=func_scope;
					cc.initialScopeStack=func_scope.size();
					break;
				}
			}
			if (no_handler)
			{
				ABCVm::cur_recursion--; //decrement current recursion depth
				Log::calls_indent--;
				throw excobj;
			}
			continue;
		}
		break;
	}
	ABCVm::cur_recursion--; //decrement current recursion depth
	Log::calls_indent--;

	hit_count++;

	this->decRef(); //free local ref
	obj->decRef();

	if(ret==NULL)
		ret=new Undefined;

	return mi->returnType->coerce(ret);
}

/**
 * This executes a C++ function.
 * It consumes one reference of obj and one of each arg
 */
ASObject* Function::call(ASObject* obj, ASObject* const* args, uint32_t num_args)
{
	/*
	 * We do not enforce ABCVm::limits.max_recursion here.
	 * This should be okey, because there is no infinite recursion
	 * using only builtin functions.
	 * Additionally, we still need to run builtin code (such as the ASError constructor) when
	 * ABCVm::limits.max_recursion is reached in SyntheticFunction::call.
	 */
	ASObject* ret;
	if(isBound())
	{ /* closure_this can never been overriden */
		LOG(LOG_CALLS,_("Calling with closure ") << this);
		if(obj)
			obj->decRef();
		obj=closure_this.getPtr();
		obj->incRef();
	}
	assert_and_throw(obj);
	ret=val(obj,args,num_args);

	for(uint32_t i=0;i<num_args;i++)
		args[i]->decRef();
	obj->decRef();
	if(ret==NULL)
		ret=new Undefined;
	return ret;
}

bool Null::isEqual(ASObject* r)
{
	switch(r->getObjectType())
	{
		case T_NULL:
		case T_UNDEFINED:
			return true;
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_STRING:
		case T_BOOLEAN:
			return false;
		default:
			return r->isEqual(this);
	}
}

TRISTATE Null::isLess(ASObject* r)
{
	if(r->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(r);
		return (0<i->toInt())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_UINTEGER)
	{
		UInteger* i=static_cast<UInteger*>(r);
		return (0<i->toUInt())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NUMBER)
	{
		Number* i=static_cast<Number*>(r);
		if(std::isnan(i->toNumber())) return TUNDEFINED;
		return (0<i->toNumber())?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_BOOLEAN)
	{
		Boolean* i=static_cast<Boolean*>(r);
		return (0<i->val)?TTRUE:TFALSE;
	}
	else if(r->getObjectType()==T_NULL)
	{
		return TFALSE;
	}
	else if(r->getObjectType()==T_UNDEFINED)
	{
		return TUNDEFINED;
	}
	else if(r->getObjectType()==T_STRING)
	{
		double val2=r->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
	else
	{
		double val2=r->toPrimitive()->toNumber();
		if(std::isnan(val2)) return TUNDEFINED;
		return (0<val2)?TTRUE:TFALSE;
	}
}

int Null::toInt()
{
	return 0;
}

void Null::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	throw UnsupportedException("Null::serialize not implemented");
}

RegExp::RegExp():global(false),ignoreCase(false),extended(false),multiline(false),lastIndex(0)
{
}

void RegExp::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("exec",AS3,Class<IFunction>::getFunction(exec),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("test",AS3,Class<IFunction>::getFunction(test),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("global","",Class<IFunction>::getFunction(_getGlobal),GETTER_METHOD,true);
}

void RegExp::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(RegExp,_constructor)
{
	RegExp* th=static_cast<RegExp*>(obj);
	if(argslen > 0)
		th->re=args[0]->toString().raw_buf();
	if(argslen>1)
	{
		const tiny_string& flags=args[1]->toString();
		for(auto i=flags.begin();i!=flags.end();++i)
		{
			switch(*i)
			{
				case 'g':
					th->global=true;
					break;
				case 'i':
					th->ignoreCase=true;
					break;
				case 'x':
					th->extended=true;
					break;
				case 'm':
					th->multiline=true;
					break;
				case 's':
					throw UnsupportedException("RegExp not completely implemented");

			}
		}
	}
	return NULL;
}

ASFUNCTIONBODY(RegExp,_getGlobal)
{
	RegExp* th=static_cast<RegExp*>(obj);
	return abstract_b(th->global);
}

ASFUNCTIONBODY(RegExp,exec)
{
	RegExp* th=static_cast<RegExp*>(obj);
	assert_and_throw(argslen==1);
	const tiny_string& arg0=args[0]->toString();
	const char* error;
	int errorOffset;
	int options=PCRE_UTF8;
	if(th->ignoreCase)
		options|=PCRE_CASELESS;
	if(th->extended)
		options|=PCRE_EXTENDED;
	if(th->multiline)
		options|=PCRE_MULTILINE;
	pcre* pcreRE=pcre_compile(th->re.raw_buf(), options, &error, &errorOffset,NULL);
	if(error)
		return new Null;
	//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
	int capturingGroups;
	int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return new Null;
	}
	assert_and_throw(capturingGroups<10);
	//Get information about named capturing groups
	int namedGroups;
	infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_NAMECOUNT, &namedGroups);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return new Null;
	}
	//Get information about the size of named entries
	int namedSize;
	infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_NAMEENTRYSIZE, &namedSize);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return new Null;
	}
	struct nameEntry
	{
		uint16_t number;
		char name[0];
	};
	char* entries;
	infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_NAMETABLE, &entries);
	if(infoOk!=0)
	{
		pcre_free(pcreRE);
		return new Null;
	}

	int ovector[30];
	int offset=(th->global)?th->lastIndex:0;
	int rc=pcre_exec(pcreRE, NULL, arg0.raw_buf(), arg0.numBytes(), offset, 0, ovector, 30);
	if(rc<0)
	{
		//No matches or error
		pcre_free(pcreRE);
		return new Null;
	}
	Array* a=Class<Array>::getInstanceS();
	//Push the whole result and the captured strings
	for(int i=0;i<capturingGroups+1;i++)
		a->push(Class<ASString>::getInstanceS( arg0.substr_bytes(ovector[i*2],ovector[i*2+1]-ovector[i*2]) ));
	args[0]->incRef();
	a->setVariableByQName("input","",args[0],DYNAMIC_TRAIT);
	a->setVariableByQName("index","",abstract_i(ovector[0]),DYNAMIC_TRAIT);
	for(int i=0;i<namedGroups;i++)
	{
		nameEntry* entry=(nameEntry*)entries;
		uint16_t num=GINT16_FROM_BE(entry->number);
		ASObject* captured=a->at(num);
		captured->incRef();
		a->setVariableByQName(entry->name,"",captured,DYNAMIC_TRAIT);
		entries+=namedSize;
	}
	th->lastIndex=ovector[1];
	pcre_free(pcreRE);
	return a;
}

ASFUNCTIONBODY(RegExp,test)
{
	RegExp* th=static_cast<RegExp*>(obj);

	const tiny_string& arg0 = args[0]->toString();

	int options = PCRE_UTF8;
	if(th->ignoreCase)
		options |= PCRE_CASELESS;
	if(th->extended)
		options |= PCRE_EXTENDED;
	if(th->multiline)
		options |= PCRE_MULTILINE;

	const char * error;
	int errorOffset;
	pcre * pcreRE = pcre_compile(th->re.raw_buf(), options, &error, &errorOffset, NULL);
	if(error)
		return new Null;

	int ovector[30];
	int offset=(th->global)?th->lastIndex:0;
	int rc = pcre_exec(pcreRE, NULL, arg0.raw_buf(), arg0.numBytes(), offset, 0, ovector, 30);
	bool ret = (rc >= 0);
	pcre_free(pcreRE);

	return abstract_b(ret);
}

ASFUNCTIONBODY(ASString,slice)
{
	ASString* th=static_cast<ASString*>(obj);
	int startIndex=0;
	if(argslen>=1)
		startIndex=args[0]->toInt();
	if(startIndex<0) {
		startIndex=th->data.numChars()+startIndex;
		if(startIndex<0)
			startIndex=0;
	}
	if(startIndex>(int)th->data.numChars())
		startIndex=th->data.numChars();

	int endIndex=0x7fffffff;
	if(argslen>=2)
		endIndex=args[1]->toInt();
	if(endIndex<0) {
		endIndex=th->data.numChars()+endIndex;
		if(endIndex<0)
			endIndex=0;
	}
	if(endIndex>(int)th->data.numChars())
		endIndex=th->data.numChars();

	if(endIndex<=startIndex)
		return Class<ASString>::getInstanceS("");
	else
		return Class<ASString>::getInstanceS(th->data.substr(startIndex,endIndex-startIndex));
}

ASFUNCTIONBODY(ASString,charAt)
{
	ASString* th=static_cast<ASString*>(obj);
	int index=args[0]->toInt();
	int maxIndex=th->data.numChars();
	if(index<0 || index>=maxIndex)
		return Class<ASString>::getInstanceS();
	return Class<ASString>::getInstanceS( tiny_string::fromChar(th->data.charAt(index)) );
}

ASFUNCTIONBODY(ASString,charCodeAt)
{
	ASString* th=static_cast<ASString*>(obj);
	unsigned int index=args[0]->toInt();
	if(index<th->data.numChars())
	{
		//Character codes are expected to be positive
		return abstract_i(th->data.charAt(index));
	}
	else
		return abstract_d(Number::NaN);
}

ASFUNCTIONBODY(ASString,indexOf)
{
	ASString* th=static_cast<ASString*>(obj);
	tiny_string arg0=args[0]->toString();
	int startIndex=0;
	if(argslen>1)
		startIndex=args[1]->toInt();

	size_t pos = th->data.find(arg0.raw_buf(), startIndex);
	if(pos == th->data.npos)
		return abstract_i(-1);
	else
		return abstract_i(pos);
}

ASFUNCTIONBODY(ASString,lastIndexOf)
{
	assert_and_throw(argslen==1 || argslen==2);
	ASString* th=static_cast<ASString*>(obj);
	tiny_string val=args[0]->toString();
	size_t startIndex=th->data.npos;
	if(argslen > 1 && args[1]->getObjectType() != T_UNDEFINED && !std::isnan(args[1]->toNumber()))
	{
		int32_t i = args[1]->toInt();
		if(i<0)
			return abstract_i(-1);
		startIndex = i;
	}

	size_t pos=th->data.rfind(val.raw_buf(), startIndex);
	if(pos==th->data.npos)
		return abstract_i(-1);
	else
		return abstract_i(pos);
}

ASFUNCTIONBODY(ASString,toLowerCase)
{
	ASString* th=static_cast<ASString*>(obj);
	return Class<ASString>::getInstanceS(th->data.lowercase());
}

ASFUNCTIONBODY(ASString,toUpperCase)
{
	ASString* th=static_cast<ASString*>(obj);
	return Class<ASString>::getInstanceS(th->data.uppercase());
}

ASFUNCTIONBODY(ASString,fromCharCode)
{
	ASString* ret=Class<ASString>::getInstanceS();
	for(uint32_t i=0;i<argslen;i++)
	{
		ret->data += tiny_string::fromChar(args[i]->toUInt());
	}
	return ret;
}

ASFUNCTIONBODY(ASString,replace)
{
	ASString* th=static_cast<ASString*>(obj);
	enum REPLACE_TYPE { STRING=0, FUNC };
	REPLACE_TYPE type;
	ASString* ret=Class<ASString>::getInstanceS(th->data);

	tiny_string replaceWith;
	if(argslen < 2)
	{
		type = STRING;
		replaceWith="";
	}
	else if(args[1]->getObjectType()!=T_FUNCTION)
	{
		type = STRING;
		replaceWith=args[1]->toString();
	}
	else
		type = FUNC;

	if(args[0]->getClass()==Class<RegExp>::getClass())
	{
		RegExp* re=static_cast<RegExp*>(args[0]);

		const char* error;
		int errorOffset;
		int options=PCRE_UTF8;
		if(re->ignoreCase)
			options|=PCRE_CASELESS;
		if(re->extended)
			options|=PCRE_EXTENDED;
		if(re->multiline)
			options|=PCRE_MULTILINE;
		pcre* pcreRE=pcre_compile(re->re.raw_buf(), options, &error, &errorOffset,NULL);
		if(error)
			return ret;
		//Verify that 30 for ovector is ok, it must be at least (captGroups+1)*3
		int capturingGroups;
		int infoOk=pcre_fullinfo(pcreRE, NULL, PCRE_INFO_CAPTURECOUNT, &capturingGroups);
		if(infoOk!=0)
		{
			pcre_free(pcreRE);
			return ret;
		}
		assert_and_throw(capturingGroups<20);
		int ovector[60];
		int offset=0;
		int retDiff=0;
		do
		{
			int rc=pcre_exec(pcreRE, NULL, ret->data.raw_buf(), ret->data.numBytes(), offset, 0, ovector, 60);
			if(rc<0)
			{
				//No matches or error
				pcre_free(pcreRE);
				return ret;
			}
			if(type==FUNC)
			{
				//Get the replace for this match
				IFunction* f=static_cast<IFunction*>(args[1]);
				ASObject** subargs = g_newa(ASObject*, 3+capturingGroups);
				//we index on bytes, not on UTF-8 characters
				subargs[0]=Class<ASString>::getInstanceS(ret->data.substr_bytes(ovector[0],ovector[1]-ovector[0]));
				for(int i=0;i<capturingGroups;i++)
					subargs[i+1]=Class<ASString>::getInstanceS(ret->data.substr_bytes(ovector[i*2+2],ovector[i*2+3]-ovector[i*2+2]));
				subargs[capturingGroups+1]=abstract_i(ovector[0]-retDiff);
				th->incRef();
				subargs[capturingGroups+2]=th;
				ASObject* ret=f->call(new Null, subargs, 3+capturingGroups);
				replaceWith=ret->toString().raw_buf();
				ret->decRef();
			} else {
					size_t pos, ipos = 0;
					tiny_string group;
					int i, j;
					while((pos = replaceWith.find("$", ipos)) != tiny_string::npos) {
						i = 0;
						ipos = pos;
						while (++ipos < replaceWith.numChars()) {
						j = replaceWith.charAt(ipos)-'0';
							if (j <0 || j> 9)
								break;
							i = 10*i + j;
						}
						if (i == 0)
							continue;
						group = ret->data.substr_bytes(ovector[i*2], ovector[i*2+1]-ovector[i*2]);
						replaceWith.replace(pos, ipos-pos, group);
					}
			}
			ret->data.replace(ovector[0],ovector[1]-ovector[0],replaceWith);
			offset=ovector[0]+replaceWith.numBytes();
			retDiff+=replaceWith.numBytes()-(ovector[1]-ovector[0]);
		}
		while(re->global);

		pcre_free(pcreRE);
	}
	else
	{
		const tiny_string& s=args[0]->toString();
		int index=ret->data.find(s,0);
		if(index==-1) //No result
			return ret;
		assert_and_throw(type==STRING);
		ret->data.replace(index,s.numChars(),replaceWith);
	}

	return ret;
}

ASFUNCTIONBODY(ASString,concat)
{
	ASString* th=static_cast<ASString*>(obj);
	ASString* ret=Class<ASString>::getInstanceS(th->data);
	for(unsigned int i=0;i<argslen;i++)
		ret->data+=args[i]->toString().raw_buf();

	return ret;
}

ASFUNCTIONBODY(ASString,generator)
{
	assert(argslen==1);
	return Class<ASString>::getInstanceS(args[0]->toString());
}

ASObject* Void::coerce(ASObject* o) const
{
	if(!o->is<Undefined>())
		throw Class<TypeError>::getInstanceS("Trying to coerce o!=undefined to void");
	return o;
}

bool Type::isTypeResolvable(const multiname* mn)
{
	assert_and_throw(mn->isQName());
	assert(mn->name_type == multiname::NAME_STRING);
	if(mn == 0)
		return true; //any
	if(mn->name_type == multiname::NAME_STRING && mn->name_s=="any"
		&& mn->ns[0].name == "")
		return true;
	if(mn->name_type == multiname::NAME_STRING && mn->name_s=="void"
		&& mn->ns[0].name == "")
		return true;

	//Check if the class has already been defined
	auto i = getSys()->classes.find(QName(mn->name_s, mn->ns[0].name));
	return i != getSys()->classes.end();
}

/*
 * This should only be called after all global objects have been created
 * by running ABCContext::exec() for all ABCContexts.
 * Therefore, all classes are at least declared.
 */
const Type* Type::getTypeFromMultiname(const multiname* mn)
{
	if(mn == 0) //multiname idx zero indicates any type
		return Type::anyType;

	if(mn->name_type == multiname::NAME_STRING && mn->name_s=="any"
		&& mn->ns.size() == 1 && mn->ns[0].name == "")
		return Type::anyType;

	if(mn->name_type == multiname::NAME_STRING && mn->name_s=="void"
		&& mn->ns.size() == 1 && mn->ns[0].name == "")
		return Type::voidType;

	ASObject* typeObject;
	/*
	 * During the newClass opcode, the class is added to sys->classes.
	 * The class variable in the global scope is only set a bit later.
	 * When the class has to be resolved in between (for example, the
	 * class has traits of the class's type), then we'll find it in
	 * sys->classes, but getGlobal()->getVariableAndTargetByMultiname()
	 * would still return "Undefined".
	 */
	auto i = getSys()->classes.find(QName(mn->name_s, mn->ns[0].name));
	if(i != getSys()->classes.end())
		typeObject = i->second;
	else
	{
		ASObject* target;
		typeObject=getGlobal()->getVariableAndTargetByMultiname(*mn,target);
	}

	if(!typeObject)
	{
		//HACK: until we have implemented all flash classes, we need this hack
		LOG(LOG_NOT_IMPLEMENTED,"getTypeFromMultiname: could not find " << *mn << ", using AnyType");
		return Type::anyType;
	}

	assert_and_throw(typeObject->is<Type>());
	return typeObject->as<Type>();
}

Class_base::Class_base(const QName& name):use_protected(false),protected_ns("",PACKAGE_NAMESPACE),constructor(NULL),
	isFinal(false),isSealed(false),context(NULL),class_name(name),class_index(-1)
{
	type=T_CLASS;
}

/*
 * This copies the non-static traits of the super class to this
 * class.
 *
 * If a property is in the protected namespace of the super class, a copy is
 * created with the protected namespace of this class.
 * That is necessary, because superclass methods are called with the protected ns
 * of the current class.
 *
 * use_protns and protectedns must be set before this function is called
 */
void Class_base::copyBorrowedTraitsFromSuper()
{
	assert(Variables.Variables.empty());
	variables_map::var_iterator i = super->Variables.Variables.begin();
	for(;i != super->Variables.Variables.end(); ++i)
	{
		const tiny_string& name = i->first;
		variable& v = i->second;
		//copy only static and instance methods
		if(v.kind != BORROWED_TRAIT)
			continue;
		if(v.var)
			v.var->incRef();
		if(v.getter)
			v.getter->incRef();
		if(v.setter)
			v.setter->incRef();
		const variables_map::var_iterator ret_end=Variables.Variables.upper_bound(name);
		variables_map::var_iterator inserted=Variables.Variables.insert(ret_end,make_pair(name,v));

		//Overwrite protected ns
		if(super->use_protected && v.ns.count(nsNameAndKind(super->protected_ns.name,PROTECTED_NAMESPACE)))
		{
			assert(use_protected);
			//add this classes protected ns
			inserted->second.ns.insert(nsNameAndKind(protected_ns.name,PROTECTED_NAMESPACE));
		}
	}
}


ASObject* Class_base::coerce(ASObject* o) const
{
	if(o->is<Null>())
		return o;
	if(o->is<Undefined>())
	{
		o->decRef();
		return new Null;
	}
	if(o->is<Class_base>())
	{ /* classes can be cast to the type 'Object' or 'Class' */
	       if(this == Class<ASObject>::getClass()
		|| (class_name.name=="Class" && class_name.ns==""))
		       return o; /* 'this' is the type of a class */
	       else
		       throw Class<TypeError>::getInstanceS("Error #1034: Wrong type");
	}
	//o->getClass() == NULL for primitive types
	//those are handled in overloads Class<Number>::coerce etc.
	if(!o->getClass() || !o->getClass()->isSubClass(this))
		throw Class<TypeError>::getInstanceS("Error #1034: Wrong type");
	return o;
}

ASFUNCTIONBODY(Class_base,_toString)
{
	Class_base* th = obj->as<Class_base>();
	tiny_string ret;
	ret = "[class ";
	ret += th->class_name.name;
	ret += "]";
	return Class<ASString>::getInstanceS(ret);
}

void Class_base::addPrototypeGetter()
{
	setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(_getter_prototype),GETTER_METHOD,false);
}

Class_base::~Class_base()
{
	if(!referencedObjects.empty())
		LOG(LOG_ERROR,_("Class destroyed without cleanUp called"));
}

ASFUNCTIONBODY_GETTER(Class_base, prototype);

ASObject* Class_base::generator(ASObject* const* args, const unsigned int argslen)
{
	ASObject *ret=ASObject::generator(NULL, args, argslen);
	for(unsigned int i=0;i<argslen;i++)
		args[i]->decRef();
	return ret;
}

void Class_base::addImplementedInterface(const multiname& i)
{
	interfaces.push_back(i);
}

void Class_base::addImplementedInterface(Class_base* i)
{
	interfaces_added.push_back(i);
}

tiny_string Class_base::toString()
{
	tiny_string ret="[Class ";
	ret+=class_name.name;
	ret+="]";
	return ret;
}

void Class_base::recursiveBuild(ASObject* target)
{
	if(super)
		super->recursiveBuild(target);

	LOG(LOG_TRACE,_("Building traits for ") << class_name);
	buildInstanceTraits(target);
}

void Class_base::setConstructor(IFunction* c)
{
	assert_and_throw(constructor==NULL);
	constructor=c;
}

void Class_base::handleConstruction(ASObject* target, ASObject* const* args, unsigned int argslen, bool buildAndLink)
{
	if(buildAndLink)
	{
	#ifndef NDEBUG
		assert_and_throw(!target->initialized);
	#endif
		//HACK: suppress implementation handling of variables just now
		bool bak=target->implEnable;
		target->implEnable=false;
		recursiveBuild(target);
		//And restore it
		target->implEnable=bak;
	#ifndef NDEBUG
		target->initialized=true;
	#endif
		/* We set this before the actual call to the constructor
		 * or any superclass constructor
		 * so that functions called from within the constructor see
		 * the object as already constructed.
		 * We also have to set this for objects without constructor,
		 * so they are not tried to buildAndLink again.
		 */
		RELEASE_WRITE(target->constructed,true);
	}

	//As constructors are not binded, we should change here the level
	if(constructor)
	{
		target->incRef();
		ASObject* ret=constructor->call(target,args,argslen);
		assert_and_throw(ret->is<Undefined>());
		ret->decRef();
	}
	if(buildAndLink)
	{
		//Tell the object that the construction is complete
		target->constructionComplete();
	}
}

void Class_base::acquireObject(ASObject* ob)
{
	Locker l(referencedObjectsMutex);
	bool ret=referencedObjects.insert(ob).second;
	assert_and_throw(ret);
}

void Class_base::abandonObject(ASObject* ob)
{
	Locker l(referencedObjectsMutex);
	set<ASObject>::size_type ret=referencedObjects.erase(ob);
	if(ret!=1)
	{
		LOG(LOG_ERROR,_("Failure in reference counting in ") << class_name);
	}
}

void Class_base::finalizeObjects() const
{
	set<ASObject*>::iterator it=referencedObjects.begin();
	for(;it!=referencedObjects.end();)
	{
		//A reference is acquired before finalizing the object, to make sure it will survive
		//the call
		ASObject* tmp=*it;
		tmp->incRef();
		tmp->finalize();
		//Advance the iterator before decReffing the current object (decReffing may destroy the object right now
		it++;
		tmp->decRef();
	}
}

void Class_base::finalize()
{
	finalizeObjects();

	ASObject::finalize();
	if(constructor)
	{
		constructor->decRef();
		constructor=NULL;
	}
}

Template_base::Template_base(QName name) : template_name(name)
{
	type = T_TEMPLATE;
}

Class_object* Class_object::getClass()
{
	//We check if we are registered in the class map
	//if not we register ourselves (see also Class<T>::getClass)
	std::map<QName, Class_base*>::iterator it=getSys()->classes.find(QName("Class",""));
	Class_object* ret=NULL;
	if(it==getSys()->classes.end()) //This class is not yet in the map, create it
	{
		ret=new Class_object();
		getSys()->classes.insert(std::make_pair(QName("Class",""),ret));
	}
	else
		ret=static_cast<Class_object*>(it->second);

	return ret;
}
_R<Class_object> Class_object::getRef()
{
	Class_object* ret = getClass();
	ret->incRef();
	return _MR(ret);
}

const std::vector<Class_base*>& Class_base::getInterfaces() const
{
	if(!interfaces.empty())
	{
		//Recursively get interfaces implemented by this interface
		for(unsigned int i=0;i<interfaces.size();i++)
		{
			ASObject* target;
			ASObject* interface_obj=getGlobal()->getVariableAndTargetByMultiname(interfaces[i], target);
			assert_and_throw(interface_obj && interface_obj->getObjectType()==T_CLASS);
			Class_base* inter=static_cast<Class_base*>(interface_obj);

			interfaces_added.push_back(inter);
			//Probe the interface for its interfaces
			inter->getInterfaces();
		}
		//Clean the interface vector to save some space
		interfaces.clear();
	}
	return interfaces_added;
}

void Class_base::linkInterface(Class_base* c) const
{
	if(class_index==-1)
	{
		//LOG(LOG_NOT_IMPLEMENTED,_("Linking of builtin interface ") << class_name << _(" not supported"));
		return;
	}
	//Recursively link interfaces implemented by this interface
	for(unsigned int i=0;i<getInterfaces().size();i++)
		getInterfaces()[i]->linkInterface(c);

	assert_and_throw(context);

	//Link traits of this interface
	for(unsigned int j=0;j<context->instances[class_index].trait_count;j++)
	{
		traits_info* t=&context->instances[class_index].traits[j];
		context->linkTrait(c,t);
	}

	if(constructor)
	{
		LOG(LOG_CALLS,_("Calling interface init for ") << class_name);
		ASObject* ret=constructor->call(c,NULL,0);
		assert_and_throw(ret==NULL);
	}
}

bool Class_base::isSubClass(const Class_base* cls) const
{
	check();
	if(cls==this || cls==Class<ASObject>::getClass())
		return true;

	//Now check the interfaces
	for(unsigned int i=0;i<getInterfaces().size();i++)
	{
		if(getInterfaces()[i]->isSubClass(cls))
			return true;
	}

	//Now ask the super
	if(super && super->isSubClass(cls))
		return true;
	return false;
}

tiny_string Class_base::getQualifiedClassName() const
{
	//TODO: use also the namespace
	if(class_index==-1)
		return class_name.name;
	else
	{
		assert_and_throw(context);
		int name_index=context->instances[class_index].name;
		assert_and_throw(name_index);
		const multiname* mname=context->getMultiname(name_index,NULL);
		return mname->qualifiedString();
	}
}

ASObject *Class_base::describeType() const
{
	xmlpp::DomParser p;
	xmlpp::Element* root=p.get_document()->create_root_node("type");

	root->set_attribute("name", getQualifiedClassName().raw_buf());
	root->set_attribute("base", "Class");
	root->set_attribute("isDynamic", "true");
	root->set_attribute("isFinal", "true");
	root->set_attribute("isStatic", "true");

	// extendsClass
	xmlpp::Element* extends_class=root->add_child("extendsClass");
	extends_class->set_attribute("type", "Class");
	extends_class=root->add_child("extendsClass");
	extends_class->set_attribute("type", "Object");

	// variable
	if(class_index>=0)
		describeTraits(root, context->classes[class_index].traits);

	// factory
	xmlpp::Element* factory=root->add_child("factory");
	factory->set_attribute("type", getQualifiedClassName().raw_buf());
	describeInstance(factory);
	
	return Class<XML>::getInstanceS(root);
}

void Class_base::describeInstance(xmlpp::Element* root) const
{
	// extendsClass
	const Class_base* c=super.getPtr();
	while(c)
	{
		xmlpp::Element* extends_class=root->add_child("extendsClass");
		extends_class->set_attribute("type", c->getQualifiedClassName().raw_buf());
		c=c->super.getPtr();
	}

	// implementsInterface
	c=this;
	while(c && c->class_index>=0)
	{
		const std::vector<Class_base*>& interfaces=c->getInterfaces();
		auto it=interfaces.begin();
		for(; it!=interfaces.end(); ++it)
		{
			xmlpp::Element* node=root->add_child("implementsInterface");
			node->set_attribute("type", (*it)->getQualifiedClassName().raw_buf());
		}
		c=c->super.getPtr();
	}

	// variables, methods, accessors
	c=this;
	while(c && c->class_index>=0)
	{
		c->describeTraits(root, c->context->instances[c->class_index].traits);
		c=c->super.getPtr();
	}
}

void Class_base::describeTraits(xmlpp::Element* root,
				std::vector<traits_info>& traits) const
{
	std::map<u30, xmlpp::Element*> accessorNodes;
	for(unsigned int i=0;i<traits.size();i++)
	{
		traits_info& t=traits[i];
		int kind=t.kind&0xf;
		multiname* mname=context->getMultiname(t.name,NULL);
		if (mname->name_type!=multiname::NAME_STRING ||
		    (mname->ns.size()==1 && mname->ns[0].name!="") ||
		    mname->ns.size() > 1)
			continue;
		
		if(kind==traits_info::Slot || kind==traits_info::Const)
		{
			multiname* type=context->getMultiname(t.type_name,NULL);
			assert(type->name_type==multiname::NAME_STRING);

			const char *nodename=kind==traits_info::Const?"constant":"variable";
			xmlpp::Element* node=root->add_child(nodename);
			node->set_attribute("name", mname->name_s.raw_buf());
			node->set_attribute("type", type->name_s.raw_buf());
		}
		else if (kind==traits_info::Method)
		{
			xmlpp::Element* node=root->add_child("method");
			node->set_attribute("name", mname->name_s.raw_buf());
			node->set_attribute("declaredBy", getQualifiedClassName().raw_buf());

			method_info& method=context->methods[t.method];
			const multiname* rtname=method.returnTypeName();
			assert(rtname->name_type==multiname::NAME_STRING);
			node->set_attribute("returnType", rtname->name_s.raw_buf());

			assert(method.numArgs() >= method.option_count);
			uint32_t firstOpt=method.numArgs() - method.option_count;
			for(uint32_t j=0;j<method.numArgs(); j++)
			{
				xmlpp::Element* param=node->add_child("parameter");
				param->set_attribute("index", UInteger::toString(j+1).raw_buf());
				param->set_attribute("type", method.paramTypeName(j)->name_s.raw_buf());
				param->set_attribute("optional", j>=firstOpt?"true":"false");
			}
		}
		else if (kind==traits_info::Getter || kind==traits_info::Setter)
		{
			// The getters and setters are separate
			// traits. Check if we have already created a
			// node for this multiname with the
			// complementary accessor. If we have, update
			// the access attribute to "readwrite".
			xmlpp::Element* node;
			auto existing=accessorNodes.find(t.name);
			if(existing==accessorNodes.end())
			{
				node=root->add_child("accessor");
				accessorNodes[t.name]=node;
			}
			else
				node=existing->second;

			node->set_attribute("name", mname->name_s.raw_buf());

			const char* access=NULL;
			tiny_string oldAccess;
			xmlpp::Attribute* oldAttr=node->get_attribute("access");
			if(oldAttr)
				oldAccess=oldAttr->get_value();

			if(kind==traits_info::Getter && oldAccess=="")
				access="readonly";
			else if(kind==traits_info::Setter && oldAccess=="")
				access="writeonly";
			else if((kind==traits_info::Getter && oldAccess=="writeonly") || 
				(kind==traits_info::Setter && oldAccess=="readonly"))
				access="readwrite";

			if(access)
				node->set_attribute("access", access);

			const char* type=NULL;
			method_info& method=context->methods[t.method];
			if(kind==traits_info::Getter)
			{
				const multiname* rtname=method.returnTypeName();
				assert(rtname->name_type==multiname::NAME_STRING);
				type=rtname->name_s.raw_buf();
			}
			else if(method.numArgs()>0) // setter
			{
				type=method.paramTypeName(0)->name_s.raw_buf();
			}
			if(type)
				node->set_attribute("type", type);

			node->set_attribute("declaredBy", getQualifiedClassName().raw_buf());
		}
	}
}

void ASQName::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(_getURI),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("local_name","",Class<IFunction>::getFunction(_getLocalName),GETTER_METHOD,true);
	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY(ASQName,_constructor)
{
	ASQName* th=static_cast<ASQName*>(obj);
	assert_and_throw(argslen<3);

	ASObject *nameval;
	ASObject *namespaceval;

	if(argslen==0)
	{
		th->local_name="";
		th->uri_is_null=false;
		th->uri="";
		// Should set th->uri to the default namespace
		LOG(LOG_NOT_IMPLEMENTED, "QName constructor not completely implemented");
		return NULL;
	}
	if(argslen==1)
	{
		nameval=args[0];
		namespaceval=NULL;
	}
	else if(argslen==2)
	{
		namespaceval=args[0];
		nameval=args[1];
	}

	// Set local_name
	if(nameval->getObjectType()==T_QNAME)
	{
		ASQName *q=static_cast<ASQName*>(nameval);
		th->local_name=q->local_name;
		if(!namespaceval)
		{
			th->uri_is_null=q->uri_is_null;
			th->uri=q->uri;
			return NULL;
		}
	}
	else if(nameval->getObjectType()==T_UNDEFINED)
		th->local_name="";
	else
		th->local_name=nameval->toString();

	// Set uri
	th->uri_is_null=false;
	if(!namespaceval || namespaceval->getObjectType()==T_UNDEFINED)
	{
		if(th->local_name=="*")
		{
			th->uri_is_null=true;
			th->uri="";
		}
		else
		{
			// Should set th->uri to the default namespace
			LOG(LOG_NOT_IMPLEMENTED, "QName constructor not completely implemented");
			th->uri="";
		}
	}
	else if(namespaceval->getObjectType()==T_NULL)
	{
		th->uri_is_null=true;
		th->uri="";
	}
	else
	{
		th->uri=namespaceval->toString();
	}

	return NULL;
}

ASFUNCTIONBODY(ASQName,_getURI)
{
	ASQName* th=static_cast<ASQName*>(obj);
	if(th->uri_is_null)
		return new Null;
	else
		return Class<ASString>::getInstanceS(th->uri);
}

ASFUNCTIONBODY(ASQName,_getLocalName)
{
	ASQName* th=static_cast<ASQName*>(obj);
	return Class<ASString>::getInstanceS(th->local_name);
}

ASFUNCTIONBODY(ASQName,_toString)
{
	if(!obj->is<ASQName>())
		throw Class<TypeError>::getInstanceS("QName.toString is not generic");
	ASQName* th=static_cast<ASQName*>(obj);
	return Class<ASString>::getInstanceS(th->toString());
}

bool ASQName::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_QNAME)
	{
		ASQName *q=static_cast<ASQName *>(o);
		return uri_is_null==q->uri_is_null && uri==q->uri && local_name==q->local_name;
	}

	return false;
}

tiny_string ASQName::toString()
{
	tiny_string s;
	if(uri_is_null)
		s = "*::";
	else if(uri!="")
		s = uri + "::";

	return s + local_name;
}

void Namespace::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(_setURI),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("uri","",Class<IFunction>::getFunction(_getURI),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("prefix","",Class<IFunction>::getFunction(_setPrefix),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("prefix","",Class<IFunction>::getFunction(_getPrefix),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(_valueOf),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(_ECMA_valueOf),DYNAMIC_TRAIT);
}

void Namespace::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Namespace,_constructor)
{
	ASObject *urival;
	ASObject *prefixval;
	Namespace* th=static_cast<Namespace*>(obj);
	assert_and_throw(argslen<3);

	if (argslen == 0)
	{
		//Return before resetting the value to preserve those eventually set by the C++ constructor
		return NULL;
	}
	else if (argslen == 1)
	{
		urival = args[0];
		prefixval = NULL;
	}
	else
	{
		prefixval = args[0];
		urival = args[1];
	}
	th->prefix_is_undefined=false;
	th->prefix = "";
	th->uri = "";

	if(!prefixval)
	{
		if(urival->getObjectType()==T_NAMESPACE)
		{
			Namespace* n=static_cast<Namespace*>(urival);
			th->uri=n->uri;
			th->prefix=n->prefix;
		}
		else if(urival->getObjectType()==T_QNAME && 
		   !(static_cast<ASQName*>(urival)->uri_is_null))
		{
			ASQName* q=static_cast<ASQName*>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=urival->toString();
			if(th->uri!="")
			{
				th->prefix_is_undefined=true;
				th->prefix="";
			}
		}
	}
	else // has both urival and prefixval
	{
		if(urival->getObjectType()==T_QNAME &&
		   !(static_cast<ASQName*>(urival)->uri_is_null))
		{
			ASQName* q=static_cast<ASQName*>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=urival->toString();
		}

		if(th->uri=="")
		{
			if(prefixval->getObjectType()==T_UNDEFINED ||
			   prefixval->toString()=="")
				th->prefix="";
			else
				throw Class<TypeError>::getInstanceS("Namespace prefix for empty uri not allowed");
		}
		else if(prefixval->getObjectType()==T_UNDEFINED ||
			!isXMLName(prefixval))
		{
			th->prefix_is_undefined=true;
			th->prefix="";
		}
		else
		{
			th->prefix=prefixval->toString();
		}
	}

	return NULL;
}

ASFUNCTIONBODY(Namespace,_setURI)
{
	Namespace* th=static_cast<Namespace*>(obj);
	th->uri=args[0]->toString();
	return NULL;
}

ASFUNCTIONBODY(Namespace,_getURI)
{
	Namespace* th=static_cast<Namespace*>(obj);
	return Class<ASString>::getInstanceS(th->uri);
}

ASFUNCTIONBODY(Namespace,_setPrefix)
{
	Namespace* th=static_cast<Namespace*>(obj);
	if(args[0]->getObjectType()==T_UNDEFINED)
	{
		th->prefix_is_undefined=true;
		th->prefix="";
	}
	else
	{
		th->prefix_is_undefined=false;
		th->prefix=args[0]->toString();
	}
	return NULL;
}

ASFUNCTIONBODY(Namespace,_getPrefix)
{
	Namespace* th=static_cast<Namespace*>(obj);
	if(th->prefix_is_undefined)
		return new Undefined;
	else
		return Class<ASString>::getInstanceS(th->prefix);
}

ASFUNCTIONBODY(Namespace,_toString)
{
	if(!obj->is<Namespace>())
		throw Class<TypeError>::getInstanceS("Namespace.toString is not generic");
	Namespace* th=static_cast<Namespace*>(obj);
	return Class<ASString>::getInstanceS(th->uri);
}

ASFUNCTIONBODY(Namespace,_valueOf)
{
	return Class<ASString>::getInstanceS(obj->as<Namespace>()->uri);
}

ASFUNCTIONBODY(Namespace,_ECMA_valueOf)
{
	if(!obj->is<Namespace>())
		throw Class<TypeError>::getInstanceS("Namespace.valueOf is not generic");
	Namespace* th=static_cast<Namespace*>(obj);
	return Class<ASString>::getInstanceS(th->uri);
}

bool Namespace::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_NAMESPACE)
	{
		Namespace *n=static_cast<Namespace *>(o);
		return uri==n->uri;
	}

	return false;
}

void InterfaceClass::lookupAndLink(Class_base* c, const tiny_string& name, const tiny_string& interfaceNs)
{
	variable* var=NULL;
	Class_base* cur=c;
	//Find the origin
	while(cur)
	{
		var=cur->Variables.findObjVar(name,nsNameAndKind("",NAMESPACE),NO_CREATE_TRAIT,BORROWED_TRAIT);
		if(var)
			break;
		cur=cur->super.getPtr();
	}
	assert_and_throw(var->var && var->var->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(var->var);
	f->incRef();
	c->setDeclaredMethodByQName(name,interfaceNs,f,NORMAL_METHOD,true);
}

void UInteger::sinit(Class_base* c)
{
	//TODO: add in the JIT support for unsigned number
	//Right now we pretend to be signed, to make comparisons work
	c->setSuper(Class<ASObject>::getRef());
	c->setVariableByQName("MAX_VALUE","",new UInteger(0x7fffffff),DECLARED_TRAIT);
	c->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY(UInteger,_toString)
{
	UInteger* th=static_cast<UInteger*>(obj);
	uint32_t radix;
	ARG_UNPACK (radix,10);

	char buf[20];
	assert_and_throw(radix==10 || radix==16);
	if(radix==10)
		snprintf(buf,20,"%u",th->val);
	else if(radix==16)
		snprintf(buf,20,"%x",th->val);

	return Class<ASString>::getInstanceS(buf);
}

bool UInteger::isEqual(ASObject* o)
{
	switch(o->getObjectType())
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_STRING:
		case T_BOOLEAN:
			return val==o->toUInt();
		case T_NULL:
		case T_UNDEFINED:
			return false;
		default:
			return o->isEqual(this);
	}
}

ASObject* ASNop(ASObject* obj, ASObject* const* args, const unsigned int argslen)
{
	return new Undefined;
}

ASObject* Class<IFunction>::getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
{
	if(argslen)
		LOG(LOG_NOT_IMPLEMENTED,"new Function() with argslen > 0");
	return Class<IFunction>::getFunction(ASNop);
}

Class<IFunction>* Class<IFunction>::getClass()
{
	std::map<QName, Class_base*>::iterator it=getSys()->classes.find(QName(ClassName<IFunction>::name,ClassName<IFunction>::ns));
	Class<IFunction>* ret=NULL;
	if(it==getSys()->classes.end()) //This class is not yet in the map, create it
	{
		ret=new Class<IFunction>;
		ret->prototype = _MNR(new_asobject());
		//This function is called from Class<ASObject>::getRef(),
		//so the Class<ASObject> we obtain will not have any
		//declared methods yet! Therefore, set super will not copy
		//up any borrowed traits from there. We do that by ourself.
		ret->setSuper(Class<ASObject>::getRef());

		ret->prototype->setprop_prototype(ret->super->prototype);

		getSys()->classes.insert(std::make_pair(QName(ClassName<IFunction>::name,ClassName<IFunction>::ns),ret));

		//we cannot use sinit, as we need to setup 'this_class' before calling
		//addPrototypeGetter and setDeclaredMethodByQName.
		//Thus we make sure that everything is in order when getFunction() below is called
		ret->addPrototypeGetter();
		//copy borrowed traits from ASObject by ourself
		ASObject::sinit(ret);
		ret->setDeclaredMethodByQName("call",AS3,Class<IFunction>::getFunction(IFunction::_call),NORMAL_METHOD,true);
		ret->setDeclaredMethodByQName("apply",AS3,Class<IFunction>::getFunction(IFunction::apply),NORMAL_METHOD,true);
		ret->setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(IFunction::_getter_prototype),GETTER_METHOD,true);
		ret->setDeclaredMethodByQName("prototype","",Class<IFunction>::getFunction(IFunction::_setter_prototype),SETTER_METHOD,true);
		ret->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(IFunction::_getter_length),GETTER_METHOD,true);
		ret->prototype->setVariableByQName("toString",AS3,Class<IFunction>::getFunction(IFunction::_toString),DYNAMIC_TRAIT);
		ret->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(Class_base::_toString),NORMAL_METHOD,false);
	}
	else
		ret=static_cast<Class<IFunction>*>(it->second);

	return ret;
}

void Global::sinit(Class_base* c)
{
	c->setSuper(Class<ASObject>::getRef());
}

_NR<ASObject> Global::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	_NR<ASObject> ret = ASObject::getVariableByMultiname(name, opt);
	/*
	 * All properties are registered by now, even if the script init has
	 * not been run. Thus if ret == NULL, we don't have to run the script init.
	 */
	if(ret.isNull() || !context || context->hasRunScriptInit[scriptId])
		return ret;
	LOG(LOG_CALLS,"Access to " << name << ", running script init");
	context->runScriptInit(scriptId, this);
	return ASObject::getVariableByMultiname(name, opt);
}

void GlobalObject::registerGlobalScope(Global* scope)
{
	globalScopes.push_back(scope);
}

ASObject* GlobalObject::getVariableByString(const std::string& str, ASObject*& target)
{
	size_t index=str.rfind('.');
	multiname name;
	name.name_type=multiname::NAME_STRING;
	if(index==str.npos) //No dot
	{
		name.name_s=str;
		name.ns.push_back(nsNameAndKind("",NAMESPACE)); //TODO: use ns kind
	}
	else
	{
		name.name_s=str.substr(index+1);
		name.ns.push_back(nsNameAndKind(str.substr(0,index),NAMESPACE));
	}
	return getVariableAndTargetByMultiname(name, target);
}

ASObject* GlobalObject::getVariableAndTargetByMultiname(const multiname& name, ASObject*& target)
{
	for(uint32_t i=0;i<globalScopes.size();i++)
	{
		_NR<ASObject> o=globalScopes[i]->getVariableByMultiname(name);
		if(!o.isNull())
		{
			target=globalScopes[i];
			// No incRef, return a reference borrowed from globalScopes
			return o.getPtr();
		}
	}
	
	return NULL;
}

GlobalObject::~GlobalObject()
{
	for(auto i = globalScopes.begin(); i != globalScopes.end(); ++i)
		(*i)->decRef();
}

/*ASObject* GlobalObject::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride, ASObject* base)
{
	ASObject* ret=ASObject::getVariableByMultiname(name, skip_impl, enableOverride, base);
	if(ret==NULL)
	{
		for(uint32_t i=0;i<globalScopes.size();i++)
		{
			ret=globalScopes[i]->getVariableByMultiname(name, skip_impl, enableOverride, base);
			if(ret)
				break;
		}
	}
	return ret;
}*/

ASFUNCTIONBODY(lightspark,parseInt)
{
	assert_and_throw(argslen==1 || argslen==2);
	if(args[0]->getObjectType()==T_UNDEFINED)
	{
		LOG(LOG_ERROR,"Undefined passed to parseInt");
		return new Undefined;
	}
	else
	{
		int radix=0;
		if(argslen==2)
		{
			radix=args[1]->toInt();
			if(radix < 2 || radix > 36)
				return abstract_d(numeric_limits<double>::quiet_NaN());
		}
		const tiny_string& str=args[0]->toString();
		const char* cur=str.raw_buf();

		errno=0;
		char *end;
		int64_t val=g_ascii_strtoll(cur, &end, radix);

		if(errno==ERANGE)
		{
			if(val==LONG_MAX)
				return abstract_d(numeric_limits<double>::infinity());
			if(val==LONG_MIN)
				return abstract_d(-numeric_limits<double>::infinity());
		}

		if(end==cur)
			return abstract_d(numeric_limits<double>::quiet_NaN());

		return abstract_d(val);
	}
}

ASFUNCTIONBODY(lightspark,parseFloat)
{
	if(args[0]->getObjectType()==T_UNDEFINED)
		return new Undefined;
	else
		return abstract_d(atof(args[0]->toString().raw_buf()));
}

ASFUNCTIONBODY(lightspark,isNaN)
{
	if(args[0]->getObjectType()==T_UNDEFINED)
		return abstract_b(true);
	else if(args[0]->getObjectType()==T_INTEGER)
		return abstract_b(false);
	else if(args[0]->getObjectType()==T_NUMBER)
	{
		if(std::isnan(args[0]->toNumber()))
			return abstract_b(true);
		else
			return abstract_b(false);
	}
	else if(args[0]->getObjectType()==T_BOOLEAN)
		return abstract_b(false);
	else if(args[0]->getObjectType()==T_STRING)
	{
		double n=args[0]->toNumber();
		return abstract_b(std::isnan(n));
	}
	else if(args[0]->getObjectType()==T_NULL)
		return abstract_b(false); // because Number(null) == 0
	else
		throw UnsupportedException("Weird argument for isNaN");
}

ASFUNCTIONBODY(lightspark,isFinite)
{
	if(args[0]->getObjectType()==T_NUMBER ||
		args[0]->getObjectType()==T_INTEGER)
	{
		if(isfinite(args[0]->toNumber()))
			return abstract_b(true);
		else
			return abstract_b(false);
	}
	else
		throw UnsupportedException("Weird argument for isFinite");
}

ASFUNCTIONBODY(lightspark,encodeURI)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::encode(th->data, URLInfo::ENCODE_URI));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::encode(args[0]->toString(), URLInfo::ENCODE_URI));
	}
}

ASFUNCTIONBODY(lightspark,decodeURI)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::decode(th->data, URLInfo::ENCODE_URI));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::decode(args[0]->toString(), URLInfo::ENCODE_URI));
	}
}

ASFUNCTIONBODY(lightspark,encodeURIComponent)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::encode(th->data, URLInfo::ENCODE_URICOMPONENT));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::encode(args[0]->toString(), URLInfo::ENCODE_URICOMPONENT));
	}
}

ASFUNCTIONBODY(lightspark,decodeURIComponent)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::decode(th->data, URLInfo::ENCODE_URICOMPONENT));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::decode(args[0]->toString(), URLInfo::ENCODE_URICOMPONENT));
	}
}

ASFUNCTIONBODY(lightspark,escape)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::encode(th->data, URLInfo::ENCODE_ESCAPE));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::encode(args[0]->toString(), URLInfo::ENCODE_ESCAPE));
	}
}

ASFUNCTIONBODY(lightspark,unescape)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* th=static_cast<ASString*>(args[0]);
		return Class<ASString>::getInstanceS(URLInfo::decode(th->data, URLInfo::ENCODE_ESCAPE));
	}
	else
	{
		return Class<ASString>::getInstanceS(URLInfo::encode(args[0]->toString(), URLInfo::ENCODE_ESCAPE));
	}
}

ASFUNCTIONBODY(lightspark,print)
{
	if(args[0]->getObjectType() == T_STRING)
	{
		ASString* str = static_cast<ASString*>(args[0]);
		Log::print(str->data);
	}
	else
		Log::print(args[0]->toString());
	return NULL;
}

ASFUNCTIONBODY(lightspark,trace)
{
	stringstream s;
	for(uint32_t i = 0; i< argslen;i++)
	{
		if(i > 0)
			s << " ";

		if(args[i]->getObjectType() == T_STRING)
		{
			ASString* str = static_cast<ASString*>(args[i]);
			s << str->data;
		}
		else
			s << args[i]->toString();
	}
	Log::print(s.str());
	return NULL;
}

bool lightspark::isXMLName(ASObject *obj)
{
	tiny_string name;

	if(obj->getObjectType()==lightspark::T_QNAME)
	{
		ASQName *q=static_cast<ASQName*>(obj);
		name=q->getLocalName();
	}
	else if(obj->getObjectType()==lightspark::T_UNDEFINED ||
		obj->getObjectType()==lightspark::T_NULL)
		name="";
	else
		name=obj->toString();

	if(name.empty())
		return false;

	// http://www.w3.org/TR/2006/REC-xml-names-20060816/#NT-NCName
	// Note: Flash follows the second edition (20060816) of the
	// standard. The character range definitions were changed in
	// the newer edition.
	#define NC_START_CHAR(x) \
	  ((0x0041 <= x && x <= 0x005A) || x == 0x5F || \
	  (0x0061 <= x && x <= 0x007A) || (0x00C0 <= x && x <= 0x00D6) || \
	  (0x00D8 <= x && x <= 0x00F6) || (0x00F8 <= x && x <= 0x00FF) || \
	  (0x0100 <= x && x <= 0x0131) || (0x0134 <= x && x <= 0x013E) || \
	  (0x0141 <= x && x <= 0x0148) || (0x014A <= x && x <= 0x017E) || \
	  (0x0180 <= x && x <= 0x01C3) || (0x01CD <= x && x <= 0x01F0) || \
	  (0x01F4 <= x && x <= 0x01F5) || (0x01FA <= x && x <= 0x0217) || \
	  (0x0250 <= x && x <= 0x02A8) || (0x02BB <= x && x <= 0x02C1) || \
	  x == 0x0386 || (0x0388 <= x && x <= 0x038A) || x == 0x038C || \
	  (0x038E <= x && x <= 0x03A1) || (0x03A3 <= x && x <= 0x03CE) || \
	  (0x03D0 <= x && x <= 0x03D6) || x == 0x03DA || x == 0x03DC || \
	  x == 0x03DE || x == 0x03E0 || (0x03E2 <= x && x <= 0x03F3) || \
	  (0x0401 <= x && x <= 0x040C) || (0x040E <= x && x <= 0x044F) || \
	  (0x0451 <= x && x <= 0x045C) || (0x045E <= x && x <= 0x0481) || \
	  (0x0490 <= x && x <= 0x04C4) || (0x04C7 <= x && x <= 0x04C8) || \
	  (0x04CB <= x && x <= 0x04CC) || (0x04D0 <= x && x <= 0x04EB) || \
	  (0x04EE <= x && x <= 0x04F5) || (0x04F8 <= x && x <= 0x04F9) || \
	  (0x0531 <= x && x <= 0x0556) || x == 0x0559 || \
	  (0x0561 <= x && x <= 0x0586) || (0x05D0 <= x && x <= 0x05EA) || \
	  (0x05F0 <= x && x <= 0x05F2) || (0x0621 <= x && x <= 0x063A) || \
	  (0x0641 <= x && x <= 0x064A) || (0x0671 <= x && x <= 0x06B7) || \
	  (0x06BA <= x && x <= 0x06BE) || (0x06C0 <= x && x <= 0x06CE) || \
	  (0x06D0 <= x && x <= 0x06D3) || x == 0x06D5 || \
	  (0x06E5 <= x && x <= 0x06E6) || (0x0905 <= x && x <= 0x0939) || \
	  x == 0x093D || (0x0958 <= x && x <= 0x0961) || \
	  (0x0985 <= x && x <= 0x098C) || (0x098F <= x && x <= 0x0990) || \
	  (0x0993 <= x && x <= 0x09A8) || (0x09AA <= x && x <= 0x09B0) || \
	  x == 0x09B2 || (0x09B6 <= x && x <= 0x09B9) || \
	  (0x09DC <= x && x <= 0x09DD) || (0x09DF <= x && x <= 0x09E1) || \
	  (0x09F0 <= x && x <= 0x09F1) || (0x0A05 <= x && x <= 0x0A0A) || \
	  (0x0A0F <= x && x <= 0x0A10) || (0x0A13 <= x && x <= 0x0A28) || \
	  (0x0A2A <= x && x <= 0x0A30) || (0x0A32 <= x && x <= 0x0A33) || \
	  (0x0A35 <= x && x <= 0x0A36) || (0x0A38 <= x && x <= 0x0A39) || \
	  (0x0A59 <= x && x <= 0x0A5C) || x == 0x0A5E || \
	  (0x0A72 <= x && x <= 0x0A74) || (0x0A85 <= x && x <= 0x0A8B) || \
	  x == 0x0A8D || (0x0A8F <= x && x <= 0x0A91) || \
	  (0x0A93 <= x && x <= 0x0AA8) || (0x0AAA <= x && x <= 0x0AB0) || \
	  (0x0AB2 <= x && x <= 0x0AB3) || (0x0AB5 <= x && x <= 0x0AB9) || \
	  x == 0x0ABD || x == 0x0AE0 || (0x0B05 <= x && x <= 0x0B0C) || \
	  (0x0B0F <= x && x <= 0x0B10) || (0x0B13 <= x && x <= 0x0B28) || \
	  (0x0B2A <= x && x <= 0x0B30) || (0x0B32 <= x && x <= 0x0B33) || \
	  (0x0B36 <= x && x <= 0x0B39) || x == 0x0B3D || \
	  (0x0B5C <= x && x <= 0x0B5D) || (0x0B5F <= x && x <= 0x0B61) || \
	  (0x0B85 <= x && x <= 0x0B8A) || (0x0B8E <= x && x <= 0x0B90) || \
	  (0x0B92 <= x && x <= 0x0B95) || (0x0B99 <= x && x <= 0x0B9A) || \
	  x == 0x0B9C || (0x0B9E <= x && x <= 0x0B9F) || \
	  (0x0BA3 <= x && x <= 0x0BA4) || (0x0BA8 <= x && x <= 0x0BAA) || \
	  (0x0BAE <= x && x <= 0x0BB5) || (0x0BB7 <= x && x <= 0x0BB9) || \
	  (0x0C05 <= x && x <= 0x0C0C) || (0x0C0E <= x && x <= 0x0C10) || \
	  (0x0C12 <= x && x <= 0x0C28) || (0x0C2A <= x && x <= 0x0C33) || \
	  (0x0C35 <= x && x <= 0x0C39) || (0x0C60 <= x && x <= 0x0C61) || \
	  (0x0C85 <= x && x <= 0x0C8C) || (0x0C8E <= x && x <= 0x0C90) || \
	  (0x0C92 <= x && x <= 0x0CA8) || (0x0CAA <= x && x <= 0x0CB3) || \
	  (0x0CB5 <= x && x <= 0x0CB9) || x == 0x0CDE || \
	  (0x0CE0 <= x && x <= 0x0CE1) || (0x0D05 <= x && x <= 0x0D0C) || \
	  (0x0D0E <= x && x <= 0x0D10) || (0x0D12 <= x && x <= 0x0D28) || \
	  (0x0D2A <= x && x <= 0x0D39) || (0x0D60 <= x && x <= 0x0D61) || \
	  (0x0E01 <= x && x <= 0x0E2E) || x == 0x0E30 || \
	  (0x0E32 <= x && x <= 0x0E33) || (0x0E40 <= x && x <= 0x0E45) || \
	  (0x0E81 <= x && x <= 0x0E82) || x == 0x0E84 || \
	  (0x0E87 <= x && x <= 0x0E88) || x == 0x0E8A || x == 0x0E8D || \
	  (0x0E94 <= x && x <= 0x0E97) || (0x0E99 <= x && x <= 0x0E9F) || \
	  (0x0EA1 <= x && x <= 0x0EA3) || x == 0x0EA5 || x == 0x0EA7 || \
	  (0x0EAA <= x && x <= 0x0EAB) || (0x0EAD <= x && x <= 0x0EAE) || \
	  x == 0x0EB0 || (0x0EB2 <= x && x <= 0x0EB3) || x == 0x0EBD || \
	  (0x0EC0 <= x && x <= 0x0EC4) || (0x0F40 <= x && x <= 0x0F47) || \
	  (0x0F49 <= x && x <= 0x0F69) || (0x10A0 <= x && x <= 0x10C5) || \
	  (0x10D0 <= x && x <= 0x10F6) || x == 0x1100 || \
	  (0x1102 <= x && x <= 0x1103) || (0x1105 <= x && x <= 0x1107) || \
	  x == 0x1109 || (0x110B <= x && x <= 0x110C) || \
	  (0x110E <= x && x <= 0x1112) || x == 0x113C || x == 0x113E || \
	  x == 0x1140 || x == 0x114C || x == 0x114E || x == 0x1150 || \
	  (0x1154 <= x && x <= 0x1155) || x == 0x1159 || \
	  (0x115F <= x && x <= 0x1161) || x == 0x1163 || x == 0x1165 || \
	  x == 0x1167 || x == 0x1169 || (0x116D <= x && x <= 0x116E) || \
	  (0x1172 <= x && x <= 0x1173) || x == 0x1175 || x == 0x119E || \
	  x == 0x11A8 || x == 0x11AB || (0x11AE <= x && x <= 0x11AF) || \
	  (0x11B7 <= x && x <= 0x11B8) || x == 0x11BA || \
	  (0x11BC <= x && x <= 0x11C2) || x == 0x11EB || x == 0x11F0 || \
	  x == 0x11F9 || (0x1E00 <= x && x <= 0x1E9B) || \
	  (0x1EA0 <= x && x <= 0x1EF9) || (0x1F00 <= x && x <= 0x1F15) || \
	  (0x1F18 <= x && x <= 0x1F1D) || (0x1F20 <= x && x <= 0x1F45) || \
	  (0x1F48 <= x && x <= 0x1F4D) || (0x1F50 <= x && x <= 0x1F57) || \
	  x == 0x1F59 || x == 0x1F5B || x == 0x1F5D || \
	  (0x1F5F <= x && x <= 0x1F7D) || (0x1F80 <= x && x <= 0x1FB4) || \
	  (0x1FB6 <= x && x <= 0x1FBC) || x == 0x1FBE || \
	  (0x1FC2 <= x && x <= 0x1FC4) || (0x1FC6 <= x && x <= 0x1FCC) || \
	  (0x1FD0 <= x && x <= 0x1FD3) || (0x1FD6 <= x && x <= 0x1FDB) || \
	  (0x1FE0 <= x && x <= 0x1FEC) || (0x1FF2 <= x && x <= 0x1FF4) || \
	  (0x1FF6 <= x && x <= 0x1FFC) || x == 0x2126 || \
	  (0x212A <= x && x <= 0x212B) || x == 0x212E || \
	  (0x2180 <= x && x <= 0x2182) || (0x3041 <= x && x <= 0x3094) || \
	  (0x30A1 <= x && x <= 0x30FA) || (0x3105 <= x && x <= 0x312C) || \
	  (0xAC00 <= x && x <= 0xD7A3) || (0x4E00 <= x && x <= 0x9FA5) || \
	  x == 0x3007 || (0x3021 <= x && x <= 0x3029))
	#define NC_CHAR(x) \
	  (NC_START_CHAR(x) || x == 0x2E || x == 0x2D || x == 0x5F || \
	  (0x0030 <= x && x <= 0x0039) || (0x0660 <= x && x <= 0x0669) || \
	  (0x06F0 <= x && x <= 0x06F9) || (0x0966 <= x && x <= 0x096F) || \
	  (0x09E6 <= x && x <= 0x09EF) || (0x0A66 <= x && x <= 0x0A6F) || \
	  (0x0AE6 <= x && x <= 0x0AEF) || (0x0B66 <= x && x <= 0x0B6F) || \
	  (0x0BE7 <= x && x <= 0x0BEF) || (0x0C66 <= x && x <= 0x0C6F) || \
	  (0x0CE6 <= x && x <= 0x0CEF) || (0x0D66 <= x && x <= 0x0D6F) || \
	  (0x0E50 <= x && x <= 0x0E59) || (0x0ED0 <= x && x <= 0x0ED9) || \
	  (0x0F20 <= x && x <= 0x0F29) || (0x0300 <= x && x <= 0x0345) || \
	  (0x0360 <= x && x <= 0x0361) || (0x0483 <= x && x <= 0x0486) || \
	  (0x0591 <= x && x <= 0x05A1) || (0x05A3 <= x && x <= 0x05B9) || \
	  (0x05BB <= x && x <= 0x05BD) || x == 0x05BF || \
	  (0x05C1 <= x && x <= 0x05C2) || x == 0x05C4 || \
	  (0x064B <= x && x <= 0x0652) || x == 0x0670 || \
	  (0x06D6 <= x && x <= 0x06DC) || (0x06DD <= x && x <= 0x06DF) || \
	  (0x06E0 <= x && x <= 0x06E4) || (0x06E7 <= x && x <= 0x06E8) || \
	  (0x06EA <= x && x <= 0x06ED) || (0x0901 <= x && x <= 0x0903) || \
	  x == 0x093C || (0x093E <= x && x <= 0x094C) || x == 0x094D || \
	  (0x0951 <= x && x <= 0x0954) || (0x0962 <= x && x <= 0x0963) || \
	  (0x0981 <= x && x <= 0x0983) || x == 0x09BC || x == 0x09BE || \
	  x == 0x09BF || (0x09C0 <= x && x <= 0x09C4) || \
	  (0x09C7 <= x && x <= 0x09C8) || (0x09CB <= x && x <= 0x09CD) || \
	  x == 0x09D7 || (0x09E2 <= x && x <= 0x09E3) || x == 0x0A02 || \
	  x == 0x0A3C || x == 0x0A3E || x == 0x0A3F || \
	  (0x0A40 <= x && x <= 0x0A42) || (0x0A47 <= x && x <= 0x0A48) || \
	  (0x0A4B <= x && x <= 0x0A4D) || (0x0A70 <= x && x <= 0x0A71) || \
	  (0x0A81 <= x && x <= 0x0A83) || x == 0x0ABC || \
	  (0x0ABE <= x && x <= 0x0AC5) || (0x0AC7 <= x && x <= 0x0AC9) || \
	  (0x0ACB <= x && x <= 0x0ACD) || (0x0B01 <= x && x <= 0x0B03) || \
	  x == 0x0B3C || (0x0B3E <= x && x <= 0x0B43) || \
	  (0x0B47 <= x && x <= 0x0B48) || (0x0B4B <= x && x <= 0x0B4D) || \
	  (0x0B56 <= x && x <= 0x0B57) || (0x0B82 <= x && x <= 0x0B83) || \
	  (0x0BBE <= x && x <= 0x0BC2) || (0x0BC6 <= x && x <= 0x0BC8) || \
	  (0x0BCA <= x && x <= 0x0BCD) || x == 0x0BD7 || \
	  (0x0C01 <= x && x <= 0x0C03) || (0x0C3E <= x && x <= 0x0C44) || \
	  (0x0C46 <= x && x <= 0x0C48) || (0x0C4A <= x && x <= 0x0C4D) || \
	  (0x0C55 <= x && x <= 0x0C56) || (0x0C82 <= x && x <= 0x0C83) || \
	  (0x0CBE <= x && x <= 0x0CC4) || (0x0CC6 <= x && x <= 0x0CC8) || \
	  (0x0CCA <= x && x <= 0x0CCD) || (0x0CD5 <= x && x <= 0x0CD6) || \
	  (0x0D02 <= x && x <= 0x0D03) || (0x0D3E <= x && x <= 0x0D43) || \
	  (0x0D46 <= x && x <= 0x0D48) || (0x0D4A <= x && x <= 0x0D4D) || \
	  x == 0x0D57 || x == 0x0E31 || (0x0E34 <= x && x <= 0x0E3A) || \
	  (0x0E47 <= x && x <= 0x0E4E) || x == 0x0EB1 || \
	  (0x0EB4 <= x && x <= 0x0EB9) || (0x0EBB <= x && x <= 0x0EBC) || \
	  (0x0EC8 <= x && x <= 0x0ECD) || (0x0F18 <= x && x <= 0x0F19) || \
	  x == 0x0F35 || x == 0x0F37 || x == 0x0F39 || x == 0x0F3E || \
	  x == 0x0F3F || (0x0F71 <= x && x <= 0x0F84) || \
	  (0x0F86 <= x && x <= 0x0F8B) || (0x0F90 <= x && x <= 0x0F95) || \
	  x == 0x0F97 || (0x0F99 <= x && x <= 0x0FAD) || \
	  (0x0FB1 <= x && x <= 0x0FB7) || x == 0x0FB9 || \
	  (0x20D0 <= x && x <= 0x20DC) || x == 0x20E1 || \
	  (0x302A <= x && x <= 0x302F) || x == 0x3099 || x == 0x309A || \
	  x == 0x00B7 || x == 0x02D0 || x == 0x02D1 || x == 0x0387 || \
	  x == 0x0640 || x == 0x0E46 || x == 0x0EC6 || x == 0x3005 || \
	  (0x3031 <= x && x <= 0x3035) || (0x309D <= x && x <= 0x309E) || \
	  (0x30FC <= x && x <= 0x30FE))

	auto it=name.begin();
	if(!NC_START_CHAR(*it))
		return false;
	++it;

	for(;it!=name.end(); ++it)
	{
		if(!(NC_CHAR(*it)))
			return false;
	}

	#undef NC_CHAR
	#undef NC_START_CHAR

	return true;
}

ASFUNCTIONBODY(lightspark,_isXMLName)
{
	assert_and_throw(argslen <= 1);
	if(argslen==0)
		return abstract_b(false);

	return abstract_b(isXMLName(args[0]));
}
