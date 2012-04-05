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

#include "XML.h"
#include "XMLList.h"
#include "flash/xml/flashxml.h"
#include "class.h"
#include "compat.h"
#include "argconv.h"
#include <libxml/tree.h>
#include <libxml++/parsers/domparser.h>
#include <libxml++/nodes/textnode.h>

using namespace std;
using namespace lightspark;

SET_NAMESPACE("");
REGISTER_CLASS_NAME(XML);

XML::XML():node(NULL),constructed(false),ignoreComments(true)
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
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(valueOf),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(valueOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toXMLString",AS3,Class<IFunction>::getFunction(toXMLString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("nodeKind",AS3,Class<IFunction>::getFunction(nodeKind),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("child",AS3,Class<IFunction>::getFunction(child),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("children",AS3,Class<IFunction>::getFunction(children),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attribute",AS3,Class<IFunction>::getFunction(attribute),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes",AS3,Class<IFunction>::getFunction(attributes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("localName",AS3,Class<IFunction>::getFunction(localName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("name",AS3,Class<IFunction>::getFunction(name),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descendants",AS3,Class<IFunction>::getFunction(descendants),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendChild",AS3,Class<IFunction>::getFunction(appendChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasSimpleContent",AS3,Class<IFunction>::getFunction(_hasSimpleContent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasComplexContent",AS3,Class<IFunction>::getFunction(_hasComplexContent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("text",AS3,Class<IFunction>::getFunction(text),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("elements",AS3,Class<IFunction>::getFunction(elements),NORMAL_METHOD,true);
}

#ifdef XMLPP_2_35_1
XML::RecoveryDocument::RecoveryDocument(_xmlDoc* d):xmlpp::Document(d)
{
}

void XML::RecoveryDomParser::parse_memory_raw(const unsigned char* contents, size_type bytes_count)
{
	release_underlying(); //Free any existing document.

	//The following is based on the implementation of xmlParseFile(), in xmlSAXParseFileWithData():
	context_ = xmlCreateMemoryParserCtxt((const char*)contents, bytes_count);
	if(!context_)
		throw xmlpp::internal_error("Couldn't create parsing context");

	xmlSAXHandlerV1* handler=(xmlSAXHandlerV1*)calloc(1,sizeof(xmlSAXHandlerV1));
	initxmlDefaultSAXHandler(handler, 0);
	context_->recovery=1;
	free(context_->sax);
	context_->sax=(xmlSAXHandler*)handler;
	context_->keepBlanks = 0;
	handler->ignorableWhitespace = xmlSAX2IgnorableWhitespace;

	//The following is based on the implementation of xmlParseFile(), in xmlSAXParseFileWithData():
	//and the implementation of xmlParseMemory(), in xmlSaxParseMemoryWithData().
	initialize_context();

	if(!context_)
		throw xmlpp::internal_error("Context not initialized");

	xmlParseDocument(context_);

	check_for_exception();

	if(!context_->wellFormed)
		LOG(LOG_ERROR, "XML data not well formed!");

	doc_ = new RecoveryDocument(context_->myDoc);
	// This is to indicate to release_underlying that we took the
	// ownership on the doc.
	context_->myDoc = 0;

	//Free the parse context, but keep the document alive so people can navigate the DOM tree:
	//TODO: Why not keep the context alive too?
	Parser::release_underlying();

	check_for_exception();
}

#endif

void XML::buildFromString(const string& str)
{
	string buf = parserQuirks(str);
	try
	{
		parser.parse_memory_raw((const unsigned char*)buf.c_str(), buf.size());
	}
	catch(const exception& e)
	{
	}
	xmlpp::Document* doc=parser.get_document();
	if(doc)
		node=doc->get_root_node();

	if(node==NULL)
	{
		LOG(LOG_ERROR, "XML parsing failed, creating text node");
		//If everything fails, create a fake document and add a single text string child
		buf="<a></a>";
		parser.parse_memory_raw((const unsigned char*)buf.c_str(), buf.size());
		node=parser.get_document()->get_root_node()->add_child_text(str);
		// TODO: node's parent (root) should be inaccessible from AS code
	}
}

// Adobe player's XML parser accepts many strings which are not valid
// XML according to the specs. This function attempts to massage
// invalid-but-accepted-by-Adobe strings into valid XML so that
// libxml++ parser doesn't throw an error.
string XML::parserQuirks(const string& str)
{
	string buf = quirkCData(str);
	buf = quirkXMLDeclarationInMiddle(buf);
	return buf;
}

string XML::quirkCData(const string& str) {
	//if this is a CDATA node replace CDATA tags to make it look like a text-node
	//for compatibility with the Adobe player
	if (str.compare(0, 9, "<![CDATA[") == 0) {
		return "<a>"+str.substr(9, str.size()-12)+"</a>";
	}
	else
		return str;
}

string XML::quirkXMLDeclarationInMiddle(const string& str) {
	string buf(str);

	// Adobe player ignores XML declarations in the middle of a
	// string.
	while (true)
	{
		size_t start = buf.find("<?xml ", 1);
		if (start == buf.npos)
			break;
		
		size_t end = buf.find("?>", start+5);
		if (end == buf.npos)
			break;
		end += 2;
		
		buf.erase(start, end-start);
	}

	return buf;
}

ASFUNCTIONBODY(XML,generator)
{
	assert(obj==NULL);
	assert_and_throw(argslen<=1);
	if (argslen == 0)
	{
		return Class<XML>::getInstanceS("");
	}
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
	else if(args[0]->getClass()==Class<XMLList>::getClass())
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
	else if(args[0]->getClass()->isSubClass(Class<ByteArray>::getClass()))
	{
		//Official documentation says that generic Objects are not supported.
		//ByteArray seems to be though (see XML test) so let's support it
		ByteArray* ba=Class<ByteArray>::cast(args[0]);
		uint32_t len=ba->getLength();
		const uint8_t* str=ba->getBuffer(len, false);
		th->buildFromString(std::string((const char*)str,len));
	}
	else if(args[0]->getObjectType()==T_STRING ||
		args[0]->getObjectType()==T_NUMBER ||
		args[0]->getObjectType()==T_INTEGER ||
		args[0]->getObjectType()==T_BOOLEAN)
	{
		//By specs, XML constructor will only convert to string Numbers or Booleans
		//ints are not explicitly mentioned, but they seem to work
		th->buildFromString(args[0]->toString());
	}
	else
		throw Class<TypeError>::getInstanceS("Unsupported type in XML conversion");
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
	_NR<XML> arg;
	if(args[0]->getClass()==Class<XML>::getClass())
	{
		args[0]->incRef();
		arg=_MR(Class<XML>::cast(args[0]));
	}
	else if(args[0]->getClass()==Class<XMLList>::getClass())
	{
		XMLList* list=Class<XMLList>::cast(args[0]);
		arg=list->convertToXML();
		assert_and_throw(!arg.isNull());
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
	_NR<XML> rootXML=NullRef;
	if(th->root.isNull())
	{
		th->incRef();
		rootXML=_MR(th);
	}
	else
		rootXML=th->root;

	std::vector<_R<XML>> ret;
	ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, attribute)));
	return Class<XMLList>::getInstanceS(ret);
}

ASFUNCTIONBODY(XML,attributes)
{
	assert_and_throw(argslen==0);
	return obj->as<XML>()->getAllAttributes();
}

XMLList* XML::getAllAttributes()
{
	assert(node);
	//Needed dynamic cast, we want the type check
	xmlpp::Element* elem=dynamic_cast<xmlpp::Element*>(node);
	if(elem==NULL)
		return Class<XMLList>::getInstanceS();
	const xmlpp::Element::AttributeList& list=elem->get_attributes();
	xmlpp::Element::AttributeList::const_iterator it=list.begin();
	std::vector<_R<XML>> ret;
	_NR<XML> rootXML=NullRef;
	if(root.isNull())
	{
		this->incRef();
		rootXML=_MR(this);
	}
	else
		rootXML=this->root;

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

void XML::childrenImpl(std::vector<_R<XML> >& ret, const tiny_string& name)
{
	assert(node);
	const xmlpp::Node::NodeList& list=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=list.begin();

	_NR<XML> rootXML=NullRef;
	if(root.isNull())
	{
		this->incRef();
		rootXML=_MR(this);
	}
	else
		rootXML=root;

	for(;it!=list.end();it++)
	{
		if(name!="*" && (*it)->get_name() != name.raw_buf())
			continue;
		ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));
	}
}

ASFUNCTIONBODY(XML,child)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==1);
	const tiny_string& arg0=args[0]->toString();
	std::vector<_R<XML>> ret;
	th->childrenImpl(ret, arg0);
	XMLList* retObj=Class<XMLList>::getInstanceS(ret);
	return retObj;
}

ASFUNCTIONBODY(XML,children)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	std::vector<_R<XML>> ret;
	th->childrenImpl(ret, "*");
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

ASFUNCTIONBODY(XML,valueOf)
{
	obj->incRef();
	return obj;
}

void XML::getText(vector<_R<XML>> &ret)
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
	vector<_R<XML>> ret;
	th->getText(ret);
	return Class<XMLList>::getInstanceS(ret);
}

ASFUNCTIONBODY(XML,elements)
{
	vector<_R<XML>> ret;
	XML *th = obj->as<XML>();
	tiny_string name;
	ARG_UNPACK (name, "");
	if (name=="*")
		name="";

	_NR<XML> rootXML=NullRef;
	if(th->root.isNull())
	{
		th->incRef();
		rootXML=_MR(th);
	}
	else
		rootXML=th->root;

	const xmlpp::Node::NodeList& children=th->node->get_children();
	xmlpp::Node::NodeList::const_iterator it=children.begin();
	for(;it!=children.end();++it)
	{
		xmlElementType nodetype=(*it)->cobj()->type;
		if(nodetype==XML_ELEMENT_NODE && (name.empty() || name == (*it)->get_name()))
			ret.push_back(_MR(Class<XML>::getInstanceS(rootXML, *it)));
	}
	return Class<XMLList>::getInstanceS(ret);
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
		ret.push_back(_MR(Class<XML>::getInstanceS(root, node)));
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
		//TODO: support namespaces
		assert_and_throw(name.ns.size()>0 && name.ns[0].name=="");

		if(normalizedName.empty())
			return _MR(getAllAttributes());

		//Normalize the name to the string form
		assert(node);
		//To have attributes we must be an Element
		xmlpp::Element* element=dynamic_cast<xmlpp::Element*>(node);
		if(element==NULL)
			return _MNR(Class<XMLList>::getInstanceS());
		xmlpp::Attribute* attr=element->get_attribute(buf);
		if(attr==NULL)
			return _MNR(Class<XMLList>::getInstanceS());

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
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	throw UnsupportedException("XML::serialize not implemented");
}
