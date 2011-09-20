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
	static Class<T>* getClass(const QName& name)
	{
		std::map<QName, Class_base*>::iterator it=sys->classes.find(name);
		Class<T>* ret=NULL;
		if(it==sys->classes.end()) //This class is not yet in the map, create it
		{
			ret=new Class<T>(name);
			sys->classes.insert(std::make_pair(name,ret));
			ret->incRef();
			ret->prototype = _MNR(new Prototype(_MR(ret)));
			T::sinit(ret);
			if(ret->super != NULL)
				ret->prototype->prototype = ret->super->prototype;
			ret->addPrototypeGetter();
		}
		else
			ret=static_cast<Class<T>*>(it->second);

		ret->incRef();
		return ret;
	}
	static Class<T>* getClass()
	{
		return getClass(QName(ClassName<T>::name,ClassName<T>::ns));
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
};

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
	static Class<ASObject>* getClass(const QName& name)
	{
		std::map<QName, Class_base*>::iterator it=sys->classes.find(name);
		Class<ASObject>* ret=NULL;
		if(it==sys->classes.end()) //This class is not yet in the map, create it
		{
			ret=new Class<ASObject>(name);
			sys->classes.insert(std::make_pair(name,ret));
			ret->incRef();
			ret->prototype = _MNR(new Prototype(_MR(ret)));
			ASObject::sinit(ret);
			ret->addPrototypeGetter();
		}
		else
			ret=static_cast<Class<ASObject>*>(it->second);

		ret->incRef();
		return ret;
	}
	static Class<ASObject>* getClass()
	{
		return getClass(QName(ClassName<ASObject>::name,ClassName<ASObject>::ns));
	}
	static ASObject* cast(ASObject* o)
	{
		return static_cast<ASObject*>(o);
	}
	void buildInstanceTraits(ASObject* o) const
	{
		ASObject::buildTraits(o);
	}
	ASObject* lazyDefine(const multiname& name);
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
		//This is modeled after the internal naming of the proprietary player
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
			ret->incRef();
			ret->prototype = _MNR(new Prototype(_MR(ret)));
			T::sinit(ret);
			if(ret->super != NULL)
				ret->prototype->prototype = ret->super->prototype;
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
