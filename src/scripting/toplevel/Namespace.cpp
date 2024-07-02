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

#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Namespace.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/ASQName.h"

using namespace std;
using namespace lightspark;

Namespace::Namespace(ASWorker* wrk, Class_base* c):ASObject(wrk,c,T_NAMESPACE),nskind(NAMESPACE),prefix_is_undefined(false),uri(BUILTIN_STRINGS::EMPTY),prefix(BUILTIN_STRINGS::EMPTY)
{
}

Namespace::Namespace(ASWorker* wrk,Class_base* c, uint32_t _uri, uint32_t _prefix, NS_KIND _nskind, bool _prefix_is_undefined)
  : ASObject(wrk,c,T_NAMESPACE),nskind(_nskind),prefix_is_undefined(_prefix_is_undefined),uri(_uri),prefix(_prefix)
{
}

void Namespace::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED | CLASS_FINAL);
	//c->setDeclaredMethodByQName("uri","",c->getSystemState()->getBuiltinFunction(_setURI),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("uri","",c->getSystemState()->getBuiltinFunction(_getURI),GETTER_METHOD,true);
	//c->setDeclaredMethodByQName("prefix","",c->getSystemState()->getBuiltinFunction(_setPrefix),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("prefix","",c->getSystemState()->getBuiltinFunction(_getPrefix),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("valueOf",AS3,c->getSystemState()->getBuiltinFunction(_valueOf),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",c->getSystemState()->getBuiltinFunction(_ECMA_valueOf),DYNAMIC_TRAIT);
}

ASFUNCTIONBODY_ATOM(Namespace,_constructor)
{
	asAtom urival=asAtomHandler::invalidAtom;
	asAtom prefixval=asAtomHandler::invalidAtom;
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	assert_and_throw(argslen<3);

	if (argslen == 0)
	{
		//Return before resetting the value to preserve those eventually set by the C++ constructor
		return;
	}
	else if (argslen == 1)
	{
		urival = args[0];
		prefixval = asAtomHandler::invalidAtom;
	}
	else
	{
		prefixval = args[0];
		urival = args[1];
	}
	th->prefix_is_undefined=false;
	th->prefix = BUILTIN_STRINGS::EMPTY;
	th->uri = BUILTIN_STRINGS::EMPTY;
;

	if(asAtomHandler::isInvalid(prefixval))
	{
		if(asAtomHandler::isNamespace(urival))
		{
			Namespace* n=asAtomHandler::as<Namespace>(urival);
			th->uri=n->uri;
			th->prefix=n->prefix;
			th->prefix_is_undefined=n->prefix_is_undefined;
		}
		else if(asAtomHandler::isQName(urival) &&
		   !(asAtomHandler::as<ASQName>(urival)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=asAtomHandler::toStringId(urival,wrk);
			if(th->uri!=BUILTIN_STRINGS::EMPTY)
			{
				th->prefix_is_undefined=true;
				th->prefix=BUILTIN_STRINGS::EMPTY;
			}
		}
	}
	else // has both urival and prefixval
	{
		if(asAtomHandler::isQName(urival) &&
		   !(asAtomHandler::as<ASQName>(urival)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=asAtomHandler::toStringId(urival,wrk);
		}

		if(th->uri==BUILTIN_STRINGS::EMPTY)
		{
			if(asAtomHandler::isUndefined(prefixval) ||
			   asAtomHandler::toString(prefixval,wrk)=="")
				th->prefix=BUILTIN_STRINGS::EMPTY;
			else
			{
				tiny_string s = asAtomHandler::toString(prefixval,wrk);
				createError<TypeError>(wrk,kXMLNamespaceWithPrefixAndNoURI,s);
				return;
			}
		}
		else if(asAtomHandler::isUndefined(prefixval) ||
			!isXMLName(wrk,prefixval))
		{
			th->prefix_is_undefined=true;
			th->prefix=BUILTIN_STRINGS::EMPTY;
		}
		else
		{
			th->prefix=asAtomHandler::toStringId(prefixval,wrk);
		}
	}
}
ASFUNCTIONBODY_ATOM(Namespace,generator)
{
	Namespace* th=Class<Namespace>::getInstanceS(wrk);
	asAtom urival=asAtomHandler::invalidAtom;
	asAtom prefixval=asAtomHandler::invalidAtom;
	assert_and_throw(argslen<3);

	if (argslen == 0)
	{
		th->prefix_is_undefined=false;
		th->prefix = BUILTIN_STRINGS::EMPTY;
		th->uri = BUILTIN_STRINGS::EMPTY;
		ret = asAtomHandler::fromObject(th);
		return;
	}
	else if (argslen == 1)
	{
		urival = args[0];
		prefixval = asAtomHandler::invalidAtom;
	}
	else
	{
		prefixval = args[0];
		urival = args[1];
	}
	th->prefix_is_undefined=false;
	th->prefix = BUILTIN_STRINGS::EMPTY;
	th->uri = BUILTIN_STRINGS::EMPTY;

	if(asAtomHandler::isInvalid(prefixval))
	{
		if(asAtomHandler::isNamespace(urival))
		{
			Namespace* n=asAtomHandler::as<Namespace>(urival);
			th->uri=n->uri;
			th->prefix=n->prefix;
			th->prefix_is_undefined=n->prefix_is_undefined;
		}
		else if(asAtomHandler::isQName(urival) &&
		   !(asAtomHandler::as<ASQName>(urival)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=asAtomHandler::toStringId(urival,wrk);
			if(th->uri!=BUILTIN_STRINGS::EMPTY)
			{
				th->prefix_is_undefined=true;
				th->prefix=BUILTIN_STRINGS::EMPTY;
			}
		}
	}
	else // has both urival and prefixval
	{
		if(asAtomHandler::isQName(urival) &&
		   !(asAtomHandler::as<ASQName>(urival)->uri_is_null))
		{
			ASQName* q=asAtomHandler::as<ASQName>(urival);
			th->uri=q->uri;
		}
		else
		{
			th->uri=asAtomHandler::toStringId(urival,wrk);
		}

		if(th->uri==BUILTIN_STRINGS::EMPTY)
		{
			if(asAtomHandler::isUndefined(prefixval) ||
			   asAtomHandler::toString(prefixval,wrk)=="")
				th->prefix=BUILTIN_STRINGS::EMPTY;
			else
			{
				createError<TypeError>(wrk,0,"Namespace prefix for empty uri not allowed");
				return;
			}
		}
		else if(asAtomHandler::isUndefined(prefixval) ||
			!isXMLName(wrk,prefixval))
		{
			th->prefix_is_undefined=true;
			th->prefix=BUILTIN_STRINGS::EMPTY;
		}
		else
		{
			th->prefix=asAtomHandler::toStringId(prefixval,wrk);
		}
	}
	ret = asAtomHandler::fromObject(th);
}
/*
ASFUNCTIONBODY_ATOM(Namespace,_setURI)
{
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	th->uri=asAtomHandler::toString(args[0],);
}
*/
ASFUNCTIONBODY_ATOM(Namespace,_getURI)
{
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	ret = asAtomHandler::fromStringID(th->uri);
}
/*
ASFUNCTIONBODY_ATOM(Namespace,_setPrefix)
{
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	if(asAtomHandler::isUndefined(args[0]))
	{
		th->prefix_is_undefined=true;
		th->prefix="";
	}
	else
	{
		th->prefix_is_undefined=false;
		th->prefix=asAtomHandler::toString(args[0],);
	}
}
*/
ASFUNCTIONBODY_ATOM(Namespace,_getPrefix)
{
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	if(th->prefix_is_undefined)
		asAtomHandler::setUndefined(ret);
	else
		ret = asAtomHandler::fromStringID(th->prefix);
}

ASFUNCTIONBODY_ATOM(Namespace,_toString)
{
	if(!asAtomHandler::is<Namespace>(obj))
	{
		createError<TypeError>(wrk,0,"Namespace.toString is not generic");
		return;
	}
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	ret = asAtomHandler::fromStringID(th->uri);
}

ASFUNCTIONBODY_ATOM(Namespace,_valueOf)
{
	ret = asAtomHandler::fromStringID(asAtomHandler::as<Namespace>(obj)->uri);
}

ASFUNCTIONBODY_ATOM(Namespace,_ECMA_valueOf)
{
	if(!asAtomHandler::is<Namespace>(obj))
	{
		createError<TypeError>(wrk,0,"Namespace.valueOf is not generic");
		return;
	}
	Namespace* th=asAtomHandler::as<Namespace>(obj);
	ret = asAtomHandler::fromStringID(th->uri);
}

bool Namespace::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_NAMESPACE)
	{
		Namespace *n=static_cast<Namespace *>(o);
		return uri==n->uri;
	}

	return ASObject::isEqual(o);
}

uint32_t Namespace::nextNameIndex(uint32_t cur_index)
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

void Namespace::nextName(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	switch(index)
	{
		case 1:
			ret = asAtomHandler::fromString(getSystemState(),"uri");
			break;
		case 2:
			ret = asAtomHandler::fromString(getSystemState(),"prefix");
			break;
		default:
			ASObject::nextName(ret,index-2);
			break;
	}
}

void Namespace::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	switch(index)
	{
		case 1:
			ret = asAtomHandler::fromStringID(this->uri);
			break;
		case 2:
			if(prefix_is_undefined)
				asAtomHandler::setUndefined(ret);
			else
				ret = asAtomHandler::fromStringID(this->prefix);
			break;
		default:
			ASObject::nextValue(ret,index-2);
			break;
	}
}
