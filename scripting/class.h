/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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
public:
	Class_inherit(const QName& name):Class_base(name),tag(NULL)
	{
		bool ret=sys->classes.insert(std::make_pair(name,this)).second;
		if(!ret)
		{
			LOG(LOG_ERROR,"Class name collision at: " << name);
			throw RunTimeException("Class name collision");
		}
	}
	void buildInstanceTraits(ASObject* o) const;
	void bindTag(DictionaryTag const* t)
	{
		tag=t;
	}
	//Closure stack
	std::vector<ASObject*> class_scope;
};

template< class T>
class Class: public Class_base
{
private:
	Class(const QName& name):Class_base(name){}
	//This function is instantiated always because of inheritance
	T* getInstance(bool construct, ASObject* const* args, const unsigned int argslen)
	{
		T* ret=new T;
		ret->setPrototype(this);
		if(construct)
			handleConstruction(ret,args,argslen,true);
		return ret;
	}
public:
	static T* getInstanceS()
	{
		Class<T>* c=Class<T>::getClass();
		return c->getInstance(true,NULL,0);
	}
	template <typename ARG1>
	static T* getInstanceS(const ARG1& a1)
	{
		Class<T>* c=Class<T>::getClass();
		T* ret=new T(a1);
		ret->setPrototype(c);
		c->handleConstruction(ret,NULL,0,true);
		return ret;
	}
	template <typename ARG1, typename ARG2>
	static T* getInstanceS(const ARG1& a1, const ARG2& a2)
	{
		Class<T>* c=Class<T>::getClass();
		T* ret=new T(a1,a2);
		ret->setPrototype(c);
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
			T::sinit(ret);
			sys->classes.insert(std::make_pair(name,ret));
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
		ret->setPrototype(this);
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
			ASObject::sinit(ret);
			sys->classes.insert(std::make_pair(name,ret));
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
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl, ASObject* base=NULL);
};

};
#endif
