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
#include "scripting/toplevel.h"
#include <algorithm>
#include <limits>
#include "compat.h"
#include "parsing/amf3_generator.h"

using namespace lightspark;
using namespace std;

REGISTER_CLASS_NAME2(ASObject,"Object","");

tiny_string ASObject::toStringImpl() const
{
	tiny_string ret;
	if(getPrototype())
	{
		ret+="[object ";
		ret+=getPrototype()->class_name.name;
		ret+="]";
		return ret;
	}
	else
		ret="[object Object]";

	return ret;
}

tiny_string ASObject::toString(bool debugMsg)
{
	check();
	multiname toStringName;
	toStringName.name_type=multiname::NAME_STRING;
	toStringName.name_s="toString";
	toStringName.ns.push_back(nsNameAndKind("",PACKAGE_NAMESPACE));
	if(debugMsg==false && hasPropertyByMultiname(toStringName, true))
	{
		ASObject* obj_toString=getVariableByMultiname(toStringName);
		if(obj_toString->getObjectType()==T_FUNCTION)
		{
			IFunction* f_toString=static_cast<IFunction*>(obj_toString);
			incRef();
			ASObject* ret=f_toString->call(this,NULL,0);
			assert_and_throw(ret->getObjectType()==T_STRING);
			tiny_string retS=ret->toString();
			ret->decRef();
			return retS;
		}
	}

	return toStringImpl();
}

TRISTATE ASObject::isLess(ASObject* r)
{
	check();
	multiname valueOfName;
	valueOfName.name_type=multiname::NAME_STRING;
	valueOfName.name_s="valueOf";
	valueOfName.ns.push_back(nsNameAndKind("",NAMESPACE));
	if(hasPropertyByMultiname(valueOfName, true))
	{
		if(r->hasPropertyByMultiname(valueOfName, true)==false)
			throw RunTimeException("Missing valueof for second operand");

		ASObject* obj1=getVariableByMultiname(valueOfName);
		ASObject* obj2=r->getVariableByMultiname(valueOfName);

		assert_and_throw(obj1!=NULL && obj2!=NULL);

		assert_and_throw(obj1->getObjectType()==T_FUNCTION && obj2->getObjectType()==T_FUNCTION);
		IFunction* f1=static_cast<IFunction*>(obj1);
		IFunction* f2=static_cast<IFunction*>(obj2);

		incRef();
		ASObject* ret1=f1->call(this,NULL,0);
		r->incRef();
		ASObject* ret2=f2->call(r,NULL,0);

		LOG(LOG_CALLS,_("Overloaded isLess"));
		return ret1->isLess(ret2);
	}

	LOG(LOG_NOT_IMPLEMENTED,_("Less than comparison between type ")<<getObjectType()<< _(" and type ") << r->getObjectType());
	if(prototype)
		LOG(LOG_NOT_IMPLEMENTED,_("Type ") << prototype->class_name);
	throw RunTimeException("Not handled less comparison for objects");
	return TFALSE;
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
}

void ASObject::buildTraits(ASObject* o)
{
	if(o->getActualPrototype()->class_name.name!="Object")
		LOG(LOG_NOT_IMPLEMENTED,_("Add buildTraits for class ") << o->getActualPrototype()->class_name);
}

bool ASObject::isEqual(ASObject* r)
{
	check();
	//if we are comparing the same object the answer is true
	if(this==r)
		return true;

	if(r->getObjectType()==T_NULL || r->getObjectType()==T_UNDEFINED)
		return false;

	multiname equalsName;
	equalsName.name_type=multiname::NAME_STRING;
	equalsName.name_s="equals";
	equalsName.ns.push_back(nsNameAndKind("",NAMESPACE));
	if(hasPropertyByMultiname(equalsName, true))
	{
		ASObject* func_equals=getVariableByMultiname(equalsName);

		assert_and_throw(func_equals!=NULL);

		assert_and_throw(func_equals->getObjectType()==T_FUNCTION);
		IFunction* func=static_cast<IFunction*>(func_equals);

		incRef();
		r->incRef();
		ASObject* ret=func->call(this,&r,1);
		assert_and_throw(ret->getObjectType()==T_BOOLEAN);

		LOG(LOG_CALLS,_("Overloaded isEqual"));
		return Boolean_concrete(ret);
	}

	//We can try to call valueOf and compare that
	multiname valueOfName;
	valueOfName.name_type=multiname::NAME_STRING;
	valueOfName.name_s="valueOf";
	valueOfName.ns.push_back(nsNameAndKind("",NAMESPACE));
	if(hasPropertyByMultiname(valueOfName, true))
	{
		if(r->hasPropertyByMultiname(valueOfName, true)==false)
			throw RunTimeException("Not handled less comparison for objects");

		ASObject* obj1=getVariableByMultiname(valueOfName);
		ASObject* obj2=r->getVariableByMultiname(valueOfName);

		assert_and_throw(obj1!=NULL && obj2!=NULL);

		assert_and_throw(obj1->getObjectType()==T_FUNCTION && obj2->getObjectType()==T_FUNCTION);
		IFunction* f1=static_cast<IFunction*>(obj1);
		IFunction* f2=static_cast<IFunction*>(obj2);

		incRef();
		ASObject* ret1=f1->call(this,NULL,0);
		r->incRef();
		ASObject* ret2=f2->call(r,NULL,0);

		LOG(LOG_CALLS,_("Overloaded isEqual"));
		return ret1->isEqual(ret2);
	}

	if(r->getObjectType()==T_OBJECT)
	{
		XMLList *xl=dynamic_cast<XMLList *>(r);
		if(xl)
			return xl->isEqual(this);
		XML *x=dynamic_cast<XML *>(r);
		if(x && x->hasSimpleContent())
			return x->toString()==toString();
	}

	LOG(LOG_CALLS,_("Equal comparison between type ")<<getObjectType()<< _(" and type ") << r->getObjectType());
	if(prototype)
		LOG(LOG_CALLS,_("Type ") << prototype->class_name);
	return false;
}

unsigned int ASObject::toUInt()
{
	return toInt();
}

int ASObject::toInt()
{
	LOG(LOG_ERROR,_("Cannot convert object of type ") << getObjectType() << _(" to int"));
	throw RunTimeException("Cannot convert object to int");
	return 0;
}

double ASObject::toNumber()
{
	//If we're here it means that the object does not have a special implementation
	tiny_string ret=toString(false);
	char* end;
	const char* str=ret.raw_buf();
	double doubleRet=strtod(str,&end);
	return (*end=='\0')?doubleRet:numeric_limits<double>::quiet_NaN();
}

obj_var* variables_map::findObjVar(const tiny_string& n, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds)
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
			return &ret->second.var;
	}

	//Name not present, insert it if we have to create it
	if(createKind==NO_CREATE_TRAIT)
		return NULL;

	var_iterator inserted=Variables.insert(ret_begin,make_pair(n, variable(ns, createKind)) );
	return &inserted->second.var;
}

bool ASObject::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	bool ret=false;
	//We look in all the object's levels
	uint32_t validTraits=DECLARED_TRAIT;
	if(considerDynamic)
		validTraits|=DYNAMIC_TRAIT;

	ret=(Variables.findObjVar(name, NO_CREATE_TRAIT, validTraits)!=NULL);

	if(!ret) //Ask the prototype chain for borrowed traits
	{
		Class_base* cur=prototype;
		while(cur)
		{
			ret=(cur->Variables.findObjVar(name, NO_CREATE_TRAIT, BORROWED_TRAIT)!=NULL);
			if(ret)
				break;
			cur=cur->super;
		}
	}
	//Must not ask for non borrowed traits as static class member are not valid
	return ret;
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
	obj_var* obj=Variables.findObjVar(name,ns, (isBorrowed)?BORROWED_TRAIT:DECLARED_TRAIT, (isBorrowed)?BORROWED_TRAIT:DECLARED_TRAIT);
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

void ASObject::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(ref_count>0);

	//Only dynamic traits are deletable
	obj_var* obj=Variables.findObjVar(name,NO_CREATE_TRAIT,DYNAMIC_TRAIT);
	if(obj==NULL)
		return;

	assert(obj->getter==NULL && obj->setter==NULL && obj->var!=NULL);
	//Now dereference the value
	obj->var->decRef();

	//Now kill the variable
	Variables.killObjVar(name);
}

//In all setter we first pass the value to the interface to see if special handling is possible
void ASObject::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	check();
	setVariableByMultiname(name,abstract_i(value));
}

obj_var* ASObject::findSettable(const multiname& name, bool borrowedMode)
{
	obj_var* ret=Variables.findObjVar(name,NO_CREATE_TRAIT,(borrowedMode)?BORROWED_TRAIT:(DECLARED_TRAIT|DYNAMIC_TRAIT));
	if(ret)
	{
		//It seems valid for a class to redefine only the getter, so if we can't find
		//something to get, it's ok
		if(!(ret->setter || ret->var))
			ret=NULL;
	}
	return ret;
}

void ASObject::setVariableByMultiname(const multiname& name, ASObject* o)
{
	check();

	//NOTE: we assume that [gs]etSuper and [sg]etProperty correctly manipulate the cur_level (for getActualPrototype)
	obj_var* obj=findSettable(name, false);

	if(obj==NULL && prototype)
	{
		//Look for borrowed traits before
		Class_base* cur=getActualPrototype();
		while(cur)
		{
			//TODO: should be only findSetter
			obj=cur->findSettable(name,true);
			if(obj)
				break;
			cur=cur->super;
		}
	}
	if(obj==NULL)
		obj=Variables.findObjVar(name,DYNAMIC_TRAIT,DYNAMIC_TRAIT);

	if(obj->setter)
	{
		//Call the setter
		LOG(LOG_CALLS,_("Calling the setter"));
		//Overriding function is automatically done by using cur_level
		IFunction* setter=obj->setter;
		//One argument can be passed without creating an array
		ASObject* target=this;
		target->incRef();
		ASObject* ret=setter->call(target,&o,1);
		assert_and_throw(ret==NULL);
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
	obj_var* obj=Variables.findObjVar(name,ns,NO_CREATE_TRAIT,traitKind);
	assert(obj==NULL);
	assert_and_throw(obj==NULL);
	obj=Variables.findObjVar(name,ns,traitKind,traitKind);
	obj->setVar(o);
}

void ASObject::initializeVariableByMultiname(const multiname& name, ASObject* o, Class_base* c)
{
	check();

	obj_var* obj=findSettable(name, false);
	if(obj)
	{
		//Initializing an already existing variable
		LOG(LOG_NOT_IMPLEMENTED,"Variable " << name << " already initialized");
		o->decRef();
		assert_and_throw(obj->type==c);
		return;
	}

	Variables.initializeVar(name, o, c);
}

void obj_var::setVar(ASObject* v)
{
	//Do the conversion early, so that errors does not leave the object in an half baked state
	ASObject* newV=v;
	if(type && v->getObjectType()!=T_NULL && (v->getPrototype()==NULL || !v->getPrototype()->isSubClass(type)))
	{
		newV=type->generator(&v,1);
		v->decRef();
	}
	if(var)
		var->decRef();
	var=newV;
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

obj_var* variables_map::findObjVar(const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds)
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
			return &ret->second.var;
	}

	//Name not present, insert it, if the multiname has a single ns and if we have to insert it
	if(createKind==NO_CREATE_TRAIT)
		return NULL;

	//TODO: HACK: this is needed if the property should be present but it's not
	if(mname.ns.size()>1)
	{
		//Hack, insert with empty name
		//Here the object MUST exist
		var_iterator inserted=Variables.insert(ret,make_pair(name, 
					variable(nsNameAndKind("",NAMESPACE), createKind)));
		return &inserted->second.var;
	}
	var_iterator inserted=Variables.insert(ret,make_pair(name, variable(mname.ns[0], createKind)));
	return &inserted->second.var;
}

void variables_map::initializeVar(const multiname& mname, ASObject* obj, Class_base* type)
{
	tiny_string name=mname.normalizedName();
	Variables.insert(make_pair(name, variable(mname.ns[0], DECLARED_TRAIT, obj, type)));
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
	return Class<ASString>::getInstanceS(obj->toStringImpl());
}

ASFUNCTIONBODY(ASObject,hasOwnProperty)
{
	assert_and_throw(argslen==1);
	multiname name;
	name.name_type=multiname::NAME_STRING;
	name.name_s=args[0]->toString();
	name.ns.push_back(nsNameAndKind("",NAMESPACE));
	bool ret=obj->hasPropertyByMultiname(name, true);
	return abstract_b(ret);
}

ASFUNCTIONBODY(ASObject,_constructor)
{
	return NULL;
}

/*ASFUNCTIONBODY(ASObject,_getPrototype)
{
	if(prototype==NULL)
		return new Undefined;

	prototype->incRef();
	return prototype;
}

ASFUNCTIONBODY(ASObject,_setPrototype)
{
	if(prototype)
		prototype->decRef();

	prototype=args->at(0);
	prototype->incRef();
	return NULL;
}*/


void ASObject::initSlot(unsigned int n, const multiname& name)
{
	//Should be correct to use the level on the prototype chain
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

obj_var* ASObject::findGettable(const multiname& name, bool borrowedMode)
{
	obj_var* ret=Variables.findObjVar(name,NO_CREATE_TRAIT,(borrowedMode)?BORROWED_TRAIT:(DECLARED_TRAIT|DYNAMIC_TRAIT));
	if(ret)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(ret->getter || ret->var))
			ret=NULL;
	}
	return ret;
}

ASObject* ASObject::getVariableByMultiname(const multiname& name, bool skip_impl)
{
	check();

	//Get from the current object without considering borrowed properties
	obj_var* obj=findGettable(name, false);

	if(obj==NULL && prototype)
	{
		//Look for borrowed traits before
		Class_base* cur=getActualPrototype();
		while(cur)
		{
			obj=cur->findGettable(name,true);
			if(obj)
				break;
			cur=cur->super;
		}
	}

	//If it has not been found
	if(obj==NULL)
	{
		if(prototype==NULL)
			return NULL;

		//Check if we can lazily define the requested property
		return Class<ASObject>::getClass()->lazyDefine(name);
	}

	if(obj->getter)
	{
		//Call the getter
		ASObject* target=this;
		if(target->prototype)
		{
			LOG(LOG_CALLS,_("Calling the getter on type ") << target->prototype->class_name);
		}
		else
		{
			LOG(LOG_CALLS,_("Calling the getter"));
		}
		IFunction* getter=obj->getter;
		target->incRef();
		ASObject* ret=getter->call(target,NULL,0);
		LOG(LOG_CALLS,_("End of getter"));
		if(ret==NULL)
			ret=new Undefined;
		//The returned value is already owned by the caller
		ret->fake_decRef();
		return ret;
	}
	else
	{
		assert_and_throw(!obj->setter);
		assert_and_throw(obj->var);
		return obj->var;
	}
}

void ASObject::check() const
{
	//Put here a bunch of safety check on the object
	assert_and_throw(ref_count>0);
	//Heavyweight stuff
#ifdef EXPENSIVE_DEBUG
	variables_map::const_var_iterator it=Variables.Variables.begin();
	for(;it!=Variables.Variables.end();++it)
	{
		variables_map::const_var_iterator next=it;
		next++;
		if(next==Variables.Variables.end())
			break;

		//No double definition of a single variable should exist
		if(it->first==next->first && it->second.ns==next->second.ns)
		{
			if(it->second.var.var==NULL && next->second.var.var==NULL)
				continue;

			if(it->second.var.var==NULL || next->second.var.var==NULL)
			{
				cout << it->first << endl;
				cout << it->second.var.var << ' ' << it->second.var.setter << ' ' << it->second.var.getter << endl;
				cout << next->second.var.var << ' ' << next->second.var.setter << ' ' << next->second.var.getter << endl;
				abort();
			}

			if(it->second.var.var->getObjectType()!=T_FUNCTION || next->second.var.var->getObjectType()!=T_FUNCTION)
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
				assert(false);
		}
		LOG(LOG_NO_INFO, kind <<  '[' << it->second.ns.name << "] "<< it->first << ' ' <<
			it->second.var.var << ' ' << it->second.var.setter << ' ' << it->second.var.getter);
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
		if(it->second.var.var)
			it->second.var.var->decRef();
		if(it->second.var.setter)
			it->second.var.setter->decRef();
		if(it->second.var.getter)
			it->second.var.getter->decRef();
	}
	Variables.clear();
}

ASObject::ASObject(Manager* m):type(T_OBJECT),ref_count(1),manager(m),cur_level(0),prototype(NULL),constructed(false),
		implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif
}

ASObject::ASObject(const ASObject& o):type(o.type),ref_count(1),manager(NULL),cur_level(0),prototype(o.prototype),
		constructed(false),implEnable(true)
{
	if(prototype)
	{
		prototype->incRef();
		cur_level=prototype->max_level;
	}

#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif

	assert_and_throw(o.Variables.size()==0);
}

void ASObject::setPrototype(Class_base* c)
{
	if(prototype)
	{
		prototype->abandonObject(this);
		prototype->decRef();
	}
	prototype=c;
	if(prototype)
	{
		prototype->acquireObject(this);
		prototype->incRef();
		setLevel(prototype->max_level);
	}
}

void ASObject::finalize()
{
	Variables.destroyContents();
}

ASObject::~ASObject()
{
	finalize();
	if(prototype)
	{
		prototype->decRef();
		prototype->abandonObject(this);
	}
}

int ASObject::_maxlevel()
{
	return (prototype)?(prototype->max_level):0;
}

void ASObject::resetLevel()
{
	cur_level=_maxlevel();
}

Class_base* ASObject::getActualPrototype() const
{
	Class_base* ret=prototype;
	if(ret==NULL)
	{
		assert(type==T_CLASS);
		return NULL;
	}

	for(int i=prototype->max_level;i>cur_level;i--)
		ret=ret->super;

	assert(ret);
	assert(ret->max_level==cur_level);
	return ret;
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
		if(slots_vars[n-1]->second.var.setter)
			throw UnsupportedException("setSlot has setters");
		slots_vars[n-1]->second.var.setVar(o);
	}
	else
		throw RunTimeException("setSlot out of bounds");
}

obj_var* variables_map::getValueAt(unsigned int index)
{
	//TODO: CHECK behaviour on overridden methods
	if(index<Variables.size())
	{
		var_iterator it=Variables.begin();

		for(unsigned int i=0;i<index;i++)
			++it;

		return &it->second.var;
	}
	else
		throw RunTimeException("getValueAt out of bounds");
}

ASObject* ASObject::getValueAt(int index)
{
	obj_var* obj=Variables.getValueAt(index);
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
	//Pairs of name, value
	auto it=Variables.Variables.begin();
	for(;it!=Variables.Variables.end();it++)
	{
		assert_and_throw(it->second.ns.name=="");
		out->writeStringVR(stringMap,it->first);
		it->second.var.var->serialize(out, stringMap, objMap);
	}
	//The empty string closes the object
	out->writeStringVR(stringMap, "");
}

void ASObject::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const
{
	Class_base* type=getPrototype();
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
