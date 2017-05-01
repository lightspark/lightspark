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
	ASObject* XMLList::name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		XMLList* th=obj->as<XMLList>(); \
		if(!th) \
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Function applied to wrong object"); \
		if(th->nodes.size()==1) \
			return XML::name(th->nodes[0].getPtr(), args, argslen); \
		else \
			throwError<TypeError>(kXMLOnlyWorksWithOneItemLists, #name); \
		return NULL; \
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

ASFUNCTIONBODY(XMLList,_getLength)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_i(obj->getSystemState(),th->nodes.size());
}

ASFUNCTIONBODY(XMLList,_hasSimpleContent)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_b(obj->getSystemState(),th->hasSimpleContent());
}

ASFUNCTIONBODY(XMLList,_hasComplexContent)
{
	XMLList* th=Class<XMLList>::cast(obj);
	assert_and_throw(argslen==0);
	return abstract_b(obj->getSystemState(),th->hasComplexContent());
}

ASFUNCTIONBODY(XMLList,generator)
{
	assert(obj==NULL);
	if(argslen==0)
	{
		return Class<XMLList>::getInstanceSNoArgs(getSys());
	}
	else if(args[0]->is<ASString>() ||
		args[0]->is<Number>() ||
		args[0]->is<Integer>() ||
		args[0]->is<UInteger>() ||
		args[0]->is<Boolean>())
	{
		return Class<XMLList>::getInstanceS(args[0]->getSystemState(),args[0]->toString());
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
		return Class<XMLList>::getInstanceS(args[0]->getSystemState(),nodes);
	}
	else if(args[0]->getObjectType()==T_NULL ||
		args[0]->getObjectType()==T_UNDEFINED)
	{
		return Class<XMLList>::getInstanceSNoArgs(args[0]->getSystemState());
	}
	else
		throw RunTimeException("Type not supported in XMLList()");
}

ASFUNCTIONBODY(XMLList,descendants)
{
	XMLList* th=Class<XMLList>::cast(obj);
	_NR<ASObject> name;
	ARG_UNPACK(name,_NR<ASObject>(abstract_s(obj->getSystemState(),"*")));
	XML::XMLVector ret;
	multiname mname(NULL);
	name->applyProxyProperty(mname);
	th->getDescendantsByQName(name->toString(),BUILTIN_STRINGS::EMPTY,mname.isAttribute,ret);
	return create(obj->getSystemState(),ret,th->targetobject,multiname(NULL));
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
	return create(obj->getSystemState(),elems,th->targetobject,multiname(NULL));
}

ASFUNCTIONBODY(XMLList,parent)
{
	XMLList* th=Class<XMLList>::cast(obj);

	if(th->nodes.size()==0)
		return obj->getSystemState()->getUndefinedRef();

	auto it=th->nodes.begin();
	ASObject *parent=(*it)->getParentNode();
	++it;

	for(; it!=th->nodes.end(); ++it)
	{
		ASObject *otherParent=(*it)->getParentNode();
		if(!parent->isEqual(otherParent))
			return obj->getSystemState()->getUndefinedRef();
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
	return create(obj->getSystemState(),ret,th->targetobject,multiname(NULL));
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
	return create(obj->getSystemState(),ret,th->targetobject,multiname(NULL));
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
	return create(obj->getSystemState(),ret,th->targetobject,multiname(NULL));
}

ASFUNCTIONBODY(XMLList,contains)
{
	XMLList* th = obj->as<XMLList>();
	_NR<ASObject> value;
	ARG_UNPACK (value);
	if(!value->is<XML>())
		return abstract_b(obj->getSystemState(),false);

	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		if((*it)->isEqual(value.getPtr()))
			return abstract_b(obj->getSystemState(),true);
	}

	return abstract_b(obj->getSystemState(),false);
}

ASFUNCTIONBODY(XMLList,copy)
{
	XMLList* th = obj->as<XMLList>();
	XMLList *dest = Class<XMLList>::getInstanceSNoArgs(obj->getSystemState());
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
	mname.name_s_id=obj->getSystemState()->getUniqueStringId(attrname);
	mname.ns.emplace_back(obj->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	mname.isAttribute = true;

	_NR<ASObject> attr=th->getVariableByMultiname(mname, NONE);
	assert(!attr.isNull());
	attr->incRef();
	return attr.getPtr();
}

ASFUNCTIONBODY(XMLList,attributes)
{
	XMLList *th = obj->as<XMLList>();
	XMLList *res = Class<XMLList>::getInstanceSNoArgs(obj->getSystemState());
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

	XMLList *res = Class<XMLList>::getInstanceSNoArgs(obj->getSystemState());
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

	XMLList *res = Class<XMLList>::getInstanceSNoArgs(obj->getSystemState());
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
		return abstract_b(obj->getSystemState(),n < (int32_t)th->nodes.size());
		
	}
	return abstract_b(obj->getSystemState(),false);
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
		if(name.ns.size() > 0 && !name.ns[0].hasEmptyName())
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
			ASString *contentstr=abstract_s(getSystemState(),nodes[0]->toString_priv());
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
			if(!o->is<XMLList>())
				continue;

			retnodes.insert(retnodes.end(), o->as<XMLList>()->nodes.begin(), o->as<XMLList>()->nodes.end());
		}

		if(retnodes.size()==0 && (opt & XML_STRICT)!=0)
			return NullRef;

		return _MNR(create(getSystemState(),retnodes,this,name));
	}
	unsigned int index=0;
	if(XML::isValidMultiname(getSystemState(),name,index))
	{
		if(index<nodes.size())
			return nodes[index];
		else
			return _MNR(getSystemState()->getUndefinedRef());
	}
	else
	{
		XML::XMLVector retnodes;
		auto it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			_NR<ASObject> o=(*it)->getVariableByMultiname(name,opt);
			if(!o->is<XMLList>())
				continue;

			retnodes.insert(retnodes.end(), o->as<XMLList>()->nodes.begin(), o->as<XMLList>()->nodes.end());
		}

		if(retnodes.size()==0 && (opt & XML_STRICT)!=0)
			return NullRef;

		this->incRef();
		return _MNR(create(getSystemState(),retnodes,this,name));
	}
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

void XMLList::replace(unsigned int idx, ASObject *o, const XML::XMLVector &retnodes,CONST_ALLOWED_FLAG allowConst)
{
	if (idx >= nodes.size())
		return;

	if (nodes[idx]->isAttribute)
	{
		if (targetobject)
			targetobject->setVariableByMultiname(targetproperty,o,allowConst);
		nodes[idx]->setTextContent(o->toString());
	}
	if (o->is<XMLList>())
	{
		if (o->as<XMLList>()->nodes.size() == 1)
		{
			replace(idx,o->as<XMLList>()->nodes[0].getPtr(),retnodes,allowConst);
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
			m.name_type = multiname::NAME_UINT;
			m.name_ui = idx;
			m.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
			targetobject->setVariableByMultiname(m,o,allowConst);
		}
		if (o->as<XML>()->getNodeKind() == pugi::node_pcdata)
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
		else
		{
			o->incRef();
			nodes[idx] = _MR(o->as<XML>());
		}
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
	int64_t value;
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

ASFUNCTIONBODY(XMLList,_toString)
{
	XMLList* th=Class<XMLList>::cast(obj);
	return abstract_s(obj->getSystemState(),th->toString_priv());
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
	ASString* ret=abstract_s(obj->getSystemState(),th->toXMLString_internal());
	return ret;
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

_R<ASObject> XMLList::nextName(uint32_t index)
{
	if(index<=nodes.size())
		return _MR(abstract_i(getSystemState(),index-1));
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

void XMLList::prependNodesTo(XML *dest) const
{
	std::vector<_R<XML>, reporter_allocator<_R<XML>>>::const_reverse_iterator it;
	for (it=nodes.rbegin(); it!=nodes.rend(); ++it)
	{
		ASObject *arg0=it->getPtr();
		ASObject *ret=XML::_prependChild(dest, &arg0, 1);
		if(ret)
			ret->decRef();
	}
}
