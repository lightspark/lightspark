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

using namespace lightspark;
using namespace std;

string ASObject::toDebugString()
{
	check();
	string ret;
	if(this->is<Class_base>())
	{
		ret = "[class ";
		ret+=this->as<Class_base>()->class_name.getQualifiedName().raw_buf();
		ret+="]";
	}
	else if(getClass())
	{
		ret="[object ";
		ret+=getClass()->class_name.name.raw_buf();
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
		this->proxyMultiName = new (getVm()->vmDataMemory) multiname(getVm()->vmDataMemory);
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
	name.ns.clear();
	name.ns.reserve(this->proxyMultiName->ns.size());
	for(unsigned int i=0;i<this->proxyMultiName->ns.size();i++)
	{
		name.ns.push_back(this->proxyMultiName->ns[i]);
	}
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
		return as<ASString>()->data;
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

	const_var_iterator it=Variables.begin();

	unsigned int i=0;
	while(i<start)
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

_R<ASObject> ASObject::nextName(uint32_t index)
{
	assert_and_throw(implEnable);

	return _MR(Class<ASString>::getInstanceS(getNameAt(index-1)));
}

_R<ASObject> ASObject::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);

	return getValueAt(index-1);
}

void ASObject::sinit(Class_base* c)
{
	c->setDeclaredMethodByQName("hasOwnProperty",AS3,Class<IFunction>::getFunction(hasOwnProperty),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("setPropertyIsEnumerable",AS3,Class<IFunction>::getFunction(setPropertyIsEnumerable),NORMAL_METHOD,true);

	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",Class<IFunction>::getFunction(_toLocaleString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",Class<IFunction>::getFunction(valueOf),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("hasOwnProperty","",Class<IFunction>::getFunction(hasOwnProperty),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("isPrototypeOf","",Class<IFunction>::getFunction(isPrototypeOf),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("propertyIsEnumerable","",Class<IFunction>::getFunction(propertyIsEnumerable),DYNAMIC_TRAIT);

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
			XMLList *xl=dynamic_cast<XMLList *>(r);
			if(xl)
				return xl->isEqual(this);
			XML *x=dynamic_cast<XML *>(r);
			if(x && x->hasSimpleContent())
				return x->toString()==toString();
		}
	}
	if (r->is<ObjectConstructor>())
		return this == r->getClass();

	LOG(LOG_CALLS,_("Equal comparison between type ")<<getObjectType()<< _(" and type ") << r->getObjectType());
	if(classdef)
		LOG(LOG_CALLS,_("Type ") << classdef->class_name);
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

	throw Class<TypeError>::getInstanceS();
	return _MR((ASObject*)NULL);
}

bool ASObject::has_valueOf()
{
	multiname valueOfName(NULL);
	valueOfName.name_type=multiname::NAME_STRING;
	valueOfName.name_s_id=getSys()->getUniqueStringId("valueOf");
	valueOfName.ns.push_back(nsNameAndKind("",NAMESPACE));
	valueOfName.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
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
	valueOfName.name_s_id=getSys()->getUniqueStringId("valueOf");
	valueOfName.ns.push_back(nsNameAndKind("",NAMESPACE));
	valueOfName.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	valueOfName.isAttribute = false;
	assert_and_throw(hasPropertyByMultiname(valueOfName, true, true));

	_NR<ASObject> o=getVariableByMultiname(valueOfName,SKIP_IMPL);
	if (!o->is<IFunction>())
		throwError<TypeError>(kCallOfNonFunctionError, valueOfName.normalizedName());
	IFunction* f=o->as<IFunction>();

	incRef();
	ASObject *ret=f->call(this,NULL,0);
	return _MR(ret);
}

bool ASObject::has_toString()
{
	multiname toStringName(NULL);
	toStringName.name_type=multiname::NAME_STRING;
	toStringName.name_s_id=getSys()->getUniqueStringId("toString");
	toStringName.ns.push_back(nsNameAndKind("",NAMESPACE));
	toStringName.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
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
	toStringName.name_s_id=getSys()->getUniqueStringId("toString");
	toStringName.ns.push_back(nsNameAndKind("",NAMESPACE));
	toStringName.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	toStringName.isAttribute = false;
	assert(ASObject::hasPropertyByMultiname(toStringName, true, true));

	_NR<ASObject> o=getVariableByMultiname(toStringName,SKIP_IMPL);
	assert_and_throw(o->is<IFunction>());
	IFunction* f=o->as<IFunction>();

	incRef();
	ASObject *ret=f->call(this,NULL,0);
	return _MR(ret);
}

bool ASObject::has_toJSON()
{
	multiname toJSONName(NULL);
	toJSONName.name_type=multiname::NAME_STRING;
	toJSONName.name_s_id=getSys()->getUniqueStringId("toJSON");
	toJSONName.ns.push_back(nsNameAndKind("",NAMESPACE));
	toJSONName.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	toJSONName.isAttribute = false;
	return ASObject::hasPropertyByMultiname(toJSONName, true, true);
}

tiny_string ASObject::call_toJSON()
{
	multiname toJSONName(NULL);
	toJSONName.name_type=multiname::NAME_STRING;
	toJSONName.name_s_id=getSys()->getUniqueStringId("toJSON");
	toJSONName.ns.push_back(nsNameAndKind("",NAMESPACE));
	toJSONName.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	toJSONName.isAttribute = false;
	assert(ASObject::hasPropertyByMultiname(toJSONName, true, true));

	_NR<ASObject> o=getVariableByMultiname(toJSONName,SKIP_IMPL);
	assert_and_throw(o->is<IFunction>());
	IFunction* f=o->as<IFunction>();

	incRef();
	ASObject *ret=f->call(this,NULL,0);
	tiny_string res;
	if (ret->is<ASString>())
	{
		res += "\"";
		res += ret->toString();
		res += "\"";
	}
	else 
		res = ret->toString();
	
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
	Variables(std::less<mapType::key_type>(), reporter_allocator<mapType::value_type>(m)),slots_vars(m)
{
}

variable* variables_map::findObjVar(uint32_t nameId, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds)
{
	var_iterator ret=Variables.find(varName(nameId,ns));
	if(ret!=Variables.end())
	{
		if(!(ret->second.kind & traitKinds))
		{
			assert(createKind==NO_CREATE_TRAIT);
			return NULL;
		}
		return &ret->second;
	}

	//Name not present, insert it if we have to create it
	if(createKind==NO_CREATE_TRAIT)
		return NULL;

	var_iterator inserted=Variables.insert(make_pair(varName(nameId, ns), variable(createKind)) ).first;
	return &inserted->second;
}

bool ASObject::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	//We look in all the object's levels
	uint32_t validTraits=DECLARED_TRAIT;
	if(considerDynamic)
		validTraits|=DYNAMIC_TRAIT;

	if(Variables.findObjVar(name, NO_CREATE_TRAIT, validTraits)!=NULL)
		return true;

	if(classdef && classdef->borrowedVariables.findObjVar(name, NO_CREATE_TRAIT, DECLARED_TRAIT)!=NULL)
		return true;

	NS_KIND nskind;
	//Check prototype inheritance chain
	if(getClass() && considerPrototype)
	{
		Prototype* proto = getClass()->prototype.getPtr();
		while(proto)
		{
			if(proto->getObj()->findGettable(name,nskind) != NULL)
				return true;
			proto=proto->prevPrototype.getPtr();
		}
	}

	//Must not ask for non borrowed traits as static class member are not valid
	return false;
}

void ASObject::setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed)
{
	setDeclaredMethodByQName(name, nsNameAndKind(ns, NAMESPACE), o, type, isBorrowed);
}

void ASObject::setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed)
{
	setDeclaredMethodByQName(getSys()->getUniqueStringId(name), ns, o, type, isBorrowed);
}

void ASObject::setDeclaredMethodByQName(uint32_t nameId, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed)
{
	check();
#ifndef NDEBUG
	assert(!initialized);
#endif
	//borrowed properties only make sense on class objects
	assert(!isBorrowed || dynamic_cast<Class_base*>(this));
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
	}
	else
		obj=Variables.findObjVar(nameId,ns,DECLARED_TRAIT, DECLARED_TRAIT);
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
}

bool ASObject::deleteVariableByMultiname(const multiname& name)
{
	variable* obj=Variables.findObjVar(name,NO_CREATE_TRAIT,DYNAMIC_TRAIT|DECLARED_TRAIT);
	
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
	Variables.killObjVar(name);
	return true;
}

//In all setter we first pass the value to the interface to see if special handling is possible
void ASObject::setVariableByMultiname_i(const multiname& name, int32_t value)
{
	check();
	setVariableByMultiname(name,abstract_i(value),CONST_NOT_ALLOWED);
}

variable* ASObject::findSettableImpl(variables_map& map, const multiname& name, bool* has_getter)
{
	variable* ret=map.findObjVar(name,NO_CREATE_TRAIT,DECLARED_TRAIT|DYNAMIC_TRAIT);
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
	return findSettableImpl(Variables, name, has_getter);
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
		throwError<ReferenceError>(kConstWriteError, name.normalizedName(), classdef->as<Class_base>()->getQualifiedClassName());
	}
	if(!obj && cls)
	{
		//Look for borrowed traits before
		//It's valid to override only a getter, so keep
		//looking for a settable even if a super class sets
		//has_getter to true.
		obj=cls->findBorrowedSettable(name,&has_getter);
		if(obj && cls->isFinal && !obj->setter)
		{
			throwError<ReferenceError>(kCannotAssignToMethodError, name.normalizedName(), cls ? cls->getQualifiedClassName() : "");
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
		    ((protoObj->var && protoObj->var->is<Function>()) ||
		     protoObj->setter))
		{
			throwError<ReferenceError>(kCannotAssignToMethodError, name.normalizedName(), cls ? cls->getQualifiedClassName() : "");
		}
	}

	if(!obj)
	{
		if(has_getter)  // Is this a read-only property?
		{
			throwError<ReferenceError>(kConstWriteError, name.normalizedName(), cls ? cls->getQualifiedClassName() : "");
		}

		// Properties can not be added to a sealed class
		if (cls && cls->isSealed)
		{
			throwError<ReferenceError>(kWriteSealedError, name.normalizedName(), cls->getQualifiedClassName());
		}

		//Create a new dynamic variable
		obj=Variables.findObjVar(name,DYNAMIC_TRAIT,DYNAMIC_TRAIT);
	}

	if(obj->setter)
	{
		//Call the setter
		LOG(LOG_CALLS,_("Calling the setter"));
		//Overriding function is automatically done by using cur_level
		IFunction* setter=obj->setter;
		//One argument can be passed without creating an array
		ASObject* target=this;
		target->incRef();
		_R<ASObject> ret= _MR( setter->call(target,&o,1) );
		assert_and_throw(ret->is<Undefined>());
		LOG(LOG_CALLS,_("End of setter"));
	}
	else
	{
		assert_and_throw(!obj->getter);
		obj->setVar(o);
	}
}

void ASObject::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind)
{
	const nsNameAndKind tmpns(ns, NAMESPACE);
	setVariableByQName(name, tmpns, o, traitKind);
}

void ASObject::setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind)
{
	setVariableByQName(getSys()->getUniqueStringId(name), ns, o, traitKind);
}

void ASObject::setVariableByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind)
{
	assert_and_throw(Variables.findObjVar(nameId,ns,NO_CREATE_TRAIT,traitKind)==NULL);
	variable* obj=Variables.findObjVar(nameId,ns,traitKind,traitKind);
	obj->setVar(o);
}

void ASObject::initializeVariableByMultiname(const multiname& name, ASObject* o, multiname* typemname,
		ABCContext* context, TRAIT_KIND traitKind, bool bOverwrite)
{
	check();
	if (!bOverwrite)
	{
		variable* obj=findSettable(name);
		if(obj)
		{
			//Initializing an already existing variable
			LOG(LOG_NOT_IMPLEMENTED,"Variable " << name << " already initialized");
			if (o != NULL)
				o->decRef();
			return;
		}
	}
	Variables.initializeVar(name, o, typemname, context, traitKind,this);
}

variable::variable(TRAIT_KIND _k, ASObject* _v, multiname* _t, const Type* _type)
		: var(_v),typeUnion(NULL),setter(NULL),getter(NULL),kind(_k),traitState(NO_STATE),isenumerable(true)
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
	if(!(traitState&TYPE_RESOLVED) && traitTypemname && getVm()->currentCallContext)
	{
		type = Type::getTypeFromMultiname(traitTypemname, getVm()->currentCallContext->context);
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

void variables_map::killObjVar(const multiname& mname)
{
	uint32_t name=mname.normalizedNameId();
	//The namespaces in the multiname are ordered. So it's possible to use lower_bound
	//to find the first candidate one and move from it
	assert(!mname.ns.empty());
	var_iterator ret=Variables.lower_bound(varName(name,mname.ns.front()));
	auto nsIt=mname.ns.begin();

	//Find the namespace
	while(ret!=Variables.end() && ret->first.nameId==name)
	{
		//breaks when the namespace is not found
		const nsNameAndKind& ns=ret->first.ns;
		if(ns==*nsIt)
		{
			Variables.erase(ret);
			return;
		}
		else if(*nsIt<ns)
		{
			++nsIt;
			if(nsIt==mname.ns.end())
				break;
		}
		else if(ns<*nsIt)
			++ret;
	}
	throw RunTimeException("Variable to kill not found");
}

variable* variables_map::findObjVar(const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds)
{
	uint32_t name=mname.normalizedNameId();
	assert(!mname.ns.empty());

	var_iterator ret=Variables.lower_bound(varName(name,mname.ns.front()));
	auto nsIt=mname.ns.begin();

	//Find the namespace
	while(ret!=Variables.end() && ret->first.nameId==name)
	{
		//breaks when the namespace is not found
		const nsNameAndKind& ns=ret->first.ns;
		if(ns==*nsIt)
		{
			if(ret->second.kind & traitKinds)
				return &ret->second;
			else
				return NULL;
		}
		else if(*nsIt<ns)
		{
			++nsIt;
			if(nsIt==mname.ns.end())
				break;
		}
		else if(ns<*nsIt)
			++ret;
	}

	//Name not present, insert it, if the multiname has a single ns and if we have to insert it
	if(createKind==NO_CREATE_TRAIT)
		return NULL;
	if(createKind == DYNAMIC_TRAIT)
	{
		if(!mname.ns.begin()->hasEmptyName())
			throwError<ReferenceError>(kWriteSealedError, mname.normalizedName(), "" /* TODO: class name */);
		var_iterator inserted=Variables.insert(
			make_pair(varName(name,mname.ns[0]),variable(createKind))).first;
		return &inserted->second;
	}
	assert(mname.ns.size() == 1);
	var_iterator inserted=Variables.insert(
		make_pair(varName(name,mname.ns[0]),variable(createKind))).first;
	return &inserted->second;
}

const variable* variables_map::findObjVar(const multiname& mname, uint32_t traitKinds, NS_KIND &nskind) const
{
	uint32_t name=mname.normalizedNameId();
	assert(!mname.ns.empty());

	const_var_iterator ret=Variables.lower_bound(varName(name,mname.ns.front()));
	auto nsIt=mname.ns.begin();

	//Find the namespace
	while(ret!=Variables.end() && ret->first.nameId==name)
	{
		//breaks when the namespace is not found
		const nsNameAndKind& ns=ret->first.ns;
		if(ns==*nsIt)
		{
			nskind = ns.getImpl().kind;
			if(ret->second.kind & traitKinds)
				return &ret->second;
			else
				return NULL;
		}
		else if(*nsIt<ns)
		{
			++nsIt;
			if(nsIt==mname.ns.end())
				break;
		}
		else if(ns<*nsIt)
			++ret;
	}

	return NULL;
}

void variables_map::initializeVar(const multiname& mname, ASObject* obj, multiname* typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj)
{
	const Type* type = NULL;
	 /* If typename is a builtin type, we coerce obj.
	  * It it's not it must be a user defined class,
	  * so we try to find the class it is derived from and create an apropriate uninitialized instance */
	if (type == NULL)
		type = Type::getBuiltinType(typemname);
	if (type == NULL)
		type = Type::getTypeFromMultiname(typemname,context);
	if(type==NULL)
	{
		if (obj == NULL) // create dynamic object
		{
			obj = getSys()->getUndefinedRef();
		}
		else
		{
			assert_and_throw(obj->is<Null>() || obj->is<Undefined>());
			if(obj->is<Undefined>())
			{
				//Casting undefined to an object (of unknown class)
				//results in Null
				obj->decRef();
				obj = getSys()->getNullRef();
			}
		}
	}
	else
	{
		if (obj == NULL) // create dynamic object
		{
			if (type == Type::anyType)
			{
				// type could not be found, so it's stored as an uninitialized variable
				LOG(LOG_CALLS,"add uninitialized var:"<<mname);
				uninitializedVar v;
				mainObj->incRef();
				v.mainObj = mainObj;
				v.mname = &mname;
				v.traitKind = traitKind;
				v.typemname = typemname;
				context->addUninitializedVar(v);
				obj = getSys()->getUndefinedRef();
				obj = type->coerce(obj);
			}
			else if(mainObj->is<Class_base>() &&
				mainObj->as<Class_base>()->class_name.getQualifiedName() == typemname->qualifiedString())
			{
				// avoid recursive construction
				obj = getSys()->getNullRef();
			}
			else if (type != Class_object::getClass() &&
					dynamic_cast<const Class_base*>(type))
			{
				if (!((Class_base*)type)->super)
				{
					// super type could not be found, so the class is stored as an uninitialized variable
					LOG(LOG_CALLS,"add uninitialized class var:"<<mname);
					uninitializedVar v;
					mainObj->incRef();
					v.mainObj = mainObj;
					v.mname = &mname;
					v.traitKind = traitKind;
					v.typemname = typemname;
					context->addUninitializedVar(v);
					obj = getSys()->getUndefinedRef();
					obj = type->coerce(obj);
				}
				else
					obj = ((Class_base*)type)->getInstance(false,NULL,0);
			}
			else
			{
				obj = getSys()->getUndefinedRef();
				obj = type->coerce(obj);
			}
		}
	}
	assert(traitKind==DECLARED_TRAIT || traitKind==CONSTANT_TRAIT || traitKind == INSTANCE_TRAIT);

	uint32_t name=mname.normalizedNameId();
	Variables.insert(make_pair(varName(name, mname.ns[0]), variable(traitKind, obj, typemname, type)));
}

ASFUNCTIONBODY(ASObject,generator)
{
	//By default we assume it's a passthrough cast
	if(argslen==1)
	{
		LOG(LOG_CALLS,_("Passthrough of ") << args[0]);
		args[0]->incRef();
		return args[0];
	}
	else
		return Class<ASObject>::getInstanceS();
}

ASFUNCTIONBODY(ASObject,_toString)
{
	tiny_string ret;
	if(obj->getClass())
	{
		ret="[object ";
		ret+=obj->getClass()->class_name.name;
		ret+="]";
	}
	else if (obj->is<Class_base>())
	{
		ret="[object ";
		ret+=static_cast<Class_base*>(obj)->class_name.name;
		ret+="]";
	}
	else
		ret="[object Object]";

	return Class<ASString>::getInstanceS(ret);
}

ASFUNCTIONBODY(ASObject,_toLocaleString)
{
	if (!obj->has_toString())
		throwError<TypeError>(kCallNotFoundError, "toString", obj->getClassName());

	_R<ASObject> res = obj->call_toString();
	res->incRef();
	return res.getPtr();
}

ASFUNCTIONBODY(ASObject,hasOwnProperty)
{
	assert_and_throw(argslen==1);
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=getSys()->getUniqueStringId(args[0]->toString());
	name.ns.push_back(nsNameAndKind("",NAMESPACE));
	name.isAttribute=false;
	bool ret=obj->hasPropertyByMultiname(name, true, false);
	return abstract_b(ret);
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
	return abstract_b(ret);
}

ASFUNCTIONBODY(ASObject,propertyIsEnumerable)
{
	if (argslen == 0)
		return abstract_b(false);
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=getSys()->getUniqueStringId(args[0]->toString());
	name.ns.push_back(nsNameAndKind("",NAMESPACE));
	name.isAttribute=false;
	if (obj->is<Array>()) // propertyIsEnumerable(index) isn't mentioned in the ECMA specs but is tested for
	{
		Array* a = static_cast<Array*>(obj);
		unsigned int index = 0;
		if (a->isValidMultiname(name,index))
		{
			return abstract_b(index < (unsigned int)a->size());
		}
	}
	if (obj->hasPropertyByMultiname(name,true,false))
		return abstract_b(true);
	return abstract_b(false);
}
ASFUNCTIONBODY(ASObject,setPropertyIsEnumerable)
{
	tiny_string propname;
	bool isEnum;
	ARG_UNPACK(propname) (isEnum, true);
	multiname name(NULL);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=getSys()->getUniqueStringId(args[0]->toString());
	name.ns.push_back(nsNameAndKind("",NAMESPACE));
	name.isAttribute=false;
	variable* v =obj->Variables.findObjVar(name, NO_CREATE_TRAIT,DYNAMIC_TRAIT);
	if (v)
		v->isenumerable = isEnum;
	return NULL;
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
void ASObject::appendSlot(const multiname& name)
{
	Variables.appendSlot(name.name_s_id,name.ns[0]);
}

int32_t ASObject::getVariableByMultiname_i(const multiname& name)
{
	check();

	_NR<ASObject> ret=getVariableByMultiname(name);
	assert_and_throw(!ret.isNull());
	return ret->toInt();
}

const variable* ASObject::findGettableImpl(const variables_map& map, const multiname& name, NS_KIND &nskind)
{
	const variable* ret=map.findObjVar(name,DECLARED_TRAIT|DYNAMIC_TRAIT,nskind);
	if(ret)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(ret->getter || ret->var))
			ret=NULL;
	}
	return ret;
}

const variable* ASObject::findGettable(const multiname& name, NS_KIND &nskind) const
{
	return findGettableImpl(Variables,name,nskind);
}

const variable* ASObject::findVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls, NS_KIND &nskind)
{
	//Get from the current object without considering borrowed properties
	const variable* var=findGettable(name,nskind);

	if(!var && cls)
	{
		//Look for borrowed traits before
		var=cls->findBorrowedGettable(name,nskind);
	}

	if(!var && cls)
	{
		//Check prototype chain
		Prototype* proto = cls->prototype.getPtr();
		while(proto)
		{
			var = proto->getObj()->findGettable(name,nskind);
			if(var)
				break;
			proto = proto->prevPrototype.getPtr();
		}
	}
	return var;
}

_NR<ASObject> ASObject::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls)
{
	check();
	assert(!cls || classdef->isSubClass(cls));
	NS_KIND nskind;
	const variable* obj=findVariableByMultiname(name, opt, cls,nskind);

	if(!obj)
		return NullRef;
	if (this->is<Class_base>() && 
			(!obj->var || 
			 (obj->var->getObjectType() != T_UNDEFINED && 
			  obj->var->getObjectType() != T_NULL && 
			  obj->var->getObjectType() != T_FUNCTION )))
	{
		LOG(LOG_CALLS,"accessing class:"<<name<<" "<< this->as<Class_base>()->getQualifiedClassName()<<" "<<nskind);
		if (obj->kind == INSTANCE_TRAIT &&
				nskind != NAMESPACE && 
				nskind != PACKAGE_INTERNAL_NAMESPACE && 
				nskind != STATIC_PROTECTED_NAMESPACE)
			throwError<TypeError>(kCallOfNonFunctionError,name.normalizedName());
	}

	if(obj->getter)
	{
		//Call the getter
		ASObject* target=this;
		if(target->classdef)
		{
			LOG(LOG_CALLS,_("Calling the getter on type ") << target->classdef->class_name);
		}
		else
		{
			LOG(LOG_CALLS,_("Calling the getter"));
		}
		IFunction* getter=obj->getter;
		target->incRef();
		ASObject* ret=getter->call(target,NULL,0);
		LOG(LOG_CALLS,_("End of getter"));
		// No incRef because ret is a new instance
		return _MNR(ret);
	}
	else
	{
		assert_and_throw(!obj->setter);
		assert_and_throw(obj->var);
		if(obj->var->getObjectType()==T_FUNCTION && obj->var->as<IFunction>()->isMethod())
		{
			LOG(LOG_CALLS,"Attaching this " << this << " to function " << name);
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
	varName.name_s_id=getSys()->getUniqueStringId(name);
	for (auto ns=namespaces.begin(); ns!=namespaces.end(); ns++)
		varName.ns.push_back(nsNameAndKind(*ns,NAMESPACE));
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
		if(it->first.nameId==next->first.nameId && it->first.ns==next->first.ns)
		{
			if(it->second.var==NULL && next->second.var==NULL)
				continue;

			if(it->second.var==NULL || next->second.var==NULL)
			{
				LOG(LOG_INFO, it->first.nameId << " " << it->first.ns);
				LOG(LOG_INFO, it->second.var << ' ' << it->second.setter << ' ' << it->second.getter);
				LOG(LOG_INFO, next->second.var << ' ' << next->second.setter << ' ' << next->second.getter);
				abort();
			}

			if(it->second.var->getObjectType()!=T_FUNCTION || next->second.var->getObjectType()!=T_FUNCTION)
			{
				LOG(LOG_INFO, it->first.nameId);
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
			case CONSTANT_TRAIT:
				kind="Declared: ";
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
		LOG(LOG_INFO, kind <<  '[' << it->first.ns << "] "<<
			getSys()->getStringFromUniqueId(it->first.nameId) << ' ' <<
			it->second.var << ' ' << it->second.setter << ' ' << it->second.getter);
	}
}

variables_map::~variables_map()
{
	destroyContents();
}

void variables_map::destroyContents()
{
	var_iterator it=Variables.begin();
	for(;it!=Variables.end();++it)
	{
		if(it->second.var)
			it->second.var->decRef();
		if(it->second.setter)
			it->second.setter->decRef();
		if(it->second.getter)
			it->second.getter->decRef();
	}
	Variables.clear();
}

ASObject::ASObject(MemoryAccount* m):Variables(m),classdef(NULL),proxyMultiName(NULL),
	type(T_OBJECT),traitsInitialized(false),constructIndicator(false),implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif
}

ASObject::ASObject(Class_base* c):Variables((c)?c->memoryAccount:NULL),classdef(NULL),proxyMultiName(NULL),
	type(T_OBJECT),traitsInitialized(false),constructIndicator(false),implEnable(true)
{
	setClass(c);
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif
}

ASObject::ASObject(const ASObject& o):Variables((o.classdef)?o.classdef->memoryAccount:NULL),classdef(NULL),proxyMultiName(NULL),
	type(o.type),traitsInitialized(false),constructIndicator(false),implEnable(true)
{
	if(o.classdef)
		setClass(o.classdef);

#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif

	assert_and_throw(o.Variables.size()==0);
}

void ASObject::setClass(Class_base* c)
{
	if(classdef)
	{
		classdef->abandonObject(this);
		classdef->decRef();
	}
	classdef=c;
	if(classdef)
	{
		classdef->acquireObject(this);
		classdef->incRef();
	}
}

void ASObject::finalize()
{
	Variables.destroyContents();
	if(classdef)
	{
		classdef->abandonObject(this);
		classdef->decRef();
		classdef=NULL;
	}
	if (proxyMultiName)
		delete proxyMultiName;
	proxyMultiName = NULL;
}

ASObject::~ASObject()
{
	finalize();
}

void variables_map::initSlot(unsigned int n, uint32_t nameId, const nsNameAndKind& ns)
{
	if(n>slots_vars.size())
		slots_vars.resize(n,Variables.end());

	var_iterator ret=Variables.find(varName(nameId,ns));

	if(ret==Variables.end())
	{
		//Name not present, no good
		throw RunTimeException("initSlot on missing variable");
	}

	slots_vars[n-1]=ret;
}

void variables_map::setSlot(unsigned int n,ASObject* o)
{
	validateSlotId(n);
	slots_vars[n-1]->second.setVar(o);
}

void variables_map::setSlotNoCoerce(unsigned int n,ASObject* o)
{
	validateSlotId(n);
	slots_vars[n-1]->second.setVarNoCoerce(o);
}

void variables_map::validateSlotId(unsigned int n) const
{
	if(n == 0 || n-1<slots_vars.size())
	{
		assert_and_throw(slots_vars[n-1]!=Variables.end());
		if(slots_vars[n-1]->second.setter)
			throw UnsupportedException("setSlot has setters");
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

		for(unsigned int i=0;i<index;i++)
			++it;

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
		LOG(LOG_CALLS,_("Calling the getter"));
		IFunction* getter=obj->getter;
		incRef();
		_R<ASObject> ret(getter->call(this,NULL,0));
		LOG(LOG_CALLS,_("End of getter"));
		return ret;
	}
	else
	{
		obj->var->incRef();
		return _MR(obj->var);
	}
}

tiny_string variables_map::getNameAt(unsigned int index) const
{
	//TODO: CHECK behaviour on overridden methods
	if(index<Variables.size())
	{
		const_var_iterator it=Variables.begin();

		for(unsigned int i=0;i<index;i++)
			++it;

		return getSys()->getStringFromUniqueId(it->first.nameId);
	}
	else
		throw RunTimeException("getNameAt out of bounds");
}

unsigned int ASObject::numVariables() const
{
	return Variables.size();
}

void ASObject::constructionComplete()
{
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
	//Pairs of name, value
	auto it=Variables.begin();
	for(;it!=Variables.end();it++)
	{
		if(it->second.kind!=DYNAMIC_TRAIT)
			continue;
		//Dynamic traits always have empty namespace
		assert(it->first.ns.hasEmptyName());
		out->writeStringVR(stringMap,getSys()->getStringFromUniqueId(it->first.nameId));
		it->second.var->serialize(out, stringMap, objMap, traitsMap);
	}
	//The empty string closes the object
	out->writeStringVR(stringMap, "");
}

void ASObject::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	//0x0A -> object marker
	out->writeByte(object_marker);
	//Check if the object has been already serialized to send it by reference
	auto it=objMap.find(this);
	if(it!=objMap.end())
	{
		//The least significant bit is 0 to signal a reference
		out->writeU29(it->second << 1);
		return;
	}

	Class_base* type=getClass();
	assert_and_throw(type);

	//Check if an alias is registered
	auto aliasIt=getSys()->aliasMap.begin();
	const auto aliasEnd=getSys()->aliasMap.end();
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

	if(type->isSubClass(InterfaceClass<IExternalizable>::getClass()))
	{
		//Custom serialization necessary
		if(!serializeTraits)
			throwError<TypeError>(kInvalidParamError);
		out->writeU29(0x7);
		out->writeStringVR(stringMap, alias);

		//Invoke writeExternal
		multiname writeExternalName(NULL);
		writeExternalName.name_type=multiname::NAME_STRING;
		writeExternalName.name_s_id=getSys()->getUniqueStringId("writeExternal");
		writeExternalName.ns.push_back(nsNameAndKind("",NAMESPACE));
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
	if(it2!=traitsMap.end())
		out->writeU29((it2->second << 2) | 1);
	else
	{
		traitsMap.insert(make_pair(type, traitsMap.size()));
		for(variables_map::const_var_iterator varIt=beginIt; varIt != endIt; ++varIt)
		{
			if(varIt->second.kind==DECLARED_TRAIT)
			{
				if(!varIt->first.ns.hasEmptyName())
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
				if(!varIt->first.ns.hasEmptyName())
				{
					//Skip variable with a namespace, like protected ones
					continue;
				}
				out->writeStringVR(stringMap, getSys()->getStringFromUniqueId(varIt->first.nameId));
			}
		}
	}
	for(variables_map::const_var_iterator varIt=beginIt; varIt != endIt; ++varIt)
	{
		if(varIt->second.kind==DECLARED_TRAIT)
		{
			if(!varIt->first.ns.hasEmptyName())
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
	xmlpp::DomParser p;
	xmlpp::Element* root=p.get_document()->create_root_node("type");

	// type attributes
	Class_base* prot=getClass();
	if(prot)
	{
		root->set_attribute("name", prot->getQualifiedClassName().raw_buf());
		if(prot->super)
			root->set_attribute("base", prot->super->getQualifiedClassName().raw_buf());
	}
	bool isDynamic = classdef && !classdef->isSealed;
	root->set_attribute("isDynamic", isDynamic?"true":"false");
	bool isFinal = classdef && classdef->isFinal;
	root->set_attribute("isFinal", isFinal?"true":"false");
	root->set_attribute("isStatic", "false");

	if(prot)
		prot->describeInstance(root);

	// TODO: undocumented constructor node

	return Class<XML>::getInstanceS(root);
}

tiny_string ASObject::toJSON(std::vector<ASObject *> &path, IFunction *replacer, const tiny_string &spaces,const tiny_string& filter)
{
	if (has_toJSON())
	{
		return call_toJSON();
	}

	tiny_string newline = (spaces.empty() ? "" : "\n");
	tiny_string res;
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
		for(variables_map::const_var_iterator varIt=beginIt; varIt != endIt; ++varIt)
		{
			// check for cylic reference
			if (!varIt->second.var->isPrimitive() && std::find(path.begin(),path.end(), varIt->second.var) != path.end())
				throwError<TypeError>(kJSONCyclicStructure);

			if (replacer != NULL)
			{
				if (!bfirst)
					res += ",";
				res += newline+spaces;
				res += "\"";
				res += getSys()->getStringFromUniqueId(varIt->first.nameId);
				res += "\"";
				res += ":";
				if (!spaces.empty())
					res += " ";
				ASObject* params[2];
				
				params[0] = Class<ASString>::getInstanceS(getSys()->getStringFromUniqueId(varIt->first.nameId));
				params[1] = varIt->second.var;
				params[1]->incRef();
				ASObject *funcret=replacer->call(getSys()->getNullRef(), params, 2);
				LOG(LOG_ERROR,"funcall:"<<res<<"|"<<funcret);
				if (funcret)
					res += funcret->toString();
				else
					res += varIt->second.var->toJSON(path,replacer,spaces+spaces,filter);
				bfirst = false;
			}
			else if (filter.empty() || filter.find(tiny_string(" ")+getSys()->getStringFromUniqueId(varIt->first.nameId)+" ") != tiny_string::npos)
			{
				if (!bfirst)
					res += ",";
				res += newline+spaces;
				res += "\"";
				res += getSys()->getStringFromUniqueId(varIt->first.nameId);
				res += "\"";
				res += ":";
				if (!spaces.empty())
					res += " ";
				res += varIt->second.var->toJSON(path,replacer,spaces+spaces,filter);
				bfirst = false;
			}
			path.push_back(varIt->second.var);
		}
		if (!bfirst)
			res += newline+spaces.substr_bytes(0,spaces.numBytes()/2);

		res += "}";
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
	prototypeName.name_s_id=getSys()->getUniqueStringId("prototype");
	prototypeName.ns.push_back(nsNameAndKind("",NAMESPACE));
	bool has_getter = false;
	variable* ret=findSettable(prototypeName,&has_getter);
	if(!ret && has_getter)
		throwError<ReferenceError>(kConstWriteError,
					   prototypeName.normalizedName(),
					   classdef ? classdef->as<Class_base>()->getQualifiedClassName() : "");
	if(!ret)
		ret = Variables.findObjVar(prototypeName,DYNAMIC_TRAIT,DECLARED_TRAIT|DYNAMIC_TRAIT);
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
