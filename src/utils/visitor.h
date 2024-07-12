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

#ifndef UTILS_VISITOR_H
#define UTILS_VISITOR_H 1

#include <utility>
#include "utils/type_list.h"
#include "utils/type_traits.h"

namespace lightspark
{

template<typename... Ts>
struct Visitor;

template<typename T>
struct Visitor<T> : T
{
	using T::operator();

	Visitor(const T&& a) : T(a) {}
};

template<typename T, typename... Ts>
struct Visitor<T, Ts...> : T, Visitor<Ts...>
{
	using T::operator();
	using Visitor<Ts...>::operator();

	Visitor(const T&& a, const Ts&&... args) : T(a), Visitor<Ts...>(std::forward<const Ts>(args)...)  {}
};

template<typename... Args>
Visitor<Args...> makeVisitor(const Args&&... args)
{
	return Visitor<Args...>(std::forward<const Args>(args)...);
}

template<typename V, typename T>
struct VisitorReturn;

template<typename V, typename... Ts>
struct VisitorReturn<V, TypeList<Ts...>>
{
	using type = CommonType<decltype(std::declval<V&>()(std::declval<Ts&>()))...>;
};

template<typename V, typename T>
using VisitorReturnType = typename VisitorReturn<V, T>::type;

};
#endif /* UTILS_VISITOR_H */
