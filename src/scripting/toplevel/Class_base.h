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

#ifndef SCRIPTING_TOPLEVEL_CLASS_BASE_H
#define SCRIPTING_TOPLEVEL_CLASS_BASE_H 1

#include "compat.h"
#include "asobject.h"
#include "scripting/flash/system/flashsystem.h"


namespace pugi
{
class xml_node;
}
namespace lightspark
{
const tiny_string AS3="http://adobe.com/AS3/2006/builtin";

class IFunction;
class Prototype;
class ObjectConstructor;
struct traits_info;
class Any;
class Void;
class Class_object;

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
	static Any* anyType;
	static Void* voidType;
	/*
	 * This returns the Type for the given multiname.
	 * It searches for the and object of the type in global object.
	 * If no object of that name is found or it is not a Type,
	 * then an exception is thrown.
	 * The caller does not own the object returned.
	 */
	static Type* getTypeFromMultiname(multiname* mn, ABCContext* context, bool opportunistic=false);
	/*
	 * Checks if the type is already in sys->classes
	 */
	static Type *getBuiltinType(ASWorker* wrk, multiname* mn);
	/*
	 * Converts the given object to an object of this type.
	 * If the argument cannot be converted, it throws a TypeError
	 * returns true if the atom is really converted into another instance
	 */
	virtual bool coerce(ASWorker* wrk, asAtom& o)=0;

	virtual void coerceForTemplate(ASWorker* wrk, asAtom& o)=0;
	
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

class Any: public Type
{
public:
	bool coerce(ASWorker* wrk,asAtom& o) override { return false; }
	void coerceForTemplate(ASWorker* wrk, asAtom& o) override {}
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
	bool coerce(ASWorker* wrk,asAtom& o) override { return false; }
	void coerceForTemplate(ASWorker* wrk, asAtom& o) override { }
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
	bool coerce(ASWorker* wrk,asAtom& o) override { throw RunTimeException("Coercing to an ActivationType should not happen");}
	void coerceForTemplate(ASWorker* wrk,asAtom& o) override { throw RunTimeException("Coercing to an ActivationType should not happen");}
	virtual ~ActivationType() {}
	tiny_string getName() const override { return "activation"; }
	EARLY_BIND_STATUS resolveMultinameStatically(const multiname& name) const override;
	const multiname* resolveSlotTypeName(uint32_t slotId) const override;
	bool isBuiltin() const override { return true; }
	Global* getGlobalScope() const override { return nullptr; }
};

class Class_base: public ASObject, public Type
{
friend class ABCVm;
friend class ABCContext;
template<class T> friend class Template;
private:
	mutable std::vector<multiname> interfaces;
	mutable std::vector<Class_base*> interfaces_added;
	std::unordered_set<uint32_t> overriddenmethods;
	std::unordered_map<Class_base*,bool> subclasses_map;
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
	ASFUNCTION_ATOM(_toString);
	void initStandardProps();
public:
	void copyBorrowedTraits(Class_base* src);
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
	void handleConstruction(asAtom &target, asAtom *args, unsigned int argslen, bool buildAndLink, bool _explicit = false);
	void setConstructor(ASObject* c);
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
	bool isSubClass(Class_base* cls, bool considerInterfaces=true);
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
	bool coerce(ASWorker* wrk, asAtom& o) override;
	
	void coerceForTemplate(ASWorker* wrk, asAtom& o) override;

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
	virtual Class_base* applyType(const std::vector<Type*>& t,_NR<ApplicationDomain> appdomain)=0;
	QName getTemplateName() { return template_name; }
	ASPROPERTY_GETTER(_NR<Prototype>,prototype);
	void addPrototypeGetter(SystemState *sys);


	bool coerce(ASWorker* wrk, asAtom& o) override { return false;}
	void coerceForTemplate(ASWorker* wrk, asAtom& o) override {}
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
	void setDeclaredMethodByQName(const tiny_string& name, const tiny_string& ns, ASObject* o, METHOD_TYPE type, bool isBorrowed, bool isEnumerable= true);
	
	virtual Prototype* clonePrototype(ASWorker* wrk) = 0;
};

}

#endif /* SCRIPTING_TOPLEVEL_CLASS_BASE_H */
