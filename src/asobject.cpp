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
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Date.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/toplevel/Error.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/display/DisplayObject.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/display/Stage.h"
#include "scripting/flash/utils/Dictionary.h"
#include "scripting/toplevel/Undefined.h"
#include "scripting/toplevel/Null.h"
#include <3rdparty/pugixml/src/pugixml.hpp>

using namespace lightspark;
using namespace std;

asfreelist::~asfreelist()
{
	for (int i = 0; i < freelistsize; i++)
		delete freelist[i];
	freelistsize = 0;
}

string ASObject::toDebugString() const
{
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
	else if (this->preparedforshutdown)
	{
		ret = "[object prepared for shutdown]";
	}
	else
	{
		assert(false);
	}
#ifndef NDEBUG
	assert(storedmembercount<=uint32_t(this->getRefCount()) || this->getConstant());
	char buf[300];
	if (this->getConstant())
		sprintf(buf,"(%p%s)",this,this->isConstructed()?"":" not constructed");
	else
		sprintf(buf,"(%p/%d/%d/%d%s)",this,this->getRefCount(),this->storedmembercount,this->getConstant(),this->isConstructed()?"":" not constructed");
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
		return getSystemState()->getSwfVersion() > 6 ? "undefined" : "";
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
			tiny_string s;
			bool isrefcounted;
			if (toPrimitive(ret,isrefcounted,getInstanceWorker()->needsActionScript3() ? STRING_HINT : NO_HINT))
				s=asAtomHandler::toString(ret,getInstanceWorker());
			if (isrefcounted)
				ASATOM_DECREF(ret);
			return s;
		}
	}
}

tiny_string ASObject::toLocaleString()
{
	asAtom str=asAtomHandler::invalidAtom;
	executeASMethod(str,"toLocaleString", {""}, nullptr, 0);
	if (asAtomHandler::isInvalid(str))
		return "";
	tiny_string ret = asAtomHandler::toString(str,getInstanceWorker());
	ASATOM_DECREF(str);
	return ret;
}

TRISTATE ASObject::isLess(ASObject* r)
{
	asAtom o = asAtomHandler::fromObject(r);
	asAtom ret = asAtomHandler::fromObject(this);
	TRISTATE res = TUNDEFINED;
	bool isrefcounted;
	if (toPrimitive(ret,isrefcounted))
		res = asAtomHandler::isLess(ret,getInstanceWorker(),o);
	if (isrefcounted)
		ASATOM_DECREF(ret);
	return res;
}
TRISTATE ASObject::isLessAtom(asAtom& r)
{
	asAtom ret = asAtomHandler::fromObject(this);
	TRISTATE res = TUNDEFINED;
	bool isrefcounted;
	if (toPrimitive(ret,isrefcounted))
		res = asAtomHandler::isLess(ret,getInstanceWorker(),r);
	if (isrefcounted)
		ASATOM_DECREF(ret);
	return res;
}

uint32_t ASObject::nextNameIndex(uint32_t cur_index)
{
	while (cur_index < Variables.dynamic_vars.size())
	{
		// skip deleted vars
		if (Variables.dynamic_vars[cur_index]==nullptr)
			++cur_index;
		else
			return cur_index+1;
	}
	if (getClass() == nullptr || getClass()->prototype.isNull())
		return 0;
	if (getClass()->prototype->getObj() != this)
	{
		uint32_t res = getClass()->prototype->getObj()->nextNameIndex(cur_index-Variables.dynamic_vars.size());
		if (res > 0)
			return res+Variables.dynamic_vars.size();
	}
	return 0;
}

void ASObject::nextName(asAtom& ret, uint32_t index)
{
	assert_and_throw(implEnable);

	bool nameIsInteger;
	uint32_t n = getNameAt(index-1,nameIsInteger);
	if (nameIsInteger)
	{
		const tiny_string& name = getSystemState()->getStringFromUniqueId(n);
		// not mentioned in the specs, but Adobe seems to convert names to Integers, if the key multiname is of type Integer
		asAtomHandler::setInt(ret,getInstanceWorker(),Integer::stringToASInteger(name.raw_buf(), 0));
	}
	else
		ret = asAtomHandler::fromStringID(n);
}

void ASObject::nextValue(asAtom& ret,uint32_t index)
{
	assert_and_throw(implEnable);

	getValueAt(ret,index-1);
}

void ASObject::AVM1enumerate(std::stack<asAtom>& stack)
{
	// add prototype vars first
	ASObject* pr = this->getprop_prototype();
	if (!pr)
		pr = this->getClass()->prototype->getObj();
	if (pr)
	{
		for (auto it = pr->Variables.dynamic_vars.begin(); it != pr->Variables.dynamic_vars.end(); it++)
		{
			if (*it == nullptr)
				continue;
			asAtom name = asAtomHandler::fromStringID((*it)->nameStringID);
			ACTIONRECORD::PushStack(stack, name);
		}
	}
	for (auto it = Variables.dynamic_vars.begin(); it != Variables.dynamic_vars.end(); it++)
	{
		if (*it == nullptr)
			continue;
		asAtom name = asAtomHandler::fromStringID((*it)->nameStringID);
		ACTIONRECORD::PushStack(stack, name);
	}
}

void ASObject::addOwnedObject(ASObject* obj)
{
	obj->incRef(); // will be decreffed when this is destructed
	obj->addStoredMember();
	ownedObjects.insert(obj);
}

void ASObject::sinit(Class_base* c)
{
	c->setDeclaredMethodByQName("hasOwnProperty",AS3,c->getSystemState()->getBuiltinFunction(hasOwnProperty,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true,true,6);
	c->setDeclaredMethodByQName("setPropertyIsEnumerable",AS3,c->getSystemState()->getBuiltinFunction(setPropertyIsEnumerable),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("isPrototypeOf",AS3,c->getSystemState()->getBuiltinFunction(isPrototypeOf,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("propertyIsEnumerable",AS3,c->getSystemState()->getBuiltinFunction(propertyIsEnumerable,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),NORMAL_METHOD,true);

	c->prototype->setVariableByQName("toString","",c->getSystemState()->getBuiltinFunction(_toString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",c->getSystemState()->getBuiltinFunction(_toLocaleString,0,Class<ASString>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("valueOf","",c->getSystemState()->getBuiltinFunction(valueOf,0,Class<ASObject>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("hasOwnProperty","",c->getSystemState()->getBuiltinFunction(hasOwnProperty,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT,6);
	c->prototype->setVariableByQName("isPrototypeOf","",c->getSystemState()->getBuiltinFunction(isPrototypeOf,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("propertyIsEnumerable","",c->getSystemState()->getBuiltinFunction(propertyIsEnumerable,1,Class<Boolean>::getRef(c->getSystemState()).getPtr()),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("setPropertyIsEnumerable","",c->getSystemState()->getBuiltinFunction(setPropertyIsEnumerable),DYNAMIC_TRAIT);
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
			bool res=false;
			bool isrefcounted;
			if (toPrimitive(primitive,isrefcounted))
			{
				asAtom o = asAtomHandler::fromObject(r);
				res = asAtomHandler::isEqual(primitive,getInstanceWorker(),o);
			}
			if (isrefcounted)
				ASATOM_DECREF(primitive);
			return res;
		}
		case T_BOOLEAN:
		{
			asAtom primitive=asAtomHandler::invalidAtom;
			bool res=false;
			bool isrefcounted;
			if (toPrimitive(primitive,isrefcounted))
				res = asAtomHandler::toNumber(primitive)==r->toNumber();
			if (isrefcounted)
				ASATOM_DECREF(primitive);
			return res;
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

//	LOG_CALL("Equal comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
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
	bool isrefcounted;
	toPrimitive(ret,isrefcounted,NUMBER_HINT);
	int32_t res = asAtomHandler::toInt(ret);
	if (isrefcounted)
		ASATOM_DECREF(ret);
	return res;
}

int64_t ASObject::toInt64()
{
	asAtom ret = asAtomHandler::fromObject(this);
	bool isrefcounted;
	toPrimitive(ret,isrefcounted,NUMBER_HINT);
	int32_t res = asAtomHandler::toInt64(ret);
	if (isrefcounted)
		ASATOM_DECREF(ret);
	return res;
}
number_t ASObject::toNumberForComparison()
{
	return toNumber();
}

/* Implements ECMA's ToPrimitive (9.1) and [[DefaultValue]] (8.6.2.6) */
bool ASObject::toPrimitive(asAtom& ret, bool& isrefcounted, TP_HINT hint)
{
	if (!getInstanceWorker()->needsActionScript3())
	{
		bool fromValueOf;
		return AVM1toPrimitive(ret,isrefcounted,fromValueOf,hint != NO_HINT);
	}
	isrefcounted=false;

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
		return true;
	}

	/* for HINT_STRING evaluate first toString, then valueOf
	 * for HINT_NUMBER do it the other way around */
	if(hint == STRING_HINT && has_toString())
	{
		call_toString(ret);
		if(asAtomHandler::isPrimitive(ret))
		{
			isrefcounted=true;
			return true;
		}
		if (this->getInstanceWorker()->currentCallContext && this->getInstanceWorker()->currentCallContext->exceptionthrown)
			return false;
		ASATOM_DECREF(ret);
	}
	if(has_valueOf())
	{
		call_valueOf(ret);
		if(asAtomHandler::isPrimitive(ret))
		{
			isrefcounted=true;
			return true;
		}
		if (this->getInstanceWorker()->currentCallContext && this->getInstanceWorker()->currentCallContext->exceptionthrown)
			return false;
		ASATOM_DECREF(ret);
	}
	if(hint != STRING_HINT && has_toString())
	{
		call_toString(ret);
		if(asAtomHandler::isPrimitive(ret))
		{
			isrefcounted=true;
			return true;
		}
		if (this->getInstanceWorker()->currentCallContext && this->getInstanceWorker()->currentCallContext->exceptionthrown)
			return false;
		ASATOM_DECREF(ret);
	}
	if (getInstanceWorker()->needsActionScript3())
		createError<TypeError>(getInstanceWorker(),kConvertToPrimitiveError,this->getClassName());
	else
	{
		tiny_string res;
		if(is<RootMovieClip>())
		{
			// handle special case for AVM1 root movie
			res="_level";
			res += Integer::toString(this->as<RootMovieClip>()->AVM1getLevel());
		}
		else if(is<IFunction>())
			res = "[type Function]";
		else
			res="[object Object]";
		return true;
	}
	return false;
}

bool ASObject::AVM1toPrimitive(asAtom& ret, bool& isrefcounted, bool& fromValueOf, bool fromAVM1Add2)
{
	assert(!getInstanceWorker()->needsActionScript3());
	fromValueOf=false;
	isrefcounted=false;
	if(isPrimitive())
	{
		ret = asAtomHandler::fromObject(this);
		return true;
	}
	if (!fromAVM1Add2)
	{
		call_toString(ret);
		if(asAtomHandler::isPrimitive(ret))
		{
			isrefcounted=true;
			return true;
		}
		ASATOM_DECREF(ret);
	}
	else
	{
		call_valueOf(ret);
		if(asAtomHandler::isPrimitive(ret))
		{
			fromValueOf=true;
			isrefcounted=true;
			return true;
		}
		if (!this->is<Date>() || getInstanceWorker()->AVM1getSwfVersion() > 5)
		{
			ASATOM_DECREF(ret);
			call_toString(ret);
			if(asAtomHandler::isPrimitive(ret))
			{
				isrefcounted=true;
				return true;
			}
		}
	}
	tiny_string res;
	if(is<RootMovieClip>())
	{
		// handle special case for AVM1 root movie
		res="_level";
		res += Integer::toString(this->as<RootMovieClip>()->AVM1getLevel());
	}
	else if(is<IFunction>())
		res = "[type Function]";
	else
		res="[type Object]";
	ret = asAtomHandler::fromString(getSystemState(),res);
	return true;
}

asAtom ASObject::callResolveMethod(const tiny_string& name, ASWorker* wrk)
{
	auto pr = getprop_prototype();
	asAtom ret = asAtomHandler::invalidAtom;

	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = sys->getUniqueStringId("__resolve", wrk->AVM1isCaseSensitive());
	m.isAttribute = false;

	for (uint8_t depth = 0; pr != nullptr; pr = pr->getprop_prototype(), ++depth)
	{
		if (depth == UINT8_MAX)
		{
			throw ScriptLimitException
			(
				"Reached maximum prototype recursion limit",
				ScriptLimitException::MaxPrototypeRecursion
			);
		}

		auto func = asAtomHandler::invalidAtom;

		GET_VARIABLE_RESULT varres = pr->AVM1getVariableByMultiname
		(
			func,
			m,
			GET_VARIABLE_OPTION
			(
				GET_VARIABLE_OPTION::DONT_CALL_GETTER |
				GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE
			),
			wrk,
			false
		);

		if (asAtomHandler::isInvalid(func))
			continue;

		auto thisObj = asAtomHandler::fromObject(this);
		auto strAtom = asAtomHandler::fromString(sys, name);
		asAtomHandler::callFunction
		(
			func,
			wrk,
			ret,
			thisObj,
			&strAtom,
			1,
			false,
			true,
			true,
			varres & GETVAR_ISGETTER
		);
		ASATOM_DECREF(func);
		break;
	}

	return ret;
}

bool ASObject::has_valueOf()
{
	multiname valueOfName(nullptr);
	valueOfName.name_type=multiname::NAME_STRING;
	valueOfName.name_s_id=BUILTIN_STRINGS::STRING_VALUEOF;
	valueOfName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	valueOfName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	valueOfName.isAttribute = false;
	return hasPropertyByMultiname(valueOfName, true, true,getInstanceWorker());
}

/* calls the valueOf function on this object
 * we cannot just call the c-function, because it can be overriden from AS3 code
 */
void ASObject::call_valueOf(asAtom& ret)
{
	ret = asAtomHandler::invalidAtom;
	multiname valueOfName(nullptr);
	valueOfName.name_type=multiname::NAME_STRING;
	valueOfName.name_s_id=BUILTIN_STRINGS::STRING_VALUEOF;
	valueOfName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	valueOfName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	valueOfName.isAttribute = false;

	asAtom o=asAtomHandler::invalidAtom;
	if(!getInstanceWorker()->needsActionScript3())
	{
		// AVM1 object has overwritten it's own prototype
		ASObject* pr = this->getprop_prototype();
		if (pr)
			pr->getVariableByMultiname(o,valueOfName,SKIP_IMPL,getInstanceWorker());

		// NOTE: Special case for `_global`, since it doesn't have a
		// prototype, and returns `undefined` instead.
		if (is<Global>() || (hasprop_prototype() && pr == nullptr))
		{
			ret = asAtomHandler::undefinedAtom;
			return;
		}
	}
	if (asAtomHandler::isInvalid(o))
		getVariableByMultiname(o,valueOfName,SKIP_IMPL,getInstanceWorker());
	if (!asAtomHandler::isFunction(o))
	{
		if (getInstanceWorker()->needsActionScript3())
			createError<TypeError>(getInstanceWorker(),kCallOfNonFunctionError, valueOfName.normalizedNameUnresolved(getSystemState()));
		ret = asAtomHandler::undefinedAtom;
	}
	else
	{
		asAtom v =asAtomHandler::fromObject(this);
		asAtomHandler::callFunction(o,getInstanceWorker(),ret,v,nullptr,0,false);
	}
	ASATOM_DECREF(o);
}

bool ASObject::has_toString()
{
	multiname toStringName(nullptr);
	toStringName.name_type=multiname::NAME_STRING;
	toStringName.name_s_id=BUILTIN_STRINGS::STRING_TOSTRING;
	toStringName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	toStringName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	toStringName.isAttribute = false;
	return ASObject::hasPropertyByMultiname(toStringName, true, true,this->getInstanceWorker());
}

/* calls the toString function on this object
 * we cannot just call the c-function, because it can be overriden from AS3 code
 */
void ASObject::call_toString(asAtom &ret)
{
	ret = asAtomHandler::invalidAtom;
	multiname toStringName(nullptr);
	toStringName.name_type=multiname::NAME_STRING;
	toStringName.name_s_id=BUILTIN_STRINGS::STRING_TOSTRING;
	toStringName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	toStringName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	toStringName.isAttribute = false;
	asAtom o=asAtomHandler::invalidAtom;
	if(!getInstanceWorker()->needsActionScript3())
	{
		// AVM1 object has overwritten it's own prototype
		ASObject* pr = this->getprop_prototype();
		if (pr)
			pr->getVariableByMultiname(o,toStringName,SKIP_IMPL,getInstanceWorker());

		// NOTE: Special case for `_global`, since it doesn't have a
		// prototype, and returns `undefined` instead.
		if (is<Global>() || (hasprop_prototype() && pr == nullptr))
		{
			ret = asAtomHandler::undefinedAtom;
			return;
		}
	}
	if (asAtomHandler::isInvalid(o))
		getVariableByMultiname(o,toStringName,SKIP_IMPL,getInstanceWorker());
	if (!asAtomHandler::isFunction(o))
	{
		if (getInstanceWorker()->needsActionScript3())
			createError<TypeError>(getInstanceWorker(), kCallOfNonFunctionError, toStringName.normalizedNameUnresolved(getSystemState()));
	}
	else
	{
		asAtom v =asAtomHandler::fromObject(this);
		asAtomHandler::callFunction(o,getInstanceWorker(),ret,v,nullptr,0,false);
	}
	ASATOM_DECREF(o);
}

tiny_string ASObject::call_toJSON(bool& ok,std::vector<ASObject *> &path, asAtom replacer, const tiny_string &spaces,const tiny_string& filter)
{
	tiny_string res;
	ok = false;
	multiname toJSONName(nullptr);
	toJSONName.name_type=multiname::NAME_STRING;
	toJSONName.name_s_id=getSystemState()->getUniqueStringId("toJSON");
	toJSONName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	toJSONName.ns.emplace_back(getSystemState(),BUILTIN_STRINGS::STRING_AS3NS,NAMESPACE);
	toJSONName.isAttribute = false;
	if (!ASObject::hasPropertyByMultiname(toJSONName, true, true,getInstanceWorker()))
		return res;

	asAtom o=asAtomHandler::invalidAtom;
	getVariableByMultiname(o,toJSONName,SKIP_IMPL,getInstanceWorker());
	if (!asAtomHandler::isFunction(o))
	{
		ASATOM_DECREF(o);
		return res;
	}
	asAtom v=asAtomHandler::fromObject(this);
	asAtom ret=asAtomHandler::invalidAtom;
	asAtomHandler::callFunction(o,getInstanceWorker(), ret,v,nullptr,0,false);
	ASATOM_DECREF(o);
	if (getInstanceWorker()->currentCallContext && getInstanceWorker()->currentCallContext->exceptionthrown)
		return res;
	if (asAtomHandler::isString(ret))
	{
		res += "\"";
		res += asAtomHandler::toString(ret,getInstanceWorker());
		res += "\"";
	}
	else
		res = asAtomHandler::toObject(ret,getInstanceWorker())->toJSON(path,replacer,spaces,filter);
	ASATOM_DECREF(ret);
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

variable* variables_map::findObjVar(uint32_t nameId, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds, bool isEnumerable)
{
	var_iterator ret=Variables.find(nameId);
	while(ret!=Variables.end() && ret->first==nameId)
	{
		if (ret->second.ns == ns)
		{
			if(!(ret->second.kind & traitKinds))
			{
				assert(createKind==NO_CREATE_TRAIT);
				return nullptr;
			}
			return &ret->second;
		}
		ret++;
	}

	//Name not present, insert it if we have to create it
	if(createKind==NO_CREATE_TRAIT)
		return nullptr;

	var_iterator inserted=Variables.insert(Variables.cbegin(),make_pair(nameId, variable(createKind,ns,false,nameId)) );
	inserted->second.isenumerable = isEnumerable;
	insertVar(&inserted->second);
	return &inserted->second;
}

bool ASObject::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk)
{
	//We look in all the object's levels
	uint32_t validTraits=DECLARED_TRAIT;
	if(considerDynamic)
		validTraits|=DYNAMIC_TRAIT;

	if(Variables.findObjVar(getInstanceWorker(),name, validTraits)!=nullptr)
		return true;

	if(classdef && classdef->borrowedVariables.findObjVar(getInstanceWorker(),name, DECLARED_TRAIT)!=nullptr)
		return true;

	//Check prototype inheritance chain
	if(getClass() && considerPrototype)
	{
		Prototype* proto = getClass()->getPrototype(wrk);
		while(proto)
		{
			if(proto->getObj()->findGettable(name) != nullptr)
				return true;
			proto=proto->prevPrototype.getPtr();
		}
	}

	//Must not ask for non borrowed traits as static class member are not valid
	return false;
}

void ASObject::setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable,uint8_t min_swfversion)
{
	setDeclaredMethodByQName(name, nsNameAndKind(getSystemState(),ns, NAMESPACE), o, type, isBorrowed,isEnumerable,min_swfversion);
}

void ASObject::setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable,uint8_t min_swfversion)
{
	setDeclaredMethodByQName(getSystemState()->getUniqueStringId(name), ns, o, type, isBorrowed,isEnumerable,min_swfversion);
}

void ASObject::setDeclaredMethodByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable,uint8_t min_swfversion)
{
	check();
#ifndef NDEBUG
	assert(!initialized);
#endif
	assert(o->is<IFunction>());
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
	if(isBorrowed && o->as<IFunction>()->inClass == nullptr)
		o->as<IFunction>()->inClass = this->as<Class_base>();
	o->as<IFunction>()->isStatic = !isBorrowed;
	o->as<IFunction>()->isGetter = type==METHOD_TYPE::GETTER_METHOD;
	o->as<IFunction>()->isSetter = type==METHOD_TYPE::SETTER_METHOD;
	o->as<IFunction>()->namespaceNameID=ns.kind == NAMESPACE  ? ns.nsNameId : (uint32_t)BUILTIN_STRINGS::EMPTY;

	variable* obj=nullptr;
	if(isBorrowed)
	{
		assert(this->is<Class_base>());
		obj=this->as<Class_base>()->borrowedVariables.findObjVar(nameId,ns,DECLARED_TRAIT, DECLARED_TRAIT);
	}
	else
	{
		obj=Variables.findObjVar(nameId,ns,DECLARED_TRAIT, DECLARED_TRAIT);
	}
	if(this->is<Class_base>())
	{
		o->setRefConstant();
		Class_base* cls = this->as<Class_base>()->super.getPtr();
		while (cls)
		{
			cls->addoverriddenmethod(nameId);
			cls = cls->super.getPtr();
		}
	}

	obj->isenumerable = isEnumerable;
	obj->min_swfversion = min_swfversion;
	switch(type)
	{
		case NORMAL_METHOD:
		{
			asAtom v = asAtomHandler::fromObject(o);
			obj->setVarNoCoerce(v);
			break;
		}
		case GETTER_METHOD:
		{
			ASATOM_REMOVESTOREDMEMBER(obj->getter);
			obj->getter=asAtomHandler::fromObject(o);
			break;
		}
		case SETTER_METHOD:
		{
			ASATOM_REMOVESTOREDMEMBER(obj->setter);
			obj->setter=asAtomHandler::fromObject(o);
			break;
		}
	}
	if (type != SETTER_METHOD)
	{
		if (o->as<IFunction>()->getReturnType(true))
			obj->setResultType(o->as<IFunction>()->getReturnType(true));
	}
	o->as<IFunction>()->functionname = nameId;
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
			obj->setVar(this->getInstanceWorker(),f,!o->getConstant());
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

bool ASObject::deleteVariableByMultiname_intern(const multiname& name, ASWorker* wrk)
{
	variable* obj=Variables.findObjVar(getInstanceWorker(),name,NO_CREATE_TRAIT,DYNAMIC_TRAIT|DECLARED_TRAIT);

	if(obj==nullptr)
	{
		if (classdef && classdef->isSealed)
		{
			if (classdef->Variables.findObjVar(getInstanceWorker(),name, DECLARED_TRAIT)!=nullptr)
				createError<ReferenceError>(getInstanceWorker(),kDeleteSealedError,name.normalizedNameUnresolved(getSystemState()),classdef->getName());
			return false;
		}

		// fixed properties cannot be deleted
		if (hasPropertyByMultiname(name,true,true,wrk))
			return false;

		//unknown variables must return true
		return true;
	}
	//Only dynamic traits are deletable
	if (obj->kind != DYNAMIC_TRAIT && obj->kind != INSTANCE_TRAIT)
		return false;

	assert(asAtomHandler::isInvalid(obj->getter) && asAtomHandler::isInvalid(obj->setter) && asAtomHandler::isValid(obj->var));

	ASObject* o = asAtomHandler::getObject(obj->var);
	//Now kill the variable
	Variables.killObjVar(getInstanceWorker(),name);
	//Now dereference the value
	if (o)
		o->removeStoredMember();

	return true;
}

//In all setter we first pass the value to the interface to see if special handling is possible
void ASObject::setVariableByMultiname_i(multiname& name, int32_t value, ASWorker* wrk)
{
	check();
	asAtom v = asAtomHandler::fromInt(value);
	setVariableByMultiname(name,v,CONST_NOT_ALLOWED,nullptr,wrk);
}

variable* ASObject::findSettableImpl(ASWorker* wrk,variables_map& map, const multiname& name, bool* has_getter)
{
	variable* ret=map.findVarOrSetter(wrk,name,DECLARED_TRAIT|DYNAMIC_TRAIT);
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
	return findSettableImpl(getInstanceWorker(),Variables, name, has_getter);
}

multiname *ASObject::setVariableByMultiname_intern(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, Class_base* cls, bool *alreadyset, ASWorker* wrk)
{
	multiname *retval = nullptr;
	check();
	assert(!cls || classdef->isSubClass(cls));
	bool isAS3 = is<DisplayObject>() ? as<DisplayObject>()->needsActionScript3() :
	(
		sys->mainClip->needsActionScript3() &&
		worker->needsActionScript3()
	);
	//NOTE: we assume that [gs]etSuper and [sg]etProperty correctly manipulate the cur_level (for getActualClass)
	bool has_getter=false;
	variable* obj=findSettable(name, &has_getter);
	if (obj && (obj->kind == CONSTANT_TRAIT && allowConst==CONST_NOT_ALLOWED))
	{
		if (asAtomHandler::isFunction(obj->var) || asAtomHandler::isValid(obj->setter))
			createError<ReferenceError>(getInstanceWorker(),kCannotAssignToMethodError, name.normalizedNameUnresolved(getSystemState()), classdef->as<Class_base>()->getQualifiedClassName());
		else
			createError<ReferenceError>(getInstanceWorker(),kConstWriteError, name.normalizedNameUnresolved(getSystemState()), classdef->as<Class_base>()->getQualifiedClassName());
		return nullptr;
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
			createError<ReferenceError>(getInstanceWorker(),kCannotAssignToMethodError, name.normalizedNameUnresolved(getSystemState()), cls ? cls->getQualifiedClassName() : "");
			return nullptr;
		}
		// no setter and dynamic variables are allowed, so we force creation of a dynamic variable
		if (obj && asAtomHandler::isInvalid(obj->setter))
			obj=nullptr;
	}
	if (!obj && cls && !isAS3)
	{
		// we are in AVM1, look for setter in prototype
		bool has_getter=false;
		obj = cls->findSettableInPrototype(name,&has_getter);
		if (has_getter)
		{
			// found getter without setter in prototype
			return nullptr;
		}
		if (obj && asAtomHandler::isInvalid(obj->setter))
			obj=nullptr;
	}
	//Do not set variables in prototype chain. Still have to do
	//lookup to throw a correct error in case a named function
	//exists in prototype chain. See Tamarin test
	//ecma3/Boolean/ecma4_sealedtype_1_rt
	if(!obj && cls && cls->isSealed)
	{
		variable *protoObj = cls->findSettableInPrototype(name,nullptr);
		if (protoObj &&
			((asAtomHandler::isFunction(protoObj->var)) ||
			 asAtomHandler::isValid(protoObj->setter)))
		{
			createError<ReferenceError>(getInstanceWorker(),kCannotAssignToMethodError, name.normalizedNameUnresolved(getSystemState()), cls ? cls->getQualifiedClassName() : "");
			return nullptr;
		}
	}

	if(!obj)
	{
		if(has_getter)  // Is this a read-only property?
		{
			if (isAS3)
				createError<ReferenceError>(getInstanceWorker(), kConstWriteError, name.normalizedNameUnresolved(getSystemState()), cls ? cls->getQualifiedClassName() : "");
			else
				ASATOM_DECREF(o);
			return nullptr;
		}

		// Properties can not be added to a sealed class
		// NOTE: In AVM1, all classes are treated as if they were dynamic.
		if (cls && cls->isSealed && isAS3)
		{
			ABCContext* c = nullptr;
			c = wrk->currentCallContext ? wrk->currentCallContext->mi->context : nullptr;
			Type* type =c ? Type::getTypeFromMultiname(&name,c) : nullptr;
			if (type)
				createError<ReferenceError>(getInstanceWorker(), kConstWriteError, name.normalizedNameUnresolved(getSystemState()), cls ? cls->getQualifiedClassName() : "");
			else
				createError<ReferenceError>(getInstanceWorker(), kWriteSealedError, name.normalizedNameUnresolved(getSystemState()), cls->getQualifiedClassName());
			return nullptr;
		}

		obj=Variables.findObjVar(getInstanceWorker(),name,NO_CREATE_TRAIT,DYNAMIC_TRAIT);
		//Create a new dynamic variable
		if(!obj)
		{
			if(this->is<Global>() && !name.hasGlobalNS)
			{
				createError<ReferenceError>(getInstanceWorker(), kWriteSealedError, name.normalizedNameUnresolved(getSystemState()), this->getClassName());
				return nullptr;
			}

			uint32_t nameID = name.normalizedNameId(getInstanceWorker());
			variables_map::var_iterator inserted=Variables.Variables.insert(Variables.Variables.cbegin(),
				make_pair(nameID,variable(DYNAMIC_TRAIT,name.ns.size() == 1 ? name.ns[0] : nsNameAndKind(),name.isInteger,nameID)));
			Variables.insertVar(&inserted->second);
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
		LOG_CALL("Calling the setter "<<obj->type<<" "<<obj->slotid);
		//One argument can be passed without creating an array
		ASObject* target=this;
		asAtom* arg1 = &o;

		asAtom v =asAtomHandler::fromObject(target);
		asAtom ret=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(obj->setter,wrk,ret,v,arg1,1,false,true,true,true);
		if (asAtomHandler::is<SyntheticFunction>(obj->setter))
			retval = asAtomHandler::as<SyntheticFunction>(obj->setter)->getSimpleName();
		ASATOM_DECREF(ret);
		if (asAtomHandler::is<SyntheticFunction>(o))
		{
			if (obj->kind == CONSTANT_TRAIT)
				asAtomHandler::getObjectNoCheck(o)->setRefConstant();
		}
		if (alreadyset)
			*alreadyset=false;
		ASATOM_DECREF(o);
		// it seems that Adobe allows setters with return values...
		//assert_and_throw(asAtomHandler::isUndefined(ret));
		LOG_CALL("End of setter");
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
				obj->setVar(wrk,o);
				*alreadyset = o.uintval != obj->var.uintval; // setVar may coerce the object into a new instance, so we need to check if decRef is necessary
			}
		}
		else
			obj->setVar(wrk,o);
		if (isfunc)
		{
			if (obj->kind == CONSTANT_TRAIT)
				asAtomHandler::getObjectNoCheck(o)->setRefConstant();
		}
	}
	return retval;
}

void ASObject::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable, uint8_t min_swfversion)
{
	const nsNameAndKind tmpns(getSystemState(),ns, NAMESPACE);
	setVariableByQName(name, tmpns, o, traitKind,isEnumerable,min_swfversion);
}

void ASObject::setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable, uint8_t min_swfversion)
{
	setVariableByQName(getSystemState()->getUniqueStringId(name), ns, o, traitKind,isEnumerable,min_swfversion);
}

variable* ASObject::setVariableByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable, uint8_t min_swfversion)
{
	asAtom v = asAtomHandler::fromObject(o);
	return setVariableAtomByQName(nameId, ns, v, traitKind, isEnumerable,true,min_swfversion);
}
variable *ASObject::setVariableAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable, uint8_t min_swfversion)
{
	return setVariableAtomByQName(getSystemState()->getUniqueStringId(name), ns, o, traitKind,isEnumerable,true,min_swfversion);
}
variable *ASObject::setVariableAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable, bool isRefcounted,uint8_t min_swfversion)
{
	variable* obj=Variables.findObjVar(nameId,ns,traitKind,traitKind,isEnumerable);
	obj->setVar(this->getInstanceWorker(),o,isRefcounted && asAtomHandler::isObject(o) && !asAtomHandler::getObjectNoCheck(o)->getConstant());
	obj->min_swfversion=min_swfversion;
	return obj;
}

void ASObject::initializeVariableByMultiname(multiname& name, asAtom &o, multiname* typemname,
		ABCContext* context, TRAIT_KIND traitKind, uint32_t slot_id, bool isenumerable)
{
	check();
	Variables.initializeVar(name, o, typemname, context, traitKind,this,slot_id,isenumerable);
}

variable::variable(TRAIT_KIND _k, asAtom _v, multiname* _t, Type* _type, const nsNameAndKind& _ns, bool _isenumerable, bool _nameIsInteger, uint32_t nameID)
		: var(_v),typeUnion(nullptr),setter(asAtomHandler::invalidAtom),getter(asAtomHandler::invalidAtom)
		,nameStringID(nameID),ns(_ns),slotid(0),kind(_k)
		,isResolved(false),isenumerable(_isenumerable),issealed(false),isrefcounted(true),nameIsInteger(_nameIsInteger),min_swfversion(0)
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
	ASObject* o = asAtomHandler::getObject(var);
	if (o && !o->getConstant())
		o->addStoredMember();
}
void variable::setVar(ASWorker* wrk, asAtom v, bool _isrefcounted)
{
	//Resolve the typename if we have one
	//currentCallContext may be NULL when inserting legacy
	//children, which is done outside any ABC context
	if(!isResolved && traitTypemname)
	{
		type = Type::getTypeFromMultiname(traitTypemname, wrk->currentCallContext->mi->context);
		assert(type);
		isResolved=true;
	}
	if(isResolved && type)
	{
		type->coerce(wrk,v);
		if (wrk->currentCallContext && wrk->currentCallContext->exceptionthrown)
			return;
	}
	asAtom oldvar = var;
	var=v;
	if(isrefcounted && asAtomHandler::isObject(oldvar))
	{
		LOG_CALL("remove old var:"<<asAtomHandler::toDebugString(oldvar));
		asAtomHandler::getObjectNoCheck(oldvar)->removeStoredMember();
	}
	if(asAtomHandler::isObject(v) && _isrefcounted)
		asAtomHandler::getObjectNoCheck(v)->addStoredMember();
	isrefcounted = _isrefcounted;
}

void variables_map::killObjVar(ASWorker* wrk,const multiname& mname)
{
	uint32_t name=mname.normalizedNameId(wrk);
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
			removeVar(&ret->second);
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

Class_base* variables_map::getSlotType(unsigned int n,ABCContext* context)
{
	variable* v = slots_vars[n-1];
	if (!v->isResolved && v->traitTypemname)
	{
		v->type = Type::getTypeFromMultiname(v->traitTypemname,context);
		assert_and_throw(v->type);
		v->isResolved=true;
	}
	return v->isResolved ? dynamic_cast<Class_base*>(v->type) : nullptr;
}

variable* variables_map::findObjVar(ASWorker* wrk,const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds)
{
	uint32_t name=mname.name_type == multiname::NAME_STRING ? mname.name_s_id : mname.normalizedNameId(wrk);

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
				return nullptr;
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
		return nullptr;
	if(createKind == DYNAMIC_TRAIT)
	{
		var_iterator inserted=Variables.insert(Variables.cbegin(),
			make_pair(name,variable(createKind,nsNameAndKind(),mname.isInteger,name)));
		insertVar(&inserted->second);
		return &inserted->second;
	}
	assert(mname.ns.size() == 1);
	var_iterator inserted=Variables.insert(Variables.cbegin(),
		make_pair(name,variable(createKind,mname.ns[0],mname.isInteger,name)));
	insertVar(&inserted->second);
	return &inserted->second;
}

void variables_map::initializeVar(multiname& mname, asAtom& obj, multiname* typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj, uint32_t slot_id,bool isenumerable)
{
	Type* type = nullptr;
	if (typemname->isStatic)
		type = typemname->cachedType;

	asAtom value=asAtomHandler::invalidAtom;
	 /* If typename is a builtin type, we coerce obj.
	  * It it's not it must be a user defined class,
	  * so we try to find the class it is derived from and create an apropriate uninitialized instance */
	if (type == nullptr)
		type = Type::getBuiltinType(mainObj->getInstanceWorker(),typemname);
	if (type == nullptr)
		type = Type::getTypeFromMultiname(typemname,context,true);
	if(type==nullptr)
	{
		if (asAtomHandler::isInvalid(obj)) // create dynamic object
		{
			value = asAtomHandler::undefinedAtom;
		}
		else
		{
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
				&& mainObj->as<Class_base>()->class_name.nameId == typemname->normalizedNameId(mainObj->getInstanceWorker())
				&& mainObj->as<Class_base>()->class_name.nsStringId == typemname->ns[0].nsNameId
				&& typemname->ns[0].kind == NAMESPACE)
			{
				// avoid recursive construction
				value = asAtomHandler::undefinedAtom;
			}
			else
			{
				value = asAtomHandler::undefinedAtom;
				type->coerce(mainObj->getInstanceWorker(),value);
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

	uint32_t name=mname.normalizedNameId(mainObj->getInstanceWorker());
	auto it = Variables.insert(Variables.cbegin(),make_pair(name, variable(traitKind, value, typemname, type,mname.ns[0],isenumerable,mname.isInteger,name)));
	insertVar(&it->second);
	if (slot_id)
		initSlot(slot_id,&(it->second));
}

ASFUNCTIONBODY_ATOM(ASObject,generator)
{
	//By default we assume it's a passthrough cast
	if(argslen==1)
	{
		LOG_CALL("Passthrough of " << asAtomHandler::toDebugString(args[0]));
		ASATOM_INCREF(args[0]);
		ret = args[0];
	}
	else if (argslen > 1)
		createError<ArgumentError>(wrk,kCoerceArgumentCountError,Integer::toString(argslen));
	else
		ret = asAtomHandler::fromObject(new_asobject(wrk));
}

ASFUNCTIONBODY_ATOM(ASObject,_toString)
{
	tiny_string res;
	if(wrk->needsActionScript3())
	{
		if (asAtomHandler::is<Class_base>(obj))
		{
			res="[class ";
			res+=asAtomHandler::as<Class_base>(obj)->as<Class_base>()->class_name.getQualifiedName(wrk->getSystemState(),false);
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
		else if(asAtomHandler::getClass(obj,wrk->getSystemState()))
		{
			res="[object ";
			res+=asAtomHandler::getClass(obj,wrk->getSystemState())->class_name.getQualifiedName(wrk->getSystemState(),false);
			res+="]";
		}
		else
			res="[object Object]";
	}
	else
	{
		if(asAtomHandler::is<RootMovieClip>(obj))
		{
			// handle special case for AVM1 root movie
			res="_level";
			res += Integer::toString(asAtomHandler::as<RootMovieClip>(obj)->AVM1getLevel());
		}
		else if(asAtomHandler::is<IFunction>(obj))
			res = "[type Function]";
		else
			res="[object Object]";
	}
	ret = asAtomHandler::fromString(wrk->getSystemState(),res);
}

ASFUNCTIONBODY_ATOM(ASObject,_toLocaleString)
{
	_toString(ret,wrk,obj,args,argslen);
}

ASFUNCTIONBODY_ATOM(ASObject,hasOwnProperty)
{
	if (argslen==0)
	{
		ret = asAtomHandler::falseAtom;
		return;
	}
	multiname name(nullptr);
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
			name.name_s_id=asAtomHandler::toStringId(args[0],wrk);
			if (name.name_s_id == BUILTIN_STRINGS::EMPTY)
			{
				ret = asAtomHandler::falseAtom;
				return;
			}
			name.isInteger=Array::isIntegerWithoutLeadingZeros(wrk->getSystemState()->getStringFromUniqueId(name.name_s_id)) ;
			break;
	}
	name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	asAtomHandler::setBool(ret,asAtomHandler::hasPropertyByMultiname(obj,name, true, false,wrk));
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
	Class_base* cls = asAtomHandler::toObject(args[0],wrk)->getClass();

	while (cls != nullptr)
	{
		if (cls->getPrototype(wrk)->getObj() == asAtomHandler::getObject(obj))
		{
			res = true;
			break;
		}
		Class_base* clsparent = cls->getPrototype(wrk)->getObj()->getClass();
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
	multiname name(nullptr);
	if ((asAtomHandler::isInteger(args[0]) || asAtomHandler::isUInteger(args[0]))&& asAtomHandler::toInt(args[0])>=0)
	{
		name.name_type=multiname::NAME_INT;
		name.name_i=asAtomHandler::toInt(args[0]);
		name.isInteger=true;
	}
	else
	{
		name.name_type=multiname::NAME_STRING;
		name.name_s_id=asAtomHandler::toStringId(args[0],wrk);
	}
	name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	if (asAtomHandler::is<Array>(obj)) // propertyIsEnumerable(index) isn't mentioned in the ECMA specs but is tested for
	{
		Array* a = asAtomHandler::as<Array>(obj);
		unsigned int index = 0;
		if (a->isValidMultiname(wrk,name,index))
		{
			asAtomHandler::setBool(ret,index < (unsigned int)a->size());
			return;
		}
	}
	else if (asAtomHandler::is<Dictionary>(obj))
	{
		if (asAtomHandler::as<Dictionary>(obj)->hasPropertyByMultiname(name,true,false,wrk))
		{
			asAtomHandler::setBool(ret,true);
			return;
		}
	}
	variable* v = asAtomHandler::toObject(obj,wrk)->Variables.findObjVar(wrk,name, NO_CREATE_TRAIT,DYNAMIC_TRAIT|DECLARED_TRAIT);
	if (v)
		asAtomHandler::setBool(ret,v->isenumerable);
	else
		asAtomHandler::setBool(ret,asAtomHandler::getObject(obj)->hasPropertyByMultiname(name,true,false,wrk));
}
ASFUNCTIONBODY_ATOM(ASObject,setPropertyIsEnumerable)
{
	tiny_string propname;
	bool isEnum;
	ARG_CHECK(ARG_UNPACK(propname) (isEnum, true));
	if (asAtomHandler::is<Dictionary>(obj)) // it seems that all entries in dictionary are always enumerable
		return;
	multiname name(nullptr);
	name.name_type=multiname::NAME_STRING;
	name.name_s_id=asAtomHandler::toStringId(args[0],wrk);
	name.ns.emplace_back(wrk->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
	name.isAttribute=false;
	asAtomHandler::toObject(obj,wrk)->setIsEnumerable(name, isEnum);
}
ASFUNCTIONBODY_ATOM(ASObject,addProperty)
{
	ret = asAtomHandler::falseAtom;
	if (argslen < 3)
		return;
	asAtom name = asAtomHandler::invalidAtom;
	_NR<IFunction> getter;
	_NR<IFunction> setter;
	ARG_CHECK(ARG_UNPACK(name,asAtomHandler::invalidAtom)(getter, NullRef)(setter, NullRef));
	if (asAtomHandler::isInvalid(name) )
		return;
	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.name_s_id = asAtomHandler::toStringId(name,wrk);
	m.ns.push_back(nsNameAndKind());
	if (m.name_s_id == BUILTIN_STRINGS::EMPTY)
		return;
	asAtomHandler::toObject(obj,wrk)->deleteVariableByMultiname_intern(m,wrk);
	if (!getter.isNull())
	{
		if (!getter->is<IFunction>())
			return;
		ret = asAtomHandler::trueAtom;
		getter->incRef();
		getter->addStoredMember();
		asAtomHandler::toObject(obj,wrk)->setDeclaredMethodByQName(m.name_s_id,nsNameAndKind(),getter.getPtr(),GETTER_METHOD,false);
	}
	if (!setter.isNull())
	{
		if (!setter->is<IFunction>())
			return;
		ret = asAtomHandler::trueAtom;
		setter->incRef();
		setter->addStoredMember();
		asAtomHandler::toObject(obj,wrk)->setDeclaredMethodByQName(m.name_s_id,nsNameAndKind(),setter.getPtr(),SETTER_METHOD,false);
	}
}
ASFUNCTIONBODY_ATOM(ASObject,registerClass)
{
	ret = asAtomHandler::falseAtom;
	tiny_string name;
	_NR<IFunction> theClassConstructor;
	ARG_CHECK(ARG_UNPACK(name)(theClassConstructor));
	if (name.empty())
		return;
	if (!theClassConstructor.isNull())
	{
		ApplicationDomain* appDomain = nullptr;
		if (theClassConstructor->is<AVM1Function>())
			appDomain = theClassConstructor->as<AVM1Function>()->getClip()->loadedFrom;
		if (!appDomain)
			appDomain = wrk->getSystemState()->mainClip->applicationDomain.getPtr();
		ret = asAtomHandler::fromBool(appDomain->AVM1registerTagClass(name,theClassConstructor));
	}
}
ASFUNCTIONBODY_ATOM(ASObject,AVM1_IgnoreSetter)
{
	// this is for AVM1 readonly getters, it does nothing and is only needed to avoid an exception if setter is called
}

void ASObject::setIsEnumerable(const multiname &name, bool isEnum)
{
	variable* v = Variables.findObjVar(getInstanceWorker(),name, NO_CREATE_TRAIT,DYNAMIC_TRAIT);
	if (v && v->isenumerable != isEnum)
	{
		v->isenumerable = isEnum;
		if (isEnum)
			Variables.insertVar(v);
		else
			Variables.removeVar(v);
	}
}

bool ASObject::cloneInstance(ASObject *target)
{
	return Variables.cloneInstance(target->Variables);
}

asAtom ASObject::getVariableBindingValue(const tiny_string &name)
{
	uint32_t nameId = getSystemState()->getUniqueStringId(name);
	asAtom obj=asAtomHandler::invalidAtom;
	multiname objName(NULL);
	objName.name_type=multiname::NAME_STRING;
	objName.name_s_id=nameId;
	getVariableByMultiname(obj,objName,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
	return obj;
}

ASFUNCTIONBODY_ATOM(ASObject,_constructor)
{
}

ASFUNCTIONBODY_ATOM(ASObject,_constructorNotInstantiatable)
{
	createError<ArgumentError>(wrk,kCantInstantiateError, asAtomHandler::toObject(obj,wrk)->getClassName());
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
		uint32_t nameId = (*it)->normalizedNameId(getInstanceWorker());
		auto ret=Variables.Variables.find(nameId);

		assert_and_throw(ret!=Variables.Variables.end());
		Variables.initSlot(++n,&(ret->second));
	}
}

int32_t ASObject::getVariableByMultiname_i(const multiname& name, ASWorker* wrk)
{
	check();

	asAtom ret=asAtomHandler::invalidAtom;
	getVariableByMultinameIntern(ret,name,this->getClass(),GET_VARIABLE_OPTION::NONE,wrk);
	assert_and_throw(asAtomHandler::isValid(ret));
	return asAtomHandler::toInt(ret);
}

variable* ASObject::findVariableByMultiname(const multiname& name, Class_base* cls, uint32_t *nsRealID, bool *isborrowed, bool considerdynamic,ASWorker* wrk)
{
	//Get from the current object without considering borrowed properties
	variable* obj=Variables.findObjVar(getInstanceWorker(),name,name.hasEmptyNS || considerdynamic ? DECLARED_TRAIT|DYNAMIC_TRAIT : DECLARED_TRAIT,nsRealID);
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
			obj= ASObject::findGettableImpl(getInstanceWorker(), cls->borrowedVariables,name,nsRealID);
			if(!obj && name.hasEmptyNS)
			{
				//Check prototype chain
				Prototype* proto = cls->getPrototype(wrk);
				while(proto)
				{
					obj=proto->getObj()->Variables.findObjVar(getInstanceWorker(),name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealID);
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

GET_VARIABLE_RESULT ASObject::getVariableByMultinameIntern(asAtom &ret, const multiname& name, Class_base* cls, GET_VARIABLE_OPTION opt,ASWorker* wrk)
{
	check();
	assert(!cls || classdef->isSubClass(cls) || this->is<Class_base>());
	assert(wrk==getWorker());
	uint32_t nsRealId;
	GET_VARIABLE_RESULT res = GET_VARIABLE_RESULT::GETVAR_NORMAL;
	variable* obj=Variables.findObjVar(getInstanceWorker(),name,((opt & FROM_GETLEX) || name.hasEmptyNS || name.hasBuiltinNS || name.ns.empty()) ? DECLARED_TRAIT|DYNAMIC_TRAIT : DECLARED_TRAIT,&nsRealId);
	if(obj)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(asAtomHandler::isValid(obj->getter) || asAtomHandler::isValid(obj->var)))
			obj=nullptr;
		else if (obj->min_swfversion &&  wrk->AVM1getSwfVersion() < obj->min_swfversion)
			obj = nullptr; // we are in AVM1 script execution and the property asked for is not available for the current swfversion
	}
	else if (opt & DONT_CHECK_CLASS)
		return res;

	if(!obj)
	{
		//Look for borrowed traits before
		if (cls)
		{
			obj= ASObject::findGettableImpl(getInstanceWorker(), cls->borrowedVariables,name,&nsRealId);
			if(!obj && ((opt & DONT_CHECK_CLASS) == 0) && name.hasEmptyNS)
			{
				//Check prototype chain
				Prototype* proto = cls->getPrototype(wrk);
				while(proto)
				{
					obj=proto->getObj()->Variables.findObjVar(getInstanceWorker(),name,DECLARED_TRAIT|DYNAMIC_TRAIT,&nsRealId);
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
		if (obj && obj->min_swfversion &&  wrk->AVM1getSwfVersion() < obj->min_swfversion)
			obj = nullptr; // we are in AVM1 script execution and the property asked for is not available for the current swfversion
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
		{
			if (getSystemState()->flashMode != SystemState::AIR) // it seems that Adobe AIR doesn't throw an exception when trying to access an instance property from the class
				createError<TypeError>(wrk, kCallOfNonFunctionError,name.normalizedNameUnresolved(getSystemState()));
			return res;
		}
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
		asAtom closure = asAtomHandler::getClosureAtom(obj->getter,asAtomHandler::fromObject(this));
		asAtomHandler::as<IFunction>(obj->getter)->callGetter(ret,closure,wrk);
		LOG_CALL("End of getter"<< ' ' << asAtomHandler::toDebugString(obj->getter)<<" result:"<<asAtomHandler::toDebugString(ret));
	}
	else
	{
		assert_and_throw(asAtomHandler::isInvalid(obj->setter));
		if(asAtomHandler::isFunction(obj->var) && asAtomHandler::as<IFunction>(obj->var)->isMethod())
		{
			asAtom closure = asAtomHandler::getClosureAtom(obj->var,asAtomHandler::invalidAtom);
			if (asAtomHandler::isValid(closure))
			{
				LOG_CALL("function " << name << " is already bound to "<<asAtomHandler::toDebugString(closure));
				if (asAtomHandler::as<IFunction>(obj->var)->clonedFrom)
					ASATOM_INCREF(obj->var);
				asAtomHandler::set(ret,obj->var);
			}
			else
			{
				LOG_CALL("Attaching this " << this->toDebugString() << " to function " << name << " "<<asAtomHandler::toDebugString(obj->var));
				asAtomHandler::setFunction(ret,asAtomHandler::getObjectNoCheck(obj->var),asAtomHandler::fromObject(this),wrk);
				// the function is always cloned
				res = (GET_VARIABLE_RESULT)(res | GET_VARIABLE_RESULT::GETVAR_ISNEWOBJECT);
			}
		}
		else
		{
			if (!(opt & NO_INCREF))
				ASATOM_INCREF(obj->var);
			asAtomHandler::set(ret,obj->var);
		}
	}
	return res;
}

void ASObject::getVariableByMultiname(asAtom& ret, const tiny_string& name, std::list<tiny_string> namespaces, ASWorker* wrk)
{
	multiname varName(nullptr);
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

	getVariableByMultinameIntern(ret,varName,this->getClass(),NONE,wrk);
}

void ASObject::executeASMethod(asAtom& ret,const tiny_string& methodName,
					std::list<tiny_string> namespaces,
					asAtom* args,
					uint32_t num_args)
{
	asAtom o=asAtomHandler::invalidAtom;
	getVariableByMultiname(o,methodName, namespaces,getInstanceWorker());
	if (!asAtomHandler::isFunction(o))
	{
		createError<TypeError>(getInstanceWorker(), kCallOfNonFunctionError, methodName);
		return;
	}
	asAtom v =asAtomHandler::fromObject(this);
	asAtomHandler::callFunction(o,getInstanceWorker(),ret,v,args,num_args,false);
	ASATOM_DECREF(o);
}

void ASObject::check() const
{
	//Put here a bunch of safety check on the object
#ifndef NDEBUG
	assert(getConstant() || (getRefCount()>0));
#endif
	//	Variables.check();
}
void ASObject::prepareShutdown()
{
	if (preparedforshutdown)
		return;
	preparedforshutdown=true;
	classdef=nullptr;
	objfreelist=nullptr;
	Variables.prepareShutdown();
	for (auto it = ownedObjects.begin(); it != ownedObjects.end(); it++)
		(*it)->prepareShutdown();
}

void ASObject::setRefConstant()
{
	getInstanceWorker()->registerConstantRef(this);
	setConstant();
}

std::pair<asAtom, uint8_t> ASObject::AVM1searchPrototypeByMultiname
(
	const multiname& name,
	bool isSlashPath,
	ASWorker* wrk
)
{
	ASObject* pr;
	if (is<Class_base>())
	{
		auto _class = as<Class_base>();
		auto proto = _class->getPrototype(wrk);
		pr = proto != nullptr ? proto->getObj() : getprop_prototype();
	}
	else
		pr = getprop_prototype();

	asAtom ret = asAtomHandler::invalidAtom;
	for (uint8_t depth = 0; pr != nullptr; pr = pr->getprop_prototype(), ++depth)
	{
		if (depth == UINT8_MAX)
		{
			throw ScriptLimitException
			(
				"Reached maximum prototype recursion limit",
				ScriptLimitException::MaxPrototypeRecursion
			);
		}

		bool isGetter = pr->AVM1getVariableByMultiname
		(
			ret,
			name,
			GET_VARIABLE_OPTION
			(
				GET_VARIABLE_OPTION::DONT_CALL_GETTER |
				GET_VARIABLE_OPTION::DONT_CHECK_PROTOTYPE
			),
			wrk,
			isSlashPath
		) & GET_VARIABLE_RESULT::GETVAR_ISGETTER;

		if (isGetter)
		{
			IFunction* f = asAtomHandler::as<IFunction>(ret);
			auto thisObj = asAtomHandler::fromObject(this);
			ret = asAtomHandler::invalidAtom;
			f->callGetter(ret, thisObj, wrk);
			return std::make_pair(ret, depth);
		}
		else if (asAtomHandler::isValid(ret))
			return std::make_pair(ret, depth);
	}

	return std::make_pair(callResolveMethod(name.normalizedName(wrk), wrk), 0);
}

GET_VARIABLE_RESULT ASObject::AVM1getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk, bool isSlashPath)
{
	GET_VARIABLE_RESULT res = getVariableByMultiname(ret,name,opt,wrk);
	if (asAtomHandler::isInvalid(ret) && !(opt & DONT_CHECK_PROTOTYPE))
	{
		auto pair = AVM1searchPrototypeByMultiname(name, isSlashPath, wrk);
		ret = pair.first;
	}
	return res;
}

bool ASObject::AVM1setLocalByMultiname(multiname& name, asAtom& value, CONST_ALLOWED_FLAG allowConst, ASWorker* wrk)
{
	bool alreadySet = false;
	setVariableByMultiname(name, value, allowConst, &alreadySet, wrk);
	return alreadySet;
}

bool ASObject::AVM1setVariableByMultiname(multiname& name, asAtom& value, CONST_ALLOWED_FLAG allowConst, ASWorker* wrk)
{
	auto s = name.normalizedName(wrk);

	if (s.empty())
		return false;

	// TODO: Support watchers.

	if (!hasPropertyByMultiname(name, true, false, wrk))
	{
		// We have to check the prototype chain for virtual setters,
		// before inserting a new property.
		for (auto pr = getprop_prototype(); pr != nullptr; pr = pr->getprop_prototype())
		{
			bool hasVirtual = false;
			auto var = findSettable(name, &hasVirtual);
			if (var == nullptr || !hasVirtual)
				continue;

			if (asAtomHandler::isInvalid(var->setter))
				continue;

			auto thisObj = asAtomHandler::fromObject(this);
			auto res = asAtomHandler::invalidAtom;
			asAtomHandler::callFunction
			(
				var->setter,
				wrk,
				res,
				thisObj,
				&value,
				1,
				false
			);
			return false;
		}
	}

	return AVM1setLocalByMultiname(name, value, allowConst, wrk);
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
				continue;
		}
		LOG(LOG_INFO, kind <<  '[' << it->second.ns << "] "<< hex<<it->first<<dec<<" "<<
			getSys()->getStringFromUniqueId(it->first) << ' ' <<
			asAtomHandler::toDebugString(it->second.var) << ' ' << asAtomHandler::toDebugString(it->second.setter) << ' ' << asAtomHandler::toDebugString(it->second.getter) << ' ' <<it->second.slotid << ' ');//<<dynamic_cast<const Class_base*>(it->second.type));
	}
}

variables_map::~variables_map()
{
	destroyContents();
}

void variables_map::destroyContents()
{
	while(!Variables.empty())
	{
		var_iterator it=Variables.begin();
		if (it->second.isrefcounted)
		{
			asAtom getter=it->second.getter;
			asAtom setter=it->second.setter;
			ASObject* o = asAtomHandler::isAccessible(it->second.var) ? asAtomHandler::getObject(it->second.var) :nullptr;
			Variables.erase(it);
			if (o)
				o->removeStoredMember();
			o = asAtomHandler::isAccessible(getter) ? asAtomHandler::getObject(getter) :nullptr;
			if (o)
				o->removeStoredMember();
			o = asAtomHandler::isAccessible(setter) ? asAtomHandler::getObject(setter) :nullptr;
			if (o)
				o->removeStoredMember();
		}
		else
			Variables.erase(it);
	}
	slots_vars.clear();
	dynamic_vars.clear();
	slotcount=0;
}
void variables_map::prepareShutdown()
{
	var_iterator it=Variables.begin();
	while(it!=Variables.end())
	{
		ASObject* v = it->second.isrefcounted && asAtomHandler::isAccessibleObject(it->second.var) ? asAtomHandler::getObject(it->second.var) : nullptr;
		if (v)
			v->prepareShutdown();
		v = asAtomHandler::getObject(it->second.getter);
		if (v)
			v->prepareShutdown();
		v = asAtomHandler::getObject(it->second.setter);
		if (v)
			v->prepareShutdown();
		it++;
	}
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

bool variables_map::countCylicMemberReferences(garbagecollectorstate& gcstate, ASObject* parent)
{
	if (gcstate.stopped)
		return false;
	gcstate.setAncestor(parent);
	bool ret = false;
	auto it=Variables.cbegin();
	while(it!=Variables.cend())
	{
		if (gcstate.stopped)
			return false;
		if (!asAtomHandler::isAccessible(it->second.var))
		{
			it++;
			continue;
		}
		ASObject* o = asAtomHandler::getObject(it->second.var);
		if (it->second.isrefcounted && o && !o->getInDestruction() && o->canHaveCyclicMemberReference() && !o->deletedingarbagecollection)
		{
			if (o==gcstate.startobj)
			{
				gcstate.incCount(o,false);
				if (gcstate.stopped)
					return false;
				parent->gccounter.hasmember=true;
				ret = true;
			}
			else if (o == parent)
			{
				if (o->getConstant())
				{
					gcstate.ignoreCount(o);
					if (gcstate.stopped)
						return false;
					ret = o->gccounter.hasmember;
				}
				else
				{
					gcstate.incCount(o,ret);
					if (gcstate.stopped)
						return false;
				}
			}
			else if (o != parent && ((uint32_t)o->getRefCount()==o->storedmembercount))
			{
				if(o->gccounter.ischecked)
					ret = o->gccounter.hasmember || ret;
				else if (o->countAllCylicMemberReferences(gcstate))
				{
					o->gccounter.hasmember=true;
					if (!o->gccounter.ignore && gcstate.isIgnored(parent))
					{
						o->gccounter.ignore=true;
						gcstate.stopped=true;
						return false;
					}
					ret = true;
				}
				else
				{
					ret = o->gccounter.hasmember || ret;
					if (gcstate.stopped)
						return false;
				}
			}
			else
			{
				ret = o->gccounter.hasmember || ret;
				if (gcstate.stopped)
					return false;
			}
		}
		it++;
	}
	return ret;
}

void variables_map::insertVar(variable* v,bool prepend)
{
	if (v->nameStringID==BUILTIN_STRINGS::STRING_PROTO || v->nameStringID==BUILTIN_STRINGS::PROTOTYPE)
		v->isenumerable=false;
	if (!v->isenumerable || v->kind != DYNAMIC_TRAIT)
		return;
	if (prepend)
		dynamic_vars.insert(dynamic_vars.begin(),v);
	else
		dynamic_vars.push_back(v);
}
void variables_map::removeVar(variable* v)
{
	if (!v->isenumerable)
		return;
	for (auto it = dynamic_vars.begin(); it != dynamic_vars.end(); it++)
	{
		if (*it == v)
		{
			// don't erase, as we might be inside a "hasnext" loop and have to keep the index valid
			*it=nullptr;
			break;
		}
	}
}

#ifndef NDEBUG
Mutex memcheckmutex;
std::set<ASObject*> memcheckset;
std::map<Class_base*,uint32_t> ASObject::objectcounter;
void ASObject::dumpObjectCounters(uint32_t threshhold)
{
	uint64_t c = 0;
	auto it = objectcounter.begin();
	while (it != objectcounter.end())
	{
		if (it->second > threshhold)
		{
			if (it->first->classID==UINT32_MAX)
				LOG(LOG_INFO,"counter:"<<it->first->toDebugString()<<":"<<it->second);
			else
				LOG(LOG_INFO,"counter:"<<it->first->toDebugString()<<":"<<it->second<<"-"<<getSys()->worker->freelist[it->first->classID].freelistsize<<"="<<(it->second-getSys()->worker->freelist[it->first->classID].freelistsize));
		}
		c += it->second;
		it++;
	}
	LOG(LOG_INFO,"countall:"<<c);
}
#endif
ASObject::ASObject(ASWorker* wrk, Class_base* c, SWFOBJECT_TYPE t, CLASS_SUBTYPE st):
	objfreelist(c ? c->getFreeList(wrk) : nullptr),
	classdef(c),proxyMultiName(nullptr),sys(c?c->sys:nullptr),worker(wrk),gcNext(nullptr),gcPrev(nullptr),
	stringId(UINT32_MAX),storedmembercount(0),type(t),subtype(st),traitsInitialized(false),constructIndicator(false),constructorCallComplete(false),preparedforshutdown(false),
	markedforgarbagecollection(false),deletedingarbagecollection(false),implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
	memcheckmutex.lock();
	memcheckset.insert(this);
	memcheckmutex.unlock();
	if (c)
	{
		uint32_t x = objectcounter[c];
		x++;
		objectcounter[c] = x;
	}
#endif
}
ASObject::ASObject(const ASObject& o):objfreelist(o.objfreelist),classdef(nullptr),proxyMultiName(nullptr),sys(o.classdef? o.classdef->sys : nullptr),worker(o.worker),gcNext(nullptr),gcPrev(nullptr),
	stringId(o.stringId),storedmembercount(o.storedmembercount),type(o.type),subtype(o.subtype),traitsInitialized(false),constructIndicator(false),constructorCallComplete(false),preparedforshutdown(false),
	markedforgarbagecollection(false),deletedingarbagecollection(false),implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
	memcheckmutex.lock();
	memcheckset.insert(this);
	memcheckmutex.unlock();
#endif
	assert(o.Variables.size()==0);
}

ASObject::ASObject(MemoryAccount* m):objfreelist(nullptr),classdef(nullptr),proxyMultiName(nullptr),sys(nullptr),worker(nullptr),gcNext(nullptr),gcPrev(nullptr),
	stringId(UINT32_MAX),storedmembercount(0),type(T_OBJECT),subtype(SUBTYPE_NOT_SET),traitsInitialized(false),constructIndicator(false),constructorCallComplete(false),preparedforshutdown(false),
	markedforgarbagecollection(false),deletedingarbagecollection(false),implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
	memcheckmutex.lock();
	memcheckset.insert(this);
	memcheckmutex.unlock();
#endif
}

ASObject::~ASObject()
{
#ifndef NDEBUG
	memcheckmutex.lock();
	memcheckset.erase(this);
	memcheckmutex.unlock();
#endif
}

void ASObject::setClass(Class_base* c)
{
	if (classdef == c)
		return;
	classdef=c;
	if(c)
		this->sys = c->sys;
}

bool ASObject::removefromGarbageCollection()
{
	if (!canHaveCyclicMemberReference())
		return true;
	if (getInstanceWorker() && getInstanceWorker()->isInGarbageCollection())
		return true;
	if (getInstanceWorker())
		getInstanceWorker()->removeObjectFromGarbageCollector(this);
	markedforgarbagecollection = false;
	deletedingarbagecollection = false;
	return false;
}

void ASObject::addToGarbageCollection()
{
	if (getInstanceWorker() && canHaveCyclicMemberReference())
	{
		getInstanceWorker()->addObjectToGarbageCollector(this);
		markedforgarbagecollection = true;
	}
}

void ASObject::addStoredMember()
{
	if (this->getConstant() || !this->canHaveCyclicMemberReference())
		return;
	assert(storedmembercount<uint32_t(this->getRefCount()));
	storedmembercount++;
	if (markedforgarbagecollection)
	{
		if (!removefromGarbageCollection())
			decRef();
	}
}

void ASObject::removeStoredMember()
{
	if (getConstant() || getCached() || this->getInDestruction() || deletedingarbagecollection)
		return;
	if(!this->canHaveCyclicMemberReference())
	{
		decRef();
		return;
	}
	assert(storedmembercount);
	assert(storedmembercount<=uint32_t(this->getRefCount()));
	storedmembercount--;
	if (storedmembercount
		&& ((uint32_t)this->getRefCount() == storedmembercount+1))
	{
		getInstanceWorker()->addObjectToGarbageCollector(this);
		this->markedforgarbagecollection=true;
		return;
	}
	decRef();
}

bool ASObject::handleGarbageCollection()
{
	if (getConstant() || getCached() || this->getInDestruction())
		return false;
	if (storedmembercount && this->canHaveCyclicMemberReference() && ((uint32_t)this->getRefCount() == storedmembercount+1))
	{
		garbagecollectorstate gcstate(this);
		this->countCylicMemberReferences(gcstate);
		markedforgarbagecollection=false;
		if (gcstate.stopped)
		{
			gcstate.reset();
			decRef();
			return false;
		}
		uint32_t c =0;
		for (auto it = gcstate.checkedobjects.begin(); it != gcstate.checkedobjects.end(); it++)
		{
			if ((*it) == this)
			{
				if (c != UINT32_MAX)
					c = (*it)->gccounter.count;
			}
			else if (((*it)->gccounter.ignore || (*it)->getConstant() || (*it)->gccounter.count!=(uint32_t)(*it)->getRefCount()) && (*it)->gccounter.hasmember)
			{
				c = UINT32_MAX;
				if (!(*it)->gccounter.ignore && (*it)->isMarkedForGarbageCollection() && !deletedingarbagecollection)
				{
					getInstanceWorker()->addObjectToGarbageCollector(this);
					gcstate.reset();
					return false;
				}
				break;
			}
		}
		assert(c == UINT32_MAX || c <= storedmembercount || this->preparedforshutdown);
		if (c == storedmembercount)
		{
			for (auto it = gcstate.checkedobjects.begin(); it != gcstate.checkedobjects.end(); it++)
			{
				if ((*it) != this && !(*it)->gccounter.ignore && (*it)->gccounter.count==(uint32_t)(*it)->getRefCount() && (*it)->gccounter.hasmember)
				{
					assert (!(*it)->getCached());
					(*it)->setConstant();// this ensures that the object is deleted _after_ all garbage collected objects are processed
				}
			}
			this->setConstant();// this ensures that the object is deleted _after_ all garbage collected objects are processed
			assert (!this->getCached());
			for (auto it = gcstate.checkedobjects.begin(); it != gcstate.checkedobjects.end(); it++)
			{
				if ((*it) != this && !(*it)->gccounter.ignore && (*it)->gccounter.count==(uint32_t)(*it)->getRefCount() && (*it)->gccounter.hasmember)
				{
					getInstanceWorker()->addObjectToGarbageCollector((*it));
					(*it)->deletedingarbagecollection=true;
					(*it)->markedforgarbagecollection=true;
					(*it)->destruct();
					(*it)->finalize();
				}
			}
			getInstanceWorker()->addObjectToGarbageCollector(this);
			this->destruct();
			this->finalize();
			this->deletedingarbagecollection=true;
			this->markedforgarbagecollection=true;
			gcstate.reset();
			return true;
		}
		gcstate.reset();
	}
	decRef();
	return false;
}

bool ASObject::countCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if(this->gccounter.ischecked)
		return this->gccounter.hasmember;
	bool ret = false;
	for (auto it = ownedObjects.begin(); it != ownedObjects.end(); it++)
	{
		if (!(*it)->gccounter.ischecked)
			ret = (*it)->countCylicMemberReferences(gcstate) || ret;
	}
	if (gcstate.stopped)
		return false;
	ret = Variables.countCylicMemberReferences(gcstate,this) || ret;
	return ret;
}

bool ASObject::countAllCylicMemberReferences(garbagecollectorstate& gcstate)
{
	if (gcstate.stopped)
		return false;
	bool ret = false;
	if (this == gcstate.startobj)
	{
		gcstate.incCount(this,false);
		ret = true;
	}
	else if (getConstant())
	{
		gcstate.ignoreCount(this);
		ret = this->gccounter.hasmember;
	}
	else if (!getInDestruction() && !getCached() && canHaveCyclicMemberReference() && !markedforgarbagecollection && !deletedingarbagecollection)
	{
		if (!this->gccounter.ischecked)
		{
			if (this->gccounter.isAncestor)
			{
				ret = gcstate.incCount(this,false);
				return this->gccounter.hasmember;
			}
			ret = countCylicMemberReferences(gcstate);
			gcstate.incCount(this,false);
			if (ret)
				this->gccounter.hasmember=true;
			else
				ret=this->gccounter.hasmember;
		}
		else if (!this->gccounter.isAncestor)
			ret = gcstate.incCount(this,false);
	}
	else
	{
		ret = this->gccounter.hasmember;
	}

	return ret;
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
		AVM1getVariableByMultiname(f,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
		if (asAtomHandler::is<AVM1Function>(f))
			asAtomHandler::as<AVM1Function>(f)->call(nullptr,nullptr,nullptr,0);
		ASATOM_DECREF(f);
	}
	if (e->type =="keyUp")
	{
		multiname m(nullptr);
		m.name_type = multiname::NAME_STRING;
		m.name_s_id = getSystemState()->getUniqueStringId("onKeyUp");
		asAtom f=asAtomHandler::invalidAtom;
		AVM1getVariableByMultiname(f,m,GET_VARIABLE_OPTION::NONE,getInstanceWorker());
		if (asAtomHandler::is<AVM1Function>(f))
			asAtomHandler::as<AVM1Function>(f)->call(nullptr,nullptr,nullptr,0);
		ASATOM_DECREF(f);
	}
	return false;
}

bool ASObject::AVM1HandleMouseEvent(EventDispatcher *dispatcher, MouseEvent *e)
{
	return AVM1HandleMouseEventStandard(dispatcher,e);
}
bool ASObject::AVM1HandleMouseEventStandard(ASObject *dispobj,MouseEvent *e)
{
	asAtom func=asAtomHandler::invalidAtom;
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.isAttribute = false;
	asAtom ret=asAtomHandler::invalidAtom;
	asAtom obj = asAtomHandler::fromObject(this);
	ASWorker* wrk = getInstanceWorker();
	if (e->type == "mouseMove")
	{
		if (!this->is<DisplayObject>() || this->is<MovieClip>())
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEMOVE;
			AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::is<AVM1Function>(func))
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			ASATOM_DECREF(func);
		}
	}
	else if (e->type == "mouseDown")
	{
		if (!this->is<DisplayObject>() || this->is<MovieClip>())
		{
			func=asAtomHandler::invalidAtom;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEDOWN;
			AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::is<AVM1Function>(func))
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			ASATOM_DECREF(func);
		}
	}
	else if (e->type == "mouseUp")
	{
		if (!this->is<DisplayObject>() || this->is<MovieClip>())
		{
			func=asAtomHandler::invalidAtom;
			m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEUP;
			AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::is<AVM1Function>(func))
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			ASATOM_DECREF(func);
		}
		if (dispobj && ((dispobj == this && !dispobj->is<DisplayObject>())
				|| (dispobj->as<DisplayObject>()->isVisible()
					&& (this->is<SimpleButton>()
						||(this->is<MovieClip>() && dispobj->as<DisplayObject>()->findParent(this->as<DisplayObject>())))
						)))
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONRELEASE;
			AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::is<AVM1Function>(func))
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			ASATOM_DECREF(func);
		}
	}
	else if (e->type == "mouseWheel")
	{
		m.name_s_id=BUILTIN_STRINGS::STRING_ONMOUSEWHEEL;
		AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::is<AVM1Function>(func))
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
		ASATOM_DECREF(func);
	}
	else if (e->type == "releaseOutside")
	{
		if (!this->is<DisplayObject>() || this->is<MovieClip>())
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONRELEASEOUTSIDE;
			AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::is<AVM1Function>(func))
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			ASATOM_DECREF(func);
		}
	}
	else if (e->type == "rollOver")
	{
		if (!this->is<DisplayObject>() || this->is<MovieClip>() || this->is<SimpleButton>())
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONROLLOVER;
			AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::is<AVM1Function>(func))
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			ASATOM_DECREF(func);
		}
	}
	else if (e->type == "rollOut")
	{
		if (dispobj == this && (!this->is<DisplayObject>() || this->is<MovieClip>() || this->is<SimpleButton>()))
		{
			m.name_s_id=BUILTIN_STRINGS::STRING_ONROLLOUT;
			AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::is<AVM1Function>(func))
				asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
			ASATOM_DECREF(func);
		}
	}
	else if (e->type == "mouseOver" || e->type == "mouseOut" || e->type == "click")
	{
		// not available in AVM1
	}
	else
		LOG(LOG_NOT_IMPLEMENTED,"handling avm1 mouse event "<<e->type);
	return false;
}
void ASObject::AVM1HandleSetFocusEvent(ASObject *dispobj)
{
	_NR<InteractiveObject> oldfocus = getSystemState()->stage->getFocusTarget();
	if (dispobj && oldfocus.getPtr()==dispobj)
		return;
	if ((dispobj && dispobj->is<TextField>())
		|| (oldfocus && oldfocus->is<TextField>()))
	{
		asAtom func=asAtomHandler::invalidAtom;
		multiname m(nullptr);
		m.name_type=multiname::NAME_STRING;
		m.isAttribute = false;
		asAtom ret=asAtomHandler::invalidAtom;
		asAtom obj = asAtomHandler::fromObject(this);
		ASWorker* wrk = getInstanceWorker();
		asAtom args[2];
		args[0] = !oldfocus || oldfocus==getSystemState()->stage || oldfocus->is<RootMovieClip>() ? asAtomHandler::nullAtom : asAtomHandler::fromObject(oldfocus.getPtr());
		args[1] = !dispobj || dispobj==getSystemState()->stage || dispobj->is<RootMovieClip>() || !dispobj->is<TextField>() ? asAtomHandler::nullAtom : asAtomHandler::fromObject(dispobj);
		m.name_s_id=BUILTIN_STRINGS::STRING_ONSETFOCUS;
		AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::is<AVM1Function>(func))
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,args,2);
		ASATOM_DECREF(func);
		ASATOM_DECREF(ret)
	}
}

void ASObject::AVM1HandlePressedEvent(ASObject *dispobj)
{
	multiname m(nullptr);
	m.name_type=multiname::NAME_STRING;
	m.isAttribute = false;
	asAtom ret=asAtomHandler::invalidAtom;
	asAtom obj = asAtomHandler::fromObject(this);
	ASWorker* wrk = getInstanceWorker();
	if (dispobj && ((dispobj == this && !dispobj->is<DisplayObject>())
					|| (dispobj->as<DisplayObject>()->isVisible() && (this->is<MovieClip>() || this->is<SimpleButton>()) && dispobj->as<DisplayObject>()->findParent(this->as<DisplayObject>()))))
	{
		m.name_s_id=BUILTIN_STRINGS::STRING_ONPRESS;
		asAtom func=asAtomHandler::invalidAtom;
		AVM1getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
		if (asAtomHandler::is<AVM1Function>(func))
			asAtomHandler::as<AVM1Function>(func)->call(&ret,&obj,nullptr,0);
		ASATOM_DECREF(func);
	}
}
void ASObject::AVM1UpdateAllBindings(DisplayObject* target, ASWorker* wrk)
{
	auto it = Variables.Variables.begin();
	while (it != Variables.Variables.end())
	{
		if (it->second.kind == DYNAMIC_TRAIT || it->second.kind == CONSTANT_TRAIT)
		{
			asAtom v=it->second.var;
			target->AVM1UpdateVariableBindings(it->first,v);
		}
		it++;
	}
}

void ASObject::copyValues(ASObject *target,ASWorker* wrk)
{
	bool needsactionscript3 = (target->is<DisplayObject>() && target->as<DisplayObject>()->needsActionScript3())
			|| (!target->is<DisplayObject>() && wrk->rootClip->needsActionScript3());
	auto it = Variables.Variables.begin();
	while (it != Variables.Variables.end())
	{
		if (it->second.kind == DYNAMIC_TRAIT || it->second.kind == CONSTANT_TRAIT)
		{
			multiname m(nullptr);
			m.name_type = multiname::NAME_STRING;
			m.name_s_id = it->first;
			asAtom v=it->second.var;
			if (wrk)
			{
				// prepare value for use in another worker
				if (needsactionscript3 && asAtomHandler::isFunction(v))
					v = asAtomHandler::fromObjectNoPrimitive(asAtomHandler::as<IFunction>(v)->createFunctionInstance(wrk));
				else if (asAtomHandler::isObject(v))
				{
					asAtomHandler::getObjectNoCheck(v)->incRef();
					asAtomHandler::getObjectNoCheck(v)->objfreelist=nullptr;
				}
			}
			else
			{
				ASATOM_INCREF(v);
			}

			if (needsactionscript3)
				target->setVariableByMultiname(m,v,CONST_ALLOWED,nullptr,wrk);
			else
				(void)target->AVM1setVariableByMultiname(m, v, CONST_ALLOWED, wrk);
		}
		it++;
	}
}

uint32_t variables_map::findInstanceSlotByMultiname(multiname* name,ASWorker* wrk)
{
	uint32_t nameId = name->normalizedNameId(wrk);
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

const variable* variables_map::getValueAt(unsigned int index)
{
	assert(index < dynamic_vars.size());
	return dynamic_vars[index];
}

void ASObject::getValueAt(asAtom &ret,int index)
{
	variable* obj=nullptr;
	if ((uint32_t)index >= Variables.dynamic_vars.size() && getClass()->prototype->getObj() != this)
		obj = getClass()->prototype->getObj()->Variables.dynamic_vars[index - Variables.dynamic_vars.size()];
	else
		obj = Variables.dynamic_vars[index];
	assert_and_throw(obj);

	if(asAtomHandler::isValid(obj->getter))
	{
		//Call the getter
		LOG_CALL("Calling the getter");
		asAtom v=asAtomHandler::fromObject(this);
		asAtom caller = obj->getter;
		asAtomHandler::callFunction(caller,getInstanceWorker(),ret, v,nullptr,0,false);
		LOG_CALL("End of getter at index "<<index<<":"<< asAtomHandler::toDebugString(obj->getter)<<" result:"<<asAtomHandler::toDebugString(ret));
	}
	else
	{
		ASATOM_INCREF(obj->var);
		ret = obj->var;
	}
}

uint32_t variables_map::getNameAt(unsigned int index,bool& nameIsInteger)
{
	assert(index < dynamic_vars.size());
	variable* v = dynamic_vars[index];
	nameIsInteger = v->nameIsInteger;
	return v->nameStringID;
}

unsigned int ASObject::numVariables() const
{
	return Variables.size();
}

uint32_t ASObject::getNameAt(int i, bool &nameIsInteger)
{
	if ((uint32_t)i >= Variables.dynamic_vars.size() && getClass()->prototype->getObj() != this)
		return getClass()->prototype->getObj()->getNameAt(i - Variables.dynamic_vars.size(),nameIsInteger);
	return Variables.getNameAt(i,nameIsInteger);
}

void ASObject::serializeDynamicProperties(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t> traitsMap, ASWorker* wrk, bool usedynamicPropertyWriter, bool forSharedObject)
{
	if (usedynamicPropertyWriter &&
			!out->getSystemState()->static_ObjectEncoding_dynamicPropertyWriter.isNull() &&
			!out->getSystemState()->static_ObjectEncoding_dynamicPropertyWriter->is<Null>())
	{
		DynamicPropertyOutput* o = Class<DynamicPropertyOutput>::getInstanceS(wrk);
		multiname m(nullptr);
		m.name_type = multiname::NAME_STRING;
		m.name_s_id = out->getSystemState()->getUniqueStringId("writeDynamicProperties");

		asAtom wr=asAtomHandler::invalidAtom;
		out->getSystemState()->static_ObjectEncoding_dynamicPropertyWriter->getVariableByMultiname(wr,m,SKIP_IMPL,wrk);
		if (!asAtomHandler::isFunction(wr))
		{
			o->decRef();
			ASATOM_DECREF(wr);
			createError<TypeError>(wrk,kCallOfNonFunctionError, m.normalizedNameUnresolved(out->getSystemState()));
		}

		asAtom ret=asAtomHandler::invalidAtom;
		asAtom v =asAtomHandler::fromObject(out->getSystemState()->static_ObjectEncoding_dynamicPropertyWriter.getPtr());
		ASATOM_INCREF(v);
		asAtom args[2];
		args[0] = asAtomHandler::fromObject(this);
		args[1] = asAtomHandler::fromObject(o);
		asAtomHandler::callFunction(wr,wrk,ret,v,args,2,false);
		o->serializeDynamicProperties(out, stringMap, objMap, traitsMap,wrk,false,false);
		o->decRef();
		ASATOM_DECREF(wr);
		ASATOM_DECREF(v);
	}
	else
	{
		Variables.serialize(out, stringMap, objMap, traitsMap,forSharedObject,wrk);
		//The empty string closes the object
		if(!getClass()->isSealed && out->getObjectEncoding() != OBJECT_ENCODING::AMF0 && !forSharedObject)
			out->writeStringVR(stringMap, "");
	}
}

void variables_map::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, bool forsharedobject, ASWorker* wrk)
{
	bool amf0 = out->getObjectEncoding() == OBJECT_ENCODING::AMF0;
	//Pairs of name, value
	auto it=Variables.begin();
	for(;it!=Variables.end();it++)
	{
		if(it->second.kind!=DYNAMIC_TRAIT)
			continue;
		if(!it->second.ns.hasEmptyName())
			continue;
		if (amf0)
			out->writeStringAMF0(out->getSystemState()->getStringFromUniqueId(it->first));
		else
			out->writeStringVR(stringMap,out->getSystemState()->getStringFromUniqueId(it->first));
		asAtomHandler::serialize(out,stringMap,objMap,traitsMap,wrk,it->second.var);
		if (forsharedobject)
		{
			// it seems that on shared objects an additional 0 is written after each property
			out->writeByte(0);
		}
	}
}

void ASObject::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk)
{
	bool amf0 = out->getObjectEncoding() == OBJECT_ENCODING::AMF0;
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
	tiny_string alias=wrk->rootClip->applicationDomain->findClassAlias(type);
	bool serializeTraits = alias.empty()==false;

	if(type->isSubClass(InterfaceClass<IExternalizable>::getClass(getSystemState())))
	{
		//Custom serialization necessary
		if(!serializeTraits)
		{
			createError<TypeError>(wrk,kInvalidParamError);
			return;
		}
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
		getVariableByMultiname(o,writeExternalName,SKIP_IMPL,wrk);
		assert_and_throw(asAtomHandler::isFunction(o));
		asAtom tmpArg[1] = { asAtomHandler::fromObject(out) };
		asAtom v=asAtomHandler::fromObject(this);
		asAtom r=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(o,wrk,r,v, tmpArg, 1,false);
		ASATOM_DECREF(o);
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
					asAtomHandler::serialize(out, stringMap, objMap, traitsMap,wrk,varIt->second.var);
				}
			}
		}
		if(!type->isSealed)
			serializeDynamicProperties(out, stringMap, objMap, traitsMap,wrk);
		out->writeShort(0);
		out->writeByte(amf0_object_end_marker);
		return;
	}

	if(it2!=traitsMap.end())
		out->writeU29((it2->second << 2) | 1);
	else
	{
		traitsMap.insert(make_pair(type, traitsMap.size()));
		// count serializable traits in variables
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
		// count serializable traits with getter _and_ setter in borrowed variables
		for(auto varIt=getClass()->borrowedVariables.Variables.begin(); varIt != getClass()->borrowedVariables.Variables.end(); ++varIt)
		{
			if(varIt->second.kind==DECLARED_TRAIT)
			{
				if(!varIt->second.ns.hasEmptyName())
				{
					//Skip variable with a namespace, like protected ones
					continue;
				}
				if(asAtomHandler::isInvalid(varIt->second.getter) || asAtomHandler::isInvalid(varIt->second.setter))
					continue;
				traitsCount++;
			}
		}
		uint32_t dynamicFlag=(type->isSealed)?0:(1 << 3);
		out->writeU29((traitsCount << 4) | dynamicFlag | 0x03);
		out->writeStringVR(stringMap, alias);
		// write borrowed trait names with getter _and_ setter
		for(auto varIt=getClass()->borrowedVariables.Variables.begin(); varIt != getClass()->borrowedVariables.Variables.end(); ++varIt)
		{
			if(varIt->second.kind==DECLARED_TRAIT)
			{
				if(!varIt->second.ns.hasEmptyName())
				{
					//Skip variable with a namespace, like protected ones
					continue;
				}
				if(asAtomHandler::isInvalid(varIt->second.getter) || asAtomHandler::isInvalid(varIt->second.setter))
					continue;

				out->writeStringVR(stringMap, getSystemState()->getStringFromUniqueId(varIt->first));
			}
		}
		// write trait names
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
	// serialize declared borrowed properties that have getters _and_ setters
	for(auto itbor=getClass()->borrowedVariables.Variables.begin();itbor!=getClass()->borrowedVariables.Variables.end();itbor++)
	{
		if(itbor->second.kind!=DECLARED_TRAIT)
			continue;
		if(!itbor->second.ns.hasEmptyName())
			continue;
		if(asAtomHandler::isInvalid(itbor->second.getter) || asAtomHandler::isInvalid(itbor->second.setter))
			continue;
		asAtom closure = asAtomHandler::getClosureAtom(itbor->second.getter,asAtomHandler::fromObject(this));
		asAtom ret = asAtomHandler::invalidAtom;
		asAtomHandler::as<IFunction>(itbor->second.getter)->callGetter(ret,closure,wrk);
		if (!asAtomHandler::isValid(ret))
			continue;
		asAtomHandler::serialize(out,stringMap,objMap,traitsMap,wrk,ret);
		ASATOM_DECREF(ret);
	}
	// serialize declared properties
	for(variables_map::var_iterator varIt=beginIt; varIt != endIt; ++varIt)
	{
		if(varIt->second.kind==DECLARED_TRAIT)
		{
			if(!varIt->second.ns.hasEmptyName())
			{
				//Skip variable with a namespace, like protected ones
				continue;
			}
			asAtomHandler::serialize(out, stringMap, objMap, traitsMap,wrk,varIt->second.var);
		}
	}
	serializeDynamicProperties(out, stringMap, objMap, traitsMap,wrk);
}

ASObject *ASObject::describeType(ASWorker* wrk) const
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

	//LOG(LOG_INFO,"describeType:"<< Class<XML>::getInstanceS(getInstanceWorker(),root)->toXMLString_internal());

	return XML::createFromNode(wrk,root);
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
				res = this->toString().toQuotedString();
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
				ASObject* v = nullptr;
				bool newobj = false;
				if (asAtomHandler::isValid(varIt->second.var))
				{
					asAtom tmp = varIt->second.var;
					newobj = !asAtomHandler::isObject(varIt->second.var); // variable is not a pointer to an ASObject, so toObject() will create a temporary ASObject that has to be decreffed after usage
					v = asAtomHandler::toObject(tmp,getInstanceWorker());
				}
				else if (asAtomHandler::isValid(varIt->second.getter))
				{
					asAtom t=asAtomHandler::fromObject(this);
					asAtom res=asAtomHandler::invalidAtom;
					asAtomHandler::callFunction(varIt->second.getter,getInstanceWorker(),res,t,NULL,0,false);
					newobj = !asAtomHandler::isObject(res); // result is not a pointer to an ASObject, so toObject() will create a temporary ASObject that has to be decreffed after usage
					asAtom tmp = res;
					v=asAtomHandler::toObject(tmp,getInstanceWorker());
				}
				if(v && v->getObjectType() != T_UNDEFINED && varIt->second.isenumerable)
				{
					// check for cylic reference
					if (v->getObjectType() != T_UNDEFINED &&
						v->getObjectType() != T_NULL &&
						v->getObjectType() != T_BOOLEAN &&
						std::find(path.begin(),path.end(), v) != path.end())
					{
						createError<TypeError>(getInstanceWorker(), kJSONCyclicStructure);
						return res;
					}
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
						asAtomHandler::callFunction(replacer,getInstanceWorker(),funcret,asAtomHandler::nullAtom, params, 2,true);
						if (asAtomHandler::isValid(funcret))
						{
							res += asAtomHandler::toString(funcret,getInstanceWorker());
							ASATOM_DECREF(funcret);
						}
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

				if (newobj)
					v->decRef();
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
	if (var != nullptr)
		return asAtomHandler::isValid(var->var);
	if (!sys->mainClip->needsActionScript3())
	{
		var = Variables.findObjVar
		(
			BUILTIN_STRINGS::STRING_PROTO,
			nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),
			NO_CREATE_TRAIT,
			(DECLARED_TRAIT|DYNAMIC_TRAIT)
		);
	}
	return (var && asAtomHandler::isValid(var->var));
}

ASObject* ASObject::getprop_prototype()
{
	variable* var=Variables.findObjVar(BUILTIN_STRINGS::PROTOTYPE,nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),
			NO_CREATE_TRAIT,(DECLARED_TRAIT|DYNAMIC_TRAIT));
	if (var)
		return asAtomHandler::toObject(var->var,getInstanceWorker());
	if (!getSystemState()->mainClip->needsActionScript3())
		var=Variables.findObjVar(BUILTIN_STRINGS::STRING_PROTO,nsNameAndKind(BUILTIN_NAMESPACES::EMPTY_NS),
			NO_CREATE_TRAIT,(DECLARED_TRAIT|DYNAMIC_TRAIT));
	if (var != nullptr && asAtomHandler::isObjectPtr(var->var))
		return asAtomHandler::getObjectNoCheck(var->var);
	return nullptr;
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
	{
		createError<ReferenceError>(getInstanceWorker(), kConstWriteError,
					   prototypeName.normalizedNameUnresolved(getSystemState()),
					   classdef ? classdef->as<Class_base>()->getQualifiedClassName() : "");
		return;
	}
	if(!ret)
	{
		ret = Variables.findObjVar(getInstanceWorker(),prototypeName,DYNAMIC_TRAIT,DECLARED_TRAIT|DYNAMIC_TRAIT);
	}
	ret->isenumerable=false;
	if(asAtomHandler::isValid(ret->setter))
	{
		asAtom arg1= asAtomHandler::fromObject(obj);
		asAtom v=asAtomHandler::fromObject(this);
		asAtom res=asAtomHandler::invalidAtom;
		asAtomHandler::callFunction(ret->setter,getInstanceWorker(),res,v,&arg1,1,false);
	}
	else
	{
		obj->incRef();
		asAtom v = asAtomHandler::fromObject(obj);
		ret->setVar(this->getInstanceWorker(),v);
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

bool asAtomHandler::stringcompare(asAtom& a, ASWorker* wrk, uint32_t stringID)
{
	return toObject(a,wrk)->toString() == wrk->getSystemState()->getStringFromUniqueId(stringID);
}
bool asAtomHandler::functioncompare(asAtom& a, ASWorker* wrk, asAtom& v2)
{
	if (getObject(a) && asAtomHandler::isValid(getObject(a)->as<IFunction>()->closure_this)
		&& getObject(v2) && asAtomHandler::isValid(getObject(v2)->as<IFunction>()->closure_this)
		&& getObject(a)->as<IFunction>()->closure_this.uintval != getObject(v2)->as<IFunction>()->closure_this.uintval)
		return false;
	return toObject(v2,wrk)->isEqual(toObject(a,wrk));
}
asAtom asAtomHandler::getClosureAtom(asAtom& a, asAtom defaultAtom)
{
	return (is<IFunction>(a)) && asAtomHandler::isValid(as<IFunction>(a)->closure_this) ? as<IFunction>(a)->closure_this : defaultAtom;
}
asAtom asAtomHandler::fromString(SystemState* sys, const tiny_string& s)
{
	asAtom a=asAtomHandler::invalidAtom;
	a.uintval = (sys->getUniqueStringId(s)<<3) | ATOM_STRINGID;
	return a;
}

void asAtomHandler::callFunction(asAtom& caller,ASWorker* wrk,asAtom& ret,asAtom &obj, asAtom *args, uint32_t num_args, bool args_refcounted, bool coerceresult, bool coercearguments, bool isAVM1InternalCall)
{
	if (USUALLY_FALSE(!asAtomHandler::isFunction(caller)))
	{
		createError<TypeError>(wrk,kCallOfNonFunctionError, asAtomHandler::toString(caller,wrk));
		if (args_refcounted)
		{
			for (uint32_t i = 0; i < num_args; i++)
				ASATOM_DECREF(args[i]);
		}
		ASATOM_DECREF(obj);
		return;
	}

	asAtom c = obj;
	if (getObjectNoCheck(caller)->is<SyntheticFunction>())
	{
		if (!args_refcounted)
		{
			for (uint32_t i = 0; i < num_args; i++)
				ASATOM_INCREF(args[i]);
			ASATOM_INCREF(obj); // ensure we have a reference to the calling object as it may be decreffed during the call
		}
		getObjectNoCheck(caller)->as<SyntheticFunction>()->call(wrk,ret,c, args, num_args,coerceresult,coercearguments);
		ASATOM_DECREF(c);
		return;
	}
	if (getObjectNoCheck(caller)->is<AVM1Function>())
	{
		// The caller is just the previous callee.
		auto argCaller = wrk->AVM1getCallee();
		getObjectNoCheck(caller)->as<AVM1Function>()->call(&ret,&c, args, num_args, argCaller,isAVM1InternalCall);
	}
	else
	{
		// when calling builtin functions, normally no refcounting is needed
		// if it is, it has to be done inside the called function
		getObjectNoCheck(caller)->as<Function>()->call(ret,wrk, c, args, num_args);
	}
	if (args_refcounted)
	{
		for (uint32_t i = 0; i < num_args; i++)
			ASATOM_DECREF(args[i]);
		ASATOM_DECREF(c);
	}
}

multiname* asAtomHandler::getVariableByMultiname(asAtom& a, asAtom& ret, const multiname &name, ASWorker* wrk, bool& canCache,GET_VARIABLE_OPTION opt)
{
	// classes for primitives are final and sealed, so we only have to check the class for the variable
	// no need to create ASObjects for the primitives
	multiname* simplegetter = nullptr;
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
		case ATOM_UINTEGER:
		case ATOM_U_INTEGERPTR:
		case ATOM_NUMBERPTR:
			simplegetter = Class<Number>::getClass(wrk->getSystemState())->getClassVariableByMultiname(ret,name,wrk,a);
			canCache = asAtomHandler::isValid(ret);
			break;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_BOOL_BIT:
					simplegetter = Class<Boolean>::getClass(wrk->getSystemState())->getClassVariableByMultiname(ret,name,wrk,a);
					canCache = asAtomHandler::isValid(ret);
					break;
				default:
					canCache = false;
					break;
			}
			break;
		}
		case ATOM_STRINGID:
			simplegetter =  Class<ASString>::getClass(wrk->getSystemState())->getClassVariableByMultiname(ret,name,wrk,a);
			canCache = asAtomHandler::isValid(ret);
			break;
		default:
		{
			GET_VARIABLE_RESULT varres = asAtomHandler::getObjectNoCheck(a)->getVariableByMultiname(ret,name, GET_VARIABLE_OPTION(opt|DONT_CALL_GETTER),wrk);
			canCache = varres & GET_VARIABLE_RESULT::GETVAR_CACHEABLE;
			if (varres & GET_VARIABLE_RESULT::GETVAR_ISGETTER)
			{
				//Call the getter
				LOG_CALL("Calling the getter for " << name << " on " << asAtomHandler::toDebugString(a));
				assert(asAtomHandler::isFunction(ret));
				IFunction* f = asAtomHandler::as<IFunction>(ret);
				asAtom closure = asAtomHandler::getClosureAtom(ret,a);
				ret = asAtom();
				simplegetter = f->callGetter(ret,closure,wrk);
				LOG_CALL("End of getter"<< ' ' << f->toDebugString()<<" result:"<<asAtomHandler::toDebugString(ret)<<" "<<(simplegetter ? " simplegetter":""));
			}
			break;
		}
	}
	return simplegetter;
}
void asAtomHandler::getVariableByInteger(asAtom& a, asAtom &ret, int index, ASWorker* wrk)
{
	if (asAtomHandler::is<Vector>(a))
	{
		asAtomHandler::as<Vector>(a)->getVariableByIntegerDirect(ret,index,wrk);
		ASATOM_INCREF(ret);
	}
	else if (asAtomHandler::isObject(a))
		asAtomHandler::getObjectNoCheck(a)->getVariableByInteger(ret,index,GET_VARIABLE_OPTION::NONE,wrk);

}
bool asAtomHandler::hasPropertyByMultiname(const asAtom& a, const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk)
{
	// we use a temporary atom here to avoid converting the source atom into an ASObject if it isn't an ASObject
	asAtom tmp = a;
	bool isobject = asAtomHandler::isObject(tmp);
	bool found=asAtomHandler::toObject(tmp,wrk)->hasPropertyByMultiname(name, considerDynamic, considerPrototype,wrk);
	if (!isobject)
		ASATOM_DECREF(tmp);
	return found;
}
Class_base *asAtomHandler::getClass(const asAtom& a,SystemState* sys,bool followclass)
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

void asAtomHandler::fillMultiname(asAtom& a, ASWorker* wrk, multiname &name)
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
			name.name_type=multiname::NAME_OBJECT;
			name.name_o=a;
			break;
		case ATOM_STRINGID:
			name.name_type = multiname::NAME_STRING;
			name.name_s_id = a.uintval>>3;
			break;
		case ATOM_STRINGPTR:
			name.name_type = multiname::NAME_STRING;
			name.name_s_id = asAtomHandler::toStringId(a,wrk);
			break;
		case ATOM_U_INTEGERPTR:
			if (asAtomHandler::isInteger(a))
			{
				name.name_type = multiname::NAME_INT;
				name.name_i = asAtomHandler::toInt(a);
			}
			else
			{
				name.name_type = multiname::NAME_UINT;
				name.name_ui = asAtomHandler::toUInt(a);
			}
			break;
		default:
			name.name_type=multiname::NAME_OBJECT;
			name.name_o=a;
			break;
	}
}

void asAtomHandler::replaceBool(asAtom& a, ASObject *obj)
{
	a.uintval = obj->as<Boolean>()->val ? 0x100 : 0;
}

std::string asAtomHandler::toDebugString(const asAtom a)
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
			sprintf(buf,"(%p/%d/%d/%d)",getObject(a),getObject(a)->getRefCount(),getObject(a)->storedmembercount,getObject(a)->getConstant());
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

asAtom asAtomHandler::asTypelate(asAtom& a, asAtom& b, ASWorker* wrk)
{
	LOG_CALL("asTypelate");

	if(!isObject(b) || !is<Class_base>(b))
	{
		LOG(LOG_ERROR,"trying to call asTypelate on non class object:"<<toDebugString(a));
		createError<TypeError>(wrk,kConvertNullToObjectError);
		return asAtomHandler::invalidAtom;
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
		LOG_CALL("Numeric type is " << ((real_ret)?"":"not ") << "subclass of " << c->class_name);
		ASATOM_DECREF(b);
		if(real_ret)
			return a;
		else
			return asAtomHandler::nullAtom;
	}

	Class_base* objc = toObject(a,wrk)->getClass();
	if(!objc)
	{
		ASATOM_DECREF(b);
		return asAtomHandler::nullAtom;
	}

	bool real_ret=objc->isSubClass(c);
	LOG_CALL("Type " << objc->class_name << " is " << ((real_ret)?" ":"not ")
			<< "subclass of " << c->class_name);
	ASATOM_DECREF(b);
	if(real_ret)
		return a;
	else
		return asAtomHandler::nullAtom;
}

bool asAtomHandler::isTypelate(asAtom& a,ASObject *type)
{
	LOG_CALL("isTypelate");
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
			createError<TypeError>(type->getInstanceWorker(),kIsTypeMustBeClassError);
			return false;
		case T_NULL:
			LOG(LOG_ERROR,"trying to call isTypelate on null:"<<toDebugString(a));
			createError<TypeError>(type->getInstanceWorker(),kConvertNullToObjectError);
			return false;
		case T_UNDEFINED:
			LOG(LOG_ERROR,"trying to call isTypelate on undefined:"<<toDebugString(a));
			createError<TypeError>(type->getInstanceWorker(),kConvertUndefinedToObjectError);
			return false;
		case T_CLASS:
			break;
		default:
			createError<TypeError>(type->getInstanceWorker(),kIsTypeMustBeClassError);
			return false;
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
		LOG_CALL("Numeric type is " << ((real_ret)?"":"not ") << "subclass of " << c->class_name);
		type->decRef();
		return real_ret;
	}

	objc = getClass(a,c->getSystemState(),false);
	if(!objc)
	{
		real_ret=getObjectType(a)==type->getObjectType();
		LOG_CALL("isTypelate on non classed atom " << real_ret<<" "<<asAtomHandler::toDebugString(a)<<" "<<type->toDebugString());
		type->decRef();
		return real_ret;
	}

	real_ret=objc->isSubClass(c);
	LOG_CALL("Type " << objc->class_name << " is " << ((real_ret)?"":"not ")
			<< "subclass of " << c->class_name);
	type->decRef();
	return real_ret;
}

bool asAtomHandler::isTypelate(asAtom& a,asAtom& t)
{
	LOG_CALL("isTypelate");
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
			createError<TypeError>(getWorker(),kIsTypeMustBeClassError);
			return false;
		case T_NULL:
			LOG(LOG_ERROR,"trying to call isTypelate on null:"<<toDebugString(a));
			createError<TypeError>(getWorker(),kConvertNullToObjectError);
			return false;
		case T_UNDEFINED:
			LOG(LOG_ERROR,"trying to call isTypelate on undefined:"<<toDebugString(a));
			createError<TypeError>(getWorker(),kConvertUndefinedToObjectError);
			return false;
		case T_CLASS:
			break;
		default:
			createError<TypeError>(getWorker(),kIsTypeMustBeClassError);
			return false;
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
		LOG_CALL("Numeric type is " << ((real_ret)?"":"not ") << "subclass of " << c->class_name);
		c->decRef();
		return real_ret;
	}

	objc = getClass(a,c->getSystemState(),false);
	if(!objc)
	{
		real_ret=getObjectType(a)==asAtomHandler::getObjectType(t);
		LOG_CALL("isTypelate on non classed atom/atom " << real_ret<<" "<<asAtomHandler::toDebugString(a)<<" "<<asAtomHandler::toDebugString(t));
		c->decRef();
		return real_ret;
	}

	real_ret=objc->isSubClass(c);
	LOG_CALL("Type " << objc->class_name << " is " << ((real_ret)?"":"not ")
			<< "subclass of " << c->class_name);
	c->decRef();
	return real_ret;
}

void asAtomHandler::getStringView(tiny_string& res, const asAtom& a, ASWorker* wrk)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					res = "null";
					return;
				case ATOMTYPE_UNDEFINED_BIT:
					res = wrk->getSystemState()->getSwfVersion() > 6 ? "undefined" : "";
					return;
				case ATOMTYPE_BOOL_BIT:
					res = a.uintval&0x80 ? "true" : "false";
					return;
				default:
					res = "";
					return;
			}
		}
		case ATOM_NUMBERPTR:
			res = Number::toString(toNumber(a));
			return;
		case ATOM_INTEGER:
			res = Integer::toString(a.intval>>3);
			return;
		case ATOM_UINTEGER:
			res = UInteger::toString(a.uintval>>3);
			return;
		case ATOM_STRINGID:
			if ((a.uintval>>3) == 0)
				res= "";
			else if ((a.uintval>>3) < BUILTIN_STRINGS_CHAR_MAX)
				res.setChar(a.uintval>>3);
			else
				res = wrk->getSystemState()->getStringFromUniqueId(a.uintval>>3);
			return;
		case ATOM_STRINGPTR:
		{
			assert(getObject(a));
			const tiny_string& s = getObject(a)->as<ASString>()->getData();
			res.setValue(s.raw_buf(),s.numBytes(),s.numChars(),s.isSinglebyte(),s.hasNullEntries(),s.isIntegerValue(),false);
			return;
		}
		default:
			assert(getObject(a));
			res = getObject(a)->toString();
			return;
	}
}

tiny_string asAtomHandler::toString(const asAtom& a, ASWorker* wrk, bool fromAVM1add2)
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
					return !fromAVM1add2 || wrk->getSystemState()->getSwfVersion() > 6 ? "undefined" : "";
				case ATOMTYPE_BOOL_BIT:
					// NOTE: In SWF 4, bool to string conversions return
					// 1, or 0, rather than true, or false.
					if (wrk->AVM1getSwfVersion() < 5)
						return a.uintval&0x80 ? "1" : "0";
					return a.uintval&0x80 ? "true" : "false";
				default:
					return "";
			}
		}
		case ATOM_NUMBERPTR:
		{
			if (wrk->needsActionScript3())
				return Number::toString(toNumber(a));
			else
				return as<Number>(a)->toString();
		}
		case ATOM_INTEGER:
			return Integer::toString(a.intval>>3);
		case ATOM_UINTEGER:
			return UInteger::toString(a.uintval>>3);
		case ATOM_STRINGID:
			if ((a.uintval>>3) == 0)
				return "";
			if ((a.uintval>>3) < BUILTIN_STRINGS_CHAR_MAX)
				return tiny_string::fromChar(a.uintval>>3);
			return wrk->getSystemState()->getStringFromUniqueId(a.uintval>>3);
		default:
			assert(getObject(a));
			return getObject(a)->toString();
	}
}

tiny_string asAtomHandler::toErrorString(const asAtom& a, ASWorker* wrk)
{
	tiny_string s = toString(a,wrk);
	if (asAtomHandler::isString(a))
	{
		tiny_string ret("\"");
		ret += s;
		ret += "\"";
		return ret;
	}
	else if (asAtomHandler::isObject(a))
	{
		s = asAtomHandler::getObjectNoCheck(a)->getClassName();
		s += "@00000000000"; //TODO check what's the meaning of this
		return s;
	}
	return s;
}

tiny_string asAtomHandler::toLocaleString(const asAtom& a, ASWorker* wrk)
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
			ASString* s = (ASString*)abstract_s(wrk,a.uintval>>3);
			tiny_string res = s->toLocaleString();
			delete s;
			return res;
		}
		default:
			assert(getObject(a));
			return getObject(a)->toLocaleString();
	}
}

uint32_t asAtomHandler::toStringId(asAtom& a, ASWorker* wrk)
{
	if (isStringID(a))
		return a.uintval>>3;
	if (isObject(a))
	{
		assert(getObjectNoCheck(a) && getObjectNoCheck(a)->getRefCount() >= 1);
		return getObjectNoCheck(a)->toStringId();
	}
	asAtom b=a;
	ASObject* o = toObject(b,wrk);
	uint32_t ret = o->toStringId();
	o->decRef();
	return ret;
}

bool asAtomHandler::AVM1toPrimitive(asAtom& ret, ASWorker* wrk, bool& isRefCounted, const TP_HINT& hint)
{
	assert(!wrk->needsActionScript3());

	auto swfVersion = wrk->AVM1getSwfVersion();
	isRefCounted = false;

	if (isPrimitive(ret))
		return true;

	assert(getObject(ret) != nullptr);

	auto obj = getObjectNoCheck(ret);
	bool called=false;

	// NOTE: `toString()` is never called, when using a number hint.
	if (hint != NUMBER_HINT && (swfVersion > 5 && obj->is<Date>()))
	{
		// NOTE: In SWF 6, and later, `Date` objects call `toString()`.
		obj->call_toString(ret);
		called=true;
	}
	// NOTE: `valueOf()` is never called on `DisplayObject`s, when using
	// a number hint.
	else if (hint != NUMBER_HINT || !obj->is<DisplayObject>())
	{
		// Most objects call `valueOf()`.
		obj->call_valueOf(ret);
		called=true;
	}

	if (isPrimitive(ret))
	{
		isRefCounted = true;
		return true;
	}
	if (called)
		ASATOM_DECREF(ret);

	// If the above conversion returns an object, then the conversion
	// failed, so fall back to the original object.
	ret = fromObject(obj);
	return false;
}

tiny_string asAtomHandler::AVM1toString(const asAtom& a, ASWorker* wrk)
{
	auto swfVersion = wrk->AVM1getSwfVersion();
	switch (a.uintval & 0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval & 0x70)
			{
				case ATOMTYPE_NULL_BIT:
					return "null";
				case ATOMTYPE_UNDEFINED_BIT:
					return swfVersion < 7 ? "" : "undefined";
				case ATOMTYPE_BOOL_BIT:
					// NOTE: In SWF 4, bool to string conversions return
					// 1, or 0, rather than true, or false.
					if (swfVersion < 5)
						return a.uintval & 0x80 ? "1" : "0";
					return a.uintval & 0x80 ? "true" : "false";
				default:
					return "";
			}
		}
		case ATOM_NUMBERPTR:
			return as<Number>(a)->toString();
		case ATOM_INTEGER:
			return Integer::toString(a.intval >> 3);
		case ATOM_UINTEGER:
			return UInteger::toString(a.uintval >> 3);
		case ATOM_STRINGID:
		{
			auto stringId = a.uintval >> 3;
			if (!stringId)
				return "";
			if (stringId < BUILTIN_STRINGS_CHAR_MAX)
				return tiny_string::fromChar(stringId);
			return wrk->getSystemState()->getStringFromUniqueId(stringId);
		}
		default:
		{
			assert(getObject(a) != nullptr);
			auto obj = getObject(a);

			if (isString(a))
				return obj->toString();
			else if (obj->is<DisplayObject>())
			{
				// Special case for `DisplayObject`s.
				return obj->as<DisplayObject>()->AVM1GetPath();
			}

			tiny_string ret = "[type Object]";
			asAtom atom = asAtomHandler::invalidAtom;
			obj->call_toString(atom);


			if (isString(atom))
				ret = toString(atom, wrk, true);
			else if (obj->is<IFunction>())
				ret = "[type Function]";

			ASATOM_DECREF(atom);
			return ret;
		}
	}
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

bool asAtomHandler::add(asAtom& a, asAtom &v2, ASWorker* wrk, bool forceint)
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
			setInt(a,wrk,int32_t(res));
		else if (res >= 0 && res < (1<<29))
			setUInt(a,wrk,res);
		else
			return replaceNumber(a,wrk,res);
	}
	else if((isInteger(a) || isUInteger(a) || isNumber(a)) &&
			(isNumber(v2) || isInteger(v2) || isUInteger(v2)))
	{
		double num1=toNumber(a);
		double num2=toNumber(v2);
		LOG_CALL("addN " << num1 << '+' << num2<<" "<<toDebugString(a)<<" "<<toDebugString(v2));
		if (forceint)
			setInt(a,wrk,num1+num2);
		else
			return replaceNumber(a,wrk,num1+num2);
	}
	else if(isString(a) || isString(v2))
	{
		tiny_string sa = toString(a,wrk);
		sa += toString(v2,wrk);
		LOG_CALL("add " << toDebugString(a) << '+' << toDebugString(v2));
		if (forceint)
			setInt(a,wrk,Integer::stringToASInteger(sa.raw_buf(),0));
		else
			a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_s(wrk,sa))|ATOM_STRINGPTR;
	}
	else
	{
		bool ret = true;
		bool isObject_a = asAtomHandler::isObject(a);
		bool isObject_v2 = asAtomHandler::isObject(v2);
		asAtom atmp = a;
		asAtom v2tmp = v2;
		ASObject* val1 = toObject(atmp,wrk);
		ASObject* val2 = toObject(v2tmp,wrk);
		if( (val1->is<XML>() || val1->is<XMLList>()) && (val2->is<XML>() || val2->is<XMLList>()) )
		{
			//Check if the objects are both XML or XMLLists
			Class_base* xmlClass=Class<XML>::getClass(val1->getSystemState());

			XMLList* newList=Class<XMLList>::getInstanceS(val1->getInstanceWorker(),true);
			if(val1->getClass()==xmlClass)
				newList->append(_MNR(static_cast<XML*>(val1)));
			else //if(val1->getClass()==xmlListClass)
				newList->append(_MNR(static_cast<XMLList*>(val1)));

			val2->incRef();
			if(val2->getClass()==xmlClass)
				newList->append(_MNR(static_cast<XML*>(val2)));
			else //if(val2->getClass()==xmlListClass)
				newList->append(_MNR(static_cast<XMLList*>(val2)));

			if (forceint)
			{
				setInt(a,wrk,newList->toInt());
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
			bool isrefcounted1;
			val1->toPrimitive(val1p,isrefcounted1);
			asAtom val2p=asAtomHandler::invalidAtom;
			bool isrefcounted2;
			val2->toPrimitive(val2p,isrefcounted2);
			if(is<ASString>(val1p) || is<ASString>(val2p))
			{//If one is String, convert both to strings and concat
				string as(toString(val1p,wrk).raw_buf());
				string bs(toString(val2p,wrk).raw_buf());
				if (isrefcounted1)
					ASATOM_DECREF(val1p);
				if (isrefcounted2)
					ASATOM_DECREF(val2p);
				LOG_CALL("add " << as << '+' << bs);
				if (forceint)
				{
					tiny_string s = as+bs;
					setInt(a,wrk,Integer::stringToASInteger(s.raw_buf(),0));
				}
				else
					a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_s(wrk,as+bs))|ATOM_STRINGPTR;
			}
			else
			{//Convert both to numbers and add
				number_t num1=toNumber(val1p);
				number_t num2=toNumber(val2p);
				if (isrefcounted1)
					ASATOM_DECREF(val1p);
				if (isrefcounted2)
					ASATOM_DECREF(val2p);
				LOG_CALL("addN " << num1 << '+' << num2);
				number_t result = num1 + num2;
				if (forceint)
					setInt(a,wrk,result);
				else
					ret = replaceNumber(a,wrk,result);
			}
		}
		// cleanup new objects created in toObject() calls
		if (!isObject_a)
			ASATOM_DECREF(atmp);
		if (!isObject_v2)
			ASATOM_DECREF(v2tmp);
		return ret;
	}
	return true;
}
void asAtomHandler::addreplace(asAtom& ret, ASWorker* wrk, asAtom& v1, asAtom &v2, bool forceint)
{
	//Implement ECMA add algorithm, for XML and default (see avm2overview)

	// if both values are Integers or UIntegers the result is also an int Number
	if( (isInteger(v1) || isUInteger(v1)) &&
		(isInteger(v2) || isUInteger(v2)))
	{
		int64_t num1=toInt64(v1);
		int64_t num2=toInt64(v2);
		int64_t res = num1+num2;
		ASATOM_DECREF(ret);
		LOG_CALL("addI replace " << num1 << '+' << num2 <<"="<<res);
		if (forceint || (res >= -(1<<28) && res < (1<<28)))
			setInt(ret,wrk,int32_t(res));
		else if (res >= 0 && res < (1<<29))
			setUInt(ret,wrk,res);
		else
			setNumber(ret,wrk,res);
	}
	else if((isInteger(v1) || isUInteger(v1) || isNumber(v1)) &&
			(isNumber(v2) || isInteger(v2) || isUInteger(v2)))
	{
		double num1=toNumber(v1);
		double num2=toNumber(v2);
		LOG_CALL("addN replace " << num1 << '+' << num2<<" "<<toDebugString(v1)<<" "<<toDebugString(v2)<<" "<<forceint);
		ASObject* o = getObject(ret);
		if (forceint)
		{
			setInt(ret,wrk,num1+num2);
			if (o)
				o->decRef();
		}
		else if (replaceNumber(ret,wrk,num1+num2) && o)
			o->decRef();
	}
	else if(isString(v1) || isString(v2))
	{
		tiny_string sa = toString(v1,wrk);
		sa += toString(v2,wrk);
		LOG_CALL("add replace " << toString(v1,wrk) << '+' << toString(v2,wrk));
		ASATOM_DECREF(ret);
		if (forceint)
			setInt(ret,wrk,Integer::stringToASInteger(sa.raw_buf(),0));
		else
			ret.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_s(wrk,sa))|ATOM_STRINGPTR;
	}
	else
	{
		asAtom v1tmp = v1;
		asAtom v2tmp = v2;
		ASObject* val1 = toObject(v1tmp,wrk);
		ASObject* val2 = toObject(v2tmp,wrk);
		if( (val1->is<XML>() || val1->is<XMLList>()) && (val2->is<XML>() || val2->is<XMLList>()) )
		{
			//Check if the objects are both XML or XMLLists
			Class_base* xmlClass=Class<XML>::getClass(val1->getSystemState());

			XMLList* newList=Class<XMLList>::getInstanceS(val1->getInstanceWorker(),true);
			val1->incRef();
			if(val1->getClass()==xmlClass)
				newList->append(_MNR(static_cast<XML*>(val1)));
			else //if(val1->getClass()==xmlListClass)
				newList->append(_MNR(static_cast<XMLList*>(val1)));

			val2->incRef();
			if(val2->getClass()==xmlClass)
				newList->append(_MNR(static_cast<XML*>(val2)));
			else //if(val2->getClass()==xmlListClass)
				newList->append(_MNR(static_cast<XMLList*>(val2)));

			if (forceint)
			{
				setInt(ret,wrk,newList->toInt());
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
			bool isrefcounted1;
			val1->toPrimitive(val1p,isrefcounted1);
			bool isrefcounted2;
			asAtom val2p=asAtomHandler::invalidAtom;
			val2->toPrimitive(val2p,isrefcounted2);
			if(is<ASString>(val1p) || is<ASString>(val2p))
			{//If one is String, convert both to strings and concat
				string as(toString(val1p,wrk).raw_buf());
				string bs(toString(val2p,wrk).raw_buf());
				if (isrefcounted1)
					ASATOM_DECREF(val1p);
				if (isrefcounted2)
					ASATOM_DECREF(val2p);
				LOG_CALL("add " << as << '+' << bs);
				ASATOM_DECREF(ret);
				if (forceint)
				{
					tiny_string s = as+bs;
					setInt(ret,wrk,Integer::stringToASInteger(s.raw_buf(),0));
				}
				else
					ret.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_s(wrk,as+bs))|ATOM_STRINGPTR;
			}
			else
			{//Convert both to numbers and add
				number_t num1=toNumber(val1p);
				number_t num2=toNumber(val2p);
				if (isrefcounted1)
					ASATOM_DECREF(val1p);
				if (isrefcounted2)
					ASATOM_DECREF(val2p);
				LOG_CALL("addN replace primitive " << num1 << '+' << num2);
				ASObject* o = getObject(ret);
				if (forceint)
				{
					setInt(ret,wrk,num1+num2);
					if (o)
						o->decRef();
				}
				else if (replaceNumber(ret,wrk,num1+num2) && o)
					o->decRef();
			}
		}
		// cleanup new objects created in toObject() calls
		if (!asAtomHandler::isObject(v1))
			ASATOM_DECREF(v1tmp);
		if (!asAtomHandler::isObject(v2))
			ASATOM_DECREF(v2tmp);
	}
}

bool asAtomHandler::AVM1add(asAtom& a, asAtom &v2, ASWorker* wrk, bool forceint)
{
	//Implement AVM1 add algorithm for ActionAdd2 opcode
	bool ret = true;
	bool isRefCounted1 = false;
	bool isRefCounted2 = false;
	asAtom val1 = a;
	asAtom val2 = v2;

	// NOTE: These calls to `toPrimitve()` have to be done in reverse
	// order because they're supposed to be called in the order they
	// were popped.
	//
	// TODO: Move these `toPrimitive()` calls into the AVM1 interpreter.
	AVM1toPrimitive(val2, wrk, isRefCounted2, NO_HINT);
	AVM1toPrimitive(val1, wrk, isRefCounted1, NO_HINT);

	if(isString(val1) || isString(val2))
	{
		auto str1 = AVM1toString(val1, wrk);
		auto str2 = AVM1toString(val2, wrk);
		auto str = str1 + str2;
		LOG_CALL("add " << toDebugString(val1) << '+' << toDebugString(val2));
		if (forceint)
			setInt(a, wrk, Integer::stringToASInteger(str.raw_buf(), 0));
		else
			replace(a, abstract_s(wrk, str));
	}
	else
	{
		double num1 = AVM1toNumber(val1, wrk->AVM1getSwfVersion());
		double num2 = AVM1toNumber(val2, wrk->AVM1getSwfVersion());
		auto val = num1 + num2;
		bool isInt = (val - std::floor(val)) == 0;
		LOG_CALL("addN " << num1 << '+' << num2<<" "<<toDebugString(val1)<<" "<<toDebugString(val2));


		// NOTE: Returning an integer here is an optimization.
		if (forceint || (isInt && val >= -(1 << 28) && val < (1 << 28)))
			setInt(a, wrk, val);
		else if (isInt && val >= 0 && val < (1 << 29))
			setUInt(a, wrk, val);
		else
			ret = replaceNumber(a, wrk, val);
	}

	if (isRefCounted1)
		ASATOM_DECREF(val1);
	if (isRefCounted2)
		ASATOM_DECREF(val2);

	return ret;
}

void asAtomHandler::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap, std::map<const ASObject*, uint32_t>& objMap, std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk, asAtom& a)
{
	switch (a.uintval&0x7)
	{
		case ATOM_INTEGER:
			Integer::serializeValue(out,asAtomHandler::getInt(a));
			break;
		case ATOM_UINTEGER:
			UInteger::serializeValue(out,asAtomHandler::getUInt(a));
			break;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			switch (a.uintval&0xf0)
			{
				case ATOMTYPE_UNDEFINED_BIT: // UNDEFINED
					if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
						out->writeByte(amf0_undefined_marker);
					else
						out->writeByte(undefined_marker);
					break;
				case ATOMTYPE_NULL_BIT:
					if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
						out->writeByte(amf0_null_marker);
					else
						out->writeByte(null_marker);
					break;
				default: // BOOL
					if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
					{
						out->writeByte(amf0_boolean_marker);
						out->writeByte(a.uintval == asAtomHandler::trueAtom.uintval ? 1:0);
					}
					else
						out->writeByte(a.uintval == asAtomHandler::trueAtom.uintval ? true_marker : false_marker);
					break;
			}
			break;
		case ATOM_STRINGID:
			if (out->getObjectEncoding() == OBJECT_ENCODING::AMF0)
			{
				out->writeByte(amf0_string_marker);
				out->writeStringAMF0(out->getSystemState()->getStringFromUniqueId(asAtomHandler::getStringId(a)));
			}
			else
			{
				out->writeByte(string_marker);
				out->writeStringVR(stringMap, out->getSystemState()->getStringFromUniqueId(asAtomHandler::getStringId(a)));
			}
			break;
		default:
			asAtomHandler::getObjectNoCheck(a)->serialize(out, stringMap, objMap, traitsMap, wrk);
			break;
	}
}
void asAtomHandler::setFunction(asAtom& a, ASObject *obj, const asAtom& closure, ASWorker* wrk)
{
	// type may be T_CLASS or T_FUNCTION
	if (asAtomHandler::isValid(closure) && obj->is<IFunction>())
	{
		a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(obj->as<IFunction>()->bind(closure,wrk))|ATOM_OBJECTPTR;
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

void asAtomHandler::setNumber(asAtom& a, ASWorker* w, number_t val)
{
	if (std::isnan(val))
		a.uintval = w->getSystemState()->nanAtom.uintval;
	else
		a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_d(w,val))|ATOM_NUMBERPTR;
}
bool asAtomHandler::replaceNumber(asAtom& a, ASWorker* w, number_t val)
{
	if (isNumber(a) && getObject(a)->isLastRef())
	{
		as<Number>(a)->setNumber(val);
		return false;
	}
	if (std::isnan(val))
		a.uintval = w->getSystemState()->nanAtom.uintval;
	else
		a.uintval = (LIGHTSPARK_ATOM_VALTYPE)(abstract_d(w,val))|ATOM_NUMBERPTR;
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

TRISTATE asAtomHandler::isLessIntern(asAtom& a, ASWorker* w, asAtom &v2)
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
				case ATOM_STRINGID:
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
					return ((a.intval>>3) < getObjectNoCheck(v2)->toNumberForComparison())?TTRUE:TFALSE;
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
				case ATOM_STRINGID:
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
					return ((a.uintval>>3) < getObjectNoCheck(v2)->toNumberForComparison())?TTRUE:TFALSE;
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
							return toObject(a,w)->isLess(getObject(v2));
					}
					break;
				}
				case ATOMTYPE_UNDEFINED_BIT:
					return TUNDEFINED;
				case ATOMTYPE_BOOL_BIT:
				{
					switch (v2.uintval&0x7)
					{
						case ATOM_STRINGID:
							return (toInt(a) < toNumber(v2))?TTRUE:TFALSE;
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
							return toObject(a,w)->isLess(getObject(v2));
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
					return toString(a,w) < toString(v2,w) ? TTRUE : TFALSE;
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
				case ATOM_STRINGPTR:
					return toString(a,w) < toString(v2,w) ? TTRUE : TFALSE;
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
							return isEqual(a,w,v2) ? TFALSE : TTRUE;
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
					return toString(a,w) < toString(v2,w) ? TTRUE : TFALSE;
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
		case ATOM_OBJECTPTR:
		{
			switch (v2.uintval&0x7)
			{
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
				case ATOM_STRINGID:
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
				{
					TRISTATE tmp = asAtomHandler::isLessIntern(v2,w,a);
					switch (tmp)
					{
						case TFALSE:
							return TTRUE;
						case TTRUE:
							return TFALSE;
						default:
							return tmp;
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

bool asAtomHandler::isEqualIntern(asAtom& a, ASWorker* w, asAtom &v2)
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
					return (a.intval>>3)==getObjectNoCheck(v2)->toNumberForComparison();
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
					return (a.uintval>>3)==getObjectNoCheck(v2)->toNumberForComparison();
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
					return toObject(v2,w)->isEqual(toObject(a,w));
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
					return getObjectNoCheck(a)->toInt64()==getObjectNoCheck(v2)->toNumberForComparison();
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
							return toObject(v2,w)->isEqual(toObject(a,w));
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
							return toObject(v2,w)->isEqual(toObject(a,w));
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
					return isEqual(v2,w,a);
				default:
					return isEqual(v2,w,a);
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
					return toString(a,w) == toString(v2,w);
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
					return isEqual(v2,w,a);
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
					return functioncompare(a,w,v2);
				else
					return false;
			}
			switch (v2.uintval&0x7)
			{
				case ATOM_INVALID_UNDEFINED_NULL_BOOL:
					return getObject(a)->isEqual(toObject(v2,w));
				case ATOM_STRINGID:
				{
					asAtom primitive=asAtomHandler::invalidAtom;
					bool res = false;
					bool isrefcounted;
					if (getObject(a)->toPrimitive(primitive,isrefcounted))
						res = toString(primitive,w) == toString(v2,w);
					if (isrefcounted)
						ASATOM_DECREF(primitive);
					return res;
				}
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
					return isEqual(v2,w,a);
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

bool asAtomHandler::AVM1isEqualStrict(asAtom& a, asAtom& b, ASWorker* wrk)
{
	if (!isSameType(a, b))
		return false;

	switch (getType(a))
	{
		case ATOMTYPE_BOOL_BIT:
			return (a.uintval & 0x80) == (b.uintval & 0x80);
			break;
		case ATOM_INTEGER:
		case ATOM_UINTEGER:
		case ATOM_NUMBERPTR:
		{
			auto swfVersion = wrk->AVM1getSwfVersion();
			auto num1 = AVM1toNumber(a, swfVersion, true);
			auto num2 = AVM1toNumber(b, swfVersion, true);
			// NOTE, PLAYER-SPECIFIC: In Flash Player 7, and later,
			// `NaN == NaN` returns true, while in Flash Player 6, and
			// earlier, `NaN == NaN` returns false. Let's do what Flash
			// Player 7, and later does, and return true.
			//
			// TODO: Allow for using either, once support for
			// mimicking different player versions is added.
			return
			(
				num1 == num2 ||
				(std::isnan(num1) && std::isnan(num2))
			);
			break;
		}
		case ATOM_STRINGID:
		case ATOM_STRINGPTR:
			return AVM1toString(a, wrk) == AVM1toString(b, wrk);
			break;
		case ATOM_OBJECTPTR:
		{
			if (is<DisplayObject>(a) && is<DisplayObject>(b))
			{
				// Special case for `DisplayObject`.
				auto clip1 = as<DisplayObject>(a);
				auto clip2 = as<DisplayObject>(b);
				return clip1->AVM1GetPath() == clip2->AVM1GetPath();
			}
			else if (is<XMLNode>(a) && is<XMLNode>(b))
			{
				// Special case for `XMLNode`.
				// TODO: This is a hack, and should be removed once a
				// proper refactor of the `flash.xml` code is done.
				return getObject(a)->isEqual(getObject(b));
			}
			else if (is<IFunction>(a) && is<IFunction>(b))
				return as<IFunction>(a)->isEqual(as<IFunction>(b));

			return getObject(a) == getObject(b);
			break;
		}
		// `undefined`, `null`, and invalid.
		default:
			return true;
			break;
	}
}

// Implements ECMA-262 2nd edition, section 11.9.3. The abstract equality
// comparison algorithm.
bool asAtomHandler::AVM1isEqual(asAtom& v1, asAtom &v2, ASWorker* wrk)
{
	auto swfVersion = wrk->AVM1getSwfVersion();

	bool isRefCounted1 = false;
	bool isRefCounted2 = false;
	asAtom a = v2;
	asAtom b = v1;

	if (swfVersion < 6)
	{
		// NOTE: In SWF 5, `valueOf()` is always called, even in `Object`
		// to `Object` comparisons. Since `Object.prototype.valueOf()`
		// returns `this`, it'll do a pointer comparison. For `Object`
		// to atom/value comparisons, `valueOf` will be called a second
		// time.
		AVM1toPrimitive(a, wrk, isRefCounted1, NUMBER_HINT);
		AVM1toPrimitive(b, wrk, isRefCounted2, NUMBER_HINT);
	}

	bool ret = false;

	if (isSameType(a, b))
		ret = AVM1isEqualStrict(a, b, wrk);
	else if ((isNullOrUndefined(a) || isInvalid(a)) && (isNullOrUndefined(b) || isInvalid(b)))
		ret = true;
	// `bool` to atom/value comparision. Convert `bool` to 0/1, and
	// compare.
	else if (isBool(a) || isBool(b))
	{
		bool flag = (isBool(a) ? a : b).uintval & 0x80;
		auto flagVal = fromNumber(wrk, flag, false);
		auto val = isBool(a) ? b : a;
		ret = AVM1isEqual(flagVal, val, wrk);
		ASATOM_DECREF(flagVal);
	}
	// `Number` to atom/value comparison. Convert value to `Number`, and
	// compare.
	else if ((isNumeric(a) || isString(a)) && (isNumeric(b) || isString(b)))
	{
		auto num1 = AVM1toNumber(isNumeric(a) ? a : b, swfVersion, true);
		auto num2 = AVM1toNumber(isString(a) ? a : b, swfVersion, true);
		ret = num1 == num2;
	}
	// `Object` to atom/value comparison. Call `object.valueOf()`, and
	// compare.
	else if (isObjectPtr(a) || isObjectPtr(b))
	{
		bool isRefCounted = false;
		auto objVal = isObjectPtr(a) ? a : b;
		auto val = isObjectPtr(a) ? b : a;
		AVM1toPrimitive(objVal, wrk, isRefCounted, NUMBER_HINT);
		ret = isPrimitive(objVal) && AVM1isEqual(val, objVal, wrk);
		if (isRefCounted)
			ASATOM_DECREF(objVal);
	}

	if (isRefCounted1)
		ASATOM_DECREF(a);
	if (isRefCounted2)
		ASATOM_DECREF(b);
	return ret;
}

ASObject *asAtomHandler::toObject(asAtom& a, ASWorker* wrk, bool isconstant)
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
			a.uintval = ((LIGHTSPARK_ATOM_VALTYPE)abstract_di(wrk,(a.intval>>3)))|ATOM_NUMBERPTR;
			break;
		case ATOM_UINTEGER:
			// uints are internally treated as numbers, so create a Number instance
			a.uintval = ((LIGHTSPARK_ATOM_VALTYPE)abstract_di(wrk,(a.uintval>>3)))|ATOM_NUMBERPTR;
			break;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					return abstract_null(wrk->getSystemState());
				case ATOMTYPE_UNDEFINED_BIT:
					return abstract_undefined(wrk->getSystemState());
				case ATOMTYPE_BOOL_BIT:
					return abstract_b(wrk->getSystemState(),(a.uintval & 0x80)>>7);
				default:
					break;
			}
			break;
		}
		case ATOM_STRINGID:
			a.uintval = ((LIGHTSPARK_ATOM_VALTYPE)abstract_s(wrk,(a.uintval>>3))) | ATOM_STRINGPTR ;
			break;
		default:
			throw RunTimeException("calling toObject on invalid asAtom, should not happen");
			break;
	}
	return getObjectNoCheck(a);
}

bool garbagecollectorstate::incCount(ASObject* o, bool hasMember)
{
	o->gccounter.count++;
	o->gccounter.hasmember=o->gccounter.hasmember||hasMember;
	if (!o->gccounter.ischecked)
	{
		this->checkedobjects.push_back(o);
		o->gccounter.ischecked=true;
	}
	if (o->gccounter.hasmember && o->gccounter.ignore)
		this->stopped=true;
	assert((int)o->gccounter.count <= o->getRefCount());
	return o->gccounter.hasmember;
}

void garbagecollectorstate::ignoreCount(ASObject* o)
{
	o->gccounter.ignore=true;
	if (!o->gccounter.ischecked)
	{
		this->checkedobjects.push_back(o);
		o->gccounter.ischecked=true;
	}
	if (o->gccounter.hasmember)
		this->stopped=true;
}
void garbagecollectorstate::setAncestor(ASObject* o)
{
	o->gccounter.isAncestor=true;
	if (!o->gccounter.ischecked)
	{
		this->checkedobjects.push_back(o);
		o->gccounter.ischecked=true;
	}
}
void garbagecollectorstate::reset()
{
	startobj->gccounter.reset();
	for (auto it = checkedobjects.begin(); it != checkedobjects.end(); it++)
		(*it)->gccounter.reset();
}
bool garbagecollectorstate::isIgnored(ASObject* o)
{
	return o->gccounter.ischecked && o->gccounter.ignore;
}
bool garbagecollectorstate::hasMember(ASObject* o)
{
	return o->gccounter.ischecked && o->gccounter.hasmember;
}
