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

#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/flash/xml/flashxml.h"
#include "scripting/class.h"
#include "compat.h"
#include "scripting/argconv.h"
#include "abc.h"
#include "parsing/amf3_generator.h"
#include <libxml/tree.h>
#include <libxml++/parsers/domparser.h>
#include <libxml++/nodes/textnode.h>

using namespace std;
using namespace lightspark;

XML::XML(Class_base* c):ASObject(c),node(NULL),constructed(false),ignoreComments(true)
{
}

XML::XML(Class_base* c,const string& str):ASObject(c),node(NULL),constructed(true)
{
	node=buildFromString(str);
}

XML::XML(Class_base* c,_R<XML> _r, xmlpp::Node* _n):ASObject(c),root(_r),node(_n),constructed(true)
{
	assert(node);
}

XML::XML(Class_base* c,xmlpp::Node* _n):ASObject(c),constructed(true)
{
	assert(_n);
	node=buildCopy(_n);
	assert(node);
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
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(valueOf),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(valueOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toXMLString",AS3,Class<IFunction>::getFunction(toXMLString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nodeKind",AS3,Class<IFunction>::getFunction(nodeKind),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("child",AS3,Class<IFunction>::getFunction(child),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("children",AS3,Class<IFunction>::getFunction(children),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("childIndex",AS3,Class<IFunction>::getFunction(childIndex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains",AS3,Class<IFunction>::getFunction(contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attribute",AS3,Class<IFunction>::getFunction(attribute),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes",AS3,Class<IFunction>::getFunction(attributes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("length",AS3,Class<IFunction>::getFunction(length),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("localName",AS3,Class<IFunction>::getFunction(localName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("name",AS3,Class<IFunction>::getFunction(name),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("namespace",AS3,Class<IFunction>::getFunction(_namespace),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descendants",AS3,Class<IFunction>::getFunction(descendants),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendChild",AS3,Class<IFunction>::getFunction(appendChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parent",AS3,Class<IFunction>::getFunction(parent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inScopeNamespaces",AS3,Class<IFunction>::getFunction(inScopeNamespaces),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addNamespace",AS3,Class<IFunction>::getFunction(addNamespace),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasSimpleContent",AS3,Class<IFunction>::getFunction(_hasSimpleContent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasComplexContent",AS3,Class<IFunction>::getFunction(_hasComplexContent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("text",AS3,Class<IFunction>::getFunction(text),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("elements",AS3,Class<IFunction>::getFunction(elements),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setLocalName",AS3,Class<IFunction>::getFunction(_setLocalName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setName",AS3,Class<IFunction>::getFunction(_setName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setNamespace",AS3,Class<IFunction>::getFunction(_setNamespace),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copy",AS3,Class<IFunction>::getFunction(_copy),NORMAL_METHOD,true);
}

ASFUNCTIONBODY(XML,generator)
{
	assert(obj==NULL);
	assert_and_throw(argslen<=1);
	if (argslen == 0 ||
	    args[0]->is<Null>() ||
	    args[0]->is<Undefined>())
	{
		return Class<XML>::getInstanceS("");
	}
	else if(args[0]->is<ASString>() ||
		args[0]->is<Number>() ||
		args[0]->is<Integer>() ||
		args[0]->is<UInteger>() ||
		args[0]->is<Boolean>())
	{
		return Class<XML>::getInstanceS(args[0]->toString());
	}
	else if(args[0]->is<XML>())
	{
		args[0]->incRef();
		return args[0];
	}
	else if(args[0]->is<XMLList>())
	{
		_R<XML> ret=args[0]->as<XMLList>()->reduceToXML();
		ret->incRef();
		return ret.getPtr();
	}
	else if(args[0]->is<XMLNode>())
	{
		return Class<XML>::getInstanceS(args[0]->as<XMLNode>()->node);
	}
	else
	{
		return Class<XML>::getInstanceS(args[0]->toString());
	}
}

ASFUNCTIONBODY(XML,_constructor)
{
	assert_and_throw(argslen<=1);
	XML* th=Class<XML>::cast(obj);
	if(argslen==0 && th->constructed) //If root is already set we have been constructed outside AS code
		return NULL;

	if(argslen==0 ||
	   args[0]->is<Null>() || 
	   args[0]->is<Undefined>())
	{
		th->node=th->buildFromString("");
	}
	else if(args[0]->getClass()->isSubClass(Class<ByteArray>::getClass()))
	{
		//Official documentation says that generic Objects are not supported.
		//ByteArray seems to be though (see XML test) so let's support it
		ByteArray* ba=Class<ByteArray>::cast(args[0]);
		uint32_t len=ba->getLength();
		const uint8_t* str=ba->getBuffer(len, false);
		th->node=th->buildFromString(std::string((const char*)str,len),
					     getVm()->getDefaultXMLNamespace());
	}
	else if(args[0]->is<ASString>() ||
		args[0]->is<Number>() ||
		args[0]->is<Integer>() ||
		args[0]->is<UInteger>() ||
		args[0]->is<Boolean>())
	{
		//By specs, XML constructor will only convert to string Numbers or Booleans
		//ints are not explicitly mentioned, but they seem to work
		th->node=th->buildFromString(args[0]->toString(),
					     getVm()->getDefaultXMLNamespace());
	}
	else if(args[0]->is<XML>())
	{
		th->node=th->buildCopy(args[0]->as<XML>()->node);
	}
	else if(args[0]->is<XMLList>())
	{
		XMLList *list=args[0]->as<XMLList>();
		_R<XML> reduced=list->reduceToXML();
		th->node=th->buildCopy(reduced->node);
	}
	else
	{
		th->node=th->buildFromString(args[0]->toString(),
					     getVm()->getDefaultXMLNamespace());
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
		case XML_COMMENT_NODE:
			return Class<ASString>::getInstanceS("comment");
		case XML_PI_NODE:
			return Class<ASString>::getInstanceS("processing-instruction");
		default:
		{
			LOG(LOG_ERROR,"Unsupported XML type " << libXml2Node->type);
			throw UnsupportedException("Unsupported XML node type");
		}
	}
}

ASFUNCTIONBODY(XML,length)
{
	return abstract_i(1);
}

ASFUNCTIONBODY(XML,localName)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	assert(th->node);
	xmlElementType nodetype=th->node->cobj()->type;
	if(nodetype==XML_TEXT_NODE || nodetype==XML_COMMENT_NODE)
		return getSys()->getNullRef();
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
		return getSys()->getNullRef();
	else
	{
		ASQName* ret = Class<ASQName>::getInstanceS();
		ret->setByNode(th->node);
		return ret;
	}
}

ASFUNCTIONBODY(XML,descendants)
{
	XML* th=Class<XML>::cast(obj);
	tiny_string name;
	ARG_UNPACK(name,"*");
 	XMLVector ret;
	th->getDescendantsByQName(name,"",ret);
 	return Class<XMLList>::getInstanceS(ret);
}

ASFUNCTIONBODY(XML,appendChild)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==1);
	_NR<XML> arg;
	if(args[0]->getClass()==Class<XML>::getClass())
	{
		args[0]->incRef();
		arg=_MR(Class<XML>::cast(args[0]));
	}
	else if(args[0]->getClass()==Class<XMLList>::getClass())
	{
		XMLList* list=Class<XMLList>::cast(args[0]);
		list->appendNodesTo(th);
		th->incRef();
		return th;
	}
	else
	{
		//The appendChild specs says that any other type is converted to string
		//NOTE: this is explicitly different from XML constructor, that will only convert to
		//string Numbers and Booleans
		arg=_MR(Class<XML>::getInstanceS(args[0]->toString()));
	}

	th->node->import_node(arg->node, true);
	th->incRef();
	return th;
}

/* returns the named attribute in an XMLList */
ASFUNCTIONBODY(XML,attribute)
{
	XML* th = obj->as<XML>();
	tiny_string attrname;
	//see spec for QName handling
	if(argslen > 0 && args[0]->is<QName>())
		LOG(LOG_NOT_IMPLEMENTED,"XML.attribute called with QName");
	ARG_UNPACK (attrname);

	xmlpp::Element* elem=dynamic_cast<xmlpp::Element*>(th->node);
        if(elem==NULL)
		return Class<XMLList>::getInstanceS();
	xmlpp::Attribute* attribute = elem->get_attribute(attrname);
	if(!attribute)
	{
		LOG(LOG_ERROR,"attribute " << attrname << " not found");
		return Class<XMLList>::getInstanceS();
	}

	XMLVector ret;
	ret.push_back(_MR(Class<XML>::getInstanceS(th->getRootNode(), attribute)));
	return Class<XMLList>::getInstanceS(ret);
}

ASFUNCTIONBODY(XML,attributes)
{
	assert_and_throw(argslen==0);
	return obj->as<XML>()->getAllAttributes();
}

XMLList* XML::getAllAttributes()
{
	XML::XMLVector attributes=getAttributes();
	return Class<XMLList>::getInstanceS(attributes);
}

XML::XMLVector XML::getAttributes(const tiny_string& name,
				  const tiny_string& namespace_uri)
{
	assert(node);
	//Use low level libxml2 access for speed
	const xmlNode* xmlN = node->cobj();
	XMLVector ret;
	//Only elements can have attributes
	if(xmlN->type!=XML_ELEMENT_NODE)
		return ret;

	_NR<XML> rootXML=getRootNode();
	for(xmlAttr* attr=xmlN->properties; attr!=NULL; attr=attr->next)
	{
		if((name=="*" || name==attr->name) &&
		   (namespace_uri=="*" || (namespace_uri=="" && attr->ns==NULL) || (attr->ns && namespace_uri==attr->ns->href)))
		{
			//NOTE: libxmlpp headers says that Node::create_wrapper
			//is supposed to be internal API. Still it's very useful and
			//we use it.
			xmlpp::Node::create_wrapper(reinterpret_cast<xmlNode*>(attr));
			xmlpp::Node* attrX=static_cast<xmlpp::Node*>(attr->_private);
			ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, attrX)));
		}
	}
	return ret;
}

void XML::toXMLString_priv(xmlBufferPtr buf)
{
	//NOTE: this function is not thread-safe, as it can modify the xmlNode
	xmlDocPtr xmlDoc=getRootNode()->parser.get_document()->cobj();
	assert(xmlDoc);
	xmlNodePtr cNode=node->cobj();
	int retVal;
	if(cNode->type == XML_ATTRIBUTE_NODE)
	{
		//cobj() return a xmlNodePtr for XML_ATTRIBUTE_NODE
		//even though its actually a different structure
		//containing only the first few member. Especially,
		//there is no nsDef member in that struct.
		retVal=xmlNodeBufGetContent(buf, cNode);
	}
	else
	{
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
			for(uint32_t i=0;i<localNamespaces.size()-1;++i)
				localNamespaces[i].next=&localNamespaces[i+1];
			localNamespaces.back().next=NULL;
			//Free the namespaces arrary
			xmlFree(neededNamespaces);
			//Override the node defined namespaces
			cNode->nsDef=&localNamespaces.front();
		}
		retVal=xmlNodeDump(buf, xmlDoc, cNode, 0, 0);
		//Restore the previously defined namespaces
		cNode->nsDef=oldNsDef;
	}
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

void XML::childrenImpl(XMLVector& ret, const tiny_string& name)
{
	assert(node);
	const xmlpp::Node::NodeList& list=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=list.begin();
	_NR<XML> rootXML=getRootNode();
	for(;it!=list.end();++it)
	{
		if(name!="*" && (*it)->get_name() != name.raw_buf())
			continue;

		// Ignore white-space-only text nodes
		xmlpp::TextNode *textnode=dynamic_cast<xmlpp::TextNode*>(*it);
		if (textnode && textnode->is_white_space())
			continue;

		ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));
	}
}

void XML::childrenImpl(XMLVector& ret, uint32_t index)
{
	assert(node);
	_NR<XML> rootXML=getRootNode();
	uint32_t i=0;
	const xmlpp::Node::NodeList& list=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=list.begin();
	while(it!=list.end())
	{
		xmlpp::TextNode* nodeText = dynamic_cast<xmlpp::TextNode*>(*it);
		if(!(nodeText && nodeText->is_white_space()))
		{
			if(i==index)
			{
				ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));
				break;
			}

			++i;
		}

		++it;
	}
}

ASFUNCTIONBODY(XML,child)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==1);
	const tiny_string& arg0=args[0]->toString();
	XMLVector ret;
	uint32_t index=0;
	multiname mname(NULL);
	mname.name_s_id=getSys()->getUniqueStringId(arg0);
	mname.name_type=multiname::NAME_STRING;
	mname.ns.push_back(nsNameAndKind("",NAMESPACE));
	mname.isAttribute=false;
	if(Array::isValidMultiname(mname, index))
		th->childrenImpl(ret, index);
	else
		th->childrenImpl(ret, arg0);
	XMLList* retObj=Class<XMLList>::getInstanceS(ret);
	return retObj;
}

ASFUNCTIONBODY(XML,children)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	XMLVector ret;
	th->childrenImpl(ret, "*");
	XMLList* retObj=Class<XMLList>::getInstanceS(ret);
	return retObj;
}

ASFUNCTIONBODY(XML,childIndex)
{
	XML* th=Class<XML>::cast(obj);
	xmlpp::Node *parent=th->node->get_parent();
	if (parent && (th->node->cobj()->type!=XML_ATTRIBUTE_NODE))
	{
		xmlpp::Node::NodeList children=parent->get_children();
		xmlpp::Node::NodeList::const_iterator it;
		unsigned int n;
		
		it=children.begin();
		n=0;
		while(it!=children.end())
		{
			if((*it)==th->node)
				return abstract_i(n);

			// Ignore white-space text nodes
			xmlpp::TextNode *textnode=dynamic_cast<xmlpp::TextNode*>(*it);
			if (!(textnode && textnode->is_white_space()))
			{
				++n;
			}

			++it;
		}
	}

	return abstract_i(-1);
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

ASFUNCTIONBODY(XML,valueOf)
{
	obj->incRef();
	return obj;
}

void XML::getText(XMLVector& ret)
{
	xmlpp::Node::NodeList nl = node->get_children();
	xmlpp::Node::NodeList::iterator i;
	_NR<XML> childroot = root;
	if(childroot.isNull())
	{
		this->incRef();
		childroot = _MR(this);
	}
	for(i=nl.begin(); i!= nl.end(); ++i)
	{
		xmlpp::TextNode* nodeText = dynamic_cast<xmlpp::TextNode*>(*i);
		//The official implementation seems to ignore whitespace-only textnodes
		if(nodeText && !nodeText->is_white_space())
			ret.push_back( _MR(Class<XML>::getInstanceS(childroot, nodeText)) );
	}
}

ASFUNCTIONBODY(XML,text)
{
	XML *th = obj->as<XML>();
	ARG_UNPACK;
	XMLVector ret;
	th->getText(ret);
	return Class<XMLList>::getInstanceS(ret);
}

ASFUNCTIONBODY(XML,elements)
{
	XMLVector ret;
	XML *th = obj->as<XML>();
	tiny_string name;
	ARG_UNPACK (name, "");
	if (name=="*")
		name="";

	th->getElementNodes(name, ret);
	return Class<XMLList>::getInstanceS(ret);
}

void XML::getElementNodes(const tiny_string& name, XMLVector& foundElements)
{
	_NR<XML> rootXML=getRootNode();
	const xmlpp::Node::NodeList& children=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=children.begin();
	for(;it!=children.end();++it)
	{
		xmlElementType nodetype=(*it)->cobj()->type;
		if(nodetype==XML_ELEMENT_NODE && (name.empty() || name == (*it)->get_name()))
			foundElements.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));
	}
}

ASFUNCTIONBODY(XML,inScopeNamespaces)
{
	XML *th = obj->as<XML>();
	Array *namespaces = Class<Array>::getInstanceS();
	set<tiny_string> seen_prefix;

	xmlDocPtr xmlDoc=th->getRootNode()->parser.get_document()->cobj();
	xmlNsPtr *nsarr=xmlGetNsList(xmlDoc, th->node->cobj());
	if(nsarr)
	{
		for(int i=0; nsarr[i]!=NULL; i++)
		{
			if(!nsarr[i]->prefix)
				continue;

			tiny_string prefix((const char*)nsarr[i]->prefix, true);
			if(seen_prefix.find(prefix)==seen_prefix.end())
			{
				tiny_string uri((const char*)nsarr[i]->href, true);
				Namespace *ns=Class<Namespace>::getInstanceS(uri, prefix);
				namespaces->push(_MR(ns));
			}

			seen_prefix.insert(prefix);
		}

		xmlFree(nsarr);
	}

	return namespaces;
}

ASFUNCTIONBODY(XML,addNamespace)
{
	XML *th = obj->as<XML>();
	if(argslen == 0)
		throw Class<ArgumentError>::getInstanceS("Error #1063: Non-optional argument missing");

	xmlpp::Element *element=dynamic_cast<xmlpp::Element*>(th->node);
	if(!element)
		return NULL;

	// TODO: check if the prefix already exists

	Namespace *ns=dynamic_cast<Namespace *>(args[0]);
	if(ns)
	{
		tiny_string uri=ns->getURI();
		bool prefix_is_undefined=false;
		tiny_string prefix=ns->getPrefix(prefix_is_undefined);
		element->set_namespace_declaration(uri, prefix);
	}
	else
	{
		tiny_string uri=args[0]->toString();
		element->set_namespace_declaration(uri);
	}

	return NULL;
}

ASObject *XML::getParentNode()
{
	xmlpp::Node *parent=node->get_parent();
	if (parent)
		return Class<XML>::getInstanceS(getRootNode(), parent);
	else
		return getSys()->getUndefinedRef();
}

ASFUNCTIONBODY(XML,parent)
{
	XML *th = obj->as<XML>();
	return th->getParentNode();
}

ASFUNCTIONBODY(XML,contains)
{
	XML *th = obj->as<XML>();
	_NR<ASObject> value;
	ARG_UNPACK(value);
	if(!value->is<XML>())
		return abstract_b(false);

	return abstract_b(th->isEqual(value.getPtr()));
}

ASFUNCTIONBODY(XML,_namespace)
{
	XML *th = obj->as<XML>();
	tiny_string prefix;
	ARG_UNPACK(prefix, "");

	xmlElementType nodetype=th->node->cobj()->type;
	if(prefix.empty() && 
	   nodetype!=XML_ELEMENT_NODE && 
	   nodetype!=XML_ATTRIBUTE_NODE)
	{
		return getSys()->getNullRef();
	}

	tiny_string local_uri=th->node->get_namespace_uri();
	ASObject *ret=NULL;
	xmlDocPtr xmlDoc=th->getRootNode()->parser.get_document()->cobj();
	xmlNsPtr *nsarr=xmlGetNsList(xmlDoc, th->node->cobj());
	if(nsarr)
	{
		for(int i=0; nsarr[i]!=NULL; i++)
		{
			tiny_string ns_prefix;
			if(nsarr[i]->prefix)
				ns_prefix=tiny_string((const char*)nsarr[i]->prefix, true);
			tiny_string ns_uri((const char*)nsarr[i]->href, true);
			if(!prefix.empty() && ns_prefix==prefix)
			{
				ret=Class<Namespace>::getInstanceS(ns_uri, prefix);
				break;
			}
			else if(prefix.empty() && ns_uri==local_uri)
			{
				ret=Class<Namespace>::getInstanceS(ns_uri);
				break;
			}
		}

		xmlFree(nsarr);
	}

	if(!ret)
	{
		if(prefix.empty() && local_uri.empty())
		{
			ret=Class<Namespace>::getInstanceS();
		}
		else
		{
			ret=getSys()->getUndefinedRef();
		}
	}

	return ret;
}

ASFUNCTIONBODY(XML,_setLocalName)
{
	XML *th = obj->as<XML>();
	if(argslen == 0)
		throw Class<ArgumentError>::getInstanceS("Error #1063: Non-optional argument missing");

	xmlElementType nodetype=th->node->cobj()->type;
	if(nodetype==XML_TEXT_NODE || nodetype==XML_COMMENT_NODE)
		return NULL;

	tiny_string new_name;
	if(args[0]->is<ASQName>())
	{
		new_name=args[0]->as<ASQName>()->getLocalName();
	}
	else
	{
		new_name=args[0]->toString();
	}

	th->setLocalName(new_name);

	return NULL;
}

void XML::setLocalName(const tiny_string& new_name)
{
	if(!isXMLName(Class<ASString>::getInstanceS(new_name)))
	{
		throw Class<TypeError>::getInstanceS("Error #1117");
	}

	node->set_name(new_name);
}

ASFUNCTIONBODY(XML,_setName)
{
	XML *th = obj->as<XML>();
	if(argslen == 0)
		throw Class<ArgumentError>::getInstanceS("Error #1063: Non-optional argument missing");

	xmlElementType nodetype=th->node->cobj()->type;
	if(nodetype==XML_TEXT_NODE || nodetype==XML_COMMENT_NODE)
		return NULL;

	tiny_string localname;
	tiny_string ns_uri;
	if(args[0]->is<ASQName>())
	{
		ASQName *qname=args[0]->as<ASQName>();
		localname=qname->getLocalName();
		ns_uri=qname->getURI();
	}
	else if (!args[0]->is<Undefined>())
	{
		localname=args[0]->toString();
	}

	th->setLocalName(localname);
	th->setNamespace(ns_uri);

	return NULL;
}

ASFUNCTIONBODY(XML,_setNamespace)
{
	XML *th = obj->as<XML>();
	if(argslen == 0)
		throw Class<ArgumentError>::getInstanceS("Error #1063: Non-optional argument missing");

	xmlElementType nodetype=th->node->cobj()->type;
	if(nodetype==XML_TEXT_NODE ||
	   nodetype==XML_COMMENT_NODE ||
	   nodetype==XML_PI_NODE)
		return NULL;

	tiny_string ns_uri;
	tiny_string ns_prefix;
	if(args[0]->is<Namespace>())
	{
		Namespace *ns=args[0]->as<Namespace>();
		ns_uri=ns->getURI();
		bool prefix_is_undefined=true;
		ns_prefix=ns->getPrefix(prefix_is_undefined);
	}
	else if(args[0]->is<ASQName>())
	{
		ASQName *qname=args[0]->as<ASQName>();
		ns_uri=qname->getURI();
	}
	else if (!args[0]->is<Undefined>())
	{
		ns_uri=args[0]->toString();
	}

	th->setNamespace(ns_uri, ns_prefix);
	
	return NULL;
}

void XML::setNamespace(const tiny_string& ns_uri, const tiny_string& ns_prefix)
{
	if(ns_uri.empty())
	{
		// libxml++ set_namespace() doesn't seem to be able to
		// reset the namespace to empty (default) namespace,
		// so we have to do this through libxml2
		xmlDocPtr xmlDoc=getRootNode()->parser.get_document()->cobj();
		xmlNsPtr default_ns=xmlSearchNs(xmlDoc, node->cobj(), NULL);
		xmlSetNs(node->cobj(), default_ns);
	}
	else
	{
		if(!ns_prefix.empty())
		{
			xmlpp::Element *element;
			element=dynamic_cast<xmlpp::Element *>(node);
			xmlpp::Attribute *attribute;
			attribute=dynamic_cast<xmlpp::Attribute *>(node);
			if(attribute)
				element=attribute->get_parent();

			if(element)
				element->set_namespace_declaration(ns_uri, ns_prefix);
		}

		node->set_namespace(getNamespacePrefixByURI(ns_uri, true));
	}
}

ASFUNCTIONBODY(XML,_copy)
{
	XML* th=Class<XML>::cast(obj);
	return th->copy();
}

XML *XML::copy() const
{
	return Class<XML>::getInstanceS(node);
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
		XMLVector& ret)
{
	//Check if this node is being requested. The "*" string means all
	if(name=="*" || name == node->get_name())
		ret.push_back(_MR(Class<XML>::getInstanceS(root, node)));
	//NOTE: Creating a temporary list is quite a large overhead, but there is no way in libxml++ to access the first child
	const xmlpp::Node::NodeList& list=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=list.begin();
	for(;it!=list.end();++it)
		recursiveGetDescendantsByQName(root, *it, name, ns, ret);
}

void XML::getDescendantsByQName(const tiny_string& name, const tiny_string& ns, XMLVector& ret)
{
	assert(node);
	assert_and_throw(ns=="");
	recursiveGetDescendantsByQName(getRootNode(), node, name, ns, ret);
}

_NR<ASObject> XML::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0)
	{
		_NR<ASObject> res=ASObject::getVariableByMultiname(name,opt);

		//If a method is not found on XML object and the
		//object is a leaf node, delegate to ASString
		if(res.isNull() && hasSimpleContent())
		{
			ASString *contentstr=Class<ASString>::getInstanceS(toString_priv());
			res=contentstr->getVariableByMultiname(name, opt);
			contentstr->decRef();
		}

		return res;
	}

	if(node==NULL)
	{
		//This is possible if the XML object was created from an empty string
		return NullRef;
	}

	bool isAttr=name.isAttribute;
	unsigned int index=0;
	//Normalize the name to the string form
	tiny_string normalizedName=name.normalizedName();
	if(normalizedName=="*")
		normalizedName="";

	//Only the first namespace is used, is this right?
	tiny_string namespace_uri;
	if(name.ns.size() > 0 && !name.ns[0].hasEmptyName())
	{
		nsNameAndKindImpl ns=name.ns[0].getImpl();
		assert_and_throw(ns.kind==NAMESPACE);
		namespace_uri=ns.name;
	}

	// namespace set by "default xml namespace = ..."
	if(namespace_uri.empty())
		namespace_uri=getVm()->getDefaultXMLNamespace();

	const char *buf=normalizedName.raw_buf();
	if(!normalizedName.empty() && normalizedName.charAt(0)=='@')
	{
		isAttr=true;
		buf+=1;
	}
	if(isAttr)
	{
		//Lookup attribute
		tiny_string ns_uri=namespace_uri.empty() ? "*" : namespace_uri;
		const XMLVector& attributes=getAttributes(buf, ns_uri);
		return _MNR(Class<XMLList>::getInstanceS(attributes));
	}
	else if(Array::isValidMultiname(name,index))
	{
		// If the multiname is a valid array property, the XML
		// object is treated as a single-item XMLList.
		if(index==0)
		{
			incRef();
			return _MNR(this);
		}
		else
			return NullRef;
	}
	else
	{
		//Lookup children
		_NR<XML> rootnode=getRootNode();

		//Use low level libxml2 access for speed
		const xmlNode* xmlN = node->cobj();
		XMLVector ret;
		for(xmlNode* child=xmlN->children; child!=NULL; child=child->next)
		{
			bool nameMatches = (normalizedName=="" || normalizedName==child->name);
			bool nsMatches = (namespace_uri=="*" || 
					  (namespace_uri=="" && child->ns==NULL) || 
					  (child->ns && namespace_uri==child->ns->href));

			if(nameMatches && nsMatches)
			{
				//NOTE: libxmlpp headers says that Node::create_wrapper
				//is supposed to be internal API. Still it's very useful and
				//we use it.
				xmlpp::Node::create_wrapper(child);
				xmlpp::Node* attrX=static_cast<xmlpp::Node*>(child->_private);
				ret.push_back(_MR(Class<XML>::getInstanceS(rootnode, attrX)));
			}
		}
		if(ret.empty() && (opt & XML_STRICT)!=0)
			return NullRef;

		return _MNR(Class<XMLList>::getInstanceS(ret));
	}
}

void XML::setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
{
	unsigned int index=0;
	bool isAttr=name.isAttribute;
	//Normalize the name to the string form
	const tiny_string normalizedName=name.normalizedName();

	//Only the first namespace is used, is this right?
	tiny_string ns_uri;
	tiny_string ns_prefix;
	if(name.ns.size() > 0 && !name.ns[0].hasEmptyName())
	{
		nsNameAndKindImpl ns=name.ns[0].getImpl();
		assert_and_throw(ns.kind==NAMESPACE);
		ns_uri=ns.name;
		ns_prefix=getNamespacePrefixByURI(ns_uri);
	}

	// namespace set by "default xml namespace = ..."
	if (ns_uri.empty() && ns_prefix.empty())
	{
		ns_uri = getVm()->getDefaultXMLNamespace();
	}

	const char *buf=normalizedName.raw_buf();
	if(!normalizedName.empty() && normalizedName.charAt(0)=='@')
	{
		isAttr=true;
		buf+=1;
	}
	if(isAttr)
	{
		//To have attributes we must be an Element
		xmlpp::Element* element=dynamic_cast<xmlpp::Element*>(node);
		assert_and_throw(element);
		element->set_attribute(buf, o->toString(), ns_prefix);

		if(ns_prefix.empty() && !ns_uri.empty())
		{
			element->set_namespace_declaration(ns_uri);
		}
	}
	else if(Array::isValidMultiname(name,index))
	{
		LOG(LOG_NOT_IMPLEMENTED, "XML::setVariableByMultiname: array indexes");
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED, "XML::setVariableByMultiname: should replace if a node with the given name exists");
		if(o->is<XML>() || o->is<XMLList>())
		{
			LOG(LOG_NOT_IMPLEMENTED, "XML::setVariableByMultiname: assigning XML values not implemented");
			return;
		}

		xmlpp::Element* child=node->add_child(getSys()->getStringFromUniqueId(name.name_s_id), ns_prefix);

		child->add_child_text(o->toString());

		if(ns_prefix.empty() && !ns_uri.empty())
		{
			child->set_namespace_declaration(ns_uri);
		}
	}
}

bool XML::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(node==NULL || considerDynamic == false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	//Only the first namespace is used, is this right?
	tiny_string ns_uri;
	tiny_string ns_prefix;
	if(name.ns.size() > 0 && !name.ns[0].hasEmptyName())
	{
		nsNameAndKindImpl ns=name.ns[0].getImpl();
		assert_and_throw(ns.kind==NAMESPACE);
		ns_uri=ns.name;
		ns_prefix=getNamespacePrefixByURI(ns_uri);
	}

	// namespace set by "default xml namespace = ..."
	if (ns_uri.empty() && ns_prefix.empty())
	{
		ns_uri = getVm()->getDefaultXMLNamespace();
	}

	bool isAttr=name.isAttribute;
	const tiny_string normalizedName=name.normalizedName();
	const char *buf=normalizedName.raw_buf();
	if(!normalizedName.empty() && normalizedName.charAt(0)=='@')
	{
		isAttr=true;
		buf+=1;
	}
	if(isAttr)
	{
		//Lookup attribute
		assert(node);
		//To have attributes we must be an Element
		xmlpp::Element* element=dynamic_cast<xmlpp::Element*>(node);
		if(element)
		{
			xmlpp::Attribute* attr=element->get_attribute(buf, ns_prefix);
			if(attr!=NULL)
				return true;
		}
	}
	else
	{
		//Lookup children
		assert(node);
		//Use low level libxml2 access to optimize the code
		const xmlNode* cNode=node->cobj();
		for(const xmlNode* cur=cNode->children;cur!=NULL;cur=cur->next)
		{
			//NOTE: xmlStrEqual returns 1 when the strings are equal.
			bool name_match=xmlStrEqual(cur->name,(const xmlChar*)buf);
			bool ns_match=ns_uri.empty() || 
				(cur->ns && xmlStrEqual(cur->ns->href, (const xmlChar*)ns_uri.raw_buf()));
			if(name_match && ns_match)
				return true;
		}
	}

	//Try the normal path as the last resource
	return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
}

tiny_string XML::getNamespacePrefixByURI(const tiny_string& uri, bool create)
{
	tiny_string prefix;
	bool found=false;
	xmlDocPtr xmlDoc=getRootNode()->parser.get_document()->cobj();
	xmlNsPtr* namespaces=xmlGetNsList(xmlDoc,node->cobj());

	if(namespaces)
	{
		for(int i=0; namespaces[i]!=NULL; i++)
		{
			tiny_string nsuri((const char*)namespaces[i]->href);
			if(nsuri==uri)
			{
				if(namespaces[i]->prefix)
				{
					prefix=tiny_string((const char*)namespaces[i]->prefix, true);
				}

				found = true;
				break;
			}
		}

		xmlFree(namespaces);
	}

	if(!found && create)
	{
		xmlpp::Element *element=dynamic_cast<xmlpp::Element *>(node);
		xmlpp::Attribute *attribute=dynamic_cast<xmlpp::Attribute *>(node);
		if(attribute)
			element=attribute->get_parent();
		if(element)
			element->set_namespace_declaration(uri);
	}

	return prefix;
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
		    std::map<const ASObject*, uint32_t>& objMap,
		    std::map<const Class_base*, uint32_t>& traitsMap)
{
	out->writeByte(xml_marker);
	out->writeXMLString(objMap, this, toString());
}

_NR<XML> XML::getRootNode()
{
	if(root.isNull())
	{
		incRef();
		return _MR(this);
	}
	else
		return root;
}
