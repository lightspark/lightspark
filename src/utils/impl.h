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

#ifndef UTILS_IMPL_H
#define UTILS_IMPL_H 1

#include "utils/any.h"
#include "utils/type_traits.h"

namespace lightspark
{

template<typename T>
class Impl
{
private:
	Any data;
	T& (*getter)(Any& data);
public:
	constexpr Impl() : getter(nullptr) {}
	template<typename U, EnableIf<std::is_base_of<T, U>::value, bool> = false>
	constexpr Impl(U&& obj) :
	data(std::forward<U>(obj)),
	getter([](Any& data) -> T& { return data.unsafeAs<T&>(); }) {}

	constexpr T& operator*() { return getter(data); }
	constexpr const T& operator*() const { return getter(data); }
	constexpr T* operator->() { return &getter(data); }
	constexpr const T* operator->() const { return &getter(data); }
	constexpr operator T&() { return getter(data); }
	constexpr operator const T&() { return getter(data); }

	constexpr bool hasValue() const { return getter != nullptr; }
};

};
#endif /* UTILS_IMPL_H */
