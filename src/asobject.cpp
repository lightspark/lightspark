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
#include <limits>
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/argconv.h"
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
	else
	{
		assert(false);
	}
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
	name.ns.clear();
	name.ns.reserve(this->proxyMultiName->ns.size());
	for(unsigned int i=0;i<this->proxyMultiName->ns.size();i++)
	{
		if (this->proxyMultiName->ns[i].hasEmptyName())
			name.hasEmptyNS = true;
		if (this->proxyMultiName->ns[i].hasBuiltinName())
			name.hasBuiltinNS = true;
		name.ns.push_back(this->proxyMultiName->ns[i]);
	}
}

void ASObject::dumpVariables() const
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
		//everything else is an Object regarding to the spec
		return toPrimitive(STRING_HINT)->toString();
	}
}

tiny_string ASObject::toLocaleString()
{
	asAtom str = executeASMethod("toLocaleString", {""}, NULL, 0);
	if (str.type == T_INVALID)
		return "";
	return str.toString();
}

TRISTATE ASObject::isLess(ASObject* r)
{
	check();
	return toPrimitive()->isLess(r);
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

asAtom ASObject::nextName(uint32_t index)
{
	assert_and_throw(implEnable);

	const tiny_string& name = getNameAt(index-1);
	// not mentioned in the specs, but Adobe seems to convert string names to Integers, if possible
	if (Array::isIntegerWithoutLeadingZeros(name))
		return asAtom(Integer::stringToASInteger(name.raw_buf(), 0));
	return asAtom::fromObject(abstract_s(getSystemState(),name));
}

asAtom ASObject::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);

	return getValueAt(index-1);
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
	check();
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
			_R<ASObject> primitive(toPrimitive());
			return primitive->isEqual(r);
		}
		case T_BOOLEAN:
		{
			_R<ASObject> primitive(toPrimitive());
			return primitive->toNumber()==r->toNumber();
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

	LOG_CALL(_("Equal comparison between type ")<<getObjectType()<< _(" and type ") << r->getObjectType());
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
	return toPrimitive()->toInt();
}

int64_t ASObject::toInt64()
{
	return toPrimitive()->toInt64();
}

/* Implements ECMA's ToPrimitive (9.1) and [[DefaultValue]] (8.6.2.6) */
_R<ASObject> ASObject::toPrimitive(TP_HINT hint)
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
		return _MR(this);
	}

	/* for HINT_STRING evaluate first toString, then valueOf
	 * for HINT_NUMBER do it the other way around */
	if(hint == STRING_HINT && has_toString())
	{
		_R<ASObject> ret = call_toString();
		if(ret->isPrimitive())
			return ret;
	}
	if(has_valueOf())
	{
		_R<ASObject> ret = call_valueOf();
		if(ret->isPrimitive())
			return ret;
	}
	if(hint != STRING_HINT && has_toString())
	{
		_R<ASObject> ret = call_toString();
		if(ret->isPrimitive())
			return ret;
	}

	throwError<TypeError>(kConvertToPrimitiveError,this->getClassName());
	return _MR((ASObject*)NULL);
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
_R<ASObject> ASObject::call_valueOf()
{
	multiname valueOfName(NULL);
	valueOfName.name_type=multiname::NAME_STRING;
	valueOfName.name_s_id=BUILTIN_STRINGS::STRING_VALUEOF;
	valueOfName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	valueOfName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	valueOfName.isAttribute = false;
	assert_and_throw(hasPropertyByMultiname(valueOfName, true, true));

	asAtom o=getVariableByMultiname(valueOfName,SKIP_IMPL);
	if (o.type != T_FUNCTION)
		throwError<TypeError>(kCallOfNonFunctionError, valueOfName.normalizedNameUnresolved(getSystemState()));

	asAtom v =asAtom::fromObject(this);
	asAtom ret=o.callFunction(v,NULL,0);
	return _MR(ret.toObject(getSystemState()));
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
_R<ASObject> ASObject::call_toString()
{
	multiname toStringName(NULL);
	toStringName.name_type=multiname::NAME_STRING;
	toStringName.name_s_id=BUILTIN_STRINGS::STRING_TOSTRING;
	toStringName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	toStringName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	toStringName.isAttribute = false;
	assert(ASObject::hasPropertyByMultiname(toStringName, true, true));

	asAtom o=getVariableByMultiname(toStringName,SKIP_IMPL);
	if (o.type != T_FUNCTION)
		throwError<TypeError>(kCallOfNonFunctionError, toStringName.normalizedNameUnresolved(getSystemState()));

	asAtom v =asAtom::fromObject(this);
	asAtom ret=o.callFunction(v,NULL,0);
	return _MR(ret.toObject(getSystemState()));
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

	asAtom o=getVariableByMultiname(toJSONName,SKIP_IMPL);
	if (o.type != T_FUNCTION)
		return res;
	asAtom v=asAtom::fromObject(this);
	asAtom ret=o.callFunction(v,NULL,0);
	if (ret.type == T_STRING)
	{
		res += "\"";
		res += ret.toString();
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
variables_map::variables_map(MemoryAccount *m)
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
			obj->setVar(asAtom::fromObject(o),getSystemState());
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

	assert(f.type == T_FUNCTION);
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

	assert(obj->getter.type == T_INVALID && obj->setter.type == T_INVALID && obj->var.type!=T_INVALID);
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
		if(!(ret->setter.type != T_INVALID || ret->var.type != T_INVALID))
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


void ASObject::setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, Class_base* cls)
{
	check();
	assert(!cls || classdef->isSubClass(cls));
	//NOTE: we assume that [gs]etSuper and [sg]etProperty correctly manipulate the cur_level (for getActualClass)
	bool has_getter=false;
	variable* obj=findSettable(name, &has_getter);

	if (obj && (obj->kind == CONSTANT_TRAIT && allowConst==CONST_NOT_ALLOWED))
	{
		throwError<ReferenceError>(kConstWriteError, name.normalizedNameUnresolved(getSystemState()), classdef->as<Class_base>()->getQualifiedClassName());
	}
	if(!obj && cls)
	{
		//Look for borrowed traits before
		//It's valid to override only a getter, so keep
		//looking for a settable even if a super class sets
		//has_getter to true.
		obj=cls->findBorrowedSettable(name,&has_getter);
		if(obj && (obj->issealed || cls->isFinal || cls->isSealed) && obj->setter.type == T_INVALID)
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
		    ((protoObj->var.type == T_FUNCTION) ||
		     protoObj->setter.type != T_INVALID))
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
			const Type* type = Type::getTypeFromMultiname(&name,getVm(getSystemState())->currentCallContext->context);
			if (type)
				throwError<ReferenceError>(kConstWriteError, name.normalizedNameUnresolved(getSystemState()), cls ? cls->getQualifiedClassName() : "");
			throwError<ReferenceError>(kWriteSealedError, name.normalizedNameUnresolved(getSystemState()), cls->getQualifiedClassName());
		}

		//Create a new dynamic variable
		obj=Variables.findObjVar(getSystemState(),name,DYNAMIC_TRAIT,DYNAMIC_TRAIT);
		++varcount;
	}

	if(obj->setter.type != T_INVALID)
	{
		//Call the setter
		LOG_CALL(_("Calling the setter"));
		//One argument can be passed without creating an array
		ASObject* target=this;
		asAtom* arg1 = &o;

		asAtom v =asAtom::fromObject(target);
		asAtom ret= obj->setter.callFunction(v,arg1,1);
		assert_and_throw(ret.type == T_UNDEFINED);
		LOG_CALL(_("End of setter"));
	}
	else
	{
		assert_and_throw(obj->getter.type == T_INVALID);
		obj->setVar(o,getSystemState());
	}
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
void ASObject::setVariableAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom& o, TRAIT_KIND traitKind, bool isEnumerable)
{
	setVariableAtomByQName(getSystemState()->getUniqueStringId(name), ns, o, traitKind,isEnumerable);
}
void ASObject::setVariableAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom& o, TRAIT_KIND traitKind, bool isEnumerable)
{
	assert_and_throw(Variables.findObjVar(nameId,ns,NO_CREATE_TRAIT,traitKind)==NULL);
	variable* obj=Variables.findObjVar(nameId,ns,traitKind,traitKind);
	obj->setVar(o,getSystemState());
	obj->isenumerable=isEnumerable;
	++varcount;
}

void ASObject::initializeVariableByMultiname(const multiname& name, ASObject* o, multiname* typemname,
		ABCContext* context, TRAIT_KIND traitKind, uint32_t slot_id, bool isenumerable)
{
	check();
	Variables.initializeVar(name, o, typemname, context, traitKind,this,slot_id,isenumerable);
	++varcount;
}

variable::variable(TRAIT_KIND _k, asAtom _v, multiname* _t, const Type* _type, const nsNameAndKind& _ns, bool _isenumerable)
		: var(_v),typeUnion(NULL),ns(_ns),kind(_k),traitState(NO_STATE),isenumerable(_isenumerable),issealed(false)
{
	if(_type)
	{
		//The type is known, use it instead of the typemname
		type=_type;
		traitState=TYPE_RESOLVED;
	}
	else
	{
		traitTypemname=_t;
	}
}

void variable::setVar(asAtom v, SystemState* sys)
{
	//Resolve the typename if we have one
	//currentCallContext may be NULL when inserting legacy
	//children, which is done outisde any ABC context
	if(!(traitState&TYPE_RESOLVED) && traitTypemname && getVm(sys)->currentCallContext)
	{
		type = Type::getTypeFromMultiname(traitTypemname, getVm(sys)->currentCallContext->context);
		assert(type);
		traitState=TYPE_RESOLVED;
	}
	if((traitState&TYPE_RESOLVED) && type)
		v = type->coerce(sys,v);
	ASATOM_DECREF(var);
	var=v;
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
	uint32_t name=mname.normalizedNameId(sys);
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
			if(ret->second.kind & traitKinds)
				return &ret->second;
			else
				return NULL;
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
		if(!mname.hasEmptyNS)
			throwError<ReferenceError>(kWriteSealedError, mname.normalizedNameUnresolved(sys), "" /* TODO: class name */);
		var_iterator inserted=Variables.insert(Variables.cbegin(),
			make_pair(name,variable(createKind,nsNameAndKind())));
		return &inserted->second;
	}
	assert(mname.ns.size() == 1);
	var_iterator inserted=Variables.insert(Variables.cbegin(),
		make_pair(name,variable(createKind,mname.ns[0])));
	return &inserted->second;
}

void variables_map::initializeVar(const multiname& mname, ASObject* obj, multiname* typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj, uint32_t slot_id,bool isenumerable)
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
		if (obj == NULL) // create dynamic object
		{
			value = asAtom::undefinedAtom;
		}
		else
		{
			assert_and_throw(obj->is<Null>() || obj->is<Undefined>());
			value = asAtom::fromObject(obj);
			if(obj->is<Undefined>())
			{
				//Casting undefined to an object (of unknown class)
				//results in Null
				value = asAtom::nullAtom;
			}
		}
	}
	else
	{
		if (obj == NULL) // create dynamic object
		{
			if(mainObj->is<Class_base>() 
				&& mainObj->as<Class_base>()->class_name.nameId == typemname->normalizedNameId(mainObj->getSystemState())
				&& mainObj->as<Class_base>()->class_name.nsStringId == typemname->ns[0].nsNameId
				&& typemname->ns[0].kind == NAMESPACE)
			{
				// avoid recursive construction
				value = asAtom(T_UNDEFINED);
			}
			else
				value = type->coerce(mainObj->getSystemState(),asAtom::undefinedAtom);
		}
		else
			value = asAtom::fromObject(obj);

		if (typemname->isStatic && typemname->cachedType == NULL)
			typemname->cachedType = type;
	}
	assert(traitKind==DECLARED_TRAIT || traitKind==CONSTANT_TRAIT || traitKind == INSTANCE_TRAIT);

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
		return args[0];
	}
	else
		return asAtom::fromObject(Class<ASObject>::getInstanceS(getSys()));
}

ASFUNCTIONBODY(ASObject,_toString)
{
	tiny_string ret;
	if (obj->is<Class_base>())
	{
		ret="[object ";
		ret+=obj->getSystemState()->getStringFromUniqueId(static_cast<Class_base*>(obj)->class_name.nameId);
		ret+="]";
	}
	else if(obj->is<IFunction>())
	{
		// ECMA spec 15.3.4.2 says that toString on a function object is implementation dependent
		// adobe player returns "[object Function-46]", so we do the same
		ret="[object ";
		ret+="Function-46";
		ret+="]";
	}
	else if(obj->getClass())
	{
		ret="[object ";
		ret+=obj->getSystemState()->getStringFromUniqueId(obj->getClass()->class_name.nameId);
		ret+="]";
	}
	else
		ret="[object Object]";

	return abstract_s(obj->getSystemState(),ret);
}

ASFUNCTIONBODY(ASObject,_toLocaleString)
{
	return _toString(obj,args,argslen);
}

ASFUNCTIONBODY(ASObject,hasOwnProperty)
{
	assert_and_throw(argslen==1);
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=args[0]->toStringId();
	name.ns.emplace_back(obj->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	bool ret=obj->hasPropertyByMultiname(name, true, false);
	return abstract_b(obj->getSystemState(),ret);
}

ASFUNCTIONBODY(ASObject,valueOf)
{
	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(ASObject,isPrototypeOf)
{
	assert_and_throw(argslen==1);
	bool ret =false;
	Class_base* cls = args[0]->getClass();
	
	while (cls != NULL)
	{
		if (cls->prototype->getObj() == obj)
		{
			ret = true;
			break;
		}
		Class_base* clsparent = cls->prototype->getObj()->getClass();
		if (clsparent == cls)
			break;
		cls = clsparent;
	}
	return abstract_b(obj->getSystemState(),ret);
}

ASFUNCTIONBODY(ASObject,propertyIsEnumerable)
{
	if (argslen == 0)
		return abstract_b(obj->getSystemState(),false);
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=args[0]->toStringId();
	name.ns.emplace_back(obj->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	if (obj->is<Array>()) // propertyIsEnumerable(index) isn't mentioned in the ECMA specs but is tested for
	{
		Array* a = static_cast<Array*>(obj);
		unsigned int index = 0;
		if (a->isValidMultiname(obj->getSystemState(),name,index))
		{
			return abstract_b(obj->getSystemState(),index < (unsigned int)a->size());
		}
	}
	variable* v = obj->Variables.findObjVar(obj->getSystemState(),name, NO_CREATE_TRAIT,DYNAMIC_TRAIT|DECLARED_TRAIT);
	if (v)
		return abstract_b(obj->getSystemState(),v->isenumerable);
	if (obj->hasPropertyByMultiname(name,true,false))
		return abstract_b(obj->getSystemState(),true);
	return abstract_b(obj->getSystemState(),false);
}
ASFUNCTIONBODY(ASObject,setPropertyIsEnumerable)
{
	tiny_string propname;
	bool isEnum;
	ARG_UNPACK(propname) (isEnum, true);
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=args[0]->toStringId();
	name.ns.emplace_back(obj->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	obj->setIsEnumerable(name, isEnum);
	return NULL;
}
void ASObject::setIsEnumerable(const multiname &name, bool isEnum)
{
	variable* v = Variables.findObjVar(getSystemState(),name, NO_CREATE_TRAIT,DYNAMIC_TRAIT);
	if (v)
		v->isenumerable = isEnum;
}

ASFUNCTIONBODY(ASObject,_constructor)
{
	return NULL;
}

ASFUNCTIONBODY(ASObject,_constructorNotInstantiatable)
{
	throwError<ArgumentError>(kCantInstantiateError, obj->getClassName());
	return NULL;
}

void ASObject::initSlot(unsigned int n, const multiname& name)
{
	Variables.initSlot(n,name.name_s_id,name.ns[0]);
}
int32_t ASObject::getVariableByMultiname_i(const multiname& name)
{
	check();

	asAtom ret=getVariableByMultiname(name);
	assert_and_throw(ret.type != T_INVALID);
	return ret.toInt();
}

variable* ASObject::findVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls, uint32_t *nsRealID)
{
	//Get from the current object without considering borrowed properties
	variable* obj=varcount ? Variables.findObjVar(getSystemState(),name,name.hasEmptyNS ? DECLARED_TRAIT|DYNAMIC_TRAIT : DECLARED_TRAIT,nsRealID):NULL;
	if(obj)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(obj->getter.type != T_INVALID|| obj->var.type != T_INVALID))
			obj=NULL;
	}

	if(!obj)
	{
		//Look for borrowed traits before
		if (cls)
		{
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
						if(!(obj->getter.type != T_INVALID || obj->var.type != T_INVALID))
							obj=NULL;
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

asAtom ASObject::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls)
{
	check();
	assert(!cls || classdef->isSubClass(cls));
	uint32_t nsRealId;

	variable* obj=varcount ? Variables.findObjVar(getSystemState(),name,name.hasEmptyNS ? DECLARED_TRAIT|DYNAMIC_TRAIT : DECLARED_TRAIT,&nsRealId):NULL;
	if(obj)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(obj->getter.type != T_INVALID || obj->var.type != T_INVALID))
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
						if(!(obj->getter.type != T_INVALID || obj->var.type != T_INVALID))
							obj=NULL;
					}
					if(obj)
						break;
					proto = proto->prevPrototype.getPtr();
				}
			}
		}
		if(!obj)
			return asAtom::invalidAtom;
	}


	if ( this->is<Class_base>() && obj->kind == INSTANCE_TRAIT &&
			(obj->var.type == T_INVALID || !obj->var.isConstructed() ||
			 obj->var.type == T_UNDEFINED ||
			 obj->var.type == T_NULL))
	{
		if (getSystemState()->getNamespaceFromUniqueId(nsRealId).kind != STATIC_PROTECTED_NAMESPACE)
			throwError<TypeError>(kCallOfNonFunctionError,name.normalizedNameUnresolved(getSystemState()));
	}

	if(obj->getter.type != T_INVALID)
	{
		//Call the getter
		LOG_CALL("Calling the getter for " << name);
		ASObject* target=NULL;
		if (!obj->getter.isBound())
			target=this;
		
		asAtom v =asAtom::fromObject(target);
		asAtom ret=obj->getter.callFunction(v,NULL,0);
		LOG_CALL(_("End of getter"));
		// No incRef because ret is a new instance
		return ret;
	}
	else
	{
		assert_and_throw(obj->setter.type == T_INVALID);
		if(obj->var.type==T_FUNCTION && obj->var.getObject()->as<IFunction>()->isMethod())
		{
			if (obj->var.isBound())
			{
				LOG_CALL("function " << name << " is already bound to "<<obj->var.toDebugString() );
				return obj->var;
			}
			else
			{
				LOG_CALL("Attaching this " << this->toDebugString() << " to function " << name);
				return asAtom::fromFunction(obj->var.getObject(),this);
			}
		}
		return obj->var;
	}
}

asAtom ASObject::getVariableByMultiname(const tiny_string& name, std::list<tiny_string> namespaces)
{
	multiname varName(NULL);
	varName.name_type=multiname::NAME_STRING;
	varName.name_s_id=getSystemState()->getUniqueStringId(name);
	varName.hasEmptyNS = false;
	varName.hasBuiltinNS = false;
	for (auto ns=namespaces.begin(); ns!=namespaces.end(); ns++)
	{
		nsNameAndKind newns(getSystemState(),*ns,NAMESPACE);
		varName.ns.push_back(newns);
		if (newns.hasEmptyName())
			varName.hasEmptyNS = true;
		if (newns.hasBuiltinName())
			varName.hasBuiltinNS = true;
	}
	varName.isAttribute = false;

	return getVariableByMultiname(varName,SKIP_IMPL);
}

asAtom ASObject::executeASMethod(const tiny_string& methodName,
					std::list<tiny_string> namespaces,
					asAtom* args,
					uint32_t num_args)
{
	asAtom o = getVariableByMultiname(methodName, namespaces);
	if (o.type != T_FUNCTION)
		throwError<TypeError>(kCallOfNonFunctionError, methodName);
	asAtom v =asAtom::fromObject(this);
	return o.callFunction(v,args,num_args);
}

void ASObject::check() const
{
	//Put here a bunch of safety check on the object
#ifndef NDEBUG
	assert(getRefCount()>0);
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
			if(it->second.var.type == T_INVALID && next->second.var.type == T_INVALID)
				continue;

			if(it->second.var.type == T_INVALID || next->second.var.type == T_INVALID)
			{
				LOG(LOG_INFO, it->first << " " << it->second.ns);
				LOG(LOG_INFO, it->second.var.type << ' ' << it->second.setter.type << ' ' << it->second.getter.type);
				LOG(LOG_INFO, next->second.var.type << ' ' << next->second.setter.type << ' ' << next->second.getter.type);
				abort();
			}

			if(it->second.var.type!=T_FUNCTION || next->second.var.type!=T_FUNCTION)
			{
				LOG(LOG_INFO, it->first);
				abort();
			}
		}
	}
#endif
}

void variables_map::dumpVariables() const
{
	const_var_iterator it=Variables.cbegin();
	for(;it!=Variables.cend();++it)
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
			it->second.var.type << ' ' << it->second.setter.type << ' ' << it->second.getter.type);
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
		ASATOM_DECREF(it->second.var);
		ASATOM_DECREF(it->second.setter);
		ASATOM_DECREF(it->second.getter);
		it = Variables.erase(it);
	}
}

ASObject::ASObject(Class_base* c,SWFOBJECT_TYPE t,CLASS_SUBTYPE st):objfreelist(c && c->isReusable ? c->freelist : NULL),Variables((c)?c->memoryAccount:NULL),varcount(0),classdef(c),proxyMultiName(NULL),sys(c?c->sys:NULL),
	stringId(UINT32_MAX),type(t),subtype(st),traitsInitialized(false),constructIndicator(false),constructorCallComplete(false),implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif
}

ASObject::ASObject(const ASObject& o):objfreelist(o.classdef && o.classdef->isReusable ? o.classdef->freelist : NULL),Variables((o.classdef)?o.classdef->memoryAccount:NULL),varcount(0),classdef(NULL),proxyMultiName(NULL),sys(o.classdef? o.classdef->sys : NULL),
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
		dodestruct = !objfreelist->pushObjectToFreeList(this);
	}
	if (dodestruct)
	{
		finalize();
	}
	return dodestruct;
}

void variables_map::initSlot(unsigned int n, uint32_t nameId, const nsNameAndKind& ns)
{
	if(n>slots_vars.size())
		slots_vars.resize(n+8);

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

asAtom ASObject::getValueAt(int index)
{
	variable* obj=Variables.getValueAt(index);
	assert_and_throw(obj);
	if(obj->getter.type != T_INVALID)
	{
		//Call the getter
		LOG_CALL(_("Calling the getter"));
		asAtom v=asAtom::fromObject(this);
		asAtom ret = obj->getter.callFunction(v,NULL,0);
		LOG_CALL(_("End of getter"));
		return ret;
	}
	else
	{
		ASATOM_INCREF(obj->var);
		return obj->var;
	}
}

tiny_string variables_map::getNameAt(SystemState *sys, unsigned int index) const
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
		return sys->getStringFromUniqueId(it->first);
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
				std::map<const Class_base*, uint32_t> traitsMap)
{
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

		asAtom o=getVariableByMultiname(writeExternalName,SKIP_IMPL);
		assert_and_throw(o.type==T_FUNCTION);
		out->incRef();
		asAtom tmpArg[1] = { asAtom::fromObject(out) };
		asAtom v=asAtom::fromObject(this);
		o.callFunction(v, tmpArg, 1);
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
		root.append_attribute("name").set_value(prot->getQualifiedClassName().raw_buf());
		if(prot->super)
			root.append_attribute("base").set_value(prot->super->getQualifiedClassName().raw_buf());
	}
	bool isDynamic = classdef && !classdef->isSealed;
	root.append_attribute("isDynamic").set_value(isDynamic?"true":"false");
	bool isFinal = !classdef || classdef->isFinal;
	root.append_attribute("isFinal").set_value(isFinal?"true":"false");
	root.append_attribute("isStatic").set_value("false");

	if(prot)
		prot->describeInstance(root);

	// TODO: undocumented constructor node
	//LOG(LOG_INFO,"describeType:"<< Class<XML>::getInstanceS(root)->toXMLString_internal());

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
				LOG(LOG_ERROR,"tojson");
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
			if(varIt->second.ns.hasEmptyName() && (varIt->second.getter.type != T_INVALID || varIt->second.var.type!= T_INVALID))
			{
				ASObject* v = varIt->second.var.toObject(getSystemState());
				if (varIt->second.getter.type != T_INVALID)
				{
					asAtom t=asAtom::fromObject(this);
					v=varIt->second.getter.callFunction(t,NULL,0).toObject(getSystemState());
				}
				if(v->getObjectType() != T_UNDEFINED && varIt->second.isenumerable)
				{
					// check for cylic reference
					if (v->getObjectType() != T_UNDEFINED &&
						v->getObjectType() != T_NULL &&
						v->getObjectType() != T_BOOLEAN &&
						std::find(path.begin(),path.end(), v) != path.end())
						throwError<TypeError>(kJSONCyclicStructure);
		
					if (replacer.type != T_INVALID)
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
						
						params[0] = asAtom::fromObject(abstract_s(getSystemState(),getSystemState()->getStringFromUniqueId(varIt->first)));
						params[1] = asAtom::fromObject(v);
						ASATOM_INCREF(params[1]);
						asAtom funcret=replacer.callFunction(asAtom::nullAtom, params, 2);
						if (funcret.type != T_INVALID)
							res += funcret.toString();
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
	return (var && var->var.type != T_INVALID);
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
	obj->incRef();

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
	if(ret->setter.type != T_INVALID)
	{
		asAtom arg1= asAtom::fromObject(obj);
		asAtom v=asAtom::fromObject(this);
		ret->setter.callFunction(v,&arg1,1);
	}
	else
		ret->setVar(asAtom::fromObject(obj),getSystemState());
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

ASObject *asAtom::toObject(SystemState *sys)
{
	if (objval)
		return objval;
	switch(type)
	{
		case T_INTEGER:
			// ints are internally treated as numbers, so create a Number instance
			objval = abstract_di(sys,intval);
			break;
		case T_UINTEGER:
			// uints are internally treated as numbers, so create a Number instance
			objval = abstract_di(sys,uintval);
			break;
		case T_NUMBER:
			objval = abstract_d(sys,numberval);
			break;
		case T_BOOLEAN:
			return (boolval ? sys->getTrueRef(): sys->getFalseRef());
		case T_NULL:
			return sys->getNullRef();
		case T_UNDEFINED:
			return sys->getUndefinedRef();
		case T_INVALID:
			LOG(LOG_ERROR,"calling toObject on invalid asAtom, should not happen");
			return objval;
		default:
			break;
	}
	return objval;
}

asAtom asAtom::fromObject(ASObject *obj)
{
	asAtom a;
	if (obj)
		a.replace(obj);
	return a;
}

asAtom asAtom::fromFunction(ASObject *f, ASObject *closure)
{
	asAtom a;
	a.replace(f);
	a.closure_this = closure;
	return a;
}

asAtom asAtom::callFunction(asAtom &obj, asAtom *args, uint32_t num_args)
{
	assert_and_throw(type == T_FUNCTION);
		
	ASObject* closure = closure_this;
	if ((obj.type == T_FUNCTION && obj.closure_this) || obj.is<XML>() || obj.is<XMLList>())
		closure = NULL; // force use of function closure in call
	asAtom c;
	if(closure)
	{ /* closure_this can never been overriden */
		LOG_CALL(_("Calling with closure ") << toDebugString());
		c=asAtom::fromObject(closure);
	}
	else
		c = obj;
	ASATOM_INCREF(c);
	
	if (objval->is<SyntheticFunction>())
		return objval->as<SyntheticFunction>()->call(c, args, num_args);
	return objval->as<Function>()->call(c, args, num_args);
}

void asAtom::replace(ASObject *obj)
{
	assert(obj);
	type = obj->getObjectType();
	switch(type)
	{
		case T_INTEGER:
			intval = obj->as<Integer>()->val;
			break;
		case T_UINTEGER:
			uintval = obj->as<UInteger>()->val;
			break;
		case T_NUMBER:
			if (!obj->as<Number>()->isfloat)
			{ 
				if (obj->toInt64() > INT32_MIN && obj->toInt64()< INT32_MAX)
				{
					intval = obj->as<Number>()->toInt();
					type = T_INTEGER;
				}
				else if (obj->toInt64() > 0 && obj->toInt64()< UINT32_MAX)
				{
					uintval = obj->as<Number>()->toUInt();
					type = T_UINTEGER;
				}
				else
					numberval = obj->as<Number>()->toNumber();
			}
			else
				numberval = obj->as<Number>()->toNumber();
			break;
		case T_BOOLEAN:
			boolval = obj->as<Boolean>()->val;
			break;
		case T_FUNCTION:
			closure_this = NULL;
			break;
		default:
			break;
	}
	objval = obj;
}

void asAtom::decRef()
{
	if (objval && objval->decRef())
		objval = NULL;
}

std::string asAtom::toDebugString()
{
	switch(type)
	{
		case T_INTEGER:
			return Integer::toString(intval)+"i";
		case T_UINTEGER:
			return UInteger::toString(uintval)+"ui";
		case T_NUMBER:
			return Number::toString(numberval)+"d";
		case T_BOOLEAN:
			return Integer::toString(boolval)+"b";
		case T_NULL:
			return "Null";
		case T_UNDEFINED:
			return "Undefined";
		case T_INVALID:
			return "Invalid";
		case T_FUNCTION:
			assert(objval);
			if (closure_this)
				return objval->toDebugString()+ "(closure:"+closure_this->toDebugString()+")";
			return objval->toDebugString();
		default:
			assert(objval);
			return objval->toDebugString();
	}
}

void asAtom::applyProxyProperty(multiname &name)
{
	switch(type)
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_BOOLEAN:
		case T_NULL:
		case T_UNDEFINED:
		case T_INVALID:
			break;
		default:
			assert(objval);
			objval->applyProxyProperty(name);
			break;
	}
}
TRISTATE asAtom::isLess(SystemState* sys,asAtom &v2)
{
	switch (type)
	{
		case T_INTEGER:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return (intval < v2.intval)?TTRUE:TFALSE;
				case T_UINTEGER:
					return (intval < 0 || ((uint32_t)intval)  < v2.uintval)?TTRUE:TFALSE;
				case T_NUMBER:
					if(std::isnan(v2.numberval))
						return TUNDEFINED;
					return (intval < v2.numberval)?TTRUE:TFALSE;
				case T_BOOLEAN:
					return (intval < v2.toInt())?TTRUE:TFALSE;
				case T_UNDEFINED:
					return TUNDEFINED;
				case T_NULL:
					return (intval < 0)?TTRUE:TFALSE;
				default:
					break;
			}
			break;
		}
		case T_UINTEGER:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return (v2.intval > 0 && (uintval < (uint32_t)v2.intval))?TTRUE:TFALSE;
				case T_UINTEGER:
					return (uintval < v2.uintval)?TTRUE:TFALSE;
				case T_NUMBER:
					if(std::isnan(v2.numberval))
						return TUNDEFINED;
					return (uintval < v2.numberval)?TTRUE:TFALSE;
				case T_BOOLEAN:
					return (uintval < v2.toUInt())?TTRUE:TFALSE;
				case T_UNDEFINED:
					return TUNDEFINED;
				case T_NULL:
					return TFALSE;
				default:
					break;
			}
			break;
		}
		case T_NUMBER:
		{
			if(std::isnan(numberval))
				return TUNDEFINED;
			switch (v2.type)
			{
				case T_INTEGER:
					return (numberval  < v2.intval)?TTRUE:TFALSE;
				case T_UINTEGER:
					return (numberval < v2.uintval)?TTRUE:TFALSE;
				case T_NUMBER:
					if(std::isnan(v2.numberval))
						return TUNDEFINED;
					return (numberval < v2.numberval)?TTRUE:TFALSE;
				case T_BOOLEAN:
					return (numberval < v2.toInt())?TTRUE:TFALSE;
				case T_UNDEFINED:
					return TUNDEFINED;
				case T_NULL:
					return (numberval < 0)?TTRUE:TFALSE;
				default:
					break;
			}
			break;
		}
		case T_BOOLEAN:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return (boolval < v2.intval)?TTRUE:TFALSE;
				case T_UINTEGER:
					return (boolval < v2.uintval)?TTRUE:TFALSE;
				case T_NUMBER:
					if(std::isnan(v2.numberval))
						return TUNDEFINED;
					return (boolval < v2.numberval)?TTRUE:TFALSE;
				case T_BOOLEAN:
					return (boolval < v2.boolval)?TTRUE:TFALSE;
				case T_UNDEFINED:
					return TUNDEFINED;
				case T_NULL:
					return (boolval)?TTRUE:TFALSE;
				default:
					break;
			}
			break;
		}
		case T_NULL:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return (0 < v2.intval)?TTRUE:TFALSE;
				case T_UINTEGER:
					return (0 < v2.uintval)?TTRUE:TFALSE;
				case T_NUMBER:
					if(std::isnan(v2.numberval))
						return TUNDEFINED;
					return (0 < v2.numberval)?TTRUE:TFALSE;
				case T_BOOLEAN:
					return (0 < v2.boolval)?TTRUE:TFALSE;
				case T_UNDEFINED:
					return TUNDEFINED;
				case T_NULL:
					return TFALSE;
				default:
					break;
			}
			break;
		}
		case T_UNDEFINED:
			return TUNDEFINED;
		case T_INVALID:
			return TUNDEFINED;
		default:
			break;
	}
	return toObject(sys)->isLess(v2.toObject(sys));
}
bool asAtom::isEqual(SystemState *sys, asAtom &v2)
{
	switch (type)
	{
		case T_INTEGER:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return intval==v2.toInt();
				case T_UINTEGER:
					return intval >= 0 && intval==v2.toInt();
				case T_NUMBER:
					return intval==v2.toNumber();
				case T_BOOLEAN:
					return intval==v2.toInt();
				case T_STRING:
					return intval==v2.toNumber();
				case T_NULL:
				case T_UNDEFINED:
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case T_UINTEGER:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return v2.intval >= 0 && uintval==v2.toUInt();
				case T_UINTEGER:
				case T_NUMBER:
				case T_STRING:
				case T_BOOLEAN:
					return uintval==v2.toUInt();
				case T_NULL:
				case T_UNDEFINED:
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case T_NUMBER:
		{
			switch (v2.type)
			{
				case T_INTEGER:
				case T_UINTEGER:
				case T_BOOLEAN:
					return toNumber()==v2.toNumber();
				case T_NUMBER:
				case T_STRING:
					return toNumber()==v2.toNumber();
				case T_NULL:
				case T_UNDEFINED:
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case T_BOOLEAN:
		{
			switch (v2.type)
			{
				case T_BOOLEAN:
					return boolval==v2.boolval;
				case T_STRING:
					if (!v2.objval->isConstructed())
						return false;
					return boolval==v2.toNumber();
				case T_INTEGER:
				case T_UINTEGER:
				case T_NUMBER:
					return boolval==v2.toNumber();
				case T_NULL:
				case T_UNDEFINED:
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case T_NULL:
		{
			switch (v2.type)
			{
				case T_NULL:
				case T_UNDEFINED:
					return true;
				case T_INTEGER:
				case T_UINTEGER:
				case T_NUMBER:
				case T_BOOLEAN:
					return false;
				case T_FUNCTION:
				case T_STRING:
					if (!v2.objval->isConstructed())
						return true;
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case T_UNDEFINED:
		{
			switch (v2.type)
			{
				case T_UNDEFINED:
				case T_NULL:
					return true;
				case T_NUMBER:
				case T_INTEGER:
				case T_UINTEGER:
				case T_BOOLEAN:
					return false;
				case T_FUNCTION:
				case T_STRING:
					if (!v2.objval->isConstructed())
						return true;
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
		}
		case T_FUNCTION:
		{
			switch (v2.type)
			{
				case T_FUNCTION:
					if (closure_this != NULL && v2.closure_this != NULL && closure_this != v2.closure_this)
						return false;
					return v2.toObject(sys)->isEqual(this->toObject(sys));
				default:
					return false;
			}
		}
		case T_INVALID:
			return false;
		default:
			break;
	}
	return toObject(sys)->isEqual(v2.toObject(sys));
}

bool asAtom::isEqualStrict(SystemState *sys, asAtom &v2)
{
	if(type!=v2.type)
	{
		//Type conversions are ok only for numeric types
		switch(type)
		{
			case T_NUMBER:
			case T_INTEGER:
			case T_UINTEGER:
				break;
			case T_NULL:
				if (!v2.isConstructed() && v2.type!=T_CLASS)
					return true;
				return false;
			default:
				return false;
		}
		switch(v2.type)
		{
			case T_NUMBER:
			case T_INTEGER:
			case T_UINTEGER:
				break;
			case T_NULL:
				if (!isConstructed() && type!=T_CLASS)
					return true;
				return false;
			default:
				return false;
		}
	}
	return isEqual(sys,v2);
	
}

bool asAtom::isConstructed() const
{
	switch(type)
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_BOOLEAN:
		case T_NULL:
		case T_UNDEFINED:
			return true;
		case T_INVALID:
			return false;
		default:
			assert(objval);
			return objval->isConstructed();
	}
}

bool asAtom::isPrimitive() const
{
	// ECMA 3, section 4.3.2, T_INTEGER and T_UINTEGER are added
	// because they are special cases of Number
	return type==T_NUMBER || type ==T_UNDEFINED || type == T_NULL ||
		type==T_STRING || type==T_BOOLEAN || type==T_INTEGER ||
			type==T_UINTEGER;
}

asAtom asAtom::asTypelate(asAtom& atomtype)
{
	LOG_CALL(_("asTypelate"));

	if(atomtype.type != T_CLASS)
	{
		LOG(LOG_ERROR,"trying to call asTypelate on non class object:"<<toDebugString());
		throwError<TypeError>(kConvertNullToObjectError);
	}
	Class_base* c=static_cast<Class_base*>(atomtype.getObject());
	//Special case numeric types
	if(type==T_INTEGER || type==T_UINTEGER || type==T_NUMBER)
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
		ASATOM_DECREF(atomtype);
		if(real_ret)
			return *this;
		else
			return asAtom::nullAtom;
	}

	Class_base* objc = toObject(c->getSystemState())->getClass();
	if(!objc)
	{
		ASATOM_DECREF(atomtype);
		return asAtom::nullAtom;
	}

	bool real_ret=objc->isSubClass(c);
	LOG_CALL(_("Type ") << objc->class_name << _(" is ") << ((real_ret)?_(" "):_("not ")) 
			<< _("subclass of ") << c->class_name);
	ASATOM_DECREF(atomtype);
	if(real_ret)
		return *this;
	else
		return asAtom::nullAtom;
	
}

number_t asAtom::toNumber()
{
	switch(type)
	{
		case T_INTEGER:
			return intval;
		case T_UINTEGER:
			return uintval;
		case T_NUMBER:
			return numberval;
		case T_BOOLEAN:
			return boolval;
		case T_NULL:
			return 0;
		case T_UNDEFINED:
			return Number::NaN;
		case T_INVALID:
			return 0;
		default:
			assert(objval);
			return objval->toNumber();
	}
}
int32_t asAtom::toInt()
{
	switch(type)
	{
		case T_UNDEFINED:
			return 0;
		case T_NULL:
			return 0;
		case T_INTEGER:
			return intval;
		case T_UINTEGER:
			return uintval;
		case T_NUMBER:
			return Number::toInt(numberval);
		case T_BOOLEAN:
			return boolval;
		case T_INVALID:
			return 0;
		default:
			assert(objval);
			return objval->toInt();
	}
}
int64_t asAtom::toInt64()
{
	switch(type)
	{
		case T_UNDEFINED:
			return 0;
		case T_NULL:
			return 0;
		case T_INTEGER:
			return intval;
		case T_UINTEGER:
			return uintval;
		case T_NUMBER:
			return numberval;
		case T_BOOLEAN:
			return boolval;
		case T_INVALID:
			return 0;
		default:
			assert(objval);
			return objval->toInt64();
	}
}
uint32_t asAtom::toUInt()
{
	switch(type)
	{
		case T_UNDEFINED:
			return 0;
		case T_NULL:
			return 0;
		case T_INTEGER:
			return intval;
		case T_UINTEGER:
			return uintval;
		case T_NUMBER:
			return numberval;
		case T_BOOLEAN:
			return boolval;
		case T_INVALID:
			return 0;
		default:
			assert(objval);
			return objval->toUInt();
	}
}

tiny_string asAtom::toString()
{
	switch(type)
	{
		case T_UNDEFINED:
			return "undefined";
		case T_NULL:
			return "null";
		case T_BOOLEAN:
			return boolval ? "true" : "false";
		case T_NUMBER:
			return Number::toString(numberval);
		case T_INTEGER:
			return Integer::toString(intval);
		case T_UINTEGER:
			return UInteger::toString(uintval);
		case T_STRING:
			assert(objval);
			return objval->as<ASString>()->getData();
		case T_INVALID:
			return "";
		default:
			assert(objval);
			return objval->toString();
	}
}
tiny_string asAtom::toLocaleString()
{
	switch(type)
	{
		case T_UNDEFINED:
			return "undefined";
		case T_NULL:
			return "null";
		case T_BOOLEAN:
			return "[object Boolean]";
		case T_NUMBER:
			return Number::toString(numberval);
		case T_INTEGER:
			return Integer::toString(intval);
		case T_UINTEGER:
			return UInteger::toString(uintval);
		case T_INVALID:
			return "";
		default:
			assert(objval);
			return objval->toLocaleString();
	}
}

uint32_t asAtom::toStringId(SystemState* sys)
{
	return toObject(sys)->toStringId();
}

asAtom asAtom::typeOf(SystemState* sys)
{
	string ret="object";
	switch(type)
	{
		case T_UNDEFINED:
			ret="undefined";
			break;
		case T_OBJECT:
			if(objval->is<XML>() || objval->is<XMLList>())
				ret = "xml";
			break;
		case T_BOOLEAN:
			ret="boolean";
			break;
		case T_NUMBER:
		case T_INTEGER:
		case T_UINTEGER:
			ret="number";
			break;
		case T_STRING:
			ret="string";
			break;
		case T_FUNCTION:
			ret="function";
			break;
		default:
			break;
	}
	return asAtom::fromObject(abstract_s(sys,ret));
}

/* implements ecma3's ToBoolean() operation, see section 9.2, but returns the value instead of an Boolean object */
bool asAtom::Boolean_concrete()
{
	switch(type)
	{
		case T_UNDEFINED:
		case T_NULL:
			return false;
		case T_BOOLEAN:
			return boolval;
		case T_NUMBER:
			return numberval != 0.0 && !std::isnan(numberval);
		case T_INTEGER:
			return intval != 0;
		case T_UINTEGER:
			return uintval != 0;
		case T_STRING:
			assert(objval);
			if (!objval->isConstructed())
				return false;
			return !objval->as<ASString>()->isEmpty();
		case T_FUNCTION:
		case T_ARRAY:
		case T_OBJECT:
			assert(objval);
			// not constructed objects return false
			if (!objval->isConstructed())
				return false;
			return true;
		default:
			//everything else is an Object regarding to the spec
			return true;
	}
}

void asAtom::convert_i()
{
	if (type == T_INTEGER)
		return;
	int32_t v = toInt();
	decRef();
	setInt(v);
}

void asAtom::convert_u()
{
	if (type == T_UINTEGER)
		return;
	uint32_t v = toUInt();
	decRef();
	setUInt(v);
}

void asAtom::convert_d()
{
	if (type == T_NUMBER || type == T_INTEGER || type == T_UINTEGER)
		return;
	number_t v = toNumber();
	decRef();
	setNumber(v);
}

void asAtom::convert_b()
{
	if (type == T_BOOLEAN)
		return;
	bool v = false;
	switch(type)
	{
		case T_UNDEFINED:
		case T_NULL:
			v= false;
			break;
		case T_BOOLEAN:
			break;
		case T_NUMBER:
			v= numberval != 0.0 && !std::isnan(numberval);
			break;
		case T_INTEGER:
			v= intval != 0;
			break;
		case T_UINTEGER:
			v= uintval != 0;
			break;
		case T_STRING:
			if (objval->isConstructed())
				v= !objval->as<ASString>()->isEmpty();
			break;
		case T_FUNCTION:
		case T_ARRAY:
		case T_OBJECT:
			// not constructed objects return false
			v= (objval->isConstructed());
			break;
		default:
			//everything else is an Object regarding to the spec
			v= true;
			break;
	}
	decRef();
	setBool(v);
}

void asAtom::setInt(int32_t val)
{
	type = T_INTEGER;
	intval = val;
	objval = NULL;
}

void asAtom::setUInt(uint32_t val)
{
	type = T_UINTEGER;
	uintval = val;
	objval = NULL;
}

void asAtom::setNumber(number_t val)
{
	type = T_NUMBER;
	numberval = val;
	objval = NULL;
}

void asAtom::setBool(bool val)
{
	type = T_BOOLEAN;
	boolval = val;
	objval = NULL;
}

void asAtom::increment()
{
	switch(type)
	{
		case T_UNDEFINED:
			setNumber(Number::NaN);
			break;
		case T_NULL:
			setInt(1);
			break;
		case T_INTEGER:
			setInt(intval+1);
			break;
		case T_UINTEGER:
			setUInt(uintval+1);
			break;
		case T_NUMBER:
			setNumber(numberval+1);
			break;
		case T_BOOLEAN:
			setInt((boolval ? 1 : 0)+1);
			break;
		default:
			number_t n=objval->toNumber();
			objval->decRef();
			setNumber(n+1);
			break;
	}
}

void asAtom::decrement()
{
	switch(type)
	{
		case T_UNDEFINED:
			setNumber(Number::NaN);
			break;
		case T_NULL:
			setInt(-1);
			break;
		case T_INTEGER:
			setInt(intval-1);
			break;
		case T_UINTEGER:
		{
			if (uintval > 0)
				setUInt(uintval-1);
			else
			{
				number_t val = uintval;
				setNumber(val-1);
			}
			break;
		}
		case T_NUMBER:
			setNumber(numberval-1);
			break;
		case T_BOOLEAN:
			setInt((boolval ? 1 : 0)-1);
			break;
		default:
			number_t n=objval->toNumber();
			objval->decRef();
			setNumber(n-1);
			break;
	}

}

void asAtom::increment_i()
{
	setInt(toInt()+1);
}
void asAtom::decrement_i()
{
	setInt(toInt()-1);
}

void asAtom::add(asAtom &v2, SystemState* sys)
{
	//Implement ECMA add algorithm, for XML and default (see avm2overview)

	// if both values are Integers or UIntegers the result is also an int Number
	if( (type == T_INTEGER || type == T_UINTEGER) &&
		(v2.type == T_INTEGER || v2.type ==T_UINTEGER))
	{
		int64_t num1=toInt64();
		int64_t num2=v2.toInt64();
		LOG_CALL("addI " << num1 << '+' << num2);
		int64_t res = num1+num2;
		if (res > INT32_MIN && res < INT32_MAX)
			setInt(res);
		else if (res >= 0 && res < UINT32_MAX)
			setUInt(res);
		else
			setNumber(res);
	}
	else if((type == T_NUMBER || type == T_INTEGER || type == T_UINTEGER) &&
			(v2.type == T_NUMBER || v2.type == T_INTEGER || v2.type ==T_UINTEGER))
	{
		double num1=toNumber();
		double num2=v2.toNumber();
		LOG_CALL("addN " << num1 << '+' << num2);
		setNumber(num1+num2);
	}
	else if(type== T_STRING || v2.type == T_STRING)
	{
		tiny_string a = toString();
		tiny_string b = v2.toString();
		LOG_CALL("add " << a << '+' << b);
		decRef();
		ASATOM_DECREF(v2);
		type = T_STRING;
		objval = abstract_s(sys,a + b);
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

			//The references of val1 and val2 have been passed to the smart references
			//no decRef is needed
			type = T_OBJECT;
			objval = newList;
		}
		else
		{//If none of the above apply, convert both to primitives with no hint
			_R<ASObject> val1p = val1->toPrimitive(NO_HINT);
			_R<ASObject> val2p = val2->toPrimitive(NO_HINT);
			if(val1p->is<ASString>() || val2p->is<ASString>())
			{//If one is String, convert both to strings and concat
				string a(val1p->toString().raw_buf());
				string b(val2p->toString().raw_buf());
				LOG_CALL("add " << a << '+' << b);
				val1->decRef();
				val2->decRef();
				type = T_STRING;
				objval = abstract_s(sys,a+b);
			}
			else
			{//Convert both to numbers and add
				number_t num1=val1p->toNumber();
				number_t num2=val2p->toNumber();
				LOG_CALL("addN " << num1 << '+' << num2);
				number_t result = num1 + num2;
				val1->decRef();
				val2->decRef();
				setNumber(result);
			}
		}
	}
}

void asAtom::bitnot()
{
	int32_t i1=toInt();
	decRef();
	LOG_CALL(_("bitNot ") << hex << i1 << dec);
	setInt(~i1);
}

void asAtom::subtract(asAtom &v2)
{
	if( (type == T_INTEGER || type == T_UINTEGER) &&
		(v2.type == T_INTEGER || v2.type ==T_UINTEGER))
	{
		int64_t num1=toInt64();
		int64_t num2=v2.toInt64();
	
		decRef();
		ASATOM_DECREF(v2);
		LOG_CALL(_("subtractI ") << num1 << '-' << num2);
		int64_t res = num1-num2;
		if (res > INT32_MIN && res < INT32_MAX)
			setInt(res);
		else if (res >= 0 && res < UINT32_MAX)
			setUInt(res);
		else
			setNumber(res);
	}
	else
	{
		number_t num2=v2.toNumber();
		number_t num1=toNumber();
	
		decRef();
		ASATOM_DECREF(v2);
		LOG_CALL(_("subtract ") << num1 << '-' << num2);
		setNumber(num1-num2);
	}
}

void asAtom::multiply(asAtom &v2)
{
	if( (type == T_INTEGER || type == T_UINTEGER) &&
		(v2.type == T_INTEGER || v2.type ==T_UINTEGER))
	{
		int64_t num1=toInt64();
		int64_t num2=v2.toInt64();
	
		decRef();
		ASATOM_DECREF(v2);
		LOG_CALL(_("multiplyI ") << num1 << '*' << num2);
		int64_t res = num1*num2;
		if (res > INT32_MIN && res < INT32_MAX)
			setInt(res);
		else if (res >= 0 && res < UINT32_MAX)
			setUInt(res);
		else
			setNumber(res);
	}
	else
	{
		double num1=v2.toNumber();
		double num2=toNumber();
		LOG_CALL(_("multiply ")  << num1 << '*' << num2);
	
		setNumber(num1*num2);
	}
}

void asAtom::divide(asAtom &v2)
{
	double num1=toNumber();
	double num2=v2.toNumber();

	decRef();
	ASATOM_DECREF(v2);
	LOG_CALL(_("divide ")  << num1 << '/' << num2);
	// handling of infinity according to ECMA-262, chapter 11.5.2
	if (std::isinf(num1))
	{
		if (std::isinf(num2) || std::isnan(num2))
			setNumber(Number::NaN);
		else
		{
			bool needssign = (std::signbit(num1) || std::signbit(num2)) && !(std::signbit(num1) && std::signbit(num2)); 
			setNumber( needssign  ? -numeric_limits<double>::infinity() : numeric_limits<double>::infinity());
		}
	}
	else
		setNumber(num1/num2);
}

void asAtom::modulo(asAtom &v2)
{
	// if both values are Integers the result is also an int
	if( ((type == T_INTEGER) || (type == T_UINTEGER)) &&
		((v2.type == T_INTEGER) || (v2.type == T_UINTEGER)))
	{
		int32_t num1=toInt();
		int32_t num2=v2.toInt();
		LOG_CALL(_("moduloI ")  << num1 << '%' << num2);
		if (num2 == 0)
			setNumber(Number::NaN);
		else
			setInt(num1%num2);
	}
	else
	{
		number_t num1=toNumber();
		number_t num2=v2.toNumber();

		decRef();
		ASATOM_DECREF(v2);
		LOG_CALL(_("modulo ")  << num1 << '%' << num2);
		/* fmod returns NaN if num2 == 0 as the spec mandates */
		setNumber(::fmod(num1,num2));
	}
}

void asAtom::lshift(asAtom &v1)
{
	int32_t i2=toInt();
	uint32_t i1=v1.toUInt()&0x1f;
	ASATOM_DECREF(v1);
	decRef();
	LOG_CALL(_("lShift ")<<hex<<i2<<_("<<")<<i1<<dec);
	//Left shift are supposed to always work in 32bit
	setInt(i2<<i1);
}

void asAtom::rshift(asAtom &v1)
{
	int32_t i2=toInt();
	uint32_t i1=v1.toUInt()&0x1f;
	ASATOM_DECREF(v1);
	decRef();
	LOG_CALL(_("rShift ")<<hex<<i2<<_(">>")<<i1<<dec);
	setInt(i2>>i1);
}

void asAtom::urshift(asAtom &v1)
{
	uint32_t i2=toUInt();
	uint32_t i1=v1.toUInt()&0x1f;
	ASATOM_DECREF(v1);
	decRef();
	LOG_CALL(_("urShift ")<<hex<<i2<<_(">>")<<i1<<dec);
	setUInt(i2>>i1);
}
void asAtom::bit_and(asAtom &v1)
{
	int32_t i1=v1.toInt();
	int32_t i2=toInt();
	ASATOM_DECREF(v1);
	decRef();
	LOG_CALL(_("bitAnd_oo ") << hex << i1 << '&' << i2 << dec);
	setInt(i1&i2);
}
void asAtom::bit_or(asAtom &v1)
{
	int32_t i1=v1.toInt();
	int32_t i2=toInt();
	LOG_CALL(_("bitOr ") << hex << i1 << '|' << i2 << dec);
	setInt(i1|i2);
}
void asAtom::bit_xor(asAtom &v1)
{
	int32_t i1=v1.toInt();
	int32_t i2=toInt();
	ASATOM_DECREF(v1);
	decRef();
	LOG_CALL(_("bitXor ") << hex << i1 << '^' << i2 << dec);
	setInt(i1^i2);
}
void asAtom::_not()
{
	LOG_CALL( _("not:") <<this->toDebugString()<<" "<<!Boolean_concrete());
	
	bool ret=!Boolean_concrete();
	decRef();
	setBool(ret);
}

void asAtom::negate_i()
{
	LOG_CALL(_("negate_i"));
	int n=toInt();
	decRef();
	setInt(-n);
}

void asAtom::add_i(asAtom &v2)
{
	int32_t num2=v2.toInt();
	int32_t num1=toInt();

	decRef();
	ASATOM_DECREF(v2);
	LOG_CALL(_("add_i ") << num1 << '+' << num2);
	setInt(num1+num2);
}

void asAtom::subtract_i(asAtom &v2)
{
	int num2=v2.toInt();
	int num1=toInt();

	decRef();
	ASATOM_DECREF(v2);
	LOG_CALL(_("subtract_i ") << num1 << '-' << num2);
	setInt(num1-num2);
}

void asAtom::multiply_i(asAtom &v2)
{
	int num1=toInt();
	int num2=v2.toInt();
	decRef();
	ASATOM_DECREF(v2);
	LOG_CALL(_("multiply ")  << num1 << '*' << num2);
	setInt(num1*num2);
}

