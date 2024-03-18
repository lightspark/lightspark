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

#ifndef UTILS_TRAITS_H
#define UTILS_TRAITS_H 1

#include <string>
#include <type_traits>

template<typename T>
struct is_char {
	using char_type = typename std::char_traits<T>::char_type;
	static constexpr bool value = std::is_integral<T>::value && std::is_same<T, char_type>::value;
};

template<typename T>
struct is_wchar {
	static constexpr bool value = is_char<T>::value && !std::is_same<T, char>::value;
};

#endif /* UTILS_TRAITS_H */
