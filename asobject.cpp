/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

using namespace lightspark;
using namespace std;

tiny_string ASObject::toString(bool debugMsg)
{
	check();
	if(debugMsg==false && hasPropertyByQName("toString",""))
	{
		ASObject* obj_toString=getVariableByQName("toString","");
		if(obj_toString->getObjectType()==T_FUNCTION)
		{
			IFunction* f_toString=static_cast<IFunction*>(obj_toString);
			ASObject* ret=f_toString->call(this,NULL,0);
			assert_and_throw(ret->getObjectType()==T_STRING);
			return ret->toString();
		}
	}

	if(getPrototype())
	{
		tiny_string ret;
		ret+="[object ";
		ret+=getPrototype()->class_name;
		ret+="]";
		return ret;
	}
	else
		return "[object Object]";
}

TRISTATE ASObject::isLess(ASObject* r)
{
	check();
	if(hasPropertyByQName("valueOf",""))
	{
		if(r->hasPropertyByQName("valueOf","")==false)
			throw RunTimeException("Missing valueof for second operand");

		ASObject* obj1=getVariableByQName("valueOf","");
		ASObject* obj2=r->getVariableByQName("valueOf","");

		assert_and_throw(obj1!=NULL && obj2!=NULL);

		assert_and_throw(obj1->getObjectType()==T_FUNCTION && obj2->getObjectType()==T_FUNCTION);
		IFunction* f1=static_cast<IFunction*>(obj1);
		IFunction* f2=static_cast<IFunction*>(obj2);

		ASObject* ret1=f1->call(this,NULL,0);
		ASObject* ret2=f2->call(r,NULL,0);

		LOG(LOG_CALLS,"Overloaded isLess");
		return ret1->isLess(ret2);
	}

	LOG(LOG_NOT_IMPLEMENTED,"Less than comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	if(prototype)
		LOG(LOG_NOT_IMPLEMENTED,"Type " << prototype->class_name);
	throw RunTimeException("Not handled less comparison for objects");
	return TFALSE;
}

bool ASObject::hasNext(unsigned int& index, bool& out)
{
	assert_and_throw(implEnable);
	return false;
}

bool ASObject::nextName(unsigned int index, ASObject*& out)
{
	assert_and_throw(implEnable);
	return false;
}

bool ASObject::nextValue(unsigned int index, ASObject*& out)
{
	assert_and_throw(implEnable);
	return false;
}

void ASObject::sinit(Class_base* c)
{
}

void ASObject::buildTraits(ASObject* o)
{
	if(o->getActualPrototype()->class_name!="ASObject")
		LOG(LOG_NOT_IMPLEMENTED,"Add buildTraits for class " << o->getActualPrototype()->class_name);
}

bool ASObject::isEqual(ASObject* r)
{
	check();
	//if we are comparing the same object the answer is true
	if(this==r)
		return true;

	if(r->getObjectType()==T_NULL || r->getObjectType()==T_UNDEFINED)
		return false;

	if(hasPropertyByQName("equals",""))
	{
		ASObject* func_equals=getVariableByQName("equals","");

		assert_and_throw(func_equals!=NULL);

		assert_and_throw(func_equals->getObjectType()==T_FUNCTION);
		IFunction* func=static_cast<IFunction*>(func_equals);

		ASObject* ret=func->call(this,&r,1);
		assert_and_throw(ret->getObjectType()==T_BOOLEAN);

		LOG(LOG_CALLS,"Overloaded isEqual");
		return Boolean_concrete(ret);
	}

	//We can try to call valueOf (maybe equals) and compare that
	if(hasPropertyByQName("valueOf",""))
	{
		if(r->hasPropertyByQName("valueOf","")==false)
			throw RunTimeException("Not handled less comparison for objects");

		ASObject* obj1=getVariableByQName("valueOf","");
		ASObject* obj2=r->getVariableByQName("valueOf","");

		assert_and_throw(obj1!=NULL && obj2!=NULL);

		assert_and_throw(obj1->getObjectType()==T_FUNCTION && obj2->getObjectType()==T_FUNCTION);
		IFunction* f1=static_cast<IFunction*>(obj1);
		IFunction* f2=static_cast<IFunction*>(obj2);

		ASObject* ret1=f1->call(this,NULL,0);
		ASObject* ret2=f2->call(r,NULL,0);

		LOG(LOG_CALLS,"Overloaded isEqual");
		return ret1->isEqual(ret2);
	}

	LOG(LOG_CALLS,"Equal comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	if(prototype)
		LOG(LOG_CALLS,"Type " << prototype->class_name);
	return false;
}

unsigned int ASObject::toUInt()
{
	return toInt();
}

int ASObject::toInt()
{
	LOG(LOG_ERROR,"Cannot convert object of type " << getObjectType() << " to int");
	throw RunTimeException("Cannot converto object to int");
	return 0;
}

double ASObject::toNumber()
{
	LOG(LOG_ERROR,"Cannot convert object of type " << getObjectType() << " to float");
	throw RunTimeException("Cannot converto object to float");
	return 0;
}

obj_var* variables_map::findObjVar(const tiny_string& n, const tiny_string& ns, bool create)
{
	const var_iterator ret_begin=Variables.lower_bound(n);
	//This actually look for the first different name, if we accept also previous levels
	//Otherwise we are just doing equal_range
	const var_iterator ret_end=Variables.upper_bound(n);

	var_iterator ret=ret_begin;
	for(;ret!=ret_end;ret++)
	{
		if(ret->second.first==ns)
			return &ret->second.second;
	}

	//Name not present, insert it if we have to create it
	if(create)
	{
		var_iterator inserted=Variables.insert(ret_begin,make_pair(n, make_pair(ns, obj_var() ) ) );
		return &inserted->second.second;
	}
	else
		return NULL;
}

bool ASObject::hasPropertyByQName(const tiny_string& name, const tiny_string& ns)
{
	check();
	//We look in all the object's levels
	bool ret=(Variables.findObjVar(name, ns, false)!=NULL);
	if(!ret) //Try the classes
	{
		Class_base* cur=prototype;
		while(cur)
		{
			ret=(cur->Variables.findObjVar(name, ns, false)!=NULL);
			if(ret)
				break;
			cur=cur->super;
		}
	}
	return ret;
}

bool ASObject::hasPropertyByMultiname(const multiname& name)
{
	check();
	//We look in all the object's levels
	bool ret=(Variables.findObjVar(name, false)!=NULL);
	if(!ret) //Try the classes
	{
		Class_base* cur=prototype;
		while(cur)
		{
			ret=(cur->Variables.findObjVar(name, false)!=NULL);
			if(ret)
				break;
			cur=cur->super;
		}
	}
	return ret;
}

void ASObject::setGetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o)
{
	check();
#ifndef NDEBUG
	assert(!initialized);
#endif
	assert(getObjectType()==T_CLASS);
	obj_var* obj=Variables.findObjVar(name,ns,true);
	if(obj->getter!=NULL)
	{
		//This happens when interfaces are declared multiple times
		assert_and_throw(o==obj->getter);
		return;
	}
	obj->getter=o;
}

void ASObject::setSetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o)
{
	check();
#ifndef NDEBUG
	assert_and_throw(!initialized);
#endif
	assert(getObjectType()==T_CLASS);
	obj_var* obj=Variables.findObjVar(name,ns,true);
	if(obj->setter!=NULL)
	{
		//This happens when interfaces are declared multiple times
		assert_and_throw(o==obj->setter);
		return;
	}
	obj->setter=o;
}

void ASObject::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(ref_count>0);

	obj_var* obj=Variables.findObjVar(name,false);
	if(obj==NULL)
		return;

	//Now dereference the values
	obj=Variables.findObjVar(name,false);
	if(obj->var)
		obj->var->decRef();
	if(obj->getter)
		obj->getter->decRef();
	if(obj->setter)
		obj->setter->decRef();

	//Now kill the variable
	Variables.killObjVar(name);
}

//In all setter we first pass the value to the interface to see if special handling is possible
void ASObject::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	check();
	setVariableByMultiname(name,abstract_i(value));
}

obj_var* ASObject::findSettable(const multiname& name)
{
	obj_var* ret=Variables.findObjVar(name,false);
	if(ret)
	{
		//It seems valid for a class to redefine only the getter, so if we can't find
		//something to get, it's ok
		if(!(ret->setter || ret->var))
			ret=NULL;
	}
	return ret;
}

void ASObject::setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride, ASObject* base)
{
	check();

	//NOTE: we assume that [gs]etSuper and [sg]etProperty correctly manipulate the cur_level (for getActualPrototype)
	obj_var* obj=findSettable(name);

	if(obj==NULL && prototype)
	{
		Class_base* cur=getActualPrototype();
		while(cur)
		{
			//TODO: should be only findSetter
			obj=cur->findSettable(name);
			if(obj)
				break;
			cur=cur->super;
		}
	}
	if(obj==NULL)
		obj=Variables.findObjVar(name,true);

	if(obj->setter)
	{
		//Call the setter
		LOG(LOG_CALLS,"Calling the setter");
		//Overriding function is automatically done by using cur_level
		IFunction* setter=obj->setter;
		if(enableOverride)
			setter=setter->getOverride();

		//One argument can be passed without creating an array
		ASObject* target=(base)?base:this;
		target->incRef();
		ASObject* ret=setter->call(target,&o,1);
		assert_and_throw(ret==NULL);
		LOG(LOG_CALLS,"End of setter");
	}
	else
	{
		assert_and_throw(!obj->getter);
		if(obj->var)
			obj->var->decRef();
		obj->var=o;
	}
}

void ASObject::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool skip_impl)
{
	//NOTE: we assume that [gs]etSuper and setProperty correctly manipulate the cur_level
	obj_var* obj=Variables.findObjVar(name,ns,false);

	if(obj==NULL)
		obj=Variables.findObjVar(name,ns,true);

	if(obj->setter)
	{
		//Call the setter
		LOG(LOG_CALLS,"Calling the setter");

		IFunction* setter=obj->setter->getOverride();
		incRef();
		//One argument can be passed without creating an array
		ASObject* ret=setter->call(this,&o,1);
		assert_and_throw(ret==NULL);
		LOG(LOG_CALLS,"End of setter");
	}
	else
	{
		assert_and_throw(!obj->getter);
		if(obj->var)
			obj->var->decRef();
		obj->var=o;
	}
	check();
}

void variables_map::killObjVar(const multiname& mname)
{
	tiny_string name;
	switch(mname.name_type)
	{
		case multiname::NAME_INT:
			name=tiny_string(mname.name_i);
			break;
		case multiname::NAME_NUMBER:
			name=tiny_string(mname.name_d);
			break;
		case multiname::NAME_STRING:
			name=mname.name_s;
			break;
		default:
			assert_and_throw("Unexpected name kind" && false);
	}


	const pair<var_iterator, var_iterator> ret=Variables.equal_range(name);
	assert_and_throw(ret.first!=ret.second);

	//Find the namespace
	assert_and_throw(!mname.ns.empty());
	for(unsigned int i=0;i<mname.ns.size();i++)
	{
		const tiny_string& ns=mname.ns[i].name;
		var_iterator start=ret.first;
		for(;start!=ret.second;start++)
		{
			if(start->second.first==ns)
			{
				Variables.erase(start);
				return;
			}
		}
	}

	throw RunTimeException("Variable to kill not found");
}

obj_var* variables_map::findObjVar(const multiname& mname, bool create)
{
	tiny_string name;
	switch(mname.name_type)
	{
		case multiname::NAME_INT:
			name=tiny_string(mname.name_i);
			break;
		case multiname::NAME_NUMBER:
			name=tiny_string(mname.name_d);
			break;
		case multiname::NAME_STRING:
			name=mname.name_s;
			break;
		case multiname::NAME_OBJECT:
			name=mname.name_o->toString();
			break;
		default:
			assert_and_throw("Unexpected name kind" && false);
	}

	const var_iterator ret_begin=Variables.lower_bound(name);
	//This actually look for the first different name, if we accept also previous levels
	//Otherwise we are just doing equal_range
	const var_iterator ret_end=Variables.upper_bound(name);

	var_iterator ret=ret_begin;
	for(;ret!=ret_end;ret++)
	{
		//Check if one the namespace is already present
		assert_and_throw(!mname.ns.empty());
		//We can use binary search, as the namespace are ordered
		if(binary_search(mname.ns.begin(),mname.ns.end(),ret->second.first))
			return &ret->second.second;
	}

	//Name not present, insert it, if the multiname has a single ns and if we have to insert it
	//TODO: HACK: this is needed if the property should be present but it's not
	if(create)
	{
		if(mname.ns.size()>1)
		{
			//Hack, insert with empty name
			//Here the object MUST exist
			var_iterator inserted=Variables.insert(ret,make_pair(name, make_pair("", obj_var() ) ) );
			return &inserted->second.second;
		}
		var_iterator inserted=Variables.insert(ret,make_pair(name, make_pair(mname.ns[0].name, obj_var() ) ) );
		return &inserted->second.second;
	}
	else
		return NULL;
}

ASFUNCTIONBODY(ASObject,generator)
{
	//By default we assume it's a passtrough cast
	assert_and_throw(argslen==1);
	LOG(LOG_CALLS,"Passthorugh of " << args[0]);
	args[0]->incRef();
	return args[0];
}

ASFUNCTIONBODY(ASObject,_toString)
{
	return Class<ASString>::getInstanceS(obj->toString());
}

ASFUNCTIONBODY(ASObject,hasOwnProperty)
{
	assert_and_throw(argslen==1);
	bool ret=obj->hasPropertyByQName(args[0]->toString(),"");
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


void ASObject::initSlot(unsigned int n,const tiny_string& name, const tiny_string& ns)
{
	//Should be correct to use the level on the prototype chain
#ifndef NDEBUG
	assert(!initialized);
#endif
	Variables.initSlot(n,name,ns);
}

ASObject* ASObject::getVariableByString(const std::string& name)
{
	ASObject* ret=Variables.getVariableByString(name);
	return ret;
}

//In all the getter function we first ask the interface, so that special handling (e.g. Array)
//can be done
intptr_t ASObject::getVariableByMultiname_i(const multiname& name)
{
	check();

	ASObject* ret=getVariableByMultiname(name);
	assert_and_throw(ret);
	return ret->toInt();
}

obj_var* ASObject::findGettable(const multiname& name)
{
	obj_var* ret=Variables.findObjVar(name,false);
	if(ret)
	{
		//It seems valid for a class to redefine only the setter, so if we can't find
		//something to get, it's ok
		if(!(ret->getter || ret->var))
			ret=NULL;
	}
	return ret;
}

ASObject* ASObject::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride, ASObject* base)
{
	check();

	obj_var* obj=findGettable(name);

	if(obj!=NULL)
	{
		if(obj->getter)
		{
			//Call the getter
			ASObject* target=(base)?base:this;
			if(target->prototype)
			{
				LOG(LOG_CALLS,"Calling the getter on type " << target->prototype->class_name);
			}
			else
			{
				LOG(LOG_CALLS,"Calling the getter");
			}
			IFunction* getter=obj->getter;
			if(enableOverride)
				getter=getter->getOverride();
			target->incRef();
			ASObject* ret=getter->call(target,NULL,0);
			LOG(LOG_CALLS,"End of getter");
			assert_and_throw(ret);
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
	else if(prototype && getActualPrototype())
	{
		//It has not been found yet, ask the prototype
		ASObject* ret=getActualPrototype()->getVariableByMultiname(name,skip_impl,true,this);
		return ret;
	}

	//If it has not been found
	return NULL;
}

ASObject* ASObject::getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl)
{
	check();

	obj_var* obj=NULL;
	obj=Variables.findObjVar(name,ns,false);

	if(obj!=NULL)
	{
		if(obj->getter)
		{
			//Call the getter
			LOG(LOG_CALLS,"Calling the getter");
			IFunction* getter=obj->getter->getOverride();
			incRef();
			ASObject* ret=getter->call(this,NULL,0);
			LOG(LOG_CALLS,"End of getter");
			//The variable is already owned by the caller
			ret->fake_decRef();
			return ret;
		}
		else
			return obj->var;
	}
	else if(prototype)
	{
		ASObject* ret=prototype->getVariableByQName(name,ns);
		if(ret)
			return ret;
	}

	return NULL;
}

ASObject* variables_map::getVariableByString(const std::string& name)
{
	//Slow linear lookup, should be avoided
	var_iterator it=Variables.begin();
	for(;it!=Variables.end();it++)
	{
		string cur(it->second.first.raw_buf());
		if(!cur.empty())
			cur+='.';
		cur+=it->first.raw_buf();
		if(cur==name)
		{
			if(it->second.second.getter)
				throw UnsupportedException("Getters are not supported in getVariableByString"); 
			return it->second.second.var;
		}
	}
	
	return NULL;
}

void ASObject::check() const
{
	//Put here a bunch of safety check on the object
	assert_and_throw(ref_count>0);
	//Heavyweight stuff
#ifdef EXPENSIVE_DEBUG
	variables_map::const_var_iterator it=Variables.Variables.begin();
	for(;it!=Variables.Variables.end();it++)
	{
		variables_map::const_var_iterator next=it;
		next++;
		if(next==Variables.Variables.end())
			break;

		//No double definition of a single variable should exist
		if(it->first==next->first && it->second.first==next->second.first)
		{
			if(it->second.second.var==NULL && next->second.second.var==NULL)
				continue;

			if(it->second.second.var==NULL || next->second.second.var==NULL)
			{
				cout << it->first << endl;
				cout << it->second.second.var << ' ' << it->second.second.setter << ' ' << it->second.second.getter << endl;
				cout << next->second.second.var << ' ' << next->second.second.setter << ' ' << next->second.second.getter << endl;
				abort();
			}

			if(it->second.second.var->getObjectType()!=T_FUNCTION || next->second.second.var->getObjectType()!=T_FUNCTION)
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
	for(;it!=Variables.end();it++)
		LOG(LOG_NO_INFO,"[" << it->second.first << "] "<< it->first << " " << 
			it->second.second.var << ' ' << it->second.second.setter << ' ' << it->second.second.getter);
}

variables_map::~variables_map()
{
	destroyContents();
}

void variables_map::destroyContents()
{
	if(sys->finalizingDestruction) //Objects are being destroyed by the relative classes
		return;
	var_iterator it=Variables.begin();
	for(;it!=Variables.end();it++)
	{
		if(it->second.second.var)
			it->second.second.var->decRef();
		if(it->second.second.setter)
			it->second.second.setter->decRef();
		if(it->second.second.getter)
			it->second.second.getter->decRef();
	}
	Variables.clear();
}

ASObject::ASObject(Manager* m):type(T_OBJECT),ref_count(1),manager(m),cur_level(0),prototype(NULL),implEnable(true)
{
#ifndef NDEBUG
	//Stuff only used in debugging
	initialized=false;
#endif
}

ASObject::ASObject(const ASObject& o):type(o.type),ref_count(1),manager(NULL),cur_level(0),prototype(o.prototype),implEnable(true)
{
	if(prototype)
		prototype->incRef();

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
	}
}

ASObject::~ASObject()
{
	if(prototype && !sys->finalizingDestruction)
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
		assert_and_throw(type==T_CLASS);
		return NULL;
	}

	for(int i=prototype->max_level;i>cur_level;i--)
		ret=ret->super;

	assert_and_throw(ret);
	assert_and_throw(ret->max_level==cur_level);
	return ret;
}

void variables_map::initSlot(unsigned int n, const tiny_string& name, const tiny_string& ns)
{
	if(n>slots_vars.size())
		slots_vars.resize(n,Variables.end());

	pair<var_iterator, var_iterator> ret=Variables.equal_range(name);
	if(ret.first!=ret.second)
	{
		//Check if this namespace is already present
		var_iterator start=ret.first;
		for(;start!=ret.second;start++)
		{
			if(start->second.first==ns)
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
		if(slots_vars[n-1]->second.second.setter)
			throw UnsupportedException("setSlot has setters");
		slots_vars[n-1]->second.second.var->decRef();
		slots_vars[n-1]->second.second.var=o;
	}
	else
		throw RunTimeException("setSlot out of bounds");
}

obj_var* variables_map::getValueAt(unsigned int index)
{
	//TODO: CHECK behavious on overridden methods
	if(index<Variables.size())
	{
		var_iterator it=Variables.begin();

		for(unsigned int i=0;i<index;i++)
			it++;

		return &it->second.second;
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
		LOG(LOG_CALLS,"Calling the getter");
		IFunction* getter=obj->getter->getOverride();
		incRef();
		ret=getter->call(this,NULL,0);
		ret->fake_decRef();
		LOG(LOG_CALLS,"End of getter");
	}
	else
		ret=obj->var;

	return ret;
}

tiny_string variables_map::getNameAt(unsigned int index)
{
	//TODO: CHECK behaviour on overridden methods
	if(index<Variables.size())
	{
		var_iterator it=Variables.begin();

		for(unsigned int i=0;i<index;i++)
			it++;

		return it->first;
	}
	else
		throw RunTimeException("getNameAt out of bounds");
}

unsigned int ASObject::numVariables()
{
	return Variables.size();
}

