/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef UTILS_SPAN_H
#define UTILS_SPAN_H 1

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>

#include "utils/type_traits.h"
#include "utils/utility.h"

// Based on `tcb::span` from https://github.com/tcbrindle/span
namespace lightspark
{

constexpr size_t dynExtent = SIZE_MAX;

template<typename T, size_t N = dynExtent>
class Span;

namespace Detail
{

template<typename T, size_t N>
class SpanStorage
{
private:
	T* data;
	static constexpr size_t size = N;
public:
	template<EnableIf<!std::is_const<T>::value, bool> = false>
	constexpr SpanStorage
	(
		T* _data = nullptr,
		size_t _size = 0
	) noexcept : data(_data) {}

	constexpr SpanStorage
	(
		const T* _data = nullptr,
		size_t _size = 0
	) noexcept : data(const_cast<T*>(_data)) {}

	constexpr T* getData() const { return data; }
	constexpr T* getData() { return data; }
	constexpr size_t getSize() const { return size; }
};

template<typename T>
class SpanStorage<T, dynExtent>
{
private:
	T* data;
	size_t size;
public:
	constexpr SpanStorage
	(
		T* _data = nullptr,
		size_t _size = 0
	) noexcept : data(_data), size(_size) {}

	constexpr T* getData() const { return data; }
	constexpr T* getData() { return data; }
	constexpr size_t getSize() const { return size; }
};

// Implementation of C++17's `std::{size,data}()`.
template<typename T>
constexpr auto size(const T& cont) -> decltype(cont.size())
{
	return cont.size();
}

template<typename T, size_t N>
constexpr size_t size(const T(&array)[N])
{
	return N;
}

template<typename T>
constexpr auto data(const T& cont) -> decltype(cont.data())
{
	return cont.data();
}

template<typename T>
constexpr auto data(T& cont) -> decltype(cont.data())
{
	return cont.data();
}

template<typename T, size_t N>
constexpr T* data(T(&array)[N])
{
	return array;
}

template<typename T>
constexpr const T* data(const std::initializer_list<T>& list)
{
	return list.begin();
}

template<typename T>
struct IsSpan : std::false_type {};
template<typename T, size_t N>
struct IsSpan<Span<T, N>> : std::true_type {};
template<typename T>
struct IsStdArray : std::false_type {};
template<typename T, size_t N>
struct IsStdArray<std::array<T, N>> : std::true_type {};

template<typename, typename = void>
struct HasSizeAndData : std::false_type {};
template<typename T>
struct HasSizeAndData<T, Void
<
	decltype(Detail::size(std::declval<T>())),
	decltype(Detail::data(std::declval<T>()))
>> : std::true_type {};

template<typename T, typename U = RemoveCVRef<T>>
struct IsContainer : Conj
<
	Neg<IsSpan<U>>,
	Neg<IsStdArray<U>>,
	Neg<IsArray<U>>,
	HasSizeAndData<T>
> {};

template<typename T, typename E, typename U = RemoveCVRef<T>>
struct IsValidElemType : Conj
<
	Neg<IsSame<RemoveCV
	<
		decltype(Detail::data(std::declval<T>()))
	>, void>>,
	std::is_convertible<RemovePtr
	<
		decltype(Detail::data(std::declval<T>()))
	> (*)[], E (*)[]>
> {};

template<typename, typename = size_t>
struct IsComplete : std::false_type {};
template<typename T>
struct IsComplete<T, decltype(sizeof(T))> : std::true_type {};

};

template<typename T, size_t N>
class Span
{
public:
	using ElemType = T;
	using ValueType = RemoveCV<T>;
	using SizeType = size_t;
	using DiffType = ptrdiff_t;
	using Ptr = T*;
	using ConstPtr = const T*;
	using Ref = T&;
	using ConstRef = const T&;
	using Iter = Ptr;
	using RevIter = std::reverse_iterator<Iter>;
private:
	static_assert
	(
		std::is_object<T>::value,
		"`Span`'s element type must be an object type "
		"(not a reference, or `void`)."
	);

	static_assert
	(
		Detail::IsComplete<T>::value,
		"`Span`'s element type must be a complete type "
		"(not a forward declaration)."
	);

	static_assert
	(
		!std::is_abstract<T>::value,
		"`Span`'s element type can't be an abstract class."
	);

	using StorageType = Detail::SpanStorage<T, N>;
	StorageType storage;
	#define EXPECT(exceptionType, expr) \
		if (!(expr)) \
			throw exceptionType("Expected: `" #expr "`.")

	template<size_t... Is>
	constexpr ElemType atLEImpl(SizeType idx, std::index_sequence<Is...>) const
	{
		static_assert
		(
			sizeof...(Is) == sizeof(ElemType),
			"Span::atLEImpl(): Index sequence size must be "
			"the same size as `ElemType`."
		);

		ValueType ret(0);
		auto bytes = subSpan(idx, 1).asBytes();
		auto it = bytes.begin();

		// Perform the little endian read by shifting in bytes to the
		// left, which is both portable, and efficient.
		(void)std::initializer_list<int> { (ret |= *it++ << (Is * CHAR_BIT), 0)... };
		return ret;
	}

	template<size_t... Is>
	constexpr ElemType atBEImpl(SizeType idx, std::index_sequence<Is...>) const
	{
		static_assert
		(
			sizeof...(Is) == sizeof(ElemType),
			"Span::atBEImpl(): Index sequence size must be "
			"the same size as `ElemType`."
		);

		ValueType ret(0);
		auto bytes = subSpan(idx, 1).asBytes();
		auto it = bytes.rbegin();

		// Perform the big endian read by shifting in bytes to the left,
		// which is both portable, and efficient.
		(void)std::initializer_list<int> { (ret |= *it++ << (Is * CHAR_BIT), 0)... };
		return ret;
	}
public:
	// STL compatible type names.
	using element_type = ElemType;
	using value_type = ValueType;
	using size_type = SizeType;
	using difference_type = DiffType;
	using pointer = Ptr;
	using const_pointer = ConstPtr;
	using reference = Ref;
	using const_reference = ConstRef;
	using iterator = Iter;
	using reverse_iterator = RevIter;

	static constexpr SizeType extent = N;

	typename<size_t M = N, EnableIf<
	(
		M == dynExtent ||
		M <= 0
	), bool> = false>
	constexpr Span() noexcept {}

	constexpr Span(Iter data, SizeType size) : storage(data, size)
	{
		EXPECT(std::length_error,
		(
			N == dynExtent ||
			size == N
		));
	}

	constexpr Span(Iter first, Iter last) : storage(first, last - first)
	{
		EXPECT(std::length_error,
		(
			N == dynExtent ||
			(last - first) == static_cast<ptrdiff_t>(N)
		));
	}

	template<size_t M, size_t E = N, EnableIf<
	(
		(E == dynExtent || M == E) &&
		!std::is_const<T>::value &&
		Detail::IsValidElemType<T (&)[M], T>::value
	), bool> = false>
	constexpr Span(T (&array)[M]) noexcept : storage(array, M) {}

	template<size_t M, size_t E = N, EnableIf<
	(
		(E == dynExtent || M == E) &&
		Detail::IsValidElemType<const T (&)[M], const T>::value
	), bool> = false>
	constexpr Span(const T (&array)[M]) noexcept : storage(array, M) {}

	template<size_t M, size_t E = N, EnableIf<
	(
		(E == dynExtent || M == E) &&
		Detail::IsValidElemType<std::array<T, M>, T>::value
	), bool> = false>
	constexpr Span
	(
		std::array<T, M>& array
	) noexcept : storage(array.data(), M) {}

	template<size_t M, size_t E = N, EnableIf<
	(
		(E == dynExtent || M == E) &&
		Detail::IsValidElemType<const std::array<T, M>, T>::value
	), bool> = false>
	constexpr Span
	(
		const std::array<T, M>& array
	) noexcept : storage(array.data(), M) {}

	template<typename U, size_t E = N, EnableIf<
	(
		E == dynExtent &&
		Detail::IsContainer<U>::value &&
		Detail::IsValidElemType<U&, T>::value
	), bool> = false>
	constexpr Span(U& cont) : storage
	(
		Detail::data(cont),
		Detail::size(cont)
	) {}

	template<typename U, size_t E = N, EnableIf<
	(
		E == dynExtent &&
		Detail::IsContainer<U>::value &&
		Detail::IsValidElemType<const U&, T>::value
	), bool> = false>
	constexpr Span(const U& cont) : storage
	(
		Detail::data(cont),
		Detail::size(cont)
	) {}

	constexpr Span(const Span& other) noexcept = default;

	template<typename U, size_t M, EnableIf<
	(
		(N == dynExtent || M == dynExtent || N == M) &&
		std::is_convertible<U (*)[], T (*)[]>::value
	), bool> = false>
	constexpr Span(const Span<U, M>& other) : storage
	(
		other.getData(),
		other.getSize()
	) {}

	~Span() noexcept = default;

	constexpr Span& operator=(const Span& other) noexcept = default;

	template<size_t M>
	constexpr Span<T, M> getFirst() const
	{
		EXPECT(std::range_error, M <= getSize());
		return { getData(), M };
	}

	template<size_t M>
	constexpr Span<T, M> getLast() const
	{
		EXPECT(std::range_error, M <= getSize());
		return { getData() + (getSize() - M), M };
	}

	template<size_t offset, size_t M = dynExtent>
	constexpr auto subSpan() const
	{
		EXPECT(std::range_error,
		(
			offset <= getSize() ||
			(offset + M) <= getSize()
		));
		return
		{
			getData() + offset,
			M != dynExtent ? M : getSize() - offset
		};
	}

	constexpr Span<T, dynExtent> getFirst(SizeType size) const
	{
		EXPECT(std::range_error, size <= getSize());
		return { getData(), size };
	}

	constexpr Span<T, dynExtent> getLast(SizeType size) const
	{
		EXPECT(std::range_error, size <= getSize());
		return { getData() + (getSize() - size), size };
	}

	constexpr Span<T, dynExtent> subSpan
	(
		SizeType offset,
		SizeType size = dynExtent
	) const
	{
		EXPECT(std::range_error, size <= getSize() &&
		(
			size == dynExtent ||
			(offset + size) <= getSize()
		));
		return
		{
			getData() + offset,
			size != dynExtent ? size : getSize() - offset
		};
	}

	constexpr SizeType getSize() const noexcept
	{
		return storage.getSize();
	}

	template<typename U>
	constexpr SizeType getSizeAs() const noexcept
	{
		return getByteSize() / sizeof(U);
	}

	constexpr SizeType getByteSize() const noexcept
	{
		return getSize() * sizeof(T);
	}

	constexpr bool empty() const noexcept { return getSize() == 0; }

	constexpr Ref at(SizeType i) const
	{
		if (i >= getSize())
			throw std::out_of_range("Span::at(): `i >= getSize()`");
		return getData()[i];
	}

	// Returns a reference of type `U` to an element at the specified
	// index.
	//
	// NOTE: The index read is in terms of `T`, not `U`, which means it
	// might perform an unaligned `U` read.
	template<typename U>
	constexpr U& atAs(SizeType i) const
	{
		auto byteIdx = i * sizeof(T);
		if ((byteIdx / sizeof(U)) >= getSizeAs<U>())
			throw std::out_of_range("Span::atAs(): `i >= getSizeAs<U>()`");
		return subSpan(i, 1).as<U>().front();
	}

	// Performs a little endian read of type `U` at the specified index.
	//
	// NOTE: The index read is in terms of `T`, not `U`, but shouldn't
	// perform an unaligned read, due to reading it out as bytes.
	template<typename U>
	constexpr U atLE(SizeType idx) const
	{
		auto byteIdx = idx * sizeof(T);
		if ((byteIdx / sizeof(U)) >= getSizeAs<U>())
			throw std::out_of_range("Span::atLE<U>(): `idx >= getSizeAs<U>()`");
		return subSpan(idx, 1).template as<U>().atLE(0);
	}

	template<typename U, EnableIf<IsSame<U, float>::value, bool> = false>
	constexpr float atLE(SizeType idx) const
	{
		auto ret = atLE<uint32_t>(idx);
		return *static_cast<float*>(&ret);
	}

	template<typename U, EnableIf<IsSame<U, double>::value, bool> = false>
	constexpr double atLE(SizeType idx) const
	{
		auto ret = atLE<uint64_t>(idx);
		return *static_cast<double*>(&ret);
	}

	// Performs a little endian read at the specified index.
	constexpr ElemType atLE(SizeType idx) const
	{
		if (idx >= getSize())
			throw std::out_of_range("Span::atLE(): `idx >= getSize()`");

		return atLEImpl
		(
			idx,
			std::make_index_sequence<sizeof(ElemType)> {}
		);
	}

	// Performs a big endian read of type `U` at the specified index.
	//
	// NOTE: The index read is in terms of `T`, not `U`, but shouldn't
	// perform an unaligned read, due to reading it out as bytes.
	template<typename U>
	constexpr U atBE(SizeType idx) const
	{
		auto byteIdx = idx * sizeof(T);
		if ((byteIdx / sizeof(U)) >= getSizeAs<U>())
			throw std::out_of_range("Span::atBE<U>(): `idx >= getSizeAs<U>()`");
		return subSpan(idx, 1).template as<U>().atBE(0);
	}

	template<typename U, EnableIf<IsSame<U, float>::value, bool> = false>
	constexpr float atBE(SizeType idx) const
	{
		auto ret = atBE<uint32_t>(idx);
		return *static_cast<float*>(&ret);
	}

	template<typename U, EnableIf<IsSame<U, double>::value, bool> = false>
	constexpr double atBE(SizeType idx) const
	{
		auto ret = atBE<uint64_t>(idx);
		return *static_cast<double*>(&ret);
	}

	// Performs a big endian read at the specified index.
	constexpr ElemType atBE(SizeType idx) const
	{
		if (idx >= getSize())
			throw std::out_of_range("Span::atBE(): `idx >= getSize()`");

		return atBEImpl
		(
			idx,
			std::make_index_sequence<sizeof(ElemType)> {}
		);
	}

	constexpr Ref operator[](SizeType i) const
	{
		EXPECT(std::out_of_range, i < getSize());
		return getData()[i];
	}

	constexpr Ref front() const
	{
		EXPECT(std::length_error, !empty());
		return *getData();
	}

	constexpr Ref back() const
	{
		EXPECT(std::length_error, !empty());
		return getData()[getSize() - 1];
	}

	constexpr Ptr getData() const noexcept
	{
		return storage.getData();
	}

	constexpr Iter begin() const noexcept { return getData(); }
	constexpr Iter end() const noexcept
	{
		return &getData()[getSize()];
	}

	RevIter rbegin() const noexcept { return RevIter(end()); }
	RevIter rend() const noexcept { return RevIter(begin()); }

	using ByteSpan = Span
	<
		uint8_t,
		N == dynExtent ? dynExtent : N * sizeof(T)
	>;

	constexpr ByteSpan asBytes() const noexcept
	{
		return
		{
			reinterpret_cast<const uint8_t*>(getData()),
			getByteSize()
		};
	}

	constexpr ByteSpan asBytes() noexcept
	{
		return
		{
			reinterpret_cast<uint8_t*>(getData()),
			getByteSize()
		};
	}

	template<typename U>
	constexpr Span
	<
		U,
		N != dynExtent ?
		(N * sizeof(T)) / sizeof(U) :
		dynExtent
	> as() const noexcept
	{
		return
		{
			reinterpret_cast<const U*>(getData()),
			getByteSize() / sizeof(U)
		};
	}

	template<typename U>
	constexpr Span
	<
		U,
		N != dynExtent ?
		(N * sizeof(T)) / sizeof(U) :
		dynExtent
	> as() noexcept
	{
		return
		{
			reinterpret_cast<U*>(getData()),
			getByteSize() / sizeof(U)
		};
	}

	#undef EXPECT
};

template<typename T, size_t N>
constexpr Span<T, N> makeSpan(const Span<T, N>& span) noexcept
{
	return span;
}

template<typename T, size_t N>
constexpr Span<T, N> makeSpan(T (&array)[N]) noexcept
{
	return { array };
}

template<typename T, size_t N>
constexpr Span<T, N> makeSpan(const T (&array)[N]) noexcept
{
	return { array };
}

template<typename T, size_t N>
constexpr Span<T, N> makeSpan(std::array<T, N>& array) noexcept
{
	return { array };
}

template<typename T, size_t N>
constexpr Span<T, N> makeSpan(const std::array<T, N>& array) noexcept
{
	return { array };
}

template<typename T>
constexpr Span<RemoveRef
<
	decltype(*Detail::data(std::declval<T&>()))
>> makeSpan(T& cont) noexcept
{
	return { cont };
}

template<typename T>
constexpr Span
<
	const typename T::value_type
> makeSpan(const T& cont) noexcept
{
	return { cont };
}

};
#endif /* UTILS_SPAN_H */
