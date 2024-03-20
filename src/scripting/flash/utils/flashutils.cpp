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

#include "scripting/abc.h"
#include "scripting/flash/utils/flashutils.h"
#include "asobject.h"
#include "scripting/class.h"
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/IntervalManager.h"
#include "scripting/flash/display/DisplayObject.h"
#include <sstream>
#include <zlib.h>
#include <glib.h>

using namespace std;
using namespace lightspark;

#define BA_CHUNK_SIZE 4096

const char* Endian::littleEndian = "littleEndian";
const char* Endian::bigEndian = "bigEndian";

void Endian::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED | CLASS_FINAL);
	c->setVariableAtomByQName("LITTLE_ENDIAN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),littleEndian),DECLARED_TRAIT);
	c->setVariableAtomByQName("BIG_ENDIAN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),bigEndian),DECLARED_TRAIT);
}

void IExternalizable::linkTraits(Class_base* c)
{
	lookupAndLink(c,"readExternal","flash.utils:IExternalizable");
	lookupAndLink(c,"writeExternal","flash.utils:IExternalizable");
}

void IDataInput::linkTraits(Class_base* c)
{
	lookupAndLink(c,"bytesAvailable","flash.utils:IDataInput");
	lookupAndLink(c,"endian","flash.utils:IDataInput");
	lookupAndLink(c,"objectEncoding","flash.utils:IDataInput");
	lookupAndLink(c,"readBoolean","flash.utils:IDataInput");
	lookupAndLink(c,"readByte","flash.utils:IDataInput");
	lookupAndLink(c,"readBytes","flash.utils:IDataInput");
	lookupAndLink(c,"readDouble","flash.utils:IDataInput");
	lookupAndLink(c,"readFloat","flash.utils:IDataInput");
	lookupAndLink(c,"readInt","flash.utils:IDataInput");
	lookupAndLink(c,"readMultiByte","flash.utils:IDataInput");
	lookupAndLink(c,"readObject","flash.utils:IDataInput");
	lookupAndLink(c,"readShort","flash.utils:IDataInput");
	lookupAndLink(c,"readUnsignedByte","flash.utils:IDataInput");
	lookupAndLink(c,"readUnsignedInt","flash.utils:IDataInput");
	lookupAndLink(c,"readUnsignedShort","flash.utils:IDataInput");
	lookupAndLink(c,"readUTF","flash.utils:IDataInput");
	lookupAndLink(c,"readUTFBytes","flash.utils:IDataInput");
}

void IDataOutput::linkTraits(Class_base* c)
{
	lookupAndLink(c,"endian","flash.utils:IDataOutput");
	lookupAndLink(c,"objectEncoding","flash.utils:IDataOutput");
	lookupAndLink(c,"writeBoolean","flash.utils:IDataOutput");
	lookupAndLink(c,"writeByte","flash.utils:IDataOutput");
	lookupAndLink(c,"writeBytes","flash.utils:IDataOutput");
	lookupAndLink(c,"writeDouble","flash.utils:IDataOutput");
	lookupAndLink(c,"writeFloat","flash.utils:IDataOutput");
	lookupAndLink(c,"writeInt","flash.utils:IDataOutput");
	lookupAndLink(c,"writeMultiByte","flash.utils:IDataOutput");
	lookupAndLink(c,"writeObject","flash.utils:IDataOutput");
	lookupAndLink(c,"writeShort","flash.utils:IDataOutput");
	lookupAndLink(c,"writeUnsignedInt","flash.utils:IDataOutput");
	lookupAndLink(c,"writeUTF","flash.utils:IDataOutput");
	lookupAndLink(c,"writeUTFBytes","flash.utils:IDataOutput");
}



ASFUNCTIONBODY_ATOM(lightspark,getQualifiedClassName)
{
	//CHECK: what to do if ns is empty
	ASObject* target=asAtomHandler::toObject(args[0],wrk);
	Class_base* c;
	switch(target->getObjectType())
	{
		case T_NULL:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"null");
			return;
		case T_UNDEFINED:
			// Testing shows that this really returns "void"!
			ret = asAtomHandler::fromString(wrk->getSystemState(),"void");
			return;
		case T_CLASS:
			c=static_cast<Class_base*>(target);
			break;
		case T_NUMBER:
			if (target->as<Number>()->isfloat)
				c=target->getClass();
			else if (target->toInt64() > INT32_MIN && target->toInt64()< INT32_MAX)
				c=Class<Integer>::getRef(target->getSystemState()).getPtr();
			else if (target->toInt64() > 0 && target->toInt64()< UINT32_MAX)
				c=Class<UInteger>::getRef(target->getSystemState()).getPtr();
			else 
				c=target->getClass();
			break;
		case T_TEMPLATE:
			ret = asAtomHandler::fromString(wrk->getSystemState(), target->as<Template_base>()->getTemplateName().getQualifiedName(wrk->getSystemState()));
			return;
		default:
			assert_and_throw(target->getClass());
			c=target->getClass();
			break;
	}

	ret = asAtomHandler::fromStringID(c->getQualifiedClassNameID());
}

ASFUNCTIONBODY_ATOM(lightspark,getQualifiedSuperclassName)
{
	//CHECK: what to do is ns is empty
	ASObject* target=asAtomHandler::toObject(args[0],wrk);
	Class_base* c;
	if(target->getObjectType()!=T_CLASS)
	{
		assert_and_throw(target->getClass());
		c=target->getClass()->super.getPtr();
	}
	else
		c=static_cast<Class_base*>(target)->super.getPtr();

	if (!c)
		asAtomHandler::setNull(ret);
	else
		ret = asAtomHandler::fromStringID(c->getQualifiedClassNameID());
}

ASFUNCTIONBODY_ATOM(lightspark,getDefinitionByName)
{
	assert_and_throw(args && argslen==1);
	const tiny_string& tmp=asAtomHandler::toString(args[0],wrk);
	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;

	tiny_string nsName;
	tiny_string tmpName;
	stringToQName(tmp,tmpName,nsName);
	name.name_s_id=wrk->getSystemState()->getUniqueStringId(tmpName);
	if (nsName != "")
	{
		name.ns.push_back(nsNameAndKind(wrk->getSystemState(),nsName,NAMESPACE));
		name.hasEmptyNS=false;
	}

	LOG(LOG_CALLS,"Looking for definition of " << name);
	ASObject* target;
	ret = asAtomHandler::invalidAtom;
	if (nsName.empty() || nsName.startsWith("flash."))
		wrk->getSystemState()->systemDomain->getVariableAndTargetByMultinameIncludeTemplatedClasses(ret,name,target,wrk);
	if(asAtomHandler::isInvalid(ret))
		ABCVm::getCurrentApplicationDomain(wrk->currentCallContext)->getVariableAndTargetByMultinameIncludeTemplatedClasses(ret,name,target,wrk);

	if(asAtomHandler::isInvalid(ret))
	{
		ret = asAtomHandler::undefinedAtom;
		createError<ReferenceError>(wrk,kClassNotFoundError, tmp);
		return;
	}

	assert_and_throw(asAtomHandler::isClass(ret));
	assert(!asAtomHandler::getObject(ret)->is<Class_inherit>() || asAtomHandler::getObject(ret)->getInstanceWorker() == wrk);

	LOG(LOG_CALLS,"Getting definition for " << name<<" "<<asAtomHandler::toDebugString(ret)<<" "<<asAtomHandler::getObject(ret)->getInstanceWorker());
	ASATOM_INCREF(ret);
}

ASFUNCTIONBODY_ATOM(lightspark,describeType)
{
	assert_and_throw(argslen>=1);
	ASObject* o = asAtomHandler::toObject(args[0],wrk);
	if (o->is<Class_inherit>())
		o->as<Class_inherit>()->checkScriptInit();
	ret = asAtomHandler::fromObject(o->describeType(wrk));
}

ASFUNCTIONBODY_ATOM(lightspark,describeTypeJSON)
{
	_NR<ASObject> o;
	uint32_t flags;
	ARG_CHECK(ARG_UNPACK(o)(flags));
	ASObject* res = Class<ASObject>::getInstanceS(wrk);

	if (o)
	{
		asAtom v;
		Class_base* cls = nullptr;
		if (o->is<Class_base>())
			cls = o->as<Class_base>();
		else
			cls = o->getClass();
			
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("name");
		v = asAtomHandler::fromString(wrk->getSystemState(),cls->getQualifiedClassName(true));
		res->setVariableByMultiname(m,v,ASObject::CONST_ALLOWED,nullptr,wrk);
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("isDynamic");
		v = asAtomHandler::fromBool(!cls->isSealed);
		res->setVariableByMultiname(m,v,ASObject::CONST_ALLOWED,nullptr,wrk);
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("isFinal");
		v = asAtomHandler::fromBool(!cls->isFinal);
		res->setVariableByMultiname(m,v,ASObject::CONST_ALLOWED,nullptr,wrk);
	
		ASObject* traits = Class<ASObject>::getInstanceS(wrk);
	
		bool INCLUDE_BASES = flags & 0x0002;
		bool INCLUDE_INTERFACES = flags & 0x0004;
		bool INCLUDE_VARIABLES = flags & 0x0008;
		bool INCLUDE_ACCESSORS = flags & 0x0010;
		bool INCLUDE_METHODS = flags & 0x0020;
		bool INCLUDE_METADATA = flags & 0x0040;
		bool INCLUDE_CONSTRUCTOR = flags & 0x0080;
		bool INCLUDE_TRAITS = flags & 0x0100;

		if (INCLUDE_TRAITS)
		{
			if (INCLUDE_METADATA)
			{
				LOG(LOG_NOT_IMPLEMENTED,"describeTypeJSON flag INCLUDE_METADATA");
			}
			if (INCLUDE_BASES)
			{
				Array* bases = Class<Array>::getInstanceS(wrk);
				Class_base* baseclass = cls;
				while (baseclass)
				{
					bases->push(asAtomHandler::fromString(wrk->getSystemState(),baseclass->getQualifiedClassName(true)));
					baseclass = baseclass->super.getPtr();
				}
				m.name_s_id=wrk->getSystemState()->getUniqueStringId("bases");
				v = asAtomHandler::fromObject(bases);
				traits->setVariableByMultiname(m,v,ASObject::CONST_ALLOWED,nullptr,wrk);
			}
			if (INCLUDE_INTERFACES)
			{
				LOG(LOG_NOT_IMPLEMENTED,"describeTypeJSON flag INCLUDE_INTERFACES");
			}
			if (INCLUDE_CONSTRUCTOR)
			{
				LOG(LOG_NOT_IMPLEMENTED,"describeTypeJSON flag INCLUDE_CONSTRUCTOR");
			}
			if (INCLUDE_ACCESSORS || INCLUDE_METHODS || INCLUDE_VARIABLES)
			{
				LOG(LOG_NOT_IMPLEMENTED,"describeTypeJSON flag INCLUDE_ACCESSORS || INCLUDE_METHODS || INCLUDE_VARIABLES");
			}
		}
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("traits");
		v = asAtomHandler::fromObjectNoPrimitive(traits);
		res->setVariableByMultiname(m,v,ASObject::CONST_ALLOWED,nullptr,wrk);
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"avmplus.describeTypeJSON with flags:"<<hex<<flags);
	ret = asAtomHandler::fromObject(res);
}

ASFUNCTIONBODY_ATOM(lightspark,getTimer)
{
	uint64_t res=wrk->getSystemState()->getCurrentTime_ms() - wrk->getSystemState()->startTime;
	asAtomHandler::setInt(ret,wrk,(int32_t)res);
}


ASFUNCTIONBODY_ATOM(lightspark,setInterval)
{
	assert_and_throw(argslen >= 2);

	uint32_t paramstart = 2;
	asAtom func = args[0];
	uint32_t delayarg = 1;
	asAtom o = asAtomHandler::nullAtom;
	if (!asAtomHandler::isFunction(args[0])) // AVM1 also allows setInterval with arguments object,functionname,interval,params...
	{
		assert_and_throw(argslen >= 3);
		paramstart = 3;
		delayarg = 2;
		ASObject* oref = asAtomHandler::toObject(args[0],wrk);
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id= asAtomHandler::toStringId(args[1],wrk);
		func = asAtomHandler::invalidAtom;
		oref->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::isInvalid(func))
		{
			ASObject* pr = oref->getprop_prototype();
			while (pr)
			{
				pr->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
				if (asAtomHandler::isValid(func))
					break;
				pr = pr->getprop_prototype();
			}
		}
		if (asAtomHandler::isInvalid(func) && oref->is<DisplayObject>())
		{
			uint32_t nameidlower = wrk->getSystemState()->getUniqueStringId(asAtomHandler::toString(args[1],wrk).lowercase());
			AVM1Function* f = oref->as<DisplayObject>()->AVM1GetFunction(nameidlower);
			if (f)
				func = asAtomHandler::fromObjectNoPrimitive(f);
		}
		assert_and_throw (asAtomHandler::isFunction(func));
	}
	if (!asAtomHandler::is<AVM1Function>(func))
		o = asAtomHandler::getClosureAtom(func);
	else
	{
		// it seems that adobe uses the ObjectReference as "this" for the callback
		if (!asAtomHandler::isFunction(args[0]))
			o = args[0];
	}
	//Build arguments array
	asAtom* callbackArgs = g_newa(asAtom,argslen-paramstart);
	uint32_t i;
	for(i=0; i<argslen-paramstart; i++)
	{
		callbackArgs[i] = args[i+paramstart];
		//incRef all passed arguments
		ASATOM_INCREF(args[i+paramstart]);
	}


	//incRef the function
	ASATOM_INCREF(func);
	//Add interval through manager
	uint32_t id = wrk->getSystemState()->intervalManager->setInterval(func, callbackArgs, argslen-paramstart,
			o, asAtomHandler::toInt(args[delayarg]));
	asAtomHandler::setInt(ret,wrk,(int32_t)id);
}

ASFUNCTIONBODY_ATOM(lightspark,setTimeout)
{
	assert_and_throw(argslen >= 2 && asAtomHandler::isFunction(args[0]));

	//Build arguments array
	asAtom* callbackArgs = g_newa(asAtom,argslen-2);
	uint32_t i;
	for(i=0; i<argslen-2; i++)
	{
		callbackArgs[i] = args[i+2];
		//incRef all passed arguments
		ASATOM_INCREF(args[i+2]);
	}

	asAtom o = asAtomHandler::nullAtom;
	if (asAtomHandler::as<IFunction>(args[0])->closure_this)
		o = asAtomHandler::fromObject(asAtomHandler::as<IFunction>(args[0])->closure_this);

	//incRef the function
	ASATOM_INCREF(args[0]);
	//Add timeout through manager
	uint32_t id = wrk->getSystemState()->intervalManager->setTimeout(args[0], callbackArgs, argslen-2,
			o, asAtomHandler::toInt(args[1]));
	asAtomHandler::setInt(ret,wrk,(int32_t)id);
}

ASFUNCTIONBODY_ATOM(lightspark,clearInterval)
{
	assert_and_throw(argslen == 1);
	wrk->getSystemState()->intervalManager->clearInterval(asAtomHandler::toInt(args[0]), IntervalRunner::INTERVAL, true);
}

ASFUNCTIONBODY_ATOM(lightspark,clearTimeout)
{
	assert_and_throw(argslen == 1);
	wrk->getSystemState()->intervalManager->clearInterval(asAtomHandler::toInt(args[0]), IntervalRunner::TIMEOUT, true);
}

ASFUNCTIONBODY_ATOM(lightspark,escapeMultiByte)
{
	tiny_string str;
	ARG_CHECK(ARG_UNPACK (str, "undefined"));
	ret = asAtomHandler::fromObject(abstract_s(wrk,URLInfo::encode(str, URLInfo::ENCODE_ESCAPE)));
}
ASFUNCTIONBODY_ATOM(lightspark,unescapeMultiByte)
{
	tiny_string str;
	ARG_CHECK(ARG_UNPACK (str, "undefined"));
	ret = asAtomHandler::fromObject(abstract_s(wrk,URLInfo::decode(str, URLInfo::ENCODE_ESCAPE)));
}
