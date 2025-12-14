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

#define ASPROPERTY_STATIC(type, class, name) \
	type static_##class##_##name

/* declare setter/getter for already existing member variable */
#define ASFUNCTION_GETTER(name) \
	ASFUNCTION_ATOM( _getter_##name)

#define ASFUNCTION_SETTER(name) \
	ASFUNCTION_ATOM( _setter_##name)

#define ASFUNCTION_GETTER_SETTER(name) \
	ASFUNCTION_ATOM( _getter_##name); \
	ASFUNCTION_ATOM( _setter_##name)


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
struct call_context;

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

enum CONST_ALLOWED_FLAG { CONST_ALLOWED=0, CONST_NOT_ALLOWED };
enum METHOD_TYPE { NORMAL_METHOD=0, SETTER_METHOD=1, GETTER_METHOD=2 };
//for toPrimitive
enum TP_HINT { NO_HINT, NUMBER_HINT, STRING_HINT };
enum TRAIT_KIND { NO_CREATE_TRAIT=0, DECLARED_TRAIT=1, DYNAMIC_TRAIT=2, INSTANCE_TRAIT=5, CONSTANT_TRAIT=9 /* constants are also declared traits */ };
enum GET_VARIABLE_RESULT {GETVAR_NORMAL=0x00, GETVAR_CACHEABLE=0x01, GETVAR_ISGETTER=0x02, GETVAR_ISCONSTANT=0x04, GETVAR_ISINCREFFED=0x08};
enum GET_VARIABLE_OPTION {NONE=0x00, SKIP_IMPL=0x01, FROM_GETLEX=0x02, DONT_CALL_GETTER=0x04, NO_INCREF=0x08, DONT_CHECK_CLASS=0x10, DONT_CHECK_PROTOTYPE=0x20};

// asAtom is a 32bit value (64bit on 64bit architecture):
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
// 0000 1000: local number (index in call_context is stored in upper 3 bytes)
// dddd d001: uint
// dddd d101: Number
// dddd d010: stringID
// dddd d110: ASString
// dddd d011: int
// dddd d111: (U)Integer
// dddd d100: ASObject
enum ATOM_TYPE
{
	ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER=0x0,
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
#define ATOMTYPE_LOCALNUMBER_BIT 0x8
#define ATOMTYPE_BIT_FLAGS (ATOMTYPE_NULL_BIT|ATOMTYPE_UNDEFINED_BIT|ATOMTYPE_BOOL_BIT|ATOMTYPE_LOCALNUMBER_BIT)
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
				a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER;
				break;
			case T_UNDEFINED:
				a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER | ATOMTYPE_UNDEFINED_BIT;
				break;
			case T_NULL:
				a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER | ATOMTYPE_NULL_BIT;
				break;
			case T_BOOLEAN:
				a.uintval=ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER | ATOMTYPE_BOOL_BIT;
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
		a.uintval =(ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER | ATOMTYPE_BOOL_BIT | (val ? 0x80 : 0));
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
	static asAtom getClosureAtom(asAtom& a, asAtom defaultAtom);
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
	static void callFunction(asAtom& caller, ASWorker* wrk, asAtom& ret, asAtom &obj, asAtom *args, uint32_t num_args, bool args_refcounted, bool coerceresult=true, bool coercearguments=true, bool isAVM1InternalCall=false, uint16_t resultlocalnumberpos=UINT16_MAX);
	static multiname* getVariableByMultiname(asAtom& a, asAtom &ret, const multiname& name, ASWorker* wrk, bool& canCache, GET_VARIABLE_OPTION opt, uint16_t resultlocalnumberpos=UINT16_MAX);
	static void getVariableByInteger(asAtom& a, asAtom &ret, int index, ASWorker* wrk);
	static bool hasPropertyByMultiname(const asAtom& a, const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk);
	static GET_VARIABLE_RESULT AVM1getVariableByMultiname(const asAtom& a,asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk, bool isSlashPath);
	static bool AVM1setVariableByMultiname(const asAtom& a,multiname& name, asAtom& value, CONST_ALLOWED_FLAG allowConst, ASWorker* wrk);
	static Class_base* getClass(const asAtom& a,SystemState *sys, bool followclass=true);
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
	static TRISTATE AVM1isLess(asAtom& a, ASWorker* wrk, asAtom& v2);
	static FORCE_INLINE bool isEqual(asAtom& a, ASWorker* wrk, asAtom& v2);
	static bool AVM1isEqual(asAtom& v1, asAtom& v2, ASWorker* wrk);
	static FORCE_INLINE bool isEqualStrict(asAtom& a, ASWorker* wrk, asAtom& v2);
	static bool AVM1isEqualStrict(asAtom& a, asAtom& b, ASWorker* wrk);
	static FORCE_INLINE LIGHTSPARK_ATOM_VALTYPE getType(const asAtom& a)
	{
		return
		(
			(a.uintval & 0x7) == ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER ?
			a.uintval & 0x70 :
			a.uintval & 0x7
		);
	}
	static FORCE_INLINE bool isSameType(const asAtom& a, const asAtom& b)
	{
		return
		(
			isExactSameType(a, b) ||
			(isNumeric(a) && isNumeric(b)) ||
			(isString(a) && isString(b))
		);
	}
	static call_context* getCallContext(ASWorker* wrk);
	static FORCE_INLINE bool isExactSameType(const asAtom& a, const asAtom& b) { return getType(a) == getType(b); }
	static FORCE_INLINE bool isConstructed(const asAtom& a);
	static FORCE_INLINE bool isPrimitive(const asAtom& a);
	static FORCE_INLINE bool isNumeric(const asAtom& a);
	static FORCE_INLINE bool isNumber(const asAtom& a);
	static FORCE_INLINE bool isNumberPtr(const asAtom& a);
	static FORCE_INLINE bool isLocalNumber(const asAtom& a);
	static FORCE_INLINE bool isValid(const asAtom& a) { return a.uintval; }
	static FORCE_INLINE bool isInvalid(const asAtom& a) { return !a.uintval; }
	static FORCE_INLINE bool isNull(const asAtom& a) { return (a.uintval&0x7f) == ATOMTYPE_NULL_BIT; }
	static FORCE_INLINE bool isUndefined(const asAtom& a) { return (a.uintval&0x7f) == ATOMTYPE_UNDEFINED_BIT; }
	static FORCE_INLINE bool isNullOrUndefined(const asAtom& a) { return isNull(a) || isUndefined(a); }
	static FORCE_INLINE bool isBool(const asAtom& a) { return (a.uintval&0x7f) == ATOMTYPE_BOOL_BIT; }
	static FORCE_INLINE bool isInteger(const asAtom& a);
	static FORCE_INLINE bool isUInteger(const asAtom& a);
	static FORCE_INLINE bool isObject(const asAtom& a) { return a.uintval & ATOMTYPE_OBJECT_BIT; }
	static FORCE_INLINE bool isObjectPtr(const asAtom& a) { return (a.uintval & 0x7) == ATOM_OBJECTPTR; }
	static FORCE_INLINE bool isFunction(const asAtom& a);
	static FORCE_INLINE bool isString(const asAtom& a);
	static FORCE_INLINE bool isStringID(const asAtom& a) { return (a.uintval&0x7) == ATOM_STRINGID; }
	static FORCE_INLINE bool isQName(const asAtom& a);
	static FORCE_INLINE bool isNamespace(const asAtom& a);
	static FORCE_INLINE bool isArray(const asAtom& a);
	static FORCE_INLINE bool isClass(const asAtom& a);
	static FORCE_INLINE bool isTemplate(const asAtom& a);
	static FORCE_INLINE bool isAccessible(const asAtom& a);
	static FORCE_INLINE bool isAccessibleObject(const asAtom& a);
	static FORCE_INLINE SWFOBJECT_TYPE getObjectType(const asAtom& a);
	static FORCE_INLINE bool checkArgumentConversion(const asAtom& a,const asAtom& obj);
	static bool localNumberToGlobalNumber(ASWorker *wrk, asAtom& a);
	static asAtom asTypelate(asAtom& a, asAtom& b, ASWorker* wrk);
	static bool isTypelate(asAtom& a,ASObject* type);
	static bool isTypelate(asAtom& a,asAtom& t);
	static bool AVM1toPrimitive(asAtom& ret, ASWorker* wrk, bool& isRefCounted, const TP_HINT& hint = NO_HINT);
	static FORCE_INLINE number_t getNumber(ASWorker* wrk, const asAtom& a);
	static FORCE_INLINE number_t toNumber(const asAtom& a);
	static inline number_t AVM1toNumber(const asAtom& a, uint32_t swfversion, bool primitiveHint = false);
	static FORCE_INLINE bool AVM1toBool(asAtom& a, ASWorker* wrk, uint32_t swfversion);
	static FORCE_INLINE int32_t toInt(const asAtom& a);
	static FORCE_INLINE int32_t toIntStrict(ASWorker* wrk, const asAtom& a);
	static FORCE_INLINE int64_t toInt64(const asAtom& a);
	static FORCE_INLINE uint32_t toUInt(const asAtom& a);
	static FORCE_INLINE uint32_t getUInt(ASWorker* wrk, asAtom& a);
	static int32_t localNumbertoInt(ASWorker* wrk, const asAtom& a);
	static void getStringView(tiny_string& res, const asAtom &a, ASWorker* wrk); // this doesn't deep copy the data buffer if parameter a is an ASString
	static tiny_string toString(const asAtom &a, ASWorker* wrk, bool fromAVM1add2=false);
	static tiny_string toErrorString(const asAtom& a, ASWorker* wrk); // returns string in format needed as argument in error messages
	static tiny_string AVM1toString(const asAtom &a, ASWorker* wrk, bool fortrace=false);
	static tiny_string toLocaleString(const asAtom &a, ASWorker* wrk);
	static uint32_t toStringId(asAtom &a, ASWorker* wrk);
	static FORCE_INLINE asAtom typeOf(asAtom& a);
	static number_t getLocalNumber(call_context *cc, const asAtom& a);
	static FORCE_INLINE uint16_t getLocalNumberPos(const asAtom& a)
	{
		if (isLocalNumber(a))
			return a.uintval>>8;
		return UINT16_MAX;
	}
	static bool Boolean_concrete(asAtom& a);
	static bool Boolean_concrete_object(asAtom& a);
	static void convert_b(asAtom& a, bool refcounted);
	static FORCE_INLINE int32_t getInt(const asAtom& a) { assert((a.uintval&0x7) == ATOM_INTEGER || (a.uintval&0x7) == ATOM_UINTEGER); return a.intval>>3; }
	static FORCE_INLINE uint32_t getUInt(const asAtom& a) { assert((a.uintval&0x7) == ATOM_UINTEGER || (a.uintval&0x7) == ATOM_INTEGER); return a.uintval>>3; }
	static FORCE_INLINE uint32_t getStringId(const asAtom& a) { assert((a.uintval&0x7) == ATOM_STRINGID); return a.uintval>>3; }
	static FORCE_INLINE void setInt(asAtom& a,ASWorker* wrk, int64_t val);
	static FORCE_INLINE void setUInt(asAtom& a, ASWorker* wrk, uint32_t val);
	static void setNumber(asAtom& a, ASWorker *wrk, number_t val, uint16_t localnumberpos=UINT16_MAX);
	static bool replaceNumber(asAtom& a, ASWorker* w, number_t val);
	static FORCE_INLINE void setBool(asAtom& a,bool val);
	static FORCE_INLINE void setNull(asAtom& a);
	static FORCE_INLINE void setUndefined(asAtom& a);
	static void setFunction(asAtom& a, ASObject* obj, const asAtom& closure, ASWorker* wrk);
	static FORCE_INLINE bool increment(asAtom& a, ASWorker* wrk, bool replace);
	static FORCE_INLINE bool decrement(asAtom& a, ASWorker* wrk, bool refplace);
	static FORCE_INLINE void increment_i(asAtom& a, ASWorker* wrk, int32_t amount=1);
	static FORCE_INLINE void decrement_i(asAtom& a, ASWorker* wrk, int32_t amount=1);
	static bool add(asAtom& a, asAtom& v2, ASWorker *wrk, bool forceint);
	static void addreplace(asAtom& ret, ASWorker* wrk, asAtom &v1, asAtom &v2, bool forceint, uint16_t resultlocalnumberpos);
	static bool AVM1add(asAtom& a, asAtom& v2, ASWorker *wrk, bool forceint);
	static FORCE_INLINE void bitnot(asAtom& a, ASWorker* wrk);
	static FORCE_INLINE void subtract(asAtom& a,ASWorker* wrk,asAtom& v2,bool forceint);
	static FORCE_INLINE void subtractreplace(asAtom& ret, ASWorker* wrk,const asAtom& v1, const asAtom& v2,bool forceint, uint16_t resultlocalnumberpos);
	static FORCE_INLINE void multiply(asAtom& a,ASWorker* wrk,asAtom& v2,bool forceint);
	static FORCE_INLINE void multiplyreplace(asAtom& ret, ASWorker* wrk, const asAtom& v1, const asAtom &v2, bool forceint, uint16_t resultlocalnumberpos);
	static FORCE_INLINE void divide(asAtom& a,ASWorker* wrk,asAtom& v2,bool forceint);
	static FORCE_INLINE void dividereplace(asAtom& ret, ASWorker* wrk,const asAtom& v1, const asAtom& v2,bool forceint, uint16_t resultlocalnumberpos);
	static FORCE_INLINE void modulo(asAtom& a,ASWorker* wrk,asAtom& v2,bool forceint);
	static FORCE_INLINE void moduloreplace(asAtom& ret, ASWorker* wrk, const asAtom& v1, const asAtom &v2, bool forceint, uint16_t resultlocalnumberpos);
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
	template<class T> static bool is(const asAtom& a);
	template<class T> static T* as(const asAtom& a)
	{
		assert(isObject(a));
		return static_cast<T*>((void*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)));
	}
};
#define ASATOM_INCREF(a) if (asAtomHandler::isAccessibleObject(a)) asAtomHandler::getObjectNoCheck(a)->incRef()
#define ASATOM_INCREF_POINTER(a) if (asAtomHandler::isAccessibleObject(*a)) asAtomHandler::getObjectNoCheck(*a)->incRef()
#define ASATOM_DECREF(a) if (asAtomHandler::isAccessibleObject(a)) { ASObject* obj_b = asAtomHandler::getObjectNoCheck(a); if (!obj_b->getConstant() && !obj_b->getInDestruction()) obj_b->decRef(); }
#define ASATOM_DECREF_POINTER(a) if (asAtomHandler::isAccessibleObject(*a)) { ASObject* obj_b = asAtomHandler::getObject(*a); if (obj_b && !obj_b->getConstant() && !obj_b->getInDestruction()) obj_b->decRef(); }
#define ASATOM_ADDSTOREDMEMBER(a) if (asAtomHandler::isAccessibleObject(a)) { asAtomHandler::getObjectNoCheck(a)->incRef();asAtomHandler::getObjectNoCheck(a)->addStoredMember(); }
#define ASATOM_REMOVESTOREDMEMBER(a) if (asAtomHandler::isAccessibleObject(a)) { asAtomHandler::getObjectNoCheck(a)->removeStoredMember();}
#define ASATOM_PREPARESHUTDOWN(a) if (asAtomHandler::isAccessibleObject(a)) { asAtomHandler::getObjectNoCheck(a)->prepareShutdown();}

// avoids creating number objects and refcounting, if possible
struct asAtomWithNumber
{
	asAtom value;
	number_t numbervalue;
	asAtomWithNumber():value(asAtomHandler::invalidAtom),numbervalue(0)
	{
	}
	asAtomWithNumber(asAtom v):value(v),numbervalue(0)
	{
	}
	asAtomWithNumber(number_t n):numbervalue(n)
	{
		value.uintval=UINT16_MAX<<8 | ATOMTYPE_LOCALNUMBER_BIT;
	}
	number_t toNumber(ASWorker* wrk) const;
	int32_t toInt() const
	{
		return asAtomHandler::isLocalNumber(value) ? int32_t(numbervalue) : asAtomHandler::toInt(value);
	}
	int32_t toUInt() const
	{
		return asAtomHandler::isLocalNumber(value) ? uint32_t(numbervalue) : asAtomHandler::toUInt(value);
	}
	tiny_string toString(ASWorker* wrk) const;
	std::string toDebugString() const;
};

struct variable
{
private:
	asAtomWithNumber var;
public:
	union
	{
		multiname* traitTypemname;
		Type* type;
		void* typeUnion;
	};
	asAtom setter;
	asAtom getter;
	uint32_t nameStringID;
	nsNameAndKind ns;
	uint32_t slotid;
	TRAIT_KIND kind:4;
	bool isResolved:1;
	bool isenumerable:1;
	bool issealed:1;
	bool nameIsInteger:1;
	bool varIsRefCounted:1;
	bool isStatic:1; // indicates if this variable is a member of a static ASObject

	uint8_t min_swfversion;
	variable(TRAIT_KIND _k,const nsNameAndKind& _ns,bool _nameIsInteger,uint32_t nameID, bool _isStatic)
		: var(asAtomHandler::invalidAtom)
		,typeUnion(nullptr)
		,setter(asAtomHandler::invalidAtom)
		,getter(asAtomHandler::invalidAtom)
		,nameStringID(nameID)
		,ns(_ns)
		,slotid(0)
		,kind(_k)
		,isResolved(false)
		,isenumerable(true)
		,issealed(false)
		,nameIsInteger(_nameIsInteger)
		,varIsRefCounted(false)
		,isStatic(_isStatic)
		,min_swfversion(0)
	{}
	variable(TRAIT_KIND _k, asAtom _v, multiname* _t, Type* type, const nsNameAndKind &_ns, bool _isenumerable, bool _nameIsInteger, uint32_t nameID, bool _isStatic);
	void setVar(ASWorker* wrk, asAtom v, bool _isrefcounted = true);
	/*
	 * To be used only if the value is guaranteed to be of the right type
	 */
	void setVarNoCoerce(asAtom &v, ASWorker *wrk);
	void setVarFromVariable(variable* v, uint16_t localnumberpos);

	void setResultType(Type* t)
	{
		isResolved=true;
		type=t;
	}
	asAtom getVar(ASWorker* wrk,uint16_t localnumberpos=UINT16_MAX)
	{
		if (asAtomHandler::isLocalNumber(var.value))
		{
			asAtom res = asAtomHandler::invalidAtom;
			asAtomHandler::setNumber(res,wrk,var.numbervalue,localnumberpos);
			return res;
		}
		return var.value;
	}
	asAtomWithNumber* getVarPtr()
	{
		return &var;
	}
	asAtom* getVarValuePtr()
	{
		return &var.value;
	}
	number_t* getVarNumberPtr()
	{
		return &var.numbervalue;
	}
	bool isRefcountedVar() const
	{
		return varIsRefCounted;
	}
	void resetRefcountedVar()
	{
		varIsRefCounted=false;
	}
	bool isValidVar() const
	{
		return asAtomHandler::isValid(var.value);
	}
	bool isEqualVar(const asAtom& a) const
	{
		return var.value.uintval==a.uintval && !asAtomHandler::isLocalNumber(var.value);
	}
	bool isLocalNumberVar() const
	{
		return asAtomHandler::isLocalNumber(var.value);
	}
	bool isClassVar() const
	{
		return asAtomHandler::isClass(var.value);
	}
	bool isClassBaseVar() const;
	bool isSyntheticFunctionVar() const;
	bool isBuiltinFunctionVar() const;
	bool isFunctionVar() const;
	bool isDisplayObjectVar() const;
	bool isNullVar() const
	{
		return asAtomHandler::isNull(var.value);
	}
	bool isNullOrUndefinedVar() const
	{
		return asAtomHandler::isNullOrUndefined(var.value);
	}
	SyntheticFunction* getSyntheticFunctionVar() const;
	IFunction* getFunctionVar() const;
	FORCE_INLINE ASObject* getObjectVar() const
	{
		return asAtomHandler::getObject(var.value);
	}
	FORCE_INLINE bool isAccessibleObjectVar() const
	{
		return asAtomHandler::isAccessibleObject(var.value);
	}
	void setVarNoCheck(asAtom &v, ASWorker *wrk);
	FORCE_INLINE Class_base* getClassVar(SystemState* sys) const
	{
		return asAtomHandler::getClass(var.value,sys);
	}
	FORCE_INLINE number_t getLocalNumber() const
	{
		assert(asAtomHandler::isLocalNumber(var.value));
		return var.numbervalue;
	}
	FORCE_INLINE void setLocalNumberPos(uint16_t pos)
	{
		if (asAtomHandler::isLocalNumber(var.value))
			var.value.uintval=pos<<8 | ATOMTYPE_LOCALNUMBER_BIT;
	}
};
// struct used to count cyclic references
struct cyclicmembercount
{
	uint32_t count; // number of references counted
	bool hasmember:1; // indicates if the member object has any references to the main object in its members
	bool ignore:1; // indicates if the member object doesn't have to be checked for cyclic member count and its count should be ignored
	bool ischecked:1;
	bool inchecking:1;
	std::vector<ASObject*> delayedcheck;
	FORCE_INLINE void reset()
	{
		count=0;
		hasmember=false;
		ignore=false;
		ischecked=false;
		inchecking=false;
		delayedcheck.clear();
	}
	cyclicmembercount() : count(0)
		,hasmember(false)
		,ignore(false)
		,ischecked(false)
		,inchecking(false)
	{
	}
};
// struct used to keep track of entries when executing garbage collection
struct garbagecollectorstate
{
	std::vector<ASObject*> checkedobjects;
	std::vector<ASObject*> objectstack;
	ASObject* startobj;
	bool stopped; // indicates that an object has a member and should be ignored, so we can stop gc for the startobject immediately
	void ignoreCount(ASObject* o);
	bool isIgnored(ASObject* o);
	bool hasMember(ASObject* o);
	void reset();
	void setChecked(ASObject* o);
	garbagecollectorstate(ASObject* _startobj, uint32_t capacity):startobj(_startobj),stopped(false)
	{
		checkedobjects.reserve(capacity);
		objectstack.reserve(capacity);
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
	std::vector<variable*> dynamic_vars;
	uint32_t slotcount;
	// indicates if this map was initialized with no variables with non-primitive values
	bool cloneable:1; // indicates if this map was initialized with no variables with non-primitive values
	bool isStatic:1; // indicates if this map belongs to a static ASObject
	variables_map(bool _isStatic)
		:slotcount(0)
		,cloneable(true)
		,isStatic(_isStatic)
	{
	}
	/**
	   Find a variable in the map

	   @param createKind If this is different from NO_CREATE_TRAIT and no variable is found
				a new one is created with the given kind
	   @param traitKinds Bitwise OR of accepted trait kinds
	*/
	variable* findObjVar(uint32_t nameId, const nsNameAndKind& ns, TRAIT_KIND createKind, uint32_t traitKinds, bool isEnumerable=true);
	variable* findObjVar(ASWorker* wrk, const multiname& mname, TRAIT_KIND createKind, uint32_t traitKinds);
	// adds a dynamic variable without checking if a variable with this name already exists
	void setDynamicVarNoCheck(uint32_t nameID, asAtom& v, bool nameIsInteger, ASWorker *wrk, bool prepend);
	/**
	 * Const version of findObjVar, useful when looking for getters
	 */
	FORCE_INLINE const variable* findObjVarConst(ASWorker* wrk,const multiname& mname, uint32_t traitKinds, uint32_t* nsRealId = nullptr) const
	{
		if (mname.isEmpty())
			return nullptr;
		uint32_t name=mname.name_type == multiname::NAME_STRING ? mname.name_s_id : mname.normalizedNameId(wrk);
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
	FORCE_INLINE variable* findObjVar(ASWorker* wrk,const multiname& mname, uint32_t traitKinds, uint32_t* nsRealId = nullptr)
	{
		if (mname.isEmpty())
			return nullptr;
		uint32_t name=mname.name_type == multiname::NAME_STRING ? mname.name_s_id : mname.normalizedNameId(wrk);
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

	FORCE_INLINE variable* findVarOrSetter(ASWorker* wrk,const multiname& mname, uint32_t traitKinds)
	{
		uint32_t name=mname.name_type == multiname::NAME_STRING ? mname.name_s_id : mname.normalizedNameId(wrk);

		var_iterator ret=Variables.find(name);
		bool noNS = mname.ns.empty(); // no Namespace in multiname means we check for the empty Namespace
		auto nsIt=mname.ns.begin();

		variable* res = nullptr;
		//Find the namespace
		while(ret!=Variables.end() && ret->first==name)
		{
			//breaks when the namespace is not found
			const nsNameAndKind& ns=ret->second.ns;
			if((noNS && ns.hasEmptyName()) || (!noNS && ns==*nsIt))
			{
				if(ret->second.kind & traitKinds)
				{
					res = &ret->second;
					if ((ret->second.isValidVar()) || asAtomHandler::isValid(ret->second.setter))
						return &ret->second;
					else
					{
						if (noNS)
							++ret;
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
				}
				else
					return res;
			}
			else if (noNS)
			{
				++ret;
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
		return res;
	}

	//Initialize a new variable specifying the type (TODO: add support for const)
	void initializeVar(multiname &mname, asAtom &obj, multiname *typemname, ABCContext* context, TRAIT_KIND traitKind, ASObject* mainObj, uint32_t slot_id, bool isenumerable);
	void killObjVar(ASWorker* wrk, const multiname& mname);
	FORCE_INLINE asAtom getSlot(unsigned int n, ASWorker* wrk, uint16_t localnumberpos)
	{
		assert_and_throw(n > 0 && n <= slotcount);
		return slots_vars[n-1]->getVar(wrk,localnumberpos);
	}
	FORCE_INLINE variable* getSlotVar(unsigned int n)
	{
		return slots_vars[n-1];
	}
	FORCE_INLINE asAtom getSlotNoCheck(unsigned int n, ASWorker* wrk, uint16_t localnumberpos)
	{
		return slots_vars[n]->getVar(wrk,localnumberpos);
	}
	FORCE_INLINE TRAIT_KIND getSlotKind(unsigned int n)
	{
		assert_and_throw(n > 0 && n <= slotcount);
		return slots_vars[n-1]->kind;
	}
	Class_base* getSlotType(unsigned int n, lightspark::ABCContext* context);

	uint32_t findInstanceSlotByMultiname(multiname* name, ASWorker* wrk);
	FORCE_INLINE TRISTATE setSlot(ASWorker* wrk, unsigned int n, asAtom &o);
	/*
	 * This version of the call is guarantee to require no type conversion
	 * this is verified at optimization time
	 */
	FORCE_INLINE void setSlotNoCoerce(unsigned int n, asAtom o, ASWorker *wrk);

	// copies var and numbervalue from variable without converting numbervalue
	FORCE_INLINE void setSlotFromVariable(unsigned int n, variable* v, uint16_t localnumberpos);

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
	uint32_t getNameAt(unsigned int i, bool& nameIsInteger);
	const variable* getValueAt(unsigned int i);
	~variables_map();
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, bool forsharedobject, ASWorker* wrk);
	void dumpVariables();
	void destroyContents();
	void prepareShutdown();
	bool cloneInstance(variables_map& map);
	void removeAllDeclaredProperties();
	bool countCylicMemberReferences(garbagecollectorstate& gcstate, ASObject* parent);
	void insertVar(variable* v, bool prepend=false);
	void removeVar(variable* v);
};

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
		const variable* ret=Variables.findObjVarConst(getInstanceWorker(),name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId);
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(asAtomHandler::isValid(ret->getter) || ret->isValidVar()))
				ret=nullptr;
		}
		return ret;
	}

	variable* findSettable(const multiname& name, bool* has_getter=nullptr) DLL_LOCAL;
	multiname* proxyMultiName;
	SystemState* sys;
	ASWorker* worker;
	// double-linked list of objects marked for garbage collection
	ASObject* gcNext;
	ASObject* gcPrev;
	std::map<uint32_t,pair<IFunction*,asAtom>> avm1watcherlist;
protected:
	ASObject(MemoryAccount* m);

	ASObject(const ASObject& o);
	virtual ~ASObject();
	uint32_t stringId;
	uint32_t storedmembercount; // count how often this object is stored as a member of another object (needed for cyclic reference detection)
	uint32_t storedmembercountstatic; // count how often this object is stored as a member of another static object (needed for cyclic reference detection)
	SWFOBJECT_TYPE type;
	CLASS_SUBTYPE subtype;

	bool traitsInitialized:1;
	bool constructIndicator:1;
	bool constructorCallComplete:1; // indicates that the constructor including all super constructors has been called
	bool preparedforshutdown:1;
	bool markedforgarbagecollection:1;
	bool deletedingarbagecollection:1;
	static variable* findSettableImpl(ASWorker* wrk, variables_map& map, const multiname& name, bool* has_getter);
	static FORCE_INLINE const variable* findGettableImplConst(ASWorker* wrk, const variables_map& map, const multiname& name, uint32_t* nsRealId = nullptr)
	{
		const variable* ret=map.findObjVarConst(wrk,name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId);
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(asAtomHandler::isValid(ret->getter) || ret->isValidVar()))
				ret=nullptr;
		}
		return ret;
	}
	static FORCE_INLINE variable* findGettableImpl(ASWorker* wrk, variables_map& map, const multiname& name, uint32_t* nsRealId = nullptr)
	{
		variable* ret=map.findObjVar(wrk,name,DECLARED_TRAIT|DYNAMIC_TRAIT,nsRealId);
		if(ret)
		{
			//It seems valid for a class to redefine only the setter, so if we can't find
			//something to get, it's ok
			if(!(asAtomHandler::isValid(ret->getter) || ret->isValidVar()))
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
		destroyContents();
		for (auto it = ownedObjects.begin(); it != ownedObjects.end(); it++)
		{
			(*it)->removeStoredMember();
		}
		ownedObjects.clear();
		if (!avm1watcherlist.empty())
			AVM1clearWatcherList();
		if (proxyMultiName)
		{
			delete proxyMultiName;
			proxyMultiName = nullptr;
		}
		stringId = UINT32_MAX;
		traitsInitialized =false;
		constructIndicator = false;
		constructorCallComplete = false;
		bool keep = canHaveCyclicMemberReference() && removefromGarbageCollection();
		implEnable = true;
		storedmembercount=0;
		storedmembercountstatic=0;
		gccounter.reset();
#ifndef NDEBUG
		//Stuff only used in debugging
		initialized=false;
#endif
		bool dodestruct = true;
		if (!keep && objfreelist)
		{
			if (!getCached())
			{
//				assert(getWorker() == this->getInstanceWorker() || !getWorker());
				dodestruct = !objfreelist->pushObjectToFreeList(this);
			}
			else
				dodestruct = false;
		}
		if (dodestruct)
		{
			if (keep)
			{
				setConstant();
				deletedingarbagecollection=true;
				addToGarbageCollection();
				return false;
			}
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
	void AVM1clearWatcherList();
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
	void addStoredMember();
	void addStoredMemberStatic() { ++storedmembercountstatic; addStoredMember(); }
	bool isMarkedForGarbageCollection() const { return markedforgarbagecollection; }
	bool removefromGarbageCollection();
	bool addToGarbageCollection();
	void removeStoredMember();
	void removeStoredMemberStatic() { --storedmembercountstatic; removeStoredMember(); }
	bool hasStoredMemberStatic() const { return storedmembercountstatic; }
	uint32_t getStoredMemberStatic() const { return storedmembercountstatic; }
	FORCE_INLINE void decRefAndGCCheck()
	{
		if (storedmembercount
			&& getRefCount()==int(storedmembercount)+1
			&& addToGarbageCollection())
			return;
		else
			decRef();
	}
	bool handleGarbageCollection();
	virtual bool countCylicMemberReferences(garbagecollectorstate& gcstate);
	FORCE_INLINE bool canHaveCyclicMemberReference()
	{
		return type == T_ARRAY || type == T_CLASS || type == T_PROXY || type == T_TEMPLATE || type == T_FUNCTION ||
				(type == T_OBJECT &&
				subtype != SUBTYPE_DATE
				); // TODO check other subtypes
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
	ASFUNCTION_ATOM(AVM1_watch);
	ASFUNCTION_ATOM(AVM1_unwatch);

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
	virtual asAtomWithNumber getAtomWithNumberByMultiname(const multiname& name, ASWorker* wrk, GET_VARIABLE_OPTION opt);

	std::pair<asAtom, GET_VARIABLE_RESULT> AVM1searchPrototypeByMultiname
	(
		const multiname& name,
		GET_VARIABLE_OPTION opt,
		ASWorker* wrk,
		bool isSlashPath
	);
	// AVM1 needs to check the "protoype" variable in addition to the normal behaviour
	virtual GET_VARIABLE_RESULT AVM1getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk, bool isSlashPath = true);
	virtual bool AVM1setLocalByMultiname(multiname& name, asAtom& value, CONST_ALLOWED_FLAG allowConst, ASWorker* wrk);
	virtual bool AVM1setVariableByMultiname(multiname& name, asAtom& value, CONST_ALLOWED_FLAG allowConst, ASWorker* wrk);
	bool AVM1checkWatcher(multiname& name, asAtom& value, ASWorker* wrk);

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
		m.isInteger=index>=0;
		setVariableByMultiname(m,o,allowConst,alreadyset,wrk);
	}

	// sets dynamic variable without checking for existence
	// use it if it is guarranteed that the variable doesn't exist in this object
	FORCE_INLINE void setDynamicVariableNoCheck(uint32_t nameID, asAtom& o, bool nameIsInteger,ASWorker* wrk,bool prepend=false)
	{
		Variables.setDynamicVarNoCheck(nameID,o,nameIsInteger,wrk,prepend);
	}
	/*
	 * Called by ABCVm::buildTraits to create DECLARED_TRAIT or CONSTANT_TRAIT and set their type
	 * and by new_catchscopeObject to initialize first slot for exception object
	 */
	void initializeVariableByMultiname(multiname &name, asAtom& o, multiname* typemname,
			ABCContext* context, TRAIT_KIND traitKind, uint32_t slot_id, bool isenumerable);
	virtual bool deleteVariableByMultiname(const multiname& name, ASWorker* wrk)
	{
		return deleteVariableByMultiname_intern(name, wrk);
	}
	bool deleteVariableByMultiname_intern(const multiname& name, ASWorker* wrk);
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true, uint8_t min_swfversion=0);
	void setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true, uint8_t min_swfversion=0);
	variable *setVariableByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind, bool isEnumerable = true, uint8_t min_swfversion=0);
	variable *setVariableAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable = true, uint8_t min_swfversion=0);
	variable *setVariableAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind, bool isEnumerable = true, bool isRefcounted = true, uint8_t min_swfversion=0);
	//NOTE: the isBorrowed flag is used to distinguish methods/setters/getters that are inside a class but on behalf of the instances
	void setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true,uint8_t min_swfversion=0);
	void setDeclaredMethodByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true, uint8_t min_swfversion=0);
	void setDeclaredMethodByQName(uint32_t nameId, const nsNameAndKind& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true, uint8_t min_swfversion=0);
	void setDeclaredMethodAtomByQName(const tiny_string& name, const tiny_string& ns, asAtom o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	void setDeclaredMethodAtomByQName(uint32_t nameId, const nsNameAndKind& ns, asAtom f, METHOD_TYPE type, bool isBorrowed, bool isEnumerable = true);
	virtual bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk);
	FORCE_INLINE asAtom getSlot(unsigned int n,uint16_t localnumberpos=UINT16_MAX)
	{
		return Variables.getSlot(n,getInstanceWorker(),localnumberpos);
	}
	FORCE_INLINE variable* getSlotVar(unsigned int n)
	{
		return Variables.getSlotVar(n);
	}
	FORCE_INLINE asAtom getSlotNoCheck(unsigned int n,uint16_t localnumberpos=UINT16_MAX)
	{
		return Variables.getSlotNoCheck(n,getInstanceWorker(),localnumberpos);
	}
	FORCE_INLINE TRAIT_KIND getSlotKind(unsigned int n)
	{
		return Variables.getSlotKind(n);
	}
	// return value:
	// TTRUE: slot is updated, no new object created
	// TFALSE: slot is updated, new object was created through coercion
	// TUNDEFINED: slot was already set to new value, no changes necessary
	FORCE_INLINE TRISTATE setSlot(ASWorker* wrk,unsigned int n,asAtom o)
	{
		return Variables.setSlot(wrk,n,o);
	}
	FORCE_INLINE void setSlotNoCoerce(unsigned int n,asAtom o,ASWorker* wrk)
	{
		Variables.setSlotNoCoerce(n,o,wrk);
	}
	FORCE_INLINE void setSlotFromVariable(unsigned int n,variable* v, uint16_t localnumberpos)
	{
		Variables.setSlotFromVariable(n,v,localnumberpos);
	}
	FORCE_INLINE Class_base* getSlotType(unsigned int n,ABCContext* context)
	{
		return Variables.getSlotType(n,context);
	}
	uint32_t findInstanceSlotByMultiname(multiname* name)
	{
		return Variables.findInstanceSlotByMultiname(name,getInstanceWorker());
	}
	unsigned int numSlots() const
	{
		return Variables.slots_vars.size();
	}

	void initSlot(unsigned int n, variable *v);

	void initAdditionalSlots(std::vector<multiname *> &additionalslots);
	virtual void AVM1enumerate(std::stack<asAtom>& stack);
	unsigned int numVariables() const;
	uint32_t getNameAt(int i, bool& nameIsInteger);
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
	bool AVM1toPrimitive(asAtom& ret, bool& isrefcounted, bool& fromValueOf, bool fromAVM1Add2);

	bool isInitialized() const {return traitsInitialized;}
	virtual bool isConstructed() const;

	asAtom callResolveMethod(const tiny_string& name, ASWorker* wrk);
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
	virtual ASObject* getprop_prototype();
	virtual asAtom getprop_prototypeAtom();
	void setprop_prototype(asAtom& prototype, uint32_t nameID=BUILTIN_STRINGS::PROTOTYPE);

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
	inline virtual void constructionComplete(bool _explicit = false, bool forInitAction = false)
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
	inline bool getConstructionComplete() const { return constructorCallComplete; }


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
	void AVM1HandleSetFocusEvent(ASObject *dispobj, ASObject* oldfocus);
	virtual void AVM1HandlePressedEvent(ASObject *dispobj);
	// updates AVM1 bindings in target for all members of this ASObject
	void AVM1UpdateAllBindings(DisplayObject* target, ASWorker* wrk);
	virtual ASObject* AVM1getClassPrototypeObject() const;

	// copies all dynamic values to the target
	void copyValues(ASObject* target, ASWorker* wrk);
	void copyValuesForPrototype(ASObject* target, ASWorker* wrk);

	virtual asAtom getThisAtom() { return asAtomHandler::fromObject(this); }
	const variables_map* getVariables() const { return &Variables; }

	cyclicmembercount gccounter;
};


FORCE_INLINE TRISTATE variables_map::setSlot(ASWorker* wrk,unsigned int n, asAtom &o)
{
	assert_and_throw(n < slotcount);
	if (!slots_vars[n]->isEqualVar(o))
	{
		slots_vars[n]->setVar(wrk,o);
		return slots_vars[n]->isEqualVar(o) ? TTRUE : TFALSE; // setVar may coerce the object into a new instance, so we need to check if incRef is necessary
	}
	return TUNDEFINED;
}

FORCE_INLINE void variables_map::setSlotNoCoerce(unsigned int n, asAtom o, ASWorker* wrk)
{
	assert_and_throw(n < slotcount);
	if (!slots_vars[n]->isEqualVar(o))
		slots_vars[n]->setVarNoCoerce(o,wrk);
}
FORCE_INLINE void variables_map::setSlotFromVariable(unsigned int n, variable* v, uint16_t localnumberpos)
{
	assert_and_throw(n < slotcount);
	slots_vars[n]->setVarFromVariable(v, localnumberpos);
}
FORCE_INLINE void variables_map::setDynamicVarNoCheck(uint32_t nameID, asAtom& v, bool nameIsInteger, ASWorker* wrk, bool prepend)
{
	var_iterator inserted=Variables.insert(Variables.cbegin(),
			make_pair(nameID,variable(DYNAMIC_TRAIT,nsNameAndKind(),nameIsInteger,nameID,this->isStatic)));
	insertVar(&inserted->second,prepend);

	ASObject* o = asAtomHandler::getObject(v);
	if (o)
	{
		o->addStoredMember();
		if (this->isStatic)
			o->storedmembercountstatic++;
	}
	inserted->second.setVarNoCheck(v,wrk);
}
FORCE_INLINE void variable::setVarNoCoerce(asAtom &v, ASWorker* wrk)
{
	asAtom oldvar = var.value;
	bool oldisrefcounted = varIsRefCounted;
	setVarNoCheck(v,wrk);
	if(oldisrefcounted && asAtomHandler::isObject(oldvar))
	{
		LOG_CALL("remove old var no coerce:"<<asAtomHandler::toDebugString(oldvar));
		if (this->isStatic)
			asAtomHandler::getObjectNoCheck(oldvar)->storedmembercountstatic--;
		asAtomHandler::getObjectNoCheck(oldvar)->removeStoredMember();
	}
	varIsRefCounted = asAtomHandler::isObject(v);
	if(varIsRefCounted)
	{
		asAtomHandler::getObjectNoCheck(v)->incRef();
		asAtomHandler::getObjectNoCheck(v)->addStoredMember();
		if (this->isStatic)
			asAtomHandler::getObjectNoCheck(v)->storedmembercountstatic++;
	}
}

FORCE_INLINE void variable::setVarFromVariable(variable* v, uint16_t localnumberpos)
{
	asAtom oldvar = var.value;
	if (v->isLocalNumberVar())
	{
		var.value.uintval=localnumberpos<<8 | ATOMTYPE_LOCALNUMBER_BIT;
		var.numbervalue=v->var.numbervalue;
	}
	else
		var.value=v->var.value;
	if(varIsRefCounted && asAtomHandler::isObject(oldvar))
	{
		LOG_CALL("remove old var no coerce:"<<asAtomHandler::toDebugString(oldvar));
		if (this->isStatic)
			asAtomHandler::getObjectNoCheck(oldvar)->storedmembercountstatic--;
		asAtomHandler::getObjectNoCheck(oldvar)->removeStoredMember();
	}
	varIsRefCounted = asAtomHandler::isObject(var.value);
	if(varIsRefCounted)
	{
		asAtomHandler::getObjectNoCheck(var.value)->incRef();
		asAtomHandler::getObjectNoCheck(var.value)->addStoredMember();
		if (this->isStatic)
			asAtomHandler::getObjectNoCheck(var.value)->storedmembercountstatic++;
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
class AVM1LoadVars;
class AVM1LocalConnection;
class AVM1MovieClip;
class AVM1MovieClipLoader;
class AVM1Point;
class AVM1Rectangle;
class AVM1Sound;
class AVM1Super_object;
class AVM1XMLDocument;
class AVM1XMLNode;
class BevelFilter;
class Bitmap;
class BitmapData;
class BitmapFilter;
class Boolean;
class BlurFilter;
class ByteArray;
class Catchscope_object;
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
class FileReferenceList;
class FileStream;
class FocusEvent;
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
class HttpStatusEvent;
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
class StackOverflowError;
class Stage;
class Stage3D;
class StatusEvent;
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
class Timer;
class TimerEvent;
class ThrottleEvent;
class Type;
class TypeError;
class UInteger;
class Undefined;
class UninitializedError;
class URIError;
class URLLoader;
class URLRequest;
class URLVariables;
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
template<> inline bool ASObject::is<ASError>() const { return subtype==SUBTYPE_ERROR || subtype==SUBTYPE_SECURITYERROR || subtype==SUBTYPE_ARGUMENTERROR || subtype==SUBTYPE_DEFINITIONERROR || subtype==SUBTYPE_EVALERROR || subtype==SUBTYPE_RANGEERROR || subtype==SUBTYPE_REFERENCEERROR || subtype==SUBTYPE_SYNTAXERROR || subtype==SUBTYPE_TYPEERROR || subtype==SUBTYPE_URIERROR || subtype==SUBTYPE_VERIFYERROR || subtype==SUBTYPE_UNINITIALIZEDERROR || subtype==SUBTYPE_STACKOVERFLOWERROR; }
template<> inline bool ASObject::is<ASFile>() const { return subtype==SUBTYPE_FILE; }
template<> inline bool ASObject::is<ASMutex>() const { return subtype==SUBTYPE_MUTEX; }
template<> inline bool ASObject::is<ASObject>() const { return true; }
template<> inline bool ASObject::is<ASQName>() const { return type==T_QNAME; }
template<> inline bool ASObject::is<ASString>() const { return type==T_STRING; }
template<> inline bool ASObject::is<ASWorker>() const { return subtype==SUBTYPE_WORKER; }
template<> inline bool ASObject::is<AVM1Function>() const { return subtype==SUBTYPE_AVM1FUNCTION; }
template<> inline bool ASObject::is<AVM1LoadVars>() const { return subtype == SUBTYPE_AVM1LOADVARS; }
template<> inline bool ASObject::is<AVM1LocalConnection>() const { return subtype==SUBTYPE_AVM1LOCALCONNECTION; }
template<> inline bool ASObject::is<AVM1Movie>() const { return subtype == SUBTYPE_AVM1MOVIE; }
template<> inline bool ASObject::is<AVM1MovieClip>() const { return subtype == SUBTYPE_AVM1MOVIECLIP; }
template<> inline bool ASObject::is<AVM1MovieClipLoader>() const { return subtype == SUBTYPE_AVM1MOVIECLIPLOADER; }
template<> inline bool ASObject::is<AVM1Point>() const { return subtype == SUBTYPE_AVM1POINT; }
template<> inline bool ASObject::is<AVM1Rectangle>() const { return subtype == SUBTYPE_AVM1RECTANGLE; }
template<> inline bool ASObject::is<AVM1Sound>() const { return subtype == SUBTYPE_AVM1SOUND; }
template<> inline bool ASObject::is<AVM1Super_object>() const { return subtype == SUBTYPE_AVM1SUPEROBJECT; }
template<> inline bool ASObject::is<AVM1XMLDocument>() const { return subtype == SUBTYPE_AVM1XMLDOCUMENT; }
template<> inline bool ASObject::is<AVM1XMLNode>() const { return subtype == SUBTYPE_AVM1XMLNODE; }
template<> inline bool ASObject::is<BevelFilter>() const { return subtype==SUBTYPE_BEVELFILTER; }
template<> inline bool ASObject::is<Bitmap>() const { return subtype==SUBTYPE_BITMAP; }
template<> inline bool ASObject::is<BitmapData>() const { return subtype==SUBTYPE_BITMAPDATA; }
template<> inline bool ASObject::is<BitmapFilter>() const { return subtype==SUBTYPE_BITMAPFILTER || subtype==SUBTYPE_GLOWFILTER || subtype==SUBTYPE_DROPSHADOWFILTER || subtype==SUBTYPE_GRADIENTGLOWFILTER || subtype==SUBTYPE_BEVELFILTER || subtype==SUBTYPE_COLORMATRIXFILTER || subtype==SUBTYPE_BLURFILTER || subtype==SUBTYPE_CONVOLUTIONFILTER || subtype==SUBTYPE_DISPLACEMENTFILTER || subtype==SUBTYPE_GRADIENTBEVELFILTER || subtype==SUBTYPE_SHADERFILTER; }
template<> inline bool ASObject::is<Boolean>() const { return type==T_BOOLEAN; }
template<> inline bool ASObject::is<BlurFilter>() const { return subtype==SUBTYPE_BLURFILTER; }
template<> inline bool ASObject::is<ByteArray>() const { return subtype==SUBTYPE_BYTEARRAY; }
template<> inline bool ASObject::is<Catchscope_object>() const { return subtype==SUBTYPE_CATCHSCOPEOBJECT; }
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
template<> inline bool ASObject::is<DisplayObject>() const
{
	return subtype == SUBTYPE_DISPLAYOBJECT
		   || subtype == SUBTYPE_INTERACTIVE_OBJECT
		   || subtype == SUBTYPE_TEXTFIELD
		   || subtype == SUBTYPE_BITMAP
		   || subtype == SUBTYPE_DISPLAYOBJECTCONTAINER
		   || subtype == SUBTYPE_STAGE
		   || subtype == SUBTYPE_ROOTMOVIECLIP
		   || subtype == SUBTYPE_SPRITE
		   || subtype == SUBTYPE_MOVIECLIP
		   || subtype == SUBTYPE_TEXTLINE
		   || subtype == SUBTYPE_VIDEO
		   || subtype == SUBTYPE_SIMPLEBUTTON
		   || subtype == SUBTYPE_SHAPE
		   || subtype == SUBTYPE_MORPHSHAPE
		   || subtype == SUBTYPE_LOADER
		   || subtype == SUBTYPE_AVM1MOVIECLIP
		   || subtype == SUBTYPE_AVM1MOVIE;
}
template<> inline bool ASObject::is<DisplayObjectContainer>() const { return subtype==SUBTYPE_DISPLAYOBJECTCONTAINER || subtype==SUBTYPE_STAGE || subtype==SUBTYPE_ROOTMOVIECLIP || subtype==SUBTYPE_SPRITE || subtype == SUBTYPE_MOVIECLIP || subtype == SUBTYPE_TEXTLINE || subtype == SUBTYPE_SIMPLEBUTTON || subtype==SUBTYPE_LOADER || subtype == SUBTYPE_AVM1MOVIECLIP || subtype == SUBTYPE_AVM1MOVIE; }
template<> inline bool ASObject::is<DropShadowFilter>() const { return subtype==SUBTYPE_DROPSHADOWFILTER; }
template<> inline bool ASObject::is<EastAsianJustifier>() const { return subtype==SUBTYPE_EASTASIANJUSTIFIER; }
template<> inline bool ASObject::is<ElementFormat>() const { return subtype==SUBTYPE_ELEMENTFORMAT; }
template<> inline bool ASObject::is<EvalError>() const { return subtype==SUBTYPE_EVALERROR; }
template<> inline bool ASObject::is<Event>() const
{
	return subtype == SUBTYPE_EVENT
			|| subtype == SUBTYPE_WAITABLE_EVENT
			|| subtype == SUBTYPE_PROGRESSEVENT
			|| subtype == SUBTYPE_KEYBOARD_EVENT
			|| subtype == SUBTYPE_MOUSE_EVENT
			|| subtype == SUBTYPE_SAMPLEDATA_EVENT
			|| subtype == SUBTYPE_THROTTLE_EVENT
			|| subtype == SUBTYPE_CONTEXTMENUEVENT
			|| subtype == SUBTYPE_GAMEINPUTEVENT
			|| subtype == SUBTYPE_NATIVEWINDOWBOUNDSEVENT
			|| subtype == SUBTYPE_FOCUSEVENT
			|| subtype == SUBTYPE_HTTPSTATUSEVENT
			|| subtype == SUBTYPE_STATUSEVENT
			|| subtype == SUBTYPE_TIMEREVENT;
}
template<> inline bool ASObject::is<ExtensionContext>() const { return subtype==SUBTYPE_EXTENSIONCONTEXT; }
template<> inline bool ASObject::is<FontDescription>() const { return subtype==SUBTYPE_FONTDESCRIPTION; }
template<> inline bool ASObject::is<FileMode>() const { return subtype==SUBTYPE_FILEMODE; }
template<> inline bool ASObject::is<FileReference>() const { return subtype==SUBTYPE_FILE||subtype==SUBTYPE_FILEREFERENCE; }
template<> inline bool ASObject::is<FileReferenceList>() const { return subtype==SUBTYPE_FILEREFERENCELIST; }
template<> inline bool ASObject::is<FileStream>() const { return subtype==SUBTYPE_FILESTREAM; }
template<> inline bool ASObject::is<FocusEvent>() const { return subtype==SUBTYPE_FOCUSEVENT; }
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
template<> inline bool ASObject::is<HttpStatusEvent>() const { return subtype==SUBTYPE_HTTPSTATUSEVENT; }
template<> inline bool ASObject::is<IFunction>() const { return type==T_FUNCTION; }
template<> inline bool ASObject::is<IndexBuffer3D>() const { return subtype==SUBTYPE_INDEXBUFFER3D; }
template<> inline bool ASObject::is<Integer>() const { return type==T_INTEGER; }
template<> inline bool ASObject::is<InteractiveObject>() const
{
	return subtype==SUBTYPE_INTERACTIVE_OBJECT
		   || subtype == SUBTYPE_TEXTFIELD
		   || subtype == SUBTYPE_TEXTLINE
		   || subtype == SUBTYPE_DISPLAYOBJECTCONTAINER
		   || subtype == SUBTYPE_STAGE
		   || subtype == SUBTYPE_ROOTMOVIECLIP
		   || subtype == SUBTYPE_SPRITE
		   || subtype == SUBTYPE_MOVIECLIP
		   || subtype == SUBTYPE_SIMPLEBUTTON
		   || subtype == SUBTYPE_LOADER
		   || subtype == SUBTYPE_AVM1MOVIECLIP
		   || subtype == SUBTYPE_AVM1MOVIE;
}
template<> inline bool ASObject::is<KeyboardEvent>() const { return subtype==SUBTYPE_KEYBOARD_EVENT; }
template<> inline bool ASObject::is<LocalConnection>() const { return subtype==SUBTYPE_LOCALCONNECTION || subtype==SUBTYPE_AVM1LOCALCONNECTION; }
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
template<> inline bool ASObject::is<Point>() const { return subtype==SUBTYPE_POINT || subtype==SUBTYPE_AVM1POINT; }
template<> inline bool ASObject::is<Program3D>() const { return subtype==SUBTYPE_PROGRAM3D; }
template<> inline bool ASObject::is<ProgressEvent>() const { return subtype==SUBTYPE_PROGRESSEVENT; }
template<> inline bool ASObject::is<Proxy>() const { return subtype==SUBTYPE_PROXY; }
template<> inline bool ASObject::is<RangeError>() const { return subtype==SUBTYPE_RANGEERROR; }
template<> inline bool ASObject::is<Rectangle>() const { return subtype==SUBTYPE_RECTANGLE || subtype==SUBTYPE_AVM1RECTANGLE; }
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
template<> inline bool ASObject::is<StackOverflowError>() const { return subtype==SUBTYPE_STACKOVERFLOWERROR; }
template<> inline bool ASObject::is<Stage>() const { return subtype==SUBTYPE_STAGE; }
template<> inline bool ASObject::is<Stage3D>() const { return subtype==SUBTYPE_STAGE3D; }
template<> inline bool ASObject::is<StatusEvent>() const { return subtype==SUBTYPE_STATUSEVENT; }
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
template<> inline bool ASObject::is<Timer>() const { return subtype==SUBTYPE_TIMER; }
template<> inline bool ASObject::is<TimerEvent>() const { return subtype==SUBTYPE_TIMEREVENT; }
template<> inline bool ASObject::is<ThrottleEvent>() const { return subtype==SUBTYPE_THROTTLE_EVENT; }
template<> inline bool ASObject::is<Type>() const { return type==T_CLASS; }
template<> inline bool ASObject::is<TypeError>() const { return subtype==SUBTYPE_TYPEERROR; }
template<> inline bool ASObject::is<UInteger>() const { return type==T_UINTEGER; }
template<> inline bool ASObject::is<Undefined>() const { return type==T_UNDEFINED; }
template<> inline bool ASObject::is<UninitializedError>() const { return subtype == SUBTYPE_UNINITIALIZEDERROR; }
template<> inline bool ASObject::is<URIError>() const { return subtype == SUBTYPE_URIERROR; }
template<> inline bool ASObject::is<URLLoader>() const { return subtype == SUBTYPE_URLLOADER; }
template<> inline bool ASObject::is<URLRequest>() const { return subtype == SUBTYPE_URLREQUEST; }
template<> inline bool ASObject::is<URLVariables>() const { return subtype == SUBTYPE_URLVARIABLES || subtype == SUBTYPE_AVM1LOADVARS; }
template<> inline bool ASObject::is<Vector>() const { return subtype==SUBTYPE_VECTOR; }
template<> inline bool ASObject::is<Vector3D>() const { return subtype==SUBTYPE_VECTOR3D; }
template<> inline bool ASObject::is<VerifyError>() const { return subtype==SUBTYPE_VERIFYERROR; }
template<> inline bool ASObject::is<VertexBuffer3D>() const { return subtype==SUBTYPE_VERTEXBUFFER3D; }
template<> inline bool ASObject::is<Video>() const { return subtype==SUBTYPE_VIDEO; }
template<> inline bool ASObject::is<VideoTexture>() const { return subtype==SUBTYPE_VIDEOTEXTURE; }
template<> inline bool ASObject::is<WaitableEvent>() const { return subtype==SUBTYPE_WAITABLE_EVENT; }
template<> inline bool ASObject::is<WorkerDomain>() const { return subtype==SUBTYPE_WORKERDOMAIN; }
template<> inline bool ASObject::is<XML>() const { return subtype==SUBTYPE_XML; }
template<> inline bool ASObject::is<XMLDocument>() const { return subtype==SUBTYPE_XMLDOCUMENT || subtype == SUBTYPE_AVM1XMLDOCUMENT; }
template<> inline bool ASObject::is<XMLNode>() const { return subtype==SUBTYPE_XMLNODE || subtype==SUBTYPE_XMLDOCUMENT || subtype == SUBTYPE_AVM1XMLDOCUMENT || subtype == SUBTYPE_AVM1XMLNODE; }
template<> inline bool ASObject::is<XMLList>() const { return subtype==SUBTYPE_XMLLIST; }



template<class T> inline bool asAtomHandler::is(const asAtom& a) {
	return isObject(a) ? getObjectNoCheck(a)->is<T>() : false;
}
template<> inline bool asAtomHandler::is<asAtom>(const asAtom& a) { return true; }
template<> inline bool asAtomHandler::is<ASObject>(const asAtom& a) { return true; }
template<> inline bool asAtomHandler::is<ASString>(const asAtom& a) { return isStringID(a) || isString(a); }
template<> inline bool asAtomHandler::is<Boolean>(const asAtom& a) { return isBool(a); }
template<> inline bool asAtomHandler::is<Integer>(const asAtom& a) { return isInteger(a); }
template<> inline bool asAtomHandler::is<Null>(const asAtom& a) { return isNull(a); }
template<> inline bool asAtomHandler::is<Number>(const asAtom& a) { return isNumber(a); }
template<> inline bool asAtomHandler::is<UInteger>(const asAtom& a) { return isUInteger(a); }
template<> inline bool asAtomHandler::is<Undefined>(const asAtom& a) { return isUndefined(a); }

FORCE_INLINE int32_t asAtomHandler::toInt(const asAtom& a)
{
	if ((a.uintval&0x7)==ATOM_INTEGER)
 		return a.intval>>3;
	else if ((a.uintval&0x7)==ATOM_UINTEGER)
		return a.uintval>>3;
	else if ((a.uintval&0x7)==ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER)
	{
		if (a.uintval & ATOMTYPE_LOCALNUMBER_BIT)
			return localNumbertoInt(getWorker(),a);
		return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : 0;
	}
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
FORCE_INLINE int32_t asAtomHandler::toIntStrict(ASWorker *wrk, const asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
			if (a.uintval & ATOMTYPE_LOCALNUMBER_BIT)
				return localNumbertoInt(wrk,a);
			return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(wrk,a.uintval>>3);
			int32_t ret = s->toIntStrict();
			s->decRef();
			return ret;
		}
		default:
			assert(getObject(a));
			return getObjectNoCheck(a)->toIntStrict();
	}
}
FORCE_INLINE number_t asAtomHandler::getNumber(ASWorker* wrk, const asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
			if (a.uintval & ATOMTYPE_LOCALNUMBER_BIT)
				return getLocalNumber(getCallContext(wrk),a);
			return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : (a.uintval&ATOMTYPE_UNDEFINED_BIT) ? numeric_limits<double>::quiet_NaN() : 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(wrk,a.uintval>>3);
			number_t ret = s->toNumber();
			s->decRef();
			return ret;
		}
		default:
			assert(getObject(a));
			return getObjectNoCheck(a)->toNumber();
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
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
			if (a.uintval & ATOMTYPE_LOCALNUMBER_BIT)
				return getLocalNumber(getCallContext(getWorker()),a);
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
inline number_t asAtomHandler::AVM1toNumber(const asAtom& a, uint32_t swfversion, bool primitiveHint)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
		{
			switch(a.uintval&ATOMTYPE_BIT_FLAGS)
			{
				case ATOMTYPE_BOOL_BIT:
					return (a.uintval&0x80)>>7;
				// NOTE: `undefined` in SWF 6, and lower gets converted
				// to 0.
				case ATOMTYPE_UNDEFINED_BIT:
				case ATOMTYPE_NULL_BIT:
					return swfversion > 6 && swfversion != UINT8_MAX ? numeric_limits<double>::quiet_NaN() : 0;
				case ATOMTYPE_LOCALNUMBER_BIT:
					return getLocalNumber(getCallContext(getWorker()),a);
				default:
					assert(false);
					return 0;
			}
		}
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(getWorker(),a.uintval>>3);
			number_t ret = s->toNumber();
			s->decRef();
			return ret;
		}
		case ATOM_STRINGPTR:
		case ATOM_NUMBERPTR:
			assert(getObject(a));
			return getObjectNoCheck(a)->toNumber();
		default:
		{
			assert(getObject(a));
			if (primitiveHint)
				return std::numeric_limits<double>::quiet_NaN();
			asAtom atom = a;
			bool isRefCounted = false;

			AVM1toPrimitive(atom, getWorker(), isRefCounted, NUMBER_HINT);
			auto ret = AVM1toNumber(atom, swfversion, true);

			if (isRefCounted)
				ASATOM_DECREF(atom);
			return ret;
		}
	}
}
FORCE_INLINE bool asAtomHandler::AVM1toBool(asAtom& a, ASWorker* wrk, uint32_t swfversion)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_NUMBERPTR:
		{
			number_t r = toNumber(a) ;
			return !std::isnan(r) && r!= 0.0 ;
		}
		case ATOM_STRINGID:
			if (swfversion <7)
			{
				number_t r = toNumber(a) ;
				return r!=0.0 && !std::isnan(r);
			}
			else
				return getStringId(a) != BUILTIN_STRINGS::EMPTY;
		case ATOM_STRINGPTR:
			if (swfversion <7)
			{
				number_t r = toNumber(a) ;
				return r!=0.0 && !std::isnan(r);
			}
			else
				return !toString(a,wrk).empty();
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
			if (a.uintval & ATOMTYPE_LOCALNUMBER_BIT)
			{
				number_t r = toNumber(a) ;
				return !std::isnan(r) && r!= 0.0 ;
			}
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
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
			if (a.uintval & ATOMTYPE_LOCALNUMBER_BIT)
			{
				number_t n = getLocalNumber(getCallContext(getWorker()),a);
				if(std::isnan(n) || std::isinf(n))
					return INT64_MAX;
				return n;
			}
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
FORCE_INLINE uint32_t asAtomHandler::toUInt(const asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
			if (a.uintval & ATOMTYPE_LOCALNUMBER_BIT)
				return (unsigned int)localNumbertoInt(getWorker(),a);
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

FORCE_INLINE uint32_t asAtomHandler::getUInt(ASWorker* wrk, asAtom& a)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INTEGER:
			return a.intval>>3;
		case ATOM_UINTEGER:
			return a.uintval>>3;
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
			if (a.uintval & ATOMTYPE_LOCALNUMBER_BIT)
				return (unsigned int)localNumbertoInt(wrk,a);
			return (a.uintval&ATOMTYPE_BOOL_BIT) ? (a.uintval&0x80)>>7 : 0;
		case ATOM_STRINGID:
		{
			ASObject* s = abstract_s(wrk,a.uintval>>3);
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
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
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
			case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
			{
				if (a.uintval & ATOMTYPE_LOCALNUMBER_BIT)
					break;
				switch (v2.uintval&ATOMTYPE_BIT_FLAGS)
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
			case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
			{
				if (v2.uintval & ATOMTYPE_LOCALNUMBER_BIT)
					break;
				switch (v2.uintval&ATOMTYPE_BIT_FLAGS)
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
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
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
	a.uintval = ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER | ATOMTYPE_BOOL_BIT | (val ? 0x80 : 0);
}

FORCE_INLINE void asAtomHandler::setNull(asAtom& a)
{
	a.uintval = ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER | ATOMTYPE_NULL_BIT;
}
FORCE_INLINE void asAtomHandler::setUndefined(asAtom& a)
{
	a.uintval = ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER | ATOMTYPE_UNDEFINED_BIT;
}
FORCE_INLINE bool asAtomHandler::increment(asAtom& a, ASWorker* wrk, bool replace)
{
	switch(a.uintval&0x7)
	{
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
		{
			switch (a.uintval&ATOMTYPE_BIT_FLAGS)
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
				case ATOMTYPE_LOCALNUMBER_BIT:
				{
					number_t n = getLocalNumber(getCallContext(wrk),a);
					if (USUALLY_FALSE(std::isnan(n) || std::isinf(n)))
						break;
					else if(trunc(n) == n && n >= INT32_MIN && n < UINT32_MAX)
					{
						if (replace)
							ASATOM_DECREF(a);
						if (n < INT32_MAX)
							asAtomHandler::setInt(a,wrk,n+1);
						else
							asAtomHandler::setUInt(a,wrk,n+1);
					}
					else
						setNumber(a,wrk,n+1);
					break;
				}
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
			if (USUALLY_FALSE(std::isnan(n) || std::isinf(n)))
				break;
			else if(trunc(n) == n && n >= INT32_MIN && n < UINT32_MAX)
			{
				if (replace)
					ASATOM_DECREF(a);
				if (n < INT32_MAX)
					asAtomHandler::setInt(a,wrk,n+1);
				else
					asAtomHandler::setUInt(a,wrk,n+1);
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
			else if(trunc(n) == n && n >= INT32_MIN && n < UINT32_MAX)
			{
				if (n < INT32_MAX)
					asAtomHandler::setInt(a,wrk,n+1);
				else
					asAtomHandler::setUInt(a,wrk,n+1);
			}
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
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
		{
			switch (a.uintval&ATOMTYPE_BIT_FLAGS)
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
				case ATOMTYPE_LOCALNUMBER_BIT:
				{
					number_t n = getLocalNumber(getCallContext(wrk),a);
					if (USUALLY_FALSE(std::isnan(n) || std::isinf(n)))
						break;
					else if(trunc(n) == n && n > INT32_MIN && n <= UINT32_MAX)
					{
						if (replace)
							ASATOM_DECREF(a);
						if (n <= INT32_MAX)
							asAtomHandler::setInt(a,wrk,n-1);
						else
							asAtomHandler::setUInt(a,wrk,n-1);
					}
					else
						setNumber(a,wrk,n-1);
					break;
				}
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
				setInt(a,wrk,-1);
			break;
		}
		case ATOM_NUMBERPTR:
		{
			number_t n = getObjectNoCheck(a)->toNumber();
			if (USUALLY_FALSE(std::isnan(n) || std::isinf(n)))
				break;
			else if(trunc(n) == n && n > INT32_MIN && n <= UINT32_MAX)
			{
				if (replace)
					ASATOM_DECREF(a);
				if (n <= INT32_MAX)
					asAtomHandler::setInt(a,wrk,n-1);
				else
					asAtomHandler::setUInt(a,wrk,n-1);
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
			else if(trunc(n) == n && n > INT32_MIN && n <= UINT32_MAX)
			{
				if (n <= INT32_MAX)
					asAtomHandler::setInt(a,wrk,n-1);
				else
					asAtomHandler::setUInt(a,wrk,n-1);
			}
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
			a = fromNumber(wrk,res,false);
	}
	else
	{
		number_t num2=getNumber(wrk,v2);
		number_t num1=getNumber(wrk,a);

		LOG_CALL("subtract " << num1 << '-' << num2);
		if (forceint)
			setInt(a,wrk,num1-num2);
		else
			a = fromNumber(wrk,num1-num2,false);
	}
}
FORCE_INLINE void asAtomHandler::subtractreplace(asAtom& ret, ASWorker* wrk, const asAtom &v1, const asAtom &v2, bool forceint, uint16_t resultlocalnumberpos)
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
			setNumber(ret,wrk,res,resultlocalnumberpos);
	}
	else
	{
		number_t num2=getNumber(wrk,v2);
		number_t num1=getNumber(wrk,v1);

		ASObject* o = getObject(ret);
		LOG_CALL("subtractreplace "  << num1 << '-' << num2);
		if (forceint)
		{
			setInt(ret,wrk,num1-num2);
			if (o)
				o->decRef();
		}
		else if (resultlocalnumberpos != UINT16_MAX)
		{
			setNumber(ret,wrk,num1-num2,resultlocalnumberpos);
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
			a = fromNumber(wrk,res,false);
	}
	else
	{
		double num1=getNumber(wrk,v2);
		double num2=getNumber(wrk,a);
		LOG_CALL("multiply "  << num1 << '*' << num2);
		if (forceint)
			setInt(a,wrk,num1*num2);
		else
			a = fromNumber(wrk,num1*num2,false);
	}
}

FORCE_INLINE void asAtomHandler::multiplyreplace(asAtom& ret, ASWorker* wrk, const asAtom& v1, const asAtom &v2, bool forceint, uint16_t resultlocalnumberpos)
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
		else if (resultlocalnumberpos != UINT16_MAX)
		{
			setNumber(ret,wrk,num1*num2,resultlocalnumberpos);
			if (o)
				o->decRef();
		}
		else if (replaceNumber(ret,wrk,num1*num2) && o)
			o->decRef();
	}
	else
	{
		double num1=getNumber(wrk,v2);
		double num2=getNumber(wrk,v1);
		ASObject* o = getObject(ret);
		LOG_CALL("multiplyreplace "  << num1 << '*' << num2);
		if (forceint)
		{
			setInt(ret,wrk,num1*num2);
			if (o)
				o->decRef();
		}
		else if (resultlocalnumberpos != UINT16_MAX)
		{
			setNumber(ret,wrk,num1*num2,resultlocalnumberpos);
			if (o)
				o->decRef();
		}
		else if (replaceNumber(ret,wrk,num1*num2) && o)
			o->decRef();
		LOG_CALL("multiplyreplace done "  << num1*num2<<" "<<resultlocalnumberpos<<" "<<asAtomHandler::toDebugString(ret));
	}
}

FORCE_INLINE void asAtomHandler::divide(asAtom& a, ASWorker* wrk, asAtom &v2, bool forceint)
{
	double num1=getNumber(wrk,a);
	double num2=getNumber(wrk,v2);

	LOG_CALL("divide "  << num1 << '/' << num2);
	// handling of infinity according to ECMA-262, chapter 11.5.2
	if (std::isinf(num1))
	{
		if (std::isinf(num2) || std::isnan(num2))
			a = fromNumber(wrk,numeric_limits<double>::quiet_NaN(),false);
		else
		{
			bool needssign = (std::signbit(num1) || std::signbit(num2)) && !(std::signbit(num1) && std::signbit(num2));
			a = fromNumber(wrk, needssign  ? -numeric_limits<double>::infinity() : numeric_limits<double>::infinity(),false);
		}
	}
	else if (forceint)
		setInt(a,wrk,num1/num2);
	else
		a = fromNumber(wrk,num1/num2,false);
}
FORCE_INLINE void asAtomHandler::dividereplace(asAtom& ret, ASWorker* wrk, const asAtom& v1, const asAtom &v2, bool forceint, uint16_t resultlocalnumberpos)
{
	double num1=getNumber(wrk,v1);
	double num2=getNumber(wrk,v2);

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
	else if (resultlocalnumberpos != UINT16_MAX)
	{
		setNumber(ret,wrk,res,resultlocalnumberpos);
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
			a = fromNumber(wrk,::fmod(num1,num2),false);
	}
}
FORCE_INLINE void asAtomHandler::moduloreplace(asAtom& ret, ASWorker* wrk, const asAtom& v1, const asAtom &v2, bool forceint, uint16_t resultlocalnumberpos)
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
		number_t num1=getNumber(wrk,v1);
		number_t num2=getNumber(wrk,v2);
		LOG_CALL("moduloreplace "  << num1 << '%' << num2);
		/* fmod returns NaN if num2 == 0 as the spec mandates */
		ASObject* o = getObject(ret);
		if (forceint)
		{
			setInt(ret,wrk,::fmod(num1,num2));
			if (o)
				o->decRef();
		}
		else if (resultlocalnumberpos != UINT16_MAX)
		{
			setNumber(ret,wrk,::fmod(num1,num2),resultlocalnumberpos);
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
	uint32_t i1=getUInt(wrk,v1)&0x1f;
	LOG_CALL("lShift "<<std::hex<<i2<<"<<"<<i1<<std::dec);
	//Left shift are supposed to always work in 32bit
	setInt(a,wrk,i2<<i1);
}

FORCE_INLINE void asAtomHandler::rshift(asAtom& a, ASWorker* wrk, asAtom &v1)
{
	int32_t i2=toInt(a);
	uint32_t i1=getUInt(wrk,v1)&0x1f;
	LOG_CALL("rShift "<<std::hex<<i2<<">>"<<i1<<std::dec);
	setInt(a,wrk,i2>>i1);
}

FORCE_INLINE void asAtomHandler::urshift(asAtom& a, ASWorker* wrk, asAtom &v1)
{
	uint32_t i2=getUInt(wrk,a);
	uint32_t i1=getUInt(wrk,v1)&0x1f;
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
	{
		if (res >=0 && res <= UINT32_MAX)
			setUInt(a,wrk,res);
		else
			setNumber(a,wrk,res);
	}
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
FORCE_INLINE bool asAtomHandler::isNumberPtr(const asAtom& a)
{
	return (a.uintval&0x7)==ATOM_NUMBERPTR;
}
FORCE_INLINE bool asAtomHandler::isNumber(const asAtom& a)
{
	return (a.uintval&0x7)==ATOM_NUMBERPTR || isLocalNumber(a);
}
FORCE_INLINE bool asAtomHandler::isLocalNumber(const asAtom& a)
{
	return (a.uintval&0x7f)==ATOMTYPE_LOCALNUMBER_BIT;
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
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
			switch (a.uintval&ATOMTYPE_BIT_FLAGS)
			{
				case ATOMTYPE_NULL_BIT:
					return T_NULL;
				case ATOMTYPE_UNDEFINED_BIT:
					return T_UNDEFINED;
				case ATOMTYPE_LOCALNUMBER_BIT:
					return T_NUMBER;
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
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
		{
			switch (a.uintval&ATOMTYPE_BIT_FLAGS)
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
				case ATOMTYPE_LOCALNUMBER_BIT:
					ret=BUILTIN_STRINGS::STRING_NUMBER;
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
			!isNumber(a)) // number needs special handling for NaN
		return true;
	return isEqualIntern(a,wrk,v2);
}
TRISTATE asAtomHandler::isLess(asAtom& a, ASWorker* wrk, asAtom &v2)
{
	if ((((a.intval ^ ATOM_INTEGER) | (v2.intval ^ ATOM_INTEGER)) & 7) == 0)
		return (a.intval < v2.intval)?TTRUE:TFALSE;
	if (a.uintval == v2.uintval &&
			!isNumber(a)) // number needs special handling for NaN
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
		case ATOM_INVALID_UNDEFINED_NULL_BOOL_LOCALNUMBER:
		{
			switch (a.uintval&ATOMTYPE_BIT_FLAGS)
			{
				case ATOMTYPE_NULL_BIT:
				case ATOMTYPE_UNDEFINED_BIT:
					return false;
				case ATOMTYPE_BOOL_BIT:
					return (a.uintval&0x80)>>7;
				case ATOMTYPE_LOCALNUMBER_BIT:
					return toNumber(a) != 0.0 && !std::isnan(toNumber(a));
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
FORCE_INLINE bool asAtomHandler::isAccessible(const asAtom& a)
{
	return !(a.uintval & ATOMTYPE_OBJECT_BIT) || !((ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)))->getCached() || ((ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)))->getInDestruction();
}
FORCE_INLINE bool asAtomHandler::isAccessibleObject(const asAtom& a)
{
	return (a.uintval & ATOMTYPE_OBJECT_BIT) && !((ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)))->getCached() && !((ASObject*)(a.uintval& ~((LIGHTSPARK_ATOM_VALTYPE)0x7)))->getInDestruction();
}

FORCE_INLINE ASObject* asAtomHandler::getObject(const asAtom& a)
{
	assert(isAccessible(a));
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
	SyntheticFunction* function;
	void set(asAtom o, SyntheticFunction*f) { object=o; function=f; }
};

struct stacktrace_string_entry
{
	uint32_t clsname;
	uint32_t init;
	uint32_t function;
	uint32_t ns;
	uint32_t methodnumber;
	bool isGetter:1;
	bool isSetter:1;
};
typedef std::vector<stacktrace_string_entry> StackTraceList;
}
#endif /* ASOBJECT_H */
