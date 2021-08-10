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
#include "asobject.h"
#include "scripting/class.h"
#include <algorithm>
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Date.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/toplevel/Error.h"
#include <3rdparty/pugixml/src/pugixml.hpp>

using namespace lightspark;
using namespace std;

asfreelist::~asfreelist()
{
	for (int i = 0; i < freelistsize; i++)
		delete freelist[i];
	freelistsize = 0;
}

string ASObject::toDebugString()
{
	check();
	string ret;
	if(this->is<Class_base>())
	{
		ret = "[class ";
		ret+=this->as<Class_base>()->class_name.getQualifiedName(getSystemState()).raw_buf();
		ret+="]";
	}
	else if(getClass())
	{
		ret="[object ";
		ret+=getClass()->class_name.getQualifiedName(getSystemState()).raw_buf();
		ret+="]";
	}
	else if(this->is<Undefined>())
		ret = "Undefined";
	else if(this->is<Null>())
		ret = "Null";
	else if(this->is<Template_base>())
	{
		ret = "[templated class]";
	}
	else if (this->is<Event>())
	{
		ret = "[event]";
	}
	else
	{
		assert(false);
	}
#ifndef _NDEBUG
	char buf[300];
	sprintf(buf,"(%p / %d%s) ",this,this->getRefCount(),this->isConstructed()?"":" not constructed");
	ret += buf;
#endif
	return ret;
}

void ASObject::setProxyProperty(const multiname &name)
{
	if (this->proxyMultiName)
		this->proxyMultiName->ns.clear();
	else
		this->proxyMultiName = new (getVm(getSystemState())->vmDataMemory) multiname(getVm(getSystemState())->vmDataMemory);
	this->proxyMultiName->isAttribute = name.isAttribute;
	this->proxyMultiName->ns.reserve(name.ns.size());
	for(unsigned int i=0;i<name.ns.size();i++)
	{
		this->proxyMultiName->ns.push_back(name.ns[i]);
	}
	
}

void ASObject::applyProxyProperty(multiname &name)
{
	if (!this->proxyMultiName)
		return;
	name.isAttribute = this->proxyMultiName->isAttribute;
	name.hasEmptyNS = false;
	name.hasBuiltinNS = false;
	name.hasGlobalNS = false;
	name.ns.clear();
	name.ns.reserve(this->proxyMultiName->ns.size());
	for(unsigned int i=0;i<this->proxyMultiName->ns.size();i++)
	{
		if (this->proxyMultiName->ns[i].hasEmptyName())
			name.hasEmptyNS = true;
		if (this->proxyMultiName->ns[i].hasBuiltinName())
			name.hasBuiltinNS = true;
		if (this->proxyMultiName->ns[i].kind == NAMESPACE)
			name.hasGlobalNS = true;
		name.ns.push_back(this->proxyMultiName->ns[i]);
	}
}

void ASObject::dumpVariables()
{
	LOG(LOG_INFO,"variables:");
	Variables.dumpVariables();
	if (this->is<Class_base>())
	{
		LOG(LOG_INFO,"borrowed variables:");
		this->as<Class_base>()->borrowedVariables.dumpVariables();
	}
}
uint32_t ASObject::toStringId()
{
	if (stringId == UINT32_MAX)
		stringId = getSystemState()->getUniqueStringId(toString());
	return stringId;
}
tiny_string ASObject::toString()
{
	check();
	switch(this->getObjectType())
	{
	case T_UNDEFINED:
		return "undefined";
	case T_NULL:
		return "null";
	case T_BOOLEAN:
		return as<Boolean>()->val ? "true" : "false";
	case T_NUMBER:
		return as<Number>()->toString();
	case T_INTEGER:
		return as<Integer>()->toString();
	case T_UINTEGER:
		return as<UInteger>()->toString();
	case T_STRING:
		return as<ASString>()->getData();
	default:
		{
			//everything else is an Object regarding to the spec
				asAtom ret = asAtomHandler::fromObject(this);
				toPrimitive(ret,STRING_HINT);
				return asAtomHandler::toString(ret,getSystemState());
		}
	}
}

tiny_string ASObject::toLocaleString()
{
	asAtom str=asAtomHandler::invalidAtom;
	executeASMethod(str,"toLocaleString", {""}, NULL, 0);
	if (asAtomHandler::isInvalid(str))
		return "";
	return asAtomHandler::toString(str,getSystemState());
}

TRISTATE ASObject::isLess(ASObject* r)
{
	asAtom o = asAtomHandler::fromObject(r);
	asAtom ret = asAtomHandler::fromObject(this);
	toPrimitive(ret);
	return asAtomHandler::isLess(ret,getSystemState(),o);
}
TRISTATE ASObject::isLessAtom(asAtom& r)
{
	asAtom ret = asAtomHandler::fromObject(this);
	toPrimitive(ret);
	return asAtomHandler::isLess(ret,getSystemState(),r);
}

int variables_map::getNextEnumerable(unsigned int start) const
{
	if(start>=Variables.size())
		return -1;

	const_var_iterator it=Variables.begin();

	unsigned int i=0;
	while (i < start)
	{
		++i;
		++it;
	}

	while(it->second.kind!=DYNAMIC_TRAIT || !it->second.isenumerable)
	{
		++i;
		++it;
		if(it==Variables.end())
			return -1;
	}

	return i;
}

uint32_t ASObject::nextNameIndex(uint32_t cur_index)
{
	// First -1 converts one-base cur_index to zero-based, +1
	// moves to the next position (first position to be
	// considered). Final +1 converts back to one-based.
	return Variables.getNextEnumerable(cur_index-1+1)+1;
}

void ASObject::nextName(asAtom& ret, uint32_t index)
{
	assert_and_throw(implEnable);

	uint32_t n = getNameAt(index-1);
	const tiny_string& name = getSystemState()->getStringFromUniqueId(n);
	// not mentioned in the specs, but Adobe seems to convert string names to Integers, if possible
	if (Array::isIntegerWithoutLeadingZeros(name))
		asAtomHandler::setInt(ret,getSystemState(),Integer::stringToASInteger(name.raw_buf(), 0));
	else
		ret = asAtomHandler::fromStringID(n);
}

void ASObject::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);

	getValueAt(ret,index-1);
}

void ASObject::sinit(Class_base* c)
{
	c->setDeclaredMethodByQName("hasOwnProperty",AS3,Class<IFunction>::getFunction(c->getSystemState(),hasOwnProperty,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPropertyIsEnumerable",AS3,Class<IFunction>::getFunction(c->getSystemState(),setPropertyIsEnumerable),NORMAL_METHOD,true);

	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",Class<IFunction>::getFunction(c->getSystemState(),_toLocaleString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(c->getSystemState(),valueOf,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("hasOwnProperty","",Class<IFunction>::getFunction(c->getSystemState(),hasOwnProperty,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("isPrototypeOf","",Class<IFunction>::getFunction(c->getSystemState(),isPrototypeOf,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("propertyIsEnumerable","",Class<IFunction>::getFunction(c->getSystemState(),propertyIsEnumerable,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("setPropertyIsEnumerable","",Class<IFunction>::getFunction(c->getSystemState(),setPropertyIsEnumerable),DYNAMIC_TRAIT);
}

void ASObject::buildTraits(ASObject* o)
{
}

bool ASObject::isEqual(ASObject* r)
{
	//if we are comparing the same object the answer is true
	if(this==r)
		return true;

	switch(r->getObjectType())
	{
		case T_NULL:
		case T_UNDEFINED:
			if (!this->isInitialized() && !this->is<Class_base>())
				return true;
			return false;
		case T_NUMBER:
		case T_INTEGER:
		case T_UINTEGER:
		case T_STRING:
		{
			asAtom primitive=asAtomHandler::invalidAtom;
			toPrimitive(primitive);
			asAtom o = asAtomHandler::fromObject(r);
			return asAtomHandler::isEqual(primitive,getSystemState(),o);
		}
		case T_BOOLEAN:
		{
			asAtom primitive=asAtomHandler::invalidAtom;
			toPrimitive(primitive);
			return asAtomHandler::toNumber(primitive)==r->toNumber();
		}
		default:
		{
			if (r->is<XMLList>())
				return r->as<XMLList>()->isEqual(this);
			if (r->is<XML>() && r->as<XML>()->hasSimpleContent())
				return r->toString()==toString();
		}
	}
	if (r->is<ObjectConstructor>())
		return this == r->getClass();

//	LOG_CALL(_("Equal comparison between type ")<<getObjectType()<< _(" and type ") << r->getObjectType());
	return false;
}

bool ASObject::isEqualStrict(ASObject* r)
{
	return ABCVm::strictEqualImpl(this, r);
}

uint32_t ASObject::toUInt()
{
	return toInt();
}

uint16_t ASObject::toUInt16()
{
	unsigned int val32=toUInt();
	return val32 & 0xFFFF;
}

int32_t ASObject::toInt()
{
	asAtom ret = asAtomHandler::fromObject(this);
	toPrimitive(ret);
	return asAtomHandler::toInt(ret);
}

int64_t ASObject::toInt64()
{
	asAtom ret = asAtomHandler::fromObject(this);
	toPrimitive(ret);
	return asAtomHandler::toInt64(ret);
}

/* Implements ECMA's ToPrimitive (9.1) and [[DefaultValue]] (8.6.2.6) */
void ASObject::toPrimitive(asAtom& ret,TP_HINT hint)
{
	//See ECMA 8.6.2.6 for default hint regarding Date
	if(hint == NO_HINT)
	{
		if(this->is<Date>())
			hint = STRING_HINT;
		else
			hint = NUMBER_HINT;
	}

	if(isPrimitive())
	{
		ret = asAtomHandler::fromObject(this);
		return;
	}

	/* for HINT_STRING evaluate first toString, then valueOf
	 * for HINT_NUMBER do it the other way around */
	if(hint == STRING_HINT && has_toString())
	{
		call_toString(ret);
		if(asAtomHandler::isPrimitive(ret))
			return;
	}
	if(has_valueOf())
	{
		call_valueOf(ret);
		if(asAtomHandler::isPrimitive(ret))
			return;
	}
	if(hint != STRING_HINT && has_toString())
	{
		call_toString(ret);
		if(asAtomHandler::isPrimitive(ret))
			return;
	}

	throwError<TypeError>(kConvertToPrimitiveError,this->getClassName());
}

bool ASObject::has_valueOf()
{
	multiname valueOfName(NULL);
	valueOfName.name_type=multiname::NAME_STRING;
	valueOfName.name_s_id=BUILTIN_STRINGS::STRING_VALUEOF;
	valueOfName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	valueOfName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	valueOfName.isAttribute = false;
	return hasPropertyByMultiname(valueOfName, true, true);
}

/* calls the valueOf function on this object
 * we cannot just call the c-function, because it can be overriden from AS3 code
 */
void ASObject::call_valueOf(asAtom& ret)
{
	multiname valueOfName(NULL);
	valueOfName.name_type=multiname::NAME_STRING;
	valueOfName.name_s_id=BUILTIN_STRINGS::STRING_VALUEOF;
	valueOfName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	valueOfName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	valueOfName.isAttribute = false;
	assert_and_throw(hasPropertyByMultiname(valueOfName, true, true));

	asAtom o=asAtomHandler::invalidAtom;
	getVariableByMultiname(o,valueOfName,SKIP_IMPL);
	if (!asAtomHandler::isFunction(o))
		throwError<TypeError>(kCallOfNonFunctionError, valueOfName.normalizedNameUnresolved(getSystemState()));

	asAtom v =asAtomHandler::fromObject(this);
	asAtomHandler::callFunction(o,ret,v,NULL,0,false);
}

bool ASObject::has_toString()
{
	multiname toStringName(NULL);
	toStringName.name_type=multiname::NAME_STRING;
	toStringName.name_s_id=BUILTIN_STRINGS::STRING_TOSTRING;
	toStringName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	toStringName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	toStringName.isAttribute = false;
	return ASObject::hasPropertyByMultiname(toStringName, true, true);
}

/* calls the toString function on this object
 * we cannot just call the c-function, because it can be overriden from AS3 code
 */
void ASObject::call_toString(asAtom &ret)
{
	multiname toStringName(NULL);
	toStringName.name_type=multiname::NAME_STRING;
	toStringName.name_s_id=BUILTIN_STRINGS::STRING_TOSTRING;
	toStringName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	toStringName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	toStringName.isAttribute = false;
	assert(ASObject::hasPropertyByMultiname(toStringName, true, true));

	asAtom o=asAtomHandler::invalidAtom;
	getVariableByMultiname(o,toStringName,SKIP_IMPL);
	if (!asAtomHandler::isFunction(o))
		throwError<TypeError>(kCallOfNonFunctionError, toStringName.normalizedNameUnresolved(getSystemState()));

	asAtom v =asAtomHandler::fromObject(this);
	asAtomHandler::callFunction(o,ret,v,NULL,0,false);
}

tiny_string ASObject::call_toJSON(bool& ok,std::vector<ASObject *> &path, asAtom replacer, const tiny_string &spaces,const tiny_string& filter)
{
	tiny_string res;
	ok = false;
	multiname toJSONName(NULL);
	toJSONName.name_type=multiname::NAME_STRING;
	toJSONName.name_s_id=getSystemState()->getUniqueStringId("toJSON");
	toJSONName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	toJSONName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	toJSONName.isAttribute = false;
	if (!ASObject::hasPropertyByMultiname(toJSONName, true, true))
		return res;

	asAtom o=asAtomHandler::invalidAtom;
	getVariableByMultiname(o,toJSONName,SKIP_IMPL);
	if (!asAtomHandler::isFunction(o))
		return res;
	asAtom v=asAtomHandler::fromObject(this);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::callFunction(o,ret,v,NULL,0,false);
	if (asAtomHandler::isString(ret))
	{
		res += "\"";
		res += asAtomHandler::toString(ret,getSystemState());
		res += "\"";
	}
	else 
		res = asAtomHandler::toObject(ret,getSystemState())->toJSON(path,replacer,spaces,filter);
	ok = true;
	return res;
}

bool ASObject::isPrimitive() const
{
	// ECMA 3, section 4.3.2, T_INTEGER and T_UINTEGER are added
	// because they are special cases of Number
	return type==T_NUMBER || type ==T_UNDEFINED || type == T_NULL ||
		type==T_STRING || type==T_BOOLEAN || type==T_INTEGER ||
		type==T_UINTEGER;
}
bool ASObject::isConstructed() const
{
	return traitsInitialized && constructIndicator;
}
variables_map::variables_map(MemoryAccount *m):slotcount(0),cloneable(true)
{
}

variable* variables_map::findObjVar(uint32_t nameId, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds)
{
	var_iterator ret=Variables.find(nameId);
	while(ret!=Variables.end() && ret->first==nameId)
	{
		if (ret->second.ns == ns)
		{
			if(!(ret->second.kind & traitKinds))
			{
				assert(createKind==NO_CREATE_TRAIT);
				return NULL;
			}
			return &ret->second;
		}
		ret++;
	}

	//Name not present, insert it if we have to create it
	if(createKind==NO_CREATE_TRAIT)
		return NULL;

	var_iterator inserted=Variables.insert(Variables.cbegin(),make_pair(nameId, variable(createKind,ns)) );
	return &inserted->second;
}

bool ASObject::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	//We look in all the object's levels
	uint32_t validTraits=DECLARED_TRAIT;
	if(considerDynamic)
		validTraits|=DYNAMIC_TRAIT;

	if(Variables.findObjVar(getSystemState(),name, validTraits)!=NULL)
		return true;

	if(classdef && classdef->borrowedVariables.findObjVar(getSystemState(),name, DECLARED_TRAIT)!=NULL)
		return true;

	//Check prototype inheritance chain
	if(getClass() && considerPrototype)
	{
		Prototype* proto = getClass()->prototype.getPtr();
		while(proto)
		{
			if(proto->getObj()->findGettable(name) != NULL)
				return true;
			proto=proto->prevPrototype.getPtr();
		}
	}

	//Must not ask for non borrowed traits as static class member are not valid
	return false;
}

void ASObject::setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable)
{
	setDeclaredMethodByQName(name, nsNameAndKind(getSystemState(),ns, NAMESPACE), o, type, isBorrowed,isEnumerable);
}

void ASObject::setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable)
{
	setDeclaredMethodByQName(getSystemState()->getUniqueStringId(name), ns, o, type, isBorrowed,isEnumerable);
}

void ASObject::setDeclaredMethodByQName(uint32_t nameId, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable)
{
	check();
#ifndef NDEBUG
	assert(!initialized);
#endif
	//borrowed properties only make sense on class objects
	assert(!isBorrowed || this->is<Class_base>());
	//use setVariableByQName(name,ns,o,DYNAMIC_TRAIT) on prototypes

	/*
	 * Set the inClass property if not previously set.
	 * This is used for builtin methods. Methods defined by AS3 code
	 * get their inClass set in buildTrait.
	 * It is necesarry to decide if o is a function or a method,
	 * i.e. if a method closure should be created in getProperty.
	 */
	if(isBorrowed && o->inClass == nullptr)
		o->inClass = this->as<Class_base>();
	o->isStatic = !isBorrowed;

	variable* obj=nullptr;
	if(isBorrowed)
	{
		assert(this->is<Class_base>());
		obj=this->as<Class_base>()->borrowedVariables.findObjVar(nameId,ns,DECLARED_TRAIT, DECLARED_TRAIT);
		if (!this->is<Class_inherit>() || this->as<Class_base>()->isSealed || this->as<Class_base>()->isFinal)
			o->setRefConstant();
	}
	else
	{
		obj=Variables.findObjVar(nameId,ns,DECLARED_TRAIT, DECLARED_TRAIT);
	}
	if(this->is<Class_base>() && !this->is<Class_inherit>())
	{
		Class_base* cls = this->as<Class_base>()->super.getPtr();
		while (cls)
		{
			cls->addoverriddenmethod(nameId);
			cls = cls->super.getPtr();
		}
	}
	
	obj->isenumerable = isEnumerable;
	switch(type)
	{
		case NORMAL_METHOD:
		{
			asAtom v = asAtomHandler::fromObject(o);
			obj->setVarNoCoerce(v,this);
			break;
		}
		case GETTER_METHOD:
		{
			ASATOM_DECREF(obj->getter);
			obj->getter=asAtomHandler::fromObject(o);
			break;
		}
		case SETTER_METHOD:
		{
			ASATOM_DECREF(obj->setter);
			obj->setter=asAtomHandler::fromObject(o);
			break;
		}
	}
	if (type != SETTER_METHOD)
		obj->setResultType((const Type*)o->getReturnType());
	o->functionname = nameId;
}

void ASObject::setDeclaredMethodAtomByQName(const tiny_string& name, const tiny_string& ns, asAtom o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable)
{
	setDeclaredMethodAtomByQName(name, nsNameAndKind(getSystemState(),ns, NAMESPACE), o, type, isBorrowed,isEnumerable);
}
void ASObject::setDeclaredMethodAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable)
{
	setDeclaredMethodAtomByQName(getSystemState()->getUniqueStringId(name), ns, o, type, isBorrowed,isEnumerable);
}
void ASObject::setDeclaredMethodAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom f, METHOD_TYPE type, bool isBorrowed, bool isEnumerable)
{
	check();
#ifndef NDEBUG
	assert(!initialized);
#endif
	//borrowed properties only make sense on class objects
	assert(!isBorrowed || this->is<Class_base>());
	//use setVariableByQName(name,ns,o,DYNAMIC_TRAIT) on prototypes

	assert(asAtomHandler::isFunction(f));
	IFunction* o = asAtomHandler::getObject(f)->as<IFunction>();
	/*
	 * Set the inClass property if not previously set.
	 * This is used for builtin methods. Methods defined by AS3 code
	 * get their inClass set in buildTrait.
	 * It is necesarry to decide if o is a function or a method,
	 * i.e. if a method closure should be created in getProperty.
	 */
	if(isBorrowed && o->inClass == NULL)
		o->inClass = this->as<Class_base>();

	variable* obj=NULL;
	if(isBorrowed)
	{
		assert(this->is<Class_base>());
		obj=this->as<Class_base>()->borrowedVariables.findObjVar(nameId,ns,DECLARED_TRAIT, DECLARED_TRAIT);
		if (!this->is<Class_inherit>())
			o->setRefConstant();
	}
	else
	{
		obj=Variables.findObjVar(nameId,ns,DECLARED_TRAIT, DECLARED_TRAIT);
	}
	obj->isenumerable = isEnumerable;
	switch(type)
	{
		case NORMAL_METHOD:
		{
			obj->setVar(f,this);
			break;
		}
		case GETTER_METHOD:
		{
			ASATOM_DECREF(obj->getter);
			obj->getter=f;
			break;
		}
		case SETTER_METHOD:
		{
			ASATOM_DECREF(obj->setter);
			obj->setter=f;
			break;
		}
	}
	o->functionname = nameId;
}

bool ASObject::deleteVariableByMultiname(const multiname& name)
{
	variable* obj=Variables.findObjVar(getSystemState(),name,NO_CREATE_TRAIT,DYNAMIC_TRAIT|DECLARED_TRAIT);
	
	if(obj==nullptr)
	{
		if (classdef && classdef->isSealed)
		{
			if (classdef->Variables.findObjVar(getSystemState(),name, DECLARED_TRAIT)!=nullptr)
				throwError<ReferenceError>(kDeleteSealedError,name.normalizedNameUnresolved(getSystemState()),classdef->getName());
			return false;
		}

		// fixed properties cannot be deleted
		if (hasPropertyByMultiname(name,true,true))
			return false;

		//unknown variables must return true
		return true;
	}
	//Only dynamic traits are deletable
	if (obj->kind != DYNAMIC_TRAIT && obj->kind != INSTANCE_TRAIT)
		return false;

	assert(asAtomHandler::isInvalid(obj->getter) && asAtomHandler::isInvalid(obj->setter) && asAtomHandler::isValid(obj->var));
	//Now dereference the value
	ASATOM_DECREF(obj->var);

	//Now kill the variable
	Variables.killObjVar(getSystemState(),name);
	return true;
}

//In all setter we first pass the value to the interface to see if special handling is possible
void ASObject::setVariableByMultiname_i(multiname& name, int32_t value)
{
	check();
	asAtom v = asAtomHandler::fromInt(value);
	setVariableByMultiname(name,v,CONST_NOT_ALLOWED);
}

variable* ASObject::findSettableImpl(SystemState* sys,variables_map& map, const multiname& name, bool* has_getter)
{
	variable* ret=map.findObjVar(sys,name,NO_CREATE_TRAIT,DECLARED_TRAIT|DYNAMIC_TRAIT);
	if(ret)
	{
		//It seems valid for a class to redefine only the getter, so if we can't find
		//something to get, it's ok
		if(!(asAtomHandler::isValid(ret->setter) || asAtomHandler::isValid(ret->var)))
		{
			ret=nullptr;
			if(has_getter)
				*has_getter=true;
		}
	}
	return ret;
}

variable* ASObject::findSettable(const multiname& name, bool* has_getter)
{
	return findSettableImpl(getSystemState(),Variables, name, has_getter);
}


multiname *ASObject::setVariableByMultiname_intern(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, Class_base* cls, bool *alreadyset)
{
	multiname *retval = nullptr;
	check();
	assert(!cls || classdef->isSubClass(cls));
	//NOTE: we assume that [gs]etSuper and [sg]etProperty correctly manipulate the cur_level (for getActualClass)
	bool has_getter=false;
	variable* obj=findSettable(name, &has_getter);

	if (obj && (obj->kind == CONSTANT_TRAIT && allowConst==CONST_NOT_ALLOWED))
	{
		if (asAtomHandler::isFunction(obj->var) || asAtomHandler::isValid(obj->setter))
			throwError<ReferenceError>(kCannotAssignToMethodError, name.normalizedNameUnresolved(getSystemState()), classdef->as<Class_base>()->getQualifiedClassName());
		else
			throwError<ReferenceError>(kConstWriteError, name.normalizedNameUnresolved(getSystemState()), classdef->as<Class_base>()->getQualifiedClassName());
	}
	if(!obj && cls)
	{
		//Look for borrowed traits before
		//It's valid to override only a getter, so keep
		//looking for a settable even if a super class sets
		//has_getter to true.
		obj=cls->findBorrowedSettable(name,&has_getter);
		if(obj && (obj->issealed || cls->isFinal || cls->isSealed) && asAtomHandler::isInvalid(obj->setter))
		{
			throwError<ReferenceError>(kCannotAssignToMethodError, name.normalizedNameUnresolved(getSystemState()), cls ? cls->getQualifiedClassName() : "");
		}
		// no setter and dynamic variables are allowed, so we force creation of a dynamic variable
		if (obj && asAtomHandler::isInvalid(obj->setter))
			obj=nullptr;
	}

	//Do not set variables in prototype chain. Still have to do
	//lookup to throw a correct error in case a named function
	//exists in prototype chain. See Tamarin test
	//ecma3/Boolean/ecma4_sealedtype_1_rt
	if(!obj && cls && cls->isSealed)
	{
		variable *protoObj = cls->findSettableInPrototype(name);
		if (protoObj && 
			((asAtomHandler::isFunction(protoObj->var)) ||
			 asAtomHandler::isValid(protoObj->setter)))
		{
			throwError<ReferenceError>(kCannotAssignToMethodError, name.normalizedNameUnresolved(getSystemState()), cls ? cls->getQualifiedClassName() : "");
		}
	}

	if(!obj)
	{
		if(has_getter)  // Is this a read-only property?
		{
			throwError<ReferenceError>(kConstWriteError, name.normalizedNameUnresolved(getSystemState()), cls ? cls->getQualifiedClassName() : "");
		}

		// Properties can not be added to a sealed class
		if (cls && cls->isSealed && getVm(getSystemState())->currentCallContext)
		{
			const Type* type = Type::getTypeFromMultiname(&name,getVm(getSystemState())->currentCallContext->mi->context);
			if (type)
				throwError<ReferenceError>(kConstWriteError, name.normalizedNameUnresolved(getSystemState()), cls ? cls->getQualifiedClassName() : "");
			throwError<ReferenceError>(kWriteSealedError, name.normalizedNameUnresolved(getSystemState()), cls->getQualifiedClassName());
		}

		obj=Variables.findObjVar(getSystemState(),name,NO_CREATE_TRAIT,DYNAMIC_TRAIT);
		//Create a new dynamic variable
		if(!obj)
		{
			if(this->is<Global>() && !name.hasGlobalNS)
				throwError<ReferenceError>(kWriteSealedError, name.normalizedNameUnresolved(getSystemState()), this->getClassName());
			
			variables_map::var_iterator inserted=Variables.Variables.insert(Variables.Variables.cbegin(),
				make_pair(name.normalizedNameId(getSystemState()),variable(DYNAMIC_TRAIT,name.ns.size() == 1 ? name.ns[0] : nsNameAndKind())));
			obj = &inserted->second;
		}
	}
	// it seems that instance traits are changed into declared traits if they are overwritten in class objects
	// see tamarin test as3/Definitions/FunctionAccessors/GetSetStatic
	if (this->is<Class_base>() && obj->kind == INSTANCE_TRAIT)
		obj->kind =DECLARED_TRAIT;

	if(asAtomHandler::isValid(obj->setter))
	{
		//Call the setter
		LOG_CALL(_("Calling the setter ")<<obj->type<<" "<<obj->slotid);
		//One argument can be passed without creating an array
		ASObject* target=this;
		asAtom* arg1 = &o;

		asAtom v =asAtomHandler::fromObject(target);
		asAtom ret=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(obj->setter,ret,v,arg1,1,false);
		if (asAtomHandler::is<SyntheticFunction>(obj->setter))
			retval = asAtomHandler::as<SyntheticFunction>(obj->setter)->getSimpleName();
		ASATOM_DECREF(ret);
		if (asAtomHandler::is<SyntheticFunction>(o))
		{
			if (obj->kind == CONSTANT_TRAIT)
				asAtomHandler::getObjectNoCheck(o)->setRefConstant();
			else
				checkFunctionScope(asAtomHandler::getObjectNoCheck(o)->as<SyntheticFunction>());
		}
		if (alreadyset)
			*alreadyset=false;
		ASATOM_DECREF(o);
		// it seems that Adobe allows setters with return values...
		//assert_and_throw(asAtomHandler::isUndefined(ret));
		LOG_CALL(_("End of setter"));
	}
	else
	{
		assert_and_throw(asAtomHandler::isInvalid(obj->getter));
		bool isfunc = asAtomHandler::is<SyntheticFunction>(o);
		if (alreadyset)
		{
			if (o.uintval == obj->var.uintval)
				*alreadyset = true;
			else
			{
				obj->setVar(o,this);
				*alreadyset = o.uintval != obj->var.uintval; // setVar may coerce the object into a new instance, so we need to check if decRef is necessary
			}
		}
		else
			obj->setVar(o,this);
		if (isfunc)
		{
			if (obj->kind == CONSTANT_TRAIT)
				asAtomHandler::getObject(o)->setRefConstant();
			else
				checkFunctionScope(asAtomHandler::getObject(o)->as<SyntheticFunction>());
		}
	}
	return retval;
}

void ASObject::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable)
{
	const nsNameAndKind tmpns(getSystemState(),ns, NAMESPACE);
	setVariableByQName(name, tmpns, o, traitKind,isEnumerable);
}

void ASObject::setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable)
{
	setVariableByQName(getSystemState()->getUniqueStringId(name), ns, o, traitKind,isEnumerable);
}

variable* ASObject::setVariableByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable)
{
	asAtom v = asAtomHandler::fromObject(o);
	return setVariableAtomByQName(nameId, ns, v, traitKind, isEnumerable);
}
variable *ASObject::setVariableAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable)
{
	return setVariableAtomByQName(getSystemState()->getUniqueStringId(name), ns, o, traitKind,isEnumerable);
}
variable *ASObject::setVariableAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable, bool isRefcounted)
{
	variable* obj=Variables.findObjVar(nameId,ns,traitKind,traitKind);
	obj->setVar(o,this,isRefcounted);
	obj->isenumerable=isEnumerable;
	return obj;
}

void ASObject::initializeVariableByMultiname(multiname& name, asAtom &o, multiname* typemname,
		ABCContext* context, TRAIT_KIND traitKind, uint32_t slot_id, bool isenumerable)
{
	check();
	Variables.initializeVar(name, o, typemname, context, traitKind,this,slot_id,isenumerable);
}

variable::variable(TRAIT_KIND _k, asAtom _v, multiname* _t, const Type* _type, const nsNameAndKind& _ns, bool _isenumerable)
		: var(_v),typeUnion(nullptr),setter(asAtomHandler::invalidAtom),getter(asAtomHandler::invalidAtom),ns(_ns),slotid(0),kind(_k),isResolved(false),isenumerable(_isenumerable),issealed(false),isrefcounted(true)
{
	if(_type)
	{
		//The type is known, use it instead of the typemname
		type=_type;
		isResolved=true;
	}
	else
	{
		traitTypemname=_t;
	}
}
void variable::setVar(asAtom v,ASObject *obj, bool _isrefcounted)
{
	//Resolve the typename if we have one
	//currentCallContext may be NULL when inserting legacy
	//children, which is done outisde any ABC context
	if(!isResolved && traitTypemname && getVm(obj->getSystemState())->currentCallContext)
	{
		type = Type::getTypeFromMultiname(traitTypemname, getVm(obj->getSystemState())->currentCallContext->mi->context);
		assert(type);
		isResolved=true;
	}
	if(isResolved && type)
		type->coerce(obj->getSystemState(),v);
	if(isrefcounted && asAtomHandler::getObject(var))
	{
		LOG_CALL("replacing:"<<asAtomHandler::toDebugString(var));
		if (obj->is<Activation_object>() && (asAtomHandler::is<SyntheticFunction>(var) || asAtomHandler::is<AVM1Function>(var)))
			asAtomHandler::getObject(var)->decActivationCount();
		ASATOM_DECREF(var);
	}
	var=v;
	isrefcounted = _isrefcounted;
}

void variable::preparereplacevar(ASObject *obj)
{
	LOG_CALL("replacing:"<<asAtomHandler::toDebugString(var));
	if (obj->is<Activation_object>() && (asAtomHandler::is<SyntheticFunction>(var) || asAtomHandler::is<AVM1Function>(var)))
		asAtomHandler::getObject(var)->decActivationCount();
	ASATOM_DECREF(var);
}

void variables_map::killObjVar(SystemState* sys,const multiname& mname)
{
	uint32_t name=mname.normalizedNameId(sys);
	//The namespaces in the multiname are ordered. So it's possible to use lower_bound
	//to find the first candidate one and move from it
	assert(!mname.ns.empty());
	var_iterator ret=Variables.find(name);
	auto nsIt=mname.ns.begin();

	//Find the namespace
	while(ret!=Variables.end() && ret->first==name)
	{
		//breaks when the namespace is not found
		const nsNameAndKind& ns=ret->second.ns;
		if(ns==*nsIt)
		{
			Variables.erase(ret);
			return;
		}
		else
		{
			++nsIt;
			if(nsIt==mname.ns.cend())
			{
				nsIt=mname.ns.cbegin();
				++ret;
			}
		}
	}
	throw RunTimeException("Variable to kill not found");
}

Class_base* variables_map::getSlotType(unsigned int n)
{
	return (Class_base*)(dynamic_cast<const Class_base*>(slots_vars[n-1]->type));
}

variable* variables_map::findObjVar(SystemState* sys,const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds)
{
	uint32_t name=mname.name_type == multiname::NAME_STRING ? mname.name_s_id : mname.normalizedNameId(sys);

	var_iterator ret=Variables.find(name);
	bool noNS = mname.ns.empty(); // no Namespace in multiname means we check for the empty Namespace
	auto nsIt=mname.ns.begin();

	//Find the namespace
	while(ret!=Variables.end() && ret->first==name)
	{
		//breaks when the namespace is not found
		const nsNameAndKind& ns=ret->second.ns;
		if((noNS && ns.hasEmptyName()) || (!noNS && ns==*nsIt))
		{
			if(ret->second.kind & traitKinds)
				return &ret->second;
			else
				return NULL;
		}
		else if (noNS)
		{
			++ret;
		}
		else
		{
			++nsIt;
			if(nsIt==mname.ns.cend())
			{
				nsIt=mname.ns.cbegin();
				++ret;
			}
		}
	}

	//Name not present, insert it, if the multiname has a single ns and if we have to insert it
	if(createKind==NO_CREATE_TRAIT)
		return NULL;
	if(createKind == DYNAMIC_TRAIT)
	{
		var_iterator inserted=Variables.insert(Variables.cbegin(),
			make_pair(name,variable(createKind,nsNameAndKind())));
		return &inserted->second;
	}
	assert(mname.ns.size() == 1);
	var_iterator inserted=Variables.insert(Variables.cbegin(),
		make_pair(name,variable(createKind,mname.ns[0])));
	return &inserted->second;
}

void variables_map::initializeVar(multiname& mname, asAtom& obj, multiname* typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj, uint32_t slot_id,bool isenumerable)
{
	const Type* type = nullptr;
	if (typemname->isStatic)
		type = typemname->cachedType;
	
	asAtom value=asAtomHandler::invalidAtom;
	 /* If typename is a builtin type, we coerce obj.
	  * It it's not it must be a user defined class,
	  * so we try to find the class it is derived from and create an apropriate uninitialized instance */
	if (type == nullptr)
		type = Type::getBuiltinType(mainObj->getSystemState(),typemname);
	if (type == nullptr)
		type = Type::getTypeFromMultiname(typemname,context);
	if(type==nullptr)
	{
		if (asAtomHandler::isInvalid(obj)) // create dynamic object
		{
			value = asAtomHandler::undefinedAtom;
		}
		else
		{
			assert_and_throw(asAtomHandler::is<Null>(obj) || asAtomHandler::is<Undefined>(obj));
			value = obj;
			if(asAtomHandler::is<Undefined>(obj))
			{
				//Casting undefined to an object (of unknown class)
				//results in Null
				value = asAtomHandler::nullAtom;
			}
		}
	}
	else
	{
		if (asAtomHandler::isInvalid(obj)) // create dynamic object
		{
			if(mainObj->is<Class_base>() 
				&& mainObj->as<Class_base>()->class_name.nameId == typemname->normalizedNameId(mainObj->getSystemState())
				&& mainObj->as<Class_base>()->class_name.nsStringId == typemname->ns[0].nsNameId
				&& typemname->ns[0].kind == NAMESPACE)
			{
				// avoid recursive construction
				value = asAtomHandler::undefinedAtom;
			}
			else
			{
				value = asAtomHandler::undefinedAtom;
				type->coerce(mainObj->getSystemState(),value);
			}
		}
		else
			value = obj;

		if (typemname->isStatic && typemname->cachedType == nullptr)
			typemname->cachedType = type;
	}
	assert(traitKind==DECLARED_TRAIT || traitKind==CONSTANT_TRAIT || traitKind == INSTANCE_TRAIT);
	if (asAtomHandler::getObject(value))
		cloneable = false;

	uint32_t name=mname.normalizedNameId(mainObj->getSystemState());
	auto it = Variables.insert(Variables.cbegin(),make_pair(name, variable(traitKind, value, typemname, type,mname.ns[0],isenumerable)));
	if (slot_id)
		initSlot(slot_id,&(it->second));
}

ASFUNCTIONBODY_ATOM(ASObject,generator)
{
	//By default we assume it's a passthrough cast
	if(argslen==1)
	{
		LOG_CALL(_("Passthrough of ") << asAtomHandler::toDebugString(args[0]));
		ASATOM_INCREF(args[0]);
		ret = args[0];
	}
	else if (argslen > 1)
		throwError<ArgumentError>(kCoerceArgumentCountError,Integer::toString(argslen));
	else
		ret = asAtomHandler::fromObject(Class<ASObject>::getInstanceS(sys));
}

ASFUNCTIONBODY_ATOM(ASObject,_toString)
{
	tiny_string res;
	if (asAtomHandler::is<Class_base>(obj))
	{
		res="[object ";
		res+=sys->getStringFromUniqueId(asAtomHandler::as<Class_base>(obj)->class_name.nameId);
		res+="]";
	}
	else if(asAtomHandler::is<IFunction>(obj))
	{
		// ECMA spec 15.3.4.2 says that toString on a function object is implementation dependent
		// adobe player returns "[object Function-46]", so we do the same
		res="[object ";
		res+="Function-46";
		res+="]";
	}
	else if(asAtomHandler::getClass(obj,sys))
	{
		res="[object ";
		res+=sys->getStringFromUniqueId(asAtomHandler::getClass(obj,sys)->class_name.nameId);
		res+="]";
	}
	else
		res="[object Object]";

	ret = asAtomHandler::fromString(sys,res);
}

ASFUNCTIONBODY_ATOM(ASObject,_toLocaleString)
{
	_toString(ret,sys,obj,args,argslen);
}

ASFUNCTIONBODY_ATOM(ASObject,hasOwnProperty)
{
	assert_and_throw(argslen==1);
	multiname name(NULL);
	switch (asAtomHandler::getObjectType(args[0]))
	{
		case T_INTEGER:
			name.name_type=multiname::NAME_INT;
			name.name_i = asAtomHandler::toInt(args[0]);
			break;
		case T_UINTEGER:
			name.name_type=multiname::NAME_UINT;
			name.name_ui = asAtomHandler::toUInt(args[0]);
			break;
		case T_NUMBER:
			name.name_type=multiname::NAME_NUMBER;
			name.name_d = asAtomHandler::toNumber(args[0]);
			break;
		default:
			name.name_type=multiname::NAME_STRING;
			name.name_s_id=asAtomHandler::toStringId(args[0],sys);
			name.isInteger=Array::isIntegerWithoutLeadingZeros(sys->getStringFromUniqueId(name.name_s_id)) ;
			break;
	}
	name.ns.emplace_back(sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	asAtomHandler::setBool(ret,asAtomHandler::toObject(obj,sys)->hasPropertyByMultiname(name, true, false));
}

ASFUNCTIONBODY_ATOM(ASObject,valueOf)
{
	ASATOM_INCREF(obj);
	ret = obj;
}

ASFUNCTIONBODY_ATOM(ASObject,isPrototypeOf)
{
	assert_and_throw(argslen==1);
	bool res =false;
	Class_base* cls = asAtomHandler::toObject(args[0],sys)->getClass();
	
	while (cls != NULL)
	{
		if (cls->prototype->getObj() == asAtomHandler::getObject(obj))
		{
			res = true;
			break;
		}
		Class_base* clsparent = cls->prototype->getObj()->getClass();
		if (clsparent == cls)
			break;
		cls = clsparent;
	}
	asAtomHandler::setBool(ret,res);
}

ASFUNCTIONBODY_ATOM(ASObject,propertyIsEnumerable)
{
	if (argslen == 0)
	{
		asAtomHandler::setBool(ret,false);
		return;
	}
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=asAtomHandler::toStringId(args[0],sys);
	name.ns.emplace_back(sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	if (asAtomHandler::is<Array>(obj)) // propertyIsEnumerable(index) isn't mentioned in the ECMA specs but is tested for
	{
		Array* a = asAtomHandler::as<Array>(obj);
		unsigned int index = 0;
		if (a->isValidMultiname(sys,name,index))
		{
			asAtomHandler::setBool(ret,index < (unsigned int)a->size());
			return;
		}
	}
	variable* v = asAtomHandler::toObject(obj,sys)->Variables.findObjVar(sys,name, NO_CREATE_TRAIT,DYNAMIC_TRAIT|DECLARED_TRAIT);
	if (v)
		asAtomHandler::setBool(ret,v->isenumerable);
	else
		asAtomHandler::setBool(ret,asAtomHandler::getObject(obj)->hasPropertyByMultiname(name,true,false));
}
ASFUNCTIONBODY_ATOM(ASObject,setPropertyIsEnumerable)
{
	tiny_string propname;
	bool isEnum;
	ARG_UNPACK_ATOM(propname) (isEnum, true);
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=asAtomHandler::toStringId(args[0],sys);
	name.ns.emplace_back(sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	asAtomHandler::toObject(obj,sys)->setIsEnumerable(name, isEnum);
}
ASFUNCTIONBODY_ATOM(ASObject,addProperty)
{
	ret = asAtomHandler::falseAtom;
	tiny_string name;
	_NR<IFunction> getter;
	_NR<IFunction> setter;
	ARG_UNPACK_ATOM(name)(getter)(setter);
	if (name.empty())
		return;
	if (!getter.isNull())
	{
		ret = asAtomHandler::trueAtom;
		asAtomHandler::toObject(obj,sys)->setDeclaredMethodByQName(name,"",getter.getPtr(),GETTER_METHOD,false);
	}
	if (!setter.isNull())
	{
		ret = asAtomHandler::trueAtom;
		asAtomHandler::toObject(obj,sys)->setDeclaredMethodByQName(name,"",setter.getPtr(),SETTER_METHOD,false);
	}
}
ASFUNCTIONBODY_ATOM(ASObject,registerClass)
{
	ret = asAtomHandler::falseAtom;
	tiny_string name;
	_NR<IFunction> theClassConstructor;
	ARG_UNPACK_ATOM(name)(theClassConstructor);
	if (name.empty())
		return;
	if (!theClassConstructor.isNull())
	{
		RootMovieClip* root = nullptr;
		if (theClassConstructor->is<AVM1Function>())
			root = theClassConstructor->as<AVM1Function>()->getClip()->loadedFrom;
		if (!root)
			root = sys->mainClip;
		ret = asAtomHandler::fromBool(root->AVM1registerTagClass(name,theClassConstructor));
	}
}

void ASObject::setIsEnumerable(const multiname &name, bool isEnum)
{
	variable* v = Variables.findObjVar(getSystemState(),name, NO_CREATE_TRAIT,DYNAMIC_TRAIT);
	if (v)
		v->isenumerable = isEnum;
}

bool ASObject::cloneInstance(ASObject *target)
{
	return Variables.cloneInstance(target->Variables);
}

void ASObject::checkFunctionScope(ASObject* o)
{
	SyntheticFunction* f = o->as<SyntheticFunction>();
	if (!getVm(getSystemState())->currentCallContext->mi->needsActivation() || this->is<Global>() || this->is<Function_object>())
		return;
	for (auto it = f->func_scope->scope.rbegin(); it != f->func_scope->scope.rend(); it++)
	{
		if (asAtomHandler::getObject(it->object) != this)
			continue;
		if (this->is<Activation_object>())
		{
			f->incActivationCount();
			this->as<Activation_object>()->addDynamicFunctionUsage(f);
		}
		else
			f->addDynamicReferenceObject(this);
		break;
	}
}

asAtom ASObject::getVariableBindingValue(const tiny_string &name)
{
	uint32_t nameId = getSystemState()->getUniqueStringId(name);
	asAtom obj=asAtomHandler::invalidAtom;
	multiname objName(NULL);
	objName.name_type=multiname::NAME_STRING;
	objName.name_s_id=nameId;
	getVariableByMultiname(obj,objName);
	return obj;
}

ASFUNCTIONBODY_ATOM(ASObject,_constructor)
{
}

ASFUNCTIONBODY_ATOM(ASObject,_constructorNotInstantiatable)
{
	throwError<ArgumentError>(kCantInstantiateError, asAtomHandler::toObject(obj,sys)->getClassName());
}

void ASObject::initSlot(unsigned int n, variable* v)
{
	Variables.initSlot(n,v);
}
void ASObject::initAdditionalSlots(std::vector<multiname*>& additionalslots)
{
	unsigned int n = Variables.slots_vars.size();
	for (auto it = additionalslots.begin(); it != additionalslots.end(); it++)
	{
		uint32_t nameId = (*it)->normalizedNameId(getSystemState());
		auto ret=Variables.Variables.find(nameId);
	
		assert_and_throw(ret!=Variables.Variables.end());
		Variables.initSlot(++n,&(ret->second));
	}
}
int32_t ASObject::getVariableByMultiname_i(const multiname& name)
{
	check();

	asAtom ret=asAtomHandler::invalidAtom;
	getVariableByMultinameIntern(ret,name,this->getClass());
	assert_and_throw(asAtomHandler::isValid(ret));
	return asAtomHandler::toInt(ret);
}

variable* ASObject::findVariableByMultiname(const multiname& name, Class_base* cls, uint32_t *nsRealID, bool *isborrowed, bool considerdynamic)
{
	//Get from the current object without considering borrowed properties
	variable* obj=Variables.findObjVar(getSystemState(),name,name.hasEmptyNS || considerdynamic ? DECLARED_TRAIT|DYNAMIC_TRAIT : DECLARED_TRAIT,nsRealID);
	if(obj)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(asAtomHandler::isValid(obj->getter)|| asAtomHandler::isValid(obj->var)))
			obj=nullptr;
		if (isborrowed)
			*isborrowed=false;
	}

	if(!obj)
	{
		//Look for borrowed traits before
		if (cls)
		{
			if (isborrowed)
				*isborrowed=true;
			obj= ASObject::findGettableImpl(getSystemState(), cls->borrowedVariables,name,nsRealID);
			if(!obj && name.hasEmptyNS)
			{
				//Check prototype chain
				Prototype* proto = cls->prototype.getPtr();
				while(proto)
				{
					obj=proto->getObj()->Variables.findObjVar(getSystemState(),name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealID);
					if(obj)
					{
						//It seems valid for a class to redefine only the setter, so if we can't find
						//something to get, it's ok
						if(!(asAtomHandler::isValid(obj->getter) || asAtomHandler::isValid(obj->var)))
							obj=nullptr;
					}
					if(obj)
						break;
					proto = proto->prevPrototype.getPtr();
				}
			}
		}
	}
	return obj;
}

GET_VARIABLE_RESULT ASObject::getVariableByMultinameIntern(asAtom &ret, const multiname& name, Class_base* cls,  GET_VARIABLE_OPTION opt)
{
	check();
	assert(!cls || classdef->isSubClass(cls));
	uint32_t nsRealId;
	GET_VARIABLE_RESULT res = GET_VARIABLE_RESULT::GETVAR_NORMAL;
	variable* obj=Variables.findObjVar(getSystemState(),name,((opt & FROM_GETLEX) || name.hasEmptyNS || name.hasBuiltinNS || name.ns.empty()) ? DECLARED_TRAIT|DYNAMIC_TRAIT : DECLARED_TRAIT,&nsRealId);
	if(obj)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(asAtomHandler::isValid(obj->getter) || asAtomHandler::isValid(obj->var)))
			obj=nullptr;
	}
	else if (opt & DONT_CHECK_CLASS)
		return res;

	if(!obj)
	{
		//Look for borrowed traits before
		if (cls)
		{
			obj= ASObject::findGettableImpl(getSystemState(), cls->borrowedVariables,name,&nsRealId);
			if(!obj && name.hasEmptyNS)
			{
				//Check prototype chain
				Prototype* proto = cls->prototype.getPtr();
				while(proto)
				{
					obj=proto->getObj()->Variables.findObjVar(getSystemState(),name,DECLARED_TRAIT|DYNAMIC_TRAIT,&nsRealId);
					if(obj)
					{
						//It seems valid for a class to redefine only the setter, so if we can't find
						//something to get, it's ok
						if(!(asAtomHandler::isValid(obj->getter) || asAtomHandler::isValid(obj->var)))
							obj=nullptr;
					}
					if(obj)
						break;
					proto = proto->prevPrototype.getPtr();
				}
			}
		}
		if(!obj)
			return res;
		res = (GET_VARIABLE_RESULT)(res | GETVAR_CACHEABLE);
	}
	if (obj->kind == CONSTANT_TRAIT)
		res = (GET_VARIABLE_RESULT)(res | GETVAR_CACHEABLE | GETVAR_ISCONSTANT);


	if ( this->is<Class_base>() )
	{
		res = (GET_VARIABLE_RESULT)(res | GET_VARIABLE_RESULT::GETVAR_CACHEABLE);
		if (!(opt & FROM_GETLEX) && obj->kind == INSTANCE_TRAIT && getSystemState()->getNamespaceFromUniqueId(nsRealId).kind != STATIC_PROTECTED_NAMESPACE)
			throwError<TypeError>(kCallOfNonFunctionError,name.normalizedNameUnresolved(getSystemState()));
	}

	if(asAtomHandler::isValid(obj->getter))
	{
		if (opt & DONT_CALL_GETTER)
		{
			asAtomHandler::set(ret,obj->getter);
			res = (GET_VARIABLE_RESULT)(res | GET_VARIABLE_RESULT::GETVAR_ISGETTER);
			return res;
		}
		//Call the getter
		LOG_CALL("Calling the getter intern for " << name << " on " << this->toDebugString());
		assert(asAtomHandler::isFunction(obj->getter));
		asAtomHandler::as<IFunction>(obj->getter)->callGetter(ret,asAtomHandler::getClosure(obj->getter) ? asAtomHandler::getClosure(obj->getter) : this);
		LOG_CALL(_("End of getter")<< ' ' << asAtomHandler::toDebugString(obj->getter)<<" result:"<<asAtomHandler::toDebugString(ret));
	}
	else
	{
		assert_and_throw(asAtomHandler::isInvalid(obj->setter));
		if (!(opt & NO_INCREF))
			ASATOM_INCREF(obj->var);
		if(asAtomHandler::isFunction(obj->var) && asAtomHandler::getObject(obj->var)->as<IFunction>()->isMethod())
		{
			if (asAtomHandler::getClosure(obj->var))
			{
				LOG_CALL("function " << name << " is already bound to "<<asAtomHandler::toDebugString(obj->var) );
				if (asAtomHandler::getObject(obj->var)->as<IFunction>()->clonedFrom)
					ASATOM_INCREF(obj->var);
				asAtomHandler::getClosure(obj->var)->incRef();
				asAtomHandler::set(ret,obj->var);
			}
			else
			{
				LOG_CALL("Attaching this " << this->toDebugString() << " to function " << name << " "<<asAtomHandler::toDebugString(obj->var));
				asAtomHandler::setFunction(ret,asAtomHandler::getObject(obj->var),this);
			}
		}
		else
			asAtomHandler::set(ret,obj->var);
	}
	return res;
}

void ASObject::getVariableByMultiname(asAtom& ret, const tiny_string& name, std::list<tiny_string> namespaces)
{
	multiname varName(NULL);
	varName.name_type=multiname::NAME_STRING;
	varName.name_s_id=getSystemState()->getUniqueStringId(name);
	varName.hasEmptyNS = false;
	varName.hasBuiltinNS = false;
	varName.hasGlobalNS = false;
	for (auto ns=namespaces.begin(); ns!=namespaces.end(); ns++)
	{
		nsNameAndKind newns(getSystemState(),*ns,NAMESPACE);
		varName.ns.push_back(newns);
		if (newns.hasEmptyName())
			varName.hasEmptyNS = true;
		if (newns.hasBuiltinName())
			varName.hasBuiltinNS = true;
		if (newns.kind == NAMESPACE)
			varName.hasGlobalNS = true;
	}
	varName.isAttribute = false;

	getVariableByMultinameIntern(ret,varName,this->getClass());
}

void ASObject::executeASMethod(asAtom& ret,const tiny_string& methodName,
					std::list<tiny_string> namespaces,
					asAtom* args,
					uint32_t num_args)
{
	asAtom o=asAtomHandler::invalidAtom;
	getVariableByMultiname(o,methodName, namespaces);
	if (!asAtomHandler::isFunction(o))
		throwError<TypeError>(kCallOfNonFunctionError, methodName);
	asAtom v =asAtomHandler::fromObject(this);
	asAtomHandler::callFunction(o,ret,v,args,num_args,false);
}

void ASObject::check() const
{
	//Put here a bunch of safety check on the object
#ifndef NDEBUG
	assert(getConstant() || (getRefCount()>0));
#endif
	//	Variables.check();
}

void ASObject::setRefConstant()
{
	getSystemState()->registerConstantRef(this);
	setConstant();
}
GET_VARIABLE_RESULT ASObject::AVM1getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt)
{
	GET_VARIABLE_RESULT res = getVariableByMultiname(ret,name,opt);
	if (asAtomHandler::isInvalid(ret))
	{
		ASObject* pr = getprop_prototype();
		while (pr)
		{
			res = pr->getVariableByMultiname(ret,name,opt);
			if (asAtomHandler::isValid(ret))
				break;
			pr = pr->getprop_prototype();
		}
	}
	return res;
}

void variables_map::check() const
{
	//Heavyweight stuff
#ifdef EXPENSIVE_DEBUG
	variables_map::const_var_iterator it=Variables.begin();
	for(;it!=Variables.end();++it)
	{
		variables_map::const_var_iterator next=it;
		++next;
		if(next==Variables.end())
			break;

		//No double definition of a single variable should exist
		if(it->first==next->first && it->second.ns==next->second.ns)
		{
			if(asAtomHandler::isInvalid(it->second.var) && asAtomHandler::isInvalid(next->second.var))
				continue;

			if(asAtomHandler::isInvalid(it->second.var) || asAtomHandler::isInvalid(next->second.var))
			{
				LOG(LOG_INFO, it->first << " " << it->second.ns);
				LOG(LOG_INFO, asAtomHandler::getObjectType(it->second.var) << ' ' << asAtomHandler::getObjectType(it->second.setter) << ' ' << asAtomHandler::getObjectType(it->second.getter));
				LOG(LOG_INFO, asAtomHandler::getObjectType(next->second.var) << ' ' << asAtomHandler::getObjectType(next->second.setter) << ' ' << asAtomHandler::getObjectType(next->second.getter));
				abort();
			}

			if(!asAtomHandler::isFunction(it->second.var) || !asAtomHandler::isFunction(next->second.var))
			{
				LOG(LOG_INFO, it->first);
				abort();
			}
		}
	}
#endif
}

void variables_map::dumpVariables()
{
	var_iterator it=Variables.begin();
	for(;it!=Variables.end();++it)
	{
		const char* kind;
		switch(it->second.kind)
		{
			case DECLARED_TRAIT:
				kind="Declared: ";
				break;
			case CONSTANT_TRAIT:
				kind="Constant: ";
				break;
			case INSTANCE_TRAIT:
				kind="Declared (instance)";
				break;
			case DYNAMIC_TRAIT:
				kind="Dynamic: ";
				break;
			case NO_CREATE_TRAIT:
			default:
				assert(false);
		}
		LOG(LOG_INFO, kind <<  '[' << it->second.ns << "] "<< hex<<it->first<<dec<<" "<<
			getSys()->getStringFromUniqueId(it->first) << ' ' <<
			asAtomHandler::toDebugString(it->second.var) << ' ' << asAtomHandler::toDebugString(it->second.setter) << ' ' << asAtomHandler::toDebugString(it->second.getter) << ' ' <<it->second.slotid << ' ' );//<<dynamic_cast<const Class_base*>(it->second.type));
	}
}

variables_map::~variables_map()
{
	destroyContents();
}

void variables_map::destroyContents()
{
	var_iterator it=Variables.begin();
	while(it!=Variables.cend())
	{
		if (it->second.isrefcounted)
		{
			ASATOM_DECREF(it->second.var);
			ASATOM_DECREF(it->second.setter);
			ASATOM_DECREF(it->second.getter);
		}
		it = Variables.erase(it);
	}
	slots_vars.clear();
	slotcount=0;
}

bool variables_map::cloneInstance(variables_map &map)
{
	if (!cloneable)
		return false;
	map.Variables = Variables;
	auto it = map.Variables.begin();
	while (it !=map.Variables.end())
	{
		if (it->second.slotid)
			map.initSlot(it->second.slotid,&(it->second));
		it++;
	}
	return true;
}

void variables_map::removeAllDeclaredProperties()
{
	var_iterator it=Variables.begin();
	while(it!=Variables.cend())
	{
		if (asAtomHandler::isValid(it->second.getter)
			|| asAtomHandler::isValid(it->second.setter))
		{
			it = Variables.erase(it);
		}
		else
			it++;
	}
}

#ifndef NDEBUG
std::map<Class_base*,uint32_t> ASObject::objectcounter;
void ASObject::dumpObjectCounters(uint32_t threshhold)
{
	uint64_t c = 0;
	auto it = objectcounter.begin();
	while (it != objectcounter.end())
	{
		if (it->second > threshhold)
			LOG(LOG_INFO,"counter:"<<it->first->toDebugString()<<":"<<it->second<<"-"<<it->first->freelist->freelistsize<<"="<<(it->second-it->first->freelist->freelistsize));
		c += it->second;
		it++;
	}
	LOG(LOG_INFO,"countall:"<<c);
}
#endif
ASObject::ASObject(Class_base* c,SWFOBJECT_TYPE t,CLASS_SUBTYPE st):objfreelist(c && c->getSystemState()->singleworker && c->isReusable ? c->freelist : nullptr),Variables((c)?c->memoryAccount:nullptr),classdef(c),proxyMultiName(nullptr),sys(c?c->sys:nullptr),
	stringId(UINT32_MAX),type(t),subtype(st),traitsInitialized(false),constructIndicator(false),constructorCallComplete(false),implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
	if (c)
	{
		uint32_t x = objectcounter[c];
		x++;
		objectcounter[c] = x;
	}
#endif
}

ASObject::ASObject(const ASObject& o):objfreelist(o.classdef && o.classdef->getSystemState()->singleworker && o.classdef->isReusable ? o.classdef->freelist : nullptr),Variables((o.classdef)?o.classdef->memoryAccount:nullptr),classdef(nullptr),proxyMultiName(nullptr),sys(o.classdef? o.classdef->sys : nullptr),
	stringId(o.stringId),type(o.type),subtype(o.subtype),traitsInitialized(false),constructIndicator(false),constructorCallComplete(false),implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif
	assert(o.Variables.size()==0);
}

void ASObject::setClass(Class_base* c)
{
	if (classdef == c)
		return;
	classdef=c;
	if(c)
		this->sys = c->sys;
}

bool ASObject::destruct()
{
	return destructIntern();
}

bool ASObject::AVM1HandleKeyboardEvent(KeyboardEvent *e) 
{ 
	if (e->type =="keyDown")
	{
		multiname m(nullptr);
		m.name_type = multiname::NAME_STRING;
		m.name_s_id = getSystemState()->getUniqueStringId("onKeyDown");
		asAtom f=asAtomHandler::invalidAtom;
		AVM1getVariableByMultiname(f,m);
		if (asAtomHandler::is<AVM1Function>(f))
			asAtomHandler::as<AVM1Function>(f)->call(nullptr,nullptr,nullptr,0);
	}
	if (e->type =="keyUp")
	{
		multiname m(nullptr);
		m.name_type = multiname::NAME_STRING;
		m.name_s_id = getSystemState()->getUniqueStringId("onKeyUp");
		asAtom f=asAtomHandler::invalidAtom;
		AVM1getVariableByMultiname(f,m);
		if (asAtomHandler::is<AVM1Function>(f))
			asAtomHandler::as<AVM1Function>(f)->call(nullptr,nullptr,nullptr,0);
	}
	return false; 
}

bool ASObject::AVM1HandleMouseEvent(EventDispatcher *dispatcher, MouseEvent *e) 
{
	return AVM1HandleMouseEventStandard(dispatcher,e);
}
bool ASObject::AVM1HandleMouseEventStandard(ASObject *dispobj,MouseEvent *e)
{
	bool result = false;
	asAtom func=asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.isAttribute = false;
	asAtom ret=asAtomHandler::invalidAtom;
	asAtom obj = asAtomHandler::fromObject(this);
	if (e->type == "mouseMove")
	{
		m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEMOVE;
		AVM1getVariableByMultiname(func,m);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			result=true;
		}
	}
	else if (e->type == "mouseDown")
	{
		if (dispobj && ((dispobj == this && !dispobj->is<DisplayObject>()) 
				|| (dispobj->as<DisplayObject>()->isVisible() && this->is<DisplayObject>() && dispobj->as<DisplayObject>()->findParent(this->as<DisplayObject>()))))
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONPRESS;
			AVM1getVariableByMultiname(func,m);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				result=true;
			}
		}
		func=asAtomHandler::invalidAtom;
		m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEDOWN;
		AVM1getVariableByMultiname(func,m);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			result=true;
		}
	}
	else if (e->type == "mouseUp")
	{
		if (dispobj && ((dispobj == this && !dispobj->is<DisplayObject>()) 
				|| (dispobj->as<DisplayObject>()->isVisible() && this->is<DisplayObject>() && dispobj->as<DisplayObject>()->findParent(this->as<DisplayObject>()))))
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONRELEASE;
			AVM1getVariableByMultiname(func,m);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				result=true;
			}
		}
		m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEUP;
		AVM1getVariableByMultiname(func,m);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			result=true;
		}
	}
	else if (e->type == "mouseWheel")
	{
		m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEWHEEL;
		AVM1getVariableByMultiname(func,m);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			result=true;
		}
	}
	else if (e->type == "releaseOutside")
	{
		m.name_s_id=BUILTIN_STRINGS::STRING_ONRELEASEOUTSIDE;
		AVM1getVariableByMultiname(func,m);
		if (asAtomHandler::is<AVM1Function>(func))
		{
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			result=true;
		}
	}
	else if (e->type == "rollOver")
	{
		if (dispobj && ((dispobj == this && !dispobj->is<DisplayObject>()) 
				|| (dispobj->as<DisplayObject>()->isVisible() && this->is<DisplayObject>() && dispobj->as<DisplayObject>()->findParent(this->as<DisplayObject>()))))
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONROLLOVER;
			AVM1getVariableByMultiname(func,m);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				result=true;
			}
		}
	}
	else if (e->type == "rollOut")
	{
		if (dispobj && ((dispobj == this && !dispobj->is<DisplayObject>()) 
				|| (dispobj->as<DisplayObject>()->isVisible() && this->is<DisplayObject>() && dispobj->as<DisplayObject>()->findParent(this->as<DisplayObject>()))))
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONROLLOUT;
			AVM1getVariableByMultiname(func,m);
			if (asAtomHandler::is<AVM1Function>(func))
			{
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
				result=true;
			}
		}
	}
	else if (e->type == "mouseOver" || e->type == "mouseOut")
	{
		// not available in AVM1
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"handling avm1 mouse event "<<e->type);
	return result;
}

void ASObject::copyValues(ASObject *target)
{
	auto it = Variables.Variables.begin();
	while (it != Variables.Variables.end())
	{
		if (it->second.kind == DYNAMIC_TRAIT)
		{
			multiname m(nullptr);
			m.name_type = multiname::NAME_STRING;
			m.name_s_id = it->first;
			target->setVariableByMultiname(m,it->second.var,CONST_ALLOWED);
		}
		it++;
	}
}






uint32_t variables_map::findInstanceSlotByMultiname(multiname* name,SystemState* sys)
{
	uint32_t nameId = name->normalizedNameId(sys);
	var_iterator it = Variables.find(nameId);
	while(it!=Variables.end() && it->first == nameId)
	{
		if ((name->ns.size() == 0 || name->ns[0] == it->second.ns)
				&& (it->second.kind == INSTANCE_TRAIT))
				return it->second.slotid;
		it++;
	}
	return UINT32_MAX;
}

variable* variables_map::getValueAt(unsigned int index)
{
	//TODO: CHECK behaviour on overridden methods
	if(index<Variables.size())
	{
		var_iterator it=Variables.begin();
		uint32_t i = 0;
		while (i < index)
		{
			++i;
			++it;
		}
		return &it->second;
	}
	else
		throw RunTimeException("getValueAt out of bounds");
}

void ASObject::getValueAt(asAtom &ret,int index)
{
	variable* obj=Variables.getValueAt(index);
	assert_and_throw(obj);
	if(asAtomHandler::isValid(obj->getter))
	{
		//Call the getter
		LOG_CALL(_("Calling the getter"));
		asAtom v=asAtomHandler::fromObject(this);
		asAtomHandler::callFunction(obj->getter,ret, v,NULL,0,false);
		LOG_CALL("End of getter at index "<<index<<":"<< asAtomHandler::toDebugString(obj->getter)<<" result:"<<asAtomHandler::toDebugString(ret));
	}
	else
	{
		ASATOM_INCREF(obj->var);
		ret = obj->var;
	}
}

uint32_t variables_map::getNameAt(unsigned int index) const
{
	//TODO: CHECK behaviour on overridden methods
	if(index<Variables.size())
	{
		const_var_iterator it=Variables.begin();
		uint32_t i = 0;
		while (i < index)
		{
			++i;
			++it;
		}
		return it->first;
	}
	else
		throw RunTimeException("getNameAt out of bounds");
}

unsigned int ASObject::numVariables() const
{
	return Variables.size();
}

void ASObject::serializeDynamicProperties(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t> traitsMap,bool usedynamicPropertyWriter)
{
	if (usedynamicPropertyWriter && 
			!out->getSystemState()->static_ObjectEncoding_dynamicPropertyWriter.isNull() &&
			!out->getSystemState()->static_ObjectEncoding_dynamicPropertyWriter->is<Null>())
	{
		DynamicPropertyOutput* o = Class<DynamicPropertyOutput>::getInstanceS(out->getSystemState());
		multiname m(nullptr);
		m.name_type = multiname::NAME_STRING;
		m.name_s_id = out->getSystemState()->getUniqueStringId("writeDynamicProperties");

		asAtom wr=asAtomHandler::invalidAtom;
		out->getSystemState()->static_ObjectEncoding_dynamicPropertyWriter->getVariableByMultiname(wr,m,SKIP_IMPL);
		if (!asAtomHandler::isFunction(wr))
			throwError<TypeError>(kCallOfNonFunctionError, m.normalizedNameUnresolved(out->getSystemState()));
	
		asAtom ret=asAtomHandler::invalidAtom;
		asAtom v =asAtomHandler::fromObject(out->getSystemState()->static_ObjectEncoding_dynamicPropertyWriter.getPtr());
		ASATOM_INCREF(v);
		asAtom args[2];
		args[0] = asAtomHandler::fromObject(this);
		args[1] = asAtomHandler::fromObject(o);
		asAtomHandler::callFunction(wr,ret,v,args,2,false);
		o->serializeDynamicProperties(out, stringMap, objMap, traitsMap,false);
		delete o;
	}
	else
		Variables.serialize(out, stringMap, objMap, traitsMap);
}

void variables_map::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	bool amf0 = out->getObjectEncoding() == ObjectEncoding::AMF0;
	//Pairs of name, value
	auto it=Variables.begin();
	for(;it!=Variables.end();it++)
	{
		if(it->second.kind!=DYNAMIC_TRAIT)
			continue;
		//Dynamic traits always have empty namespace
		assert(it->second.ns.hasEmptyName());
		if (amf0)
			out->writeStringAMF0(out->getSystemState()->getStringFromUniqueId(it->first));
		else
			out->writeStringVR(stringMap,out->getSystemState()->getStringFromUniqueId(it->first));
		asAtomHandler::toObject(it->second.var,out->getSystemState())->serialize(out, stringMap, objMap, traitsMap);
	}
	//The empty string closes the object
	if (!amf0) out->writeStringVR(stringMap, "");
}

void ASObject::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	bool amf0 = out->getObjectEncoding() == ObjectEncoding::AMF0;
	if (amf0)
		out->writeByte(amf0_object_marker);
	else
		out->writeByte(object_marker);
	//Check if the object has been already serialized to send it by reference
	auto it=objMap.find(this);
	if(it!=objMap.end())
	{
		if (amf0)
		{
			out->writeByte(amf0_reference_marker);
			out->writeShort(it->second);
		}
		else
		{
			//The least significant bit is 0 to signal a reference
			out->writeU29(it->second << 1);
		}
		return;
		
	}

	Class_base* type=getClass();
	assert_and_throw(type);

	//Check if an alias is registered
	auto aliasIt=getSystemState()->aliasMap.begin();
	const auto aliasEnd=getSystemState()->aliasMap.end();
	//Linear search for alias
	tiny_string alias;
	for(;aliasIt!=aliasEnd;++aliasIt)
	{
		if(aliasIt->second==type)
		{
			alias=aliasIt->first;
			break;
		}
	}
	bool serializeTraits = alias.empty()==false;

	if(type->isSubClass(InterfaceClass<IExternalizable>::getClass(getSystemState())))
	{
		//Custom serialization necessary
		if(!serializeTraits)
			throwError<TypeError>(kInvalidParamError);
		if (amf0)
		{
			LOG(LOG_NOT_IMPLEMENTED,"serializing IExternalizable in AMF0 not implemented");
			out->writeShort(0);
			out->writeByte(amf0_object_end_marker);
			return;
		}
		out->writeU29(0x7);
		out->writeStringVR(stringMap, alias);

		//Invoke writeExternal
		multiname writeExternalName(NULL);
		writeExternalName.name_type=multiname::NAME_STRING;
		writeExternalName.name_s_id=getSystemState()->getUniqueStringId("writeExternal");
		writeExternalName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
		writeExternalName.isAttribute = false;

		asAtom o=asAtomHandler::invalidAtom;
		getVariableByMultiname(o,writeExternalName,SKIP_IMPL);
		assert_and_throw(asAtomHandler::isFunction(o));
		asAtom tmpArg[1] = { asAtomHandler::fromObject(out) };
		asAtom v=asAtomHandler::fromObject(this);
		asAtom r=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(o,r,v, tmpArg, 1,false);
		return;
	}

	//Add the object to the map
	objMap.insert(make_pair(this, objMap.size()));

	uint32_t traitsCount=0;
	const variables_map::var_iterator beginIt = Variables.Variables.begin();
	const variables_map::var_iterator endIt = Variables.Variables.end();
	//Check if the class traits has been already serialized to send it by reference
	auto it2=traitsMap.find(type);

	if (amf0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing ASObject in AMF0 not completely implemented");
		if(it2!=traitsMap.end())
		{
			out->writeByte(amf0_reference_marker);
			out->writeShort(it2->second);
			for(variables_map::var_iterator varIt=beginIt; varIt != endIt; ++varIt)
			{
				if(varIt->second.kind==DECLARED_TRAIT)
				{
					if(!varIt->second.ns.hasEmptyName())
					{
						//Skip variable with a namespace, like protected ones
						continue;
					}
					out->writeStringAMF0(getSystemState()->getStringFromUniqueId(varIt->first));
					asAtomHandler::toObject(varIt->second.var,getSystemState())->serialize(out, stringMap, objMap, traitsMap);
				}
			}
		}
		if(!type->isSealed)
			serializeDynamicProperties(out, stringMap, objMap, traitsMap);
		out->writeShort(0);
		out->writeByte(amf0_object_end_marker);
		return;
	}

	if(it2!=traitsMap.end())
		out->writeU29((it2->second << 2) | 1);
	else
	{
		traitsMap.insert(make_pair(type, traitsMap.size()));
		for(variables_map::const_var_iterator varIt=beginIt; varIt != endIt; ++varIt)
		{
			if(varIt->second.kind==DECLARED_TRAIT)
			{
				if(!varIt->second.ns.hasEmptyName())
				{
					//Skip variable with a namespace, like protected ones
					continue;
				}
				traitsCount++;
			}
		}
		uint32_t dynamicFlag=(type->isSealed)?0:(1 << 3);
		out->writeU29((traitsCount << 4) | dynamicFlag | 0x03);
		out->writeStringVR(stringMap, alias);
		for(variables_map::const_var_iterator varIt=beginIt; varIt != endIt; ++varIt)
		{
			if(varIt->second.kind==DECLARED_TRAIT)
			{
				if(!varIt->second.ns.hasEmptyName())
				{
					//Skip variable with a namespace, like protected ones
					continue;
				}
				out->writeStringVR(stringMap, getSystemState()->getStringFromUniqueId(varIt->first));
			}
		}
	}
	for(variables_map::var_iterator varIt=beginIt; varIt != endIt; ++varIt)
	{
		if(varIt->second.kind==DECLARED_TRAIT)
		{
			if(!varIt->second.ns.hasEmptyName())
			{
				//Skip variable with a namespace, like protected ones
				continue;
			}
			asAtomHandler::toObject(varIt->second.var,getSystemState())->serialize(out, stringMap, objMap, traitsMap);
		}
	}
	if(!type->isSealed)
		serializeDynamicProperties(out, stringMap, objMap, traitsMap);
}

ASObject *ASObject::describeType() const
{
	pugi::xml_document p;
	pugi::xml_node root = p.append_child("type");

	switch (getObjectType())
	{
		case T_NULL:
			root.append_attribute("name").set_value("null");
			break;
		case T_UNDEFINED:
			root.append_attribute("name").set_value("void");
			break;
		default:
			break;
	}

	// type attributes
	Class_base* prot=getClass();
	if(prot)
	{
		root.append_attribute("name").set_value(prot->getQualifiedClassName(true).raw_buf());
		if(prot->super)
			root.append_attribute("base").set_value(prot->super->getQualifiedClassName(true).raw_buf());
	}
	bool isDynamic = classdef && !classdef->isSealed;
	root.append_attribute("isDynamic").set_value(isDynamic?"true":"false");
	bool isFinal = !classdef || classdef->isFinal;
	root.append_attribute("isFinal").set_value(isFinal?"true":"false");
	root.append_attribute("isStatic").set_value("false");

	if(prot)
		prot->describeInstance(root,false,true);

	//LOG(LOG_INFO,"describeType:"<< Class<XML>::getInstanceS(getSystemState(),root)->toXMLString_internal());

	return XML::createFromNode(root);
}

tiny_string ASObject::toJSON(std::vector<ASObject *> &path, asAtom replacer, const tiny_string &spaces,const tiny_string& filter)
{
	bool ok;
	tiny_string res = call_toJSON(ok,path,replacer,spaces,filter);
	if (ok)
		return res;

	tiny_string newline = (spaces.empty() ? "" : "\n");
	if (this->isPrimitive())
	{
		switch(this->type)
		{
			case T_STRING:
			{
				res += "\"";
				tiny_string sub = this->toString();
				for (CharIterator it=sub.begin(); it!=sub.end(); it++)
				{
					switch (*it)
					{
						case '\b':
							res += "\\b";
							break;
						case '\f':
							res += "\\f";
							break;
						case '\n':
							res += "\\n";
							break;
						case '\r':
							res += "\\r";
							break;
						case '\t':
							res += "\\t";
							break;
						case '\"':
							res += "\\\"";
							break;
						case '\\':
							res += "\\\\";
							break;
						default:
							if (*it < 0x20 || *it > 0xff)
							{
								char hexstr[7];
								sprintf(hexstr,"\\u%04x",*it);
								res += hexstr;
							}
							else
								res += *it;
							break;
					}
				}
				res += "\"";
				break;
			}
			case T_UNDEFINED:
				res += "null";
				break;
			case T_NUMBER:
			case T_INTEGER:
			case T_UINTEGER:
			{
				tiny_string s = this->toString();
				if (s == "Infinity" || s == "-Infinity" || s == "NaN")
					res += "null";
				else
					res += s;
				break;
			}
			default:
				res += this->toString();
				break;
		}
	}
	else
	{
		res += "{";
		
		// 
		std::vector<uint32_t> tmp;
		variables_map::var_iterator beginIt = Variables.Variables.begin();
		variables_map::var_iterator endIt = Variables.Variables.end();
		variables_map::var_iterator varIt = beginIt;
		while (varIt != endIt)
		{
			tmp.push_back(varIt->first);
			varIt++;
		}
		std::sort(tmp.begin(),tmp.end());
		bool bfirst = true;
		bool bObjectVars = true;
		path.push_back(this);
		auto tmpIt = tmp.begin();
		while (tmpIt != tmp.end())
		{
			varIt = bObjectVars ? Variables.Variables.find(*tmpIt) : this->getClass()->borrowedVariables.Variables.find(*tmpIt);
			tmpIt++;
			if (tmpIt == tmp.end() && bObjectVars)
			{
				bObjectVars = false;
				if (this->getClass())
				{
					variables_map::var_iterator varIt2 = this->getClass()->borrowedVariables.Variables.begin();
					while (varIt2 != this->getClass()->borrowedVariables.Variables.end())
					{
						tmp.push_back(varIt2->first);
						varIt2++;
					}
					std::sort(tmp.begin(),tmp.end());
					tmpIt = tmp.begin();
				}
			}
			if (varIt == endIt)
				continue;
			if(varIt->second.ns.hasEmptyName() && (asAtomHandler::isValid(varIt->second.getter) || asAtomHandler::isValid(varIt->second.var)))
			{
				ASObject* v = asAtomHandler::isValid(varIt->second.var) ? asAtomHandler::toObject(varIt->second.var,getSystemState()) : nullptr;
				if (asAtomHandler::isValid(varIt->second.getter))
				{
					asAtom t=asAtomHandler::fromObject(this);
					asAtom res=asAtomHandler::invalidAtom;
					asAtomHandler::callFunction(varIt->second.getter,res,t,NULL,0,false);
					v=asAtomHandler::toObject(res,getSystemState());
				}
				if(v && v->getObjectType() != T_UNDEFINED && varIt->second.isenumerable)
				{
					// check for cylic reference
					if (v->getObjectType() != T_UNDEFINED &&
						v->getObjectType() != T_NULL &&
						v->getObjectType() != T_BOOLEAN &&
						std::find(path.begin(),path.end(), v) != path.end())
						throwError<TypeError>(kJSONCyclicStructure);
		
					if (asAtomHandler::isValid(replacer))
					{
						if (!bfirst)
							res += ",";
						res += newline+spaces;
						res += "\"";
						res += getSystemState()->getStringFromUniqueId(varIt->first);
						res += "\"";
						res += ":";
						if (!spaces.empty())
							res += " ";
						asAtom params[2];
						
						params[0] = asAtomHandler::fromStringID(varIt->first);
						params[1] = asAtomHandler::fromObject(v);
						ASATOM_INCREF(params[1]);
						asAtom funcret=asAtomHandler::invalidAtom;
						asAtomHandler::callFunction(replacer,funcret,asAtomHandler::nullAtom, params, 2,true);
						if (asAtomHandler::isValid(funcret))
							res += asAtomHandler::toString(funcret,getSystemState());
						else
							res += v->toJSON(path,replacer,spaces+spaces,filter);
						bfirst = false;
					}
					else if (filter.empty() || filter.find(tiny_string(" ")+getSystemState()->getStringFromUniqueId(varIt->first)+" ") != tiny_string::npos)
					{
						if (!bfirst)
							res += ",";
						res += newline+spaces;
						res += "\"";
						res += getSystemState()->getStringFromUniqueId(varIt->first);
						res += "\"";
						res += ":";
						if (!spaces.empty())
							res += " ";
						res += v->toJSON(path,replacer,spaces+spaces,filter);
						bfirst = false;
					}
				}
				if (!bfirst)
					res += newline+spaces.substr_bytes(0,spaces.numBytes()/2);
			}
		}
		res += "}";
		path.pop_back();
	}
	return res;
}

bool ASObject::hasprop_prototype()
{
	variable* var=Variables.findObjVar(BUILTIN_STRINGS::PROTOTYPE,nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),
			NO_CREATE_TRAIT,(DECLARED_TRAIT|DYNAMIC_TRAIT));
	return (var && asAtomHandler::isValid(var->var));
}

ASObject* ASObject::getprop_prototype()
{
	variable* var=Variables.findObjVar(BUILTIN_STRINGS::PROTOTYPE,nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),
			NO_CREATE_TRAIT,(DECLARED_TRAIT|DYNAMIC_TRAIT));
	if (var)
		return asAtomHandler::toObject(var->var,getSystemState());
	if (!getSystemState()->mainClip->usesActionScript3)
		var=Variables.findObjVar(BUILTIN_STRINGS::STRING_PROTO,nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),
			NO_CREATE_TRAIT,(DECLARED_TRAIT|DYNAMIC_TRAIT));
	return var ? asAtomHandler::toObject(var->var,getSystemState()) : nullptr;
}

/*
 * (creates and) sets the property 'prototype' to o
 * 'prototype' is usually DYNAMIC_TRAIT, but on Class_base
 * it is a DECLARED_TRAIT, which is gettable only
 */
void ASObject::setprop_prototype(_NR<ASObject>& prototype,uint32_t nameID)
{
	ASObject* obj = prototype.getPtr();

	multiname prototypeName(nullptr);
	prototypeName.name_type=multiname::NAME_STRING;
	prototypeName.name_s_id=nameID;
	prototypeName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	bool has_getter = false;
	variable* ret=findSettable(prototypeName,&has_getter);
	if(!ret && has_getter)
		throwError<ReferenceError>(kConstWriteError,
					   prototypeName.normalizedNameUnresolved(getSystemState()),
					   classdef ? classdef->as<Class_base>()->getQualifiedClassName() : "");
	if(!ret)
	{
		ret = Variables.findObjVar(getSystemState(),prototypeName,DYNAMIC_TRAIT,DECLARED_TRAIT|DYNAMIC_TRAIT);
	}
	ret->isenumerable=false;
	if(asAtomHandler::isValid(ret->setter))
	{
		asAtom arg1= asAtomHandler::fromObject(obj);
		asAtom v=asAtomHandler::fromObject(this);
		asAtom res=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(ret->setter,res,v,&arg1,1,false);
	}
	else
	{
		obj->incRef();
		asAtom v = asAtomHandler::fromObject(obj);
		ret->setVar(v,this);
	}
}

tiny_string ASObject::getClassName() const
{
	if (getClass())
		return getClass()->getQualifiedClassName();
	else
		return "";
}

asAtom asAtomHandler::undefinedAtom = asAtomHandler::fromType(T_UNDEFINED);
asAtom asAtomHandler::nullAtom = asAtomHandler::fromType(T_NULL);
asAtom asAtomHandler::invalidAtom = asAtomHandler::fromType(T_INVALID);
asAtom asAtomHandler::trueAtom = asAtomHandler::fromBool(true);
asAtom asAtomHandler::falseAtom = asAtomHandler::fromBool(false);

void asAtomHandler::decRef(asAtom &a)
{
	if (getObject(a))
		getObject(a)->decRef();
}

bool asAtomHandler::stringcompare(asAtom& a, SystemState* sys,uint32_t stringID)
{
	return toObject(a,sys)->toString() == sys->getStringFromUniqueId(stringID);
}
bool asAtomHandler::functioncompare(asAtom& a,SystemState* sys,asAtom& v2)
{
	if (getObject(a) && getObject(a)->as<IFunction>()->closure_this 
			&& getObject(v2) && getObject(v2)->as<IFunction>()->closure_this
			&& getObject(a)->as<IFunction>()->closure_this != getObject(v2)->as<IFunction>()->closure_this)
		return false;
	return toObject(v2,sys)->isEqual(toObject(a,sys));
}
ASObject* asAtomHandler::getClosure(asAtom& a)
{
	return (getObject(a) && getObject(a)->is<IFunction>()) ? getObject(a)->as<IFunction>()->closure_this.getPtr() : nullptr;
}
asAtom asAtomHandler::getClosureAtom(asAtom& a, asAtom defaultAtom)
{
	ASObject* o = getClosure(a);
	return o ? fromObject(o) : defaultAtom;
}
asAtom asAtomHandler::fromString(SystemState* sys, const tiny_string& s)
{
	asAtom a=asAtomHandler::invalidAtom;
	a.uintval = (sys->getUniqueStringId(s)<<3) | ATOM_STRINGID;
	return a;
}

void asAtomHandler::callFunction(asAtom& caller,asAtom& ret,asAtom &obj, asAtom *args, uint32_t num_args, bool args_refcounted, bool coerceresult, bool coercearguments)
{
	assert_and_throw(asAtomHandler::isFunction(caller));

	asAtom c = obj;
	if (getObjectNoCheck(caller)->is<SyntheticFunction>())
	{
		if (!args_refcounted)
		{
			for (uint32_t i = 0; i < num_args; i++)
				ASATOM_INCREF(args[i]);
		}
		getObjectNoCheck(caller)->as<SyntheticFunction>()->call(ret,c, args, num_args,coerceresult,coercearguments);
		if (args_refcounted)
			ASATOM_DECREF(c);
		return;
	}
	// when calling builtin functions, normally no refcounting is needed
	// if it is, it has to be done inside the called function
	getObjectNoCheck(caller)->as<Function>()->call(ret, c, args, num_args);
	if (args_refcounted)
	{
		for (uint32_t i = 0; i < num_args; i++)
			ASATOM_DECREF(args[i]);
		ASATOM_DECREF(c);
	}
}

void asAtomHandler::getVariableByMultiname(asAtom& a, asAtom& ret,SystemState* sys, const multiname &name)
{
	// classes for primitives are final and sealed, so we only have to check the class for the variable
	// no need to create ASObjects for the primitives
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			Class<Integer>::getClass(sys)->getClassVariableByMultiname(ret,name);
			return;
		case ATOM_UINTEGER:
			Class<UInteger>::getClass(sys)->getClassVariableByMultiname(ret,name);
			return;
		case ATOM_U_INTEGERPTR:
		case ATOM_NUMBERPTR:
			Class<Number>::getClass(sys)->getClassVariableByMultiname(ret,name);
			return;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_BOOL_BIT:
					Class<Boolean>::getClass(sys)->getClassVariableByMultiname(ret,name);
					return;
				default:
					return;
			}
		}
		case ATOM_STRINGID:
			Class<ASString>::getClass(sys)->getClassVariableByMultiname(ret,name);
			return;
		default:
			return;
	}
}
Class_base *asAtomHandler::getClass(asAtom& a,SystemState* sys,bool followclass)
{
	// classes for primitives are final and sealed, so we only have to check the class for the variable
	// no need to create ASObjects for the primitives
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return Class<Integer>::getRef(sys).getPtr()->as<Class_base>();
		case ATOM_UINTEGER:
			return Class<UInteger>::getRef(sys).getPtr()->as<Class_base>();
		case ATOM_U_INTEGERPTR:
		case ATOM_NUMBERPTR:
			return Class<Number>::getRef(sys).getPtr()->as<Class_base>();
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_BOOL_BIT:
					return Class<Boolean>::getRef(sys).getPtr()->as<Class_base>();
				default:
					return nullptr;
			}
		}
		case ATOM_STRINGID:
			return Class<ASString>::getRef(sys).getPtr()->as<Class_base>();
		default:
			return getObject(a) ? (followclass && getObject(a)->is<Class_base>() ? getObject(a)->as<Class_base>() : getObject(a)->getClass()) : nullptr;
	}
	return nullptr;
}

bool asAtomHandler::canCacheMethod(asAtom& a,const multiname* name)
{
	assert(name->isStatic);
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
		case ATOM_UINTEGER:
		case ATOM_NUMBERPTR:
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		case ATOM_STRINGID:
			return true;
		case ATOM_OBJECTPTR:
		{
			if (!getObject(a))
				return false;
			switch (getObject(a)->getObjectType())
			{
				case T_FUNCTION:
					return true;
				case T_CLASS:
					return getObject(a)->as<Class_base>()->isSealed;
				case T_ARRAY:
					// Array class is dynamic, but if it is not inherited, no methods are overwritten and we can cache them
					return (getObject(a)->getClass()->isSealed || !getObject(a)->getClass()->is<Class_inherit>());
				default:
					return getObject(a)->getClass()->isSealed;
			}
		}
		default:
			break;
	}
	return false;
}

void asAtomHandler::fillMultiname(asAtom& a,SystemState* sys, multiname &name)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			name.name_type = multiname::NAME_INT;
			name.name_i = a.intval>>3;
			break;
		case ATOM_UINTEGER:
			name.name_type = multiname::NAME_UINT;
			name.name_ui = a.uintval>>3;
			break;
		case ATOM_NUMBERPTR:
			name.name_type = multiname::NAME_NUMBER;
			name.name_d = toNumber(a);
			break;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			name.name_type = multiname::NAME_INT;
			name.name_i = toInt(a);
			break;
		case ATOM_STRINGID:
			name.name_type = multiname::NAME_STRING;
			name.name_s_id = a.uintval>>3;
			break;
		default:
			name.name_type=multiname::NAME_OBJECT;
			name.name_o=toObject(a,sys);
			break;
	}
}

void asAtomHandler::replaceBool(asAtom& a, ASObject *obj)
{
	a.uintval = obj->as<Boolean>()->val ? 0x100 : 0;
}

std::string asAtomHandler::toDebugString(asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return Integer::toString(a.intval>>3)+"i";
		case ATOM_UINTEGER:
			return UInteger::toString(a.uintval>>3)+"ui";
		case ATOM_NUMBERPTR:
		{
			std::string ret = Number::toString(toNumber(a))+"d";
#ifndef NDEBUG
			assert(getObject(a));
			char buf[300];
			sprintf(buf,"(%p / %d/%d)",getObject(a),getObject(a)->getRefCount(),getObject(a)->getConstant());
			ret += buf;
#endif
			return ret;
		}
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					return "Null";
				case ATOMTYPE_UNDEFINED_BIT:
					return "Undefined";
				case ATOMTYPE_BOOL_BIT:
					return Integer::toString((a.uintval&0x80)>>7)+"b";
				default:
					return "Invalid";
			}
		}
		case ATOM_STRINGID:
			return getSys()->getStringFromUniqueId(a.uintval>>3);
		default:
			assert(getObject(a));
			return getObject(a)->toDebugString();
	}
}

asAtom asAtomHandler::asTypelate(asAtom& a, asAtom& b)
{
	LOG_CALL(_("asTypelate"));

	if(!isObject(b) || !is<Class_base>(b))
	{
		LOG(LOG_ERROR,"trying to call asTypelate on non class object:"<<toDebugString(a));
		throwError<TypeError>(kConvertNullToObjectError);
	}
	Class_base* c=static_cast<Class_base*>(getObject(b));
	//Special case numeric types
	if(isNumeric(a))
	{
		bool real_ret;
		if(c==Class<Number>::getClass(c->getSystemState()) || c==c->getSystemState()->getObjectClassRef())
			real_ret=true;
		else if(c==Class<Integer>::getClass(c->getSystemState()))
			real_ret=(toNumber(a)==toInt(a));
		else if(c==Class<UInteger>::getClass(c->getSystemState()))
			real_ret=(toNumber(a)==toUInt(a));
		else
			real_ret=false;
		LOG_CALL(_("Numeric type is ") << ((real_ret)?"":_("not ")) << _("subclass of ") << c->class_name);
		ASATOM_DECREF(b);
		if(real_ret)
			return a;
		else
			return asAtomHandler::nullAtom;
	}

	Class_base* objc = toObject(a,c->getSystemState())->getClass();
	if(!objc)
	{
		ASATOM_DECREF(b);
		return asAtomHandler::nullAtom;
	}

	bool real_ret=objc->isSubClass(c);
	LOG_CALL(_("Type ") << objc->class_name << _(" is ") << ((real_ret)?_(" "):_("not ")) 
			<< _("subclass of ") << c->class_name);
	ASATOM_DECREF(b);
	if(real_ret)
		return a;
	else
		return asAtomHandler::nullAtom;
	
}

bool asAtomHandler::isTypelate(asAtom& a,ASObject *type)
{
	LOG_CALL(_("isTypelate"));
	bool real_ret=false;

	Class_base* objc=nullptr;
	Class_base* c=nullptr;
	switch (type->getObjectType())
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_OBJECT:
		case T_STRING:
			LOG(LOG_ERROR,"trying to call isTypelate on object:"<<toDebugString(a));
			throwError<TypeError>(kIsTypeMustBeClassError);
			break;
		case T_NULL:
			LOG(LOG_ERROR,"trying to call isTypelate on null:"<<toDebugString(a));
			throwError<TypeError>(kConvertNullToObjectError);
			break;
		case T_UNDEFINED:
			LOG(LOG_ERROR,"trying to call isTypelate on undefined:"<<toDebugString(a));
			throwError<TypeError>(kConvertUndefinedToObjectError);
			break;
		case T_CLASS:
			break;
		default:
			throwError<TypeError>(kIsTypeMustBeClassError);
	}

	c=static_cast<Class_base*>(type);
	//Special case numeric types
	if(isNumeric(a))
	{
		if(c==Class<Number>::getClass(c->getSystemState()) || c==c->getSystemState()->getObjectClassRef())
			real_ret=true;
		else if(c==Class<Integer>::getClass(c->getSystemState()))
			real_ret=(toNumber(a)==toInt(a));
		else if(c==Class<UInteger>::getClass(c->getSystemState()))
			real_ret=(toNumber(a)==toUInt(a));
		else
			real_ret=false;
		LOG_CALL(_("Numeric type is ") << ((real_ret)?"":_("not ")) << _("subclass of ") << c->class_name);
		type->decRef();
		return real_ret;
	}

	objc = getClass(a,c->getSystemState(),false);
	if(!objc)
	{
		real_ret=getObjectType(a)==type->getObjectType();
		LOG_CALL(_("isTypelate on non classed object ") << real_ret);
		type->decRef();
		return real_ret;
	}

	real_ret=objc->isSubClass(c);
	LOG_CALL(_("Type ") << objc->class_name << _(" is ") << ((real_ret)?"":_("not ")) 
			<< "subclass of " << c->class_name);
	type->decRef();
	return real_ret;
}

bool asAtomHandler::isTypelate(asAtom& a,asAtom& t)
{
	LOG_CALL(_("isTypelate"));
	bool real_ret=false;

	Class_base* objc=nullptr;
	Class_base* c=nullptr;
	switch (asAtomHandler::getObjectType(t))
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_OBJECT:
		case T_STRING:
			LOG(LOG_ERROR,"trying to call isTypelate on object:"<<toDebugString(a));
			throwError<TypeError>(kIsTypeMustBeClassError);
			break;
		case T_NULL:
			LOG(LOG_ERROR,"trying to call isTypelate on null:"<<toDebugString(a));
			throwError<TypeError>(kConvertNullToObjectError);
			break;
		case T_UNDEFINED:
			LOG(LOG_ERROR,"trying to call isTypelate on undefined:"<<toDebugString(a));
			throwError<TypeError>(kConvertUndefinedToObjectError);
			break;
		case T_CLASS:
			break;
		default:
			throwError<TypeError>(kIsTypeMustBeClassError);
	}

	c=asAtomHandler::as<Class_base>(t);
	//Special case numeric types
	if(isNumeric(a))
	{
		if(c==Class<Number>::getClass(c->getSystemState()) || c==c->getSystemState()->getObjectClassRef())
			real_ret=true;
		else if(c==Class<Integer>::getClass(c->getSystemState()))
			real_ret=(toNumber(a)==toInt(a));
		else if(c==Class<UInteger>::getClass(c->getSystemState()))
			real_ret=(toNumber(a)==toUInt(a));
		else
			real_ret=false;
		LOG_CALL(_("Numeric type is ") << ((real_ret)?"":_("not ")) << _("subclass of ") << c->class_name);
		c->decRef();
		return real_ret;
	}

	objc = getClass(a,c->getSystemState(),false);
	if(!objc)
	{
		real_ret=getObjectType(a)==asAtomHandler::getObjectType(t);
		LOG_CALL(_("isTypelate on non classed object ") << real_ret);
		c->decRef();
		return real_ret;
	}

	real_ret=objc->isSubClass(c);
	LOG_CALL(_("Type ") << objc->class_name << _(" is ") << ((real_ret)?"":_("not ")) 
			<< "subclass of " << c->class_name);
	c->decRef();
	return real_ret;
}

tiny_string asAtomHandler::toString(const asAtom& a,SystemState* sys)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					return "null";
				case ATOMTYPE_UNDEFINED_BIT:
					return "undefined";
				case ATOMTYPE_BOOL_BIT:
					return a.uintval&0x80 ? "true" : "false";
				default:
					return "";
			}
		}
		case ATOM_NUMBERPTR:
			return Number::toString(toNumber(a));
		case ATOM_INTEGER:
			return Integer::toString(a.intval>>3);
		case ATOM_UINTEGER:
			return UInteger::toString(a.uintval>>3);
		case ATOM_STRINGID:
			if ((a.uintval>>3) == 0)
				return "";
			if ((a.uintval>>3) < BUILTIN_STRINGS_CHAR_MAX)
				return tiny_string::fromChar(a.uintval>>3);
			return sys->getStringFromUniqueId(a.uintval>>3);
		default:
			assert(getObject(a));
			return getObject(a)->toString();
	}
}
tiny_string asAtomHandler::toLocaleString(const asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					return "null";
				case ATOMTYPE_UNDEFINED_BIT:
					return "undefined";
				case ATOMTYPE_BOOL_BIT:
					return "[object Boolean]";
				default:
					return "";
			}
		}
		case ATOM_NUMBERPTR:
			return Number::toString(toNumber(a));
		case ATOM_INTEGER:
			return Integer::toString(a.intval>>3);
		case ATOM_UINTEGER:
			return UInteger::toString(a.uintval>>3);
		case ATOM_STRINGID:
		{
			ASString* s = (ASString*)abstract_s(getSys(),a.uintval>>3);
			tiny_string res = s->toLocaleString();
			delete s;
			return res;
		}
		default:
			assert(getObject(a));
			return getObject(a)->toLocaleString();
	}
}

uint32_t asAtomHandler::toStringId(asAtom& a,SystemState* sys)
{
	if (isStringID(a))
		return a.uintval>>3;
	return toObject(a,sys)->toStringId();
}


bool asAtomHandler::Boolean_concrete_string(asAtom &a)
{
	return getObject(a) && !getObject(a)->as<ASString>()->isEmpty();
}

void asAtomHandler::convert_b(asAtom& a, bool refcounted)
{
	bool v = false;
	switch(a.uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
				case ATOMTYPE_UNDEFINED_BIT:
					v= false;
					break;
				case ATOMTYPE_BOOL_BIT:
					return;
				default:
					return;
			}
			break;
		}
		case ATOM_INTEGER:
			v= a.intval>>3 != 0;
			break;
		case ATOM_UINTEGER:
			v= a.uintval>>3 != 0;
			break;
		case ATOM_STRINGID:
			v = a.uintval>>3 != BUILTIN_STRINGS::EMPTY;
			break;
		default:
			v= lightspark::Boolean_concrete(getObject(a));
			break;
	}
	if (refcounted)
		decRef(a);
	setBool(a,v);
}

bool asAtomHandler::add(asAtom& a,asAtom &v2, SystemState* sys,bool forceint)
{
	//Implement ECMA add algorithm, for XML and default (see avm2overview)

	// if both values are Integers or UIntegers the result is also an int Number
	if( (isInteger(a) || isUInteger(a)) &&
		(isInteger(v2) || isUInteger(v2)))
	{
		int64_t num1=toInt64(a);
		int64_t num2=toInt64(v2);
		int64_t res = num1+num2;
		LOG_CALL("addI " << num1 << '+' << num2 <<"="<<res);
		if (forceint || (res >= -(1<<28) && res < (1<<28)))
			setInt(a,sys,res);
		else if (res >= 0 && res < (1<<29))
			setUInt(a,sys,res);
		else
			setNumber(a,sys,res);
	}
	else if((isInteger(a) || isUInteger(a) || isNumber(a)) &&
			(isNumber(v2) || isInteger(v2) || isUInteger(v2)))
	{
		double num1=toNumber(a);
		double num2=toNumber(v2);
		LOG_CALL("addN " << num1 << '+' << num2<<" "<<toDebugString(a)<<" "<<toDebugString(v2));
		if (forceint)
			setInt(a,sys,num1+num2);
		else
			setNumber(a,sys,num1+num2);
	}
	else if(isString(a) || isString(v2))
	{
		tiny_string sa = toString(a,sys);
		sa += toString(v2,sys);
		LOG_CALL("add " << toString(a,sys) << '+' << toString(v2,sys));
		if (forceint)
			setInt(a,sys,Integer::stringToASInteger(sa.raw_buf(),0));
		else
			a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_s(sys,sa))|ATOM_STRINGPTR;
	}
	else
	{
		ASObject* val1 = toObject(a,sys);
		ASObject* val2 = toObject(v2,sys);
		if( (val1->is<XML>() || val1->is<XMLList>()) && (val2->is<XML>() || val2->is<XMLList>()) )
		{
			//Check if the objects are both XML or XMLLists
			Class_base* xmlClass=Class<XML>::getClass(val1->getSystemState());

			XMLList* newList=Class<XMLList>::getInstanceS(val1->getSystemState(),true);
			val1->incRef();
			if(val1->getClass()==xmlClass)
				newList->append(_MR(static_cast<XML*>(val1)));
			else //if(val1->getClass()==xmlListClass)
				newList->append(_MR(static_cast<XMLList*>(val1)));

			val2->incRef();
			if(val2->getClass()==xmlClass)
				newList->append(_MR(static_cast<XML*>(val2)));
			else //if(val2->getClass()==xmlListClass)
				newList->append(_MR(static_cast<XMLList*>(val2)));

			if (forceint)
			{
				setInt(a,sys,newList->toInt());
				newList->decRef();
			}
			else
				a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(newList)|ATOM_OBJECTPTR;
			//The references of val1 and val2 have been passed to the smart references
			//no decRef is needed
			return false;
		}
		else
		{//If none of the above apply, convert both to primitives with no hint
			asAtom val1p=asAtomHandler::invalidAtom;
			val1->toPrimitive(val1p,NO_HINT);
			asAtom val2p=asAtomHandler::invalidAtom;
			val2->toPrimitive(val2p,NO_HINT);
			if(is<ASString>(val1p) || is<ASString>(val2p))
			{//If one is String, convert both to strings and concat
				string as(toString(val1p,sys).raw_buf());
				string bs(toString(val2p,sys).raw_buf());
				LOG_CALL("add " << as << '+' << bs);
				if (forceint)
				{
					tiny_string s = as+bs;
					setInt(a,sys,Integer::stringToASInteger(s.raw_buf(),0));
				}
				else
					a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_s(sys,as+bs))|ATOM_STRINGPTR;
			}
			else
			{//Convert both to numbers and add
				number_t num1=AVM1toNumber(val1p,sys->getSwfVersion());
				number_t num2=AVM1toNumber(val2p,sys->getSwfVersion());
				LOG_CALL("addN " << num1 << '+' << num2);
				number_t result = num1 + num2;
				if (forceint)
					setInt(a,sys,result);
				else
					setNumber(a,sys,result);
			}
		}
	}
	return true;
}
void asAtomHandler::addreplace(asAtom& ret, SystemState* sys,asAtom& v1, asAtom &v2,bool forceint)
{
	//Implement ECMA add algorithm, for XML and default (see avm2overview)

	// if both values are Integers or UIntegers the result is also an int Number
	if( (isInteger(v1) || isUInteger(v1)) &&
		(isInteger(v2) || isUInteger(v2)))
	{
		int64_t num1=toInt64(v1);
		int64_t num2=toInt64(v2);
		int64_t res = num1+num2;
		LOG_CALL("addI " << num1 << '+' << num2 <<"="<<res);
		if (forceint || (res >= -(1<<28) && res < (1<<28)))
			setInt(ret,sys,res);
		else if (res >= 0 && res < (1<<29))
			setUInt(ret,sys,res);
		else
			setNumber(ret,sys,res);
	}
	else if((isInteger(v1) || isUInteger(v1) || isNumber(v1)) &&
			(isNumber(v2) || isInteger(v2) || isUInteger(v2)))
	{
		double num1=toNumber(v1);
		double num2=toNumber(v2);
		LOG_CALL("addN " << num1 << '+' << num2<<" "<<toDebugString(v1)<<" "<<toDebugString(v2));
		ASObject* o = getObject(ret);
		if (forceint)
		{
			setInt(ret,sys,num1+num2);
			if (o)
				o->decRef();
		}
		else if (replaceNumber(ret,sys,num1+num2) && o)
			o->decRef();
	}
	else if(isString(v1) || isString(v2))
	{
		tiny_string sa = toString(v1,sys);
		sa += toString(v2,sys);
		LOG_CALL("add " << toString(v1,sys) << '+' << toString(v2,sys));
		ASATOM_DECREF(ret);
		if (forceint)
			setInt(ret,sys,Integer::stringToASInteger(sa.raw_buf(),0));
		else
			ret.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_s(sys,sa))|ATOM_STRINGPTR;
	}
	else
	{
		ASObject* val1 = toObject(v1,sys);
		ASObject* val2 = toObject(v2,sys);
		if( (val1->is<XML>() || val1->is<XMLList>()) && (val2->is<XML>() || val2->is<XMLList>()) )
		{
			//Check if the objects are both XML or XMLLists
			Class_base* xmlClass=Class<XML>::getClass(val1->getSystemState());

			XMLList* newList=Class<XMLList>::getInstanceS(val1->getSystemState(),true);
			val1->incRef();
			if(val1->getClass()==xmlClass)
				newList->append(_MR(static_cast<XML*>(val1)));
			else //if(val1->getClass()==xmlListClass)
				newList->append(_MR(static_cast<XMLList*>(val1)));

			val2->incRef();
			if(val2->getClass()==xmlClass)
				newList->append(_MR(static_cast<XML*>(val2)));
			else //if(val2->getClass()==xmlListClass)
				newList->append(_MR(static_cast<XMLList*>(val2)));

			if (forceint)
			{
				setInt(ret,sys,newList->toInt());
				newList->decRef();
			}
			else
				ret.uintval = (LIGHTSPARK_ATOM_VALTYPE)(newList)|ATOM_OBJECTPTR;
			//The references of val1 and val2 have been passed to the smart references
			//no decRef is needed
		}
		else
		{//If none of the above apply, convert both to primitives with no hint
			asAtom val1p=asAtomHandler::invalidAtom;
			val1->toPrimitive(val1p,NO_HINT);
			asAtom val2p=asAtomHandler::invalidAtom;
			val2->toPrimitive(val2p,NO_HINT);
			if(is<ASString>(val1p) || is<ASString>(val2p))
			{//If one is String, convert both to strings and concat
				string as(toString(val1p,sys).raw_buf());
				string bs(toString(val2p,sys).raw_buf());
				LOG_CALL("add " << as << '+' << bs);
				ASATOM_DECREF(ret);
				if (forceint)
				{
					tiny_string s = as+bs;
					setInt(ret,sys,Integer::stringToASInteger(s.raw_buf(),0));
				}
				else
					ret.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_s(sys,as+bs))|ATOM_STRINGPTR;
			}
			else
			{//Convert both to numbers and add
				number_t num1=AVM1toNumber(val1p,sys->getSwfVersion());
				number_t num2=AVM1toNumber(val2p,sys->getSwfVersion());
				LOG_CALL("addN " << num1 << '+' << num2);
				ASObject* o = getObject(ret);
				if (forceint)
				{
					setInt(ret,sys,num1+num2);
					if (o)
						o->decRef();
				}
				else if (replaceNumber(ret,sys,num1+num2) && o)
					o->decRef();
			}
		}
	}
}
void asAtomHandler::setFunction(asAtom& a,ASObject *obj, ASObject *closure)
{
	// type may be T_CLASS or T_FUNCTION
	if (closure &&obj->is<IFunction>())
	{
		closure->incRef();
		a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(obj->as<IFunction>()->bind(_MR(closure)))|ATOM_OBJECTPTR;
	}
	else
	{
		a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(obj)|ATOM_OBJECTPTR;
	}
}
bool asAtomHandler::Boolean_concrete_object(asAtom& a)
{
	assert(getObject(a));
	return lightspark::Boolean_concrete(getObject(a));
}

void asAtomHandler::setNumber(asAtom& a, SystemState* sys, number_t val)
{
	if (std::isnan(val))
		a.uintval = sys->nanAtom.uintval;
	else
		a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_d(sys,val))|ATOM_NUMBERPTR;
}
bool asAtomHandler::replaceNumber(asAtom& a, SystemState* sys, number_t val)
{
	if (isNumber(a) && getObject(a)->isLastRef())
	{
		as<Number>(a)->setNumber(val);
		return false;
	}
	if (std::isnan(val))
		a.uintval = sys->nanAtom.uintval;
	else
		a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_d(sys,val))|ATOM_NUMBERPTR;
	return true;
}

void asAtomHandler::replace(asAtom& a, ASObject *obj)
{
	assert(((LIGHTSPARK_ATOM_VALTYPE)obj) % 8 == 0);
	switch(obj->getObjectType())
	{
		case T_INVALID:
			a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL;
			break;
		case T_UNDEFINED:
			a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_UNDEFINED_BIT;
			break;
		case T_NULL:
			a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_NULL_BIT;
			break;
		case T_BOOLEAN:
			a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_BOOL_BIT | (obj->toInt() ? 0x80: 0);
			break;
		case T_NUMBER:
			a.uintval=ATOM_NUMBERPTR | (LIGHTSPARK_ATOM_VALTYPE)obj;
			break;
		case T_UINTEGER:
		case T_INTEGER:
			a.uintval=ATOM_U_INTEGERPTR | (LIGHTSPARK_ATOM_VALTYPE)obj;
			break;
		case T_STRING:
			a.uintval=ATOM_STRINGPTR | (LIGHTSPARK_ATOM_VALTYPE)obj;
			break;
		default:
			a.uintval=ATOM_OBJECTPTR | (LIGHTSPARK_ATOM_VALTYPE)obj;
			break;
	}
}

TRISTATE asAtomHandler::isLessIntern(asAtom& a,SystemState *sys, asAtom &v2)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return (a.intval < v2.intval)?TTRUE:TFALSE;
				case ATOM_UINTEGER:
					return ((a.intval>>3) < 0 || ((uint32_t)(a.intval>>3)) < (v2.uintval>>3))?TTRUE:TFALSE;
				case ATOM_NUMBERPTR:
					if(std::isnan(toNumber(v2)))
						return TUNDEFINED;
					return ((a.intval>>3) < toNumber(v2))?TTRUE:TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return ((a.intval>>3) < 0)?TTRUE:TFALSE;
						case ATOMTYPE_UNDEFINED_BIT:
							return TUNDEFINED;
						case ATOMTYPE_BOOL_BIT:
							return ((a.intval>>3) < (int32_t)((v2.uintval&0x80)>>7))?TTRUE:TFALSE;
						default: // INVALID
							return TUNDEFINED;
					}
				}
				default:
					return ((a.intval>>3) < toInt(v2))?TTRUE:TFALSE;
			}
			break;
		}
		case ATOM_UINTEGER:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return ((v2.intval>>3) > 0 && ((a.uintval>>3) < (uint32_t)(v2.intval>>3)))?TTRUE:TFALSE;
				case ATOM_UINTEGER:
					return ((a.uintval>>3) < (v2.uintval>>3))?TTRUE:TFALSE;
				case ATOM_NUMBERPTR:
					if(std::isnan(toNumber(v2)))
						return TUNDEFINED;
					return ((a.uintval>>3) < toNumber(v2))?TTRUE:TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return TFALSE;
						case ATOMTYPE_UNDEFINED_BIT:
							return TUNDEFINED;
						case ATOMTYPE_BOOL_BIT:
							return ((a.uintval>>3) < ((v2.uintval&0x80)>>7))?TTRUE:TFALSE;
						default: // INVALID
							return TUNDEFINED;
					}
				}
				default:
					return ((a.uintval>>3) < toUInt(v2))?TTRUE:TFALSE;
			}
			break;
		}
		case ATOM_NUMBERPTR:
		{
			if(std::isnan(toNumber(a)))
				return TUNDEFINED;
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return (toNumber(a) < (v2.intval>>3))?TTRUE:TFALSE;
				case ATOM_UINTEGER:
					return (toNumber(a) < (v2.uintval>>3))?TTRUE:TFALSE;
				case ATOM_NUMBERPTR:
					if(std::isnan(toNumber(v2)))
						return TUNDEFINED;
					return (toNumber(a) < toNumber(v2))?TTRUE:TFALSE;
				case ATOM_STRINGID:
					return (toNumber(a) < toNumber(v2))?TTRUE:TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return (toNumber(a) < 0)?TTRUE:TFALSE;
						case ATOMTYPE_UNDEFINED_BIT:
							return TUNDEFINED;
						case ATOMTYPE_BOOL_BIT:
							return (toNumber(a) < toInt(v2))?TTRUE:TFALSE;
						default: // INVALID
							return TUNDEFINED;
					}
				}
				default:
					break;
			}
			break;
		}
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
				{
					switch (v2.uintval&0x7)
					{
						case ATOM_INTEGER:
							return (0 < (v2.intval>>3))?TTRUE:TFALSE;
						case ATOM_UINTEGER:
							return (0 < (v2.uintval>>3))?TTRUE:TFALSE;
						case ATOM_STRINGID:
						case ATOM_NUMBERPTR:
							if(std::isnan(toNumber(v2)))
								return TUNDEFINED;
							return (0 < toNumber(v2))?TTRUE:TFALSE;
						case ATOM_INVALID_UNDEFINED_NULL_BOOL:
						{
							switch (v2.uintval&0x70)
							{
								case ATOMTYPE_NULL_BIT:
									return TFALSE;
								case ATOMTYPE_UNDEFINED_BIT:
									return TUNDEFINED;
								case ATOMTYPE_BOOL_BIT:
									return (0 < (v2.uintval&0x80)>>7)?TTRUE:TFALSE;
								default: // INVALID
									return TUNDEFINED;
							}
						}
						default:
							return toObject(a,sys)->isLess(getObject(v2));
					}
					break;
				}
				case ATOMTYPE_UNDEFINED_BIT:
					return TUNDEFINED;
				case ATOMTYPE_BOOL_BIT:
				{
					switch (v2.uintval&0x7)
					{
						case ATOM_INTEGER:
							return ((int32_t)(a.uintval&0x80)>>7 < (v2.intval>>3))?TTRUE:TFALSE;
						case ATOM_UINTEGER:
							return ((a.uintval&0x80)>>7 < (v2.uintval>>3))?TTRUE:TFALSE;
						case ATOM_NUMBERPTR:
							if(std::isnan(toNumber(v2)))
								return TUNDEFINED;
							return ((a.uintval&0x80)>>7 < toNumber(v2))?TTRUE:TFALSE;
						case ATOM_INVALID_UNDEFINED_NULL_BOOL:
						{
							switch (v2.uintval&0x70)
							{
								case ATOMTYPE_NULL_BIT:
									return ((a.uintval&0x80)>>7)?TTRUE:TFALSE;
								case ATOMTYPE_UNDEFINED_BIT:
									return TUNDEFINED;
								case ATOMTYPE_BOOL_BIT:
									return ((a.uintval&0x80)>>7 < (v2.uintval&0x80)>>7)?TTRUE:TFALSE;
								default: // INVALID
									return TUNDEFINED;
							}
						}
						default:
							return toObject(a,sys)->isLess(getObject(v2));
					}
					break;
				}
				default: // INVALID
					return TUNDEFINED;
			}
			break;
		}
		case ATOM_STRINGID:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_STRINGID:
					if (((a.uintval>>3) < BUILTIN_STRINGS_CHAR_MAX) && ((v2.uintval>>3) < BUILTIN_STRINGS_CHAR_MAX))
						return ((a.uintval>>3) < (v2.uintval>>3))?TTRUE:TFALSE;
					return toString(a,sys) < toString(v2,sys) ? TTRUE : TFALSE;
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
				case ATOM_STRINGPTR:
					return toString(a,sys) < toString(v2,sys) ? TTRUE : TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return (toNumber(a) < 0)?TTRUE:TFALSE;
						case ATOMTYPE_UNDEFINED_BIT:
							return TUNDEFINED;
						case ATOMTYPE_BOOL_BIT:
							return (toNumber(a) < toInt(v2))?TTRUE:TFALSE;
						default: // INVALID
							return TUNDEFINED;
					}
				}
				default:
				{
					TRISTATE ret = getObject(v2)->isLessAtom(a);
					switch (ret)
					{
						case TTRUE:
							return TFALSE;
						case TFALSE:
							return isEqual(a,sys,v2) ? TFALSE : TTRUE;
						default:
							return TUNDEFINED;
					}
				}
			}
			break;
		}
		case ATOM_STRINGPTR:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
				case ATOM_STRINGID:
					return toString(a,sys) < toString(v2,sys) ? TTRUE : TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
					return getObject(a)->isLessAtom(v2);
				default:
					break;
			}
			break;
		}
		case ATOM_U_INTEGERPTR:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return (toInt(a) < (v2.intval>>3))?TTRUE:TFALSE;
				case ATOM_UINTEGER:
					return (toUInt(a) < (v2.uintval>>3))?TTRUE:TFALSE;
				case ATOM_NUMBERPTR:
					if(std::isnan(toNumber(v2)))
						return TUNDEFINED;
					return (toNumber(a) < toNumber(v2))?TTRUE:TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return (toNumber(a) < 0)?TTRUE:TFALSE;
						case ATOMTYPE_UNDEFINED_BIT:
							return TUNDEFINED;
						case ATOMTYPE_BOOL_BIT:
							return (toNumber(a) < (int32_t)((v2.uintval&0x80)>>7))?TTRUE:TFALSE;
						default: // INVALID
							return TUNDEFINED;
					}
				}
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	assert(getObject(a));
	assert(getObject(v2));
	return getObject(a)->isLess(getObject(v2));
}

bool asAtomHandler::isEqualIntern(asAtom& a, SystemState *sys, asAtom &v2)
{
	switch (a.uintval&0x7)
	{
		case ATOM_INTEGER:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return false;
				case ATOM_UINTEGER:
					return (a.intval>>3) >= 0 && (a.intval>>3)==toInt(v2);
				case ATOM_U_INTEGERPTR:
				case ATOM_NUMBERPTR:
					return (a.intval>>3)==toNumber(v2);
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return false;
						case ATOMTYPE_UNDEFINED_BIT:
							return false;
						case ATOMTYPE_BOOL_BIT:
							return (a.intval>>3)==toInt(v2);
						default: // INVALID
							return false;
					}
				}
				case ATOM_STRINGID:
				case ATOM_STRINGPTR:
					return (a.intval>>3)==toNumber(v2);
				default:
					return (a.intval>>3)==toInt(v2);
			}
			break;
		}
		case ATOM_UINTEGER:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return (v2.intval>>3) >= 0 && (a.uintval>>3)==toUInt(v2);
				case ATOM_UINTEGER:
					return false;
				case ATOM_NUMBERPTR:
				case ATOM_U_INTEGERPTR:
					return (a.uintval>>3)==toUInt(v2);
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return false;
						case ATOMTYPE_UNDEFINED_BIT:
							return false;
						case ATOMTYPE_BOOL_BIT:
							return (a.uintval>>3)==toUInt(v2);
						default: // INVALID
							return false;
					}
				}
				case ATOM_STRINGID:
				case ATOM_STRINGPTR:
					return (a.uintval>>3)==toNumber(v2);
				default:
					return (a.uintval>>3)==toUInt(v2);
			}
			break;
		}
		case ATOM_NUMBERPTR:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
					return toNumber(a)==toNumber(v2);
				case ATOM_NUMBERPTR:
					return toNumber(a) == toNumber(v2);
				case ATOM_STRINGID:
				case ATOM_STRINGPTR:
					return toNumber(a)==toNumber(v2);
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return false;
						case ATOMTYPE_UNDEFINED_BIT:
							return false;
						case ATOMTYPE_BOOL_BIT:
							return toNumber(a)==toNumber(v2);
						default: // INVALID
							return false;
					}
				}
				default:
					return toObject(v2,sys)->isEqual(toObject(a,sys));
			}
			break;
		}
		case ATOM_U_INTEGERPTR:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
				case ATOM_U_INTEGERPTR:
					return getObject(a)->toInt64()==toInt64(v2);
				case ATOM_NUMBERPTR:
					return getObject(a)->toInt64()==toNumber(v2);
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return false;
						case ATOMTYPE_UNDEFINED_BIT:
							return false;
						case ATOMTYPE_BOOL_BIT:
							return getObject(a)->toInt64()==toInt64(v2);
						default: // INVALID
							return false;
					}
				}
				case ATOM_STRINGID:
				case ATOM_STRINGPTR:
					return getObject(a)->toInt64()==toNumber(v2);
				default:
					return getObject(a)->toInt64()==toInt64(v2);
			}
			break;
		}
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
				case ATOMTYPE_UNDEFINED_BIT:
				{
					switch (v2.uintval&0x7)
					{
						case ATOM_INVALID_UNDEFINED_NULL_BOOL:
						{
							switch (v2.uintval&0x70)
							{
								case ATOMTYPE_NULL_BIT:
								case ATOMTYPE_UNDEFINED_BIT:
									return true;
								default: // BOOL
									return false;
							}
						}
						case ATOM_INTEGER:
						case ATOM_UINTEGER:
						case ATOM_NUMBERPTR:
							return false;
						case ATOM_STRINGID:
							return false;
						default:
							return toObject(v2,sys)->isEqual(toObject(a,sys));
					}
				}
				case ATOMTYPE_BOOL_BIT:
					switch (v2.uintval&0x7)
					{
						case ATOM_STRINGID:
							return (bool)((a.uintval&0x80)>>7)==toNumber(v2);
						case ATOM_INTEGER:
						case ATOM_UINTEGER:
						case ATOM_NUMBERPTR:
							return (bool)((a.uintval&0x80)>>7)==toNumber(v2);
						case ATOM_INVALID_UNDEFINED_NULL_BOOL:
						{
							switch (v2.uintval&0x70)
							{
								case ATOMTYPE_NULL_BIT:
								case ATOMTYPE_UNDEFINED_BIT:
									return false;
								case ATOMTYPE_BOOL_BIT:
									return (a.uintval&0x80)>>7==(v2.uintval&0x80)>>7;
								default: // INVALID
									return false;
							}
						}
						default:
							return toObject(v2,sys)->isEqual(toObject(a,sys));
					}
				default: // INVALID
					return false;
			}
		}
		case ATOM_STRINGID:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
						case ATOMTYPE_UNDEFINED_BIT:
							return false;
						case ATOMTYPE_BOOL_BIT:
							return (bool)((v2.uintval&0x80)>>7)==toNumber(a);
						default: // INVALID
							return false;
					}
					break;
				}
				case ATOM_STRINGID:
					return (v2.uintval>>3) == (a.uintval>>3);
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
					return isEqual(v2,sys,a);
				default:
					return isEqual(v2,sys,a);
			}
			break;
		}
		case ATOM_STRINGPTR:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
						case ATOMTYPE_UNDEFINED_BIT:
							return false;
						case ATOMTYPE_BOOL_BIT:
							return (bool)((v2.uintval&0x80)>>7)==toNumber(a);
						default: // INVALID
							return false;
					}
					break;
				}
				case ATOM_STRINGID:
					return toString(a,sys) == toString(v2,sys);
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
					return isEqual(v2,sys,a);
				default:
					break;
			}
			break;
		}
		case ATOM_OBJECTPTR:
		{
			if (is<IFunction>(a))
			{
				if (is<IFunction>(v2))
					return functioncompare(a,sys,v2);
				else
					return false;
			}
			switch (v2.uintval&0x7)
			{
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
					return getObject(a)->isEqual(toObject(v2,sys));
				case ATOM_STRINGID:
				{
					asAtom primitive=asAtomHandler::invalidAtom;
					getObject(a)->toPrimitive(primitive);
					return toString(primitive,sys) == toString(v2,sys);
				}
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
					return isEqual(v2,sys,a);
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	assert(getObject(a));
	assert(getObject(v2));
	return getObject(a)->isEqual(getObject(v2));
}

ASObject *asAtomHandler::toObject(asAtom& a, SystemState *sys, bool isconstant)
{
	if (isObject(a))
	{
		assert(getObjectNoCheck(a) && getObjectNoCheck(a)->getRefCount() >= 1);
		return getObjectNoCheck(a);
	}
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			// ints are internally treated as numbers, so create a Number instance
			a.uintval = ((LIGHTSPARK_ATOM_VALTYPE)abstract_di(sys,(a.intval>>3)))|ATOM_U_INTEGERPTR;
			break;
		case ATOM_UINTEGER:
			// uints are internally treated as numbers, so create a Number instance
			a.uintval = ((LIGHTSPARK_ATOM_VALTYPE)abstract_di(sys,(a.uintval>>3)))|ATOM_U_INTEGERPTR;
			break;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					return abstract_null(sys);
				case ATOMTYPE_UNDEFINED_BIT:
					return abstract_undefined(sys);
				case ATOMTYPE_BOOL_BIT:
					return abstract_b(sys,(a.uintval & 0x80)>>7);
				default:
					break;
			}
			break;
		}
		case ATOM_STRINGID:
			a.uintval = ((LIGHTSPARK_ATOM_VALTYPE)abstract_s(sys,(a.uintval>>3))) | ATOM_STRINGPTR ;
			break;
		default:
			throw RunTimeException("calling toObject on invalid asAtom, should not happen");
			break;
	}
	if (isconstant)
		getObjectNoCheck(a)->setRefConstant();
	return getObjectNoCheck(a);
}
