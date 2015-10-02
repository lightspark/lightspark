/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/flash/xml/flashxml.h"
#include "scripting/flash/utils/ByteArray.h"
#include "swf.h"
#include "compat.h"
#include "scripting/argconv.h"
#include "parsing/amf3_generator.h"

using namespace std;
using namespace lightspark;

XMLNode::XMLNode(Class_base* c, _R<XMLDocument> _r, pugi::xml_node _n):ASObject(c),root(_r),node(_n)
{
}

void XMLNode::finalize()
{
	ASObject::finalize();
	root.reset();
}

void XMLNode::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes","",Class<IFunction>::getFunction(attributes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("childNodes","",Class<IFunction>::getFunction(XMLNode::childNodes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("firstChild","",Class<IFunction>::getFunction(XMLNode::firstChild),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("lastChild","",Class<IFunction>::getFunction(lastChild),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nextSibling","",Class<IFunction>::getFunction(nextSibling),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeType","",Class<IFunction>::getFunction(_getNodeType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeName","",Class<IFunction>::getFunction(_getNodeName),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeValue","",Class<IFunction>::getFunction(_getNodeValue),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("parentNode","",Class<IFunction>::getFunction(parentNode),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("previousSibling","",Class<IFunction>::getFunction(previousSibling),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localName","",Class<IFunction>::getFunction(_getLocalName),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("appendChild","",Class<IFunction>::getFunction(appendChild),NORMAL_METHOD,true);
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
	if(th->node.type()==pugi::node_null || th->node.type() == pugi::node_pcdata)
		return getSys()->getNullRef();
	pugi::xml_node newNode =th->node.first_child();
	if(newNode.type() == pugi::node_null)
		return getSys()->getNullRef();
	assert_and_throw(!th->root.isNull());
	return Class<XMLNode>::getInstanceS(th->root,newNode);
}

ASFUNCTIONBODY(XMLNode,lastChild)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	assert_and_throw(argslen==0);
	if(th->node.type()==pugi::node_null || th->node.type() == pugi::node_pcdata)
		return getSys()->getNullRef();
	pugi::xml_node newNode =th->node.last_child();
	if(newNode.type() == pugi::node_null)
		return getSys()->getNullRef();
	assert_and_throw(!th->root.isNull());
	return Class<XMLNode>::getInstanceS(th->root,newNode);
}

ASFUNCTIONBODY(XMLNode,childNodes)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	Array* ret = Class<Array>::getInstanceS();
	assert_and_throw(argslen==0);
	if(th->node.type()==pugi::node_null)
		return ret;
	assert_and_throw(!th->root.isNull());
	auto it = th->node.begin();
	for(;it!=th->node.end();++it)
	{
		if(it->type()!=pugi::node_pcdata) {
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
	if(th->node.type()==pugi::node_null)
		return ret;
	auto it=th->node.attributes_begin();
	for(;it!=th->node.attributes_end();++it)
	{
		tiny_string attrName = it->name();
		ASString* attrValue=Class<ASString>::getInstanceS(it->value());
		ret->setVariableByQName(attrName,"",attrValue,DYNAMIC_TRAIT);
	}
	return ret;
}

pugi::xml_node XMLNode::getParentNode()
{
	return node.parent();
}

ASFUNCTIONBODY(XMLNode,parentNode)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	pugi::xml_node parent = th->getParentNode();
	if (parent.type()!=pugi::node_null)
		return Class<XMLNode>::getInstanceS(th->root, parent);
	else
		return getSys()->getNullRef();
}

ASFUNCTIONBODY(XMLNode,nextSibling)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	if(th->node.type()==pugi::node_null)
		return getSys()->getNullRef();

	pugi::xml_node sibling = th->node.next_sibling();
	if (sibling.type()!=pugi::node_null)
		return Class<XMLNode>::getInstanceS(th->root, sibling);
	else
		return getSys()->getNullRef();
}

ASFUNCTIONBODY(XMLNode,previousSibling)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	if(th->node.type()==pugi::node_null)
		return getSys()->getNullRef();

	pugi::xml_node sibling = th->node.previous_sibling();
	if (sibling.type()!=pugi::node_null)
		return Class<XMLNode>::getInstanceS(th->root, sibling);
	else
		return getSys()->getNullRef();
}

ASFUNCTIONBODY(XMLNode,_getNodeType)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	int t = 0;
	switch (th->node.type())
	{
		case pugi::node_element:
			t = 1;
			break;
		case pugi::node_pcdata:
			t = 3;
			break;
		case pugi::node_declaration: 
			t = 5;
			break;
		case pugi::node_pi:
			t = 9;
			break;
		case pugi::node_document:
			t = 11;
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"XMLNode.getNodeType: unhandled type:"<<th->node.type());
			break;
	}
	return abstract_i(t);
}

ASFUNCTIONBODY(XMLNode,_getNodeName)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	return Class<ASString>::getInstanceS(th->node.name());
}

ASFUNCTIONBODY(XMLNode,_getNodeValue)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	if(th->node.type() == pugi::node_pcdata)
		return Class<ASString>::getInstanceS(th->node.value());
	else
		return getSys()->getNullRef();
}

ASFUNCTIONBODY(XMLNode,_toString)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	return Class<ASString>::getInstanceS(th->toString_priv(th->node));
}

ASFUNCTIONBODY(XMLNode,_getLocalName)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	tiny_string localname =th->node.name();
	uint32_t pos = localname.find(".");
	if (pos != tiny_string::npos)
	{
		localname = localname.substr(pos,localname.numChars()-pos);
	}
	return Class<ASString>::getInstanceS(localname);
}
ASFUNCTIONBODY(XMLNode,appendChild)
{
	XMLNode* th=Class<XMLNode>::cast(obj);
	_NR<XMLNode> c;
	ARG_UNPACK(c);
	th->node.append_move(c->node);
	if (!c->root.isNull())
		c->root->decRef();
	c->root = th->root;
	th->root->incRef();
	return NULL;
}
tiny_string XMLNode::toString()
{
	return toString_priv(node);
}

tiny_string XMLNode::toString_priv(pugi::xml_node outputNode)
{
	if(outputNode.type() == pugi::node_null)
		return "";

	ostringstream buf;
	outputNode.print(buf);
	tiny_string ret = tiny_string(buf.str());
	return ret;
}

XMLDocument::XMLDocument(Class_base* c, tiny_string s)
  : XMLNode(c),rootNode(NULL),ignoreWhite(false)
{
	if(!s.empty())
	{
		parseXMLImpl(s);
	}
}

void XMLDocument::sinit(Class_base* c)
{
	CLASS_SETUP(c, XMLNode, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("parseXML","",Class<IFunction>::getFunction(parseXML),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("firstChild","",Class<IFunction>::getFunction(XMLDocument::firstChild),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("createElement","",Class<IFunction>::getFunction(XMLDocument::createElement),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, ignoreWhite);
}

ASFUNCTIONBODY_GETTER_SETTER(XMLDocument, ignoreWhite);

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
	if (out->getObjectEncoding() == ObjectEncoding::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing XMLDocument in AMF0 not implemented");
		return;
	}
	out->writeByte(xml_doc_marker);
	out->writeXMLString(objMap, this, toString());
}

void XMLDocument::parseXMLImpl(const string& str)
{
	unsigned int parsemode = pugi::parse_full |pugi::parse_fragment;
	if (!ignoreWhite) parsemode |= pugi::parse_ws_pcdata;

	rootNode=buildFromString(str, parsemode);
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
	pugi::xml_node newNode=th->rootNode;
	th->incRef();
	return Class<XMLNode>::getInstanceS(_MR(th),newNode);
}
ASFUNCTIONBODY(XMLDocument,createElement)
{
	XMLDocument* th=Class<XMLDocument>::cast(obj);
	assert(th->node==NULL);
	tiny_string name;
	ARG_UNPACK(name);
	pugi::xml_node newNode;
	newNode.set_name(name.raw_buf());
	th->incRef();
	return Class<XMLNode>::getInstanceS(_MR(th),newNode);
}
