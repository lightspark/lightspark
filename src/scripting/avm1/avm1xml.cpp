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

#include "avm1xml.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/Stage.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"

using namespace std;
using namespace lightspark;

AVM1XMLDocument::AVM1XMLDocument(ASWorker* wrk, Class_base* c):XMLDocument(wrk,c)
{
	subtype=SUBTYPE_AVM1XMLDOCUMENT;
	needsActionScript3=false;
}

void AVM1XMLDocument::finalize()
{
	XMLDocument::finalize();
	loader.reset();
}
bool AVM1XMLDocument::destruct()
{
	loader.reset();
	getSystemState()->stage->AVM1RemoveEventListener(this);
	return XMLDocument::destruct();
}

void AVM1XMLDocument::prepareShutdown()
{
	if (this->preparedforshutdown)
		return;
	XMLDocument::prepareShutdown();
	if (loader)
		loader->prepareShutdown();
}

void AVM1XMLDocument::sinit(Class_base* c)
{
	CLASS_SETUP(c, XMLDocument, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->isReusable=true;
	c->prototype->setVariableByQName("parseXML","",c->getSystemState()->getBuiltinFunction(parseXML),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("createElement","",c->getSystemState()->getBuiltinFunction(createElement,1,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("createTextNode","",c->getSystemState()->getBuiltinFunction(createTextNode,1,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("load","",c->getSystemState()->getBuiltinFunction(load),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("idmap","",c->getSystemState()->getBuiltinFunction(_idmap,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("docTypeDecl	","",c->getSystemState()->getBuiltinFunction(_docTypeDecl,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("xmlDecl","",c->getSystemState()->getBuiltinFunction(_xmlDecl,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getBytesTotal","",c->getSystemState()->getBuiltinFunction(getBytesTotal),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getBytesLoaded","",c->getSystemState()->getBuiltinFunction(getBytesLoaded),DYNAMIC_TRAIT);
	c->prototype->setDeclaredMethodByQName("_bytesTotal","",c->getSystemState()->getBuiltinFunction(getBytesTotal,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("_bytesLoaded","",c->getSystemState()->getBuiltinFunction(getBytesLoaded,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("status","",c->getSystemState()->getBuiltinFunction(_getter_status,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("firstChild","",c->getSystemState()->getBuiltinFunction(firstChild,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("ignoreWhite","",c->getSystemState()->getBuiltinFunction(_get_ignoreWhite,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("ignoreWhite","",c->getSystemState()->getBuiltinFunction(_set_ignoreWhite),SETTER_METHOD,false,false);
}

ASFUNCTIONBODY_GETTER(AVM1XMLDocument,status)

ASFUNCTIONBODY_ATOM(AVM1XMLDocument,_get_ignoreWhite)
{
	ret = asAtomHandler::undefinedAtom;

	if (asAtomHandler::is<AVM1XMLDocument>(obj))
	{
		AVM1XMLDocument* th = asAtomHandler::as<AVM1XMLDocument>(obj);
		ret = asAtomHandler::fromBool(th->ignoreWhite);
	}
	else
		ret = asAtomHandler::fromBool(wrk->getSystemState()->static_AVM1XMLDocument_ignoreWhite);
}

ASFUNCTIONBODY_ATOM(AVM1XMLDocument,_set_ignoreWhite)
{
	if (!argslen)
		return;
	if (asAtomHandler::is<AVM1XMLDocument>(obj))
	{
		AVM1XMLDocument* th = asAtomHandler::as<AVM1XMLDocument>(obj);
		th->ignoreWhite = asAtomHandler::AVM1toBool(args[0],wrk,wrk->AVM1getSwfVersion());
	}
	else
		wrk->getSystemState()->static_AVM1XMLDocument_ignoreWhite = asAtomHandler::AVM1toBool(args[0],wrk,wrk->AVM1getSwfVersion());
}

ASFUNCTIONBODY_ATOM(AVM1XMLDocument,load)
{
	AVM1XMLDocument* th=asAtomHandler::as<AVM1XMLDocument>(obj);
	if (th->loader.isNull())
		th->loader = _MR(Class<URLLoader>::getInstanceS(wrk));
	tiny_string url;
	ARG_CHECK(ARG_UNPACK(url));
	URLRequest* req = Class<URLRequest>::getInstanceS(wrk,url);
	asAtom urlarg = asAtomHandler::fromObjectNoPrimitive(req);
	asAtom loaderobj = asAtomHandler::fromObjectNoPrimitive(th->loader.getPtr());
	URLLoader::load(ret,wrk,loaderobj,&urlarg,1);
	req->decRef();
}
ASFUNCTIONBODY_ATOM(AVM1XMLDocument,getBytesTotal)
{
	AVM1XMLDocument* th=asAtomHandler::as<AVM1XMLDocument>(obj);
	ret = asAtomHandler::undefinedAtom;
	if (th->loader && th->loader->bytesTotal != UINT32_MAX)
		ret = asAtomHandler::fromUInt(th->loader->bytesTotal);
}
ASFUNCTIONBODY_ATOM(AVM1XMLDocument,getBytesLoaded)
{
	AVM1XMLDocument* th=asAtomHandler::as<AVM1XMLDocument>(obj);
	ret = asAtomHandler::undefinedAtom;
	if (th->loader)
		ret = asAtomHandler::fromUInt(th->loader->bytesLoaded);
}

multiname* AVM1XMLDocument::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset,ASWorker* wrk)
{
	multiname* res = XMLDocument::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);
	if (name.name_s_id == BUILTIN_STRINGS::STRING_ONLOAD || name.name_s_id == BUILTIN_STRINGS::STRING_ONDATA)
	{
		getSystemState()->stage->AVM1AddEventListener(this);
		setIsEnumerable(name, false);
	}
	return res;
}
void AVM1XMLDocument::AVM1HandleEvent(EventDispatcher* dispatcher, Event* e)
{
	if (dispatcher == loader.getPtr())
	{
		if (e->type == "complete" || e->type=="ioError")
		{
			asAtom func=asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONDATA;
			getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtom args[1];

				args[0] = loader->getData() ? asAtomHandler::fromObject(loader->getData()) : asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				asAtomHandler::as<AVM1Function>(func)->decRef();
			}

			if (e->type == "complete" && loader->getData())
			{
				std::string str = loader->getData()->toString().raw_buf();
				this->status = this->parseXMLImpl(str);
			}
			func=asAtomHandler::invalidAtom;
			m.name_type=multiname::NAME_STRING;
			m.isAttribute = false;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONLOAD;
			getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(this);
				asAtom args[1];
				args[0] = asAtomHandler::fromBool(e->type=="complete");
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,1);
				asAtomHandler::as<AVM1Function>(func)->decRef();
			}
		}
	}
}
AVM1XMLNode::AVM1XMLNode(ASWorker* wrk, Class_base* c):XMLNode(wrk,c)
{
	subtype = SUBTYPE_AVM1XMLNODE;
}

AVM1XMLNode::AVM1XMLNode(ASWorker* wrk, Class_base* c, XMLDocument* _r, pugi::xml_node _n, XMLNode* _p):
	XMLNode(wrk,c,_r,_n,_p)
{
	subtype = SUBTYPE_AVM1XMLNODE;
}
void AVM1XMLNode::sinit(Class_base* c)
{
	CLASS_SETUP(c, XMLNode, _constructor, CLASS_SEALED);
	c->isReusable = true;
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setDeclaredMethodByQName("attributes","",c->getSystemState()->getBuiltinFunction(attributes,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("childNodes","",c->getSystemState()->getBuiltinFunction(XMLNode::childNodes,0,Class<Array>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("firstChild","",c->getSystemState()->getBuiltinFunction(XMLNode::firstChild,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("lastChild","",c->getSystemState()->getBuiltinFunction(lastChild,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("nextSibling","",c->getSystemState()->getBuiltinFunction(nextSibling,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("nodeType","",c->getSystemState()->getBuiltinFunction(_getNodeType,0,Class<Integer>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("nodeName","",c->getSystemState()->getBuiltinFunction(_getNodeName,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("nodeName","",c->getSystemState()->getBuiltinFunction(_setNodeName),SETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("nodeValue","",c->getSystemState()->getBuiltinFunction(_getNodeValue,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("parentNode","",c->getSystemState()->getBuiltinFunction(parentNode,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("previousSibling","",c->getSystemState()->getBuiltinFunction(previousSibling,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("localName","",c->getSystemState()->getBuiltinFunction(_getLocalName,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setVariableByQName("appendChild","",c->getSystemState()->getBuiltinFunction(appendChild),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("cloneNode","",c->getSystemState()->getBuiltinFunction(cloneNode,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("removeNode","",c->getSystemState()->getBuiltinFunction(removeNode,0,Class<XMLNode>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("hasChildNodes","",c->getSystemState()->getBuiltinFunction(hasChildNodes,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("insertBefore","",c->getSystemState()->getBuiltinFunction(insertBefore,2),DYNAMIC_TRAIT);
	c->prototype->setDeclaredMethodByQName("prefix","",c->getSystemState()->getBuiltinFunction(prefix,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setDeclaredMethodByQName("namespaceURI","",c->getSystemState()->getBuiltinFunction(namespaceURI,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false,false);
	c->prototype->setVariableByQName("getNamespaceForPrefix","",c->getSystemState()->getBuiltinFunction(getNamespaceForPrefix,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("getPrefixForNamespace","",c->getSystemState()->getBuiltinFunction(getPrefixForNamespace,1,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
}
