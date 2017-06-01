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

#ifndef ASOBJECT_H
#define ASOBJECT_H 1

#include "compat.h"
#include "swftypes.h"
#include "smartrefs.h"
#include "threading.h"
#include "memory_support.h"
#include <map>
#include <boost/intrusive/list.hpp>
#include <boost/container/flat_map.hpp>

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
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 0) \
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Arguments provided in getter"); \
		return ArgumentConversion<decltype(th->name)>::toAbstract(obj->getSystemState(),th->name); \
	}

#define ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(c,name) \
	ASObject* c::_getter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		if(!obj->is<c>()) \
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 0) \
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Arguments provided in getter"); \
		LOG(LOG_NOT_IMPLEMENTED,obj->getClassName() <<"."<< #name << " getter is not implemented"); \
		return ArgumentConversion<decltype(th->name)>::toAbstract(obj->getSystemState(),th->name); \
	}

/* full body for a getter declared by ASPROPERTY_SETTER or ASFUNCTION_SETTER */
#define ASFUNCTIONBODY_SETTER(c,name) \
	ASObject* c::_setter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		if(!obj->is<c>()) \
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Wrong number of arguments in setter"); \
		th->name = ArgumentConversion<decltype(th->name)>::toConcrete(args[0]); \
		return NULL; \
	}

#define ASFUNCTIONBODY_SETTER_NOT_IMPLEMENTED(c,name) \
	ASObject* c::_setter_##name(ASObject* obj, ASObject* const* args, const unsigned int argslen) \
	{ \
		if(!obj->is<c>()) \
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Wrong number of arguments in setter"); \
		LOG(LOG_NOT_IMPLEMENTED,obj->getClassName() <<"."<< #name << " setter is not implemented"); \
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
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Function applied to wrong object"); \
		c* th = obj->as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS(obj->getSystemState(),"Wrong number of arguments in setter"); \
		decltype(th->name) oldValue = th->name; \
		th->name = ArgumentConversion<decltype(th->name)>::toConcrete(args[0]); \
		th->callback(oldValue); \
		return NULL; \
	}

/* full body for a getter declared by ASPROPERTY_GETTER_SETTER or ASFUNCTION_GETTER_SETTER */
#define ASFUNCTIONBODY_GETTER_SETTER(c,name) \
		ASFUNCTIONBODY_GETTER(c,name) \
		ASFUNCTIONBODY_SETTER(c,name)

#define ASFUNCTIONBODY_GETTER_SETTER_NOT_IMPLEMENTED(c,name) \
		ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(c,name) \
		ASFUNCTIONBODY_SETTER_NOT_IMPLEMENTED(c,name)

#define ASFUNCTIONBODY_GETTER_SETTER_CB(c,name,callback) \
		ASFUNCTIONBODY_GETTER(c,name) \
		ASFUNCTIONBODY_SETTER_CB(c,name,callback)

/* registers getter/setter with Class_base. To be used in ::sinit()-functions */
#define REGISTER_GETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(c->getSystemState(),_getter_##name),GETTER_METHOD,true)

#define REGISTER_SETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(c->getSystemState(),_setter_##name),SETTER_METHOD,true)

#define REGISTER_GETTER_SETTER(c,name) \
		REGISTER_GETTER(c,name); \
		REGISTER_SETTER(c,name)

#define CLASS_DYNAMIC_NOT_FINAL 0
#define CLASS_FINAL 1
#define CLASS_SEALED 2

// TODO: Every class should have a constructor
#define CLASS_SETUP_NO_CONSTRUCTOR(c, superClass, attributes) \
	c->setSuper(Class<superClass>::getRef(c->getSystemState())); \
	c->setConstructor(NULL); \
	c->isFinal = ((attributes) & CLASS_FINAL) != 0;	\
	c->isSealed = ((attributes) & CLASS_SEALED) != 0

#define CLASS_SETUP(c, superClass, constructor, attributes) \
	CLASS_SETUP_NO_CONSTRUCTOR(c, superClass, attributes); \
	c->setConstructor(Class<IFunction>::getFunction(c->getSystemState(),constructor));

#define CLASS_SETUP_CONSTRUCTOR_LENGTH(c, superClass, constructor, ctorlength, attributes) \
	CLASS_SETUP_NO_CONSTRUCTOR(c, superClass, attributes); \
	c->setConstructor(Class<IFunction>::getFunction(c->getSystemState(),(constructor), (ctorlength)));

namespace lightspark
{

class ASObject;
class IFunction;
template<class T> class Class;
class Class_base;
class ByteArray;
class Loader;
class Type;
class ABCContext;
class SystemState;
struct asfreelist;

extern SystemState* getSys();
enum TRAIT_KIND { NO_CREATE_TRAIT=0, DECLARED_TRAIT=1, DYNAMIC_TRAIT=2, INSTANCE_TRAIT=5, CONSTANT_TRAIT=9 /* constants are also declared traits */ };
enum TRAIT_STATE { NO_STATE=0, HAS_GETTER_SETTER=1, TYPE_RESOLVED=2 };

struct variable
{
	ASObject* var;
	union
	{
		multiname* traitTypemname;
		const Type* type;
		void* typeUnion;
	};
	IFunction* setter;
	IFunction* getter;
	nsNameAndKind ns;
	TRAIT_KIND kind;
	TRAIT_STATE traitState;
	bool isenumerable:1;
	bool issealed:1;
	variable(TRAIT_KIND _k,const nsNameAndKind& _ns)
		: var(NULL),typeUnion(NULL),setter(NULL),getter(NULL),ns(_ns),kind(_k),traitState(NO_STATE),isenumerable(true),issealed(false) {}
	variable(TRAIT_KIND _k, ASObject* _v, multiname* _t, const Type* type, const nsNameAndKind &_ns, bool _isenumerable);
	void setVar(ASObject* v);
	/*
	 * To be used only if the value is guaranteed to be of the right type
	 */
	void setVarNoCoerce(ASObject* v);
};

struct varName
{
	uint32_t nameId;
	nsNameAndKind ns;
	varName():nameId(0){}
	varName(uint32_t name, const nsNameAndKind& _ns):nameId(name),ns(_ns){}
	inline bool operator<(const varName& r) const
	{
		//Sort by name first
		if(nameId==r.nameId)
		{
			//Then by namespace
			return ns<r.ns;
		}
		else
			return nameId<r.nameId;
	}
	inline bool operator==(const varName& r) const
	{
		return nameId==r.nameId && ns == r.ns;
	}
};

class variables_map
{
public:
	//Names are represented by strings in the string and namespace pools
	typedef boost::container::flat_multimap<uint32_t,variable,std::less<uint32_t>,reporter_allocator<std::pair<uint32_t, variable>>>
		mapType;
	mapType Variables;
	typedef boost::container::flat_multimap<uint32_t,variable>::iterator var_iterator;
	typedef boost::container::flat_multimap<uint32_t,variable>::const_iterator const_var_iterator;
	std::vector<varName> slots_vars;
	variables_map(MemoryAccount* m);
	/**
	   Find a variable in the map

	   @param createKind If this is different from NO_CREATE_TRAIT and no variable is found
				a new one is created with the given kind
	   @param traitKinds Bitwise OR of accepted trait kinds
	*/
	variable* findObjVar(uint32_t nameId, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds);
	variable* findObjVar(SystemState* sys,const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds);
	/**
	 * Const version of findObjVar, useful when looking for getters
	 */
	inline const variable* findObjVar(SystemState* sys,const multiname& mname, uint32_t traitKinds, uint32_t* nsRealId = NULL) const
	{
		if (mname.isEmpty())
			return NULL;
		uint32_t name=mname.name_type == multiname::NAME_STRING ? mname.name_s_id : mname.normalizedNameId(sys);
		assert(!mname.ns.empty());
		
		const_var_iterator ret=Variables.lower_bound(name);
		auto nsIt=mname.ns.cbegin();
		//Find the namespace
		while(ret!=Variables.cend() && ret->first==name)
		{
			//breaks when the namespace is not found
			const nsNameAndKind& ns=ret->second.ns;
			if(ns==*nsIt || (mname.hasEmptyNS && ns.hasEmptyName()) || (mname.hasBuiltinNS && ns.hasBuiltinName()))
			{
				if(ret->second.kind & traitKinds)
				{
					if (nsRealId)
						*nsRealId = ns.nsRealId;
					return &ret->second;
				}
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
	
		return NULL;
	}
	
	//Initialize a new variable specifying the type (TODO: add support for const)
	void initializeVar(const multiname& mname, ASObject* obj, multiname *typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj, uint32_t slot_id, bool isenumerable);
	void killObjVar(SystemState* sys, const multiname& mname);
	ASObject* getSlot(unsigned int n);
	/*
	 * This method does throw if the slot id is not valid
	 */
	void validateSlotId(unsigned int n) const;
	void setSlot(unsigned int n,ASObject* o);
	/*
	 * This version of the call is guarantee to require no type conversion
	 * this is verified at optimization time
	 */
	void setSlotNoCoerce(unsigned int n,ASObject* o);
	void initSlot(unsigned int n, uint32_t nameId, const nsNameAndKind& ns);
	inline unsigned int size() const
	{
		return Variables.size();
	}
	tiny_string getNameAt(SystemState* sys,unsigned int i) const;
	variable* getValueAt(unsigned int i);
	int getNextEnumerable(unsigned int i) const;
	~variables_map();
	void check() const;
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap) const;
	void dumpVariables() const;
	void destroyContents();
};

enum METHOD_TYPE { NORMAL_METHOD=0, SETTER_METHOD=1, GETTER_METHOD=2 };
//for toPrimitive
enum TP_HINT { NO_HINT, NUMBER_HINT, STRING_HINT };

class ASObject: public memory_reporter, public RefCountable
{
friend class ABCVm;
friend class ABCContext;
friend class Class_base; //Needed for forced cleanup
friend class Class_inherit; 
friend void lookupAndLink(Class_base* c, const tiny_string& name, const tiny_string& interfaceNs);
friend class IFunction; //Needed for clone
friend struct asfreelist;
public:
	asfreelist* objfreelist;
private:
	variables_map Variables;
	unsigned int varcount;
	Class_base* classdef;
	inline const variable* findGettable(const multiname& name, uint32_t* nsRealId = NULL) const DLL_LOCAL
	{
		const variable* ret=varcount ? Variables.findObjVar(getSystemState(),name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId):NULL;
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(ret->getter || ret->var))
				ret=NULL;
		}
		return ret;
	}
	
	variable* findSettable(const multiname& name, bool* has_getter=NULL) DLL_LOCAL;
	multiname* proxyMultiName;
	SystemState* sys;
protected:
	ASObject(MemoryAccount* m):objfreelist(NULL),Variables(m),varcount(0),classdef(NULL),proxyMultiName(NULL),sys(NULL),
		stringId(UINT32_MAX),type(T_OBJECT),subtype(SUBTYPE_NOT_SET),traitsInitialized(false),constructIndicator(false),constructorCallComplete(false),implEnable(true)
	{
#ifndef NDEBUG
		//Stuff only used in debugging
		initialized=false;
#endif
	}
	
	ASObject(const ASObject& o);
	virtual ~ASObject()
	{
		destroy();
	}
	uint32_t stringId;
	SWFOBJECT_TYPE type;
	CLASS_SUBTYPE subtype;
	
	bool traitsInitialized:1;
	bool constructIndicator:1;
	bool constructorCallComplete:1; // indicates that the constructor including all super constructors has been called
	void serializeDynamicProperties(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t> traitsMap) const;
	void setClass(Class_base* c);
	static variable* findSettableImpl(SystemState* sys,variables_map& map, const multiname& name, bool* has_getter);
	inline static const variable* findGettableImpl(SystemState* sys,const variables_map& map, const multiname& name, uint32_t* nsRealId = NULL)
	{
		const variable* ret=map.findObjVar(sys,name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId);
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(ret->getter || ret->var))
				ret=NULL;
		}
		return ret;
	}
	
	/*
	   overridden from RefCountable
	   The destruct function is called when the reference count reaches 0 and the object is added to the free list of the class.
	   It should be implemented in all derived classes.
	   It should decRef all referenced objects.
	   It has to reset all data to their default state.
	   It has to call ASObject::destruct() as last instruction
	   The destruct method must be callable multiple time with the same effects (no double frees).
	*/
	bool destruct();
	// called when object is really destroyed
	virtual void destroy(){}
public:
	ASObject(Class_base* c,SWFOBJECT_TYPE t = T_OBJECT,CLASS_SUBTYPE subtype = SUBTYPE_NOT_SET);
	
#ifndef NDEBUG
	//Stuff only used in debugging
	bool initialized:1;
#endif
	bool implEnable:1;
	Class_base* getClass() const { return classdef; }
	ASFUNCTION(_constructor);
	// constructor for subclasses that can't be instantiated.
	// Throws ArgumentError.
	ASFUNCTION(_constructorNotInstantiatable);
	ASFUNCTION(_toString);
	ASFUNCTION(_toLocaleString);
	ASFUNCTION(hasOwnProperty);
	ASFUNCTION(valueOf);
	ASFUNCTION(isPrototypeOf);
	ASFUNCTION(propertyIsEnumerable);
	ASFUNCTION(setPropertyIsEnumerable);
	void check() const;
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
	   The finalize function is used only for classes that don't have the reusable flag set
	   if a class is made reusable, it should implement destruct() instead
	   It should decRef all referenced objects.
	   It has to reset all data to their default state.
	   The finalize method must be callable multiple time with the same effects (no double frees).
	*/
	inline virtual void finalize() {}

	enum GET_VARIABLE_OPTION {NONE=0x00, SKIP_IMPL=0x01, XML_STRICT=0x02};

	virtual _NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE)
	{
		return getVariableByMultiname(name,opt,classdef);
	}
	/*
	 * Helper method using the get the raw variable struct instead of calling the getter.
	 * It is used by getVariableByMultiname and by early binding code
	 */
	const variable* findVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls, uint32_t* nsRealID = NULL) const;
	/*
	 * Gets a variable of this object. It looks through all classes (beginning at cls),
	 * then the prototype chain, and then instance variables.
	 * If the property found is a getter, it is called and its return value returned.
	 */
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls);
	virtual int32_t getVariableByMultiname_i(const multiname& name);
	/* Simple getter interface for the common case */
	_NR<ASObject> getVariableByMultiname(const tiny_string& name, std::list<tiny_string> namespaces);
	/*
	 * Execute a AS method on this object. Returns the value
	 * returned by the function. One reference of each args[i] is
	 * consumed. The method must exist, otherwise a TypeError is
	 * thrown.
	 */
	_NR<ASObject> executeASMethod(const tiny_string& methodName, std::list<tiny_string> namespaces, ASObject* const* args, uint32_t num_args);
	virtual void setVariableByMultiname_i(const multiname& name, int32_t value);
	enum CONST_ALLOWED_FLAG { CONST_ALLOWED=0, CONST_NOT_ALLOWED };
	virtual void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
	{
		setVariableByMultiname(name,o,allowConst,classdef);
	}
	/*
	 * Sets  variable of this object. It looks through all classes (beginning at cls),
	 * then the prototype chain, and then instance variables.
	 * If the property found is a setter, it is called with the given 'o'.
	 * If no property is found, an instance variable is created.
	 * Setting CONSTANT_TRAIT is only allowed if allowConst is true
	 */
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst, Class_base* cls);
	/*
	 * Called by ABCVm::buildTraits to create DECLARED_TRAIT or CONSTANT_TRAIT and set their type
	 */
	void initializeVariableByMultiname(const multiname& name, ASObject* o, multiname* typemname,
			ABCContext* context, TRAIT_KIND traitKind, uint32_t slot_id, bool isenumerable);
	/*
	 * Called by ABCVm::initProperty (implementation of ABC instruction), it is allowed to set CONSTANT_TRAIT
	 */
	void initializeVariableByMultiname(const multiname& name, ASObject* o);
	virtual bool deleteVariableByMultiname(const multiname& name);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true);
	void setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true);
	void setVariableByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true);
	//NOTE: the isBorrowed flag is used to distinguish methods/setters/getters that are inside a class but on behalf of the instances
	void setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodByQName(uint32_t nameId, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	virtual bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	ASObject* getSlot(unsigned int n)
	{
		return Variables.getSlot(n);
	}
	void setSlot(unsigned int n,ASObject* o)
	{
		Variables.setSlot(n,o);
	}
	void setSlotNoCoerce(unsigned int n,ASObject* o)
	{
		Variables.setSlotNoCoerce(n,o);
	}
	void initSlot(unsigned int n, const multiname& name);
	unsigned int numVariables() const;
	inline tiny_string getNameAt(int i) const
	{
		return Variables.getNameAt(sys,i);
	}
	_R<ASObject> getValueAt(int i);
	inline SWFOBJECT_TYPE getObjectType() const
	{
		return type;
	}
	inline SystemState* getSystemState() const
	{
		assert(sys);
		return sys;
	}
	void setSystemState(SystemState* s)
	{
		sys = s;
	}

	/* Implements ECMA's 9.8 ToString operation, but returns the concrete value */
	tiny_string toString();
	tiny_string toLocaleString();
	uint32_t toStringId();
	virtual int32_t toInt();
	virtual uint32_t toUInt();
	virtual int64_t toInt64();
	uint16_t toUInt16();
	/* Implements ECMA's 9.3 ToNumber operation, but returns the concrete value */
	virtual number_t toNumber();
	/* Implements ECMA's ToPrimitive (9.1) and [[DefaultValue]] (8.6.2.6) */
	_R<ASObject> toPrimitive(TP_HINT hint = NO_HINT);
	bool isPrimitive() const;

	bool isInitialized() const {return traitsInitialized;}
	virtual bool isConstructed() const;
	
	/* helper functions for calling the "valueOf" and
	 * "toString" AS-functions which may be members of this
	 *  object */
	bool has_valueOf();
	_R<ASObject> call_valueOf();
	bool has_toString();
	_R<ASObject> call_toString();
	tiny_string call_toJSON(bool &ok, std::vector<ASObject *> &path, IFunction *replacer, const tiny_string &spaces, const tiny_string &filter);

	/* Helper function for calling getClass()->getQualifiedClassName() */
	virtual tiny_string getClassName() const;

	ASFUNCTION(generator);

	/* helpers for the dynamic property 'prototype' */
	bool hasprop_prototype();
	ASObject* getprop_prototype();
	void setprop_prototype(_NR<ASObject>& prototype);

	//Comparison operators
	virtual bool isEqual(ASObject* r);
	virtual bool isEqualStrict(ASObject* r);
	virtual TRISTATE isLess(ASObject* r);

	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);

	//Enumeration handling
	virtual uint32_t nextNameIndex(uint32_t cur_index);
	virtual _R<ASObject> nextName(uint32_t index);
	virtual _R<ASObject> nextValue(uint32_t index);

	//Called when the object construction is completed. Used by MovieClip implementation
	inline virtual void constructionComplete()
	{
	}

	/**
	  Serialization interface

	  The various maps are used to implement reference type of the AMF3 spec
	*/
	virtual void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);

	virtual ASObject *describeType() const;

	virtual tiny_string toJSON(std::vector<ASObject *> &path, IFunction *replacer, const tiny_string &spaces,const tiny_string& filter);
	/* returns true if the current object is of type T */
	template<class T> bool is() const { 
		LOG(LOG_ERROR,"dynamic cast:"<<this->getClassName());
		return dynamic_cast<const T*>(this); 
	}
	/* returns this object casted to the given type.
	 * You have to make sure that it actually is the type (see is<T>() above)
	 */
	template<class T> const T* as() const { return static_cast<const T*>(this); }
	template<class T> T* as() { return static_cast<T*>(this); }

	/* Returns a debug string identifying this object */
	virtual std::string toDebugString();
	
	/* stores proxy namespace settings for internal usage */
	void setProxyProperty(const multiname& name); 
	/* applies proxy namespace settings to name for internal usage */
	void applyProxyProperty(multiname &name); 
	
	void dumpVariables() const;
	
	inline void setConstructIndicator() { constructIndicator = true; }
	inline void setConstructorCallComplete() { constructorCallComplete = true; }
	inline void setIsInitialized() { traitsInitialized=true;}
	
	void setIsEnumerable(const multiname& name, bool isEnum);
	inline void destroyContents()
	{
		if (varcount)
		{
			Variables.destroyContents();
			varcount=0;
		}
	}
	
};

class Array;
class ASQName;
class ASString;
class Bitmap;
class BitmapData;
class Boolean;
class ByteArray;
class Class_inherit;
class ColorTransform;
class ContextMenu;
class ContextMenuBuiltInItems;
class Date;
class DisplayObject;
class DisplayObjectContainer;
class Event;
class Function;
class Function_object;
class Global;
class IFunction;
class Integer;
class InteractiveObject;
class KeyboardEvent;
class LoaderInfo;
class Matrix;
class MouseEvent;
class Namespace;
class Null;
class Number;
class ObjectConstructor;
class Point;
class ProgressEvent;
class Proxy;
class Rectangle;
class RegExp;
class RootMovieClip;
class SharedObject;
class Sound;
class SoundChannel;
class SoundTransform;
class Stage;
class SyntheticFunction;
class Template_base;
class TextField;
class TextFormat;
class Type;
class UInteger;
class Undefined;
class Vector;
class WaitableEvent;
class XML;
class XMLList;


// this is used to avoid calls to dynamic_cast when testing for some classes
// keep in mind that when adding a class here you have to take care of the class inheritance and add the new SUBTYPE_ to all apropriate is<> methods 
template<> inline bool ASObject::is<Array>() const { return type==T_ARRAY; }
template<> inline bool ASObject::is<ASObject>() const { return true; }
template<> inline bool ASObject::is<ASQName>() const { return type==T_QNAME; }
template<> inline bool ASObject::is<ASString>() const { return type==T_STRING; }
template<> inline bool ASObject::is<Bitmap>() const { return subtype==SUBTYPE_BITMAP; }
template<> inline bool ASObject::is<BitmapData>() const { return subtype==SUBTYPE_BITMAPDATA; }
template<> inline bool ASObject::is<Boolean>() const { return type==T_BOOLEAN; }
template<> inline bool ASObject::is<ByteArray>() const { return subtype==SUBTYPE_BYTEARRAY; }
template<> inline bool ASObject::is<Class_base>() const { return type==T_CLASS; }
template<> inline bool ASObject::is<Class_inherit>() const { return subtype==SUBTYPE_INHERIT; }
template<> inline bool ASObject::is<ColorTransform>() const { return subtype==SUBTYPE_COLORTRANSFORM; }
template<> inline bool ASObject::is<ContextMenu>() const { return subtype==SUBTYPE_CONTEXTMENU; }
template<> inline bool ASObject::is<ContextMenuBuiltInItems>() const { return subtype==SUBTYPE_CONTEXTMENUBUILTINITEMS; }
template<> inline bool ASObject::is<Date>() const { return subtype==SUBTYPE_DATE; }
template<> inline bool ASObject::is<DisplayObject>() const { return subtype==SUBTYPE_DISPLAYOBJECT || subtype==SUBTYPE_INTERACTIVE_OBJECT || subtype==SUBTYPE_TEXTFIELD || subtype==SUBTYPE_BITMAP || subtype==SUBTYPE_DISPLAYOBJECTCONTAINER || subtype==SUBTYPE_STAGE || subtype==SUBTYPE_ROOTMOVIECLIP; }
template<> inline bool ASObject::is<DisplayObjectContainer>() const { return subtype==SUBTYPE_DISPLAYOBJECTCONTAINER || subtype==SUBTYPE_STAGE || subtype==SUBTYPE_ROOTMOVIECLIP; }
template<> inline bool ASObject::is<Event>() const { return subtype==SUBTYPE_EVENT || subtype==SUBTYPE_WAITABLE_EVENT || subtype==SUBTYPE_PROGRESSEVENT || subtype==SUBTYPE_KEYBOARD_EVENT || subtype==SUBTYPE_MOUSE_EVENT; }
template<> inline bool ASObject::is<Function_object>() const { return subtype==SUBTYPE_FUNCTIONOBJECT; }
template<> inline bool ASObject::is<Function>() const { return subtype==SUBTYPE_FUNCTION; }
template<> inline bool ASObject::is<Global>() const { return subtype==SUBTYPE_GLOBAL; }
template<> inline bool ASObject::is<IFunction>() const { return type==T_FUNCTION; }
template<> inline bool ASObject::is<Integer>() const { return type==T_INTEGER; }
template<> inline bool ASObject::is<InteractiveObject>() const { return subtype==SUBTYPE_INTERACTIVE_OBJECT || subtype==SUBTYPE_TEXTFIELD || subtype==SUBTYPE_DISPLAYOBJECTCONTAINER || subtype==SUBTYPE_STAGE || subtype==SUBTYPE_ROOTMOVIECLIP; }
template<> inline bool ASObject::is<KeyboardEvent>() const { return subtype==SUBTYPE_KEYBOARD_EVENT; }
template<> inline bool ASObject::is<LoaderInfo>() const { return subtype==SUBTYPE_LOADERINFO; }
template<> inline bool ASObject::is<Namespace>() const { return type==T_NAMESPACE; }
template<> inline bool ASObject::is<Matrix>() const { return subtype==SUBTYPE_MATRIX; }
template<> inline bool ASObject::is<MouseEvent>() const { return subtype==SUBTYPE_MOUSE_EVENT; }
template<> inline bool ASObject::is<Null>() const { return type==T_NULL; }
template<> inline bool ASObject::is<Number>() const { return type==T_NUMBER; }
template<> inline bool ASObject::is<ObjectConstructor>() const { return subtype==SUBTYPE_OBJECTCONSTRUCTOR; }
template<> inline bool ASObject::is<Point>() const { return subtype==SUBTYPE_POINT; }
template<> inline bool ASObject::is<ProgressEvent>() const { return subtype==SUBTYPE_PROGRESSEVENT; }
template<> inline bool ASObject::is<Proxy>() const { return subtype==SUBTYPE_PROXY; }
template<> inline bool ASObject::is<Rectangle>() const { return subtype==SUBTYPE_RECTANGLE; }
template<> inline bool ASObject::is<RegExp>() const { return subtype==SUBTYPE_REGEXP; }
template<> inline bool ASObject::is<RootMovieClip>() const { return subtype==SUBTYPE_ROOTMOVIECLIP; }
template<> inline bool ASObject::is<SharedObject>() const { return subtype==SUBTYPE_SHAREDOBJECT; }
template<> inline bool ASObject::is<Sound>() const { return subtype==SUBTYPE_SOUND; }
template<> inline bool ASObject::is<SoundChannel>() const { return subtype==SUBTYPE_SOUNDCHANNEL; }
template<> inline bool ASObject::is<SoundTransform>() const { return subtype==SUBTYPE_SOUNDTRANSFORM; }
template<> inline bool ASObject::is<Stage>() const { return subtype==SUBTYPE_STAGE; }
template<> inline bool ASObject::is<SyntheticFunction>() const { return subtype==SUBTYPE_SYNTHETICFUNCTION; }
template<> inline bool ASObject::is<Template_base>() const { return type==T_TEMPLATE; }
template<> inline bool ASObject::is<TextField>() const { return subtype==SUBTYPE_TEXTFIELD; }
template<> inline bool ASObject::is<TextFormat>() const { return subtype==SUBTYPE_TEXTFORMAT; }
template<> inline bool ASObject::is<Type>() const { return type==T_CLASS; }
template<> inline bool ASObject::is<UInteger>() const { return type==T_UINTEGER; }
template<> inline bool ASObject::is<Undefined>() const { return type==T_UNDEFINED; }
template<> inline bool ASObject::is<Vector>() const { return subtype==SUBTYPE_VECTOR; }
template<> inline bool ASObject::is<WaitableEvent>() const { return subtype==SUBTYPE_WAITABLE_EVENT; }
template<> inline bool ASObject::is<XML>() const { return subtype==SUBTYPE_XML; }
template<> inline bool ASObject::is<XMLList>() const { return subtype==SUBTYPE_XMLLIST; }




}
#endif /* ASOBJECT_H */
