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

#include "swftypes.h"
#include <unordered_map>
#include <unordered_set>
#include <limits>

#define ASFUNCTION_ATOM(name) \
	static void name(asAtom& ret,ASWorker* wrk, asAtom& , asAtom* args, const unsigned int argslen)

/* declare setter/getter and associated member variable */
#define ASPROPERTY_GETTER(type,name) \
	type name; \
	ASFUNCTION_ATOM( _getter_##name)

#define ASPROPERTY_GETTER_ATOM(name) \
	asAtom name=asAtomHandler::invalidAtom; \
	ASFUNCTION_ATOM( _getter_##name)

#define ASPROPERTY_SETTER(type,name) \
	type name; \
	ASFUNCTION_ATOM( _setter_##name)

#define ASPROPERTY_GETTER_SETTER(type, name) \
	type name; \
	ASFUNCTION_ATOM( _getter_##name); \
	ASFUNCTION_ATOM( _setter_##name)

#define ASPROPERTY_GETTER_SETTER_ATOM(name) \
	asAtom name=asAtomHandler::invalidAtom; \
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
	void c::name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen)

/* full body for a getter declared by ASPROPERTY_GETTER or ASFUNCTION_GETTER */
#define ASFUNCTIONBODY_GETTER(c,name) \
	void c::_getter_##name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!asAtomHandler::is<c>(obj)) { \
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; }\
		c* th = asAtomHandler::as<c>(obj); \
		if(argslen != 0) {\
			createError<ArgumentError>(wrk,0,"Arguments provided in getter"); return; } \
		ArgumentConversionAtom<decltype(th->name)>::toAbstract(ret,wrk,th->name); \
	}

#define ASFUNCTIONBODY_GETTER_STATIC(c,name) \
	void c::_getter_##name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(asAtomHandler::getObject(obj) != Class<c>::getRef(wrk->getSystemState()).getPtr()) { \
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; } \
		if(argslen != 0) {\
			createError<ArgumentError>(wrk,0,"Arguments provided in getter"); return; } \
		ArgumentConversionAtom<decltype(wrk->getSystemState()->static_##c##_##name)>::toAbstract(ret,wrk,wrk->getSystemState()->static_##c##_##name); \
	}

#define ASFUNCTIONBODY_GETTER_STRINGID(c,name) \
	void c::_getter_##name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!asAtomHandler::is<c>(obj)) { \
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; } \
		c* th = asAtomHandler::as<c>(obj); \
		if(argslen != 0) {\
			createError<ArgumentError>(wrk,0,"Arguments provided in getter"); return; }\
		ret = asAtomHandler::fromStringID(th->name); \
	}

#define ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(c,name) \
	void c::_getter_##name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!asAtomHandler::is<c>(obj)) {\
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; }\
		c* th = asAtomHandler::as<c>(obj); \
		if(argslen != 0) {\
			createError<ArgumentError>(wrk,0,"Arguments provided in getter"); return; }\
		LOG(LOG_NOT_IMPLEMENTED,asAtomHandler::getObject(obj)->getClassName() <<"."<< #name << " getter is not implemented"); \
		ArgumentConversionAtom<decltype(th->name)>::toAbstract(ret,wrk,th->name); \
	}

/* full body for a getter declared by ASPROPERTY_SETTER or ASFUNCTION_SETTER */
#define ASFUNCTIONBODY_SETTER(c,name) \
	void c::_setter_##name(asAtom& ret,ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!asAtomHandler::is<c>(obj)) {\
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; }\
		c* th = asAtomHandler::as<c>(obj); \
		if(argslen != 1) {\
			createError<ArgumentError>(wrk,0,"Arguments provided in getter"); return; }\
		th->name = ArgumentConversionAtom<decltype(th->name)>::toConcrete(wrk,args[0],th->name); \
	}
#define ASFUNCTIONBODY_SETTER_STATIC(c,name) \
	void c::_setter_##name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(asAtomHandler::getObject(obj) != Class<c>::getRef(wrk->getSystemState()).getPtr()) { \
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; }\
		if(argslen != 1) {\
			createError<ArgumentError>(wrk,0,"Arguments provided in getter"); return; }\
		wrk->getSystemState()->static_##c##_##name = ArgumentConversionAtom<decltype(wrk->getSystemState()->static_##c##_##name)>::toConcrete(wrk,args[0],wrk->getSystemState()->static_##c##_##name); \
	}

#define ASFUNCTIONBODY_SETTER_STRINGID(c,name) \
	void c::_setter_##name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!asAtomHandler::is<c>(obj)) {\
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; }\
		c* th = asAtomHandler::as<c>(obj); \
		if(argslen != 1) {\
			createError<ArgumentError>(wrk,0,"Arguments provided in getter"); return; }\
		th->name = asAtomHandler::toStringId(args[0],wrk); \
	}

#define ASFUNCTIONBODY_SETTER_NOT_IMPLEMENTED(c,name) \
	void c::_setter_##name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!asAtomHandler::is<c>(obj)) {\
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; }\
		c* th = asAtomHandler::as<c>(obj); \
		if(argslen != 1) {\
			createError<ArgumentError>(wrk,0,"Arguments provided in getter"); return; }\
		LOG(LOG_NOT_IMPLEMENTED,asAtomHandler::getObject(obj)->getClassName() <<"."<< #name << " setter is not implemented"); \
		th->name = ArgumentConversionAtom<decltype(th->name)>::toConcrete(wrk,args[0],th->name); \
	}

/* full body for a getter declared by ASPROPERTY_SETTER or ASFUNCTION_SETTER.
 * After the property has been updated, the callback member function is called with the old value
 * as parameter */
#define ASFUNCTIONBODY_SETTER_CB(c,name,callback) \
	void c::_setter_##name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!asAtomHandler::is<c>(obj)) {\
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; }\
		c* th = asAtomHandler::as<c>(obj); \
		if(argslen != 1) {\
			createError<ArgumentError>(wrk,0,"Arguments provided in getter"); return; }\
		decltype(th->name) oldValue = th->name; \
		th->name = ArgumentConversionAtom<decltype(th->name)>::toConcrete(wrk,args[0],th->name); \
		th->callback(oldValue); \
	}

#define ASFUNCTIONBODY_SETTER_STRINGID_CB(c,name,callback) \
	void c::_setter_##name(asAtom& ret, ASWorker* wrk, asAtom& obj, asAtom* args, const unsigned int argslen) \
	{ \
		if(!asAtomHandler::is<c>(obj)) {\
			createError<ArgumentError>(wrk,0,"Function applied to wrong object"); return; }\
		c* th = asAtomHandler::as<c>(obj); \
		if(argslen != 1) {\
			createError<ArgumentError>(wrk,0,"Arguments provided in getter"); return; }\
		decltype(th->name) oldValue = th->name; \
		th->name = asAtomHandler::toStringId(args[0],wrk); \
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
#define REGISTER_GETTER_RESULTTYPE(c,name,cls) \
	c->setDeclaredMethodByQName(#name,"",c->getSystemState()->getBuiltinFunction(_getter_##name,0,Class<cls>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,true)


#define REGISTER_GETTER_SETTER_RESULTTYPE(c,name,cls) \
		REGISTER_GETTER_RESULTTYPE(c,name,cls); \
		REGISTER_SETTER(c,name)

#define REGISTER_GETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",c->getSystemState()->getBuiltinFunction(_getter_##name),GETTER_METHOD,true)

#define REGISTER_SETTER(c,name) \
	c->setDeclaredMethodByQName(#name,"",c->getSystemState()->getBuiltinFunction(_setter_##name),SETTER_METHOD,true)

#define REGISTER_GETTER_SETTER(c,name) \
		REGISTER_GETTER(c,name); \
		REGISTER_SETTER(c,name)

#define REGISTER_GETTER_STATIC(c,name) \
	c->setDeclaredMethodByQName(#name,"",c->getSystemState()->getBuiltinFunction(_getter_##name),GETTER_METHOD,false)

#define REGISTER_SETTER_STATIC(c,name) \
	c->setDeclaredMethodByQName(#name,"",c->getSystemState()->getBuiltinFunction(_setter_##name),SETTER_METHOD,false)

#define REGISTER_GETTER_STATIC_RESULTTYPE(c,name,cls) \
	c->setDeclaredMethodByQName(#name,"",c->getSystemState()->getBuiltinFunction(_getter_##name,0,Class<cls>::getRef(c->getSystemState()).getPtr()),GETTER_METHOD,false)

#define REGISTER_GETTER_SETTER_STATIC(c,name) \
		REGISTER_GETTER_STATIC(c,name); \
		REGISTER_SETTER_STATIC(c,name)

#define REGISTER_GETTER_SETTER_STATIC_RESULTTYPE(c,name,cls) \
		REGISTER_GETTER_STATIC_RESULTTYPE(c,name,cls); \
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
	c->setConstructor(c->getSystemState()->getBuiltinFunction(constructor));

#define CLASS_SETUP_CONSTRUCTOR_LENGTH(c, superClass, constructor, ctorlength, attributes) \
	CLASS_SETUP_NO_CONSTRUCTOR(c, superClass, attributes); \
	c->setConstructor(c->getSystemState()->getBuiltinFunction((constructor), (ctorlength)));

using namespace std;
namespace lightspark
{

class ASObject;
class ASWorker;
class IFunction;
template<class T> class Class;
class Class_base;
class ByteArray;
class Loader;
class Type;
class ABCContext;
class SystemState;
class SyntheticFunction;
class SoundTransform;
class KeyboardEvent;
class EventDispatcher;
class MouseEvent;
class Event;

#define FREELIST_SIZE 16
struct asfreelist
{
	ASObject* freelist[FREELIST_SIZE];
	int freelistsize;
	asfreelist():freelistsize(0) {}
	~asfreelist();

	inline ASObject* getObjectFromFreeList();
	inline bool pushObjectToFreeList(ASObject *obj);
};

extern SystemState* getSys();
extern ASWorker* getWorker();
enum TRAIT_KIND { NO_CREATE_TRAIT=0, DECLARED_TRAIT=1, DYNAMIC_TRAIT=2, INSTANCE_TRAIT=5, CONSTANT_TRAIT=9 /* constants are also declared traits */ };
enum GET_VARIABLE_RESULT {GETVAR_NORMAL=0x00, GETVAR_CACHEABLE=0x01, GETVAR_ISGETTER=0x02, GETVAR_ISCONSTANT=0x04, GETVAR_ISNEWOBJECT=0x08};
enum GET_VARIABLE_OPTION {NONE=0x00, SKIP_IMPL=0x01, FROM_GETLEX=0x02, DONT_CALL_GETTER=0x04, NO_INCREF=0x08, DONT_CHECK_CLASS=0x10};

#ifdef LIGHTSPARK_64
union asAtom
{
	int64_t intval;
	uint64_t uintval;
};
#define LIGHTSPARK_ATOM_VALTYPE uint64_t
#else
union asAtom
{
	int32_t intval;
	uint32_t uintval;
};
#define LIGHTSPARK_ATOM_VALTYPE uint32_t
#endif
// atom is a 32bit value (64bit on 64bit architecture):
// malloc guarantees that the returned pointer is always a multiple of eight
// so we can use the lower 3 bits to indicate the type
// layout of the least significant byte:
// d: data
// p: indicates that data is pointer to ASObject
// t: defines type of atom
// dddd dptt
// 0000 0000: invalid
// 0100 0000: null
// 0010 0000: undefined
// d001 0000: bool (d indicates true/false)
// dddd d001: uint
// dddd d101: Number
// dddd d010: stringID
// dddd d110: ASString
// dddd d011: int
// dddd d111: (U)Integer
// dddd d100: ASObject
enum ATOM_TYPE 
{ 
	ATOM_INVALID_UNDEFINED_NULL_BOOL=0x0, 
	ATOM_UINTEGER=0x1, 
	ATOM_STRINGID=0x2,
	ATOM_INTEGER=0x3, 
	ATOM_OBJECTPTR=0x4, 
	ATOM_NUMBERPTR=0x5,
	ATOM_STRINGPTR=0x6, 
	ATOM_U_INTEGERPTR=0x7
};

class asAtomHandler
{
private:
#define ATOMTYPE_NULL_BIT 0x40
#define ATOMTYPE_UNDEFINED_BIT 0x20
#define ATOMTYPE_BOOL_BIT 0x10
#define ATOMTYPE_OBJECT_BIT 0x4
	static void decRef(asAtom& a);
	static void replaceBool(asAtom &a, ASObject* obj);
	static bool Boolean_concrete_string(asAtom &a);
	static TRISTATE isLessIntern(asAtom& a, ASWorker* w, asAtom& v2);
	static bool isEqualIntern(asAtom& a, ASWorker* w, asAtom& v2);
public:
	static FORCE_INLINE asAtom fromType(SWFOBJECT_TYPE _t)
	{
		asAtom a=asAtomHandler::invalidAtom;
		switch (_t)
		{
			case T_INVALID:
				a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL;
				break;
			case T_UNDEFINED:
				a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_UNDEFINED_BIT;
				break;
			case T_NULL:
				a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_NULL_BIT;
				break;
			case T_BOOLEAN:
				a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_BOOL_BIT;
				break;
			case T_NUMBER:
				a.uintval=ATOM_NUMBERPTR;
				break;
			case T_INTEGER:
				a.uintval=ATOM_INTEGER;
				break;
			case T_UINTEGER:
				a.uintval=ATOM_UINTEGER;
				break;
			case T_STRING:
				a.uintval=ATOM_STRINGID;
				break;
			default:
				a.uintval=ATOM_OBJECTPTR;
				break;
		}
		return a;
	}
	static FORCE_INLINE asAtom fromInt(int32_t val)
	{
		asAtom a=asAtomHandler::invalidAtom;
#ifdef LIGHTSPARK_64
		a.intval = ((((int64_t)val)<<3)|ATOM_INTEGER);
#else
		a.intval = ((val<<3)|ATOM_INTEGER);
		if (val <-(1<<28)  && val > (1<<28))
			a.uintval =((LIGHTSPARK_ATOM_VALTYPE)(abstract_d(getWorker(),val))|ATOM_NUMBERPTR);
#endif
		return a;
	}
	static FORCE_INLINE asAtom fromUInt(uint32_t val)
	{
		asAtom a=asAtomHandler::invalidAtom;
#ifdef LIGHTSPARK_64
		a.uintval = ((((uint64_t)val)<<3)|ATOM_UINTEGER);
#else
		a.uintval =((val<<3)|ATOM_UINTEGER);
		if (val >= (1<<29))
			a.uintval =((LIGHTSPARK_ATOM_VALTYPE)(abstract_d(getWorker(),val))|ATOM_NUMBERPTR);
#endif
		return a;
	}
	static FORCE_INLINE asAtom fromNumber(ASWorker* wrk, number_t val,bool constant)
	{
		asAtom a=asAtomHandler::invalidAtom;
		a.uintval =((LIGHTSPARK_ATOM_VALTYPE)(constant ? abstract_d_constant(wrk,val) : abstract_d(wrk,val))|ATOM_NUMBERPTR);
		return a;
	}
	
	static FORCE_INLINE asAtom fromBool(bool val)
	{
		asAtom a=asAtomHandler::invalidAtom;
		a.uintval =(ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_BOOL_BIT | (val ? 0x80 : 0));
		return a;
	}
	static ASObject* toObject(asAtom& a, ASWorker* wrk, bool isconstant=false);
	// returns NULL if this atom is a primitive;
	static FORCE_INLINE ASObject* getObject(const asAtom& a);
	static FORCE_INLINE ASObject* getObjectNoCheck(const asAtom& a);
	static FORCE_INLINE void resetCached(const asAtom& a);
	static FORCE_INLINE asAtom fromObject(ASObject* obj)
	{
		asAtom a=asAtomHandler::invalidAtom;
		if (obj)
			replace(a,obj);
		return a;
	}
	static FORCE_INLINE asAtom fromObjectNoPrimitive(ASObject* obj);
	static FORCE_INLINE asAtom fromStringID(uint32_t sID)
	{
#ifndef LIGHTSPARK_64
		assert(sID <(1<<29));
#endif
		asAtom a=asAtomHandler::invalidAtom;
		a.uintval= (sID<<3)|ATOM_STRINGID;
		return a;
	}
	// only use this for strings that should get an internal stringID
	static asAtom fromString(SystemState *sys, const tiny_string& s);
	static ASObject* getClosure(asAtom& a);
	static asAtom getClosureAtom(asAtom& a, asAtom defaultAtom=asAtomHandler::nullAtom);
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
	static void callFunction(asAtom& caller, ASWorker* wrk, asAtom& ret, asAtom &obj, asAtom *args, uint32_t num_args, bool args_refcounted, bool coerceresult=true, bool coercearguments=true);
	// returns invalidAtom for not-primitive values
	static void getVariableByMultiname(asAtom& a, asAtom &ret, SystemState *sys, const multiname& name, ASWorker* wrk);
	static bool hasPropertyByMultiname(const asAtom& a, const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk);
	static Class_base* getClass(asAtom& a,SystemState *sys, bool followclass=true);
	static bool canCacheMethod(asAtom& a,const multiname* name);
	static void fillMultiname(asAtom& a, ASWorker *wrk, multiname& name);
	static void replace(asAtom& a,ASObject* obj);
	static FORCE_INLINE void set(asAtom& a,const asAtom& b)
	{
		a.uintval = b.uintval;
	}
	static bool stringcompare(asAtom& a, ASWorker* wrk, uint32_t stringID);
	static bool functioncompare(asAtom& a, ASWorker* wrk, asAtom& v2);
	static std::string toDebugString(const asAtom a);
	static FORCE_INLINE void applyProxyProperty(asAtom& a,SystemState *sys, multiname& name);
	static FORCE_INLINE TRISTATE isLess(asAtom& a, ASWorker* wrk, asAtom& v2);
	static FORCE_INLINE bool isEqual(asAtom& a, ASWorker* wrk, asAtom& v2);
	static FORCE_INLINE bool isEqualStrict(asAtom& a, ASWorker* wrk, asAtom& v2);
	static FORCE_INLINE bool isConstructed(const asAtom& a);
	static FORCE_INLINE bool isPrimitive(const asAtom& a);
	static FORCE_INLINE bool isNumeric(const asAtom& a); 
	static FORCE_INLINE bool isNumber(const asAtom& a); 
	static FORCE_INLINE bool isValid(const asAtom& a) { return a.uintval; }
	static FORCE_INLINE bool isInvalid(const asAtom& a) { return !a.uintval; }
	static FORCE_INLINE bool isNull(const asAtom& a) { return (a.uintval&0x7f) == ATOMTYPE_NULL_BIT; }
	static FORCE_INLINE bool isUndefined(const asAtom& a) { return (a.uintval&0x7f) == ATOMTYPE_UNDEFINED_BIT; }
	static FORCE_INLINE bool isBool(const asAtom& a) { return (a.uintval&0x7f) == ATOMTYPE_BOOL_BIT; }
	static FORCE_INLINE bool isInteger(const asAtom& a);
	static FORCE_INLINE bool isUInteger(const asAtom& a);
	static FORCE_INLINE bool isObject(const asAtom& a) { return a.uintval & ATOMTYPE_OBJECT_BIT; }
	static FORCE_INLINE bool isFunction(const asAtom& a);
	static FORCE_INLINE bool isString(const asAtom& a);
	static FORCE_INLINE bool isStringID(const asAtom& a) { return (a.uintval&0x7) == ATOM_STRINGID; }
	static FORCE_INLINE bool isQName(const asAtom& a);
	static FORCE_INLINE bool isNamespace(const asAtom& a);
	static FORCE_INLINE bool isArray(const asAtom& a);
	static FORCE_INLINE bool isClass(const asAtom& a);
	static FORCE_INLINE bool isTemplate(const asAtom& a);
	static FORCE_INLINE SWFOBJECT_TYPE getObjectType(const asAtom& a);
	static FORCE_INLINE bool checkArgumentConversion(const asAtom& a,const asAtom& obj);
	static asAtom asTypelate(asAtom& a, asAtom& b, ASWorker* wrk);
	static bool isTypelate(asAtom& a,ASObject* type);
	static bool isTypelate(asAtom& a,asAtom& t);
	static FORCE_INLINE number_t toNumber(const asAtom& a);
	static FORCE_INLINE number_t AVM1toNumber(asAtom& a, uint32_t swfversion);
	static FORCE_INLINE bool AVM1toBool(asAtom& a);
	static FORCE_INLINE int32_t toInt(const asAtom& a);
	static FORCE_INLINE int32_t toIntStrict(const asAtom& a);
	static FORCE_INLINE int64_t toInt64(const asAtom& a);
	static FORCE_INLINE uint32_t toUInt(asAtom& a);
	static void getStringView(tiny_string& res, const asAtom &a, ASWorker* wrk); // this doesn't deep copy the data buffer if parameter a is an ASString
	static tiny_string toString(const asAtom &a, ASWorker* wrk);
	static tiny_string toLocaleString(const asAtom &a, ASWorker* wrk);
	static uint32_t toStringId(asAtom &a, ASWorker* wrk);
	static FORCE_INLINE asAtom typeOf(asAtom& a);
	static bool Boolean_concrete(asAtom& a);
	static bool Boolean_concrete_object(asAtom& a);
	static void convert_b(asAtom& a, bool refcounted);
	static FORCE_INLINE int32_t getInt(const asAtom& a) { assert((a.uintval&0x7) == ATOM_INTEGER || (a.uintval&0x7) == ATOM_UINTEGER); return a.intval>>3; }
	static FORCE_INLINE uint32_t getUInt(const asAtom& a) { assert((a.uintval&0x7) == ATOM_UINTEGER || (a.uintval&0x7) == ATOM_INTEGER); return a.uintval>>3; }
	static FORCE_INLINE uint32_t getStringId(const asAtom& a) { assert((a.uintval&0x7) == ATOM_STRINGID); return a.uintval>>3; }
	static FORCE_INLINE void setInt(asAtom& a,ASWorker* wrk, int64_t val);
	static FORCE_INLINE void setUInt(asAtom& a, ASWorker* wrk, uint32_t val);
	static void setNumber(asAtom& a,ASWorker* w,number_t val);
	static bool replaceNumber(asAtom& a, ASWorker* w, number_t val);
	static FORCE_INLINE void setBool(asAtom& a,bool val);
	static FORCE_INLINE void setNull(asAtom& a);
	static FORCE_INLINE void setUndefined(asAtom& a);
	static void setFunction(asAtom& a, ASObject* obj, ASObject* closure, ASWorker* wrk);
	static FORCE_INLINE bool increment(asAtom& a, ASWorker* wrk, bool replace);
	static FORCE_INLINE bool decrement(asAtom& a, ASWorker* wrk, bool refplace);
	static FORCE_INLINE void increment_i(asAtom& a, ASWorker* wrk, int32_t amount=1);
	static FORCE_INLINE void decrement_i(asAtom& a, ASWorker* wrk, int32_t amount=1);
	static bool add(asAtom& a, asAtom& v2, ASWorker *wrk, bool forceint);
	static void addreplace(asAtom& ret, ASWorker* wrk, asAtom &v1, asAtom &v2,bool forceint);
	static FORCE_INLINE void bitnot(asAtom& a, ASWorker* wrk);
	static FORCE_INLINE void subtract(asAtom& a,ASWorker* wrk,asAtom& v2,bool forceint);
	static FORCE_INLINE void subtractreplace(asAtom& ret, ASWorker* wrk,const asAtom& v1, const asAtom& v2,bool forceint);
	static FORCE_INLINE void multiply(asAtom& a,ASWorker* wrk,asAtom& v2,bool forceint);
	static FORCE_INLINE void multiplyreplace(asAtom& ret, ASWorker* wrk,const asAtom& v1, const asAtom &v2,bool forceint);
	static FORCE_INLINE void divide(asAtom& a,ASWorker* wrk,asAtom& v2,bool forceint);
	static FORCE_INLINE void dividereplace(asAtom& ret, ASWorker* wrk,const asAtom& v1, const asAtom& v2,bool forceint);
	static FORCE_INLINE void modulo(asAtom& a,ASWorker* wrk,asAtom& v2,bool forceint);
	static FORCE_INLINE void moduloreplace(asAtom& ret, ASWorker* wrk, const asAtom& v1, const asAtom &v2, bool forceint);
	static FORCE_INLINE void lshift(asAtom& a,ASWorker* wrk,asAtom& v1);
	static FORCE_INLINE void rshift(asAtom& a,ASWorker* wrk,asAtom& v1);
	static FORCE_INLINE void urshift(asAtom& a,ASWorker* wrk,asAtom& v1);
	static FORCE_INLINE void bit_and(asAtom& a,ASWorker* wrk,asAtom& v1);
	static FORCE_INLINE void bit_or(asAtom& a,ASWorker* wrk,asAtom& v1);
	static FORCE_INLINE void bit_xor(asAtom& a, ASWorker* wrk, asAtom& v1);
	static FORCE_INLINE void _not(asAtom& a);
	static FORCE_INLINE void negate_i(asAtom& a,ASWorker* wrk);
	static FORCE_INLINE void add_i(asAtom& a,ASWorker* wrk,asAtom& v2);
	static FORCE_INLINE void subtract_i(asAtom& a,ASWorker* wrk,asAtom& v2);
	static FORCE_INLINE void multiply_i(asAtom& a,ASWorker* wrk,asAtom& v2);
	static void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
						  std::map<const ASObject*, uint32_t>& objMap,
						  std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk,
						  asAtom& a);
	template<class T> static bool is(asAtom& a);
	template<class T> static T* as(asAtom& a) 
	{ 
		assert(isObject(a));
		return static_cast<T*>((void*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)));
	}
};
#define ASATOM_INCREF(a) if (asAtomHandler::isObject(a)) asAtomHandler::getObjectNoCheck(a)->incRef()
#define ASATOM_INCREF_POINTER(a) if (asAtomHandler::isObject(*a)) asAtomHandler::getObjectNoCheck(*a)->incRef()
#define ASATOM_DECREF(a) if (asAtomHandler::isObject(a)) { ASObject* obj_b = asAtomHandler::getObjectNoCheck(a); if (!obj_b->getConstant() && !obj_b->getInDestruction()) obj_b->decRef(); }
#define ASATOM_DECREF_POINTER(a) if (asAtomHandler::isObject(*a)) { ASObject* obj_b = asAtomHandler::getObject(*a); if (obj_b && !obj_b->getConstant() && !obj_b->getInDestruction()) obj_b->decRef(); }
#define ASATOM_ADDSTOREDMEMBER(a) if (asAtomHandler::isObject(a)) { asAtomHandler::getObjectNoCheck(a)->incRef();asAtomHandler::getObjectNoCheck(a)->addStoredMember(); }
#define ASATOM_REMOVESTOREDMEMBER(a) if (asAtomHandler::isObject(a)) { asAtomHandler::getObjectNoCheck(a)->removeStoredMember();}
#define ASATOM_PREPARESHUTDOWN(a) if (asAtomHandler::isObject(a)) { asAtomHandler::getObjectNoCheck(a)->prepareShutdown();}

struct variable
{
	asAtom var;
	union
	{
		multiname* traitTypemname;
		Type* type;
		void* typeUnion;
	};
	asAtom setter;
	asAtom getter;
	nsNameAndKind ns;
	uint32_t slotid;
	TRAIT_KIND kind:4;
	bool isResolved:1;
	bool isenumerable:1;
	bool issealed:1;
	bool isrefcounted:1;
	variable(TRAIT_KIND _k,const nsNameAndKind& _ns)
		: var(asAtomHandler::invalidAtom),typeUnion(nullptr),setter(asAtomHandler::invalidAtom),getter(asAtomHandler::invalidAtom),ns(_ns),slotid(0),kind(_k),isResolved(false),isenumerable(true),issealed(false),isrefcounted(true) {}
	variable(TRAIT_KIND _k, asAtom _v, multiname* _t, Type* type, const nsNameAndKind &_ns, bool _isenumerable);
	void setVar(ASWorker* wrk, asAtom v, bool _isrefcounted = true);
	/*
	 * To be used only if the value is guaranteed to be of the right type
	 */
	void setVarNoCoerce(asAtom &v);

	void setResultType(Type* t)
	{
		isResolved=true;
		type=t;
	}
};
// struct used to count cyclic references
struct cyclicmembercount
{
	uint32_t count; // number of references counted
	bool hasmember; // indicates if the member object has any references to the main object in it's members
};
// struct used to keep track of entries when executing garbage collection
struct garbagecollectorstate
{
	std::unordered_map<ASObject*,cyclicmembercount> checkedobjects;
	std::unordered_set<ASObject*> ancestors;
	std::unordered_set<ASObject*> countedobjects;
	ASObject* startobj;
	int level;
	int incCount(ASObject* o);
	FORCE_INLINE bool checkAncestors(ASObject* o)
	{
		return ancestors.find(o)!=ancestors.end();
	}
	garbagecollectorstate(ASObject* _startobj):startobj(_startobj),level(0)
	{
	}
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
	std::vector<variable*> slots_vars;
	uint32_t slotcount;
	// indicates if this map was initialized with no variables with non-primitive values
	bool cloneable;
	variables_map():slotcount(0),cloneable(true)
	{
	}
	/**
	   Find a variable in the map

	   @param createKind If this is different from NO_CREATE_TRAIT and no variable is found
				a new one is created with the given kind
	   @param traitKinds Bitwise OR of accepted trait kinds
	*/
	variable* findObjVar(uint32_t nameId, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds);
	variable* findObjVar(SystemState* sys,const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds);
	// adds a dynamic variable without checking if a variable with this name already exists
	void setDynamicVarNoCheck(uint32_t nameID,asAtom& v);
	/**
	 * Const version of findObjVar, useful when looking for getters
	 */
	FORCE_INLINE const variable* findObjVarConst(SystemState* sys,const multiname& mname, uint32_t traitKinds, uint32_t* nsRealId = nullptr) const
	{
		if (mname.isEmpty())
			return nullptr;
		uint32_t name=mname.name_type == multiname::NAME_STRING ? mname.name_s_id : mname.normalizedNameId(sys);
		bool noNS = mname.ns.empty(); // no Namespace in multiname means we don't care about the namespace and take the first match
		const_var_iterator ret=Variables.find(name);
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
					return nullptr;
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
	
		return nullptr;
	}

	//
	FORCE_INLINE variable* findObjVar(SystemState* sys,const multiname& mname, uint32_t traitKinds, uint32_t* nsRealId = nullptr)
	{
		if (mname.isEmpty())
			return nullptr;
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
					return nullptr;
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

		return nullptr;
	}
	
	//Initialize a new variable specifying the type (TODO: add support for const)
	void initializeVar(multiname &mname, asAtom &obj, multiname *typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj, uint32_t slot_id, bool isenumerable);
	void killObjVar(SystemState* sys, const multiname& mname);
	FORCE_INLINE asAtom getSlot(unsigned int n)
	{
		assert_and_throw(n > 0 && n <= slotcount);
		return slots_vars[n-1]->var;
	}
	FORCE_INLINE variable* getSlotVar(unsigned int n)
	{
		return slots_vars[n-1];
	}
	FORCE_INLINE asAtom getSlotNoCheck(unsigned int n)
	{
		return slots_vars[n]->var;
	}
	FORCE_INLINE TRAIT_KIND getSlotKind(unsigned int n)
	{
		assert_and_throw(n > 0 && n <= slotcount);
		return slots_vars[n-1]->kind;
	}
	Class_base* getSlotType(unsigned int n);
	
	uint32_t findInstanceSlotByMultiname(multiname* name, SystemState *sys);
	FORCE_INLINE bool setSlot(ASWorker* wrk, unsigned int n, asAtom &o);
	/*
	 * This version of the call is guarantee to require no type conversion
	 * this is verified at optimization time
	 */
	FORCE_INLINE void setSlotNoCoerce(unsigned int n, asAtom o);

	FORCE_INLINE void initSlot(unsigned int n, variable *v)
	{
		if (n>slots_vars.capacity())
			slots_vars.reserve(n+8);
		if(n>slotcount)
		{
			slots_vars.resize(n);
			slotcount= n;
		}
		v->slotid = n;
		slots_vars[n-1]=v;
	}
	FORCE_INLINE  unsigned int size() const
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
				std::map<const Class_base*, uint32_t>& traitsMap, bool forsharedobject, ASWorker* wrk);
	void dumpVariables();
	void destroyContents();
	void prepareShutdown();
	bool cloneInstance(variables_map& map);
	void removeAllDeclaredProperties();
	bool countCylicMemberReferences(garbagecollectorstate& gcstate, ASObject* parent);
};

enum METHOD_TYPE { NORMAL_METHOD=0, SETTER_METHOD=1, GETTER_METHOD=2 };
//for toPrimitive
enum TP_HINT { NO_HINT, NUMBER_HINT, STRING_HINT };
class ASWorker;

extern ASWorker* getWorker();

class ASObject: public memory_reporter, public RefCountable
{
friend class ABCVm;
friend class ABCContext;
friend class Class_base; //Needed for forced cleanup
friend class Class_inherit; 
friend void lookupAndLink(Class_base* c, const tiny_string& name, const tiny_string& interfaceNs);
friend class IFunction; //Needed for clone
friend struct asfreelist;
friend class SystemState;
friend struct variable;
friend class variables_map;
friend class RootMovieClip;
friend class asAtomHandler;
friend class ASWorker;
public:
	asfreelist* objfreelist;
private:
	variables_map Variables;
	// list of ASObjects which have a (not refcounted) reference to this ASObject (used to avoid circular references)
	unordered_set<ASObject*> ownedObjects;
	Class_base* classdef;
	inline const variable* findGettable(const multiname& name, uint32_t* nsRealId = nullptr) const DLL_LOCAL
	{
		const variable* ret=Variables.findObjVarConst(getSystemState(),name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId);
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(asAtomHandler::isValid(ret->getter) || asAtomHandler::isValid(ret->var)))
				ret=nullptr;
		}
		return ret;
	}
	
	variable* findSettable(const multiname& name, bool* has_getter=nullptr) DLL_LOCAL;
	multiname* proxyMultiName;
	SystemState* sys;
	ASWorker* worker;
protected:
	ASObject(MemoryAccount* m);
	
	ASObject(const ASObject& o);
	virtual ~ASObject();
	uint32_t stringId;
	uint32_t storedmembercount; // count how often this object is stored as a member of another object (needed for cyclic reference detection)
	SWFOBJECT_TYPE type;
	CLASS_SUBTYPE subtype;
	
	bool traitsInitialized:1;
	bool constructIndicator:1;
	bool constructorCallComplete:1; // indicates that the constructor including all super constructors has been called
	bool preparedforshutdown:1;
	bool markedforgarbagecollection:1;
	static variable* findSettableImpl(SystemState* sys,variables_map& map, const multiname& name, bool* has_getter);
	static FORCE_INLINE const variable* findGettableImplConst(SystemState* sys, const variables_map& map, const multiname& name, uint32_t* nsRealId = nullptr)
	{
		const variable* ret=map.findObjVarConst(sys,name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId);
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(asAtomHandler::isValid(ret->getter) || asAtomHandler::isValid(ret->var)))
				ret=nullptr;
		}
		return ret;
	}
	static FORCE_INLINE variable* findGettableImpl(SystemState* sys, variables_map& map, const multiname& name, uint32_t* nsRealId = nullptr)
	{
		variable* ret=map.findObjVar(sys,name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId);
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(asAtomHandler::isValid(ret->getter) || asAtomHandler::isValid(ret->var)))
				ret=nullptr;
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
	bool destruct() override;

	FORCE_INLINE bool destructIntern()
	{
		if (markedforgarbagecollection)
			removefromGarbageCollection();
		destroyContents();
		for (auto it = ownedObjects.begin(); it != ownedObjects.end(); it++)
		{
			(*it)->removeStoredMember();
		}
		ownedObjects.clear();
		if (proxyMultiName)
		{
			delete proxyMultiName;
			proxyMultiName = nullptr;
		}
		stringId = UINT32_MAX;
		traitsInitialized =false;
		constructIndicator = false;
		constructorCallComplete =false;
		markedforgarbagecollection=false;
		implEnable = true;
		storedmembercount=0;
#ifndef NDEBUG
		//Stuff only used in debugging
		initialized=false;
#endif
		bool dodestruct = true;
		if (objfreelist)
		{
			if (!getCached())
			{
				assert(getWorker() == this->getInstanceWorker() || !getWorker());
				dodestruct = !objfreelist->pushObjectToFreeList(this);
			}
			else
				dodestruct = false;
		}
		if (dodestruct)
		{
#ifndef NDEBUG
			if (classdef)
			{
				uint32_t x = objectcounter[classdef];
				x--;
				objectcounter[classdef] = x;
			}
#endif
			finalize();
		}
		return dodestruct;
	}
public:
	ASObject(ASWorker* wrk, Class_base* c,SWFOBJECT_TYPE t = T_OBJECT,CLASS_SUBTYPE subtype = SUBTYPE_NOT_SET);
	void serializeDynamicProperties(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t> traitsMap, ASWorker* wrk, bool usedynamicPropertyWriter=true, bool forSharedObject = false);
#ifndef NDEBUG
	//Stuff only used in debugging
	bool initialized:1;
	static std::map<Class_base*,uint32_t> objectcounter;
	static void dumpObjectCounters(uint32_t threshhold);
#endif
	bool implEnable:1;

	inline Class_base* getClass() const { return classdef; }
	void setClass(Class_base* c);
	FORCE_INLINE void addStoredMember()
	{
		assert(storedmembercount<=uint32_t(this->getRefCount()) || this->getConstant());
		storedmembercount++;
		if (markedforgarbagecollection)
		{
			removefromGarbageCollection();
			decRef();
		}
	}
	bool isMarkedForGarbageCollection() const { return markedforgarbagecollection; }
	void removefromGarbageCollection();
	void removeStoredMember();
	void handleGarbageCollection();
	virtual bool countCylicMemberReferences(garbagecollectorstate& gcstate);
	FORCE_INLINE bool canHaveCyclicMemberReference()
	{
		return type == T_ARRAY || type == T_CLASS || type == T_PROXY || type == T_TEMPLATE || type == T_FUNCTION ||
				(type == T_OBJECT && 
				subtype != SUBTYPE_DATE && 
				subtype != SUBTYPE_URLREQUEST); // TODO check other subtypes
	}
	bool countAllCylicMemberReferences(garbagecollectorstate& gcstate);
	
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
	ASFUNCTION_ATOM(registerClass);
	ASFUNCTION_ATOM(AVM1_IgnoreSetter);
	
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
	   The finalize function is used only for classes that don't have the reusable flag set and on destruction at application exit
	   It should decRef all referenced objects.
	   It has to reset all data to their default state.
	   The finalize method must be callable multiple time with the same effects (no double frees).
	*/
	inline virtual void finalize() {}
	// use this to mark an ASObject as constant, instead of RefCountable->setConstant()
	// because otherwise it will not be properly deleted on application exit.
	void setRefConstant();

	virtual GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk)
	{
		return getVariableByMultinameIntern(ret,name,classdef,opt,wrk);
	}
	virtual GET_VARIABLE_RESULT getVariableByInteger(asAtom& ret, int index, GET_VARIABLE_OPTION opt, ASWorker* wrk)
	{
		return getVariableByIntegerIntern(ret,index,opt,wrk);
	}
	// AVM1 needs to check the "protoype" variable in addition to the normal behaviour
	GET_VARIABLE_RESULT AVM1getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk);
	/*
	 * Helper method using the get the raw variable struct instead of calling the getter.
	 * It is used by getVariableByMultiname and by early binding code
	 */
	virtual variable *findVariableByMultiname(const multiname& name, Class_base* cls, uint32_t* nsRealID, bool* isborrowed, bool considerdynamic, ASWorker* wrk);
	/*
	 * Gets a variable of this object. It looks through all classes (beginning at cls),
	 * then the prototype chain, and then instance variables.
	 * If the property found is a getter, it is called and its return value returned.
	 */
	GET_VARIABLE_RESULT getVariableByMultinameIntern(asAtom& ret, const multiname& name, Class_base* cls, GET_VARIABLE_OPTION opt,ASWorker* wrk);
	GET_VARIABLE_RESULT getVariableByIntegerIntern(asAtom& ret, int index, GET_VARIABLE_OPTION opt,ASWorker* wrk)
	{
		multiname m(nullptr);
		m.name_type = multiname::NAME_INT;
		m.name_i = index;
		return getVariableByMultiname(ret,m,opt,wrk);
	}
	virtual int32_t getVariableByMultiname_i(const multiname& name, ASWorker* wrk);
	/* Simple getter interface for the common case */
	void getVariableByMultiname(asAtom& ret, const tiny_string& name, std::list<tiny_string> namespaces,ASWorker* wrk);
	/*
	 * Execute a AS method on this object. Returns the value
	 * returned by the function. One reference of each args[i] is
	 * consumed. The method must exist, otherwise a TypeError is
	 * thrown.
	 */
	void executeASMethod(asAtom &ret, const tiny_string& methodName, std::list<tiny_string> namespaces, asAtom *args, uint32_t num_args);
	virtual void setVariableByMultiname_i(multiname &name, int32_t value, ASWorker* wrk);
	enum CONST_ALLOWED_FLAG { CONST_ALLOWED=0, CONST_NOT_ALLOWED };
	/*
	 * If alreadyset is not null, it has to be initialized to false by the caller.
	 * It will be set to true if the old and new value are the same.
	 * In that case the old value will not be decReffed.
	 */
	virtual multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
	{
		return setVariableByMultiname_intern(name,o,allowConst,classdef,alreadyset,wrk);
	}
	virtual void setVariableByInteger(int index, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
	{
		setVariableByInteger_intern(index,o,allowConst,alreadyset,wrk);
	}
	/*
	 * Sets  variable of this object. It looks through all classes (beginning at cls),
	 * then the prototype chain, and then instance variables.
	 * If the property found is a setter, it is called with the given 'o'.
	 * If no property is found, an instance variable is created.
	 * Setting CONSTANT_TRAIT is only allowed if allowConst is true
	 */
	multiname* setVariableByMultiname_intern(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, Class_base* cls, bool *alreadyset, ASWorker* wrk);
	void setVariableByInteger_intern(int index, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk)
	{
		multiname m(nullptr);
		m.name_type = multiname::NAME_INT;
		m.name_i = index;
		setVariableByMultiname(m,o,allowConst,alreadyset,wrk);
	}
	
	// sets dynamic variable without checking for existence
	// use it if it is guarranteed that the variable doesn't exist in this object
	FORCE_INLINE void setDynamicVariableNoCheck(uint32_t nameID, asAtom& o)
	{
		Variables.setDynamicVarNoCheck(nameID,o);
	}
	/*
	 * Called by ABCVm::buildTraits to create DECLARED_TRAIT or CONSTANT_TRAIT and set their type
	 */
	void initializeVariableByMultiname(multiname &name, asAtom& o, multiname* typemname,
			ABCContext* context, TRAIT_KIND traitKind, uint32_t slot_id, bool isenumerable);
	virtual bool deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
	{
		return deleteVariableByMultiname_intern(name, wrk);
	}
	bool deleteVariableByMultiname_intern(const multiname& name, ASWorker* wrk);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true);
	void setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true);
	variable *setVariableByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true);
	variable *setVariableAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable = true);
	variable *setVariableAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable = true, bool isRefcounted = true);
	//NOTE: the isBorrowed flag is used to distinguish methods/setters/getters that are inside a class but on behalf of the instances
	void setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodAtomByQName(const tiny_string& name, const tiny_string& ns, asAtom o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom f, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	virtual bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk);
	FORCE_INLINE asAtom getSlot(unsigned int n)
	{
		return Variables.getSlot(n);
	}
	FORCE_INLINE variable* getSlotVar(unsigned int n)
	{
		return Variables.getSlotVar(n);
	}
	FORCE_INLINE asAtom getSlotNoCheck(unsigned int n)
	{
		return Variables.getSlotNoCheck(n);
	}
	FORCE_INLINE TRAIT_KIND getSlotKind(unsigned int n)
	{
		return Variables.getSlotKind(n);
	}
	FORCE_INLINE bool setSlot(ASWorker* wrk,unsigned int n,asAtom o)
	{
		return Variables.setSlot(wrk,n,o);
	}
	FORCE_INLINE void setSlotNoCoerce(unsigned int n,asAtom o)
	{
		Variables.setSlotNoCoerce(n,o);
	}
	FORCE_INLINE Class_base* getSlotType(unsigned int n)
	{
		return Variables.getSlotType(n);
	}
	uint32_t findInstanceSlotByMultiname(multiname* name)
	{
		return Variables.findInstanceSlotByMultiname(name,getSystemState());
	}
	unsigned int numSlots() const
	{
		return Variables.slots_vars.size();
	}
	
	void initSlot(unsigned int n, variable *v);
	
	void initAdditionalSlots(std::vector<multiname *> &additionalslots);
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
	inline void setSystemState(SystemState* s)
	{
		sys = s;
	}
	inline ASWorker* getInstanceWorker() const
	{
		return worker;
	}
	inline void setWorker(ASWorker* w)
	{
		worker = w;
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
	virtual number_t toNumberForComparison();
	/* Implements ECMA's ToPrimitive (9.1) and [[DefaultValue]] (8.6.2.6) */
	bool toPrimitive(asAtom& ret,bool& isrefcounted, TP_HINT hint = NO_HINT);
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
	void setprop_prototype(_NR<ASObject>& prototype, uint32_t nameID=BUILTIN_STRINGS::PROTOTYPE);

	//Comparison operators
	virtual bool isEqual(ASObject* r);
	virtual bool isEqualStrict(ASObject* r);
	virtual TRISTATE isLess(ASObject* r);
	virtual TRISTATE isLessAtom(asAtom& r);

	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o);

	//Enumeration handling
	virtual uint32_t nextNameIndex(uint32_t cur_index);
	virtual void nextName(asAtom &ret, uint32_t index);
	virtual void nextValue(asAtom &ret, uint32_t index);

	//Called when the object construction is completed. Used by MovieClip implementation
	inline virtual void constructionComplete(bool _explicit = false)
	{
	}
	// Called before the object's ActionScript constructor is executed. Mainly used by MovieClip, and DisplayObject.
	inline virtual void beforeConstruction(bool _explicit = false)
	{
	}
	//Called after the object Actionscript constructor was executed. Used by MovieClip implementation
	inline virtual void afterConstruction(bool _explicit = false)
	{
	}

	void addOwnedObject(ASObject* obj);
	/**
	  Serialization interface

	  The various maps are used to implement reference type of the AMF3 spec
	*/
	virtual void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker*wrk);

	virtual ASObject *describeType(ASWorker* wrk) const;

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
	virtual std::string toDebugString() const;
	
	/* stores proxy namespace settings for internal usage */
	void setProxyProperty(const multiname& name); 
	/* applies proxy namespace settings to name for internal usage */
	void applyProxyProperty(multiname &name); 
	
	void dumpVariables();
	
	inline void setConstructIndicator() { constructIndicator = true; }
	inline void setConstructorCallComplete() { constructorCallComplete = true; }
	inline void setIsInitialized(bool init=true) { traitsInitialized=init; }
	inline bool getConstructIndicator() const { return constructIndicator; }

	
	void setIsEnumerable(const multiname& name, bool isEnum);
	inline void destroyContents()
	{
		Variables.destroyContents();
	}
	// this is called when shutting down the application, removes all pointers to freelist to avoid any caching of ASObjects
	virtual void prepareShutdown();
	CLASS_SUBTYPE getSubtype() const { return subtype;}
	// copies all variables into the target
	// returns false if cloning is not possible
	bool cloneInstance(ASObject* target);

	virtual asAtom getVariableBindingValue(const tiny_string &name);
	virtual void AVM1HandleEvent(EventDispatcher* dispatcher, Event* e) { }
	virtual bool AVM1HandleKeyboardEvent(KeyboardEvent* e);
	virtual bool AVM1HandleMouseEvent(EventDispatcher* dispatcher,MouseEvent* e);
	bool AVM1HandleMouseEventStandard(ASObject *dispobj, MouseEvent *e);
	// updates AVM1 bindings in target for all members of this ASObject
	void AVM1UpdateAllBindings(DisplayObject* target, ASWorker* wrk);

	// copies all dynamic values to the target
	void copyValues(ASObject* target, ASWorker* wrk);
};


FORCE_INLINE bool variables_map::setSlot(ASWorker* wrk,unsigned int n, asAtom &o)
{
	assert_and_throw(n < slotcount);
	if (slots_vars[n]->var.uintval != o.uintval)
	{
		slots_vars[n]->setVar(wrk,o);
		return slots_vars[n]->var.uintval == o.uintval; // setVar may coerce the object into a new instance, so we need to check if incRef is necessary
	}
	return false;
}

FORCE_INLINE void variables_map::setSlotNoCoerce(unsigned int n, asAtom o)
{
	assert_and_throw(n < slotcount);
	if (slots_vars[n]->var.uintval != o.uintval)
		slots_vars[n]->setVarNoCoerce(o);
}
FORCE_INLINE void variables_map::setDynamicVarNoCheck(uint32_t nameID,asAtom& v)
{
	var_iterator inserted=Variables.insert(Variables.cbegin(),
			make_pair(nameID,variable(DYNAMIC_TRAIT,nsNameAndKind())));
	ASObject* o = asAtomHandler::getObject(v);
	if (o && !o->getConstant())
		o->addStoredMember();
	asAtomHandler::set(inserted->second.var,v);
}
FORCE_INLINE void variable::setVarNoCoerce(asAtom &v)
{
	asAtom oldvar = var;
	var=v;
	if(isrefcounted && asAtomHandler::isObject(oldvar))
	{
		LOG_CALL("remove old var no coerce:"<<asAtomHandler::toDebugString(oldvar));
		asAtomHandler::getObjectNoCheck(oldvar)->removeStoredMember();
	}
	isrefcounted = asAtomHandler::isObject(v);
	if(isrefcounted)
	{
		asAtomHandler::getObjectNoCheck(v)->incRef();
		asAtomHandler::getObjectNoCheck(v)->addStoredMember();
	}
}


class Activation_object;
class ApplicationDomain;
class ArgumentError;
class Array;
class ASCondition;
class ASError;
class ASFile;
class ASMutex;
class ASQName;
class ASString;
class ASWorker;
class AVM1Function;
class AVM1MovieClip;
class AVM1MovieClipLoader;
class AVM1Sound;
class BevelFilter;
class Bitmap;
class BitmapData;
class BitmapFilter;
class Boolean;
class BlurFilter;
class ByteArray;
class Class_inherit;
class ColorTransform;
class ColorMatrixFilter;
class ConvolutionFilter;
class ContentElement;
class Context3D;
class ContextMenu;
class ContextMenuBuiltInItems;
class ContextMenuEvent;
class CubeTexture;
class DatagramSocket;
class Date;
class DefinitionError;
class Dictionary;
class DisplacementFilter;
class DisplayObject;
class DisplayObjectContainer;
class DropShadowFilter;
class EastAsianJustifier;
class ElementFormat;
class EvalError;
class Event;
class ExtensionContext;
class FileMode;
class FileReference;
class FileStream;
class Function;
class Function_object;
class FontDescription;
class GameInputDevice;
class GameInputEvent;
class Global;
class GlowFilter;
class GradientGlowFilter;
class GradientBevelFilter;
class GraphicsEndFill;
class GraphicsPath;
class GraphicsSolidFill;
class IFunction;
class Integer;
class InteractiveObject;
class IndexBuffer3D;
class KeyboardEvent;
class LocalConnection;
class Loader;
class LoaderContext;
class LoaderInfo;
class Matrix;
class Matrix3D;
class MessageChannel;
class MorphShape;
class MouseEvent;
class MovieClip;
class Namespace;
class NativeWindow;
class NativeWindowBoundsEvent;
class NetStream;
class Null;
class Number;
class ObjectConstructor;
class Point;
class Program3D;
class ProgressEvent;
class Proxy;
class RangeError;
class Rectangle;
class RectangleTexture;
class ReferenceError;
class RegExp;
class RootMovieClip;
class SampleDataEvent;
class SecurityError;
class ShaderFilter;
class SharedObject;
class Shape;
class SimpleButton;
class Sound;
class SoundChannel;
class SpaceJustifier;
class Sprite;
class Stage;
class Stage3D;
class SyntaxError;
class Template_base;
class TextBlock;
class TextElement;
class TextField;
class TextFormat;
class TextJustifier;
class TextLine;
class TextLineMetrics;
class Texture;
class TextureBase;
class ThrottleEvent;
class Type;
class TypeError;
class UInteger;
class Undefined;
class UninitializedError;
class URIError;
class URLLoader;
class URLRequest;
class Vector;
class Vector3D;
class VerifyError;
class VertexBuffer3D;
class Video;
class VideoTexture;
class WaitableEvent;
class WorkerDomain;
class XML;
class XMLNode;
class XMLDocument;
class XMLList;


// this is used to avoid calls to dynamic_cast when testing for some classes
// keep in mind that when adding a class here you have to take care of the class inheritance and add the new SUBTYPE_ to all apropriate is<> methods 
template<> inline bool ASObject::is<Activation_object>() const { return subtype==SUBTYPE_ACTIVATIONOBJECT; }
template<> inline bool ASObject::is<ApplicationDomain>() const { return subtype==SUBTYPE_APPLICATIONDOMAIN; }
template<> inline bool ASObject::is<ArgumentError>() const { return subtype==SUBTYPE_ARGUMENTERROR; }
template<> inline bool ASObject::is<Array>() const { return type==T_ARRAY; }
template<> inline bool ASObject::is<ASCondition>() const { return subtype==SUBTYPE_CONDITION; }
template<> inline bool ASObject::is<ASError>() const { return subtype==SUBTYPE_ERROR || subtype==SUBTYPE_SECURITYERROR || subtype==SUBTYPE_ARGUMENTERROR || subtype==SUBTYPE_DEFINITIONERROR || subtype==SUBTYPE_EVALERROR || subtype==SUBTYPE_RANGEERROR || subtype==SUBTYPE_REFERENCEERROR || subtype==SUBTYPE_SYNTAXERROR || subtype==SUBTYPE_TYPEERROR || subtype==SUBTYPE_URIERROR || subtype==SUBTYPE_VERIFYERROR || subtype==SUBTYPE_UNINITIALIZEDERROR; }
template<> inline bool ASObject::is<ASFile>() const { return subtype==SUBTYPE_FILE; }
template<> inline bool ASObject::is<ASMutex>() const { return subtype==SUBTYPE_MUTEX; }
template<> inline bool ASObject::is<ASObject>() const { return true; }
template<> inline bool ASObject::is<ASQName>() const { return type==T_QNAME; }
template<> inline bool ASObject::is<ASString>() const { return type==T_STRING; }
template<> inline bool ASObject::is<ASWorker>() const { return subtype==SUBTYPE_WORKER; }
template<> inline bool ASObject::is<AVM1Function>() const { return subtype==SUBTYPE_AVM1FUNCTION; }
template<> inline bool ASObject::is<AVM1Movie>() const { return subtype == SUBTYPE_AVM1MOVIE; }
template<> inline bool ASObject::is<AVM1MovieClip>() const { return subtype == SUBTYPE_AVM1MOVIECLIP; }
template<> inline bool ASObject::is<AVM1MovieClipLoader>() const { return subtype == SUBTYPE_AVM1MOVIECLIPLOADER; }
template<> inline bool ASObject::is<AVM1Sound>() const { return subtype == SUBTYPE_AVM1SOUND; }
template<> inline bool ASObject::is<BevelFilter>() const { return subtype==SUBTYPE_BEVELFILTER; }
template<> inline bool ASObject::is<Bitmap>() const { return subtype==SUBTYPE_BITMAP; }
template<> inline bool ASObject::is<BitmapData>() const { return subtype==SUBTYPE_BITMAPDATA; }
template<> inline bool ASObject::is<BitmapFilter>() const { return subtype==SUBTYPE_BITMAPFILTER || subtype==SUBTYPE_GLOWFILTER || subtype==SUBTYPE_DROPSHADOWFILTER || subtype==SUBTYPE_GRADIENTGLOWFILTER || subtype==SUBTYPE_BEVELFILTER || subtype==SUBTYPE_COLORMATRIXFILTER || subtype==SUBTYPE_BLURFILTER || subtype==SUBTYPE_CONVOLUTIONFILTER || subtype==SUBTYPE_DISPLACEMENTFILTER || subtype==SUBTYPE_GRADIENTBEVELFILTER || subtype==SUBTYPE_SHADERFILTER; }
template<> inline bool ASObject::is<Boolean>() const { return type==T_BOOLEAN; }
template<> inline bool ASObject::is<BlurFilter>() const { return subtype==SUBTYPE_BLURFILTER; }
template<> inline bool ASObject::is<ByteArray>() const { return subtype==SUBTYPE_BYTEARRAY; }
template<> inline bool ASObject::is<Class_base>() const { return type==T_CLASS; }
template<> inline bool ASObject::is<Class_inherit>() const { return subtype==SUBTYPE_INHERIT; }
template<> inline bool ASObject::is<ColorTransform>() const { return subtype==SUBTYPE_COLORTRANSFORM; }
template<> inline bool ASObject::is<ColorMatrixFilter>() const { return subtype==SUBTYPE_COLORMATRIXFILTER; }
template<> inline bool ASObject::is<ContentElement>() const { return subtype==SUBTYPE_CONTENTELEMENT || subtype == SUBTYPE_TEXTELEMENT; }
template<> inline bool ASObject::is<Context3D>() const { return subtype==SUBTYPE_CONTEXT3D; }
template<> inline bool ASObject::is<ContextMenu>() const { return subtype==SUBTYPE_CONTEXTMENU; }
template<> inline bool ASObject::is<ContextMenuBuiltInItems>() const { return subtype==SUBTYPE_CONTEXTMENUBUILTINITEMS; }
template<> inline bool ASObject::is<ContextMenuEvent>() const { return subtype==SUBTYPE_CONTEXTMENUEVENT; }
template<> inline bool ASObject::is<ConvolutionFilter>() const { return subtype==SUBTYPE_CONVOLUTIONFILTER; }
template<> inline bool ASObject::is<CubeTexture>() const { return subtype==SUBTYPE_CUBETEXTURE; }
template<> inline bool ASObject::is<Date>() const { return subtype==SUBTYPE_DATE; }
template<> inline bool ASObject::is<DatagramSocket>() const { return subtype==SUBTYPE_DATAGRAMSOCKET; }
template<> inline bool ASObject::is<DefinitionError>() const { return subtype==SUBTYPE_DEFINITIONERROR; }
template<> inline bool ASObject::is<Dictionary>() const { return subtype==SUBTYPE_DICTIONARY; }
template<> inline bool ASObject::is<DisplacementFilter>() const { return subtype==SUBTYPE_DISPLACEMENTFILTER; }
template<> inline bool ASObject::is<DisplayObject>() const { return subtype==SUBTYPE_DISPLAYOBJECT || subtype==SUBTYPE_INTERACTIVE_OBJECT || subtype==SUBTYPE_TEXTFIELD || subtype==SUBTYPE_BITMAP || subtype==SUBTYPE_DISPLAYOBJECTCONTAINER || subtype==SUBTYPE_STAGE || subtype==SUBTYPE_ROOTMOVIECLIP || subtype==SUBTYPE_SPRITE || subtype == SUBTYPE_MOVIECLIP || subtype == SUBTYPE_TEXTLINE || subtype == SUBTYPE_VIDEO || subtype == SUBTYPE_SIMPLEBUTTON || subtype == SUBTYPE_SHAPE || subtype == SUBTYPE_MORPHSHAPE || subtype==SUBTYPE_LOADER || subtype == SUBTYPE_AVM1MOVIECLIP || subtype == SUBTYPE_AVM1MOVIE; }
template<> inline bool ASObject::is<DisplayObjectContainer>() const { return subtype==SUBTYPE_DISPLAYOBJECTCONTAINER || subtype==SUBTYPE_STAGE || subtype==SUBTYPE_ROOTMOVIECLIP || subtype==SUBTYPE_SPRITE || subtype == SUBTYPE_MOVIECLIP || subtype == SUBTYPE_TEXTLINE || subtype == SUBTYPE_SIMPLEBUTTON || subtype==SUBTYPE_LOADER || subtype == SUBTYPE_AVM1MOVIECLIP || subtype == SUBTYPE_AVM1MOVIE; }
template<> inline bool ASObject::is<DropShadowFilter>() const { return subtype==SUBTYPE_DROPSHADOWFILTER; }
template<> inline bool ASObject::is<EastAsianJustifier>() const { return subtype==SUBTYPE_EASTASIANJUSTIFIER; }
template<> inline bool ASObject::is<ElementFormat>() const { return subtype==SUBTYPE_ELEMENTFORMAT; }
template<> inline bool ASObject::is<EvalError>() const { return subtype==SUBTYPE_EVALERROR; }
template<> inline bool ASObject::is<Event>() const { return subtype==SUBTYPE_EVENT || subtype==SUBTYPE_WAITABLE_EVENT || subtype==SUBTYPE_PROGRESSEVENT || subtype==SUBTYPE_KEYBOARD_EVENT || subtype==SUBTYPE_MOUSE_EVENT || subtype==SUBTYPE_SAMPLEDATA_EVENT || subtype == SUBTYPE_THROTTLE_EVENT || subtype == SUBTYPE_CONTEXTMENUEVENT || subtype == SUBTYPE_GAMEINPUTEVENT || subtype == SUBTYPE_NATIVEWINDOWBOUNDSEVENT; }
template<> inline bool ASObject::is<ExtensionContext>() const { return subtype==SUBTYPE_EXTENSIONCONTEXT; }
template<> inline bool ASObject::is<FontDescription>() const { return subtype==SUBTYPE_FONTDESCRIPTION; }
template<> inline bool ASObject::is<FileMode>() const { return subtype==SUBTYPE_FILEMODE; }
template<> inline bool ASObject::is<FileReference>() const { return subtype==SUBTYPE_FILE||subtype==SUBTYPE_FILEREFERENCE; }
template<> inline bool ASObject::is<FileStream>() const { return subtype==SUBTYPE_FILESTREAM; }
template<> inline bool ASObject::is<Function_object>() const { return subtype==SUBTYPE_FUNCTIONOBJECT; }
template<> inline bool ASObject::is<Function>() const { return subtype==SUBTYPE_FUNCTION; }
template<> inline bool ASObject::is<GameInputDevice>() const { return subtype==SUBTYPE_GAMEINPUTDEVICE; }
template<> inline bool ASObject::is<GameInputEvent>() const { return subtype==SUBTYPE_GAMEINPUTEVENT; }
template<> inline bool ASObject::is<Global>() const { return subtype==SUBTYPE_GLOBAL; }
template<> inline bool ASObject::is<GlowFilter>() const { return subtype==SUBTYPE_GLOWFILTER; }
template<> inline bool ASObject::is<GradientGlowFilter>() const { return subtype==SUBTYPE_GRADIENTGLOWFILTER; }
template<> inline bool ASObject::is<GradientBevelFilter>() const { return subtype==SUBTYPE_GRADIENTBEVELFILTER; }
template<> inline bool ASObject::is<GraphicsEndFill>() const { return subtype==SUBTYPE_GRAPHICSENDFILL; }
template<> inline bool ASObject::is<GraphicsPath>() const { return subtype==SUBTYPE_GRAPHICSPATH; }
template<> inline bool ASObject::is<GraphicsSolidFill>() const { return subtype==SUBTYPE_GRAPHICSSOLIDFILL; }
template<> inline bool ASObject::is<IFunction>() const { return type==T_FUNCTION; }
template<> inline bool ASObject::is<IndexBuffer3D>() const { return subtype==SUBTYPE_INDEXBUFFER3D; }
template<> inline bool ASObject::is<Integer>() const { return type==T_INTEGER; }
template<> inline bool ASObject::is<InteractiveObject>() const { return subtype==SUBTYPE_INTERACTIVE_OBJECT || subtype==SUBTYPE_TEXTFIELD || subtype==SUBTYPE_DISPLAYOBJECTCONTAINER || subtype==SUBTYPE_STAGE || subtype==SUBTYPE_ROOTMOVIECLIP || subtype==SUBTYPE_SPRITE || subtype == SUBTYPE_MOVIECLIP || subtype == SUBTYPE_SIMPLEBUTTON || subtype==SUBTYPE_LOADER || subtype == SUBTYPE_AVM1MOVIECLIP || subtype == SUBTYPE_AVM1MOVIE; }
template<> inline bool ASObject::is<LocalConnection>() const { return subtype==SUBTYPE_LOCALCONNECTION; }
template<> inline bool ASObject::is<KeyboardEvent>() const { return subtype==SUBTYPE_KEYBOARD_EVENT; }
template<> inline bool ASObject::is<Loader>() const { return subtype==SUBTYPE_LOADER; }
template<> inline bool ASObject::is<LoaderContext>() const { return subtype==SUBTYPE_LOADERCONTEXT; }
template<> inline bool ASObject::is<LoaderInfo>() const { return subtype==SUBTYPE_LOADERINFO; }
template<> inline bool ASObject::is<Matrix>() const { return subtype==SUBTYPE_MATRIX; }
template<> inline bool ASObject::is<Matrix3D>() const { return subtype==SUBTYPE_MATRIX3D; }
template<> inline bool ASObject::is<MessageChannel>() const { return subtype==SUBTYPE_MESSAGECHANNEL; }
template<> inline bool ASObject::is<MorphShape>() const { return subtype==SUBTYPE_MORPHSHAPE; }
template<> inline bool ASObject::is<MouseEvent>() const { return subtype==SUBTYPE_MOUSE_EVENT; }
template<> inline bool ASObject::is<MovieClip>() const { return subtype==SUBTYPE_ROOTMOVIECLIP || subtype == SUBTYPE_MOVIECLIP || subtype == SUBTYPE_AVM1MOVIECLIP; }
template<> inline bool ASObject::is<Null>() const { return type==T_NULL; }
template<> inline bool ASObject::is<Namespace>() const { return type==T_NAMESPACE; }
template<> inline bool ASObject::is<NativeWindow>() const { return subtype==SUBTYPE_NATIVEWINDOW; }
template<> inline bool ASObject::is<NativeWindowBoundsEvent>() const { return subtype==SUBTYPE_NATIVEWINDOWBOUNDSEVENT; }
template<> inline bool ASObject::is<NetStream>() const { return subtype==SUBTYPE_NETSTREAM; }
template<> inline bool ASObject::is<Number>() const { return type==T_NUMBER; }
template<> inline bool ASObject::is<ObjectConstructor>() const { return subtype==SUBTYPE_OBJECTCONSTRUCTOR; }
template<> inline bool ASObject::is<Point>() const { return subtype==SUBTYPE_POINT; }
template<> inline bool ASObject::is<Program3D>() const { return subtype==SUBTYPE_PROGRAM3D; }
template<> inline bool ASObject::is<ProgressEvent>() const { return subtype==SUBTYPE_PROGRESSEVENT; }
template<> inline bool ASObject::is<Proxy>() const { return subtype==SUBTYPE_PROXY; }
template<> inline bool ASObject::is<RangeError>() const { return subtype==SUBTYPE_RANGEERROR; }
template<> inline bool ASObject::is<Rectangle>() const { return subtype==SUBTYPE_RECTANGLE; }
template<> inline bool ASObject::is<RectangleTexture>() const { return subtype==SUBTYPE_RECTANGLETEXTURE; }
template<> inline bool ASObject::is<ReferenceError>() const { return subtype==SUBTYPE_REFERENCEERROR; }
template<> inline bool ASObject::is<RegExp>() const { return subtype==SUBTYPE_REGEXP; }
template<> inline bool ASObject::is<RootMovieClip>() const { return subtype==SUBTYPE_ROOTMOVIECLIP; }
template<> inline bool ASObject::is<SampleDataEvent>() const { return subtype==SUBTYPE_SAMPLEDATA_EVENT; }
template<> inline bool ASObject::is<ShaderFilter>() const { return subtype==SUBTYPE_SHADERFILTER; }
template<> inline bool ASObject::is<SecurityError>() const { return subtype==SUBTYPE_SECURITYERROR; }
template<> inline bool ASObject::is<Shape>() const { return subtype==SUBTYPE_SHAPE; }
template<> inline bool ASObject::is<SharedObject>() const { return subtype==SUBTYPE_SHAREDOBJECT; }
template<> inline bool ASObject::is<SimpleButton>() const { return subtype==SUBTYPE_SIMPLEBUTTON; }
template<> inline bool ASObject::is<Sound>() const { return subtype==SUBTYPE_SOUND || subtype == SUBTYPE_AVM1SOUND; }
template<> inline bool ASObject::is<SoundChannel>() const { return subtype==SUBTYPE_SOUNDCHANNEL; }
template<> inline bool ASObject::is<SoundTransform>() const { return subtype==SUBTYPE_SOUNDTRANSFORM; }
template<> inline bool ASObject::is<SpaceJustifier>() const { return subtype==SUBTYPE_SPACEJUSTIFIER; }
template<> inline bool ASObject::is<Sprite>() const { return subtype==SUBTYPE_SPRITE || subtype==SUBTYPE_ROOTMOVIECLIP || subtype == SUBTYPE_MOVIECLIP || subtype == SUBTYPE_AVM1MOVIECLIP; }
template<> inline bool ASObject::is<Stage>() const { return subtype==SUBTYPE_STAGE; }
template<> inline bool ASObject::is<Stage3D>() const { return subtype==SUBTYPE_STAGE3D; }
template<> inline bool ASObject::is<SyntaxError>() const { return subtype==SUBTYPE_SYNTAXERROR; }
template<> inline bool ASObject::is<SyntheticFunction>() const { return subtype==SUBTYPE_SYNTHETICFUNCTION; }
template<> inline bool ASObject::is<Template_base>() const { return type==T_TEMPLATE; }
template<> inline bool ASObject::is<TextBlock>() const { return subtype==SUBTYPE_TEXTBLOCK; }
template<> inline bool ASObject::is<TextElement>() const { return subtype==SUBTYPE_TEXTELEMENT; }
template<> inline bool ASObject::is<TextField>() const { return subtype==SUBTYPE_TEXTFIELD; }
template<> inline bool ASObject::is<TextFormat>() const { return subtype==SUBTYPE_TEXTFORMAT; }
template<> inline bool ASObject::is<TextJustifier>() const { return subtype==SUBTYPE_TEXTJUSTIFIER || subtype==SUBTYPE_SPACEJUSTIFIER || subtype==SUBTYPE_EASTASIANJUSTIFIER; }
template<> inline bool ASObject::is<TextLine>() const { return subtype==SUBTYPE_TEXTLINE; }
template<> inline bool ASObject::is<TextLineMetrics>() const { return subtype==SUBTYPE_TEXTLINEMETRICS; }
template<> inline bool ASObject::is<Texture>() const { return subtype==SUBTYPE_TEXTURE; }
template<> inline bool ASObject::is<TextureBase>() const { return subtype==SUBTYPE_TEXTUREBASE || subtype==SUBTYPE_TEXTURE || subtype==SUBTYPE_CUBETEXTURE || subtype==SUBTYPE_RECTANGLETEXTURE || subtype==SUBTYPE_TEXTURE || subtype==SUBTYPE_VIDEOTEXTURE; }
template<> inline bool ASObject::is<ThrottleEvent>() const { return subtype==SUBTYPE_THROTTLE_EVENT; }
template<> inline bool ASObject::is<Type>() const { return type==T_CLASS; }
template<> inline bool ASObject::is<TypeError>() const { return subtype==SUBTYPE_TYPEERROR; }
template<> inline bool ASObject::is<UInteger>() const { return type==T_UINTEGER; }
template<> inline bool ASObject::is<Undefined>() const { return type==T_UNDEFINED; }
template<> inline bool ASObject::is<UninitializedError>() const { return subtype == SUBTYPE_UNINITIALIZEDERROR; }
template<> inline bool ASObject::is<URIError>() const { return subtype == SUBTYPE_URIERROR; }
template<> inline bool ASObject::is<URLLoader>() const { return subtype == SUBTYPE_URLLOADER; }
template<> inline bool ASObject::is<URLRequest>() const { return subtype == SUBTYPE_URLREQUEST; }
template<> inline bool ASObject::is<Vector>() const { return subtype==SUBTYPE_VECTOR; }
template<> inline bool ASObject::is<Vector3D>() const { return subtype==SUBTYPE_VECTOR3D; }
template<> inline bool ASObject::is<VerifyError>() const { return subtype==SUBTYPE_VERIFYERROR; }
template<> inline bool ASObject::is<VertexBuffer3D>() const { return subtype==SUBTYPE_VERTEXBUFFER3D; }
template<> inline bool ASObject::is<Video>() const { return subtype==SUBTYPE_VIDEO; }
template<> inline bool ASObject::is<VideoTexture>() const { return subtype==SUBTYPE_VIDEOTEXTURE; }
template<> inline bool ASObject::is<WaitableEvent>() const { return subtype==SUBTYPE_WAITABLE_EVENT; }
template<> inline bool ASObject::is<WorkerDomain>() const { return subtype==SUBTYPE_WORKERDOMAIN; }
template<> inline bool ASObject::is<XML>() const { return subtype==SUBTYPE_XML; }
template<> inline bool ASObject::is<XMLDocument>() const { return subtype==SUBTYPE_XMLDOCUMENT; }
template<> inline bool ASObject::is<XMLNode>() const { return subtype==SUBTYPE_XMLNODE || subtype==SUBTYPE_XMLDOCUMENT; }
template<> inline bool ASObject::is<XMLList>() const { return subtype==SUBTYPE_XMLLIST; }



template<class T> inline bool asAtomHandler::is(asAtom& a) {
	return isObject(a) ? getObjectNoCheck(a)->is<T>() : false;
}
template<> inline bool asAtomHandler::is<asAtom>(asAtom& a) { return true; }
template<> inline bool asAtomHandler::is<ASObject>(asAtom& a) { return true; }
template<> inline bool asAtomHandler::is<ASString>(asAtom& a) { return isStringID(a) || isString(a); }
template<> inline bool asAtomHandler::is<Boolean>(asAtom& a) { return isBool(a); }
template<> inline bool asAtomHandler::is<Integer>(asAtom& a) { return isInteger(a); }
template<> inline bool asAtomHandler::is<Null>(asAtom& a) { return isNull(a); }
template<> inline bool asAtomHandler::is<Number>(asAtom& a) { return isNumber(a); }
template<> inline bool asAtomHandler::is<UInteger>(asAtom& a) { return isUInteger(a); }
template<> inline bool asAtomHandler::is<Undefined>(asAtom& a) { return isUndefined(a); }


FORCE_INLINE int32_t asAtomHandler::toInt(const asAtom& a)
{
	if ((a.uintval&0x7)==ATOM_INTEGER)
        return a.intval>>3;
    else if ((a.uintval&0x7)==ATOM_UINTEGER)
        return a.uintval>>3;
    else if ((a.uintval&0x7)==ATOM_INVALID_UNDEFINED_NULL_BOOL)
        return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : 0;
    else if ((a.uintval&0x7)==ATOM_STRINGID)
    {
        ASObject* s = abstract_s(getWorker(),a.uintval>>3);
        int32_t ret = s->toInt();
        s->decRef();
        return ret;
    }
    assert(getObject(a));
    return getObjectNoCheck(a)->toInt();
}
FORCE_INLINE int32_t asAtomHandler::toIntStrict(const asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(getWorker(),a.uintval>>3);
			int32_t ret = s->toIntStrict();
			s->decRef();
			return ret;
		}
		default:
			assert(getObject(a));
			return getObjectNoCheck(a)->toIntStrict();
	}
}
FORCE_INLINE number_t asAtomHandler::toNumber(const asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : (a.uintval&ATOMTYPE_UNDEFINED_BIT) ? numeric_limits<double>::quiet_NaN() : 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(getWorker(),a.uintval>>3);
			number_t ret = s->toNumber();
			s->decRef();
			return ret;
		}
		default:
			assert(getObject(a));
			return getObjectNoCheck(a)->toNumber();
	}
}
FORCE_INLINE number_t asAtomHandler::AVM1toNumber(asAtom& a, uint32_t swfversion)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : (a.uintval&ATOMTYPE_UNDEFINED_BIT) && swfversion > 6 ? numeric_limits<double>::quiet_NaN() : 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(getWorker(),a.uintval>>3);
			number_t ret = s->toNumber();
			s->decRef();
			return ret;
		}
		default:
			assert(getObject(a));
			return getObjectNoCheck(a)->toNumber();
	}
}
FORCE_INLINE bool asAtomHandler::AVM1toBool(asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_NUMBERPTR:
			return toNumber(a);
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : false;
		default:
			return true;
	}
}

FORCE_INLINE int64_t asAtomHandler::toInt64(const asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(getWorker(),a.uintval>>3);
			int64_t ret = s->toInt64();
			s->decRef();
			return ret;
		}
		default:
			assert(getObject(a));
			return getObjectNoCheck(a)->toInt64();
	}
}
FORCE_INLINE uint32_t asAtomHandler::toUInt(asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : 0;
		case ATOM_STRINGID:	
		{
			ASObject* s = abstract_s(getWorker(),a.uintval>>3);
			uint32_t ret = s->toUInt();
			s->decRef();
			return ret;
		}
		default:
			assert(getObject(a));
			return getObjectNoCheck(a)->toUInt();
	}
}

FORCE_INLINE void asAtomHandler::applyProxyProperty(asAtom& a,SystemState* sys,multiname &name)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
		case ATOM_UINTEGER:
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		case ATOM_NUMBERPTR:
		case ATOM_U_INTEGERPTR:
			break;
		case ATOM_STRINGID:
			break; // no need to create string, as it won't have a proxyMultiName
		default:
			assert(getObject(a));
			getObjectNoCheck(a)->applyProxyProperty(name);
			break;
	}
}

FORCE_INLINE bool asAtomHandler::isEqualStrict(asAtom& a, ASWorker* wrk, asAtom &v2)
{
	if(getObjectType(a)!=getObjectType(v2))
	{
		//Type conversions are ok only for numeric types
		switch(a.uintval&0x7)
		{
			case ATOM_NUMBERPTR:
			case ATOM_INTEGER:
			case ATOM_UINTEGER:
			case ATOM_U_INTEGERPTR:
				break;
			case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			{
				switch (v2.uintval&0x70)
				{
					case ATOMTYPE_NULL_BIT:
						if (!isConstructed(v2) && getObjectType(v2)!=T_CLASS)
							return true;
						return false;
					default:
						return false;
				}
				break;
			}
			case ATOM_OBJECTPTR:
				if (isNumber(a))
					break;
				return false;
			default:
				return false;
		}
		switch(v2.uintval&0x7)
		{
			case ATOM_NUMBERPTR:
			case ATOM_INTEGER:
			case ATOM_UINTEGER:
			case ATOM_U_INTEGERPTR:
				break;
			case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			{
				switch (v2.uintval&0x70)
				{
					case ATOMTYPE_NULL_BIT:
						if (!isConstructed(a) && getObjectType(a)!=T_CLASS)
							return true;
						return false;
					default:
						return false;
				}
				break;
			}
			case ATOM_OBJECTPTR:
				if (isNumber(v2))
					break;
				return false;
			default:
				return false;
		}
	}
	return isEqual(a,wrk,v2);
}

FORCE_INLINE bool asAtomHandler::isConstructed(const asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
		case ATOM_UINTEGER:
		case ATOM_NUMBERPTR:
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			return a.uintval;
		case ATOM_STRINGID:
			return true;
		default:
			return isObject(a) && getObjectNoCheck(a)->isConstructed();
	}
}

FORCE_INLINE bool asAtomHandler::isPrimitive(const asAtom& a)
{
	// ECMA 3, section 4.3.2, T_INTEGER and T_UINTEGER are added
	// because they are special cases of Number
	return isUndefined(a) || isNull(a) || isNumber(a) || isString(a) || isInteger(a)|| isUInteger(a) || isBool(a);
}
FORCE_INLINE bool asAtomHandler::checkArgumentConversion(const asAtom& a,const asAtom& obj)
{
	if ((a.uintval&0x7) == (obj.uintval&0x7))
	{
		if ((a.uintval&0x7) == ATOM_OBJECTPTR)
			return getObjectNoCheck(a)->getObjectType() == getObjectNoCheck(obj)->getObjectType();
		return true;
	}
	if (isNumeric(a) && isNumeric(obj))
		return true;
	if (isString(a) && isString(obj))
		return true;
	return asAtomHandler::isInvalid(obj);
}

FORCE_INLINE void asAtomHandler::setInt(asAtom& a, ASWorker* wrk, int64_t val)
{
	const int32_t val32 = val;
#ifdef LIGHTSPARK_64
	a.intval = ((int64_t)val32<<3)|ATOM_INTEGER;
#else
	if (val32 >=-(1<<28)  && val32 <=(1<<28))
		a.intval = (val32<<3)|ATOM_INTEGER;
	else
		setNumber(a,wrk,val32);
#endif
}

FORCE_INLINE void asAtomHandler::setUInt(asAtom& a, ASWorker* wrk, uint32_t val)
{
#ifdef LIGHTSPARK_64
	a.uintval = ((uint64_t)val<<3)|ATOM_UINTEGER;
#else
	if (val <(1<<29))
		a.uintval = (val<<3)|ATOM_UINTEGER;
	else
		setNumber(a,wrk,val);
#endif
}

FORCE_INLINE void asAtomHandler::setBool(asAtom& a,bool val)
{
	a.uintval = ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_BOOL_BIT | (val ? 0x80 : 0);
}

FORCE_INLINE void asAtomHandler::setNull(asAtom& a)
{
	a.uintval = ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_NULL_BIT;
}
FORCE_INLINE void asAtomHandler::setUndefined(asAtom& a)
{
	a.uintval = ATOM_INVALID_UNDEFINED_NULL_BOOL | ATOMTYPE_UNDEFINED_BIT;
}
FORCE_INLINE bool asAtomHandler::increment(asAtom& a, ASWorker* wrk, bool replace)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					setInt(a,wrk,1);
					break;
				case ATOMTYPE_UNDEFINED_BIT:
					setNumber(a,wrk,numeric_limits<double>::quiet_NaN());
					break;
				case ATOMTYPE_BOOL_BIT:
					setInt(a,wrk,(a.uintval & 0x80 ? 1 : 0)+1);
					break;
				default:
					return true;
			}
			break;
		}
		case ATOM_INTEGER:
			setInt(a,wrk,(a.intval>>3)+1);
			break;
		case ATOM_UINTEGER:
			setUInt(a,wrk,(a.uintval>>3)+1);
			break;
		case ATOM_NUMBERPTR:
		{
			number_t n = getObjectNoCheck(a)->toNumber();
			if (std::isnan(n) || std::isinf(n))
				setNumber(a,wrk,n);
			else if(trunc(n) == n && n < INT32_MAX)
			{
				if (replace)
					ASATOM_DECREF(a);
				asAtomHandler::setInt(a,wrk,n+1);
			}
			else if (replace)
				return replaceNumber(a,wrk,n+1);
			else
				setNumber(a,wrk,n+1);
			break;
		}
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(wrk,a.uintval>>3);
			number_t n = s->toNumber();
			setNumber(a,wrk,n+1);
			s->decRef();
			break;
		}
		default:
		{
			number_t n=toNumber(a);
			if (replace)
				ASATOM_DECREF(a);
			if (std::isnan(n) || std::isinf(n))
				setNumber(a,wrk,n);
			else if(trunc(n) == n)
				asAtomHandler::setInt(a,wrk,n+1);
			else
				setNumber(a,wrk,n+1);
			break;
		}
	}
	return true;
}

FORCE_INLINE bool asAtomHandler::decrement(asAtom& a, ASWorker* wrk, bool replace)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					setInt(a,wrk,-1);
					break;
				case ATOMTYPE_UNDEFINED_BIT:
					setNumber(a,wrk,numeric_limits<double>::quiet_NaN());
					break;
				case ATOMTYPE_BOOL_BIT:
					setInt(a,wrk,(a.uintval & 0x80 ? 1 : 0)-1);
					break;
				default:
					return true;
			}
			break;
		}
		case ATOM_INTEGER:
			setInt(a,wrk,(a.uintval>>3)-1);
			break;
		case ATOM_UINTEGER:
		{
			if (a.uintval>>3 > 0)
				setUInt(a,wrk,(a.uintval>>3)-1);
			else
			{
				number_t val = a.uintval>>3;
				setNumber(a,wrk,val-1);
			}
			break;
		}
		case ATOM_NUMBERPTR:
		{
			number_t n = getObjectNoCheck(a)->toNumber();
			if (std::isnan(n) || std::isinf(n))
				setNumber(a,wrk,n);
			else if(trunc(n) == n && n > INT32_MIN)
			{
				if (replace)
					ASATOM_DECREF(a);
				asAtomHandler::setInt(a,wrk,n-1);
			}
			else if (replace)
				return replaceNumber(a,wrk,n-1);
			else
				setNumber(a,wrk,n-1);
			break;
		}
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(wrk,a.uintval>>3);
			number_t n = s->toNumber();
			setNumber(a,wrk,n-1);
			s->decRef();
			break;
		}
		default:
		{
			number_t n=toNumber(a);
			if (replace)
				ASATOM_DECREF(a);
			if (std::isnan(n) || std::isinf(n))
				setNumber(a,wrk,n);
			else if(trunc(n) == n)
				asAtomHandler::setInt(a,wrk,n-1);
			else
				setNumber(a,wrk,n-1);
			break;
		}
	}
	return true;
}

FORCE_INLINE void asAtomHandler::increment_i(asAtom& a, ASWorker* wrk, int32_t amount)
{
	if ((a.uintval&0x7) == ATOM_INTEGER)
		setInt(a,wrk,int32_t(a.intval>>3)+amount);
	else
		setInt(a,wrk,toInt(a)+amount);
}
FORCE_INLINE void asAtomHandler::decrement_i(asAtom& a, ASWorker* wrk, int32_t amount)
{
	if ((a.uintval&0x7) == ATOM_INTEGER)
		setInt(a,wrk,int32_t(a.intval>>3)-amount);
	else
		setInt(a,wrk,toInt(a)-amount);
}

FORCE_INLINE void asAtomHandler::bit_xor(asAtom& a, ASWorker* wrk, asAtom &v1)
{
	int32_t i1=toInt(v1);
	int32_t i2=toInt(a);
	LOG_CALL("bitXor " << std::hex << i1 << '^' << i2 << std::dec);
	setInt(a,wrk,i1^i2);
}

FORCE_INLINE void asAtomHandler::bitnot(asAtom& a, ASWorker* wrk)
{
	int32_t i1=toInt(a);
	LOG_CALL("bitNot " << std::hex << i1 << std::dec);
	setInt(a,wrk,~i1);
}

FORCE_INLINE void asAtomHandler::subtract(asAtom& a, ASWorker* wrk, asAtom &v2, bool forceint)
{
	if( ((a.uintval&0x7) == ATOM_INTEGER || (a.uintval&0x7) == ATOM_UINTEGER) &&
		(isInteger(v2) || (v2.uintval&0x7) ==ATOM_UINTEGER))
	{
		int64_t num1=toInt64(a);
		int64_t num2=toInt64(v2);
	
		LOG_CALL("subtractI " << num1 << '-' << num2);
		int64_t res = num1-num2;
		if (forceint || (res > INT32_MIN>>3 && res < INT32_MAX>>3))
			setInt(a,wrk,int32_t(res));
		else if (res >= 0 && res < UINT32_MAX>>3)
			setUInt(a,wrk,res);
		else
			setNumber(a,wrk,res);
	}
	else
	{
		number_t num2=toNumber(v2);
		number_t num1=toNumber(a);
	
		LOG_CALL("subtract " << num1 << '-' << num2);
		if (forceint)
			setInt(a,wrk,num1-num2);
		else
			setNumber(a,wrk,num1-num2);
	}
}
FORCE_INLINE void asAtomHandler::subtractreplace(asAtom& ret, ASWorker* wrk, const asAtom &v1, const asAtom &v2, bool forceint)
{
	if( ((v1.uintval&0x7) == ATOM_INTEGER || (v1.uintval&0x7) == ATOM_UINTEGER) &&
		(isInteger(v2) || (v2.uintval&0x7) ==ATOM_UINTEGER))
	{
		int64_t num1=toInt64(v1);
		int64_t num2=toInt64(v2);
	
		LOG_CALL("subtractreplaceI " << num1 << '-' << num2);
		ASATOM_DECREF(ret);
		int64_t res = num1-num2;
		if (forceint || (res > INT32_MIN>>3 && res < INT32_MAX>>3))
			setInt(ret,wrk,int32_t(res));
		else if (res >= 0 && res < UINT32_MAX>>3)
			setUInt(ret,wrk,res);
		else
			setNumber(ret,wrk,res);
	}
	else
	{
		number_t num2=toNumber(v2);
		number_t num1=toNumber(v1);
	
		ASObject* o = getObject(ret);
		LOG_CALL("subtractreplace "  << num1 << '-' << num2);
		if (forceint)
		{
			setInt(ret,wrk,num1-num2);
			if (o)
				o->decRef();
		}
		else if (replaceNumber(ret,wrk,num1-num2) && o)
			o->decRef();
	}
}

FORCE_INLINE void asAtomHandler::multiply(asAtom& a, ASWorker* wrk, asAtom &v2, bool forceint)
{
	if( ((a.uintval&0x7) == ATOM_INTEGER || (a.uintval&0x7) == ATOM_UINTEGER) &&
		(isInteger(v2) || (v2.uintval&0x7) ==ATOM_UINTEGER))
	{
		int64_t num1=toInt64(a);
		int64_t num2=toInt64(v2);
	
		LOG_CALL("multiplyI " << num1 << '*' << num2);
		int64_t res = num1*num2;
		if (forceint || (res > INT32_MIN>>3 && res < INT32_MAX>>3))
			setInt(a,wrk,int32_t(res));
		else if (res >= 0 && res < UINT32_MAX>>3)
			setUInt(a,wrk,res);
		else
			setNumber(a,wrk,res);
	}
	else
	{
		double num1=toNumber(v2);
		double num2=toNumber(a);
		LOG_CALL("multiply "  << num1 << '*' << num2);
		if (forceint)
			setInt(a,wrk,num1*num2);
		else 
			setNumber(a,wrk,num1*num2);
	}
}

FORCE_INLINE void asAtomHandler::multiplyreplace(asAtom& ret, ASWorker* wrk, const asAtom& v1, const asAtom &v2, bool forceint)
{
	if( ((v1.uintval&0x7) == ATOM_INTEGER || (v1.uintval&0x7) == ATOM_UINTEGER) &&
		(isInteger(v2) || (v2.uintval&0x7) ==ATOM_UINTEGER))
	{
		int64_t num1=toInt64(v1);
		int64_t num2=toInt64(v2);
	
		LOG_CALL("multiplyreplaceI " << num1 << '*' << num2);
		
		ASObject* o = getObject(ret);
		int64_t res = num1*num2;
		
		if (forceint || (res > INT32_MIN>>3 && res < INT32_MAX>>3))
		{
			setInt(ret,wrk,int32_t(res));
			if (o)
				o->decRef();
		}
		else if (res >= 0 && res < UINT32_MAX>>3)
		{
			setUInt(ret,wrk,res);
			if (o)
				o->decRef();
		}
		else if (replaceNumber(ret,wrk,num1*num2) && o)
			o->decRef();
	}
	else
	{
		double num1=toNumber(v2);
		double num2=toNumber(v1);
		ASObject* o = getObject(ret);
		LOG_CALL("multiplyreplace "  << num1 << '*' << num2);
		if (forceint)
		{
			setInt(ret,wrk,num1*num2);
			if (o)
				o->decRef();
		}
		else if (replaceNumber(ret,wrk,num1*num2) && o)
			o->decRef();
	}
}

FORCE_INLINE void asAtomHandler::divide(asAtom& a, ASWorker* wrk, asAtom &v2, bool forceint)
{
	double num1=toNumber(a);
	double num2=toNumber(v2);

	LOG_CALL("divide "  << num1 << '/' << num2);
	// handling of infinity according to ECMA-262, chapter 11.5.2
	if (std::isinf(num1))
	{
		if (std::isinf(num2) || std::isnan(num2))
			setNumber(a,wrk,numeric_limits<double>::quiet_NaN());
		else
		{
			bool needssign = (std::signbit(num1) || std::signbit(num2)) && !(std::signbit(num1) && std::signbit(num2)); 
			setNumber(a,wrk, needssign  ? -numeric_limits<double>::infinity() : numeric_limits<double>::infinity());
		}
	}
	else if (forceint)
		setInt(a,wrk,num1/num2);
	else
		setNumber(a,wrk,num1/num2);
}
FORCE_INLINE void asAtomHandler::dividereplace(asAtom& ret, ASWorker* wrk, const asAtom& v1, const asAtom &v2, bool forceint)
{
	double num1=toNumber(v1);
	double num2=toNumber(v2);

	number_t res=0;
	LOG_CALL("divide "  << num1 << '/' << num2);
	// handling of infinity according to ECMA-262, chapter 11.5.2
	if (std::isinf(num1))
	{
		if (std::isinf(num2) || std::isnan(num2))
			res = numeric_limits<double>::quiet_NaN();
		else
		{
			bool needssign = (std::signbit(num1) || std::signbit(num2)) && !(std::signbit(num1) && std::signbit(num2)); 
			res = needssign  ? -numeric_limits<double>::infinity() : numeric_limits<double>::infinity();
		}
	}
	else
		res = num1/num2;
	
	ASObject* o = getObject(ret);
	LOG_CALL("dividereplace "  << num1 << '/' << num2);
	if (forceint)
	{
		setInt(ret,wrk,num1/num2);
		if (o)
			o->decRef();
	}
	else if (replaceNumber(ret,wrk,res) && o)
		o->decRef();
	
}

FORCE_INLINE void asAtomHandler::modulo(asAtom& a, ASWorker* wrk, asAtom &v2, bool forceint)
{
	// if both values are Integers the result is also an int
	if( ((a.uintval&0x7) == ATOM_INTEGER || (a.uintval&0x7) == ATOM_UINTEGER) &&
		(isInteger(v2) || (v2.uintval&0x7) ==ATOM_UINTEGER))
	{
		int32_t num1=toInt(a);
		int32_t num2=toInt(v2);
		LOG_CALL("moduloI "  << num1 << '%' << num2);
		if (!forceint && num2 == 0)
			setNumber(a,wrk,numeric_limits<double>::quiet_NaN());
		else
			setInt(a,wrk,num1%num2);
	}
	else
	{
		number_t num1=toNumber(a);
		number_t num2=toNumber(v2);
		LOG_CALL("modulo "  << num1 << '%' << num2);
		/* fmod returns NaN if num2 == 0 as the spec mandates */
		if (forceint)
			setInt(a,wrk,::fmod(num1,num2));
		else 
			setNumber(a,wrk,::fmod(num1,num2));
	}
}
FORCE_INLINE void asAtomHandler::moduloreplace(asAtom& ret, ASWorker* wrk, const asAtom& v1, const asAtom &v2, bool forceint)
{
	// if both values are Integers the result is also an int
	if( ((v1.uintval&0x7) == ATOM_INTEGER || (v1.uintval&0x7) == ATOM_UINTEGER) &&
		(isInteger(v2) || (v2.uintval&0x7) ==ATOM_UINTEGER))
	{
		int32_t num1=toInt(v1);
		int32_t num2=toInt(v2);
		ASATOM_DECREF(ret);
		LOG_CALL("moduloreplaceI "  << num1 << '%' << num2);
		if (!forceint && num2 == 0)
			setNumber(ret,wrk,numeric_limits<double>::quiet_NaN());
		else
			setInt(ret,wrk,num2 == 0 ? 0 :num1%num2);
	}
	else
	{
		number_t num1=toNumber(v1);
		number_t num2=toNumber(v2);
		LOG_CALL("moduloreplace "  << num1 << '%' << num2);
		/* fmod returns NaN if num2 == 0 as the spec mandates */
		ASObject* o = getObject(ret);
		if (forceint)
		{
			setInt(ret,wrk,::fmod(num1,num2));
			if (o)
				o->decRef();
		}
		else if (replaceNumber(ret,wrk,::fmod(num1,num2)) && o)
			o->decRef();
	}
}

FORCE_INLINE void asAtomHandler::lshift(asAtom& a, ASWorker* wrk, asAtom &v1)
{
	int32_t i2=toInt(a);
	uint32_t i1=toUInt(v1)&0x1f;
	LOG_CALL("lShift "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	//Left shift are supposed to always work in 32bit
	setInt(a,wrk,i2<<i1);
}

FORCE_INLINE void asAtomHandler::rshift(asAtom& a, ASWorker* wrk, asAtom &v1)
{
	int32_t i2=toInt(a);
	uint32_t i1=toUInt(v1)&0x1f;
	LOG_CALL("rShift "<<std::hex<<i2<<">>"<<i1<<std::dec);
	setInt(a,wrk,i2>>i1);
}

FORCE_INLINE void asAtomHandler::urshift(asAtom& a, ASWorker* wrk, asAtom &v1)
{
	uint32_t i2=toUInt(a);
	uint32_t i1=toUInt(v1)&0x1f;
	LOG_CALL("urShift "<<std::hex<<i2<<">>"<<i1<<std::dec);
	setUInt(a,wrk,i2>>i1);
}
FORCE_INLINE void asAtomHandler::bit_and(asAtom& a, ASWorker* wrk, asAtom &v1)
{
	int32_t i1=toInt(v1);
	int32_t i2=toInt(a);
	LOG_CALL("bitAnd_oo " << std::hex << i1 << '&' << i2 << std::dec);
	setInt(a,wrk,i1&i2);
}
FORCE_INLINE void asAtomHandler::bit_or(asAtom& a, ASWorker* wrk, asAtom &v1)
{
	int32_t i1=toInt(v1);
	int32_t i2=toInt(a);
	LOG_CALL("bitOr " << std::hex << i1 << '|' << i2 << std::dec);
	setInt(a,wrk,i1|i2);
}
FORCE_INLINE void asAtomHandler::_not(asAtom& a)
{
	LOG_CALL("not:" <<toDebugString(a)<<" "<<!Boolean_concrete(a));
	
	bool ret=!Boolean_concrete(a);
	ASATOM_DECREF(a);
	setBool(a,ret);
}

FORCE_INLINE void asAtomHandler::negate_i(asAtom& a, ASWorker* wrk)
{
	LOG_CALL("negate_i");
	int n=toInt(a);
	setInt(a,wrk,-n);
}

FORCE_INLINE void asAtomHandler::add_i(asAtom& a, ASWorker* wrk, asAtom &v2)
{
	int64_t num2=toInt(v2);
	int64_t num1=toInt(a);

	LOG_CALL("add_i " << num1 << '+' << num2);
	int64_t res = num1+num2;
	if (res >= INT32_MAX || res <= INT32_MIN)
		setNumber(a,wrk,res);
	else
		setInt(a,wrk,res);
}

FORCE_INLINE void asAtomHandler::subtract_i(asAtom& a, ASWorker* wrk, asAtom &v2)
{
	int num2=toInt(v2);
	int num1=toInt(a);

	LOG_CALL("subtract_i " << num1 << '-' << num2);
	setInt(a,wrk,num1-num2);
}

FORCE_INLINE void asAtomHandler::multiply_i(asAtom& a, ASWorker* wrk, asAtom &v2)
{
	int num1=toInt(a);
	int num2=toInt(v2);
	LOG_CALL("multiply_i "  << num1 << '*' << num2);
	int64_t res = num1*num2;
	setInt(a,wrk,res);
}

FORCE_INLINE bool asAtomHandler::isNumeric(const asAtom& a)
{
	return (isInteger(a) || isUInteger(a) || isNumber(a));
}
FORCE_INLINE bool asAtomHandler::isNumber(const asAtom& a)
{
	return (a.uintval&0x7)==ATOM_NUMBERPTR;
}
FORCE_INLINE bool asAtomHandler::isInteger(const asAtom& a)
{ 
	return (a.uintval&0x3) == ATOM_INTEGER || ((a.uintval&0x7) == ATOM_U_INTEGERPTR && isObject(a) && getObjectNoCheck(a)->getObjectType() == T_INTEGER);
}
FORCE_INLINE bool asAtomHandler::isUInteger(const asAtom& a)
{ 
	return (a.uintval&0x7) == ATOM_UINTEGER || ((a.uintval&0x7) == ATOM_U_INTEGERPTR  && isObject(a) && getObjectNoCheck(a)->getObjectType() == T_UINTEGER);
}
FORCE_INLINE asAtom asAtomHandler::fromObjectNoPrimitive(ASObject* obj)
{
	assert(!obj->isPrimitive());
	asAtom a;
	a.uintval = ATOM_OBJECTPTR | (LIGHTSPARK_ATOM_VALTYPE)obj;
	return a;
}

FORCE_INLINE SWFOBJECT_TYPE asAtomHandler::getObjectType(const asAtom& a)
{
	switch (a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return T_INTEGER;
		case ATOM_UINTEGER:
			return T_UINTEGER;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
			switch (a.uintval&0xf0)
			{
				case ATOMTYPE_NULL_BIT: 
					return T_NULL;
				case ATOMTYPE_UNDEFINED_BIT: // UNDEFINED
					return T_UNDEFINED;
				default: // BOOL
					return T_BOOLEAN;
			}
		case ATOM_NUMBERPTR:
			return T_NUMBER;
		case ATOM_STRINGID:
		case ATOM_STRINGPTR:
			return T_STRING;
		case ATOM_U_INTEGERPTR:
		case ATOM_OBJECTPTR:
			return isObject(a) ? getObjectNoCheck(a)->getObjectType() : T_INVALID;
		default:
			return T_INVALID;
	}
}
FORCE_INLINE asAtom asAtomHandler::typeOf(asAtom& a)
{
	BUILTIN_STRINGS ret=BUILTIN_STRINGS::STRING_OBJECT;
	switch(a.uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
					ret= BUILTIN_STRINGS::STRING_OBJECT;
					break;
				case ATOMTYPE_UNDEFINED_BIT:
					ret=BUILTIN_STRINGS::STRING_UNDEFINED;
					break;
				case ATOMTYPE_BOOL_BIT:
					ret=BUILTIN_STRINGS::STRING_BOOLEAN;
					break;
				default:
					break;
			}
			break;
		}
		case ATOM_OBJECTPTR:
			if(getObjectNoCheck(a)->is<XML>() || getObjectNoCheck(a)->is<XMLList>())
				ret = BUILTIN_STRINGS::STRING_XML;
			if(getObjectNoCheck(a)->is<Number>() || getObjectNoCheck(a)->is<Integer>() || getObjectNoCheck(a)->is<UInteger>())
				ret = BUILTIN_STRINGS::STRING_NUMBER;
			if(getObjectNoCheck(a)->is<ASString>())
				ret = BUILTIN_STRINGS::STRING_STRING;
			if(getObjectNoCheck(a)->is<IFunction>())
				ret = BUILTIN_STRINGS::STRING_FUNCTION_LOWERCASE;
			if(getObjectNoCheck(a)->is<Undefined>())
				ret = BUILTIN_STRINGS::STRING_UNDEFINED;
			if(getObjectNoCheck(a)->is<Boolean>())
				ret = BUILTIN_STRINGS::STRING_BOOLEAN;
			break;
		case ATOM_NUMBERPTR:
		case ATOM_INTEGER:
		case ATOM_UINTEGER:
		case ATOM_U_INTEGERPTR:
			ret=BUILTIN_STRINGS::STRING_NUMBER;
			break;
		case ATOM_STRINGID:
		case ATOM_STRINGPTR:
			ret = BUILTIN_STRINGS::STRING_STRING;
			break;
		default:
			break;
	}
	return asAtomHandler::fromStringID(ret);
}
bool asAtomHandler::isEqual(asAtom& a, ASWorker* wrk, asAtom &v2)
{
	if ((((a.intval ^ ATOM_INTEGER) | (v2.intval ^ ATOM_INTEGER)) & 7) == 0)
		return (a.intval == v2.intval);
	if (a.uintval == v2.uintval && 
			((a.uintval&0x7) != ATOM_NUMBERPTR)) // number needs special handling for NaN
		return true;
	return isEqualIntern(a,wrk,v2);
}
TRISTATE asAtomHandler::isLess(asAtom& a, ASWorker* wrk, asAtom &v2)
{
	if ((((a.intval ^ ATOM_INTEGER) | (v2.intval ^ ATOM_INTEGER)) & 7) == 0)
		return (a.intval < v2.intval)?TTRUE:TFALSE;
	if (a.uintval == v2.uintval && 
			((a.uintval&0x7) != ATOM_NUMBERPTR)) // number needs special handling for NaN
	{
		return a.uintval == ATOMTYPE_UNDEFINED_BIT ? TUNDEFINED : TFALSE;
	}
	return isLessIntern(a,wrk,v2);
}
/* implements ecma3's ToBoolean() operation, see section 9.2, but returns the value instead of an Boolean object */
FORCE_INLINE bool asAtomHandler::Boolean_concrete(asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL:
		{
			switch (a.uintval&0x70)
			{
				case ATOMTYPE_NULL_BIT:
				case ATOMTYPE_UNDEFINED_BIT:
					return false;
				case ATOMTYPE_BOOL_BIT:
					return (a.uintval&0x80)>>7;
				default:
					return false;
			}
			break;
		}
		case ATOM_NUMBERPTR:
			return toNumber(a) != 0.0 && !std::isnan(toNumber(a));
		case ATOM_INTEGER:
			return (a.intval>>3) != 0;
		case ATOM_UINTEGER:
			return (a.uintval>>3) != 0;
		case ATOM_STRINGID:
			return (a.uintval>>3) != BUILTIN_STRINGS::EMPTY;
		default:
			return Boolean_concrete_object(a);
	}
}

FORCE_INLINE bool asAtomHandler::isFunction(const asAtom& a) { return isObject(a) && getObjectNoCheck(a)->is<IFunction>(); }
FORCE_INLINE bool asAtomHandler::isString(const asAtom& a) { return isStringID(a) || (isObject(a) && getObjectNoCheck(a)->is<ASString>()); }
FORCE_INLINE bool asAtomHandler::isQName(const asAtom& a) { return getObject(a) && getObjectNoCheck(a)->is<ASQName>(); }
FORCE_INLINE bool asAtomHandler::isNamespace(const asAtom& a) { return isObject(a) && getObjectNoCheck(a)->is<Namespace>(); }
FORCE_INLINE bool asAtomHandler::isArray(const asAtom& a) { return isObject(a) && getObjectNoCheck(a)->is<Array>(); }
FORCE_INLINE bool asAtomHandler::isClass(const asAtom& a) { return isObject(a) && getObjectNoCheck(a)->is<Class_base>(); }
FORCE_INLINE bool asAtomHandler::isTemplate(const asAtom& a) { return isObject(a) && getObjectNoCheck(a)->getObjectType() == T_TEMPLATE; }

FORCE_INLINE ASObject* asAtomHandler::getObject(const asAtom& a)
{
	assert(!(a.uintval & ATOMTYPE_OBJECT_BIT) || !((ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)))->getCached() || ((ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)))->getInDestruction());
	return a.uintval & ATOMTYPE_OBJECT_BIT ? (ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)) : nullptr;
}
FORCE_INLINE ASObject* asAtomHandler::getObjectNoCheck(const asAtom& a)
{
	assert(!(a.uintval & ATOMTYPE_OBJECT_BIT) || !((ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)))->getCached() || ((ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)))->getInDestruction());
	return (ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7));
}
FORCE_INLINE void asAtomHandler::resetCached(const asAtom& a)
{
	ASObject* o = a.uintval & ATOMTYPE_OBJECT_BIT ? (ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)) : nullptr;
	if (o)
		o->resetCached();
}

inline ASObject* asfreelist::getObjectFromFreeList()
{
	assert(freelistsize>=0);
	ASObject* o = freelistsize ? freelist[--freelistsize] :nullptr;
	LOG_CALL("getfromfreelist:"<<freelistsize<<" "<<o<<" "<<this);
	return o;
}
inline bool asfreelist::pushObjectToFreeList(ASObject *obj)
{
	if (freelistsize < FREELIST_SIZE)
	{
		assert(freelistsize>=0);
		LOG_CALL("pushtofreelist:"<<freelistsize<<" "<<obj<<" "<<this);
		obj->setCached();
		obj->resetRefCount();
		freelist[freelistsize++]=obj;
		return true;
	}
	return false;
}

struct abc_limits {
	/* maxmium number of recursion allowed. See ScriptLimitsTag */
	uint32_t max_recursion;
	/* maxmium number of seconds for script execution. See ScriptLimitsTag */
	uint32_t script_timeout;
};

struct stacktrace_entry
{
	asAtom object;
	uint32_t name;
	void set(asAtom o, uint32_t n) { object=o; name=n; }
};

}
#endif /* ASOBJECT_H */
