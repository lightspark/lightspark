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
	_NR<ASObject> str = executeASMethod("toLocaleString", {""}, NULL, 0);
	if (str.isNull())
		return "";
	return str->toString();
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

	const_var_iterator it=Variables.begin()+start;

	unsigned int i=start;

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

_R<ASObject> ASObject::nextName(uint32_t index)
{
	assert_and_throw(implEnable);

	const tiny_string& name = getNameAt(index-1);
	// not mentioned in the specs, but Adobe seems to convert string names to Integers, if possible
	if (Array::isIntegerWithoutLeadingZeros(name))
		return _MR(abstract_i(getSystemState(),Integer::stringToASInteger(name.raw_buf(), 0)));
	return _MR(abstract_s(getSystemState(),name));
}

_R<ASObject> ASObject::nextValue(uint32_t index)
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

	_NR<ASObject> o=getVariableByMultiname(valueOfName,SKIP_IMPL);
	if (!o->is<IFunction>())
		throwError<TypeError>(kCallOfNonFunctionError, valueOfName.normalizedNameUnresolved(getSystemState()));
	IFunction* f=o->as<IFunction>();

	incRef();
	ASObject *ret=f->call(this,NULL,0);
	return _MR(ret);
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

	_NR<ASObject> o=getVariableByMultiname(toStringName,SKIP_IMPL);
	assert_and_throw(o->is<IFunction>());
	IFunction* f=o->as<IFunction>();

	incRef();
	ASObject *ret=f->call(this,NULL,0);
	return _MR(ret);
}

tiny_string ASObject::call_toJSON(bool& ok,std::vector<ASObject *> &path, IFunction *replacer, const tiny_string &spaces,const tiny_string& filter)
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

	_NR<ASObject> o=getVariableByMultiname(toJSONName,SKIP_IMPL);
	if (!o->is<IFunction>())
		return res;

	IFunction* f=o->as<IFunction>();

	incRef();
	ASObject *ret=f->call(this,NULL,0);
	if (ret->is<ASString>())
	{
		res += "\"";
		res += ret->toString();
		res += "\"";
	}
	else 
		res = ret->toJSON(path,replacer,spaces,filter);
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
variables_map::variables_map(MemoryAccount* m):
	Variables(std::less<mapType::key_type>(), reporter_allocator<mapType::value_type>(m)),slots_vars()
{
}

variable* variables_map::findObjVar(uint32_t nameId, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds)
{
	var_iterator ret=Variables.lower_bound(nameId);
	while(ret!=Variables.end() && ret->first == nameId)
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

	if (Variables.capacity() == Variables.size())
		Variables.reserve(Variables.capacity()+8);
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

void ASObject::setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed)
{
	setDeclaredMethodByQName(name, nsNameAndKind(getSystemState(),ns, NAMESPACE), o, type, isBorrowed);
}

void ASObject::setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed)
{
	setDeclaredMethodByQName(getSystemState()->getUniqueStringId(name), ns, o, type, isBorrowed);
}

void ASObject::setDeclaredMethodByQName(uint32_t nameId, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed)
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
	switch(type)
	{
		case NORMAL_METHOD:
		{
			obj->setVar(o);
			break;
		}
		case GETTER_METHOD:
		{
			if(obj->getter!=NULL)
				obj->getter->decRef();
			obj->getter=o;
			break;
		}
		case SETTER_METHOD:
		{
			if(obj->setter!=NULL)
				obj->setter->decRef();
			obj->setter=o;
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

	assert(obj->getter==NULL && obj->setter==NULL && obj->var!=NULL);
	//Now dereference the value
	obj->var->decRef();

	//Now kill the variable
	Variables.killObjVar(getSystemState(),name);
	--varcount;
	return true;
}

//In all setter we first pass the value to the interface to see if special handling is possible
void ASObject::setVariableByMultiname_i(const multiname& name, int32_t value)
{
	check();
	setVariableByMultiname(name,abstract_i(this->getSystemState(),value),CONST_NOT_ALLOWED);
}

variable* ASObject::findSettableImpl(SystemState* sys,variables_map& map, const multiname& name, bool* has_getter)
{
	variable* ret=map.findObjVar(sys,name,NO_CREATE_TRAIT,DECLARED_TRAIT|DYNAMIC_TRAIT);
	if(ret)
	{
		//It seems valid for a class to redefine only the getter, so if we can't find
		//something to get, it's ok
		if(!(ret->setter || ret->var))
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


void ASObject::setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst, Class_base* cls)
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
		if(obj && (obj->issealed || cls->isFinal || cls->isSealed) && !obj->setter)
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
		    ((protoObj->var && protoObj->var->is<IFunction>()) ||
		     protoObj->setter))
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

	if(obj->setter)
	{
		//Call the setter
		LOG_CALL(_("Calling the setter"));
		//Overriding function is automatically done by using cur_level
		IFunction* setter=obj->setter;
		//One argument can be passed without creating an array
		ASObject* target=this;
		target->incRef();
		_R<ASObject> ret= _MR( setter->call(target,&o,1) );
		assert_and_throw(ret->is<Undefined>());
		LOG_CALL(_("End of setter"));
	}
	else
	{
		assert_and_throw(!obj->getter);
		obj->setVar(o);
	}
}

void ASObject::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind)
{
	const nsNameAndKind tmpns(getSystemState(),ns, NAMESPACE);
	setVariableByQName(name, tmpns, o, traitKind);
}

void ASObject::setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind)
{
	setVariableByQName(getSystemState()->getUniqueStringId(name), ns, o, traitKind);
}

void ASObject::setVariableByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind)
{
	assert_and_throw(Variables.findObjVar(nameId,ns,NO_CREATE_TRAIT,traitKind)==NULL);
	variable* obj=Variables.findObjVar(nameId,ns,traitKind,traitKind);
	obj->setVar(o);
	++varcount;
}

void ASObject::initializeVariableByMultiname(const multiname& name, ASObject* o, multiname* typemname,
		ABCContext* context, TRAIT_KIND traitKind, uint32_t slot_id)
{
	check();
	Variables.initializeVar(name, o, typemname, context, traitKind,this,slot_id);
	++varcount;
}

variable::variable(TRAIT_KIND _k, ASObject* _v, multiname* _t, const Type* _type,const nsNameAndKind& _ns)
		: var(_v),typeUnion(NULL),setter(NULL),getter(NULL),ns(_ns),kind(_k),traitState(NO_STATE),isenumerable(true),issealed(false)
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

void variable::setVar(ASObject* v)
{
	//Resolve the typename if we have one
	//currentCallContext may be NULL when inserting legacy
	//children, which is done outisde any ABC context
	if(!(traitState&TYPE_RESOLVED) && traitTypemname && getVm(v->getSystemState())->currentCallContext)
	{
		type = Type::getTypeFromMultiname(traitTypemname, getVm(v->getSystemState())->currentCallContext->context);
		assert(type);
		traitState=TYPE_RESOLVED;
	}

	if((traitState&TYPE_RESOLVED) && type)
		v = type->coerce(v);

	if(var)
		var->decRef();
	var=v;
}

void variable::setVarNoCoerce(ASObject* v)
{
	if(var)
		var->decRef();
	var=v;
}

void variables_map::killObjVar(SystemState* sys,const multiname& mname)
{
	uint32_t name=mname.normalizedNameId(sys);
	//The namespaces in the multiname are ordered. So it's possible to use lower_bound
	//to find the first candidate one and move from it
	assert(!mname.ns.empty());
	var_iterator ret=Variables.lower_bound(name);
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

	var_iterator ret=Variables.lower_bound(name);
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
	if (Variables.capacity() == Variables.size())
		Variables.reserve(Variables.capacity()+8);
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

void variables_map::initializeVar(const multiname& mname, ASObject* obj, multiname* typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj, uint32_t slot_id)
{
	const Type* type = NULL;
	if (typemname->isStatic)
		type = typemname->cachedType;
	
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
			obj = mainObj->getSystemState()->getUndefinedRef();
		}
		else
		{
			assert_and_throw(obj->is<Null>() || obj->is<Undefined>());
			if(obj->is<Undefined>())
			{
				//Casting undefined to an object (of unknown class)
				//results in Null
				obj = mainObj->getSystemState()->getNullRef();
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
				obj = mainObj->getSystemState()->getUndefinedRef();
			}
			else
			{
				obj = mainObj->getSystemState()->getUndefinedRef();
				obj = type->coerce(obj);
			}
		}
		if (typemname->isStatic && typemname->cachedType == NULL)
			typemname->cachedType = type;
	}
	assert(traitKind==DECLARED_TRAIT || traitKind==CONSTANT_TRAIT || traitKind == INSTANCE_TRAIT);

	uint32_t name=mname.normalizedNameId(mainObj->getSystemState());
	if (Variables.capacity() == Variables.size())
		Variables.reserve(Variables.capacity()+8);
	Variables.insert(Variables.cbegin(),make_pair(name, variable(traitKind, obj, typemname, type,mname.ns[0])));
	if (slot_id)
		initSlot(slot_id,name, mname.ns[0]);
}

ASFUNCTIONBODY(ASObject,generator)
{
	//By default we assume it's a passthrough cast
	if(argslen==1)
	{
		LOG_CALL(_("Passthrough of ") << args[0]);
		args[0]->incRef();
		return args[0];
	}
	else
		return Class<ASObject>::getInstanceS(getSys());
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
	multiname toStringName(NULL);
	toStringName.name_type=multiname::NAME_STRING;
	toStringName.name_s_id=BUILTIN_STRINGS::STRING_TOSTRING;
	toStringName.ns.emplace_back(obj->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	toStringName.isAttribute = false;
	if (obj->hasPropertyByMultiname(toStringName, true, false))
	{
		_NR<ASObject> o=obj->getVariableByMultiname(toStringName,SKIP_IMPL);
		assert_and_throw(o->is<IFunction>());
		IFunction* f=o->as<IFunction>();
		
		obj->incRef();
		ASObject *ret=f->call(obj,NULL,0);
		return ret;
	}
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
	variable* v = obj->Variables.findObjVar(obj->getSystemState(),name, NO_CREATE_TRAIT,DYNAMIC_TRAIT);
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

	_NR<ASObject> ret=getVariableByMultiname(name);
	assert_and_throw(!ret.isNull());
	return ret->toInt();
}

const variable* ASObject::findVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls, uint32_t *nsRealID) const
{
	//Get from the current object without considering borrowed properties
	const variable* obj=varcount ? Variables.findObjVar(getSystemState(),name,name.hasEmptyNS ? DECLARED_TRAIT|DYNAMIC_TRAIT : DECLARED_TRAIT,nsRealID):NULL;
	if(obj)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(obj->getter || obj->var))
			obj=NULL;
	}

	if(!obj)
	{
		//Look for borrowed traits before
		if (cls)
		{
			obj=cls->findBorrowedGettable(name,nsRealID);
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
						if(!(obj->getter || obj->var))
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

_NR<ASObject> ASObject::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls)
{
	check();
	assert(!cls || classdef->isSubClass(cls));
	uint32_t nsRealId;

	const variable* obj=varcount ? Variables.findObjVar(getSystemState(),name,name.hasEmptyNS ? DECLARED_TRAIT|DYNAMIC_TRAIT : DECLARED_TRAIT,&nsRealId):NULL;
	if(obj)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(obj->getter || obj->var))
			obj=NULL;
	}

	if(!obj)
	{
		//Look for borrowed traits before
		if (cls)
		{
			obj=cls->findBorrowedGettable(name,&nsRealId);
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
						if(!(obj->getter || obj->var))
							obj=NULL;
					}
					if(obj)
						break;
					proto = proto->prevPrototype.getPtr();
				}
			}
		}
		if(!obj)
			return NullRef;
	}


	if ( this->is<Class_base>() && obj->kind == INSTANCE_TRAIT &&
			(!obj->var || !obj->var->isConstructed() ||
			 obj->var->getObjectType() == T_UNDEFINED ||
			 obj->var->getObjectType() == T_NULL))
	{
		if (getSystemState()->getNamespaceFromUniqueId(nsRealId).kind != STATIC_PROTECTED_NAMESPACE)
			throwError<TypeError>(kCallOfNonFunctionError,name.normalizedNameUnresolved(getSystemState()));
	}

	if(obj->getter)
	{
		//Call the getter
		LOG_CALL("Calling the getter for " << name);
		IFunction* getter=obj->getter;
		ASObject* target=NULL;
		if (!getter->isBound())
		{
			target=this;
			target->incRef();
		}
		ASObject* ret=getter->call(target,NULL,0);
		LOG_CALL(_("End of getter"));
		// No incRef because ret is a new instance
		return _MNR(ret);
	}
	else
	{
		assert_and_throw(!obj->setter);
		assert_and_throw(obj->var);
		if(obj->var->getObjectType()==T_FUNCTION && obj->var->as<IFunction>()->isMethod())
		{
			LOG_CALL("Attaching this " << this << " to function " << name);
			//the obj reference is acquired by the smart reference
			this->incRef();
			IFunction* f=obj->var->as<IFunction>()->bind(_MR(this),-1);
			//No incref is needed, as the function is a new instance
			return _MNR(f);
		}
		obj->var->incRef();
		return _MNR(obj->var);
	}
}

_NR<ASObject> ASObject::getVariableByMultiname(const tiny_string& name, std::list<tiny_string> namespaces)
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

_NR<ASObject> ASObject::executeASMethod(const tiny_string& methodName,
					std::list<tiny_string> namespaces,
					ASObject* const* args,
					uint32_t num_args)
{
	_NR<ASObject> o = getVariableByMultiname(methodName, namespaces);
	if (o.isNull() || !o->is<IFunction>())
		throwError<TypeError>(kCallOfNonFunctionError, methodName);
	IFunction* f=o->as<IFunction>();

	incRef();
	ASObject *ret=f->call(this,args,num_args);
	return _MNR(ret);
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
			if(it->second.var==NULL && next->second.var==NULL)
				continue;

			if(it->second.var==NULL || next->second.var==NULL)
			{
				LOG(LOG_INFO, it->first << " " << it->second.ns);
				LOG(LOG_INFO, it->second.var << ' ' << it->second.setter << ' ' << it->second.getter);
				LOG(LOG_INFO, next->second.var << ' ' << next->second.setter << ' ' << next->second.getter);
				abort();
			}

			if(it->second.var->getObjectType()!=T_FUNCTION || next->second.var->getObjectType()!=T_FUNCTION)
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
			it->second.var << ' ' << it->second.setter << ' ' << it->second.getter);
	}
}

variables_map::~variables_map()
{
	destroyContents();
}

void variables_map::destroyContents()
{
	const_var_iterator it=Variables.cbegin();
	while(it!=Variables.cend())
	{
		if(it->second.var)
			it->second.var->decRef();
		if(it->second.setter)
			it->second.setter->decRef();
		if(it->second.getter)
			it->second.getter->decRef();
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

ASObject *variables_map::getSlot(unsigned int n)
{
	assert_and_throw(n > 0 && n<=slots_vars.size());
	var_iterator it = Variables.find(slots_vars[n-1].nameId);
	while(it!=Variables.end() && it->first == slots_vars[n-1].nameId)
	{
		if (it->second.ns == slots_vars[n-1].ns)
			return it->second.var;
		it++;
	}
	return NULL;
}
void variables_map::setSlot(unsigned int n,ASObject* o)
{
	validateSlotId(n);
	var_iterator it = Variables.find(slots_vars[n-1].nameId);
	while(it!=Variables.end() && it->first == slots_vars[n-1].nameId)
	{
		if (it->second.ns == slots_vars[n-1].ns)
		{
			it->second.setVar(o);
			return;
		}
		it++;
	}
}

void variables_map::setSlotNoCoerce(unsigned int n,ASObject* o)
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
		var_iterator it=Variables.begin()+index;
		return &it->second;
	}
	else
		throw RunTimeException("getValueAt out of bounds");
}

_R<ASObject> ASObject::getValueAt(int index)
{
	variable* obj=Variables.getValueAt(index);
	assert_and_throw(obj);
	if(obj->getter)
	{
		//Call the getter
		LOG_CALL(_("Calling the getter"));
		IFunction* getter=obj->getter;
		incRef();
		_R<ASObject> ret(getter->call(this,NULL,0));
		LOG_CALL(_("End of getter"));
		return ret;
	}
	else
	{
		obj->var->incRef();
		return _MR(obj->var);
	}
}

tiny_string variables_map::getNameAt(SystemState *sys, unsigned int index) const
{
	//TODO: CHECK behaviour on overridden methods
	if(index<Variables.size())
	{
		const_var_iterator it=Variables.begin()+index;
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
				std::map<const Class_base*, uint32_t> traitsMap) const
{
	Variables.serialize(out, stringMap, objMap, traitsMap);
}

void variables_map::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap) const
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
		it->second.var->serialize(out, stringMap, objMap, traitsMap);
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

		_NR<ASObject> o=getVariableByMultiname(writeExternalName,SKIP_IMPL);
		assert_and_throw(!o.isNull() && o->getObjectType()==T_FUNCTION);
		IFunction* f=o->as<IFunction>();
		this->incRef();
		out->incRef();
		ASObject* const tmpArg[1] = {out};
		f->call(this, tmpArg, 1);
		return;
	}

	//Add the object to the map
	objMap.insert(make_pair(this, objMap.size()));

	uint32_t traitsCount=0;
	const variables_map::const_var_iterator beginIt = Variables.Variables.begin();
	const variables_map::const_var_iterator endIt = Variables.Variables.end();
	//Check if the class traits has been already serialized to send it by reference
	auto it2=traitsMap.find(type);

	if (amf0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing ASObject in AMF0 not completely implemented");
		if(it2!=traitsMap.end())
		{
			out->writeByte(amf0_reference_marker);
			out->writeShort(it2->second);
			for(variables_map::const_var_iterator varIt=beginIt; varIt != endIt; ++varIt)
			{
				if(varIt->second.kind==DECLARED_TRAIT)
				{
					if(!varIt->second.ns.hasEmptyName())
					{
						//Skip variable with a namespace, like protected ones
						continue;
					}
					out->writeStringAMF0(getSystemState()->getStringFromUniqueId(varIt->first));
					varIt->second.var->serialize(out, stringMap, objMap, traitsMap);
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
	for(variables_map::const_var_iterator varIt=beginIt; varIt != endIt; ++varIt)
	{
		if(varIt->second.kind==DECLARED_TRAIT)
		{
			if(!varIt->second.ns.hasEmptyName())
			{
				//Skip variable with a namespace, like protected ones
				continue;
			}
			varIt->second.var->serialize(out, stringMap, objMap, traitsMap);
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

tiny_string ASObject::toJSON(std::vector<ASObject *> &path, IFunction *replacer, const tiny_string &spaces,const tiny_string& filter)
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
		const variables_map::const_var_iterator beginIt = Variables.Variables.begin();
		const variables_map::const_var_iterator endIt = Variables.Variables.end();
		bool bfirst = true;
		path.push_back(this);
		for(variables_map::const_var_iterator varIt=beginIt; varIt != endIt; ++varIt)
		{
			// check for cylic reference
			if (varIt->second.var->getObjectType() != T_UNDEFINED &&
				varIt->second.var->getObjectType() != T_NULL &&
				varIt->second.var->getObjectType() != T_BOOLEAN &&
				std::find(path.begin(),path.end(), varIt->second.var) != path.end())
				throwError<TypeError>(kJSONCyclicStructure);

			if (replacer != NULL)
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
				ASObject* params[2];
				
				params[0] = abstract_s(getSystemState(),getSystemState()->getStringFromUniqueId(varIt->first));
				params[1] = varIt->second.var;
				params[1]->incRef();
				ASObject *funcret=replacer->call(getSystemState()->getNullRef(), params, 2);
				if (funcret)
					res += funcret->toString();
				else
					res += varIt->second.var->toJSON(path,replacer,spaces+spaces,filter);
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
				res += varIt->second.var->toJSON(path,replacer,spaces+spaces,filter);
				bfirst = false;
			}
		}
		if (!bfirst)
			res += newline+spaces.substr_bytes(0,spaces.numBytes()/2);

		res += "}";
		path.pop_back();
	}
	return res;
}

bool ASObject::hasprop_prototype()
{
	variable* var=Variables.findObjVar(BUILTIN_STRINGS::PROTOTYPE,nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),
			NO_CREATE_TRAIT,(DECLARED_TRAIT|DYNAMIC_TRAIT));
	return (var && var->var);
}

ASObject* ASObject::getprop_prototype()
{
	variable* var=Variables.findObjVar(BUILTIN_STRINGS::PROTOTYPE,nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),
			NO_CREATE_TRAIT,(DECLARED_TRAIT|DYNAMIC_TRAIT));
	return var ? var->var : NULL;
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
	if(ret->setter)
	{
		this->incRef();
		_MR( ret->setter->call(this,&obj,1) );
	}
	else
		ret->setVar(obj);
}

tiny_string ASObject::getClassName() const
{
	if (getClass())
		return getClass()->getQualifiedClassName();
	else
		return "";
}
