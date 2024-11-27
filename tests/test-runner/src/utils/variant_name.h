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

#ifndef UTILS_VARIANT_NAME_H
#define UTILS_VARIANT_NAME_H 1

#include <array>
#include <string>
#include <string_view>
#include <variant>

#include "comp_time_switch.h"
#include "type_name.h"

template<class... Args>
constexpr auto makeVariantNames(const std::variant<Args...>&)
{
	return makeTypeNames<Args...>();
}

template<class... Args>
constexpr size_t fromVariantName(const std::variant<Args...>&, const TypeName& name)
{
	return compileSwitchIdx(nameHash(name), -1, TypeNameHashSeq<Args...>{});
}

template<class... Args>
constexpr TypeName toVariantName(const std::variant<Args...>&, size_t target)
{
	constexpr auto names = makeTypeNames<Args...>();
	return target < names.size() ? names[target] : TypeName("invalid target");
}

template<typename Variant>
constexpr TypeName toVariantName(const Variant& variant)
{
	return toVariantName(variant, variant.index());
}

#endif /* UTILS_VARIANT_NAME_H */
