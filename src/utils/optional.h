/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef UTILS_OPTIONAL_H
#define UTILS_OPTIONAL_H 1

#include <stdexcept>

#include "utils/type_traits.h"
#include "utils/utility.h"

namespace lightspark
{

template<typename T, bool = std::is_trivially_destructible<T>::value>
struct OptionalStorage
{
	constexpr OptionalStorage() noexcept = default;

	OptionalStorage(const T& value) : exists(true)
	{
		new (&data) T(value);
	}

	OptionalStorage(T&& value) : exists(true)
	{
		new (&data) T(std::move(value));
	}

	template<typename... Args>
	OptionalStorage(InPlaceTag, Args&&... args) : exists(true)
	{
		new (&data) T { std::forward<Args>(args)... };
	}

	template<typename U, typename... Args, EnableIf<std::is_constructible
	<
		T,
		std::initializer_list<U>&,
		Args&&...
	>::value, bool> = false>
	OptionalStorage(InPlaceTag, std::initializer_list<U> list, Args&&... args) : exists(true)
	{
		new (&data) T { list, std::forward<Args>(args)... };
	}

	~OptionalStorage() { clear(); }

	void clear()
	{
		if (exists)
		{
			reinterpret_cast<T&>(data).~T();
			exists = false;
		}
	}

	alignas(T) uint8_t data[sizeof(T)];
	bool exists { false };
};

template<typename T>
struct OptionalStorage<T, true>
{
	constexpr OptionalStorage() noexcept = default;

	OptionalStorage(const T& value) : exists(true)
	{
		new (&data) T(value);
	}

	OptionalStorage(T&& value) : exists(true)
	{
		new (&data) T(std::move(value));
	}

	template<typename... Args>
	OptionalStorage(InPlaceTag, Args&&... args) : exists(true)
	{
		new (&data) T { std::forward<Args>(args)... };
	}

	template<typename U, typename... Args, EnableIf<std::is_constructible
	<
		T,
		std::initializer_list<U>&,
		Args&&...
	>::value, bool> = false>
	OptionalStorage(InPlaceTag, std::initializer_list<U> list, Args&&... args) : exists(true)
	{
		new (&data) T { list, std::forward<Args>(args)... };
	}

	~OptionalStorage() = default;

	void clear() {}

	alignas(T) uint8_t data[sizeof(T)];
	bool exists { false };
};

struct BadOptionalAccess : public std::logic_error
{
	BadOptionalAccess() : std::logic_error("BadOptionalAccess exception") {}
	virtual ~BadOptionalAccess() noexcept {}
};

struct NullOpt
{
	constexpr explicit NullOpt() = default;
};

static constexpr NullOpt nullOpt {};

// Based on `optional` from EASTL.
template<typename T, EnableIf<!std::is_lvalue_reference<T>::value, bool> = false>
class Optional : private OptionalStorage<T>
{
	using Base = OptionalStorage<T>;

	using Base::clear;
	using Base::exists;
	using Base::data;
public:
	using ValueType = T;

	constexpr Optional() noexcept = default;
	constexpr Optional(NullOpt) noexcept {}

	constexpr Optional(const T& value) : Base(value) {}

	constexpr Optional(T&& value) noexcept
	(
		std::is_nothrow_move_constructible<T>::value
	) : Base(value) {}

	Optional(const Optional& other) : Base()
	{
		if (exists = other.exists)
			new(&data) T(other.getValue());
	}

	Optional(Optional&& other) : Base()
	{
		if (exists = other.exists)
			new(&data) T(other.releaseValue());
	}

	template<typename... Args>
	constexpr explicit Optional
	(
		InPlaceTag tag,
		Args&&... args
	) : Base(tag, std::forward<Args>(args)...) {}

	template<typename U, typename... Args, EnableIf<std::is_constructible
	<
		T,
		std::initializer_list<U>&,
		Args&&...
	>::value, bool> = false>
	explicit Optional
	(
		InPlaceTag tag,
		std::initializer_list<U> list,
		Args&&... args
	) : Base(tag, list, std::forward<Args>(args)...) {}

	template<typename U = T, EnableIf<
	(
		std::is_constructible<T, U&&>::value &&
		!std::is_same<RemoveCVRef<U>, InPlaceTag>::value &&
		!std::is_same<RemoveCVRef<U>, Optional>::value
	), bool> = false>
	explicit constexpr Optional(InPlaceTag tag, U&& value) : Base(tag, std::forward<U>(value)) {}

	Optional& operator=(NullOpt)
	{
		reset();
		return *this;
	}

	Optional& operator=(const Optional& other)
	{
		if (exists == other.exists)
		{
			if (exists)
				getValue() = other.getValue();
		}
		else if (exists)
		{
			clear();
			exists = false;
		}
		else
		{
			constructValue(other.getValue());
			exists = false;
		}
		return *this;
	}

	Optional& operator=(Optional&& other) noexcept(noexcept
	(
		std::is_nothrow_move_assignable<T>::value &&
		std::is_nothrow_move_constructible<T>::value
	))
	{
		if (exists == other.exists)
		{
			if (exists)
				getValue() = std::move(other.getValue());
		}
		else if (exists)
		{
			clear();
			exists = false;
		}
		else
		{
			constructValue(std::move(other.getValue()));
			exists = false;
		}
		return *this;
	}

	template<typename U, EnableIf<std::is_same<Decay<U>, T>::value, bool> = false>
	Optional& operator=(U&& value)
	{
		if (exists)
			getValue() = std::forward<U>(value);
		else
		{
			exists = true;
			constructValue(std::forward<U>(value));
		}
		return *this;
	}

	constexpr explicit operator bool() const { return exists; }
	constexpr bool hasValue() const noexcept { return exists; }

	T& getValue() & { return getValueRef(); }
	T& getValue() const& { return getValueRef(); }
	T&& getValue() && { return getRValueRef(); }
	T&& getValue() const&& { return getRValueRef(); }

	T releaseValue()
	{
		if (!exists)
			throw BadOptionalAccess();
		T releasedValue = std::move(getValue());
		clear();
		return releasedValue;
	}

	T* operator->() { return getValuePtr(); }
	T* operator->() const { return getValuePtr(); }

	T& operator*() & { return getValue(); }
	T& operator*() const& { return getValue(); }
	T&& operator*() && { return getValue(); }
	T&& operator*() const&& { return getValue(); }

	template<typename U>
	T valueOr(U&& _default) const
	{
		return exists ? getValue() : static_cast<T>(std::forward<U>(_default));
	}

	template<typename U>
	T valueOr(U&& _default)
	{
		return exists ? getValue() : static_cast<T>(std::forward<U>(_default));
	}

	template<typename F>
	constexpr auto andThen(const F&& func) const
	{
		return hasValue() ? func(getValue()) : decltype(func(getValue())){};
	}

	template<typename F>
	constexpr auto transform(const F&& func) const
	{
		return hasValue() ? func(getValue()) : decltype(func(getValue())){};
	}

	template<typename F>
	constexpr Optional<T> orElse(const F&& func) const { return hasValue() ? *this : func(); }


	template<typename... Args>
	void emplace(Args&&... args)
	{
		if (exists)
		{
			clear();
			exists = false;
		}
		constructValue(std::forward<Args>(args)...);
		exists = true;
	}

	template<typename U, typename... Args>
	void emplace(std::initializer_list<U> list, Args&&... args)
	{
		if (exists)
		{
			clear();
			exists = false;
		}
		constructValue(list, std::forward<Args>(args)...);
		exists = true;
	}

	void swap(Optional& other) noexcept
	(
		std::is_nothrow_move_constructible<T>::value &&
		IsNothrowSwappable<T>::value
	)
	{
		if (exists == other.exists)
		{
			if (exists)
				std::swap(**this, *other);
			return;
		}
		else if (exists)
		{
			other.constructValue(std::move(getValue()));
			clear();
		}
		else
		{
			constructValue(std::move(other.getValue()));
			other.clear();
		}
		swap(exists, other.exists);
	}

	void reset()
	{
		if (exists)
		{
			clear();
			exists = false;
		}
	}

	template<typename U>
	constexpr bool operator==(const Optional<U>& other) const
	{
		return hasValue() == other.hasValue() && (!hasValue() || getValue() == other.getValue());
	}

	template<typename U>
	constexpr bool operator==(const U& other) const
	{
		return hasValue() && getValue() == other;
	}
private:
	template<typename... Args>
	void constructValue(Args&&... args) { new(&data) T(std::forward<Args>(args)...); }

	T* getValuePtr()
	{
		if (!exists)
			throw BadOptionalAccess();
		return reinterpret_cast<T*>(&data);
	}

	const T* getValuePtr() const
	{
		if (!exists)
			throw BadOptionalAccess();
		return reinterpret_cast<const T*>(&data);
	}

	T& getValueRef()
	{
		if (!exists)
			throw BadOptionalAccess();
		return reinterpret_cast<T&>(data);
	}

	const T& getValueRef() const
	{
		if (!exists)
			throw BadOptionalAccess();
		return reinterpret_cast<const T&>(data);
	}

	T&& getRValueRef()
	{
		if (!exists)
			throw BadOptionalAccess();
		return std::move(reinterpret_cast<T&>(data));
	}
};

};
#endif /* UTILS_OPTIONAL_H */
