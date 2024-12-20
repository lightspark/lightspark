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

#ifndef INPUT_FORMATS_LIGHTSPARK_ENUM_H
#define INPUT_FORMATS_LIGHTSPARK_ENUM_H 1

#include <exception>
#include <list>
#include <map>

#include <lightspark/tiny_string.h>
#include <lightspark/utils/optional.h>

using namespace lightspark;

// TODO: Move this into the main codebase, at some point.

class LSEnumException : public std::exception
{
private:
	tiny_string reason;
public:
	LSEnumException(const tiny_string& _reason) : reason(_reason) {}

	const char* what() const noexcept override { return reason.raw_buf(); }
	const tiny_string& getReason() const noexcept { return reason; }
};

class LSEnum
{
private:
	std::map<tiny_string, ssize_t> values;
	Optional<ssize_t> defaultVal;
public:
	LSEnum
	(
		const std::map<tiny_string, ssize_t>& _values,
		Optional<ssize_t> _defaultVal = {}
	) : values(_values), defaultVal(_defaultVal) {}

	ssize_t getValue(const tiny_string& name) const
	{
		auto it = values.find(name);
		if (it != values.end())
			return it->second;
		return *defaultVal.orElse([&]
		{
			std::stringstream s;
			s << "getValue: `" << name << "` isn't a valid enum value, and there's no default.";
			throw LSEnumException(s.str());
			return nullOpt;
		});
	
	}

	template<typename T>
	T getValue(const tiny_string& name) const
	{
		auto it = values.find(name);
		if (it != values.end())
			return static_cast<T>(it->second);
		return static_cast<T>(*defaultVal.orElse([&]
		{
			std::stringstream s;
			s << "getValue: `" << name << "` isn't a valid enum value, and there's no default.";
			throw LSEnumException(s.str());
			return nullOpt;
		}));
	
	}

	Optional<ssize_t> tryGetValue(const tiny_string& name) const
	{
		auto it = values.find(name);
		if (it != values.end())
			return it->second;
		return {};
	}

	template<typename T>
	Optional<T> tryGetValue(const tiny_string& name) const
	{
		auto it = values.find(name);
		if (it != values.end())
			return static_cast<T>(it->second);
		return {};
	}

	Optional<ssize_t> tryGetValueStripPrefix(tiny_string& name) const
	{
		for (auto it : values)
		{
			if (name.startsWith(it.first))
			{
				name = name.stripPrefix(it.first);
				return it.second;
			}
		}
		return {};
	}

	template<typename T>
	Optional<T> tryGetValueStripPrefix(tiny_string& name) const
	{
		for (auto it : values)
		{
			if (name.startsWith(it.first))
			{
				name = name.stripPrefix(it.first);
				return static_cast<T>(it.second);
			}
		}
		return {};
	}
};

class LSShortEnum
{
using ShortEnumPair = std::pair<uint32_t, ssize_t>;
using ConstIter = std::list<ShortEnumPair>::const_iterator;
private:
	std::list<ShortEnumPair> values;
	Optional<ssize_t> defaultVal;

	ConstIter find(uint32_t ch) const
	{
		return std::find_if
		(
			values.cbegin(),
			values.cend(),
			[&](const ShortEnumPair& pair)
			{
				return pair.first == ch;
			}
		);
	}
public:
	LSShortEnum
	(
		const std::list<ShortEnumPair>& _values = {},
		Optional<ssize_t> _defaultVal = {}
	) : values(_values), defaultVal(_defaultVal) {}

	ssize_t getValue(uint32_t ch) const
	{
		auto it = find(ch);
		if (it != values.end())
			return it->second;
		return *defaultVal.orElse([&]
		{
			std::stringstream s;
			s << "getValue: `" << tiny_string::fromChar(ch) <<
			"` isn't a valid enum value, and there's no default.";
			throw LSEnumException(s.str());
			return nullOpt;
		});
	
	}

	template<typename T>
	T getValue(uint32_t ch) const
	{
		auto it = find(ch);
		if (it != values.end())
			return static_cast<T>(it->second);
		return static_cast<T>(*defaultVal.orElse([&]
		{
			std::stringstream s;
			s << "getValue: `" << tiny_string::fromChar(ch) <<
			"` isn't a valid enum value, and there's no default.";
			throw LSEnumException(s.str());
			return nullOpt;
		}));
	
	}

	Optional<ssize_t> tryGetValue(uint32_t ch) const
	{
		auto it = find(ch);
		if (it != values.end())
			return it->second;
		return {};
	}

	template<typename T>
	Optional<T> tryGetValue(uint32_t ch) const
	{
		auto it = find(ch);
		if (it != values.end())
			return static_cast<T>(it->second);
		return {};
	}
};

struct LSEnumPair
{
	LSShortEnum shortEnum;
	LSEnum _enum;
};

#endif /* INPUT_FORMATS_LIGHTSPARK_ENUM_H */
