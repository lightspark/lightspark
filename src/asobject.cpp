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
				asAtom ret = asAtom::fromObject(this);
				toPrimitive(ret,STRING_HINT);
				return ret.toString(getSystemState());
		}
	}
}

tiny_string ASObject::toLocaleString()
{
	asAtom str;
	executeASMethod(str,"toLocaleString", {""}, NULL, 0);
	if (str.isInvalid())
		return "";
	return str.toString(getSystemState());
}

TRISTATE ASObject::isLess(ASObject* r)
{
	asAtom o = asAtom::fromObject(r);
	asAtom ret = asAtom::fromObject(this);
	toPrimitive(ret);
	return ret.isLess(getSystemState(),o);
}
TRISTATE ASObject::isLessAtom(asAtom& r)
{
	asAtom ret = asAtom::fromObject(this);
	toPrimitive(ret);
	return ret.isLess(getSystemState(),r);
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
		ret.setInt(getSystemState(),Integer::stringToASInteger(name.raw_buf(), 0));
	else
		ret = asAtom::fromStringID(n);
}

void ASObject::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);

	getValueAt(ret,index-1);
}

void ASObject::sinit(Class_base* c)
{
	c->setDeclaredMethodByQName("hasOwnProperty",AS3,Class<IFunction>::getFunction(c->getSystemState(),hasOwnProperty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPropertyIsEnumerable",AS3,Class<IFunction>::getFunction(c->getSystemState(),setPropertyIsEnumerable),NORMAL_METHOD,true);

	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",Class<IFunction>::getFunction(c->getSystemState(),_toLocaleString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(c->getSystemState(),valueOf),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("hasOwnProperty","",Class<IFunction>::getFunction(c->getSystemState(),hasOwnProperty),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("isPrototypeOf","",Class<IFunction>::getFunction(c->getSystemState(),isPrototypeOf),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("propertyIsEnumerable","",Class<IFunction>::getFunction(c->getSystemState(),propertyIsEnumerable),DYNAMIC_TRAIT);
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
			if (!this->isConstructed() && !this->is<Class_base>())
				return true;
			return false;
		case T_NUMBER:
		case T_INTEGER:
		case T_UINTEGER:
		case T_STRING:
		{
			asAtom primitive;
			toPrimitive(primitive);
			asAtom o = asAtom::fromObject(r);
			return primitive.isEqual(getSystemState(),o);
		}
		case T_BOOLEAN:
		{
			asAtom primitive;
			toPrimitive(primitive);
			return primitive.toNumber()==r->toNumber();
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
	asAtom ret = asAtom::fromObject(this);
	toPrimitive(ret);
	return ret.toInt();
}

int64_t ASObject::toInt64()
{
	asAtom ret = asAtom::fromObject(this);
	toPrimitive(ret);
	return ret.toInt64();
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
		this->incRef();
		ret = asAtom::fromObject(this);
		return;
	}

	/* for HINT_STRING evaluate first toString, then valueOf
	 * for HINT_NUMBER do it the other way around */
	if(hint == STRING_HINT && has_toString())
	{
		call_toString(ret);
		if(ret.isPrimitive())
			return;
	}
	if(has_valueOf())
	{
		call_valueOf(ret);
		if(ret.isPrimitive())
			return;
	}
	if(hint != STRING_HINT && has_toString())
	{
		call_toString(ret);
		if(ret.isPrimitive())
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

	asAtom o;
	getVariableByMultiname(o,valueOfName,SKIP_IMPL);
	if (!o.isFunction())
		throwError<TypeError>(kCallOfNonFunctionError, valueOfName.normalizedNameUnresolved(getSystemState()));

	asAtom v =asAtom::fromObject(this);
	ASATOM_INCREF(v);
	o.callFunction(ret,v,NULL,0,false);
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

	asAtom o;
	getVariableByMultiname(o,toStringName,SKIP_IMPL);
	if (!o.isFunction())
		throwError<TypeError>(kCallOfNonFunctionError, toStringName.normalizedNameUnresolved(getSystemState()));

	asAtom v =asAtom::fromObject(this);
	ASATOM_INCREF(v);
	o.callFunction(ret,v,NULL,0,false);
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

	asAtom o;
	getVariableByMultiname(o,toJSONName,SKIP_IMPL);
	if (!o.isFunction())
		return res;
	asAtom v=asAtom::fromObject(this);
	asAtom ret;
	o.callFunction(ret,v,NULL,0,false);
	if (ret.isString())
	{
		res += "\"";
		res += ret.toString(getSystemState());
		res += "\"";
	}
	else 
		res = ret.toObject(getSystemState())->toJSON(path,replacer,spaces,filter);
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
variables_map::variables_map(MemoryAccount *m):cloneable(true)
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

	if(varcount && Variables.findObjVar(getSystemState(),name, validTraits)!=NULL)
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
	if(isBorrowed && o->inClass == NULL)
		o->inClass = this->as<Class_base>();
	o->isStatic = !isBorrowed;

	variable* obj=NULL;
	if(isBorrowed)
	{
		assert(this->is<Class_base>());
		obj=this->as<Class_base>()->borrowedVariables.findObjVar(nameId,ns,DECLARED_TRAIT, DECLARED_TRAIT);
		if (!this->is<Class_inherit>())
			o->setConstant();
	}
	else
	{
		obj=Variables.findObjVar(nameId,ns,DECLARED_TRAIT, DECLARED_TRAIT);
		++varcount;
	}
	obj->isenumerable = isEnumerable;
	switch(type)
	{
		case NORMAL_METHOD:
		{
			asAtom v = asAtom::fromObject(o);
			obj->setVar(v,getSystemState());
			break;
		}
		case GETTER_METHOD:
		{
			ASATOM_DECREF(obj->getter);
			obj->getter=asAtom::fromObject(o);
			break;
		}
		case SETTER_METHOD:
		{
			ASATOM_DECREF(obj->setter);
			obj->setter=asAtom::fromObject(o);
			break;
		}
	}
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

	assert(f.isFunction());
	IFunction* o = f.getObject()->as<IFunction>();
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
			o->setConstant();
	}
	else
	{
		obj=Variables.findObjVar(nameId,ns,DECLARED_TRAIT, DECLARED_TRAIT);
		++varcount;
	}
	obj->isenumerable = isEnumerable;
	switch(type)
	{
		case NORMAL_METHOD:
		{
			obj->setVar(f,getSystemState());
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
	variable* obj=varcount ? Variables.findObjVar(getSystemState(),name,NO_CREATE_TRAIT,DYNAMIC_TRAIT|DECLARED_TRAIT) : NULL;
	
	if(obj==NULL)
	{
		if (classdef && classdef->isSealed)
			return false;

		// fixed properties cannot be deleted
		if (hasPropertyByMultiname(name,true,true))
			return false;

		//unknown variables must return true
		return true;
	}
	//Only dynamic traits are deletable
	if (obj->kind != DYNAMIC_TRAIT && obj->kind != INSTANCE_TRAIT)
		return false;

	assert(obj->getter.isInvalid() && obj->setter.isInvalid() && obj->var.isValid());
	//Now dereference the value
	ASATOM_DECREF(obj->var);

	//Now kill the variable
	Variables.killObjVar(getSystemState(),name);
	--varcount;
	return true;
}

//In all setter we first pass the value to the interface to see if special handling is possible
void ASObject::setVariableByMultiname_i(const multiname& name, int32_t value)
{
	check();
	asAtom v(value);
	setVariableByMultiname(name,v,CONST_NOT_ALLOWED);
}

variable* ASObject::findSettableImpl(SystemState* sys,variables_map& map, const multiname& name, bool* has_getter)
{
	variable* ret=map.findObjVar(sys,name,NO_CREATE_TRAIT,DECLARED_TRAIT|DYNAMIC_TRAIT);
	if(ret)
	{
		//It seems valid for a class to redefine only the getter, so if we can't find
		//something to get, it's ok
		if(!(ret->setter.isValid() || ret->var.isValid()))
		{
			ret=NULL;
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


multiname *ASObject::setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, Class_base* cls)
{
	multiname *retval = nullptr;
	check();
	assert(!cls || classdef->isSubClass(cls));
	//NOTE: we assume that [gs]etSuper and [sg]etProperty correctly manipulate the cur_level (for getActualClass)
	bool has_getter=false;
	variable* obj=findSettable(name, &has_getter);

	if (obj && (obj->kind == CONSTANT_TRAIT && allowConst==CONST_NOT_ALLOWED))
	{
		if (obj->var.isFunction() || obj->setter.isValid())
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
		if(obj && (obj->issealed || cls->isFinal || cls->isSealed) && obj->setter.isInvalid())
		{
			throwError<ReferenceError>(kCannotAssignToMethodError, name.normalizedNameUnresolved(getSystemState()), cls ? cls->getQualifiedClassName() : "");
		}
	}

	//Do not set variables in prototype chain. Still have to do
	//lookup to throw a correct error in case a named function
	//exists in prototype chain. See Tamarin test
	//ecma3/Boolean/ecma4_sealedtype_1_rt
	if(!obj && cls && cls->isSealed)
	{
		variable *protoObj = cls->findSettableInPrototype(name);
		if (protoObj && 
			((protoObj->var.isFunction()) ||
			 protoObj->setter.isValid()))
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
			++varcount;
		}
		
	}
	// it seems that instance traits are changed into declared traits if they are overwritten in class objects
	// see tamarin test as3/Definitions/FunctionAccessors/GetSetStatic
	if (this->is<Class_base>() && obj->kind == INSTANCE_TRAIT)
		obj->kind =DECLARED_TRAIT;

	if(obj->setter.isValid())
	{
		//Call the setter
		LOG_CALL(_("Calling the setter"));
		//One argument can be passed without creating an array
		ASObject* target=this;
		asAtom* arg1 = &o;

		asAtom v =asAtom::fromObject(target);
		asAtom ret;
		obj->setter.callFunction(ret,v,arg1,1,false);
		if (obj->setter.is<SyntheticFunction>())
			retval = obj->setter.as<SyntheticFunction>()->getSimpleName();
		ASATOM_DECREF(ret);
		ASATOM_DECREF(o);
		// it seems that Adobe allows setters with return values...
		//assert_and_throw(ret.isUndefined());
		LOG_CALL(_("End of setter"));
	}
	else
	{
		assert_and_throw(obj->getter.isInvalid());
		obj->setVar(o,getSystemState());
	}
	if (o.is<SyntheticFunction>())
		checkFunctionScope(o.getObject()->as<SyntheticFunction>());
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

void ASObject::setVariableByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable)
{
	asAtom v = asAtom::fromObject(o);
	setVariableAtomByQName(nameId, ns, v, traitKind, isEnumerable);
}
void ASObject::setVariableAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable)
{
	setVariableAtomByQName(getSystemState()->getUniqueStringId(name), ns, o, traitKind,isEnumerable);
}
void ASObject::setVariableAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable, bool isRefcounted)
{
	assert_and_throw(Variables.findObjVar(nameId,ns,NO_CREATE_TRAIT,traitKind)==NULL);
	variable* obj=Variables.findObjVar(nameId,ns,traitKind,traitKind);
	obj->setVar(o,getSystemState(),isRefcounted);
	obj->isenumerable=isEnumerable;
	++varcount;
}

void ASObject::initializeVariableByMultiname(const multiname& name, asAtom &o, multiname* typemname,
		ABCContext* context, TRAIT_KIND traitKind, uint32_t slot_id, bool isenumerable)
{
	check();
	Variables.initializeVar(name, o, typemname, context, traitKind,this,slot_id,isenumerable);
	++varcount;
}

variable::variable(TRAIT_KIND _k, asAtom _v, multiname* _t, const Type* _type, const nsNameAndKind& _ns, bool _isenumerable)
		: var(_v),typeUnion(NULL),ns(_ns),kind(_k),isResolved(false),isenumerable(_isenumerable),issealed(false),isrefcounted(true)
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

void variable::setVar(asAtom& v, SystemState* sys, bool _isrefcounted)
{
	//Resolve the typename if we have one
	//currentCallContext may be NULL when inserting legacy
	//children, which is done outisde any ABC context
	if(!isResolved && traitTypemname && getVm(sys)->currentCallContext)
	{
		type = Type::getTypeFromMultiname(traitTypemname, getVm(sys)->currentCallContext->mi->context);
		assert(type);
		isResolved=true;
	}
	if(isResolved && type)
		type->coerce(sys,v);
	if(isrefcounted)
		ASATOM_DECREF(var);
	var=v;
	isrefcounted = _isrefcounted;
}

void variable::setVarNoCoerce(asAtom &v)
{
	ASATOM_DECREF(var);
	var=v;
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

void variables_map::initializeVar(const multiname& mname, asAtom& obj, multiname* typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj, uint32_t slot_id,bool isenumerable)
{
	const Type* type = NULL;
	if (typemname->isStatic)
		type = typemname->cachedType;
	
	asAtom value;
	 /* If typename is a builtin type, we coerce obj.
	  * It it's not it must be a user defined class,
	  * so we try to find the class it is derived from and create an apropriate uninitialized instance */
	if (type == NULL)
		type = Type::getBuiltinType(mainObj->getSystemState(),typemname);
	if (type == NULL)
		type = Type::getTypeFromMultiname(typemname,context);
	if(type==NULL)
	{
		if (obj.isInvalid()) // create dynamic object
		{
			value = asAtom::undefinedAtom;
		}
		else
		{
			assert_and_throw(obj.is<Null>() || obj.is<Undefined>());
			value = obj;
			if(obj.is<Undefined>())
			{
				//Casting undefined to an object (of unknown class)
				//results in Null
				value = asAtom::nullAtom;
			}
		}
	}
	else
	{
		if (obj.isInvalid()) // create dynamic object
		{
			if(mainObj->is<Class_base>() 
				&& mainObj->as<Class_base>()->class_name.nameId == typemname->normalizedNameId(mainObj->getSystemState())
				&& mainObj->as<Class_base>()->class_name.nsStringId == typemname->ns[0].nsNameId
				&& typemname->ns[0].kind == NAMESPACE)
			{
				// avoid recursive construction
				value = asAtom::undefinedAtom;
			}
			else
			{
				value = asAtom::undefinedAtom;
				type->coerce(mainObj->getSystemState(),value);
			}
		}
		else
			value = obj;

		if (typemname->isStatic && typemname->cachedType == NULL)
			typemname->cachedType = type;
	}
	assert(traitKind==DECLARED_TRAIT || traitKind==CONSTANT_TRAIT || traitKind == INSTANCE_TRAIT);
	if (value.getObject())
		cloneable = false;

	uint32_t name=mname.normalizedNameId(mainObj->getSystemState());
	Variables.insert(Variables.cbegin(),make_pair(name, variable(traitKind, value, typemname, type,mname.ns[0],isenumerable)));
	if (slot_id)
		initSlot(slot_id,name, mname.ns[0]);
}

ASFUNCTIONBODY_ATOM(ASObject,generator)
{
	//By default we assume it's a passthrough cast
	if(argslen==1)
	{
		LOG_CALL(_("Passthrough of ") << args[0].toDebugString());
		ASATOM_INCREF(args[0]);
		ret = args[0];
	}
	else if (argslen > 1)
		throwError<ArgumentError>(kCoerceArgumentCountError,Integer::toString(argslen));
	else
		ret = asAtom::fromObject(Class<ASObject>::getInstanceS(sys));
}

ASFUNCTIONBODY_ATOM(ASObject,_toString)
{
	tiny_string res;
	if (obj.is<Class_base>())
	{
		res="[object ";
		res+=sys->getStringFromUniqueId(obj.as<Class_base>()->class_name.nameId);
		res+="]";
	}
	else if(obj.is<IFunction>())
	{
		// ECMA spec 15.3.4.2 says that toString on a function object is implementation dependent
		// adobe player returns "[object Function-46]", so we do the same
		res="[object ";
		res+="Function-46";
		res+="]";
	}
	else if(obj.getClass(sys))
	{
		res="[object ";
		res+=sys->getStringFromUniqueId(obj.getClass(sys)->class_name.nameId);
		res+="]";
	}
	else
		res="[object Object]";

	ret = asAtom::fromString(sys,res);
}

ASFUNCTIONBODY_ATOM(ASObject,_toLocaleString)
{
	_toString(ret,sys,obj,args,argslen);
}

ASFUNCTIONBODY_ATOM(ASObject,hasOwnProperty)
{
	assert_and_throw(argslen==1);
	multiname name(NULL);
	switch (args[0].getObjectType())
	{
		case T_INTEGER:
			name.name_type=multiname::NAME_INT;
			name.name_i = args[0].toInt();
			break;
		case T_UINTEGER:
			name.name_type=multiname::NAME_UINT;
			name.name_ui = args[0].toUInt();
			break;
		case T_NUMBER:
			name.name_type=multiname::NAME_NUMBER;
			name.name_d = args[0].toNumber();
			break;
		default:
			name.name_type=multiname::NAME_STRING;
			name.name_s_id=args[0].toStringId(sys);
			name.isInteger=Array::isIntegerWithoutLeadingZeros(sys->getStringFromUniqueId(name.name_s_id)) ;
			break;
	}
	name.ns.emplace_back(sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	ret.setBool(obj.toObject(sys)->hasPropertyByMultiname(name, true, false));
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
	Class_base* cls = args[0].toObject(sys)->getClass();
	
	while (cls != NULL)
	{
		if (cls->prototype->getObj() == obj.getObject())
		{
			res = true;
			break;
		}
		Class_base* clsparent = cls->prototype->getObj()->getClass();
		if (clsparent == cls)
			break;
		cls = clsparent;
	}
	ret.setBool(res);
}

ASFUNCTIONBODY_ATOM(ASObject,propertyIsEnumerable)
{
	if (argslen == 0)
	{
		ret.setBool(false);
		return;
	}
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=args[0].toStringId(sys);
	name.ns.emplace_back(sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	if (obj.is<Array>()) // propertyIsEnumerable(index) isn't mentioned in the ECMA specs but is tested for
	{
		Array* a = obj.as<Array>();
		unsigned int index = 0;
		if (a->isValidMultiname(sys,name,index))
		{
			ret.setBool(index < (unsigned int)a->size());
			return;
		}
	}
	variable* v = obj.toObject(sys)->Variables.findObjVar(sys,name, NO_CREATE_TRAIT,DYNAMIC_TRAIT|DECLARED_TRAIT);
	if (v)
		ret.setBool(v->isenumerable);
	else
		ret.setBool(obj.getObject()->hasPropertyByMultiname(name,true,false));
}
ASFUNCTIONBODY_ATOM(ASObject,setPropertyIsEnumerable)
{
	tiny_string propname;
	bool isEnum;
	ARG_UNPACK_ATOM(propname) (isEnum, true);
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=args[0].toStringId(sys);
	name.ns.emplace_back(sys,BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	obj.toObject(sys)->setIsEnumerable(name, isEnum);
}
ASFUNCTIONBODY_ATOM(ASObject,addProperty)
{
	tiny_string name;
	_NR<IFunction> getter;
	_NR<IFunction> setter;
	ARG_UNPACK_ATOM(name)(getter)(setter);
	if (name.empty())
		throwError<ArgumentError>(kInvalidArgumentError,Integer::toString(0));
	if (getter.isNull())
		throwError<ArgumentError>(kInvalidArgumentError,Integer::toString(1));
	if (setter.isNull())
		throwError<ArgumentError>(kInvalidArgumentError,Integer::toString(2));
	obj.toObject(sys)->setDeclaredMethodByQName(name,"",getter.getPtr(),GETTER_METHOD,false);
	obj.toObject(sys)->setDeclaredMethodByQName(name,"",setter.getPtr(),SETTER_METHOD,false);
}

void ASObject::setIsEnumerable(const multiname &name, bool isEnum)
{
	variable* v = Variables.findObjVar(getSystemState(),name, NO_CREATE_TRAIT,DYNAMIC_TRAIT);
	if (v)
		v->isenumerable = isEnum;
}

bool ASObject::cloneInstance(ASObject *target)
{
	if (Variables.cloneInstance(target->Variables))
	{
		target->varcount = this->varcount;
		return true;
	}
	return false;
}

void ASObject::checkFunctionScope(ASObject* o)
{
	SyntheticFunction* f = o->as<SyntheticFunction>();
	if (f->inClass)
		return;
	for (auto it = f->func_scope->scope.rbegin(); it != f->func_scope->scope.rend(); it++)
	{
		if ((it->considerDynamic)
				&& it->object.getObject() == this
				&& !it->object.is<Global>())
		{
			f->incActivationCount();
			if (!it->object.is<Activation_object>())
				f->addDynamicReferenceObject(this);
			break;
		}
	}
}

asAtom ASObject::getVariableBindingValue(const tiny_string &name)
{
	uint32_t nameId = getSystemState()->getUniqueStringId(name);
	asAtom obj;
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
	throwError<ArgumentError>(kCantInstantiateError, obj.toObject(sys)->getClassName());
}

void ASObject::initSlot(unsigned int n, const multiname& name)
{
	Variables.initSlot(n,name.name_s_id,name.ns[0]);
}
void ASObject::initAdditionalSlots(std::vector<multiname*> additionalslots)
{
	unsigned int n = Variables.slots_vars.size();
	for (auto it = additionalslots.begin(); it != additionalslots.end(); it++)
		Variables.initSlot(++n,(*it)->name_s_id,(*it)->ns[0]);
}
int32_t ASObject::getVariableByMultiname_i(const multiname& name)
{
	check();

	asAtom ret;
	getVariableByMultinameIntern(ret,name,this->getClass());
	assert_and_throw(ret.isValid());
	return ret.toInt();
}

variable* ASObject::findVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls, uint32_t *nsRealID, bool *isborrowed)
{
	//Get from the current object without considering borrowed properties
	variable* obj=varcount ? Variables.findObjVar(getSystemState(),name,name.hasEmptyNS ? DECLARED_TRAIT|DYNAMIC_TRAIT : DECLARED_TRAIT,nsRealID):NULL;
	if(obj)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(obj->getter.isValid()|| obj->var.isValid()))
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
						if(!(obj->getter.isValid() || obj->var.isValid()))
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
	variable* obj=varcount ? Variables.findObjVar(getSystemState(),name,((opt & FROM_GETLEX) || name.hasEmptyNS || name.hasBuiltinNS) ? DECLARED_TRAIT|DYNAMIC_TRAIT : DECLARED_TRAIT,&nsRealId):NULL;
	if(obj)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(obj->getter.isValid() || obj->var.isValid()))
			obj=NULL;
	}

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
						if(!(obj->getter.isValid() || obj->var.isValid()))
							obj=NULL;
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

	if(obj->getter.isValid())
	{
		if (opt & DONT_CALL_GETTER)
		{
			ret.set(obj->getter);
			res = (GET_VARIABLE_RESULT)(res | GET_VARIABLE_RESULT::GETVAR_ISGETTER);
			return res;
		}
		//Call the getter
		LOG_CALL("Calling the getter intern for " << name << " on " << this->toDebugString());
		assert(obj->getter.isFunction());
		obj->getter.as<IFunction>()->callGetter(ret,obj->getter.getClosure() ? obj->getter.getClosure() : this);
		LOG_CALL(_("End of getter")<< ' ' << obj->getter.toDebugString()<<" result:"<<ret.toDebugString());
	}
	else
	{
		assert_and_throw(obj->setter.isInvalid());
		if (!(opt & NO_INCREF))
			ASATOM_INCREF(obj->var);
		if(obj->var.isFunction() && obj->var.getObject()->as<IFunction>()->isMethod())
		{
			if (obj->var.getClosure())
			{
				LOG_CALL("function " << name << " is already bound to "<<obj->var.toDebugString() );
				ret.set(obj->var);
			}
			else
			{
				LOG_CALL("Attaching this " << this->toDebugString() << " to function " << name << " "<<obj->var.toDebugString());
				if (!(opt & FROM_GETLEX) && !(opt & SKIP_IMPL))
					this->incRef();
				ret.setFunction(obj->var.getObject(),this);
			}
		}
		else
			ret.set(obj->var);
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
	asAtom o;
	getVariableByMultiname(o,methodName, namespaces);
	if (!o.isFunction())
		throwError<TypeError>(kCallOfNonFunctionError, methodName);
	asAtom v =asAtom::fromObject(this);
	o.callFunction(ret,v,args,num_args,false);
}

void ASObject::check() const
{
	//Put here a bunch of safety check on the object
#ifndef NDEBUG
	assert(getConstant() || (getRefCount()>0));
#endif
	Variables.check();
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
			if(it->second.var.isInvalid() && next->second.var.isInvalid())
				continue;

			if(it->second.var.isInvalid() || next->second.var.isInvalid())
			{
				LOG(LOG_INFO, it->first << " " << it->second.ns);
				LOG(LOG_INFO, it->second.var.getObjectType() << ' ' << it->second.setter.getObjectType() << ' ' << it->second.getter.getObjectType());
				LOG(LOG_INFO, next->second.var.getObjectType() << ' ' << next->second.setter.getObjectType() << ' ' << next->second.getter.getObjectType());
				abort();
			}

			if(!it->second.var.isFunction() || !next->second.var.isFunction())
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
				assert(false);
		}
		LOG(LOG_INFO, kind <<  '[' << it->second.ns << "] "<<
			getSys()->getStringFromUniqueId(it->first) << ' ' <<
			it->second.var.toDebugString() << ' ' << it->second.setter.toDebugString() << ' ' << it->second.getter.toDebugString());
	}
	auto it2 = slots_vars.begin();
	while (it2 != slots_vars.end())
	{
		LOG(LOG_INFO, "slot:" << (it2-slots_vars.begin() )
			<<" "<<getSys()->getStringFromUniqueId(it2->nameId) 
			<<"[" << it2->ns << "]");
		it2++;
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
	this->slots_vars.clear();
}

bool variables_map::cloneInstance(variables_map &map)
{
	if (!cloneable)
		return false;
	map.Variables = Variables;
	map.slots_vars = this->slots_vars;
	return true;
}

void variables_map::removeAllDeclaredProperties()
{
	var_iterator it=Variables.begin();
	while(it!=Variables.cend())
	{
		
		if (it->second.getter.isValid()
			|| it->second.setter.isValid())
		{
			it = Variables.erase(it);
		}
		else
			it++;
	}
}

ASObject::ASObject(Class_base* c,SWFOBJECT_TYPE t,CLASS_SUBTYPE st):objfreelist(c && c->getSystemState()->singleworker && c->isReusable ? c->freelist : NULL),Variables((c)?c->memoryAccount:NULL),varcount(0),classdef(c),proxyMultiName(NULL),sys(c?c->sys:NULL),
	stringId(UINT32_MAX),type(t),subtype(st),traitsInitialized(false),constructIndicator(false),constructorCallComplete(false),implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif
}

ASObject::ASObject(const ASObject& o):objfreelist(o.classdef && o.classdef->getSystemState()->singleworker && o.classdef->isReusable ? o.classdef->freelist : NULL),Variables((o.classdef)?o.classdef->memoryAccount:NULL),varcount(0),classdef(NULL),proxyMultiName(NULL),sys(o.classdef? o.classdef->sys : NULL),
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
	destroyContents();
	if (proxyMultiName)
		delete proxyMultiName;
	stringId = UINT32_MAX;
	proxyMultiName = NULL;
	traitsInitialized =false;
	constructIndicator = false;
	constructorCallComplete =false;
	implEnable = true;
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif
	bool dodestruct = true;
	if (objfreelist)
	{
		if (!getCached())
			dodestruct = !objfreelist->pushObjectToFreeList(this);
		else
			dodestruct = false;
	}
	if (dodestruct)
	{
		finalize();
	}
	return dodestruct;
}

void variables_map::initSlot(unsigned int n, uint32_t nameId, const nsNameAndKind& ns)
{
	if (n>slots_vars.capacity())
		slots_vars.reserve(n+8);
	if(n>slots_vars.size())
		slots_vars.resize(n);

	var_iterator ret=Variables.find(nameId);

	if(ret==Variables.end())
	{
		//Name not present, no good
		throw RunTimeException("initSlot on missing variable");
	}
	slots_vars[n-1]=varName(nameId,ns);
}

asAtom variables_map::getSlot(unsigned int n)
{
	assert_and_throw(n > 0 && n<=slots_vars.size());
	var_iterator it = Variables.find(slots_vars[n-1].nameId);
	while(it!=Variables.end() && it->first == slots_vars[n-1].nameId)
	{
		if (it->second.ns == slots_vars[n-1].ns)
			return it->second.var;
		it++;
	}
	return asAtom::invalidAtom;
}
TRAIT_KIND variables_map::getSlotKind(unsigned int n)
{
	assert_and_throw(n > 0 && n<=slots_vars.size());
	var_iterator it = Variables.find(slots_vars[n-1].nameId);
	while(it!=Variables.end() && it->first == slots_vars[n-1].nameId)
	{
		if (it->second.ns == slots_vars[n-1].ns)
			return it->second.kind;
		it++;
	}
	return TRAIT_KIND::NO_CREATE_TRAIT;
}
uint32_t variables_map::findInstanceSlotByMultiname(multiname* name)
{
	for (auto it = slots_vars.begin(); it != slots_vars.end();it++)
	{
		if (it->nameId == name->name_s_id && (name->ns.size() == 0 || it->ns == name->ns[0]))
		{
			var_iterator it2 = Variables.find(it->nameId);
			while(it2!=Variables.end() && it2->first == it->nameId)
			{
				if ((name->ns.size() == 0 || it->ns == it2->second.ns)
						&& (it2->second.kind == INSTANCE_TRAIT))
						return it-slots_vars.begin()+1;
				it2++;
			}
		}
	}
	return UINT32_MAX;
}
void variables_map::setSlot(unsigned int n,asAtom o,SystemState* sys)
{
	validateSlotId(n);
	var_iterator it = Variables.find(slots_vars[n-1].nameId);
	while(it!=Variables.end() && it->first == slots_vars[n-1].nameId)
	{
		if (it->second.ns == slots_vars[n-1].ns)
		{
			it->second.setVar(o,sys);
			return;
		}
		it++;
	}
}

void variables_map::setSlotNoCoerce(unsigned int n, asAtom o)
{
	validateSlotId(n);
	var_iterator it = Variables.find(slots_vars[n-1].nameId);
	while(it!=Variables.end() && it->first == slots_vars[n-1].nameId)
	{
		if (it->second.ns == slots_vars[n-1].ns)
		{
			it->second.setVarNoCoerce(o);
			return;
		}
		it++;
	}
}

void variables_map::validateSlotId(unsigned int n) const
{
	if(n == 0 || n-1<slots_vars.size())
	{
//		if(slots_vars[n-1] && slots_vars[n-1]->setter)
//		{
//			LOG(LOG_INFO,"validate2:"<<this<<" "<<n<<" "<<slots_vars[n-1]->setter);
//			throw UnsupportedException("setSlot has setters");
//		}
	}
	else
		throw RunTimeException("setSlot out of bounds");
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
	if(obj->getter.isValid())
	{
		//Call the getter
		LOG_CALL(_("Calling the getter"));
		asAtom v=asAtom::fromObject(this);
		obj->getter.callFunction(ret, v,NULL,0,false);
		LOG_CALL("End of getter at index "<<index<<":"<< obj->getter.toDebugString()<<" result:"<<ret.toDebugString());
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
	return varcount;
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

		asAtom wr;
		out->getSystemState()->static_ObjectEncoding_dynamicPropertyWriter->getVariableByMultiname(wr,m,SKIP_IMPL);
		if (!wr.isFunction())
			throwError<TypeError>(kCallOfNonFunctionError, m.normalizedNameUnresolved(out->getSystemState()));
	
		asAtom ret;
		asAtom v =asAtom::fromObject(out->getSystemState()->static_ObjectEncoding_dynamicPropertyWriter.getPtr());
		ASATOM_INCREF(v);
		asAtom args[2];
		args[0] = asAtom::fromObject(this);
		args[1] = asAtom::fromObject(o);
		wr.callFunction(ret,v,args,2,false);
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
		it->second.var.toObject(out->getSystemState())->serialize(out, stringMap, objMap, traitsMap);
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

		asAtom o;
		getVariableByMultiname(o,writeExternalName,SKIP_IMPL);
		assert_and_throw(o.isFunction());
		asAtom tmpArg[1] = { asAtom::fromObject(out) };
		asAtom v=asAtom::fromObject(this);
		asAtom r;
		o.callFunction(r,v, tmpArg, 1,false);
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
					varIt->second.var.toObject(getSystemState())->serialize(out, stringMap, objMap, traitsMap);
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
			varIt->second.var.toObject(getSystemState())->serialize(out, stringMap, objMap, traitsMap);
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
		prot->describeInstance(root,false);

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
			if(varIt->second.ns.hasEmptyName() && (varIt->second.getter.isValid() || varIt->second.var.isValid()))
			{
				ASObject* v = varIt->second.var.isValid() ? varIt->second.var.toObject(getSystemState()) : nullptr;
				if (varIt->second.getter.isValid())
				{
					asAtom t=asAtom::fromObject(this);
					asAtom res;
					varIt->second.getter.callFunction(res,t,NULL,0,false);
					v=res.toObject(getSystemState());
				}
				if(v && v->getObjectType() != T_UNDEFINED && varIt->second.isenumerable)
				{
					// check for cylic reference
					if (v->getObjectType() != T_UNDEFINED &&
						v->getObjectType() != T_NULL &&
						v->getObjectType() != T_BOOLEAN &&
						std::find(path.begin(),path.end(), v) != path.end())
						throwError<TypeError>(kJSONCyclicStructure);
		
					if (replacer.isValid())
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
						
						params[0] = asAtom::fromStringID(varIt->first);
						params[1] = asAtom::fromObject(v);
						ASATOM_INCREF(params[1]);
						asAtom funcret;
						replacer.callFunction(funcret,asAtom::nullAtom, params, 2,true);
						if (funcret.isValid())
							res += funcret.toString(getSystemState());
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
	return (var && var->var.isValid());
}

ASObject* ASObject::getprop_prototype()
{
	variable* var=Variables.findObjVar(BUILTIN_STRINGS::PROTOTYPE,nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),
			NO_CREATE_TRAIT,(DECLARED_TRAIT|DYNAMIC_TRAIT));
	return var ? var->var.toObject(getSystemState()) : NULL;
}

/*
 * (creates and) sets the property 'prototype' to o
 * 'prototype' is usually DYNAMIC_TRAIT, but on Class_base
 * it is a DECLARED_TRAIT, which is gettable only
 */
void ASObject::setprop_prototype(_NR<ASObject>& o)
{
	ASObject* obj = o.getPtr();

	multiname prototypeName(NULL);
	prototypeName.name_type=multiname::NAME_STRING;
	prototypeName.name_s_id=getSystemState()->getUniqueStringId("prototype");
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
		++varcount;
	}
	if(ret->setter.isValid())
	{
		asAtom arg1= asAtom::fromObject(obj);
		asAtom v=asAtom::fromObject(this);
		asAtom res;
		ret->setter.callFunction(res,v,&arg1,1,false);
	}
	else
	{
		obj->incRef();
		asAtom v = asAtom::fromObject(obj);
		ret->setVar(v,getSystemState());
	}
}

tiny_string ASObject::getClassName() const
{
	if (getClass())
		return getClass()->getQualifiedClassName();
	else
		return "";
}

asAtom asAtom::undefinedAtom(T_UNDEFINED);
asAtom asAtom::nullAtom(T_NULL);
asAtom asAtom::invalidAtom(T_INVALID);
asAtom asAtom::trueAtom(true);
asAtom asAtom::falseAtom(false);

bool asAtom::stringcompare(SystemState* sys,uint32_t stringID)
{
	return this->toObject(sys)->toString() == sys->getStringFromUniqueId(stringID);
}
bool asAtom::functioncompare(SystemState* sys,asAtom& v2)
{
	if (getObject() && getObject()->as<IFunction>()->closure_this 
			&& v2.getObject() && v2.getObject()->as<IFunction>()->closure_this
			&& getObject()->as<IFunction>()->closure_this != v2.getObject()->as<IFunction>()->closure_this)
		return false;
	return v2.toObject(sys)->isEqual(this->toObject(sys));
}
ASObject* asAtom::getClosure() const
{
	return (getObject() && getObject()->is<IFunction>()) ? getObject()->as<IFunction>()->closure_this.getPtr() : nullptr;
}
asAtom asAtom::getClosureAtom(asAtom defaultAtom) const
{
	ASObject* o = getClosure();
	return o ? asAtom::fromObject(o) : defaultAtom;
}
asAtom asAtom::fromString(SystemState* sys, const tiny_string& s)
{
	asAtom a;
	a.uintval = (sys->getUniqueStringId(s)<<3) | ATOM_STRINGID;
	return a;
}

void asAtom::callFunction(asAtom& ret,asAtom &obj, asAtom *args, uint32_t num_args, bool args_refcounted, bool coerceresult)
{
	assert_and_throw(getObject() && getObject()->is<IFunction>());

	asAtom c = obj;
	if (getObject()->is<SyntheticFunction>())
	{
		if (!args_refcounted)
		{
			for (uint32_t i = 0; i < num_args; i++)
				ASATOM_INCREF(args[i]);
		}
		getObject()->as<SyntheticFunction>()->call(ret,c, args, num_args,coerceresult);
		if (args_refcounted)
			ASATOM_DECREF(c);
		return;
	}
	// when calling builtin functions, normally no refcounting is needed
	// if it is, it has to be done inside the called function
	getObject()->as<Function>()->call(ret, c, args, num_args);
	if (args_refcounted)
	{
		for (uint32_t i = 0; i < num_args; i++)
			ASATOM_DECREF(args[i]);
		ASATOM_DECREF(c);
	}
}

void asAtom::getVariableByMultiname(asAtom& ret,SystemState* sys, const multiname &name)
{
	// classes for primitives are final and sealed, so we only have to check the class for the variable
	// no need to create ASObjects for the primitives
	switch(uintval&0x7)
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
			switch (uintval&0x70)
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
Class_base *asAtom::getClass(SystemState* sys,bool followclass)
{
	// classes for primitives are final and sealed, so we only have to check the class for the variable
	// no need to create ASObjects for the primitives
	switch(uintval&0x7)
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
			switch (uintval&0x70)
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
			return getObject() ? (followclass && getObject()->is<Class_base>() ? getObject()->as<Class_base>() : getObject()->getClass()) : nullptr;
	}
	return nullptr;
}

bool asAtom::canCacheMethod(const multiname* name)
{
	assert(name->isStatic);
	switch(uintval&0x7)
	{
		case ATOM_INTEGER:
		case ATOM_UINTEGER:
		case ATOM_NUMBERPTR:
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		case ATOM_STRINGID:
			return true;
		case ATOM_OBJECTPTR:
		{
			if (!getObject())
				return false;
			switch (getObject()->getObjectType())
			{
				case T_FUNCTION:
					return true;
				case T_CLASS:
					return getObject()->as<Class_base>()->isSealed;
				case T_ARRAY:
					// Array class is dynamic, but if it is not inherited, no methods are overwritten and we can cache them
					return (getObject()->getClass()->isSealed || !getObject()->getClass()->is<Class_inherit>());
				default:
					return getObject()->getClass()->isSealed;
			}
		}
		default:
			break;
	}
	return false;
}

void asAtom::fillMultiname(SystemState* sys, multiname &name)
{
	switch(uintval&0x7)
	{
		case ATOM_INTEGER:
			name.name_type = multiname::NAME_INT;
			name.name_i = intval>>3;
			break;
		case ATOM_UINTEGER:
			name.name_type = multiname::NAME_UINT;
			name.name_ui = uintval>>3;
			break;
		case ATOM_NUMBERPTR:
			name.name_type = multiname::NAME_NUMBER;
			name.name_d = toNumber();
			break;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			name.name_type = multiname::NAME_INT;
			name.name_i = toInt();
			break;
		case ATOM_STRINGID:
			name.name_type = multiname::NAME_STRING;
			name.name_s_id = uintval>>3;
			break;
		default:
			name.name_type=multiname::NAME_OBJECT;
			name.name_o=toObject(sys);
			break;
	}
}

void asAtom::replaceBool(ASObject *obj)
{
	uintval = obj->as<Boolean>()->val ? 0x100 : 0;
}

std::string asAtom::toDebugString()
{
	switch(uintval&0x7)
	{
		case ATOM_INTEGER:
			return Integer::toString(intval>>3)+"i";
		case ATOM_UINTEGER:
			return UInteger::toString(uintval>>3)+"ui";
		case ATOM_NUMBERPTR:
		{
			std::string ret = Number::toString(toNumber())+"d";
#ifndef _NDEBUG
			assert(getObject());
			char buf[300];
			sprintf(buf,"(%p / %d/%d)",getObject(),getObject()->getRefCount(),getObject()->getConstant());
			ret += buf;
#endif
			return ret;
		}
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					return "Null";
				case ATOMTYPE_UNDEFINED_BIT:
					return "Undefined";
				case ATOMTYPE_BOOL_BIT:
					return Integer::toString((uintval&0x80)>>7)+"b";
				default:
					return "Invalid";
			}
		}
		case ATOM_STRINGID:
			return getSys()->getStringFromUniqueId(uintval>>3);
		default:
			assert(getObject());
			return getObject()->toDebugString();
	}
}

asAtom asAtom::asTypelate(asAtom& a)
{
	LOG_CALL(_("asTypelate"));

	if(!a.isObject() || !a.is<Class_base>())
	{
		LOG(LOG_ERROR,"trying to call asTypelate on non class object:"<<toDebugString());
		throwError<TypeError>(kConvertNullToObjectError);
	}
	Class_base* c=static_cast<Class_base*>(a.getObject());
	//Special case numeric types
	if(isNumeric())
	{
		bool real_ret;
		if(c==Class<Number>::getClass(c->getSystemState()) || c==c->getSystemState()->getObjectClassRef())
			real_ret=true;
		else if(c==Class<Integer>::getClass(c->getSystemState()))
			real_ret=(toNumber()==toInt());
		else if(c==Class<UInteger>::getClass(c->getSystemState()))
			real_ret=(toNumber()==toUInt());
		else
			real_ret=false;
		LOG_CALL(_("Numeric type is ") << ((real_ret)?"":_("not ")) << _("subclass of ") << c->class_name);
		ASATOM_DECREF(a);
		if(real_ret)
			return *this;
		else
			return asAtom::nullAtom;
	}

	Class_base* objc = toObject(c->getSystemState())->getClass();
	if(!objc)
	{
		ASATOM_DECREF(a);
		return asAtom::nullAtom;
	}

	bool real_ret=objc->isSubClass(c);
	LOG_CALL(_("Type ") << objc->class_name << _(" is ") << ((real_ret)?_(" "):_("not ")) 
			<< _("subclass of ") << c->class_name);
	ASATOM_DECREF(a);
	if(real_ret)
		return *this;
	else
		return asAtom::nullAtom;
	
}

bool asAtom::isTypelate(ASObject *type)
{
	LOG_CALL(_("isTypelate"));
	bool real_ret=false;

	Class_base* objc=NULL;
	Class_base* c=NULL;
	switch (type->getObjectType())
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_OBJECT:
		case T_STRING:
			LOG(LOG_ERROR,"trying to call isTypelate on object:"<<toDebugString());
			throwError<TypeError>(kIsTypeMustBeClassError);
			break;
		case T_NULL:
			LOG(LOG_ERROR,"trying to call isTypelate on null:"<<toDebugString());
			throwError<TypeError>(kConvertNullToObjectError);
			break;
		case T_UNDEFINED:
			LOG(LOG_ERROR,"trying to call isTypelate on undefined:"<<toDebugString());
			throwError<TypeError>(kConvertUndefinedToObjectError);
			break;
		case T_CLASS:
			break;
		default:
			throwError<TypeError>(kIsTypeMustBeClassError);
	}

	c=static_cast<Class_base*>(type);
	//Special case numeric types
	if(isNumeric())
	{
		if(c==Class<Number>::getClass(c->getSystemState()) || c==c->getSystemState()->getObjectClassRef())
			real_ret=true;
		else if(c==Class<Integer>::getClass(c->getSystemState()))
			real_ret=(toNumber()==toInt());
		else if(c==Class<UInteger>::getClass(c->getSystemState()))
			real_ret=(toNumber()==toUInt());
		else
			real_ret=false;
		LOG_CALL(_("Numeric type is ") << ((real_ret)?"":_("not ")) << _("subclass of ") << c->class_name);
		type->decRef();
		return real_ret;
	}

	objc = getClass(c->getSystemState(),false);
	if(!objc)
	{
		real_ret=getObjectType()==type->getObjectType();
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


tiny_string asAtom::toString(SystemState* sys)
{
	switch(uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					return "null";
				case ATOMTYPE_UNDEFINED_BIT:
					return "undefined";
				case ATOMTYPE_BOOL_BIT:
					return uintval&0x80 ? "true" : "false";
				default:
					return "";
			}
		}
		case ATOM_NUMBERPTR:
			return Number::toString(toNumber());
		case ATOM_INTEGER:
			return Integer::toString(intval>>3);
		case ATOM_UINTEGER:
			return UInteger::toString(uintval>>3);
		case ATOM_STRINGID:
			if ((uintval>>3) == 0)
				return "";
			if ((uintval>>3) < BUILTIN_STRINGS_CHAR_MAX)
				return tiny_string::fromChar(uintval>>3);
			return sys->getStringFromUniqueId(uintval>>3);
		default:
			assert(getObject());
			return getObject()->toString();
	}
}
tiny_string asAtom::toLocaleString()
{
	switch(uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0x70)
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
			return Number::toString(toNumber());
		case ATOM_INTEGER:
			return Integer::toString(intval>>3);
		case ATOM_UINTEGER:
			return UInteger::toString(uintval>>3);
		case ATOM_STRINGID:
		{
			ASString* s = (ASString*)abstract_s(getSys(),uintval>>3);
			tiny_string res = s->toLocaleString();
			delete s;
			return res;
		}
		default:
			assert(getObject());
			return getObject()->toLocaleString();
	}
}

uint32_t asAtom::toStringId(SystemState* sys)
{
	if (isStringID())
		return uintval>>3;
	return toObject(sys)->toStringId();
}

asAtom asAtom::typeOf(SystemState* sys)
{
	string ret="object";
	switch(uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					ret= "object";
					break;
				case ATOMTYPE_UNDEFINED_BIT:
					ret="undefined";
					break;
				case ATOMTYPE_BOOL_BIT:
					ret="boolean";
					break;
				default:
					break;
			}
			break;
		}
		case ATOM_OBJECTPTR:
			if(getObject()->is<XML>() || getObject()->is<XMLList>())
				ret = "xml";
			if(getObject()->is<Number>() || getObject()->is<Integer>() || getObject()->is<UInteger>())
				ret = "number";
			if(getObject()->is<ASString>())
				ret = "string";
			if(getObject()->is<IFunction>())
				ret = "function";
			if(getObject()->is<Undefined>())
				ret = "undefined";
			if(getObject()->is<Boolean>())
				ret = "boolean";
			break;
		case ATOM_NUMBERPTR:
		case ATOM_INTEGER:
		case ATOM_UINTEGER:
		case ATOM_U_INTEGERPTR:
			ret="number";
			break;
		case ATOM_STRINGID:
		case ATOM_STRINGPTR:
			ret = "string";
			break;
		default:
			break;
	}
	return asAtom::fromString(sys,ret);
}

bool asAtom::Boolean_concrete_string()
{
	return getObject() && !getObject()->as<ASString>()->isEmpty();
}

asAtom::asAtom(SystemState* sys, number_t val, bool constant):uintval((LIGHTSPARK_ATOM_VALTYPE)(constant ? abstract_d_constant(sys,val) : abstract_d(sys,val))|ATOM_NUMBERPTR) 
{
}

void asAtom::convert_b()
{
	bool v = false;
	switch(uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0x70)
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
			v= intval>>3 != 0;
			break;
		case ATOM_UINTEGER:
			v= uintval>>3 != 0;
			break;
		case ATOM_STRINGID:
			v = uintval>>3 != BUILTIN_STRINGS::EMPTY;
			break;
		default:
			v= lightspark::Boolean_concrete(getObject());
			break;
	}
	decRef();
	setBool(v);
}

bool asAtom::add(asAtom &v2, SystemState* sys)
{
	//Implement ECMA add algorithm, for XML and default (see avm2overview)

	// if both values are Integers or UIntegers the result is also an int Number
	if( (isInteger() || isUInteger()) &&
		(v2.isInteger() || v2.isUInteger()))
	{
		int64_t num1=toInt64();
		int64_t num2=v2.toInt64();
		int64_t res = num1+num2;
		LOG_CALL("addI " << num1 << '+' << num2 <<"="<<res);
		if (res >= -(1<<28) && res < (1<<28))
			setInt(sys,res);
		else if (res >= 0 && res < (1<<29))
			setUInt(sys,res);
		else
			setNumber(sys,res);
	}
	else if((isInteger() || isUInteger() || isNumber()) &&
			(v2.isNumber() || v2.isInteger() || v2.isUInteger()))
	{
		double num1=toNumber();
		double num2=v2.toNumber();
		LOG_CALL("addN " << num1 << '+' << num2<<" "<<this->toDebugString()<<" "<<v2.toDebugString());
		setNumber(sys,num1+num2);
	}
	else if(isString() || v2.isString())
	{
		tiny_string a = toString(sys);
		a += v2.toString(sys);
		LOG_CALL("add " << toString(sys) << '+' << v2.toString(sys));
		uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_s(sys,a))|ATOM_STRINGPTR;
	}
	else
	{
		ASObject* val1 = toObject(sys);
		ASObject* val2 = v2.toObject(sys);
		if( (val1->is<XML>() || val1->is<XMLList>()) && (val2->is<XML>() || val2->is<XMLList>()) )
		{
			//Check if the objects are both XML or XMLLists
			Class_base* xmlClass=Class<XML>::getClass(val1->getSystemState());

			XMLList* newList=Class<XMLList>::getInstanceS(val1->getSystemState(),true);
			if(val1->getClass()==xmlClass)
				newList->append(_MR(static_cast<XML*>(val1)));
			else //if(val1->getClass()==xmlListClass)
				newList->append(_MR(static_cast<XMLList*>(val1)));

			if(val2->getClass()==xmlClass)
				newList->append(_MR(static_cast<XML*>(val2)));
			else //if(val2->getClass()==xmlListClass)
				newList->append(_MR(static_cast<XMLList*>(val2)));

			uintval = (LIGHTSPARK_ATOM_VALTYPE)(newList)|ATOM_OBJECTPTR;
			//The references of val1 and val2 have been passed to the smart references
			//no decRef is needed
			return false;
		}
		else
		{//If none of the above apply, convert both to primitives with no hint
			asAtom val1p;
			val1->toPrimitive(val1p,NO_HINT);
			asAtom val2p;
			val2->toPrimitive(val2p,NO_HINT);
			if(val1p.is<ASString>() || val2p.is<ASString>())
			{//If one is String, convert both to strings and concat
				string a(val1p.toString(sys).raw_buf());
				string b(val2p.toString(sys).raw_buf());
				LOG_CALL("add " << a << '+' << b);
				uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_s(sys,a+b))|ATOM_STRINGPTR;
			}
			else
			{//Convert both to numbers and add
				number_t num1=val1p.AVM1toNumber(sys->mainClip->version);
				number_t num2=val2p.AVM1toNumber(sys->mainClip->version);
				LOG_CALL("addN " << num1 << '+' << num2);
				number_t result = num1 + num2;
				setNumber(sys,result);
			}
		}
	}
	return true;
}

void asAtom::setFunction(ASObject *obj, ASObject *closure)
{
	// type may be T_CLASS or T_FUNCTION
	if (closure &&obj->is<IFunction>())
	{
		closure->incRef();
		uintval = (LIGHTSPARK_ATOM_VALTYPE)(obj->as<IFunction>()->bind(_MR(closure)))|ATOM_OBJECTPTR;
	}
	else
	{
		obj->incRef();
		uintval = (LIGHTSPARK_ATOM_VALTYPE)(obj)|ATOM_OBJECTPTR;
	}
}
/* implements ecma3's ToBoolean() operation, see section 9.2, but returns the value instead of an Boolean object */
bool asAtom::Boolean_concrete()
{
	switch(uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
				case ATOMTYPE_UNDEFINED_BIT:
					return false;
				case ATOMTYPE_BOOL_BIT:
					return (uintval&0x80)>>7;
				default:
					return false;
			}
			break;
		}
		case ATOM_NUMBERPTR:
			return toNumber() != 0.0 && !std::isnan(toNumber());
		case ATOM_INTEGER:
			return (intval>>3) != 0;
		case ATOM_UINTEGER:
			return (uintval>>3) != 0;
		case ATOM_STRINGID:
			return (uintval>>3) != BUILTIN_STRINGS::EMPTY;
		default:
			assert(getObject());
			return lightspark::Boolean_concrete(getObject());
	}
}

void lightspark::asAtom::setNumber(SystemState* sys,number_t val)
{
	uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_d(sys,val))|ATOM_NUMBERPTR;
}

void lightspark::asAtom::replace(ASObject *obj)
{
	assert(((LIGHTSPARK_ATOM_VALTYPE)obj) % 8 == 0);
	switch(obj->getObjectType())
	{
		case T_INVALID:
			uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL;
			break;
		case T_UNDEFINED:
			uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_UNDEFINED_BIT;
			break;
		case T_NULL:
			uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_NULL_BIT;
			break;
		case T_BOOLEAN:
			uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_BOOL_BIT | (obj->toInt() ? 0x80: 0);
			break;
		case T_NUMBER:
			uintval=ATOM_NUMBERPTR | (LIGHTSPARK_ATOM_VALTYPE)obj;
			break;
		case T_UINTEGER:
		case T_INTEGER:
			uintval=ATOM_U_INTEGERPTR | (LIGHTSPARK_ATOM_VALTYPE)obj;
			break;
		case T_STRING:
			uintval=ATOM_STRINGPTR | (LIGHTSPARK_ATOM_VALTYPE)obj;
			break;
		default:
			uintval=ATOM_OBJECTPTR | (LIGHTSPARK_ATOM_VALTYPE)obj;
			break;
	}
}

TRISTATE asAtom::isLess(SystemState *sys, asAtom &v2)
{
	switch(uintval&0x7)
	{
		case ATOM_INTEGER:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return ((intval>>3) < (v2.intval>>3))?TTRUE:TFALSE;
				case ATOM_UINTEGER:
					return ((intval>>3) < 0 || ((uint32_t)(intval>>3)) < (v2.uintval>>3))?TTRUE:TFALSE;
				case ATOM_NUMBERPTR:
					if(std::isnan(v2.toNumber()))
						return TUNDEFINED;
					return ((intval>>3) < v2.toNumber())?TTRUE:TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return ((intval>>3) < 0)?TTRUE:TFALSE;
						case ATOMTYPE_UNDEFINED_BIT:
							return TUNDEFINED;
						case ATOMTYPE_BOOL_BIT:
							return ((intval>>3) < (int32_t)((v2.uintval&0x80)>>7))?TTRUE:TFALSE;
						default: // INVALID
							return TUNDEFINED;
					}
				}
				default:
					return ((intval>>3) < v2.toInt())?TTRUE:TFALSE;
			}
			break;
		}
		case ATOM_UINTEGER:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return ((v2.intval>>3) > 0 && ((uintval>>3) < (uint32_t)(v2.intval>>3)))?TTRUE:TFALSE;
				case ATOM_UINTEGER:
					return ((uintval>>3) < (v2.uintval>>3))?TTRUE:TFALSE;
				case ATOM_NUMBERPTR:
					if(std::isnan(v2.toNumber()))
						return TUNDEFINED;
					return ((uintval>>3) < v2.toNumber())?TTRUE:TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return TFALSE;
						case ATOMTYPE_UNDEFINED_BIT:
							return TUNDEFINED;
						case ATOMTYPE_BOOL_BIT:
							return ((uintval>>3) < ((v2.uintval&0x80)>>7))?TTRUE:TFALSE;
						default: // INVALID
							return TUNDEFINED;
					}
				}
				default:
					return ((uintval>>3) < v2.toUInt())?TTRUE:TFALSE;
			}
			break;
		}
		case ATOM_NUMBERPTR:
		{
			if(std::isnan(toNumber()))
				return TUNDEFINED;
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return (toNumber() < (v2.intval>>3))?TTRUE:TFALSE;
				case ATOM_UINTEGER:
					return (toNumber() < (v2.uintval>>3))?TTRUE:TFALSE;
				case ATOM_NUMBERPTR:
					if(std::isnan(v2.toNumber()))
						return TUNDEFINED;
					return (toNumber() < v2.toNumber())?TTRUE:TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return (toNumber() < 0)?TTRUE:TFALSE;
						case ATOMTYPE_UNDEFINED_BIT:
							return TUNDEFINED;
						case ATOMTYPE_BOOL_BIT:
							return (toNumber() < v2.toInt())?TTRUE:TFALSE;
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
			switch (uintval&0x70)
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
							if(std::isnan(v2.toNumber()))
								return TUNDEFINED;
							return (0 < v2.toNumber())?TTRUE:TFALSE;
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
							return this->toObject(sys)->isLess(v2.getObject());
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
							return ((int32_t)(uintval&0x80)>>7 < (v2.intval>>3))?TTRUE:TFALSE;
						case ATOM_UINTEGER:
							return ((uintval&0x80)>>7 < (v2.uintval>>3))?TTRUE:TFALSE;
						case ATOM_NUMBERPTR:
							if(std::isnan(v2.toNumber()))
								return TUNDEFINED;
							return ((uintval&0x80)>>7 < v2.toNumber())?TTRUE:TFALSE;
						case ATOM_INVALID_UNDEFINED_NULL_BOOL:
						{
							switch (v2.uintval&0x70)
							{
								case ATOMTYPE_NULL_BIT:
									return ((uintval&0x80)>>7)?TTRUE:TFALSE;
								case ATOMTYPE_UNDEFINED_BIT:
									return TUNDEFINED;
								case ATOMTYPE_BOOL_BIT:
									return ((uintval&0x80)>>7 < (v2.uintval&0x80)>>7)?TTRUE:TFALSE;
								default: // INVALID
									return TUNDEFINED;
							}
						}
						default:
							return this->toObject(sys)->isLess(v2.getObject());
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
					if (((uintval>>3) < BUILTIN_STRINGS_CHAR_MAX) && ((v2.uintval>>3) < BUILTIN_STRINGS_CHAR_MAX))
						return ((uintval>>3) < (v2.uintval>>3))?TTRUE:TFALSE;
					return toString(sys) < v2.toString(sys) ? TTRUE : TFALSE;
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
				case ATOM_STRINGPTR:
					return toString(sys) < v2.toString(sys) ? TTRUE : TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return (toNumber() < 0)?TTRUE:TFALSE;
						case ATOMTYPE_UNDEFINED_BIT:
							return TUNDEFINED;
						case ATOMTYPE_BOOL_BIT:
							return (toNumber() < v2.toInt())?TTRUE:TFALSE;
						default: // INVALID
							return TUNDEFINED;
					}
				}
				default:
				{
					TRISTATE ret = v2.getObject()->isLessAtom(*this);
					switch (ret)
					{
						case TTRUE:
							return TFALSE;
						case TFALSE:
							return isEqual(sys,v2) ? TFALSE : TTRUE;
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
					return toString(sys) < v2.toString(sys) ? TTRUE : TFALSE;
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
					return this->getObject()->isLessAtom(v2);
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	assert(getObject());
	assert(v2.getObject());
	return getObject()->isLess(v2.getObject());
}

bool asAtom::isEqual(SystemState *sys, asAtom &v2)
{
	switch (uintval&0x7)
	{
		case ATOM_INTEGER:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return (intval>>3)==v2.toInt();
				case ATOM_UINTEGER:
					return (intval>>3) >= 0 && (intval>>3)==v2.toInt();
				case ATOM_U_INTEGERPTR:
				case ATOM_NUMBERPTR:
					return (intval>>3)==v2.toNumber();
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return false;
						case ATOMTYPE_UNDEFINED_BIT:
							return false;
						case ATOMTYPE_BOOL_BIT:
							return (intval>>3)==v2.toInt();
						default: // INVALID
							return false;
					}
				}
				case ATOM_STRINGID:
				case ATOM_STRINGPTR:
					return (intval>>3)==v2.toNumber();
				default:
					return (intval>>3)==v2.toInt();
			}
			break;
		}
		case ATOM_UINTEGER:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
					return (v2.intval>>3) >= 0 && (uintval>>3)==v2.toUInt();
				case ATOM_UINTEGER:
				case ATOM_NUMBERPTR:
				case ATOM_U_INTEGERPTR:
					return (uintval>>3)==v2.toUInt();
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return false;
						case ATOMTYPE_UNDEFINED_BIT:
							return false;
						case ATOMTYPE_BOOL_BIT:
							return (uintval>>3)==v2.toUInt();
						default: // INVALID
							return false;
					}
				}
				case ATOM_STRINGID:
				case ATOM_STRINGPTR:
					return (uintval>>3)==v2.toNumber();
				default:
					return (uintval>>3)==v2.toUInt();
			}
			break;
		}
		case ATOM_NUMBERPTR:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
					return toNumber()==v2.toNumber();
				case ATOM_NUMBERPTR:
					return toNumber() == v2.toNumber();
				case ATOM_STRINGID:
				case ATOM_STRINGPTR:
					return toNumber()==v2.toNumber();
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return false;
						case ATOMTYPE_UNDEFINED_BIT:
							return false;
						case ATOMTYPE_BOOL_BIT:
							return toNumber()==v2.toNumber();
						default: // INVALID
							return false;
					}
				}
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
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
					return getObject()->toInt64()==v2.toInt64();
				case ATOM_NUMBERPTR:
					return getObject()->toInt64()==v2.toNumber();
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0x70)
					{
						case ATOMTYPE_NULL_BIT:
							return false;
						case ATOMTYPE_UNDEFINED_BIT:
							return false;
						case ATOMTYPE_BOOL_BIT:
							return getObject()->toInt64()==v2.toInt64();
						default: // INVALID
							return false;
					}
				}
				case ATOM_STRINGID:
				case ATOM_STRINGPTR:
					return getObject()->toInt64()==v2.toNumber();
				default:
					return getObject()->toInt64()==v2.toInt64();
			}
			break;
		}
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0x70)
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
							return v2.toObject(sys)->isEqual(this->toObject(sys));
					}
				}
				case ATOMTYPE_BOOL_BIT:
					switch (v2.uintval&0x7)
					{
						case ATOM_STRINGID:
							return (bool)((uintval&0x80)>>7)==v2.toNumber();
						case ATOM_INTEGER:
						case ATOM_UINTEGER:
						case ATOM_NUMBERPTR:
							return (bool)((uintval&0x80)>>7)==v2.toNumber();
						case ATOM_INVALID_UNDEFINED_NULL_BOOL:
						{
							switch (v2.uintval&0x70)
							{
								case ATOMTYPE_NULL_BIT:
								case ATOMTYPE_UNDEFINED_BIT:
									return false;
								case ATOMTYPE_BOOL_BIT:
									return (uintval&0x80)>>7==(v2.uintval&0x80)>>7;
								default: // INVALID
									return false;
							}
						}
						default:
							return v2.toObject(sys)->isEqual(this->toObject(sys));
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
							return (bool)((v2.uintval&0x80)>>7)==toNumber();
						default: // INVALID
							return false;
					}
					break;
				}
				case ATOM_STRINGID:
					return (v2.uintval>>3) == (uintval>>3);
				default:
					return v2.isEqual(sys,*this);
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
							return (bool)((v2.uintval&0x80)>>7)==toNumber();
						default: // INVALID
							return false;
					}
					break;
				}
				case ATOM_STRINGID:
					return this->toString(sys) == v2.toString(sys);
				default:
					break;
			}
			break;
		}
		case ATOM_OBJECTPTR:
		{
			if (this->is<IFunction>())
			{
				if (v2.is<IFunction>())
					return functioncompare(sys,v2);
				else
					return false;
			}
			switch (v2.uintval&0x7)
			{
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
					return getObject()->isEqual(v2.toObject(sys));
				case ATOM_STRINGID:
					// TODO avoid creating of string object here
					return getObject()->isEqual(v2.toObject(sys));
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
					return v2.isEqual(sys,*this);
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	assert(getObject());
	assert(v2.getObject());
	return getObject()->isEqual(v2.getObject());
}

ASObject *asAtom::toObject(SystemState *sys, bool isconstant)
{
	if (isObject())
	{
		assert(getObject() && getObject()->getRefCount() >= 1);
		return getObject();
	}
	switch(uintval&0x7)
	{
		case ATOM_INTEGER:
			// ints are internally treated as numbers, so create a Number instance
			uintval = ((LIGHTSPARK_ATOM_VALTYPE)abstract_di(sys,(intval>>3)))|ATOM_U_INTEGERPTR;
			break;
		case ATOM_UINTEGER:
			// uints are internally treated as numbers, so create a Number instance
			uintval = ((LIGHTSPARK_ATOM_VALTYPE)abstract_di(sys,(uintval>>3)))|ATOM_U_INTEGERPTR;
			break;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					return abstract_null(sys);
				case ATOMTYPE_UNDEFINED_BIT:
					return abstract_undefined(sys);
				case ATOMTYPE_BOOL_BIT:
					return abstract_b(sys,(uintval & 0x80)>>7);
				default:
					break;
			}
			break;
		}
		case ATOM_STRINGID:
			uintval = ((LIGHTSPARK_ATOM_VALTYPE)abstract_s(sys,(uintval>>3))) | ATOM_STRINGPTR ;
			break;
		default:
			throw RunTimeException("calling toObject on invalid asAtom, should not happen");
			break;
	}
	if (isconstant)
		getObject()->setConstant();
	return getObject();
}
