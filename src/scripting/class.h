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

#ifndef SCRIPTING_CLASS_H
#define SCRIPTING_CLASS_H 1

#include "compat.h"
#include "asobject.h"
#include "swf.h"
#include "scripting/toplevel/Class_base.h"
#include "scripting/flash/system/flashsystem.h"
#include "backends/streamcache.h"


namespace lightspark
{
class Number;
class Integer;
class UInteger;

template<typename T>
class ClassName
{
public:
	static const char* ns;
	static const char* name;
	static uint32_t id;
};

class Class_inherit:public Class_base
{
private:
	void getInstance(ASWorker* worker,asAtom& ret, bool construct, asAtom *args, const unsigned int argslen, Class_base* realClass) override;
	DictionaryTag* tag;
	bool bindedToRoot;
	bool bindingchecked;
	bool inScriptInit;
	void recursiveBuild(ASObject* target) const;
	const traits_info* classtrait;
	_NR<ASObject> instancefactory;
	asfreelist freelist;
public:
	Class_inherit(const QName& name, MemoryAccount* m,const traits_info* _classtrait, Global* _global);
	bool checkScriptInit();
	bool destruct() override
	{
		instancefactory.reset();
		bindingchecked=false;
		bindedToRoot=false;
		auto it = class_scope.begin();
		while (it != class_scope.end())
		{
			ASATOM_DECREF((*it).object);
			it++;
		}
		class_scope.clear();
		return Class_base::destruct();
	}
	void finalize() override;
	void prepareShutdown() override;
	asfreelist* getFreeList(ASWorker*) override
	{
		return &freelist;
	}
	
	void buildInstanceTraits(ASObject* o) const override;
	void setupDeclaredTraits(ASObject *target, bool checkclone=true) override;
	void bindToTag(DictionaryTag* t)
	{
		tag=t;
	}
	void bindToRoot()
	{
		bindedToRoot=true;
	}
	bool isBinded() const
	{
		return tag || bindedToRoot;
	}
	bool needsBindingCheck() const
	{
		return !bindingchecked;
	}
	//Closure stack
	std::vector<scope_entry> class_scope;
	void describeClassMetadata(pugi::xml_node &root) const override;
	bool isBuiltin() const override { return false; }
	bool hasoverriddenmethod(multiname* name) const override;
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt, ASWorker* wrk) override;
	variable* findVariableByMultiname(const multiname& name, Class_base* cls, uint32_t* nsRealID, bool* isborrowed, bool considerdynamic, ASWorker* wrk) override;
	multiname* setVariableByMultiname(multiname& name, asAtom& o, CONST_ALLOWED_FLAG allowConst, bool* alreadyset, ASWorker* wrk) override;
	std::string toDebugString() const override;
};

/* helper function: does Class<ASObject>::getInstances(), but solves forward declaration problem */
ASObject* new_asobject(ASWorker* wrk);
Prototype* new_objectPrototype(ASWorker* wrk);
Prototype* new_functionPrototype(ASWorker* wrk, Class_base* functionClass, _NR<Prototype> p);
Function_object* new_functionObject(_NR<ASObject> p);
ObjectConstructor* new_objectConstructor(Class_base* cls,uint32_t length);
Activation_object* new_activationObject(ASWorker* wrk);

template<class T,std::size_t N>
struct newWithOptionalClass
{
	template<class F, typename... Args>
	static T* doNew(ASWorker* wrk,Class_base* c, const F& f, Args&&... args)
	{
		return newWithOptionalClass<T, N-1>::doNew(wrk,c, std::forward<Args>(args)..., f);
	}
};
template<class T>
struct newWithOptionalClass<T, 1>
{
	template<class F, typename... Args>
	static T* doNew(ASWorker* wrk,Class_base* c, const F& f, Args&&... args)
	{
		//Last parameter is not a class pointer
		return new (c->memoryAccount) T(wrk,c, std::forward<Args>(args)..., f);
	}
	template<typename... Args>
	static T* doNew(ASWorker* wrk, Class_base* c, Class_base* f, Args&&... args)
	{
		//Last parameter is a class pointer
		return new (f->memoryAccount) T(wrk,f, std::forward<Args>(args)...);
	}
};

template<class T>
struct newWithOptionalClass<T, 0>
{
	static T* doNew(ASWorker* wrk, Class_base* c)
	{
		//Last parameter is not a class pointer
		return new (c->memoryAccount) T(wrk,c);
	}
};

template< class T>
class Class: public Class_base
{
protected:
	Class(const QName& name,uint32_t classID, MemoryAccount* m):Class_base(name, classID,m){}
	//This function is instantiated always because of inheritance
	void getInstance(ASWorker* worker,asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass=nullptr) override
	{
		if(realClass==nullptr)
			realClass=this;
		ASObject* o = worker->freelist[ClassName<T>::id].getObjectFromFreeList();
		if (!o)
			o=(new (realClass->memoryAccount) T(worker,realClass));
		else
			o->setClass(realClass);
		ret = asAtomHandler::fromObject(o);
		asAtomHandler::resetCached(ret);
		if(construct)
			handleConstruction(ret,args,argslen,true,worker->isExplicitlyConstructed());
	}
public:
	template<typename... Args>
	static T* getInstanceS(ASWorker* wrk, Args&&... args)
	{
		Class<T>* c=static_cast<Class<T>*>(wrk->getSystemState()->builtinClasses[ClassName<T>::id]);
		if (!c)
			c = getClass(wrk->getSystemState());
		T* ret=newWithOptionalClass<T, sizeof...(Args)>::doNew(wrk,c, std::forward<Args>(args)...);
		c->setupDeclaredTraits(ret);
		ret->constructionComplete();
		ret->setConstructIndicator();
		return ret;
	}
	// constructor without arguments
	inline static T* getInstanceSNoArgs(ASWorker* wrk)
	{
		Class<T>* c=static_cast<Class<T>*>(wrk->getSystemState()->builtinClasses[ClassName<T>::id]);
		if (!c)
			c = getClass(wrk->getSystemState());
		T* ret = wrk->freelist[ClassName<T>::id].getObjectFromFreeList()->template as<T>();
		if (!ret)
		{
			ret=new (c->memoryAccount) T(wrk,c);
			assert_and_throw(ret);
		}
		ret->resetCached();
		ret->setIsInitialized();
		ret->constructionComplete();
		ret->setConstructIndicator();
		return ret;
	}

	inline static Class<T>* getClass(SystemState* sys)
	{
		uint32_t classId=ClassName<T>::id;
		Class<T>* ret=nullptr;
		Class_base** retAddr= &sys->builtinClasses[classId];
		if(*retAddr==nullptr)
		{
			//Create the class
			QName name(sys->getUniqueStringId(ClassName<T>::name),sys->getUniqueStringId(ClassName<T>::ns));
			MemoryAccount* m = sys->allocateMemoryAccount(ClassName<T>::name);
			ret=new (m) Class<T>(name, ClassName<T>::id, m);
			ret->setWorker(sys->worker);
			ret->setSystemState(sys);
			ret->incRef();
			*retAddr=ret;
			ret->prototype = _MNR(new_objectPrototype(sys->worker));
			T::sinit(ret);

			ret->initStandardProps();
		}
		else
			ret=static_cast<Class<T>*>(*retAddr);

		return ret;
	}
	static _R<Class<T>> getRef(SystemState* sys)
	{
		Class<T>* ret = getClass(sys);
		ret->incRef();
		return _MR(ret);
	}
	static T* cast(ASObject* o)
	{
		return static_cast<T*>(o);
	}
	static T* dyncast(ASObject* o)
	{
		return dynamic_cast<T*>(o);
	}
	void buildInstanceTraits(ASObject* o) const override
	{
		T::buildTraits(o);
	}
	void generator(ASWorker* wrk,asAtom& ret, asAtom* args, const unsigned int argslen) override
	{
		T::generator(ret,wrk, asAtomHandler::invalidAtom, args, argslen);
	}
	bool coerce(ASWorker* wrk,asAtom& o) override
	{
		return Class_base::coerce(wrk,o);
	}
};

template<>
void Class<Global>::getInstance(ASWorker* worker,asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass);

template<>
inline bool Class<Number>::coerce(ASWorker* wrk,asAtom& o)
{
	if (asAtomHandler::isNumeric(o))
		return false;
	number_t n = asAtomHandler::toNumber(o);
	asAtomHandler::setNumber(o,wrk,n);
	return true;
}

template<>
inline bool Class<UInteger>::coerce(ASWorker* wrk,asAtom& o)
{
	if (asAtomHandler::isUInteger(o))
		return false;
	uint32_t n = asAtomHandler::toUInt(o);
	asAtomHandler::setUInt(o,wrk,n);
	return true;
}

template<>
inline bool Class<Integer>::coerce(ASWorker* wrk,asAtom& o)
{
	if (asAtomHandler::isInteger(o))
		return false;
	int32_t n = asAtomHandler::toInt(o);
	asAtomHandler::setInt(o,wrk,n);
	return true;
}

template<>
inline bool Class<Boolean>::coerce(ASWorker* wrk,asAtom& o)
{
	if (asAtomHandler::isBool(o))
		return false;
	bool n = asAtomHandler::Boolean_concrete(o);
	asAtomHandler::setBool(o,n);
	return true;
}
template<>
inline void Class<Boolean>::getInstance(ASWorker* worker,asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass)
{
	if (argslen> 0)
		ret = asAtomHandler::fromObject(abstract_b(worker->getSystemState(),asAtomHandler::Boolean_concrete(args[0])));
	else
		ret = asAtomHandler::fromObject(abstract_b(worker->getSystemState(),false));
}

template<>
class Class<ASObject>: public Class_base
{
private:
	Class(const QName& name,uint32_t classID, MemoryAccount* m):Class_base(name, classID,m){}
	//This function is instantiated always because of inheritance
	void getInstance(ASWorker* worker,asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass=nullptr);
public:
	static ASObject* getInstanceS(ASWorker* wrk)
	{
		Class<ASObject>* c=Class<ASObject>::getClass(wrk->getSystemState());
		ASObject* ret = wrk->freelist[c->classID].getObjectFromFreeList();
		if (!ret)
			ret=new (c->memoryAccount) ASObject(wrk,c);
		ret->resetCached();
		c->setupDeclaredTraits(ret);
		ret->constructionComplete();
		ret->setConstructIndicator();
		ret->setConstructorCallComplete();
		return ret;
	}

	static Class<ASObject>* getClass(SystemState* sys);
	static _R<Class<ASObject>> getRef(SystemState* sys)
	{
		Class<ASObject>* ret = getClass(sys);
		ret->incRef();
		return _MR(ret);
	}
	static ASObject* cast(ASObject* o)
	{
		return static_cast<ASObject*>(o);
	}
	void buildInstanceTraits(ASObject* o) const
	{
		ASObject::buildTraits(o);
	}
	void generator(ASWorker* wrk,asAtom& ret, asAtom* args, const unsigned int argslen)
	{
		if(argslen==0 || asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
			ret=asAtomHandler::fromObject(Class<ASObject>::getInstanceS(wrk));
		else
		{
			ASATOM_INCREF(args[0]);
			ret=args[0];
		}
	}
};

/* InterfaceClass implements interfaces. E.g., if you declare a variable of type IEventDispatcher in AS3,
 * then the type in our code will be InterfaceClass<IEventDispatcher>.
 * All classes that implement an interface in AS3 do so in the C++ code, i.e.
 * EventDispatcher derives from IEventDispatcher both in AS3 and C++ code.
 * This makes it possible to use C++ casts similar to their AS3 counterparts.
 *
 * TODO: InterfaceClass should derive from Type, not from Class_base!
 */
void lookupAndLink(Class_base* c, const tiny_string& name, const tiny_string& interfaceNs);
void lookupAndLink(Class_base* c, uint32_t nameID, uint32_t interfaceNsID);
template<class T>
class InterfaceClass: public Class_base
{
	virtual ~InterfaceClass() { }
	void buildInstanceTraits(ASObject*) const {}
	void getInstance(ASWorker* worker,asAtom&, bool, asAtom*, unsigned int, Class_base* realClass)
	{
		assert(false);
	}
	void generator(ASWorker* wrk,asAtom& ret, asAtom* args, const unsigned int argslen)
	{
		assert(argslen == 1);
		ASATOM_INCREF(args[0]);
		ret = args[0];
	}
	InterfaceClass(const QName& name, uint32_t classID, MemoryAccount* m):Class_base(name, classID, m) { }
public:
	static InterfaceClass<T>* getClass(SystemState* sys)
	{
		uint32_t classId=ClassName<T>::id;
		InterfaceClass<T>* ret=nullptr;
		Class_base** retAddr=&sys->builtinClasses[classId];
		if(*retAddr==nullptr)
		{
			//Create the class
			QName name(sys->getUniqueStringId(ClassName<T>::name),sys->getUniqueStringId(ClassName<T>::ns));
			MemoryAccount* m = sys->allocateMemoryAccount(name.getQualifiedName(sys));
			ret=new (m) InterfaceClass<T>(name, ClassName<T>::id, m);
			ret->setWorker(sys->worker);
			ret->setSystemState(sys);
			ret->isInterface = true;
			ret->incRef();
			*retAddr=ret;
		}
		else
			ret=static_cast<InterfaceClass<T>*>(*retAddr);

		return ret;
	}
	static _R<InterfaceClass<T>> getRef(SystemState* sys)
	{
		InterfaceClass<T>* ret = getClass(sys);
		ret->incRef();
		return _MR(ret);
	}
	void linkInterface(Class_base* c) const
	{
		T::linkTraits(c);
	}
};

}
#endif /* SCRIPTING_CLASS_H */
