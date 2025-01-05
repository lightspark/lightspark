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
#include "scripting/avm1/avm1array.h"
#include "scripting/avm1/avm1xml.h"
#include "parsing/amf3_generator.h"

using namespace std;
using namespace lightspark;

XMLNode::XMLNode(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_XMLNODE)
	,root(nullptr),parent(nullptr),children(nullptr),node()
{
}

XMLNode::XMLNode(ASWorker* wrk, Class_base* c, XMLDocument* _r, pugi::xml_node _n, XMLNode* _p):
	ASObject(wrk,c,T_OBJECT,SUBTYPE_XMLNODE),root(_r),parent(_p),children(nullptr),node(_n)
{
	if (root)
	{
		root->incRef();
		root->addStoredMember();
	}
	if (parent)
	{
		parent->incRef();
		parent->addStoredMember();
	}
}

void XMLNode::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes","",c->getSystemState()->getBuiltinFunction(attributes,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
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
	c->setDeclaredMethodByQName("cloneNode","",c->getSystemState()->getBuiltinFunction(cloneNode,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeNode","",c->getSystemState()->getBuiltinFunction(removeNode,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasChildNodes","",c->getSystemState()->getBuiltinFunction(hasChildNodes,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("insertBefore","",c->getSystemState()->getBuiltinFunction(insertBefore,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("prefix","",c->getSystemState()->getBuiltinFunction(prefix,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("namespaceURI","",c->getSystemState()->getBuiltinFunction(namespaceURI,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("getNamespaceForPrefix","",c->getSystemState()->getBuiltinFunction(getNamespaceForPrefix,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("getPrefixForNamespace","",c->getSystemState()->getBuiltinFunction(getPrefixForNamespace,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
}

void XMLNode::finalize()
{
	if (root)
		root->removeStoredMember();
	root=nullptr;
	if (children)
		children->removeStoredMember();
	children = nullptr;
	if (parent)
		parent->removeStoredMember();
	parent = nullptr;
}

bool XMLNode::destruct()
{
	if (root)
		root->removeStoredMember();
	root=nullptr;
	if (children)
		children->removeStoredMember();
	children = nullptr;
	if (parent)
		parent->removeStoredMember();
	parent = nullptr;
	node = pugi::xml_node();
	tmpdoc.reset();
	return destructIntern();
}

void XMLNode::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (root)
		root->prepareShutdown();
	if (children)
		children->prepareShutdown();
	if (parent)
		parent->prepareShutdown();
}
bool XMLNode::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	if (root)
		ret = root->countAllCylicMemberReferences(gcstate) || ret;
	if (children)
		ret = children->countAllCylicMemberReferences(gcstate) || ret;
	if (parent)
		ret = parent->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

ASFUNCTIONBODY_ATOM(XMLNode,_constructor)
{
	if(argslen==0)
		return;
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	uint32_t type;
	tiny_string value;
	ARG_CHECK(ARG_UNPACK(type)(value));
	if(wrk->needsActionScript3())
		th->root= Class<XMLDocument>::getInstanceS(wrk);
	else
		th->root = Class<AVM1XMLDocument>::getInstanceS(wrk);
	th->root->addStoredMember();
	if(type==1 || type==3)
	{
		if (!value.empty() || type==3)
		{
			th->node = th->root->xmldoc.root().append_child(value.raw_buf());
		}
	}
	else
		LOG(LOG_ERROR,"invalid type for XMLNode constructor:"<<type);
}

ASFUNCTIONBODY_ATOM(XMLNode,firstChild)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	assert_and_throw(argslen==0);
	asAtomHandler::setNull(ret);
	th->fillChildren();
	for (uint32_t i = 0; i < th->children->size(); i++)
	{
		asAtom a = asAtomHandler::invalidAtom;
		th->children->at_nocheck(a,i);
		if (asAtomHandler::is<XMLNode>(a))
		{
			pugi::xml_node_type t = asAtomHandler::as<XMLNode>(a)->node.type();
			if (t==pugi::node_declaration) // skip declaration node
				continue;
			ret = a;
			ASATOM_INCREF(ret);
			break;
		}
	}
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
	XMLDocument* r = th->getRootDoc();
	ret = asAtomHandler::fromObject(wrk->needsActionScript3() ? Class<XMLNode>::getInstanceS(wrk,r,newNode,th) :Class<AVM1XMLNode>::getInstanceS(wrk,r,newNode,th));
}

ASFUNCTIONBODY_ATOM(XMLNode,childNodes)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	assert_and_throw(argslen==0);
	Array* res;
	if(th->node.type()==pugi::node_null)
	{
		res = wrk->needsActionScript3() ? Class<Array>::getInstanceSNoArgs(wrk) : Class<AVM1Array>::getInstanceSNoArgs(wrk);
		ret = asAtomHandler::fromObject(res);
		return;
	}
	th->fillChildren();
	res = th->children;
	res->incRef();
	ret = asAtomHandler::fromObject(res);
}


ASFUNCTIONBODY_ATOM(XMLNode,attributes)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	assert_and_throw(argslen==0);
	ret = asAtomHandler::nullAtom;
	if(th->node.type()==pugi::node_null)
		return;
	ASObject* res=new_asobject(wrk);
	ret = asAtomHandler::fromObject(res);
	auto it=th->node.attributes_begin();
	for(;it!=th->node.attributes_end();++it)
	{
		uint32_t attrName = wrk->getSystemState()->getUniqueStringId(it->name());
		asAtom  attrValue = asAtomHandler::fromString(wrk->getSystemState(),it->value());
		res->setDynamicVariableNoCheck(attrName,attrValue,false,!wrk->needsActionScript3());
	}
}

pugi::xml_node XMLNode::getParentNode()
{
	return node.parent();
}

tiny_string XMLNode::getPrefix()
{
	tiny_string prefix = node.name();
	uint32_t pos = prefix.find(":");
	if (pos == tiny_string::npos)
		return "";
	else
		return prefix.substr_bytes(0,pos,prefix.isSinglebyte());
}

bool XMLNode::getNamespaceURI(const pugi::xml_node& n, const tiny_string& prefix, tiny_string& uri)
{
	tiny_string attrname = "xmlns";
	if (!prefix.empty())
	{
		attrname +=":";
		attrname += prefix;
	}
	pugi::xml_attribute a = n.attribute(attrname.raw_buf());
	if (a.empty() && prefix.empty())
	{
		for (auto it = n.attributes_begin(); it != n.attributes_end(); it++)
		{
			tiny_string attr = it->name();
			if (attr.startsWith("xmlns"))
			{
				a = *it;
				break;
			}
		}
	}
	if (a.empty())
	{
		if (!n.parent().empty())
			return getNamespaceURI(n.parent(),prefix,uri);
		return false;
	}
	uri  = a.value();
	return true;
}

bool XMLNode::getPrefixFromNamespaceURI(const pugi::xml_node& n, const tiny_string& uri, tiny_string& prefix)
{
	for (auto it = n.attributes_begin(); it != n.attributes_end(); it++ )
	{
		if  (uri == (*it).value())
		{
			prefix = (*it).name();
			if (prefix.find("xmlns:")==0)
				prefix = prefix.substr_bytes(6,UINT32_MAX,prefix.isSinglebyte());
			else if (prefix.startsWith("xmlns"))
				prefix = "";
			return true;
		}
	}
	if (!n.parent().empty())
		return getPrefixFromNamespaceURI(n.parent(),uri,prefix);
	return false;
}

void XMLNode::removeChild(const pugi::xml_node& child)
{
	if (this->children)
	{
		for (uint32_t i = 0; i < this->children->size(); i++)
		{
			asAtom a = asAtomHandler::invalidAtom;
			this->children->at_nocheck(a,i);
			if (asAtomHandler::is<XMLNode>(a) && asAtomHandler::as<XMLNode>(a)->node == child)
			{
				asAtom tmp = asAtomHandler::invalidAtom;
				asAtom arrobj = asAtomHandler::fromObject(this->children);
				asAtom arrarg = asAtomHandler::fromUInt(i);
				this->children->removeAt(tmp,getInstanceWorker(),arrobj,&arrarg,1);
				ASATOM_DECREF(tmp);
			}
		}
	}
}

void XMLNode::fillChildren()
{
	if (!children)
	{
		children =  getInstanceWorker()->needsActionScript3() ? Class<Array>::getInstanceSNoArgs(getInstanceWorker()) : Class<AVM1Array>::getInstanceSNoArgs(getInstanceWorker());
		children->addStoredMember();
		XMLDocument* r = getRootDoc();
		auto range = node.children();
		for(auto it = range.begin();it!=range.end();++it)
		{
			if(it->type()==pugi::node_declaration || it->type()==pugi::node_comment || it->type()==pugi::node_doctype)
				continue;
			children->push(asAtomHandler::fromObject(getInstanceWorker()->needsActionScript3() ? Class<XMLNode>::getInstanceS(getInstanceWorker(),r, *it,this) : Class<AVM1XMLNode>::getInstanceS(getInstanceWorker(),r, *it,this)));
		}
	}
}

void XMLNode::reloadChildren()
{
	if (children)
	{
		children->resize(0);
		XMLDocument* r = getRootDoc();
		auto range = node.children();
		for(auto it = range.begin();it!=range.end();++it)
		{
			if(it->type()==pugi::node_declaration || it->type()==pugi::node_comment)
				continue;
			children->push(asAtomHandler::fromObject(getInstanceWorker()->needsActionScript3() ? Class<XMLNode>::getInstanceS(getInstanceWorker(),r, *it,this) : Class<AVM1XMLNode>::getInstanceS(getInstanceWorker(),r, *it,this)));
		}
	}
}

void XMLNode::refreshChildren()
{
	if (children)
	{
		uint32_t i = 0;
		while (i < children->size())
		{
			asAtom a = asAtomHandler::invalidAtom;
			children->at_nocheck(a,i);
			if (asAtomHandler::is<XMLNode>(a) && asAtomHandler::as<XMLNode>(a)->parent != this)
			{
				asAtom tmp = asAtomHandler::invalidAtom;
				asAtom arrobj = asAtomHandler::fromObject(this->children);
				asAtom arrarg = asAtomHandler::fromUInt(i);
				this->children->removeAt(tmp,getInstanceWorker(),arrobj,&arrarg,1);
				ASATOM_DECREF(tmp);
			}
			else
				i++;
		}
	}
}

void XMLNode::fillIDMap(ASObject* o)
{
	fillChildren();
	for (uint32_t i =0; i < children->size(); i++)
	{
		asAtom a = asAtomHandler::invalidAtom;
		children->at_nocheck(a,i);
		if (!asAtomHandler::is<XMLNode>(a))
			continue;
		asAtomHandler::as<XMLNode>(a)->fillIDMap(o);
		pugi::xml_attribute attr = asAtomHandler::as<XMLNode>(a)->node.attribute("id");
		if (attr.empty())
			continue;
		tiny_string val = attr.value();
		if (val.empty())
			continue;
		ASATOM_INCREF(a);
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.name_s_id=getSystemState()->getUniqueStringId(val);
		m.isAttribute = false;
		o->setVariableByMultiname(m,a,ASObject::CONST_NOT_ALLOWED,nullptr,getInstanceWorker());
	}
}

ASFUNCTIONBODY_ATOM(XMLNode,parentNode)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	if (!th->parent)
	{
		pugi::xml_node parent = th->getParentNode();
		if (parent.type()!=pugi::node_null && parent.type()!=pugi::node_document)
		{
			XMLNode* grandparent = th->parent ? th->parent->parent : nullptr;
			th->parent = wrk->needsActionScript3() ? Class<XMLNode>::getInstanceS(wrk,th->root, parent,grandparent) : Class<AVM1XMLNode>::getInstanceS(wrk,th->root, parent,grandparent);
			th->parent->addStoredMember();
		}
	}
	if (th->parent)
	{
		th->parent->incRef();
		ret = asAtomHandler::fromObject(th->parent);
	}
	else
		asAtomHandler::setNull(ret);
}

ASFUNCTIONBODY_ATOM(XMLNode,nextSibling)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	asAtomHandler::setNull(ret);
	if(th->node.type()==pugi::node_null)
		return;
	pugi::xml_node sibling;
	if (th->node.parent().type()!=pugi::node_null)
		sibling = th->node.next_sibling();
	if (sibling.type()!=pugi::node_null)
		ret = asAtomHandler::fromObject(wrk->needsActionScript3() ? Class<XMLNode>::getInstanceS(wrk,th->root, sibling,th->parent) : Class<AVM1XMLNode>::getInstanceS(wrk,th->root, sibling,th->parent));
}

ASFUNCTIONBODY_ATOM(XMLNode,previousSibling)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	asAtomHandler::setNull(ret);
	if(th->node.type()==pugi::node_null)
		return;
	pugi::xml_node sibling;
	if (th->node.parent().type()!=pugi::node_null)
		sibling = th->node.previous_sibling();
	if (sibling.type()!=pugi::node_null)
		ret = asAtomHandler::fromObject(wrk->needsActionScript3() ? Class<XMLNode>::getInstanceS(wrk,th->root, sibling,th->parent) : Class<AVM1XMLNode>::getInstanceS(wrk,th->root, sibling,th->parent));
}
ASFUNCTIONBODY_ATOM(XMLNode,_getNodeType)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	int t = 0;
	switch (th->node.type())
	{
		case pugi::node_null:
		case pugi::node_element:
		case pugi::node_document:
			t = 1;
			break;
		case pugi::node_pcdata:
		case pugi::node_cdata:
			t = 3;
			break;
		// case pugi::node_declaration:
		// 	t = 5;
		// 	break;
		// case pugi::node_pi:
		// 	t = 9;
		// 	break;
		// case pugi::node_document:
		// 	t = 11;
		// 	break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"XMLNode.getNodeType: unhandled type:"<<th->node.type());
			break;
	}
	asAtomHandler::setInt(ret,wrk,t);
}

ASFUNCTIONBODY_ATOM(XMLNode,_getNodeName)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	switch (th->node.type())
	{
		case pugi::node_element:
			ret = asAtomHandler::fromObject(abstract_s(wrk,th->node.name()));
			break;
		default:
			ret = asAtomHandler::nullAtom;
			break;
	}
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
	uint32_t pos = localname.find(":");
	if (pos != tiny_string::npos)
	{
		localname = localname.substr(pos+1,UINT32_MAX);
	}
	else if (!wrk->needsActionScript3() && localname.empty())
	{
		ret = asAtomHandler::nullAtom;
		return;
	}
	ret = asAtomHandler::fromString(wrk->getSystemState(),localname);
}
ASFUNCTIONBODY_ATOM(XMLNode,appendChild)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	_NR<XMLNode> c;
	ARG_CHECK(ARG_UNPACK(c));
	if (c.isNull() || c->parent == th)
		return;

	pugi::xml_node newnode = c->is<XMLDocument>() ?  th->node.append_copy(c->node.first_child()) : th->node.append_copy(c->node);

	if (c->parent)
		c->parent->removeChild(c->node);
	if (th->children)
	{
		th->refreshChildren();
		c->incRef();
		asAtom ch = asAtomHandler::fromObjectNoPrimitive(c.getPtr());
		th->children->push(ch);
	}
	if (c->root && c->root != th)
		c->root->removeStoredMember();
	if (th->is<XMLDocument>())
	{
		if (c->root != th)
		{
			c->root = th->as<XMLDocument>();
			c->root->incRef();
			c->root->addStoredMember();
		}
	}
	else
	{
		c->root = th->getRootDoc();
		c->root->incRef();
		c->root->addStoredMember();
	}
	th->incRef();
	th->addStoredMember();
	c->parent = th;
	c->node = newnode;
}

ASFUNCTIONBODY_ATOM(XMLNode,cloneNode)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	bool deep=false;
	ARG_CHECK(ARG_UNPACK(deep,false));
	XMLNode* newnode = wrk->needsActionScript3() ? Class<XMLNode>::getInstanceSNoArgs(wrk) : Class<AVM1XMLNode>::getInstanceSNoArgs(wrk);
	if (deep)
	{
		if (th->is<XMLDocument>())
		{
			newnode->node = newnode->tmpdoc.append_child();
			newnode->node.append_copy(th->node.first_child());
		}
		else
			newnode->node = newnode->tmpdoc.append_copy(th->node);
	}
	else
	{
		newnode->node = newnode->tmpdoc.append_child(th->is<XMLDocument>() ? pugi::node_element : th->node.type());
		newnode->node.set_name(th->node.name());
		newnode->node.set_value(th->node.value());
		for (auto it = th->node.attributes_begin();it != th->node.attributes_end(); it++)
		{
			newnode->node.append_copy(*it);
		}
	}
	ret = asAtomHandler::fromObjectNoPrimitive(newnode);
}

ASFUNCTIONBODY_ATOM(XMLNode,removeNode)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	if (!th->node.parent().empty())
	{
		th->node.parent().remove_child(th->node);
	}
	if (th->parent)
	{
		th->parent->removeChild(th->node);
		th->parent->removeStoredMember();
		th->parent=nullptr;
	}

	if (th->root)
		th->root->removeStoredMember();
	th->root=nullptr;
}
ASFUNCTIONBODY_ATOM(XMLNode,hasChildNodes)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	ret = asAtomHandler::fromBool(th->node.first_child().type()!= pugi::node_null);
}
ASFUNCTIONBODY_ATOM(XMLNode,insertBefore)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);
	_NR<XMLNode> newChild;
	_NR<XMLNode> insertPoint;
	ARG_CHECK(ARG_UNPACK(newChild)(insertPoint));
	if (newChild.isNull() || newChild->parent == th)
		return;
	if (insertPoint.isNull())
		return;
	if (newChild->parent)
		newChild->parent->removeChild(newChild->node);

	pugi::xml_node newnode = th->node.append_copy(newChild->node);
	th->node.insert_move_before(newnode,insertPoint->node);
	th->reloadChildren();
	if (newChild->root && newChild->root != th)
		newChild->root->removeStoredMember();
	if (th->is<XMLDocument>())
	{
		if (newChild->root != th)
		{
			newChild->root = th->as<XMLDocument>();
			newChild->root->incRef();
			newChild->root->addStoredMember();
		}
	}
	else
	{
		newChild->root = th->getRootDoc();
		newChild->root->incRef();
		newChild->root->addStoredMember();
	}
	th->incRef();
	th->addStoredMember();
	newChild->parent = th;
	newChild->node = newnode;
}
ASFUNCTIONBODY_ATOM(XMLNode,prefix)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);

	tiny_string prefix = th->getPrefix();
	tiny_string name = th->node.name();
	if (!wrk->needsActionScript3() && name.empty())
	{
		ret = asAtomHandler::nullAtom;
		return;
	}

	ret = asAtomHandler::fromString(wrk->getSystemState(),prefix);
}

ASFUNCTIONBODY_ATOM(XMLNode,namespaceURI)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);

	ret = asAtomHandler::nullAtom;
	tiny_string prefix = th->getPrefix();
	tiny_string uri;
	if (th->getNamespaceURI(th->node,prefix,uri))
		ret = asAtomHandler::fromString(wrk->getSystemState(),uri);
}

ASFUNCTIONBODY_ATOM(XMLNode,getNamespaceForPrefix)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);

	tiny_string prefix;
	ARG_CHECK(ARG_UNPACK(prefix));

	tiny_string uri;
	if (th->getNamespaceURI(th->node,prefix,uri))
		ret = asAtomHandler::fromString(wrk->getSystemState(),uri);
	else
		ret = asAtomHandler::nullAtom;
}

ASFUNCTIONBODY_ATOM(XMLNode,getPrefixForNamespace)
{
	XMLNode* th=asAtomHandler::as<XMLNode>(obj);

	tiny_string uri;
	ARG_CHECK(ARG_UNPACK(uri));
	tiny_string prefix;
	if (th->getPrefixFromNamespaceURI(th->node,uri,prefix))
		ret = asAtomHandler::fromString(wrk->getSystemState(),prefix);
	else
		ret = asAtomHandler::nullAtom;
}

tiny_string XMLNode::toString()
{
	return toString_priv(node);
}

bool XMLNode::isEqual(ASObject* r)
{
	if (this == r)
		return true;
	if(r->is<XMLNode>())
		return r->as<XMLNode>()->node==this->node;
	return false;
}

tiny_string XMLNode::toString_priv(pugi::xml_node outputNode)
{
	if(outputNode.type() == pugi::node_null)
		return "";

	ostringstream buf;
	outputNode.print(buf,"\t",pugi::format_raw);
	tiny_string ret = tiny_string(buf.str());
	return ret;
}

void XMLDocument::setDecl()
{
	auto it = xmldoc.children().begin();
	while (it != xmldoc.children().end() && doctypedecl.empty() && xmldecl.empty())
	{
		switch (it->type())
		{
			case pugi::node_doctype:
				doctypedecl = "<!DOCTYPE ";
				doctypedecl += it->value();
				doctypedecl += ">";
				break;
			case pugi::node_declaration:
				xmldecl = "<?xml";
				for (auto attr = it->attributes_begin(); attr != it->attributes_end(); attr++)
				{
					xmldecl +=" ";
					xmldecl +=attr->name();
					xmldecl +="='";
					xmldecl +=attr->value();
					xmldecl +="'";
				}
				xmldecl +=" ?>";
				break;
			default:
				break;
		}
		it++;
	}
}

XMLDocument::XMLDocument(ASWorker* wrk, Class_base* c, tiny_string s)
	: XMLNode(wrk,c),status(0),idmap(nullptr),needsActionScript3(true),ignoreWhite(false)
{
	subtype=SUBTYPE_XMLDOCUMENT;
	if (!s.empty())
	{
		parseXMLImpl(s);
		setDecl();
	}
}

void XMLDocument::sinit(Class_base* c)
{
	CLASS_SETUP(c, XMLNode, _constructor, CLASS_SEALED);
	c->isReusable=true;
	c->setDeclaredMethodByQName("parseXML","",c->getSystemState()->getBuiltinFunction(parseXML),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createElement","",c->getSystemState()->getBuiltinFunction(createElement,1,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("createTextNode","",c->getSystemState()->getBuiltinFunction(createTextNode,1,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("idmap","",c->getSystemState()->getBuiltinFunction(_idmap,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("docTypeDecl","",c->getSystemState()->getBuiltinFunction(_docTypeDecl,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("xmlDecl","",c->getSystemState()->getBuiltinFunction(_xmlDecl,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	REGISTER_GETTER_SETTER_RESULTTYPE(c, ignoreWhite,Boolean);
}

void XMLDocument::finalize()
{
	status=0;
	rootNode = pugi::xml_node();
	xmldoc.reset();
	ignoreWhite = false;
	idmap = nullptr;
	doctypedecl.clear();
	xmldecl.clear();
}

bool XMLDocument::destruct()
{
	status=0;
	rootNode = pugi::xml_node();
	xmldoc.reset();
	ignoreWhite = false;
	idmap = nullptr;
	doctypedecl.clear();
	xmldecl.clear();
	return XMLNode::destruct();
}

ASFUNCTIONBODY_GETTER_SETTER(XMLDocument, ignoreWhite)

ASFUNCTIONBODY_ATOM(XMLDocument,_constructor)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	tiny_string source;
	th->idmap = new_asobject(wrk);
	th->addOwnedObject(th->idmap);
	th->idmap->decRef();
	th->ignoreWhite = th->getSystemState()->static_AVM1XMLDocument_ignoreWhite;

	ARG_CHECK(ARG_UNPACK(source, ""));
	if (!source.empty())
	{
		th->parseXMLImpl(source);
		th->setDecl();
	}
	else
		th->rootNode = th->node = th->xmldoc.root();
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
	unsigned int parsemode = pugi::parse_default | pugi::parse_pi | pugi::parse_declaration | pugi::parse_doctype |pugi::parse_fragment;
	if (!ignoreWhite)
		parsemode |= pugi::parse_ws_pcdata;

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
				node=rootNode=pugi::xml_node();
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
	// it seems tha parseXML does replace the document tree, but does not clear the idmap
	th->parseXMLImpl(asAtomHandler::toString(args[0],wrk));
	th->reloadChildren();
}

ASFUNCTIONBODY_ATOM(XMLDocument,createElement)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	tiny_string name;
	ARG_CHECK(ARG_UNPACK(name));
	pugi::xml_node newNode = th->tmpdoc.append_child(name.raw_buf());
	ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(wrk,th,newNode,nullptr));
}
ASFUNCTIONBODY_ATOM(XMLDocument,createTextNode)
{
	ret = asAtomHandler::undefinedAtom;

	if (!asAtomHandler::is<XMLDocument>(obj))
		return;

	XMLDocument* th = asAtomHandler::as<XMLDocument>(obj);
	tiny_string text;
	ARG_CHECK(ARG_UNPACK(text));

	pugi::xml_node newNode = th->tmpdoc.append_child(pugi::node_pcdata);
	newNode.set_value(text.raw_buf());

	ret = asAtomHandler::fromObject(Class<XMLNode>::getInstanceS(wrk, th, newNode,th));
	return;
}

ASFUNCTIONBODY_ATOM(XMLDocument,_idmap)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	th->idmap->incRef();
	ret = asAtomHandler::fromObject(th->idmap);
	th->fillIDMap(th->idmap);
}
ASFUNCTIONBODY_ATOM(XMLDocument,_docTypeDecl)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	if (th->doctypedecl.empty())
		ret = asAtomHandler::undefinedAtom;
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),th->doctypedecl);
}
ASFUNCTIONBODY_ATOM(XMLDocument,_xmlDecl)
{
	XMLDocument* th=asAtomHandler::as<XMLDocument>(obj);
	if (th->xmldecl.empty())
		ret = asAtomHandler::undefinedAtom;
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),th->xmldecl);
}
