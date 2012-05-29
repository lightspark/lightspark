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

#ifndef TOPLEVEL_H
#define TOPLEVEL_H
#include "compat.h"
#include <vector>
#include <set>
#include "asobject.h"
#include "exceptions.h"
#include "threading.h"
#include "scripting/abcutils.h"
#include "Boolean.h"
#include "Error.h"
#include "XML.h"
#include "memory_support.h"

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
class Any;
class Void;
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
	static const Type* getTypeFromMultiname(const multiname* mn, const ABCContext* context);
	/*
	 * Checks if the type is already in sys->classes
	 */
	static bool isTypeResolvable(const multiname* mn);
	/*
	 * Converts the given object to an object of this type.
	 * It consumes one reference of 'o'.
	 * The returned object must be decRef'ed by caller.
	 * If the argument cannot be converted, it throws a TypeError
	 */
	virtual ASObject* coerce(ASObject* o) const=0;

	/* Return "any" for anyType, "void" for voidType and class_name.name for Class_base */
	virtual tiny_string getName() const=0;
};
template<> inline Type* ASObject::as<Type>() { return dynamic_cast<Type*>(this); }
template<> inline const Type* ASObject::as<Type>() const { return dynamic_cast<const Type*>(this); }

class Any: public Type
{
public:
	ASObject* coerce(ASObject* o) const { return o; }
	virtual ~Any() {};
	tiny_string getName() const { return "any"; }
};

class Void: public Type
{
public:
	ASObject* coerce(ASObject* o) const;
	virtual ~Void() {};
	tiny_string getName() const { return "void"; }
};

class Class_base: public ASObject, public Type
{
friend class ABCVm;
friend class ABCContext;
template<class T> friend class Template;
private:
	mutable std::vector<multiname> interfaces;
	mutable std::vector<Class_base*> interfaces_added;
	bool use_protected;
	nsNameAndKind protected_ns;
	void recursiveBuild(ASObject* target);
	IFunction* constructor;
	void describeTraits(xmlpp::Element* root, std::vector<traits_info>& traits) const;
	//Naive garbage collection until reference cycles are detected
	Mutex referencedObjectsMutex;
	std::set<ASObject*, std::less<ASObject*>, reporter_allocator<ASObject*>> referencedObjects;
	void finalizeObjects() const;
protected:
	void copyBorrowedTraitsFromSuper();
	ASFUNCTION(_toString);
public:
	bool isFinal:1;
	bool isSealed:1;
	void addPrototypeGetter();
	ASPROPERTY_GETTER(_NR<ASObject>,prototype);
	_NR<Class_base> super;
	//We need to know what is the context we are referring to
	ABCContext* context;
	const QName class_name;
	int class_index;
	//Memory reporter to keep track of used bytes
	MemoryAccount* memoryAccount;
	void handleConstruction(ASObject* target, ASObject* const* args, unsigned int argslen, bool buildAndLink);
	void setConstructor(IFunction* c);
	Class_base(const QName& name, MemoryAccount* m);
	~Class_base();
	void finalize();
	virtual ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass=NULL)=0;
	void addImplementedInterface(const multiname& i);
	void addImplementedInterface(Class_base* i);
	virtual void buildInstanceTraits(ASObject* o) const=0;
	const std::vector<Class_base*>& getInterfaces() const;
	void linkInterface(Class_base* c) const;
	/*
	 * Returns true when 'this' is a subclass of 'cls',
	 * i.e. this == cls or cls equals some super of this.
	 */
	bool isSubClass(const Class_base* cls) const;
	tiny_string getQualifiedClassName() const;
	tiny_string getName() const { return class_name.name; }
	tiny_string toString();
	virtual ASObject* generator(ASObject* const* args, const unsigned int argslen);
	ASObject *describeType() const;
	void describeInstance(xmlpp::Element* root) const;
	virtual const Template_base* getTemplate() const { return NULL; }
	//DEPRECATED: naive garbage collector
	void abandonObject(ASObject* ob) DLL_PUBLIC;
	void acquireObject(ASObject* ob) DLL_PUBLIC;
	/*
	 * Converts the given object to an object of this Class_base's type.
	 * It consumes one reference of 'o'.
	 * The returned object must be decRef'ed by caller.
	 */
	virtual ASObject* coerce(ASObject* o) const;

	void setSuper(_R<Class_base> super_)
	{
		assert(!super);
		super = super_;
		copyBorrowedTraitsFromSuper();
	}
};

class Template_base : public ASObject
{
private:
	QName template_name;
public:
	Template_base(QName name);
	virtual Class_base* applyType(const std::vector<Type*>& t)=0;
};

class Class_object: public Class_base
{
private:
	Class_object():Class_base(QName("Class",""),NULL){}
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass)
	{
		throw RunTimeException("Class_object::getInstance");
		return NULL;
	}
	void buildInstanceTraits(ASObject* o) const
	{
		throw RunTimeException("Class_object::buildInstanceTraits");
	}
public:
	static Class_object* getClass();
	static _R<Class_object> getRef();
};

/* Adaptor from fuction to class, it does not seems to be a good idea to
 * derive IFunction from Class_base, because it's too heavyweight
 * This is the class of an object created by (new f()) where f is a function.
 * Its prototype points to f->prototype which is defined in IFunction.
 *
 * Class_function uses prototype based inheritance only.
 */
class Class_function: public Class_base
{
private:
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass)
	{
		throw RunTimeException("Class_function::getInstance");
		return NULL;
	}
	void buildInstanceTraits(ASObject* o) const
	{
		throw RunTimeException("Class_function::buildInstanceTraits");
	}
public:
	//Name is 'Object' because trace(new f()) gives "[object Object]"
	Class_function() : Class_base(QName("Object",""),NULL) {}
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE)
	{
		return NullRef;
	}
	int32_t getVariableByMultiname_i(const multiname& name)
	{
		throw UnsupportedException("Class_function::getVariableByMultiname_i");
		return 0;
	}
	void setVariableByMultiname_i(const multiname& name, int32_t value)
	{
		throw UnsupportedException("Class_function::setVariableByMultiname_i");
	}
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
	{
		throw UnsupportedException("Class_function::setVariableByMultiname");
	}
	bool hasPropertyByMultiname(const multiname& name, bool considerDynamic)
	{
		return false;
	}
};

class IFunction: public ASObject
{
protected:
	IFunction(Class_base *c);
	virtual IFunction* clone()=0;
	_NR<ASObject> closure_this;
public:
	/* If this is a method, inClass is the class this is defined in.
	 * If this is a function, inClass == NULL
	 */
	Class_base* inClass;
	/* returns wether this is this a method of a function */
	bool isMethod() const { return inClass != NULL; }
	bool isBound() const { return closure_this; }
	void finalize();
	ASFUNCTION(apply);
	ASFUNCTION(_call);
	ASFUNCTION(_toString);
	ASPROPERTY_GETTER_SETTER(_NR<ASObject>,prototype);
	ASPROPERTY_GETTER_SETTER(uint32_t,length);
	/*
	 * Calls this function with the given object and args.
	 * One reference of obj and each args[i] is consumed.
	 * Return the ASObject the function returned.
	 * This never returns NULL.
	 */
	virtual ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args)=0;
	IFunction* bind(_NR<ASObject> c, int level)
	{
		if(!isBound())
		{
			IFunction* ret=NULL;
			if(!c)
			{
				//If binding with null we are generated from newFunction, don't copy
				ret=this;
			}
			else
			{
				//Generate a copy
				ret=clone();
				ret->setClass(getClass());
			}
			ret->closure_this=c;
			//std::cout << "Binding " << ret << std::endl;
			return ret;
		}
		else
		{
			incRef();
			return this;
		}
	}
	virtual method_info* getMethodInfo() const=0;
	virtual ASObject *describeType() const;
};

class Function : public IFunction
{
friend class Class<IFunction>;
public:
	typedef ASObject* (*as_function)(ASObject*, ASObject* const *, const unsigned int);
private:
	as_function val;
	Function(Class_base* c, as_function v=NULL):IFunction(c),val(v){}
	Function* clone()
	{
		return new (getClass()->memoryAccount) Function(*this);
	}
	method_info* getMethodInfo() const { return NULL; }
public:
	ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args);
	bool isEqual(ASObject* r)
	{
		Function* f=dynamic_cast<Function*>(r);
		if(f==NULL)
			return false;
		return val==f->val;
	}
};

class SyntheticFunction : public IFunction
{
friend class ABCVm;
friend class Class<IFunction>;
public:
	typedef ASObject* (*synt_function)(call_context* cc);
private:
	int hit_count;
	method_info* mi;
	synt_function val;
	SyntheticFunction(Class_base* c,method_info* m);
	SyntheticFunction* clone()
	{
		return new (getClass()->memoryAccount) SyntheticFunction(*this);
	}
	method_info* getMethodInfo() const { return mi; }
public:
	~SyntheticFunction();
	ASObject* call(ASObject* obj, ASObject* const* args, uint32_t num_args);
	void finalize();
	std::vector<scope_entry> func_scope;
	bool isEqual(ASObject* r)
	{
		SyntheticFunction* sf=dynamic_cast<SyntheticFunction*>(r);
		if(sf==NULL)
			return false;
		return mi==sf->mi;
	}
	void acquireScope(const std::vector<scope_entry>& scope)
	{
		assert_and_throw(func_scope.empty());
		func_scope=scope;
	}
	void addToScope(const scope_entry& s)
	{
		func_scope.emplace_back(s);
	}
};

//Specialized class for IFunction
template<>
class Class<IFunction>: public Class_base
{
private:
	Class<IFunction>(MemoryAccount* m):Class_base(QName("Function",""),m){}
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass);
public:
	static Class<IFunction>* getClass();
	static _R<Class<IFunction>> getRef()
	{
		Class<IFunction>* ret = getClass();
		ret->incRef();
		return _MR(ret);
	}
	static Function* getFunction(Function::as_function v)
	{
		Class<IFunction>* c=Class<IFunction>::getClass();
		Function* ret=new (c->memoryAccount) Function(c, v);
		return ret;
	}
	static Function* getFunction(Function::as_function v, int len)
	{
		Class<IFunction>* c=Class<IFunction>::getClass();
		Function* ret=new (c->memoryAccount) Function(c, v);
		ret->length = len;
		return ret;
	}
	static SyntheticFunction* getSyntheticFunction(method_info* m)
	{
		Class<IFunction>* c=Class<IFunction>::getClass();
		SyntheticFunction* ret=new (c->memoryAccount) SyntheticFunction(c, m);
		c->handleConstruction(ret,NULL,0,true);
		return ret;
	}
	void buildInstanceTraits(ASObject* o) const
	{
	}
};

class Undefined : public ASObject
{
public:
	ASFUNCTION(call);
	Undefined();
	int32_t toInt();
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	ASObject *describeType() const;
	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst);
};

class Null: public ASObject
{
public:
	Null();
	bool isEqual(ASObject* r);
	TRISTATE isLess(ASObject* r);
	int32_t toInt();
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt);
	int32_t getVariableByMultiname_i(const multiname& name);
	void setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst);

	//Serialization interface
	void serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap);
};

class ASQName: public ASObject
{
friend class multiname;
friend class Namespace;
private:
	bool uri_is_null;
	tiny_string uri;
	tiny_string local_name;
public:
	ASQName(Class_base* c);
	void setByNode(xmlpp::Node* node);
	static void sinit(Class_base*);
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	ASFUNCTION(_getURI);
	ASFUNCTION(_getLocalName);
	ASFUNCTION(_toString);
	tiny_string getURI() const { return uri; }
	tiny_string getLocalName() const { return local_name; }
	bool isEqual(ASObject* o);
	tiny_string toString();
};

class Namespace: public ASObject
{
friend class ASQName;
friend class ABCContext;
private:
	bool prefix_is_undefined;
	tiny_string uri;
	tiny_string prefix;
public:
	Namespace(Class_base* c);
	Namespace(Class_base* c, const tiny_string& _uri);
	static void sinit(Class_base*);
	static void buildTraits(ASObject* o);
	ASFUNCTION(_constructor);
	ASFUNCTION(generator);
	ASFUNCTION(_getURI);
	ASFUNCTION(_setURI);
	ASFUNCTION(_getPrefix);
	ASFUNCTION(_setPrefix);
	ASFUNCTION(_toString);
	ASFUNCTION(_valueOf);
	ASFUNCTION(_ECMA_valueOf);
	bool isEqual(ASObject* o);
};


class Global : public ASObject
{
private:
	ABCContext* context;
	int scriptId;
public:
	Global(Class_base* cb, ABCContext* c, int s);
	static void sinit(Class_base* c);
	static void buildTraits(ASObject* o) {};
	_NR<ASObject> getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt=NONE);
	/*
	 * Utility method to register builtin methods and classes
	 */
	void registerBuiltin(const char* name, const char* ns, _R<ASObject> o);
};

ASObject* eval(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* parseInt(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* parseFloat(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* isNaN(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* isFinite(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* encodeURI(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* decodeURI(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* encodeURIComponent(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* decodeURIComponent(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* escape(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* unescape(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* print(ASObject* obj,ASObject* const* args, const unsigned int argslen);
ASObject* trace(ASObject* obj,ASObject* const* args, const unsigned int argslen);
bool isXMLName(ASObject* obj);
ASObject* _isXMLName(ASObject* obj,ASObject* const* args, const unsigned int argslen);


inline void Manager::put(ASObject* o)
{
	if(available.size()>=maxCache)
		delete o;
	else
	{
		//The Manager now owns this object
		if(o->classdef)
			o->classdef->abandonObject(o);
		available.push_back(o);
	}
}

template<class T>
T* Manager::get()
{
	if(available.size())
	{
		T* ret=static_cast<T*>(available.back());
		available.pop_back();
		ret->incRef();
		//Transfer ownership back to the classdef
		if(ret->getClass())
			ret->getClass()->acquireObject(ret);
		//std::cout << "getting[" << name << "] " << ret << std::endl;
		return ret;
	}
	else
	{
		T* ret=Class<T>::getInstanceS();
		ret->manager = this;
		//std::cout << "newing" << ret << std::endl;
		return ret;
	}
}

inline Manager::~Manager()
{
	for(auto i = available.begin(); i != available.end(); ++i)
	{
		// ~ASObject will call abandonObject() again
		if((*i)->classdef)
			(*i)->classdef->acquireObject(*i);
		delete *i;
	}
}

};

#endif
