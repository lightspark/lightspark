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
#include "scripting/flash/system/ApplicationDomain.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/utils/IntervalManager.h"
#include "scripting/flash/display/DisplayObject.h"
#include <sstream>
#include <zlib.h>
#include <3rdparty/pugixml/src/pugixml.hpp>

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
	assert_and_throw(args && argslen==1);
	//CHECK: what to do if ns is empty
	Class_base* c;
	switch(asAtomHandler::getObjectType(args[0]))
	{
		case T_NULL:
			ret = asAtomHandler::fromString(wrk->getSystemState(),"null");
			return;
		case T_UNDEFINED:
			// Testing shows that this really returns "void"!
			ret = asAtomHandler::fromString(wrk->getSystemState(),"void");
			return;
		case T_CLASS:
			c=asAtomHandler::as<Class_base>(args[0]);
			break;
		case T_NUMBER:
			if (asAtomHandler::isLocalNumber(args[0]) || asAtomHandler::as<Number>(args[0])->isfloat)
				c=Class<Number>::getRef(wrk->getSystemState()).getPtr();
			else if (asAtomHandler::toInt64(args[0]) > INT32_MIN && asAtomHandler::toInt64(args[0])< INT32_MAX)
				c=Class<Integer>::getRef(wrk->getSystemState()).getPtr();
			else if (asAtomHandler::toInt64(args[0]) > 0 && asAtomHandler::toInt64(args[0])< UINT32_MAX)
				c=Class<UInteger>::getRef(wrk->getSystemState()).getPtr();
			else 
				c=asAtomHandler::getClass(args[0],wrk->getSystemState());
			break;
		case T_TEMPLATE:
			ret = asAtomHandler::fromString(wrk->getSystemState(), asAtomHandler::as<Template_base>(args[0])->getTemplateName().getQualifiedName(wrk->getSystemState()));
			return;
		default:
			assert_and_throw(asAtomHandler::getClass(args[0],wrk->getSystemState()));
			c=asAtomHandler::getClass(args[0],wrk->getSystemState());
			break;
	}

	ret = asAtomHandler::fromStringID(c->getQualifiedClassNameID());
}

ASFUNCTIONBODY_ATOM(lightspark,getQualifiedSuperclassName)
{
	assert_and_throw(args && argslen==1);
	//CHECK: what to do is ns is empty
	Class_base* c;
	if(asAtomHandler::getObjectType(args[0])!=T_CLASS)
	{
		c = asAtomHandler::getClass(args[0],wrk->getSystemState());
		assert_and_throw(c);
		c=c->super.getPtr();
	}
	else
		c=asAtomHandler::as<Class_base>(args[0])->super.getPtr();

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
	name.ns.push_back(nsNameAndKind(wrk->getSystemState(),nsName,NAMESPACE));
	name.hasEmptyNS=nsName.empty();

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
		createError<ReferenceError>(wrk,kUndefinedVarError, tmp);
		return;
	}
	
	assert(!asAtomHandler::getObject(ret)->is<Class_inherit>() || asAtomHandler::getObject(ret)->getInstanceWorker() == wrk);

	LOG(LOG_CALLS,"Getting definition for " << name<<" "<<asAtomHandler::toDebugString(ret)<<" "<<asAtomHandler::getObject(ret)->getInstanceWorker());
	ASATOM_INCREF(ret);
}

ASFUNCTIONBODY_ATOM(lightspark,describeType)
{
	assert_and_throw(argslen>=1);
	ASObject* o = asAtomHandler::getObject(args[0]);
	if (o)
	{
		if (o->is<Class_inherit>())
			o->as<Class_inherit>()->checkScriptInit();
		ret = asAtomHandler::fromObject(o->describeType(wrk));
		return;
	}

	switch (asAtomHandler::getObjectType(args[0]))
	{
		case T_INTEGER:
			o = Class<Integer>::getInstanceSNoArgs(wrk);
			break;
		case T_UINTEGER:
			o = Class<UInteger>::getInstanceSNoArgs(wrk);
			break;
		case T_NUMBER:
			o = Class<Number>::getInstanceSNoArgs(wrk);
			break;
		case T_STRING:
			o = Class<ASString>::getInstanceSNoArgs(wrk);
			break;
		default:
			return;
	}
	ret = asAtomHandler::fromObject(o->describeType(wrk));
	o->decRef();
}
asAtom fillForDescribeTypeJSON(pugi::xml_node& node, ASWorker* wrk,const tiny_string& accessor, bool needsparams)
{
	ASObject* var = new_asobject(wrk);
	multiname mvar(nullptr);
	mvar.name_type=multiname::NAME_STRING;
	mvar.isAttribute = false;
	asAtom varAtom;

	if (!accessor.empty())
	{
		mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("access");
		varAtom =asAtomHandler::fromStringID(wrk->getSystemState()->getUniqueStringId(accessor));
		var->setVariableByMultiname(mvar,varAtom,CONST_ALLOWED,nullptr,wrk);
	}
	mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("name");
	varAtom = asAtomHandler::fromString(wrk->getSystemState(),node.attribute("name").as_string());
	var->setVariableByMultiname(mvar,varAtom,CONST_ALLOWED,nullptr,wrk);
	if (needsparams)
	{
		mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("returnType");
		varAtom = asAtomHandler::fromString(wrk->getSystemState(),node.attribute("returnType").as_string());
		var->setVariableByMultiname(mvar,varAtom,CONST_ALLOWED,nullptr,wrk);
		varAtom = asAtomHandler::fromStringID(BUILTIN_STRINGS::EMPTY);
		Array* params = Class<Array>::getInstanceS(wrk);
		for (auto it = node.children().begin(); it != node.children().end(); it++)
		{
			ASObject* param = new_asobject(wrk);
			mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("type");
			varAtom = asAtomHandler::fromString(wrk->getSystemState(),(*it).attribute("type").as_string());
			param->setVariableByMultiname(mvar,varAtom,CONST_ALLOWED,nullptr,wrk);
			mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("optional");
			varAtom = asAtomHandler::fromBool((*it).attribute("optional").as_bool());
			param->setVariableByMultiname(mvar,varAtom,CONST_ALLOWED,nullptr,wrk);
			params->push(asAtomHandler::fromObjectNoPrimitive(param));
		}
		mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("parameters");
		varAtom = asAtomHandler::fromObjectNoPrimitive(params);
		var->setVariableByMultiname(mvar,varAtom,CONST_ALLOWED,nullptr,wrk);
	}
	else
	{
		mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("type");
		varAtom = asAtomHandler::fromString(wrk->getSystemState(),node.attribute("type").as_string());
		var->setVariableByMultiname(mvar,varAtom,CONST_ALLOWED,nullptr,wrk);
	}
	mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("uri");
	varAtom = asAtomHandler::fromString(wrk->getSystemState(),node.attribute("uri").as_string("null"));
	var->setVariableByMultiname(mvar,varAtom,CONST_ALLOWED,nullptr,wrk);
	tiny_string declaredBy = node.attribute("declaredBy").as_string();
	if (!declaredBy.empty())
	{
		mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("declaredBy");
		varAtom = asAtomHandler::fromString(wrk->getSystemState(),declaredBy);
		var->setVariableByMultiname(mvar,varAtom,CONST_ALLOWED,nullptr,wrk);
	}

	mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("metadata");
	varAtom = asAtomHandler::nullAtom;
	auto metadata = node.children("metadata");
	for (auto itmeta = metadata.begin(); itmeta != metadata.end(); itmeta++)
	{
		LOG(LOG_NOT_IMPLEMENTED,"describeTypeJson with metadata");
	}
	var->setVariableByMultiname(mvar,varAtom,CONST_ALLOWED,nullptr,wrk);
	return asAtomHandler::fromObjectNoPrimitive(var);
}
ASFUNCTIONBODY_ATOM(lightspark,describeTypeJSON)
{
	asAtom arg= asAtomHandler::invalidAtom;
	uint32_t flags;
	ARG_CHECK(ARG_UNPACK(arg)(flags));

	bool HIDE_NSURI_METHODS = flags & 0x0001;
	bool INCLUDE_BASES = flags & 0x0002;
	bool INCLUDE_INTERFACES = flags & 0x0004;
	bool INCLUDE_VARIABLES = flags & 0x0008;
	bool INCLUDE_ACCESSORS = flags & 0x0010;
	bool INCLUDE_METHODS = flags & 0x0020;
	bool INCLUDE_METADATA = flags & 0x0040;
	bool INCLUDE_CONSTRUCTOR = flags & 0x0080;
	bool INCLUDE_TRAITS = flags & 0x0100;
	bool USE_ITRAITS = flags & 0x0200;
	bool HIDE_OBJECT = flags & 0x0400;
	if (HIDE_NSURI_METHODS)
		LOG(LOG_NOT_IMPLEMENTED, "describeTypeJSON: flag HIDE_NSURI_METHODS not implemented");
	if (USE_ITRAITS)
		LOG(LOG_NOT_IMPLEMENTED, "describeTypeJSON: flag USE_ITRAITS not implemented");
	if (HIDE_OBJECT)
		LOG(LOG_NOT_IMPLEMENTED, "describeTypeJSON: flag USE_ITRAITS not implemented");

	ASObject* res = new_asobject(wrk);
	ASObject* o = asAtomHandler::getObject(arg);
	if (o && (!USE_ITRAITS || o->is<Class_base>()))
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
		res->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("isDynamic");
		v = asAtomHandler::fromBool(o->is<Class_base>() || !cls->isSealed);
		res->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("isFinal");
		v = asAtomHandler::fromBool(o->is<Class_base>() || cls->isFinal);
		res->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);
		m.name_s_id=wrk->getSystemState()->getUniqueStringId("isStatic");
		v = asAtomHandler::fromBool(o->is<Class_base>());
		res->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);


		ASObject* traits = nullptr;
		if (INCLUDE_TRAITS)
		{
			traits = new_asobject(wrk);
			pugi::xml_document root;
			cls->describeInstance(root,false,!o->is<Class_base>(),false,true);

			m.name_s_id=wrk->getSystemState()->getUniqueStringId("bases");
			v = asAtomHandler::nullAtom;
			if (INCLUDE_BASES)
			{
				Array* bases = Class<Array>::getInstanceS(wrk);
				auto children = root.children("extendsClass");
				for (auto it = children.begin(); it != children.end(); it++)
					bases->push(asAtomHandler::fromString(wrk->getSystemState(),(*it).attribute("type").as_string()));
				v = asAtomHandler::fromObject(bases);
			}
			traits->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);

			m.name_s_id=wrk->getSystemState()->getUniqueStringId("interfaces");
			v = asAtomHandler::nullAtom;
			if (INCLUDE_INTERFACES)
			{
				Array* interfaces = Class<Array>::getInstanceS(wrk);
				auto ifacevars = root.children("implementsInterface");
				for (auto it = ifacevars.begin(); it != ifacevars.end(); it++)
				{
					asAtom iface = asAtomHandler::fromString(wrk->getSystemState(),(*it).attribute("type").as_string());
					interfaces->push(iface);
				}
				v = asAtomHandler::fromObject(interfaces);
			}
			traits->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);

			m.name_s_id=wrk->getSystemState()->getUniqueStringId("constructor");
			v = asAtomHandler::nullAtom;
			if (INCLUDE_CONSTRUCTOR)
			{
				Array* params = Class<Array>::getInstanceS(wrk);
				multiname mvar(nullptr);
				mvar.name_type=multiname::NAME_STRING;
				mvar.isAttribute = false;
				auto node = root.child("constructor");
				for (auto it = node.children().begin(); it != node.children().end(); it++)
				{
					ASObject* param = new_asobject(wrk);
					mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("type");
					v = asAtomHandler::fromString(wrk->getSystemState(),(*it).attribute("type").as_string());
					param->setVariableByMultiname(mvar,v,CONST_ALLOWED,nullptr,wrk);
					mvar.name_s_id=wrk->getSystemState()->getUniqueStringId("optional");
					v = asAtomHandler::fromBool((*it).attribute("optional").as_bool());
					param->setVariableByMultiname(mvar,v,CONST_ALLOWED,nullptr,wrk);
					params->push(asAtomHandler::fromObjectNoPrimitive(param));
				}
				v = asAtomHandler::fromObjectNoPrimitive(params);
			}
			traits->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);

			m.name_s_id=wrk->getSystemState()->getUniqueStringId("accessors");
			v = asAtomHandler::nullAtom;
			if (INCLUDE_ACCESSORS)
			{
				Array* accessors = Class<Array>::getInstanceS(wrk);

				auto childrenvars = root.children("accessor");
				for (auto it = childrenvars.begin(); it != childrenvars.end(); it++)
					accessors->push(fillForDescribeTypeJSON(*it,wrk,(*it).attribute("access").as_string(),false));
				v = asAtomHandler::fromObjectNoPrimitive(accessors);
			}
			traits->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);

			m.name_s_id=wrk->getSystemState()->getUniqueStringId("methods");
			v = asAtomHandler::nullAtom;
			if (INCLUDE_METHODS)
			{
				Array* methods = Class<Array>::getInstanceS(wrk);

				auto methodvars = root.children("method");
				for (auto it = methodvars.begin(); it != methodvars.end(); it++)
					methods->push(fillForDescribeTypeJSON(*it,wrk,"",true));
				v = asAtomHandler::fromObjectNoPrimitive(methods);
			}
			traits->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);

			m.name_s_id=wrk->getSystemState()->getUniqueStringId("variables");
			v = asAtomHandler::nullAtom;
			if (INCLUDE_VARIABLES)
			{
				Array* variables = Class<Array>::getInstanceS(wrk);

				auto childrenvars = root.children("variable");
				for (auto it = childrenvars.begin(); it != childrenvars.end(); it++)
					variables->push(fillForDescribeTypeJSON(*it,wrk,"readwrite",false));
				auto childrenconsts = root.children("constant");
				for (auto it = childrenconsts.begin(); it != childrenconsts.end(); it++)
					variables->push(fillForDescribeTypeJSON(*it,wrk,"readonly",false));
				v = asAtomHandler::fromObjectNoPrimitive(variables);
			}
			traits->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);
			m.name_s_id=wrk->getSystemState()->getUniqueStringId("metadata");
			v = asAtomHandler::nullAtom;
			if (INCLUDE_METADATA)
			{
				Array* variables = Class<Array>::getInstanceS(wrk);
				auto metadata = root.children("metadata");
				for (auto it = metadata.begin(); it != metadata.end(); it++)
					variables->push(asAtomHandler::fromString(wrk->getSystemState(),(*it).attribute("name").as_string()));
				v = asAtomHandler::fromObjectNoPrimitive(variables);
			}
			traits->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);
		}

		m.name_s_id=wrk->getSystemState()->getUniqueStringId("traits");
		v = traits ? asAtomHandler::fromObjectNoPrimitive(traits) : asAtomHandler::nullAtom;
		res->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);
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
	wrk->getSystemState()->intervalManager->setupTimer(ret,wrk,obj,args,argslen,true);
}

ASFUNCTIONBODY_ATOM(lightspark,setTimeout)
{
	wrk->getSystemState()->intervalManager->setupTimer(ret,wrk,obj,args,argslen,false);
}

ASFUNCTIONBODY_ATOM(lightspark,clearInterval)
{
	assert_and_throw(argslen == 1);
	wrk->getSystemState()->intervalManager->clearInterval(asAtomHandler::toInt(args[0]), false);
}

ASFUNCTIONBODY_ATOM(lightspark,clearTimeout)
{
	assert_and_throw(argslen == 1);
	wrk->getSystemState()->intervalManager->clearInterval(asAtomHandler::toInt(args[0]), true);
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
