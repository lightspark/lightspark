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
class ObjectReference
{
protected:
	T* m;
public:
	explicit ObjectReference(T* o):m(o){}
	ObjectReference(NullRef_t):m(NULL){}
	ObjectReference(const ObjectReference& r):m(r.m)
	{
		if(m)
			m->incRef();
	}
	template<class D> ObjectReference(const ObjectReference<D>& r):m(r.m)
	{
		if(m)
			m->incRef();
	}
	~ObjectReference()
	{
		if(m)
			m->decRef();
	}
	T* operator->() const {return m;}
	T* getPtr() const { return m; }
	bool isNull() const { return m==NULL; }
};

//Shorthand notation
template<class T>
class _R: public ObjectReference<T>
{
public:
	explicit _R(T* o):ObjectReference<T>(o)
	{
	}
	_R(const _R<T>& r):ObjectReference<T>(r)
	{
	}
	_R(NullRef_t n):ObjectReference<T>(n)
	{
	}
	template<class D> _R(const _R<D>& r):ObjectReference<T>(r.getPtr())
	{
		if(ObjectReference<T>::m)
			ObjectReference<T>::m->incRef();
	}
};

template<class T>
_R<T> _MR(T* a)
{
	return _R<T>(a);
}

template<class T>
class NonNullObjectReference
{
private:
	NonNullObjectReference(NonNullObjectReference<T>&&) = delete;
protected:
	T* m;
public:
	explicit NonNullObjectReference(T* o):m(o)
	{
		assert(m);
	}
	NonNullObjectReference(const NonNullObjectReference<T>& r):m(r.m)
	{
		m->incRef();
	}
	template<class D> explicit NonNullObjectReference(const ObjectReference<D>& r):m(r.m)
	{
		assert(m);
		m->incRef();
	}
	template<class D> NonNullObjectReference(const NonNullObjectReference<D>& r):m(r.getPtr())
	{
		m->incRef();
	}
	~NonNullObjectReference()
	{
		m->decRef();
	}
	T* operator->() const {return m;}
	T* getPtr() const { return m; }
	bool isNull() const { return false; }
};

//Shorthand notation
template<class T>
class _NR: public NonNullObjectReference<T>
{
public:
	explicit _NR(T* o):NonNullObjectReference<T>(o)
	{
	}
	_NR(const _NR<T>& r):NonNullObjectReference<T>(r)
	{
	}
	template<class D> _NR(const _NR<D>& r):NonNullObjectReference<T>(r)
	{
	}
	template<class D> explicit _NR(const _R<D>& r):NonNullObjectReference<T>(r)
	{
	}
};

template<class T>
_NR<T> _MNR(T* a)
{
	return _NR<T>(a);
}

};
