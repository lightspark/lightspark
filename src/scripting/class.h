/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/toplevel/Array.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "compat.h"
#include "asobject.h"
#include "swf.h"

#ifndef SCRIPTING_CLASS_H
#define SCRIPTING_CLASS_H 1

namespace lightspark
{

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
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass);
	DictionaryTag const* tag;
	bool bindedToRoot;
public:
	Class_inherit(const QName& name, MemoryAccount* m);
	void finalize();
	void buildInstanceTraits(ASObject* o) const;
	void bindToTag(DictionaryTag const* t)
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
	//Closure stack
	std::vector<scope_entry> class_scope;
};

/* helper function: does Class<ASObject>::getInstances(), but solves forward declaration problem */
ASObject* new_asobject();
Prototype* new_objectPrototype();
Prototype* new_functionPrototype(Class_base* functionClass, _NR<Prototype> p);
Function_object* new_functionObject(_NR<ASObject> p);

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
	T* getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass=NULL)
	{
		if(realClass==NULL)
			realClass=this;
		T* ret=new (realClass->memoryAccount) T(realClass);
		if(construct)
			handleConstruction(ret,args,argslen,true);
		return ret;
	}
public:
	template<typename... Args>
	static T* getInstanceS(Args&&... args)
	{
		Class<T>* c=Class<T>::getClass();
		T* ret=newWithOptionalClass<T, sizeof...(Args)>::doNew(c, std::forward<Args>(args)...);
		c->handleConstruction(ret,NULL,0,true);
		return ret;
	}
	static Class<T>* getClass()
	{
		uint32_t classId=ClassName<T>::id;
		Class<T>* ret=NULL;
		Class_base** retAddr=&getSys()->builtinClasses[classId];
		if(*retAddr==NULL)
		{
			//Create the class
			QName name(ClassName<T>::name,ClassName<T>::ns);
			MemoryAccount* memoryAccount = getSys()->allocateMemoryAccount(name.name);
			ret=new (getSys()->unaccountedMemory) Class<T>(name, memoryAccount);
			ret->incRef();
			*retAddr=ret;
			ret->prototype = _MNR(new_objectPrototype());
			T::sinit(ret);

			ret->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(Class_base::_toString),NORMAL_METHOD,false);
			ret->incRef();
			ret->prototype->setVariableByQName("constructor","",ret,DYNAMIC_TRAIT);
			if(ret->super)
				ret->prototype->prevPrototype=ret->super->prototype;
			ret->addPrototypeGetter();
			ret->addLengthGetter();
		}
		else
			ret=static_cast<Class<T>*>(*retAddr);

		return ret;
	}
	static _R<Class<T>> getRef()
	{
		Class<T>* ret = getClass();
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
	void buildInstanceTraits(ASObject* o) const
	{
		T::buildTraits(o);
	}
	ASObject* generator(ASObject* const* args, const unsigned int argslen)
	{
		ASObject *ret=T::generator(NULL, args, argslen);
		for(unsigned int i=0;i<argslen;i++)
			args[i]->decRef();
		return ret;
	}
	ASObject* coerce(ASObject* o) const
	{
		return Class_base::coerce(o);
	}
};

template<>
Global* Class<Global>::getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass);

template<>
inline ASObject* Class<Number>::coerce(ASObject* o) const
{
	number_t n = o->toNumber();
	o->decRef();
	return abstract_d(n);
}

template<>
inline ASObject* Class<UInteger>::coerce(ASObject* o) const
{
	uint32_t n = o->toUInt();
	o->decRef();
	return abstract_ui(n);
}

template<>
inline ASObject* Class<Integer>::coerce(ASObject* o) const
{
	int32_t n = o->toInt();
	o->decRef();
	return abstract_i(n);
}

template<>
inline ASObject* Class<Boolean>::coerce(ASObject* o) const
{
	bool n = Boolean_concrete(o);
	o->decRef();
	return abstract_b(n);
}

template<>
class Class<ASObject>: public Class_base
{
private:
	Class<ASObject>(const QName& name, MemoryAccount* m):Class_base(name, m){}
	//This function is instantiated always because of inheritance
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass=NULL);
public:
	static ASObject* getInstanceS()
	{
		Class<ASObject>* c=Class<ASObject>::getClass();
		return c->getInstance(true,NULL,0);
	}
	/* This creates a stub class, i.e. a class with given name but without
	 * any implementation.
	 */
	static _R<Class<ASObject>> getStubClass(const QName& name)
	{
		MemoryAccount* memoryAccount = getSys()->allocateMemoryAccount(name.name);
		Class<ASObject>* ret = new (getSys()->unaccountedMemory) Class<ASObject>(name, memoryAccount);

		ret->setSuper(Class<ASObject>::getRef());
		ret->prototype = _MNR(new_objectPrototype());
		ret->prototype->prevPrototype=ret->super->prototype;
		ret->incRef();
		ret->prototype->setVariableByQName("constructor","",ret,DYNAMIC_TRAIT);
		ret->addPrototypeGetter();
		ret->addLengthGetter();

		ret->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(Class_base::_toString),NORMAL_METHOD,false);
		getSys()->customClasses.insert(ret);
		ret->incRef();
		return _MR(ret);
	}
	static Class<ASObject>* getClass()
	{
		uint32_t classId=ClassName<ASObject>::id;
		Class<ASObject>* ret=NULL;
		Class_base** retAddr=&getSys()->builtinClasses[classId];
		if(*retAddr==NULL)
		{
			//Create the class
			QName name(ClassName<ASObject>::name,ClassName<ASObject>::ns);
			MemoryAccount* memoryAccount = getSys()->allocateMemoryAccount(name.name);
			ret=new (getSys()->unaccountedMemory) Class<ASObject>(name, memoryAccount);
			ret->incRef();
			*retAddr=ret;
			ret->prototype = _MNR(new_objectPrototype());
			ASObject::sinit(ret);

			ret->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(Class_base::_toString),NORMAL_METHOD,false);
			ret->incRef();
			ret->prototype->setVariableByQName("constructor","",ret,DYNAMIC_TRAIT);
			ret->addPrototypeGetter();
			ret->addLengthGetter();
		}
		else
			ret=static_cast<Class<ASObject>*>(*retAddr);

		return ret;
	}
	static _R<Class<ASObject>> getRef()
	{
		Class<ASObject>* ret = getClass();
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
	ASObject* generator(ASObject* const* args, const unsigned int argslen)
	{
		ASObject *ret;
		if(argslen==0 || args[0]->is<Null>() || args[0]->is<Undefined>())
			ret=Class<ASObject>::getInstanceS();
		else
		{
			args[0]->incRef();
			ret=args[0];
		}
		for(unsigned int i=0;i<argslen;i++)
			args[i]->decRef();
		return ret;
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
template<class T>
class InterfaceClass: public Class_base
{
	virtual ~InterfaceClass() {}
	void buildInstanceTraits(ASObject*) const {}
	ASObject* getInstance(bool, ASObject* const*, unsigned int, Class_base* realClass)
	{
		assert(false);
		return NULL;
	}
	ASObject* generator(ASObject* const* args, const unsigned int argslen)
	{
		assert(argslen == 1);
		return args[0];
	}
	InterfaceClass(const QName& name, MemoryAccount* m):Class_base(name, m) {}
public:
	static InterfaceClass<T>* getClass()
	{
		uint32_t classId=ClassName<T>::id;
		InterfaceClass<T>* ret=NULL;
		Class_base** retAddr=&getSys()->builtinClasses[classId];
		if(*retAddr==NULL)
		{
			//Create the class
			QName name(ClassName<T>::name,ClassName<T>::ns);
			MemoryAccount* memoryAccount = getSys()->allocateMemoryAccount(name.name);
			ret=new (getSys()->unaccountedMemory) InterfaceClass<T>(name, memoryAccount);
			ret->incRef();
			*retAddr=ret;
		}
		else
			ret=static_cast<InterfaceClass<T>*>(*retAddr);

		return ret;
	}
	static _R<InterfaceClass<T>> getRef()
	{
		InterfaceClass<T>* ret = getClass();
		ret->incRef();
		return _MR(ret);
	}
	void linkInterface(Class_base* c) const
	{
		T::linkTraits(c);
	}
};

/* This is a class which was instantiated from a Template<T> */
template<class T>
class TemplatedClass : public Class<T>
{
private:
	/* the Template<T>* this class was generated from */
	const Template_base* templ;
	std::vector<Type*> types;
public:
	TemplatedClass(const QName& name, const std::vector<Type*>& _types, Template_base* _templ, MemoryAccount* m)
		: Class<T>(name, m), templ(_templ), types(_types)
	{
	}

	T* getInstance(bool construct, ASObject* const* args, const unsigned int argslen, Class_base* realClass=NULL)
	{
		if(realClass==NULL)
			realClass=this;
		T* ret=new (realClass->memoryAccount) T(realClass);
		ret->setTypes(types);
		if(construct)
			this->handleConstruction(ret,args,argslen,true);
		return ret;
	}

	/* This function is called for as3 code like v = Vector.<String>(["Hello", "World"])
	 * this->types will be Class<ASString> on entry of this function.
	 */
	ASObject* generator(ASObject* const* args, const unsigned int argslen)
	{
		ASObject* ret = T::generator(this,args,argslen);
		for(size_t i=0;i<argslen;++i)
			args[i]->decRef();
		return ret;
	}

	const Template_base* getTemplate() const
	{
		return templ;
	}

	const std::vector<Type*> getTypes() const
	{
		return types;
	}
};

/* this is modeled closely after the Class/Class_base pattern */
template<class T>
class Template : public Template_base
{
public:
	Template(QName name) : Template_base(name) {};

	QName getQName(const std::vector<Type*>& types)
	{
		//This is the naming scheme that the ABC compiler uses,
		//and we need to stay in sync here
		QName ret(ClassName<T>::name, ClassName<T>::ns);
		for(size_t i=0;i<types.size();++i)
		{
			ret.name += "$";
			ret.name += types[i]->getName();
		}
		return ret;
	}

	Class_base* applyType(const std::vector<Type*>& types)
	{
		QName instantiatedQName = getQName(types);

		std::map<QName, Class_base*>::iterator it=getSys()->instantiatedTemplates.find(instantiatedQName);
		Class<T>* ret=NULL;
		if(it==getSys()->instantiatedTemplates.end()) //This class is not yet in the map, create it
		{
			MemoryAccount* memoryAccount = getSys()->allocateMemoryAccount(instantiatedQName.name);
			ret=new (getSys()->unaccountedMemory) TemplatedClass<T>(instantiatedQName,types,this,memoryAccount);
			getSys()->instantiatedTemplates.insert(std::make_pair(instantiatedQName,ret));
			ret->prototype = _MNR(new_objectPrototype());
			T::sinit(ret);
			if(ret->super)
				ret->prototype->prevPrototype=ret->super->prototype;
			ret->addPrototypeGetter();
		}
		else
			ret=static_cast<TemplatedClass<T>*>(it->second);

		ret->incRef();
		return ret;
	}

	static Ref<Class_base> getTemplateInstance(Type* type)
	{
		std::vector<Type*> t(1,type);
		Template<T>* templ=getTemplate();
		Ref<Class_base> ret=_MR(templ->applyType(t));
		templ->decRef();
		return ret;
	}

	static Template<T>* getTemplate(const QName& name)
	{
		std::map<QName, Template_base*>::iterator it=getSys()->templates.find(name);
		Template<T>* ret=NULL;
		if(it==getSys()->templates.end()) //This class is not yet in the map, create it
		{
			ret=new (getSys()->unaccountedMemory) Template<T>(name);
			getSys()->templates.insert(std::make_pair(name,ret));
		}
		else
			ret=static_cast<Template<T>*>(it->second);

		ret->incRef();
		return ret;
	}

	static Template<T>* getTemplate()
	{
		return getTemplate(QName(ClassName<T>::name,ClassName<T>::ns));
	}
};

};
#endif /* SCRIPTING_CLASS_H */
