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

#ifndef UTILS_COMP_TIME_SWITCH_H
#define UTILS_COMP_TIME_SWITCH_H 1

#include <cstdlib>
#include <utility>

template<typename T, T value>
struct TypeContainer {};

template<size_t I>
using Size = TypeContainer<size_t, I>;

template<size_t N, typename T, T... Is>
struct GetSequenceIndexImpl;
template<size_t N, typename T, T... Is>
struct ConstexprSwitchImpl;
template<typename T, T... Is>
struct CompileSwitchImpl;

template<size_t N, typename T, T I>
struct GetSequenceIndexImpl<N, T, I>
{
	static constexpr size_t call(const T& value, size_t fallback)
	{
		return (value == I) ? N : fallback;
	}
};

template<size_t N, typename T, T I, T... Is>
struct GetSequenceIndexImpl<N, T, I, Is...>
{
	static constexpr size_t call(const T& value, size_t fallback)
	{
		return (value == I) ? N : GetSequenceIndexImpl<N + 1, T, Is...>::call
		(
			value,
			fallback
		);
	}
};

template<size_t N, typename T, T I>
struct ConstexprSwitchImpl<N, T, I>
{
	template<typename F, typename F2>
	static constexpr T call(const T& value, F&& f, F2&& fallback)
	{
		return (value == I) ? f(Size<I>(), Size<N>()) : fallback();
	}
};

template<size_t N, typename T, T I, T... Is>
struct ConstexprSwitchImpl<N, T, I, Is...>
{
	template<typename F, typename F2>
	static constexpr T call(const T& value, F&& f, F2&& fallback)
	{
		return (value == I) ? f(Size<I>(), Size<N>()) : ConstexprSwitchImpl<N + 1, T, Is...>::call
		(
			value,
			std::forward<F>(f),
			std::forward<F2>(fallback)
		);
	}
};

template<typename T, T I>
struct CompileSwitchImpl<T, I>
{
	template<typename F, typename F2>
	static constexpr T call(const T& value, F&& f, F2&& fallback)
	{
		return (value == I) ? f(I) : fallback();
	}
};

template<typename T, T I, T... Is>
struct CompileSwitchImpl<T, I, Is...>
{
	template<typename F, typename F2>
	static constexpr T call(const T& value, F&& f, F2&& fallback)
	{
		return (value == I) ? f(I) : CompileSwitchImpl<T, Is...>::call
		(
			value,
			std::forward<F>(f),
			std::forward<F2>(fallback)
		);
	}
};


template<typename T, T... Is>
constexpr size_t getSequenceIndex(const T& value, size_t fallback, std::integer_sequence<T, Is...> seq)
{
	return GetSequenceIndexImpl<0, T, Is...>::call(value, fallback);
}

template<typename T, T I, typename F, typename F2>
constexpr T compileSwitchImpl(const T &value, std::integer_sequence<T, I>, F&& f, F2&& fallback)
{
	return (value == I) ? f(I) : fallback();
}

template<typename T, T I, T ... Is, typename F, typename F2>
constexpr T compileSwitchImpl(const T& value, std::integer_sequence<T, I, Is...>, F&& f, F2&& fallback)
{
	return (value == I) ? f(I) : compileSwitchImpl
	(
		value,
		std::integer_sequence<T, Is...>{},
		std::forward<F>(f),
		std::forward<F2>(fallback)
	);
}

template<typename T, T... Is>
constexpr size_t compileSwitchIdx(const T& i, size_t fallback, std::integer_sequence<T, Is...> seq)
{
	return getSequenceIndex<>(i, fallback, seq);
}

template<typename T, T... Is, typename F, typename F2>
constexpr T constexprSwitch(const T& value, std::integer_sequence<T, Is...>, F&& f, F2&& fallback)
{
	return ConstexprSwitchImpl<0, T, Is...>::call(value, std::forward<F>(f), std::forward<F2>(fallback));
}

template<typename T, T... Is, typename F, typename F2>
constexpr T compileSwitch(const T& value, std::integer_sequence<T, Is...> seq, F&& f, F2&& fallback)
{
	return compileSwitchImpl(value, seq, std::forward<F>(f), std::forward<F2>(fallback));
}

#endif /* UTILS_COMP_TIME_SWITCH_H */
