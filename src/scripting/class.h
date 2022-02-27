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

#include "compat.h"
#include "asobject.h"
#include "swf.h"
#include "scripting/toplevel/toplevel.h"

#ifndef SCRIPTING_CLASS_H
#define SCRIPTING_CLASS_H 1

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
	static unsigned int id;
};

class Class_inherit:public Class_base
{
private:
	void getInstance(asAtom& ret, bool construct, asAtom *args, const unsigned int argslen, Class_base* realClass) override;
	DictionaryTag* tag;
	bool bindedToRoot;
	bool bindingchecked;
	void recursiveBuild(ASObject* target) const;
	const traits_info* classtrait;
	_NR<ASObject> instancefactory;
public:
	Class_inherit(const QName& name, MemoryAccount* m,const traits_info* _classtrait, Global* _global);
	void checkScriptInit()
	{
		if (global)
			global->checkScriptInit();
	}
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
	GET_VARIABLE_RESULT getVariableByMultiname(asAtom& ret, const multiname& name, GET_VARIABLE_OPTION opt) override;
	std::string toDebugString() override;
};

/* helper function: does Class<ASObject>::getInstances(), but solves forward declaration problem */
ASObject* new_asobject(SystemState *sys);
Prototype* new_objectPrototype(SystemState *sys);
Prototype* new_functionPrototype(Class_base* functionClass, _NR<Prototype> p);
Function_object* new_functionObject(_NR<ASObject> p);
ObjectConstructor* new_objectConstructor(Class_base* cls,uint32_t length);
Activation_object* new_activationObject(SystemState* sys);

template<class T,std::size_t N>
struct newWithOptionalClass
{
	template<class F, typename... Args>
	static T* doNew(Class_base* c, const F& f, Args&&... args)
	{
		return newWithOptionalClass<T, N-1>::doNew(c, std::forward<Args>(args)..., f);
	}
};
template<class T>
struct newWithOptionalClass<T, 1>
{
	template<class F, typename... Args>
	static T* doNew(Class_base* c, const F& f, Args&&... args)
	{
		//Last parameter is not a class pointer
		return new (c->memoryAccount) T(c, std::forward<Args>(args)..., f);
	}
	template<typename... Args>
	static T* doNew(Class_base* c, Class_base* f, Args&&... args)
	{
		//Last parameter is a class pointer
		return new (f->memoryAccount) T(f, std::forward<Args>(args)...);
	}
};

template<class T>
struct newWithOptionalClass<T, 0>
{
	static T* doNew(Class_base* c)
	{
		//Last parameter is not a class pointer
		return new (c->memoryAccount) T(c);
	}
};

template< class T>
class Class: public Class_base
{
protected:
	Class(const QName& name, MemoryAccount* m):Class_base(name, m){}
	//This function is instantiated always because of inheritance
	void getInstance(asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass=nullptr) override
	{
		if(realClass==nullptr)
			realClass=this;
		ret = asAtomHandler::fromObject(realClass->freelist[0].getObjectFromFreeList());
		if (asAtomHandler::isInvalid(ret))
			ret=asAtomHandler::fromObject(new (realClass->memoryAccount) T(realClass));
		asAtomHandler::resetCached(ret);
		if(construct)
			handleConstruction(ret,args,argslen,true);
	}
public:
	/*
	template<typename... Args>
	static T* getInstanceS(Args&&... args)
	{
		Class<T>* c=Class<T>::getClass();
		T* ret=newWithOptionalClass<T, sizeof...(Args)>::doNew(c, std::forward<Args>(args)...);
		c->handleConstruction(ret,NULL,0,true);
		return ret;
	}
	*/
	template<typename... Args>
	static T* getInstanceS(SystemState* sys, Args&&... args)
	{
		Class<T>* c=static_cast<Class<T>*>(sys->builtinClasses[ClassName<T>::id]);
		if (!c)
			c = getClass(sys);
		T* ret=newWithOptionalClass<T, sizeof...(Args)>::doNew(c, std::forward<Args>(args)...);
		c->setupDeclaredTraits(ret);
		ret->constructionComplete();
		ret->setConstructIndicator();
		return ret;
	}
	// constructor without arguments
	inline static T* getInstanceSNoArgs(SystemState* sys)
	{
		Class<T>* c=static_cast<Class<T>*>(sys->builtinClasses[ClassName<T>::id]);
		if (!c)
			c = getClass(sys);
		T* ret = sys->singleworker || !getWorker() ? c->freelist[0].getObjectFromFreeList()->template as<T>() : nullptr;
		if (!ret)
		{
			ret=new (c->memoryAccount) T(c);
			assert_and_throw(ret);
		}
		ret->resetCached();
		ret->setIsInitialized();
		ret->constructionComplete();
		ret->setConstructIndicator();
		return ret;
	}
	// constructor that doesn't use the freelist (must be used when not in VM thread)
	inline static T* getInstanceSNoArgsNoFreelist(SystemState* sys)
	{
		Class<T>* c=static_cast<Class<T>*>(sys->builtinClasses[ClassName<T>::id]);
		if (!c)
			c = getClass(sys);
		T* ret=new (c->memoryAccount) T(c);
		assert_and_throw(ret);
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
			ret=new (m) Class<T>(name, m);
			ret->setSystemState(sys);
			ret->incRef();
			*retAddr=ret;
			ret->prototype = _MNR(new_objectPrototype(sys));
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
	void generator(asAtom& ret, asAtom* args, const unsigned int argslen) override
	{
		T::generator(ret,getSystemState(), asAtomHandler::invalidAtom, args, argslen);
	}
	bool coerce(SystemState* sys,asAtom& o) const override
	{
		return Class_base::coerce(sys,o);
	}
};

template<>
void Class<Global>::getInstance(asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass);

template<>
inline bool Class<Number>::coerce(SystemState* sys,asAtom& o) const
{
	if (asAtomHandler::isNumeric(o))
		return false;
	number_t n = asAtomHandler::toNumber(o);
	asAtomHandler::setNumber(o,sys,n);
	return true;
}

template<>
inline bool Class<UInteger>::coerce(SystemState* sys,asAtom& o) const
{
	if (asAtomHandler::isUInteger(o))
		return false;
	uint32_t n = asAtomHandler::toUInt(o);
	asAtomHandler::setUInt(o,sys,n);
	return true;
}

template<>
inline bool Class<Integer>::coerce(SystemState* sys,asAtom& o) const
{
	if (asAtomHandler::isInteger(o))
		return false;
	int32_t n = asAtomHandler::toInt(o);
	asAtomHandler::setInt(o,sys,n);
	return true;
}

template<>
inline bool Class<Boolean>::coerce(SystemState* sys,asAtom& o) const
{
	if (asAtomHandler::isBool(o))
		return false;
	bool n = asAtomHandler::Boolean_concrete(o);
	asAtomHandler::setBool(o,n);
	return true;
}
template<>
inline void Class<Boolean>::getInstance(asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass)
{
	if (argslen> 0)
		ret = asAtomHandler::fromObject(abstract_b(getSys(),asAtomHandler::Boolean_concrete(args[0])));
	else
		ret = asAtomHandler::fromObject(abstract_b(getSys(),false));
}

template<>
class Class<ASObject>: public Class_base
{
private:
	Class<ASObject>(const QName& name, MemoryAccount* m):Class_base(name, m){}
	//This function is instantiated always because of inheritance
	void getInstance(asAtom& ret, bool construct, asAtom* args, const unsigned int argslen, Class_base* realClass=nullptr);
public:
	static ASObject* getInstanceS(SystemState* sys)
	{
		Class<ASObject>* c=Class<ASObject>::getClass(sys);
		ASObject* ret = c->freelist[0].getObjectFromFreeList();
		if (!ret)
			ret=new (c->memoryAccount) ASObject(c);
		ret->resetCached();
		c->setupDeclaredTraits(ret);
		ret->constructionComplete();
		ret->setConstructIndicator();
		ret->setConstructorCallComplete();
		return ret;
		//return c->getInstance(true,NULL,0);
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
	void generator(asAtom& ret, asAtom* args, const unsigned int argslen)
	{
		if(argslen==0 || asAtomHandler::is<Null>(args[0]) || asAtomHandler::is<Undefined>(args[0]))
			ret=asAtomHandler::fromObject(Class<ASObject>::getInstanceS(getSys()));
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
	void getInstance(asAtom&, bool, asAtom*, unsigned int, Class_base* realClass)
	{
		assert(false);
	}
	void generator(asAtom& ret, asAtom* args, const unsigned int argslen)
	{
		assert(argslen == 1);
		ASATOM_INCREF(args[0]);
		ret = args[0];
	}
	InterfaceClass(const QName& name, MemoryAccount* m):Class_base(name, m) { }
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
			ret=new (m) InterfaceClass<T>(name, m);
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
