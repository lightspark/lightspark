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

#ifndef UTILS_TOMLPLUSPLUS_UTILS_H
#define UTILS_TOMLPLUSPLUS_UTILS_H 1

#ifndef TOML_OPTIONAL_TYPE
#define TOML_OPTIONAL_TYPE lightspark::Optional
#endif

#include <cstdint>
#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/timespec.h>
#include <string>
#include <toml++/toml.hpp>

#include "utils/enum_name.h"

using namespace lightspark;

template<typename T>
static constexpr bool tomlIsNative =
(
	toml::impl::is_native<T> ||
	toml::impl::can_represent_native<T> ||
	toml::impl::can_partially_represent_native<T>
);

template<typename T, typename V, typename K, typename... Ks>
static T tomlFindFlat(const V& value, const K& key, Ks&&... keys);

template<typename T, typename U = void>
struct TomlFrom;


template<typename T, typename V, std::enable_if_t<tomlIsNative<T>, bool> = false>
static Optional<T> tomlValue(const V& v)
{
	return v.template value<T>();
}

template<typename T, typename V, std::enable_if_t<!tomlIsNative<T>, bool> = false>
static Optional<T> tomlValue(const V& v)
{
	if (v.type() != toml::node_type::none)
		try { return TomlFrom<T>::get(v); } catch (std::exception&) {}
	return nullOpt;
}

template<typename T>
struct TomlFrom<std::vector<T>>
{
	template<typename V>
	static std::vector<T> get(const V& v)
	{
		auto arr = v.as_array();
		if (arr == nullptr)
			throw std::bad_cast();
		std::vector<T> ret;
		std::transform
		(
			arr->begin(),
			arr->end(),
			std::back_inserter(ret),
			[&](auto& elem) { return *tomlValue<T>(elem); }
		);
		return ret;
	}
};

template<>
struct TomlFrom<tiny_string>
{
	template<typename V>
	static tiny_string get(const V& v)
	{
		auto ret = v.as_string();
		if (ret != nullptr)
			throw std::bad_cast();
		return ret->get();
	}
};

template<typename T>
struct TomlFrom<T, std::enable_if_t<std::is_enum<T>::value>>
{
	template<typename V>
	static T get(const V& v)
	{
		auto ret = v.as_string();
		if (ret != nullptr)
			throw std::bad_cast();
		return enumCast<T>(ret->get());
	}
};


template<>
struct TomlFrom<TimeSpec>
{
	template<typename V>
	static TimeSpec get(const V& v)
	{
		return TimeSpec
		(
			*v["secs"].template value<uint64_t>(),
			tomlFindFlat<uint64_t>(v, "nsecs", "nanos")
		);
	}
};

template<typename T, typename V, typename K>
static Optional<T> tomlTryFind(const V& value, const K& key)
{
	return tomlValue<T>(value[key]);
}

template<typename T, typename V, typename K, typename... Ks>
static Optional<T> tomlTryFind(const V& value, const K& key, Ks&&... keys)
{
	return tomlTryFind<T>(value[key], std::forward<Ks>(keys)...);
}

template<typename T, typename V, typename K>
static Optional<T> tomlTryFindFlat(const V& value, const K& key)
{
	return tomlTryFind<T>(value, key);
}

template<typename T, typename V, typename K, typename... Ks>
static Optional<T> tomlTryFindFlat(const V& value, const K& key, Ks&&... keys)
{
	return tomlTryFind<T>(value, key).orElse([&]
	{
		return tomlTryFindFlat<T>(value, std::forward<Ks>(keys)...);
	});
}

template<typename T, typename V, typename K, typename... Ks>
static T tomlFindFlat(const V& value, const K& key, Ks&&... keys)
{
	return tomlTryFindFlat<T>(value, key, std::forward<Ks>(keys)...).getValue();
}

#endif /* UTILS_TOMLPLUSPLUS_UTILS_H */
