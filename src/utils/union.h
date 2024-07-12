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

#ifndef UTILS_UNION_H
#define UTILS_UNION_H 1

#include <stdlib.h>
#include "utils/type_list.h"
#include "utils/type_traits.h"

namespace lightspark
{

template<typename T>
struct UnionSize;
template<typename... Args>
struct UnionSize<TypeList<Args...>>
{
	static constexpr size_t value = StaticMax<sizeof(Args)...>::value;
};

template<typename T>
struct UnionAlign;
template<typename... Args>
struct UnionAlign<TypeList<Args...>>
{
	static constexpr size_t value = StaticMax<alignof(Args)...>::value;
};

};
#endif /* UTILS_UNION_H */


