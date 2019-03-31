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

#define ASFUNCTIONBODY_GETTER_STATIC(c,name) \
	void c::_getter_##name(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(obj.getObject() != Class<c>::getRef(sys).getPtr()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		if(argslen != 0) \
			throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter"); \
		ArgumentConversionAtom<decltype(sys->static_##c##_##name)>::toAbstract(ret,sys,sys->static_##c##_##name); \
	}

#define ASFUNCTIONBODY_GETTER_STRINGID(c,name) \
	void c::_getter_##name(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!obj.is<c>()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		c* th = obj.as<c>(); \
		if(argslen != 0) \
			throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter"); \
		ret = asAtom::fromStringID(th->name); \
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
#define ASFUNCTIONBODY_SETTER_STATIC(c,name) \
	void c::_setter_##name(asAtom& ret,SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(obj.getObject() != Class<c>::getRef(sys).getPtr()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter"); \
		sys->static_##c##_##name = ArgumentConversionAtom<decltype(sys->static_##c##_##name)>::toConcrete(sys,args[0],sys->static_##c##_##name); \
	}

#define ASFUNCTIONBODY_SETTER_STRINGID(c,name) \
	void c::_setter_##name(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!obj.is<c>()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		c* th = obj.as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter"); \
		th->name = args[0].toStringId(sys); \
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

#define ASFUNCTIONBODY_SETTER_STRINGID_CB(c,name,callback) \
	void c::_setter_##name(asAtom& ret, SystemState* sys, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!obj.is<c>()) \
			throw Class<ArgumentError>::getInstanceS(sys,"Function applied to wrong object"); \
		c* th = obj.as<c>(); \
		if(argslen != 1) \
			throw Class<ArgumentError>::getInstanceS(sys,"Arguments provided in getter"); \
		decltype(th->name) oldValue = th->name; \
		th->name = args[0].toStringId(sys); \
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

#define ASFUNCTIONBODY_GETTER_SETTER_STATIC(c,name) \
		ASFUNCTIONBODY_GETTER_STATIC(c,name) \
		ASFUNCTIONBODY_SETTER_STATIC(c,name)

#define ASFUNCTIONBODY_GETTER_SETTER_STRINGID(c,name) \
		ASFUNCTIONBODY_GETTER_STRINGID(c,name) \
		ASFUNCTIONBODY_SETTER_STRINGID(c,name)

#define ASFUNCTIONBODY_GETTER_SETTER_STRINGID_CB(c,name,callback) \
		ASFUNCTIONBODY_GETTER_STRINGID(c,name) \
		ASFUNCTIONBODY_SETTER_STRINGID_CB(c,name,callback)

/* registers getter/setter with Class_base. To be used in ::sinit()-functions */
#define REGISTER_GETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(c->getSystemState(),_getter_##name),GETTER_METHOD,true)

#define REGISTER_SETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(c->getSystemState(),_setter_##name),SETTER_METHOD,true)

#define REGISTER_GETTER_SETTER(c,name) \
		REGISTER_GETTER(c,name); \
		REGISTER_SETTER(c,name)

#define REGISTER_GETTER_STATIC(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(c->getSystemState(),_getter_##name),GETTER_METHOD,false)

#define REGISTER_SETTER_STATIC(c,name) \
	c->setDeclaredMethodByQName(#name,"",Class<IFunction>::getFunction(c->getSystemState(),_setter_##name),SETTER_METHOD,false)

#define REGISTER_GETTER_SETTER_STATIC(c,name) \
		REGISTER_GETTER_STATIC(c,name); \
		REGISTER_SETTER_STATIC(c,name)

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
class SyntheticFunction;
class SoundTransform;

extern SystemState* getSys();
enum TRAIT_KIND { NO_CREATE_TRAIT=0, DECLARED_TRAIT=1, DYNAMIC_TRAIT=2, INSTANCE_TRAIT=5, CONSTANT_TRAIT=9 /* constants are also declared traits */ };
enum GET_VARIABLE_RESULT {GETVAR_NORMAL=0x00, GETVAR_CACHEABLE=0x01, GETVAR_ISGETTER=0x02, GETVAR_ISCONSTANT=0x04};
enum GET_VARIABLE_OPTION {NONE=0x00, SKIP_IMPL=0x01, FROM_GETLEX=0x02, DONT_CALL_GETTER=0x04, NO_INCREF=0x08};

class asAtom
{
friend class multiname;
friend class ABCContext;

private:
	enum ATOM_TYPE { ATOM_INVALID=0, ATOM_OBJECT=1, ATOM_STRINGID=2, ATOM_UNDEFINED_NULL_BOOL=3,ATOM_INTEGER=4, ATOM_UINTEGER=5, ATOM_NUMBER=6 };
	union
	{
		int32_t intval;
		uint32_t uintval;
		uint32_t stringID;
		ASObject* objval;
	};
	ATOM_TYPE type;
	inline void decRef();
	void replaceBool(ASObject* obj);
	bool Boolean_concrete_string();
public:
	asAtom():objval(nullptr),type(ATOM_INVALID) {}
	asAtom(SWFOBJECT_TYPE _t):objval(nullptr)
	{
		switch (_t)
		{
			case T_INVALID:
				type=ATOM_INVALID;
				break;
			case T_UNDEFINED:
				type=ATOM_UNDEFINED_NULL_BOOL;
				uintval = 0x40;
				break;
			case T_NULL:
				type=ATOM_UNDEFINED_NULL_BOOL;
				uintval = 0x80;
				break;
			case T_BOOLEAN:
				type=ATOM_UNDEFINED_NULL_BOOL;
				break;
			case T_NUMBER:
				type=ATOM_NUMBER;
				break;
			case T_INTEGER:
				type=ATOM_INTEGER;
				break;
			case T_UINTEGER:
				type=ATOM_UINTEGER;
				break;
			default:
				type=ATOM_OBJECT;
				break;
		}
	}
	asAtom(int32_t val):intval(val),type(ATOM_INTEGER) {}
	asAtom(uint32_t val):uintval(val),type(ATOM_UINTEGER) {}
	asAtom(SystemState* sys, number_t val);
	asAtom(bool val):uintval(val ? 0x100 : 0),type(ATOM_UNDEFINED_NULL_BOOL) {}
	ASObject* toObject(SystemState* sys);
	// returns NULL if this atom is a primitive;
	FORCE_INLINE ASObject* getObject() const; 
	static FORCE_INLINE asAtom fromObject(ASObject* obj)
	{
		asAtom a;
		if (obj)
			a.replace(obj);
		return a;
	}
	
	static FORCE_INLINE asAtom fromStringID(uint32_t sID)
	{
		asAtom a;
		a.type = ATOM_STRINGID;
		a.stringID = sID;
		return a;
	}
	// only use this for strings that should get an internal stringID
	static asAtom fromString(SystemState *sys, const tiny_string& s);
	ASObject* getClosure() const;
	asAtom getClosureAtom(asAtom defaultAtom=asAtom::nullAtom) const;
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
	void callFunction(asAtom& ret, asAtom &obj, asAtom *args, uint32_t num_args, bool args_refcounted, bool coerceresult=true);
	// returns invalidAtom for not-primitive values
	void getVariableByMultiname(asAtom &ret, SystemState *sys, const multiname& name);
	Class_base* getClass(SystemState *sys);
	bool canCacheMethod(const multiname* name);
	void fillMultiname(SystemState *sys, multiname& name);
	void replace(ASObject* obj)
	{
		type = ATOM_OBJECT;
		objval = obj;
	}
	FORCE_INLINE void set(const asAtom& a)
	{
		type = a.type;
		objval = a.objval;
	}
	bool stringcompare(SystemState* sys, uint32_t stringID);
	bool functioncompare(SystemState* sys,asAtom& v2);
	std::string toDebugString();
	FORCE_INLINE void applyProxyProperty(SystemState *sys, multiname& name);
	FORCE_INLINE TRISTATE isLess(SystemState *sys, asAtom& v2);
	FORCE_INLINE bool isEqual(SystemState *sys, asAtom& v2);
	FORCE_INLINE bool isEqualStrict(SystemState *sys, asAtom& v2);
	FORCE_INLINE bool isConstructed() const;
	FORCE_INLINE bool isPrimitive() const;
	FORCE_INLINE bool isNumeric() const; 
	FORCE_INLINE bool isNumber() const; 
	FORCE_INLINE bool isValid() const { return type != ATOM_INVALID; }
	FORCE_INLINE bool isInvalid() const { return type == ATOM_INVALID; }
	FORCE_INLINE bool isInteger() const;
	FORCE_INLINE bool isUInteger() const;
	FORCE_INLINE bool isBool() const;
	FORCE_INLINE bool isObject() const { return type == ATOM_OBJECT; }
	FORCE_INLINE bool isFunction() const;
	FORCE_INLINE bool isString() const;
	FORCE_INLINE bool isStringID() const { return type == ATOM_STRINGID; }
	FORCE_INLINE bool isQName() const;
	FORCE_INLINE bool isNamespace() const;
	FORCE_INLINE bool isArray() const;
	FORCE_INLINE bool isClass() const;
	FORCE_INLINE bool isTemplate() const;
	FORCE_INLINE bool isNull() const;
	FORCE_INLINE bool isUndefined() const;
	FORCE_INLINE SWFOBJECT_TYPE getObjectType() const;
	FORCE_INLINE bool checkArgumentConversion(const asAtom& obj) const;
	FORCE_INLINE ASObject* checkObject();
	asAtom asTypelate(asAtom& atomtype);
	FORCE_INLINE number_t toNumber();
	FORCE_INLINE number_t AVM1toNumber(int swfversion);
	FORCE_INLINE bool AVM1toBool();
	FORCE_INLINE int32_t toInt();
	FORCE_INLINE int32_t toIntStrict();
	FORCE_INLINE int64_t toInt64();
	FORCE_INLINE uint32_t toUInt();
	tiny_string toString(SystemState *sys);
	tiny_string toLocaleString();
	uint32_t toStringId(SystemState *sys);
	asAtom typeOf(SystemState *sys);
	bool Boolean_concrete();
	FORCE_INLINE void convert_i();
	FORCE_INLINE void convert_u();
	FORCE_INLINE void convert_d(SystemState* sys);
	void convert_b();
	FORCE_INLINE int32_t getInt() const { assert(type == ATOM_INTEGER); return intval; }
	FORCE_INLINE uint32_t getUInt() const{ assert(type == ATOM_UINTEGER); return uintval; }
	FORCE_INLINE uint32_t getStringId() const{ assert(type == ATOM_STRINGID); return stringID; }
	FORCE_INLINE void setInt(int32_t val);
	FORCE_INLINE void setUInt(uint32_t val);
	void setNumber(SystemState* sys,number_t val);
	FORCE_INLINE void setBool(bool val);
	FORCE_INLINE void setNull();
	FORCE_INLINE void setUndefined();
	void setFunction(ASObject* obj, ASObject* closure);
	FORCE_INLINE void increment(SystemState* sys);
	FORCE_INLINE void decrement(SystemState* sys);
	FORCE_INLINE void increment_i();
	FORCE_INLINE void decrement_i();
	void add(asAtom& v2, SystemState *sys, bool isrefcounted);
	FORCE_INLINE void bitnot();
	FORCE_INLINE void subtract(SystemState* sys,asAtom& v2);
	FORCE_INLINE void multiply(SystemState* sys,asAtom& v2);
	FORCE_INLINE void divide(SystemState* sys,asAtom& v2);
	FORCE_INLINE void modulo(SystemState* sys,asAtom& v2);
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
#define ASATOM_DECREF(a) do { ASObject* b = a.getObject(); if (b && !b->getConstant() && !b->getInDestruction()) b->decRef(); } while (0)
#define ASATOM_DECREF_POINTER(a) { ASObject* b = a->getObject(); if (b && !b->getConstant() && !b->getInDestruction()) b->decRef(); } while (0)
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
	// indicates if this map was initialized with no variables with non-primitive values
	bool cloneable;
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
	TRAIT_KIND getSlotKind(unsigned int n);
	uint32_t findInstanceSlotByMultiname(multiname* name);
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
	uint32_t getNameAt(unsigned int i) const;
	variable* getValueAt(unsigned int i);
	int getNextEnumerable(unsigned int i) const;
	~variables_map();
	void check() const;
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
	void dumpVariables();
	void destroyContents();
	bool cloneInstance(variables_map& map);
	void removeAllDeclaredProperties();
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
friend class asAtom;
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
			if(!(ret->getter.isValid() || ret->var.isValid()))
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
				std::map<const Class_base*, uint32_t> traitsMap,bool usedynamicPropertyWriter=true);
	void setClass(Class_base* c);
	static variable* findSettableImpl(SystemState* sys,variables_map& map, const multiname& name, bool* has_getter);
	static FORCE_INLINE const variable* findGettableImplConst(SystemState* sys, const variables_map& map, const multiname& name, uint32_t* nsRealId = NULL)
	{
		const variable* ret=map.findObjVarConst(sys,name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId);
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(ret->getter.isValid() || ret->var.isValid()))
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
			if(!(ret->getter.isValid() || ret->var.isValid()))
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
	ASFUNCTION_ATOM(addProperty);
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

	virtual GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt=NONE)
	{
		return getVariableByMultinameIntern(ret,name,classdef,opt);
	}
	/*
	 * Helper method using the get the raw variable struct instead of calling the getter.
	 * It is used by getVariableByMultiname and by early binding code
	 */
	variable *findVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt, Class_base* cls, uint32_t* nsRealID = nullptr, bool* isborrowed=nullptr);
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
	virtual multiname* setVariableByMultiname(const multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst)
	{
		return setVariableByMultiname(name,o,allowConst,classdef);
	}
	/*
	 * Sets  variable of this object. It looks through all classes (beginning at cls),
	 * then the prototype chain, and then instance variables.
	 * If the property found is a setter, it is called with the given 'o'.
	 * If no property is found, an instance variable is created.
	 * Setting CONSTANT_TRAIT is only allowed if allowConst is true
	 */
	multiname* setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, Class_base* cls);
	
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
	TRAIT_KIND getSlotKind(unsigned int n)
	{
		return Variables.getSlotKind(n);
	}
	void setSlot(unsigned int n,asAtom o)
	{
		Variables.setSlot(n,o,getSystemState());
		if (o.is<SyntheticFunction>())
			checkFunctionScope(o.getObject());
	}
	void setSlotNoCoerce(unsigned int n,asAtom o)
	{
		Variables.setSlotNoCoerce(n,o);
	}
	uint32_t findInstanceSlotByMultiname(multiname* name)
	{
		return Variables.findInstanceSlotByMultiname(name);
	}
	void initSlot(unsigned int n, const multiname& name);
	
	void initAdditionalSlots(std::vector<multiname*> additionalslots);
	unsigned int numVariables() const;
	inline uint32_t getNameAt(int i) const
	{
		return Variables.getNameAt(i);
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
	inline bool getConstructIndicator() const { return constructIndicator; }

	
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
	// copies all variables into the target
	// returns false if cloning is not possible
	bool cloneInstance(ASObject* target);
	
	/*!
	 * \brief checks for circular dependencies within the function scope
	 * \param o the function to check
	 */
	void checkFunctionScope(ASObject *o);
	
	virtual asAtom getVariableBindingValue(const tiny_string &name);
};

class AVM1Function;
class Activation_object;
class ApplicationDomain;
class Array;
class ASMutex;
class ASQName;
class ASString;
class ASWorker;
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
class NetStream;
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
class Sprite;
class Stage;
class Stage3D;
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
class WorkerDomain;
class XML;
class XMLList;


// this is used to avoid calls to dynamic_cast when testing for some classes
// keep in mind that when adding a class here you have to take care of the class inheritance and add the new SUBTYPE_ to all apropriate is<> methods 
template<> inline bool ASObject::is<AVM1Function>() const { return subtype==SUBTYPE_AVM1FUNCTION; }
template<> inline bool ASObject::is<Activation_object>() const { return subtype==SUBTYPE_ACTIVATIONOBJECT; }
template<> inline bool ASObject::is<ApplicationDomain>() const { return subtype==SUBTYPE_APPLICATIONDOMAIN; }
template<> inline bool ASObject::is<Array>() const { return type==T_ARRAY; }
template<> inline bool ASObject::is<ASMutex>() const { return subtype==SUBTYPE_MUTEX; }
template<> inline bool ASObject::is<ASObject>() const { return true; }
template<> inline bool ASObject::is<ASQName>() const { return type==T_QNAME; }
template<> inline bool ASObject::is<ASString>() const { return type==T_STRING; }
template<> inline bool ASObject::is<ASWorker>() const { return subtype==SUBTYPE_WORKER; }
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
template<> inline bool ASObject::is<NetStream>() const { return subtype==SUBTYPE_NETSTREAM; }
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
template<> inline bool ASObject::is<WorkerDomain>() const { return subtype==SUBTYPE_WORKERDOMAIN; }
template<> inline bool ASObject::is<XML>() const { return subtype==SUBTYPE_XML; }
template<> inline bool ASObject::is<XMLList>() const { return subtype==SUBTYPE_XMLLIST; }



template<class T> inline bool asAtom::is() const {
	return type == ATOM_OBJECT && objval ? objval->is<T>() : false;
}
template<> inline bool asAtom::is<asAtom>() const { return true; }
template<> inline bool asAtom::is<ASObject>() const { return true; }
template<> inline bool asAtom::is<ASString>() const { return type==ATOM_STRINGID || (type==ATOM_OBJECT && objval && objval->is<ASString>()); }
template<> inline bool asAtom::is<Boolean>() const { return (type==ATOM_UNDEFINED_NULL_BOOL && ((uintval & 0xff) == 0)) || (type==ATOM_OBJECT && objval && objval->is<Boolean>()); }
template<> inline bool asAtom::is<Integer>() const { return type==ATOM_INTEGER || (type==ATOM_OBJECT && objval && objval->is<Integer>()); }
template<> inline bool asAtom::is<Null>() const { return (type==ATOM_UNDEFINED_NULL_BOOL && ((uintval & 0xff) == 0x80))|| (type==ATOM_OBJECT && objval && objval->is<Null>()); }
template<> inline bool asAtom::is<Number>() const { return type==ATOM_NUMBER || (type==ATOM_OBJECT && objval && objval->is<Number>()); }
template<> inline bool asAtom::is<UInteger>() const { return type==ATOM_UINTEGER || (type==ATOM_OBJECT && objval && objval->is<UInteger>()); }
template<> inline bool asAtom::is<Undefined>() const { return (type==ATOM_UNDEFINED_NULL_BOOL && ((uintval & 0xff) == 0x40))|| (type==ATOM_OBJECT && objval && objval->is<Undefined>()); }


void asAtom::decRef()
{
	if ((type==ATOM_NUMBER || type==ATOM_OBJECT) && objval)
		objval->decRef();
}

ASObject* asAtom::checkObject()
{
	assert(objval);
	return objval;
}

FORCE_INLINE int32_t asAtom::toInt()
{
	switch(type)
	{
		case ATOM_INTEGER:
			return intval;
		case ATOM_UINTEGER:
			return uintval;
		case ATOM_UNDEFINED_NULL_BOOL:
			return (uintval&0x100)>>8;
		case ATOM_INVALID:
			return 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(getSys(),stringID);
			int32_t ret = s->toInt();
			s->decRef();
			return ret;
		}
		default:
			return checkObject()->toInt();
	}
}
FORCE_INLINE int32_t asAtom::toIntStrict()
{
	switch(type)
	{
		case ATOM_INTEGER:
			return intval;
		case ATOM_UINTEGER:
			return uintval;
		case ATOM_UNDEFINED_NULL_BOOL:
			return (uintval&0x100)>>8;
		case ATOM_INVALID:
			return 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(getSys(),stringID);
			int32_t ret = s->toIntStrict();
			s->decRef();
			return ret;
		}
		default:
			return checkObject()->toIntStrict();
	}
}
FORCE_INLINE number_t asAtom::toNumber()
{
	switch(type)
	{
		case ATOM_INTEGER:
			return intval;
		case ATOM_UINTEGER:
			return uintval;
		case ATOM_NUMBER:
			return checkObject()->toNumber();
		case ATOM_UNDEFINED_NULL_BOOL:
			return uintval&0x40 ? numeric_limits<double>::quiet_NaN() : (uintval&0x100)>>8;
		case ATOM_INVALID:
			return 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(getSys(),stringID);
			number_t ret = s->toNumber();
			s->decRef();
			return ret;
		}
		default:
			return checkObject()->toNumber();
	}
}
FORCE_INLINE number_t asAtom::AVM1toNumber(int swfversion)
{
	switch(type)
	{
		case ATOM_INTEGER:
			return intval;
		case ATOM_UINTEGER:
			return uintval;
		case ATOM_UNDEFINED_NULL_BOOL:
			return uintval&0x40 && swfversion >= 7 ? numeric_limits<double>::quiet_NaN() : (uintval&0x100)>>8;
		case ATOM_INVALID:
			return 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(getSys(),stringID);
			number_t ret = s->toNumber();
			s->decRef();
			return ret;
		}
		default:
			return checkObject()->toNumber();
	}
}
FORCE_INLINE bool asAtom::AVM1toBool()
{
	switch(type)
	{
		case ATOM_INTEGER:
			return intval;
		case ATOM_UINTEGER:
			return uintval;
		case ATOM_NUMBER:
			return toNumber();
		case ATOM_UNDEFINED_NULL_BOOL:
			return (uintval&0x100)>>8;
		default:
			return true;
	}
}

FORCE_INLINE int64_t asAtom::toInt64()
{
	switch(type)
	{
		case ATOM_INTEGER:
			return intval;
		case ATOM_UINTEGER:
			return uintval;
		case ATOM_UNDEFINED_NULL_BOOL:
			return (uintval&0x100)>>8;
		case ATOM_INVALID:
			return 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(getSys(),stringID);
			int64_t ret = s->toInt64();
			s->decRef();
			return ret;
		}
		default:
			return checkObject()->toInt64();
	}
}
FORCE_INLINE uint32_t asAtom::toUInt()
{
	switch(type)
	{
		case ATOM_INTEGER:
			return intval;
		case ATOM_UINTEGER:
			return uintval;
		case ATOM_UNDEFINED_NULL_BOOL:
			return (uintval&0x100)>>8;
		case ATOM_INVALID:
			return 0;
		case ATOM_STRINGID:	
		{
			ASObject* s = abstract_s(getSys(),stringID);
			uint32_t ret = s->toUInt();
			s->decRef();
			return ret;
		}
		default:
			return checkObject()->toUInt();
	}
}

FORCE_INLINE void asAtom::applyProxyProperty(SystemState* sys,multiname &name)
{
	switch(type)
	{
		case ATOM_INTEGER:
		case ATOM_UINTEGER:
		case ATOM_NUMBER:
		case ATOM_UNDEFINED_NULL_BOOL:
		case ATOM_INVALID:
			break;
		case ATOM_STRINGID:
			break; // no need to create string, as it won't have a proxyMultiName
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
		case ATOM_INTEGER:
		{
			switch (v2.type)
			{
				case ATOM_INTEGER:
					return (intval < v2.intval)?TTRUE:TFALSE;
				case ATOM_UINTEGER:
					return (intval < 0 || ((uint32_t)intval)  < v2.uintval)?TTRUE:TFALSE;
				case ATOM_NUMBER:
					if(std::isnan(v2.toNumber()))
						return TUNDEFINED;
					return (intval < v2.toNumber())?TTRUE:TFALSE;
				case ATOM_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0xf0)
					{
						case 0x80: // NULL
							return (intval < 0)?TTRUE:TFALSE;
						case 0x40: // UNDEFINED
							return TUNDEFINED;
						default: // BOOL
							return (intval < (int32_t)((v2.uintval&0x100)>>8))?TTRUE:TFALSE;
					}
				}
				default:
					break;
			}
			break;
		}
		case ATOM_UINTEGER:
		{
			switch (v2.type)
			{
				case ATOM_INTEGER:
					return (v2.intval > 0 && (uintval < (uint32_t)v2.intval))?TTRUE:TFALSE;
				case ATOM_UINTEGER:
					return (uintval < v2.uintval)?TTRUE:TFALSE;
				case ATOM_NUMBER:
					if(std::isnan(v2.toNumber()))
						return TUNDEFINED;
					return (uintval < v2.toNumber())?TTRUE:TFALSE;
				case ATOM_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0xf0)
					{
						case 0x80: // NULL
							return TFALSE;
						case 0x40: // UNDEFINED
							return TUNDEFINED;
						default: // BOOL
							return (uintval < (v2.uintval&0x100)>>8)?TTRUE:TFALSE;
					}
				}
				default:
					break;
			}
			break;
		}
		case ATOM_NUMBER:
		{
			if(std::isnan(toNumber()))
				return TUNDEFINED;
			switch (v2.type)
			{
				case ATOM_INTEGER:
					return (toNumber()  < v2.intval)?TTRUE:TFALSE;
				case ATOM_UINTEGER:
					return (toNumber() < v2.uintval)?TTRUE:TFALSE;
				case ATOM_NUMBER:
					if(std::isnan(v2.toNumber()))
						return TUNDEFINED;
					return (toNumber() < v2.toNumber())?TTRUE:TFALSE;
				case ATOM_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0xf0)
					{
						case 0x80: // NULL
							return (toNumber() < 0)?TTRUE:TFALSE;
						case 0x40: // UNDEFINED
							return TUNDEFINED;
						default: // BOOL
							return (toNumber() < v2.toInt())?TTRUE:TFALSE;
					}
				}
				default:
					break;
			}
			break;
		}
		case ATOM_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0xf0)
			{
				case 0x80: // NULL
				{
					switch (v2.type)
					{
						case ATOM_INTEGER:
							return (0 < v2.intval)?TTRUE:TFALSE;
						case ATOM_UINTEGER:
							return (0 < v2.uintval)?TTRUE:TFALSE;
						case ATOM_NUMBER:
							if(std::isnan(v2.toNumber()))
								return TUNDEFINED;
							return (0 < v2.toNumber())?TTRUE:TFALSE;
						case ATOM_UNDEFINED_NULL_BOOL:
							switch (v2.uintval&0xf0)
							{
								case 0x80: // NULL
									return TFALSE;
								case 0x40: // UNDEFINED
									return TUNDEFINED;
								default: // BOOL
									return (0 < (v2.uintval&0x100)>>8)?TTRUE:TFALSE;
							}
						default:
							break;
					}
					break;
				}
				case 0x40: // UNDEFINED
					return TUNDEFINED;
				default: // BOOL
				{
					switch (v2.type)
					{
						case ATOM_INTEGER:
							return ((int32_t)(uintval&0x100)>>8 < v2.intval)?TTRUE:TFALSE;
						case ATOM_UINTEGER:
							return ((uintval&0x100)>>8 < v2.uintval)?TTRUE:TFALSE;
						case ATOM_NUMBER:
							if(std::isnan(v2.toNumber()))
								return TUNDEFINED;
							return ((uintval&0x100)>>8 < v2.toNumber())?TTRUE:TFALSE;
						case ATOM_UNDEFINED_NULL_BOOL:
							switch (v2.uintval&0xf0)
							{
								case 0x80: // NULL
									return ((uintval&0x100)>>8)?TTRUE:TFALSE;
								case 0x40: // UNDEFINED
									return TUNDEFINED;
								default: // BOOL
									return ((uintval&0x100)>>8 < (v2.uintval&0x100)>>8)?TTRUE:TFALSE;
							}
						default:
							break;
					}
					break;
				}
			}
			break;
		}
		case ATOM_INVALID:
			return TUNDEFINED;
		case ATOM_STRINGID:
		{
			if (stringID < BUILTIN_STRINGS_CHAR_MAX)
			{
				switch (v2.type)
				{
					case ATOM_STRINGID:
						if (v2.stringID < BUILTIN_STRINGS_CHAR_MAX)
							return (stringID < v2.stringID)?TTRUE:TFALSE;
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
	return toObject(sys)->isLess(v2.toObject(sys));
}
FORCE_INLINE bool asAtom::isEqual(SystemState *sys, asAtom &v2)
{
	switch (type)
	{
		case ATOM_INTEGER:
		{
			switch (v2.type)
			{
				case ATOM_INTEGER:
					return intval==v2.toInt();
				case ATOM_UINTEGER:
					return intval >= 0 && intval==v2.toInt();
				case ATOM_NUMBER:
					return intval==v2.toNumber();
				case ATOM_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0xf0)
					{
						case 0x80: // NULL
							return false;
						case 0x40: // UNDEFINED
							return false;
						default: // BOOL
							return intval==v2.toInt();
					}
				}
				case ATOM_STRINGID:
					return intval==v2.toNumber();
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case ATOM_UINTEGER:
		{
			switch (v2.type)
			{
				case ATOM_INTEGER:
					return v2.intval >= 0 && uintval==v2.toUInt();
				case ATOM_UINTEGER:
				case ATOM_NUMBER:
				case ATOM_STRINGID:
					return uintval==v2.toUInt();
				case ATOM_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0xf0)
					{
						case 0x80: // NULL
							return false;
						case 0x40: // UNDEFINED
							return false;
						default: // BOOL
							return uintval==v2.toUInt();
					}
				}
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case ATOM_NUMBER:
		{
			switch (v2.type)
			{
				case ATOM_INTEGER:
				case ATOM_UINTEGER:
					return toNumber()==v2.toNumber();
				case ATOM_NUMBER:
					return toNumber() == v2.toNumber();
				case ATOM_STRINGID:
					return toNumber()==v2.toNumber();
				case ATOM_UNDEFINED_NULL_BOOL:
					switch (v2.uintval&0xf0)
					{
						case 0x80: // NULL
							return false;
						case 0x40: // UNDEFINED
							return false;
						default: // BOOL
							return toNumber()==v2.toNumber();
					}
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case ATOM_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0xf0)
			{
				case 0x80: // NULL
				case 0x40: // UNDEFINED
					switch (v2.type)
					{
						case ATOM_UNDEFINED_NULL_BOOL:
						{
							switch (v2.uintval&0xf0)
							{
								case 0x80: // NULL
									return true;
								case 0x40: // UNDEFINED
									return true;
								default: // BOOL
									return false;
							}
						}
						case ATOM_INTEGER:
						case ATOM_UINTEGER:
						case ATOM_NUMBER:
							return false;
						case ATOM_STRINGID:
							return false;
						default:
							return v2.toObject(sys)->isEqual(this->toObject(sys));
					}
				default: // BOOL
					switch (v2.type)
					{
						case ATOM_STRINGID:
							return (bool)((uintval&0x100)>>8)==v2.toNumber();
						case ATOM_INTEGER:
						case ATOM_UINTEGER:
						case ATOM_NUMBER:
							return (bool)((uintval&0x100)>>8)==v2.toNumber();
						case ATOM_UNDEFINED_NULL_BOOL:
						{
							switch (v2.uintval&0xf0)
							{
								case 0x80: // NULL
									return false;
								case 0x40: // UNDEFINED
									return false;
								default: // BOOL
									return (uintval&0x100)>>8==(v2.uintval&0x100)>>8;
							}
						}
						default:
							return v2.toObject(sys)->isEqual(this->toObject(sys));
					}
			}
			break;
		}
		case ATOM_INVALID:
			return false;
		case ATOM_STRINGID:
		{
			switch (v2.type)
			{
				case ATOM_UNDEFINED_NULL_BOOL:
				{
					switch (v2.uintval&0xf0)
					{
						case 0x80: // NULL
							return false;
						case 0x40: // UNDEFINED
							return false;
						default: // BOOL
							break;
					}
					break;
				}
				case ATOM_STRINGID:
					return v2.stringID == stringID;
				default:
					return v2.toObject(sys)->isEqual(this->toObject(sys));
			}
			break;
		}
		case ATOM_OBJECT:
		{
			if (this->is<IFunction>())
			{
				if (v2.is<IFunction>())
					return functioncompare(sys,v2);
				else
					return false;
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
	if(getObjectType()!=v2.getObjectType())
	{
		//Type conversions are ok only for numeric types
		switch(type)
		{
			case ATOM_NUMBER:
			case ATOM_INTEGER:
			case ATOM_UINTEGER:
				break;
			case ATOM_UNDEFINED_NULL_BOOL:
			{
				switch (uintval&0xf0)
				{
					case 0x80: // NULL
						if (!v2.isConstructed() && v2.getObjectType()!=T_CLASS)
							return true;
						return false;
					case 0x40: // UNDEFINED
					default: // BOOL
						return false;
				}
				break;
			}
			case ATOM_OBJECT:
				if (isNumber())
					break;
				return false;
			default:
				return false;
		}
		switch(v2.type)
		{
			case ATOM_NUMBER:
			case ATOM_INTEGER:
			case ATOM_UINTEGER:
				break;
			case ATOM_UNDEFINED_NULL_BOOL:
			{
				switch (v2.uintval&0xf0)
				{
					case 0x80: // NULL
						if (!isConstructed() && getObjectType()!=T_CLASS)
							return true;
						return false;
					case 0x40: // UNDEFINED
					default: // BOOL
						return false;
				}
				break;
			}
			case ATOM_OBJECT:
				if (v2.isNumber())
					break;
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
		case ATOM_INTEGER:
		case ATOM_UINTEGER:
		case ATOM_NUMBER:
		case ATOM_UNDEFINED_NULL_BOOL:
			return true;
		case ATOM_INVALID:
			return false;
		case ATOM_STRINGID:
			return true;
		default:
			assert(objval);
			return objval->isConstructed();
	}
}

FORCE_INLINE bool asAtom::isPrimitive() const
{
	// ECMA 3, section 4.3.2, T_INTEGER and T_UINTEGER are added
	// because they are special cases of Number
	return isUndefined() || isNull() || isNumber() || isString() || isInteger()|| isUInteger() || isBool();
}
FORCE_INLINE bool asAtom::checkArgumentConversion(const asAtom& obj) const
{
	if (type == obj.type)
	{
		if (type == ATOM_OBJECT)
			return objval->getObjectType() == obj.objval->getObjectType();
		return true;
	}
	if ((type==ATOM_NUMBER || type==ATOM_INTEGER || type==ATOM_UINTEGER) &&
		(obj.type==ATOM_NUMBER || obj.isInteger() || obj.type==ATOM_UINTEGER))
		return true;
	return false;
}

FORCE_INLINE void asAtom::setInt(int32_t val)
{
	type = ATOM_INTEGER;
	intval = val;
}

FORCE_INLINE void asAtom::setUInt(uint32_t val)
{
	type = ATOM_UINTEGER;
	uintval = val;
}

FORCE_INLINE void asAtom::setBool(bool val)
{
	type = ATOM_UNDEFINED_NULL_BOOL;
	uintval = val ? 0x100 : 0;
}

FORCE_INLINE void asAtom::setNull()
{
	type = ATOM_UNDEFINED_NULL_BOOL;
	uintval = 0x80;
}
FORCE_INLINE void asAtom::setUndefined()
{
	type = ATOM_UNDEFINED_NULL_BOOL;
	uintval = 0x40;
}
FORCE_INLINE void asAtom::increment(SystemState* sys)
{
	switch(type)
	{
		case ATOM_UNDEFINED_NULL_BOOL:
			switch (uintval&0xf0)
			{
				case 0x80: // NULL
					setInt(1);
					break;
				case 0x40: // UNDEFINED
					setNumber(sys,numeric_limits<double>::quiet_NaN());
					break;
				default: // BOOL
					setInt((uintval & 0x100 ? 1 : 0)+1);
					break;
			}
			break;
		case ATOM_INTEGER:
			setInt(intval+1);
			break;
		case ATOM_UINTEGER:
			setUInt(uintval+1);
			break;
		case ATOM_NUMBER:
			setNumber(sys,checkObject()->toNumber()+1);
			break;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(sys,stringID);
			number_t n = s->toNumber();
			setNumber(sys,n+1);
			s->decRef();
			break;
		}
		default:
		{
			number_t n=checkObject()->toNumber();
			objval->decRef();
			setNumber(sys,n+1);
			break;
		}
	}
}

FORCE_INLINE void asAtom::decrement(SystemState* sys)
{
	switch(type)
	{
		case ATOM_UNDEFINED_NULL_BOOL:
			switch (uintval&0xf0)
			{
				case 0x80: // NULL
					setInt(-1);
					break;
				case 0x40: // UNDEFINED
					setNumber(sys,numeric_limits<double>::quiet_NaN());
					break;
				default: // BOOL
					setInt((uintval & 0x100 ? 1 : 0)-1);
					break;
			}
			break;
		case ATOM_INTEGER:
			setInt(intval-1);
			break;
		case ATOM_UINTEGER:
		{
			if (uintval > 0)
				setUInt(uintval-1);
			else
			{
				number_t val = uintval;
				setNumber(sys,val-1);
			}
			break;
		}
		case ATOM_NUMBER:
			setNumber(sys,checkObject()->toNumber()-1);
			break;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(sys,stringID);
			number_t n = s->toNumber();
			setNumber(sys,n-1);
			s->decRef();
			break;
		}
		default:
		{
			number_t n=checkObject()->toNumber();
			objval->decRef();
			setNumber(sys,n-1);
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
	if (type == ATOM_INTEGER)
		return;
	int32_t v = toIntStrict();
	decRef();
	setInt(v);
}

FORCE_INLINE void asAtom::convert_u()
{
	if (type == ATOM_UINTEGER)
		return;
	uint32_t v = toUInt();
	decRef();
	setUInt(v);
}

FORCE_INLINE void asAtom::convert_d(SystemState* sys)
{
	if (type == ATOM_NUMBER || type == ATOM_INTEGER || type == ATOM_UINTEGER)
		return;
	number_t v = toNumber();
	decRef();
	setNumber(sys,v);
}

FORCE_INLINE void asAtom::bit_xor(asAtom &v1)
{
	int32_t i1=v1.toInt();
	int32_t i2=toInt();
	LOG_CALL(_("bitXor ") << std::hex << i1 << '^' << i2 << std::dec);
	setInt(i1^i2);
}

FORCE_INLINE void asAtom::bitnot()
{
	int32_t i1=toInt();
	LOG_CALL(_("bitNot ") << std::hex << i1 << std::dec);
	setInt(~i1);
}

FORCE_INLINE void asAtom::subtract(SystemState* sys,asAtom &v2)
{
	if( (type == ATOM_INTEGER || type == ATOM_UINTEGER) &&
		(v2.isInteger() || v2.type ==ATOM_UINTEGER))
	{
		int64_t num1=toInt64();
		int64_t num2=v2.toInt64();
	
		LOG_CALL(_("subtractI ") << num1 << '-' << num2);
		int64_t res = num1-num2;
		if (res > INT32_MIN && res < INT32_MAX)
			setInt(res);
		else if (res >= 0 && res < UINT32_MAX)
			setUInt(res);
		else
			setNumber(sys,res);
	}
	else
	{
		number_t num2=v2.toNumber();
		number_t num1=toNumber();
	
		LOG_CALL(_("subtract ") << num1 << '-' << num2);
		setNumber(sys,num1-num2);
	}
}

FORCE_INLINE void asAtom::multiply(SystemState* sys,asAtom &v2)
{
	if( (type == ATOM_INTEGER || type == ATOM_UINTEGER) &&
		(v2.isInteger() || v2.type ==ATOM_UINTEGER))
	{
		int64_t num1=toInt64();
		int64_t num2=v2.toInt64();
	
		LOG_CALL(_("multiplyI ") << num1 << '*' << num2);
		int64_t res = num1*num2;
		if (res > INT32_MIN && res < INT32_MAX)
			setInt(res);
		else if (res >= 0 && res < UINT32_MAX)
			setUInt(res);
		else
			setNumber(sys,res);
	}
	else
	{
		double num1=v2.toNumber();
		double num2=toNumber();
		LOG_CALL(_("multiply ")  << num1 << '*' << num2);
	
		setNumber(sys,num1*num2);
	}
}

FORCE_INLINE void asAtom::divide(SystemState* sys,asAtom &v2)
{
	double num1=toNumber();
	double num2=v2.toNumber();

	LOG_CALL(_("divide ")  << num1 << '/' << num2);
	// handling of infinity according to ECMA-262, chapter 11.5.2
	if (std::isinf(num1))
	{
		if (std::isinf(num2) || std::isnan(num2))
			setNumber(sys,numeric_limits<double>::quiet_NaN());
		else
		{
			bool needssign = (std::signbit(num1) || std::signbit(num2)) && !(std::signbit(num1) && std::signbit(num2)); 
			setNumber(sys, needssign  ? -numeric_limits<double>::infinity() : numeric_limits<double>::infinity());
		}
	}
	else
		setNumber(sys,num1/num2);
}

FORCE_INLINE void asAtom::modulo(SystemState* sys,asAtom &v2)
{
	// if both values are Integers the result is also an int
	if( (type == ATOM_INTEGER || type == ATOM_UINTEGER) &&
		(v2.isInteger() || v2.type ==ATOM_UINTEGER))
	{
		int32_t num1=toInt();
		int32_t num2=v2.toInt();
		LOG_CALL(_("moduloI ")  << num1 << '%' << num2);
		if (num2 == 0)
			setNumber(sys,numeric_limits<double>::quiet_NaN());
		else
			setInt(num1%num2);
	}
	else
	{
		number_t num1=toNumber();
		number_t num2=v2.toNumber();

		LOG_CALL(_("modulo ")  << num1 << '%' << num2);
		/* fmod returns NaN if num2 == 0 as the spec mandates */
		setNumber(sys,::fmod(num1,num2));
	}
}

FORCE_INLINE void asAtom::lshift(asAtom &v1)
{
	int32_t i2=toInt();
	uint32_t i1=v1.toUInt()&0x1f;
	LOG_CALL(_("lShift ")<<std::hex<<i2<<_("<<")<<i1<<std::dec);
	//Left shift are supposed to always work in 32bit
	setInt(i2<<i1);
}

FORCE_INLINE void asAtom::rshift(asAtom &v1)
{
	int32_t i2=toInt();
	uint32_t i1=v1.toUInt()&0x1f;
	LOG_CALL(_("rShift ")<<std::hex<<i2<<_(">>")<<i1<<std::dec);
	setInt(i2>>i1);
}

FORCE_INLINE void asAtom::urshift(asAtom &v1)
{
	uint32_t i2=toUInt();
	uint32_t i1=v1.toUInt()&0x1f;
	LOG_CALL(_("urShift ")<<std::hex<<i2<<_(">>")<<i1<<std::dec);
	setUInt(i2>>i1);
}
FORCE_INLINE void asAtom::bit_and(asAtom &v1)
{
	int32_t i1=v1.toInt();
	int32_t i2=toInt();
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

	LOG_CALL(_("add_i ") << num1 << '+' << num2);
	setInt(num1+num2);
}

FORCE_INLINE void asAtom::subtract_i(asAtom &v2)
{
	int num2=v2.toInt();
	int num1=toInt();

	LOG_CALL(_("subtract_i ") << num1 << '-' << num2);
	setInt(num1-num2);
}

FORCE_INLINE void asAtom::multiply_i(asAtom &v2)
{
	int num1=toInt();
	int num2=v2.toInt();
	LOG_CALL(_("multiply ")  << num1 << '*' << num2);
	setInt(num1*num2);
}
FORCE_INLINE ASObject *asAtom::toObject(SystemState *sys)
{
	if (type == ATOM_OBJECT || type == ATOM_NUMBER)
	{
		assert(objval && objval->getRefCount() >= 1);
		return objval;
	}
	switch(type)
	{
		case ATOM_INTEGER:
			// ints are internally treated as numbers, so create a Number instance
			objval = abstract_di(sys,intval);
			break;
		case ATOM_UINTEGER:
			// uints are internally treated as numbers, so create a Number instance
			objval = abstract_di(sys,uintval);
			break;
		case ATOM_UNDEFINED_NULL_BOOL:
		{
			switch (uintval&0xf0)
			{
				case 0x80: // NULL
					objval = abstract_null(sys);
					break;
				case 0x40: // UNDEFINED
					objval = abstract_undefined(sys);
					break;
				default: // BOOL
					objval = abstract_b(sys,(uintval & 0x100)>>8);
					break;
			}
			break;
		}
		case ATOM_STRINGID:
			objval = abstract_s(sys,stringID);
			break;
		default:
			throw RunTimeException("calling toObject on invalid asAtom, should not happen");
			break;
	}
	type = ATOM_OBJECT;
	return objval;
}

FORCE_INLINE ASObject* asAtom::getObject() const 
{
	return type == ATOM_OBJECT || type == ATOM_NUMBER ? objval : nullptr;
}

FORCE_INLINE bool asAtom::isNumeric() const
{
	return (type==ATOM_INTEGER || type==ATOM_UINTEGER || isNumber());
}
FORCE_INLINE bool asAtom::isNumber() const
{
	return (type==ATOM_NUMBER ||
			(type==ATOM_OBJECT 
				&& objval
				&& (
					objval->is<Integer>() ||
					objval->is<UInteger>() ||
					objval->is<Number>()
				))); 
}
FORCE_INLINE SWFOBJECT_TYPE asAtom::getObjectType() const
{
	switch (type)
	{
		case ATOM_INTEGER:
			return T_INTEGER;
		case ATOM_UINTEGER:
			return T_UINTEGER;
		case ATOM_UNDEFINED_NULL_BOOL:
			switch (uintval&0xf0)
			{
				case 0x80: // NULL
					return T_NULL;
				case 0x40: // UNDEFINED
					return T_UNDEFINED;
				default: // BOOL
					return T_BOOLEAN;
			}
		case ATOM_NUMBER:
			return T_NUMBER;
		case ATOM_STRINGID:
			return T_STRING;
		case ATOM_OBJECT:
			return objval->getObjectType();
		default:
			return T_INVALID;
	}
}
FORCE_INLINE bool asAtom::isFunction() const { return type == ATOM_OBJECT && objval && objval->is<IFunction>(); }
FORCE_INLINE bool asAtom::isString() const { return type == ATOM_STRINGID || (type == ATOM_OBJECT && objval && objval->is<ASString>()); }
FORCE_INLINE bool asAtom::isQName() const { return type == ATOM_OBJECT && objval->is<ASQName>(); }
FORCE_INLINE bool asAtom::isNamespace() const { return type == ATOM_OBJECT && objval && objval->is<Namespace>(); }
FORCE_INLINE bool asAtom::isArray() const { return type == ATOM_OBJECT && objval && objval->is<Array>(); }
FORCE_INLINE bool asAtom::isClass() const { return type == ATOM_OBJECT && objval && objval->is<Class_base>(); }
FORCE_INLINE bool asAtom::isTemplate() const { return type == ATOM_OBJECT && objval && objval->getObjectType() == T_TEMPLATE; }
FORCE_INLINE bool asAtom::isNull() const { return (type == ATOM_UNDEFINED_NULL_BOOL && ((uintval & 0xf0) == 0x80)) || (type == ATOM_OBJECT && objval && objval->getObjectType() == T_NULL); }
FORCE_INLINE bool asAtom::isUndefined() const { return (type == ATOM_UNDEFINED_NULL_BOOL && ((uintval & 0xf0) == 0x40)) || (type == ATOM_OBJECT && objval && objval->getObjectType() == T_UNDEFINED); }
FORCE_INLINE bool asAtom::isInteger() const { return type == ATOM_INTEGER || (type == ATOM_OBJECT && objval && objval->getObjectType() == T_INTEGER); }
FORCE_INLINE bool asAtom::isUInteger() const { return type == ATOM_UINTEGER || (type == ATOM_OBJECT && objval && objval->getObjectType() == T_UINTEGER); }
FORCE_INLINE bool asAtom::isBool() const { return (type == ATOM_UNDEFINED_NULL_BOOL && ((uintval & 0xff) == 0)) || (type == ATOM_OBJECT && objval && objval->getObjectType() == T_BOOLEAN); }

}
#endif /* ASOBJECT_H */
