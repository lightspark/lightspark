/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#define GL_GLEXT_PROTOTYPES
#include "abc.h"
#include "swftypes.h"
#include "tags.h"
#include "logger.h"
#include "actions.h"
#include <string.h>
#include <algorithm>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "swf.h"
#include "geometry.h"
#include "class.h"

using namespace std;
using namespace lightspark;

extern TLSDATA SystemState* sys;
extern TLSDATA RenderThread* rt;
extern TLSDATA ParseThread* pt;
extern TLSDATA Manager* iManager;
extern TLSDATA Manager* dManager;

tiny_string ASObject::toString(bool debugMsg)
{
	assert(ref_count>0);
	if(implementation)
	{
		tiny_string ret;
		assert(prototype);
		//When in an internal debug msg, do not print complex objects
		if(debugMsg)
		{
			ret="[object ";
			ret+=prototype->class_name;
			ret+="]";
			return ret;
		}
		if(implementation->toString(ret))
			return ret;
	}
	cout << "Cannot convert object of type " << getObjectType() << " to String" << endl;
	if(debugMsg==false)
		assert(!hasPropertyByQName("toString",""));
	return "[object Object]";
}

bool ASObject::isGreater(ASObject* r)
{
	assert(ref_count>0);
	if(implementation)
	{
		bool ret;
		if(implementation->isGreater(ret,r))
			return ret;
	}
	LOG(LOG_NOT_IMPLEMENTED,"Greater than comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	if(prototype)
		LOG(LOG_NOT_IMPLEMENTED,"Type " << prototype->class_name);
	abort();
	return false;
}

bool ASObject::isLess(ASObject* r)
{
	assert(ref_count>0);
	if(implementation)
	{
		bool ret;
		if(implementation->isLess(ret,r))
			return ret;
	}

	//We can try to call valueOf and compare that
	objAndLevel obj1=getVariableByQName("valueOf","");
	objAndLevel obj2=r->getVariableByQName("valueOf","");
	if(obj1.obj==NULL || obj2.obj==NULL)
	{
		LOG(LOG_NOT_IMPLEMENTED,"Less than comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
		abort();
	}

	assert(obj1.obj->getObjectType()==T_FUNCTION && obj2.obj->getObjectType()==T_FUNCTION);

	IFunction* f1=static_cast<IFunction*>(obj1.obj);
	IFunction* f2=static_cast<IFunction*>(obj2.obj);

	ASObject* ret1=f1->call(this,NULL,obj1.level);
	ASObject* ret2=f2->call(r,NULL,obj2.level);

	return ret1->isLess(ret2);
}

void ASObject::acquireInterface(IInterface* i)
{
	assert(ref_count>0);
	assert(i!=implementation);
	delete implementation;
	implementation=i;
	implementation->obj=this;
}

SWFOBJECT_TYPE ASObject::getObjectType() const
{
	return (implementation)?(implementation->type):type;
}

IInterface::IInterface(const IInterface& r):type(r.type),obj(NULL)
{
	if(r.obj)
	{
		abort();
		//An IInterface derived class can only be cloned if the obj is a Class
		assert(r.obj->getObjectType()==T_CLASS);
		Class_base* c=static_cast<Class_base*>(r.obj);
		obj=new ASObject;
		obj->prototype=c;
		obj->actualPrototype=c;
		c->incRef();
		obj->implementation=this;
	}
}

bool IInterface::isGreater(bool& ret, ASObject* r)
{
	return false;
}

bool IInterface::isLess(bool& ret, ASObject* r)
{
	return false;
}

bool IInterface::isEqual(bool& ret, ASObject* r)
{
	return false;
}

bool IInterface::toNumber(double& ret)
{
	return false;
}

bool IInterface::toInt(int& ret)
{
	return false;
}

bool IInterface::toString(tiny_string& ret)
{
	return false;
}

bool IInterface::getVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject*& out)
{
	return false;
}

bool IInterface::getVariableByMultiname(const multiname& name, ASObject*& out)
{
	return false;
}

bool IInterface::getVariableByMultiname_i(const multiname& name, intptr_t& out)
{
	return false;
}

bool IInterface::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o)
{
	return false;
}

bool IInterface::deleteVariableByMultiname(const multiname& name)
{
	return false;
}

bool IInterface::setVariableByMultiname(const multiname& name, ASObject* o)
{
	return false;
}

bool IInterface::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	return false;
}

bool IInterface::hasNext(int& index, bool& out)
{
	return false;
}

bool IInterface::nextName(int index, ASObject*& out)
{
	return false;
}

bool IInterface::nextValue(int index, ASObject*& out)
{
	return false;
}

void IInterface::buildTraits(ASObject* o)
{
	assert(o->actualPrototype);
	if(o->actualPrototype->class_name!="IInterface")
		LOG(LOG_NOT_IMPLEMENTED,"Add buildTraits for class " << o->actualPrototype->class_name);
}

tiny_string multiname::qualifiedString() const
{
	assert(ns.size()==1);
	assert(name_type==NAME_STRING);
	//TODO: what if the ns is empty
	if(false && ns[0].name=="")
		return name_s;
	else
	{
		tiny_string ret=ns[0].name;
		ret+="::";
		ret+=name_s;
		return ret;
	}
}

bool Integer::isGreater(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return val > i->toInt();
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		const Number* d=static_cast<const Number*>(o);
		return val > d->toNumber();
	}
	else
	{
		return ASObject::isGreater(o);
	}
}

bool Integer::isLess(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
	{
		const Integer* i=static_cast<const Integer*>(o);
		return val < i->toInt();
	}
	else if(o->getObjectType()==T_NUMBER)
	{
		const Number* i=static_cast<const Number*>(o);
		return val < i->toNumber();
	}
	else
		return ASObject::isLess(o);
}

bool Integer::isEqual(ASObject* o)
{
	if(o->getObjectType()==T_INTEGER)
		return val==o->toInt();
	else if(o->getObjectType()==T_NUMBER)
		return val==o->toInt();
	else
	{
		return ASObject::isEqual(o);
	}
}

bool ASObject::isEqual(ASObject* r)
{
	assert(ref_count>0);
	if(implementation)
	{
		bool ret;
		if(implementation->isEqual(ret,r))
			return ret;
	}

	//if we are comparing the same object the answer is true
	if(this==r)
		return true;

	if(r->getObjectType()==T_NULL || r->getObjectType()==T_UNDEFINED)
		return false;

	//We can try to call valueOf (maybe equals) and compare that
	objAndLevel obj1=getVariableByQName("valueOf","");
	objAndLevel obj2=r->getVariableByQName("valueOf","");
	if(obj1.obj!=NULL && obj2.obj!=NULL)
	{
		assert(obj1.obj->getObjectType()==T_FUNCTION && obj2.obj->getObjectType()==T_FUNCTION);
		IFunction* f1=static_cast<IFunction*>(obj1.obj);
		IFunction* f2=static_cast<IFunction*>(obj2.obj);

		ASObject* ret1=f1->call(this,NULL,obj1.level);
		ASObject* ret2=f2->call(r,NULL,obj2.level);

		LOG(LOG_CALLS,"Overloaded isEqual");
		return ret1->isEqual(ret2);
	}

	LOG(LOG_NOT_IMPLEMENTED,"Equal comparison between type "<<getObjectType()<< " and type " << r->getObjectType());
	if(prototype)
		LOG(LOG_NOT_IMPLEMENTED,"Type " << prototype->class_name);
	return false;
}

unsigned int ASObject::toUInt() const
{
	return toInt();
}

int ASObject::toInt() const
{
	if(implementation)
	{
		int ret;
		if(implementation->toInt(ret))
			return ret;
	}
	LOG(LOG_ERROR,"Cannot convert object of type " << getObjectType() << " to Int");
	cout << "imanager " << iManager->available.size() << endl;
	cout << "dmanager " << dManager->available.size() << endl;
	abort();
	return 0;
}

double ASObject::toNumber() const
{
	if(getObjectType()==T_UNDEFINED)
	{
		cout << "HACK: returning 0 from Undefined::toNumber" << endl;
		abort();
		return 0;
	}
	LOG(LOG_ERROR,"Cannot convert object of type " << getObjectType() << " to float");
	abort();
	return 0;
}

obj_var* variables_map::findObjVar(const tiny_string& n, const tiny_string& ns, int& level, bool create, bool searchPreviusLevels)
{
	nameAndLevel name(n,level);
	const var_iterator ret_begin=Variables.lower_bound(name);
	//This actually look for the first different name, if we accept also previous levels
	//Otherwise we are just doing equal_range
	if(searchPreviusLevels)
		name.level=0;
	const var_iterator ret_end=Variables.upper_bound(name);
	name.level=level;

	var_iterator ret=ret_begin;
	for(ret;ret!=ret_end;ret++)
	{
		if(ret->second.first==ns)
		{
			level=ret->first.level;
			return &ret->second.second;
		}
	}

	//Name not present, insert it if we have to create it
	if(create)
	{
		var_iterator inserted=Variables.insert(ret_begin,make_pair(nameAndLevel(n,level), make_pair(ns, obj_var() ) ) );
		return &inserted->second.second;
	}
	else
		return NULL;
}

bool ASObject::hasPropertyByQName(const tiny_string& name, const tiny_string& ns)
{
	assert(ref_count>0);

	//We look in all the object's levels
	int level=(prototype)?(prototype->max_level):0;
	return (Variables.findObjVar(name, ns, level, false, true)!=NULL);
}

bool ASObject::hasPropertyByMultiname(const multiname& name)
{
	assert(ref_count>0);

	//We look in all the object's levels
	int level=(prototype)?(prototype->max_level):0;
	return (Variables.findObjVar(name, level, false, true)!=NULL);
}

void ASObject::setGetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o)
{
	assert(ref_count>0);
	//Getters are inserted with the current level of the prototype chain
	int level=(actualPrototype)?actualPrototype->max_level:0;
	obj_var* obj=Variables.findObjVar(name,ns,level,true,false);
	if(obj->getter!=NULL)
	{
		assert(o==obj->getter);
		return;
	}
	obj->getter=o;
}

void ASObject::setSetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o)
{
	assert(ref_count>0);
	//Setters are inserted with the current level of the prototype chain
	int level=(actualPrototype)?actualPrototype->max_level:0;
	obj_var* obj=Variables.findObjVar(name,ns,level,true,false);
	if(obj->setter!=NULL)
	{
		assert(o==obj->setter);
		return;
	}
	obj->setter=o;
}

void ASObject::deleteVariableByMultiname(const multiname& name)
{
	assert(ref_count>0);
	if(implementation)
	{
		if(implementation->deleteVariableByMultiname(name))
			return;
	}

	//Find out if the variable is declared more than once
	obj_var* obj=NULL;
	int level;
	unsigned int count=0;
	//We search in every level
	int max_level=(prototype)?prototype->max_level:0;
	for(int i=max_level;i>=0;i--)
	{
		//We stick to the old iteration mode, as we need to count
		obj=Variables.findObjVar(name,max_level,false,false);
		if(obj)
		{
			count++;
			level=i;
		}
	}
	//if it's not present it's ok
	if(count==0)
		return;

	assert(count==1);

	//Now dereference the values
	//TODO: maybe we can look on the previous levels
	obj=Variables.findObjVar(name,level,false,false);
	if(obj->var)
		obj->var->decRef();
	if(obj->getter)
		obj->getter->decRef();
	if(obj->setter)
		obj->setter->decRef();

	//Now kill the variable
	Variables.killObjVar(name,level);
}

//In all setter we first pass the value to the interface to see if special handling is possible
void ASObject::setVariableByMultiname_i(const multiname& name, intptr_t value)
{
	check();
	if(implementation)
	{
		if(implementation->setVariableByMultiname_i(name,value))
			return;
	}

	setVariableByMultiname(name,abstract_i(value));
}

void ASObject::setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride)
{
	check();
	if(implementation)
	{
		if(implementation->setVariableByMultiname(name,o))
			return;
	}

	obj_var* obj=NULL;
	//It's always correct to use the current level for the object
	//NOTE: we assume that [gs]etSuper and [sg]etProperty correctly manipulate the cur_level
	int level=cur_level;
	obj=Variables.findObjVar(name,level,false,true);

	if(obj==NULL)
	{
		assert(level==cur_level);
		obj=Variables.findObjVar(name,level,true,false);
	}

	if(obj->setter)
	{
		//Call the setter
		LOG(LOG_CALLS,"Calling the setter");
		arguments args(1);
		args.set(0,o);
		//TODO: check
		o->incRef();
		//Overriding function is automatically done by using cur_level
		IFunction* setter=obj->setter;
		if(enableOverride)
			setter=setter->getOverride();
		setter->call(this,&args, level);
		LOG(LOG_CALLS,"End of setter");
	}
	else
	{
		if(obj->var)
			obj->var->decRef();
		obj->var=o;
	}
	check();
}

void ASObject::setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, bool find_back, bool skip_impl)
{
	check();
	if(implementation && !skip_impl)
	{
		if(implementation->setVariableByQName(name,ns,o))
			return;
	}

	obj_var* obj=NULL;
	//It's always correct to use the current level for the object
	//NOTE: we assume that [gs]etSuper and setProperty correctly manipulate the cur_level
	int level=cur_level;
	obj=Variables.findObjVar(name,ns,level,false,find_back);

	if(obj==NULL)
	{
		//When the var is not found level should not be modified
		assert(cur_level==level);
		obj=Variables.findObjVar(name,ns,level,true,false);
	}

	if(obj->setter)
	{
		//Call the setter
		LOG(LOG_CALLS,"Calling the setter");
		arguments args(1);
		args.set(0,o);
		//TODO: check
		o->incRef();
		IFunction* setter=obj->setter->getOverride();
		setter->call(this,&args, level);
		LOG(LOG_CALLS,"End of setter");
	}
	else
	{
		if(obj->var)
			obj->var->decRef();
		obj->var=o;
	}
}

void variables_map::killObjVar(const multiname& mname, int level)
{
	nameAndLevel name("",level);
	if(mname.name_type==multiname::NAME_INT)
		name.name=tiny_string(mname.name_i);
	else if(mname.name_type==multiname::NAME_NUMBER)
		name.name=tiny_string(mname.name_d);
	else if(mname.name_type==multiname::NAME_STRING)
		name.name=mname.name_s;
	else
		abort();

	const pair<var_iterator, var_iterator> ret=Variables.equal_range(name);
	assert(ret.first!=ret.second);

	//Find the namespace
	assert(!mname.ns.empty());
	for(int i=0;i<mname.ns.size();i++)
	{
		const tiny_string& ns=mname.ns[i].name;
		var_iterator start=ret.first;
		for(start;start!=ret.second;start++)
		{
			if(start->second.first==ns)
			{
				Variables.erase(start);
				return;
			}
		}
	}

	abort();
}

obj_var* variables_map::findObjVar(const multiname& mname, int& level, bool create, bool searchPreviusLevels)
{
	nameAndLevel name("",level);
	if(mname.name_type==multiname::NAME_INT)
		name.name=tiny_string(mname.name_i);
	else if(mname.name_type==multiname::NAME_NUMBER)
		name.name=tiny_string(mname.name_d);
	else if(mname.name_type==multiname::NAME_STRING)
		name.name=mname.name_s;
	else
		abort();

	const var_iterator ret_begin=Variables.lower_bound(name);
	//This actually look for the first different name, if we accept also previous levels
	//Otherwise we are just doing equal_range
	if(searchPreviusLevels)
		name.level=0;
	const var_iterator ret_end=Variables.upper_bound(name);
	name.level=level;

	var_iterator ret=ret_begin;
	for(ret;ret!=ret_end;ret++)
	{
		//Check if one the namespace is already present
		assert(!mname.ns.empty());
		//We can use binary search, as the namespace are ordered
		if(binary_search(mname.ns.begin(),mname.ns.end(),ret->second.first))
		{
			level=ret->first.level;
			return &ret->second.second;
		}
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

ASFUNCTIONBODY(ASObject,_toString)
{
	return Class<ASString>::getInstanceS(true,obj->toString())->obj;
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


void ASObject::initSlot(int n,const tiny_string& name, const tiny_string& ns)
{
	//Should be correct to use the level on the prototype chain
	int level=(actualPrototype)?actualPrototype->max_level:0;
	Variables.initSlot(n,level,name,ns);
}

//In all the getter function we first ask the interface, so that special handling (e.g. Array)
//can be done
intptr_t ASObject::getVariableByMultiname_i(const multiname& name)
{
	check();
	if(implementation)
	{
		intptr_t ret;
		if(implementation->getVariableByMultiname_i(name,ret))
			return ret;
	}

	ASObject* ret=getVariableByMultiname(name).obj;
	assert(ret);
	return ret->toInt();
}

objAndLevel ASObject::getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride)
{
	check();
	if(implementation && !skip_impl)
	{
		ASObject* ret;
		//It seems that various kind of implementation works only with the empty namespace
		assert(name.ns.size()>0);
		if(name.ns[0].name=="" && implementation->getVariableByMultiname(name,ret))
		{
			//TODO check
			assert(prototype);
			return objAndLevel(ret, cur_level);
		}
	}

	obj_var* obj=NULL;
	int level;
	int max_level=cur_level;
	for(int i=max_level;i>=0;i--)
	{
		//The variable i is automatically moved to the right level
		obj=Variables.findObjVar(name,i,false,true);
		if(obj)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, just go to the previous level
			if(obj->getter || obj->var)
			{
				level=i;
				break;
			}
		}
	}

	if(obj!=NULL)
	{
		if(obj->getter)
		{
			//Call the getter
			if(prototype)
			{
				LOG(LOG_CALLS,"Calling the getter on type " << prototype->class_name);
			}
			else
			{
				LOG(LOG_CALLS,"Calling the getter");
			}
			IFunction* getter=obj->getter;
			if(enableOverride)
				getter=getter->getOverride();
			ASObject* ret=getter->call(this,NULL,level);
			LOG(LOG_CALLS,"End of getter");
			assert(ret);
			//The returned value is already owned by the caller
			ret->fake_decRef();
			//TODO: check
			return objAndLevel(ret,level);
		}
		else
		{
			assert(obj->var);
			return objAndLevel(obj->var,level);
		}
	}
	else
	{
		//Check if we should do lazy definition
		if(name.name_s=="toString")
		{
			ASObject* ret=new Function(ASObject::_toString);
			setVariableByQName("toString","",ret);
			//Added at level 0, as Object is always the base
			return objAndLevel(ret,0);
		}
		else if(getObjectType()==T_FUNCTION && name.name_s=="call")
		{
			//Fake returning the function itself
			return objAndLevel(this,0);
		}

		//It has not been found yet, ask the prototype
		if(prototype)
			return prototype->getVariableByMultiname(name,skip_impl);
	}

	//If it has not been found
	return objAndLevel(NULL,0);
}

objAndLevel ASObject::getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl)
{
	check();

	if(implementation && !skip_impl)
	{
		ASObject* ret;
		assert(prototype);
		if(implementation->getVariableByQName(name,ns,ret))
		{
			assert(prototype);
			return objAndLevel(ret, cur_level);
		}
	}

	obj_var* obj=NULL;
	int level=cur_level;
	obj=Variables.findObjVar(name,ns,level,false,true);

	if(obj!=NULL)
	{
		if(obj->getter)
		{
			//Call the getter
			LOG(LOG_CALLS,"Calling the getter");
			IFunction* getter=obj->getter->getOverride();
			ASObject* ret=getter->call(this,NULL,level);
			LOG(LOG_CALLS,"End of getter");
			//The variable is already owned by the caller
			ret->fake_decRef();
			return objAndLevel(ret,level);
		}
		else
			return objAndLevel(obj->var,level);
	}
	else if(prototype)
	{
		objAndLevel ret=prototype->getVariableByQName(name,ns);
		if(ret.obj)
			return ret;
	}

	return objAndLevel(NULL,0);
}

ASObject* variables_map::getVariableByString(const std::string& name)
{
	//Slow linear lookup, should be avoided
	var_iterator it=Variables.begin();
	for(it;it!=Variables.end();it++)
	{
		string cur(it->second.first.raw_buf());
		if(!cur.empty())
			cur+='.';
		cur+=it->first.name.raw_buf();
		if(cur==name)
		{
			if(it->second.second.getter)
				abort();
			return it->second.second.var;
		}
	}
	
	return NULL;
}

std::ostream& lightspark::operator<<(std::ostream& s, const tiny_string& r)
{
	s << r.buf;
	return s;
}

std::ostream& lightspark::operator<<(std::ostream& s, const multiname& r)
{
	for(int i=0;i<r.ns.size();i++)
	{
		string prefix;
		switch(r.ns[i].kind)
		{
			case 0x08:
				prefix="ns:";
				break;
			case 0x16:
				prefix="pakns:";
				break;
			case 0x17:
				prefix="pakintns:";
				break;
			case 0x18:
				prefix="protns:";
				break;
			case 0x19:
				prefix="explns:";
				break;
			case 0x1a:
				prefix="staticprotns:";
				break;
			case 0x05:
				prefix="privns:";
				break;
		}
		s << '[' << prefix << r.ns[i].name << "] ";
	}
	if(r.name_type==multiname::NAME_INT)
		s << r.name_i;
	else if(r.name_type==multiname::NAME_NUMBER)
		s << r.name_d;
	else if(r.name_type==multiname::NAME_STRING)
		s << r.name_s;
	else
		s << r.name_o; //We print the hexadecimal value
	return s;
}

void ASObject::check()
{
	//Put here a bunch of safety check on the object
	assert(ref_count>0);
	//Heavyweight stuff
#if 1
	variables_map::var_iterator it=Variables.Variables.begin();
	for(it;it!=Variables.Variables.end();it++)
	{
		variables_map::var_iterator next=it;
		next++;
		if(next==Variables.Variables.end())
			break;

		//No double definition of a single variable should exist
		if(it->first.name==next->first.name && it->second.first==next->second.first)
		{
			if(it->second.second.var==NULL && next->second.second.var==NULL)
				continue;

			if(it->second.second.var==NULL || next->second.second.var==NULL)
			{
				cout << it->first.name << endl;
				cout << it->second.second.var << ' ' << it->second.second.setter << ' ' << it->second.second.getter << endl;
				cout << next->second.second.var << ' ' << next->second.second.setter << ' ' << next->second.second.getter << endl;
				abort();
			}

			if(it->second.second.var->getObjectType()!=T_FUNCTION || next->second.second.var->getObjectType()!=T_FUNCTION)
			{
				cout << it->first.name << endl;
				abort();
			}
		}
	}

#endif
}

void variables_map::dumpVariables()
{
	var_iterator it=Variables.begin();
	for(it;it!=Variables.end();it++)
		cerr << it->first.level << ": [" << it->second.first << "] "<< it->first.name << " " << 
			it->second.second.var << ' ' << it->second.second.setter << ' ' << it->second.second.getter << endl;
}

lightspark::RECT::RECT()
{
}

std::ostream& lightspark::operator<<(std::ostream& s, const RECT& r)
{
	s << '{' << (int)r.Xmin << ',' << r.Xmax << ',' << r.Ymin << ',' << r.Ymax << '}';
	return s;
}

ostream& lightspark::operator<<(ostream& s, const STRING& t)
{
	for(unsigned int i=0;i<t.String.size();i++)
		s << t.String[i];
	return s;
}

std::ostream& operator<<(std::ostream& s, const RGBA& r)
{
	s << "RGBA <" << (int)r.Red << ',' << (int)r.Green << ',' << (int)r.Blue << ',' << (int)r.Alpha << '>';
	return s;
}

std::ostream& operator<<(std::ostream& s, const RGB& r)
{
	s << "RGB <" << (int)r.Red << ',' << (int)r.Green << ',' << (int)r.Blue << '>';
	return s;
}

void MATRIX::get4DMatrix(float matrix[16])
{
	memset(matrix,0,sizeof(float)*16);
	matrix[0]=ScaleX;
	matrix[1]=RotateSkew0;

	matrix[4]=RotateSkew1;
	matrix[5]=ScaleY;

	matrix[10]=1;

	matrix[12]=TranslateX;
	matrix[13]=TranslateY;
	matrix[15]=1;
}

void MATRIX::multiply2D(number_t xin, number_t yin, number_t& xout, number_t& yout)
{
	xout=xin*ScaleX + yin*RotateSkew1 + TranslateX;
	yout=xin*RotateSkew0 + yin*ScaleY + TranslateY;
}

void MATRIX::getTranslation(int& x, int& y)
{
	x=TranslateX;
	y=TranslateY;
}

std::ostream& operator<<(std::ostream& s, const MATRIX& r)
{
	s << "| " << r.ScaleX << ' ' << r.RotateSkew0 << " |" << std::endl;
	s << "| " << r.RotateSkew1 << ' ' << r.ScaleY << " |" << std::endl;
	s << "| " << (int)r.TranslateX << ' ' << (int)r.TranslateY << " |" << std::endl;
	return s;
}

std::istream& lightspark::operator>>(std::istream& stream, STRING& v)
{
	v.String.clear();
	UI8 c;
	do
	{
		stream >> c;
		if(c==0)
			break;
		v.String.push_back(c);
	}
	while(c!=0);
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, RECT& v)
{
	BitStream s(stream);
	v.NBits=UB(5,s);
	v.Xmin=SB(v.NBits,s);
	v.Xmax=SB(v.NBits,s);
	v.Ymin=SB(v.NBits,s);
	v.Ymax=SB(v.NBits,s);
	return stream;
}

std::istream& lightspark::operator>>(std::istream& s, RGB& v)
{
	s >> v.Red >> v.Green >> v.Blue;
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, RGBA& v)
{
	s >> v.Red >> v.Green >> v.Blue >> v.Alpha;
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, LINESTYLEARRAY& v)
{
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		LOG(LOG_ERROR,"Line array extended not supported");
	if(v.version<4)
	{
		v.LineStyles=new LINESTYLE[v.LineStyleCount];
		for(int i=0;i<v.LineStyleCount;i++)
		{
			v.LineStyles[i].version=v.version;
			s >> v.LineStyles[i];
		}
	}
	else
	{
		v.LineStyles2=new LINESTYLE2[v.LineStyleCount];
		for(int i=0;i<v.LineStyleCount;i++)
			s >> v.LineStyles2[i];
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, MORPHLINESTYLEARRAY& v)
{
	s >> v.LineStyleCount;
	if(v.LineStyleCount==0xff)
		LOG(LOG_ERROR,"Line array extended not supported");
	v.LineStyles=new MORPHLINESTYLE[v.LineStyleCount];
	for(int i=0;i<v.LineStyleCount;i++)
	{
		s >> v.LineStyles[i];
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, FILLSTYLEARRAY& v)
{
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		LOG(LOG_ERROR,"Fill array extended not supported");
	v.FillStyles=new FILLSTYLE[v.FillStyleCount];
	for(int i=0;i<v.FillStyleCount;i++)
	{
		v.FillStyles[i].version=v.version;
		s >> v.FillStyles[i];
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, MORPHFILLSTYLEARRAY& v)
{
	s >> v.FillStyleCount;
	if(v.FillStyleCount==0xff)
		LOG(LOG_ERROR,"Fill array extended not supported");
	v.FillStyles=new MORPHFILLSTYLE[v.FillStyleCount];
	for(int i=0;i<v.FillStyleCount;i++)
	{
		s >> v.FillStyles[i];
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, SHAPE& v)
{
	BitStream bs(s);
	v.NumFillBits=UB(4,bs);
	v.NumLineBits=UB(4,bs);
	v.ShapeRecords=SHAPERECORD(&v,bs);
	if(v.ShapeRecords.TypeFlag + v.ShapeRecords.StateNewStyles+v.ShapeRecords.StateLineStyle+v.ShapeRecords.StateFillStyle1+
			v.ShapeRecords.StateFillStyle0+v.ShapeRecords.StateMoveTo)
	{
		SHAPERECORD* cur=&(v.ShapeRecords);
		while(1)
		{
			cur->next=new SHAPERECORD(&v,bs);
			cur=cur->next;
			if(cur->TypeFlag+cur->StateNewStyles+cur->StateLineStyle+cur->StateFillStyle1+cur->StateFillStyle0+cur->StateMoveTo==0)
				break;
		}
	}
	return s;
}

std::istream& lightspark::operator>>(std::istream& s, SHAPEWITHSTYLE& v)
{
	v.FillStyles.version=v.version;
	v.LineStyles.version=v.version;
	s >> v.FillStyles >> v.LineStyles;
	BitStream bs(s);
	v.NumFillBits=UB(4,bs);
	v.NumLineBits=UB(4,bs);
	v.ShapeRecords=SHAPERECORD(&v,bs);
	if(v.ShapeRecords.TypeFlag+v.ShapeRecords.StateNewStyles+v.ShapeRecords.StateLineStyle+v.ShapeRecords.StateFillStyle1+
			v.ShapeRecords.StateFillStyle0+v.ShapeRecords.StateMoveTo)
	{
		SHAPERECORD* cur=&(v.ShapeRecords);
		while(1)
		{
			cur->next=new SHAPERECORD(&v,bs);
			cur=cur->next;
			if(cur->TypeFlag+cur->StateNewStyles+cur->StateLineStyle+cur->StateFillStyle1+cur->StateFillStyle0+cur->StateMoveTo==0)
				break;
		}
	}
	return s;
}

istream& lightspark::operator>>(istream& s, LINESTYLE2& v)
{
	s >> v.Width;
	BitStream bs(s);
	v.StartCapStyle=UB(2,bs);
	v.JointStyle=UB(2,bs);
	v.HasFillFlag=UB(1,bs);
	v.NoHScaleFlag=UB(1,bs);
	v.NoVScaleFlag=UB(1,bs);
	v.PixelHintingFlag=UB(1,bs);
	UB(5,bs);
	v.NoClose=UB(1,bs);
	v.EndCapStyle=UB(2,bs);
	if(v.JointStyle==2)
		s >> v.MiterLimitFactor;
	if(v.HasFillFlag)
		s >> v.FillType;
	else
		s >> v.Color;

	return s;
}

istream& lightspark::operator>>(istream& s, LINESTYLE& v)
{
	s >> v.Width;
	if(v.version==2 || v.version==1)
	{
		RGB tmp;
		s >> tmp;
		v.Color=tmp;
	}
	else
		s >> v.Color;
	return s;
}

istream& lightspark::operator>>(istream& s, MORPHLINESTYLE& v)
{
	s >> v.StartWidth >> v.EndWidth >> v.StartColor >> v.EndColor;
	return s;
}

std::istream& lightspark::operator>>(std::istream& in, TEXTRECORD& v)
{
	BitStream bs(in);
	v.TextRecordType=UB(1,bs);
	v.StyleFlagsReserved=UB(3,bs);
	if(v.StyleFlagsReserved)
		LOG(LOG_ERROR,"Reserved bits not so reserved");
	v.StyleFlagsHasFont=UB(1,bs);
	v.StyleFlagsHasColor=UB(1,bs);
	v.StyleFlagsHasYOffset=UB(1,bs);
	v.StyleFlagsHasXOffset=UB(1,bs);
	if(!v.TextRecordType)
		return in;
	if(v.StyleFlagsHasFont)
		in >> v.FontID;
	if(v.StyleFlagsHasColor)
	{
		RGB t;
		in >> t;
		v.TextColor=t;
	}
	if(v.StyleFlagsHasXOffset)
		in >> v.XOffset;
	if(v.StyleFlagsHasYOffset)
		in >> v.YOffset;
	if(v.StyleFlagsHasFont)
		in >> v.TextHeight;
	in >> v.GlyphCount;
	v.GlyphEntries.clear();
	for(int i=0;i<v.GlyphCount;i++)
	{
		v.GlyphEntries.push_back(GLYPHENTRY(&v,bs));
	}

	return in;
}

std::istream& lightspark::operator>>(std::istream& s, GRADRECORD& v)
{
	s >> v.Ratio;
	if(v.version==1 || v.version==2)
	{
		RGB tmp;
		s >> tmp;
		v.Color=tmp;
	}
	else
		s >> v.Color;

	return s;
}

std::istream& lightspark::operator>>(std::istream& s, GRADIENT& v)
{
	BitStream bs(s);
	v.SpreadMode=UB(2,bs);
	v.InterpolationMode=UB(2,bs);
	v.NumGradient=UB(4,bs);
	GRADRECORD gr;
	gr.version=v.version;
	for(int i=0;i<v.NumGradient;i++)
	{
		s >> gr;
		v.GradientRecords.push_back(gr);
	}
	sort(v.GradientRecords.begin(),v.GradientRecords.end());
	return s;
}

inline RGBA medianColor(const RGBA& a, const RGBA& b, float factor)
{
	return RGBA(a.Red+(b.Red-a.Red)*factor,
		a.Green+(b.Green-a.Green)*factor,
		a.Blue+(b.Blue-a.Blue)*factor,
		a.Alpha+(b.Alpha-a.Alpha)*factor);
}

void FILLSTYLE::setVertexData(arrayElem* elem)
{
	if(FillStyleType==0x00)
	{
		//LOG(TRACE,"Fill color");
		elem->colors[0]=1;
		elem->colors[1]=0;
		elem->colors[2]=0;

		elem->texcoord[0]=float(Color.Red)/256.0f;
		elem->texcoord[1]=float(Color.Green)/256.0f;
		elem->texcoord[2]=float(Color.Blue)/256.0f;
		elem->texcoord[3]=float(Color.Alpha)/256.0f;
	}
	else if(FillStyleType==0x10)
	{
		//LOG(TRACE,"Fill gradient");
		elem->colors[0]=0;
		elem->colors[1]=1;
		elem->colors[2]=0;

		/*color_entry buffer[256];
		int grad_index=0;
		RGBA color_l(0,0,0,1);
		int index_l=0;
		RGBA color_r(Gradient.GradientRecords[0].Color);
		int index_r=Gradient.GradientRecords[0].Ratio;

		for(int i=0;i<256;i++)
		{
			float dist=i-index_l;
			dist/=(index_r-index_l);
			RGBA c=medianColor(color_l,color_r,dist);
			buffer[i].r=float(c.Red)/256.0f;
			buffer[i].g=float(c.Green)/256.0f;
			buffer[i].b=float(c.Blue)/256.0f;
			buffer[i].a=1;

			if(Gradient.GradientRecords[grad_index].Ratio==i)
			{
				grad_index++;
				color_l=color_r;
				index_l=index_r;
				color_r=Gradient.GradientRecords[grad_index].Color;
				index_r=Gradient.GradientRecords[grad_index].Ratio;
			}
		}

		glBindTexture(GL_TEXTURE_2D,rt->data_tex);
		glTexImage2D(GL_TEXTURE_2D,0,4,256,1,0,GL_RGBA,GL_FLOAT,buffer);*/
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"Reverting to fixed function");
		elem->colors[0]=1;
		elem->colors[1]=0;
		elem->colors[2]=0;

		elem->texcoord[0]=0.5;
		elem->texcoord[1]=0.5;
		elem->texcoord[2]=0;
		elem->texcoord[3]=1;
	}
}

void FILLSTYLE::setFragmentProgram() const
{
	//Let's abuse of glColor and glTexCoord to transport
	//custom information
	struct color_entry
	{
		float r,g,b,a;
	};

	glBindTexture(GL_TEXTURE_2D,rt->data_tex);

	if(FillStyleType==0x00)
	{
		//LOG(TRACE,"Fill color");
		glColor3f(1,0,0);
		glTexCoord4f(float(Color.Red)/256.0f,
			float(Color.Green)/256.0f,
			float(Color.Blue)/256.0f,
			float(Color.Alpha)/256.0f);
	}
	else if(FillStyleType==0x10)
	{
		//LOG(TRACE,"Fill gradient");
		glColor3f(0,1,0);

		color_entry buffer[256];
		int grad_index=0;
		RGBA color_l(0,0,0,1);
		int index_l=0;
		RGBA color_r(Gradient.GradientRecords[0].Color);
		int index_r=Gradient.GradientRecords[0].Ratio;

		for(int i=0;i<256;i++)
		{
			float dist=i-index_l;
			dist/=(index_r-index_l);
			RGBA c=medianColor(color_l,color_r,dist);
			buffer[i].r=float(c.Red)/256.0f;
			buffer[i].g=float(c.Green)/256.0f;
			buffer[i].b=float(c.Blue)/256.0f;
			buffer[i].a=1;

			if(Gradient.GradientRecords[grad_index].Ratio==i)
			{
				grad_index++;
				color_l=color_r;
				index_l=index_r;
				color_r=Gradient.GradientRecords[grad_index].Color;
				index_r=Gradient.GradientRecords[grad_index].Ratio;
			}
		}

		glBindTexture(GL_TEXTURE_2D,rt->data_tex);
		glTexImage2D(GL_TEXTURE_2D,0,4,256,1,0,GL_RGBA,GL_FLOAT,buffer);
	}
	else
	{
		LOG(LOG_NOT_IMPLEMENTED,"Reverting to fixed function");
		FILLSTYLE::fixedColor(0.5,0.5,0);
	}
}

void FILLSTYLE::fixedColor(float r, float g, float b)
{
	glBindTexture(GL_TEXTURE_2D,rt->data_tex);

	//Let's abuse of glColor and glTexCoord to transport
	//custom information
	glColor3f(1,0,0);
	glTexCoord4f(r,g,b,1);
}

std::istream& lightspark::operator>>(std::istream& s, FILLSTYLE& v)
{
	s >> v.FillStyleType;
	if(v.FillStyleType==0x00)
	{
		if(v.version==1 || v.version==2)
		{
			RGB tmp;
			s >> tmp;
			v.Color=tmp;
		}
		else
			s >> v.Color;
	}
	else if(v.FillStyleType==0x10 || v.FillStyleType==0x12)
	{
		s >> v.GradientMatrix;
		v.Gradient.version=v.version;
		s >> v.Gradient;
	}
	else if(v.FillStyleType==0x41 || v.FillStyleType==0x42 || v.FillStyleType==0x43)
	{
		s >> v.BitmapId >> v.BitmapMatrix;
	}
	else
	{
		LOG(LOG_ERROR,"Not supported fill style " << (int)v.FillStyleType << "... Aborting");
	}
	return s;
}


std::istream& lightspark::operator>>(std::istream& s, MORPHFILLSTYLE& v)
{
	s >> v.FillStyleType;
	if(v.FillStyleType==0x00)
	{
		s >> v.StartColor >> v.EndColor;
	}
	else if(v.FillStyleType==0x10 || v.FillStyleType==0x12)
	{
		s >> v.StartGradientMatrix >> v.EndGradientMatrix;
		s >> v.NumGradients;
		UI8 t;
		RGBA t2;
		for(int i=0;i<v.NumGradients;i++)
		{
			s >> t >> t2;
			v.StartRatios.push_back(t);
			v.StartColors.push_back(t2);
			s >> t >> t2;
			v.EndRatios.push_back(t);
			v.EndColors.push_back(t2);
		}
	}
	else
	{
		LOG(LOG_ERROR,"Not supported fill style " << (int)v.FillStyleType << "... Aborting");
	}
	return s;
}

GLYPHENTRY::GLYPHENTRY(TEXTRECORD* p,BitStream& bs):parent(p)
{
	GlyphIndex = UB(parent->parent->GlyphBits,bs);
	GlyphAdvance = SB(parent->parent->AdvanceBits,bs);
}

SHAPERECORD::SHAPERECORD(SHAPE* p,BitStream& bs):parent(p),next(0)
{
	TypeFlag = UB(1,bs);
	if(TypeFlag)
	{
		StraightFlag=UB(1,bs);
		NumBits=UB(4,bs);
		if(StraightFlag)
		{

			GeneralLineFlag=UB(1,bs);
			if(!GeneralLineFlag)
				VertLineFlag=UB(1,bs);

			if(GeneralLineFlag || !VertLineFlag)
			{
				DeltaX=SB(NumBits+2,bs);
			}
			if(GeneralLineFlag || VertLineFlag)
			{
				DeltaY=SB(NumBits+2,bs);
			}
		}
		else
		{
			
			ControlDeltaX=SB(NumBits+2,bs);
			ControlDeltaY=SB(NumBits+2,bs);
			AnchorDeltaX=SB(NumBits+2,bs);
			AnchorDeltaY=SB(NumBits+2,bs);
			
		}
	}
	else
	{
		StateNewStyles = UB(1,bs);
		StateLineStyle = UB(1,bs);
		StateFillStyle1 = UB(1,bs);
		StateFillStyle0 = UB(1,bs);
		StateMoveTo = UB(1,bs);
		if(StateMoveTo)
		{
			MoveBits = UB(5,bs);
			MoveDeltaX = SB(MoveBits,bs);
			MoveDeltaY = SB(MoveBits,bs);
		}
		if(StateFillStyle0)
		{
			FillStyle0=UB(parent->NumFillBits,bs);
		}
		if(StateFillStyle1)
		{
			FillStyle1=UB(parent->NumFillBits,bs);
		}
		if(StateLineStyle)
		{
			LineStyle=UB(parent->NumLineBits,bs);
		}
		if(StateNewStyles)
		{
			SHAPEWITHSTYLE* ps=static_cast<SHAPEWITHSTYLE*>(parent);
			bs.pos=0;
			FILLSTYLEARRAY a;
			bs.f >> ps->FillStyles;
			LINESTYLEARRAY b;
			bs.f >> ps->LineStyles;
			parent->NumFillBits=UB(4,bs);
			parent->NumLineBits=UB(4,bs);
		}
	}
}

std::istream& lightspark::operator>>(std::istream& stream, CXFORMWITHALPHA& v)
{
	BitStream bs(stream);
	v.HasAddTerms=UB(1,bs);
	v.HasMultTerms=UB(1,bs);
	v.NBits=UB(4,bs);
	if(v.HasMultTerms)
	{
		v.RedMultTerm=SB(v.NBits,bs);
		v.GreenMultTerm=SB(v.NBits,bs);
		v.BlueMultTerm=SB(v.NBits,bs);
		v.AlphaMultTerm=SB(v.NBits,bs);
	}
	if(v.HasAddTerms)
	{
		v.RedAddTerm=SB(v.NBits,bs);
		v.GreenAddTerm=SB(v.NBits,bs);
		v.BlueAddTerm=SB(v.NBits,bs);
		v.AlphaAddTerm=SB(v.NBits,bs);
	}
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, MATRIX& v)
{
	BitStream bs(stream);
	int HasScale=UB(1,bs);
	if(HasScale)
	{
		int NScaleBits=UB(5,bs);
		v.ScaleX=FB(NScaleBits,bs);
		v.ScaleY=FB(NScaleBits,bs);
	}
	int HasRotate=UB(1,bs);
	if(HasRotate)
	{
		int NRotateBits=UB(5,bs);
		v.RotateSkew0=FB(NRotateBits,bs);
		v.RotateSkew1=FB(NRotateBits,bs);
	}
	int NTranslateBits=UB(5,bs);
	v.TranslateX=SB(NTranslateBits,bs);
	v.TranslateY=SB(NTranslateBits,bs);
	return stream;
}

std::istream& lightspark::operator>>(std::istream& stream, BUTTONRECORD& v)
{
	BitStream bs(stream);

	UB(2,bs);
	v.ButtonHasBlendMode=UB(1,bs);
	v.ButtonHasFilterList=UB(1,bs);
	v.ButtonStateHitTest=UB(1,bs);
	v.ButtonStateDown=UB(1,bs);
	v.ButtonStateOver=UB(1,bs);
	v.ButtonStateUp=UB(1,bs);

	if(v.isNull())
		return stream;

	stream >> v.CharacterID >> v.PlaceDepth >> v.PlaceMatrix >> v.ColorTransform;

	if(v.ButtonHasFilterList | v.ButtonHasBlendMode)
		LOG(LOG_ERROR,"Button record not yet totally supported");

	return stream;
}

void DictionaryDefinable::define(ASObject* g)
{
	abort();
/*	DictionaryTag* t=p->root->dictionaryLookup(dict_id);
	ASObject* o=dynamic_cast<ASObject*>(t);
	if(o==NULL)
	{
		//Should not happen in real live
		ASObject* ret=new ASObject;
		g->setVariableByName(p->Name,new ASObject);
	}
	else
	{
		ASObject* ret=o->clone();
		if(ret->constructor)
			ret->constructor->call(ret,NULL);
		p->setWrapped(ret);
		g->setVariableByName(p->Name,ret);
	}*/
}

variables_map::~variables_map()
{
	var_iterator it=Variables.begin();
	for(it;it!=Variables.end();it++)
	{
		if(it->second.second.var)
			it->second.second.var->decRef();
		if(it->second.second.setter)
			it->second.second.setter->decRef();
		if(it->second.second.getter)
			it->second.second.getter->decRef();
	}
}

ASObject::ASObject(Manager* m):
	prototype(NULL),actualPrototype(NULL),ref_count(1),
	manager(m),type(T_OBJECT),implementation(NULL),cur_level(0)
{
}

ASObject::ASObject(const ASObject& o):
	prototype(o.prototype),actualPrototype(o.prototype),manager(NULL),ref_count(1),
	type(o.type),implementation(NULL),cur_level(0)
{
	if(prototype)
		prototype->incRef();

	assert(o.Variables.size()==0);
}

ASObject::~ASObject()
{
	if(prototype)
		prototype->decRef();

	assert(prototype==actualPrototype);
}

int ASObject::_maxlevel()
{
	return (prototype)?(prototype->max_level):0;
}

void ASObject::resetLevel()
{
	cur_level=_maxlevel();
}

void variables_map::initSlot(int n, int level, const tiny_string& name, const tiny_string& ns)
{
	if(n>slots_vars.size())
		slots_vars.resize(n,Variables.end());

	pair<var_iterator, var_iterator> ret=Variables.equal_range(nameAndLevel(name,level));
	if(ret.first!=ret.second)
	{
		//Check if this namespace is already present
		var_iterator start=ret.first;
		for(start;start!=ret.second;start++)
		{
			if(start->second.first==ns)
			{
				slots_vars[n-1]=start;
				return;
			}
		}
	}

	//Name not present, no good
	abort();
}

void variables_map::setSlot(int n,ASObject* o)
{
	if(n-1<slots_vars.size())
	{
		assert(slots_vars[n-1]!=Variables.end());
		if(slots_vars[n-1]->second.second.setter)
			abort();
		slots_vars[n-1]->second.second.var->decRef();
		slots_vars[n-1]->second.second.var=o;
	}
	else
	{
		LOG(LOG_ERROR,"Setting slot out of range");
		abort();
		//slots_vars.resize(n);
		//slots[n-1]=o;
	}
}

obj_var* variables_map::getValueAt(int index, int& level)
{
	//TODO: CHECK behavious on overridden methods
	if(index<Variables.size())
	{
		var_iterator it=Variables.begin();

		for(int i=0;i<index;i++)
			it++;

		level=it->first.level;
		return &it->second.second;
	}
	else
	{
		LOG(LOG_ERROR,"Index too big");
		abort();
	}
}

ASObject* ASObject::getValueAt(int index)
{
	int level;
	obj_var* obj=Variables.getValueAt(index,level);
	assert(obj);
	ASObject* ret;
	if(obj->getter)
	{
		//Call the getter
		LOG(LOG_CALLS,"Calling the getter");
		IFunction* getter=obj->getter->getOverride();
		ASObject* ret=getter->call(this,NULL,level);
		ret=getter->call(this,NULL,level);
		ret->fake_decRef();
		LOG(LOG_CALLS,"End of getter");
	}
	else
		ret=obj->var;

	return ret;
}

tiny_string variables_map::getNameAt(int index)
{
	//TODO: CHECK behavious on overridden methods
	if(index<Variables.size())
	{
		var_iterator it=Variables.begin();

		for(int i=0;i<index;i++)
			it++;

		return tiny_string(it->first.name);
	}
	else
	{
		LOG(LOG_ERROR,"Index too big");
		abort();
	}
}

int ASObject::numVariables()
{
	return Variables.size();
}

void ASObject::recursiveBuild(const Class_base* cur)
{
	if(cur->super)
	{
		Class_base* old=actualPrototype;
		actualPrototype=cur->super;
		recursiveBuild(cur->super);
		assert(cur==old);
		actualPrototype=old;
	}
	LOG(LOG_TRACE,"Building traits for " << cur->class_name);
	cur_level=cur->max_level;
	cur->buildInstanceTraits(this);

	//Link the interfaces for this level
	const vector<Class_base*>& interfaces=cur->getInterfaces();
	for(int i=0;i<interfaces.size();i++)
	{
		LOG(LOG_CALLS,"Linking with interface " << interfaces[i]->class_name);
		interfaces[i]->linkInterface(this);
	}
}

void ASObject::handleConstruction(arguments* args, bool buildAndLink)
{
/*	if(actualPrototype->class_index==-2)
	{
		abort();
		//We have to build the method traits
		SyntheticFunction* sf=static_cast<SyntheticFunction*>(this);
		LOG(LOG_CALLS,"Building method traits");
		for(int i=0;i<sf->mi->body->trait_count;i++)
			sf->mi->context->buildTrait(this,&sf->mi->body->traits[i]);
		sf->call(this,args,max_level);
	}*/
	assert(actualPrototype->class_index!=-2);
	assert(buildAndLink==false || actualPrototype==prototype);

	if(buildAndLink)
	{
		//HACK: suppress implementation handling of variable just now
		IInterface* impl=implementation;
		implementation=NULL;
		recursiveBuild(actualPrototype);
		//And restore it
		implementation=impl;
		assert(cur_level==prototype->max_level);
	}

	if(actualPrototype->constructor)
	{
		LOG(LOG_CALLS,"Calling Instance init " << actualPrototype->class_name);
		//As constructors are not binded, we should change here the level

		int l=cur_level;
		cur_level=actualPrototype->max_level;
		actualPrototype->constructor->call(this,args,actualPrototype->max_level);
		cur_level=l;
	}
}

std::istream& lightspark::operator>>(std::istream& s, CLIPEVENTFLAGS& v)
{
	if(pt->version<=5)
	{
		UI16 t;
		s >> t;
		v.toParse=t;
	}
	else
	{
		s >> v.toParse;
	}
	return s;
}

bool CLIPEVENTFLAGS::isNull()
{
	return toParse==0;
}

std::istream& lightspark::operator>>(std::istream& s, CLIPACTIONRECORD& v)
{
	s >> v.EventFlags;
	if(v.EventFlags.isNull())
		return s;
	s >> v.ActionRecordSize;
	LOG(LOG_NOT_IMPLEMENTED,"Skipping " << v.ActionRecordSize << " of action data");
	ignore(s,v.ActionRecordSize);
	return s;
}

bool CLIPACTIONRECORD::isLast()
{
	return EventFlags.isNull();
}

std::istream& lightspark::operator>>(std::istream& s, CLIPACTIONS& v)
{
	s >> v.Reserved >> v.AllEventFlags;
	while(1)
	{
		CLIPACTIONRECORD t;
		s >> t;
		if(t.isLast())
			break;
		v.ClipActionRecords.push_back(t);
	}
	return s;
}

ASObject* lightspark::abstract_d(number_t i)
{
	Number* ret=dManager->get<Number>();
	ret->val=i;
	return ret;
}

ASObject* lightspark::abstract_b(bool i)
{
	return new Boolean(i);
}

ASObject* lightspark::abstract_i(intptr_t i)
{
	Integer* ret=iManager->get<Integer>();
	ret->val=i;
	return ret;
}

void lightspark::stringToQName(const tiny_string& tmp, tiny_string& name, tiny_string& ns)
{
	//Ok, let's split our string into namespace and name part
	for(int i=tmp.len()-1;i>0;i--)
	{
		if(tmp[i]==':')
		{
			assert(tmp[i-1]==':');
			ns=tmp.substr(0,i-1);
			name=tmp.substr(i+1,tmp.len());
			return;
		}
	}
	//No double colons in the string
	name=tmp;
	ns="";
}

