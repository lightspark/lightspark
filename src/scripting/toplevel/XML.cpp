/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include <libxml++/nodes/entityreference.h>

using namespace std;
using namespace lightspark;

static bool ignoreComments;
static bool ignoreProcessingInstructions;
static bool ignoreWhitespace;
static int32_t prettyIndent;
static bool prettyPrinting;

void setDefaultXMLSettings()
{
	ignoreComments = true;
	ignoreProcessingInstructions = true;
	ignoreWhitespace = true;
	prettyIndent = 2;
	prettyPrinting = true;
}

XML::XML(Class_base* c):ASObject(c),parentNode(0),nodetype((xmlElementType)0),constructed(false), hasParentNode(false)
{
}

XML::XML(Class_base* c, const std::string &str):ASObject(c),parentNode(0),nodetype((xmlElementType)0),constructed(false)
{
	createTree(buildFromString(str, false,&hasParentNode));
}

XML::XML(Class_base* c,xmlpp::Node* _n):ASObject(c),parentNode(0),nodetype((xmlElementType)0),constructed(false)
{
	createTree(_n);
}

void XML::finalize()
{
	ASObject::finalize();
}

void XML::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	setDefaultXMLSettings();

	c->setDeclaredMethodByQName("ignoreComments","",Class<IFunction>::getFunction(_getIgnoreComments),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("ignoreComments","",Class<IFunction>::getFunction(_setIgnoreComments),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("ignoreProcessingInstructions","",Class<IFunction>::getFunction(_getIgnoreProcessingInstructions),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("ignoreProcessingInstructions","",Class<IFunction>::getFunction(_setIgnoreProcessingInstructions),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("ignoreWhitespace","",Class<IFunction>::getFunction(_getIgnoreWhitespace),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("ignoreWhitespace","",Class<IFunction>::getFunction(_setIgnoreWhitespace),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("prettyIndent","",Class<IFunction>::getFunction(_getPrettyIndent),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("prettyIndent","",Class<IFunction>::getFunction(_setPrettyIndent),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("prettyPrinting","",Class<IFunction>::getFunction(_getPrettyPrinting),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("prettyPrinting","",Class<IFunction>::getFunction(_setPrettyPrinting),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("settings","",Class<IFunction>::getFunction(_getSettings),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("setSettings","",Class<IFunction>::getFunction(_setSettings),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("defaultSettings","",Class<IFunction>::getFunction(_getDefaultSettings),NORMAL_METHOD,false);

	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(valueOf),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(valueOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toXMLString",AS3,Class<IFunction>::getFunction(toXMLString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("nodeKind","",Class<IFunction>::getFunction(nodeKind),DYNAMIC_TRAIT);	c->setDeclaredMethodByQName("nodeKind",AS3,Class<IFunction>::getFunction(nodeKind),NORMAL_METHOD,true);
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
	c->setDeclaredMethodByQName("normalize",AS3,Class<IFunction>::getFunction(_normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descendants",AS3,Class<IFunction>::getFunction(descendants),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendChild",AS3,Class<IFunction>::getFunction(_appendChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parent",AS3,Class<IFunction>::getFunction(parent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inScopeNamespaces",AS3,Class<IFunction>::getFunction(inScopeNamespaces),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addNamespace",AS3,Class<IFunction>::getFunction(addNamespace),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("hasSimpleContent",AS3,Class<IFunction>::getFunction(_hasSimpleContent),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("hasComplexContent",AS3,Class<IFunction>::getFunction(_hasComplexContent),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("text",AS3,Class<IFunction>::getFunction(text),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("elements",AS3,Class<IFunction>::getFunction(elements),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setLocalName",AS3,Class<IFunction>::getFunction(_setLocalName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setName",AS3,Class<IFunction>::getFunction(_setName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setNamespace",AS3,Class<IFunction>::getFunction(_setNamespace),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("copy",AS3,Class<IFunction>::getFunction(_copy),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("setChildren",AS3,Class<IFunction>::getFunction(_setChildren),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toJSON",AS3,Class<IFunction>::getFunction(_toJSON),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("insertChildAfter",AS3,Class<IFunction>::getFunction(insertChildAfter),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("insertChildBefore",AS3,Class<IFunction>::getFunction(insertChildBefore),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("namespaceDeclarations",AS3,Class<IFunction>::getFunction(namespaceDeclarations),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeNamespace",AS3,Class<IFunction>::getFunction(removeNamespace),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("comments",AS3,Class<IFunction>::getFunction(comments),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("processingInstructions",AS3,Class<IFunction>::getFunction(processingInstructions),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("propertyIsEnumerable",AS3,Class<IFunction>::getFunction(_propertyIsEnumerable),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("hasOwnProperty",AS3,Class<IFunction>::getFunction(_hasOwnProperty),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("prependChild",AS3,Class<IFunction>::getFunction(_prependChild),NORMAL_METHOD,true);
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
		th->createTree(th->buildFromString("", false,&th->hasParentNode));
	}
	else if(args[0]->getClass()->isSubClass(Class<ByteArray>::getClass()))
	{
		//Official documentation says that generic Objects are not supported.
		//ByteArray seems to be though (see XML test) so let's support it
		ByteArray* ba=Class<ByteArray>::cast(args[0]);
		uint32_t len=ba->getLength();
		const uint8_t* str=ba->getBuffer(len, false);
		th->createTree(th->buildFromString(std::string((const char*)str,len), false,&th->hasParentNode,
					     getVm()->getDefaultXMLNamespace()));
	}
	else if(args[0]->is<ASString>() ||
		args[0]->is<Number>() ||
		args[0]->is<Integer>() ||
		args[0]->is<UInteger>() ||
		args[0]->is<Boolean>())
	{
		//By specs, XML constructor will only convert to string Numbers or Booleans
		//ints are not explicitly mentioned, but they seem to work
		th->createTree(th->buildFromString(args[0]->toString(), false,&th->hasParentNode,
					     getVm()->getDefaultXMLNamespace()));
	}
	else if(args[0]->is<XML>())
	{
		th->createTree(th->buildFromString(args[0]->as<XML>()->toXMLString_internal(), false,&th->hasParentNode,
					     getVm()->getDefaultXMLNamespace()));
	}
	else if(args[0]->is<XMLList>())
	{
		XMLList *list=args[0]->as<XMLList>();
		_R<XML> reduced=list->reduceToXML();
		th->createTree(th->buildFromString(reduced->toXMLString_internal(), false,&th->hasParentNode));
	}
	else
	{
		th->createTree(th->buildFromString(args[0]->toString(), false,&th->hasParentNode,
					     getVm()->getDefaultXMLNamespace()));
	}
	return NULL;
}

ASFUNCTIONBODY(XML,nodeKind)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	return Class<ASString>::getInstanceS(th->nodekindString());
}
const char *XML::nodekindString()
{
	switch(nodetype)
	{
		case XML_ATTRIBUTE_NODE:
			return "attribute";
		case XML_ELEMENT_NODE:
			return "element";
		case XML_CDATA_SECTION_NODE:
		case XML_TEXT_NODE:
			return "text";
		case XML_COMMENT_NODE:
			return "comment";
		case XML_PI_NODE:
			return "processing-instruction";
		default:
		{
			LOG(LOG_ERROR,"Unsupported XML type " << nodetype);
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
	if(th->nodetype==XML_TEXT_NODE || th->nodetype==XML_COMMENT_NODE)
		return getSys()->getNullRef();
	else
		return Class<ASString>::getInstanceS(th->nodename);
}

ASFUNCTIONBODY(XML,name)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	//TODO: add namespace
	if(th->nodetype==XML_TEXT_NODE || th->nodetype==XML_COMMENT_NODE)
		return getSys()->getNullRef();
	else
	{
		ASQName* ret = Class<ASQName>::getInstanceS();
		ret->setByXML(th);
		return ret;
	}
}

ASFUNCTIONBODY(XML,descendants)
{
	XML* th=Class<XML>::cast(obj);
	_NR<ASObject> name;
	ARG_UNPACK(name,_NR<ASObject>(Class<ASString>::getInstanceS("*")));
	XMLVector ret;
	multiname mname(NULL);
	name->applyProxyProperty(mname);
	th->getDescendantsByQName(name->toString(),"",mname.isAttribute,ret);
	return Class<XMLList>::getInstanceS(ret,th->getChildrenlist(),multiname(NULL));
}

ASFUNCTIONBODY(XML,_appendChild)
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

	th->appendChild(arg);
	th->incRef();
	return th;
}

void XML::appendChild(_R<XML> newChild)
{
	if (newChild->constructed)
	{
		this->incRef();
		newChild->parentNode = _NR<XML>(this);
		childrenlist->append(newChild);
	}
	else
		newChild->decRef();
}

/* returns the named attribute in an XMLList */
ASFUNCTIONBODY(XML,attribute)
{
	XML* th = obj->as<XML>();
	tiny_string attrname;
	//see spec for QName handling
	ARG_UNPACK (attrname);
	tiny_string tmpns;
	if(argslen > 0 && args[0]->is<ASQName>())
	{
		tmpns= args[0]->as<ASQName>()->getURI();
		attrname = args[0]->as<ASQName>()->getLocalName();
			
	}

	XMLVector tmp;
	XMLList* res = Class<XMLList>::getInstanceS(tmp,th->getChildrenlist(),multiname(NULL));
	for (XMLList::XMLListVector::const_iterator it = th->attributelist->nodes.begin(); it != th->attributelist->nodes.end(); it++)
	{
		_R<XML> attr = *it;
		if (attr->nodenamespace_uri == tmpns && (attrname== "*" || attr->nodename == attrname))
		{
			attr->incRef();
			res->append(attr);
		}
	}
	return res;
}

ASFUNCTIONBODY(XML,attributes)
{
	assert_and_throw(argslen==0);
	return obj->as<XML>()->getAllAttributes();
}

XMLList* XML::getAllAttributes()
{
	attributelist->incRef();
	return attributelist.getPtr();
}

const tiny_string XML::toXMLString_internal(bool pretty, tiny_string defaultnsprefix, const char *indent,bool bfirst)
{
	tiny_string res;
	set<tiny_string> seen_prefix;

	if (bfirst)
	{
		tiny_string defns = getVm()->getDefaultXMLNamespace();
		XML* tmp = this;
		bool bfound = false;
		while(tmp)
		{
			for (uint32_t j = 0; j < tmp->namespacedefs.size(); j++)
			{
				bool b;
				_R<Namespace> tmpns = tmp->namespacedefs[j];
				if (tmpns->getURI() == defns)
				{
					defaultnsprefix = tmpns->getPrefix(b);
					bfound = true;
					break;
				}
			}
			if (!bfound && tmp->parentNode)
				tmp = tmp->parentNode.getPtr();
			else
				break;
		}
	}
	switch (nodetype)
	{
		case XML_TEXT_NODE:
			res = indent;
			res += encodeToXML(nodevalue,false);
			break;
		case XML_ATTRIBUTE_NODE:
			res += nodevalue;
			break;
		case XML_COMMENT_NODE:
			res = indent;
			res += "<!--";
			res += nodevalue;
			res += "-->";
			break;
		case XML_PI_NODE:
			if (ignoreProcessingInstructions)
				break;
			res = indent;
			res += "<?";
			res +=this->nodename;
			res += " ";
			res += nodevalue;
			res += "?>";
			break;
		case XML_ELEMENT_NODE:
		{
			tiny_string curprefix = this->nodenamespace_prefix;
			res = indent;
			res += "<";
			if (this->nodenamespace_prefix.empty())
			{
				if (defaultnsprefix != "")
				{
					res += defaultnsprefix;
					res += ":";
					curprefix = defaultnsprefix;
				}
			}
			else
			{
				res += this->nodenamespace_prefix;
				res += ":";
			}
			res +=this->nodename;
			for (uint32_t i = 0; i < this->namespacedefs.size(); i++)
			{
				bool b;
				_R<Namespace> tmpns = this->namespacedefs[i];
				tiny_string tmpprefix = tmpns->getPrefix(b);
				if(tmpprefix == "" || tmpprefix==this->nodenamespace_prefix || seen_prefix.find(tmpprefix)!=seen_prefix.end())
					continue;
				seen_prefix.insert(tmpprefix);
				res += " xmlns:";
				res += tmpprefix;
				res += "=\"";
				res += tmpns->getURI();
				res += "\"";
			}
			if (this->parentNode)
			{
				if (bfirst)
				{
					XML* tmp = this->parentNode.getPtr();
					while(tmp)
					{
						for (uint32_t i = 0; i < tmp->namespacedefs.size(); i++)
						{
							bool b;
							_R<Namespace> tmpns = tmp->namespacedefs[i];
							tiny_string tmpprefix = tmpns->getPrefix(b);
							if(tmpprefix != "" && seen_prefix.find(tmpprefix)==seen_prefix.end())
							{
								seen_prefix.insert(tmpprefix);
								res += " xmlns:";
								res += tmpprefix;
								res += "=\"";
								res += tmpns->getURI();
								res += "\"";
							}
						}
						if (tmp->parentNode)
							tmp = tmp->parentNode.getPtr();
						else
							break;
					}
				}
				else if (!curprefix.empty())
				{
					XML* tmp = this->parentNode.getPtr();
					bool bfound = false;
					while(tmp)
					{
						for (uint32_t i = 0; i < tmp->namespacedefs.size(); i++)
						{
							bool b;
							_R<Namespace> tmpns = tmp->namespacedefs[i];
							tiny_string tmpprefix = tmpns->getPrefix(b);
							if(tmpprefix == curprefix)
							{
								seen_prefix.insert(tmpprefix);
								bfound = true;
								break;
							}
						}
						if (!bfound && tmp->parentNode)
							tmp = tmp->parentNode.getPtr();
						else
							break;
					}
				}
			}
			if (!this->nodenamespace_uri.empty() && 
					((this->nodenamespace_prefix.empty() && defaultnsprefix == "") ||
					 (!this->nodenamespace_prefix.empty() && seen_prefix.find(this->nodenamespace_prefix)==seen_prefix.end())))
			{
				if (!this->nodenamespace_prefix.empty())
				{
					seen_prefix.insert(this->nodenamespace_prefix);
					res += " xmlns:";
					res += this->nodenamespace_prefix;
				}
				else
					res += " xmlns";
				res += "=\"";
				res += this->nodenamespace_uri;
				res += "\"";
			}
			else if (defaultnsprefix != "" && seen_prefix.find(defaultnsprefix)==seen_prefix.end())
			{
				seen_prefix.insert(defaultnsprefix);
				res += " xmlns:";
				res += defaultnsprefix;
				res += "=\"";
				res += getVm()->getDefaultXMLNamespace();
				res += "\"";
			}
			for (XMLList::XMLListVector::const_iterator it = attributelist->nodes.begin(); it != attributelist->nodes.end(); it++)
			{
				_R<XML> attr = *it;
				res += " ";
				if (attr->nodenamespace_prefix != "")
				{
					res += attr->nodenamespace_prefix;
					res += ":";
				}
				res += attr->nodename;
				res += "=\"";
				res += encodeToXML(attr->nodevalue,true);
				res += "\"";
			}
			if (childrenlist->nodes.size() == 0)
			{
				res += "/>";
				break;
			}
			res += ">";
			tiny_string newindent;
			bool bindent = (pretty && prettyPrinting && prettyIndent >=0 && 
							(childrenlist->nodes.size() >1 || 
							(childrenlist->nodes[0]->nodetype != XML_TEXT_NODE && childrenlist->nodes[0]->nodetype != XML_CDATA_SECTION_NODE)));
			if (bindent)
			{
				newindent = indent;
				for (int32_t j = 0; j < prettyIndent; j++)
				{
					newindent += " ";
				}
			}
			for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
			{
				if (bindent)
					res += "\n";
				_R<XML> child= childrenlist->nodes[i];
				res += child->toXMLString_internal(pretty,defaultnsprefix,newindent.raw_buf(),false);
			}
			if (bindent)
			{
				res += "\n";
				res += indent;
			}
			res += "</";
			if (this->nodenamespace_prefix.empty())
			{
				if (defaultnsprefix != "")
				{
					res += defaultnsprefix;
					res += ":";
				}
			}
			else
			{
				res += this->nodenamespace_prefix;
				res += ":";
			}
			res += this->nodename;
			res += ">";
			break;
		}
		case XML_CDATA_SECTION_NODE:
			res += "<![CDATA[";
			res += nodevalue;
			res += "]]>";
			break;
		default:
			LOG(LOG_NOT_IMPLEMENTED,"XML::toXMLString unhandled nodetype:"<<nodetype);
			break;
	}
	return res;
}

ASFUNCTIONBODY(XML,toXMLString)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	tiny_string res = th->toXMLString_internal();
	ASString* ret=Class<ASString>::getInstanceS(res);
	return ret;
}

void XML::childrenImpl(XMLVector& ret, const tiny_string& name)
{
	for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
	{
		_R<XML> child= childrenlist->nodes[i];
		if(name!="*" && child->nodename != name)
			continue;
		child->incRef();
		ret.push_back(child);
	}
}

void XML::childrenImpl(XMLVector& ret, uint32_t index)
{
	if (constructed && index < childrenlist->nodes.size())
	{
		_R<XML> child= childrenlist->nodes[index];
		child->incRef();
		ret.push_back(child);
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
	if(XML::isValidMultiname(mname, index))
		th->childrenImpl(ret, index);
	else
		th->childrenImpl(ret, arg0);
	XMLList* retObj=Class<XMLList>::getInstanceS(ret,th->getChildrenlist(),mname);
	return retObj;
}

ASFUNCTIONBODY(XML,children)
{
	XML* th=Class<XML>::cast(obj);
	assert_and_throw(argslen==0);
	XMLVector ret;
	th->childrenImpl(ret, "*");
	multiname mname(NULL);
	mname.name_s_id=getSys()->getUniqueStringId("*");
	mname.name_type=multiname::NAME_STRING;
	mname.ns.push_back(nsNameAndKind("",NAMESPACE));
	XMLList* retObj=Class<XMLList>::getInstanceS(ret,th->getChildrenlist(),mname);
	return retObj;
}

ASFUNCTIONBODY(XML,childIndex)
{
	XML* th=Class<XML>::cast(obj);
	if (th->parentNode)
	{
		XML* parent = th->parentNode.getPtr();
		for (uint32_t i = 0; i < parent->childrenlist->nodes.size(); i++)
		{
			ASObject* o= parent->childrenlist->nodes[i].getPtr();
			if (o == th)
				return abstract_i(i);
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
	for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
	{
		_R<XML> child= childrenlist->nodes[i];
		if (child->getNodeKind() == XML_TEXT_NODE  ||
			child->getNodeKind() == XML_CDATA_SECTION_NODE)
		{
			child->incRef();
			ret.push_back( child );
		}
	}
}

ASFUNCTIONBODY(XML,text)
{
	XML *th = obj->as<XML>();
	ARG_UNPACK;
	XMLVector ret;
	th->getText(ret);
	return Class<XMLList>::getInstanceS(ret,th->getChildrenlist(),multiname(NULL));
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
	return Class<XMLList>::getInstanceS(ret,th->getChildrenlist(),multiname(NULL));
}

void XML::getElementNodes(const tiny_string& name, XMLVector& foundElements)
{
	for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
	{
		_R<XML> child= childrenlist->nodes[i];
		if(child->nodetype==XML_ELEMENT_NODE && (name.empty() || name == child->nodename))
		{
			child->incRef();
			foundElements.push_back( child );
		}
	}
}

ASFUNCTIONBODY(XML,inScopeNamespaces)
{
	XML *th = obj->as<XML>();
	Array *namespaces = Class<Array>::getInstanceS();
	set<tiny_string> seen_prefix;

	XML* tmp = th;
	while(tmp)
	{
		for (uint32_t i = 0; i < tmp->namespacedefs.size(); i++)
		{
			bool b;
			_R<Namespace> tmpns = tmp->namespacedefs[i];
			if(seen_prefix.find(tmpns->getPrefix(b))==seen_prefix.end())
			{
				tmpns->incRef();
				namespaces->push(tmpns);
				seen_prefix.insert(tmp->nodenamespace_prefix);
			}
		}
		if (tmp->parentNode)
			tmp = tmp->parentNode.getPtr();
		else
			break;
	}
	return namespaces;
}

ASFUNCTIONBODY(XML,addNamespace)
{
	XML *th = obj->as<XML>();
	_NR<ASObject> newNamespace;
	ARG_UNPACK(newNamespace);


	tiny_string ns_uri;
	tiny_string ns_prefix;
	if (newNamespace->is<Namespace>())
	{
		Namespace* tmpns = newNamespace->as<Namespace>();
		bool b;
		ns_prefix = tmpns->getPrefix(b);
		ns_uri = tmpns->getURI();
	}
	else if (newNamespace->is<ASQName>())
	{
		ns_uri = newNamespace->as<ASQName>()->getURI();
	}
	else
		ns_uri = newNamespace->toString();
	if (th->nodenamespace_prefix == ns_prefix)
		th->nodenamespace_prefix="";
	for (uint32_t i = 0; i < th->namespacedefs.size(); i++)
	{
		bool b;
		_R<Namespace> tmpns = th->namespacedefs[i];
		if (tmpns->getPrefix(b) == ns_prefix)
		{
			th->namespacedefs[i] = _R<Namespace>(Class<Namespace>::getInstanceS(ns_uri,ns_prefix));
			return NULL;
		}
	}
	th->namespacedefs.push_back(_R<Namespace>(Class<Namespace>::getInstanceS(ns_uri,ns_prefix)));
	return NULL;
}

ASObject *XML::getParentNode()
{
	if (parentNode && parentNode->is<XML>())
	{
		parentNode->incRef();
		return parentNode.getPtr();
	}
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

	xmlElementType nodetype=th->nodetype;
	if(prefix.empty() && 
	   nodetype!=XML_ELEMENT_NODE && 
	   nodetype!=XML_ATTRIBUTE_NODE)
	{
		return getSys()->getNullRef();
	}
	if (prefix.empty())
		return Class<Namespace>::getInstanceS(th->nodenamespace_uri, th->nodenamespace_prefix);
		
	for (uint32_t i = 0; i < th->namespacedefs.size(); i++)
	{
		bool b;
		_R<Namespace> tmpns = th->namespacedefs[i];
		if (tmpns->getPrefix(b) == prefix)
		{
			return Class<Namespace>::getInstanceS(tmpns->getURI(), prefix);
		}
	}
	return getSys()->getUndefinedRef();
}

ASFUNCTIONBODY(XML,_setLocalName)
{
	XML *th = obj->as<XML>();
	_NR<ASObject> newName;
	ARG_UNPACK(newName);

	xmlElementType nodetype=th->nodetype;
	if(nodetype==XML_TEXT_NODE || nodetype==XML_COMMENT_NODE)
		return NULL;

	tiny_string new_name;
	if(newName->is<ASQName>())
	{
		new_name=newName->as<ASQName>()->getLocalName();
	}
	else
	{
		new_name=newName->toString();
	}

	th->setLocalName(new_name);

	return NULL;
}

void XML::setLocalName(const tiny_string& new_name)
{
	if(!isXMLName(Class<ASString>::getInstanceS(new_name)))
	{
		throwError<TypeError>(kXMLInvalidName, new_name);
	}
	this->nodename = new_name;
}

ASFUNCTIONBODY(XML,_setName)
{
	XML *th = obj->as<XML>();
	_NR<ASObject> newName;
	ARG_UNPACK(newName);

	if(th->nodetype==XML_TEXT_NODE || th->nodetype==XML_COMMENT_NODE)
		return NULL;

	tiny_string localname;
	tiny_string ns_uri;
	tiny_string ns_prefix = th->nodenamespace_prefix;
	if(newName->is<ASQName>())
	{
		ASQName *qname=newName->as<ASQName>();
		localname=qname->getLocalName();
		ns_uri=qname->getURI();
	}
	else if (!newName->is<Undefined>())
	{
		localname=newName->toString();
		ns_prefix= "";
	}

	th->setLocalName(localname);
	th->setNamespace(ns_uri,ns_prefix);

	return NULL;
}

ASFUNCTIONBODY(XML,_setNamespace)
{
	XML *th = obj->as<XML>();
	_NR<ASObject> newNamespace;
	ARG_UNPACK(newNamespace);

	if(th->nodetype==XML_TEXT_NODE ||
	   th->nodetype==XML_COMMENT_NODE ||
	   th->nodetype==XML_PI_NODE)
		return NULL;
	tiny_string ns_uri;
	tiny_string ns_prefix;
	if(newNamespace->is<Namespace>())
	{
		Namespace *ns=newNamespace->as<Namespace>();
		ns_uri=ns->getURI();
		bool prefix_is_undefined=true;
		ns_prefix=ns->getPrefix(prefix_is_undefined);
	}
	else if(newNamespace->is<ASQName>())
	{
		ASQName *qname=newNamespace->as<ASQName>();
		ns_uri=qname->getURI();
		for (uint32_t i = 0; i < th->namespacedefs.size(); i++)
		{
			bool b;
			_R<Namespace> tmpns = th->namespacedefs[i];
			if (tmpns->getURI() == ns_uri)
			{
				ns_prefix = tmpns->getPrefix(b);
				break;
			}
		}
	}
	else if (!newNamespace->is<Undefined>())
	{
		ns_uri=newNamespace->toString();
		for (uint32_t i = 0; i < th->namespacedefs.size(); i++)
		{
			bool b;
			_R<Namespace> tmpns = th->namespacedefs[i];
			if (tmpns->getURI() == ns_uri)
			{
				ns_prefix = tmpns->getPrefix(b);
				break;
			}
		}
	}
	th->setNamespace(ns_uri, ns_prefix);
	if (th->nodetype==XML_ATTRIBUTE_NODE && th->parentNode)
	{
		XML* tmp = th->parentNode.getPtr();
		for (uint32_t i = 0; i < tmp->namespacedefs.size(); i++)
		{
			bool b;
			_R<Namespace> tmpns = tmp->namespacedefs[i];
			if (tmpns->getPrefix(b) == ns_prefix)
			{
				tmp->namespacedefs[i] = _R<Namespace>(Class<Namespace>::getInstanceS(ns_uri,ns_prefix));
				return NULL;
			}
		}
		tmp->namespacedefs.push_back(_R<Namespace>(Class<Namespace>::getInstanceS(ns_uri,ns_prefix)));
	}
	
	return NULL;
}

void XML::setNamespace(const tiny_string& ns_uri, const tiny_string& ns_prefix)
{
	this->nodenamespace_prefix = ns_prefix;
	this->nodenamespace_uri = ns_uri;
}

ASFUNCTIONBODY(XML,_copy)
{
	XML* th=Class<XML>::cast(obj);
	return th->copy();
}

XML *XML::copy()
{
	return Class<XML>::getInstanceS(this->toXMLString_internal(false));
}

ASFUNCTIONBODY(XML,_setChildren)
{
	XML* th=obj->as<XML>();
	_NR<ASObject> newChildren;
	ARG_UNPACK(newChildren);

	th->childrenlist->clear();

	if (newChildren->is<XML>())
	{
		XML *newChildrenXML=newChildren->as<XML>();
		newChildrenXML->incRef();
		th->appendChild(_R<XML>(newChildrenXML));
	}
	else if (newChildren->is<XMLList>())
	{
		XMLList *list=newChildren->as<XMLList>();
		list->incRef();
		list->appendNodesTo(th);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED, "XML::setChildren supports only XMLs and XMLLists");
	}

	th->incRef();
	return th;
}

ASFUNCTIONBODY(XML,_normalize)
{
	XML* th=obj->as<XML>();
	th->normalize();

	th->incRef();
	return th;
}

void XML::normalize()
{
	childrenlist->normalize();
}

void XML::addTextContent(const tiny_string& str)
{
	assert(getNodeKind() == XML_TEXT_NODE);

	nodevalue += str;
}

void XML::setTextContent(const tiny_string& content)
{
	if (getNodeKind() == XML_TEXT_NODE ||
	    getNodeKind() == XML_ATTRIBUTE_NODE ||
	    getNodeKind() == XML_COMMENT_NODE ||
	    getNodeKind() == XML_PI_NODE ||
		getNodeKind() == XML_CDATA_SECTION_NODE)
	{
		nodevalue = content;
	}
}


bool XML::hasSimpleContent() const
{
	if (getNodeKind() == XML_COMMENT_NODE ||
		getNodeKind() == XML_PI_NODE)
		return false;
	for(size_t i=0; i<childrenlist->nodes.size(); i++)
	{
		if (childrenlist->nodes[i]->getNodeKind() == XML_ELEMENT_NODE)
			return false;
	}
	return true;
}

bool XML::hasComplexContent() const
{
	return !hasSimpleContent();
}

xmlElementType XML::getNodeKind() const
{
	return nodetype;
}


void XML::getDescendantsByQName(const tiny_string& name, const tiny_string& ns, bool bIsAttribute, XMLVector& ret)
{
	if (!constructed)
		return;
	if (bIsAttribute)
	{
		for (uint32_t i = 0; i < attributelist->nodes.size(); i++)
		{
			_R<XML> child= attributelist->nodes[i];
			if(name=="" || name=="*" || (name == child->nodename && (ns == "*" || ns == child->nodenamespace_uri)))
			{
				child->incRef();
				ret.push_back(child);
			}
		}
	}
	for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
	{
		_R<XML> child= childrenlist->nodes[i];
		if(!bIsAttribute && (name=="" || name=="*" || (name == child->nodename && (ns == "*" || ns == child->nodenamespace_uri))))
		{
			child->incRef();
			ret.push_back(child);
		}
		child->getDescendantsByQName(name, ns, bIsAttribute, ret);
	}
}

XML::XMLVector XML::getAttributes()
{ 
	multiname mn(NULL);
	mn.name_type=multiname::NAME_STRING;
	mn.ns.push_back(nsNameAndKind("",NAMESPACE));
	mn.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	mn.isAttribute = true;
	return getAttributesByMultiname(mn); 
}
XML::XMLVector XML::getAttributesByMultiname(const multiname& name)
{
	XMLVector ret;
	tiny_string defns = "|";
	defns += getVm()->getDefaultXMLNamespace();
	defns += "|";
	tiny_string normalizedName= "";
	if (!name.isEmpty()) normalizedName= name.normalizedName();
	if (normalizedName.startsWith("@"))
		normalizedName = normalizedName.substr(1,normalizedName.end());
	tiny_string namespace_uri="|";
	uint32_t i = 0;
	while (i < name.ns.size())
	{
		nsNameAndKindImpl ns=name.ns[i].getImpl();
		if (ns.kind==NAMESPACE && ns.name != AS3)
		{
			if (ns.name.empty())
				namespace_uri +=getVm()->getDefaultXMLNamespace();
			else
				namespace_uri +=ns.name;
			namespace_uri += "|";
		}
		i++;
	}
	for (uint32_t i = 0; i < attributelist->nodes.size(); i++)
	{
		_R<XML> child= attributelist->nodes[i];
		tiny_string childnamespace_uri = "|";
		childnamespace_uri += child->nodenamespace_uri;
		childnamespace_uri += "|";
		
		bool bmatch = (
					((normalizedName=="") &&
					 ((namespace_uri.find(defns)!= tiny_string::npos) ||
					  (namespace_uri=="|*|") ||
					  (namespace_uri.find(childnamespace_uri) != tiny_string::npos)
					 )
					)||
					((normalizedName=="*") &&
					 ((namespace_uri.find(defns)!= tiny_string::npos) ||
					  (namespace_uri=="|*|") ||
					  (namespace_uri.find(childnamespace_uri) != tiny_string::npos)
					 )
					)||
					((normalizedName==child->nodename) &&
					 (
					  (namespace_uri=="|*|") ||
					  (namespace_uri=="|" && childnamespace_uri=="||") ||
					  (namespace_uri.find(childnamespace_uri) != tiny_string::npos)
					 )
					)
					);
		if(bmatch)
		{
			child->incRef();
			ret.push_back(child);
		}
	}
	return ret;
}
XML::XMLVector XML::getValuesByMultiname(_NR<XMLList> nodelist, const multiname& name)
{
	XMLVector ret;
	tiny_string defns = "|";
	defns += getVm()->getDefaultXMLNamespace();
	defns += "|";
	tiny_string normalizedName= "";
	if (!name.isEmpty()) normalizedName= name.normalizedName();
	if (normalizedName.startsWith("@"))
		normalizedName = normalizedName.substr(1,normalizedName.end());
	tiny_string namespace_uri="|";
	uint32_t i = 0;
	while (i < name.ns.size())
	{
		nsNameAndKindImpl ns=name.ns[i].getImpl();
		if (ns.kind==NAMESPACE && ns.name != AS3)
		{
			if (ns.name.empty())
				namespace_uri +=getVm()->getDefaultXMLNamespace();
			else
				namespace_uri +=ns.name;
			namespace_uri += "|";
		}
		i++;
	}

	for (uint32_t i = 0; i < nodelist->nodes.size(); i++)
	{
		_R<XML> child= nodelist->nodes[i];
		tiny_string childnamespace_uri = "|";
		childnamespace_uri += child->nodenamespace_uri;
		childnamespace_uri += "|";
		
		bool bmatch = (
					((normalizedName=="") &&
					 ((namespace_uri.find(defns)!= tiny_string::npos) ||
					  (namespace_uri=="|*|") ||
					  (namespace_uri.find(childnamespace_uri) != tiny_string::npos)
					 )
					)||
					((normalizedName=="*") &&
					 ((namespace_uri.find(defns)!= tiny_string::npos) ||
					  (namespace_uri=="|*|") ||
					  (namespace_uri.find(childnamespace_uri) != tiny_string::npos)
					 )
					)||
					((normalizedName==child->nodename) &&
					 ((namespace_uri.find(defns)!= tiny_string::npos) ||
					  (namespace_uri=="|*|") ||
					  (namespace_uri=="|" && childnamespace_uri=="||") ||
					  (namespace_uri.find(childnamespace_uri) != tiny_string::npos)
					 )
					)
					);
		if(bmatch)
		{
			child->incRef();
			ret.push_back(child);
		}
	}
	return ret;
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

	bool isAttr=name.isAttribute;
	unsigned int index=0;

	tiny_string normalizedName=name.normalizedName();
	if(!normalizedName.empty() && normalizedName.charAt(0)=='@')
	{
		isAttr=true;
	}
	if(isAttr)
	{
		//Lookup attribute
		const XMLVector& attributes=getAttributesByMultiname(name);
		return _MNR(Class<XMLList>::getInstanceS(attributes,attributelist.getPtr(),name));
	}
	else if(XML::isValidMultiname(name,index))
	{
		// If the multiname is a valid array property, the XML
		// object is treated as a single-item XMLList.
		if(index==0)
		{
			incRef();
			return _MNR(this);
		}
		else
			return _MNR(getSys()->getUndefinedRef());
	}
	else
	{
		const XMLVector& ret=getValuesByMultiname(childrenlist,name);
		if(ret.empty() && (opt & XML_STRICT)!=0)
			return NullRef;

		_R<XMLList> ch =_MNR(Class<XMLList>::getInstanceS(ret,this->getChildrenlist(),name));
		return ch;
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
		if (ns.kind==NAMESPACE)
		{
			ns_uri=ns.name;
			ns_prefix=getNamespacePrefixByURI(ns_uri);
		}
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
		bool found = false;
		for (XMLList::XMLListVector::iterator it = attributelist->nodes.begin(); it != attributelist->nodes.end(); it++)
		{
			_R<XML> attr = *it;
			if (attr->nodenamespace_uri == ns_uri && (attr->nodename == buf || (*buf==0)))
			{
				if(o->is<XMLList>())
				{
					_NR<XMLList> x = _NR<XMLList>(o->as<XMLList>());
					for (auto it = x->nodes.begin(); it != x->nodes.end(); it++)
					{
						if (!found)
							attr->nodevalue = (*it)->toString();
						else
						{
							attr->nodevalue += " ";
							attr->nodevalue += (*it)->toString();
						}
						found = true;
					}
				}
				else
				{
					if (!found)
						attr->nodevalue = o->toString();
					else
					{
						attr->nodevalue += " ";
						attr->nodevalue += o->toString();
					}
					found = true;
				}
				
			}
		}
		if (!found && !normalizedName.empty())
		{
			_NR<XML> tmp = _MR<XML>(Class<XML>::getInstanceS());
			this->incRef();
			tmp->parentNode = _MR<XML>(this);
			tmp->nodetype = XML_ATTRIBUTE_NODE;
			tmp->nodename = buf;
			tmp->nodenamespace_uri = ns_uri;
			tmp->nodenamespace_prefix = ns_prefix;
			tmp->nodevalue = o->toString();
			tmp->constructed = true;
			attributelist->nodes.push_back(tmp);
		}
	}
	else if(XML::isValidMultiname(name,index))
	{
		childrenlist->setVariableByMultiname(name,o,allowConst);
	}
	else
	{
		bool found = false;
		XMLVector tmpnodes;
		while (!childrenlist->nodes.empty())
		{
			_R<XML> tmpnode = childrenlist->nodes.back();
			if (tmpnode->nodenamespace_uri == ns_uri && tmpnode->nodename == normalizedName)
			{
				if(o->is<XMLList>())
				{
					if (!found)
					{
						_NR<XMLList> x = _NR<XMLList>(Class<XMLList>::getInstanceS(o->as<XMLList>()->toXMLString_internal(false)));
						tmpnodes.insert(tmpnodes.end(), x->nodes.rbegin(),x->nodes.rend());
					}
				}
				else if(o->is<XML>())
				{
					_NR<XML> tmp = _MR<XML>(o->as<XML>());
					tmp->parentNode = _MR<XML>(this);
					tmp->incRef();
					
					if (!found)
						tmpnodes.push_back(tmp);
				}
				else
				{
					if (tmpnode->childrenlist->nodes.size() == 1 && tmpnode->childrenlist->nodes[0]->nodetype == XML_TEXT_NODE)
						tmpnode->childrenlist->nodes[0]->nodevalue = o->toString();
					else
					{
						XML* newnode = Class<XML>::getInstanceS(o->toString());
						tmpnode->childrenlist->clear();
						tmpnode->setVariableByMultiname(name,newnode,allowConst);
					}
					if (!found)
					{
						tmpnode->incRef();
						tmpnodes.push_back(tmpnode);
					}
				}
				found = true;
			}
			else
			{
				tmpnode->incRef();
				tmpnodes.push_back(tmpnode);
			}
			childrenlist->nodes.pop_back();
		}
		if (!found)
		{
			if(o->is<XML>())
			{
				_R<XML> tmp = _MR<XML>(o->as<XML>());
				tmp->parentNode = _MR<XML>(this);
				tmp->incRef();
				tmpnodes.insert(tmpnodes.begin(),tmp);
			}
			else
			{
				tiny_string tmpstr = "<";
				if (!this->nodenamespace_prefix.empty())
				{
					tmpstr += ns_prefix;
					tmpstr += ":";
				}
				tmpstr += normalizedName;
				if (!ns_prefix.empty() && !ns_uri.empty())
				{
					tmpstr += " xmlns:";
					tmpstr += ns_prefix;
					tmpstr += "=\"";
					tmpstr += ns_uri;
					tmpstr += "\"";
				}
				tmpstr +=">";
				tmpstr += encodeToXML(o->toString(),false);
				tmpstr +="</";
				if (!ns_prefix.empty())
				{
					tmpstr += ns_prefix;
					tmpstr += ":";
				}
				tmpstr += normalizedName;
				tmpstr +=">";
				_NR<XML> tmp = _MR<XML>(Class<XML>::getInstanceS(tmpstr));
				this->incRef();
				tmp->parentNode = _MR<XML>(this);
				tmpnodes.push_back(tmp);
			}
		}
		childrenlist->nodes.insert(childrenlist->nodes.begin(), tmpnodes.rbegin(),tmpnodes.rend());
	}
}

bool XML::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(considerDynamic == false)
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
		for (XMLList::XMLListVector::const_iterator it = attributelist->nodes.begin(); it != attributelist->nodes.end(); it++)
		{
			_R<XML> attr = *it;
			if (attr->nodenamespace_uri == ns_uri && attr->nodename == buf)
				return true;
		}
	}
	else
	{
		//Lookup children
		for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
		{
			_R<XML> child= childrenlist->nodes[i];
			bool name_match=(child->nodename == buf);
			bool ns_match=ns_uri.empty() || 
				(child->nodenamespace_uri == ns_uri);
			if(name_match && ns_match)
				return true;
		}
	}

	//Try the normal path as the last resource
	return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
}
bool XML::deleteVariableByMultiname(const multiname& name)
{
	unsigned int index=0;
	if(name.isAttribute)
	{
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
		if (ns_uri.empty() && ns_prefix.empty())
		{
			ns_uri = getVm()->getDefaultXMLNamespace();
		}
		if (attributelist->nodes.size() > 0)
		{
			XMLList::XMLListVector::iterator it = attributelist->nodes.end();
			while (it != attributelist->nodes.begin())
			{
				it--;
				_R<XML> attr = *it;
				if ((ns_uri=="" && name.normalizedName() == "") ||
						(ns_uri=="" && name.normalizedName() == attr->nodename) ||
						(attr->nodenamespace_uri == ns_uri && name.normalizedName() == "") ||
						(attr->nodenamespace_uri == ns_uri && attr->nodename == name.normalizedName()))
				{
					attributelist->nodes.erase(it);
				}
			}
		}
	}
	else if(XML::isValidMultiname(name,index))
	{
		childrenlist->nodes.erase(childrenlist->nodes.begin() + index);
	}
	else
	{
		//Only the first namespace is used, is this right?
		tiny_string ns_uri;
		if(name.ns.size() > 0 && !name.ns[0].hasEmptyName())
		{
			nsNameAndKindImpl ns=name.ns[0].getImpl();
			assert_and_throw(ns.kind==NAMESPACE);
			ns_uri=ns.name;
		}
		if (childrenlist->nodes.size() > 0)
		{
			XMLList::XMLListVector::iterator it = childrenlist->nodes.end();
			while (it != childrenlist->nodes.begin())
			{
				it--;
				_R<XML> node = *it;
				if ((ns_uri=="" && name.normalizedName() == "") ||
						(ns_uri=="" && name.normalizedName() == node->nodename) ||
						(node->nodenamespace_uri == ns_uri && name.normalizedName() == "") ||
						(node->nodenamespace_uri == ns_uri && node->nodename == name.normalizedName()))
				{
					childrenlist->nodes.erase(it);
				}
			}
		}
	}
	return true;
}
bool XML::isValidMultiname(const multiname& name, uint32_t& index)
{
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
	if(name.ns.size()!=0 && !name.ns[0].hasEmptyName())
		return false;

	if (name.isEmpty())
		return false;
	bool validIndex=name.toUInt(index, true);
	// Don't throw for non-numeric NAME_STRING or NAME_OBJECT
	// because they can still be valid built-in property names.
	if(!validIndex && (name.name_type==multiname::NAME_INT || name.name_type==multiname::NAME_NUMBER))
		throwError<RangeError>(kOutOfRangeError, name.normalizedName(), "?");

	return validIndex;
}

tiny_string XML::getNamespacePrefixByURI(const tiny_string& uri, bool create)
{
	tiny_string prefix;
	bool found=false;


	XML* tmp = this;
	while(tmp && tmp->is<XML>())
	{
		if(tmp->nodenamespace_uri==uri)
		{
			prefix=tmp->nodenamespace_prefix;
			found = true;
			break;
		}
		if (!tmp->parentNode)
			break;
		tmp = tmp->parentNode.getPtr();
	}

	if(!found && create)
	{
		nodenamespace_uri = uri;
	}

	return prefix;
}

ASFUNCTIONBODY(XML,_toString)
{
	XML* th=Class<XML>::cast(obj);
	return Class<ASString>::getInstanceS(th->toString_priv());
}

ASFUNCTIONBODY(XML,_getIgnoreComments)
{
	return abstract_b(ignoreComments);
}
ASFUNCTIONBODY(XML,_setIgnoreComments)
{
	assert(args && argslen==1);
	ignoreComments = Boolean_concrete(args[0]);
	return NULL;
}
ASFUNCTIONBODY(XML,_getIgnoreProcessingInstructions)
{
	return abstract_b(ignoreProcessingInstructions);
}
ASFUNCTIONBODY(XML,_setIgnoreProcessingInstructions)
{
	assert(args && argslen==1);
	ignoreProcessingInstructions = Boolean_concrete(args[0]);
	return NULL;
}
ASFUNCTIONBODY(XML,_getIgnoreWhitespace)
{
	return abstract_b(ignoreWhitespace);
}
ASFUNCTIONBODY(XML,_setIgnoreWhitespace)
{
	assert(args && argslen==1);
	ignoreWhitespace = Boolean_concrete(args[0]);
	xmlKeepBlanksDefault(ignoreWhitespace ? 0 : 1);
	return NULL;
}
ASFUNCTIONBODY(XML,_getPrettyIndent)
{
	return abstract_i(prettyIndent);
}
ASFUNCTIONBODY(XML,_setPrettyIndent)
{
	assert(args && argslen==1);
	prettyIndent = args[0]->toInt();
	return NULL;
}
ASFUNCTIONBODY(XML,_getPrettyPrinting)
{
	return abstract_b(prettyPrinting);
}
ASFUNCTIONBODY(XML,_setPrettyPrinting)
{
	assert(args && argslen==1);
	prettyPrinting = Boolean_concrete(args[0]);
	return NULL;
}
ASFUNCTIONBODY(XML,_getSettings)
{
	ASObject* res = Class<ASObject>::getInstanceS();
	multiname mn(NULL);
	mn.name_type=multiname::NAME_STRING;
	mn.ns.push_back(nsNameAndKind("",NAMESPACE));
	mn.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	mn.isAttribute = true;

	mn.name_s_id=getSys()->getUniqueStringId("ignoreComments");
	res->setVariableByMultiname(mn,abstract_b(ignoreComments),CONST_NOT_ALLOWED);
	mn.name_s_id=getSys()->getUniqueStringId("ignoreProcessingInstructions");
	res->setVariableByMultiname(mn,abstract_b(ignoreProcessingInstructions),CONST_NOT_ALLOWED);
	mn.name_s_id=getSys()->getUniqueStringId("ignoreWhitespace");
	res->setVariableByMultiname(mn,abstract_b(ignoreWhitespace),CONST_NOT_ALLOWED);
	mn.name_s_id=getSys()->getUniqueStringId("prettyIndent");
	res->setVariableByMultiname(mn,abstract_i(prettyIndent),CONST_NOT_ALLOWED);
	mn.name_s_id=getSys()->getUniqueStringId("prettyPrinting");
	res->setVariableByMultiname(mn,abstract_b(prettyPrinting),CONST_NOT_ALLOWED);
	return res;
}
ASFUNCTIONBODY(XML,_setSettings)
{
	if (argslen == 0)
	{
		setDefaultXMLSettings();
		return getSys()->getNullRef();
	}
	_NR<ASObject> arg0;
	ARG_UNPACK(arg0);
	if (arg0->is<Null>() || arg0->is<Undefined>())
	{
		setDefaultXMLSettings();
		return getSys()->getNullRef();
	}
	multiname mn(NULL);
	mn.name_type=multiname::NAME_STRING;
	mn.ns.push_back(nsNameAndKind("",NAMESPACE));
	mn.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	mn.isAttribute = true;
	_NR<ASObject> o;

	mn.name_s_id=getSys()->getUniqueStringId("ignoreComments");
	if (arg0->hasPropertyByMultiname(mn,true,true))
	{
		o=arg0->getVariableByMultiname(mn,SKIP_IMPL);
		ignoreComments = o->toInt();
	}

	mn.name_s_id=getSys()->getUniqueStringId("ignoreProcessingInstructions");
	if (arg0->hasPropertyByMultiname(mn,true,true))
	{
		o=arg0->getVariableByMultiname(mn,SKIP_IMPL);
		ignoreProcessingInstructions = o->toInt();
	}

	mn.name_s_id=getSys()->getUniqueStringId("ignoreWhitespace");
	if (arg0->hasPropertyByMultiname(mn,true,true))
	{
		o=arg0->getVariableByMultiname(mn,SKIP_IMPL);
		ignoreWhitespace = o->toInt();
	}

	mn.name_s_id=getSys()->getUniqueStringId("prettyIndent");
	if (arg0->hasPropertyByMultiname(mn,true,true))
	{
		o=arg0->getVariableByMultiname(mn,SKIP_IMPL);
		prettyIndent = o->toInt();
	}

	mn.name_s_id=getSys()->getUniqueStringId("prettyPrinting");
	if (arg0->hasPropertyByMultiname(mn,true,true))
	{
		o=arg0->getVariableByMultiname(mn,SKIP_IMPL);
		prettyPrinting = o->toInt();
	}
	return getSys()->getNullRef();
}
ASFUNCTIONBODY(XML,_getDefaultSettings)
{
	ASObject* res = Class<ASObject>::getInstanceS();
	multiname mn(NULL);
	mn.name_type=multiname::NAME_STRING;
	mn.ns.push_back(nsNameAndKind("",NAMESPACE));
	mn.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	mn.isAttribute = true;

	mn.name_s_id=getSys()->getUniqueStringId("ignoreComments");
	res->setVariableByMultiname(mn,abstract_b(true),CONST_NOT_ALLOWED);
	mn.name_s_id=getSys()->getUniqueStringId("ignoreProcessingInstructions");
	res->setVariableByMultiname(mn,abstract_b(true),CONST_NOT_ALLOWED);
	mn.name_s_id=getSys()->getUniqueStringId("ignoreWhitespace");
	res->setVariableByMultiname(mn,abstract_b(true),CONST_NOT_ALLOWED);
	mn.name_s_id=getSys()->getUniqueStringId("prettyIndent");
	res->setVariableByMultiname(mn,abstract_i(2),CONST_NOT_ALLOWED);
	mn.name_s_id=getSys()->getUniqueStringId("prettyPrinting");
	res->setVariableByMultiname(mn,abstract_b(true),CONST_NOT_ALLOWED);
	return res;
}
ASFUNCTIONBODY(XML,_toJSON)
{
	return Class<ASString>::getInstanceS("XML");
}

void XML::CheckCyclicReference(XML* node)
{
	XML* tmp = node;
	if (tmp == this)
		throwError<TypeError>(kXMLIllegalCyclicalLoop);
	for (auto it = tmp->childrenlist->nodes.begin(); it != tmp->childrenlist->nodes.end(); it++)
	{
		if ((*it).getPtr() == this)
			throwError<TypeError>(kXMLIllegalCyclicalLoop);
		CheckCyclicReference((*it).getPtr());
	}
}

ASFUNCTIONBODY(XML,insertChildAfter)
{
	XML* th=Class<XML>::cast(obj);
	_NR<ASObject> child1;
	_NR<ASObject> child2;
	ARG_UNPACK(child1)(child2);
	if (th->nodetype != XML_ELEMENT_NODE)
		return getSys()->getUndefinedRef();
	
	if (child2->is<XML>())
		th->CheckCyclicReference(child2->as<XML>());
	else if (child2->is<XMLList>())
	{
		for (auto it = child2->as<XMLList>()->nodes.begin(); it < child2->as<XMLList>()->nodes.end(); it++)
		{
			th->CheckCyclicReference((*it).getPtr());
		}
	}
	else
		child2 = _NR<XML>(Class<XML>::getInstanceS(child2->toString()));
	if (child1->is<Null>())
	{
		th->incRef();
		child2->as<XML>()->parentNode = _NR<XML>(th);
		if (child2->is<XML>())
		{
			th->incRef();
			child2->incRef();
			child2->as<XML>()->parentNode = _NR<XML>(th);
			th->childrenlist->nodes.insert(th->childrenlist->nodes.begin(),_NR<XML>(child2->as<XML>()));
		}
		else if (child2->is<XMLList>())
		{
			for (auto it2 = child2->as<XMLList>()->nodes.begin(); it2 < child2->as<XMLList>()->nodes.end(); it2++)
			{
				th->incRef();
				(*it2)->incRef();
				(*it2)->parentNode = _NR<XML>(th);
			}
			th->childrenlist->nodes.insert(th->childrenlist->nodes.begin(),child2->as<XMLList>()->nodes.begin(), child2->as<XMLList>()->nodes.end());
		}
		th->incRef();
		return th;
	}
	if (child1->is<XMLList>())
	{
		if (child1->as<XMLList>()->nodes.size()==0)
			return getSys()->getUndefinedRef();
		child1 = child1->as<XMLList>()->nodes[0];
	}
	for (auto it = th->childrenlist->nodes.begin(); it != th->childrenlist->nodes.end(); it++)
	{
		if ((*it).getPtr() == child1.getPtr())
		{
			th->incRef();
			if (child2->is<XML>())
			{
				th->incRef();
				child2->incRef();
				child2->as<XML>()->parentNode = _NR<XML>(th);
				th->childrenlist->nodes.insert(it+1,_NR<XML>(child2->as<XML>()));
			}
			else if (child2->is<XMLList>())
			{
				for (auto it2 = child2->as<XMLList>()->nodes.begin(); it2 < child2->as<XMLList>()->nodes.end(); it2++)
				{
					th->incRef();
					(*it2)->incRef();
					(*it2)->parentNode = _NR<XML>(th);
				}
				th->childrenlist->nodes.insert(it+1,child2->as<XMLList>()->nodes.begin(), child2->as<XMLList>()->nodes.end());
			}
			return th;
		}
	}
	return getSys()->getUndefinedRef();
}
ASFUNCTIONBODY(XML,insertChildBefore)
{
	XML* th=Class<XML>::cast(obj);
	_NR<ASObject> child1;
	_NR<ASObject> child2;
	ARG_UNPACK(child1)(child2);
	if (th->nodetype != XML_ELEMENT_NODE)
		return getSys()->getUndefinedRef();
	
	if (child2->is<XML>())
		th->CheckCyclicReference(child2->as<XML>());
	else if (child2->is<XMLList>())
	{
		for (auto it = child2->as<XMLList>()->nodes.begin(); it < child2->as<XMLList>()->nodes.end(); it++)
		{
			th->CheckCyclicReference((*it).getPtr());
		}
	}
	else
		child2 = _NR<XML>(Class<XML>::getInstanceS(child2->toString()));

	if (child1->is<Null>())
	{
		if (child2->is<XML>())
		{
			th->appendChild(_NR<XML>(child2->as<XML>()));
		}
		else if (child2->is<XMLList>())
		{
			for (auto it = child2->as<XMLList>()->nodes.begin(); it < child2->as<XMLList>()->nodes.end(); it++)
			{
				th->incRef();
				(*it)->incRef();
				(*it)->parentNode = _NR<XML>(th);
				th->childrenlist->nodes.push_back(_NR<XML>(*it));
			}
		}
		th->incRef();
		return th;
	}
	if (child1->is<XMLList>())
	{
		if (child1->as<XMLList>()->nodes.size()==0)
			return getSys()->getUndefinedRef();
		child1 = child1->as<XMLList>()->nodes[0];
	}
	for (auto it = th->childrenlist->nodes.begin(); it != th->childrenlist->nodes.end(); it++)
	{
		if ((*it).getPtr() == child1.getPtr())
		{
			th->incRef();
			if (child2->is<XML>())
			{
				th->incRef();
				child2->incRef();
				child2->as<XML>()->parentNode = _NR<XML>(th);
				th->childrenlist->nodes.insert(it,_NR<XML>(child2->as<XML>()));
			}
			else if (child2->is<XMLList>())
			{
				for (auto it2 = child2->as<XMLList>()->nodes.begin(); it2 < child2->as<XMLList>()->nodes.end(); it2++)
				{
					th->incRef();
					(*it2)->incRef();
					(*it2)->parentNode = _NR<XML>(th);
				}
				th->childrenlist->nodes.insert(it,child2->as<XMLList>()->nodes.begin(), child2->as<XMLList>()->nodes.end());
			}
			return th;
		}
	}
	return getSys()->getUndefinedRef();
}

ASFUNCTIONBODY(XML,namespaceDeclarations)
{
	XML *th = obj->as<XML>();
	Array *namespaces = Class<Array>::getInstanceS();
	for (uint32_t i = 0; i < th->namespacedefs.size(); i++)
	{
		_R<Namespace> tmpns = th->namespacedefs[i];
		bool b;
		if (tmpns->getPrefix(b) != "")
		{
			tmpns->incRef();
			namespaces->push(tmpns);
		}
	}
	return namespaces;
}

ASFUNCTIONBODY(XML,removeNamespace)
{
	XML *th = obj->as<XML>();
	_NR<ASObject> arg1;
	ARG_UNPACK(arg1);
	Namespace* ns;
	if (arg1->is<Namespace>())
		ns = arg1->as<Namespace>();
	else
		ns = Class<Namespace>::getInstanceS(arg1->toString(), "");

	th->RemoveNamespace(ns);
	th->incRef();
	return th;
}
void XML::RemoveNamespace(Namespace *ns)
{
	if (this->nodenamespace_uri == ns->getURI())
	{
		this->nodenamespace_uri = "";
		this->nodenamespace_prefix = "";
	}
	for (auto it = namespacedefs.begin(); it !=  namespacedefs.end(); it++)
	{
		_R<Namespace> tmpns = *it;
		if (tmpns->getURI() == ns->getURI())
		{
			namespacedefs.erase(it);
			break;
		}
	}
	if (childrenlist)
	{
		for (auto it = childrenlist->nodes.begin(); it != childrenlist->nodes.end(); it++)
		{
			(*it)->RemoveNamespace(ns);
		}
	}
}

ASFUNCTIONBODY(XML,comments)
{
	XML* th=Class<XML>::cast(obj);
	tiny_string name;
	ARG_UNPACK(name,"*");
	XMLVector ret;
	th->getComments(ret);
	return Class<XMLList>::getInstanceS(ret,th->getChildrenlist(),multiname(NULL));
}
void XML::getComments(XMLVector& ret)
{
	if (childrenlist)
	{
		for (auto it = childrenlist->nodes.begin(); it != childrenlist->nodes.end(); it++)
		{
			if ((*it)->getNodeKind() == XML_COMMENT_NODE)
			{
				(*it)->incRef();
				ret.push_back(*it);
			}
		}
	}
}

ASFUNCTIONBODY(XML,processingInstructions)
{
	XML* th=Class<XML>::cast(obj);
	tiny_string name;
	ARG_UNPACK(name,"*");
	XMLVector ret;
	th->getprocessingInstructions(ret,name);
	return Class<XMLList>::getInstanceS(ret,th->getChildrenlist(),multiname(NULL));
}
void XML::getprocessingInstructions(XMLVector& ret, tiny_string name)
{
	if (childrenlist)
	{
		for (auto it = childrenlist->nodes.begin(); it != childrenlist->nodes.end(); it++)
		{
			if ((*it)->getNodeKind() == XML_PI_NODE && (name == "*" || name == (*it)->nodename))
			{
				(*it)->incRef();
				ret.push_back(*it);
			}
		}
	}
}
ASFUNCTIONBODY(XML,_propertyIsEnumerable)
{
	return abstract_b(argslen == 1 && args[0]->toString() == "0" );
}
ASFUNCTIONBODY(XML,_hasOwnProperty)
{
	tiny_string prop;
	ARG_UNPACK(prop);

	bool ret = false;
	if (prop == "0")
		ret = true;
	else
	{
		multiname name(NULL);
		name.name_type=multiname::NAME_STRING;
		name.name_s_id=getSys()->getUniqueStringId(args[0]->toString());
		name.ns.push_back(nsNameAndKind("",NAMESPACE));
		name.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
		name.isAttribute=false;
		ret=obj->hasPropertyByMultiname(name, true, true);
	}
	return abstract_b(ret);
}

tiny_string XML::toString_priv()
{
	tiny_string ret;
	if (getNodeKind() == XML_TEXT_NODE ||
		getNodeKind() == XML_ATTRIBUTE_NODE ||
		getNodeKind() == XML_CDATA_SECTION_NODE)
	{
		ret=nodevalue;
	}
	else if (hasSimpleContent())
	{
		auto it = childrenlist->nodes.begin();
		while(it != childrenlist->nodes.end())
		{
			if ((*it)->getNodeKind() != XML_COMMENT_NODE &&
					(*it)->getNodeKind() != XML_PI_NODE)
				ret += (*it)->toString_priv();
			it++;
		}
	}
	else
	{
		ret=toXMLString_internal();
	}
	return ret;
}

const tiny_string XML::encodeToXML(const tiny_string value, bool bIsAttribute)
{

	tiny_string res;
	auto it = value.begin();
	while (it != value.end())
	{
		switch (*it)
		{
			case '<':
				res += "&lt;";
				break;
			case '>':
				res += bIsAttribute ? ">" : "&gt;";
				break;
			case '&':
				res += "&amp;";
				break;
			case '\"':
				res += bIsAttribute ? "&quot;" : "\"";
				break;
			case '\r':
				res += bIsAttribute ? "&#xD;" : "\r";
				break;
			case '\n':
				res += bIsAttribute ? "&#xA;" : "\n";
				break;
			case '\t':
				res += bIsAttribute ? "&#x9;" : "\t";
				break;
			default:
				res += *it;
				break;
		}
		it++;
	}
	return res;
}

bool XML::getPrettyPrinting()
{
	return prettyPrinting;
}

tiny_string XML::toString()
{
	return toString_priv();
}

int32_t XML::toInt()
{
	if (!hasSimpleContent())
		return 0;

	tiny_string str = toString_priv();
	return Integer::stringToASInteger(str.raw_buf(), 0);
}

bool XML::nodesEqual(XML *a, XML *b) const
{
	assert(a && b);

	// type
	if(a->nodetype!=b->nodetype)
		return false;

	// name
	if(a->nodename!=b->nodename || 
	   (!a->nodename.empty() && 
	    a->nodenamespace_uri!=b->nodenamespace_uri))
		return false;

	// attributes
	if(a->nodetype==XML_ELEMENT_NODE)
	{
		if (a->attributelist->nodes.size() != b->attributelist->nodes.size())
			return false;
			
		for (int i = 0; i < (int)a->attributelist->nodes.size(); i++)
		{
			_R<XML> oa= a->attributelist->nodes[i];
			_R<XML> ob= b->attributelist->nodes[i];
			if (!oa->isEqual(ob.getPtr()))
				return false;
		}
	}
	// content
	if (a->nodevalue != b->nodevalue)
		return false;
	
	// children
	return a->childrenlist->isEqual(b->childrenlist.getPtr());
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
	if (!isConstructed())
		return !r->isConstructed() || r->getObjectType() == T_NULL || r->getObjectType() == T_UNDEFINED;
	XML *x=dynamic_cast<XML *>(r);
	if(x)
		return nodesEqual(this, x);

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

void XML::createTree(xmlpp::Node* node)
{
	const xmlpp::Node::NodeList& list=node->get_children();
	xmlpp::Node::NodeList::const_iterator it=list.begin();
	childrenlist = _MR(Class<XMLList>::getInstanceS());
	childrenlist->incRef();

	this->nodetype = node->cobj()->type;
	this->nodename = node->get_name();
	this->nodenamespace_uri = node->get_namespace_uri();
	this->nodenamespace_prefix = node ->get_namespace_prefix();
	
	switch (this->nodetype)
	{
		case XML_ATTRIBUTE_NODE:
		case XML_TEXT_NODE:
		{
			xmlpp::ContentNode *textnode=dynamic_cast<xmlpp::ContentNode*>(node);
			this->nodevalue = textnode->get_content();
			if (ignoreWhitespace)
			{
				nodevalue = removeWhitespace(nodevalue);
				if (nodevalue.empty())
					return;
			}
			break;
		}
		case XML_PI_NODE:
		{
			xmlpp::ContentNode *textnode=dynamic_cast<xmlpp::ContentNode*>(node);
			this->nodevalue = textnode->get_content();
			break;
		}
		case XML_COMMENT_NODE:
		case XML_CDATA_SECTION_NODE:
		{
			xmlpp::ContentNode *textnode=dynamic_cast<xmlpp::ContentNode*>(node);
			this->nodevalue = textnode->get_content();
			break;
		}
		default:
			break;
	}
	for(;it!=list.end();++it)
	{
		if (ignoreProcessingInstructions && (*it)->cobj()->type == XML_PI_NODE)
			continue;
		if (ignoreComments && (*it)->cobj()->type == XML_COMMENT_NODE)
			continue;
		if (ignoreWhitespace && (*it)->cobj()->type == XML_TEXT_NODE)
		{
			xmlpp::ContentNode *textnode=dynamic_cast<xmlpp::ContentNode*>(*it);
			tiny_string tmpstr = textnode->get_content();
			if (ignoreWhitespace)
			{
				tmpstr = removeWhitespace(tmpstr);
				if (tmpstr.empty())
					continue;
			}
		}
		_NR<XML> tmp = _MR<XML>(Class<XML>::getInstanceS(*it));
		this->incRef();
		tmp->parentNode = _MR<XML>(this);
		childrenlist->append(_R<XML>(tmp));
	}
	const xmlNode* xmlN = node->cobj();

	xmlNsPtr nsdefs = xmlN->nsDef;
	while (nsdefs)
	{
		tiny_string uri;
		if (nsdefs->href) uri= (char*)nsdefs->href;
		tiny_string prefix;
		if (nsdefs->prefix) prefix= (char*)nsdefs->prefix;
		Namespace* ns = Class<Namespace>::getInstanceS(uri,prefix);
		namespacedefs.push_back(_MR(ns));
		nsdefs = nsdefs->next;
	}
	attributelist = _MR(Class<XMLList>::getInstanceS());
	for(xmlAttr* attr=xmlN->properties; attr!=NULL; attr=attr->next)
	{
		_NR<XML> tmp = _MR<XML>(Class<XML>::getInstanceS());
		this->incRef();
		tmp->parentNode = _MR<XML>(this);
		tmp->nodetype = XML_ATTRIBUTE_NODE;
		tmp->nodename = (char*)attr->name;
		if (attr->ns)
		{
			tmp->nodenamespace_uri = (char*)attr->ns->href;
			tmp->nodenamespace_prefix = (char*)attr->ns->prefix;
		}
		else 
			tmp->nodenamespace_uri = getVm()->getDefaultXMLNamespace();

		//NOTE: libxmlpp headers says that Node::create_wrapper
		//is supposed to be internal API. Still it's very useful and
		//we use it.
		xmlpp::Node::create_wrapper(reinterpret_cast<xmlNode*>(attr));
		xmlpp::Node* attrX=static_cast<xmlpp::Node*>(attr->_private);
		xmlpp::Attribute *textnode=dynamic_cast<xmlpp::Attribute*>(attrX);
		tmp->nodevalue = textnode->get_value();
		tmp->constructed = true;
		attributelist->nodes.push_back(tmp);
	}
	constructed=true;
}

ASFUNCTIONBODY(XML,_prependChild)
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

	th->appendChild(arg);
	th->incRef();
	return th;
}
void XML::prependChild(_R<XML> newChild)
{
	if (newChild->constructed)
	{
		this->incRef();
		newChild->parentNode = _NR<XML>(this);
		childrenlist->append(newChild);
	}
	else
		newChild->decRef();
}
