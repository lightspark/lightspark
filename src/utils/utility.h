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

#ifndef UTILS_UTILITY_H
#define UTILS_UTILITY_H 1

#include <type_traits>

namespace lightspark
{

struct Unused {};

template<typename T>
struct IsSwappable : public std::integral_constant<bool, !std::is_same<decltype(std::swap
(
	std::declval<T&>(),
	std::declval<T&>()
)), Unused>::value> {};

template<typename T, bool>
struct IsNothrowSwappableImpl
{
	constexpr static bool value = noexcept(swap(std::declval<T&>(), std::declval<T&>()));
};

template<typename T>
struct IsNothrowSwappableImpl<T, false> : std::false_type {};

template<typename T>
struct IsNothrowSwappable : public IsNothrowSwappableImpl<T, IsSwappable<T>::value> {};

};
#endif /* UTILS_UTILITY_H */
