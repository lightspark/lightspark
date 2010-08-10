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

extern TLSDATA lightspark::SystemState* sys DLL_PUBLIC;

namespace lightspark
{

template<typename T>
class ClassName
{
public:
	static const char* name;
};

#define REGISTER_CLASS_NAME(X) template<> \
	const char* ClassName<X>::name = #X;

#define REGISTER_CLASS_NAME2(X,NAME) template<> \
	const char* ClassName<X>::name = NAME;

class Class_inherit:public Class_base
{
private:
	ASObject* getInstance(bool construct, ASObject* const* args, const unsigned int argslen);
	DictionaryTag const* tag;
public:
	Class_inherit(const tiny_string& name):Class_base(name),tag(NULL)
	{
		bool ret=sys->classes.insert(std::make_pair(name,this)).second;
		assert_and_throw(ret && "Class name collision found");
	}
	void buildInstanceTraits(ASObject* o) const;
	void bindTag(DictionaryTag const* t)
	{
		tag=t;
	}
};

template< class T>
class Class: public Class_base
{
private:
	Class(const tiny_string& name):Class_base(name){}
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
	static Class<T>* getClass(const tiny_string& name)
	{
		std::map<tiny_string, Class_base*>::iterator it=sys->classes.find(name);
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
		return getClass(ClassName<T>::name);
	}
	static T* cast(ASObject* o)
	{
		return static_cast<T*>(o);
	}
	void buildInstanceTraits(ASObject* o) const
	{
		T::buildTraits(o);
	}
};

template<>
class Class<ASObject>: public Class_base
{
private:
	Class<ASObject>(const tiny_string& name):Class_base(name){}
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
	static Class<ASObject>* getClass(const tiny_string& name)
	{
		std::map<tiny_string, Class_base*>::iterator it=sys->classes.find(name);
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
		return getClass(ClassName<ASObject>::name);
	}
	static ASObject* cast(ASObject* o)
	{
		return static_cast<ASObject*>(o);
	}
	void buildInstanceTraits(ASObject* o) const
	{
		ASObject::buildTraits(o);
	}
	ASObject* getVariableByMultiname(const multiname& name, bool skip_impl, bool enableOverride=true, ASObject* base=NULL);
};

};
#endif
