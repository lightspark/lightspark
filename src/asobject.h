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

/* declare setter/getter and associated member variable */
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

/* declare setter/getter for already existing member variable */
#define ASFUNCTION_GETTER(name) \
	ASFUNCTION( _getter_##name)

#define ASFUNCTION_SETTER(name) \
	ASFUNCTION( _setter_##name)

#define ASFUNCTION_GETTER_SETTER(name) \
	ASFUNCTION( _getter_##name); \
	ASFUNCTION( _setter_##name)

/* general purpose body for an AS function */
#define ASFUNCTIONBODY(c,name) \
	ASObject* c::name(ASObject* obj, ASObject* const* args, const unsigned int argslen)

/* full body for a getter declared by ASPROPERTY_GETTER or ASFUNCTION_GETTER */
#define ASFUNCTIONBODY_GETTER(c,name) \
	ASObject* c::_getter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		if(!obj->is<c>()) \
			throw Class<ArgumentError>::getInstanceS("Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 0) \
			throw Class<ArgumentError>::getInstanceS("Arguments provided in getter"); \
		return ArgumentConversion<decltype(th->name)>::toAbstract(th->name); \
	}

/* full body for a getter declared by ASPROPERTY_SETTER or ASFUNCTION_SETTER */
#define ASFUNCTIONBODY_SETTER(c,name) \
	ASObject* c::_setter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		if(!obj->is<c>()) \
			throw Class<ArgumentError>::getInstanceS("Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS("Wrong number of arguments in setter"); \
		th->name = ArgumentConversion<decltype(th->name)>::toConcrete(args[0]); \
		return NULL; \
	}

/* full body for a getter declared by ASPROPERTY_SETTER or ASFUNCTION_SETTER.
 * After the property has been updated, the callback member function is called with the old value
 * as parameter */
#define ASFUNCTIONBODY_SETTER_CB(c,name,callback) \
	ASObject* c::_setter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		if(!obj->is<c>()) \
			throw Class<ArgumentError>::getInstanceS("Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS("Wrong number of arguments in setter"); \
		decltype(th->name) oldValue = th->name; \
		th->name = ArgumentConversion<decltype(th->name)>::toConcrete(args[0]); \
		th->callback(oldValue); \
		return NULL; \
	}

/* full body for a getter declared by ASPROPERTY_GETTER_SETTER or ASFUNCTION_GETTER_SETTER */
#define ASFUNCTIONBODY_GETTER_SETTER(c,name) \
		ASFUNCTIONBODY_GETTER(c,name) \
		ASFUNCTIONBODY_SETTER(c,name)

#define ASFUNCTIONBODY_GETTER_SETTER_CB(c,name,callback) \
		ASFUNCTIONBODY_GETTER(c,name) \
		ASFUNCTIONBODY_SETTER(c,name,callback)

/* registers getter/setter with Class_base. To be used in ::sinit()-functions */
#define REGISTER_GETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(_getter_##name),GETTER_METHOD,true)

#define REGISTER_SETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(_setter_##name),SETTER_METHOD,true)

#define REGISTER_GETTER_SETTER(c,name) \
		REGISTER_GETTER(c,name); \
		REGISTER_SETTER(c,name)

#define CLASSBUILDABLE(className) \
	friend class Class<className>; 

namespace lightspark
{

class ASObject;
class IFunction;
class Manager;
template<class T> class Class;
class Class_base;
class ByteArray;
class Type;

enum TRAIT_KIND { NO_CREATE_TRAIT=0, DECLARED_TRAIT=1, DYNAMIC_TRAIT=2, BORROWED_TRAIT=4 };

struct variable
{
	nsNameAndKind ns;
	ASObject* var;
	multiname* typemname;
	const Type* type;
	IFunction* setter;
	IFunction* getter;
	TRAIT_KIND kind;
	//obj_var(ASObject* _v, Class_base* _t):var(_v),type(_t),{}
	variable(const nsNameAndKind& _ns, TRAIT_KIND _k)
		:ns(_ns),var(NULL),typemname(NULL),type(NULL),setter(NULL),getter(NULL),kind(_k){}
	variable(const nsNameAndKind& _ns, TRAIT_KIND _k, ASObject* _v, multiname* _t, const Type* type)
		:ns(_ns),var(_v),typemname(_t),type(type),setter(NULL),getter(NULL),kind(_k){}
	void setVar(ASObject* v);
};

class variables_map
{
private:
	std::multimap<tiny_string,variable> Variables;
	typedef std::multimap<tiny_string,variable>::iterator var_iterator;
	typedef std::multimap<tiny_string,variable>::const_iterator const_var_iterator;
	std::vector<var_iterator> slots_vars;
public:
	/**
	   Find a variable in the map

	   @param createKind If this is different from NO_CREATE_TRAIT and no variable is found
				a new one is created with the given kind
	   @param traitKinds Bitwise OR of accepted trait kinds
	*/
	variable* findObjVar(const tiny_string& name, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds);
	variable* findObjVar(const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds);
	//Initialize a new variable specifying the type (TODO: add support for const)
	void initializeVar(const multiname& mname, ASObject* obj, multiname* typemname);
	void killObjVar(const multiname& mname);
	ASObject* getSlot(unsigned int n)
	{
		assert(n<=slots_vars.size());
		return slots_vars[n-1]->second.var;
	}
	void setSlot(unsigned int n,ASObject* o);
	void initSlot(unsigned int n,const tiny_string& name, const nsNameAndKind& ns);
	int size() const
	{
		return Variables.size();
	}
	tiny_string getNameAt(unsigned int i) const;
	variable* getValueAt(unsigned int i);
	~variables_map();
	void check() const;
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
			std::map<const ASObject*, uint32_t>& objMap) const;
	void dumpVariables();
	void destroyContents();
};

/*
 * This class manages a list of unreferenced
 * objects (ref_count == 0), which can be reused
 * to save new/delete. ASObjects that have their
 * 'manager' property set wont delete themselves
 * on decRef to zero, but call Manager::put.
 */
class Manager
{
private:
	std::vector<ASObject*> available;
	uint32_t maxCache;
public:
	Manager(uint32_t m):maxCache(m){}
template<class T>
	T* get();
	void put(ASObject* o);
	~Manager();
};

enum METHOD_TYPE { NORMAL_METHOD=0, SETTER_METHOD=1, GETTER_METHOD=2 };
//for toPrimitive
enum TP_HINT { NO_HINT, NUMBER_HINT, STRING_HINT };


class ASObject
{
friend class Manager;
friend class ABCVm;
friend class ABCContext;
friend class Class_base; //Needed for forced cleanup
friend class InterfaceClass;
friend class IFunction; //Needed for clone
CLASSBUILDABLE(ASObject);
protected:
	ASObject();
	ASObject(const ASObject& o);
	virtual ~ASObject();
	SWFOBJECT_TYPE type;
	void serializeDynamicProperties(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap) const;
private:
	//maps variable name to namespace and var
	variables_map Variables;
	variable* findGettable(const multiname& name, bool borrowedMode) DLL_LOCAL;
	variable* findSettable(const multiname& name, bool borrowedMode, bool* has_getter=NULL) DLL_LOCAL;

	ATOMIC_INT32(ref_count);
	Manager* manager;
	Class_base* classdef;
	ACQUIRE_RELEASE_FLAG(constructed);
public:
#ifndef NDEBUG
	//Stuff only used in debugging
	bool initialized;
	int getRefCount(){ return ref_count; }
#endif
	bool implEnable;
	void setClass(Class_base* c);
	Class_base* getClass() const { return classdef; }
	bool isConstructed() const { return ACQUIRE_READ(constructed); }
	ASFUNCTION(_constructor);
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

	enum GET_VARIABLE_OPTION {NONE=0x00, SKIP_IMPL=0x01, XML_STRICT=0x02};
	virtual ASObject* getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE);
	virtual intptr_t getVariableByMultiname_i(const multiname& name);
	virtual void setVariableByMultiname_i(const multiname& name, intptr_t value);
	virtual void setVariableByMultiname(const multiname& name, ASObject* o);
	void initializeVariableByMultiname(const multiname& name, ASObject* o, multiname* typemname);
	virtual bool deleteVariableByMultiname(const multiname& name);
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
	/* Implements ECMA's 9.8 ToString operation, but returns the concrete value */
	tiny_string toString(bool debugMsg=false);
	virtual int32_t toInt();
	virtual uint32_t toUInt();
	/* Implements ECMA's 9.3 ToNumber operation, but returns the concrete value */
	number_t toNumber();
	/* Implements ECMA's ToPrimitive (9.1) and [[DefaultValue]] (8.6.2.6) */
	_R<ASObject> toPrimitive(TP_HINT hint = NO_HINT);
	bool isPrimitive() const;

	/* helper functions for calling the "valueOf" and
	 * "toString" AS-functions which may be members of this
	 *  object */
	bool has_valueOf();
	_R<ASObject> call_valueOf();
	bool has_toString();
	_R<ASObject> call_toString();

	ASFUNCTION(generator);

	/* helpers for the dynamic property 'prototype' */
	bool hasprop_prototype();
	ASObject* getprop_prototype();
	void setprop_prototype(_NR<ASObject>& prototype);

	//Comparison operators
	virtual bool isEqual(ASObject* r);
	virtual TRISTATE isLess(ASObject* r);

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

	virtual ASObject *describeType() const;

	/* returns true if the current object is of type T */
	template<class T> bool is() const { return dynamic_cast<const T*>(this); }
	/* returns this object casted to the given type.
	 * You have to make sure that it actually is the type (see is<T>() above)
	 */
	template<class T> const T* as() const { return static_cast<const T*>(this); }
	template<class T> T* as() { return static_cast<T*>(this); }
};

class Number;
class UInteger;
class Integer;
class Boolean;
class Template_base;
class ASString;
class Function;
class Array;
class Definable;
class Null;
class Undefined;
class Type;
template<> inline bool ASObject::is<Number>() const { return type==T_NUMBER; }
template<> inline bool ASObject::is<Integer>() const { return type==T_INTEGER; }
template<> inline bool ASObject::is<UInteger>() const { return type==T_UINTEGER; }
template<> inline bool ASObject::is<Boolean>() const { return type==T_BOOLEAN; }
template<> inline bool ASObject::is<ASString>() const { return type==T_STRING; }
template<> inline bool ASObject::is<Function>() const { return type==T_FUNCTION; }
template<> inline bool ASObject::is<Undefined>() const { return type==T_UNDEFINED; }
template<> inline bool ASObject::is<Null>() const { return type==T_NULL; }
template<> inline bool ASObject::is<Definable>() const { return type==T_DEFINABLE; }
template<> inline bool ASObject::is<Array>() const { return type==T_ARRAY; }
template<> inline bool ASObject::is<Class_base>() const { return type==T_CLASS; }
template<> inline bool ASObject::is<Template_base>() const { return type==T_TEMPLATE; }
template<> inline bool ASObject::is<Type>() const { return type==T_CLASS; }
}
#endif
