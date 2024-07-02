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
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/toplevel.h"
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
	c->setDeclaredMethodByQName(#name,AS3,c->getSystemState()->getBuiltinFunction(name),NORMAL_METHOD,true)

#define REGISTER_XML_DELEGATE2(asname,cppname) \
	c->setDeclaredMethodByQName(#asname,AS3,c->getSystemState()->getBuiltinFunction(cppname),NORMAL_METHOD,true)

#define ASFUNCTIONBODY_XML_DELEGATE(name) \
	void XMLList::name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!asAtomHandler::is<XMLList>(obj)) {\
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; } \
		XMLList* th=asAtomHandler::as<XMLList>(obj); \
		if(th->nodes.size()==1) {\
			asAtom a = asAtomHandler::fromObject(th->nodes[0].getPtr()); \
			XML::name(ret,wrk,a, args, argslen); \
		} else \
			createError<TypeError>(wrk,kXMLOnlyWorksWithOneItemLists, #name); \
	}

XMLList::XMLList(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_XMLLIST),nodes(c->memoryAccount),constructed(false),targetobject(nullptr),targetproperty(c->memoryAccount)
{
}

XMLList::XMLList(ASWorker* wrk,Class_base* cb,bool c):ASObject(wrk,cb,T_OBJECT,SUBTYPE_XMLLIST),nodes(cb->memoryAccount),constructed(c),targetobject(nullptr),targetproperty(cb->memoryAccount)
{
	assert(c);
}

XMLList::XMLList(ASWorker* wrk,Class_base* c, const std::string& str):ASObject(wrk,c,T_OBJECT,SUBTYPE_XMLLIST),nodes(c->memoryAccount),constructed(true),targetobject(nullptr),targetproperty(c->memoryAccount)
{
	buildFromString(wrk,str);
}

XMLList::XMLList(ASWorker* wrk,Class_base* c, const XML::XMLVector& r):
	ASObject(wrk,c,T_OBJECT,SUBTYPE_XMLLIST),nodes(r.begin(),r.end(),c->memoryAccount),constructed(true),targetobject(nullptr),targetproperty(c->memoryAccount)
{
}
XMLList::XMLList(ASWorker* wrk,Class_base* c, const XML::XMLVector& r, XMLList *targetobject, const multiname &targetproperty):
	ASObject(wrk,c,T_OBJECT,SUBTYPE_XMLLIST),nodes(r.begin(),r.end(),c->memoryAccount),constructed(true),targetobject(targetobject),targetproperty(c->memoryAccount)
{
	if (targetobject)
		targetobject->incRef();
	this->targetproperty.name_type = targetproperty.name_type;
	this->targetproperty.isAttribute = targetproperty.isAttribute;
	this->targetproperty.name_s_id = targetproperty.name_s_id;
	this->targetproperty.hasEmptyNS = targetproperty.hasEmptyNS;
	for (auto it = targetproperty.ns.begin();it != targetproperty.ns.end(); it++)
	{
		this->targetproperty.ns.push_back(*it);
	}
}
XMLList* XMLList::create(ASWorker* wrk,const XML::XMLVector& r, XMLList *targetobject, const multiname &targetproperty)
{
	XMLList* res = Class<XMLList>::getInstanceSNoArgs(wrk);
	res->constructed = true;
	res->nodes.assign(r.begin(),r.end());
	
	res->targetobject = targetobject;
	if (res->targetobject)
	{
		res->targetobject->incRef();
		res->targetobject->addStoredMember();
	}
	res->targetproperty.name_type = targetproperty.name_type;
	res->targetproperty.isAttribute = targetproperty.isAttribute;
	res->targetproperty.name_s_id = targetproperty.name_s_id;
	res->targetproperty.hasEmptyNS = targetproperty.hasEmptyNS;
	for (auto it = targetproperty.ns.begin();it != targetproperty.ns.end(); it++)
	{
		res->targetproperty.ns.push_back(*it);
	}
	return res;
}

void XMLList::finalize()
{
	if (targetobject)
		targetobject->removeStoredMember();
	targetobject=nullptr;
	nodes.clear();
}
bool XMLList::destruct()
{
	if (targetobject)
		targetobject->removeStoredMember();
	nodes.clear();
	constructed = false;
	targetobject = nullptr;
	targetproperty = multiname(nullptr);
	return destructIntern();
}

void XMLList::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (targetobject)
		targetobject->prepareShutdown();
	for (auto it = nodes.begin(); it != nodes.end(); it++)
	{
		(*it)->prepareShutdown();
	}
}

bool XMLList::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.checkAncestors(this))
		return false;
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	if (targetobject)
		ret = targetobject->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
	
}

void XMLList::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	c->isReusable=true;
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(_getLength,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attribute",AS3,c->getSystemState()->getBuiltinFunction(attribute,1,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attributes",AS3,c->getSystemState()->getBuiltinFunction(attributes,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("child",AS3,c->getSystemState()->getBuiltinFunction(child,1,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("children",AS3,c->getSystemState()->getBuiltinFunction(children,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains",AS3,c->getSystemState()->getBuiltinFunction(contains,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("copy",AS3,c->getSystemState()->getBuiltinFunction(copy,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descendants",AS3,c->getSystemState()->getBuiltinFunction(descendants,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("elements",AS3,c->getSystemState()->getBuiltinFunction(elements,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("normalize",AS3,c->getSystemState()->getBuiltinFunction(_normalize,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parent",AS3,c->getSystemState()->getBuiltinFunction(parent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasSimpleContent",AS3,c->getSystemState()->getBuiltinFunction(_hasSimpleContent,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasComplexContent",AS3,c->getSystemState()->getBuiltinFunction(_hasComplexContent,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("valueOf","",c->getSystemState()->getBuiltinFunction(valueOf,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("valueOf",AS3,c->getSystemState()->getBuiltinFunction(valueOf,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toXMLString",AS3,c->getSystemState()->getBuiltinFunction(toXMLString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("text",AS3,c->getSystemState()->getBuiltinFunction(text,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("comments",AS3,c->getSystemState()->getBuiltinFunction(comments,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("processingInstructions",AS3,c->getSystemState()->getBuiltinFunction(processingInstructions,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("propertyIsEnumerable",AS3,c->getSystemState()->getBuiltinFunction(_propertyIsEnumerable,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("hasOwnProperty",AS3,c->getSystemState()->getBuiltinFunction(_hasOwnProperty,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
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

ASFUNCTIONBODY_XML_DELEGATE(addNamespace)
ASFUNCTIONBODY_XML_DELEGATE(_appendChild)
ASFUNCTIONBODY_XML_DELEGATE(childIndex)
ASFUNCTIONBODY_XML_DELEGATE(inScopeNamespaces)
ASFUNCTIONBODY_XML_DELEGATE(insertChildAfter)
ASFUNCTIONBODY_XML_DELEGATE(insertChildBefore)
ASFUNCTIONBODY_XML_DELEGATE(localName)
ASFUNCTIONBODY_XML_DELEGATE(name)
ASFUNCTIONBODY_XML_DELEGATE(_namespace)
ASFUNCTIONBODY_XML_DELEGATE(namespaceDeclarations)
ASFUNCTIONBODY_XML_DELEGATE(nodeKind)
ASFUNCTIONBODY_XML_DELEGATE(_prependChild)
ASFUNCTIONBODY_XML_DELEGATE(removeNamespace)
ASFUNCTIONBODY_XML_DELEGATE(_setChildren)
ASFUNCTIONBODY_XML_DELEGATE(_setLocalName)
ASFUNCTIONBODY_XML_DELEGATE(_setName)
ASFUNCTIONBODY_XML_DELEGATE(_setNamespace)

ASFUNCTIONBODY_ATOM(XMLList,_constructor)
{
	assert_and_throw(argslen<=1);
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	if(argslen==0 && th->constructed)
	{
		//Called from internal code
		return;
	}
	if(argslen==0 ||
	   asAtomHandler::is<Null>(args[0]) || 
	   asAtomHandler::is<Undefined>(args[0]))
	{
		th->constructed=true;
		return;
	}
	else if(asAtomHandler::is<XML>(args[0]))
	{
		ASATOM_INCREF(args[0]);
		th->append(_MNR(asAtomHandler::as<XML>(args[0])));
	}
	else if(asAtomHandler::is<XMLList>(args[0]))
	{
		ASATOM_INCREF(args[0]);
		th->append(_MNR(asAtomHandler::as<XMLList>(args[0])));
	}
	else if(asAtomHandler::isString(args[0]) ||
		asAtomHandler::is<Number>(args[0]) ||
		asAtomHandler::is<Integer>(args[0]) ||
		asAtomHandler::is<UInteger>(args[0]) ||
		asAtomHandler::is<Boolean>(args[0]))
	{
		th->buildFromString(wrk,asAtomHandler::toString(args[0],wrk));
	}
	else
	{
		throw RunTimeException("Type not supported in XMLList()");
	}
}

void XMLList::buildFromString(ASWorker* wrk, const tiny_string &str)
{
	pugi::xml_document xmldoc;

	pugi::xml_parse_result res = xmldoc.load_buffer((void*)str.raw_buf(),str.numBytes(),XML::getParseMode());
	switch (res.status)
	{
		case pugi::status_ok:
			break;
		case pugi::status_end_element_mismatch:
			createError<TypeError>(wrk,kXMLUnterminatedElementTag);
			return;
		case pugi::status_unrecognized_tag:
			createError<TypeError>(wrk,kXMLMalformedElement);
			return;
		case pugi::status_bad_pi:
			createError<TypeError>(getWorker(),kXMLUnterminatedProcessingInstruction);
			break;
		case pugi::status_bad_declaration:
			createError<TypeError>(getWorker(),kXMLUnterminatedXMLDecl);
			break;
		case pugi::status_bad_attribute:
			createError<TypeError>(wrk,kXMLUnterminatedAttribute);
			return;
		case pugi::status_bad_cdata:
			createError<TypeError>(wrk,kXMLUnterminatedCData);
			return;
		case pugi::status_bad_doctype:
			createError<TypeError>(wrk,kXMLUnterminatedDocTypeDecl);
			return;
		case pugi::status_bad_comment:
			createError<TypeError>(wrk,kXMLUnterminatedComment);
			return;
		default:
			LOG(LOG_ERROR,"xmllist parser error:"<<str<<" "<<res.status<<" "<<res.description());
			break;
	}
	
	pugi::xml_node_iterator it=xmldoc.begin();
	for(;it!=xmldoc.end();++it)
	{
		XML* tmp = XML::createFromNode(wrk,*it,(XML*)nullptr,true);
		if (tmp->constructed)
			nodes.push_back(_MNR(tmp));
		else
			tmp->decRef();
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

_NR<XML> XMLList::reduceToXML() const
{
	//Needed to convert XMLList to XML
	//Check if there is a single non null XML object. If not throw an exception
	if(nodes.size()==1)
		return nodes[0];
	else
	{
		createError<TypeError>(getInstanceWorker(),kXMLMarkupMustBeWellFormed);
		return NullRef;
	}
}

ASFUNCTIONBODY_ATOM(XMLList,_getLength)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	assert_and_throw(argslen==0);
	asAtomHandler::setInt(ret,wrk,(int32_t)th->nodes.size());
}

ASFUNCTIONBODY_ATOM(XMLList,_hasSimpleContent)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	assert_and_throw(argslen==0);
	asAtomHandler::setBool(ret,th->hasSimpleContent());
}

ASFUNCTIONBODY_ATOM(XMLList,_hasComplexContent)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	assert_and_throw(argslen==0);
	asAtomHandler::setBool(ret,th->hasComplexContent());
}

ASFUNCTIONBODY_ATOM(XMLList,generator)
{
	if(argslen==0)
	{
		ret = asAtomHandler::fromObject(Class<XMLList>::getInstanceSNoArgs(wrk));
	}
	else if(asAtomHandler::isString(args[0]) ||
		asAtomHandler::is<Number>(args[0]) ||
		asAtomHandler::is<Integer>(args[0]) ||
		asAtomHandler::is<UInteger>(args[0]) ||
		asAtomHandler::is<Boolean>(args[0]))
	{
		ret = asAtomHandler::fromObject(Class<XMLList>::getInstanceS(wrk,asAtomHandler::toString(args[0],wrk)));
	}
	else if(asAtomHandler::is<XMLList>(args[0]))
	{
		ASATOM_INCREF(args[0]);
		ret = args[0];
	}
	else if(asAtomHandler::is<XML>(args[0]))
	{
		XML::XMLVector nodes;
		ASATOM_INCREF(args[0]);
		nodes.push_back(_MNR(asAtomHandler::as<XML>(args[0])));
		ret = asAtomHandler::fromObject(Class<XMLList>::getInstanceS(wrk,nodes));
	}
	else if(asAtomHandler::isNull(args[0]) ||
		asAtomHandler::isUndefined(args[0]))
	{
		ret = asAtomHandler::fromObject(Class<XMLList>::getInstanceSNoArgs(wrk));
	}
	else
		throw RunTimeException("Type not supported in XMLList()");
}

ASFUNCTIONBODY_ATOM(XMLList,descendants)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	asAtom name = asAtomHandler::invalidAtom;
	asAtom defname = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_WILDCARD);
	ARG_CHECK(ARG_UNPACK(name,defname));
	XML::XMLVector res;
	multiname mname(nullptr);
	mname.name_s_id = asAtomHandler::toStringId(name,wrk);
	mname.name_type = multiname::NAME_STRING;
	ASObject* o = asAtomHandler::getObject(name);
	if (o)
		o->applyProxyProperty(mname);
	th->getDescendantsByQName(mname,res);
	ret = asAtomHandler::fromObject(create(wrk,res,th->targetobject,multiname(nullptr)));
}

ASFUNCTIONBODY_ATOM(XMLList,elements)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	asAtom name = asAtomHandler::invalidAtom;
	asAtom defname = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	ARG_CHECK(ARG_UNPACK (name, defname));
	uint32_t nameID = asAtomHandler::toStringId(name,wrk);

	XML::XMLVector elems;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getElementNodes(nameID, elems);
	}
	ret = asAtomHandler::fromObject(create(wrk,elems,th->targetobject,multiname(nullptr)));
}

ASFUNCTIONBODY_ATOM(XMLList,parent)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);

	if(th->nodes.size()==0)
	{
		asAtomHandler::setUndefined(ret);
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
			asAtomHandler::setUndefined(ret);
			return;
		}
	}

	ret = asAtomHandler::fromObject(parent);
}

ASFUNCTIONBODY_ATOM(XMLList,valueOf)
{
	ASATOM_INCREF(obj);
	ret = obj;
}

ASFUNCTIONBODY_ATOM(XMLList,child)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	asAtom propertyName = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK (propertyName));
	if (asAtomHandler::isNull(propertyName))
	{
		createError<TypeError>(wrk,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(propertyName))
	{
		createError<TypeError>(wrk,kConvertUndefinedToObjectError);
		return;
	}
	XML::XMLVector res;
	if(asAtomHandler::is<Number>(propertyName) ||
		asAtomHandler::is<Integer>(propertyName) ||
		asAtomHandler::is<UInteger>(propertyName))
	{
		uint32_t index =asAtomHandler::toUInt(propertyName);
		auto it=th->nodes.begin();
		for(; it!=th->nodes.end(); ++it)
		{
			(*it)->childrenImplIndex(res, index);
		}
	}
	else
	{
		uint32_t arg0=asAtomHandler::toStringId(propertyName,wrk);
		auto it=th->nodes.begin();
		for(; it!=th->nodes.end(); ++it)
		{
			(*it)->childrenImpl(res, arg0);
		}
	}
	ret = asAtomHandler::fromObject(create(wrk,res,th->targetobject,multiname(nullptr)));
}

ASFUNCTIONBODY_ATOM(XMLList,children)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	assert_and_throw(argslen==0);
	XML::XMLVector res;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->childrenImpl(res, BUILTIN_STRINGS::STRING_WILDCARD);
	}
	ret = asAtomHandler::fromObject(create(wrk,res,th->targetobject,multiname(nullptr)));
}

ASFUNCTIONBODY_ATOM(XMLList,text)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	XML::XMLVector res;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getText(res);
	}
	ret = asAtomHandler::fromObject(create(wrk,res,th->targetobject,multiname(nullptr)));
}

ASFUNCTIONBODY_ATOM(XMLList,contains)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	_NR<ASObject> value;
	ARG_CHECK(ARG_UNPACK (value));
	if(!value->is<XML>())
	{
		asAtomHandler::setBool(ret,false);
		return;
	}

	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		if((*it)->isEqual(value.getPtr()))
		{
			asAtomHandler::setBool(ret,true);
			return;
		}
	}

	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(XMLList,copy)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	XMLList *dest = Class<XMLList>::getInstanceSNoArgs(wrk);
	th->copy(dest);
	ret = asAtomHandler::fromObject(dest);
}
void XMLList::copy(XMLList* res, XML* parent)
{
	if (targetobject)
	{
		targetobject->incRef();
		targetobject->addStoredMember();
	}
	res->targetobject = targetobject;
	res->nodes.reserve(nodes.size());
	auto it=nodes.begin();
	for(; it!=nodes.end(); ++it)
	{
		if (XML::getIgnoreComments() && (*it)->nodetype == pugi::node_comment)
			continue;
		_NR<XML> node = _MR(Class<XML>::getInstanceSNoArgs(getInstanceWorker()));
		(*it)->copy(node.getPtr(),parent);
		res->nodes.push_back(node);
	}
}
ASFUNCTIONBODY_ATOM(XMLList,attribute)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);

	if(argslen > 0 && asAtomHandler::is<QName>(args[0]))
		LOG(LOG_NOT_IMPLEMENTED,"XMLList.attribute called with QName");

	tiny_string attrname;
	ARG_CHECK(ARG_UNPACK (attrname));
	if (asAtomHandler::isNull(args[0]))
	{
		createError<TypeError>(wrk,kConvertNullToObjectError);
		return;
	}
	if (asAtomHandler::isUndefined(args[0]))
	{
		createError<TypeError>(wrk,kConvertUndefinedToObjectError);
		return;
	}
	multiname mname(NULL);
	mname.name_type=multiname::NAME_STRING;
	mname.name_s_id=wrk->getSystemState()->getUniqueStringId(attrname);
	mname.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	mname.isAttribute = true;
	th->getVariableByMultiname(ret,mname, NONE,wrk);
	assert(asAtomHandler::isValid(ret));
}

ASFUNCTIONBODY_ATOM(XMLList,attributes)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	XMLList *res = Class<XMLList>::getInstanceSNoArgs(wrk);
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		XMLList* attrlist = (*it)->getAllAttributes();
		res->nodes.insert(res->nodes.end(), attrlist->nodes.begin(), attrlist->nodes.end());
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(XMLList,comments)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);

	XMLList *res = Class<XMLList>::getInstanceSNoArgs(wrk);
	XML::XMLVector nodecomments;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getComments(nodecomments);
	}
	res->nodes.insert(res->nodes.end(), nodecomments.begin(), nodecomments.end());
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(XMLList,processingInstructions)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	asAtom name = asAtomHandler::invalidAtom;
	asAtom defname = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_WILDCARD);
	ARG_CHECK(ARG_UNPACK (name, defname));
	uint32_t nameID = asAtomHandler::toStringId(name,wrk);

	XMLList *res = Class<XMLList>::getInstanceSNoArgs(wrk);
	XML::XMLVector nodeprocessingInstructions;
	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		(*it)->getprocessingInstructions(nodeprocessingInstructions,nameID);
	}
	res->nodes.insert(res->nodes.end(), nodeprocessingInstructions.begin(), nodeprocessingInstructions.end());
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(XMLList,_propertyIsEnumerable)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	asAtomHandler::setBool(ret,false);
	if (argslen == 1)
	{
		if (asAtomHandler::isInteger(args[0]))
		{
			int32_t n = asAtomHandler::toInt(args[0]);
			asAtomHandler::setBool(ret,n < (int32_t)th->nodes.size());
		}
		else
		{
			tiny_string s = asAtomHandler::toString(args[0],wrk);
			if (Array::isIntegerWithoutLeadingZeros(s))
			{
				int32_t n = asAtomHandler::toInt(args[0]);
				asAtomHandler::setBool(ret,n < (int32_t)th->nodes.size());
			}
		}
	}
}
ASFUNCTIONBODY_ATOM(XMLList,_hasOwnProperty)
{
	if (!asAtomHandler::is<XMLList>(obj))
	{
		ASObject::hasOwnProperty(ret,wrk,obj,args,argslen);
		return;
	}
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	tiny_string prop;
	ARG_CHECK(ARG_UNPACK(prop));

	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=asAtomHandler::toStringId(args[0],wrk);
	name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	name.isAttribute=false;

	unsigned int index=0;
	if(XML::isValidMultiname(wrk->getSystemState(),name,index))
	{
		asAtomHandler::setBool(ret,index<th->nodes.size());
		return;
	}

	auto it=th->nodes.begin();
	for(; it!=th->nodes.end(); ++it)
	{
		if ((*it)->hasProperty(name,true, true, true,wrk))
		{
			asAtomHandler::setBool(ret,true);
			return;
		}
	}
	asAtomHandler::setBool(ret,false);
}

ASFUNCTIONBODY_ATOM(XMLList,_normalize)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	th->normalize();
	th->incRef();
	ret = asAtomHandler::fromObject(th);
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
			if ((*it)->toString().isWhiteSpaceOnly())
			{
				it = nodes.erase(it);
			}
			else
			{
				_NR<XML> textnode = *it;

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
	auto it = nodes.end();
	while (it != nodes.begin())
	{
		it--;
		_NR<XML> n = *it;
		if (n.getPtr() == node)
		{
			node->parentNode = nullptr;
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
		uint32_t normalizedNameID=name.normalizedNameId(getSystemState());
		
		//Only the first namespace is used, is this right?
		uint32_t namespace_uri = BUILTIN_STRINGS::EMPTY;
		if(name.ns.size() > 0 && !name.hasEmptyNS)
		{
			if (name.ns[0].kind==NAMESPACE)
				namespace_uri=name.ns[0].nsNameId;
		}
		
		// namespace set by "default xml namespace = ..."
		if(namespace_uri==BUILTIN_STRINGS::EMPTY)
			namespace_uri=getInstanceWorker()->getDefaultXMLNamespaceID();

		for (uint32_t i = 0; i < nodes.size(); i++)
		{
			_NR<XML> child= nodes[i];
			bool nameMatches = (normalizedNameID==BUILTIN_STRINGS::EMPTY || normalizedNameID==child->nodenameID  || normalizedNameID==BUILTIN_STRINGS::STRING_WILDCARD);
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

GET_VARIABLE_RESULT XMLList::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
	{
		GET_VARIABLE_RESULT res = getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);

		//If a method is not found on XMLList object and this
		//is a single element list with simple content,
		//delegate to ASString
		if(asAtomHandler::isInvalid(ret) && nodes.size()==1 && nodes[0]->hasSimpleContent())
		{
			ASObject *contentstr=abstract_s(getInstanceWorker(),nodes[0]->toString_priv());
			contentstr->getVariableByMultiname(ret,name,opt,wrk);
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
			asAtom o=asAtomHandler::invalidAtom;
			(*it)->getVariableByMultiname(o,name,opt,wrk);
			if(asAtomHandler::is<XMLList>(o))
				retnodes.insert(retnodes.end(), asAtomHandler::as<XMLList>(o)->nodes.begin(), asAtomHandler::as<XMLList>(o)->nodes.end());
			ASATOM_DECREF(o);
		}

		if(retnodes.size()==0 && (opt & FROM_GETLEX)!=0)
			return GET_VARIABLE_RESULT::GETVAR_NORMAL;

		ret = asAtomHandler::fromObject(create(getInstanceWorker(),retnodes,this,name));
		return GET_VARIABLE_RESULT::GETVAR_ISNEWOBJECT;
	}
	unsigned int index=0;
	if(XML::isValidMultiname(getSystemState(),name,index))
	{
		if(index<nodes.size())
		{
			if ((opt & NO_INCREF)==0)
				nodes[index]->incRef();
			ret = asAtomHandler::fromObject(nodes[index].getPtr());
		}
		else
			asAtomHandler::setUndefined(ret);
	}
	else
	{
		XML::XMLVector retnodes;
		auto it=nodes.begin();
		for(; it!=nodes.end(); ++it)
		{
			asAtom o=asAtomHandler::invalidAtom;
			(*it)->getVariableByMultiname(o,name,opt,wrk);
			if(asAtomHandler::is<XMLList>(o))
				retnodes.insert(retnodes.end(), asAtomHandler::as<XMLList>(o)->nodes.begin(), asAtomHandler::as<XMLList>(o)->nodes.end());
			ASATOM_DECREF(o);
		}

		if(retnodes.size()==0 && (opt & FROM_GETLEX)!=0)
			return GET_VARIABLE_RESULT::GETVAR_NORMAL;

		ret = asAtomHandler::fromObject(create(getInstanceWorker(),retnodes,this,name));
		return GET_VARIABLE_RESULT::GETVAR_ISNEWOBJECT;
	}
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}
GET_VARIABLE_RESULT XMLList::getVariableByInteger(asAtom &ret, int index, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	if (index < 0)
		return getVariableByIntegerIntern(ret,index,opt,wrk);
	if(uint32_t(index)<nodes.size())
	{
		if ((opt & NO_INCREF)==0)
			nodes[index]->incRef();
		ret = asAtomHandler::fromObject(nodes[index].getPtr());
	}
	else
		asAtomHandler::setUndefined(ret);
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}

bool XMLList::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
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
			bool ret=(*it)->hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
			if(ret)
				return ret;
		}
	}

	return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
}

multiname *XMLList::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	return setVariableByMultinameIntern(name, o, allowConst, false,alreadyset,wrk);
}
void XMLList::setVariableByInteger(int index, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	if (index < 0)
	{
		setVariableByInteger_intern(index,o,allowConst,alreadyset,wrk);
		return;
	}
	*alreadyset=false;
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
	if (uint32_t(index) >= nodes.size())
	{
		if (targetobject)
		{
			ASATOM_INCREF(o);
			targetobject->appendSingleNode(o);
		}
		if (!appendSingleNode(o) && alreadyset)
			ASATOM_DECREF(o);
	}
	else
	{
		replace(index, o,retnodes,allowConst,false,alreadyset,wrk);
	}
}

multiname* XMLList::setVariableByMultinameIntern(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool replacetext,bool* alreadyset, ASWorker* wrk)
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
			{
				ASATOM_INCREF(o);
				targetobject->appendSingleNode(o);
			}
			if (!appendSingleNode(o) && alreadyset)
				ASATOM_DECREF(o);
		}
		else
		{
			replace(index, o,retnodes,allowConst,replacetext,alreadyset,wrk);
		}
	}
	else if (nodes.size() == 0)
	{
		if (tmplist)
		{
			if (!tmpprop.isEmpty())
			{
				XML* tmp = Class<XML>::getInstanceSNoArgs(getInstanceWorker());
				tmp->nodetype = pugi::node_element;
				tmp->nodenameID = targetproperty.normalizedNameId(getSystemState());
				tmp->attributelist = _MNR(Class<XMLList>::getInstanceSNoArgs(getInstanceWorker()));
				tmp->constructed = true;
				tmp->setVariableByMultiname(name,o,allowConst,nullptr,wrk);
				uint32_t tmpnameID = tmpprop.normalizedNameId(getSystemState());
				if (retnodes.empty() && tmpnameID != BUILTIN_STRINGS::EMPTY && tmpnameID != BUILTIN_STRINGS::STRING_WILDCARD)
				{
					XML* tmp2 = Class<XML>::getInstanceSNoArgs(getInstanceWorker());
					tmp2->nodetype = pugi::node_element;
					tmp2->nodenameID = tmpnameID;
					tmp2->attributelist = _MNR(Class<XMLList>::getInstanceSNoArgs(getInstanceWorker()));
					tmp2->constructed = true;
					asAtom v = asAtomHandler::fromObject(tmp);
					tmp2->setVariableByMultiname(targetproperty,v,allowConst,nullptr,wrk);
					tmp2->incRef();
					tmplist->appendSingleNode(asAtomHandler::fromObjectNoPrimitive(tmp2));
					appendSingleNode(asAtomHandler::fromObjectNoPrimitive(tmp2));
				}
				else
				{
					asAtom v = asAtomHandler::fromObject(tmp);
					tmplist->setVariableByMultinameIntern(tmpprop,v,allowConst, replacetext,alreadyset,wrk);
					if (getInstanceWorker()->currentCallContext->exceptionthrown)
						tmp->decRef();
				}
			}
			else
			{
				ASATOM_INCREF(o);
				tmplist->appendSingleNode(o);
				if (!appendSingleNode(o) && alreadyset)
					ASATOM_DECREF(o);
			}
		}
		else
		{
			if (!appendSingleNode(o) && alreadyset)
				ASATOM_DECREF(o);
		}
	}
	else if (nodes.size() == 1)
	{
		nodes[0]->setVariableByMultinameIntern(name, o, allowConst,replacetext,alreadyset,wrk);
	}
	else
	{
		createError<TypeError>(getInstanceWorker(),kXMLAssigmentOneItemLists);
	}
	return nullptr;
}

bool XMLList::deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
{
	unsigned int index=0;
	bool bdeleted = false;
	
	if(XML::isValidMultiname(getSystemState(),name,index))
	{
		_NR<XML> node = nodes[index];
		if (node->parentNode && node->parentNode->childrenlist.getPtr() != this)
		{
			// the node to remove is also added to another list, so it has to be deleted there, too
			if (node->parentNode)
			{
				auto it = node->parentNode->childrenlist->nodes.end();
				while (it != node->parentNode->childrenlist->nodes.begin())
				{
					it--;
					_NR<XML> n = *it;
					if (n.getPtr() == node.getPtr())
					{
						node->parentNode->childrenlist->nodes.erase(it);
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
		for (auto it = nodes.begin(); it != nodes.end(); it++)
		{
			_NR<XML> node = *it;
			if (node->deleteVariableByMultiname(name,wrk))
				bdeleted = true;
		}
	}
	return bdeleted;
}

void XMLList::getDescendantsByQName(const multiname& name, XML::XMLVector& ret)
{
	auto it=nodes.begin();
	for(; it!=nodes.end(); ++it)
	{
		(*it)->getDescendantsByQName(name, ret);
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

bool XMLList::appendSingleNode(asAtom x)
{
	if (asAtomHandler::is<XML>(x))
	{
		append(_MNR(asAtomHandler::as<XML>(x)));
	}
	else if (asAtomHandler::is<XMLList>(x))
	{
		XMLList *list = asAtomHandler::as<XMLList>(x);
		if (list->nodes.size() == 1)
		{
			append(list->nodes[0]);
		}
		else
			return false;
		// do nothing, if length != 1. See ECMA-357, Section
		// 9.2.1.2
	}
	else
	{
		tiny_string str = asAtomHandler::toString(x,getInstanceWorker());
		append(_MNR(XML::createFromString(getInstanceWorker(),str)));
		return false;
	}
	return true;
}

void XMLList::append(_NR<XML> x)
{
	nodes.push_back(x);
}

void XMLList::append(_NR<XMLList> x)
{
	nodes.insert(nodes.end(),x->nodes.begin(),x->nodes.end());
}

void XMLList::prepend(_NR<XML> x)
{
	nodes.insert(nodes.begin(),x);
}

void XMLList::prepend(_NR<XMLList> x)
{
	nodes.insert(nodes.begin(),x->nodes.begin(),x->nodes.end());
}

void XMLList::replace(unsigned int idx, asAtom o, const XML::XMLVector &retnodes, CONST_ALLOWED_FLAG allowConst, bool replacetext,bool* alreadyset, ASWorker* wrk)
{
	if (idx >= nodes.size())
		return;

	if (nodes[idx]->isAttribute)
	{
		if (targetobject)
		{
			ASATOM_INCREF(o);
			targetobject->setVariableByMultinameIntern(targetproperty,o,allowConst,replacetext,alreadyset,wrk);
		}
		nodes[idx]->setTextContent(asAtomHandler::toString(o,getInstanceWorker()));
	}
	if (asAtomHandler::is<XMLList>(o))
	{
		if (asAtomHandler::as<XMLList>(o)->nodes.size() == 1)
		{
			replace(idx,asAtomHandler::fromObjectNoPrimitive(asAtomHandler::as<XMLList>(o)->nodes[0].getPtr()),retnodes,allowConst,replacetext,alreadyset,wrk);
			ASATOM_DECREF(o);
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
					ASATOM_INCREF(o);
					targetobject->setVariableByMultinameIntern(m,o,allowConst,replacetext,alreadyset,wrk);
					break;
				}
			}
		}
		unsigned int k = 0;
		auto it = nodes.begin();
		while (k < idx && it!=nodes.end())
		{
			++k;
			++it;
		}

		it = nodes.erase(it);

		XMLList *toAdd = asAtomHandler::as<XMLList>(o);
		nodes.insert(it, toAdd->nodes.begin(), toAdd->nodes.end());
		ASATOM_DECREF(o);
	}
	else if (asAtomHandler::is<XML>(o))
	{
		if (targetobject)
		{
			multiname m(NULL);
			m.name_type = multiname::NAME_UINT;
			m.name_ui = idx;
			m.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
			ASATOM_INCREF(o);
			targetobject->setVariableByMultinameIntern(m,o,allowConst,replacetext,alreadyset,wrk);
		}
		if (asAtomHandler::as<XML>(o)->getNodeKind() == pugi::node_pcdata)
		{
			if (replacetext)
			{
				nodes[idx]->childrenlist->clear();
				nodes[idx]->nodetype = pugi::node_pcdata;
				nodes[idx]->nodenameID = BUILTIN_STRINGS::STRING_TEXT;
				nodes[idx]->nodevalue = asAtomHandler::toString(o,wrk);
				nodes[idx]->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
				nodes[idx]->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
				ASATOM_DECREF(o);
			}
			else
			{
				nodes[idx]->childrenlist->clear();
				XML* tmp = Class<XML>::getInstanceSNoArgs(getInstanceWorker());
				tmp->parentNode = nodes[idx].getPtr();
				tmp->nodetype = pugi::node_pcdata;
				tmp->nodenameID = BUILTIN_STRINGS::STRING_TEXT;
				tmp->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
				tmp->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
				tmp->nodevalue = asAtomHandler::toString(o,wrk);
				tmp->constructed = true;
				nodes[idx]->childrenlist->append(_MNR(tmp));
			}
		}
		else
			nodes[idx] = _MNR(asAtomHandler::as<XML>(o));
	}
	else
	{
		if (replacetext)
		{
			nodes[idx]->childrenlist->clear();
			nodes[idx]->nodetype = pugi::node_pcdata;
			nodes[idx]->nodenameID = BUILTIN_STRINGS::STRING_TEXT;
			nodes[idx]->nodevalue = asAtomHandler::toString(o,getInstanceWorker());
			nodes[idx]->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
			nodes[idx]->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
			ASATOM_DECREF(o);
		}
		else
		{
			if (nodes[idx]->nodetype == pugi::node_pcdata)
			{
				nodes[idx]->nodevalue = asAtomHandler::toString(o,getInstanceWorker());
				ASATOM_DECREF(o);
			}
			else 
			{
				nodes[idx]->childrenlist->clear();
				XML* tmp = Class<XML>::getInstanceSNoArgs(getInstanceWorker());
				tmp->parentNode = nodes[idx].getPtr();
				tmp->nodetype = pugi::node_pcdata;
				tmp->nodenameID = BUILTIN_STRINGS::STRING_TEXT;
				tmp->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
				tmp->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
				tmp->nodevalue = asAtomHandler::toString(o,wrk);
				tmp->constructed = true;
				nodes[idx]->childrenlist->append(_MNR(tmp));
				ASATOM_DECREF(o);
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
	if (!hasSimpleContent()
			|| this->nodes.size()==0) // not mentioned in the specs but Adobe returns 0 if the nodelist is empty
		return 0;

	tiny_string str = toString();
	return Integer::stringToASInteger(str.raw_buf(), 0);
}
int64_t XMLList::toInt64()
{
	if (!hasSimpleContent()
			|| this->nodes.size()==0) // not mentioned in the specs but Adobe returns 0 if the nodelist is empty
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
	if (!hasSimpleContent()
			|| this->nodes.size()==0) // not mentioned in the specs but Adobe returns 0 if the nodelist is empty
		return 0;
	return parseNumber(toString_priv(),getSystemState()->getSwfVersion()<11);
}

number_t XMLList::toNumberForComparison()
{
	return parseNumber(toString_priv(),getSystemState()->getSwfVersion()<11);
}

ASFUNCTIONBODY_ATOM(XMLList,_toString)
{
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv()));
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
	XMLList* th=asAtomHandler::as<XMLList>(obj);
	assert_and_throw(argslen==0);
	ASObject* res=abstract_s(wrk,th->toXMLString_internal());
	ret = asAtomHandler::fromObject(res);
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
		asAtomHandler::setUInt(ret,getInstanceWorker(),index-1);
	else
		throw RunTimeException("XMLList::nextName out of bounds");
}

void XMLList::nextValue(asAtom& ret,uint32_t index)
{
	if(index<=nodes.size())
	{
		nodes[index-1]->incRef();
		ret = asAtomHandler::fromObject(nodes[index-1].getPtr());
	}
	else
		throw RunTimeException("XMLList::nextValue out of bounds");
}

void XMLList::appendNodesTo(XML *dest) const
{
	for (auto it=nodes.begin(); it!=nodes.end(); ++it)
	{
		asAtom arg0=asAtomHandler::fromObject(it->getPtr());
		asAtom obj = asAtomHandler::fromObject(dest);
		asAtom ret=asAtomHandler::invalidAtom;
		XML::_appendChild(ret,getInstanceWorker(),obj, &arg0, 1);
		ASATOM_DECREF(ret);
	}
}

void XMLList::prependNodesTo(XML *dest) const
{
	for (auto it=nodes.rbegin(); it!=nodes.rend(); ++it)
	{
		asAtom arg0=asAtomHandler::fromObject(it->getPtr());
		asAtom obj = asAtomHandler::fromObject(dest);
		asAtom ret=asAtomHandler::invalidAtom;
		XML::_prependChild(ret,getInstanceWorker(),obj, &arg0, 1);
		ASATOM_DECREF(ret);
	}
}
string XMLList::toDebugString() const
{
	std::string res = ASObject::toDebugString();
	char buf[100];
	sprintf(buf," ch=%lu",this->nodes.size());
	res += buf;
	return res;
}
