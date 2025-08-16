/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2024-2025  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef UTILS_ENUM_H
#define UTILS_ENUM_H 1

#include "utils/type_traits.h"

namespace lightspark
{

template<typename T, EnableIf<IsEnum<T>::value, bool> = false>
static bool operator!(const T& a)
{
	return !bool(a);
}

template<typename T, EnableIf<IsEnum<T>::value, bool> = false>
static T operator~(const T& a)
{
	return static_cast<T>
	(
		~static_cast<UnderlyingType<T>>(a)
	);
}

template<typename T, EnableIf<IsEnum<T>::value, bool> = false>
static T operator&(const T& a, const T& b)
{
	return static_cast<T>
	(
		static_cast<UnderlyingType<T>>(a) &
		static_cast<UnderlyingType<T>>(b)
	);
}

template<typename T, EnableIf<IsEnum<T>::value, bool> = false>
static T operator|(const T& a, const T& b)
{
	return static_cast<T>
	(
		static_cast<UnderlyingType<T>>(a) |
		static_cast<UnderlyingType<T>>(b)
	);
}

template<typename T, EnableIf<IsEnum<T>::value, bool> = false>
static T operator^(const T& a, const T& b)
{
	return static_cast<T>
	(
		static_cast<UnderlyingType<T>>(a) ^
		static_cast<UnderlyingType<T>>(b)
	);
}

template<typename T, EnableIf<IsEnum<T>::value, bool> = false>
static T& operator&=(T& a, T b)
{
	return a = a & b;
}

template<typename T, EnableIf<IsEnum<T>::value, bool> = false>
static T& operator|=(T& a, T b)
{
	return a = a | b;
}

template<typename T, EnableIf<IsEnum<T>::value, bool> = false>
static T& operator^=(T& a, T b)
{
	return a = a ^ b;
}

};
#endif /* UTILS_ENUM_H */
