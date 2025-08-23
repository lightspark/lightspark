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
#include "scripting/flash/utils/Proxy.h"
#include "asobject.h"
#include "scripting/class.h"
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/flash/errors/flasherrors.h"
#include <sstream>
#include <zlib.h>

using namespace std;
using namespace lightspark;



void Proxy::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject,CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("isAttribute","",c->getSystemState()->getBuiltinFunction(_isAttribute,0,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
}

ASFUNCTIONBODY_ATOM(Proxy,_isAttribute)
{
	_NR<ASObject> name;
	ARG_CHECK(ARG_UNPACK(name));
	multiname mname(nullptr);
	name->applyProxyProperty(mname);
	asAtomHandler::setBool(ret,mname.isAttribute);
}

multiname *Proxy::setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
{
	//If a variable named like this already exist, use that
	if(ASObject::hasPropertyByMultiname(name, true, false,wrk) || !implEnable)
		return ASObject::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);

	//Check if there is a custom setter defined, skipping implementation to avoid recursive calls
	multiname setPropertyName(nullptr);
	setPropertyName.name_type=multiname::NAME_STRING;
	setPropertyName.name_s_id=getSystemState()->getUniqueStringId("setProperty");
	setPropertyName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom proxySetter=asAtomHandler::invalidAtom;
	GET_VARIABLE_RESULT res = getVariableByMultiname(proxySetter,setPropertyName,GET_VARIABLE_OPTION::SKIP_IMPL,wrk);

	if(asAtomHandler::isInvalid(proxySetter))
		return ASObject::setVariableByMultiname(name,o,allowConst,alreadyset,wrk);

	assert_and_throw(asAtomHandler::isFunction(proxySetter));


	ASObject* namearg = abstract_s(wrk,name.normalizedName(wrk));
	namearg->setProxyProperty(name);
	asAtom args[2];
	args[0]=asAtomHandler::fromObject(namearg);
	args[1]=o;
	ASATOM_INCREF(o);
	//We now suppress special handling
	implEnable=false;
	LOG_CALL("Proxy::setProperty");
	asAtom v = asAtomHandler::fromObject(this);
	ASATOM_INCREF(v);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::callFunction(proxySetter,wrk,ret,v,args,2,true);
	assert_and_throw(asAtomHandler::isUndefined(ret));
	implEnable=true;
	if (res & GET_VARIABLE_RESULT::GETVAR_ISINCREFFED)
		ASATOM_DECREF(proxySetter);
	return nullptr;
}

GET_VARIABLE_RESULT Proxy::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
{
	//It seems that various kind of implementation works only with the empty namespace
	asAtom o=asAtomHandler::invalidAtom;
	GET_VARIABLE_RESULT res = GET_VARIABLE_RESULT::GETVAR_NORMAL;
	LOG_CALL("Proxy::getVar "<< name << " " << this->toDebugString()<<" "<<ASObject::hasPropertyByMultiname(name, true, true,wrk));
	if(ASObject::hasPropertyByMultiname(name, true, true,wrk) || !implEnable || (opt & GET_VARIABLE_OPTION::SKIP_IMPL)!=0)
		res = getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	if (asAtomHandler::isValid(ret) || !implEnable || (opt & GET_VARIABLE_OPTION::SKIP_IMPL)!=0)
		return res;
		
	//Check if there is a custom getter defined, skipping implementation to avoid recursive calls
	multiname getPropertyName(NULL);
	getPropertyName.name_type=multiname::NAME_STRING;
	getPropertyName.name_s_id=getSystemState()->getUniqueStringId("getProperty");
	getPropertyName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	res = getVariableByMultiname(o,getPropertyName,GET_VARIABLE_OPTION::SKIP_IMPL,wrk);

	if(asAtomHandler::isInvalid(o))
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt,wrk);
	}
	assert_and_throw(asAtomHandler::isFunction(o));

	ASObject* namearg = abstract_s(getInstanceWorker(),name.normalizedName(wrk));
	namearg->setProxyProperty(name);
	asAtom arg = asAtomHandler::fromObject(namearg);
	//We now suppress special handling
	implEnable=false;
	LOG_CALL("Proxy::getProperty "<< name.normalizedNameUnresolved(getSystemState()) << " " << this->toDebugString() <<" "<<asAtomHandler::toDebugString(o) <<" "<<asAtomHandler::toDebugString(o));
	asAtom v = asAtomHandler::fromObject(this);
	ASATOM_INCREF(v);
	asAtomHandler::callFunction(o,getInstanceWorker(),ret,v,&arg,1,true);
	if (res & GET_VARIABLE_RESULT::GETVAR_ISINCREFFED)
		ASATOM_DECREF(o);
	implEnable=true;
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}

asAtomWithNumber Proxy::getAtomWithNumberByMultiname(const multiname& name, ASWorker* wrk, GET_VARIABLE_OPTION opt)
{
	//It seems that various kind of implementation works only with the empty namespace
	asAtom o=asAtomHandler::invalidAtom;
	GET_VARIABLE_RESULT res = GET_VARIABLE_RESULT::GETVAR_NORMAL;
	LOG_CALL("Proxy::getVar "<< name << " " << this->toDebugString()<<" "<<ASObject::hasPropertyByMultiname(name, true, true,wrk));
	if(ASObject::hasPropertyByMultiname(name, true, true,wrk))
		return ASObject::getAtomWithNumberByMultiname(name,wrk,opt);
	if (!implEnable)
		return asAtomWithNumber();

	//Check if there is a custom getter defined, skipping implementation to avoid recursive calls
	multiname getPropertyName(nullptr);
	getPropertyName.name_type=multiname::NAME_STRING;
	getPropertyName.name_s_id=getSystemState()->getUniqueStringId("getProperty");
	getPropertyName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	res = getVariableByMultiname(o,getPropertyName,GET_VARIABLE_OPTION::SKIP_IMPL,wrk);

	if(asAtomHandler::isInvalid(o))
		return ASObject::getAtomWithNumberByMultiname(name,wrk,opt);
	assert_and_throw(asAtomHandler::isFunction(o));

	ASObject* namearg = abstract_s(getInstanceWorker(),name.normalizedName(wrk));
	namearg->setProxyProperty(name);
	asAtom arg = asAtomHandler::fromObject(namearg);
	//We now suppress special handling
	implEnable=false;
	LOG_CALL("Proxy::getProperty "<< name.normalizedNameUnresolved(getSystemState()) << " " << this->toDebugString() <<" "<<asAtomHandler::toDebugString(o) <<" "<<asAtomHandler::toDebugString(o));
	asAtom v = asAtomHandler::fromObject(this);
	ASATOM_INCREF(v);
	asAtomWithNumber ret;
	// TODO avoid creation of Number object for return value
	asAtomHandler::callFunction(o,getInstanceWorker(),ret.value,v,&arg,1,true);
	if (res & GET_VARIABLE_RESULT::GETVAR_ISINCREFFED)
		ASATOM_DECREF(o);
	implEnable=true;
	return res;

}

bool Proxy::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk)
{
	if (name.normalizedName(getInstanceWorker()) == "isAttribute")
		return true;
	//If a variable named like this already exist, use that
	bool asobject_has_property=ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
	if(asobject_has_property || !implEnable)
		return asobject_has_property;
	if (!isConstructed())
		return false;
	//Check if there is a custom hasProperty defined, skipping implementation to avoid recursive calls
	multiname hasPropertyName(NULL);
	hasPropertyName.name_type=multiname::NAME_STRING;
	hasPropertyName.name_s_id=getSystemState()->getUniqueStringId("hasProperty");
	hasPropertyName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom proxyHasProperty=asAtomHandler::invalidAtom;
	GET_VARIABLE_RESULT res = getVariableByMultiname(proxyHasProperty, hasPropertyName,GET_VARIABLE_OPTION::SKIP_IMPL,wrk);

	if(asAtomHandler::isInvalid(proxyHasProperty))
	{
		return false;
	}

	assert_and_throw(asAtomHandler::isFunction(proxyHasProperty));

	ASObject* namearg = abstract_s(getInstanceWorker(),name.normalizedName(getInstanceWorker()));
	namearg->setProxyProperty(name);
	asAtom arg = asAtomHandler::fromObject(namearg);
	//We now suppress special handling
	implEnable=false;
	LOG_CALL("Proxy::hasProperty");
	asAtom v = asAtomHandler::fromObject(this);
	ASATOM_INCREF(v);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::callFunction(proxyHasProperty,getInstanceWorker(), ret,v,&arg,1,true);
	implEnable=true;
	if (res & GET_VARIABLE_RESULT::GETVAR_ISINCREFFED)
		ASATOM_DECREF(proxyHasProperty);
	return asAtomHandler::Boolean_concrete(ret);
}
bool Proxy::deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
{
	//If a variable named like this already exist, use that
	if(ASObject::hasPropertyByMultiname(name, true, false,wrk) || !implEnable)
	{
		return ASObject::deleteVariableByMultiname(name,wrk);
	}

	//Check if there is a custom deleter defined, skipping implementation to avoid recursive calls
	multiname deletePropertyName(NULL);
	deletePropertyName.name_type=multiname::NAME_STRING;
	deletePropertyName.name_s_id=getSystemState()->getUniqueStringId("deleteProperty");
	deletePropertyName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom proxyDeleter=asAtomHandler::invalidAtom;
	GET_VARIABLE_RESULT res = getVariableByMultiname(proxyDeleter, deletePropertyName,GET_VARIABLE_OPTION::SKIP_IMPL,wrk);

	if(asAtomHandler::isInvalid(proxyDeleter))
	{
		return ASObject::deleteVariableByMultiname(name,wrk);
	}

	assert_and_throw(asAtomHandler::isFunction(proxyDeleter));

	ASObject* namearg = abstract_s(getInstanceWorker(),name.normalizedName(getInstanceWorker()));
	namearg->setProxyProperty(name);
	asAtom arg = asAtomHandler::fromObject(namearg);
	//We now suppress special handling
	implEnable=false;
	LOG_CALL("Proxy::deleteProperty");
	asAtom v = asAtomHandler::fromObject(this);
	ASATOM_INCREF(v);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::callFunction(proxyDeleter,getInstanceWorker(),ret,v,&arg,1,true);
	if (res & GET_VARIABLE_RESULT::GETVAR_ISINCREFFED)
		ASATOM_DECREF(proxyDeleter);
	implEnable=true;
	return asAtomHandler::Boolean_concrete(ret);
}

uint32_t Proxy::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	LOG_CALL("Proxy::nextNameIndex");
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameIndexName(nullptr);
	nextNameIndexName.name_type=multiname::NAME_STRING;
	nextNameIndexName.name_s_id=getSystemState()->getUniqueStringId("nextNameIndex");
	nextNameIndexName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom o=asAtomHandler::invalidAtom;
	GET_VARIABLE_RESULT res = getVariableByMultiname(o,nextNameIndexName,GET_VARIABLE_OPTION::SKIP_IMPL,getInstanceWorker());
	assert_and_throw(asAtomHandler::isFunction(o));
	asAtom arg=asAtomHandler::fromUInt(cur_index);
	asAtom v = asAtomHandler::fromObject(this);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::callFunction(o,getInstanceWorker(),ret,v,&arg,1,false);
	uint32_t newIndex=asAtomHandler::toInt(ret);
	ASATOM_DECREF(ret);
	if (res & GET_VARIABLE_RESULT::GETVAR_ISINCREFFED)
		ASATOM_DECREF(o);
	return newIndex;
}

void Proxy::nextName(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	LOG_CALL( "Proxy::nextName");
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameName(nullptr);
	nextNameName.name_type=multiname::NAME_STRING;
	nextNameName.name_s_id=getSystemState()->getUniqueStringId("nextName");
	nextNameName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom o=asAtomHandler::invalidAtom;
	GET_VARIABLE_RESULT res = getVariableByMultiname(o,nextNameName,GET_VARIABLE_OPTION::SKIP_IMPL,getInstanceWorker());
	assert_and_throw(asAtomHandler::isFunction(o));
	asAtom arg=asAtomHandler::fromUInt(index);
	asAtom v = asAtomHandler::fromObject(this);
	asAtomHandler::callFunction(o,getInstanceWorker(),ret,v,&arg,1,false);
	if (res & GET_VARIABLE_RESULT::GETVAR_ISINCREFFED)
		ASATOM_DECREF(o);
}

void Proxy::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	LOG_CALL( "Proxy::nextValue");
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextValueName(NULL);
	nextValueName.name_type=multiname::NAME_STRING;
	nextValueName.name_s_id=getSystemState()->getUniqueStringId("nextValue");
	nextValueName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom o=asAtomHandler::invalidAtom;
	GET_VARIABLE_RESULT res = getVariableByMultiname(o,nextValueName,GET_VARIABLE_OPTION::SKIP_IMPL,getInstanceWorker());
	assert_and_throw(asAtomHandler::isFunction(o));
	asAtom arg=asAtomHandler::fromUInt(index);
	asAtom v = asAtomHandler::fromObject(this);
	asAtomHandler::callFunction(o,getInstanceWorker(),ret,v,&arg,1,false);
	if (res & GET_VARIABLE_RESULT::GETVAR_ISINCREFFED)
		ASATOM_DECREF(o);
}
bool Proxy::isConstructed() const
{
	return ASObject::isConstructed() && constructorCallComplete;
}
