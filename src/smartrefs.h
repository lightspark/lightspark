/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2011-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef SMARTREFS_H
#define SMARTREFS_H 1

#include <stdexcept>
#include "compat.h"

namespace lightspark
{

class RefCountable {
private:
	ATOMIC_INT32(ref_count);
	bool isConstant:1;
	bool inDestruction:1;
	bool cached:1;
protected:
	RefCountable() : ref_count(1),isConstant(false),inDestruction(false),cached(false) {}

public:
	virtual ~RefCountable() {}

	int getRefCount() const { return ref_count; }
	inline bool isLastRef() const { return !isConstant && ref_count==1; }
	inline void setConstant()
	{
		isConstant=true;
	}
	inline bool getConstant() const { return isConstant; }
	inline bool getInDestruction() const { return inDestruction; }
	inline void resetInDestruction() { inDestruction=false; }
	inline bool getCached() const { return cached; }
	inline void setCached() { cached=true; }
	inline void resetCached() { cached=false; }
	inline void incRef()
	{
		if (!isConstant)
			++ref_count;
	}
	bool handleDestruction()
	{
		if (inDestruction)
			return true;
		inDestruction = true;
		ref_count=1;
		if (destruct())
		{
			//Let's make refcount very invalid
			ref_count=-1024;
			inDestruction = false;
			delete this;
		}
		else
			inDestruction = false;
		return true;
	}
	inline bool decRef()
	{
		if (!isConstant && !cached)
		{
			assert(ref_count>0);
			if (ref_count == 1)
				return handleDestruction();
			else
				--ref_count;
		}
		return cached;
	}
	virtual bool destruct()
	{
		return true;
	}
	inline void resetRefCount()
	{
		ref_count=1;
	}
};

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

		T* old=m;
		m=r.m;

		//decRef as the very last function call, because it
		//may cause this Ref to be deleted (if old owns this Ref)
		old->decRef();

		return *this;
	}
	template<class D> inline Ref<T>& operator=(const Ref<D>& r)
	{
		//incRef before decRef to make sure this works even if the pointer is the same
		r.m->incRef();

		T* old=m;
		m=r.m;

		old->decRef();

		return *this;
	}
	template<class D> inline bool operator==(const Ref<D>& r) const
	{
		return m==r.getPtr();
	}
	template<class D> inline bool operator!=(const Ref<D>& r) const
	{
		return m!=r.getPtr();
	}
	template<class D> inline bool operator==(const NullableRef<D>&r) const;
	inline bool operator==(T* r) const
	{
		return m==r;
	}
	//Order operator for Dictionary map
	inline bool operator<(const Ref<T>& r) const
	{
		return m<r.m;
	}
	template<class D> inline Ref<D> cast() const
	{
		D* p = static_cast<D*>(m);
		p->incRef();
		return _MR(p);
	}
	~Ref()
	{
		m->decRef();
	}
	inline T* operator->() const 
	{
		return m;
	}
	inline T* getPtr() const 
	{ 
		return m; 
	}
};

#define _R Ref

template<class T>
inline std::ostream& operator<<(std::ostream& s, const Ref<T>& r)
{
	return s << "Ref: ";
}

template<class T>
Ref<T> _MR(T* a)
{
	return Ref<T>(a);
}

class NullRef_t
{
};

static NullRef_t NullRef;

template<class T>
class NullableRef
{
private:
	T* m;
public:
	NullableRef(): m(nullptr) {}
	explicit NullableRef(T* o):m(o){}
	NullableRef(NullRef_t):m(nullptr){}
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
	inline NullableRef<T>& operator=(const NullableRef<T>& r)
	{
		if(r.m)
			r.m->incRef();

		T* old=m;
		m=r.m;
		if(old)
			old->decRef();
		return *this;
	}
	template<class D> inline NullableRef<T>& operator=(const NullableRef<D>& r)
	{
		if(r.getPtr())
			r->incRef();

		T* old=m;
		m=r.getPtr();
		if(old)
			old->decRef();
		return *this;
	}
	template<class D> inline NullableRef<T>& operator=(const Ref<D>& r)
	{
		r.getPtr()->incRef();

		T* old=m;
		m=r.getPtr();
		if(old)
			old->decRef();
		return *this;
	}
	template<class D> inline bool operator==(const NullableRef<D>& r) const
	{
		return m==r.getPtr();
	}
	template<class D> inline bool operator==(const Ref<D>& r) const
	{
		return m==r.getPtr();
	}
	template<class D>
	inline bool operator==(const D* r) const
	{
		return m==r;
	}
	inline bool operator==(NullRef_t) const
	{
		return m==nullptr;
	}
	template<class D> inline bool operator!=(const NullableRef<D>& r) const
	{
		return m!=r.getPtr();
	}
	template<class D> inline bool operator!=(const Ref<D>& r) const
	{
		return m!=r.getPtr();
	}
	template<class D>
	inline bool operator!=(const D* r) const
	{
		return m!=r;
	}
	inline bool operator!=(NullRef_t) const
	{
		return m!=nullptr;
	}
	/*explicit*/ operator bool() const
	{
		return m != nullptr;
	}
	~NullableRef()
	{
		if(m)
			m->decRef();
	}
	inline T* operator->() const
	{
		if(m != nullptr)
			return m;
		else
			throw std::runtime_error("LS smart pointer: NULL pointer access");
	}
	inline T* getPtr() const { return m; }
	inline bool isNull() const { return m==nullptr; }
	inline void reset()
	{
		T* old=m;
		m=nullptr;
		if(old)
			old->decRef();
	}
	inline void fakeRelease()
	{
		m=nullptr;
	}
	template<class D> inline NullableRef<D> cast() const
	{
		if(!m)
			return NullRef;
		D* p = static_cast<D*>(m);
		p->incRef();
		return _MNR(p);
	}
};

//Shorthand notation
#define _NR NullableRef

template<class T>
inline std::ostream& operator<<(std::ostream& s, const NullableRef<T>& r)
{
	s << "NullableRef: ";
	if(r.isNull())
		return s << "null";
	else
		return s << *r.getPtr();
}

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

}

#endif /* SMARTREFS_H */
