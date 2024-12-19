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

#ifndef UTILS_TYPE_TRAITS_H
#define UTILS_TYPE_TRAITS_H 1

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <type_traits>

namespace lightspark
{

template<typename... Ts>
using CommonType = typename std::common_type<Ts...>::type;
template<typename T>
using ResultOf = typename std::result_of<T>::type;
template<bool B, typename T = void>
using EnableIf = typename std::enable_if<B, T>::type;
template<typename T>
using Decay = typename std::decay<T>::type;
template<typename T>
using RemoveCV = typename std::remove_cv<T>::type;
template<typename T>
using RemoveRef = typename std::remove_reference<T>::type;
template<typename T>
using RemoveCVRef = RemoveCV<RemoveRef<T>>;
template<bool B>
using BoolConstant = std::integral_constant<bool, B>;
template<bool B, typename T, typename F>
using CondT = typename std::conditional<B, T, F>::type;

template<typename T>
using IsVoid = std::is_void<T>;
template<typename T>
using IsRef = std::is_reference<T>;
template<typename T>
using IsArray = std::is_array<T>;

template<typename T>
using HasDtor = std::is_destructible<T>;
template<typename T>
using HasTrivialDtor = std::is_trivially_destructible<T>;
template<typename T>
using HasNothrowDtor = std::is_nothrow_destructible<T>;

template<typename T, typename... Args>
using HasCtor = std::is_constructible<T, Args...>;
template<typename T, typename... Args>
using HasTrivialCtor = std::is_trivially_constructible<T, Args...>;
template<typename T, typename... Args>
using HasNothrowCtor = std::is_nothrow_constructible<T, Args...>;

template<typename T>
using HasDefaultCtor = std::is_default_constructible<T>;
template<typename T>
using HasTrivialDefaultCtor = std::is_trivially_default_constructible<T>;
template<typename T>
using HasNothrowDefaultCtor = std::is_nothrow_default_constructible<T>;

template<typename T>
using HasCopyCtor = std::is_copy_constructible<T>;
template<typename T>
using HasTrivialCopyCtor = std::is_trivially_copy_constructible<T>;
template<typename T>
using HasNothrowCopyCtor = std::is_nothrow_copy_constructible<T>;

template<typename T>
using HasMoveCtor = std::is_move_constructible<T>;
template<typename T>
using HasTrivialMoveCtor = std::is_trivially_move_constructible<T>;
template<typename T>
using HasNothrowMoveCtor = std::is_nothrow_move_constructible<T>;

template<typename T, template<typename...> class U>
struct IsSpecializationOf : std::false_type {};

template<template<typename...> class T, typename... Args>
struct IsSpecializationOf<T<Args...>, T> : std::true_type {};

template<typename T>
struct Neg : BoolConstant<!T::value> {};

template<typename...>
struct Conj : std::true_type {};
template<typename T>
struct Conj<T> : T {};
template<typename T, typename... Args>
struct Conj<T, Args...> : CondT<T::value, Conj<Args...>, T>::type {};

template<typename...>
struct Disj : std::false_type {};
template<typename T>
struct Disj<T> : T {};
template<typename T, typename... Args>
struct Disj<T, Args...> : CondT<T::value, T, Disj<Args...>>::type {};

template<typename T, T I, T... Args>
struct StaticMin;

template<typename T, T I>
struct StaticMin<T, I>
{
	static constexpr T value = I;
};

template<typename T, T I, T J, T... Args>
struct StaticMin<T, I, J, Args...>
{
	static constexpr T value = StaticMin<T, (I <= J) ? I : J, Args...>::value;
};

template<typename T, T I, T... Args>
struct StaticMax;

template<typename T, T I>
struct StaticMax<T, I>
{
	static constexpr T value = I;
};

template<typename T, T I, T J, T... Args>
struct StaticMax<T, I, J, Args...>
{
	static constexpr T value = StaticMax<T, (I >= J) ? I : J, Args...>::value;
};

template<size_t... Args>
using StaticUMin = StaticMin<size_t, Args...>;
template<size_t... Args>
using StaticUMax = StaticMax<size_t, Args...>;

template<ssize_t... Args>
using StaticIMin = StaticMin<ssize_t, Args...>;
template<ssize_t... Args>
using StaticIMax = StaticMax<ssize_t, Args...>;

};
#endif /* UTILS_TYPE_TRAITS_H */
