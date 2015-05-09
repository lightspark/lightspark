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

#include "scripting/toplevel/XMLList.h"
#include "scripting/class.h"
#include "compat.h"
#include "scripting/argconv.h"
#include "abc.h"
#include <libxml/tree.h>
#include <libxml++/parsers/domparser.h>
#include <libxml++/nodes/textnode.h>

using namespace std;
using namespace lightspark;

/* XMLList of size 1 delegates function calls to the single XML
 * object, if no method with the same name has been defined for
 * XMLList. */
#define REGISTER_XML_DELEGATE(name) \
	c->setDeclaredMethodByQName(#name,AS3,Class<IFunction>::getFunction(name),NORMAL_METHOD,true)

#define REGISTER_XML_DELEGATE2(asname,cppname) \
	c->setDeclaredMethodByQName(#asname,AS3,Class<IFunction>::getFunction(cppname),NORMAL_METHOD,true)

#define ASFUNCTIONBODY_XML_DELEGATE(name) \
	ASObject* XMLList::name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		XMLList* th=obj->as<XMLList>(); \
		if(!th) \
			throw Class<ArgumentError>::getInstanceS("Function applied to wrong object"); \
		if(th->nodes.size()==1) \
			return XML::name(th->nodes[0].getPtr(), args, argslen); \
		else \
			throwError<TypeError>(kXMLOnlyWorksWithOneItemLists, #name); \
		return NULL; \
	}

XMLList::XMLList(Class_base* c):ASObject(c),nodes(c->memoryAccount),constructed(false),targetobject(NULL),targetproperty(c->memoryAccount)
{
}

XMLList::XMLList(Class_base* cb,bool c):ASObject(cb),nodes(cb->memoryAccount),constructed(c),targetobject(NULL),targetproperty(cb->memoryAccount)
{
	assert(c);
}

XMLList::XMLList(Class_base* c, const std::string& str):ASObject(c),nodes(c->memoryAccount),constructed(true),targetobject(NULL),targetproperty(c->memoryAccount)
{
	buildFromString(str);
}

XMLList::XMLList(Class_base* c, const XML::XMLVector& r):
	ASObject(c),nodes(r.begin(),r.end(),c->memoryAccount),constructed(true),targetobject(NULL),targetproperty(c->memoryAccount)
{
}
XMLList::XMLList(Class_base* c, const XML::XMLVector& r, XMLList *targetobject, const multiname &targetproperty):
	ASObject(c),nodes(r.begin(),r.end(),c->memoryAccount),constructed(true),targetobject(targetobject),targetproperty(c->memoryAccount)
{
	if (targetobject)
		targetobject->incRef();
	this->targetproperty.name_type = targetproperty.name_type;
	this->targetproperty.isAttribute = targetproperty.isAttribute;
	this->targetproperty.name_s_id = targetproperty.name_s_id;
	for (auto it = targetproperty.ns.begin();it != targetproperty.ns.end(); it++)
	{
		this->targetproperty.ns.push_back(*it);
	}
}

void XMLList::finalize()
{
	if (targetobject)
		targetobject->decRef();
	//nodes.clear();
	ASObject::finalize();
}

void XMLList::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_getLength),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attribute",AS3,Class<IFunction>::getFunction(attribute),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes",AS3,Class<IFunction>::getFunction(attributes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("child",AS3,Class<IFunction>::getFunction(child),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("children",AS3,Class<IFunction>::getFunction(children),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains",AS3,Class<IFunction>::getFunction(contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copy",AS3,Class<IFunction>::getFunction(copy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descendants",AS3,Class<IFunction>::getFunction(descendants),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("elements",AS3,Class<IFunction>::getFunction(elements),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("normalize",AS3,Class<IFunction>::getFunction(_normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parent",AS3,Class<IFunction>::getFunction(parent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasSimpleContent",AS3,Class<IFunction>::getFunction(_hasSimpleContent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasComplexContent",AS3,Class<IFunction>::getFunction(_hasComplexContent),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(valueOf),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(valueOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toXMLString",AS3,Class<IFunction>::getFunction(toXMLString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("text",AS3,Class<IFunction>::getFunction(text),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("comments",AS3,Class<IFunction>::getFunction(comments),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("processingInstructions",AS3,Class<IFunction>::getFunction(processingInstructions),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("propertyIsEnumerable",AS3,Class<IFunction>::getFunction(_propertyIsEnumerable),NORMAL_METHOD,true);
	REGISTER_XML_DELEGATE(addNamespace);
	REGISTER_XML_DELEGATE2(appendChild,_appendChild);
	REGISTER_XML_DELEGATE(childIndex);
	REGISTER_XML_DELEGATE(inScopeNamespaces);
	REGISTER_XML_DELEGATE(insertChildAfter);
	REGISTER_XML_DELEGATE(insertChildBefore);
	REGISTER_XML_DELEGATE(localName);
	REGISTER_XML_DELEGATE(name);
	REGISTER_XML_DELEGATE2(namespace,_namespace);
	REGISTER_XML_DELEGATE(namespaceDeclarations);
	REGISTER_XML_DELEGATE(nodeKind);
	REGISTER_XML_DELEGATE2(prependChild,_appendChild);
	REGISTER_XML_DELEGATE(removeNamespace);
	//REGISTER_XML_DELEGATE(replace);
	REGISTER_XML_DELEGATE2(setChildren,_setChildren);
	REGISTER_XML_DELEGATE2(setLocalName,_setLocalName);
	REGISTER_XML_DELEGATE2(setName,_setName);
	REGISTER_XML_DELEGATE2(setNamespace,_setNamespace);
}

ASFUNCTIONBODY_XML_DELEGATE(addNamespace);
ASFUNCTIONBODY_XML_DELEGATE(_appendChild);
ASFUNCTIONBODY_XML_DELEGATE(childIndex);
ASFUNCTIONBODY_XML_DELEGATE(inScopeNamespaces);
ASFUNCTIONBODY_XML_DELEGATE(insertChildAfter);
ASFUNCTIONBODY_XML_DELEGATE(insertChildBefore);
ASFUNCTIONBODY_XML_DELEGATE(localName);
ASFUNCTIONBODY_XML_DELEGATE(name);
ASFUNCTIONBODY_XML_DELEGATE(_namespace);
ASFUNCTIONBODY_XML_DELEGATE(namespaceDeclarations);
ASFUNCTIONBODY_XML_DELEGATE(nodeKind);
ASFUNCTIONBODY_XML_DELEGATE(_prependChild);
ASFUNCTIONBODY_XML_DELEGATE(removeNamespace);
//ASFUNCTIONBODY_XML_DELEGATE(replace);
ASFUNCTIONBODY_XML_DELEGATE(_setChildren);
ASFUNCTIONBODY_XML_DELEGATE(_setLocalName);
ASFUNCTIONBODY_XML_DELEGATE(_setName);
ASFUNCTIONBODY_XML_DELEGATE(_setNamespace);

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
	   args[0]->is<Null>() || 
	   args[0]->is<Undefined>())
	{
		th->constructed=true;
		return NULL;
	}
	else if(args[0]->is<XML>())
	{
		args[0]->incRef();
		th->append(_MR(args[0]->as<XML>()));
	}
	else if(args[0]->is<XMLList>())
	{
		args[0]->incRef();
		th->append(_MR(args[0]->as<XMLList>()));
	}
	else if(args[0]->is<ASString>() ||
		args[0]->is<Number>() ||
		args[0]->is<Integer>() ||
		args[0]->is<UInteger>() ||
		args[0]->is<Boolean>())
	{
		th->buildFromString(args[0]->toString());
	}
	else
	{
		throw RunTimeException("Type not supported in XMLList()");
	}

	return NULL;
}

void XMLList::buildFromString(const std::string& str)
{
	xmlpp::DomParser parser;
	std::string default_ns=getVm()->getDefaultXMLNamespace();
	std::string xmldecl;
	std::string str_without_xmldecl = extractXMLDeclaration(str, xmldecl);
	std::string expanded = xmldecl + 
		"<parent xmlns=\"" + default_ns + "\">" + 
		XMLBase::parserQuirks(str_without_xmldecl) + 
		"</parent>";
	try
	{
		parser.parse_memory(expanded);
	}
	catch(const exception& e)
	{
		try
		{
			parser.parse_memory(str);
		}
		catch(const exception& e)
		{
			throw RunTimeException("Error while parsing XML");
		}
	}
	const xmlpp::Node::NodeList& children=\
	  parser.get_document()->get_root_node()->get_children();
	xmlpp::Node::NodeList::const_iterator it;
	for(it=children.begin(); it!=children.end(); ++it)
	{
		_R<XML> tmp = _MR(Class<XML>::getInstanceS(*it));
		if (tmp->constructed)
			nodes.push_back(tmp);
	}
}

std::string XMLList::extractXMLDeclaration(const std::string& xml, std::string& xmldecl_out)
{
	if (xml.compare(0, 2, "<?") != 0)
		return xml;
	std::string res = xml;
	xmldecl_out = "";
	size_t declEnd = 0;
	size_t len = xml.length();
	while (declEnd<len && declEnd != xml.npos)
	{
		if (g_unichar_isspace(xml[declEnd]))
				declEnd++;
		else
		{
			if (xml.compare(declEnd, 2, "<?") == 0)
			{
				size_t tmp = xml.find("?>",declEnd);
				if (tmp == xml.npos)
					break;
				declEnd = tmp +2;
			}
			else
				break;
		}
	}
	if (declEnd && declEnd != xml.npos)
	{
		xmldecl_out = xml.substr(0, declEnd);
		if (declEnd<len)
			res = xml.substr(declEnd);
		else
			res = "";
	}
	return res;
}

_R<XML> XMLList::reduceToXML() const
{
	//Needed to convert XMLList to XML
	//Check if there is a single non null XML object. If not throw an exception
	if(nodes.size()==1)
		return nodes[0];
	else
	{
		throwError<TypeError>(kIllegalNamespaceError);
		return nodes[0]; // not reached, the previous line throws always
	}
}

ASFUNCTIONBODY(XMLList,_getLength)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_i(th->nodes.size());
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
	if(argslen==0)
	{
		return Class<XMLList>::getInstanceS("");
	}
	else if(args[0]->is<ASString>() ||
		args[0]->is<Number>() ||
		args[0]->is<Integer>() ||
		args[0]->is<UInteger>() ||
		args[0]->is<Boolean>())
	{
		return Class<XMLList>::getInstanceS(args[0]->toString());
	}
	else if(args[0]->is<XMLList>())
	{
		args[0]->incRef();
		return args[0];
	}
	else if(args[0]->is<XML>())
	{
		XML::XMLVector nodes;
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
	_NR<ASObject> name;
	ARG_UNPACK(name,_NR<ASObject>(Class<ASString>::getInstanceS("*")));
	XML::XMLVector ret;
	multiname mname(NULL);
	name->applyProxyProperty(mname);
	th->getDescendantsByQName(name->toString(),"",mname.isAttribute,ret);
	return Class<XMLList>::getInstanceS(ret,th->targetobject,multiname(NULL));
}

ASFUNCTIONBODY(XMLList,elements)
{
	XMLList* th=Class<XMLList>::cast(obj);
	tiny_string name;
	ARG_UNPACK(name, "");

	XML::XMLVector elems;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getElementNodes(name, elems);
	}
	return Class<XMLList>::getInstanceS(elems,th->targetobject,multiname(NULL));
}

ASFUNCTIONBODY(XMLList,parent)
{
	XMLList* th=Class<XMLList>::cast(obj);

	if(th->nodes.size()==0)
		return getSys()->getUndefinedRef();

	auto it=th->nodes.begin();
	ASObject *parent=(*it)->getParentNode();
	++it;

	for(; it!=th->nodes.end(); ++it)
	{
		ASObject *otherParent=(*it)->getParentNode();
		if(!parent->isEqual(otherParent))
			return getSys()->getUndefinedRef();
	}

	return parent;
}

ASFUNCTIONBODY(XMLList,valueOf)
{
	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(XMLList,child)
{
	XMLList* th = obj->as<XMLList>();
	assert_and_throw(argslen==1);
	XML::XMLVector ret;
	if(args[0]->is<Number>() ||
		args[0]->is<Integer>() ||
		args[0]->is<UInteger>())
	{
		uint32_t index =args[0]->toUInt();
		auto it=th->nodes.begin();
		for(; it!=th->nodes.end(); ++it)
		{
			(*it)->childrenImpl(ret, index);
		}
	}
	else
	{
		const tiny_string& arg0=args[0]->toString();
		auto it=th->nodes.begin();
		for(; it!=th->nodes.end(); ++it)
		{
			(*it)->childrenImpl(ret, arg0);
		}
	}
	XMLList* retObj=Class<XMLList>::getInstanceS(ret,th->targetobject,multiname(NULL));
	return retObj;
}

ASFUNCTIONBODY(XMLList,children)
{
	XMLList* th = obj->as<XMLList>();
	assert_and_throw(argslen==0);
	XML::XMLVector ret;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->childrenImpl(ret, "*");
	}
	XMLList* retObj=Class<XMLList>::getInstanceS(ret,th->targetobject,multiname(NULL));
	return retObj;
}

ASFUNCTIONBODY(XMLList,text)
{
	XMLList* th = obj->as<XMLList>();
	ARG_UNPACK;
	XML::XMLVector ret;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getText(ret);
	}
	return Class<XMLList>::getInstanceS(ret,th->targetobject,multiname(NULL));
}

ASFUNCTIONBODY(XMLList,contains)
{
	XMLList* th = obj->as<XMLList>();
	_NR<ASObject> value;
	ARG_UNPACK (value);
	if(!value->is<XML>())
		return abstract_b(false);

	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		if((*it)->isEqual(value.getPtr()))
			return abstract_b(true);
	}

	return abstract_b(false);
}

ASFUNCTIONBODY(XMLList,copy)
{
	XMLList* th = obj->as<XMLList>();
	XMLList *dest = Class<XMLList>::getInstanceS();
	dest->targetobject = th->targetobject;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		dest->nodes.push_back(_MR((*it)->copy()));
	}
	return dest;
}

ASFUNCTIONBODY(XMLList,attribute)
{
	XMLList *th = obj->as<XMLList>();

	if(argslen > 0 && args[0]->is<QName>())
		LOG(LOG_NOT_IMPLEMENTED,"XMLList.attribute called with QName");

	tiny_string attrname;
	ARG_UNPACK (attrname);
	multiname mname(NULL);
	mname.name_type=multiname::NAME_STRING;
	mname.name_s_id=getSys()->getUniqueStringId(attrname);
	mname.ns.push_back(nsNameAndKind("",NAMESPACE));
	mname.isAttribute = true;

	_NR<ASObject> attr=th->getVariableByMultiname(mname, NONE);
	assert(!attr.isNull());
	attr->incRef();
	return attr.getPtr();
}

ASFUNCTIONBODY(XMLList,attributes)
{
	XMLList *th = obj->as<XMLList>();
	XMLList *res = Class<XMLList>::getInstanceS();
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		XML::XMLVector nodeAttributes = (*it)->getAttributes();
		res->nodes.insert(res->nodes.end(), nodeAttributes.begin(), nodeAttributes.end());
	}
	return res;
}

ASFUNCTIONBODY(XMLList,comments)
{
	XMLList* th=Class<XMLList>::cast(obj);

	XMLList *res = Class<XMLList>::getInstanceS();
	XML::XMLVector nodecomments;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getComments(nodecomments);
	}
	res->nodes.insert(res->nodes.end(), nodecomments.begin(), nodecomments.end());
	return res;
}
ASFUNCTIONBODY(XMLList,processingInstructions)
{
	XMLList* th=Class<XMLList>::cast(obj);
	tiny_string name;
	ARG_UNPACK(name,"*");

	XMLList *res = Class<XMLList>::getInstanceS();
	XML::XMLVector nodeprocessingInstructions;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getprocessingInstructions(nodeprocessingInstructions,name);
	}
	res->nodes.insert(res->nodes.end(), nodeprocessingInstructions.begin(), nodeprocessingInstructions.end());
	return res;
}
ASFUNCTIONBODY(XMLList,_propertyIsEnumerable)
{
	XMLList* th=Class<XMLList>::cast(obj);
	if (argslen == 1)
	{
		int32_t n = args[0]->toInt();
		return abstract_b(n < (int32_t)th->nodes.size());
		
	}
	return abstract_b(false);
}

ASFUNCTIONBODY(XMLList,_normalize)
{
	XMLList *th = obj->as<XMLList>();
	th->normalize();
	th->incRef();
	return th;
}
void XMLList::normalize()
{
	auto it=nodes.begin();
	while (it!=nodes.end())
	{
		if ((*it)->getNodeKind() == XML_ELEMENT_NODE)
		{
			(*it)->normalize();
			++it;
		}
		else if ((*it)->getNodeKind() == XML_TEXT_NODE)
		{
			if ((*it)->toString().empty())
			{
				it = nodes.erase(it);
			}
			else
			{
				_R<XML> textnode = *it;

				++it;
				while (it!=nodes.end() && (*it)->getNodeKind() == XML_TEXT_NODE)
				{
					textnode->addTextContent((*it)->toString());
					it = nodes.erase(it);
				}
			}

		}
		else
		{
			++it;
		}
	}
}

void XMLList::clear()
{
	nodes.clear();
}
void XMLList::getTargetVariables(const multiname& name,XML::XMLVector& retnodes)
{
	unsigned int index=0;
	if(XML::isValidMultiname(name,index))
	{
		retnodes.push_back(nodes[index]);
	}
	else
	{
		tiny_string normalizedName=name.normalizedName();
		
		//Only the first namespace is used, is this right?
		tiny_string namespace_uri;
		if(name.ns.size() > 0 && !name.ns[0].hasEmptyName())
		{
			nsNameAndKindImpl ns=name.ns[0].getImpl();
			if (ns.kind==NAMESPACE)
				namespace_uri=ns.name;
		}
		
		// namespace set by "default xml namespace = ..."
		if(namespace_uri.empty())
			namespace_uri=getVm()->getDefaultXMLNamespace();

		for (uint32_t i = 0; i < nodes.size(); i++)
		{
			_R<XML> child= nodes[i];
			bool nameMatches = (normalizedName=="" || normalizedName==child->nodename);
			bool nsMatches = (namespace_uri=="" || 
							  (child->nodenamespace_uri == namespace_uri));
			
			if(nameMatches && nsMatches)
			{
				retnodes.push_back(child);
			}
			if (child->childrenlist)
				child->childrenlist->getTargetVariables(name,retnodes);
		}
	}
}

_NR<ASObject> XMLList::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
	{
		_NR<ASObject> res=ASObject::getVariableByMultiname(name,opt);

		//If a method is not found on XMLList object and this
		//is a single element list with simple content,
		//delegate to ASString
		if(res.isNull() && nodes.size()==1 && nodes[0]->hasSimpleContent())
		{
			ASString *contentstr=Class<ASString>::getInstanceS(nodes[0]->toString_priv());
			res=contentstr->getVariableByMultiname(name, opt);
			contentstr->decRef();
		}

		return res;
	}
	if (name.isAttribute)
	{
		XML::XMLVector retnodes;
		auto it=nodes.begin();
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

		this->incRef();
		return _MNR(Class<XMLList>::getInstanceS(retnodes,this,name));
	}
	unsigned int index=0;
	if(XML::isValidMultiname(name,index))
	{
		if(index<nodes.size())
			return nodes[index];
		else
			return NullRef;
	}
	else
	{
		XML::XMLVector retnodes;
		auto it=nodes.begin();
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

		this->incRef();
		return _MNR(Class<XMLList>::getInstanceS(retnodes,this,name));
	}
}

bool XMLList::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	unsigned int index=0;
	if(XML::isValidMultiname(name,index))
		return index<nodes.size();
	else
	{
		auto it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			bool ret=(*it)->hasPropertyByMultiname(name, considerDynamic, considerPrototype);
			if(ret)
				return ret;
		}
	}

	return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
}

void XMLList::setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	XML::XMLVector retnodes;
	XMLList* tmplist = targetobject;
	multiname tmpprop = targetproperty;
	if (targetobject)
	{
		while (tmplist->targetobject)
		{
			tmpprop = tmplist->targetproperty;
			tmplist = tmplist->targetobject;
		}
		if (tmplist && !tmpprop.isEmpty())
		{
			tmplist->getTargetVariables(tmpprop,retnodes);
		}
	}
	if(XML::isValidMultiname(name,index))
	{
		if (index >= nodes.size())
		{
			if (targetobject)
				targetobject->appendSingleNode(o);
			appendSingleNode(o);
		}
		else
		{
			replace(index, o,retnodes,allowConst);
		}
	}
	else if (nodes.size() == 0)
	{
		if (tmplist)
		{
			if (!tmpprop.isEmpty())
			{
				XML* tmp = Class<XML>::getInstanceS();
				tmp->nodetype = XML_ELEMENT_NODE;
				tmp->nodename = targetproperty.normalizedName();
				tmp->attributelist = _MR(Class<XMLList>::getInstanceS());
				tmp->constructed = true;
				tmp->setVariableByMultiname(name,o,allowConst);
				tmp->incRef();
				tiny_string tmpname = tmpprop.normalizedName();
				if (retnodes.empty() && tmpname != "" && tmpname != "*")
				{
					XML* tmp2 = Class<XML>::getInstanceS();
					tmp2->nodetype = XML_ELEMENT_NODE;
					tmp2->nodename = tmpname;
					tmp2->attributelist = _MR(Class<XMLList>::getInstanceS());
					tmp2->constructed = true;
					tmp2->setVariableByMultiname(targetproperty,tmp,allowConst);
					tmp2->incRef();
					tmplist->appendSingleNode(tmp2);
					appendSingleNode(tmp2);
				}
				else
					tmplist->setVariableByMultiname(tmpprop,tmp,allowConst);
			}
			else
			{
				tmplist->appendSingleNode(o);
				appendSingleNode(o);
			}
		}
		else
			appendSingleNode(o);
	}
	else if (nodes.size() == 1)
	{
		nodes[0]->setVariableByMultiname(name, o, allowConst);
	}
	else
	{
		// do nothing, see ECMA-357, Section 9.2.1.2
	}
}

bool XMLList::deleteVariableByMultiname(const multiname& name)
{
	unsigned int index=0;
	bool bdeleted = false;
	
	if(XML::isValidMultiname(name,index))
	{
		_R<XML> node = nodes[index];
		if (node->parentNode)
		{
			XMLList::XMLListVector::iterator it = node->parentNode->childrenlist->nodes.end();
			while (it != node->parentNode->childrenlist->nodes.begin())
			{
				it--;
				_R<XML> n = *it;
				if (n.getPtr() == node.getPtr())
				{
					node->parentNode->childrenlist->nodes.erase(it);
					break;
				}
			}
		}
	}
	else
	{
		for (XMLList::XMLListVector::iterator it = nodes.begin(); it != nodes.end(); it++)
		{
			_R<XML> node = *it;
			if (node->deleteVariableByMultiname(name))
				bdeleted = true;
		}
	}
	return bdeleted;
}

void XMLList::getDescendantsByQName(const tiny_string& name, const tiny_string& ns, bool bIsAttribute, XML::XMLVector& ret)
{
	auto it=nodes.begin();
	for(; it!=nodes.end(); ++it)
	{
		(*it)->getDescendantsByQName(name, ns, bIsAttribute, ret);
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
	switch(nodes.size())
	{
		case 0:
			return true;
		case 1:
			return nodes[0]->hasSimpleContent();
	}
	auto it = nodes.begin();
	while (it != nodes.end())
	{
		if ((*it)->nodetype == XML_ELEMENT_NODE)
			return false;
		it++;
	}
	return true;
}

bool XMLList::hasComplexContent() const
{
	return !hasSimpleContent();
}

void XMLList::appendSingleNode(ASObject *x)
{
	if (x->is<XML>())
	{
		x->incRef();
		append(_MR(x->as<XML>()));
	}
	else if (x->is<XMLList>())
	{
		XMLList *list = x->as<XMLList>();
		if (list->nodes.size() == 1)
		{
			append(list->nodes[0]);
		}
		// do nothing, if length != 1. See ECMA-357, Section
		// 9.2.1.2
	}
	else
	{
		tiny_string str = x->toString();
		append(_MR(Class<XML>::getInstanceS(str)));
	}
}

void XMLList::append(_R<XML> x)
{
	nodes.push_back(x);
}

void XMLList::append(_R<XMLList> x)
{
	nodes.insert(nodes.end(),x->nodes.begin(),x->nodes.end());
}

void XMLList::prepend(_R<XML> x)
{
	nodes.insert(nodes.begin(),x);
}

void XMLList::prepend(_R<XMLList> x)
{
	nodes.insert(nodes.begin(),x->nodes.begin(),x->nodes.end());
}

void XMLList::replace(unsigned int idx, ASObject *o, const XML::XMLVector &retnodes,CONST_ALLOWED_FLAG allowConst)
{
	if (idx >= nodes.size())
		return;

	if (nodes[idx]->getNodeKind() == XML_ATTRIBUTE_NODE || nodes[idx]->getNodeKind() == XML_TEXT_NODE)
	{
		if (targetobject)
			targetobject->setVariableByMultiname(targetproperty,o,allowConst);
		nodes[idx]->setTextContent(o->toString());
	}
	else if (o->is<XMLList>())
	{
		if (targetobject)
		{
			for (uint32_t i = 0; i < targetobject->nodes.size(); i++)
			{
				XML* n= targetobject->nodes[i].getPtr();
				if (n == nodes[idx].getPtr())
				{
					multiname m(NULL);
					m.name_type = multiname::NAME_INT;
					m.name_i = i;
					m.ns.push_back(nsNameAndKind("",NAMESPACE));
					targetobject->setVariableByMultiname(m,o,allowConst);
					break;
				}
			}
		}
		unsigned int k = 0;
		vector<_R<XML>, reporter_allocator<_R<XML>>>::iterator it = nodes.begin();
		while (k < idx && it!=nodes.end())
		{
			++k;
			++it;
		}

		it = nodes.erase(it);

		XMLList *toAdd = o->as<XMLList>();
		nodes.insert(it, toAdd->nodes.begin(), toAdd->nodes.end());
	}
	else if (o->is<XML>())
	{
		if (retnodes.size() > idx)
		{
			multiname m(NULL);
			m.name_type = multiname::NAME_INT;
			m.name_i = idx;
			m.ns.push_back(nsNameAndKind("",NAMESPACE));
			targetobject->setVariableByMultiname(m,o,allowConst);
		}
		o->incRef();
		nodes[idx] = _MR(o->as<XML>());
	}
	else
	{
		if (nodes[idx]->nodetype == XML_TEXT_NODE)
			nodes[idx]->nodevalue = o->toString();
		else 
		{
			nodes[idx]->childrenlist->clear();
			_R<XML> tmp = _MR<XML>(Class<XML>::getInstanceS());
			nodes[idx]->incRef();
			tmp->parentNode = nodes[idx];
			tmp->nodetype = XML_TEXT_NODE;
			tmp->nodename = "text";
			tmp->nodenamespace_uri = "";
			tmp->nodenamespace_prefix = "";
			tmp->nodevalue = o->toString();
			tmp->constructed = true;
			nodes[idx]->childrenlist->append(tmp);
		}
	}
}

tiny_string XMLList::toString_priv()
{
	if (hasSimpleContent())
	{
		tiny_string ret;
		for(size_t i=0; i<nodes.size(); i++)
		{
			xmlElementType kind=nodes[i]->getNodeKind();
			switch (kind)
			{
				case XML_COMMENT_NODE:
				case XML_PI_NODE:
					break;
				case XML_ATTRIBUTE_NODE:
					ret+=nodes[i]->toString_priv();
					break;
				default:
					ret+=nodes[i]->toString_priv();
					break;
			}

				
		}
		return ret;
	}
	return toXMLString_internal();
}

tiny_string XMLList::toString()
{
	return toString_priv();
}

int32_t XMLList::toInt()
{
	if (!hasSimpleContent())
		return 0;

	tiny_string str = toString();
	return Integer::stringToASInteger(str.raw_buf(), 0);
}

ASFUNCTIONBODY(XMLList,_toString)
{
	XMLList* th=Class<XMLList>::cast(obj);
	return Class<ASString>::getInstanceS(th->toString_priv());
}

tiny_string XMLList::toXMLString_internal(bool pretty)
{
	tiny_string res;
	size_t len = nodes.size();
	for(size_t i=0; i<len; i++)
	{
		tiny_string tmp = nodes[i].getPtr()->toXMLString_internal(pretty);
		if (tmp != "")
		{
			res += tmp;
			if (pretty && i < len-1)
				res += "\n";
		}
	}
	return res;
}

ASFUNCTIONBODY(XMLList,toXMLString)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	ASString* ret=Class<ASString>::getInstanceS(th->toXMLString_internal());
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
		return nodes[index-1];
	else
		throw RunTimeException("XMLList::nextValue out of bounds");
}

void XMLList::appendNodesTo(XML *dest) const
{
	std::vector<_R<XML>, reporter_allocator<_R<XML>>>::const_iterator it;
	for (it=nodes.begin(); it!=nodes.end(); ++it)
	{
		ASObject *arg0=it->getPtr();
		ASObject *ret=XML::_appendChild(dest, &arg0, 1);
		if(ret)
			ret->decRef();
	}
}
