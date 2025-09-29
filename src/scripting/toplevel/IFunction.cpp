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
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/toplevel/IFunction.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/XML.h"
#include "swf.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"

using namespace std;
using namespace lightspark;

IFunction::IFunction(ASWorker* wrk,Class_base* c,CLASS_SUBTYPE st):ASObject(wrk,c,T_FUNCTION,st),length(0),
	closure_this(asAtomHandler::invalidAtom),
	inClass(nullptr),isStatic(false),isGetter(false),isSetter(false),namespaceNameID(BUILTIN_STRINGS::EMPTY),
	clonedFrom(nullptr),prototype(asAtomHandler::invalidAtom),functionname(0)
{
}

void IFunction::sinit(Class_base* c)
{
	c->isReusable=true;
	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(IFunction::_toString),DYNAMIC_TRAIT);

	c->setDeclaredMethodByQName("call","",c->getSystemState()->getBuiltinFunction(IFunction::_call,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("call",AS3,c->getSystemState()->getBuiltinFunction(IFunction::_call,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("apply","",c->getSystemState()->getBuiltinFunction(IFunction::apply,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("apply",AS3,c->getSystemState()->getBuiltinFunction(IFunction::apply,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(IFunction::_length,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("length","",c->getSystemState()->getBuiltinFunction(IFunction::_length,0,Class<UInteger>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("toString","",c->getSystemState()->getBuiltinFunction(IFunction::_toString),NORMAL_METHOD,false);
}

void IFunction::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	ASObject::prepareShutdown();
	if (clonedFrom)
		clonedFrom->prepareShutdown();
	if (asAtomHandler::isAccessibleObject(closure_this))
	{
		asAtomHandler::getObjectNoCheck(closure_this)->prepareShutdown();
		ASATOM_REMOVESTOREDMEMBER(closure_this);
		closure_this=asAtomHandler::invalidAtom;
	}
	ASObject* pr = asAtomHandler::getObject(prototype);
	if (pr)
		pr->prepareShutdown();
}

bool IFunction::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	bool ret = ASObject::countCylicMemberReferences(gcstate);
	if (asAtomHandler::isAccessibleObject(closure_this))
		ret = asAtomHandler::getObjectNoCheck(closure_this)->countAllCylicMemberReferences(gcstate) || ret;
	ASObject* pr = asAtomHandler::getObject(prototype);
	if (pr)
		ret = pr->countAllCylicMemberReferences(gcstate) || ret;
	return ret;
}

ASFUNCTIONBODY_GETTER(IFunction,prototype)
void IFunction::_setter_prototype(asAtom& ret,ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen)
{
	if(!asAtomHandler::is<IFunction>(obj))
	{
		createError<ArgumentError>(wrk,0,"Function applied to wrong object");
		return;
	}
	IFunction* th = asAtomHandler::as<IFunction>(obj);
	if(argslen != 1)
	{
		createError<ArgumentError>(wrk,0,"Arguments provided in setter");
		return;
	}
	ASATOM_REMOVESTOREDMEMBER(th->prototype);
	th->prototype = args[0];
	ASATOM_ADDSTOREDMEMBER(th->prototype);
}

ASFUNCTIONBODY_ATOM(IFunction,_length)
{
	if (asAtomHandler::is<IFunction>(obj))
	{
		IFunction* th=asAtomHandler::as<IFunction>(obj);
		asAtomHandler::setUInt(ret,wrk,th->length);
	}
	else
		asAtomHandler::setUInt(ret,wrk,1);
}

ASFUNCTIONBODY_ATOM(IFunction,apply)
{
	assert_and_throw(argslen<=2);

	asAtom newObj=asAtomHandler::invalidAtom;
	asAtom* newArgs=nullptr;
	int newArgsLen=0;
	if(asAtomHandler::is<IFunction>(obj))
	{
		/* This function never changes the 'this' pointer of a method closure */
		IFunction* f = asAtomHandler::as<IFunction>(obj);
		if (f->inClass && asAtomHandler::isValid(f->closure_this))
			newObj = f->closure_this;
	}
	if(asAtomHandler::isInvalid(newObj))
	{
		//Validate parameters
		if(argslen==0 || asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
			newObj = wrk->getCurrentGlobalAtom(argslen==0 ? asAtomHandler::nullAtom : args[0]) ;
		else
			newObj=args[0];
	}
	if(argslen == 2 && !asAtomHandler::is<Null>(args[1]) && !asAtomHandler::is<Undefined>(args[1]))
	{
		if (!asAtomHandler::isArray(args[1]))
		{
			createError<TypeError>(wrk,kApplyError);
			return;
		}
		Array* array=asAtomHandler::as<Array>(args[1]);
		newArgsLen=array->size();
		newArgs=new asAtom[newArgsLen];
		for(int i=0;i<newArgsLen;i++)
		{
			asAtom val = array->at(i);
			newArgs[i]=val;
		}
	}
	if (asAtomHandler::is<Class_base>(obj))
		asAtomHandler::as<Class_base>(obj)->generator(wrk,ret,newArgs,newArgsLen);
	else if (asAtomHandler::is<IFunction>(obj))
		asAtomHandler::callFunction(obj,wrk,ret,newObj,newArgs,newArgsLen,false);
	else
		ret = asAtomHandler::undefinedAtom;
	if (newArgs)
		delete[] newArgs;
}

ASFUNCTIONBODY_ATOM(IFunction,_call)
{
	asAtom newObj=asAtomHandler::invalidAtom;
	asAtom* newArgs=nullptr;
	uint32_t newArgsLen=0;
	if(asAtomHandler::is<IFunction>(obj))
	{
		/* This function never changes the 'this' pointer of a method closure */
		IFunction* f = asAtomHandler::as<IFunction>(obj);
		if (f->inClass && asAtomHandler::isValid(f->closure_this))
			newObj = f->closure_this;
	}
	if(asAtomHandler::isInvalid(newObj))
	{
		if(argslen==0 || asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
			newObj = wrk->getCurrentGlobalAtom(argslen==0 ? asAtomHandler::nullAtom : args[0]) ;
		else
			newObj=args[0];
	}
	if(argslen > 1)
	{
		newArgsLen=argslen-1;
		newArgs=LS_STACKALLOC(asAtom, newArgsLen);
		for(unsigned int i=0;i<newArgsLen;i++)
		{
			newArgs[i]=args[i+1];
		}
	}
	if (asAtomHandler::is<Class_base>(obj))
	{
		if (wrk->needsActionScript3())
		{
			ret = asAtomHandler::undefinedAtom;
			createError<TypeError>(wrk,kCallOfNonFunctionError);
		}
		else
			asAtomHandler::as<Class_base>(obj)->generator(wrk,ret,newArgs,newArgsLen);
	}
	else if (asAtomHandler::is<IFunction>(obj))
		asAtomHandler::callFunction(obj,wrk,ret,newObj,newArgs,newArgsLen,false);
	else
		ret = asAtomHandler::undefinedAtom;
}

ASFUNCTIONBODY_ATOM(IFunction,_toString)
{
	if (asAtomHandler::is<Class_base>(obj))
		ret = asAtomHandler::fromString(wrk->getSystemState(),"[class Function]");
	else if (wrk->needsActionScript3())
		ret = asAtomHandler::fromString(wrk->getSystemState(),"function Function() {}");
	else
		ret = asAtomHandler::fromString(wrk->getSystemState(),"[type Function]");
}

ASObject *IFunction::describeType(ASWorker* wrk) const
{
	pugi::xml_document p;
	pugi::xml_node root = p.append_child("type");
	root.append_attribute("name").set_value("Function");
	root.append_attribute("base").set_value("Object");
	root.append_attribute("isDynamic").set_value("true");
	root.append_attribute("isFinal").set_value("false");
	root.append_attribute("isStatic").set_value("false");

	pugi::xml_node node=root.append_child("extendsClass");
	node.append_attribute("type").set_value("Object");

	// TODO: accessor
	LOG(LOG_NOT_IMPLEMENTED, "describeType for Function not completely implemented");

	return XML::createFromNode(wrk, root);
}

std::string IFunction::toDebugString() const
{
	string ret = ASObject::toDebugString()+(asAtomHandler::isValid(closure_this) ? "(closure:"+asAtomHandler::toDebugString(closure_this)+")":"")+(clonedFrom ?" cloned":"");
#ifndef NDEBUG
	if (functionname)
	{
		ret +=" n:";
		ret += getSystemState()->getStringFromUniqueId(functionname);
	}
	if (inClass)
	{
		ret += " c:";
		ret += inClass->toDebugString();
	}
#endif
	return ret;
}
void IFunction::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap,ASWorker* wrk)
{
	// according to avmplus functions are "serialized" as undefined
	if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
		out->writeByte(amf0_undefined_marker);
	else
		out->writeByte(undefined_marker);
}

