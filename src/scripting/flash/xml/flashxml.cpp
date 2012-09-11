/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <libxml/tree.h>
#include <libxml++/nodes/textnode.h>

#include "scripting/flash/xml/flashxml.h"
#include "swf.h"
#include "compat.h"
#include "scripting/argconv.h"
#include "parsing/amf3_generator.h"

using namespace std;
using namespace lightspark;

XMLNode::XMLNode(Class_base* c, _R<XMLDocument> _r, xmlpp::Node* _n):ASObject(c),root(_r),node(_n)
{
}

void XMLNode::finalize()
{
	ASObject::finalize();
	root.reset();
}

void XMLNode::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("firstChild","",Class<IFunction>::getFunction(XMLNode::firstChild),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("childNodes","",Class<IFunction>::getFunction(XMLNode::childNodes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("attributes","",Class<IFunction>::getFunction(attributes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeType","",Class<IFunction>::getFunction(_getNodeType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeName","",Class<IFunction>::getFunction(_getNodeName),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeValue","",Class<IFunction>::getFunction(_getNodeValue),GETTER_METHOD,true);
}

void XMLNode::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(XMLNode,_constructor)
{
	if(argslen==0)
		return NULL;
	XMLNode* th=Class<XMLNode>::cast(obj);
	uint32_t type;
	tiny_string value;
	ARG_UNPACK(type)(value);
	assert_and_throw(type==1);
	th->root=_MR(Class<XMLDocument>::getInstanceS());
	if(type==1)
	{
		th->root->parseXMLImpl(value);
		th->node=th->root->rootNode;
	}
	return NULL;
}

ASFUNCTIONBODY(XMLNode,firstChild)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	assert_and_throw(argslen==0);
	if(th->node==NULL) //We assume NULL node is like empty node
		return getSys()->getNullRef();
	assert_and_throw(th->node->cobj()->type!=XML_TEXT_NODE);
	const xmlpp::Node::NodeList& children=th->node->get_children();
	if(children.empty())
		return getSys()->getNullRef();
	xmlpp::Node* newNode=children.front();
	assert_and_throw(!th->root.isNull());
	return Class<XMLNode>::getInstanceS(th->root,newNode);
}

ASFUNCTIONBODY(XMLNode,childNodes)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	Array* ret = Class<Array>::getInstanceS();
	assert_and_throw(argslen==0);
	if(th->node==NULL) //We assume NULL node is like empty node
		return ret;
	assert_and_throw(!th->root.isNull());
	const xmlpp::Node::NodeList& children=th->node->get_children();
	xmlpp::Node::NodeList::const_iterator it = children.begin();
	for(;it!=children.end();++it)
	{
		if((*it)->cobj()->type!=XML_TEXT_NODE) {
			ret->push(_MR(Class<XMLNode>::getInstanceS(th->root, *it)));
		}
	}
	return ret;
}


ASFUNCTIONBODY(XMLNode,attributes)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	assert_and_throw(argslen==0);
	ASObject* ret=Class<ASObject>::getInstanceS();
	if(th->node==NULL) //We assume NULL node is like empty node
		return ret;
	//Needed dynamic cast, we want the type check
	xmlpp::Element* elem=dynamic_cast<xmlpp::Element*>(th->node);
	if(elem==NULL)
		return ret;
	const xmlpp::Element::AttributeList& list=elem->get_attributes();
	xmlpp::Element::AttributeList::const_iterator it=list.begin();
	for(;it!=list.end();++it)
	{
		tiny_string attrName((*it)->get_name().c_str(),true);
		const tiny_string nsName((*it)->get_namespace_prefix().c_str(),true);
		if(nsName!="")
			attrName=nsName+":"+attrName;
		ASString* attrValue=Class<ASString>::getInstanceS((*it)->get_value().c_str());
		ret->setVariableByQName(attrName,"",attrValue,DYNAMIC_TRAIT);
	}
	return ret;
}

ASFUNCTIONBODY(XMLNode,_getNodeType)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	return abstract_i(th->node->cobj()->type);
}

ASFUNCTIONBODY(XMLNode,_getNodeName)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	return Class<ASString>::getInstanceS((const char*)th->node->cobj()->name);
}

ASFUNCTIONBODY(XMLNode,_getNodeValue)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	xmlpp::TextNode* textnode=dynamic_cast<xmlpp::TextNode*>(th->node);
	if(textnode)
		return Class<ASString>::getInstanceS(textnode->get_content());
	else
		return getSys()->getNullRef();
}

ASFUNCTIONBODY(XMLNode,_toString)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	return Class<ASString>::getInstanceS(th->toString_priv(th->node));
}

tiny_string XMLNode::toString()
{
	return toString_priv(node);
}

tiny_string XMLNode::toString_priv(xmlpp::Node *outputNode)
{
	if(outputNode==NULL)
		return "";

	xmlNodePtr cNode=outputNode->cobj();
	xmlDocPtr xmlDoc=cNode->doc;
	xmlBufferPtr xmlBuffer=xmlBufferCreateSize(4096);
	int success=xmlNodeDump(xmlBuffer, xmlDoc, cNode, 0, 0);
	if (!success)
		throw RunTimeException("Error in XMLNode::toString_priv");

	tiny_string ret=tiny_string((char*)xmlBuffer->content,true);
	xmlBufferFree(xmlBuffer);

	return ret;
}

XMLDocument::XMLDocument(Class_base* c, tiny_string s)
 : XMLNode(c),rootNode(NULL)
{
	if(!s.empty())
	{
		parseXMLImpl(s);
	}
}

void XMLDocument::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<XMLNode>::getRef());
	c->setDeclaredMethodByQName("parseXML","",Class<IFunction>::getFunction(parseXML),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("firstChild","",Class<IFunction>::getFunction(XMLDocument::firstChild),GETTER_METHOD,true);
}

void XMLDocument::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(XMLDocument,_constructor)
{
	XMLDocument* th=Class<XMLDocument>::cast(obj);
	tiny_string source;

	ARG_UNPACK(source, "");
	if(!source.empty())
		th->parseXMLImpl(source);

	return NULL;
}

void XMLDocument::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	out->writeByte(xml_doc_marker);
	out->writeXMLString(objMap, this, toString());
}

void XMLDocument::parseXMLImpl(const string& str)
{
	rootNode=buildFromString(str);
}

ASFUNCTIONBODY(XMLDocument,_toString)
{
	//TODO: should output xmlDecl and docTypeDecl, see the
	//documentation on XMLNode.tostring()
	XMLDocument* th=Class<XMLDocument>::cast(obj);
	return Class<ASString>::getInstanceS(th->toString_priv(th->rootNode));
}

tiny_string XMLDocument::toString()
{
	return toString_priv(rootNode);
}

ASFUNCTIONBODY(XMLDocument,parseXML)
{
	XMLDocument* th=Class<XMLDocument>::cast(obj);
	assert_and_throw(argslen==1 && args[0]->getObjectType()==T_STRING);
	ASString* str=Class<ASString>::cast(args[0]);
	th->parseXMLImpl(str->data);
	return NULL;
}

ASFUNCTIONBODY(XMLDocument,firstChild)
{
	XMLDocument* th=Class<XMLDocument>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node==NULL);
	xmlpp::Node* newNode=th->rootNode;
	th->incRef();
	return Class<XMLNode>::getInstanceS(_MR(th),newNode);
}
