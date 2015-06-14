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
}

void Proxy::buildTraits(ASObject* o)
{
}

void Proxy::setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
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
	setPropertyName.name_s_id=getSys()->getUniqueStringId("setProperty");
	setPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> proxySetter=getVariableByMultiname(setPropertyName,ASObject::SKIP_IMPL);

	if(proxySetter.isNull())
	{
		ASObject::setVariableByMultiname(name,o,allowConst);
		return;
	}

	assert_and_throw(proxySetter->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(proxySetter.getPtr());

	ASObject* namearg = Class<ASString>::getInstanceS(name.normalizedName());
	namearg->setProxyProperty(name);
	ASObject* args[2];
	args[0]=namearg;
	args[1]=o;
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,_("Proxy::setProperty"));
	incRef();
	_R<ASObject> ret=_MR( f->call(this,args,2) );
	assert_and_throw(ret->is<Undefined>());
	implEnable=true;
}

_NR<ASObject> Proxy::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	//It seems that various kind of implementation works only with the empty namespace
	assert_and_throw(name.ns.size()>0);
	_NR<ASObject> o;
	LOG(LOG_CALLS,"Proxy::getVar "<< name << " " << this->toDebugString());
	if(ASObject::hasPropertyByMultiname(name, true, true) || !implEnable || (opt & ASObject::SKIP_IMPL)!=0)
		o = ASObject::getVariableByMultiname(name,opt);
	if (!o.isNull() || !implEnable || (opt & ASObject::SKIP_IMPL)!=0)
		return o;

	//Check if there is a custom getter defined, skipping implementation to avoid recursive calls
	multiname getPropertyName(NULL);
	getPropertyName.name_type=multiname::NAME_STRING;
	getPropertyName.name_s_id=getSys()->getUniqueStringId("getProperty");
	getPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	o=getVariableByMultiname(getPropertyName,ASObject::SKIP_IMPL);

	if(o.isNull())
		return ASObject::getVariableByMultiname(name,opt);

	assert_and_throw(o->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(o.getPtr());

	ASObject* namearg = Class<ASString>::getInstanceS(name.normalizedName());
	namearg->setProxyProperty(name);
	ASObject* arg = namearg;
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,"Proxy::getProperty "<< name.normalizedNameUnresolved() << " " << this->toDebugString());
	incRef();
	_NR<ASObject> ret=_MNR(f->call(this,&arg,1));
	implEnable=true;
	return ret;
}

bool Proxy::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if (name.normalizedName() == "isAttribute")
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
	hasPropertyName.name_s_id=getSys()->getUniqueStringId("hasProperty");
	hasPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> proxyHasProperty=getVariableByMultiname(hasPropertyName,ASObject::SKIP_IMPL);

	if(proxyHasProperty.isNull())
	{
		return false;
	}

	assert_and_throw(proxyHasProperty->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(proxyHasProperty.getPtr());

	ASObject* namearg = Class<ASString>::getInstanceS(name.normalizedName());
	namearg->setProxyProperty(name);
	ASObject* arg = namearg;
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,_("Proxy::hasProperty"));
	incRef();
	_NR<ASObject> ret=_MNR(f->call(this,&arg,1));
	implEnable=true;
	Boolean* b = static_cast<Boolean*>(ret.getPtr());
	return b->val;
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
	deletePropertyName.name_s_id=getSys()->getUniqueStringId("deleteProperty");
	deletePropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> proxyDeleter=getVariableByMultiname(deletePropertyName,ASObject::SKIP_IMPL);

	if(proxyDeleter.isNull())
	{
		return ASObject::deleteVariableByMultiname(name);
	}

	assert_and_throw(proxyDeleter->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(proxyDeleter.getPtr());

	ASObject* namearg = Class<ASString>::getInstanceS(name.normalizedName());
	namearg->setProxyProperty(name);
	ASObject* arg = namearg;
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,_("Proxy::deleteProperty"));
	incRef();
	_NR<ASObject> ret=_MNR(f->call(this,&arg,1));
	implEnable=true;
	Boolean* b = static_cast<Boolean*>(ret.getPtr());
	return b->val;
}

uint32_t Proxy::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	LOG(LOG_CALLS,"Proxy::nextNameIndex");
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameIndexName(NULL);
	nextNameIndexName.name_type=multiname::NAME_STRING;
	nextNameIndexName.name_s_id=getSys()->getUniqueStringId("nextNameIndex");
	nextNameIndexName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> o=getVariableByMultiname(nextNameIndexName,ASObject::SKIP_IMPL);
	assert_and_throw(!o.isNull() && o->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(o.getPtr());
	ASObject* arg=abstract_i(cur_index);
	this->incRef();
	ASObject* ret=f->call(this,&arg,1);
	uint32_t newIndex=ret->toInt();
	ret->decRef();
	return newIndex;
}

_R<ASObject> Proxy::nextName(uint32_t index)
{
	assert_and_throw(implEnable);
	LOG(LOG_CALLS, _("Proxy::nextName"));
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameName(NULL);
	nextNameName.name_type=multiname::NAME_STRING;
	nextNameName.name_s_id=getSys()->getUniqueStringId("nextName");
	nextNameName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> o=getVariableByMultiname(nextNameName,ASObject::SKIP_IMPL);
	assert_and_throw(!o.isNull() && o->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(o.getPtr());
	ASObject* arg=abstract_i(index);
	incRef();
	return _MR(f->call(this,&arg,1));
}

_R<ASObject> Proxy::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);
	LOG(LOG_CALLS, _("Proxy::nextValue"));
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextValueName(NULL);
	nextValueName.name_type=multiname::NAME_STRING;
	nextValueName.name_s_id=getSys()->getUniqueStringId("nextValue");
	nextValueName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> o=getVariableByMultiname(nextValueName,ASObject::SKIP_IMPL);
	assert_and_throw(!o.isNull() && o->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(o.getPtr());
	ASObject* arg=abstract_i(index);
	incRef();
	return _MR(f->call(this,&arg,1));
}

