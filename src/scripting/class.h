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

#include "scripting/abcutils.h"
#include "scripting/toplevel/Array.h"
#include "compat.h"
#include "asobject.h"
#include "swf.h"

#ifndef CLASS_H
#define CLASS_H

namespace lightspark
{

template<typename T>
class ClassName
{
public:
	static const char* ns;
	static const char* name;
};

#define SET_NAMESPACE(NS) \
	static const char* CURRENTNAMESPACE=NS;

#define REGISTER_CLASS_NAME(TYPE) \
	template<> const char* ClassName<TYPE>::name = #TYPE; \
	template<> const char* ClassName<TYPE>::ns = CURRENTNAMESPACE;

#define REGISTER_CLASS_NAME2(TYPE,NAME,NS) \
	template<> const char* ClassName<TYPE>::name = NAME; \
	template<> const char* ClassName<TYPE>::ns = NS;

class Class_inherit:public Class_base
{
private:
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen);
	DictionaryTag const* tag;
	bool bindedToRoot;
public:
	Class_inherit(const QName& name):Class_base(name),tag(NULL),bindedToRoot(false)
	{
		this->incRef(); //create on reference for the classes map
#ifndef NDEBUG
		bool ret=
#endif
		sys->classes.insert(std::make_pair(name,this)).second;
		assert(ret);
	}
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

template< class T>
class Class: public Class_base
{
protected:
	Class(const QName& name):Class_base(name){}
	//This function is instantiated always because of inheritance
	T* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
	{
		T* ret=new T;
		ret->setClass(this);
		if(construct)
			handleConstruction(ret,args,argslen,true);
		return ret;
	}
public:
	template<typename... Args>
	static T* getInstanceS(Args&&... args)
	{
		Class<T>* c=Class<T>::getClass();
		T* ret = new T(args...);
		ret->setClass(c);
		c->handleConstruction(ret,NULL,0,true);
		return ret;
	}
	static Class<T>* getClass()
	{
		QName name(ClassName<T>::name,ClassName<T>::ns);
		std::map<QName, Class_base*>::iterator it=sys->classes.find(name);
		Class<T>* ret=NULL;
		if(it==sys->classes.end()) //This class is not yet in the map, create it
		{
			ret=new Class<T>(name);
			sys->classes.insert(std::make_pair(name,ret));
			ret->prototype = _MNR(new_asobject());
			T::sinit(ret);
			ret->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(Class_base::_toString),NORMAL_METHOD,false);
			ret->incRef();
			ret->prototype->setVariableByQName("constructor","",ret,DYNAMIC_TRAIT);
			if(ret->super)
				ret->prototype->setprop_prototype(ret->super->prototype);
			ret->addPrototypeGetter();
		}
		else
			ret=static_cast<Class<T>*>(it->second);

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
		return T::generator(NULL, args, argslen);
	}
	ASObject* coerce(ASObject* o) const
	{
		return Class_base::coerce(o);
	}
};

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
inline ASObject* Class<ASString>::coerce(ASObject* o) const
{ //Special handling for Null and Undefined follows avm2overview's description of 'coerce_s' opcode
	if(o->is<Null>())
		return o;
	if(o->is<Undefined>())
	{
		o->decRef();
		return new Null;
	}
	tiny_string n = o->toString();
	o->decRef();
	return Class<ASString>::getInstanceS(n);
}

template<>
class Class<ASObject>: public Class_base
{
private:
	Class<ASObject>(const QName& name):Class_base(name){}
	//This function is instantiated always because of inheritance
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
	{
		ASObject* ret=new ASObject;
		ret->setClass(this);
		if(construct)
			handleConstruction(ret,args,argslen,true);
		return ret;
	}
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
		Class<ASObject>* ret = new Class<ASObject>(name);

		ret->setSuper(Class<ASObject>::getRef());
		ret->prototype = _MNR(new_asobject());
		ret->prototype->setprop_prototype(ret->super->prototype);
		ret->incRef();
		ret->prototype->setVariableByQName("constructor","",ret,DYNAMIC_TRAIT);
		ret->addPrototypeGetter();

		ret->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(Class_base::_toString),NORMAL_METHOD,false);
		sys->classes.insert(std::make_pair(name,ret));
		ret->incRef();
		return _MR(ret);
	}
	static Class<ASObject>* getClass()
	{
		QName name(ClassName<ASObject>::name,ClassName<ASObject>::ns);
		std::map<QName, Class_base*>::iterator it=sys->classes.find(name);
		Class<ASObject>* ret=NULL;
		if(it==sys->classes.end()) //This class is not yet in the map, create it
		{
			ret=new Class<ASObject>(name);
			sys->classes.insert(std::make_pair(name,ret));
			ret->prototype = _MNR(new_asobject());
			ASObject::sinit(ret);
			ret->setDeclaredMethodByQName("toString",AS3,Class<IFunction>::getFunction(Class_base::_toString),NORMAL_METHOD,false);
			ret->incRef();
			ret->prototype->setVariableByQName("constructor","",ret,DYNAMIC_TRAIT);

			ret->addPrototypeGetter();
		}
		else
			ret=static_cast<Class<ASObject>*>(it->second);

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
};

/* This is a class which was instantiated from a Template<T> */
template<class T>
class TemplatedClass : public Class<T>
{
private:
	/* the Template<T>* this class was generated from */
	const Template_base* templ;
	std::vector<Class_base*> types;
public:
	TemplatedClass(const QName& name, ASObject* const* _types, const unsigned int numtypes, Template_base* _templ)
		: Class<T>(name), templ(_templ)
	{
		assert(types.empty());
		types.reserve(numtypes);
		for(size_t i=0;i<numtypes;++i)
		{
			assert_and_throw(_types[i]->getObjectType() == T_CLASS);
			Class_base* o_class = static_cast<Class_base*>(_types[i]);
			types.push_back(o_class);
		}
	}

	T* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
	{
		T* ret=new T;
		ret->setClass(this);
		ret->setTypes(types);
		if(construct)
			handleConstruction(ret,args,argslen,true);
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

	const std::vector<Class_base*> getTypes() const
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

	QName getQName(ASObject* const* types, const unsigned int numtypes)
	{
		//This is the naming scheme that the ABC compiler uses,
		//and we need to stay in sync here
		assert_and_throw(numtypes);
		QName ret(ClassName<T>::name, ClassName<T>::ns);
		for(size_t i=0;i<numtypes;++i)
		{
			assert_and_throw(types[i]->getObjectType() == T_CLASS);
			Class_base* o_class = static_cast<Class_base*>(types[i]);
			ret.name += "$";
			ret.name += o_class->class_name.getQualifiedName();
		}
		return ret;
	}

	/* this function will take ownership of the types objects */
	Class_base* applyType(ASObject* const* types, const unsigned int numtypes)
	{
		QName instantiatedQName = getQName(types,numtypes);

		std::map<QName, Class_base*>::iterator it=sys->classes.find(instantiatedQName);
		Class<T>* ret=NULL;
		if(it==sys->classes.end()) //This class is not yet in the map, create it
		{
			ret=new TemplatedClass<T>(instantiatedQName,types,numtypes,this);
			sys->classes.insert(std::make_pair(instantiatedQName,ret));
			ret->prototype = _MNR(new_asobject());
			T::sinit(ret);
			if(ret->super)
				ret->prototype->setprop_prototype(ret->super->prototype);
			ret->addPrototypeGetter();
		}
		else
			ret=static_cast<TemplatedClass<T>*>(it->second);

		ret->incRef();
		return ret;
	}

	static Template<T>* getTemplate(const QName& name)
	{
		std::map<QName, Template_base*>::iterator it=sys->templates.find(name);
		Template<T>* ret=NULL;
		if(it==sys->templates.end()) //This class is not yet in the map, create it
		{
			ret=new Template<T>(name);
			sys->templates.insert(std::make_pair(name,ret));
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
#endif
