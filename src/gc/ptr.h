/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef GC_PTR_H
#define GC_PTR_H 1

#include <cassert>
#include <stdexcept>

#include "utils/optional.h"
#include "utils/type_traits.h"

namespace lightspark
{

template<typename T>
class GcPtr;
class GcResource;
template<typename T>
class NullableGcPtr;


struct NullGcType {};
static constexpr NullGcType NullGc {};

template<typename T>
constexpr GcPtr<T> makeGcPtr(T* ptr);
template<typename T>
constexpr NullableGcPtr<T> makeNullableGcPtr(T* ptr);

// A non-null garbage collected pointer.
template<typename T>
class GcPtr
{
private:
	static_assert
	(
		std::is_base_of<GcResource, T>::value,
		"GcPtr: `T` must be a base of `GcResource`"
	);

	template<typename U>
	using CanConvert = std::is_convertible<U, T>;

	T* ptr;
public:
	explicit GcPtr(T* _ptr) : ptr(_ptr)
	{
		if (ptr != nullptr)
			return;
		throw std::runtime_error("GcPtr: `ptr` must be non-null.");
	}

	constexpr GcPtr(const GcPtr& _ptr) : ptr(_ptr.getPtr()) {}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr GcPtr(const GcPtr<U>& _ptr) : ptr(_ptr.getPtr()) {}
	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr GcPtr(const NullableGcPtr<U>& _ptr);

	~GcPtr() {}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr GcPtr& operator=(const GcPtr<U>& _ptr)
	{
		ptr = _ptr.getPtr();
		return *this;
	}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr GcPtr& operator=(const NullableGcPtr<U>& _ptr);

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator==(const GcPtr<U>& _ptr) const
	{
		return ptr == _ptr.getPtr();
	}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator!=(const GcPtr<U>& _ptr) const
	{
		return ptr != _ptr.getPtr();
	}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator==(const NullableGcPtr<U>& _ptr) const;
	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator!=(const NullableGcPtr<U>& _ptr) const;

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator==(U* _ptr) const { return ptr == _ptr; }
	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator!=(U* _ptr) const { return ptr != _ptr; }

	// Needed for `std::map`, and other sorted map containers.
	constexpr bool operator<(const GcPtr& _ptr) const
	{
		return ptr < _ptr.getPtr();
	}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr GcPtr<U> cast() const
	{
		return makeGcPtr(static_cast<U*>(ptr));
	}

	template<typename U, EnableIf<Conj
	<
		CanConvert<U>,
		std::is_const<U>
	>, bool> = false>
	constexpr GcPtr<RemoveCV<U>> constCast() const
	{
		return makeGcPtr(const_cast<RemoveCV<U*>>(ptr));
	}

	constexpr T* getPtr() const { return ptr; }
	constexpr T* operator->() const { return ptr; }
	constexpr const T& operator*() const { return *ptr; }
	constexpr T& operator*() { return *ptr; }

	constexpr NullableGcPtr<T> asNullable() const;
};

// A garbage collected pointer.
template<typename T>
class NullableGcPtr
{
private:
	static_assert
	(
		std::is_base_of<GcResource, T>::value,
		"NullableGcPtr: `T` must be a base of `GcResource`"
	);

	template<typename U>
	using CanConvert = std::is_convertible<U, T>;

	T* ptr;
public:
	constexpr NullableGcPtr() : ptr(nullptr) {}
	constexpr explicit NullableGcPtr(T* _ptr) : ptr(_ptr) {}

	constexpr NullableGcPtr(const NullableGcPtr& _ptr) :
	ptr(_ptr.getPtr()) {}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr NullableGcPtr(const NullableGcPtr<U>& _ptr) :
	ptr(_ptr.getPtr()) {}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr NullableGcPtr(const GcPtr<U>& _ptr) :
	ptr(_ptr.getPtr()) {}

	~NullableGcPtr() {}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr NullableGcPtr& operator=(const NullableGcPtr<U>& _ptr)
	{
		ptr = _ptr.getPtr();
		return *this;
	}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator==(const NullableGcPtr<U>& _ptr) const
	{
		return ptr == _ptr.getPtr();
	}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator!=(const NullableGcPtr<U>& _ptr) const
	{
		return ptr != _ptr.getPtr();
	}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator==(const GcPtr<U>& _ptr) const
	{
		return ptr == _ptr.getPtr();
	}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator!=(const GcPtr<U>& _ptr) const
	{
		return ptr != _ptr.getPtr();
	}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator==(U* _ptr) const { return ptr == _ptr; }
	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr bool operator!=(U* _ptr) const { return ptr != _ptr; }

	constexpr bool operator==(NullGcType) const { ptr == nullptr; }
	constexpr bool operator!=(NullGcType) const { ptr != nullptr; }

	// Needed for `std::map`, and other sorted map containers.
	constexpr bool operator<(const NullableGcPtr& _ptr) const
	{
		return ptr < _ptr.getPtr();
	}

	template<typename U, EnableIf<CanConvert<U>::value, bool> = false>
	constexpr NullableGcPtr<U> cast() const
	{
		return makeNullableGcPtr(static_cast<U*>(ptr));
	}

	template<typename U, EnableIf<Conj
	<
		CanConvert<U>,
		std::is_const<U>
	>, bool> = false>
	constexpr NullableGcPtr<RemoveCV<U>> constCast() const
	{
		return makeNullableGcPtr(const_cast<RemoveCV<U*>>(ptr));
	}

	constexpr bool isNull() const { return ptr == nullptr; }
	constexpr T* getPtr() const { return ptr; }
	constexpr T* operator->() const
	{
		if (ptr != nullptr)
			return ptr;
		throw std::runtime_error("NullableGcPtr: Null pointer access.");
	}
	constexpr const T& operator*() const { return *ptr; }
	constexpr T& operator*() { return *ptr; }

	constexpr Optional<const T&> asOpt() const
	{
		if (ptr == nullptr)
			return nullOpt;
		return makeOptionalRef(ptr);
	}

	constexpr Optional<T&> asOpt()
	{
		if (ptr == nullptr)
			return nullOpt;
		return makeOptionalRef(ptr);
	}
};

// Shorthand notation.
template<typename T>
using _GC = GcPtr<T>;
template<typename T>
using _NGC = GcPtr<T>;
#define _MGC(ptr) makeGcPtr(ptr)
#define _MNGC(ptr) makeNullableGcPtr(ptr)

template<typename T>
inline std::ostream& operator<<(std::ostream& s, const _GC<T>& ptr)
{
	return s << "GcPtr: " << *ptr;
}

template<typename T>
inline std::ostream& operator<<(std::ostream& s, const _NGC<T>& ptr)
{
	s << "NullableGcPtr: ";
	if (ptr.isNull())
		return s << "null";
	return s << *ptr;
}

template<typename T>
constexpr _GC<T> makeGcPtr(T* ptr)
{
	return _GC<T>(ptr);
}

template<typename T>
constexpr _NGC<T> makeNullableGcPtr(T* ptr)
{
	return _NGC<T>(ptr);
}

template<typename T>
template<typename U, EnableIf<std::is_convertible<U, T>::value, bool>>
constexpr GcPtr<T>::GcPtr(const _NGC<U>& _ptr) : ptr(_ptr.getPtr())
{
	if (ptr != nullptr)
		return;
	throw std::runtime_error("GcPtr: `ptr` must be non-null.");
}

template<typename T>
template<typename U, EnableIf<std::is_convertible<U, T>::value, bool>>
constexpr GcPtr<T>& GcPtr<T>::operator=(const _NGC<U>& _ptr)
{
	if (!_ptr.isNull())
	{
		ptr = _ptr.getPtr();
		return *this;
	}
	throw std::runtime_error("GcPtr: `ptr` must be non-null.");
}

template<typename T>
template<typename U, EnableIf<std::is_convertible<U, T>::value, bool>>
constexpr bool GcPtr<T>::operator==(const _NGC<U>& _ptr) const
{
	return ptr == _ptr.getPtr();
}

template<typename T>
template<typename U, EnableIf<std::is_convertible<U, T>::value, bool>>
constexpr bool GcPtr<T>::operator!=(const _NGC<U>& _ptr) const
{
	return ptr != _ptr.getPtr();
}

template<typename T>
constexpr _NGC<T> GcPtr<T>::asNullable() const
{
	return _NGC<T>(*this);
}

}
#endif /* GC_PTR_H */
