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

#ifndef UTILS_CEREAL_OVERLOADS_H
#define UTILS_CEREAL_OVERLOADS_H 1

#include <cstdint>
#include <utility>

#include <cereal/cereal.hpp>
#include <cereal/details/traits.hpp>
#include <cereal_inline.hpp>

#include "comp_time_switch.h"
#include "enum_name.h"
#include "variant_name.h"
#include "utils.h"

template<class Archive>
struct save_visitor {
	Archive &archive;
	save_visitor(Archive &_archive) : archive(_archive) {}

	template<typename T, std::enable_if_t<!std::is_empty<T>::value, bool> = false>
	void operator()(T &data) { cereal::save_inline(archive, data); }
	template<typename T, std::enable_if_t<std::is_empty<T>::value, bool> = false>
	void operator()(T &data) {}
};

template<class Archive, typename Variant>
struct load_variant_cb {
	Archive &archive;
	Variant &variant;
	template<size_t I, size_t N>
	size_t operator()(size_t_<I>, size_t_<N>) {
		variant.template emplace<N>();
		cereal::load_inline(archive, std::get<N>(variant));
		return N;
	}
};

template<typename Variant, class Archive>
void load_variant(Archive &archive, Variant &variant, size_t target) {
	constexpr size_t variant_size = std::variant_size_v<Variant>;
	(void)constexpr_switch(target, std::make_index_sequence<variant_size>{}, load_variant_cb { archive, variant }, [] { return -1; });
}

namespace cereal {
	using namespace traits;

	template<class Archive, class T, class U>
	void serialize(Archive &archive, std::pair<T, U> &pair) {
		archive(cereal::make_size_tag(2));
		archive(pair.first, pair.second);
	}

	template<class Archive>
	std::string CEREAL_SAVE_MINIMAL_FUNCTION_NAME(const Archive &archive, const std::wstring &wstr) {
		return from_wstring<wchar_t>(wstr);
	}
	template<class Archive>
	void CEREAL_LOAD_MINIMAL_FUNCTION_NAME(const Archive &archive, std::wstring &wstr, const std::string &str) {
		wstr = to_wstring<wchar_t>(str);
	}
	template<class Archive, class T, std::enable_if_t<std::is_enum<T>::value, bool> = false>
	std::string save_minimal(Archive &archive, const T &type) {
		return std::string(enum_name(type));
	}
	
	template<class Archive, class T, std::enable_if_t<std::is_enum<T>::value, bool> = false>
	void load_minimal(const Archive &archive, T &type, const std::string &str) {
		type = enum_cast<T>(str).value();
	}

	template<class Archive, class T>
	std::basic_string<T> save_minimal(Archive &archive, const std::basic_string_view<T> &view) {
		return std::basic_string<T>(view.data(), view.size());
	}

	template<class Archive, class T>
	void load_minimal(Archive &archive, std::basic_string_view<T> &view, const std::basic_string<T> &str) {	
		view = str;
	}

	template<class Archive, typename... Types>
	void save(Archive &archive, const std::variant<Types...> &variant) {
		type_name_t type = to_variant_name(variant);
		archive(CEREAL_NVP(type));
		save_visitor<Archive> visitor(archive);
		std::visit(visitor, variant);
	}

	template<class Archive, typename... Types>
	void load(Archive &archive, std::variant<Types...> &variant) {
		type_name_t type;
		archive(CEREAL_NVP(type));
		load_variant(archive, variant, from_variant_name(variant, type));
	}
};

CEREAL_SPECIALIZE_FOR_ALL_ARCHIVES(std::wstring, cereal::specialization::non_member_load_save_minimal);

template<class Archive, class T>
void prologue(Archive &, T &) {}
template<class Archive, class T>
void epilogue(Archive &, T &) {}

#endif /* UTILS_CEREAL_OVERLOADS_H */
