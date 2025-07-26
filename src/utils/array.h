/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef UTILS_ARRAY_H
#define UTILS_ARRAY_H 1

#include <array>
#include "utils/type_traits.h"

namespace lightspark
{

template<typename... Args>
constexpr std::array<CommonType<Args...>, sizeof...(Args)> makeArray(Args&&... args)
{
	return std::array<CommonType<Args...>, sizeof...(Args)> { arg, args... };
}

template<typename T, typename... Args>
constexpr std::array<T, sizeof...(Args)> makeArray(Args&&... args)
{
	return makeArray(std::forward<Args>(args)...);
}

};
#endif /* UTILS_ARRAY_H */
