/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "asobject.h"
#include "scripting/class.h"
#include <algorithm>
#include <limits>
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "scripting/toplevel/Date.h"

using namespace lightspark;
using namespace std;

REGISTER_CLASS_NAME2(ASObject,"Object","");

tiny_string ASObject::toString(bool debugMsg)
{
	check();
	//TODO: move this debugMsg thing to its own function (maybe toString_debug)
	if(debugMsg) {
		tiny_string ret;
		if(getClass())
		{
			ret="[object ";
			ret+=getClass()->class_name.name;
			ret+="]";
			return ret;
		}
		else
			ret="[object Object]";

		return ret;
	}

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

TRISTATE ASObject::isLess(ASObject* r)
{
	check();
	return toPrimitive()->isLess(r);
}

uint32_t ASObject::nextNameIndex(uint32_t cur_index)
{
	if(cur_index < numVariables())
		return cur_index+1;
	else
		return 0;
}

_R<ASObject> ASObject::nextName(uint32_t index)
{
	assert_and_throw(implEnable);

	return _MR(Class<ASString>::getInstanceS(getNameAt(index-1)));
}

_R<ASObject> ASObject::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);

	ASObject* out = getValueAt(index-1);
	out->incRef();
	return _MR(out);
}

void ASObject::sinit(Class_base* c)
{
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(_toString),DYNAMIC_TRAIT);
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

	LOG(LOG_CALLS,_("Equal comparison between type ")<<getObjectType()<< _(" and type ") << r->getObjectType());
	if(classdef)
		LOG(LOG_CALLS,_("Type ") << classdef->class_name);
	return false;
}

unsigned int ASObject::toUInt()
{
	return toInt();
}

int ASObject::toInt()
{
	return 0;
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
	multiname valueOfName;
	valueOfName.name_type=multiname::NAME_STRING;
	valueOfName.name_s="valueOf";
	valueOfName.ns.push_back(nsNameAndKind("",NAMESPACE));
	valueOfName.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	valueOfName.isAttribute = false;
	return hasPropertyByMultiname(valueOfName, true);
}

/* calls the valueOf function on this object
 * we cannot just call the c-function, because it can be overriden from AS3 code
 */
_R<ASObject> ASObject::call_valueOf()
{
	multiname valueOfName;
	valueOfName.name_type=multiname::NAME_STRING;
	valueOfName.name_s="valueOf";
	valueOfName.ns.push_back(nsNameAndKind("",NAMESPACE));
	valueOfName.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	valueOfName.isAttribute = false;
	assert_and_throw(hasPropertyByMultiname(valueOfName, true));

	ASObject* o=getVariableByMultiname(valueOfName,SKIP_IMPL);
	assert_and_throw(o->is<IFunction>());
	IFunction* f=o->as<IFunction>();

	incRef();
	ASObject *ret=f->call(this,NULL,0);
	return _MR(ret);
}

bool ASObject::has_toString()
{
	multiname toStringName;
	toStringName.name_type=multiname::NAME_STRING;
	toStringName.name_s="toString";
	toStringName.ns.push_back(nsNameAndKind("",NAMESPACE));
	toStringName.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	toStringName.isAttribute = false;
	return hasPropertyByMultiname(toStringName, true);
}

/* calls the toString function on this object
 * we cannot just call the c-function, because it can be overriden from AS3 code
 */
_R<ASObject> ASObject::call_toString()
{
	multiname toStringName;
	toStringName.name_type=multiname::NAME_STRING;
	toStringName.name_s="toString";
	toStringName.ns.push_back(nsNameAndKind("",NAMESPACE));
	toStringName.ns.push_back(nsNameAndKind(AS3,NAMESPACE));
	toStringName.isAttribute = false;
	assert_and_throw(hasPropertyByMultiname(toStringName, true));

	ASObject* o=getVariableByMultiname(toStringName,SKIP_IMPL);
	assert_and_throw(o->is<IFunction>());
	IFunction* f=o->as<IFunction>();

	incRef();
	ASObject *ret=f->call(this,NULL,0);
	return _MR(ret);
}

bool ASObject::isPrimitive() const
{
	// ECMA 3, section 4.3.2, T_INTEGER and T_UINTEGER are added
	// because they are special cases of Number
	return type==T_NUMBER || type ==T_UNDEFINED || type == T_NULL ||
		type==T_STRING || type==T_BOOLEAN || type==T_INTEGER ||
		type==T_UINTEGER;
}

variable* variables_map::findObjVar(const tiny_string& n, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds)
{
	const var_iterator ret_begin=Variables.lower_bound(n);
	//This actually look for the first different name, if we accept also previous levels
	//Otherwise we are just doing equal_range
	const var_iterator ret_end=Variables.upper_bound(n);

	var_iterator ret=ret_begin;
	for(;ret!=ret_end;++ret)
	{
		if(!(ret->second.kind & traitKinds))
			continue;

		if(ret->second.ns==ns)
			return &ret->second;
	}

	//Name not present, insert it if we have to create it
	if(createKind==NO_CREATE_TRAIT)
		return NULL;

	var_iterator inserted=Variables.insert(ret_begin,make_pair(n, variable(ns, createKind)) );
	return &inserted->second;
}

bool ASObject::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	//We look in all the object's levels
	uint32_t validTraits=DECLARED_TRAIT;
	if(considerDynamic)
		validTraits|=DYNAMIC_TRAIT;

	if(Variables.findObjVar(name, NO_CREATE_TRAIT, validTraits)!=NULL)
		return true;

	Class_base* cur=classdef;
	while(cur)
	{
		if(cur->Variables.findObjVar(name, NO_CREATE_TRAIT, BORROWED_TRAIT)!=NULL)
			return true;
		cur=cur->super;
	}

	//Check prototype inheritance chain
	if(getClass())
	{
		ASObject* proto = getClass()->prototype.getPtr();
		while(proto)
		{
			if(proto->findGettable(name, false) != NULL)
				return true;
			proto = proto->getprop_prototype();
		}
	}

	if(Class<ASObject>::getClass()->lazyDefine(name)!=NULL)
		return true;

	//Must not ask for non borrowed traits as static class member are not valid
	return false;
}

void ASObject::setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed)
{
	setDeclaredMethodByQName(name, nsNameAndKind(ns, NAMESPACE), o, type, isBorrowed);
}

void ASObject::setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed)
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

	variable* obj=Variables.findObjVar(name,ns, (isBorrowed)?BORROWED_TRAIT:DECLARED_TRAIT, (isBorrowed)?BORROWED_TRAIT:DECLARED_TRAIT);
	switch(type)
	{
		case NORMAL_METHOD:
		{
			if(obj->var!=NULL)
			{
				//This happens when interfaces are declared multiple times
				assert_and_throw(o==obj->var);
				return;
			}
			obj->setVar(o);
			break;
		}
		case GETTER_METHOD:
		{
			if(obj->getter!=NULL)
			{
				//This happens when interfaces are declared multiple times
				assert_and_throw(o==obj->getter);
				return;
			}
			obj->getter=o;
			break;
		}
		case SETTER_METHOD:
		{
			if(obj->setter!=NULL)
			{
				//This happens when interfaces are declared multiple times
				assert_and_throw(o==obj->setter);
				return;
			}
			obj->setter=o;
			break;
		}
	}
}

bool ASObject::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(ref_count>0);

	//Only dynamic traits are deletable
	variable* obj=Variables.findObjVar(name,NO_CREATE_TRAIT,DYNAMIC_TRAIT);
	if(obj==NULL)
		return false;

	assert(obj->getter==NULL && obj->setter==NULL && obj->var!=NULL);
	//Now dereference the value
	obj->var->decRef();

	//Now kill the variable
	Variables.killObjVar(name);
	return true;
}

//In all setter we first pass the value to the interface to see if special handling is possible
void ASObject::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	check();
	setVariableByMultiname(name,abstract_i(value));
}

variable* ASObject::findSettable(const multiname& name, bool borrowedMode, bool* has_getter)
{
	variable* ret=Variables.findObjVar(name,NO_CREATE_TRAIT,(borrowedMode)?BORROWED_TRAIT:(DECLARED_TRAIT|DYNAMIC_TRAIT));
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


void ASObject::setVariableByMultiname(const multiname& name, ASObject* o, Class_base* cls)
{
	check();
	assert(!cls || classdef->isSubClass(cls));
	//NOTE: we assume that [gs]etSuper and [sg]etProperty correctly manipulate the cur_level (for getActualClass)
	bool has_getter=false;
	variable* obj=findSettable(name, false, &has_getter);

	if(!obj && cls)
	{
		//Look for borrowed traits before
		//It's valid to override only a getter, so keep
		//looking for a settable even if a super class sets
		//has_getter to true.
		Class_base* cur=cls;
		while(cur)
		{
			obj=cur->findSettable(name,true,&has_getter);
			if(obj)
				break;
			cur=cur->super;
		}
	}

	if(!obj && cls)
	{
		//Look in prototype chain
		ASObject* proto = cls->prototype.getPtr();
		while(proto)
	        {
			obj = proto->findSettable(name, false, NULL /*prototypes never have getters/setters*/);
			if(obj)
				break;
			proto = proto->getprop_prototype();
		}
	}

	if(!obj)
	{
		if(has_getter)
		{
			tiny_string err=tiny_string("Error #1074: Illegal write to read-only property ")+name.normalizedName();
			if(cls)
				err+=tiny_string(" on type ")+cls->getQualifiedClassName();
			throw Class<ReferenceError>::getInstanceS(err);
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
	assert_and_throw(Variables.findObjVar(name,ns,NO_CREATE_TRAIT,traitKind)==NULL);
	variable* obj=Variables.findObjVar(name,ns,traitKind,traitKind);
	obj->setVar(o);
}

void ASObject::initializeVariableByMultiname(const multiname& name, ASObject* o, multiname* typemname)
{
	check();

	variable* obj=findSettable(name, false);
	if(obj)
	{
		//Initializing an already existing variable
		LOG(LOG_NOT_IMPLEMENTED,"Variable " << name << " already initialized");
		o->decRef();
		assert_and_throw(obj->typemname->qualifiedString()==typemname->qualifiedString());
		return;
	}

	Variables.initializeVar(name, o, typemname);
}

void variable::setVar(ASObject* v)
{
	//Resolve the typename if we have one
	if(!type && typemname)
		type = Type::getTypeFromMultiname(typemname);

	if(type)
		v = type->coerce(v);

	if(var)
		var->decRef();
	var=v;
}

void variables_map::killObjVar(const multiname& mname)
{
	tiny_string name=mname.normalizedName();
	const pair<var_iterator, var_iterator> ret=Variables.equal_range(name);
	assert_and_throw(ret.first!=ret.second);

	//Find the namespace
	assert_and_throw(!mname.ns.empty());
	for(unsigned int i=0;i<mname.ns.size();i++)
	{
		const nsNameAndKind& ns=mname.ns[i];
		var_iterator start=ret.first;
		for(;start!=ret.second;++start)
		{
			if(start->second.ns==ns)
			{
				Variables.erase(start);
				return;
			}
		}
	}

	throw RunTimeException("Variable to kill not found");
}

variable* variables_map::findObjVar(const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds)
{
	tiny_string name=mname.normalizedName();

	const var_iterator ret_begin=Variables.lower_bound(name);
	//This actually look for the first different name, if we accept also previous levels
	//Otherwise we are just doing equal_range
	const var_iterator ret_end=Variables.upper_bound(name);

	assert(!mname.ns.empty());
	var_iterator ret=ret_begin;
	for(;ret!=ret_end;++ret)
	{
		if(!(ret->second.kind & traitKinds))
			continue;
		//Check if one the namespace is already present
		//We can use binary search, as the namespace are ordered
		if(binary_search(mname.ns.begin(),mname.ns.end(),ret->second.ns))
			return &ret->second;
	}

	//Name not present, insert it, if the multiname has a single ns and if we have to insert it
	if(createKind==NO_CREATE_TRAIT)
		return NULL;

	//TODO: HACK: this is needed if the property should be present but it's not
	if(mname.ns.size()>1)
	{
		//Hack, insert with empty name
		//Here the object MUST exist
		LOG(LOG_NOT_IMPLEMENTED,"Hack: creating variable " << mname << " which should already be there.");
		var_iterator inserted=Variables.insert(ret,make_pair(name,
					variable(nsNameAndKind("",NAMESPACE), createKind)));
		return &inserted->second;
	}
	var_iterator inserted=Variables.insert(ret,make_pair(name, variable(mname.ns[0], createKind)));
	return &inserted->second;
}

void variables_map::initializeVar(const multiname& mname, ASObject* obj, multiname* typemname)
{
	tiny_string name=mname.normalizedName();

	const Type* type = NULL;
	/* The typename may be T_DEFINABLE at this point. Then we only allow obj to be Null
	 * and proceed.
	 * If typename is resolvable right now, we coerce obj
	 */
	if(!Type::isTypeResolvable(typemname))
	{
		assert_and_throw(obj->is<Null>() || obj->is<Undefined>());
		if(obj->is<Undefined>())
		{
			//Casting undefined to an object (of unknown class)
			//results in Null
			obj->decRef();
			obj = new Null;
		}
	}
	else
	{
		type = Type::getTypeFromMultiname(typemname);
		obj = type->coerce(obj);
	}

	Variables.insert(make_pair(name, variable(mname.ns[0], DECLARED_TRAIT, obj, typemname, type)));
}

ASFUNCTIONBODY(ASObject,generator)
{
	//By default we assume it's a passthrough cast
	assert_and_throw(argslen==1);
	LOG(LOG_CALLS,_("Passthrough of ") << args[0]);
	args[0]->incRef();
	return args[0];
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
	else
		ret="[object Object]";

	return Class<ASString>::getInstanceS(ret);
}

ASFUNCTIONBODY(ASObject,hasOwnProperty)
{
	assert_and_throw(argslen==1);
	multiname name;
	name.name_type=multiname::NAME_STRING;
	name.name_s=args[0]->toString();
	name.ns.push_back(nsNameAndKind("",NAMESPACE));
	name.isAttribute=false;
	bool ret=obj->hasPropertyByMultiname(name, true);
	return abstract_b(ret);
}

ASFUNCTIONBODY(ASObject,_constructor)
{
	return NULL;
}

void ASObject::initSlot(unsigned int n, const multiname& name)
{
	//Should be correct to use the level on the classdef chain
#ifndef NDEBUG
	assert(!initialized);
#endif
	Variables.initSlot(n,name.name_s,name.ns[0]);
}

intptr_t ASObject::getVariableByMultiname_i(const multiname& name)
{
	check();

	ASObject* ret=getVariableByMultiname(name);
	assert_and_throw(ret);
	return ret->toInt();
}

variable* ASObject::findGettable(const multiname& name, bool borrowedMode)
{
	variable* ret=Variables.findObjVar(name,NO_CREATE_TRAIT,(borrowedMode)?BORROWED_TRAIT:(DECLARED_TRAIT|DYNAMIC_TRAIT));
	if(ret)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(ret->getter || ret->var))
			ret=NULL;
	}
	return ret;
}

ASObject* ASObject::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls)
{
	check();
	assert(!cls || classdef->isSubClass(cls));
	//Get from the current object without considering borrowed properties
	variable* obj=findGettable(name, false);

	if(!obj && cls)
	{
		//Look for borrowed traits before
		Class_base* cur=cls;
		while(cur)
		{
			obj=cur->findGettable(name,true);
			if(obj)
				break;
			cur=cur->super;
		}
	}

	if(!obj && cls)
	{
		//Check prototype chain
		ASObject* proto = cls->prototype.getPtr();
		while(proto)
		{
			obj = proto->findGettable(name, false);
			if(obj)
				return obj->var;
			proto = proto->getprop_prototype();
		}
	}

	if(!obj)
	{
		//Check if we can lazily define the requested property
		return Class<ASObject>::getClass()->lazyDefine(name);
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
		//The returned value is already owned by the caller
		ret->fake_decRef();
		return ret;
	}
	else
	{
		assert_and_throw(!obj->setter);
		assert_and_throw(obj->var);
		if(obj->var->getObjectType()==T_FUNCTION && obj->var->as<IFunction>()->isMethod())
		{
			//TODO: this creates a memory leak!
			//Functions defined on prototypes or on global objects
			//become function closures (instead of method closures):
			//they do not remember their 'this' pointer
			//TODO: maybe also the level should be binded
			LOG(LOG_CALLS,"Attaching this " << this << " to function " << name);
			//the obj reference is acquired by the smart reference
			this->incRef();
			IFunction* f=obj->var->as<IFunction>()->bind(_MR(this),-1);
			//No incref is needed, as the function is a new instance
			return f;
		}
		return obj->var;
	}
}

void ASObject::check() const
{
	//Put here a bunch of safety check on the object
	assert_and_throw(ref_count>0);
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
		next++;
		if(next==Variables.end())
			break;

		//No double definition of a single variable should exist
		if(it->first==next->first && it->second.ns==next->second.ns)
		{
			if(it->second.var==NULL && next->second.var==NULL)
				continue;

			if(it->second.var==NULL || next->second.var==NULL)
			{
				cout << it->first << endl;
				cout << it->second.var << ' ' << it->second.setter << ' ' << it->second.getter << endl;
				cout << next->second.var << ' ' << next->second.setter << ' ' << next->second.getter << endl;
				abort();
			}

			if(it->second.var->getObjectType()!=T_FUNCTION || next->second.var->getObjectType()!=T_FUNCTION)
			{
				cout << it->first << endl;
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
			case BORROWED_TRAIT:
				kind="Borrowed: ";
				break;
			case DYNAMIC_TRAIT:
				kind="Dynamic: ";
				break;
			case NO_CREATE_TRAIT:
				kind="NoCreate: ";
				assert(false);
		}
		LOG(LOG_INFO, kind <<  '[' << it->second.ns.name << "] "<< it->first << ' ' <<
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

ASObject::ASObject():type(T_OBJECT),ref_count(1),manager(NULL),classdef(NULL),constructed(false),
		implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif
}

ASObject::ASObject(const ASObject& o):type(o.type),ref_count(1),manager(NULL),classdef(o.classdef),
		constructed(false),implEnable(true)
{
	if(classdef)
	{
		classdef->incRef();
	}

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
}

ASObject::~ASObject()
{
	finalize();
	if(classdef)
	{
		classdef->abandonObject(this);
		classdef->decRef();
	}
}

void variables_map::initSlot(unsigned int n, const tiny_string& name, const nsNameAndKind& ns)
{
	if(n>slots_vars.size())
		slots_vars.resize(n,Variables.end());

	pair<var_iterator, var_iterator> ret=Variables.equal_range(name);
	if(ret.first!=ret.second)
	{
		//Check if this namespace is already present
		var_iterator start=ret.first;
		for(;start!=ret.second;++start)
		{
			if(start->second.ns==ns)
			{
				slots_vars[n-1]=start;
				return;
			}
		}
	}

	//Name not present, no good
	throw RunTimeException("initSlot on missing variable");
}

void variables_map::setSlot(unsigned int n,ASObject* o)
{
	if(n-1<slots_vars.size())
	{
		assert_and_throw(slots_vars[n-1]!=Variables.end());
		if(slots_vars[n-1]->second.setter)
			throw UnsupportedException("setSlot has setters");
		slots_vars[n-1]->second.setVar(o);
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

ASObject* ASObject::getValueAt(int index)
{
	variable* obj=Variables.getValueAt(index);
	assert_and_throw(obj);
	ASObject* ret;
	if(obj->getter)
	{
		//Call the getter
		LOG(LOG_CALLS,_("Calling the getter"));
		IFunction* getter=obj->getter;
		incRef();
		ret=getter->call(this,NULL,0);
		ret->fake_decRef();
		LOG(LOG_CALLS,_("End of getter"));
	}
	else
		ret=obj->var;

	return ret;
}

tiny_string variables_map::getNameAt(unsigned int index) const
{
	//TODO: CHECK behaviour on overridden methods
	if(index<Variables.size())
	{
		const_var_iterator it=Variables.begin();

		for(unsigned int i=0;i<index;i++)
			++it;

		return it->first;
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
	//Nothing to be done now
}

void ASObject::serializeDynamicProperties(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	Variables.serialize(out, stringMap, objMap);
}

void variables_map::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	//Pairs of name, value
	auto it=Variables.begin();
	for(;it!=Variables.end();it++)
	{
		assert_and_throw(it->second.ns.name=="");
		out->writeStringVR(stringMap,it->first);
		it->second.var->serialize(out, stringMap, objMap);
	}
	//The empty string closes the object
	out->writeStringVR(stringMap, "");
}

void ASObject::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	Class_base* type=getClass();
	if(type!=Class<ASObject>::getClass())
		throw UnsupportedException("ASObject::serialize not completely implemented");

	//0x0A -> object marker
	out->writeByte(amf3::object_marker);
	//0x0B -> a dynamic instance follows
	out->writeByte(0x0B);
	//The class name, empty if no alias is registered
	out->writeStringVR(stringMap, "");
	serializeDynamicProperties(out, stringMap, objMap);
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
	bool isDynamic=type==T_ARRAY; // FIXME
	root->set_attribute("isDynamic", isDynamic?"true":"false");
	bool isFinal=!(type==T_OBJECT || type==T_ARRAY); // FIXME
	root->set_attribute("isFinal", isFinal?"true":"false");
	root->set_attribute("isStatic", "false");

	if(prot)
		prot->describeInstance(root);

	// TODO: undocumented constructor node

	return Class<XML>::getInstanceS(root);
}

bool ASObject::hasprop_prototype()
{
	multiname prototypeName;
	prototypeName.name_type=multiname::NAME_STRING;
	prototypeName.name_s="prototype";
	prototypeName.ns.push_back(nsNameAndKind("",NAMESPACE));
	prototypeName.isAttribute = false;
	return findGettable(prototypeName, false) != NULL;
}

ASObject* ASObject::getprop_prototype()
{
	multiname prototypeName;
	prototypeName.name_type=multiname::NAME_STRING;
	prototypeName.name_s="prototype";
	prototypeName.ns.push_back(nsNameAndKind("",NAMESPACE));
	prototypeName.isAttribute = false;
	variable* var = findGettable(prototypeName, false);
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

	multiname prototypeName;
	prototypeName.name_type=multiname::NAME_STRING;
	prototypeName.name_s="prototype";
	prototypeName.ns.push_back(nsNameAndKind("",NAMESPACE));
	bool has_getter = false;
	variable* ret=findSettable(prototypeName,false, &has_getter);
	if(!ret && has_getter)
		throw Class<ReferenceError>::getInstanceS("Error #1074: Illegal write to read-only property prototype");
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
