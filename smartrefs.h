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

#include "asobject.h"

namespace lightspark
{

class NullRef_t
{
};

extern NullRef_t NullRef;

template<class T>
class NullableRef
{
private:
	T* m;
public:
	explicit NullableRef(T* o):m(o){}
	NullableRef(NullRef_t):m(NULL){}
	NullableRef(const NullableRef& r):m(r.m)
	{
		if(m)
			m->incRef();
	}
	template<class D> NullableRef(const NullableRef<D>& r):m(r.m)
	{
		if(m)
			m->incRef();
	}
	~NullableRef()
	{
		if(m)
			m->decRef();
	}
	T* operator->() const {return m;}
	T* getPtr() const { return m; }
	bool isNull() const { return m==NULL; }
};

//Shorthand notation
#define _NR NullableRef

template<class T>
NullableRef<T> _MNR(T* a)
{
	return NullableRef<T>(a);
}

template<class T>
class Ref
{
private:
	T* m;
public:
	explicit Ref(T* o):m(o)
	{
		assert(m);
	}
	Ref(const Ref<T>& r):m(r.m)
	{
		m->incRef();
	}
	template<class D> explicit Ref(const NullableRef<D>& r):m(r.m)
	{
		assert(m);
		m->incRef();
	}
	template<class D> Ref(const Ref<D>& r):m(r.m)
	{
		m->incRef();
	}
	~Ref()
	{
		m->decRef();
	}
	T* operator->() const {return m;}
	T* getPtr() const { return m; }
};

#define _R Ref

template<class T>
Ref<T> _MR(T* a)
{
	return Ref<T>(a);
}

};
