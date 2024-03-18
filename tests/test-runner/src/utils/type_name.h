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

using type_name_t = std::string_view;
using typename_hash = shash32;
using typename_hash_t = typename_hash::value_type;

template<typename T>
constexpr type_name_t type_name() {
	return NAMEOF_SHORT_TYPE(T);
}

template<typename T>
constexpr type_name_t type_name(const T &) {
	return type_name<T>();
}

template<class T>
constexpr typename_hash_t type_name_hash() {
	return typename_hash(type_name<T>()).value();
}

template<class T>
constexpr typename_hash_t type_name_hash(const T &) {
	return type_name_hash<T>();
}

constexpr typename_hash_t name_hash(const type_name_t &name) {
	return typename_hash(name).value();
}

template<class... Args>
using type_names_hash_seq = std::integer_sequence<typename_hash_t, type_name_hash<Args>()...>;

template<class... Args>
constexpr auto make_type_names() {
	return std::array { type_name<Args>()... };
}

template<class... Args>
constexpr auto make_type_names(Args...) {
	return make_type_names<Args...>();
}

#endif /* UTILS_TYPE_NAME_H */
