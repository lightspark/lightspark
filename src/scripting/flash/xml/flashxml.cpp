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
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Integer.h"
#include "parsing/amf3_generator.h"

using namespace std;
using namespace lightspark;

XMLNode::XMLNode(ASWorker* wrk, Class_base* c, _NR<XMLDocument> _r, pugi::xml_node _n):ASObject(wrk,c,T_OBJECT,SUBTYPE_XMLNODE),root(_r),node(_n)
{
}

void XMLNode::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes","",c->getSystemState()->getBuiltinFunction(attributes),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("childNodes","",c->getSystemState()->getBuiltinFunction(XMLNode::childNodes,0,Class<Array>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("firstChild","",c->getSystemState()->getBuiltinFunction(XMLNode::firstChild,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("lastChild","",c->getSystemState()->getBuiltinFunction(lastChild,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nextSibling","",c->getSystemState()->getBuiltinFunction(nextSibling,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeType","",c->getSystemState()->getBuiltinFunction(_getNodeType,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeName","",c->getSystemState()->getBuiltinFunction(_getNodeName,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeName","",c->getSystemState()->getBuiltinFunction(_setNodeName),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("nodeValue","",c->getSystemState()->getBuiltinFunction(_getNodeValue,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("parentNode","",c->getSystemState()->getBuiltinFunction(parentNode,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("previousSibling","",c->getSystemState()->getBuiltinFunction(previousSibling,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localName","",c->getSystemState()->getBuiltinFunction(_getLocalName,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("appendChild","",c->getSystemState()->getBuiltinFunction(appendChild),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(XMLNode,_constructor)
{
	if(argslen==0)
		return;
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	uint32_t type;
	tiny_string value;
	ARG_CHECK(ARG_UNPACK(type)(value));
	th->root=_MR(Class<XMLDocument>::getInstanceS(wrk));
	if(type==1 || type==3)
	{
		th->root->parseXMLImpl(value);
		th->node=th->root->rootNode;
	}
	else
		LOG(LOG_ERROR,"invalid type for XMLNode constructor:"<<type);
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
	ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(wrk,th->root,newNode));
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
	ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(wrk,th->root,newNode));
}

ASFUNCTIONBODY_ATOM(XMLNode,childNodes)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	Array* res = Class<Array>::getInstanceSNoArgs(wrk);
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
			res->push(asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(wrk,th->root, *it)));
		}
	}
	ret = asAtomHandler::fromObject(res);
}


ASFUNCTIONBODY_ATOM(XMLNode,attributes)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	assert_and_throw(argslen==0);
	ASObject* res=Class<ASObject>::getInstanceS(wrk);
	if(th->node.type()==pugi::node_null)
	{
		ret = asAtomHandler::fromObject(res);
		return;
	}
	auto it=th->node.attributes_begin();
	for(;it!=th->node.attributes_end();++it)
	{
		tiny_string attrName = it->name();
		ASObject* attrValue=abstract_s(wrk,it->value());
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
		ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(wrk,th->root, parent));
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
		ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(wrk,th->root, sibling));
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
		ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(wrk,th->root, sibling));
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
	asAtomHandler::setInt(ret,wrk,t);
}

ASFUNCTIONBODY_ATOM(XMLNode,_getNodeName)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->node.name()));
}
ASFUNCTIONBODY_ATOM(XMLNode,_setNodeName)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	tiny_string name;
	ARG_CHECK(ARG_UNPACK(name));
	if (name.empty())
		LOG(LOG_NOT_IMPLEMENTED,"XMLNode.setNodeName with empty argument");
	else
		th->node.set_name(name.raw_buf());
}

ASFUNCTIONBODY_ATOM(XMLNode,_getNodeValue)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	if(th->node.type() == pugi::node_pcdata)
		ret = asAtomHandler::fromObject(abstract_s(wrk,th->node.value()));
	else
		asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(XMLNode,_toString)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv(th->node)));
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
	ret = asAtomHandler::fromString(wrk->getSystemState(),localname);
}
ASFUNCTIONBODY_ATOM(XMLNode,appendChild)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	_NR<XMLNode> c;
	ARG_CHECK(ARG_UNPACK(c));
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

XMLDocument::XMLDocument(ASWorker* wrk, Class_base* c, tiny_string s)
  : XMLNode(wrk,c),rootNode(nullptr),status(0),needsActionScript3(true),ignoreWhite(false)
{
	subtype=SUBTYPE_XMLDOCUMENT;
	if(!s.empty())
	{
		parseXMLImpl(s);
	}
}

void XMLDocument::sinit(Class_base* c)
{
	CLASS_SETUP(c, XMLNode, _constructor, CLASS_SEALED);
	c->setDeclaredMethodByQName("parseXML","",c->getSystemState()->getBuiltinFunction(parseXML),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("firstChild","",c->getSystemState()->getBuiltinFunction(XMLDocument::firstChild,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("createElement","",c->getSystemState()->getBuiltinFunction(XMLDocument::createElement,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, ignoreWhite,Boolean);
}

ASFUNCTIONBODY_GETTER_SETTER(XMLDocument, ignoreWhite)

ASFUNCTIONBODY_ATOM(XMLDocument,_constructor)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	tiny_string source;

	ARG_CHECK(ARG_UNPACK(source, ""));
	if(!source.empty())
		th->parseXMLImpl(source);
}

void XMLDocument::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing XMLDocument in AMF0 not implemented");
		return;
	}
	out->writeByte(xml_doc_marker);
	out->writeXMLString(objMap, this, toString());
}

int XMLDocument::parseXMLImpl(const string& str)
{
	unsigned int parsemode = pugi::parse_full |pugi::parse_fragment;
	if (!ignoreWhite) parsemode |= pugi::parse_ws_pcdata;

	if (needsActionScript3)
	{
		node=rootNode=buildFromString(str, parsemode);
		return 0;
	}
	else
	{
		// don't throw exception when in AVM1, return status value instead
		pugi::xml_parse_result parseresult;
		node=rootNode=buildFromString(str, parsemode,&parseresult);
		switch (parseresult.status)
		{
			case pugi::status_ok:
				return 0;
			case pugi::status_end_element_mismatch:
				return -9;
			case pugi::status_unrecognized_tag:
				return -6;
			case pugi::status_bad_pi:
				return -3;
			case pugi::status_bad_attribute:
				return -8;
			case pugi::status_bad_cdata:
				return -2;
			case pugi::status_bad_doctype:
				return -4;
			case pugi::status_bad_comment:
				return -5;
			default:
				LOG(LOG_ERROR,"xml parser error:"<<str<<" "<<parseresult.status<<" "<<parseresult.description());
				return -1000;
			
		}
	}
}

ASFUNCTIONBODY_ATOM(XMLDocument,_toString)
{
	//TODO: should output xmlDecl and docTypeDecl, see the
	//documentation on XMLNode.tostring()
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv(th->rootNode)));
}

tiny_string XMLDocument::toString()
{
	return toString_priv(rootNode);
}

ASFUNCTIONBODY_ATOM(XMLDocument,parseXML)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	assert_and_throw(argslen==1 && asAtomHandler::isString(args[0]));
	th->parseXMLImpl(asAtomHandler::toString(args[0],wrk));
}

ASFUNCTIONBODY_ATOM(XMLDocument,firstChild)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	assert_and_throw(argslen==0);
	pugi::xml_node newNode=th->rootNode.first_child();
	if (newNode.type()==pugi::node_declaration) // skip declaration node
		newNode = newNode.next_sibling();
	if(newNode.type() == pugi::node_null)
	{
		asAtomHandler::setNull(ret);
		return;
	}
	th->incRef();
	ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(wrk,_MR(th),newNode));
}
ASFUNCTIONBODY_ATOM(XMLDocument,createElement)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	assert(th->node==nullptr);
	tiny_string name;
	ARG_CHECK(ARG_UNPACK(name));
	pugi::xml_node newNode;
	newNode.set_name(name.raw_buf());
	th->incRef();
	ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(wrk,_MR(th),newNode));
}
