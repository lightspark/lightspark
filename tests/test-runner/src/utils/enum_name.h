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
using EnumName = decltype(magic_enum::enum_name<T>(T()));
#else
#include <enum_name.hpp>
template<typename T>
using EnumName = decltype(mgutility::enum_name<T>(T()));
#endif

template<typename T>
EnumName<T> enumName(const T& value)
{
	#ifdef USE_MAGIC_ENUM
	return magic_enum::enum_name<T>(value);
	#else
	return mgutility::enum_name<T>(value);
	#endif
}

template<typename T, typename U>
T enumCast(const U &value)
{
	#ifdef USE_MAGIC_ENUM
	return magic_enum::enum_cast<T>(value);
	#else
	return *mgutility::to_enum<T>(value);
	#endif
}

#endif /* UTILS_ENUM_NAME_H */
