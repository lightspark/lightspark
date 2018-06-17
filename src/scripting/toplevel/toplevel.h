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

#ifndef SCRIPTING_TOPLEVEL_TOPLEVEL_H
#define SCRIPTING_TOPLEVEL_TOPLEVEL_H 1

#include "compat.h"
#include <vector>
#include <set>
#include "asobject.h"
#include "exceptions.h"
#include "threading.h"
#include "scripting/abcutils.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Error.h"
#include "memory_support.h"
#include <boost/intrusive/list.hpp>

namespace pugi
{
	class xml_node;
}
namespace lightspark
{
const tiny_string AS3="http://adobe.com/AS3/2006/builtin";

class Event;
class ABCContext;
class Template_base;
class ASString;
class method_info;
struct call_context;
struct traits_info;
struct namespace_info;
class Any;
class Void;
class Class_object;
class ApplicationDomain;
extern bool isVmThread();
// Enum used during early binding in abc_optimizer.cpp
enum EARLY_BIND_STATUS { NOT_BINDED=0, CANNOT_BIND=1, BINDED };

/* This abstract class represents a type, i.e. something that a value can be coerced to.
 * Currently Class_base and Template_base implement this interface.
 * If you let another class implement this interface, change ASObject->is<Type>(), too!
 * You never take ownership of a type, so there is no need to incRef/decRef them.
 * Types are guaranteed to survive until SystemState::destroy().
 */
class Type
{
protected:
	/* this is private because one never deletes a Type */
	~Type() {}
public:
	static Any* const anyType;
	static Void* const voidType;
	/*
	 * This returns the Type for the given multiname.
	 * It searches for the and object of the type in global object.
	 * If no object of that name is found or it is not a Type,
	 * then an exception is thrown.
	 * The caller does not own the object returned.
	 */
	static const Type* getTypeFromMultiname(const multiname* mn, ABCContext* context);
	/*
	 * Checks if the type is already in sys->classes
	 */
	static const Type *getBuiltinType(SystemState* sys, const multiname* mn);
	/*
	 * Converts the given object to an object of this type.
	 * It consumes one reference of 'o'.
	 * The returned object must be decRef'ed by caller.
	 * If the argument cannot be converted, it throws a TypeError
	 */
	virtual void coerce(SystemState* sys, asAtom& o) const=0;

	virtual void coerceForTemplate(SystemState* sys, asAtom& o) const=0;
	
	/* Return "any" for anyType, "void" for voidType and class_name.name for Class_base */
	virtual tiny_string getName() const=0;

	/* Returns true if the given multiname is present in the declared traits of the type */
	virtual EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const = 0;

	/* Returns the type multiname for the given slot id */
	virtual const multiname* resolveSlotTypeName(uint32_t slotId) const = 0;

	/* returns true if this type is a builtin type, false for classes defined in the swf file */
	virtual bool isBuiltin() const = 0;
};
template<> inline Type* ASObject::as<Type>() { return dynamic_cast<Type*>(this); }
template<> inline const Type* ASObject::as<Type>() const { return dynamic_cast<const Type*>(this); }

class Any: public Type
{
public:
	void coerce(SystemState* sys,asAtom& o) const {}
	void coerceForTemplate(SystemState* sys, asAtom& o) const {}
	virtual ~Any() {}
	tiny_string getName() const { return "any"; }
	EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const { return CANNOT_BIND; }
	const multiname* resolveSlotTypeName(uint32_t slotId) const { return NULL; }
	bool isBuiltin() const { return true; }
};

class Void: public Type
{
public:
	void coerce(SystemState* sys,asAtom& o) const;
	void coerceForTemplate(SystemState* sys, asAtom& o) const { coerce(sys,o); }
	virtual ~Void() {}
	tiny_string getName() const { return "void"; }
	EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const { return NOT_BINDED; }
	const multiname* resolveSlotTypeName(uint32_t slotId) const { return NULL; }
	bool isBuiltin() const { return true; }
};

/*
 * This class is used exclusively to do early binding when a method uses newactivation
 */
class ActivationType: public Type
{
private:
	const method_info* mi;
public:
	ActivationType(const method_info* m):mi(m){}
	void coerce(SystemState* sys,asAtom& o) const { throw RunTimeException("Coercing to an ActivationType should not happen");}
	void coerceForTemplate(SystemState* sys,asAtom& o) const { throw RunTimeException("Coercing to an ActivationType should not happen");}
	virtual ~ActivationType() {}
	tiny_string getName() const { return "activation"; }
	EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const;
	const multiname* resolveSlotTypeName(uint32_t slotId) const;
	bool isBuiltin() const { return true; }
};

class Prototype;
class ObjectConstructor;

#define FREELIST_SIZE 16
struct asfreelist
{
	ASObject* freelist[FREELIST_SIZE];
	int freelistsize;
	asfreelist():freelistsize(0) {}
	~asfreelist() 
	{
		for (int i = 0; i < freelistsize; i++)
			delete freelist[i];
		freelistsize = 0;
	}

	inline ASObject* getObjectFromFreeList()
	{
#ifndef NDEBUG
		// all ASObjects must be created in the VM thread
		//assert_and_throw(isVmThread());
#endif
		return freelistsize ? freelist[--freelistsize] :NULL;
	}
	inline bool pushObjectToFreeList(ASObject *obj)
	{
#ifndef NDEBUG
		// all ASObjects must be created in the VM thread
		//assert_and_throw(isVmThread());
#endif
		assert(obj->getRefCount() == 1);
		if (freelistsize < FREELIST_SIZE)
		{
			freelist[freelistsize++]=obj;
			return true;
		}
		return false;
	}
};

class Class_base: public ASObject, public Type
{
friend class ABCVm;
friend class ABCContext;
template<class T> friend class Template;
private:
	mutable std::vector<multiname> interfaces;
	mutable std::vector<Class_base*> interfaces_added;
	nsNameAndKind protected_ns;
	void initializeProtectedNamespace(uint32_t nameId, const namespace_info& ns);
	IFunction* constructor;
	void describeTraits(pugi::xml_node &root, std::vector<traits_info>& traits, std::map<varName,pugi::xml_node> &propnames, bool first) const;
	void describeVariables(pugi::xml_node &root, const Class_base* c, std::map<tiny_string, pugi::xml_node *> &instanceNodes, const variables_map& map, bool isTemplate) const;
	void describeConstructor(pugi::xml_node &root) const;
	virtual void describeClassMetadata(pugi::xml_node &root) const {}
protected:
	void describeMetadata(pugi::xml_node &node, const traits_info& trait) const;
	void copyBorrowedTraitsFromSuper();
	ASFUNCTION_ATOM(_toString);
	void initStandardProps();
	void destroy();
public:
	asfreelist freelist[2];
	variables_map borrowedVariables;
	ASPROPERTY_GETTER(_NR<Prototype>,prototype);
	ASPROPERTY_GETTER(_NR<ObjectConstructor>,constructorprop);
	_NR<Class_base> super;
	//We need to know what is the context we are referring to
	ABCContext* context;
	const QName class_name;
	//Memory reporter to keep track of used bytes
	MemoryAccount* memoryAccount;
	ASPROPERTY_GETTER(int32_t,length);
	int32_t class_index;
	bool isFinal:1;
	bool isSealed:1;
	bool isInterface:1;
	
	// indicates if objects can be reused after they have lost their last reference
	bool isReusable:1;
private:
	//TODO: move in Class_inherit
	bool use_protected:1;
public:
	void addConstructorGetter();
	void addPrototypeGetter();
	void addLengthGetter();
	inline virtual void setupDeclaredTraits(ASObject *target) const { target->traitsInitialized = true; }
	void handleConstruction(asAtom &target, asAtom *args, unsigned int argslen, bool buildAndLink);
	void setConstructor(IFunction* c);
	bool hasConstructor() { return constructor != NULL; }
	Class_base(const QName& name, MemoryAccount* m);
	//Special constructor for Class_object
	Class_base(const Class_object*);
	~Class_base();
	void finalize();
	virtual void getInstance(asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass=NULL)=0;
	void addImplementedInterface(const multiname& i);
	void addImplementedInterface(Class_base* i);
	virtual void buildInstanceTraits(ASObject* o) const=0;
	const std::vector<Class_base*>& getInterfaces(bool *alldefined = NULL) const;
	virtual void linkInterface(Class_base* c) const;
	/*
	 * Returns true when 'this' is a subclass of 'cls',
	 * i.e. this == cls or cls equals some super of this.
         * If considerInterfaces is true, check interfaces, too.
	 */
	bool isSubClass(const Class_base* cls, bool considerInterfaces=true) const;
	tiny_string getQualifiedClassName(bool forDescribeType = false) const;
	tiny_string getName() const;
	tiny_string toString();
	virtual void generator(asAtom &ret, asAtom* args, const unsigned int argslen);
	ASObject *describeType() const;
	void describeInstance(pugi::xml_node &root, bool istemplate) const;
	virtual const Template_base* getTemplate() const { return NULL; }
	/*
	 * Converts the given object to an object of this Class_base's type.
	 * It consumes one reference of 'o'.
	 * The returned object must be decRef'ed by caller.
	 */
	virtual void coerce(SystemState* sys, asAtom& o) const;
	
	void coerceForTemplate(SystemState* sys, asAtom& o) const;

	void setSuper(_R<Class_base> super_);
	inline const variable* findBorrowedGettable(const multiname& name, uint32_t* nsRealId = NULL) const DLL_LOCAL
	{
		return ASObject::findGettableImplConst(getSystemState(), borrowedVariables,name,nsRealId);
	}
	
	variable* findBorrowedSettable(const multiname& name, bool* has_getter=NULL) DLL_LOCAL;
	variable* findSettableInPrototype(const multiname& name) DLL_LOCAL;
	EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const;
	const multiname* resolveSlotTypeName(uint32_t slotId) const { /*TODO: implement*/ return NULL; }
	bool checkExistingFunction(const multiname& name);
	void getClassVariableByMultiname(asAtom& ret,const multiname& name);
	bool isBuiltin() const { return true; }
};

class Template_base : public ASObject
{
private:
	QName template_name;
public:
	Template_base(QName name);
	virtual Class_base* applyType(const std::vector<const Type*>& t,_NR<ApplicationDomain> appdomain)=0;
	QName getTemplateName() { return template_name; }
	ASPROPERTY_GETTER(_NR<Prototype>,prototype);
	void addPrototypeGetter(SystemState *sys);
};

class Class_object: public Class_base
{
private:
	//Invoke the special constructor that will set the super to Object
	Class_object():Class_base(this){}
	void getInstance(asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass)
	{
		throw RunTimeException("Class_object::getInstance");
	}
	void buildInstanceTraits(ASObject* o) const
	{
//		throw RunTimeException("Class_object::buildInstanceTraits");
	}
	void finalize()
	{
		//Remove the cyclic reference to itself
		setClass(NULL);
		Class_base::finalize();
	}
	
public:
	static Class_object* getClass(SystemState* sys);
	
	static _R<Class_object> getRef(SystemState* sys)
	{
		Class_object* ret = getClass(sys);
		ret->incRef();
		return _MR(ret);
	}
	
};

class Prototype
{
protected:
	ASObject* obj;
public:
	Prototype():isSealed(false) {}
	virtual ~Prototype() {}
	_NR<Prototype> prevPrototype;
	inline void incRef() { obj->incRef(); }
	inline void decRef() { obj->decRef(); }
	inline ASObject* getObj() {return obj; }
	bool isSealed;
	/*
	 * This method is actually forwarded to the object. It's here as a shorthand.
	 */
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind)
	{
		getObj()->setVariableByQName(name,ns,o,traitKind);
	}
};

/* Special object used as prototype for classes
 * It keeps a link to the upper level in the prototype chain
 */
class ObjectPrototype: public ASObject, public Prototype
{
public:
	ObjectPrototype(Class_base* c);
	inline void finalize() { prevPrototype.reset(); }
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt=NONE);
	void setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst);
	bool isEqual(ASObject* r);
};

/* Special object used as constructor property for classes
 * It has its own prototype object, but everything else is forwarded to the class object
 */
class ObjectConstructor: public ASObject
{
	Prototype* prototype;
	uint32_t _length;
public:
	ObjectConstructor(Class_base* c,uint32_t length);
	void incRef() { getClass()->incRef(); }
	void decRef() { getClass()->decRef(); }
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt=NONE);
	bool isEqual(ASObject* r);
};


class Activation_object: public ASObject
{
public:
    Activation_object(Class_base* c) : ASObject(c,T_OBJECT,SUBTYPE_ACTIVATIONOBJECT) {}
};

/* Special object returned when new func() syntax is used.
 * This object looks for properties on the prototype object that is passed in the constructor
 */
class Function_object: public ASObject
{
public:
	Function_object(Class_base* c, _R<ASObject> p);
	_NR<ASObject> functionPrototype;
	void finalize() { functionPrototype.reset(); }

	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt=NONE);
};

/*
 * The base-class for everything that resambles a function or method.
 * It is specialized in Function for C-implemented functions
 * and SyntheticFunction for AS3-implemented functions (from the SWF)
 */
class IFunction: public ASObject
{
public:
	uint32_t length;
	ASFUNCTION_ATOM(_length);
protected:
	IFunction(Class_base *c,CLASS_SUBTYPE st);
public:
	static void sinit(Class_base* c);
	/* If this is a method, inClass is the class this is defined in.
	 * If this is a function, inClass == NULL
	 */
	Class_base* inClass;
	/* returns whether this is this a method of a function */
	bool isMethod() const { return inClass != NULL; }
	bool isConstructed() const { return constructIndicator; }
	inline bool destruct() 
	{
		inClass=NULL;
		functionname=0;
		length=0;
		return ASObject::destruct();
	}
	ASFUNCTION_ATOM(apply);
	ASFUNCTION_ATOM(_call);
	ASFUNCTION_ATOM(_toString);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,prototype);
	virtual method_info* getMethodInfo() const=0;
	virtual ASObject *describeType() const;
	uint32_t functionname;
	virtual void callGetter(asAtom& ret, ASObject* target) =0;
};

/*
 * Implements the IFunction interface for functions implemented
 * in c-code.
 */
class FunctionPrototype;

class Function : public IFunction
{
friend class Class<IFunction>;
friend class Class_base;
public:
typedef void (*as_atom_function)(asAtom&, SystemState*, asAtom&, asAtom*, const unsigned int);
protected:
	/* Function pointer to the C-function implementation with atom arguments */
	as_atom_function val_atom;
	
	// type of the return value;
	Class_base* returnType;
	Function(Class_base* c,as_atom_function v = NULL):IFunction(c,SUBTYPE_FUNCTION),val_atom(v) {}
	method_info* getMethodInfo() const { return NULL; }
public:
	/**
	 * This executes a C++ function.
	 * It consumes _no_ references of obj and args
	 */
	FORCE_INLINE void call(asAtom& ret, asAtom& obj, asAtom* args, uint32_t num_args)
	{
		/*
		 * We do not enforce ABCVm::limits.max_recursion here.
		 * This should be okey, because there is no infinite recursion
		 * using only builtin functions.
		 * Additionally, we still need to run builtin code (such as the ASError constructor) when
		 * ABCVm::limits.max_recursion is reached in SyntheticFunction::call.
		 */
		val_atom(ret,getSystemState(),obj,args,num_args);
	}
	bool isEqual(ASObject* r);
	FORCE_INLINE void callGetter(asAtom& ret, ASObject* target)
	{
		asAtom c = asAtom::fromObject(target);
		val_atom(ret,getSystemState(),c,NULL,0);
	}
};

/* Special object used as prototype for the Function class
 * It keeps a link to the upper level in the prototype chain
 */
class FunctionPrototype: public Function, public Prototype
{
public:
	FunctionPrototype(Class_base* c, _NR<Prototype> p);
	inline bool destruct()
	{
		prevPrototype.reset();
		return Function::destruct();
	}
	
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt=NONE);
};

/*
 * Represents a function implemented in AS3-code
 */
class SyntheticFunction : public IFunction
{
friend class ABCVm;
friend class Class<IFunction>;
friend class Class_base;
public:
	typedef ASObject* (*synt_function)(call_context* cc);
private:
	/* Data structure with information directly loaded from the SWF */
	method_info* mi;
	/* Pointer to JIT-compiled function or NULL if not yet compiled */
	synt_function val;
	SyntheticFunction(Class_base* c,method_info* m);
	
	method_info* getMethodInfo() const { return mi; }
public:
	~SyntheticFunction() {}
	void call(asAtom &ret, asAtom& obj, asAtom *args, uint32_t num_args, bool coerceresult);
	bool destruct();
	
	_NR<scope_entry_list> func_scope;
	bool isEqual(ASObject* r)
	{
		return this == r;
	}
	void acquireScope(const std::vector<scope_entry>& scope)
	{
		if (func_scope.isNull())
			func_scope = _NR<scope_entry_list>(new scope_entry_list());
		assert_and_throw(func_scope->scope.empty());
		func_scope->scope=scope;
	}
	void addToScope(const scope_entry& s)
	{
		if (func_scope.isNull())
			func_scope = _NR<scope_entry_list>(new scope_entry_list());
		func_scope->scope.emplace_back(s);
	}
	FORCE_INLINE void callGetter(asAtom& ret, ASObject* target)
	{
		asAtom c = asAtom::fromObject(target);
		call(ret,c,NULL,0,true);
	}
};

/*
 * The Class of a Function
 */
template<>
class Class<IFunction>: public Class_base
{
private:
	Class<IFunction>(MemoryAccount* m):Class_base(QName(BUILTIN_STRINGS::STRING_FUNCTION,BUILTIN_STRINGS::EMPTY),m){}
	void getInstance(asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass);
	IFunction* getNopFunction();
public:
	static Class<IFunction>* getClass(SystemState* sys);
	static _R<Class<IFunction>> getRef(SystemState* sys)
	{
		Class<IFunction>* ret = getClass(sys);
		ret->incRef();
		return _MR(ret);
	}
	static Function* getFunction(SystemState* sys,Function::as_atom_function v, int len = 0, Class_base* returnType=NULL)
	{
		Class<IFunction>* c=Class<IFunction>::getClass(sys);
		Function*  ret = c->freelist[0].getObjectFromFreeList()->as<Function>();
		if (!ret)
			ret=new (c->memoryAccount) Function(c);
		ret->val_atom = v;
		ret->returnType = returnType;
		ret->length = len;
		ret->constructIndicator = true;
		ret->constructorCallComplete = true;
		return ret;
	}

	static SyntheticFunction* getSyntheticFunction(SystemState* sys,method_info* m, uint32_t _length)
	{
		Class<IFunction>* c=Class<IFunction>::getClass(sys);
		SyntheticFunction*  ret;
		ret= c->freelist[1].getObjectFromFreeList()->as<SyntheticFunction>();
		if (!ret)
		{
			ret=new (c->memoryAccount) SyntheticFunction(c, m);
		}
		else
		{
			ret->mi = m;
			ret->length = _length;
			ret->objfreelist = &c->freelist[1];
		}

		ret->constructIndicator = true;
		ret->constructorCallComplete = true;
		asAtom obj = asAtom::fromObject(ret);
		c->handleConstruction(obj,NULL,0,true);
		return ret;
	}
	void buildInstanceTraits(ASObject* o) const
	{
	}
	virtual void generator(asAtom& ret, asAtom* args, const unsigned int argslen);
};

class Undefined : public ASObject
{
public:
	ASFUNCTION_ATOM(call);
	Undefined();
	int32_t toInt();
	int64_t toInt64();
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	ASObject *describeType() const;
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
	void setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst);
};

class Null: public ASObject
{
public:
	Null();
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	int32_t toInt();
	int64_t toInt64();
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt);
	int32_t getVariableByMultiname_i(const multiname& name);
	void setVariableByMultiname(const multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst);

	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};

class ASQName: public ASObject
{
friend struct multiname;
friend class Namespace;
private:
	bool uri_is_null;
	uint32_t uri;
	uint32_t local_name;
public:
	ASQName(Class_base* c);
	void setByXML(XML* node);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_getURI);
	ASFUNCTION_ATOM(_getLocalName);
	ASFUNCTION_ATOM(_toString);
	uint32_t getURI() const { return uri; }
	uint32_t getLocalName() const { return local_name; }
	bool isEqual(ASObject* o);

	tiny_string toString();

	uint32_t nextNameIndex(uint32_t cur_index);
	void nextName(asAtom &ret, uint32_t index);
	void nextValue(asAtom &ret, uint32_t index);
};

class Namespace: public ASObject
{
friend class ASQName;
friend class ABCContext;
private:
	NS_KIND nskind;
	bool prefix_is_undefined;
	uint32_t uri;
	uint32_t prefix;
public:
	Namespace(Class_base* c);
	Namespace(Class_base* c, uint32_t _uri, uint32_t _prefix=BUILTIN_STRINGS::EMPTY,NS_KIND _nskind = NAMESPACE);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_getURI);
	// according to ECMA-357 and tamarin tests uri/prefix properties are readonly
	//ASFUNCTION_ATOM(_setURI);
	ASFUNCTION_ATOM(_getPrefix);
	//ASFUNCTION_ATOM(_setPrefix);
	ASFUNCTION_ATOM(_toString);
	ASFUNCTION_ATOM(_valueOf);
	ASFUNCTION_ATOM(_ECMA_valueOf);
	bool isEqual(ASObject* o);
	uint32_t getURI() const { return uri; }
	uint32_t getPrefix(bool& is_undefined) { is_undefined=prefix_is_undefined; return prefix; }

	uint32_t nextNameIndex(uint32_t cur_index);
	void nextName(asAtom &ret, uint32_t index);
	void nextValue(asAtom &ret, uint32_t index);
};


class Global : public ASObject
{
private:
	int scriptId;
	ABCContext* context;
public:
	Global(Class_base* cb, ABCContext* c, int s);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o) {}
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt=NONE);
	void getVariableByMultinameOpportunistic(asAtom& ret, const multiname& name);
	/*
	 * Utility method to register builtin methods and classes
	 */
	void registerBuiltin(const char* name, const char* ns, _R<ASObject> o);
};

void eval(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void parseInt(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void parseFloat(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void isNaN(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void isFinite(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void encodeURI(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void decodeURI(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void encodeURIComponent(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void decodeURIComponent(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void escape(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void unescape(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void print(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
void trace(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
bool isXMLName(SystemState *sys, asAtom &obj);
void _isXMLName(asAtom& ret,SystemState* sys, asAtom& obj,asAtom* args, const unsigned int argslen);
number_t parseNumber(const tiny_string str, bool useoldversion=false);
};

#endif /* SCRIPTING_TOPLEVEL_TOPLEVEL_H */
