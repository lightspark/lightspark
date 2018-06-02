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
#include <glib.h>

using namespace std;
using namespace lightspark;



void Proxy::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject,CLASS_DYNAMIC_NOT_FINAL);
	c->setDeclaredMethodByQName("isAttribute","",Class<IFunction>::getFunction(c->getSystemState(),_isAttribute),NORMAL_METHOD,true);
}

void Proxy::buildTraits(ASObject* o)
{
}
ASFUNCTIONBODY_ATOM(Proxy,_isAttribute)
{
	_NR<ASObject> name;
	ARG_UNPACK_ATOM(name);
	multiname mname(NULL);
	name->applyProxyProperty(mname);
	ret.setBool(mname.isAttribute);
}

void Proxy::setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst)
{
	//If a variable named like this already exist, use that
	if(ASObject::hasPropertyByMultiname(name, true, false) || !implEnable)
	{
		ASObject::setVariableByMultiname(name,o,allowConst);
		return;
	}

	//Check if there is a custom setter defined, skipping implementation to avoid recursive calls
	multiname setPropertyName(NULL);
	setPropertyName.name_type=multiname::NAME_STRING;
	setPropertyName.name_s_id=getSystemState()->getUniqueStringId("setProperty");
	setPropertyName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom proxySetter;
	getVariableByMultiname(proxySetter,setPropertyName,ASObject::SKIP_IMPL);

	if(proxySetter.type == T_INVALID)
	{
		ASObject::setVariableByMultiname(name,o,allowConst);
		return;
	}

	assert_and_throw(proxySetter.type==T_FUNCTION);


	ASObject* namearg = abstract_s(getSystemState(),name.normalizedName(getSystemState()));
	namearg->setProxyProperty(name);
	asAtom args[2];
	args[0]=asAtom::fromObject(namearg);
	args[1]=o;
	ASATOM_INCREF(o);
	//We now suppress special handling
	implEnable=false;
	LOG_CALL(_("Proxy::setProperty"));
	asAtom v = asAtom::fromObject(this);
	ASATOM_INCREF(v);
	asAtom ret;
	proxySetter.callFunction(ret,v,args,2,true);
	assert_and_throw(ret.type == T_UNDEFINED);
	implEnable=true;
}

GET_VARIABLE_RESULT Proxy::getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt)
{
	//It seems that various kind of implementation works only with the empty namespace
	assert_and_throw(name.ns.size()>0);
	asAtom o;
	GET_VARIABLE_RESULT res = GET_VARIABLE_RESULT::GETVAR_NORMAL;
	LOG_CALL("Proxy::getVar "<< name << " " << this->toDebugString()<<" "<<ASObject::hasPropertyByMultiname(name, true, true));
	if(ASObject::hasPropertyByMultiname(name, true, true) || !implEnable || (opt & ASObject::SKIP_IMPL)!=0)
		res = getVariableByMultinameIntern(ret,name,this->getClass(),opt);
	if (ret.type != T_INVALID || !implEnable || (opt & ASObject::SKIP_IMPL)!=0)
		return res;
		
	//Check if there is a custom getter defined, skipping implementation to avoid recursive calls
	multiname getPropertyName(NULL);
	getPropertyName.name_type=multiname::NAME_STRING;
	getPropertyName.name_s_id=getSystemState()->getUniqueStringId("getProperty");
	getPropertyName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	getVariableByMultiname(o,getPropertyName,ASObject::SKIP_IMPL);

	if(o.type == T_INVALID)
	{
		return getVariableByMultinameIntern(ret,name,this->getClass(),opt);
	}
	assert_and_throw(o.type==T_FUNCTION);

	ASObject* namearg = abstract_s(getSystemState(),name.normalizedName(getSystemState()));
	namearg->setProxyProperty(name);
	asAtom arg = asAtom::fromObject(namearg);
	//We now suppress special handling
	implEnable=false;
	LOG_CALL("Proxy::getProperty "<< name.normalizedNameUnresolved(getSystemState()) << " " << this->toDebugString());
	asAtom v = asAtom::fromObject(this);
	ASATOM_INCREF(v);
	o.callFunction(ret,v,&arg,1,true);
	implEnable=true;
	return GET_VARIABLE_RESULT::GETVAR_NORMAL;
}

bool Proxy::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if (name.normalizedName(getSystemState()) == "isAttribute")
		return true;
	//If a variable named like this already exist, use that
	bool asobject_has_property=ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
	if(asobject_has_property || !implEnable)
		return asobject_has_property;
	if (!isConstructed())
		return false;
	//Check if there is a custom hasProperty defined, skipping implementation to avoid recursive calls
	multiname hasPropertyName(NULL);
	hasPropertyName.name_type=multiname::NAME_STRING;
	hasPropertyName.name_s_id=getSystemState()->getUniqueStringId("hasProperty");
	hasPropertyName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom proxyHasProperty;
	getVariableByMultiname(proxyHasProperty, hasPropertyName,ASObject::SKIP_IMPL);

	if(proxyHasProperty.type == T_INVALID)
	{
		return false;
	}

	assert_and_throw(proxyHasProperty.type==T_FUNCTION);

	ASObject* namearg = abstract_s(getSystemState(),name.normalizedName(getSystemState()));
	namearg->setProxyProperty(name);
	asAtom arg = asAtom::fromObject(namearg);
	//We now suppress special handling
	implEnable=false;
	LOG_CALL(_("Proxy::hasProperty"));
	asAtom v = asAtom::fromObject(this);
	ASATOM_INCREF(v);
	asAtom ret;
	proxyHasProperty.callFunction(ret,v,&arg,1,true);
	implEnable=true;
	return ret.Boolean_concrete();
}
bool Proxy::deleteVariableByMultiname(const multiname& name)
{
	//If a variable named like this already exist, use that
	if(ASObject::hasPropertyByMultiname(name, true, false) || !implEnable)
	{
		return ASObject::deleteVariableByMultiname(name);
	}

	//Check if there is a custom deleter defined, skipping implementation to avoid recursive calls
	multiname deletePropertyName(NULL);
	deletePropertyName.name_type=multiname::NAME_STRING;
	deletePropertyName.name_s_id=getSystemState()->getUniqueStringId("deleteProperty");
	deletePropertyName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom proxyDeleter;
	getVariableByMultiname(proxyDeleter, deletePropertyName,ASObject::SKIP_IMPL);

	if(proxyDeleter.type == T_INVALID)
	{
		return ASObject::deleteVariableByMultiname(name);
	}

	assert_and_throw(proxyDeleter.type==T_FUNCTION);

	ASObject* namearg = abstract_s(getSystemState(),name.normalizedName(getSystemState()));
	namearg->setProxyProperty(name);
	asAtom arg = asAtom::fromObject(namearg);
	//We now suppress special handling
	implEnable=false;
	LOG_CALL(_("Proxy::deleteProperty"));
	asAtom v = asAtom::fromObject(this);
	ASATOM_INCREF(v);
	asAtom ret;
	proxyDeleter.callFunction(ret,v,&arg,1,true);
	implEnable=true;
	return ret.Boolean_concrete();
}

uint32_t Proxy::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	LOG_CALL("Proxy::nextNameIndex");
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameIndexName(NULL);
	nextNameIndexName.name_type=multiname::NAME_STRING;
	nextNameIndexName.name_s_id=getSystemState()->getUniqueStringId("nextNameIndex");
	nextNameIndexName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom o;
	getVariableByMultiname(o,nextNameIndexName,ASObject::SKIP_IMPL);
	assert_and_throw(o.type==T_FUNCTION);
	asAtom arg=asAtom(cur_index);
	asAtom v = asAtom::fromObject(this);
	asAtom ret;
	o.callFunction(ret,v,&arg,1,false);
	uint32_t newIndex=ret.toInt();
	ASATOM_DECREF(ret);
	return newIndex;
}

void Proxy::nextName(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	LOG_CALL( _("Proxy::nextName"));
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameName(NULL);
	nextNameName.name_type=multiname::NAME_STRING;
	nextNameName.name_s_id=getSystemState()->getUniqueStringId("nextName");
	nextNameName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom o;
	getVariableByMultiname(o,nextNameName,ASObject::SKIP_IMPL);
	assert_and_throw(o.type==T_FUNCTION);
	asAtom arg=asAtom(index);
	asAtom v = asAtom::fromObject(this);
	o.callFunction(ret,v,&arg,1,false);
}

void Proxy::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);
	LOG_CALL( _("Proxy::nextValue"));
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextValueName(NULL);
	nextValueName.name_type=multiname::NAME_STRING;
	nextValueName.name_s_id=getSystemState()->getUniqueStringId("nextValue");
	nextValueName.ns.emplace_back(getSystemState(),flash_proxy,NAMESPACE);
	asAtom o;
	getVariableByMultiname(o,nextValueName,ASObject::SKIP_IMPL);
	assert_and_throw(o.type==T_FUNCTION);
	asAtom arg=asAtom(index);
	asAtom v = asAtom::fromObject(this);
	o.callFunction(ret,v,&arg,1,false);
}
bool Proxy::isConstructed() const
{
	return ASObject::isConstructed() && constructorCallComplete;
}
