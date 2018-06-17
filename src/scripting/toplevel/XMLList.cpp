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

using namespace std;
using namespace lightspark;

/* XMLList of size 1 delegates function calls to the single XML
 * object, if no method with the same name has been defined for
 * XMLList. */
#define REGISTER_XML_DELEGATE(name) \
	c->setDeclaredMethodByQName(#name,AS3,Class<IFunction>::getFunction(c->getSystemState(),name),NORMAL_METHOD,true)

#define REGISTER_XML_DELEGATE2(asname,cppname) \
	c->setDeclaredMethodByQName(#asname,AS3,Class<IFunction>::getFunction(c->getSystemState(),cppname),NORMAL_METHOD,true)

#define ASFUNCTIONBODY_XML_DELEGATE(name) \
	void XMLList::name(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!obj.is<XMLList>()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		XMLList* th=obj.as<XMLList>(); \
		if(th->nodes.size()==1) {\
			asAtom a = asAtom::fromObject(th->nodes[0].getPtr()); \
			XML::name(ret,sys,a, args, argslen); \
		} else \
			throwError<TypeError>(kXMLOnlyWorksWithOneItemLists, #name); \
	}

XMLList::XMLList(Class_base* c):ASObject(c,T_OBJECT,SUBTYPE_XMLLIST),nodes(c->memoryAccount),constructed(false),targetobject(NULL),targetproperty(c->memoryAccount)
{
}

XMLList::XMLList(Class_base* cb,bool c):ASObject(cb,T_OBJECT,SUBTYPE_XMLLIST),nodes(cb->memoryAccount),constructed(c),targetobject(NULL),targetproperty(cb->memoryAccount)
{
	assert(c);
}

XMLList::XMLList(Class_base* c, const std::string& str):ASObject(c,T_OBJECT,SUBTYPE_XMLLIST),nodes(c->memoryAccount),constructed(true),targetobject(NULL),targetproperty(c->memoryAccount)
{
	buildFromString(str);
}

XMLList::XMLList(Class_base* c, const XML::XMLVector& r):
	ASObject(c,T_OBJECT,SUBTYPE_XMLLIST),nodes(r.begin(),r.end(),c->memoryAccount),constructed(true),targetobject(NULL),targetproperty(c->memoryAccount)
{
}
XMLList::XMLList(Class_base* c, const XML::XMLVector& r, XMLList *targetobject, const multiname &targetproperty):
	ASObject(c,T_OBJECT,SUBTYPE_XMLLIST),nodes(r.begin(),r.end(),c->memoryAccount),constructed(true),targetobject(targetobject),targetproperty(c->memoryAccount)
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

XMLList* XMLList::create(SystemState* sys,const XML::XMLVector& r, XMLList *targetobject, const multiname &targetproperty)
{
	XMLList* res = Class<XMLList>::getInstanceSNoArgs(sys);
	res->constructed = true;
	res->nodes.assign(r.begin(),r.end());
	
	res->targetobject = targetobject;
	if (res->targetobject)
		res->targetobject->incRef();
	res->targetproperty.name_type = targetproperty.name_type;
	res->targetproperty.isAttribute = targetproperty.isAttribute;
	res->targetproperty.name_s_id = targetproperty.name_s_id;
	for (auto it = targetproperty.ns.begin();it != targetproperty.ns.end(); it++)
	{
		res->targetproperty.ns.push_back(*it);
	}
	return res;
}

bool XMLList::destruct()
{
	if (targetobject)
		targetobject->decRef();
	nodes.clear();
	constructed = false;
	targetobject = NULL;
	targetproperty = multiname(this->getClass()->memoryAccount);
	return ASObject::destruct();
}

void XMLList::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->isReusable=true;
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_getLength),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attribute",AS3,Class<IFunction>::getFunction(c->getSystemState(),attribute),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes",AS3,Class<IFunction>::getFunction(c->getSystemState(),attributes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("child",AS3,Class<IFunction>::getFunction(c->getSystemState(),child),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("children",AS3,Class<IFunction>::getFunction(c->getSystemState(),children),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains",AS3,Class<IFunction>::getFunction(c->getSystemState(),contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copy",AS3,Class<IFunction>::getFunction(c->getSystemState(),copy),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descendants",AS3,Class<IFunction>::getFunction(c->getSystemState(),descendants),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("elements",AS3,Class<IFunction>::getFunction(c->getSystemState(),elements),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("normalize",AS3,Class<IFunction>::getFunction(c->getSystemState(),_normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parent",AS3,Class<IFunction>::getFunction(c->getSystemState(),parent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasSimpleContent",AS3,Class<IFunction>::getFunction(c->getSystemState(),_hasSimpleContent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasComplexContent",AS3,Class<IFunction>::getFunction(c->getSystemState(),_hasComplexContent),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(c->getSystemState(),valueOf),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("valueOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),valueOf),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toXMLString",AS3,Class<IFunction>::getFunction(c->getSystemState(),toXMLString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("text",AS3,Class<IFunction>::getFunction(c->getSystemState(),text),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("comments",AS3,Class<IFunction>::getFunction(c->getSystemState(),comments),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("processingInstructions",AS3,Class<IFunction>::getFunction(c->getSystemState(),processingInstructions),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("propertyIsEnumerable",AS3,Class<IFunction>::getFunction(c->getSystemState(),_propertyIsEnumerable),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasOwnProperty",AS3,Class<IFunction>::getFunction(c->getSystemState(),_hasOwnProperty),NORMAL_METHOD,true);
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
	REGISTER_XML_DELEGATE2(prependChild,_prependChild);
	REGISTER_XML_DELEGATE(removeNamespace);
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
ASFUNCTIONBODY_XML_DELEGATE(_setChildren);
ASFUNCTIONBODY_XML_DELEGATE(_setLocalName);
ASFUNCTIONBODY_XML_DELEGATE(_setName);
ASFUNCTIONBODY_XML_DELEGATE(_setNamespace);

ASFUNCTIONBODY_ATOM(XMLList,_constructor)
{
	assert_and_throw(argslen<=1);
	XMLList* th=obj.as<XMLList>();
	if(argslen==0 && th->constructed)
	{
		//Called from internal code
		return;
	}
	if(argslen==0 ||
	   args[0].is<Null>() || 
	   args[0].is<Undefined>())
	{
		th->constructed=true;
		return;
	}
	else if(args[0].is<XML>())
	{
		ASATOM_INCREF(args[0]);
		th->append(_MR(args[0].as<XML>()));
	}
	else if(args[0].is<XMLList>())
	{
		ASATOM_INCREF(args[0]);
		th->append(_MR(args[0].as<XMLList>()));
	}
	else if(args[0].is<ASString>() ||
		args[0].is<Number>() ||
		args[0].is<Integer>() ||
		args[0].is<UInteger>() ||
		args[0].is<Boolean>())
	{
		th->buildFromString(args[0].toString(sys));
	}
	else
	{
		throw RunTimeException("Type not supported in XMLList()");
	}
}

void XMLList::buildFromString(const tiny_string &str)
{
	pugi::xml_document xmldoc;

	pugi::xml_parse_result res = xmldoc.load_buffer((void*)str.raw_buf(),str.numBytes(),XML::getParseMode());
	switch (res.status)
	{
		case pugi::status_ok:
			break;
		case pugi::status_end_element_mismatch:
			throwError<TypeError>(kXMLUnterminatedElementTag);
			break;
		case pugi::status_unrecognized_tag:
			throwError<TypeError>(kXMLMalformedElement);
			break;
		case pugi::status_bad_pi:
			throwError<TypeError>(kXMLUnterminatedXMLDecl);
			break;
		case pugi::status_bad_attribute:
			throwError<TypeError>(kXMLUnterminatedAttribute);
			break;
		case pugi::status_bad_cdata:
			throwError<TypeError>(kXMLUnterminatedCData);
			break;
		case pugi::status_bad_doctype:
			throwError<TypeError>(kXMLUnterminatedDocTypeDecl);
			break;
		case pugi::status_bad_comment:
			throwError<TypeError>(kXMLUnterminatedComment);
			break;
		default:
			LOG(LOG_ERROR,"xmllist parser error:"<<str<<" "<<res.status<<" "<<res.description());
			break;
	}
	
	pugi::xml_node_iterator it=xmldoc.begin();
	for(;it!=xmldoc.end();++it)
	{
		_R<XML> tmp = _MR(XML::createFromNode(*it,(XML*)NULL,true));
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

ASFUNCTIONBODY_ATOM(XMLList,_getLength)
{
	XMLList* th=obj.as<XMLList>();
	assert_and_throw(argslen==0);
	ret.setInt((int32_t)th->nodes.size());
}

ASFUNCTIONBODY_ATOM(XMLList,_hasSimpleContent)
{
	XMLList* th=obj.as<XMLList>();
	assert_and_throw(argslen==0);
	ret.setBool(th->hasSimpleContent());
}

ASFUNCTIONBODY_ATOM(XMLList,_hasComplexContent)
{
	XMLList* th=obj.as<XMLList>();
	assert_and_throw(argslen==0);
	ret.setBool(th->hasComplexContent());
}

ASFUNCTIONBODY_ATOM(XMLList,generator)
{
	if(argslen==0)
	{
		ret = asAtom::fromObject(Class<XMLList>::getInstanceSNoArgs(getSys()));
	}
	else if(args[0].is<ASString>() ||
		args[0].is<Number>() ||
		args[0].is<Integer>() ||
		args[0].is<UInteger>() ||
		args[0].is<Boolean>())
	{
		ret = asAtom::fromObject(Class<XMLList>::getInstanceS(getSys(),args[0].toString(sys)));
	}
	else if(args[0].is<XMLList>())
	{
		ASATOM_INCREF(args[0]);
		ret = args[0];
	}
	else if(args[0].is<XML>())
	{
		XML::XMLVector nodes;
		ASATOM_INCREF(args[0]);
		nodes.push_back(_MR(args[0].as<XML>()));
		ret = asAtom::fromObject(Class<XMLList>::getInstanceS(getSys(),nodes));
	}
	else if(args[0].type ==T_NULL ||
		args[0].type ==T_UNDEFINED)
	{
		ret = asAtom::fromObject(Class<XMLList>::getInstanceSNoArgs(getSys()));
	}
	else
		throw RunTimeException("Type not supported in XMLList()");
}

ASFUNCTIONBODY_ATOM(XMLList,descendants)
{
	XMLList* th=obj.as<XMLList>();
	_NR<ASObject> name;
	ARG_UNPACK_ATOM(name,_NR<ASObject>(abstract_s(sys,"*")));
	XML::XMLVector res;
	multiname mname(NULL);
	name->applyProxyProperty(mname);
	th->getDescendantsByQName(name->toString(),BUILTIN_STRINGS::EMPTY,mname.isAttribute,res);
	ret = asAtom::fromObject(create(sys,res,th->targetobject,multiname(NULL)));
}

ASFUNCTIONBODY_ATOM(XMLList,elements)
{
	XMLList* th=obj.as<XMLList>();
	tiny_string name;
	ARG_UNPACK_ATOM(name, "");

	XML::XMLVector elems;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getElementNodes(name, elems);
	}
	ret = asAtom::fromObject(create(sys,elems,th->targetobject,multiname(NULL)));
}

ASFUNCTIONBODY_ATOM(XMLList,parent)
{
	XMLList* th=obj.as<XMLList>();

	if(th->nodes.size()==0)
	{
		ret.setUndefined();
		return;
	}

	auto it=th->nodes.begin();
	ASObject *parent=(*it)->getParentNode();
	++it;

	for(; it!=th->nodes.end(); ++it)
	{
		ASObject *otherParent=(*it)->getParentNode();
		if(!parent->isEqual(otherParent))
		{
			ret.setUndefined();
			return;
		}
	}

	ret = asAtom::fromObject(parent);
}

ASFUNCTIONBODY_ATOM(XMLList,valueOf)
{
	ASATOM_INCREF(obj);
	ret = obj;
}

ASFUNCTIONBODY_ATOM(XMLList,child)
{
	XMLList* th=obj.as<XMLList>();
	assert_and_throw(argslen==1);
	XML::XMLVector res;
	if(args[0].is<Number>() ||
		args[0].is<Integer>() ||
		args[0].is<UInteger>())
	{
		uint32_t index =args[0].toUInt();
		auto it=th->nodes.begin();
		for(; it!=th->nodes.end(); ++it)
		{
			(*it)->childrenImpl(res, index);
		}
	}
	else
	{
		const tiny_string& arg0=args[0].toString(sys);
		auto it=th->nodes.begin();
		for(; it!=th->nodes.end(); ++it)
		{
			(*it)->childrenImpl(res, arg0);
		}
	}
	ret = asAtom::fromObject(create(sys,res,th->targetobject,multiname(NULL)));
}

ASFUNCTIONBODY_ATOM(XMLList,children)
{
	XMLList* th=obj.as<XMLList>();
	assert_and_throw(argslen==0);
	XML::XMLVector res;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->childrenImpl(res, "*");
	}
	ret = asAtom::fromObject(create(sys,res,th->targetobject,multiname(NULL)));
}

ASFUNCTIONBODY_ATOM(XMLList,text)
{
	XMLList* th=obj.as<XMLList>();
	ARG_UNPACK_ATOM;
	XML::XMLVector res;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getText(res);
	}
	ret = asAtom::fromObject(create(sys,res,th->targetobject,multiname(NULL)));
}

ASFUNCTIONBODY_ATOM(XMLList,contains)
{
	XMLList* th=obj.as<XMLList>();
	_NR<ASObject> value;
	ARG_UNPACK_ATOM (value);
	if(!value->is<XML>())
	{
		ret.setBool(false);
		return;
	}

	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		if((*it)->isEqual(value.getPtr()))
		{
			ret.setBool(true);
			return;
		}
	}

	ret.setBool(false);
}

ASFUNCTIONBODY_ATOM(XMLList,copy)
{
	XMLList* th=obj.as<XMLList>();
	XMLList *dest = Class<XMLList>::getInstanceSNoArgs(sys);
	dest->targetobject = th->targetobject;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		dest->nodes.push_back(_MR((*it)->copy()));
	}
	ret = asAtom::fromObject(dest);
}

ASFUNCTIONBODY_ATOM(XMLList,attribute)
{
	XMLList* th=obj.as<XMLList>();

	if(argslen > 0 && args[0].is<QName>())
		LOG(LOG_NOT_IMPLEMENTED,"XMLList.attribute called with QName");

	tiny_string attrname;
	ARG_UNPACK_ATOM (attrname);
	multiname mname(NULL);
	mname.name_type=multiname::NAME_STRING;
	mname.name_s_id=sys->getUniqueStringId(attrname);
	mname.ns.emplace_back(sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
	mname.isAttribute = true;

	th->getVariableByMultiname(ret,mname, NONE);
	assert(ret.type != T_INVALID);
	ASATOM_INCREF(ret);
}

ASFUNCTIONBODY_ATOM(XMLList,attributes)
{
	XMLList* th=obj.as<XMLList>();
	XMLList *res = Class<XMLList>::getInstanceSNoArgs(sys);
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		XML::XMLVector nodeAttributes = (*it)->getAttributes();
		res->nodes.insert(res->nodes.end(), nodeAttributes.begin(), nodeAttributes.end());
	}
	ret = asAtom::fromObject(res);
}

ASFUNCTIONBODY_ATOM(XMLList,comments)
{
	XMLList* th=obj.as<XMLList>();

	XMLList *res = Class<XMLList>::getInstanceSNoArgs(sys);
	XML::XMLVector nodecomments;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getComments(nodecomments);
	}
	res->nodes.insert(res->nodes.end(), nodecomments.begin(), nodecomments.end());
	ret = asAtom::fromObject(res);
}
ASFUNCTIONBODY_ATOM(XMLList,processingInstructions)
{
	XMLList* th=obj.as<XMLList>();
	tiny_string name;
	ARG_UNPACK_ATOM(name,"*");

	XMLList *res = Class<XMLList>::getInstanceSNoArgs(sys);
	XML::XMLVector nodeprocessingInstructions;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getprocessingInstructions(nodeprocessingInstructions,name);
	}
	res->nodes.insert(res->nodes.end(), nodeprocessingInstructions.begin(), nodeprocessingInstructions.end());
	ret = asAtom::fromObject(res);
}
ASFUNCTIONBODY_ATOM(XMLList,_propertyIsEnumerable)
{
	XMLList* th=obj.as<XMLList>();
	if (argslen == 1)
	{
		int32_t n = args[0].toInt();
		ret.setBool(n < (int32_t)th->nodes.size());
	}
	else
		ret.setBool(false);
}
ASFUNCTIONBODY_ATOM(XMLList,_hasOwnProperty)
{
	XMLList* th=obj.as<XMLList>();
	tiny_string prop;
	ARG_UNPACK_ATOM(prop);

	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=args[0].toStringId(sys);
	name.ns.emplace_back(sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.ns.emplace_back(sys,BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	name.isAttribute=false;

	unsigned int index=0;
	if(XML::isValidMultiname(sys,name,index))
	{
		ret.setBool(index<th->nodes.size());
		return;
	}

	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		if ((*it)->hasProperty(name,true, true, true))
		{
			ret.setBool(true);
			return;
		}
	}
	ret.setBool(false);
}

ASFUNCTIONBODY_ATOM(XMLList,_normalize)
{
	XMLList* th=obj.as<XMLList>();
	th->normalize();
	th->incRef();
	ret = asAtom::fromObject(th);
}
void XMLList::normalize()
{
	auto it=nodes.begin();
	while (it!=nodes.end())
	{
		if ((*it)->getNodeKind() == pugi::node_element)
		{
			(*it)->normalize();
			++it;
		}
		else if ((*it)->getNodeKind() == pugi::node_pcdata)
		{
			if (XMLBase::removeWhitespace((*it)->toString()).empty())
			{
				it = nodes.erase(it);
			}
			else
			{
				_R<XML> textnode = *it;

				++it;
				while (it!=nodes.end() && (*it)->getNodeKind() == pugi::node_pcdata)
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

void XMLList::removeNode(XML *node)
{
	XMLList::XMLListVector::iterator it = nodes.end();
	while (it != nodes.begin())
	{
		it--;
		_R<XML> n = *it;
		if (n.getPtr() == node)
		{
			node->parentNode = NullRef;
			nodes.erase(it);
			break;
		}
	}
}
void XMLList::getTargetVariables(const multiname& name,XML::XMLVector& retnodes)
{
	unsigned int index=0;
	if(XML::isValidMultiname(getSystemState(),name,index))
	{
		retnodes.push_back(nodes[index]);
	}
	else
	{
		tiny_string normalizedName=name.normalizedName(getSystemState());
		
		//Only the first namespace is used, is this right?
		uint32_t namespace_uri = BUILTIN_STRINGS::EMPTY;
		if(name.ns.size() > 0 && !name.hasEmptyNS)
		{
			if (name.ns[0].kind==NAMESPACE)
				namespace_uri=name.ns[0].nsNameId;
		}
		
		// namespace set by "default xml namespace = ..."
		if(namespace_uri==BUILTIN_STRINGS::EMPTY)
			namespace_uri=getVm(getSystemState())->getDefaultXMLNamespaceID();

		for (uint32_t i = 0; i < nodes.size(); i++)
		{
			_R<XML> child= nodes[i];
			bool nameMatches = (normalizedName=="" || normalizedName==child->nodename  || normalizedName=="*");
			bool nsMatches = (namespace_uri==BUILTIN_STRINGS::EMPTY || 
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

GET_VARIABLE_RESULT XMLList::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
	{
		GET_VARIABLE_RESULT res = getVariableByMultinameIntern(ret,name,this->getClass(),opt);

		//If a method is not found on XMLList object and this
		//is a single element list with simple content,
		//delegate to ASString
		if(ret.type == T_INVALID && nodes.size()==1 && nodes[0]->hasSimpleContent())
		{
			ASObject *contentstr=abstract_s(getSystemState(),nodes[0]->toString_priv());
			contentstr->getVariableByMultiname(ret,name,opt);
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
			asAtom o;
			(*it)->getVariableByMultiname(o,name,opt);
			if(o.getObject() ==NULL || !o.getObject()->is<XMLList>())
				continue;

			retnodes.insert(retnodes.end(), o.getObject()->as<XMLList>()->nodes.begin(), o.getObject()->as<XMLList>()->nodes.end());
		}

		if(retnodes.size()==0 && (opt & FROM_GETLEX)!=0)
			return GET_VARIABLE_RESULT::GETVAR_NORMAL;

		ret = asAtom::fromObject(create(getSystemState(),retnodes,this,name));
		return GET_VARIABLE_RESULT::GETVAR_NORMAL;
	}
	unsigned int index=0;
	if(XML::isValidMultiname(getSystemState(),name,index))
	{
		if(index<nodes.size())
		{
			nodes[index]->incRef();
			ret = asAtom::fromObject(nodes[index].getPtr());
		}
		else
			ret.setUndefined();
	}
	else
	{
		XML::XMLVector retnodes;
		auto it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			asAtom o;
			(*it)->getVariableByMultiname(o,name,opt);
			if(o.getObject() == NULL || !o.getObject()->is<XMLList>())
				continue;

			retnodes.insert(retnodes.end(), o.getObject()->as<XMLList>()->nodes.begin(), o.getObject()->as<XMLList>()->nodes.end());
		}

		if(retnodes.size()==0 && (opt & FROM_GETLEX)!=0)
			return GET_VARIABLE_RESULT::GETVAR_NORMAL;

		ret = asAtom::fromObject(create(getSystemState(),retnodes,this,name));
	}
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}

bool XMLList::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
	if (!isConstructed())
		return false;

	unsigned int index=0;
	if(XML::isValidMultiname(getSystemState(),name,index))
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

void XMLList::setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst)
{
	setVariableByMultinameIntern(name, o, allowConst, false);
}
void XMLList::setVariableByMultinameIntern(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool replacetext)
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
		if (tmplist)
		{
			tmplist->getTargetVariables(tmpprop,retnodes);
		}
	}
	if(XML::isValidMultiname(getSystemState(),name,index))
	{
		if (index >= nodes.size())
		{
			if (targetobject)
				targetobject->appendSingleNode(o.toObject(getSystemState()));
			appendSingleNode(o.toObject(getSystemState()));
		}
		else
		{
			replace(index, o.toObject(getSystemState()),retnodes,allowConst,replacetext);
		}
	}
	else if (nodes.size() == 0)
	{
		if (tmplist)
		{
			if (tmplist->nodes.size() > 1)
				throwError<TypeError>(kXMLAssigmentOneItemLists);
			if (!tmpprop.isEmpty())
			{
				XML* tmp = Class<XML>::getInstanceSNoArgs(getSystemState());
				tmp->nodetype = pugi::node_element;
				tmp->nodename = targetproperty.normalizedName(getSystemState());
				tmp->attributelist = _MR(Class<XMLList>::getInstanceSNoArgs(getSystemState()));
				tmp->constructed = true;
				tmp->setVariableByMultiname(name,o,allowConst);
				tmp->incRef();
				tiny_string tmpname = tmpprop.normalizedName(getSystemState());
				if (retnodes.empty() && tmpname != "" && tmpname != "*")
				{
					XML* tmp2 = Class<XML>::getInstanceSNoArgs(getSystemState());
					tmp2->nodetype = pugi::node_element;
					tmp2->nodename = tmpname;
					tmp2->attributelist = _MR(Class<XMLList>::getInstanceSNoArgs(getSystemState()));
					tmp2->constructed = true;
					asAtom v = asAtom::fromObject(tmp);
					tmp2->setVariableByMultiname(targetproperty,v,allowConst);
					tmp2->incRef();
					tmplist->appendSingleNode(tmp2);
					appendSingleNode(tmp2);
				}
				else
				{
					asAtom v = asAtom::fromObject(tmp);
					tmplist->setVariableByMultinameIntern(tmpprop,v,allowConst, replacetext);
				}
			}
			else
			{
				tmplist->appendSingleNode(o.toObject(getSystemState()));
				appendSingleNode(o.toObject(getSystemState()));
			}
		}
		else
			appendSingleNode(o.toObject(getSystemState()));
	}
	else if (nodes.size() == 1)
	{
		nodes[0]->setVariableByMultinameIntern(name, o, allowConst,replacetext);
	}
	else
	{
		throwError<TypeError>(kXMLAssigmentOneItemLists);
	}
}

bool XMLList::deleteVariableByMultiname(const multiname& name)
{
	unsigned int index=0;
	bool bdeleted = false;
	
	if(XML::isValidMultiname(getSystemState(),name,index))
	{
		_R<XML> node = nodes[index];
		if (!node->parentNode.isNull() && node->parentNode->childrenlist.getPtr() != this)
		{
			// the node to remove is also added to another list, so it has to be deleted there, too
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
						bdeleted = true;
						break;
					}
				}
			}
		}
		this->nodes.erase(this->nodes.begin()+index);
		bdeleted = true;
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

void XMLList::getDescendantsByQName(const tiny_string& name, uint32_t ns, bool bIsAttribute, XML::XMLVector& ret)
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
		if ((*it)->nodetype == pugi::node_element)
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
		append(_MR(XML::createFromString(this->getSystemState(),str)));
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

void XMLList::replace(unsigned int idx, ASObject *o, const XML::XMLVector &retnodes,CONST_ALLOWED_FLAG allowConst, bool replacetext)
{
	if (idx >= nodes.size())
		return;

	if (nodes[idx]->isAttribute)
	{
		if (targetobject)
		{
			asAtom v = asAtom::fromObject(o);
			targetobject->setVariableByMultinameIntern(targetproperty,v,allowConst,replacetext);
		}
		nodes[idx]->setTextContent(o->toString());
	}
	if (o->is<XMLList>())
	{
		if (o->as<XMLList>()->nodes.size() == 1)
		{
			replace(idx,o->as<XMLList>()->nodes[0].getPtr(),retnodes,allowConst,replacetext);
			return;
		}
		if (targetobject)
		{
			for (uint32_t i = 0; i < targetobject->nodes.size(); i++)
			{
				XML* n= targetobject->nodes[i].getPtr();
				if (n == nodes[idx].getPtr())
				{
					multiname m(NULL);
					m.name_type = multiname::NAME_UINT;
					m.name_ui = i;
					m.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
					asAtom v = asAtom::fromObject(o);
					targetobject->setVariableByMultinameIntern(m,v,allowConst,replacetext);
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
			m.name_type = multiname::NAME_UINT;
			m.name_ui = idx;
			m.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
			asAtom v = asAtom::fromObject(o);
			targetobject->setVariableByMultinameIntern(m,v,allowConst,replacetext);
		}
		if (o->as<XML>()->getNodeKind() == pugi::node_pcdata)
		{
			if (replacetext)
			{
				nodes[idx]->childrenlist->clear();
				nodes[idx]->nodetype = pugi::node_pcdata;
				nodes[idx]->nodename = "text";
				nodes[idx]->nodevalue = o->toString();
				nodes[idx]->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
				nodes[idx]->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
			}
			else
			{
				nodes[idx]->childrenlist->clear();
				_R<XML> tmp = _MR<XML>(Class<XML>::getInstanceSNoArgs(getSystemState()));
				nodes[idx]->incRef();
				tmp->parentNode = nodes[idx];
				tmp->nodetype = pugi::node_pcdata;
				tmp->nodename = "text";
				tmp->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
				tmp->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
				tmp->nodevalue = o->toString();
				tmp->constructed = true;
				nodes[idx]->childrenlist->append(tmp);
			}
		}
		else
		{
			o->incRef();
			nodes[idx] = _MR(o->as<XML>());
		}
	}
	else
	{
		if (replacetext)
		{
			nodes[idx]->childrenlist->clear();
			nodes[idx]->nodetype = pugi::node_pcdata;
			nodes[idx]->nodename = "text";
			nodes[idx]->nodevalue = o->toString();
			nodes[idx]->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
			nodes[idx]->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
		}
		else
		{
			if (nodes[idx]->nodetype == pugi::node_pcdata)
				nodes[idx]->nodevalue = o->toString();
			else 
			{
				nodes[idx]->childrenlist->clear();
				_R<XML> tmp = _MR<XML>(Class<XML>::getInstanceSNoArgs(getSystemState()));
				nodes[idx]->incRef();
				tmp->parentNode = nodes[idx];
				tmp->nodetype = pugi::node_pcdata;
				tmp->nodename = "text";
				tmp->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
				tmp->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
				tmp->nodevalue = o->toString();
				tmp->constructed = true;
				nodes[idx]->childrenlist->append(tmp);
			}
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
			if (nodes[i]->isAttribute)
				ret+=nodes[i]->toString_priv();
			else
			{
				pugi::xml_node_type kind=nodes[i]->getNodeKind();
				switch (kind)
				{
					case pugi::node_comment:
					case pugi::node_pi:
						break;
					default:
						ret+=nodes[i]->toString_priv();
						break;
				}
				
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
int64_t XMLList::toInt64()
{
	if (!hasSimpleContent())
		return 0;

	tiny_string str = toString_priv();
	number_t value;
	bool valid=Integer::fromStringFlashCompatible(str.raw_buf(), value, 0);
	if (!valid)
		return 0;
	return value;
}
number_t XMLList::toNumber()
{
	if (!hasSimpleContent())
		return 0;
	return parseNumber(toString_priv());
}

ASFUNCTIONBODY_ATOM(XMLList,_toString)
{
	XMLList* th=obj.as<XMLList>();
	ret = asAtom::fromObject(abstract_s(sys,th->toString_priv()));
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

ASFUNCTIONBODY_ATOM(XMLList,toXMLString)
{
	XMLList* th=obj.as<XMLList>();
	assert_and_throw(argslen==0);
	ASObject* res=abstract_s(sys,th->toXMLString_internal());
	ret = asAtom::fromObject(res);
}

bool XMLList::isEqual(ASObject* r)
{
	if (!isConstructed())
		return !r->isConstructed() || r->getObjectType() == T_NULL || r->getObjectType() == T_UNDEFINED;

	if(nodes.size()==0 && r->getObjectType()==T_UNDEFINED)
		return true;

	if(r->is<XMLList>())
	{
		if(nodes.size()!=r->as<XMLList>()->nodes.size())
			return false;

		for(unsigned int i=0; i<nodes.size(); i++)
			if(!nodes[i]->isEqual(r->as<XMLList>()->nodes[i].getPtr()))
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

void XMLList::nextName(asAtom& ret,uint32_t index)
{
	if(index<=nodes.size())
		ret.setUInt(index-1);
	else
		throw RunTimeException("XMLList::nextName out of bounds");
}

void XMLList::nextValue(asAtom& ret,uint32_t index)
{
	if(index<=nodes.size())
	{
		nodes[index-1]->incRef();
		ret = asAtom::fromObject(nodes[index-1].getPtr());
	}
	else
		throw RunTimeException("XMLList::nextValue out of bounds");
}

void XMLList::appendNodesTo(XML *dest) const
{
	std::vector<_R<XML>, reporter_allocator<_R<XML>>>::const_iterator it;
	for (it=nodes.begin(); it!=nodes.end(); ++it)
	{
		asAtom arg0=asAtom::fromObject(it->getPtr());
		asAtom obj = asAtom::fromObject(dest);
		asAtom ret;
		XML::_appendChild(ret,this->getSystemState(),obj, &arg0, 1);
		ASATOM_DECREF(ret);
	}
}

void XMLList::prependNodesTo(XML *dest) const
{
	std::vector<_R<XML>, reporter_allocator<_R<XML>>>::const_reverse_iterator it;
	for (it=nodes.rbegin(); it!=nodes.rend(); ++it)
	{
		asAtom arg0=asAtom::fromObject(it->getPtr());
		asAtom obj = asAtom::fromObject(dest);
		asAtom ret;
		XML::_prependChild(ret,this->getSystemState(),obj, &arg0, 1);
		ASATOM_DECREF(ret);
	}
}
