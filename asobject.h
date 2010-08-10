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

#ifndef ASOBJECT_H
#define ASOBJECT_H

#include "compat.h"
#include "swftypes.h"
#include <map>

#define ASFUNCTION(name) \
	static ASObject* name(ASObject* , ASObject* const* args, const unsigned int argslen)
#define ASFUNCTIONBODY(c,name) \
	ASObject* c::name(ASObject* obj, ASObject* const* args, const unsigned int argslen)

#define CLASSBUILDABLE(className) \
	friend class Class<className>; 

enum SWFOBJECT_TYPE { T_OBJECT=0, T_INTEGER=1, T_NUMBER=2, T_FUNCTION=3, T_UNDEFINED=4, T_NULL=5, T_STRING=6, 
	T_DEFINABLE=7, T_BOOLEAN=8, T_ARRAY=9, T_CLASS=10, T_QNAME=11, T_NAMESPACE=12, T_UINTEGER=13, T_PROXY=14};

namespace lightspark
{

class ASObject;
class IFunction;
class Manager;
template<class T> class Class;
class Class_base;
class ABCContext;

struct obj_var
{
	ASObject* var;
	IFunction* setter;
	IFunction* getter;
	obj_var():var(NULL),setter(NULL),getter(NULL){}
	explicit obj_var(ASObject* v):var(v),setter(NULL),getter(NULL){}
	explicit obj_var(ASObject* v,IFunction* g,IFunction* s):var(v),setter(s),getter(g){}
};

class variables_map
{
//ASObject knows how to use its variable_map
friend class ASObject;
//ABCContext uses findObjVar when building and linking traits
friend class ABCContext;
private:
	std::multimap<tiny_string,std::pair<tiny_string, obj_var> > Variables;
	typedef std::multimap<tiny_string,std::pair<tiny_string, obj_var> >::iterator var_iterator;
	typedef std::multimap<tiny_string,std::pair<tiny_string, obj_var> >::const_iterator const_var_iterator;
	std::vector<var_iterator> slots_vars;
	//When findObjVar is invoked with create=true the pointer returned is garanteed to be valid
	obj_var* findObjVar(const tiny_string& name, const tiny_string& ns, bool create);
	obj_var* findObjVar(const multiname& mname, bool create);
	void killObjVar(const multiname& mname);
	ASObject* getSlot(unsigned int n)
	{
		return slots_vars[n-1]->second.second.var;
	}
	void setSlot(unsigned int n,ASObject* o);
	void initSlot(unsigned int n,const tiny_string& name, const tiny_string& ns);
	ASObject* getVariableByString(const std::string& name);
	int size() const
	{
		return Variables.size();
	}
	tiny_string getNameAt(unsigned int i);
	obj_var* getValueAt(unsigned int i);
	~variables_map();
public:
	void dumpVariables();
	void destroyContents();
};

class Manager
{
friend class ASObject;
private:
	std::vector<ASObject*> available;
	uint32_t maxCache;
public:
	Manager(uint32_t m):maxCache(m){}
template<class T>
	T* get();
	void put(ASObject* o);
};

class ASObject
{
friend class Manager;
friend class ABCVm;
friend class ABCContext;
friend class Class_base; //Needed for forced cleanup
friend class IFunction; //Needed for clone
CLASSBUILDABLE(ASObject);
protected:
	//ASObject* asprototype; //HUMM.. ok the prototype, actually class, should be renamed
	//maps variable name to namespace and var
	variables_map Variables;
	ASObject(Manager* m=NULL);
	ASObject(const ASObject& o);
	virtual ~ASObject();
	SWFOBJECT_TYPE type;
private:
	ATOMIC_INT32(ref_count);
	Manager* manager;
	int cur_level;
	virtual int _maxlevel();
	Class_base* prototype;
	obj_var* findGettable(const multiname& name) DLL_LOCAL;
	obj_var* findSettable(const multiname& name) DLL_LOCAL;

public:
#ifndef NDEBUG
	//Stuff only used in debugging
	bool initialized;
	int getRefCount(){ return ref_count; }
#endif
	bool implEnable;
	void setPrototype(Class_base* c);
	Class_base* getPrototype() const { return prototype; }
	ASFUNCTION(_constructor);
	ASFUNCTION(_getPrototype);
	ASFUNCTION(_setPrototype);
	ASFUNCTION(_toString);
	ASFUNCTION(hasOwnProperty);
	void check() const;
	void incRef()
	{
		//std::cout << "incref " << this << std::endl;
		ATOMIC_INCREMENT(ref_count);
		assert(ref_count>0);
	}
	void decRef()
	{
		//std::cout << "decref " << this << std::endl;
		assert_and_throw(ref_count>0);
		ATOMIC_DECREMENT(ref_count);
		// Note: The check is not done atomically, but this should be OK anyway due to
		//       how threads pass these objects between them. Be careful if you pass
		//       around objects and then decRef them.
		if(ref_count==0)
		{
			if(manager)
			{
				manager->put(this);
			}
			else
			{
				//Let's make refcount very invalid
				ref_count=-1024;
				//std::cout << "delete " << this << std::endl;
				delete this;
			}
		}
	}
	void fake_decRef()
	{
		ATOMIC_DECREMENT(ref_count);
	}
	static void s_incRef(ASObject* o)
	{
		o->incRef();
	}
	static void s_decRef(ASObject* o)
	{
		if(o)
			o->decRef();
	}
	static void s_decRef_safe(ASObject* o,ASObject* o2)
	{
		if(o && o!=o2)
			o->decRef();
	}
	virtual ASObject* getVariableByString(const std::string& name);
	//The enableOverride parameter is set to false in setSuper, getSuper and callSuper
	virtual ASObject* getVariableByMultiname(const multiname& name, bool skip_impl=false, bool enableOverride=true, ASObject* base=NULL);
	virtual intptr_t getVariableByMultiname_i(const multiname& name);
	virtual ASObject* getVariableByQName(const tiny_string& name, const tiny_string& ns, bool skip_impl=false);
	virtual void setVariableByMultiname_i(const multiname& name, intptr_t value);
	virtual void setVariableByMultiname(const multiname& name, ASObject* o, bool enableOverride=true, ASObject* base=NULL);
	virtual void deleteVariableByMultiname(const multiname& name);
	virtual void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o,bool skip_impl=false);
	void setGetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o);
	void setSetterByQName(const tiny_string& name, const tiny_string& ns, IFunction* o);
	bool hasPropertyByMultiname(const multiname& name);
	bool hasPropertyByQName(const tiny_string& name, const tiny_string& ns);
	ASObject* getSlot(unsigned int n)
	{
		return Variables.getSlot(n);
	}
	void setSlot(unsigned int n,ASObject* o)
	{
		Variables.setSlot(n,o);
	}
	void initSlot(unsigned int n,const tiny_string& name, const tiny_string& ns);
	virtual unsigned int numVariables();
	tiny_string getNameAt(int i)
	{
		return Variables.getNameAt(i);
	}
	ASObject* getValueAt(int i);
	SWFOBJECT_TYPE getObjectType() const
	{
		return type;
	}
	virtual tiny_string toString(bool debugMsg=false);
	virtual int32_t toInt();
	virtual uint32_t toUInt();
	virtual double toNumber();

	//Comparison operators
	virtual bool isEqual(ASObject* r);
	virtual TRISTATE isLess(ASObject* r);

	//Level handling
	int getLevel() const
	{
		return cur_level;
	}
	void decLevel()
	{
		assert_and_throw(cur_level>0);
		cur_level--;
	}
	void setLevel(int l)
	{
		cur_level=l;
	}
	void resetLevel();

	//Prototype handling
	Class_base* getActualPrototype() const;
	
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);
	
	//TODO: Rework this stuff
	virtual bool hasNext(unsigned int& index, bool& out);
	virtual bool nextName(unsigned int index, ASObject*& out);
	virtual bool nextValue(unsigned int index, ASObject*& out);
};

inline void Manager::put(ASObject* o)
{
	if(available.size()>maxCache)
		delete o;
	else
		available.push_back(o);
}

template<class T>
T* Manager::get()
{
	if(available.size())
	{
		T* ret=static_cast<T*>(available.back());
		available.pop_back();
		ret->incRef();
		//std::cout << "getting[" << name << "] " << ret << std::endl;
		return ret;
	}
	else
	{
		T* ret=Class<T>::getInstanceS(this);
		//std::cout << "newing" << ret << std::endl;
		return ret;
	}
}

};
#endif
