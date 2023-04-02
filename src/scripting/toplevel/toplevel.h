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
#include <unordered_set>
#include "asobject.h"
#include "exceptions.h"
#include "threading.h"
#include "scripting/abcutils.h"
#include "scripting/toplevel/Array.h"
#include "scripting/flash/system/flashsystem.h"
#include "memory_support.h"

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
class ASWorker;
extern bool isVmThread();

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
	static const Type* getTypeFromMultiname(multiname* mn, ABCContext* context, bool opportunistic=false);
	/*
	 * Checks if the type is already in sys->classes
	 */
	static const Type *getBuiltinType(ASWorker* wrk, multiname* mn);
	/*
	 * Converts the given object to an object of this type.
	 * If the argument cannot be converted, it throws a TypeError
	 * returns true if the atom is really converted into another instance
	 */
	virtual bool coerce(ASWorker* wrk, asAtom& o) const=0;

	virtual void coerceForTemplate(ASWorker* wrk, asAtom& o) const=0;
	
	/* Return "any" for anyType, "void" for voidType and class_name.name for Class_base */
	virtual tiny_string getName() const=0;

	/* Returns true if the given multiname is present in the declared traits of the type */
	virtual EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const = 0;

	/* Returns the type multiname for the given slot id */
	virtual const multiname* resolveSlotTypeName(uint32_t slotId) const = 0;

	/* returns true if this type is a builtin type, false for classes defined in the swf file */
	virtual bool isBuiltin() const = 0;
	
	/* returns the Global object this type is associated to */
	virtual Global* getGlobalScope() const = 0;
};
template<> inline Type* ASObject::as<Type>() { return dynamic_cast<Type*>(this); }
template<> inline const Type* ASObject::as<Type>() const { return dynamic_cast<const Type*>(this); }

class Any: public Type
{
public:
	bool coerce(ASWorker* wrk,asAtom& o) const override { return false; }
	void coerceForTemplate(ASWorker* wrk, asAtom& o) const override {}
	virtual ~Any() {}
	tiny_string getName() const override { return "any"; }
	EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const override { return CANNOT_BIND; }
	const multiname* resolveSlotTypeName(uint32_t slotId) const override { return nullptr; }
	bool isBuiltin() const override { return true; }
	Global* getGlobalScope() const override { return nullptr; }
};

class Void: public Type
{
public:
	bool coerce(ASWorker* wrk,asAtom& o) const override { return false; }
	void coerceForTemplate(ASWorker* wrk, asAtom& o) const override { }
	virtual ~Void() {}
	tiny_string getName() const override { return "void"; }
	EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const override { return NOT_BINDED; }
	const multiname* resolveSlotTypeName(uint32_t slotId) const override { return nullptr; }
	bool isBuiltin() const override { return true; }
	Global* getGlobalScope() const override { return nullptr; }
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
	bool coerce(ASWorker* wrk,asAtom& o) const override { throw RunTimeException("Coercing to an ActivationType should not happen");}
	void coerceForTemplate(ASWorker* wrk,asAtom& o) const override { throw RunTimeException("Coercing to an ActivationType should not happen");}
	virtual ~ActivationType() {}
	tiny_string getName() const override { return "activation"; }
	EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const override;
	const multiname* resolveSlotTypeName(uint32_t slotId) const override;
	bool isBuiltin() const override { return true; }
	Global* getGlobalScope() const override { return nullptr; }
};

class Prototype;
class ObjectConstructor;

class Class_base: public ASObject, public Type
{
friend class ABCVm;
friend class ABCContext;
template<class T> friend class Template;
private:
	mutable std::vector<multiname> interfaces;
	mutable std::vector<Class_base*> interfaces_added;
	std::unordered_set<uint32_t> overriddenmethods;
	nsNameAndKind protected_ns;
	void initializeProtectedNamespace(uint32_t nameId, const namespace_info& ns,RootMovieClip* root);
	IFunction* constructor;
	void describeTraits(pugi::xml_node &root, std::vector<traits_info>& traits, std::map<varName,pugi::xml_node> &propnames, bool first) const;
	void describeVariables(pugi::xml_node &root, const Class_base* c, std::map<tiny_string, pugi::xml_node *> &instanceNodes, const variables_map& map, bool isTemplate, bool forinstance) const;
	void describeConstructor(pugi::xml_node &root) const;
	virtual void describeClassMetadata(pugi::xml_node &root) const {}
	uint32_t qualifiedClassnameID;
protected:
	Global* global;
	void describeMetadata(pugi::xml_node &node, const traits_info& trait) const;
	void copyBorrowedTraitsFromSuper();
	ASFUNCTION_ATOM(_toString);
	void initStandardProps();
public:
	virtual asfreelist* getFreeList(ASWorker* w)
	{
		return isReusable && w ? &w->freelist[classID] : nullptr ;
	}
	variables_map borrowedVariables;
	_NR<Prototype> prototype;
	Prototype* getPrototype(ASWorker* wrk) const;
	ASFUNCTION_ATOM(_getter_prototype);
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
	uint32_t classID;
	void addConstructorGetter();
	void addPrototypeGetter();
	void addLengthGetter();
	inline virtual void setupDeclaredTraits(ASObject *target, bool checkclone=true) { target->traitsInitialized = true; }
	void handleConstruction(asAtom &target, asAtom *args, unsigned int argslen, bool buildAndLink);
	void setConstructor(IFunction* c);
	bool hasConstructor() { return constructor != nullptr; }
	IFunction* getConstructor() { return constructor; }
	Class_base(const QName& name, uint32_t _classID, MemoryAccount* m);
	//Special constructor for Class_object
	Class_base(const Class_object*c);
	~Class_base();
	void finalize() override;
	void prepareShutdown() override;
	virtual void getInstance(ASWorker* worker, asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass=nullptr)=0;
	void addImplementedInterface(const multiname& i);
	void addImplementedInterface(Class_base* i);
	virtual void buildInstanceTraits(ASObject* o) const=0;
	const std::vector<Class_base*>& getInterfaces(bool *alldefined = nullptr) const;
	virtual void linkInterface(Class_base* c) const;
	/*
	 * Returns true when 'this' is a subclass of 'cls',
	 * i.e. this == cls or cls equals some super of this.
	 * If considerInterfaces is true, check interfaces, too.
	 */
	bool isSubClass(const Class_base* cls, bool considerInterfaces=true) const;
	const tiny_string getQualifiedClassName(bool forDescribeType = false) const;
	uint32_t getQualifiedClassNameID();
	tiny_string getName() const override;
	tiny_string toString();
	virtual void generator(ASWorker* wrk,asAtom &ret, asAtom* args, const unsigned int argslen);
	ASObject *describeType(ASWorker* wrk) const override;
	void describeInstance(pugi::xml_node &root, bool istemplate, bool forinstance) const;
	virtual const Template_base* getTemplate() const { return nullptr; }
	/*
	 * Converts the given object to an object of this Class_base's type.
	 * The returned object must be decRef'ed by caller.
	 */
	bool coerce(ASWorker* wrk, asAtom& o) const override;
	
	void coerceForTemplate(ASWorker* wrk, asAtom& o) const override;

	void setSuper(_R<Class_base> super_);
	inline const variable* findBorrowedGettable(const multiname& name, uint32_t* nsRealId = nullptr) const
	{
		return ASObject::findGettableImplConst(getSystemState(), borrowedVariables,name,nsRealId);
	}
	
	variable* findBorrowedSettable(const multiname& name, bool* has_getter=nullptr);
	variable* findSettableInPrototype(const multiname& name);
	EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const override;
	const multiname* resolveSlotTypeName(uint32_t slotId) const override { /*TODO: implement*/ return nullptr; }
	bool checkExistingFunction(const multiname& name);
	void getClassVariableByMultiname(asAtom& ret, const multiname& name, ASWorker* wrk);
	variable* getBorrowedVariableByMultiname(const multiname& name)
	{
		return borrowedVariables.findObjVar(getSystemState(),name,NO_CREATE_TRAIT,DECLARED_TRAIT);
	}
	bool isBuiltin() const override { return true; }
	Global* getGlobalScope() const override { return global; }
	void setGlobalScope(Global* g) { global = g; }
	bool implementsInterfaces() const { return interfaces.size() || interfaces_added.size(); }
	bool isInterfaceMethod(const multiname &name);
	void removeAllDeclaredProperties();
	virtual bool hasoverriddenmethod(multiname* name) const
	{
		return overriddenmethods.find(name->name_s_id) != overriddenmethods.end();
	}
	void addoverriddenmethod(uint32_t nameID)
	{
		overriddenmethods.insert(nameID);
	}
	// Class_base needs special handling for dynamic variables in background workers
	multiname* setVariableByMultiname(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype, ASWorker* wrk) override;
	variable* findVariableByMultiname(const multiname& name, Class_base* cls, uint32_t* nsRealID, bool* isborrowed, bool considerdynamic, ASWorker* wrk) override;
};

class Template_base : public ASObject, public Type
{
private:
	QName template_name;
public:
	Template_base(ASWorker* wrk,QName name);
	virtual Class_base* applyType(const std::vector<const Type*>& t,_NR<ApplicationDomain> appdomain)=0;
	QName getTemplateName() { return template_name; }
	ASPROPERTY_GETTER(_NR<Prototype>,prototype);
	void addPrototypeGetter(SystemState *sys);


	bool coerce(ASWorker* wrk, asAtom& o) const override	{ return false;}
	void coerceForTemplate(ASWorker* wrk, asAtom& o) const override {}
	tiny_string getName() const override { return "template"; }
	EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const override { return CANNOT_BIND;}
	const multiname* resolveSlotTypeName(uint32_t slotId) const override { return nullptr; }
	bool isBuiltin() const override { return true;}
	Global* getGlobalScope() const override	{ return nullptr; }
};

class Class_object: public Class_base
{
private:
	//Invoke the special constructor that will set the super to Object
	Class_object():Class_base(this){}
	void getInstance(ASWorker* worker,asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass) override
	{
		throw RunTimeException("Class_object::getInstance");
	}
	void buildInstanceTraits(ASObject* o) const override
	{
//		throw RunTimeException("Class_object::buildInstanceTraits");
	}
	void finalize() override
	{
		//Remove the cyclic reference to itself
		setClass(nullptr);
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
	ASObject* workerDynamicClassVars; // this contains dynamically added properties to the class this prototype belongs to if it is added in a background worker
	ASObject* originalPrototypeVars;
	void copyOriginalValues(Prototype* target);
	void reset()
	{
		if (originalPrototypeVars)
			originalPrototypeVars->decRef();
		originalPrototypeVars=nullptr;
		if (workerDynamicClassVars)
			workerDynamicClassVars->decRef();
		workerDynamicClassVars=nullptr;
		prevPrototype.reset();
		if (obj)
			obj->decRef();
		obj=nullptr;
	}
	void prepShutdown()
	{
		if (obj)
			obj->prepareShutdown();
		if (prevPrototype)
			prevPrototype->prepShutdown();
		if (originalPrototypeVars)
			originalPrototypeVars->prepareShutdown();
		if (workerDynamicClassVars)
			workerDynamicClassVars->prepareShutdown();
	}
public:
	Prototype():obj(nullptr),workerDynamicClassVars(nullptr),originalPrototypeVars(nullptr),isSealed(false) {}
	virtual ~Prototype()
	{
	}
	_NR<Prototype> prevPrototype;
	inline void incRef() { if (obj) obj->incRef(); }
	inline void decRef() { if (obj) obj->decRef(); }
	inline ASObject* getObj() {return obj; }
	inline ASObject* getWorkerDynamicClassVars() const {return workerDynamicClassVars; }
	bool isSealed;
	/*
	 * This method is actually forwarded to the object. It's here as a shorthand.
	 */
	void setVariableByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, TRAIT_KIND traitKind);
	void setVariableByQName(const tiny_string& name, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind);
	void setVariableByQName(uint32_t nameID, const nsNameAndKind& ns, ASObject* o, TRAIT_KIND traitKind);
	void setVariableAtomByQName(const tiny_string& name, const nsNameAndKind& ns, asAtom o, TRAIT_KIND traitKind);
	void setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, IFunction* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable= true);
	
	virtual Prototype* clonePrototype(ASWorker* wrk) = 0;
};

/* Special object used as prototype for classes
 * It keeps a link to the upper level in the prototype chain
 */
class ObjectPrototype: public ASObject, public Prototype
{
public:
	ObjectPrototype(ASWorker* wrk,Class_base* c);
	inline bool destruct() override
	{
		reset();
		return ASObject::destruct();
	}
	void finalize() override
	{
		reset();
	}
	void prepareShutdown() override;
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;
	bool isEqual(ASObject* r) override;
	Prototype* clonePrototype(ASWorker* wrk) override;
};

/* Special object used as constructor property for classes
 * It has its own prototype object, but everything else is forwarded to the class object
 */
class ObjectConstructor: public ASObject
{
	Prototype* prototype;
	uint32_t _length;
public:
	ObjectConstructor(ASWorker* wrk, Class_base* c,uint32_t length);
	inline bool destruct() override
	{
		prototype=nullptr;
		return ASObject::destruct();
	}
	void finalize() override
	{
		prototype=nullptr;
	}
	void incRef() { if (getClass()) getClass()->incRef(); }
	void decRef() { if (getClass()) getClass()->decRef(); }
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	bool isEqual(ASObject* r) override;
};

class ArrayPrototype: public Array, public Prototype
{
public:
	ArrayPrototype(ASWorker* wrk,Class_base* c);
	inline bool destruct() override
	{
		reset();
		return Array::destruct();
	}
	void finalize() override
	{
		reset();
	}
	void prepareShutdown() override;
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;
	bool isEqual(ASObject* r) override;
	Prototype* clonePrototype(ASWorker* wrk) override;
};

class Activation_object: public ASObject
{
public:
	Activation_object(ASWorker* wrk, Class_base* c) : ASObject(wrk,c,T_OBJECT,SUBTYPE_ACTIVATIONOBJECT) {}
};

/* Special object returned when new func() syntax is used.
 * This object looks for properties on the prototype object that is passed in the constructor
 */
class Function_object: public ASObject
{
public:
	Function_object(ASWorker* wrk, Class_base* c, _R<ASObject> p);
	_NR<ASObject> functionPrototype;
	void finalize() override { functionPrototype.reset(); }
	void prepareShutdown() override;
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
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
	IFunction(ASWorker* wrk,Class_base *c, CLASS_SUBTYPE st);
	virtual IFunction* clone(ASWorker* wrk)=0;
public:
	ASObject* closure_this;
	static void sinit(Class_base* c);
	/* If this is a method, inClass is the class this is defined in.
	 * If this is a function, inClass == NULL
	 */
	Class_base* inClass;
	// if this is a class method, this indicates if it is a static or instance method
	bool isStatic;
	IFunction* clonedFrom;
	/* returns whether this is this a method of a function */
	bool isMethod() const { return inClass != nullptr; }
	bool isConstructed() const override { return constructIndicator; }
	inline bool destruct() override
	{
		inClass=nullptr;
		isStatic=false;
		clonedFrom=nullptr;
		functionname=0;
		length=0;
		if (closure_this)
			closure_this->removeStoredMember();
		closure_this=nullptr;
		prototype.reset();
		return destructIntern();
	}
	inline void finalize() override
	{
		inClass=nullptr;
		clonedFrom=nullptr;
		if (closure_this)
			closure_this->removeStoredMember();
		closure_this=nullptr;
		prototype.reset();
	}
	
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	IFunction* bind(ASObject* c, ASWorker* wrk)
	{
		IFunction* ret=nullptr;
		ret=clone(wrk);
		ret->setClass(getClass());
		ret->closure_this=c;
		ret->closure_this->incRef();
		ret->closure_this->addStoredMember();
		ret->clonedFrom=this;
		ret->isStatic=isStatic;
		ret->constructIndicator = true;
		ret->constructorCallComplete = true;
		return ret;
	}
	IFunction* createFunctionInstance(ASWorker* wrk)
	{
		IFunction* ret=nullptr;
		ret=clone(wrk);
		ret->setClass(getClass());
		ret->isStatic=isStatic;
		ret->constructIndicator = true;
		ret->constructorCallComplete = true;
		return ret;
	}
	ASFUNCTION_ATOM(apply);
	ASFUNCTION_ATOM(_call);
	ASFUNCTION_ATOM(_toString);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,prototype);
	virtual method_info* getMethodInfo() const=0;
	ASObject *describeType(ASWorker* wrk) const override;
	uint32_t functionname;
	virtual multiname* callGetter(asAtom& ret, ASObject* target,ASWorker* wrk) =0;
	virtual Class_base* getReturnType(bool opportunistic=false) =0;
	std::string toDebugString() const override;
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
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
typedef void (*as_atom_function)(asAtom&, ASWorker*, asAtom&, asAtom*, const unsigned int);
protected:
	/* Function pointer to the C-function implementation with atom arguments */
	as_atom_function val_atom;
	
	// type of the return value
	Class_base* returnType;
	// type of the return value if all arguments are integer
	Class_base* returnTypeAllArgsInt;
	Function(ASWorker* wrk,Class_base* c,as_atom_function v = nullptr):IFunction(wrk,c,SUBTYPE_FUNCTION),val_atom(v),returnType(nullptr),returnTypeAllArgsInt(nullptr) {}
	method_info* getMethodInfo() const override { return nullptr; }
	IFunction* clone(ASWorker* wrk) override;
	bool destruct() override
	{
		returnType=nullptr;
		returnTypeAllArgsInt=nullptr;
		return IFunction::destruct();
	}
	void finalize() override
	{
		returnType=nullptr;
		returnTypeAllArgsInt=nullptr;
		IFunction::finalize();
	}
	void prepareShutdown() override;
public:
	/**
	 * This executes a C++ function.
	 * It consumes _no_ references of obj and args
	 */
	FORCE_INLINE void call(asAtom& ret, ASWorker* wrk,asAtom& obj, asAtom* args, uint32_t num_args)
	{
		/*
		 * We do not enforce ABCVm::limits.max_recursion here.
		 * This should be okey, because there is no infinite recursion
		 * using only builtin functions.
		 * Additionally, we still need to run builtin code (such as the ASError constructor) when
		 * ABCVm::limits.max_recursion is reached in SyntheticFunction::call.
		 */
		val_atom(ret,wrk,obj,args,num_args);
	}
	bool isEqual(ASObject* r) override;
	FORCE_INLINE multiname* callGetter(asAtom& ret, ASObject* target,ASWorker* wrk) override
	{
		asAtom c = asAtomHandler::fromObject(target);
		val_atom(ret,wrk,c,nullptr,0);
		return nullptr;
	}
	Class_base* getReturnType(bool opportunistic=false) override;
	Class_base* getArgumentDependentReturnType(bool allargsint);
};

/* Special object used as prototype for the Function class
 * It keeps a link to the upper level in the prototype chain
 */
class FunctionPrototype: public Function, public Prototype
{
public:
	FunctionPrototype(ASWorker* wrk,Class_base* c, _NR<Prototype> p);
	inline bool destruct() override
	{
		reset();
		return Function::destruct();
	}
	void finalize() override
	{
		reset();
		Function::finalize();
	}
	void prepareShutdown() override;
	
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;
	Prototype* clonePrototype(ASWorker* wrk) override;
};

/*
 * Represents a function implemented in AS3-code
 */
class SyntheticFunction : public IFunction
{
friend class ABCVm;
friend class ABCContext;
friend class Class<IFunction>;
friend class Class_base;
private:
	/* Data structure with information directly loaded from the SWF */
	method_info* mi;
	/* Pointer to JIT-compiled function or NULL if not yet compiled */
	synt_function val;
	/* Pointer to multiname, if this function is a simple getter or setter */
	multiname* simpleGetterOrSetterName;
	bool fromNewFunction;
	SyntheticFunction(ASWorker* wrk,Class_base* c,method_info* m);
protected:
	IFunction* clone(ASWorker* wrk) override;
public:
	~SyntheticFunction() {}
	void call(ASWorker* wrk, asAtom &ret, asAtom& obj, asAtom *args, uint32_t num_args, bool coerceresult, bool coercearguments);
	bool destruct() override;
	void finalize() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	method_info* getMethodInfo() const override { return mi; }
	
	_NR<scope_entry_list> func_scope;
	bool isEqual(ASObject* r) override;
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
	FORCE_INLINE multiname* callGetter(asAtom& ret, ASObject* target,ASWorker* wrk) override
	{
		if (simpleGetterOrSetterName)
		{
			target->getVariableByMultiname(ret,*simpleGetterOrSetterName,GET_VARIABLE_OPTION::NONE,wrk);
			return simpleGetterOrSetterName;
		}
		asAtom c = asAtomHandler::fromObject(target);
		call(wrk,ret,c,nullptr,0,true,false);
		return nullptr;
	}
	FORCE_INLINE multiname* getSimpleName() {
		return simpleGetterOrSetterName;
	}
	Class_base* getReturnType(bool opportunistic=false) override;
	void checkParamTypes(bool opportunistic=false);
	bool canSkipCoercion(int param, Class_base* cls);
	inline bool isFromNewFunction() { return fromNewFunction; }
};

class AVM1Function : public IFunction
{
	friend class Class<IFunction>;
	friend class Class_base;
protected:
	DisplayObject* clip;
	Activation_object* activationobject;
	AVM1context context;
	asAtom superobj;
	std::vector<uint8_t> actionlist;
	std::vector<uint32_t> paramnames;
	std::vector<uint8_t> paramregisternumbers;
	std::map<uint32_t, asAtom> scopevariables;
	bool preloadParent;
	bool preloadRoot;
	bool suppressSuper;
	bool preloadSuper;
	bool suppressArguments;
	bool preloadArguments;
	bool suppressThis;
	bool preloadThis;
	bool preloadGlobal;
	AVM1Function(ASWorker* wrk,Class_base* c,DisplayObject* cl,Activation_object* act,AVM1context* ctx, std::vector<uint32_t>& p, std::vector<uint8_t>& a,std::vector<uint8_t> _registernumbers=std::vector<uint8_t>(), bool _preloadParent=false, bool _preloadRoot=false, bool _suppressSuper=false, bool _preloadSuper=false, bool _suppressArguments=false, bool _preloadArguments=false,bool _suppressThis=false, bool _preloadThis=false, bool _preloadGlobal=false);
	~AVM1Function();
	method_info* getMethodInfo() const override { return nullptr; }
	IFunction* clone(ASWorker* wrk) override
	{
		// no cloning needed in AVM1
		return nullptr;
	}
	asAtom computeSuper();
public:
	void finalize() override;
	bool destruct() override;
	void prepareShutdown() override;
	bool countCylicMemberReferences(garbagecollectorstate& gcstate) override;
	FORCE_INLINE void call(asAtom* ret, asAtom* obj, asAtom *args, uint32_t num_args, AVM1Function* caller=nullptr, std::map<uint32_t,asAtom>* locals=nullptr)
	{
		if (locals)
			this->setscopevariables(*locals);
		if (needsSuper())
		{
			asAtom newsuper = computeSuper();
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,this->scopevariables,false,ret,obj, args, num_args, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,caller,this,activationobject,&newsuper);
		}
		else
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,this->scopevariables,false,ret,obj, args, num_args, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,caller,this,activationobject);
	}
	FORCE_INLINE multiname* callGetter(asAtom& ret, ASObject* target, ASWorker* wrk) override
	{
		asAtom obj = asAtomHandler::fromObject(target);
		if (needsSuper())
		{
			asAtom newsuper = computeSuper();
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,this->scopevariables,false,&ret,&obj, nullptr, 0, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,nullptr,this,activationobject,&newsuper);
		}
		else
			ACTIONRECORD::executeActions(clip,&context,this->actionlist,0,this->scopevariables,false,&ret,&obj, nullptr, 0, paramnames,paramregisternumbers, preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal,nullptr,this,activationobject);
		return nullptr;
	}
	FORCE_INLINE Class_base* getReturnType(bool opportunistic=false) override
	{
		return nullptr;
	}
	FORCE_INLINE Activation_object* getActivationObject() const
	{
		return activationobject;
	}
	FORCE_INLINE void setSuper(asAtom s)
	{
		superobj.uintval = s.uintval;
	}
	FORCE_INLINE DisplayObject* getClip() const
	{
		return clip;
	}
	FORCE_INLINE bool needsSuper() const
	{
		return preloadSuper && !suppressSuper;
	}
	FORCE_INLINE asAtom getSuper() const
	{
		return superobj;
	}
	void filllocals(std::map<uint32_t,asAtom>& locals);
	void setscopevariables(std::map<uint32_t,asAtom>& locals);
};

/*
 * The Class of a Function
 */
template<>
class Class<IFunction>: public Class_base
{
private:
	Class<IFunction>(MemoryAccount* m, uint32_t classID):Class_base(QName(BUILTIN_STRINGS::STRING_FUNCTION,BUILTIN_STRINGS::EMPTY),classID ,m){}
	void getInstance(ASWorker* worker,asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass) override;
	IFunction* getNopFunction();
public:
	static Class<IFunction>* getClass(SystemState* sys);
	static _R<Class<IFunction>> getRef(SystemState* sys)
	{
		Class<IFunction>* ret = getClass(sys);
		ret->incRef();
		return _MR(ret);
	}
	static Function* getFunction(SystemState* sys,Function::as_atom_function v, int len = 0, Class_base* returnType=nullptr, Class_base* returnTypeAllArgsInt=nullptr)
	{
		Class<IFunction>* c=Class<IFunction>::getClass(sys);
		Function*  ret = new (c->memoryAccount) Function(c->getInstanceWorker(),c);
		ret->objfreelist = &c->getInstanceWorker()->freelist[c->classID];
		ret->setRefConstant();
		ret->resetCached();
		ret->val_atom = v;
		ret->returnType = returnType;
		ret->returnTypeAllArgsInt = returnTypeAllArgsInt;
		ret->length = len;
		ret->constructIndicator = true;
		ret->constructorCallComplete = true;
		return ret;
	}

	static SyntheticFunction* getSyntheticFunction(ASWorker* wrk,method_info* m, uint32_t _length)
	{
		Class<IFunction>* c=Class<IFunction>::getClass(wrk->getSystemState());
		SyntheticFunction* ret = nullptr;
		ret= wrk->freelist_syntheticfunction.getObjectFromFreeList()->as<SyntheticFunction>();
		if (!ret)
		{
			ret=new (c->memoryAccount) SyntheticFunction(wrk,c, m);
		}
		else
		{
			ret->resetCached();
			ret->mi = m;
			ret->length = _length;
			ret->objfreelist = &wrk->freelist_syntheticfunction;
		}

		ret->constructIndicator = true;
		ret->constructorCallComplete = true;
		asAtom obj = asAtomHandler::fromObject(ret);
		c->handleConstruction(obj,nullptr,0,true);
		return ret;
	}
	static AVM1Function* getAVM1Function(ASWorker* wrk,DisplayObject* clip,Activation_object* act, AVM1context* ctx,std::vector<uint32_t>& params, std::vector<uint8_t>& actions, std::vector<uint8_t> paramregisternumbers=std::vector<uint8_t>(), bool preloadParent=false, bool preloadRoot=false, bool suppressSuper=true, bool preloadSuper=false, bool suppressArguments=false, bool preloadArguments=false, bool suppressThis=true, bool preloadThis=false, bool preloadGlobal=false)
	{
		Class<IFunction>* c=Class<IFunction>::getClass(wrk->getSystemState());
		AVM1Function*  ret =new (c->memoryAccount) AVM1Function(wrk,c, clip, act,ctx, params,actions,paramregisternumbers,preloadParent,preloadRoot,suppressSuper,preloadSuper,suppressArguments,preloadArguments,suppressThis,preloadThis,preloadGlobal);
		ret->objfreelist = nullptr;
		ret->constructIndicator = true;
		ret->constructorCallComplete = true;
		return ret;
	}
	void buildInstanceTraits(ASObject* o) const override
	{
	}
	void generator(ASWorker* wrk, asAtom& ret, asAtom* args, const unsigned int argslen) override;
};

class Undefined : public ASObject
{
public:
	ASFUNCTION_ATOM(call);
	Undefined();
	int32_t toInt() override;
	int64_t toInt64() override;
	bool isEqual(ASObject* r) override;
	TRISTATE isLess(ASObject* r) override;
	TRISTATE isLessAtom(asAtom& r) override;
	ASObject *describeType(ASWorker* wrk) const override;
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;
};

class Null: public ASObject
{
public:
	Null();
	bool isEqual(ASObject* r) override;
	TRISTATE isLess(ASObject* r) override;
	TRISTATE isLessAtom(asAtom& r) override;
	int32_t toInt() override;
	int64_t toInt64() override;
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	int32_t getVariableByMultiname_i(const multiname& name, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname& name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;

	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap, ASWorker* wrk) override;
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
	ASQName(ASWorker* wrk,Class_base* c);
	void setByXML(XML* node);
	static void sinit(Class_base*);
	ASFUNCTION_ATOM(_constructor);
	ASFUNCTION_ATOM(generator);
	ASFUNCTION_ATOM(_getURI);
	ASFUNCTION_ATOM(_getLocalName);
	ASFUNCTION_ATOM(_toString);
	uint32_t getURI() const { return uri; }
	uint32_t getLocalName() const { return local_name; }
	bool isEqual(ASObject* o) override;

	tiny_string toString();

	uint32_t nextNameIndex(uint32_t cur_index) override;
	void nextName(asAtom &ret, uint32_t index) override;
	void nextValue(asAtom &ret, uint32_t index) override;
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
	Namespace(ASWorker* wrk,Class_base* c);
	Namespace(ASWorker* wrk, Class_base* c, uint32_t _uri, uint32_t _prefix=BUILTIN_STRINGS::EMPTY, NS_KIND _nskind = NAMESPACE);
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
	bool isEqual(ASObject* o) override;
	uint32_t getURI() const { return uri; }
	uint32_t getPrefix(bool& is_undefined) { is_undefined=prefix_is_undefined; return prefix; }

	uint32_t nextNameIndex(uint32_t cur_index) override;
	void nextName(asAtom &ret, uint32_t index) override;
	void nextValue(asAtom &ret, uint32_t index) override;
};


class Global : public ASObject
{
private:
	int scriptId;
	ABCContext* context;
	bool isavm1;
public:
	Global(ASWorker* wrk,Class_base* cb, ABCContext* c, int s, bool avm1);
	static void sinit(Class_base* c);
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname &name, asAtom &o, CONST_ALLOWED_FLAG allowConst, bool *alreadyset, ASWorker* wrk) override;
	void getVariableByMultinameOpportunistic(asAtom& ret, const multiname& name, ASWorker* wrk);
	/*
	 * Utility method to register builtin methods and classes
	 */
	void registerBuiltin(const char* name, const char* ns, _R<ASObject> o, NS_KIND nskind=NAMESPACE);
	// ensures that the init script has been run
	void checkScriptInit();
	bool isAVM1() const { return isavm1; }
	ABCContext* getContext() const { return context; }
};

void eval(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void parseInt(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void parseFloat(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void isNaN(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void isFinite(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void encodeURI(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void decodeURI(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void encodeURIComponent(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void decodeURIComponent(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void escape(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void unescape(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void print(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void trace(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void AVM1_ASSetPropFlags(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
void AVM1_updateAfterEvent(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
bool isXMLName(ASWorker* wrk, asAtom &obj);
void _isXMLName(asAtom& ret,ASWorker* wrk, asAtom& obj,asAtom* args, const unsigned int argslen);
number_t parseNumber(const tiny_string str, bool useoldversion=false);
}

#endif /* SCRIPTING_TOPLEVEL_TOPLEVEL_H */
