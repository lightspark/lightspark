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
constexpr auto make_variant_names(const std::variant<Args...> &) {
	return make_type_names<Args...>();
}

template<class... Args>
constexpr size_t from_variant_name(const std::variant<Args...> &, const type_name_t &name) {
	return compile_switch_idx(name_hash(name), -1, type_names_hash_seq<Args...>{});
}

template<class... Args>
constexpr type_name_t to_variant_name(const std::variant<Args...> &, size_t target) {
	constexpr auto names = make_type_names<Args...>();
	return (target < names.size()) ? names[target] : type_name_t("invalid target");
}

template<typename Variant>
constexpr type_name_t to_variant_name(const Variant &variant) {
	return to_variant_name(variant, variant.index());
}

#endif /* UTILS_VARIANT_NAME_H */
