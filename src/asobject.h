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

#ifndef ASOBJECT_H
#define ASOBJECT_H

#include "compat.h"
#include "swftypes.h"
#include "smartrefs.h"
#include <map>

#define ASFUNCTION(name) \
	static ASObject* name(ASObject* , ASObject* const* args, const unsigned int argslen)

#define ASPROPERTY_GETTER(type,name) \
	type name; \
	ASFUNCTION( _getter_##name)

#define ASPROPERTY_SETTER(type,name) \
	type name; \
	ASFUNCTION( _setter_##name)

#define ASPROPERTY_GETTER_SETTER(type, name) \
	type name; \
	ASFUNCTION( _getter_##name); \
	ASFUNCTION( _setter_##name)

#define ASFUNCTIONBODY(c,name) \
	ASObject* c::name(ASObject* obj, ASObject* const* args, const unsigned int argslen)

#define ASFUNCTIONBODY_GETTER(c,name) \
	ASObject* c::_getter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		c* th = Class<c>::cast(obj); \
		if(argslen != 0) \
			throw ArgumentError("Arguments provided in getter"); \
		return ArgumentConversion<decltype(th->name)>::toAbstract(th->name); \
	}

#define ASFUNCTIONBODY_SETTER(c,name) \
	ASObject* c::_setter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		c* th = Class<c>::cast(obj); \
		if(argslen != 1) \
			throw ArgumentError("Wrong number of arguments in setter"); \
		th->name = ArgumentConversion<decltype(th->name)>::toConcrete(args[0]); \
		return NULL; \
	}

#define ASFUNCTIONBODY_SETTER_CB(c,name,callback) \
	ASObject* c::_setter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		c* th = Class<c>::cast(obj); \
		if(argslen != 1) \
			throw ArgumentError("Wrong number of arguments in setter"); \
		decltype(th->name) oldValue = th->name; \
		th->name = ArgumentConversion<decltype(th->name)>::toConcrete(args[0]); \
		th->callback(oldValue); \
		return NULL; \
	}

#define ASFUNCTIONBODY_GETTER_SETTER(c,name) \
		ASFUNCTIONBODY_GETTER(c,name) \
		ASFUNCTIONBODY_SETTER(c,name)

#define ASFUNCTIONBODY_GETTER_SETTER_CB(c,name,callback) \
		ASFUNCTIONBODY_GETTER(c,name) \
		ASFUNCTIONBODY_SETTER(c,name,callback)

#define REGISTER_GETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(_getter_##name),GETTER_METHOD,true)
#define REGISTER_SETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(_setter_##name),SETTER_METHOD,true)

#define REGISTER_GETTER_SETTER(c,name) \
		REGISTER_GETTER(c,name); \
		REGISTER_SETTER(c,name)

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
class ByteArray;

struct obj_var
{
	ASObject* var;
	Class_base* type;
	IFunction* setter;
	IFunction* getter;
	obj_var():var(NULL),type(NULL),setter(NULL),getter(NULL){}
	obj_var(ASObject* _v, Class_base* _t):var(_v),type(_t),setter(NULL),getter(NULL){}
	void setVar(ASObject* v);
};

enum TRAIT_KIND { NO_CREATE_TRAIT=0, DECLARED_TRAIT=1, DYNAMIC_TRAIT=2, BORROWED_TRAIT=4 };

struct variable
{
	nsNameAndKind ns;
	obj_var var;
	TRAIT_KIND kind;
	variable(const nsNameAndKind& _ns, TRAIT_KIND _k):ns(_ns),kind(_k){}
	variable(const nsNameAndKind& _ns, TRAIT_KIND _k, ASObject* _v, Class_base* _c):ns(_ns),var(_v,_c),kind(_k){}
};

class variables_map
{
//ASObject knows how to use its variable_map
friend class ASObject;
//Class_base uses the internal data to handle borrowed variables
friend class Class_base;
//Useful when linking
friend class InterfaceClass;
//ABCContext uses findObjVar when building and linking traits
friend class ABCContext;
friend ASObject* describeType(ASObject* obj,ASObject* const* args, const unsigned int argslen);
private:
	std::multimap<tiny_string,variable> Variables;
	typedef std::multimap<tiny_string,variable>::iterator var_iterator;
	typedef std::multimap<tiny_string,variable>::const_iterator const_var_iterator;
	std::vector<var_iterator> slots_vars;
	/**
	   Find a variable in the map

	   @param createKind If this is different from NO_CREATE_TRAIT and no variable is found
				a new one is created with the given kind
	   @param traitKinds Bitwise OR of accepted trait kinds
	*/
	obj_var* findObjVar(const tiny_string& name, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds);
	obj_var* findObjVar(const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds);
	//Initialize a new variable specifying the type (TODO: add support for const)
	void initializeVar(const multiname& mname, ASObject* obj, Class_base* type);
	void killObjVar(const multiname& mname);
	ASObject* getSlot(unsigned int n)
	{
		return slots_vars[n-1]->second.var.var;
	}
	void setSlot(unsigned int n,ASObject* o);
	void initSlot(unsigned int n,const tiny_string& name, const nsNameAndKind& ns);
	int size() const
	{
		return Variables.size();
	}
	tiny_string getNameAt(unsigned int i) const;
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

enum METHOD_TYPE { NORMAL_METHOD=0, SETTER_METHOD=1, GETTER_METHOD=2 };

class ASObject
{
friend class Manager;
friend class ABCVm;
friend class ABCContext;
friend class Class_base; //Needed for forced cleanup
friend class InterfaceClass;
friend class IFunction; //Needed for clone
friend ASObject* describeType(ASObject* obj,ASObject* const* args, const unsigned int argslen);
CLASSBUILDABLE(ASObject);
protected:
	//ASObject* asprototype; //HUMM.. ok the prototype, actually class, should be renamed
	//maps variable name to namespace and var
	variables_map Variables;
	ASObject(Manager* m=NULL);
	ASObject(const ASObject& o);
	virtual ~ASObject();
	SWFOBJECT_TYPE type;
	void serializeDynamicProperties(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const;
private:
	ATOMIC_INT32(ref_count);
	Manager* manager;
	int cur_level;
	virtual int _maxlevel();
	Class_base* prototype;
	ACQUIRE_RELEASE_FLAG(constructed);
	obj_var* findGettable(const multiname& name, bool borrowedMode) DLL_LOCAL;
	obj_var* findSettable(const multiname& name, bool borrowedMode) DLL_LOCAL;
	tiny_string toStringImpl() const;
public:
#ifndef NDEBUG
	//Stuff only used in debugging
	bool initialized;
	int getRefCount(){ return ref_count; }
#endif
	bool implEnable;
	void setPrototype(Class_base* c);
	Class_base* getPrototype() const { return prototype; }
	bool isConstructed() const { return ACQUIRE_READ(constructed); }
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
		uint32_t t=ATOMIC_DECREMENT(ref_count);
		if(t==0)
		{
			if(manager)
				manager->put(this);
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
	/*
	   The finalize function should be implemented in all derived class.
	   It should decRef all referenced objects. It's guaranteed that the only operations
	   that will happen on the object after finalization are decRef and delete.
	   Each class must call BaseClass::finalize in their finalize function. 
	   The finalize method must be callable multiple time with the same effects (no double frees)*/
	virtual void finalize();

	virtual ASObject* getVariableByMultiname(const multiname& name, bool skip_impl=false);
	virtual intptr_t getVariableByMultiname_i(const multiname& name);
	virtual void setVariableByMultiname_i(const multiname& name, intptr_t value);
	virtual void setVariableByMultiname(const multiname& name, ASObject* o);
	void initializeVariableByMultiname(const multiname& name, ASObject* o, Class_base* type);
	virtual void deleteVariableByMultiname(const multiname& name);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind);
	void setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind);
	//NOTE: the isBorrowed flag is used to distinguish methods/setters/getters that are inside a class but on behalf of the instances
	void setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed);
	void setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed);
	virtual bool hasPropertyByMultiname(const multiname& name, bool considerDynamic);
	ASObject* getSlot(unsigned int n)
	{
		return Variables.getSlot(n);
	}
	void setSlot(unsigned int n,ASObject* o)
	{
		Variables.setSlot(n,o);
	}
	void initSlot(unsigned int n, const multiname& name);
	unsigned int numVariables() const;
	tiny_string getNameAt(int i) const
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
	ASFUNCTION(generator);

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
	
	//Enumeration handling
	virtual uint32_t nextNameIndex(uint32_t cur_index);
	virtual _R<ASObject> nextName(uint32_t index);
	virtual _R<ASObject> nextValue(uint32_t index);

	//Called when the object construction is completed. Used by MovieClip implementation
	virtual void constructionComplete();

	/**
	  Serialization interface

	  The various maps are used to implement reference type of the AMF3 spec
	  TODO:	Add traits map
	*/
	virtual void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const;
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
