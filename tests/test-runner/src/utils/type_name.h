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

#ifndef UTILS_TYPE_NAME_H
#define UTILS_TYPE_NAME_H 1

#include <array>
#include <string>
#include <string_view>
#include <utility>

#include <nameof.hpp>
#include <string_hash.hpp>

using TypeName = std::string_view;
using TypeNameHash = shash32;
using TypeNameHashValue = TypeNameHash::value_type;

template<typename T>
constexpr TypeName typeName()
{
	return NAMEOF_SHORT_TYPE(T);
}

template<typename T>
constexpr TypeName typeName(const T&)
{
	return typeName<T>();
}

template<typename T>
constexpr TypeNameHashValue typeNameHash()
{
	return TypeNameHash(typeName<T>()).value();
}

template<typename T>
constexpr TypeNameHashValue typeNameHash(const T &)
{
	return typeNameHash<T>();
}

constexpr TypeNameHashValue nameHash(const TypeName& name)
{
	return TypeNameHash(name).value();
}

template<typename... Args>
using TypeNameHashSeq = std::integer_sequence<TypeNameHashValue, typeNameHash<Args>()...>;

template<typename... Args>
constexpr auto makeTypeNames()
{
	return std::array { typeName<Args>()... };
}

template<typename... Args>
constexpr auto makeTypeNames(Args...)
{
	return makeTypeNames<Args...>();
}

#endif /* UTILS_TYPE_NAME_H */
