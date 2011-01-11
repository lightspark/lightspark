/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "flashxml.h"
#include "swf.h"
#include "compat.h"
#include "class.h"
#include <libxml/tree.h>

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.xml");

REGISTER_CLASS_NAME(XMLDocument);
REGISTER_CLASS_NAME(XMLNode);

XMLNode::XMLNode(XMLDocument* _r, xmlpp::Node* _n):root(_r),node(_n)
{
}

XMLNode::~XMLNode()
{
	if(root!=this)
		root->decRef();
}

void XMLNode::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<ASObject>::getClass();
	c->max_level=c->super->max_level+1;
	c->setGetterByQName("firstChild","",Class<IFunction>::getFunction(XMLNode::firstChild),true);
	c->setGetterByQName("attributes","",Class<IFunction>::getFunction(attributes),true);
}

void XMLNode::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(XMLNode,_constructor)
{
//	XMLNode* th=Class<XMLNode>::cast(obj);
	assert_and_throw(argslen==0);
	return NULL;
}

ASFUNCTIONBODY(XMLNode,firstChild)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	assert_and_throw(argslen==0);
	if(th->node==NULL) //We assume NULL node is like empty node
		return new Null;
	assert_and_throw(th->node->cobj()->type!=XML_TEXT_NODE);
	const xmlpp::Node::NodeList& children=th->node->get_children();
	if(children.empty())
		return new Null;
	xmlpp::Node* newNode=children.front();
	th->root->incRef();
	return Class<XMLNode>::getInstanceS(th->root,newNode);
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
	for(;it!=list.end();it++)
	{
		tiny_string attrName((*it)->get_name().c_str(),true);
		const tiny_string nsName((*it)->get_namespace_prefix().c_str(),true);
		if(nsName!="")
			attrName=nsName+":"+attrName;
		ASString* attrValue=Class<ASString>::getInstanceS((*it)->get_value().c_str());
		ret->setVariableByQName(attrName,"",attrValue);
	}
	return ret;
}

void XMLDocument::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->super=Class<XMLNode>::getClass();
	c->max_level=c->super->max_level+1;
	c->setMethodByQName("parseXML","",Class<IFunction>::getFunction(parseXML),true);
	c->setGetterByQName("firstChild","",Class<IFunction>::getFunction(XMLDocument::firstChild),true);
}

void XMLDocument::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(XMLDocument,_constructor)
{
//	XMLDocument* th=Class<XMLDocument>::cast(obj);
	assert_and_throw(argslen==0);
	return NULL;
}

void XMLDocument::clear()
{
	if(ownsDocument)
	{
		delete document;
		ownsDocument=false;
	}
}

ASFUNCTIONBODY(XMLDocument,parseXML)
{
	XMLDocument* th=Class<XMLDocument>::cast(obj);
	assert_and_throw(argslen==1 && args[0]->getObjectType()==T_STRING);
	th->clear();
	ASString* str=Class<ASString>::cast(args[0]);
	th->parser.parse_memory_raw((const unsigned char*)str->data.c_str(), str->data.size());
	th->document=th->parser.get_document();
	th->root=th;
	return NULL;
}

ASFUNCTIONBODY(XMLDocument,firstChild)
{
	XMLDocument* th=Class<XMLDocument>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node==NULL);
	xmlpp::Node* newNode=th->document->get_root_node();
	th->root->incRef();
	return Class<XMLNode>::getInstanceS(th->root,newNode);
}
