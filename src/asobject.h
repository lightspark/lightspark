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
#include <unordered_map>
#include <boost/intrusive/list.hpp>
#include <limits>

#define ASFUNCTION_ATOM(name) \
	static void name(asAtom& ret,SystemState* sys, asAtom& , asAtom* args, const unsigned int argslen)

/* declare setter/getter and associated member variable */
#define ASPROPERTY_GETTER(type,name) \
	type name; \
	ASFUNCTION_ATOM( _getter_##name)

#define ASPROPERTY_SETTER(type,name) \
	type name; \
	ASFUNCTION_ATOM( _setter_##name)

#define ASPROPERTY_GETTER_SETTER(type, name) \
	type name; \
	ASFUNCTION_ATOM( _getter_##name); \
	ASFUNCTION_ATOM( _setter_##name)

/* declare setter/getter for already existing member variable */
#define ASFUNCTION_GETTER(name) \
	ASFUNCTION_ATOM( _getter_##name)

#define ASFUNCTION_SETTER(name) \
	ASFUNCTION_ATOM( _setter_##name)

#define ASFUNCTION_GETTER_SETTER(name) \
	ASFUNCTION_ATOM( _getter_##name); \
	ASFUNCTION_ATOM( _setter_##name)

/* general purpose body for an AS function */
#define ASFUNCTIONBODY_ATOM(c,name) \
	void c::name(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen)

/* full body for a getter declared by ASPROPERTY_GETTER or ASFUNCTION_GETTER */
#define ASFUNCTIONBODY_GETTER(c,name) \
	void c::_getter_##name(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!obj.is<c>()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		c* th = obj.as<c>(); \
		if(argslen != 0) \
			throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter"); \
		ArgumentConversionAtom<decltype(th->name)>::toAbstract(ret,sys,th->name); \
	}

#define ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(c,name) \
	void c::_getter_##name(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!obj.is<c>()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		c* th = obj.as<c>(); \
		if(argslen != 0) \
			throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter"); \
		LOG(LOG_NOT_IMPLEMENTED,obj.getObject()->getClassName() <<"."<< #name << " getter is not implemented"); \
		ArgumentConversionAtom<decltype(th->name)>::toAbstract(ret,sys,th->name); \
	}

/* full body for a getter declared by ASPROPERTY_SETTER or ASFUNCTION_SETTER */
#define ASFUNCTIONBODY_SETTER(c,name) \
	void c::_setter_##name(asAtom& ret,SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!obj.is<c>()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		c* th = obj.as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter"); \
		th->name = ArgumentConversionAtom<decltype(th->name)>::toConcrete(sys,args[0],th->name); \
	}

#define ASFUNCTIONBODY_SETTER_NOT_IMPLEMENTED(c,name) \
	void c::_setter_##name(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!obj.is<c>()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		c* th = obj.as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter"); \
		LOG(LOG_NOT_IMPLEMENTED,obj.getObject()->getClassName() <<"."<< #name << " setter is not implemented"); \
		th->name = ArgumentConversionAtom<decltype(th->name)>::toConcrete(sys,args[0],th->name); \
	}

/* full body for a getter declared by ASPROPERTY_SETTER or ASFUNCTION_SETTER.
 * After the property has been updated, the callback member function is called with the old value
 * as parameter */
#define ASFUNCTIONBODY_SETTER_CB(c,name,callback) \
	void c::_setter_##name(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!obj.is<c>()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		c* th = obj.as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter"); \
		decltype(th->name) oldValue = th->name; \
		th->name = ArgumentConversionAtom<decltype(th->name)>::toConcrete(sys,args[0],th->name); \
		th->callback(oldValue); \
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

using namespace std;
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

class asAtom
{
friend class multiname;
friend class ABCContext;

private:
	union
	{
		int32_t intval;
		uint32_t uintval;
		uint32_t stringID;
		number_t numberval;
		bool boolval;
		ASObject* closure_this; // used for T_FUNCTION objects
	};
	ASObject* objval;
	inline void decRef();
	void replaceNumber(ASObject* obj);
	void replaceBool(ASObject* obj);
	bool Boolean_concrete_string();
public:
	SWFOBJECT_TYPE type;
	asAtom():intval(0),objval(NULL),type(T_INVALID) {}
	asAtom(SWFOBJECT_TYPE _t):intval(0),objval(NULL),type(_t) {}
	asAtom(int32_t val):intval(val),objval(NULL),type(T_INTEGER) {}
	asAtom(uint32_t val):uintval(val),objval(NULL),type(T_UINTEGER) {}
	asAtom(number_t val):numberval(val),objval(NULL),type(T_NUMBER) {}
	asAtom(bool val):boolval(val),objval(NULL),type(T_BOOLEAN) {}
	ASObject* toObject(SystemState* sys);
	// returns NULL if this atom is a primitive;
	FORCE_INLINE ASObject* getObject() const { return objval; }
	FORCE_INLINE number_t getNumber() const { return numberval; }
	static FORCE_INLINE asAtom fromObject(ASObject* obj)
	{
		asAtom a;
		if (obj)
			a.replace(obj);
		return a;
	}
	
	static FORCE_INLINE asAtom fromFunction(ASObject *f, ASObject *closure);
	static FORCE_INLINE asAtom fromStringID(uint32_t sID)
	{
		asAtom a;
		a.type = T_STRING;
		a.stringID = sID;
		a.objval = NULL;
		return a;
	}
	// only use this for strings that should get an internal stringID
	static asAtom fromString(SystemState *sys, const tiny_string& s);
	FORCE_INLINE  bool isBound() const { return type == T_FUNCTION && closure_this; }
	FORCE_INLINE  ASObject* getClosure() const  { return type == T_FUNCTION ? closure_this : NULL; }
	static asAtom undefinedAtom;
	static asAtom nullAtom;
	static asAtom invalidAtom;
	static asAtom trueAtom;
	static asAtom falseAtom;
	/*
	 * Calls this function with the given object and args.
	 * if args_refcounted is true, one reference of obj and of each arg is consumed by this method.
	 * Return the asAtom the function returned.
	 * if coerceresult is false, the result of the function will not be coerced into the type provided by the method_info
	 */
	void callFunction(asAtom& ret,asAtom &obj, asAtom *args, uint32_t num_args, bool args_refcounted, bool coerceresult=true);
	// returns invalidAtom for not-primitive values
	void getVariableByMultiname(asAtom &ret, SystemState *sys, const multiname& name);
	Class_base* getClass(SystemState *sys);
	bool canCacheMethod(const multiname* name);
	void fillMultiname(SystemState *sys, multiname& name);
	void replace(ASObject* obj);
	FORCE_INLINE void set(const asAtom& a)
	{
		type = a.type;
		numberval = a.numberval;
		objval = a.objval;
	}
	bool stringcompare(SystemState* sys, uint32_t stringID);
	std::string toDebugString();
	FORCE_INLINE void applyProxyProperty(SystemState *sys, multiname& name);
	FORCE_INLINE TRISTATE isLess(SystemState *sys, asAtom& v2);
	FORCE_INLINE bool isEqual(SystemState *sys, asAtom& v2);
	FORCE_INLINE bool isEqualStrict(SystemState *sys, asAtom& v2);
	FORCE_INLINE bool isConstructed() const;
	FORCE_INLINE bool isPrimitive() const;
	FORCE_INLINE bool isNumeric() const { return (type==T_NUMBER || type==T_INTEGER || type==T_UINTEGER); }
	FORCE_INLINE bool checkArgumentConversion(const asAtom& obj) const;
	FORCE_INLINE ASObject* checkObject();
	asAtom asTypelate(asAtom& atomtype);
	FORCE_INLINE number_t toNumber();
	FORCE_INLINE int32_t toInt();
	FORCE_INLINE int32_t toIntStrict();
	FORCE_INLINE int64_t toInt64();
	FORCE_INLINE uint32_t toUInt();
	tiny_string toString(SystemState *sys);
	tiny_string toLocaleString();
	uint32_t toStringId(SystemState *sys);
	asAtom typeOf(SystemState *sys);
	FORCE_INLINE bool Boolean_concrete();
	FORCE_INLINE void convert_i();
	FORCE_INLINE void convert_u();
	FORCE_INLINE void convert_d();
	void convert_b();
	FORCE_INLINE int32_t getInt() const { assert(type == T_INTEGER); return intval; }
	FORCE_INLINE uint32_t getUInt() const{ assert(type == T_UINTEGER); return uintval; }
	FORCE_INLINE uint32_t getStringId() const{ assert(type == T_STRING); return stringID; }
	FORCE_INLINE void setInt(int32_t val);
	FORCE_INLINE void setUInt(uint32_t val);
	FORCE_INLINE void setNumber(number_t val);
	FORCE_INLINE void setBool(bool val);
	FORCE_INLINE void setNull();
	FORCE_INLINE void setUndefined();
	FORCE_INLINE void setFunction(ASObject* obj, ASObject* closure);
	FORCE_INLINE void increment();
	FORCE_INLINE void decrement();
	FORCE_INLINE void increment_i();
	FORCE_INLINE void decrement_i();
	void add(asAtom& v2, SystemState *sys, bool isrefcounted);
	FORCE_INLINE void bitnot();
	FORCE_INLINE void subtract(asAtom& v2);
	FORCE_INLINE void multiply(asAtom& v2);
	FORCE_INLINE void divide(asAtom& v2);
	FORCE_INLINE void modulo(asAtom& v2);
	FORCE_INLINE void lshift(asAtom& v1);
	FORCE_INLINE void rshift(asAtom& v1);
	FORCE_INLINE void urshift(asAtom& v1);
	FORCE_INLINE void bit_and(asAtom& v1);
	FORCE_INLINE void bit_or(asAtom& v1);
	FORCE_INLINE void bit_xor(asAtom& v1);
	FORCE_INLINE void _not();
	FORCE_INLINE void negate_i();
	FORCE_INLINE void add_i(asAtom& v2);
	FORCE_INLINE void subtract_i(asAtom& v2);
	FORCE_INLINE void multiply_i(asAtom& v2);
	template<class T> bool is() const;
	template<class T> const T* as() const { return static_cast<const T*>(this->objval); }
	template<class T> T* as() { return static_cast<T*>(this->objval); }
};
#define ASATOM_INCREF(a) if (a.getObject()) a.getObject()->incRef()
#define ASATOM_INCREF_POINTER(a) if (a->getObject()) a->getObject()->incRef()
#define ASATOM_DECREF(a) do { ASObject* b = a.getObject(); if (b && !b->getInDestruction()) b->decRef(); } while (0)
#define ASATOM_DECREF_POINTER(a) { ASObject* b = a->getObject(); if (b && !b->getInDestruction()) b->decRef(); } while (0)
struct variable
{
	asAtom var;
	union
	{
		multiname* traitTypemname;
		const Type* type;
		void* typeUnion;
	};
	asAtom setter;
	asAtom getter;
	nsNameAndKind ns;
	TRAIT_KIND kind:4;
	bool isResolved:1;
	bool isenumerable:1;
	bool issealed:1;
	bool isrefcounted:1;
	variable(TRAIT_KIND _k,const nsNameAndKind& _ns)
		: typeUnion(NULL),ns(_ns),kind(_k),isResolved(false),isenumerable(true),issealed(false),isrefcounted(true) {}
	variable(TRAIT_KIND _k, asAtom _v, multiname* _t, const Type* type, const nsNameAndKind &_ns, bool _isenumerable);
	void setVar(asAtom& v, SystemState* sys, bool _isrefcounted = true);
	/*
	 * To be used only if the value is guaranteed to be of the right type
	 */
	void setVarNoCoerce(asAtom &v);
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
	typedef std::unordered_multimap<uint32_t,variable> mapType;
	mapType Variables;
	typedef std::unordered_multimap<uint32_t,variable>::iterator var_iterator;
	typedef std::unordered_multimap<uint32_t,variable>::const_iterator const_var_iterator;
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
	// adds a dynamic variable without checking if a variable with this name already exists
	FORCE_INLINE void setDynamicVarNoCheck(uint32_t nameID,asAtom& v)
	{
		var_iterator inserted=Variables.insert(Variables.cbegin(),
				make_pair(nameID,variable(DYNAMIC_TRAIT,nsNameAndKind())));
		inserted->second.var.set(v);
	}

	/**
	 * Const version of findObjVar, useful when looking for getters
	 */
	FORCE_INLINE const variable* findObjVarConst(SystemState* sys,const multiname& mname, uint32_t traitKinds, uint32_t* nsRealId = NULL) const
	{
		if (mname.isEmpty())
			return NULL;
		uint32_t name=mname.name_type == multiname::NAME_STRING ? mname.name_s_id : mname.normalizedNameId(sys);
		assert(!mname.ns.empty());
		
		const_var_iterator ret=Variables.find(name);
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

	//
	FORCE_INLINE variable* findObjVar(SystemState* sys,const multiname& mname, uint32_t traitKinds, uint32_t* nsRealId = NULL)
	{
		if (mname.isEmpty())
			return NULL;
		uint32_t name=mname.name_type == multiname::NAME_STRING ? mname.name_s_id : mname.normalizedNameId(sys);
		bool noNS = mname.ns.empty(); // no Namespace in multiname means we don't care about the namespace and take the first match

		var_iterator ret=Variables.find(name);
		auto nsIt=mname.ns.cbegin();
		//Find the namespace
		while(ret!=Variables.cend() && ret->first==name)
		{
			//breaks when the namespace is not found
			const nsNameAndKind& ns=ret->second.ns;
			if(noNS || ns==*nsIt || (mname.hasEmptyNS && ns.hasEmptyName()) || (mname.hasBuiltinNS && ns.hasBuiltinName()))
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
	void initializeVar(const multiname& mname, asAtom &obj, multiname *typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj, uint32_t slot_id, bool isenumerable);
	void killObjVar(SystemState* sys, const multiname& mname);
	asAtom getSlot(unsigned int n);
	/*
	 * This method does throw if the slot id is not valid
	 */
	void validateSlotId(unsigned int n) const;
	void setSlot(unsigned int n,asAtom o,SystemState* sys);
	/*
	 * This version of the call is guarantee to require no type conversion
	 * this is verified at optimization time
	 */
	void setSlotNoCoerce(unsigned int n, asAtom o);
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
				std::map<const Class_base*, uint32_t>& traitsMap);
	void dumpVariables();
	void destroyContents();
};

enum GET_VARIABLE_RESULT {GETVAR_NORMAL=0x00, GETVAR_CACHEABLE=0x01, GETVAR_ISGETTER=0x02};

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
		const variable* ret=varcount ? Variables.findObjVarConst(getSystemState(),name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId):NULL;
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(ret->getter.type != T_INVALID || ret->var.type != T_INVALID))
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
				std::map<const Class_base*, uint32_t> traitsMap);
	void setClass(Class_base* c);
	static variable* findSettableImpl(SystemState* sys,variables_map& map, const multiname& name, bool* has_getter);
	static FORCE_INLINE const variable* findGettableImplConst(SystemState* sys, const variables_map& map, const multiname& name, uint32_t* nsRealId = NULL)
	{
		const variable* ret=map.findObjVarConst(sys,name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId);
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(ret->getter.type != T_INVALID || ret->var.type != T_INVALID))
				ret=NULL;
		}
		return ret;
	}
	static FORCE_INLINE variable* findGettableImpl(SystemState* sys, variables_map& map, const multiname& name, uint32_t* nsRealId = NULL)
	{
		variable* ret=map.findObjVar(sys,name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId);
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(ret->getter.type != T_INVALID || ret->var.type != T_INVALID))
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

	inline Class_base* getClass() const { return classdef; }
	ASFUNCTION_ATOM(_constructor);
	// constructor for subclasses that can't be instantiated.
	// Throws ArgumentError.
	ASFUNCTION_ATOM(_constructorNotInstantiatable);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(_toLocaleString);
	ASFUNCTION_ATOM(hasOwnProperty);
	ASFUNCTION_ATOM(valueOf);
	ASFUNCTION_ATOM(isPrototypeOf);
	ASFUNCTION_ATOM(propertyIsEnumerable);
	ASFUNCTION_ATOM(setPropertyIsEnumerable);
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

	enum GET_VARIABLE_OPTION {NONE=0x00, SKIP_IMPL=0x01, FROM_GETLEX=0x02, DONT_CALL_GETTER=0x04};

	virtual GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt=NONE)
	{
		return getVariableByMultinameIntern(ret,name,classdef,opt);
	}
	/*
	 * Helper method using the get the raw variable struct instead of calling the getter.
	 * It is used by getVariableByMultiname and by early binding code
	 */
	variable *findVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls, uint32_t* nsRealID = NULL);
	/*
	 * Gets a variable of this object. It looks through all classes (beginning at cls),
	 * then the prototype chain, and then instance variables.
	 * If the property found is a getter, it is called and its return value returned.
	 */
	GET_VARIABLE_RESULT getVariableByMultinameIntern(asAtom& ret, const multiname& name, Class_base* cls, GET_VARIABLE_OPTION opt=NONE);
	virtual int32_t getVariableByMultiname_i(const multiname& name);
	/* Simple getter interface for the common case */
	void getVariableByMultiname(asAtom& ret, const tiny_string& name, std::list<tiny_string> namespaces);
	/*
	 * Execute a AS method on this object. Returns the value
	 * returned by the function. One reference of each args[i] is
	 * consumed. The method must exist, otherwise a TypeError is
	 * thrown.
	 */
	void executeASMethod(asAtom &ret, const tiny_string& methodName, std::list<tiny_string> namespaces, asAtom *args, uint32_t num_args);
	virtual void setVariableByMultiname_i(const multiname& name, int32_t value);
	enum CONST_ALLOWED_FLAG { CONST_ALLOWED=0, CONST_NOT_ALLOWED };
	virtual void setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst)
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
	void setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, Class_base* cls);
	
	// sets dynamic variable without checking for existence
	// use it if it is guarranteed that the variable doesn't exist in this object
	FORCE_INLINE void setDynamicVariableNoCheck(uint32_t nameID, asAtom& o)
	{
		Variables.setDynamicVarNoCheck(nameID,o);
		++varcount;
	}
	/*
	 * Called by ABCVm::buildTraits to create DECLARED_TRAIT or CONSTANT_TRAIT and set their type
	 */
	void initializeVariableByMultiname(const multiname& name, asAtom& o, multiname* typemname,
			ABCContext* context, TRAIT_KIND traitKind, uint32_t slot_id, bool isenumerable);
	virtual bool deleteVariableByMultiname(const multiname& name);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true);
	void setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true);
	void setVariableByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true);
	void setVariableAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable = true);
	void setVariableAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable = true, bool isRefcounted = true);
	//NOTE: the isBorrowed flag is used to distinguish methods/setters/getters that are inside a class but on behalf of the instances
	void setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodByQName(uint32_t nameId, const nsNameAndKind& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodAtomByQName(const tiny_string& name, const tiny_string& ns, asAtom o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom f, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	virtual bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype);
	asAtom getSlot(unsigned int n)
	{
		return Variables.getSlot(n);
	}
	void setSlot(unsigned int n,asAtom o)
	{
		Variables.setSlot(n,o,getSystemState());
	}
	void setSlotNoCoerce(unsigned int n,asAtom o)
	{
		Variables.setSlotNoCoerce(n,o);
	}
	void initSlot(unsigned int n, const multiname& name);
	void initAdditionalSlots(std::vector<multiname*> additionalslots);
	unsigned int numVariables() const;
	inline tiny_string getNameAt(int i) const
	{
		return Variables.getNameAt(sys,i);
	}
	void getValueAt(asAtom &ret, int i);
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
	virtual int32_t toIntStrict() { return toInt(); }
	virtual uint32_t toUInt();
	virtual int64_t toInt64();
	uint16_t toUInt16();
	/* Implements ECMA's 9.3 ToNumber operation, but returns the concrete value */
	virtual number_t toNumber();
	/* Implements ECMA's ToPrimitive (9.1) and [[DefaultValue]] (8.6.2.6) */
	void toPrimitive(asAtom& ret,TP_HINT hint = NO_HINT);
	bool isPrimitive() const;

	bool isInitialized() const {return traitsInitialized;}
	virtual bool isConstructed() const;
	
	/* helper functions for calling the "valueOf" and
	 * "toString" AS-functions which may be members of this
	 *  object */
	bool has_valueOf();
	void call_valueOf(asAtom &ret);
	bool has_toString();
	void call_toString(asAtom &ret);
	tiny_string call_toJSON(bool &ok, std::vector<ASObject *> &path, asAtom replacer, const tiny_string &spaces, const tiny_string &filter);

	/* Helper function for calling getClass()->getQualifiedClassName() */
	virtual tiny_string getClassName() const;

	ASFUNCTION_ATOM(generator);

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
	virtual void nextName(asAtom &ret, uint32_t index);
	virtual void nextValue(asAtom &ret, uint32_t index);

	//Called when the object construction is completed. Used by MovieClip implementation
	inline virtual void constructionComplete()
	{
	}
	//Called after the object Actionscript constructor was executed. Used by MovieClip implementation
	inline virtual void afterConstruction()
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

	virtual tiny_string toJSON(std::vector<ASObject *> &path, asAtom replacer, const tiny_string &spaces,const tiny_string& filter);
	/* returns true if the current object is of type T */
	template<class T> bool is() const { 
		LOG(LOG_INFO,"dynamic cast:"<<this->getClassName());
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
	
	void dumpVariables();
	
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
	CLASS_SUBTYPE getSubtype() const { return subtype;}
};

class Activation_object;
class ApplicationDomain;
class Array;
class ASQName;
class ASString;
class Bitmap;
class BitmapData;
class Boolean;
class ByteArray;
class Class_inherit;
class ColorTransform;
class ContentElement;
class Context3D;
class ContextMenu;
class ContextMenuBuiltInItems;
class CubeTexture;
class Date;
class DisplayObject;
class DisplayObjectContainer;
class ElementFormat;
class Event;
class Function;
class Function_object;
class FontDescription;
class Global;
class IFunction;
class Integer;
class InteractiveObject;
class IndexBuffer3D;
class KeyboardEvent;
class LoaderContext;
class LoaderInfo;
class Matrix;
class Matrix3D;
class MouseEvent;
class MovieClip;
class Namespace;
class Null;
class Number;
class ObjectConstructor;
class Point;
class Program3D;
class ProgressEvent;
class Proxy;
class Rectangle;
class RectangleTexture;
class RegExp;
class RootMovieClip;
class SharedObject;
class Sound;
class SoundChannel;
class SoundTransform;
class Sprite;
class Stage;
class Stage3D;
class SyntheticFunction;
class Template_base;
class TextBlock;
class TextElement;
class TextField;
class TextFormat;
class TextLine;
class Texture;
class TextureBase;
class Type;
class UInteger;
class Undefined;
class Vector;
class Vector3D;
class VertexBuffer3D;
class VideoTexture;
class WaitableEvent;
class XML;
class XMLList;


// this is used to avoid calls to dynamic_cast when testing for some classes
// keep in mind that when adding a class here you have to take care of the class inheritance and add the new SUBTYPE_ to all apropriate is<> methods 
template<> inline bool ASObject::is<Activation_object>() const { return subtype==SUBTYPE_ACTIVATIONOBJECT; }
template<> inline bool ASObject::is<ApplicationDomain>() const { return subtype==SUBTYPE_APPLICATIONDOMAIN; }
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
template<> inline bool ASObject::is<ContentElement>() const { return subtype==SUBTYPE_CONTENTELEMENT || subtype == SUBTYPE_TEXTELEMENT; }
template<> inline bool ASObject::is<Context3D>() const { return subtype==SUBTYPE_CONTEXT3D; }
template<> inline bool ASObject::is<ContextMenu>() const { return subtype==SUBTYPE_CONTEXTMENU; }
template<> inline bool ASObject::is<ContextMenuBuiltInItems>() const { return subtype==SUBTYPE_CONTEXTMENUBUILTINITEMS; }
template<> inline bool ASObject::is<CubeTexture>() const { return subtype==SUBTYPE_CUBETEXTURE; }
template<> inline bool ASObject::is<Date>() const { return subtype==SUBTYPE_DATE; }
template<> inline bool ASObject::is<DisplayObject>() const { return subtype==SUBTYPE_DISPLAYOBJECT || subtype==SUBTYPE_INTERACTIVE_OBJECT || subtype==SUBTYPE_TEXTFIELD || subtype==SUBTYPE_BITMAP || subtype==SUBTYPE_DISPLAYOBJECTCONTAINER || subtype==SUBTYPE_STAGE || subtype==SUBTYPE_ROOTMOVIECLIP || subtype==SUBTYPE_SPRITE || subtype == SUBTYPE_MOVIECLIP || subtype == SUBTYPE_TEXTLINE; }
template<> inline bool ASObject::is<DisplayObjectContainer>() const { return subtype==SUBTYPE_DISPLAYOBJECTCONTAINER || subtype==SUBTYPE_STAGE || subtype==SUBTYPE_ROOTMOVIECLIP || subtype==SUBTYPE_SPRITE || subtype == SUBTYPE_MOVIECLIP || subtype == SUBTYPE_TEXTLINE; }
template<> inline bool ASObject::is<ElementFormat>() const { return subtype==SUBTYPE_ELEMENTFORMAT; }
template<> inline bool ASObject::is<Event>() const { return subtype==SUBTYPE_EVENT || subtype==SUBTYPE_WAITABLE_EVENT || subtype==SUBTYPE_PROGRESSEVENT || subtype==SUBTYPE_KEYBOARD_EVENT || subtype==SUBTYPE_MOUSE_EVENT; }
template<> inline bool ASObject::is<FontDescription>() const { return subtype==SUBTYPE_FONTDESCRIPTION; }
template<> inline bool ASObject::is<Function_object>() const { return subtype==SUBTYPE_FUNCTIONOBJECT; }
template<> inline bool ASObject::is<Function>() const { return subtype==SUBTYPE_FUNCTION; }
template<> inline bool ASObject::is<Global>() const { return subtype==SUBTYPE_GLOBAL; }
template<> inline bool ASObject::is<IFunction>() const { return type==T_FUNCTION; }
template<> inline bool ASObject::is<IndexBuffer3D>() const { return subtype==SUBTYPE_INDEXBUFFER3D; }
template<> inline bool ASObject::is<Integer>() const { return type==T_INTEGER; }
template<> inline bool ASObject::is<InteractiveObject>() const { return subtype==SUBTYPE_INTERACTIVE_OBJECT || subtype==SUBTYPE_TEXTFIELD || subtype==SUBTYPE_DISPLAYOBJECTCONTAINER || subtype==SUBTYPE_STAGE || subtype==SUBTYPE_ROOTMOVIECLIP || subtype==SUBTYPE_SPRITE || subtype == SUBTYPE_MOVIECLIP; }
template<> inline bool ASObject::is<KeyboardEvent>() const { return subtype==SUBTYPE_KEYBOARD_EVENT; }
template<> inline bool ASObject::is<LoaderContext>() const { return subtype==SUBTYPE_LOADERCONTEXT; }
template<> inline bool ASObject::is<LoaderInfo>() const { return subtype==SUBTYPE_LOADERINFO; }
template<> inline bool ASObject::is<Namespace>() const { return type==T_NAMESPACE; }
template<> inline bool ASObject::is<Matrix>() const { return subtype==SUBTYPE_MATRIX; }
template<> inline bool ASObject::is<Matrix3D>() const { return subtype==SUBTYPE_MATRIX3D; }
template<> inline bool ASObject::is<MouseEvent>() const { return subtype==SUBTYPE_MOUSE_EVENT; }
template<> inline bool ASObject::is<MovieClip>() const { return subtype==SUBTYPE_ROOTMOVIECLIP || subtype == SUBTYPE_MOVIECLIP; }
template<> inline bool ASObject::is<Null>() const { return type==T_NULL; }
template<> inline bool ASObject::is<Number>() const { return type==T_NUMBER; }
template<> inline bool ASObject::is<ObjectConstructor>() const { return subtype==SUBTYPE_OBJECTCONSTRUCTOR; }
template<> inline bool ASObject::is<Point>() const { return subtype==SUBTYPE_POINT; }
template<> inline bool ASObject::is<Program3D>() const { return subtype==SUBTYPE_PROGRAM3D; }
template<> inline bool ASObject::is<ProgressEvent>() const { return subtype==SUBTYPE_PROGRESSEVENT; }
template<> inline bool ASObject::is<Proxy>() const { return subtype==SUBTYPE_PROXY; }
template<> inline bool ASObject::is<Rectangle>() const { return subtype==SUBTYPE_RECTANGLE; }
template<> inline bool ASObject::is<RectangleTexture>() const { return subtype==SUBTYPE_RECTANGLETEXTURE; }
template<> inline bool ASObject::is<RegExp>() const { return subtype==SUBTYPE_REGEXP; }
template<> inline bool ASObject::is<RootMovieClip>() const { return subtype==SUBTYPE_ROOTMOVIECLIP; }
template<> inline bool ASObject::is<SharedObject>() const { return subtype==SUBTYPE_SHAREDOBJECT; }
template<> inline bool ASObject::is<Sound>() const { return subtype==SUBTYPE_SOUND; }
template<> inline bool ASObject::is<SoundChannel>() const { return subtype==SUBTYPE_SOUNDCHANNEL; }
template<> inline bool ASObject::is<SoundTransform>() const { return subtype==SUBTYPE_SOUNDTRANSFORM; }
template<> inline bool ASObject::is<Sprite>() const { return subtype==SUBTYPE_SPRITE || subtype==SUBTYPE_ROOTMOVIECLIP || subtype == SUBTYPE_MOVIECLIP; }
template<> inline bool ASObject::is<Stage>() const { return subtype==SUBTYPE_STAGE; }
template<> inline bool ASObject::is<Stage3D>() const { return subtype==SUBTYPE_STAGE3D; }
template<> inline bool ASObject::is<SyntheticFunction>() const { return subtype==SUBTYPE_SYNTHETICFUNCTION; }
template<> inline bool ASObject::is<Template_base>() const { return type==T_TEMPLATE; }
template<> inline bool ASObject::is<TextBlock>() const { return subtype==SUBTYPE_TEXTBLOCK; }
template<> inline bool ASObject::is<TextElement>() const { return subtype==SUBTYPE_TEXTELEMENT; }
template<> inline bool ASObject::is<TextField>() const { return subtype==SUBTYPE_TEXTFIELD; }
template<> inline bool ASObject::is<TextFormat>() const { return subtype==SUBTYPE_TEXTFORMAT; }
template<> inline bool ASObject::is<TextLine>() const { return subtype==SUBTYPE_TEXTLINE; }
template<> inline bool ASObject::is<Texture>() const { return subtype==SUBTYPE_TEXTURE; }
template<> inline bool ASObject::is<TextureBase>() const { return subtype==SUBTYPE_TEXTUREBASE || subtype==SUBTYPE_TEXTURE || subtype==SUBTYPE_CUBETEXTURE || subtype==SUBTYPE_RECTANGLETEXTURE || subtype==SUBTYPE_TEXTURE || subtype==SUBTYPE_VIDEOTEXTURE; }
template<> inline bool ASObject::is<Type>() const { return type==T_CLASS; }
template<> inline bool ASObject::is<UInteger>() const { return type==T_UINTEGER; }
template<> inline bool ASObject::is<Undefined>() const { return type==T_UNDEFINED; }
template<> inline bool ASObject::is<Vector>() const { return subtype==SUBTYPE_VECTOR; }
template<> inline bool ASObject::is<Vector3D>() const { return subtype==SUBTYPE_VECTOR3D; }
template<> inline bool ASObject::is<VertexBuffer3D>() const { return subtype==SUBTYPE_VERTEXBUFFER3D; }
template<> inline bool ASObject::is<VideoTexture>() const { return subtype==SUBTYPE_VIDEOTEXTURE; }
template<> inline bool ASObject::is<WaitableEvent>() const { return subtype==SUBTYPE_WAITABLE_EVENT; }
template<> inline bool ASObject::is<XML>() const { return subtype==SUBTYPE_XML; }
template<> inline bool ASObject::is<XMLList>() const { return subtype==SUBTYPE_XMLLIST; }



template<class T> inline bool asAtom::is() const {
	return objval ? objval->is<T>() : false;
}
template<> inline bool asAtom::is<Array>() const { return type==T_ARRAY; }
template<> inline bool asAtom::is<asAtom>() const { return true; }
template<> inline bool asAtom::is<ASObject>() const { return true; }
template<> inline bool asAtom::is<ASQName>() const { return type==T_QNAME; }
template<> inline bool asAtom::is<ASString>() const { return type==T_STRING; }
template<> inline bool asAtom::is<Boolean>() const { return type==T_BOOLEAN; }
template<> inline bool asAtom::is<Class_base>() const { return type==T_CLASS; }
template<> inline bool asAtom::is<IFunction>() const { return type==T_FUNCTION; }
template<> inline bool asAtom::is<Integer>() const { return type==T_INTEGER; }
template<> inline bool asAtom::is<Namespace>() const { return type==T_NAMESPACE; }
template<> inline bool asAtom::is<Null>() const { return type==T_NULL; }
template<> inline bool asAtom::is<Number>() const { return type==T_NUMBER; }
template<> inline bool asAtom::is<Type>() const { return type==T_CLASS; }
template<> inline bool asAtom::is<UInteger>() const { return type==T_UINTEGER; }
template<> inline bool asAtom::is<Undefined>() const { return type==T_UNDEFINED; }


void asAtom::decRef()
{
	if (objval && objval->decRef())
		objval = NULL;
}

static int32_t NumbertoInt(number_t val)
{
	double posInt;

	/* step 2 */
	if(std::isnan(val) || std::isinf(val) || val == 0.0)
		return 0;
	/* step 3 */
	posInt = std::floor(std::abs(val));
	/* step 4 */
	if (posInt > 4294967295.0)
		posInt = fmod(posInt, 4294967296.0);
	/* step 5 */
	if (posInt >= 2147483648.0) {
		// follow tamarin
		if(val < 0.0)
			return 0x80000000 - (int32_t)(posInt - 2147483648.0);
		else
			return 0x80000000 + (int32_t)(posInt - 2147483648.0);
	}
	return (int32_t)(val < 0.0 ? -posInt : posInt);
}
ASObject* asAtom::checkObject()
{
	if (type == T_STRING && stringID != UINT32_MAX && !objval)
		objval = (ASObject*)abstract_s(getSys(),stringID);
	assert(objval);
	return objval;
}

FORCE_INLINE int32_t asAtom::toInt()
{
	switch(type)
	{
		case T_INTEGER:
			return intval;
		case T_UINTEGER:
			return uintval;
		case T_NUMBER:
			return NumbertoInt(numberval);
		case T_BOOLEAN:
			return boolval;
		case T_UNDEFINED:
		case T_NULL:
		case T_INVALID:
			return 0;
		default:
			return checkObject()->toInt();
	}
}
FORCE_INLINE int32_t asAtom::toIntStrict()
{
	switch(type)
	{
		case T_INTEGER:
			return intval;
		case T_UINTEGER:
			return uintval;
		case T_NUMBER:
			return NumbertoInt(numberval);
		case T_BOOLEAN:
			return boolval;
		case T_UNDEFINED:
		case T_NULL:
		case T_INVALID:
			return 0;
		default:
			return checkObject()->toIntStrict();
	}
}
FORCE_INLINE number_t asAtom::toNumber()
{
	switch(type)
	{
		case T_INTEGER:
			return intval;
		case T_UINTEGER:
			return uintval;
		case T_NUMBER:
			return numberval;
		case T_BOOLEAN:
			return boolval;
		case T_NULL:
			return 0;
		case T_UNDEFINED:
			return numeric_limits<double>::quiet_NaN();
		case T_INVALID:
			return 0;
		default:
			return checkObject()->toNumber();
	}
}
FORCE_INLINE int64_t asAtom::toInt64()
{
	switch(type)
	{
		case T_UNDEFINED:
			return 0;
		case T_NULL:
			return 0;
		case T_INTEGER:
			return intval;
		case T_UINTEGER:
			return uintval;
		case T_NUMBER:
			return numberval;
		case T_BOOLEAN:
			return boolval;
		case T_INVALID:
			return 0;
		default:
			return checkObject()->toInt64();
	}
}
FORCE_INLINE uint32_t asAtom::toUInt()
{
	switch(type)
	{
		case T_UNDEFINED:
			return 0;
		case T_NULL:
			return 0;
		case T_INTEGER:
			return intval;
		case T_UINTEGER:
			return uintval;
		case T_NUMBER:
			return numberval;
		case T_BOOLEAN:
			return boolval;
		case T_INVALID:
			return 0;
		default:
			return checkObject()->toUInt();
	}
}

FORCE_INLINE void asAtom::applyProxyProperty(SystemState* sys,multiname &name)
{
	switch(type)
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_BOOLEAN:
		case T_NULL:
		case T_UNDEFINED:
		case T_INVALID:
			break;
		case T_STRING:
			if (!objval && stringID != UINT32_MAX)
				break; // no need to create string, as it won't have a proxyMultiName
			assert(objval);
			objval->applyProxyProperty(name);
			break;
		default:
			assert(objval);
			objval->applyProxyProperty(name);
			break;
	}
}
FORCE_INLINE TRISTATE asAtom::isLess(SystemState* sys,asAtom &v2)
{
	switch (type)
	{
		case T_INTEGER:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return (intval < v2.intval)?TTRUE:TFALSE;
				case T_UINTEGER:
					return (intval < 0 || ((uint32_t)intval)  < v2.uintval)?TTRUE:TFALSE;
				case T_NUMBER:
					if(std::isnan(v2.numberval))
						return TUNDEFINED;
					return (intval < v2.numberval)?TTRUE:TFALSE;
				case T_BOOLEAN:
					return (intval < v2.toInt())?TTRUE:TFALSE;
				case T_UNDEFINED:
					return TUNDEFINED;
				case T_NULL:
					return (intval < 0)?TTRUE:TFALSE;
				default:
					break;
			}
			break;
		}
		case T_UINTEGER:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return (v2.intval > 0 && (uintval < (uint32_t)v2.intval))?TTRUE:TFALSE;
				case T_UINTEGER:
					return (uintval < v2.uintval)?TTRUE:TFALSE;
				case T_NUMBER:
					if(std::isnan(v2.numberval))
						return TUNDEFINED;
					return (uintval < v2.numberval)?TTRUE:TFALSE;
				case T_BOOLEAN:
					return (uintval < v2.toUInt())?TTRUE:TFALSE;
				case T_UNDEFINED:
					return TUNDEFINED;
				case T_NULL:
					return TFALSE;
				default:
					break;
			}
			break;
		}
		case T_NUMBER:
		{
			if(std::isnan(numberval))
				return TUNDEFINED;
			switch (v2.type)
			{
				case T_INTEGER:
					return (numberval  < v2.intval)?TTRUE:TFALSE;
				case T_UINTEGER:
					return (numberval < v2.uintval)?TTRUE:TFALSE;
				case T_NUMBER:
					if(std::isnan(v2.numberval))
						return TUNDEFINED;
					return (numberval < v2.numberval)?TTRUE:TFALSE;
				case T_BOOLEAN:
					return (numberval < v2.toInt())?TTRUE:TFALSE;
				case T_UNDEFINED:
					return TUNDEFINED;
				case T_NULL:
					return (numberval < 0)?TTRUE:TFALSE;
				default:
					break;
			}
			break;
		}
		case T_BOOLEAN:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return (boolval < v2.intval)?TTRUE:TFALSE;
				case T_UINTEGER:
					return (boolval < v2.uintval)?TTRUE:TFALSE;
				case T_NUMBER:
					if(std::isnan(v2.numberval))
						return TUNDEFINED;
					return (boolval < v2.numberval)?TTRUE:TFALSE;
				case T_BOOLEAN:
					return (boolval < v2.boolval)?TTRUE:TFALSE;
				case T_UNDEFINED:
					return TUNDEFINED;
				case T_NULL:
					return (boolval)?TTRUE:TFALSE;
				default:
					break;
			}
			break;
		}
		case T_NULL:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return (0 < v2.intval)?TTRUE:TFALSE;
				case T_UINTEGER:
					return (0 < v2.uintval)?TTRUE:TFALSE;
				case T_NUMBER:
					if(std::isnan(v2.numberval))
						return TUNDEFINED;
					return (0 < v2.numberval)?TTRUE:TFALSE;
				case T_BOOLEAN:
					return (0 < v2.boolval)?TTRUE:TFALSE;
				case T_UNDEFINED:
					return TUNDEFINED;
				case T_NULL:
					return TFALSE;
				default:
					break;
			}
			break;
		}
		case T_UNDEFINED:
			return TUNDEFINED;
		case T_INVALID:
			return TUNDEFINED;
		default:
			break;
	}
	return toObject(sys)->isLess(v2.toObject(sys));
}
FORCE_INLINE bool asAtom::isEqual(SystemState *sys, asAtom &v2)
{
	switch (type)
	{
		case T_INTEGER:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return intval==v2.toInt();
				case T_UINTEGER:
					return intval >= 0 && intval==v2.toInt();
				case T_NUMBER:
					return intval==v2.toNumber();
				case T_BOOLEAN:
					return intval==v2.toInt();
				case T_STRING:
					return intval==v2.toNumber();
				case T_NULL:
				case T_UNDEFINED:
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case T_UINTEGER:
		{
			switch (v2.type)
			{
				case T_INTEGER:
					return v2.intval >= 0 && uintval==v2.toUInt();
				case T_UINTEGER:
				case T_NUMBER:
				case T_STRING:
				case T_BOOLEAN:
					return uintval==v2.toUInt();
				case T_NULL:
				case T_UNDEFINED:
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case T_NUMBER:
		{
			switch (v2.type)
			{
				case T_INTEGER:
				case T_UINTEGER:
				case T_BOOLEAN:
					return toNumber()==v2.toNumber();
				case T_NUMBER:
				case T_STRING:
					return toNumber()==v2.toNumber();
				case T_NULL:
				case T_UNDEFINED:
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case T_BOOLEAN:
		{
			switch (v2.type)
			{
				case T_BOOLEAN:
					return boolval==v2.boolval;
				case T_STRING:
					if ((!v2.objval && v2.stringID == UINT32_MAX) || (v2.objval && !v2.objval->isConstructed()))
						return false;
					return boolval==v2.toNumber();
				case T_INTEGER:
				case T_UINTEGER:
				case T_NUMBER:
					return boolval==v2.toNumber();
				case T_NULL:
				case T_UNDEFINED:
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case T_NULL:
		{
			switch (v2.type)
			{
				case T_NULL:
				case T_UNDEFINED:
					return true;
				case T_INTEGER:
				case T_UINTEGER:
				case T_NUMBER:
				case T_BOOLEAN:
					return false;
				case T_FUNCTION:
					if (!v2.objval->isConstructed())
						return true;
					return false;
				case T_STRING:
					if ((!v2.objval && v2.stringID == UINT32_MAX) || (v2.objval && !v2.objval->isConstructed()))
						return true;
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case T_UNDEFINED:
		{
			switch (v2.type)
			{
				case T_UNDEFINED:
				case T_NULL:
					return true;
				case T_NUMBER:
				case T_INTEGER:
				case T_UINTEGER:
				case T_BOOLEAN:
					return false;
				case T_FUNCTION:
					if (!v2.objval->isConstructed())
						return true;
					return false;
				case T_STRING:
					if ((!v2.objval && v2.stringID == UINT32_MAX) || (v2.objval && !v2.objval->isConstructed()))
						return true;
					return false;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
		}
		case T_FUNCTION:
		{
			switch (v2.type)
			{
				case T_FUNCTION:
					if (closure_this != NULL && v2.closure_this != NULL && closure_this != v2.closure_this)
						return false;
					return v2.toObject(sys)->isEqual(this->toObject(sys));
				default:
					return false;
			}
		}
		case T_INVALID:
			return false;
		case T_STRING:
		{
			if (stringID != UINT32_MAX)
			{
				switch (v2.type)
				{
					case T_NULL:
					case T_UNDEFINED:
						return false;
					case T_STRING:
						if (v2.stringID != UINT32_MAX)
							return v2.stringID == stringID;
						break;
					default:
						break;
				}
			}
			else
			{
				switch (v2.type)
				{
					case T_STRING:
						if (v2.stringID != UINT32_MAX)
							return stringcompare(sys,v2.stringID);
						break;
					default:
						break;
				}
			}
			break;
		}
		default:
			break;
	}
	return toObject(sys)->isEqual(v2.toObject(sys));
}

FORCE_INLINE bool asAtom::isEqualStrict(SystemState *sys, asAtom &v2)
{
	if(type!=v2.type)
	{
		//Type conversions are ok only for numeric types
		switch(type)
		{
			case T_NUMBER:
			case T_INTEGER:
			case T_UINTEGER:
				break;
			case T_NULL:
				if (!v2.isConstructed() && v2.type!=T_CLASS)
					return true;
				return false;
			default:
				return false;
		}
		switch(v2.type)
		{
			case T_NUMBER:
			case T_INTEGER:
			case T_UINTEGER:
				break;
			case T_NULL:
				if (!isConstructed() && type!=T_CLASS)
					return true;
				return false;
			default:
				return false;
		}
	}
	return isEqual(sys,v2);
	
}

FORCE_INLINE bool asAtom::isConstructed() const
{
	switch(type)
	{
		case T_INTEGER:
		case T_UINTEGER:
		case T_NUMBER:
		case T_BOOLEAN:
		case T_NULL:
		case T_UNDEFINED:
			return true;
		case T_INVALID:
			return false;
		case T_STRING:
			return stringID != UINT32_MAX || objval->isConstructed();
		default:
			assert(objval);
			return objval->isConstructed();
	}
}

FORCE_INLINE bool asAtom::isPrimitive() const
{
	// ECMA 3, section 4.3.2, T_INTEGER and T_UINTEGER are added
	// because they are special cases of Number
	return type==T_NUMBER || type ==T_UNDEFINED || type == T_NULL ||
		type==T_STRING || type==T_BOOLEAN || type==T_INTEGER ||
			type==T_UINTEGER;
}
FORCE_INLINE bool asAtom::checkArgumentConversion(const asAtom& obj) const
{
	if (type == obj.type)
		return true;
	if ((type==T_NUMBER || type==T_INTEGER || type==T_UINTEGER) &&
		(obj.type==T_NUMBER || obj.type==T_INTEGER || obj.type==T_UINTEGER))
		return true;
	return false;
}

FORCE_INLINE void asAtom::setInt(int32_t val)
{
	type = T_INTEGER;
	intval = val;
	objval = NULL;
}

FORCE_INLINE void asAtom::setUInt(uint32_t val)
{
	type = T_UINTEGER;
	uintval = val;
	objval = NULL;
}

FORCE_INLINE void asAtom::setNumber(number_t val)
{
	type = T_NUMBER;
	numberval = val;
	objval = NULL;
}

FORCE_INLINE void asAtom::setBool(bool val)
{
	type = T_BOOLEAN;
	boolval = val;
	objval = NULL;
}

FORCE_INLINE void asAtom::setNull()
{
	type = T_NULL;
	objval = NULL;
}
FORCE_INLINE void asAtom::setUndefined()
{
	type = T_UNDEFINED;
	objval = NULL;
}
FORCE_INLINE void asAtom::setFunction(ASObject* obj, ASObject* closure)
{
	type = obj->getObjectType(); // type may be T_CLASS or T_FUNCTION
	objval = obj;
	closure_this = closure;
}
FORCE_INLINE void asAtom::increment()
{
	switch(type)
	{
		case T_UNDEFINED:
			setNumber(numeric_limits<double>::quiet_NaN());
			break;
		case T_NULL:
			setInt(1);
			break;
		case T_INTEGER:
			setInt(intval+1);
			break;
		case T_UINTEGER:
			setUInt(uintval+1);
			break;
		case T_NUMBER:
			setNumber(numberval+1);
			break;
		case T_BOOLEAN:
			setInt((boolval ? 1 : 0)+1);
			break;
		default:
		{
			number_t n=checkObject()->toNumber();
			objval->decRef();
			setNumber(n+1);
			break;
		}
	}
}

FORCE_INLINE void asAtom::decrement()
{
	switch(type)
	{
		case T_UNDEFINED:
			setNumber(numeric_limits<double>::quiet_NaN());
			break;
		case T_NULL:
			setInt(-1);
			break;
		case T_INTEGER:
			setInt(intval-1);
			break;
		case T_UINTEGER:
		{
			if (uintval > 0)
				setUInt(uintval-1);
			else
			{
				number_t val = uintval;
				setNumber(val-1);
			}
			break;
		}
		case T_NUMBER:
			setNumber(numberval-1);
			break;
		case T_BOOLEAN:
			setInt((boolval ? 1 : 0)-1);
			break;
		default:
		{
			number_t n=checkObject()->toNumber();
			objval->decRef();
			setNumber(n-1);
			break;
		}
	}

}

FORCE_INLINE void asAtom::increment_i()
{
	setInt(toInt()+1);
}
FORCE_INLINE void asAtom::decrement_i()
{
	setInt(toInt()-1);
}

FORCE_INLINE void asAtom::convert_i()
{
	if (type == T_INTEGER)
		return;
	int32_t v = toIntStrict();
	decRef();
	setInt(v);
}

FORCE_INLINE void asAtom::convert_u()
{
	if (type == T_UINTEGER)
		return;
	uint32_t v = toUInt();
	decRef();
	setUInt(v);
}

FORCE_INLINE void asAtom::convert_d()
{
	if (type == T_NUMBER || type == T_INTEGER || type == T_UINTEGER)
		return;
	number_t v = toNumber();
	decRef();
	setNumber(v);
}

FORCE_INLINE void asAtom::bit_xor(asAtom &v1)
{
	int32_t i1=v1.toInt();
	int32_t i2=toInt();
	ASATOM_DECREF(v1);
	decRef();
	LOG_CALL(_("bitXor ") << std::hex << i1 << '^' << i2 << std::dec);
	setInt(i1^i2);
}

FORCE_INLINE void asAtom::bitnot()
{
	int32_t i1=toInt();
	decRef();
	LOG_CALL(_("bitNot ") << std::hex << i1 << std::dec);
	setInt(~i1);
}

FORCE_INLINE void asAtom::subtract(asAtom &v2)
{
	if( (type == T_INTEGER || type == T_UINTEGER) &&
		(v2.type == T_INTEGER || v2.type ==T_UINTEGER))
	{
		int64_t num1=toInt64();
		int64_t num2=v2.toInt64();
	
		decRef();
		ASATOM_DECREF(v2);
		LOG_CALL(_("subtractI ") << num1 << '-' << num2);
		int64_t res = num1-num2;
		if (res > INT32_MIN && res < INT32_MAX)
			setInt(res);
		else if (res >= 0 && res < UINT32_MAX)
			setUInt(res);
		else
			setNumber(res);
	}
	else
	{
		number_t num2=v2.toNumber();
		number_t num1=toNumber();
	
		decRef();
		ASATOM_DECREF(v2);
		LOG_CALL(_("subtract ") << num1 << '-' << num2);
		setNumber(num1-num2);
	}
}

FORCE_INLINE void asAtom::multiply(asAtom &v2)
{
	if( (type == T_INTEGER || type == T_UINTEGER) &&
		(v2.type == T_INTEGER || v2.type ==T_UINTEGER))
	{
		int64_t num1=toInt64();
		int64_t num2=v2.toInt64();
	
		decRef();
		ASATOM_DECREF(v2);
		LOG_CALL(_("multiplyI ") << num1 << '*' << num2);
		int64_t res = num1*num2;
		if (res > INT32_MIN && res < INT32_MAX)
			setInt(res);
		else if (res >= 0 && res < UINT32_MAX)
			setUInt(res);
		else
			setNumber(res);
	}
	else
	{
		double num1=v2.toNumber();
		double num2=toNumber();
		LOG_CALL(_("multiply ")  << num1 << '*' << num2);
	
		setNumber(num1*num2);
	}
}

FORCE_INLINE void asAtom::divide(asAtom &v2)
{
	double num1=toNumber();
	double num2=v2.toNumber();

	decRef();
	ASATOM_DECREF(v2);
	LOG_CALL(_("divide ")  << num1 << '/' << num2);
	// handling of infinity according to ECMA-262, chapter 11.5.2
	if (std::isinf(num1))
	{
		if (std::isinf(num2) || std::isnan(num2))
			setNumber(numeric_limits<double>::quiet_NaN());
		else
		{
			bool needssign = (std::signbit(num1) || std::signbit(num2)) && !(std::signbit(num1) && std::signbit(num2)); 
			setNumber( needssign  ? -numeric_limits<double>::infinity() : numeric_limits<double>::infinity());
		}
	}
	else
		setNumber(num1/num2);
}

FORCE_INLINE void asAtom::modulo(asAtom &v2)
{
	// if both values are Integers the result is also an int
	if( ((type == T_INTEGER) || (type == T_UINTEGER)) &&
		((v2.type == T_INTEGER) || (v2.type == T_UINTEGER)))
	{
		int32_t num1=toInt();
		int32_t num2=v2.toInt();
		LOG_CALL(_("moduloI ")  << num1 << '%' << num2);
		if (num2 == 0)
			setNumber(numeric_limits<double>::quiet_NaN());
		else
			setInt(num1%num2);
	}
	else
	{
		number_t num1=toNumber();
		number_t num2=v2.toNumber();

		decRef();
		ASATOM_DECREF(v2);
		LOG_CALL(_("modulo ")  << num1 << '%' << num2);
		/* fmod returns NaN if num2 == 0 as the spec mandates */
		setNumber(::fmod(num1,num2));
	}
}

FORCE_INLINE void asAtom::lshift(asAtom &v1)
{
	int32_t i2=toInt();
	uint32_t i1=v1.toUInt()&0x1f;
	ASATOM_DECREF(v1);
	decRef();
	LOG_CALL(_("lShift ")<<std::hex<<i2<<_("<<")<<i1<<std::dec);
	//Left shift are supposed to always work in 32bit
	setInt(i2<<i1);
}

FORCE_INLINE void asAtom::rshift(asAtom &v1)
{
	int32_t i2=toInt();
	uint32_t i1=v1.toUInt()&0x1f;
	ASATOM_DECREF(v1);
	decRef();
	LOG_CALL(_("rShift ")<<std::hex<<i2<<_(">>")<<i1<<std::dec);
	setInt(i2>>i1);
}

FORCE_INLINE void asAtom::urshift(asAtom &v1)
{
	uint32_t i2=toUInt();
	uint32_t i1=v1.toUInt()&0x1f;
	ASATOM_DECREF(v1);
	decRef();
	LOG_CALL(_("urShift ")<<std::hex<<i2<<_(">>")<<i1<<std::dec);
	setUInt(i2>>i1);
}
FORCE_INLINE void asAtom::bit_and(asAtom &v1)
{
	int32_t i1=v1.toInt();
	int32_t i2=toInt();
	ASATOM_DECREF(v1);
	decRef();
	LOG_CALL(_("bitAnd_oo ") << std::hex << i1 << '&' << i2 << std::dec);
	setInt(i1&i2);
}
FORCE_INLINE void asAtom::bit_or(asAtom &v1)
{
	int32_t i1=v1.toInt();
	int32_t i2=toInt();
	LOG_CALL(_("bitOr ") << std::hex << i1 << '|' << i2 << std::dec);
	setInt(i1|i2);
}
FORCE_INLINE void asAtom::_not()
{
	LOG_CALL( _("not:") <<this->toDebugString()<<" "<<!Boolean_concrete());
	
	bool ret=!Boolean_concrete();
	decRef();
	setBool(ret);
}

FORCE_INLINE void asAtom::negate_i()
{
	LOG_CALL(_("negate_i"));
	int n=toInt();
	decRef();
	setInt(-n);
}

FORCE_INLINE void asAtom::add_i(asAtom &v2)
{
	int32_t num2=v2.toInt();
	int32_t num1=toInt();

	decRef();
	ASATOM_DECREF(v2);
	LOG_CALL(_("add_i ") << num1 << '+' << num2);
	setInt(num1+num2);
}

FORCE_INLINE void asAtom::subtract_i(asAtom &v2)
{
	int num2=v2.toInt();
	int num1=toInt();

	decRef();
	ASATOM_DECREF(v2);
	LOG_CALL(_("subtract_i ") << num1 << '-' << num2);
	setInt(num1-num2);
}

FORCE_INLINE void asAtom::multiply_i(asAtom &v2)
{
	int num1=toInt();
	int num2=v2.toInt();
	decRef();
	ASATOM_DECREF(v2);
	LOG_CALL(_("multiply ")  << num1 << '*' << num2);
	setInt(num1*num2);
}
FORCE_INLINE ASObject *asAtom::toObject(SystemState *sys)
{
	if (objval)
		return objval;
	switch(type)
	{
		case T_INTEGER:
			// ints are internally treated as numbers, so create a Number instance
			objval = abstract_di(sys,intval);
			break;
		case T_UINTEGER:
			// uints are internally treated as numbers, so create a Number instance
			objval = abstract_di(sys,uintval);
			break;
		case T_NUMBER:
			objval = abstract_d(sys,numberval);
			break;
		case T_BOOLEAN:
			return abstract_b(sys,boolval);
		case T_NULL:
			return abstract_null(sys);
		case T_UNDEFINED:
			return abstract_undefined(sys);
		case T_STRING:
			if (stringID != UINT32_MAX)
				objval = abstract_s(sys,stringID);
			break;
		case T_INVALID:
			LOG(LOG_ERROR,"calling toObject on invalid asAtom, should not happen");
			return objval;
		default:
			break;
	}
	return objval;
}
FORCE_INLINE void asAtom::replace(ASObject *obj)
{
	assert(obj);
	type = obj->getObjectType();
	switch(type)
	{
		case T_INTEGER:
			intval = obj->toInt();
			break;
		case T_UINTEGER:
			uintval = obj->toUInt();
			break;
		case T_NUMBER:
			replaceNumber(obj);
			break;
		case T_BOOLEAN:
			replaceBool(obj);
			break;
		case T_FUNCTION:
			closure_this = NULL;
			break;
		case T_STRING:
			stringID = UINT32_MAX;
			break;
		default:
			break;
	}
	objval = obj;
}
FORCE_INLINE asAtom asAtom::fromFunction(ASObject *f, ASObject *closure)
{
	asAtom a;
	a.type = f->getObjectType();
	a.objval = f;
	a.closure_this = closure;
	return a;
}
/* implements ecma3's ToBoolean() operation, see section 9.2, but returns the value instead of an Boolean object */
FORCE_INLINE bool asAtom::Boolean_concrete()
{
	switch(type)
	{
		case T_UNDEFINED:
		case T_NULL:
			return false;
		case T_BOOLEAN:
			return boolval;
		case T_NUMBER:
			return numberval != 0.0 && !std::isnan(numberval);
		case T_INTEGER:
			return intval != 0;
		case T_UINTEGER:
			return uintval != 0;
		case T_STRING:
			if (stringID != UINT32_MAX && !objval)
				return stringID != BUILTIN_STRINGS::EMPTY;
			if (!objval->isConstructed())
				return false;
			return Boolean_concrete_string();
		case T_FUNCTION:
		case T_ARRAY:
		case T_OBJECT:
			assert(objval);
			// not constructed objects return false
			if (!objval->isConstructed())
				return false;
			return true;
		default:
			//everything else is an Object regarding to the spec
			return true;
	}
}
}
#endif /* ASOBJECT_H */
