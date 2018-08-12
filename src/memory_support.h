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

#ifndef MEMORY_SUPPORT_H
#define MEMORY_SUPPORT_H 1

#include "compat.h"
#include "tiny_string.h"
#include <malloc.h>

namespace lightspark
{

#ifdef MEMORY_USAGE_PROFILING
class MemoryAccount;
DLL_PUBLIC MemoryAccount* getUnaccountedMemoryAccount();

class MemoryAccount
{
public:
	tiny_string name;
	ATOMIC_INT32(bytes);
	/*
	 * The name pointer is not copied and must survive
	 */
	MemoryAccount(const tiny_string& n):name(n),bytes(0){}
	void addBytes(uint32_t b)
	{
		ATOMIC_ADD(this->bytes, b);
	}
	void removeBytes(uint32_t b)
	{
		ATOMIC_SUB(this->bytes, b);
	}
};

//Since global overloaded delete can't be called explicitly, memory reporting is only
//enabled using class inheritance
class memory_reporter
{
private:
	struct objData
	{
		uint32_t objSize;
		MemoryAccount* memoryAccount;
	};
public:
	//Placement new and delete
	inline void* operator new( size_t size, void *p )
	{
		return p;
	}
	inline void operator delete( void*, void* )
	{
	}
	//Regular allocator
	inline void* operator new( size_t size, MemoryAccount* m)
	{
		//Prepend some internal data.
		//Adding the data to the object itself would not work
		//since it can be reset by the constructors
		objData* ret=reinterpret_cast<objData*>(malloc(size+sizeof(objData)));
		if(!m)
			m = getUnaccountedMemoryAccount();
		m->addBytes(size);
		ret->objSize = size;
		ret->memoryAccount = m;
		return ret+1;
	}
	inline void operator delete( void* obj )
	{
		//Get back the metadata
		objData* th=reinterpret_cast<objData*>(obj)-1;
		th->memoryAccount->removeBytes(th->objSize);
		free(th);
	}
};

template<class T>
class reporter_allocator: protected std::allocator<T>
{
template<class U>
friend class reporter_allocator;
template<class U, class V>
friend bool operator==(const reporter_allocator<U>& a, const reporter_allocator<V>& b);
template<class U, class V>
friend bool operator!=(const reporter_allocator<U>& a, const reporter_allocator<V>& b);
private:
	MemoryAccount* memoryAccount;
	reporter_allocator();
public:
	typedef typename std::allocator<T>::size_type size_type;
	typedef typename std::allocator<T>::value_type value_type;
	typedef typename std::allocator<T>::difference_type difference_type;
	typedef typename std::allocator<T>::pointer pointer;
	typedef typename std::allocator<T>::const_pointer const_pointer;
	typedef typename std::allocator<T>::reference reference;
	typedef typename std::allocator<T>::const_reference const_reference;
	template<class U>
	struct rebind
	{
		typedef reporter_allocator<U> other;
	};
	reporter_allocator(MemoryAccount* m):memoryAccount(m)
	{
	}
	template<class U>
	reporter_allocator(const reporter_allocator<U>& o):std::allocator<T>(o), memoryAccount(o.memoryAccount)
	{
	}
	pointer allocate(size_type n, std::allocator<void>::const_pointer hint=0)
	{
		if(memoryAccount==NULL)
			memoryAccount=getUnaccountedMemoryAccount();
		memoryAccount->addBytes(n*sizeof(T));
		return (pointer)malloc(n*sizeof(T));
	}
	void deallocate(pointer p, size_type n)
	{
		memoryAccount->removeBytes(n*sizeof(T));
		free(p);
	}
	template<class... args>
	void construct(pointer p, args&&... vals)
	{
		new ((void*)p) T (std::forward<args>(vals)...);
	}
	void destroy(pointer p)
	{
		(reinterpret_cast<T*>(p))->~T();
	}
	size_type max_size() const
	{
		return 0x7fffffff;
	}
};

template<class T, class U>
bool operator==(const reporter_allocator<T>& a,
		const reporter_allocator<U>& b)
{
	return a.memoryAccount == b.memoryAccount;
}

template<class T, class U>
bool operator!=(const reporter_allocator<T>& a,
		const reporter_allocator<U>& b)
{
	return a.memoryAccount != b.memoryAccount;
}

#else //MEMORY_USAGE_PROFILING

class MemoryAccount;

class memory_reporter
{
public:
	//Placement new and delete
	inline void* operator new( size_t size, void *p )
	{
		return p;
	}
	inline void operator delete( void*, void* )
	{
	}
	//Regular allocator
	inline void* operator new( size_t size, MemoryAccount* m)
	{
		return malloc(size);
	}
	inline void operator delete( void* obj )
	{
		free(obj);
	}
};

template<class T>
class reporter_allocator: public std::allocator<T>
{
public:
	template<class U>
	struct rebind
	{
		typedef reporter_allocator<U> other;
	};
	reporter_allocator(MemoryAccount* m)
	{
	}
	template<class U>
	reporter_allocator(const reporter_allocator<U>& o):std::allocator<T>(o)
	{
	}
};

#endif //MEMORY_USAGE_PROFILING

};
#endif /* MEMORY_SUPPORT_H */
