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

XMLNode::XMLNode(Class_base* c, _NR<XMLDocument> _r, pugi::xml_node _n):ASObject(c),root(_r),node(_n)
{
}

void XMLNode::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes","",Class<IFunction>::getFunction(c->getSystemState(),attributes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("childNodes","",Class<IFunction>::getFunction(c->getSystemState(),XMLNode::childNodes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("firstChild","",Class<IFunction>::getFunction(c->getSystemState(),XMLNode::firstChild),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("lastChild","",Class<IFunction>::getFunction(c->getSystemState(),lastChild),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nextSibling","",Class<IFunction>::getFunction(c->getSystemState(),nextSibling),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeType","",Class<IFunction>::getFunction(c->getSystemState(),_getNodeType),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeName","",Class<IFunction>::getFunction(c->getSystemState(),_getNodeName),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeName","",Class<IFunction>::getFunction(c->getSystemState(),_setNodeName),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeValue","",Class<IFunction>::getFunction(c->getSystemState(),_getNodeValue),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("parentNode","",Class<IFunction>::getFunction(c->getSystemState(),parentNode),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("previousSibling","",Class<IFunction>::getFunction(c->getSystemState(),previousSibling),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localName","",Class<IFunction>::getFunction(c->getSystemState(),_getLocalName),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("appendChild","",Class<IFunction>::getFunction(c->getSystemState(),appendChild),NORMAL_METHOD,true);
}

void XMLNode::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(XMLNode,_constructor)
{
	if(argslen==0)
		return;
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	uint32_t type;
	tiny_string value;
	ARG_UNPACK_ATOM(type)(value);
	assert_and_throw(type==1);
	th->root=_MR(Class<XMLDocument>::getInstanceS(sys));
	if(type==1)
	{
		th->root->parseXMLImpl(value);
		th->node=th->root->rootNode;
	}
}

ASFUNCTIONBODY_ATOM(XMLNode,firstChild)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	assert_and_throw(argslen==0);
	if(th->node.type()==pugi::node_null || th->node.type() == pugi::node_pcdata)
	{
		asAtomHandler::setNull(ret);
		return;
	}
	pugi::xml_node newNode =th->node.first_child();
	if(newNode.type() == pugi::node_null)
	{
		asAtomHandler::setNull(ret);
		return;
	}
	assert_and_throw(!th->root.isNull());
	ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(sys,th->root,newNode));
}

ASFUNCTIONBODY_ATOM(XMLNode,lastChild)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	assert_and_throw(argslen==0);
	if(th->node.type()==pugi::node_null || th->node.type() == pugi::node_pcdata)
	{
		asAtomHandler::setNull(ret);
		return;
	}
	pugi::xml_node newNode =th->node.last_child();
	if(newNode.type() == pugi::node_null)
	{
		asAtomHandler::setNull(ret);
		return;
	}
	assert_and_throw(!th->root.isNull());
	ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(sys,th->root,newNode));
}

ASFUNCTIONBODY_ATOM(XMLNode,childNodes)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	Array* res = Class<Array>::getInstanceSNoArgs(sys);
	assert_and_throw(argslen==0);
	if(th->node.type()==pugi::node_null)
	{
		ret = asAtomHandler::fromObject(res);
		return;
	}
	auto it = th->node.begin();
	for(;it!=th->node.end();++it)
	{
		if(it->type()!=pugi::node_pcdata && it->type()!=pugi::node_declaration) {
			res->push(asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(sys,th->root, *it)));
		}
	}
	ret = asAtomHandler::fromObject(res);
}


ASFUNCTIONBODY_ATOM(XMLNode,attributes)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	assert_and_throw(argslen==0);
	ASObject* res=Class<ASObject>::getInstanceS(sys);
	if(th->node.type()==pugi::node_null)
	{
		ret = asAtomHandler::fromObject(res);
		return;
	}
	auto it=th->node.attributes_begin();
	for(;it!=th->node.attributes_end();++it)
	{
		tiny_string attrName = it->name();
		ASObject* attrValue=abstract_s(sys,it->value());
		res->setVariableByQName(attrName,"",attrValue,DYNAMIC_TRAIT);
	}
	ret = asAtomHandler::fromObject(res);
}

pugi::xml_node XMLNode::getParentNode()
{
	return node.parent();
}

ASFUNCTIONBODY_ATOM(XMLNode,parentNode)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	pugi::xml_node parent = th->getParentNode();
	if (parent.type()!=pugi::node_null)
		ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(sys,th->root, parent));
	else
		asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(XMLNode,nextSibling)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	if(th->node.type()==pugi::node_null)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	pugi::xml_node sibling = th->node.next_sibling();
	if (sibling.type()!=pugi::node_null)
		ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(sys,th->root, sibling));
	else
		asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(XMLNode,previousSibling)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	if(th->node.type()==pugi::node_null)
	{
		asAtomHandler::setNull(ret);
		return;
	}

	pugi::xml_node sibling = th->node.previous_sibling();
	if (sibling.type()!=pugi::node_null)
		ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(sys,th->root, sibling));
	else
		asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(XMLNode,_getNodeType)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
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
	asAtomHandler::setInt(ret,sys,t);
}

ASFUNCTIONBODY_ATOM(XMLNode,_getNodeName)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->node.name()));
}
ASFUNCTIONBODY_ATOM(XMLNode,_setNodeName)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	tiny_string name;
	ARG_UNPACK_ATOM(name);
	if (name.empty())
		LOG(LOG_NOT_IMPLEMENTED,"XMLNode.setNodeName with empty argument");
	else
		th->node.set_name(name.raw_buf());
}

ASFUNCTIONBODY_ATOM(XMLNode,_getNodeValue)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	if(th->node.type() == pugi::node_pcdata)
		ret = asAtomHandler::fromObject(abstract_s(sys,th->node.value()));
	else
		asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(XMLNode,_toString)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->toString_priv(th->node)));
}

ASFUNCTIONBODY_ATOM(XMLNode,_getLocalName)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	tiny_string localname =th->node.name();
	uint32_t pos = localname.find(".");
	if (pos != tiny_string::npos)
	{
		localname = localname.substr(pos,localname.numChars()-pos);
	}
	ret = asAtomHandler::fromString(sys,localname);
}
ASFUNCTIONBODY_ATOM(XMLNode,appendChild)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	_NR<XMLNode> c;
	ARG_UNPACK_ATOM(c);
	th->node.append_move(c->node);
	if (!c->root.isNull())
		c->root->decRef();
	if (th->is<XMLDocument>())
	{
		th->incRef();
		c->root = _MR(th->as<XMLDocument>());
	}
	else
	{
		assert_and_throw(!th->root.isNull());
		c->root = th->root;
		th->root->incRef();
	}
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
	c->setDeclaredMethodByQName("parseXML","",Class<IFunction>::getFunction(c->getSystemState(),parseXML),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("firstChild","",Class<IFunction>::getFunction(c->getSystemState(),XMLDocument::firstChild),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("createElement","",Class<IFunction>::getFunction(c->getSystemState(),XMLDocument::createElement),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER(c, ignoreWhite);
}

ASFUNCTIONBODY_GETTER_SETTER(XMLDocument, ignoreWhite);

void XMLDocument::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY_ATOM(XMLDocument,_constructor)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	tiny_string source;

	ARG_UNPACK_ATOM(source, "");
	if(!source.empty())
		th->parseXMLImpl(source);
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

	node=rootNode=buildFromString(str, parsemode);
}

ASFUNCTIONBODY_ATOM(XMLDocument,_toString)
{
	//TODO: should output xmlDecl and docTypeDecl, see the
	//documentation on XMLNode.tostring()
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	ret = asAtomHandler::fromObject(abstract_s(sys,th->toString_priv(th->rootNode)));
}

tiny_string XMLDocument::toString()
{
	return toString_priv(rootNode);
}

ASFUNCTIONBODY_ATOM(XMLDocument,parseXML)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	assert_and_throw(argslen==1 && asAtomHandler::isString(args[0]));
	th->parseXMLImpl(asAtomHandler::toString(args[0],sys));
}

ASFUNCTIONBODY_ATOM(XMLDocument,firstChild)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	assert_and_throw(argslen==0);
	pugi::xml_node newNode=th->rootNode.first_child();
	if(newNode.type() == pugi::node_null)
	{
		asAtomHandler::setNull(ret);
		return;
	}
	th->incRef();
	ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(sys,_MR(th),newNode));
}
ASFUNCTIONBODY_ATOM(XMLDocument,createElement)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	assert(th->node==NULL);
	tiny_string name;
	ARG_UNPACK_ATOM(name);
	pugi::xml_node newNode;
	newNode.set_name(name.raw_buf());
	th->incRef();
	ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(sys,_MR(th),newNode));
}
