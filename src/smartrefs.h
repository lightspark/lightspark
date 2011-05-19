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

#ifndef _SMARTREFS_H
#define _SMARTREFS_H

namespace lightspark
{

/*
   NOTE: _Always_ define both copy constructor and assignment operator in non templated way.
   Templated versions must be added if copying should be allowed from compatible types.
   The compiler will _not_ use templated versions if the right hand type is the same 
   as the left hand type */

template<class T> class NullableRef;

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
	//Constructible from any compatible reference
	template<class D> Ref(const Ref<D>& r):m(r.getPtr())
	{
		m->incRef();
	}
	template<class D> Ref(const NullableRef<D>& r);
	Ref<T>& operator=(const Ref<T>& r)
	{
		//incRef before decRef to make sure this works even if the pointer is the same
		r.m->incRef();

		m->decRef();

		m=r.m;
		return *this;
	}
	template<class D> Ref<T>& operator=(const Ref<D>& r)
	{
		//incRef before decRef to make sure this works even if the pointer is the same
		r.m->incRef();

		m->decRef();

		m=r.m;
		return *this;
	}
	template<class D> bool operator==(const Ref<D>& r) const
	{
		return m==r.getPtr();
	}
	template<class D> bool operator==(const NullableRef<D>&r) const;
	bool operator==(T* r) const
	{
		return m==r;
	}
	//Order operator for Dictionary map
	bool operator<(const Ref<T>& r) const
	{
		return m<r.m;
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
	NullableRef(): m(NULL) {}
	explicit NullableRef(T* o):m(o){}
	NullableRef(NullRef_t):m(NULL){}
	NullableRef(const NullableRef& r):m(r.m)
	{
		if(m)
			m->incRef();
	}
	//Constructible from any compatible nullable reference and reference
	template<class D> NullableRef(const NullableRef<D>& r):m(r.getPtr())
	{
		if(m)
			m->incRef();
	}
	template<class D> NullableRef(const Ref<D>& r):m(r.getPtr())
	{
		//The right hand Ref object is guaranteed to be valid
		m->incRef();
	}
	NullableRef<T>& operator=(const NullableRef<T>& r)
	{
		if(r.m)
			r.m->incRef();

		if(m)
			m->decRef();
		m=r.m;
		return *this;
	}
	template<class D> NullableRef<T>& operator=(const NullableRef<D>& r)
	{
		if(r.getPtr())
			r->incRef();

		if(m)
			m->decRef();
		m=r.getPtr();
		return *this;
	}
	template<class D> NullableRef<T>& operator=(const Ref<D>& r)
	{
		r.getPtr()->incRef();

		if(m)
			m->decRef();
		m=r.getPtr();
		return *this;
	}
	template<class D> bool operator==(const NullableRef<D>& r) const
	{
		return m==r.getPtr();
	}
	template<class D> bool operator==(const Ref<D>& r) const
	{
		return m==r.getPtr();
	}
	bool operator==(T* r) const
	{
		return m==r;
	}
	template<class D> bool operator!=(const NullableRef<D>& r) const
	{
		return m!=r.getPtr();
	}
	template<class D> bool operator!=(const Ref<D>& r) const
	{
		return m!=r.getPtr();
	}
	~NullableRef()
	{
		if(m)
			m->decRef();
	}
	T* operator->() const {return m;}
	T* getPtr() const { return m; }
	bool isNull() const { return m==NULL; }
	void reset()
	{
		if(m)
			m->decRef();
		m=NULL;
	}
	void fakeRelease()
	{
		m=NULL;
	}
};

//Shorthand notation
#define _NR NullableRef

template<class T>
Ref<T> _MR(NullableRef<T> a)
{
	return Ref<T>(a);
}

template<class T>
NullableRef<T> _MNR(T* a)
{
	return NullableRef<T>(a);
}

template<class T> template<class D> Ref<T>::Ref(const NullableRef<D>& r):m(r.getPtr())
{
	assert(m);
	m->incRef();
}

template<class T> template<class D> bool Ref<T>::operator==(const NullableRef<D>&r) const
{
	return m==r.getPtr();
}

};

#endif
