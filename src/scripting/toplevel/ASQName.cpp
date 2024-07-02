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

#include "scripting/toplevel/ASQName.h"
#include "scripting/toplevel/XML.h"
#include "scripting/class.h"

using namespace lightspark;

ASQName::ASQName(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_QNAME),uri_is_null(false),uri(0),local_name(0)
{
}
void ASQName::setByXML(XML* node)
{
	uri_is_null=false;
	local_name = node->getNameID();
	uri=node->getNamespaceURI();
}

void ASQName::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	c->setDeclaredMethodByQName("uri","",c->getSystemState()->getBuiltinFunction(_getURI),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("localName","",c->getSystemState()->getBuiltinFunction(_getLocalName),GETTER_METHOD,true);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(ASQName,_constructor)
{
	ASQName* th=asAtomHandler::as<ASQName>(obj);
	assert_and_throw(argslen<3);

	asAtom nameval=asAtomHandler::invalidAtom;
	asAtom namespaceval=asAtomHandler::invalidAtom;

	if(argslen==0)
	{
		th->local_name=BUILTIN_STRINGS::EMPTY;
		th->uri_is_null=false;
		th->uri=wrk->getSystemState()->getUniqueStringId(wrk->getDefaultXMLNamespace());
		return;
	}
	if(argslen==1)
	{
		nameval=args[0];
	}
	else if(argslen==2)
	{
		namespaceval=args[0];
		nameval=args[1];
	}

	// Set local_name
	if(asAtomHandler::isQName(nameval))
	{
		ASQName *q=asAtomHandler::as<ASQName>(nameval);
		th->local_name=q->local_name;
		if(asAtomHandler::isInvalid(namespaceval))
		{
			th->uri_is_null=q->uri_is_null;
			th->uri=q->uri;
			return;
		}
	}
	else if(asAtomHandler::isUndefined(nameval))
		th->local_name=BUILTIN_STRINGS::EMPTY;
	else
		th->local_name=asAtomHandler::toStringId(nameval,wrk);

	// Set uri
	th->uri_is_null=false;
	if(asAtomHandler::isInvalid(namespaceval) || asAtomHandler::isUndefined(namespaceval))
	{
		if(th->local_name==BUILTIN_STRINGS::STRING_WILDCARD)
		{
			th->uri_is_null=true;
			th->uri=BUILTIN_STRINGS::EMPTY;
		}
		else
		{
			th->uri=wrk->getDefaultXMLNamespaceID();
		}
	}
	else if(asAtomHandler::isNull(namespaceval))
	{
		th->uri_is_null=true;
		th->uri=BUILTIN_STRINGS::EMPTY;
	}
	else
	{
		if(asAtomHandler::isQName(namespaceval) &&
		   !(asAtomHandler::as<ASQName>(namespaceval)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(namespaceval);
			th->uri=q->uri;
		}
		else if (asAtomHandler::isValid(namespaceval))
			th->uri=asAtomHandler::toStringId(namespaceval,wrk);
	}
}
ASFUNCTIONBODY_ATOM(ASQName,generator)
{
	ASQName* th=Class<ASQName>::getInstanceS(wrk);
	assert_and_throw(argslen<3);

	asAtom nameval=asAtomHandler::invalidAtom;
	asAtom namespaceval=asAtomHandler::invalidAtom;

	if(argslen==0)
	{
		th->local_name=BUILTIN_STRINGS::EMPTY;
		th->uri_is_null=false;
		th->uri=wrk->getDefaultXMLNamespaceID();
		ret = asAtomHandler::fromObject(th);
		return;
	}
	if(argslen==1)
	{
		nameval=args[0];
		namespaceval=asAtomHandler::invalidAtom;
	}
	else if(argslen==2)
	{
		namespaceval=args[0];
		nameval=args[1];
	}

	// Set local_name
	if(asAtomHandler::isQName(nameval))
	{
		ASQName *q=asAtomHandler::as<ASQName>(nameval);
		th->local_name=q->local_name;
		if(asAtomHandler::isInvalid(namespaceval))
		{
			th->uri_is_null=q->uri_is_null;
			th->uri=q->uri;
			ret = asAtomHandler::fromObject(th);
			return;
		}
	}
	else if(asAtomHandler::isUndefined(nameval))
		th->local_name=BUILTIN_STRINGS::EMPTY;
	else
		th->local_name=asAtomHandler::toStringId(nameval,wrk);

	// Set uri
	th->uri_is_null=false;
	if(asAtomHandler::isInvalid(namespaceval) || asAtomHandler::isUndefined(namespaceval))
	{
		if(th->local_name==BUILTIN_STRINGS::STRING_WILDCARD)
		{
			th->uri_is_null=true;
			th->uri=BUILTIN_STRINGS::EMPTY;
		}
		else
		{
			th->uri=wrk->getDefaultXMLNamespaceID();
		}
	}
	else if(asAtomHandler::isNull(namespaceval))
	{
		th->uri_is_null=true;
		th->uri=BUILTIN_STRINGS::EMPTY;
	}
	else
	{
		if(asAtomHandler::isQName(namespaceval) &&
		   !(asAtomHandler::as<ASQName>(namespaceval)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(namespaceval);
			th->uri=q->uri;
		}
		else if (asAtomHandler::isValid(namespaceval))
			th->uri=asAtomHandler::toStringId(namespaceval,wrk);
	}
	ret = asAtomHandler::fromObject(th);
}

ASFUNCTIONBODY_ATOM(ASQName,_getURI)
{
	ASQName* th=asAtomHandler::as<ASQName>(obj);
	if(th->uri_is_null)
		asAtomHandler::setNull(ret);
	else
		ret = asAtomHandler::fromStringID(th->uri);
}

ASFUNCTIONBODY_ATOM(ASQName,_getLocalName)
{
	ASQName* th=asAtomHandler::as<ASQName>(obj);
	ret = asAtomHandler::fromStringID(th->local_name);
}

ASFUNCTIONBODY_ATOM(ASQName,_toString)
{
	if(!asAtomHandler::is<ASQName>(obj))
	{
		createError<TypeError>(wrk,0,"QName.toString is not generic");
		return;
	}
	ASQName* th=asAtomHandler::as<ASQName>(obj);
	ret = asAtomHandler::fromString(wrk->getSystemState(),th->toString());
}

bool ASQName::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_QNAME)
	{
		ASQName *q=static_cast<ASQName *>(o);
		return uri_is_null==q->uri_is_null && uri==q->uri && local_name==q->local_name;
	}

	return ASObject::isEqual(o);
}

tiny_string ASQName::toString()
{
	tiny_string s;
	if(uri_is_null)
		s = "*::";
	else if(uri!=BUILTIN_STRINGS::EMPTY)
		s = getSystemState()->getStringFromUniqueId(uri) + "::";

	return s + getSystemState()->getStringFromUniqueId(local_name);
}

uint32_t ASQName::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<2)
		return cur_index+1;
	else
	{
		//Fall back on object properties
		uint32_t ret=ASObject::nextNameIndex(cur_index-2);
		if(ret==0)
			return 0;
		else
			return ret+2;

	}
}

void ASQName::nextName(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	switch(index)
	{
		case 1:
			ret = asAtomHandler::fromString(getSystemState(),"uri");
			break;
		case 2:
			ret = asAtomHandler::fromString(getSystemState(),"localName");
			break;
		default:
			ASObject::nextName(ret,index-2);
			break;
	}
}

void ASQName::nextValue(asAtom &ret, uint32_t index)
{
	assert_and_throw(implEnable);
	switch(index)
	{
		case 1:
			if (uri_is_null)
				asAtomHandler::setNull(ret);
			else
				ret = asAtomHandler::fromStringID(this->uri);
			break;
		case 2:
			ret = asAtomHandler::fromStringID(this->local_name);
			break;
		default:
			ASObject::nextValue(ret,index-2);
			break;
	}
}
