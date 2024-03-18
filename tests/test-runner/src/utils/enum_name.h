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

#ifndef UTILS_ENUM_NAME_H
#define UTILS_ENUM_NAME_H 1

#ifdef USE_MAGIC_ENUM
#include <magic_enum/magic_enum.hpp>

template<typename T>
using enum_name_t = decltype(magic_enum::enum_name<T>(T()));

template<typename T>
enum_name_t<T> enum_name(const T &value) {
	return magic_enum::enum_name<T>(value);
}

template<typename T, typename U>
T enum_cast(const U &value) {
	return magic_enum::enum_cast<T>(value);
}
#else
#include <enum_name.hpp>

template<typename T>
using enum_name_t = decltype(mgutility::enum_name<T>(T()));

template<typename T>
enum_name_t<T> enum_name(const T &value) {
	return mgutility::enum_name<T>(value);
}

template<typename T, typename U>
T enum_cast(const U &value) {
	return mgutility::to_enum<T>(value);
}
#endif

#endif /* UTILS_ENUM_NAME_H */
