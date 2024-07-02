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

#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/ASQName.h"
#include "scripting/toplevel/IFunction.h"
#include "scripting/toplevel/Namespace.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/Undefined.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/xml/flashxml.h"
#include "scripting/class.h"
#include "compat.h"
#include "scripting/argconv.h"
#include "abc.h"
#include "parsing/amf3_generator.h"
#include <unordered_set>

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

XML::XML(ASWorker* wrk,Class_base* c):ASObject(wrk,c,T_OBJECT,SUBTYPE_XML),parentNode(nullptr),nodetype((pugi::xml_node_type)0),isAttribute(false),nodenameID(BUILTIN_STRINGS::EMPTY),nodenamespace_uri(BUILTIN_STRINGS::EMPTY),nodenamespace_prefix(BUILTIN_STRINGS::EMPTY),constructed(false)
{
}

XML::XML(ASWorker* wrk,Class_base* c, const std::string &str):ASObject(wrk,c,T_OBJECT,SUBTYPE_XML),parentNode(nullptr),nodetype((pugi::xml_node_type)0),isAttribute(false),nodenameID(BUILTIN_STRINGS::EMPTY),nodenamespace_uri(BUILTIN_STRINGS::EMPTY),nodenamespace_prefix(BUILTIN_STRINGS::EMPTY),constructed(false)
{
	createTree(buildFromString(str, getParseMode()),false);
}

XML::XML(ASWorker* wrk,Class_base* c, const pugi::xml_node& _n, XML* parent, bool fromXMLList):ASObject(wrk,c,T_OBJECT,SUBTYPE_XML),parentNode(0),nodetype((pugi::xml_node_type)0),isAttribute(false),nodenameID(BUILTIN_STRINGS::EMPTY),nodenamespace_uri(BUILTIN_STRINGS::EMPTY),nodenamespace_prefix(BUILTIN_STRINGS::EMPTY),constructed(false)
{
	if (parent)
		parentNode = parent;
	createTree(_n,fromXMLList);
}

void XML::finalize()
{
	childrenlist.reset();
	attributelist.reset();
	procinstlist.reset();
	namespacedefs.clear();
}
bool XML::destruct()
{
	xmldoc.reset();
	parentNode=nullptr;
	nodetype =(pugi::xml_node_type)0;
	isAttribute = false;
	constructed = false;
	childrenlist.reset();
	nodenameID = BUILTIN_STRINGS::EMPTY;
	nodevalue.clear();
	nodenamespace_uri=BUILTIN_STRINGS::EMPTY;
	nodenamespace_prefix=BUILTIN_STRINGS::EMPTY;
	attributelist.reset();
	procinstlist.reset();
	namespacedefs.clear();
	return destructIntern();
}

void XML::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (childrenlist)
		childrenlist->prepareShutdown();
	if (attributelist)
		attributelist->prepareShutdown();
	if (procinstlist)
		procinstlist->prepareShutdown();
	for (auto it = namespacedefs.begin(); it != namespacedefs.end(); it++)
	{
		(*it)->prepareShutdown();
	}
}

void XML::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_FINAL);
	setDefaultXMLSettings();
	c->isReusable=true;

	c->setDeclaredMethodByQName("ignoreComments","",c->getSystemState()->getBuiltinFunction(_getIgnoreComments),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("ignoreComments","",c->getSystemState()->getBuiltinFunction(_setIgnoreComments),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("ignoreProcessingInstructions","",c->getSystemState()->getBuiltinFunction(_getIgnoreProcessingInstructions),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("ignoreProcessingInstructions","",c->getSystemState()->getBuiltinFunction(_setIgnoreProcessingInstructions),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("ignoreWhitespace","",c->getSystemState()->getBuiltinFunction(_getIgnoreWhitespace),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("ignoreWhitespace","",c->getSystemState()->getBuiltinFunction(_setIgnoreWhitespace),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("prettyIndent","",c->getSystemState()->getBuiltinFunction(_getPrettyIndent),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("prettyIndent","",c->getSystemState()->getBuiltinFunction(_setPrettyIndent),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("prettyPrinting","",c->getSystemState()->getBuiltinFunction(_getPrettyPrinting),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("prettyPrinting","",c->getSystemState()->getBuiltinFunction(_setPrettyPrinting),SETTER_METHOD,false);
	c->setDeclaredMethodByQName("settings",AS3,c->getSystemState()->getBuiltinFunction(_getSettings),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("setSettings",AS3,c->getSystemState()->getBuiltinFunction(_setSettings),NORMAL_METHOD,false);
	c->setDeclaredMethodByQName("defaultSettings",AS3,c->getSystemState()->getBuiltinFunction(_getDefaultSettings),NORMAL_METHOD,false);

	// undocumented method, see http://www.docsultant.com/site2/articles/flex_internals.html#xmlNotify
	c->setDeclaredMethodByQName("notification",AS3,c->getSystemState()->getBuiltinFunction(notification),NORMAL_METHOD,true); // undocumented
	c->setDeclaredMethodByQName("setNotification",AS3,c->getSystemState()->getBuiltinFunction(setNotification),NORMAL_METHOD,true); // undocumented

	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toString",AS3,c->getSystemState()->getBuiltinFunction(_toString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("valueOf","",c->getSystemState()->getBuiltinFunction(valueOf),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("valueOf",AS3,c->getSystemState()->getBuiltinFunction(valueOf),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toXMLString","",c->getSystemState()->getBuiltinFunction(toXMLString),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("toXMLString",AS3,c->getSystemState()->getBuiltinFunction(toXMLString),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("nodeKind","",c->getSystemState()->getBuiltinFunction(nodeKind),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("nodeKind",AS3,c->getSystemState()->getBuiltinFunction(nodeKind),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("child",AS3,c->getSystemState()->getBuiltinFunction(child),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("child","",c->getSystemState()->getBuiltinFunction(child),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("children",AS3,c->getSystemState()->getBuiltinFunction(children,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("children","",c->getSystemState()->getBuiltinFunction(children,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("childIndex",AS3,c->getSystemState()->getBuiltinFunction(childIndex),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("contains",AS3,c->getSystemState()->getBuiltinFunction(contains),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attribute",AS3,c->getSystemState()->getBuiltinFunction(attribute,1,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("attribute","",c->getSystemState()->getBuiltinFunction(attribute,1,Class<XMLList>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("attributes",AS3,c->getSystemState()->getBuiltinFunction(attributes,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("attributes","",c->getSystemState()->getBuiltinFunction(attributes,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("length",AS3,c->getSystemState()->getBuiltinFunction(length),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("localName",AS3,c->getSystemState()->getBuiltinFunction(localName),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("localName","",c->getSystemState()->getBuiltinFunction(localName),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("name",AS3,c->getSystemState()->getBuiltinFunction(name),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("name","",c->getSystemState()->getBuiltinFunction(name),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("namespace",AS3,c->getSystemState()->getBuiltinFunction(_namespace),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("namespace","",c->getSystemState()->getBuiltinFunction(_namespace),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("normalize",AS3,c->getSystemState()->getBuiltinFunction(_normalize),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("descendants",AS3,c->getSystemState()->getBuiltinFunction(descendants,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("appendChild",AS3,c->getSystemState()->getBuiltinFunction(_appendChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("parent",AS3,c->getSystemState()->getBuiltinFunction(parent),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inScopeNamespaces",AS3,c->getSystemState()->getBuiltinFunction(inScopeNamespaces),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("addNamespace",AS3,c->getSystemState()->getBuiltinFunction(addNamespace),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("hasSimpleContent","",c->getSystemState()->getBuiltinFunction(_hasSimpleContent),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("hasSimpleContent",AS3,c->getSystemState()->getBuiltinFunction(_hasSimpleContent),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("hasComplexContent",AS3,c->getSystemState()->getBuiltinFunction(_hasComplexContent),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("text",AS3,c->getSystemState()->getBuiltinFunction(text),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("elements",AS3,c->getSystemState()->getBuiltinFunction(elements,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("elements","",c->getSystemState()->getBuiltinFunction(elements,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("setLocalName",AS3,c->getSystemState()->getBuiltinFunction(_setLocalName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setName",AS3,c->getSystemState()->getBuiltinFunction(_setName),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setNamespace",AS3,c->getSystemState()->getBuiltinFunction(_setNamespace),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("copy","",c->getSystemState()->getBuiltinFunction(_copy),CONSTANT_TRAIT);
	c->prototype->setVariableByQName("copy",AS3,c->getSystemState()->getBuiltinFunction(_copy),CONSTANT_TRAIT);
	c->setDeclaredMethodByQName("setChildren",AS3,c->getSystemState()->getBuiltinFunction(_setChildren),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toJSON",AS3,c->getSystemState()->getBuiltinFunction(_toJSON),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("insertChildAfter",AS3,c->getSystemState()->getBuiltinFunction(insertChildAfter),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("insertChildBefore",AS3,c->getSystemState()->getBuiltinFunction(insertChildBefore),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("namespaceDeclarations",AS3,c->getSystemState()->getBuiltinFunction(namespaceDeclarations),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("removeNamespace",AS3,c->getSystemState()->getBuiltinFunction(removeNamespace),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("comments",AS3,c->getSystemState()->getBuiltinFunction(comments),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("processingInstructions",AS3,c->getSystemState()->getBuiltinFunction(processingInstructions,0,Class<XMLList>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("propertyIsEnumerable",AS3,c->getSystemState()->getBuiltinFunction(_propertyIsEnumerable),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("hasOwnProperty",AS3,c->getSystemState()->getBuiltinFunction(_hasOwnProperty),DYNAMIC_TRAIT);
	c->setDeclaredMethodByQName("prependChild",AS3,c->getSystemState()->getBuiltinFunction(_prependChild),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("replace",AS3,c->getSystemState()->getBuiltinFunction(_replace),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(XML,generator)
{
	assert_and_throw(argslen<=1);
	if (argslen == 0)
	{
		ret = asAtomHandler::fromObject(Class<XML>::getInstanceSNoArgs(wrk));
	}
	else if (asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
	{
		ret = asAtomHandler::fromObject(Class<XML>::getInstanceSNoArgs(wrk));
	}
	else if(asAtomHandler::isString(args[0]) ||
		asAtomHandler::is<Number>(args[0]) ||
		asAtomHandler::is<Integer>(args[0]) ||
		asAtomHandler::is<UInteger>(args[0]) ||
		asAtomHandler::is<Boolean>(args[0]))
	{
		ret = asAtomHandler::fromObject(createFromString(wrk,asAtomHandler::toString(args[0],wrk)));
	}
	else if(asAtomHandler::is<XML>(args[0]))
	{
		ASATOM_INCREF(args[0]);
		ret = args[0];
	}
	else if(asAtomHandler::is<XMLList>(args[0]))
	{
		_NR<XML> res=asAtomHandler::as<XMLList>(args[0])->reduceToXML();
		if (res)
		{
			res->incRef();
			ret = asAtomHandler::fromObject(res.getPtr());
		}
	}
	else if(asAtomHandler::is<XMLNode>(args[0]))
	{
		ret = asAtomHandler::fromObject(createFromNode(wrk,asAtomHandler::as<XMLNode>(args[0])->node));
	}
	else
	{
		ret = asAtomHandler::fromObject(createFromString(wrk,asAtomHandler::toString(args[0],wrk)));
	}
}

ASFUNCTIONBODY_ATOM(XML,_constructor)
{
	assert_and_throw(argslen<=1);
	XML* th=asAtomHandler::as<XML>(obj);
	if(argslen==0 && th->constructed) //If root is already set we have been constructed outside AS code
		return;

	if(argslen==0 ||
	   asAtomHandler::is<Null>(args[0]) || 
	   asAtomHandler::is<Undefined>(args[0]))
	{
		th->createTree(th->buildFromString("", getParseMode()),false);
	}
	else if(asAtomHandler::is<ByteArray>(args[0]))
	{
		//Official documentation says that generic Objects are not supported.
		//ByteArray seems to be though (see XML test) so let's support it
		ByteArray* ba=asAtomHandler::as<ByteArray>(args[0]);
		uint32_t len=ba->getLength();
		const uint8_t* str=ba->getBuffer(len, false);
		th->createTree(th->buildFromString(std::string((const char*)str,len), getParseMode()),false);
	}
	else if(asAtomHandler::isString(args[0]) ||
		asAtomHandler::is<Number>(args[0]) ||
		asAtomHandler::is<Integer>(args[0]) ||
		asAtomHandler::is<UInteger>(args[0]) ||
		asAtomHandler::is<Boolean>(args[0]))
	{
		//By specs, XML constructor will only convert to string Numbers or Booleans
		//ints are not explicitly mentioned, but they seem to work
		th->createTree(th->buildFromString(asAtomHandler::toString(args[0],wrk), getParseMode()),false);
	}
	else if(asAtomHandler::is<XML>(args[0]))
	{
		asAtomHandler::as<XML>(args[0])->copy(th);
	}
	else if(asAtomHandler::is<XMLList>(args[0]))
	{
		XMLList *list=asAtomHandler::as<XMLList>(args[0]);
		_NR<XML> reduced=list->reduceToXML();
		if (reduced)
			reduced->copy(th);
	}
	else
	{
		th->createTree(th->buildFromString(asAtomHandler::toString(args[0],wrk), getParseMode()),false);
	}
}

ASFUNCTIONBODY_ATOM(XML,nodeKind)
{
	XML* th=asAtomHandler::as<XML>(obj);
	assert_and_throw(argslen==0);
	tiny_string s=th->nodekindString();
	ret = asAtomHandler::fromString(wrk->getSystemState(),s);
}
const char *XML::nodekindString()
{
	if (isAttribute)
		return "attribute";
	switch(nodetype)
	{
		case pugi::node_element:
			return "element";
		case pugi::node_cdata:
		case pugi::node_pcdata:
		case pugi::node_null:
			return "text";
		case pugi::node_comment:
			return "comment";
		case pugi::node_pi:
			return "processing-instruction";
		default:
		{
			LOG(LOG_ERROR,"Unsupported XML type " << nodetype);
			throw UnsupportedException("Unsupported XML node type");
		}
	}
}

ASFUNCTIONBODY_ATOM(XML,length)
{
	asAtomHandler::setInt(ret,wrk,1);
}

ASFUNCTIONBODY_ATOM(XML,localName)
{
	XML* th=asAtomHandler::as<XML>(obj);
	assert_and_throw(argslen==0);
	if(!th->isAttribute && (th->nodetype==pugi::node_pcdata || th->nodetype==pugi::node_comment || th->nodetype==pugi::node_null))
		asAtomHandler::setNull(ret);
	else
		ret = asAtomHandler::fromStringID(th->nodenameID);
}

ASFUNCTIONBODY_ATOM(XML,name)
{
	XML* th=asAtomHandler::as<XML>(obj);
	assert_and_throw(argslen==0);
	//TODO: add namespace
	if(!th->isAttribute && (th->nodetype==pugi::node_pcdata || th->nodetype==pugi::node_comment || th->nodetype==pugi::node_null))
		asAtomHandler::setNull(ret);
	else
	{
		ASQName* res = Class<ASQName>::getInstanceSNoArgs(wrk);
		res->setByXML(th);
		ret = asAtomHandler::fromObject(res);
	}
}

ASFUNCTIONBODY_ATOM(XML,descendants)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom name = asAtomHandler::invalidAtom;
	asAtom defname = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_WILDCARD);
	ARG_CHECK(ARG_UNPACK(name,defname));
	XMLVector res;
	multiname mname(nullptr);
	uint32_t nameID;
	if(asAtomHandler::is<ASQName>(name))
	{
		nsNameAndKind ns(wrk->getSystemState(),asAtomHandler::as<ASQName>(name)->getURI(),NAMESPACE);
		mname.ns.push_back(ns);
		nameID = asAtomHandler::as<ASQName>(name)->getLocalName();
	}
	else
		nameID = asAtomHandler::toStringId(name,wrk);
	mname.name_s_id = nameID;
	mname.name_type = multiname::NAME_STRING;
	ASObject* o = asAtomHandler::getObject(name);
	if (o)
		o->applyProxyProperty(mname);
	th->getDescendantsByQName(mname,res);
	ret = asAtomHandler::fromObject(XMLList::create(wrk,res,th->getChildrenlist(),multiname(nullptr)));
}

ASFUNCTIONBODY_ATOM(XML,_appendChild)
{
	XML* th=asAtomHandler::as<XML>(obj);
	assert_and_throw(argslen==1);
	XML* arg;
	if(asAtomHandler::is<XML>(args[0]))
	{
		ASATOM_INCREF(args[0]);
		arg=asAtomHandler::as<XML>(args[0]);
	}
	else if(asAtomHandler::is<XMLList>(args[0]))
	{
		XMLList* list=asAtomHandler::as<XMLList>(args[0]);
		list->appendNodesTo(th);
		th->incRef();
		ret = asAtomHandler::fromObject(th);
		return;
	}
	else
	{
		//The appendChild specs says that any other type is converted to string
		//NOTE: this is explicitly different from XML constructor, that will only convert to
		//string Numbers and Booleans
		tiny_string s = asAtomHandler::toString(args[0],wrk);
		if (wrk->getSystemState()->getSwfVersion() > 9)
		{
			arg=createFromString(wrk,"dummy");
			//avoid interpretation of the argument, just set it as text node
			arg->setTextContent(s);
		}
		else
		{
			try
			{
				arg=createFromString(wrk,s,true);
			}
			catch(ASObject* exception)
			{
				arg=createFromString(wrk,"dummy");
				//avoid interpretation of the argument, just set it as text node
				arg->setTextContent(s);
			}
		}
	}

	th->appendChild(_MNR(arg));
	th->incRef();
	ret = asAtomHandler::fromObject(th);
}
void XML::handleNotification(const tiny_string& command, asAtom value, asAtom detail)
{
	if (!notifierfunction.isNull())
	{
		asAtom args[5];
		args[0]=asAtomHandler::fromObject(this); // TODO what is currentTarget?
		args[1]=asAtomHandler::fromStringID(getSystemState()->getUniqueStringId(command));
		args[2]=asAtomHandler::fromObject(this);
		args[3]=value;
		args[4]=detail;
		asAtom ret=asAtomHandler::invalidAtom;
		asAtom obj = asAtomHandler::nullAtom;
		asAtom func = asAtomHandler::fromObject(notifierfunction.getPtr());
		asAtomHandler::callFunction(func,getInstanceWorker(),ret,obj,args,5,false);
	}
}
void XML::appendChild(_NR<XML> newChild)
{
	if (newChild && newChild->constructed)
	{
		if (this == newChild.getPtr())
		{
			createError<TypeError>(getInstanceWorker(),kXMLIllegalCyclicalLoop);
			return;
		}
		XML* node = this->parentNode;
		while (node)
		{
			if (node == newChild.getPtr())
			{
				createError<TypeError>(getInstanceWorker(),kXMLIllegalCyclicalLoop);
				return;
			}
			node = node->parentNode;
		}
		newChild->parentNode = this;
		newChild->incRef();
		childrenlist->append(newChild);
		handleNotification("nodeAdded",asAtomHandler::fromObject(newChild.getPtr()),asAtomHandler::nullAtom);
	}
}

/* returns the named attribute in an XMLList */
ASFUNCTIONBODY_ATOM(XML,attribute)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom attrname = asAtomHandler::invalidAtom;
	//see spec for QName handling
	ARG_CHECK(ARG_UNPACK (attrname));
	uint32_t attrnameID = BUILTIN_STRINGS::EMPTY;
	uint32_t tmpns = BUILTIN_STRINGS::EMPTY;
	if(asAtomHandler::is<ASQName>(args[0]))
	{
		tmpns= asAtomHandler::as<ASQName>(args[0])->getURI();
		attrnameID = asAtomHandler::as<ASQName>(args[0])->getLocalName();
	}
	else
		attrnameID = asAtomHandler::toStringId(args[0],wrk);

	XMLVector tmp;
	XMLList* res = XMLList::create(wrk,tmp,th->getChildrenlist(),multiname(nullptr));
	if (!th->attributelist.isNull())
	{
		for (XMLList::XMLListVector::const_iterator it = th->attributelist->nodes.begin(); it != th->attributelist->nodes.end(); it++)
		{
			_NR<XML> attr = *it;
			if (attr && attr->nodenamespace_uri == tmpns && (attrnameID== BUILTIN_STRINGS::STRING_WILDCARD || attr->nodenameID == attrnameID))
			{
				res->append(attr);
			}
		}
	}
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(XML,attributes)
{
	assert_and_throw(argslen==0);
	ret = asAtomHandler::fromObject(asAtomHandler::as<XML>(obj)->getAllAttributes());
}

XMLList* XML::getAllAttributes()
{
	attributelist->incRef();
	return attributelist.getPtr();
}

const tiny_string XML::toXMLString_internal(bool pretty, uint32_t defaultnsprefix, const char *indent,bool bfirst)
{
	tiny_string res;
	set<uint32_t> seen_prefix;

	if (bfirst)
	{
		uint32_t defns = getInstanceWorker()->getDefaultXMLNamespaceID();
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
				tmp = tmp->parentNode;
			else
				break;
		}
	}
	if (isAttribute)
		res += encodeToXML(nodevalue,true);
	else
	{
	/*
		if (!ignoreProcessingInstructions && !procinstlist.isNull())
		{

			for (uint32_t i = 0; i < procinstlist->nodes.size(); i++)
			{
				_NR<XML> child= procinstlist->nodes[i];
				res += child->toXMLString_internal(pretty,defaultnsprefix,indent,false);
			LOG(LOG_INFO,"printpi:"<<res);
				if (pretty && prettyPrinting)
					res += "\n";
			}
		}
		*/
		switch (nodetype)
		{
			case pugi::node_pcdata:
				res += indent;
				res += encodeToXML(nodevalue,false);
				break;
			case pugi::node_comment:
				res += indent;
				res += "<!--";
				res += nodevalue;
				res += "-->";
				break;
			case pugi::node_declaration:
			case pugi::node_pi:
				if (ignoreProcessingInstructions)
					break;
				res += indent;
				res += "<?";
				res += getSystemState()->getStringFromUniqueId(this->nodenameID);
				res += " ";
				res += nodevalue;
				res += "?>";
				break;
			case pugi::node_element:
			{
				uint32_t curprefix = this->nodenamespace_prefix;
				res += indent;
				res += "<";
				if (this->nodenamespace_prefix == BUILTIN_STRINGS::EMPTY)
				{
					if (defaultnsprefix != BUILTIN_STRINGS::EMPTY)
					{
						res += getSystemState()->getStringFromUniqueId(defaultnsprefix);
						res += ":";
						curprefix = defaultnsprefix;
					}
					else if (!bfirst && this->nodenamespace_uri != BUILTIN_STRINGS::EMPTY)
					{
						// Namespaces without prefix should be set to an implementation defined prefix (see ECMA-357 10.2.1)
						// adobe seems to use "aaa", so we do the same
						res += "aaa:";
						curprefix = getSystemState()->getUniqueStringId("aaa");
					}
				}
				else
				{
					if (!bfirst && this->parentNode)
					{
						XML* tmp = this->parentNode;
						while(tmp)
						{
							if(tmp->nodenamespace_uri == this->nodenamespace_uri)
							{
								if (tmp->nodenamespace_prefix != BUILTIN_STRINGS::EMPTY)
										curprefix = tmp->nodenamespace_prefix;
									break;
							}
							if (tmp->parentNode)
								tmp = tmp->parentNode;
							else
								break;
						}
					}
					
					res += this->getSystemState()->getStringFromUniqueId(curprefix);
					res += ":";
				}
				res += getSystemState()->getStringFromUniqueId(this->nodenameID);
				if (!attributelist.isNull())
				{
					for (XMLList::XMLListVector::const_iterator it = attributelist->nodes.begin(); it != attributelist->nodes.end(); it++)
					{
						res += " ";
						_NR<XML> attr = *it;
						if (attr->nodenamespace_prefix != BUILTIN_STRINGS::EMPTY)
						{
							res += getSystemState()->getStringFromUniqueId(attr->nodenamespace_prefix);
							res += ":";
						}
						res += getSystemState()->getStringFromUniqueId(attr->nodenameID);
						res += "=\"";
						res += encodeToXML(attr->nodevalue,true);
						res += "\"";
					}
				}
				for (uint32_t i = 0; i < this->namespacedefs.size(); i++)
				{
					bool b;
					_R<Namespace> tmpns = this->namespacedefs[i];
					uint32_t tmpprefix = tmpns->getPrefix(b);
					if((tmpprefix == BUILTIN_STRINGS::EMPTY && tmpns->getURI() == BUILTIN_STRINGS::EMPTY) || tmpprefix==this->nodenamespace_prefix || seen_prefix.find(tmpprefix)!=seen_prefix.end())
						continue;
					seen_prefix.insert(tmpprefix);
					res += tmpprefix == BUILTIN_STRINGS::EMPTY || tmpprefix==this->nodenamespace_prefix ? " xmlns" : " xmlns:";
					if (tmpprefix!=this->nodenamespace_prefix)
						res += getSystemState()->getStringFromUniqueId(tmpprefix);
					res += "=\"";
					res += getSystemState()->getStringFromUniqueId(tmpns->getURI());
					res += "\"";
				}
				if (this->parentNode)
				{
					if (bfirst)
					{
						XML* tmp = this->parentNode;
						while(tmp)
						{
							for (uint32_t i = 0; i < tmp->namespacedefs.size(); i++)
							{
								bool b;
								_R<Namespace> tmpns = tmp->namespacedefs[i];
								uint32_t tmpprefix = tmpns->getPrefix(b);
								if(seen_prefix.find(tmpprefix)==seen_prefix.end())
								{
									seen_prefix.insert(tmpprefix);
									if (tmpprefix != BUILTIN_STRINGS::EMPTY)
									{
										res += " xmlns:";
										res += getSystemState()->getStringFromUniqueId(tmpprefix);
									}
									else
										res += " xmlns";
									res += "=\"";
									res += getSystemState()->getStringFromUniqueId(tmpns->getURI());
									res += "\"";
								}
							}
							if (tmp->parentNode)
								tmp = tmp->parentNode;
							else
								break;
						}
					}
					else if (curprefix != BUILTIN_STRINGS::EMPTY)
					{
						XML* tmp = this->parentNode;
						bool bfound = false;
						while(tmp)
						{
							for (uint32_t i = 0; i < tmp->namespacedefs.size(); i++)
							{
								bool b;
								_R<Namespace> tmpns = tmp->namespacedefs[i];
								uint32_t tmpprefix = tmpns->getPrefix(b);
								if(tmpprefix == curprefix)
								{
									seen_prefix.insert(tmpprefix);
									bfound = true;
									break;
								}
							}
							if (!bfound && tmp->parentNode)
								tmp = tmp->parentNode;
							else
								break;
						}
					}
				}
				if (this->nodenamespace_uri != BUILTIN_STRINGS::EMPTY && 
						((this->nodenamespace_prefix==BUILTIN_STRINGS::EMPTY && defaultnsprefix == BUILTIN_STRINGS::EMPTY) ||
						 (this->nodenamespace_prefix!=BUILTIN_STRINGS::EMPTY && seen_prefix.find(this->nodenamespace_prefix)==seen_prefix.end())))
				{
					if (this->nodenamespace_prefix!=BUILTIN_STRINGS::EMPTY)
					{
						seen_prefix.insert(this->nodenamespace_prefix);
						res += " xmlns:";
						res += getSystemState()->getStringFromUniqueId(this->nodenamespace_prefix);
					}
					else if(!bfirst && this->nodenamespace_prefix == BUILTIN_STRINGS::EMPTY && this->nodenamespace_uri != BUILTIN_STRINGS::EMPTY)
					{
						res += " xmlns:";
						res += getSystemState()->getStringFromUniqueId(curprefix);
					}
					else
						res += " xmlns";
					res += "=\"";
					res += getSystemState()->getStringFromUniqueId(this->nodenamespace_uri);
					res += "\"";
				}
				else if (defaultnsprefix != BUILTIN_STRINGS::EMPTY && seen_prefix.find(defaultnsprefix)==seen_prefix.end())
				{
					seen_prefix.insert(defaultnsprefix);
					res += " xmlns:";
					res += getSystemState()->getStringFromUniqueId(defaultnsprefix);
					res += "=\"";
					res += getInstanceWorker()->getDefaultXMLNamespace();
					res += "\"";
				}
				if (!attributelist.isNull())
				{
					for (XMLList::XMLListVector::const_iterator it = attributelist->nodes.begin(); it != attributelist->nodes.end(); it++)
					{
						_NR<XML> attr = *it;
						if (attr->nodenamespace_prefix != BUILTIN_STRINGS::EMPTY)
						{
							if (seen_prefix.find(attr->nodenamespace_prefix)==seen_prefix.end())
							{
								seen_prefix.insert(attr->nodenamespace_prefix);
								res += " xmlns:";
								res += getSystemState()->getStringFromUniqueId(attr->nodenamespace_prefix);
								res += "=\"";
								res += getSystemState()->getStringFromUniqueId(attr->nodenamespace_uri);
								res += "\"";
							}
						}
						else if (attr->nodenamespace_uri != BUILTIN_STRINGS::EMPTY)
						{
							res += " xmlns=\"";
							res += getSystemState()->getStringFromUniqueId(attr->nodenamespace_uri);
							res += "\"";
						}
					}
				}
				if (childrenlist.isNull() || childrenlist->nodes.size() == 0)
				{
					res += "/>";
					break;
				}
				res += ">";
				tiny_string newindent;
				bool bindent = (pretty && prettyPrinting && prettyIndent >=0 && 
								!childrenlist.isNull() &&
								(childrenlist->nodes.size() >1 || 
								 (!childrenlist->nodes[0]->procinstlist.isNull()) ||
								 (childrenlist->nodes[0]->nodetype != pugi::node_pcdata && childrenlist->nodes[0]->nodetype != pugi::node_cdata)));
				if (bindent)
				{
					newindent = indent;
					for (int32_t j = 0; j < prettyIndent; j++)
					{
						newindent += " ";
					}
				}
				if (!childrenlist.isNull())
				{
					for (auto it =childrenlist->nodes.begin(); it != childrenlist->nodes.end(); it++)
					{
						tiny_string tmpres = (*it)->toXMLString_internal(pretty,defaultnsprefix,newindent.raw_buf(),false);
						if (bindent && !tmpres.empty())
							res += "\n";
						res += tmpres;
					}
				}
				if (bindent)
				{
					res += "\n";
					res += indent;
				}
				res += "</";
				if (curprefix != BUILTIN_STRINGS::EMPTY)
				{
					res += getSystemState()->getStringFromUniqueId(curprefix);
					res += ":";
				}
				res += getSystemState()->getStringFromUniqueId(this->nodenameID);
				res += ">";
				break;
			}
			case pugi::node_cdata:
				res += "<![CDATA[";
				res += nodevalue;
				res += "]]>";
				break;
			default:
				LOG(LOG_NOT_IMPLEMENTED,"XML::toXMLString unhandled nodetype:"<<nodetype);
				break;
		}
	}
	return res;
}

ASFUNCTIONBODY_ATOM(XML,toXMLString)
{
	XML* th=asAtomHandler::as<XML>(obj);
	assert_and_throw(argslen==0);
	tiny_string res = th->toXMLString_internal();
	ret = asAtomHandler::fromObject(abstract_s(wrk,res));
}

void XML::childrenImpl(XMLVector& ret, uint32_t nameID)
{
	if (!childrenlist.isNull())
	{
		for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
		{
			_NR<XML> child= childrenlist->nodes[i];
			if(nameID!=BUILTIN_STRINGS::STRING_WILDCARD && child->nodenameID != nameID)
				continue;
			ret.push_back(child);
		}
	}
}

void XML::childrenImplIndex(XMLVector& ret, uint32_t index)
{
	if (constructed && !childrenlist.isNull() && index < childrenlist->nodes.size())
	{
		_NR<XML> child= childrenlist->nodes[index];
		ret.push_back(child);
	}
}

ASFUNCTIONBODY_ATOM(XML,child)
{
	XML* th=asAtomHandler::as<XML>(obj);
	assert_and_throw(argslen==1);
	uint32_t arg0=asAtomHandler::toStringId(args[0],wrk);
	XMLVector res;
	uint32_t index=0;
	multiname mname(nullptr);
	mname.name_s_id=arg0;
	mname.name_type=multiname::NAME_STRING;
	mname.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	mname.isAttribute=false;
	if(XML::isValidMultiname(wrk->getSystemState(),mname, index))
		th->childrenImplIndex(res, index);
	else
		th->childrenImpl(res, arg0);
	XMLList* retObj=XMLList::create(wrk,res,th->getChildrenlist(),mname);
	ret = asAtomHandler::fromObject(retObj);
}

ASFUNCTIONBODY_ATOM(XML,children)
{
	XML* th=asAtomHandler::as<XML>(obj);
	assert_and_throw(argslen==0);
	XMLVector res;
	th->childrenImpl(res, BUILTIN_STRINGS::STRING_WILDCARD);
	multiname mname(nullptr);
	mname.name_s_id=BUILTIN_STRINGS::STRING_WILDCARD;
	mname.name_type=multiname::NAME_STRING;
	mname.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	XMLList* retObj=XMLList::create(wrk,res,th->getChildrenlist(),mname);
	ret = asAtomHandler::fromObject(retObj);
}

ASFUNCTIONBODY_ATOM(XML,childIndex)
{
	XML* th=asAtomHandler::as<XML>(obj);
	if (th->parentNode && !th->parentNode->childrenlist.isNull())
	{
		XML* parent = th->parentNode;
		for (uint32_t i = 0; i < parent->childrenlist->nodes.size(); i++)
		{
			ASObject* o= parent->childrenlist->nodes[i].getPtr();
			if (o == th)
			{
				asAtomHandler::setUInt(ret,wrk,i);
				return;
			}
		}
	}
	asAtomHandler::setInt(ret,wrk,-1);
}

ASFUNCTIONBODY_ATOM(XML,_hasSimpleContent)
{
	XML *th=asAtomHandler::as<XML>(obj);
	asAtomHandler::setBool(ret,th->hasSimpleContent());
}

ASFUNCTIONBODY_ATOM(XML,_hasComplexContent)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtomHandler::setBool(ret,th->hasComplexContent());
}

ASFUNCTIONBODY_ATOM(XML,valueOf)
{
	ASATOM_INCREF(obj);
	ret = obj;
}

void XML::getText(XMLVector& ret)
{
	if (childrenlist.isNull())
		return;
	for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
	{
		_NR<XML> child= childrenlist->nodes[i];
		if (child->getNodeKind() == pugi::node_pcdata  ||
			child->getNodeKind() == pugi::node_cdata)
		{
			ret.push_back( child );
		}
	}
}

ASFUNCTIONBODY_ATOM(XML,text)
{
	XML* th=asAtomHandler::as<XML>(obj);
	XMLVector res;
	th->getText(res);
	ret = asAtomHandler::fromObject(XMLList::create(wrk,res,th->getChildrenlist(),multiname(nullptr)));
}

ASFUNCTIONBODY_ATOM(XML,elements)
{
	XMLVector res;
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom name = asAtomHandler::invalidAtom;
	asAtom defname = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	ARG_CHECK(ARG_UNPACK (name, defname));
	uint32_t nameID = asAtomHandler::toStringId(name,wrk);
	if (nameID==BUILTIN_STRINGS::STRING_WILDCARD)
		nameID=BUILTIN_STRINGS::EMPTY;

	th->getElementNodes(nameID, res);
	ret = asAtomHandler::fromObject(XMLList::create(wrk,res,th->getChildrenlist(),multiname(nullptr)));
}

void XML::getElementNodes(uint32_t nameID, XMLVector& foundElements)
{
	if (childrenlist.isNull())
		return;
	for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
	{
		_NR<XML> child= childrenlist->nodes[i];
		if(child->nodetype==pugi::node_element && (nameID == BUILTIN_STRINGS::EMPTY || nameID == child->nodenameID))
		{
			foundElements.push_back( child );
		}
	}
}

ASFUNCTIONBODY_ATOM(XML,inScopeNamespaces)
{
	XML* th=asAtomHandler::as<XML>(obj);
	Array *namespaces = Class<Array>::getInstanceSNoArgs(wrk);
	set<uint32_t> seen_prefix;

	XML* tmp = th;
	while(tmp)
	{
		for (uint32_t i = 0; i < tmp->namespacedefs.size(); i++)
		{
			bool b;
			_R<Namespace> tmpns = tmp->namespacedefs[i];
			if(seen_prefix.count(tmpns->getPrefix(b))==0)
			{
				tmpns->incRef();
				namespaces->push(asAtomHandler::fromObject(tmpns.getPtr()));
				seen_prefix.insert(tmpns->getPrefix(b));
			}
		}
		if (tmp->parentNode)
			tmp = tmp->parentNode;
		else
			break;
	}
	ret = asAtomHandler::fromObject(namespaces);
}

ASFUNCTIONBODY_ATOM(XML,addNamespace)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom newNamespace = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(newNamespace));


	uint32_t ns_uri = BUILTIN_STRINGS::EMPTY;
	uint32_t ns_prefix = BUILTIN_STRINGS::EMPTY;
	if (asAtomHandler::is<Namespace>(newNamespace))
	{
		Namespace* tmpns = asAtomHandler::as<Namespace>(newNamespace);
		bool b;
		ns_prefix = tmpns->getPrefix(b);
		ns_uri = tmpns->getURI();
	}
	else if (asAtomHandler::is<ASQName>(newNamespace))
	{
		ns_uri = asAtomHandler::as<ASQName>(newNamespace)->getURI();
	}
	else
		ns_uri = asAtomHandler::getStringId(newNamespace);
	if (th->nodenamespace_prefix == ns_prefix)
		th->nodenamespace_prefix=BUILTIN_STRINGS::EMPTY;
	for (uint32_t i = 0; i < th->namespacedefs.size(); i++)
	{
		bool b;
		_R<Namespace> tmpns = th->namespacedefs[i];
		if (tmpns->getPrefix(b) == ns_prefix)
		{
			th->namespacedefs[i] = _R<Namespace>(Class<Namespace>::getInstanceS(wrk,ns_uri,ns_prefix));
			return;
		}
	}
	th->namespacedefs.push_back(_R<Namespace>(Class<Namespace>::getInstanceS(wrk,ns_uri,ns_prefix)));
}

ASObject *XML::getParentNode()
{
	if (parentNode && parentNode->is<XML>())
	{
		parentNode->incRef();
		return parentNode;
	}
	else
		return getSystemState()->getUndefinedRef();
}

ASFUNCTIONBODY_ATOM(XML,parent)
{
	XML* th=asAtomHandler::as<XML>(obj);
	ret = asAtomHandler::fromObject(th->getParentNode());
}

ASFUNCTIONBODY_ATOM(XML,contains)
{
	XML* th=asAtomHandler::as<XML>(obj);
	_NR<ASObject> value;
	ARG_CHECK(ARG_UNPACK(value));
	if(!value->is<XML>())
		asAtomHandler::setBool(ret,false);
	else
		asAtomHandler::setBool(ret,th->isEqual(value.getPtr()));
}

ASFUNCTIONBODY_ATOM(XML,_namespace)
{
	XML* th=asAtomHandler::as<XML>(obj);
	tiny_string prefix;
	ARG_CHECK(ARG_UNPACK(prefix, ""));

	pugi::xml_node_type nodetype=th->nodetype;
	if(prefix.empty() && 
	   nodetype!=pugi::node_element && 
	   !th->isAttribute)
	{
		asAtomHandler::setNull(ret);
		return;
	}
	if (prefix.empty())
	{
		ret = asAtomHandler::fromObject(Class<Namespace>::getInstanceS(wrk,th->nodenamespace_uri, th->nodenamespace_prefix));
		return;
	}
		
	for (uint32_t i = 0; i < th->namespacedefs.size(); i++)
	{
		bool b;
		_R<Namespace> tmpns = th->namespacedefs[i];
		if (tmpns->getPrefix(b) == wrk->getSystemState()->getUniqueStringId(prefix))
		{
			ret = asAtomHandler::fromObject(Class<Namespace>::getInstanceS(wrk,tmpns->getURI(), wrk->getSystemState()->getUniqueStringId(prefix)));
			return;
		}
	}
	asAtomHandler::setUndefined(ret);
}

ASFUNCTIONBODY_ATOM(XML,_setLocalName)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom newName = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(newName));

	if(th->nodetype==pugi::node_pcdata || th->nodetype==pugi::node_comment)
		return;

	uint32_t new_name;
	if(asAtomHandler::is<ASQName>(newName))
	{
		new_name=asAtomHandler::as<ASQName>(newName)->getLocalName();
	}
	else
	{
		new_name=asAtomHandler::toStringId(newName,wrk);
	}

	th->setLocalName(new_name);
}

void XML::setLocalName(uint32_t new_name)
{
	asAtom v =asAtomHandler::fromStringID(new_name);
	if(!isXMLName(getInstanceWorker(),v))
	{
		createError<TypeError>(getInstanceWorker(),kXMLInvalidName, getSystemState()->getStringFromUniqueId(new_name));
		return;
	}
	this->nodenameID = new_name;
	handleNotification("nameSet",asAtomHandler::fromObject(this),asAtomHandler::nullAtom);
}

ASFUNCTIONBODY_ATOM(XML,_setName)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom newName = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(newName));

	if(th->nodetype==pugi::node_pcdata || th->nodetype==pugi::node_comment)
		return;

	uint32_t localname = BUILTIN_STRINGS::EMPTY;
	uint32_t ns_uri = BUILTIN_STRINGS::EMPTY;
	uint32_t ns_prefix = th->nodenamespace_prefix;
	if(asAtomHandler::is<ASQName>(newName))
	{
		ASQName *qname=asAtomHandler::as<ASQName>(newName);
		localname=qname->getLocalName();
		ns_uri=qname->getURI();
	}
	else if (!asAtomHandler::isUndefined(newName))
	{
		localname=asAtomHandler::toStringId(newName,wrk);
		ns_prefix= BUILTIN_STRINGS::EMPTY;
	}

	th->setLocalName(localname);
	th->setNamespace(ns_uri,ns_prefix);
}

ASFUNCTIONBODY_ATOM(XML,_setNamespace)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom newNamespace = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(newNamespace));

	if(th->nodetype==pugi::node_pcdata ||
	   th->nodetype==pugi::node_comment ||
	   th->nodetype==pugi::node_pi)
		return;
	uint32_t ns_uri = BUILTIN_STRINGS::EMPTY;
	uint32_t ns_prefix = BUILTIN_STRINGS::EMPTY;
	if(asAtomHandler::is<Namespace>(newNamespace))
	{
		Namespace *ns=asAtomHandler::as<Namespace>(newNamespace);
		ns_uri=ns->getURI();
		bool prefix_is_undefined=true;
		ns_prefix=ns->getPrefix(prefix_is_undefined);
	}
	else if(asAtomHandler::is<ASQName>(newNamespace))
	{
		ASQName *qname=asAtomHandler::as<ASQName>(newNamespace);
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
	else if (!asAtomHandler::isUndefined(newNamespace))
	{
		ns_uri=asAtomHandler::toStringId(newNamespace,wrk);
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
	if (th->isAttribute && th->parentNode)
	{
		XML* tmp = th->parentNode;
		for (uint32_t i = 0; i < tmp->namespacedefs.size(); i++)
		{
			bool b;
			_R<Namespace> tmpns = tmp->namespacedefs[i];
			if (tmpns->getPrefix(b) == ns_prefix)
			{
				tmp->namespacedefs[i] = _R<Namespace>(Class<Namespace>::getInstanceS(wrk,ns_uri,ns_prefix));
				return;
			}
		}
		tmp->namespacedefs.push_back(_R<Namespace>(Class<Namespace>::getInstanceS(wrk,ns_uri,ns_prefix)));
	}
}

void XML::setNamespace(uint32_t ns_uri, uint32_t ns_prefix)
{
	this->nodenamespace_prefix = ns_prefix;
	this->nodenamespace_uri = ns_uri;
	handleNotification("namespaceSet",asAtomHandler::fromObject(this),asAtomHandler::nullAtom);
}

ASFUNCTIONBODY_ATOM(XML,_copy)
{
	XML* th=asAtomHandler::as<XML>(obj);
	XML* res = Class<XML>::getInstanceSNoArgs(wrk);
	th->copy(res);
	ret = asAtomHandler::fromObject(res);
}

void XML::copy(XML* res, XML* parent)
{
	res->parentNode=parent;
	if (!childrenlist.isNull())
	{
		res->childrenlist = _MR(Class<XMLList>::getInstanceSNoArgs(getInstanceWorker()));
		res->childrenlist->nodes.reserve(childrenlist->nodes.size());
		childrenlist->copy(res->childrenlist.getPtr(),res);
	}
	res->nodetype = this->nodetype;
	res->isAttribute = this->isAttribute;
	res->nodenameID = this->nodenameID;
	res->nodevalue = this->nodevalue;
	res->nodenamespace_uri = this->nodenamespace_uri;
	res->nodenamespace_prefix = this->nodenamespace_prefix;
	if (!attributelist.isNull())
	{
		res->attributelist = _MR(Class<XMLList>::getInstanceSNoArgs(getInstanceWorker()));
		res->attributelist->nodes.reserve(attributelist->nodes.size());
		attributelist->copy(res->attributelist.getPtr());
	}
	if (!procinstlist.isNull())
	{
		res->procinstlist = _MR(Class<XMLList>::getInstanceSNoArgs(getInstanceWorker()));
		res->procinstlist->nodes.reserve(procinstlist->nodes.size());
		procinstlist->copy(res->procinstlist.getPtr());
	}
	res->namespacedefs.reserve(namespacedefs.size());
	for (uint32_t i = 0; i < namespacedefs.size(); i++)
	{
		bool is_undefined;
		uint32_t prefix = namespacedefs[i]->getPrefix(is_undefined);
		Namespace* ns =Class<Namespace>::getInstanceS(getInstanceWorker(),namespacedefs[i]->getURI(),prefix ,namespacedefs[i]->getKind(),is_undefined);
		res->namespacedefs.push_back(_MR(ns));
	}
	res->constructed = this->constructed;
}

ASFUNCTIONBODY_ATOM(XML,_setChildren)
{
	XML* th=asAtomHandler::as<XML>(obj);
	_NR<ASObject> newChildren;
	ARG_CHECK(ARG_UNPACK(newChildren));

	th->childrenlist->clear();

	if (newChildren->is<XML>())
	{
		XML *newChildrenXML=newChildren->as<XML>();
		newChildrenXML->incRef();
		th->appendChild(_NR<XML>(newChildrenXML));
	}
	else if (newChildren->is<XMLList>())
	{
		XMLList *list=newChildren->as<XMLList>();
		list->incRef();
		list->appendNodesTo(th);
	}
	else if (newChildren->is<ASString>())
	{
		ASString *newChildrenString=newChildren->as<ASString>();
		XML *newChildrenXML=Class<XML>::getInstanceS(wrk,newChildrenString->toString());
		newChildrenXML->incRef();
		th->appendChild(_NR<XML>(newChildrenXML));
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED, "XML::setChildren supports only XMLs and XMLLists");
	}

	th->incRef();
	ret = asAtomHandler::fromObject(th);
}

ASFUNCTIONBODY_ATOM(XML,_normalize)
{
	XML* th=asAtomHandler::as<XML>(obj);
	th->normalize();

	th->incRef();
	ret = asAtomHandler::fromObject(th);
}

void XML::normalize()
{
	childrenlist->normalize();
}

void XML::addTextContent(const tiny_string& str)
{
	assert(getNodeKind() == pugi::node_pcdata);

	nodevalue += str;
}

void XML::setTextContent(const tiny_string& content)
{
	if (getNodeKind() == pugi::node_pcdata ||
	    isAttribute ||
	    getNodeKind() == pugi::node_comment ||
	    getNodeKind() == pugi::node_pi ||
		getNodeKind() == pugi::node_cdata)
	{
		nodevalue = content;
	}
}


bool XML::hasSimpleContent() const
{
	if (getNodeKind() == pugi::node_comment ||
		getNodeKind() == pugi::node_pi)
		return false;
	if (childrenlist.isNull())
		return true;
	for(size_t i=0; i<childrenlist->nodes.size(); i++)
	{
		if (childrenlist->nodes[i]->getNodeKind() == pugi::node_element)
			return false;
	}
	return true;
}

bool XML::hasComplexContent() const
{
	return !hasSimpleContent();
}

pugi::xml_node_type XML::getNodeKind() const
{
	return nodetype;
}


void XML::getDescendantsByQName(const multiname& name, XMLVector& ret) const
{
	if (!constructed)
		return;
	uint32_t nodenameID = name.normalizedNameId(getSystemState());
	if (name.isAttribute && !attributelist.isNull())
	{
		for (uint32_t i = 0; i < attributelist->nodes.size(); i++)
		{
			
			_NR<XML> child= attributelist->nodes[i];
			if(nodenameID==BUILTIN_STRINGS::EMPTY || nodenameID==BUILTIN_STRINGS::STRING_WILDCARD || (nodenameID == child->nodenameID))
			{
				if (name.ns.empty())
					ret.push_back(child);
				else
				{
					for (auto it = name.ns.begin(); it != name.ns.end(); it++)
					{
						if (it->nsNameId == BUILTIN_STRINGS::STRING_WILDCARD || it->nsNameId == child->nodenamespace_uri)
						{
							ret.push_back(child);
							break;
						}
					}
				}
			}
		}
	}
	if (childrenlist.isNull())
		return;
	for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
	{
		_NR<XML> child= childrenlist->nodes[i];
		if(!name.isAttribute)
		{
			if(nodenameID==BUILTIN_STRINGS::EMPTY || nodenameID==BUILTIN_STRINGS::STRING_WILDCARD || (nodenameID == child->nodenameID))
			{
				if (name.ns.empty())
					ret.push_back(child);
				else
				{
					for (auto it = name.ns.begin(); it != name.ns.end(); it++)
					{
						if (it->nsNameId == BUILTIN_STRINGS::STRING_WILDCARD || it->nsNameId == child->nodenamespace_uri)
						{
							ret.push_back(child);
							break;
						}
					}
				}
			}
		}
		child->getDescendantsByQName(name, ret);
	}
}

XML::XMLVector XML::getAttributes()
{ 
	multiname mn(nullptr);
	mn.name_type=multiname::NAME_STRING;
	mn.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	mn.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	mn.isAttribute = true;
	return getAttributesByMultiname(mn,BUILTIN_STRINGS::EMPTY); 
}
XML::XMLVector XML::getAttributesByMultiname(const multiname& name, uint32_t normalizedNameID) const
{
	XMLVector ret;
	if (attributelist.isNull())
		return ret;
	uint32_t defns = getInstanceWorker()->getDefaultXMLNamespaceID();
	std::unordered_set<uint32_t> namespace_uri;
	namespace_uri.reserve(name.ns.size());
	bool hasAnyNS = normalizedNameID==BUILTIN_STRINGS::STRING_WILDCARD;
	bool nameHasWildcardNS=false;
	auto it = name.ns.cbegin();
	while (it != name.ns.cend())
	{
		switch (it->nsId)
		{
			case BUILTIN_NAMESPACES::EMPTY_NS:
				namespace_uri.insert(defns);
				break;
			case BUILTIN_NAMESPACES::AS3_NS:
				break;
			default:
			{
				if (it->kind==NAMESPACE)
				{
					switch (it->nsNameId)
					{
						case BUILTIN_STRINGS::EMPTY:
							namespace_uri.insert(defns);
							break;
						case BUILTIN_STRINGS::STRING_WILDCARD:
							nameHasWildcardNS=true;
							break;
						default:
							namespace_uri.insert(it->nsNameId);
							break;
					}
				}
				break;
			}
		}
		++it;
	}
	const XMLList::XMLListVector& nodes = attributelist->nodes;
	if (normalizedNameID==BUILTIN_STRINGS::EMPTY)
	{
		for (auto child = nodes.cbegin(); child != nodes.cend(); child++)
		{
			uint32_t childnamespace_uri = (*child)->nodenamespace_uri;
			
			bool bmatch =(hasAnyNS || nameHasWildcardNS ||
						  (namespace_uri.find(childnamespace_uri) != namespace_uri.end())
						 );
			if(bmatch)
			{
				ret.push_back(*child);
			}
		}
	}
	else if (normalizedNameID==BUILTIN_STRINGS::STRING_WILDCARD)
	{
		for (auto child = nodes.cbegin(); child != nodes.cend(); child++)
		{
			uint32_t childnamespace_uri = (*child)->nodenamespace_uri;
			
			bool bmatch =(hasAnyNS || nameHasWildcardNS ||
						  (namespace_uri.find(childnamespace_uri) != namespace_uri.end())
						 );
			if(bmatch)
			{
				ret.push_back(*child);
			}
		}
	}
	else
	{
		for (auto child = nodes.cbegin(); child != nodes.cend(); child++)
		{
			uint32_t childnamespace_uri = (*child)->nodenamespace_uri;
			
			bool bmatch =(normalizedNameID==(*child)->nodenameID) &&
						 (nameHasWildcardNS ||
						  (namespace_uri.size() == 0 && childnamespace_uri == BUILTIN_STRINGS::EMPTY) ||
						  (namespace_uri.find(childnamespace_uri) != namespace_uri.end())
						 );
			if(bmatch)
			{
				ret.push_back(*child);
			}
		}
	}
	return ret;
}
XML::XMLVector XML::getValuesByMultiname(_NR<XMLList> nodelist, const multiname& name)
{
	XMLVector ret;
	uint32_t defns = BUILTIN_STRINGS::EMPTY;
	if (nodenamespace_prefix == BUILTIN_STRINGS::EMPTY && nodenamespace_uri != BUILTIN_STRINGS::EMPTY)
		defns = nodenamespace_uri;
	else
		defns = getInstanceWorker()->getDefaultXMLNamespaceID();
	tiny_string normalizedName=name.normalizedName(getSystemState());
	uint32_t normalizedNameID = name.normalizedNameId(getSystemState());
	if (normalizedName.startsWith("@"))
		normalizedNameID = getSystemState()->getUniqueStringId(normalizedName.substr(1,normalizedName.end()));

	std::unordered_set<uint32_t> namespace_uri;
	bool hasAnyNS = true;
	auto it = name.ns.cbegin();
	while (it != name.ns.cend())
	{
		hasAnyNS=false;
		switch (it->nsId)
		{
			case BUILTIN_NAMESPACES::EMPTY_NS:
				namespace_uri.insert(defns);
				namespace_uri.insert(BUILTIN_STRINGS::EMPTY);
				break;
			case BUILTIN_NAMESPACES::AS3_NS:
				break;
			default:
			{
				if (it->kind==NAMESPACE)
				{
					if (it->nsNameId == BUILTIN_STRINGS::EMPTY)
					{
						namespace_uri.insert(defns);
						namespace_uri.insert(BUILTIN_STRINGS::EMPTY);
					}
					else
						namespace_uri.insert(it->nsNameId);
				}
				break;
			}
		}
		++it;
	}

	const XMLList::XMLListVector& nodes = nodelist->nodes;
	if (normalizedNameID==BUILTIN_STRINGS::EMPTY)
	{
		for (auto child = nodes.cbegin(); child != nodes.cend(); child++)
		{
			uint32_t childnamespace_uri = (*child)->nodenamespace_uri;
			bool bmatch =(hasAnyNS||
						  (namespace_uri.find(BUILTIN_STRINGS::STRING_WILDCARD)!= namespace_uri.end()) ||
						  (namespace_uri.find(childnamespace_uri) != namespace_uri.end())
						 );
			if(bmatch)
			{
				ret.push_back(*child);
			}
		}
	}
	else if (normalizedNameID==BUILTIN_STRINGS::STRING_WILDCARD)
	{
		for (auto child = nodes.cbegin(); child != nodes.cend(); child++)
		{
			uint32_t childnamespace_uri = (*child)->nodenamespace_uri;
			
			bool bmatch =(hasAnyNS||
						  (namespace_uri.find(BUILTIN_STRINGS::STRING_WILDCARD)!= namespace_uri.end()) ||
						  (namespace_uri.find(childnamespace_uri) != namespace_uri.end())
						 );
			if(bmatch)
			{
				ret.push_back(*child);
			}
		}
	}
	else
	{
		for (auto child = nodes.cbegin(); child != nodes.cend(); child++)
		{
			uint32_t childnamespace_uri = (*child)->nodenamespace_uri;
			
			bool bmatch = (normalizedNameID==(*child)->nodenameID) &&
						 (hasAnyNS||
						  (namespace_uri.find(BUILTIN_STRINGS::STRING_WILDCARD)!= namespace_uri.end()) ||
						  (namespace_uri.size() == 0 && childnamespace_uri == BUILTIN_STRINGS::EMPTY) ||
						  (namespace_uri.find(childnamespace_uri) != namespace_uri.end())
						 );
			if(bmatch)
			{
				ret.push_back(*child);
			}
		}
	}
	return ret;
}

GET_VARIABLE_RESULT XML::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	if((opt & SKIP_IMPL)!=0)
	{
		GET_VARIABLE_RESULT res = getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);

		//If a method is not found on XML object and the
		//object is a leaf node, delegate to ASString
		if(asAtomHandler::isInvalid(ret) && hasSimpleContent())
		{
			ASObject *contentstr=abstract_s(getInstanceWorker(),toString_priv());
			contentstr->getVariableByMultiname(ret,name, opt,wrk);
			contentstr->decRef();
		}
		return res;
	}

	bool isAttr=name.isAttribute;
	unsigned int index=0;

	uint32_t normalizedNameID=name.normalizedNameId(getSystemState());
	tiny_string normalizedName = name.normalizedName(getSystemState());
	if(normalizedNameID!=BUILTIN_STRINGS::EMPTY && normalizedName.charAt(0)=='@')
	{
		normalizedNameID = getSystemState()->getUniqueStringId(normalizedName.substr(1,normalizedName.end()));
		isAttr=true;
	}
	if(isAttr)
	{
		//Lookup attribute
		const XMLVector& attributes=getAttributesByMultiname(name,normalizedNameID);
		ret = asAtomHandler::fromObject(XMLList::create(getInstanceWorker(),attributes,attributelist.getPtr(),name));
		return GET_VARIABLE_RESULT::GETVAR_ISNEWOBJECT;
	}
	else if(XML::isValidMultiname(getSystemState(),name,index))
	{
		// If the multiname is a valid array property, the XML
		// object is treated as a single-item XMLList.
		if(index==0)
		{
			incRef();
			ret = asAtomHandler::fromObject(this);
		}
		else
			ret = asAtomHandler::fromObject(getSystemState()->getUndefinedRef());
	}
	else if (!childrenlist.isNull())
	{
		if (normalizedNameID == BUILTIN_STRINGS::STRING_WILDCARD)
		{
			XMLVector res;
			childrenImpl(res, BUILTIN_STRINGS::STRING_WILDCARD);
			multiname mname(nullptr);
			mname.name_s_id=BUILTIN_STRINGS::STRING_WILDCARD;
			mname.name_type=multiname::NAME_STRING;
			mname.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
			XMLList* retObj=XMLList::create(getInstanceWorker(),res,this->getChildrenlist(),mname);
			ret = asAtomHandler::fromObject(retObj);
		}
		else
		{
			const XMLVector& res=getValuesByMultiname(childrenlist,name);
			
			if(res.empty() && (opt & FROM_GETLEX)!=0)
				return GET_VARIABLE_RESULT::GETVAR_NORMAL;
			ret =asAtomHandler::fromObject(XMLList::create(getInstanceWorker(),res,this->getChildrenlist(),name));
			return GET_VARIABLE_RESULT::GETVAR_ISNEWOBJECT;
		}
	}
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}
GET_VARIABLE_RESULT XML::getVariableByInteger(asAtom &ret, int index, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	if (index < 0)
		return getVariableByIntegerIntern(ret,index,opt,wrk);
	if(index==0)
	{
		incRef();
		ret = asAtomHandler::fromObject(this);
	}
	else
		asAtomHandler::setUndefined(ret);
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}

multiname *XML::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset,ASWorker* wrk)
{
	return setVariableByMultinameIntern(name, o, allowConst, false,alreadyset,wrk);
}
void XML::setVariableByInteger(int index, asAtom &o, ASObject::CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	if (index < 0)
	{
		setVariableByInteger_intern(index,o,allowConst,alreadyset,wrk);
		return;
	}
	if (!this->parentNode)
	{
		createError<TypeError>(getWorker(),kXMLAssignmentToIndexedXMLNotAllowed);
		return;
	}
	childrenlist->setVariableByInteger(index,o,allowConst,alreadyset,wrk);
}
multiname* XML::setVariableByMultinameIntern(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool replacetext, bool* alreadyset,ASWorker* wrk)
{
	unsigned int index=0;
	bool isAttr=name.isAttribute;

	//Only the first namespace is used, is this right?
	uint32_t ns_uri = BUILTIN_STRINGS::EMPTY;
	uint32_t ns_prefix = BUILTIN_STRINGS::EMPTY;
	if(name.ns.size() > 0 && !name.hasEmptyNS)
	{
		if (name.ns[0].kind==NAMESPACE)
		{
			ns_uri=name.ns[0].nsNameId;
			ns_prefix=getNamespacePrefixByURI(ns_uri);
		}
	}

	// namespace set by "default xml namespace = ..."
	if (ns_uri == BUILTIN_STRINGS::EMPTY && ns_prefix == BUILTIN_STRINGS::EMPTY)
	{
		ns_uri = getInstanceWorker()->getDefaultXMLNamespaceID();
	}

	uint32_t normalizedNameID = name.normalizedNameId(getSystemState());
	const tiny_string normalizedName=name.normalizedName(getSystemState());
	if(!normalizedName.empty() && normalizedName.charAt(0)=='@')
	{
		isAttr=true;
		normalizedNameID=getSystemState()->getUniqueStringId(normalizedName.raw_buf()+1);
	}
	if (childrenlist.isNull())
		childrenlist = _MR(Class<XMLList>::getInstanceSNoArgs(getInstanceWorker()));
	if(isAttr)
	{
		tiny_string nodeval;
		if(asAtomHandler::is<XMLList>(o))
		{
			_NR<XMLList> x = _NR<XMLList>(asAtomHandler::as<XMLList>(o));
			for (auto it2 = x->nodes.begin(); it2 != x->nodes.end(); it2++)
			{
				if (nodeval != "")
					nodeval += " ";
				nodeval += (*it2)->toString();
			}
		}
		else
		{
			nodeval = asAtomHandler::toString(o,getInstanceWorker());
		}
		_NR<XML> a;
		auto it = attributelist->nodes.begin();
		while (it != attributelist->nodes.end())
		{
			_NR<XML> attr = *it;
			auto ittmp = it;
			it++;
			if (attr->nodenamespace_uri == ns_uri && (attr->nodenameID == normalizedNameID || (normalizedNameID==BUILTIN_STRINGS::STRING_WILDCARD)|| (normalizedNameID==BUILTIN_STRINGS::EMPTY)))
			{
				if (!a.isNull())
					it=attributelist->nodes.erase(ittmp);
				a = *ittmp;
				asAtom oldval = asAtomHandler::fromStringID(getSystemState()->getUniqueStringId(a->nodevalue));
				a->nodevalue = nodeval;
				handleNotification("attributeChanged",asAtomHandler::fromStringID(a->nodenameID),oldval);
			}
		}
		if (a.isNull() && !((normalizedNameID==BUILTIN_STRINGS::STRING_WILDCARD)|| (normalizedNameID==BUILTIN_STRINGS::EMPTY)))
		{
			_NR<XML> tmp = _MR<XML>(Class<XML>::getInstanceSNoArgs(getInstanceWorker()));
			tmp->parentNode = this;
			tmp->nodetype = pugi::node_null;
			tmp->isAttribute = true;
			tmp->nodenameID = normalizedNameID;
			tmp->nodenamespace_uri = ns_uri;
			tmp->nodenamespace_prefix = ns_prefix;
			tmp->nodevalue = nodeval;
			tmp->constructed = true;
			attributelist->nodes.push_back(tmp);
			handleNotification("attributeAdded",asAtomHandler::fromStringID(tmp->nodenameID),o);
		}
	}
	else if(XML::isValidMultiname(getSystemState(),name,index))
	{
		if (!this->parentNode)
		{
			createError<TypeError>(getWorker(),kXMLAssignmentToIndexedXMLNotAllowed);
			return nullptr;
		}
		childrenlist->setVariableByMultinameIntern(name,o,allowConst,replacetext,alreadyset,wrk);
	}
	else
	{
		bool notificationhandled = false;
		bool found = false;
		XMLVector tmpnodes;
		for (auto it = childrenlist->nodes.begin(); it != childrenlist->nodes.end();it++)
		{
			_NR<XML> tmpnode = *it;
			
			if (tmpnode->nodenamespace_uri == ns_uri && tmpnode->nodenameID == normalizedNameID)
			{
				if(asAtomHandler::is<XMLList>(o))
				{
					if (!found)
					{
						_NR<XMLList> x = _NR<XMLList>(Class<XMLList>::getInstanceS(getInstanceWorker(),asAtomHandler::getObjectNoCheck(o)->as<XMLList>()->toXMLString_internal(false)));
						tmpnodes.insert(tmpnodes.end(), x->nodes.begin(),x->nodes.end());
					}
				}
				else if(asAtomHandler::is<XML>(o))
				{
					if (asAtomHandler::getObjectNoCheck(o)->as<XML>()->getNodeKind() == pugi::node_pcdata)
					{
						if (replacetext)
						{
							tmpnode->nodetype = pugi::node_pcdata;
							tmpnode->nodenameID = BUILTIN_STRINGS::STRING_TEXT;
							tmpnode->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
							tmpnode->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
							tmpnode->nodevalue = asAtomHandler::toString(o,getInstanceWorker());
						}
						else
						{
							XML* tmp = Class<XML>::getInstanceSNoArgs(getInstanceWorker());
							tmp->parentNode = tmpnode.getPtr();
							tmp->incRef();
							tmp->nodetype = pugi::node_pcdata;
							tmp->nodenameID = BUILTIN_STRINGS::STRING_TEXT;
							tmp->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
							tmp->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
							tmp->nodevalue = asAtomHandler::toString(o,getInstanceWorker());
							tmp->constructed = true;
							tmpnode->childrenlist->clear();
							tmpnode->childrenlist->append(_MNR(tmp));
						}
						if (!found)
							tmpnodes.push_back(tmpnode);
						handleNotification("textSet",asAtomHandler::fromObject(this),asAtomHandler::nullAtom);
						notificationhandled=true;
					}
					else
					{
						XML* tmp = asAtomHandler::getObjectNoCheck(o)->as<XML>();
						tmp->parentNode = this;
						if (!found)
						{
							tmp->incRef();
							tmpnodes.push_back(_MNR(tmp));
						}
					}
					
				}
				else
				{
					if (tmpnode->childrenlist.isNull())
						tmpnode->childrenlist = _MNR(Class<XMLList>::getInstanceSNoArgs(getInstanceWorker()));
					
					if (tmpnode->childrenlist->nodes.size() == 1 && tmpnode->childrenlist->nodes[0]->nodetype == pugi::node_pcdata)
						tmpnode->childrenlist->nodes[0]->nodevalue = asAtomHandler::toString(o,getInstanceWorker());
					else
					{
						XML* newnode = createFromString(getInstanceWorker(),asAtomHandler::toString(o,getInstanceWorker()));
						tmpnode->childrenlist->clear();
						asAtom v = asAtomHandler::fromObject(newnode);
						tmpnode->setVariableByMultiname(name,v,allowConst,nullptr,wrk);
						if (newnode->getNodeKind() == pugi::node_pcdata)
						{
							handleNotification("textSet",asAtomHandler::fromObject(this),asAtomHandler::nullAtom);
							notificationhandled=true;
						}
					}
					if (!found)
					{
						tmpnodes.push_back(tmpnode);
					}
				}
				found = true;
				if (alreadyset)
					*alreadyset=true;
			}
			else
				tmpnodes.push_back(tmpnode);
		}
		if (!found)
		{
			if(asAtomHandler::getObject(o) && asAtomHandler::getObject(o)->is<XML>())
			{
				_NR<XML> tmp = _MNR(asAtomHandler::getObject(o)->as<XML>());
				tmp->parentNode = this;
				tmpnodes.push_back(tmp);
			}
			else
			{
				tiny_string tmpstr = "<";
				if (this->nodenamespace_prefix!=BUILTIN_STRINGS::EMPTY)
				{
					tmpstr += ns_prefix;
					tmpstr += ":";
				}
				tmpstr += normalizedName;
				if (ns_uri!= BUILTIN_STRINGS::EMPTY)
				{
					if(ns_prefix!=BUILTIN_STRINGS::EMPTY)
					{
						tmpstr += " xmlns:";
						tmpstr += getSystemState()->getStringFromUniqueId(ns_prefix);
						tmpstr += "=\"";
					}
					else
						tmpstr += " xmlns=\"";
					tmpstr += getSystemState()->getStringFromUniqueId(ns_uri);
					tmpstr += "\"";
				}
				tmpstr +=">";
				tmpstr += encodeToXML(asAtomHandler::toString(o,getInstanceWorker()),false);
				tmpstr +="</";
				if (ns_prefix != BUILTIN_STRINGS::EMPTY)
				{
					tmpstr += getSystemState()->getStringFromUniqueId(ns_prefix);
					tmpstr += ":";
				}
				tmpstr += normalizedName;
				tmpstr +=">";
				_NR<XML> tmp = _MR<XML>(createFromString(this->getInstanceWorker(),tmpstr));
				tmp->parentNode = this;
				tmpnodes.push_back(tmp);
				if (alreadyset)
					*alreadyset=true;
			}
		}
		childrenlist->nodes.clear();
		childrenlist->nodes.assign(tmpnodes.begin(),tmpnodes.end());
		if (!notificationhandled)
			handleNotification("nodeChanged",asAtomHandler::fromObject(this),asAtomHandler::nullAtom);
	}
	return nullptr;
}

bool XML::hasProperty(const multiname& name, bool checkXMLPropsOnly, bool considerDynamic, bool considerPrototype, ASWorker* wrk)
{
	if(considerDynamic == false && !checkXMLPropsOnly)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
	if (!isConstructed())
		return false;

	//Only the first namespace is used, is this right?
	uint32_t ns_uri = BUILTIN_STRINGS::EMPTY;
	uint32_t ns_prefix = BUILTIN_STRINGS::EMPTY;
	if(name.ns.size() > 0 && !name.hasEmptyNS)
	{
		//assert_and_throw(name.ns[0].kind==NAMESPACE);
		ns_uri=name.ns[0].nsNameId;
		ns_prefix=getNamespacePrefixByURI(ns_uri);
	}

	// namespace set by "default xml namespace = ..."
	if (ns_uri==BUILTIN_STRINGS::EMPTY && ns_prefix==BUILTIN_STRINGS::EMPTY)
	{
		ns_uri = getInstanceWorker()->getDefaultXMLNamespaceID();
	}

	bool isAttr=name.isAttribute;
	unsigned int index=0;
	const tiny_string normalizedName=name.normalizedName(getSystemState());
	uint32_t normalizedNameID=name.normalizedNameId(getSystemState());
	if(!normalizedName.empty() && normalizedName.charAt(0)=='@')
	{
		isAttr=true;
		normalizedNameID=getSystemState()->getUniqueStringId(normalizedName.raw_buf()+1);
	}
	if(isAttr)
	{
		//Lookup attribute
		if (!attributelist.isNull())
		{
			for (auto it = attributelist->nodes.begin(); it != attributelist->nodes.end(); it++)
			{
				_NR<XML> attr = *it;
				if (attr->nodenamespace_uri == ns_uri && attr->nodenameID == normalizedNameID)
					return true;
			}
		}
	}
	else if(XML::isValidMultiname(getSystemState(),name,index))
	{
		// If the multiname is a valid array property, the XML
		// object is treated as a single-item XMLList.
		return(index==0);
	}
	else if (!childrenlist.isNull())
	{
		//Lookup children
		for (uint32_t i = 0; i < childrenlist->nodes.size(); i++)
		{
			_NR<XML> child= childrenlist->nodes[i];
			bool name_match=(child->nodenameID == normalizedNameID);
			bool ns_match=ns_uri==BUILTIN_STRINGS::EMPTY || 
				(child->nodenamespace_uri == ns_uri);
			if(name_match && ns_match)
				return true;
		}
	}

	//Try the normal path as the last resource
	return checkXMLPropsOnly ? false : ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
}
bool XML::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk)
{
	return hasProperty(name,false,considerDynamic,considerPrototype,wrk);
}

bool XML::deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
{
	unsigned int index=0;
	uint32_t normalizedNameID = name.normalizedNameId(getSystemState());
	if(name.isAttribute)
	{
		//Only the first namespace is used, is this right?
		uint32_t ns_uri = BUILTIN_STRINGS::EMPTY;
		uint32_t ns_prefix = BUILTIN_STRINGS::EMPTY;
		if(name.ns.size() > 0 && !name.hasEmptyNS)
		{
			assert_and_throw(name.ns[0].kind==NAMESPACE);
			ns_uri=name.ns[0].nsNameId;
			ns_prefix=getNamespacePrefixByURI(ns_uri);
		}
		if (ns_uri == BUILTIN_STRINGS::EMPTY && ns_prefix == BUILTIN_STRINGS::EMPTY)
		{
			ns_uri = getInstanceWorker()->getDefaultXMLNamespaceID();
		}
		if (!attributelist.isNull() && attributelist->nodes.size() > 0)
		{
			auto it = attributelist->nodes.end();
			while (it != attributelist->nodes.begin())
			{
				it--;
				_NR<XML> attr = *it;
				if ((ns_uri==BUILTIN_STRINGS::EMPTY && normalizedNameID == BUILTIN_STRINGS::EMPTY) ||
						(ns_uri==BUILTIN_STRINGS::EMPTY && normalizedNameID == attr->nodenameID) ||
						(attr->nodenamespace_uri == ns_uri && normalizedNameID == BUILTIN_STRINGS::EMPTY) ||
						(attr->nodenamespace_uri == ns_uri && attr->nodenameID == normalizedNameID))
				{
					attributelist->nodes.erase(it);
					asAtom oldval = asAtomHandler::fromStringID(getSystemState()->getUniqueStringId(attr->nodevalue));
					handleNotification("attributeRemoved",asAtomHandler::fromStringID(attr->nodenameID),oldval);
				}
			}
		}
	}
	else if(XML::isValidMultiname(getSystemState(),name,index))
	{
		if (!childrenlist.isNull())
			childrenlist->nodes.erase(childrenlist->nodes.begin() + index);
	}
	else
	{
		//Only the first namespace is used, is this right?
		uint32_t ns_uri = BUILTIN_STRINGS::EMPTY;
		if(name.ns.size() > 0 && !name.hasEmptyNS)
		{
			assert_and_throw(name.ns[0].kind==NAMESPACE);
			ns_uri=name.ns[0].nsNameId;
		}
		if (!childrenlist.isNull() && childrenlist->nodes.size() > 0)
		{
			auto it = childrenlist->nodes.end();
			while (it != childrenlist->nodes.begin())
			{
				it--;
				_NR<XML> node = *it;
				if ((ns_uri==BUILTIN_STRINGS::EMPTY && normalizedNameID == BUILTIN_STRINGS::EMPTY) ||
						(ns_uri==BUILTIN_STRINGS::EMPTY && normalizedNameID == node->nodenameID) ||
						(node->nodenamespace_uri == ns_uri && normalizedNameID == BUILTIN_STRINGS::EMPTY) ||
						(node->nodenamespace_uri == ns_uri && node->nodenameID == normalizedNameID))
				{
					childrenlist->nodes.erase(it);
					handleNotification("nodeRemoved",asAtomHandler::fromObject(this),asAtomHandler::nullAtom);
				}
			}
		}
	}
	return true;
}
bool XML::isValidMultiname(SystemState* sys,const multiname& name, uint32_t& index)
{
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
	if(name.ns.size()!=0 && !name.hasEmptyNS)
		return false;

	if (name.isEmpty())
		return false;
	bool validIndex=name.toUInt(sys,index, true);
	// Don't throw for non-numeric NAME_STRING or NAME_OBJECT
	// because they can still be valid built-in property names.
	if(!validIndex && (name.name_type==multiname::NAME_INT || name.name_type == multiname::NAME_INT ||name.name_type==multiname::NAME_NUMBER))
		createError<RangeError>(getWorker(),kOutOfRangeError, name.normalizedNameUnresolved(sys), "?");

	return validIndex;
}

uint32_t XML::getNamespacePrefixByURI(uint32_t uri, bool create)
{
	uint32_t prefix = BUILTIN_STRINGS::EMPTY;
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
		tmp = tmp->parentNode;
	}

	if(!found && create)
	{
		nodenamespace_uri = uri;
	}

	return prefix;
}

ASFUNCTIONBODY_ATOM(XML,_toString)
{
	XML* th=asAtomHandler::as<XML>(obj);
	if (th->nodetype == pugi::node_element && th->hasSimpleContent() && (th->childrenlist.isNull() || th->childrenlist->nodes.empty()))
		ret = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
	else
		ret = asAtomHandler::fromObject(abstract_s(wrk,th->toString_priv()));
}

ASFUNCTIONBODY_ATOM(XML,_getIgnoreComments)
{
	asAtomHandler::setBool(ret,ignoreComments);
}
ASFUNCTIONBODY_ATOM(XML,_setIgnoreComments)
{
	assert(args && argslen==1);
	ignoreComments = asAtomHandler::Boolean_concrete(args[0]);
}
ASFUNCTIONBODY_ATOM(XML,_getIgnoreProcessingInstructions)
{
	asAtomHandler::setBool(ret,ignoreProcessingInstructions);
}
ASFUNCTIONBODY_ATOM(XML,_setIgnoreProcessingInstructions)
{
	assert(args && argslen==1);
	ignoreProcessingInstructions = asAtomHandler::Boolean_concrete(args[0]);
}
ASFUNCTIONBODY_ATOM(XML,_getIgnoreWhitespace)
{
	asAtomHandler::setBool(ret,ignoreWhitespace);
}
ASFUNCTIONBODY_ATOM(XML,_setIgnoreWhitespace)
{
	assert(args && argslen==1);
	ignoreWhitespace = asAtomHandler::Boolean_concrete(args[0]);
}
ASFUNCTIONBODY_ATOM(XML,_getPrettyIndent)
{
	asAtomHandler::setInt(ret,wrk,prettyIndent);
}
ASFUNCTIONBODY_ATOM(XML,_setPrettyIndent)
{
	assert(args && argslen==1);
	prettyIndent = asAtomHandler::toInt(args[0]);
}
ASFUNCTIONBODY_ATOM(XML,_getPrettyPrinting)
{
	asAtomHandler::setBool(ret,prettyPrinting);
}
ASFUNCTIONBODY_ATOM(XML,_setPrettyPrinting)
{
	assert(args && argslen==1);
	prettyPrinting = asAtomHandler::Boolean_concrete(args[0]);
}
ASFUNCTIONBODY_ATOM(XML,_getSettings)
{
	ASObject* res = Class<ASObject>::getInstanceS(wrk);
	multiname mn(nullptr);
	mn.name_type=multiname::NAME_STRING;
	mn.ns.emplace_back(res->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	mn.ns.emplace_back(res->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	mn.isAttribute = true;

	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("ignoreComments");
	asAtom v=asAtomHandler::invalidAtom;
	v = asAtomHandler::fromBool(ignoreComments);
	res->setVariableByMultiname(mn,v,CONST_NOT_ALLOWED,nullptr,wrk);
	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("ignoreProcessingInstructions");
	v = asAtomHandler::fromBool(ignoreProcessingInstructions);
	res->setVariableByMultiname(mn,v,CONST_NOT_ALLOWED,nullptr,wrk);
	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("ignoreWhitespace");
	v = asAtomHandler::fromBool(ignoreWhitespace);
	res->setVariableByMultiname(mn,v,CONST_NOT_ALLOWED,nullptr,wrk);
	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("prettyIndent");
	v = asAtomHandler::fromInt(prettyIndent);
	res->setVariableByMultiname(mn,v,CONST_NOT_ALLOWED,nullptr,wrk);
	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("prettyPrinting");
	v = asAtomHandler::fromBool(prettyPrinting);
	res->setVariableByMultiname(mn,v,CONST_NOT_ALLOWED,nullptr,wrk);
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(XML,_setSettings)
{
	if (argslen == 0)
	{
		setDefaultXMLSettings();
		asAtomHandler::setNull(ret);
		return;
	}
	_NR<ASObject> arg0;
	ARG_CHECK(ARG_UNPACK(arg0));
	if (arg0->is<Null>() || arg0->is<Undefined>())
	{
		setDefaultXMLSettings();
		asAtomHandler::setNull(ret);
		return;
	}
	multiname mn(nullptr);
	mn.name_type=multiname::NAME_STRING;
	mn.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	mn.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	mn.isAttribute = true;
	asAtom o=asAtomHandler::invalidAtom;

	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("ignoreComments");
	if (arg0->hasPropertyByMultiname(mn,true,true,wrk))
	{
		arg0->getVariableByMultiname(o,mn,SKIP_IMPL,wrk);
		ignoreComments = asAtomHandler::toInt(o);
	}

	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("ignoreProcessingInstructions");
	if (arg0->hasPropertyByMultiname(mn,true,true,wrk))
	{
		arg0->getVariableByMultiname(o,mn,SKIP_IMPL,wrk);
		ignoreProcessingInstructions = asAtomHandler::toInt(o);
	}

	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("ignoreWhitespace");
	if (arg0->hasPropertyByMultiname(mn,true,true,wrk))
	{
		arg0->getVariableByMultiname(o,mn,SKIP_IMPL,wrk);
		ignoreWhitespace = asAtomHandler::toInt(o);
	}

	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("prettyIndent");
	if (arg0->hasPropertyByMultiname(mn,true,true,wrk))
	{
		arg0->getVariableByMultiname(o,mn,SKIP_IMPL,wrk);
		prettyIndent = asAtomHandler::toInt(o);
	}

	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("prettyPrinting");
	if (arg0->hasPropertyByMultiname(mn,true,true,wrk))
	{
		arg0->getVariableByMultiname(o,mn,SKIP_IMPL,wrk);
		prettyPrinting = asAtomHandler::toInt(o);
	}
	asAtomHandler::setNull(ret);
}
ASFUNCTIONBODY_ATOM(XML,_getDefaultSettings)
{
	ASObject* res = Class<ASObject>::getInstanceS(wrk);
	multiname mn(nullptr);
	mn.name_type=multiname::NAME_STRING;
	mn.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	mn.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	mn.isAttribute = true;

	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("ignoreComments");
	res->setVariableByMultiname(mn,asAtomHandler::trueAtom,CONST_NOT_ALLOWED,nullptr,wrk);
	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("ignoreProcessingInstructions");
	res->setVariableByMultiname(mn,asAtomHandler::trueAtom,CONST_NOT_ALLOWED,nullptr,wrk);
	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("ignoreWhitespace");
	res->setVariableByMultiname(mn,asAtomHandler::trueAtom,CONST_NOT_ALLOWED,nullptr,wrk);
	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("prettyIndent");
	asAtom v=asAtomHandler::fromInt((int32_t)2);
	res->setVariableByMultiname(mn,v,CONST_NOT_ALLOWED,nullptr,wrk);
	mn.name_s_id=wrk->getSystemState()->getUniqueStringId("prettyPrinting");
	res->setVariableByMultiname(mn,asAtomHandler::trueAtom,CONST_NOT_ALLOWED,nullptr,wrk);
	ret = asAtomHandler::fromObject(res);
}
ASFUNCTIONBODY_ATOM(XML,_toJSON)
{
	ret = asAtomHandler::fromString(wrk->getSystemState(),"XML");
}

bool XML::CheckCyclicReference(XML* node)
{
	XML* tmp = node;
	if (tmp == this)
	{
		createError<TypeError>(getInstanceWorker(),kXMLIllegalCyclicalLoop);
		return true;
	}
	if (!childrenlist.isNull())
	{
		for (auto it = tmp->childrenlist->nodes.begin(); it != tmp->childrenlist->nodes.end(); it++)
		{
			if ((*it).getPtr() == this)
			{
				createError<TypeError>(getInstanceWorker(),kXMLIllegalCyclicalLoop);
				return true;
			}
			if (CheckCyclicReference((*it).getPtr()))
				return true;
		}
	}
	return false;
}

XML *XML::createFromString(ASWorker* wrk, const tiny_string &s, bool usefirstchild)
{
	XML* res = Class<XML>::getInstanceSNoArgs(wrk);
	pugi::xml_node root = res->buildFromString(s, getParseMode());
	res->createTree(usefirstchild ? root.first_child() : root,false);
	return res;
}

XML *XML::createFromNode(ASWorker* wrk, const pugi::xml_node &_n, XML *parent, bool fromXMLList)
{
	XML* res = Class<XML>::getInstanceSNoArgs(wrk);
	if (parent)
		res->parentNode = parent;
	res->createTree(_n,fromXMLList);
	return res;
}

ASFUNCTIONBODY_ATOM(XML,insertChildAfter)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom child1 = asAtomHandler::invalidAtom;
	asAtom child2 = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(child1)(child2));
	bool incref=true;
	if (th->nodetype != pugi::node_element)
	{
		asAtomHandler::setUndefined(ret);
		return;
	}
	
	if (asAtomHandler::is<XML>(child2))
	{
		if (th->CheckCyclicReference(asAtomHandler::as<XML>(child2)))
			return;
	}
	else if (asAtomHandler::is<XMLList>(child2))
	{
		for (auto it = asAtomHandler::as<XMLList>(child2)->nodes.begin(); it < asAtomHandler::as<XMLList>(child2)->nodes.end(); it++)
		{
			if (th->CheckCyclicReference((*it).getPtr()))
				return;
		}
	}
	else
	{
		child2 = asAtomHandler::fromObjectNoPrimitive(createFromString(wrk,asAtomHandler::toString(child2,wrk)));
		incref=false;
	}
	if (th->childrenlist.isNull())
		th->childrenlist = _MR(Class<XMLList>::getInstanceSNoArgs(wrk));
	if (asAtomHandler::isNull(child1))
	{
		if (asAtomHandler::is<XML>(child2))
		{
			if (incref)
				asAtomHandler::as<XML>(child2)->incRef();
			asAtomHandler::as<XML>(child2)->parentNode = th;
			th->childrenlist->nodes.insert(th->childrenlist->nodes.begin(),_MNR(asAtomHandler::as<XML>(child2)));
		}
		else if (asAtomHandler::is<XMLList>(child2))
		{
			for (auto it2 = asAtomHandler::as<XMLList>(child2)->nodes.begin(); it2 < asAtomHandler::as<XMLList>(child2)->nodes.end(); it2++)
			{
				(*it2)->parentNode = th;
			}
			th->childrenlist->nodes.insert(th->childrenlist->nodes.begin(),asAtomHandler::as<XMLList>(child2)->nodes.begin(), asAtomHandler::as<XMLList>(child2)->nodes.end());
		}
		th->incRef();
		ret = asAtomHandler::fromObject(th);
		return;
	}
	if (asAtomHandler::is<XMLList>(child1))
	{
		if (asAtomHandler::as<XMLList>(child1)->nodes.size()==0)
		{
			asAtomHandler::setUndefined(ret);
			return;
		}
		child1 = asAtomHandler::fromObjectNoPrimitive(asAtomHandler::as<XMLList>(child1)->nodes[0].getPtr());
	}
	for (auto it = th->childrenlist->nodes.begin(); it != th->childrenlist->nodes.end(); it++)
	{
		if ((*it).getPtr() == asAtomHandler::getObjectNoCheck(child1))
		{
			if (asAtomHandler::is<XML>(child2))
			{
				if (incref)
					asAtomHandler::as<XML>(child2)->incRef();
				asAtomHandler::as<XML>(child2)->parentNode = th;
				th->childrenlist->nodes.insert(it+1,_NR<XML>(asAtomHandler::as<XML>(child2)));
			}
			else if (asAtomHandler::is<XMLList>(child2))
			{
				for (auto it2 = asAtomHandler::as<XMLList>(child2)->nodes.begin(); it2 < asAtomHandler::as<XMLList>(child2)->nodes.end(); it2++)
				{
					(*it2)->parentNode = th;
				}
				th->childrenlist->nodes.insert(it+1,asAtomHandler::as<XMLList>(child2)->nodes.begin(), asAtomHandler::as<XMLList>(child2)->nodes.end());
			}
			th->incRef();
			ret = asAtomHandler::fromObject(th);
			return;
		}
	}
	asAtomHandler::setUndefined(ret);
}
ASFUNCTIONBODY_ATOM(XML,insertChildBefore)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom child1 = asAtomHandler::invalidAtom;
	asAtom child2 = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(child1)(child2));
	bool incref=true;
	if (th->nodetype != pugi::node_element)
	{
		asAtomHandler::setUndefined(ret);
		return;
	}
	if (asAtomHandler::is<XML>(child2))
	{
		if (th->CheckCyclicReference(asAtomHandler::as<XML>(child2)))
			return;
	}
	else if (asAtomHandler::is<XMLList>(child2))
	{
		for (auto it = asAtomHandler::as<XMLList>(child2)->nodes.begin(); it < asAtomHandler::as<XMLList>(child2)->nodes.end(); it++)
		{
			if (th->CheckCyclicReference((*it).getPtr()))
				return;
		}
	}
	else
	{
		child2 = asAtomHandler::fromObjectNoPrimitive(createFromString(wrk,asAtomHandler::toString(child2,wrk)));
		incref=false;
	}

	if (th->childrenlist.isNull())
		th->childrenlist = _MR(Class<XMLList>::getInstanceSNoArgs(wrk));
	if (asAtomHandler::isNull(child1))
	{
		if (asAtomHandler::is<XML>(child2))
		{
			th->appendChild(_NR<XML>(asAtomHandler::as<XML>(child2)));
			if (!incref)
				asAtomHandler::as<XML>(child2)->decRef();
		}
		else if (asAtomHandler::is<XMLList>(child2))
		{
			for (auto it = asAtomHandler::as<XMLList>(child2)->nodes.begin(); it < asAtomHandler::as<XMLList>(child2)->nodes.end(); it++)
			{
				(*it)->incRef();
				(*it)->parentNode = th;
				th->childrenlist->nodes.push_back(_NR<XML>(*it));
			}
		}
		th->incRef();
		ret = asAtomHandler::fromObject(th);
		return;
	}
	if (asAtomHandler::is<XMLList>(child1))
	{
		if (asAtomHandler::as<XMLList>(child1)->nodes.size()==0)
		{
			asAtomHandler::setUndefined(ret);
			return;
		}
		child1 = asAtomHandler::fromObjectNoPrimitive(asAtomHandler::as<XMLList>(child1)->nodes[0].getPtr());
	}
	for (auto it = th->childrenlist->nodes.begin(); it != th->childrenlist->nodes.end(); it++)
	{
		if ((*it).getPtr() == asAtomHandler::getObjectNoCheck(child1))
		{
			if (asAtomHandler::is<XML>(child2))
			{
				if (incref)
					asAtomHandler::as<XML>(child2)->incRef();
				asAtomHandler::as<XML>(child2)->parentNode = th;
				th->childrenlist->nodes.insert(it,_NR<XML>(asAtomHandler::as<XML>(child2)));
			}
			else if (asAtomHandler::is<XMLList>(child2))
			{
				for (auto it2 = asAtomHandler::as<XMLList>(child2)->nodes.begin(); it2 < asAtomHandler::as<XMLList>(child2)->nodes.end(); it2++)
				{
					(*it2)->parentNode = th;
				}
				th->childrenlist->nodes.insert(it,asAtomHandler::as<XMLList>(child2)->nodes.begin(), asAtomHandler::as<XMLList>(child2)->nodes.end());
			}
			th->incRef();
			ret = asAtomHandler::fromObject(th);
			return;
		}
	}
	asAtomHandler::setUndefined(ret);
}

ASFUNCTIONBODY_ATOM(XML,namespaceDeclarations)
{
	XML* th=asAtomHandler::as<XML>(obj);
	Array *namespaces = Class<Array>::getInstanceSNoArgs(wrk);
	for (uint32_t i = 0; i < th->namespacedefs.size(); i++)
	{
		_R<Namespace> tmpns = th->namespacedefs[i];
		bool b;
		if (tmpns->getPrefix(b) != BUILTIN_STRINGS::EMPTY)
		{
			tmpns->incRef();
			namespaces->push(asAtomHandler::fromObject(tmpns.getPtr()));
		}
	}
	ret = asAtomHandler::fromObject(namespaces);
}

ASFUNCTIONBODY_ATOM(XML,removeNamespace)
{
	XML* th=asAtomHandler::as<XML>(obj);
	_NR<ASObject> arg1;
	ARG_CHECK(ARG_UNPACK(arg1));
	Namespace* ns;
	if (arg1->is<Namespace>())
		ns = arg1->as<Namespace>();
	else
		ns = Class<Namespace>::getInstanceS(wrk,arg1->toStringId(), BUILTIN_STRINGS::EMPTY);

	th->RemoveNamespace(ns);
	th->incRef();
	ret = asAtomHandler::fromObject(th);
}
void XML::RemoveNamespace(Namespace *ns)
{
	if (this->nodenamespace_uri == ns->getURI())
	{
		this->nodenamespace_uri = BUILTIN_STRINGS::EMPTY;
		this->nodenamespace_prefix = BUILTIN_STRINGS::EMPTY;
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
	handleNotification("namespaceRemoved",asAtomHandler::fromObject(this),asAtomHandler::nullAtom);
}

ASFUNCTIONBODY_ATOM(XML,comments)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom name = asAtomHandler::invalidAtom;
	asAtom defname = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_WILDCARD);
	ARG_CHECK(ARG_UNPACK(name,defname));
	if (asAtomHandler::toStringId(name,wrk) != BUILTIN_STRINGS::STRING_WILDCARD)
		LOG(LOG_NOT_IMPLEMENTED,"XML.comments with name other than '*' is ignored "<<asAtomHandler::toDebugString(name));
	XMLVector res;
	th->getComments(res);
	ret = asAtomHandler::fromObject(XMLList::create(wrk,res,th->getChildrenlist(),multiname(nullptr)));
}
void XML::getComments(XMLVector& ret)
{
	if (childrenlist)
	{
		for (auto it = childrenlist->nodes.begin(); it != childrenlist->nodes.end(); it++)
		{
			if ((*it)->getNodeKind() == pugi::node_comment)
			{
				(*it)->incRef();
				ret.push_back(*it);
			}
		}
	}
}

ASFUNCTIONBODY_ATOM(XML,processingInstructions)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom name = asAtomHandler::invalidAtom;
	asAtom defname = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_WILDCARD);
	ARG_CHECK(ARG_UNPACK(name,defname));
	XMLVector res;
	th->getprocessingInstructions(res,asAtomHandler::toStringId(name,wrk));
	ret = asAtomHandler::fromObject(XMLList::create(wrk,res,th->getChildrenlist(),multiname(nullptr)));
}
void XML::getprocessingInstructions(XMLVector& ret, uint32_t name)
{
	if (childrenlist)
	{
		for (auto it = childrenlist->nodes.begin(); it != childrenlist->nodes.end(); it++)
		{
			if ((*it)->getNodeKind() == pugi::node_pi && (name == BUILTIN_STRINGS::STRING_WILDCARD || name == (*it)->nodenameID))
			{
				(*it)->incRef();
				ret.push_back(*it);
			}
		}
	}
}
ASFUNCTIONBODY_ATOM(XML,_propertyIsEnumerable)
{
	asAtomHandler::setBool(ret,argslen == 1 && asAtomHandler::toString(args[0],wrk) == "0" );
}
ASFUNCTIONBODY_ATOM(XML,_hasOwnProperty)
{
	if (!asAtomHandler::is<XML>(obj))
	{
		ASObject::hasOwnProperty(ret,wrk,obj,args,argslen);
		return;
	}
	XML* th=asAtomHandler::as<XML>(obj);
	tiny_string prop;
	ARG_CHECK(ARG_UNPACK(prop));

	bool res = false;
	if (prop == "0")
		res = true;
	else
	{
		multiname name(nullptr);
		name.name_type=multiname::NAME_STRING;
		name.name_s_id=asAtomHandler::toStringId(args[0],wrk);
		name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
		name.isAttribute=false;
		res=th->hasProperty(name,th == asAtomHandler::getObject(obj)->getprop_prototype(), true, true,wrk);
	}
	asAtomHandler::setBool(ret,res);
}

tiny_string XML::toString_priv()
{
	tiny_string ret;
	if (getNodeKind() == pugi::node_pcdata ||
		isAttribute ||
		getNodeKind() == pugi::node_cdata)
	{
		ret=nodevalue;
	}
	else if (getNodeKind() == pugi::node_comment ||
			 getNodeKind() == pugi::node_pi)
	{
		ret="";
	}
	else if (hasSimpleContent())
	{
		if (!childrenlist.isNull() && !childrenlist->nodes.empty())
		{
			auto it = childrenlist->nodes.begin();
			while(it != childrenlist->nodes.end())
			{
				if ((*it)->getNodeKind() != pugi::node_comment &&
						(*it)->getNodeKind() != pugi::node_pi)
					ret += (*it)->toString_priv();
				it++;
			}
		}
		else if (getNodeKind() == pugi::node_element && !attributelist.isNull() && !attributelist->nodes.empty())
		{
			ret=toXMLString_internal();
		}
	}
	else
	{
		ret=toXMLString_internal();
	}
	return ret;
}

bool XML::getPrettyPrinting()
{
	return prettyPrinting;
}
bool XML::getIgnoreComments()
{
	return ignoreComments;
}

unsigned int XML::getParseMode()
{
	unsigned int parsemode = pugi::parse_cdata | pugi::parse_escapes|pugi::parse_fragment | pugi::parse_doctype |pugi::parse_pi|pugi::parse_declaration|pugi::parse_validate_closing_tags;
	if (!ignoreWhitespace) parsemode |= pugi::parse_ws_pcdata;
	//if (!ignoreProcessingInstructions) parsemode |= pugi::parse_pi|pugi::parse_declaration;
	if (!ignoreComments) parsemode |= pugi::parse_comments;
	return parsemode;
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
int64_t XML::toInt64()
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

number_t XML::toNumber()
{
	if (!hasSimpleContent())
		return 0;
	return parseNumber(toString_priv(),getSystemState()->getSwfVersion()<11);
}

bool XML::nodesEqual(XML *a, XML *b) const
{
	assert(a && b);

	// type
	if(a->nodetype!=b->nodetype)
		return false;

	// name
	if(a->nodenameID!=b->nodenameID || 
	   (a->nodenameID != BUILTIN_STRINGS::EMPTY && 
	    a->nodenamespace_uri!=b->nodenamespace_uri))
		return false;

	// content
	if (a->nodevalue != b->nodevalue)
		return false;
	// attributes
	if (a->attributelist.isNull())
		return b->attributelist.isNull() || b->attributelist->nodes.size() == 0;
	if (b->attributelist.isNull())
		return a->attributelist.isNull() || a->attributelist->nodes.size() == 0;
	if (a->attributelist->nodes.size() != b->attributelist->nodes.size())
		return false;
	for (int i = 0; i < (int)a->attributelist->nodes.size(); i++)
	{
		_NR<XML> oa= a->attributelist->nodes[i];
		bool bequal = false;
		for (int j = 0; j < (int)b->attributelist->nodes.size(); j++)
		{
			_NR<XML> ob= b->attributelist->nodes[j];
			if (oa->isEqual(ob.getPtr()))
			{
				bequal = true;
				break;
			}
		}
		if (!bequal)
			return false;
	}
	if (!ignoreProcessingInstructions && (!a->procinstlist.isNull() || !b->procinstlist.isNull()))
	{
		if (a->procinstlist.isNull() || b->procinstlist.isNull())
			return false;
		if (a->procinstlist->nodes.size() != b->procinstlist->nodes.size())
			return false;
		for (int i = 0; i < (int)a->procinstlist->nodes.size(); i++)
		{
			_NR<XML> oa= a->procinstlist->nodes[i];
			bool bequal = false;
			for (int j = 0; j < (int)b->procinstlist->nodes.size(); j++)
			{
				_NR<XML> ob= b->procinstlist->nodes[j];
				if (oa->isEqual(ob.getPtr()))
				{
					bequal = true;
					break;
				}
			}
			if (!bequal)
				return false;
		}
	}
	
	// children
	if (a->childrenlist.isNull())
		return b->childrenlist.isNull() || b->childrenlist->nodes.size() == 0;
	if (b->childrenlist.isNull())
		return a->childrenlist.isNull() || a->childrenlist->nodes.size() == 0;
	
	return a->childrenlist->isEqual(b->childrenlist.getPtr());
}

uint32_t XML::nextNameIndex(uint32_t cur_index)
{
	if(cur_index < 1)
		return 1;
	else
		return 0;
}

void XML::nextName(asAtom& ret,uint32_t index)
{
	if(index<=1)
		asAtomHandler::setUInt(ret,this->getInstanceWorker(),index-1);
	else
		throw RunTimeException("XML::nextName out of bounds");
}

void XML::nextValue(asAtom& ret,uint32_t index)
{
	if(index<=1)
	{
		incRef();
		ret = asAtomHandler::fromObject(this);
	}
	else
		throw RunTimeException("XML::nextValue out of bounds");
}

bool XML::isEqual(ASObject* r)
{
	if (!isConstructed())
		return !r->isConstructed() || r->getObjectType() == T_NULL || r->getObjectType() == T_UNDEFINED;
	if(r->is<XML>())
		return nodesEqual(this, r->as<XML>());

	if(r->is<XMLList>())
		return r->as<XMLList>()->isEqual(this);

	if(hasSimpleContent())
		return toString()==r->toString();

	return false;
}

void XML::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
		    std::map<const ASObject*, uint32_t>& objMap,
		    std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing XML in AMF0 not implemented");
		return;
	}

	out->writeByte(xml_marker);
	out->writeXMLString(objMap, this, toString());
}

void XML::dumpTreeObjects(int indent)
{
	LOG(LOG_INFO,""<<std::string(2*indent,' ')<<getSystemState()->getStringFromUniqueId(this->nodenameID)<<" "<<this->toDebugString()<<" "<<this->attributelist.getPtr()<<" "<<this->childrenlist.getPtr());
	if (this->attributelist)
	{
		for (auto it= this->attributelist->nodes.begin();it != this->attributelist->nodes.end(); it++)
		{
			LOG(LOG_INFO,""<<std::string(2*indent,' ')<<" attribute: "<<getSystemState()->getStringFromUniqueId((*it)->nodenameID)<<" "<<(*it)->toDebugString());
		}
	}
	indent++;
	if (this->childrenlist)
	{
		for (auto it= this->childrenlist->nodes.begin();it != this->childrenlist->nodes.end(); it++)
		{
			(*it)->dumpTreeObjects(indent);
		}
	}
}

void XML::createTree(const pugi::xml_node& rootnode,bool fromXMLList)
{
	if (this->getInstanceWorker()->currentCallContext && this->getInstanceWorker()->currentCallContext->exceptionthrown)
		return;
	pugi::xml_node node = rootnode;
	bool done = false;
	if (this->childrenlist.isNull() || this->childrenlist->nodes.size() > 0)
	{
		this->childrenlist = _MR(Class<XMLList>::getInstanceSNoArgs(getInstanceWorker()));
	}
	if (!parentNode && !fromXMLList)
	{
		while (true)
		{
			if (this->getInstanceWorker()->currentCallContext && this->getInstanceWorker()->currentCallContext->exceptionthrown)
				return;
			//LOG(LOG_INFO,"rootfill:"<<node.name()<<" "<<node.value()<<" "<<node.type()<<" "<<parentNode.isNull());
			switch (node.type())
			{
				case pugi::node_null: // Empty (null) node handle
					fillNode(this,node);
					done = true;
					break;
				case pugi::node_document:// A document tree's absolute root
					createTree(node.first_child(),fromXMLList);
					return;
				case pugi::node_pi:	// Processing instruction, i.e. '<?name?>'
				case pugi::node_declaration: // Document declaration, i.e. '<?xml version="1.0"?>'
				{
					XML* tmp = Class<XML>::getInstanceSNoArgs(getInstanceWorker());
					fillNode(tmp,node);
					if(this->procinstlist.isNull())
						this->procinstlist = _MR(Class<XMLList>::getInstanceSNoArgs(getInstanceWorker()));
					this->procinstlist->incRef();
					this->procinstlist->append(_MNR(tmp));
					break;
				}
				case pugi::node_doctype:// Document type declaration, i.e. '<!DOCTYPE doc>'
					fillNode(this,node);
					break;
				case pugi::node_pcdata: // Plain character data, i.e. 'text'
				case pugi::node_cdata: // Character data, i.e. '<![CDATA[text]]>'
					fillNode(this,node);
					done = true;
					break;
				case pugi::node_comment: // Comment tag, i.e. '<!-- text -->'
					fillNode(this,node);
					break;
				case pugi::node_element: // Element tag, i.e. '<node/>'
				{
					fillNode(this,node);
					pugi::xml_node_iterator it=node.begin();
					while(it!=node.end())
					{
						//LOG(LOG_INFO,"rootchildnode1:"<<it->name()<<" "<<it->value()<<" "<<it->type()<<" "<<parentNode);
						this->childrenlist->append(_NR<XML>(XML::createFromNode(getInstanceWorker(),*it,this)));
						it++;
					}
					done = true;
					break;
				}
				default:
					LOG(LOG_ERROR,"createTree:unhandled type:" <<node.type());
					done=true;
					break;
			}
			if (done)
				break;
			node = node.next_sibling();
			if (node.type() == pugi::node_null)
				break;
		}
		// only one elemnt node in root allowed
		if (node.next_sibling().type() != pugi::node_null)
		{
			createError<TypeError>(getWorker(),kXMLMarkupMustBeWellFormed);
			return;
		}
		//LOG(LOG_INFO,"constructed:"<<this->toXMLString_internal());
	}
	else
	{
		switch (node.type())
		{
			case pugi::node_pi:	// Processing instruction, i.e. '<?name?>'
			case pugi::node_declaration: // Document declaration, i.e. '<?xml version="1.0"?>'
			case pugi::node_doctype:// Document type declaration, i.e. '<!DOCTYPE doc>'
			case pugi::node_pcdata: // Plain character data, i.e. 'text'
			case pugi::node_cdata: // Character data, i.e. '<![CDATA[text]]>'
			case pugi::node_comment: // Comment tag, i.e. '<!-- text -->'
				fillNode(this,node);
				break;
			case pugi::node_element: // Element tag, i.e. '<node/>'
			{
				fillNode(this,node);
				pugi::xml_node_iterator it=node.begin();
				{
					while(it!=node.end())
					{
						XML* tmp = XML::createFromNode(getInstanceWorker(),*it,this);
						this->childrenlist->append(_MNR(tmp));
						it++;
					}
				}
				break;
			}
			default:
				LOG(LOG_ERROR,"createTree:subtree unhandled type:" <<node.type());
				break;
		}
	}
}

void XML::fillNode(XML* node, const pugi::xml_node &srcnode)
{
	if (node->childrenlist.isNull())
	{
		node->childrenlist = _MR(Class<XMLList>::getInstanceSNoArgs(node->getInstanceWorker()));
	}
	tiny_string nodename = srcnode.name();
	node->nodetype = srcnode.type();
	node->nodenameID = node->getSystemState()->getUniqueStringId(nodename);
	node->nodevalue = srcnode.value();
	if (node->parentNode && node->parentNode->nodenamespace_prefix == BUILTIN_STRINGS::EMPTY)
		node->nodenamespace_uri = node->parentNode->nodenamespace_uri;
	else
		node->nodenamespace_uri = node->getInstanceWorker()->getDefaultXMLNamespaceID();
	if (ignoreWhitespace && node->nodetype == pugi::node_pcdata)
		node->nodevalue = node->nodevalue.removeWhitespace();
	node->attributelist = _MR(Class<XMLList>::getInstanceSNoArgs(node->getInstanceWorker()));
	pugi::xml_attribute_iterator itattr;
	for(itattr = srcnode.attributes_begin();itattr!=srcnode.attributes_end();++itattr)
	{
		tiny_string aname = tiny_string(itattr->name(),true);
		if(aname == "xmlns")
		{
			uint32_t uri = node->getSystemState()->getUniqueStringId(itattr->value());
			Namespace* ns = Class<Namespace>::getInstanceS(node->getInstanceWorker(),uri,BUILTIN_STRINGS::EMPTY);
			node->namespacedefs.push_back(_MR(ns));
			node->nodenamespace_uri = uri;
		}
		else if (aname.numBytes() >= 6 && aname.startsWith("xmlns:"))
		{
			uint32_t uri = node->getSystemState()->getUniqueStringId(itattr->value());
			tiny_string prefix = aname.substr(6,aname.end());
			Namespace* ns = Class<Namespace>::getInstanceS(node->getInstanceWorker(),uri,node->getSystemState()->getUniqueStringId(prefix));
			node->namespacedefs.push_back(_MR(ns));
		}
	}
	bool namespacefound = true;
	uint32_t pos = nodename.find(":");
	if (pos != tiny_string::npos)
	{
		// nodename has namespace
		if (pos==0)
		{
			createError<TypeError>(getWorker(),kXMLBadQName);
			return;
		}
		namespacefound=false;
		node->nodenamespace_prefix = node->getSystemState()->getUniqueStringId(nodename.substr(0,pos));
		node->nodenameID = node->getSystemState()->getUniqueStringId(nodename.substr(pos+1,nodename.end()));
		if (node->nodenamespace_prefix == BUILTIN_STRINGS::STRING_XML)
			node->nodenamespace_uri = BUILTIN_STRINGS::STRING_NAMESPACENS;
		else
		{
			XML* tmpnode = node;
			while (tmpnode)
			{
				for (auto itns = tmpnode->namespacedefs.begin(); itns != tmpnode->namespacedefs.end();itns++)
				{
					bool undefined;
					if ((*itns)->getPrefix(undefined) == node->nodenamespace_prefix)
					{
						node->nodenamespace_uri = (*itns)->getURI();
						namespacefound = true;
						break;
					}
				}
				if (namespacefound)
					break;
				if (!tmpnode->parentNode)
					break;
				tmpnode = tmpnode->parentNode;
			}
		}
	}
	if (!namespacefound)
	{
		createError<TypeError>(getWorker(),kXMLPrefixNotBound);
		return;
	}
	for(itattr = srcnode.attributes_begin();itattr!=srcnode.attributes_end();++itattr)
	{
		tiny_string aname = tiny_string(itattr->name(),true);
		if(aname == "xmlns" || (aname.numBytes() >= 6 && aname.startsWith("xmlns:")))
			continue;
		_NR<XML> tmp = _MR<XML>(Class<XML>::getInstanceSNoArgs(node->getInstanceWorker()));
		tmp->parentNode = node;
		tmp->nodetype = pugi::node_null;
		tmp->isAttribute = true;
		tmp->nodenameID = node->getSystemState()->getUniqueStringId(aname);
		tmp->nodenamespace_uri = node->getInstanceWorker()->getDefaultXMLNamespaceID();
		pos = aname.find(":");
		if (pos != tiny_string::npos)
		{
			tmp->nodenamespace_prefix = node->getSystemState()->getUniqueStringId(aname.substr(0,pos));
			tmp->nodenameID = node->getSystemState()->getUniqueStringId(aname.substr(pos+1,aname.end()));
			if (tmp->nodenamespace_prefix == BUILTIN_STRINGS::STRING_XML)
				tmp->nodenamespace_uri = BUILTIN_STRINGS::STRING_NAMESPACENS;
			else
			{
				XML* tmpnode = node;
				bool found = false;
				while (tmpnode)
				{
					for (auto itns = tmpnode->namespacedefs.begin(); itns != tmpnode->namespacedefs.end();itns++)
					{
						bool undefined;
						if ((*itns)->getPrefix(undefined) == tmp->nodenamespace_prefix)
						{
							tmp->nodenamespace_uri = (*itns)->getURI();
							found = true;
							break;
						}
					}
					if (found)
						break;
					if (!tmpnode->parentNode)
						break;
					tmpnode = tmpnode->parentNode;
				}
			}
		}
		tmp->nodevalue = itattr->value();
		tmp->constructed = true;
		// check for duplicates
		for (auto it = node->attributelist->nodes.begin(); it != node->attributelist->nodes.end(); it++)
		{
			if ((*it)->nodenameID == tmp->nodenameID &&
				(*it)->nodenamespace_prefix == tmp->nodenamespace_prefix &&
				(*it)->nodenamespace_uri == tmp->nodenamespace_uri
				)
			{
				createError<TypeError>(getWorker(),kXMLDuplicateAttribute,node->getSystemState()->getStringFromUniqueId(tmp->nodenameID),node->toString());
				break;
			}
		}
		node->attributelist->nodes.push_back(tmp);
		
	}
	node->constructed=true;
}

ASFUNCTIONBODY_ATOM(XML,_prependChild)
{
	XML* th=asAtomHandler::as<XML>(obj);
	assert_and_throw(argslen==1);
	XML* arg;
	if(asAtomHandler::is<XML>(args[0]))
	{
		ASATOM_INCREF(args[0]);
		arg=asAtomHandler::as<XML>(args[0]);
	}
	else if(asAtomHandler::is<XMLList>(args[0]))
	{
		XMLList* list=asAtomHandler::as<XMLList>(args[0]);
		list->prependNodesTo(th);
		th->incRef();
		ret = asAtomHandler::fromObject(th);
		return;
	}
	else
	{
		//The appendChild specs says that any other type is converted to string
		//NOTE: this is explicitly different from XML constructor, that will only convert to
		//string Numbers and Booleans
		tiny_string s = asAtomHandler::toString(args[0],wrk);
		if (wrk->getSystemState()->getSwfVersion() > 9)
		{
			arg=createFromString(wrk,"dummy");
			//avoid interpretation of the argument, just set it as text node
			arg->setTextContent(s);
		}
		else
		{
			arg=createFromString(wrk,s,true);
			if (wrk->currentCallContext->exceptionthrown)
			{
				arg=createFromString(wrk,"dummy");
				//avoid interpretation of the argument, just set it as text node
				arg->setTextContent(s);
				wrk->currentCallContext->exceptionthrown->decRef();
				wrk->currentCallContext->exceptionthrown=nullptr;
			}
		}
	}

	th->prependChild(_MNR(arg));
	th->incRef();
	ret = asAtomHandler::fromObject(th);
}
void XML::prependChild(_NR<XML> newChild)
{
	if (newChild && newChild->constructed)
	{
		if (this == newChild.getPtr())
		{
			createError<TypeError>(getInstanceWorker(),kXMLIllegalCyclicalLoop);
			return;
		}
		XML* node = this->parentNode;
		while (node)
		{
			if (node == newChild.getPtr())
			{
				createError<TypeError>(getInstanceWorker(),kXMLIllegalCyclicalLoop);
				return;
			}
			node = node->parentNode;
		}
		newChild->parentNode = this;
		childrenlist->prepend(newChild);
	}
}

ASFUNCTIONBODY_ATOM(XML,_replace)
{
	XML* th=asAtomHandler::as<XML>(obj);
	asAtom propertyName = asAtomHandler::invalidAtom;
	asAtom value = asAtomHandler::invalidAtom;
	ARG_CHECK(ARG_UNPACK(propertyName) (value));

	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;
	if (asAtomHandler::is<ASQName>(propertyName))
	{
		name.name_s_id=asAtomHandler::as<ASQName>(propertyName)->getLocalName();
		name.ns.emplace_back(wrk->getSystemState(),asAtomHandler::as<ASQName>(propertyName)->getURI(),NAMESPACE);
	}
	else if (asAtomHandler::toStringId(propertyName,wrk) == BUILTIN_STRINGS::STRING_WILDCARD)
	{
		if (asAtomHandler::is<XMLList>(value))
		{
			th->childrenlist->decRef();
			ASATOM_INCREF(value);
			th->childrenlist = _NR<XMLList>(asAtomHandler::as<XMLList>(value));
		}
		else if (asAtomHandler::is<XML>(value))
		{
			th->childrenlist->clear();
			ASATOM_INCREF(value);
			th->childrenlist->append(_MNR(asAtomHandler::as<XML>(value)));
		}
		else
		{
			XML* x = createFromString(wrk,asAtomHandler::toString(value,wrk));
			th->childrenlist->clear();
			th->childrenlist->append(_MNR(x));
		}
		th->incRef();
		ret = asAtomHandler::fromObject(th);
		return;
	}
	else
	{
		name.name_s_id=wrk->getSystemState()->getUniqueStringId(asAtomHandler::toString(propertyName,wrk));
		name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	}
	uint32_t index=0;
	if(XML::isValidMultiname(wrk->getSystemState(),name,index))
	{
		bool alreadyset=false;
		ASATOM_INCREF(value);
		th->childrenlist->setVariableByMultinameIntern(name,value,CONST_NOT_ALLOWED,true,&alreadyset,wrk);
		if (alreadyset)
			ASATOM_DECREF(value);
		
	}	
	else if (th->hasPropertyByMultiname(name,true,false,wrk))
	{
		if (asAtomHandler::is<XMLList>(value))
		{
			th->setVariableByMultinameIntern(name,value,CONST_NOT_ALLOWED,true,nullptr,wrk);
		}
		else if (asAtomHandler::is<XML>(value))
		{
			th->setVariableByMultinameIntern(name,value,CONST_NOT_ALLOWED,true,nullptr,wrk);
		}
		else
		{
			XML* x = createFromString(wrk,asAtomHandler::toString(value,wrk));
			asAtom v = asAtomHandler::fromObject(x);
			th->setVariableByMultinameIntern(name,v,CONST_NOT_ALLOWED,true,nullptr,wrk);
			x->decRef();
		}
	}
	th->incRef();
	ret = asAtomHandler::fromObject(th);
}
ASFUNCTIONBODY_ATOM(XML,setNotification)
{
	XML* th=asAtomHandler::as<XML>(obj);
	ARG_CHECK(ARG_UNPACK(th->notifierfunction));
}
ASFUNCTIONBODY_ATOM(XML,notification)
{
	XML* th=asAtomHandler::as<XML>(obj);
	if (th->notifierfunction.isNull())
		asAtomHandler::setNull(ret);
	else
	{
		th->notifierfunction->incRef();
		ret = asAtomHandler::fromObject(th->notifierfunction.getPtr());
	}
}
