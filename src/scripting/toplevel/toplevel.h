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
#include "scripting/toplevel/Class_base.h"
#include "scripting/toplevel/IFunction.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Array.h"
#include "scripting/flash/system/flashsystem.h"
#include "memory_support.h"

namespace pugi
{
	class xml_node;
}
namespace lightspark
{

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

#ifdef PROFILING_SUPPORT
extern void addFunctionCall(Class_base* cls, uint32_t functionname, uint64_t duration, bool builtin);
#endif
/*
 * Implements the IFunction interface for functions implemented
 * in c-code.
 */
class FunctionPrototype;

class Function : public IFunction
{
friend class Class<IFunction>;
friend class Class_base;
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
#ifdef PROFILING_SUPPORT
		uint64_t t1 = compat_get_thread_cputime_us();
#endif
		/*
		 * We do not enforce ABCVm::limits.max_recursion here.
		 * This should be okey, because there is no infinite recursion
		 * using only builtin functions.
		 * Additionally, we still need to run builtin code (such as the ASError constructor) when
		 * ABCVm::limits.max_recursion is reached in SyntheticFunction::call.
		 */
		val_atom(ret,wrk,obj,args,num_args);
#ifdef PROFILING_SUPPORT
		uint64_t t2 = compat_get_thread_cputime_us();
		if (this->inClass)
			addFunctionCall(this->inClass,functionname,t2-t1,true);
#endif
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

/*
 * The Class of a Function
 */
template<>
class Class<IFunction>: public Class_base
{
private:
	Class(MemoryAccount* m, uint32_t classID):Class_base(QName(BUILTIN_STRINGS::STRING_FUNCTION,BUILTIN_STRINGS::EMPTY),classID ,m){}
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
	static Function* getFunction(SystemState* sys,as_atom_function v, int len = 0, Class_base* returnType=nullptr, Class_base* returnTypeAllArgsInt=nullptr)
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
