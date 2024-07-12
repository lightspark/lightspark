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

#include <stdlib.h>
#include <type_traits>

namespace lightspark
{

template<typename... Ts>
using CommonType = typename std::common_type<Ts...>::type;
template<typename T>
using ResultOf = typename std::result_of<T>::type;

template<size_t I, size_t... Args>
struct StaticMin;

template<size_t I>
struct StaticMin<I>
{
	static constexpr size_t value = I;
};

template<size_t I, size_t J, size_t... Args>
struct StaticMin<I, J, Args...>
{
	static constexpr size_t value = StaticMin<(I <= J) ? I : J, Args...>::value;
};

template<size_t I, size_t... Args>
struct StaticMax;

template<size_t I>
struct StaticMax<I>
{
	static constexpr size_t value = I;
};

template<size_t I, size_t J, size_t... Args>
struct StaticMax<I, J, Args...>
{
	static constexpr size_t value = StaticMax<(I >= J) ? I : J, Args...>::value;
};

};
#endif /* UTILS_TYPE_TRAITS_H */
